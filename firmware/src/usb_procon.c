// Descritores USB do Pro Controller, handshake 0x80, emulacao de subcomandos e ponte BT->USB.
// Tambem expoe um CDC de diagnostico como driver de stdio (printf -> porta serial).
#include "usb_procon.h"

#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/stdio/driver.h"
#include "pico/critical_section.h"
#include "pico/bootrom.h"
#include "tusb.h"

#include "procon.h"

// 1 = repassa os comandos de luz do Home do PC ao controle (Steam/Eden); 0 = o dongle controla a luz sozinho.
#ifndef PROCON_HOME_PASSTHROUGH
#define PROCON_HOME_PASSTHROUGH 0
#endif

// =========================================================================================
// Descritores (copia fiel do Pro Controller original; ver docs/)
// =========================================================================================
#define USB_VID 0x057E
#define USB_PID 0x2009

// Interfaces: 0 = Pro Controller (HID), 1 = teclado HID so para acordar o PC (se PROCON_WAKE), depois o CDC de log.
#define ITF_HID 0
#define ITF_KBD 1
#if PROCON_WAKE
#define ITF_CDC (ITF_KBD + 1)
#else
#define ITF_CDC ITF_KBD
#endif
#define EP_HID_OUT 0x01
#define EP_HID_IN  0x81
#define EP_CDC_NOTIF 0x82
#define EP_CDC_OUT   0x03
#define EP_CDC_IN    0x83
#define EP_KBD_IN    0x84

static const uint8_t hid_report_desc[203] = {
    0x05,0x01,0x15,0x00,0x09,0x04,0xA1,0x01,0x85,0x30,0x05,0x01,0x05,0x09,0x19,0x01,
    0x29,0x0A,0x15,0x00,0x25,0x01,0x75,0x01,0x95,0x0A,0x55,0x00,0x65,0x00,0x81,0x02,
    0x05,0x09,0x19,0x0B,0x29,0x0E,0x15,0x00,0x25,0x01,0x75,0x01,0x95,0x04,0x81,0x02,
    0x75,0x01,0x95,0x02,0x81,0x03,0x0B,0x01,0x00,0x01,0x00,0xA1,0x00,0x0B,0x30,0x00,
    0x01,0x00,0x0B,0x31,0x00,0x01,0x00,0x0B,0x32,0x00,0x01,0x00,0x0B,0x35,0x00,0x01,
    0x00,0x15,0x00,0x27,0xFF,0xFF,0x00,0x00,0x75,0x10,0x95,0x04,0x81,0x02,0xC0,0x0B,
    0x39,0x00,0x01,0x00,0x15,0x00,0x25,0x07,0x35,0x00,0x46,0x3B,0x01,0x65,0x14,0x75,
    0x04,0x95,0x01,0x81,0x02,0x05,0x09,0x19,0x0F,0x29,0x12,0x15,0x00,0x25,0x01,0x75,
    0x01,0x95,0x04,0x81,0x02,0x75,0x08,0x95,0x34,0x81,0x03,0x06,0x00,0xFF,0x85,0x21,
    0x09,0x01,0x75,0x08,0x95,0x3F,0x81,0x03,0x85,0x81,0x09,0x02,0x75,0x08,0x95,0x3F,
    0x81,0x03,0x85,0x01,0x09,0x03,0x75,0x08,0x95,0x3F,0x91,0x83,0x85,0x10,0x09,0x04,
    0x75,0x08,0x95,0x3F,0x91,0x83,0x85,0x80,0x09,0x05,0x75,0x08,0x95,0x3F,0x91,0x83,
    0x85,0x82,0x09,0x06,0x75,0x08,0x95,0x3F,0x91,0x83,0xC0};

#if PROCON_WAKE
// Teclado minimo (boot): usado so para mandar F15 ao acordar o PC. O Windows so oferece "permitir que este
// dispositivo ative o computador" para dispositivos com aparencia de teclado/mouse.
static const uint8_t kbd_report_desc[] = {TUD_HID_REPORT_DESC_KEYBOARD()};
#endif

#if PROCON_DEBUG_CDC
static const tusb_desc_device_t desc_device = {
    .bLength = sizeof(tusb_desc_device_t), .bDescriptorType = TUSB_DESC_DEVICE, .bcdUSB = 0x0200,
    .bDeviceClass = TUSB_CLASS_MISC, .bDeviceSubClass = MISC_SUBCLASS_COMMON, .bDeviceProtocol = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0 = 64, .idVendor = USB_VID, .idProduct = USB_PID, .bcdDevice = 0x0210,
    .iManufacturer = 1, .iProduct = 2, .iSerialNumber = 3, .bNumConfigurations = 1};
#else
static const tusb_desc_device_t desc_device = {
    .bLength = sizeof(tusb_desc_device_t), .bDescriptorType = TUSB_DESC_DEVICE, .bcdUSB = 0x0200,
    .bDeviceClass = 0, .bDeviceSubClass = 0, .bDeviceProtocol = 0,
    .bMaxPacketSize0 = 64, .idVendor = USB_VID, .idProduct = USB_PID, .bcdDevice = 0x0210,
    .iManufacturer = 1, .iProduct = 2, .iSerialNumber = 3, .bNumConfigurations = 1};
#endif

#define ITF_COUNT (1 + PROCON_WAKE + (PROCON_DEBUG_CDC ? 2 : 0))
#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_HID_INOUT_DESC_LEN + (PROCON_WAKE ? TUD_HID_DESC_LEN : 0) +                           (PROCON_DEBUG_CDC ? TUD_CDC_DESC_LEN : 0))
static const uint8_t desc_config[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_COUNT, 0, CONFIG_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 500),
    TUD_HID_INOUT_DESCRIPTOR(ITF_HID, 0, HID_ITF_PROTOCOL_NONE, sizeof(hid_report_desc), EP_HID_OUT, EP_HID_IN, 64, 8),
#if PROCON_WAKE
    TUD_HID_DESCRIPTOR(ITF_KBD, 0, HID_ITF_PROTOCOL_KEYBOARD, sizeof(kbd_report_desc), EP_KBD_IN, 8, 10),
#endif
#if PROCON_DEBUG_CDC
    TUD_CDC_DESCRIPTOR(ITF_CDC, 4, EP_CDC_NOTIF, 8, EP_CDC_OUT, EP_CDC_IN, 64),
#endif
};

static const char *const desc_strings[] = {
    "",                    // 0: idioma (tratado a parte)
    "Nintendo Co., Ltd.",  // 1
    "Pro Controller",      // 2
    "000000000001",        // 3
    "Dongle log",          // 4 (CDC)
};

uint8_t const *tud_descriptor_device_cb(void) { return (uint8_t const *)&desc_device; }
uint8_t const *tud_descriptor_configuration_cb(uint8_t index) { (void)index; return desc_config; }
uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance) {
#if PROCON_WAKE
    if (instance == 1) return kbd_report_desc;
#endif
    (void)instance;
    return hid_report_desc;
}

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void)langid;
    static uint16_t buf[40];
    uint8_t n;
    if (index == 0) {
        buf[1] = 0x0409; n = 1;
    } else {
        if (index >= sizeof(desc_strings) / sizeof(desc_strings[0])) return NULL;
        const char *s = desc_strings[index];
        n = (uint8_t)strlen(s);
        if (n > 38) n = 38;
        for (uint8_t i = 0; i < n; i++) buf[1 + i] = s[i];
    }
    buf[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * n + 2));
    return buf;
}

// =========================================================================================
// Estado compartilhado BT -> USB
// =========================================================================================
static volatile bool wake_req;      // BT -> USB: um botao/conexao do controle pede para acordar o PC suspenso
static critical_section_t state_cs;
static uint8_t latest[64];          // formato USB do 0x30: [0]=0x30 [1]=timer ... [12]=vib, IMU 13..48
static volatile bool fresh;

// ---- estatisticas de latencia (diagnostico; impressas a cada 5 s no build debug) ------------
#define LAT_BINS 48                         // 250 us por bin (0..12 ms); o ultimo acumula o excedente
#define GAP_BINS 64                         // 1 ms por bin (0..63 ms)
static uint16_t hist_fw[LAT_BINS];          // chegada BT -> tud_hid_report() (fila do firmware)
static uint16_t hist_usb[LAT_BINS];         // chegada BT -> transferencia USB concluida (inclui a espera do poll do host)
static uint16_t hist_gap[GAP_BINS];         // intervalo entre relatorios BT consecutivos
static uint32_t latest_ts_us, inflight_ts_us, last_push_us;
static uint32_t st_in, st_sent, st_dropped;

static void lat_report(void);

static void lat_add(uint16_t *h, unsigned bins, uint32_t value, uint32_t step) {
    uint32_t b = value / step;
    h[b >= bins ? bins - 1 : b]++;
}

void usb_procon_push_input(const uint8_t *r, uint16_t len) {
    if (len < 50) return;
    uint32_t now = time_us_32();
    critical_section_enter_blocking(&state_cs);
    memcpy(latest, &r[1], 49);      // pula o byte 0xA1 do cabecalho Bluetooth
    memset(&latest[49], 0, sizeof(latest) - 49);
    if (fresh) st_dropped++;        // o relatorio anterior nem chegou a ser enviado: foi sobrescrito
    if (last_push_us) lat_add(hist_gap, GAP_BINS, now - last_push_us, 1000);
    last_push_us = now;
    latest_ts_us = now;
    st_in++;
    fresh = true;
    critical_section_exit(&state_cs);
#if PROCON_WAKE
    if ((r[4] | r[5] | r[6]) && tud_suspended()) wake_req = true;   // qualquer botao com o PC suspenso
#endif
}

// =========================================================================================
// Protocolo USB do Pro Controller
// =========================================================================================
#define TXQ 16
static uint8_t txq[TXQ][64];
static unsigned txq_head, txq_tail;
static bool stream_on;              // host pediu modo 0x30
static uint8_t last_rumble[8];

static void queue_packet(const uint8_t *p64) {
    unsigned next = (txq_head + 1) % TXQ;
    if (next == txq_tail) { printf("USB: fila de respostas cheia\n"); return; }
    memcpy(txq[txq_head], p64, 64);
    txq_head = next;
}

static void reply_subcmd(uint8_t ack, uint8_t sub, const uint8_t *data, uint8_t dlen) {
    uint8_t p[64] = {0};
    critical_section_enter_blocking(&state_cs);
    memcpy(&p[1], &latest[1], 12);   // timer, bateria, botoes, sticks, vibrador
    critical_section_exit(&state_cs);
    p[0] = 0x21;
    p[13] = ack;
    p[14] = sub;
    if (dlen > 49) dlen = 49;
    if (dlen) memcpy(&p[15], data, dlen);
    queue_packet(p);
}

static void handle_subcommand(uint8_t sub, const uint8_t *a, uint16_t alen) {
    switch (sub) {
        case 0x02: {                                  // informacoes do dispositivo
            uint8_t info[12];
            procon_get_dev_info(info);
            reply_subcmd(0x82, sub, info, sizeof(info));
            break;
        }
        case 0x03:                                    // modo de relatorio
            if (alen >= 1 && a[0] == 0x30) { stream_on = true; printf("USB: modo 0x30 ligado\n"); }
            reply_subcmd(0x80, sub, NULL, 0);
            break;
        case 0x10: {                                  // leitura da SPI
            if (alen < 5) { reply_subcmd(0x80, sub, NULL, 0); break; }
            uint32_t addr = a[0] | (a[1] << 8) | (a[2] << 16) | ((uint32_t)a[3] << 24);
            uint8_t n = a[4] > 0x1D ? 0x1D : a[4];
            uint8_t d[5 + 0x1D];
            memcpy(d, a, 4); d[4] = n;
            procon_spi_read(addr, n, &d[5]);
            reply_subcmd(0x90, sub, d, 5 + n);
            break;
        }
        case 0x30:                                    // LED de jogador: repassa ao controle real
            if (alen >= 1) procon_forward_player_lights(a[0]);
            reply_subcmd(0x80, sub, NULL, 0);
            break;
        case 0x38:                                    // luz do botao Home
#if PROCON_HOME_PASSTHROUGH
            if (alen >= 1) procon_forward_home_light(a, alen > 37 ? 37 : (uint8_t)alen);
#endif                                                // padrao: o dongle e o dono da luz (indicador de conexao/bateria)
            reply_subcmd(0x80, sub, NULL, 0);
            break;
        case 0x04: {                                  // tempos dos gatilhos
            uint8_t z[14] = {0};
            reply_subcmd(0x83, sub, z, sizeof(z));
            break;
        }
        default:                                      // 0x08, 0x38, 0x40, 0x41, 0x48, 0x22... : so reconhece
            reply_subcmd(0x80, sub, NULL, 0);
            break;
    }
}

static void handle_usb_cmd(uint8_t cmd) {            // relatorio de saida 0x80 -> resposta 0x81
    uint8_t p[64] = {0};
    p[0] = 0x81; p[1] = cmd;
    switch (cmd) {
        case 0x01: {                                  // status da conexao: tipo + MAC
            uint8_t info[12];
            procon_get_dev_info(info);
            p[2] = 0x00; p[3] = 0x03;
            memcpy(&p[4], &info[4], 6);
            queue_packet(p);
            break;
        }
        case 0x02: case 0x03:                         // handshake / baudrate
            queue_packet(p);
            break;
        case 0x04:                                    // forca modo USB sem timeout: sem resposta;
            stream_on = true;                         // o controle real passa a enviar 0x30 por padrao via USB
            break;
        case 0x05: break;                             // volta a permitir timeout: sem resposta
        default: break;
    }
    static uint32_t n04;
    if (cmd != 0x04 || (n04++ % 50) == 0) printf("USB: cmd 0x80 0x%02x%s\n", cmd, cmd == 0x04 ? " (repetido)" : "");
}

static void forward_rumble(const uint8_t *r8) {
    if (memcmp(r8, last_rumble, 8) == 0) return;
    memcpy(last_rumble, r8, 8);
    static unsigned nlog;
    if (nlog < 60) {
        nlog++;
        printf("USB rumble: %02x %02x %02x %02x  %02x %02x %02x %02x\n",
               r8[0], r8[1], r8[2], r8[3], r8[4], r8[5], r8[6], r8[7]);
    }
    procon_forward_rumble(r8);
}

static uint32_t out_count[256];     // pacotes de saida por report ID (diagnostico)

static void handle_output(const uint8_t *d, uint16_t len) {
    if (len < 2) return;
    out_count[d[0]]++;
    static unsigned logged;
    if (d[0] != 0x10 && !(d[0] == 0x80 && d[1] == 0x04) && logged < 40) {   // diagnostico: primeiros pacotes do host
        logged++;
        printf("USB OUT [%u]:", len);
        for (uint16_t i = 0; i < len && i < 20; i++) printf(" %02x", d[i]);
        printf("\n");
    }
    switch (d[0]) {
        case 0x80: handle_usb_cmd(d[1]); break;
        case 0x10: if (len >= 10) forward_rumble(&d[2]); break;
        case 0x01:
            if (len >= 11) {
                forward_rumble(&d[2]);
                if (procon_verbose()) printf("USB: subcmd 0x%02x arg %02x\n", d[10], d[11]);
                handle_subcommand(d[10], &d[11], len - 11);
            }
            break;
        default: break;
    }
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type,
                           uint8_t const *buffer, uint16_t bufsize) {
    (void)report_type;
    if (instance != 0) return;        // teclado de wake: ignora LEDs (Num Lock etc.)
    if (report_id != 0) {             // SET_REPORT pelo endpoint de controle: o ID veio separado
        uint8_t tmp[64];
        if (bufsize > 63) bufsize = 63;
        tmp[0] = report_id;
        memcpy(&tmp[1], buffer, bufsize);
        handle_output(tmp, bufsize + 1);
    } else {                          // endpoint OUT: o primeiro byte e o ID
        handle_output(buffer, bufsize);
    }
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type,
                               uint8_t *buffer, uint16_t reqlen) {
    (void)instance; (void)report_id; (void)report_type; (void)buffer; (void)reqlen;
    return 0;
}

void tud_mount_cb(void) { printf("USB: montado pelo host\n"); }
void tud_umount_cb(void) { stream_on = false; txq_head = txq_tail = 0; printf("USB: desmontado\n"); }
// --- acordar o PC (USB remote wakeup) ----------------------------------------------------
static bool wake_sent, kbd_pending;
static uint8_t kbd_state;           // 0 ocioso, 1 enviar F15 pressionado, 2 enviar solto

void usb_procon_request_wake(void) {
#if PROCON_WAKE
    if (tud_suspended()) wake_req = true;
#endif
}

void tud_suspend_cb(bool remote_wakeup_en) {
    printf("USB: suspenso (wake permitido pelo host: %s)\n", remote_wakeup_en ? "sim" : "NAO");
}
void tud_resume_cb(void) {
    printf("USB: retomado\n");
    if (wake_sent) { wake_sent = false; kbd_pending = true; }
}

// =========================================================================================
// CDC de diagnostico como driver de stdio
// =========================================================================================
#if PROCON_DEBUG_CDC
#define LOG_RING 4096
static uint8_t log_ring[LOG_RING];
static unsigned log_head, log_tail;
static critical_section_t log_cs;
static bool log_ready;
static volatile bool reboot_to_bootsel;
static uint8_t rx_ring[64];
static unsigned rx_head, rx_tail;

static void cdc_out_chars(const char *buf, int len) {
    if (!log_ready) return;
    critical_section_enter_blocking(&log_cs);
    for (int i = 0; i < len; i++) {
        unsigned next = (log_head + 1) % LOG_RING;
        if (next == log_tail) break;      // cheio: descarta o excedente
        log_ring[log_head] = (uint8_t)buf[i];
        log_head = next;
    }
    critical_section_exit(&log_cs);
}

static int cdc_in_chars(char *buf, int len) {
    int n = 0;
    while (n < len && rx_tail != rx_head) { buf[n++] = (char)rx_ring[rx_tail]; rx_tail = (rx_tail + 1) % sizeof(rx_ring); }
    return n ? n : PICO_ERROR_NO_DATA;
}

static stdio_driver_t cdc_driver = {.out_chars = cdc_out_chars, .in_chars = cdc_in_chars};

static void cdc_pump(void) {
    if (!tud_cdc_connected()) {           // sem terminal aberto: descarta
        critical_section_enter_blocking(&log_cs);
        log_tail = log_head;
        critical_section_exit(&log_cs);
        return;
    }
    uint32_t room = tud_cdc_write_available();
    while (room) {
        uint8_t chunk[64];
        unsigned n = 0;
        critical_section_enter_blocking(&log_cs);
        while (n < sizeof(chunk) && n < room && log_tail != log_head) {
            chunk[n++] = log_ring[log_tail];
            log_tail = (log_tail + 1) % LOG_RING;
        }
        critical_section_exit(&log_cs);
        if (!n) break;
        tud_cdc_write(chunk, n);
        room -= n;
    }
    tud_cdc_write_flush();
    while (tud_cdc_available()) {
        uint8_t c;
        if (tud_cdc_read(&c, 1) != 1) break;
        unsigned next = (rx_head + 1) % sizeof(rx_ring);
        if (next != rx_tail) { rx_ring[rx_head] = c; rx_head = next; }
    }
}

// abrir a porta a 1200 baud reinicia o Pico em modo BOOTSEL (gravacao sem apertar o botao)
void tud_cdc_line_coding_cb(uint8_t itf, cdc_line_coding_t const *p_line_coding) {
    (void)itf;
    if (p_line_coding->bit_rate == 1200) reboot_to_bootsel = true;   // executado no loop principal
}
#endif

// =========================================================================================
// Inicializacao e loop
// =========================================================================================
void usb_procon_init(void) {
    critical_section_init(&state_cs);
    memset(latest, 0, sizeof(latest));
    latest[0] = 0x30;
#if PROCON_DEBUG_CDC
    critical_section_init(&log_cs);
    log_ready = true;
    stdio_set_driver_enabled(&cdc_driver, true);
#endif
    tusb_init();
}

static void report_out_stats(void) {
    static uint32_t last_ms, prev[256];
    uint32_t now = to_ms_since_boot(get_absolute_time());
    if (now - last_ms < 2000) return;
    last_ms = now;
    bool any = false;
    for (int i = 0; i < 256; i++) {
        if (i == 0x80) continue;                  // 0x80 0x04 de keep-alive nao interessa
        if (out_count[i] != prev[i]) {
            if (!any) { printf("USB OUT/2s:"); any = true; }
            printf(" id%02x=%u", i, (unsigned)(out_count[i] - prev[i]));
            prev[i] = out_count[i];
        }
    }
    if (any) printf("\n");
}

void usb_procon_task(void) {
    tud_task();
    report_out_stats();
    lat_report();
#if PROCON_WAKE
    if (wake_req) {
        wake_req = false;
        if (tud_suspended()) {
            bool ok = tud_remote_wakeup();
            printf("USB: pedido de wake %s\n", ok ? "enviado" : "recusado (host nao permite)");
            wake_sent = ok;
        }
    }
    if (kbd_pending && tud_hid_n_ready(1)) {                  // depois de retomar: F15 acende a tela
        static const uint8_t f15[6] = {0x6A, 0, 0, 0, 0, 0};
        static const uint8_t none[6] = {0};
        if (kbd_state == 0) { tud_hid_n_keyboard_report(1, 0, 0, f15); kbd_state = 1; }
        else if (kbd_state == 1) { tud_hid_n_keyboard_report(1, 0, 0, none); kbd_state = 0; kbd_pending = false; }
    }
#endif
#if PROCON_DEBUG_CDC
    cdc_pump();
    if (reboot_to_bootsel) {
        tud_disconnect();            // solta o barramento antes de reiniciar
        sleep_ms(100);
        reset_usb_boot(0, 0);
    }
#endif
    if (!tud_hid_ready()) return;
    if (txq_tail != txq_head) {                       // respostas de comando tem prioridade
        tud_hid_report(0, txq[txq_tail], 64);
        txq_tail = (txq_tail + 1) % TXQ;
        return;
    }
    if (stream_on && fresh) {
        uint8_t p[64];
        uint32_t ts;
        critical_section_enter_blocking(&state_cs);
        memcpy(p, latest, 64);
        ts = latest_ts_us;
        fresh = false;
        critical_section_exit(&state_cs);
        tud_hid_report(0, p, 64);
        lat_add(hist_fw, LAT_BINS, time_us_32() - ts, 250);
        inflight_ts_us = ts;
        st_sent++;
    }
}

// transferencia IN concluida = o host leu o relatorio (depois do poll do endpoint)
void tud_hid_report_complete_cb(uint8_t instance, uint8_t const *report, uint16_t len) {
    if (instance == 0 && len == 64 && report[0] == 0x30 && inflight_ts_us) {
        lat_add(hist_usb, LAT_BINS, time_us_32() - inflight_ts_us, 250);
        inflight_ts_us = 0;
    }
}

// percentil p (0..100) de um histograma, devolvido em unidades de bin (limite superior do bin)
static unsigned hist_pct(const uint16_t *h, unsigned bins, unsigned total, unsigned p) {
    unsigned need = (total * p + 99) / 100, acc = 0;
    for (unsigned i = 0; i < bins; i++) { acc += h[i]; if (acc >= need) return i + 1; }
    return bins;
}
static unsigned hist_max(const uint16_t *h, unsigned bins) {
    for (unsigned i = bins; i > 0; i--) if (h[i - 1]) return i;
    return 0;
}

static void lat_report(void) {
    static uint32_t last_ms;
    uint32_t now = to_ms_since_boot(get_absolute_time());
    if (now - last_ms < 5000) return;
    last_ms = now;
    uint16_t fw[LAT_BINS], usb[LAT_BINS], gap[GAP_BINS];
    uint32_t in, sent, drop;
    critical_section_enter_blocking(&state_cs);
    memcpy(fw, hist_fw, sizeof(fw)); memcpy(usb, hist_usb, sizeof(usb)); memcpy(gap, hist_gap, sizeof(gap));
    in = st_in; sent = st_sent; drop = st_dropped;
    memset(hist_fw, 0, sizeof(hist_fw)); memset(hist_usb, 0, sizeof(hist_usb)); memset(hist_gap, 0, sizeof(hist_gap));
    st_in = st_sent = st_dropped = 0;
    critical_section_exit(&state_cs);
    if (!procon_verbose() || in < 10) return;
    unsigned nfw = 0, nusb = 0, ngap = 0, over25 = 0;
    for (unsigned i = 0; i < LAT_BINS; i++) { nfw += fw[i]; nusb += usb[i]; }
    for (unsigned i = 0; i < GAP_BINS; i++) { ngap += gap[i]; if (i >= 25) over25 += gap[i]; }
    printf("LAT 5s: bt=%u usb=%u descartados=%u | fila fw p50<%uus p95<%uus max<%uus | ate o host p50<%uus p95<%uus max<%uus | "
           "gap BT p50<%ums p95<%ums max<%ums >=25ms:%u\n",
           (unsigned)in, (unsigned)sent, (unsigned)drop,
           hist_pct(fw, LAT_BINS, nfw, 50) * 250, hist_pct(fw, LAT_BINS, nfw, 95) * 250, hist_max(fw, LAT_BINS) * 250,
           hist_pct(usb, LAT_BINS, nusb, 50) * 250, hist_pct(usb, LAT_BINS, nusb, 95) * 250, hist_max(usb, LAT_BINS) * 250,
           hist_pct(gap, GAP_BINS, ngap, 50), hist_pct(gap, GAP_BINS, ngap, 95), hist_max(gap, GAP_BINS), over25);
}
