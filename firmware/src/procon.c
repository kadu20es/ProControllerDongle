#include "procon.h"

#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/critical_section.h"
#include "btstack.h"

#define RID_SUBCMD   0x01
#define RID_RUMBLE   0x10
#define OUT_LEN      48        // relatorio 0x01: contador + rumble(8) + subcmd + args, 48 bytes
#define STEP_TIMEOUT_MS 400
#define MAX_RETRIES  8
#define SPI_CHUNK    29        // maximo por leitura (0x1D)

// ---- imagem da SPI flash do controle ----------------------------------------------------
#define SPI60_ADDR 0x6000
#define SPI60_SIZE 0x100
#define SPI80_ADDR 0x8000
#define SPI80_SIZE 0x40
static uint8_t spi60[SPI60_SIZE], spi80[SPI80_SIZE];
static bool spi_loaded;

#define NSPI (((SPI60_SIZE + SPI_CHUNK - 1) / SPI_CHUNK) + ((SPI80_SIZE + SPI_CHUNK - 1) / SPI_CHUNK))

static void chunk_for(unsigned i, uint16_t *addr, uint8_t *len) {
    unsigned n60 = (SPI60_SIZE + SPI_CHUNK - 1) / SPI_CHUNK;
    uint32_t base, size, off;
    if (i < n60) { base = SPI60_ADDR; size = SPI60_SIZE; off = i * SPI_CHUNK; }
    else         { base = SPI80_ADDR; size = SPI80_SIZE; off = (i - n60) * SPI_CHUNK; }
    uint32_t l = size - off;
    if (l > SPI_CHUNK) l = SPI_CHUNK;
    *addr = (uint16_t)(base + off);
    *len = (uint8_t)l;
}

static void spi_store(uint16_t addr, const uint8_t *d, uint8_t n) {
    for (uint8_t i = 0; i < n; i++) {
        uint32_t a = addr + i;
        if (a >= SPI60_ADDR && a < SPI60_ADDR + SPI60_SIZE) spi60[a - SPI60_ADDR] = d[i];
        else if (a >= SPI80_ADDR && a < SPI80_ADDR + SPI80_SIZE) spi80[a - SPI80_ADDR] = d[i];
    }
}

void procon_spi_read(uint32_t addr, uint8_t len, uint8_t *out) {
    for (uint8_t i = 0; i < len; i++) {
        uint32_t a = addr + i;
        if (a >= SPI60_ADDR && a < SPI60_ADDR + SPI60_SIZE) out[i] = spi60[a - SPI60_ADDR];
        else if (a >= SPI80_ADDR && a < SPI80_ADDR + SPI80_SIZE) out[i] = spi80[a - SPI80_ADDR];
        else out[i] = 0xFF;
    }
}

// passos de configuracao apos a leitura da SPI
typedef struct { uint8_t sub; uint8_t n; uint8_t a[2]; } cfg_step_t;
static const cfg_step_t cfg[] = {
    {0x40, 1, {0x01}},   // habilita IMU
    {0x48, 1, {0x01}},   // habilita vibracao
    {0x03, 1, {0x30}},   // modo de relatorio completo
    {0x30, 1, {0x01}},   // LED de jogador 1
};
#define NCFG (sizeof(cfg) / sizeof(cfg[0]))
#define TOTAL_STEPS (1 + NSPI + NCFG)   // 0 = info, 1..NSPI = SPI, resto = cfg

// ---- estado ----------------------------------------------------------------------------
static procon_send_fn send_out;
static procon_input_sink_fn input_sink;
#ifndef PROCON_VERBOSE_DEFAULT
#define PROCON_VERBOSE_DEFAULT 1
#endif
static bool connected, done, verbose = PROCON_VERBOSE_DEFAULT;
static uint8_t last30[64];
static bool have_last30;
static unsigned step, retries;
static uint8_t counter;
static btstack_timer_source_t step_timer, rumble_timer;
static uint32_t n_total, n_total_last, n_30, n_30_last, last_rate_ms, last_30_print_ms;
static uint8_t dev_info[12] = {0x03, 0x49, 0x03, 0x02, 0, 0, 0, 0, 0, 0, 0x01, 0x01};

typedef struct { int cx, cy, minx, maxx, miny, maxy; } stick_cal_t;
static stick_cal_t cal_l = {2048, 2048, 500, 3600, 500, 3600};
static stick_cal_t cal_r = {2048, 2048, 500, 3600, 500, 3600};

static void log_hex(const char *tag, const uint8_t *d, unsigned len) {
    printf("%s [%u]:", tag, len);
    for (unsigned i = 0; i < len; i++) printf(" %02x", d[i]);
    printf("\n");
}

// ---- envio -----------------------------------------------------------------------------
static void fill_neutral_rumble(uint8_t *p) {
    static const uint8_t n[4] = {0x00, 0x01, 0x40, 0x40};
    memcpy(p, n, 4);
    memcpy(p + 4, n, 4);
}

static void send_subcommand(uint8_t sub, const uint8_t *args, uint8_t n) {
    uint8_t buf[OUT_LEN];
    memset(buf, 0, sizeof(buf));
    buf[0] = counter++ & 0x0F;
    fill_neutral_rumble(&buf[1]);
    buf[9] = sub;
    if (n) memcpy(&buf[10], args, n);
    uint8_t st = send_out(RID_SUBCMD, buf, sizeof(buf));
    if (st) printf("TX subcmd 0x%02x falhou: 0x%02x\n", sub, st);
}

static uint8_t step_subcmd(unsigned s) {
    if (s == 0) return 0x02;
    if (s <= NSPI) return 0x10;
    return cfg[s - 1 - NSPI].sub;
}

static void step_timeout(btstack_timer_source_t *ts);

static void send_step(void) {
    uint8_t args[8] = {0};
    uint8_t n = 0;
    if (step > 0 && step <= NSPI) {
        uint16_t addr; uint8_t len;
        chunk_for(step - 1, &addr, &len);
        args[0] = addr & 0xFF; args[1] = addr >> 8; args[2] = 0; args[3] = 0; args[4] = len;
        n = 5;
    } else if (step > NSPI) {
        const cfg_step_t *c = &cfg[step - 1 - NSPI];
        memcpy(args, c->a, c->n);
        n = c->n;
    }
    send_subcommand(step_subcmd(step), args, n);
    btstack_run_loop_set_timer_handler(&step_timer, step_timeout);
    btstack_run_loop_set_timer(&step_timer, STEP_TIMEOUT_MS);
    btstack_run_loop_add_timer(&step_timer);
}

static void step_timeout(btstack_timer_source_t *ts) {
    (void)ts;
    if (!connected || done) return;
    if (++retries > MAX_RETRIES) {
        printf("Setup abortado no passo %u (sem resposta ao subcomando 0x%02x)\n", step, step_subcmd(step));
        return;
    }
    printf("Timeout passo %u, reenviando (%u)\n", step, retries);
    send_step();
}

// ---- calibracao ------------------------------------------------------------------------
static void decode6(const uint8_t *b, uint16_t d[6]) {
    d[0] = ((b[1] << 8) & 0xF00) | b[0];
    d[1] = (b[2] << 4) | (b[1] >> 4);
    d[2] = ((b[4] << 8) & 0xF00) | b[3];
    d[3] = (b[5] << 4) | (b[4] >> 4);
    d[4] = ((b[7] << 8) & 0xF00) | b[6];
    d[5] = (b[8] << 4) | (b[7] >> 4);
}

static void make_cal_left(stick_cal_t *c, const uint8_t *b) {
    uint16_t d[6]; decode6(b, d);
    c->cx = d[2]; c->cy = d[3];
    c->maxx = c->cx + d[0]; c->maxy = c->cy + d[1];
    c->minx = c->cx - d[4]; c->miny = c->cy - d[5];
}

static void make_cal_right(stick_cal_t *c, const uint8_t *b) {
    uint16_t d[6]; decode6(b, d);
    c->cx = d[0]; c->cy = d[1];
    c->minx = c->cx - d[2]; c->miny = c->cy - d[3];
    c->maxx = c->cx + d[4]; c->maxy = c->cy + d[5];
}

static void compute_calibration(void) {
    make_cal_left(&cal_l, &spi60[0x3D]);
    make_cal_right(&cal_r, &spi60[0x46]);
    printf("Calibracao de fabrica dos sticks aplicada\n");
    const uint8_t *u = spi80;
    if (u[0x10] == 0xB2 && u[0x11] == 0xA1) { make_cal_left(&cal_l, &u[0x12]); printf("Calibracao de usuario: stick esquerdo\n"); }
    if (u[0x1B] == 0xB2 && u[0x1C] == 0xA1) { make_cal_right(&cal_r, &u[0x1D]); printf("Calibracao de usuario: stick direito\n"); }
    printf("Cal L: c(%d,%d) x[%d..%d] y[%d..%d]\n", cal_l.cx, cal_l.cy, cal_l.minx, cal_l.maxx, cal_l.miny, cal_l.maxy);
    printf("Cal R: c(%d,%d) x[%d..%d] y[%d..%d]\n", cal_r.cx, cal_r.cy, cal_r.minx, cal_r.maxx, cal_r.miny, cal_r.maxy);
}

static int norm(int v, int c, int mn, int mx) {
    int d = v - c;
    int span = d >= 0 ? mx - c : c - mn;
    if (span <= 0) return 0;
    int p = d * 100 / span;
    return p > 100 ? 100 : (p < -100 ? -100 : p);
}

// ---- recepcao --------------------------------------------------------------------------
static void on_setup_done(void) {
    done = true;
    spi_loaded = true;
    btstack_run_loop_remove_timer(&step_timer);
    compute_calibration();
    printf("Setup completo: modo 0x30, IMU e vibracao ativos\n");
    procon_dump();
}

static void handle_reply(const uint8_t *r, uint16_t len) {
    if (len < 22) return;
    uint8_t ack = r[14], sub = r[15];
    if (done || sub != step_subcmd(step)) {
        if (!done) printf("Resposta inesperada: sub 0x%02x (esperava 0x%02x)\n", sub, step_subcmd(step));
        return;
    }
    if (!(ack & 0x80)) printf("NACK subcmd 0x%02x (ack 0x%02x)\n", sub, ack);

    if (sub == 0x02) {
        memcpy(dev_info, &r[16], sizeof(dev_info));
        printf("Info: firmware %u.%02x, tipo %u, MAC %02x:%02x:%02x:%02x:%02x:%02x\n",
               r[16], r[17], r[18], r[20], r[21], r[22], r[23], r[24], r[25]);
    } else if (sub == 0x10 && (ack & 0x80)) {
        uint16_t addr = r[16] | (r[17] << 8);
        uint8_t n = r[20];
        if (n > SPI_CHUNK) n = SPI_CHUNK;
        spi_store(addr, &r[21], n);
    } else if (sub == 0x03 || sub == 0x30 || sub == 0x40 || sub == 0x48) {
        if (verbose) printf("subcmd 0x%02x ack 0x%02x\n", sub, ack);
    }

    step++;
    retries = 0;
    btstack_run_loop_remove_timer(&step_timer);
    if (step >= TOTAL_STEPS) on_setup_done();
    else send_step();
}

static void log_button_change(const uint8_t *r) {
    static uint32_t prev = 0xFFFFFFFF;
    uint32_t cur = (r[4] << 16) | (r[5] << 8) | r[6];
    if (cur == prev) return;
    prev = cur;
    static const char *right[8]  = {"Y", "X", "B", "A", "SR_dir", "SL_dir", "R", "ZR"};
    static const char *shared[8] = {"Minus", "Plus", "R3", "L3", "Home", "Capture", "-", "-"};
    static const char *left[8]   = {"Baixo", "Cima", "Direita", "Esquerda", "SR_esq", "SL_esq", "L", "ZL"};
    printf("BTN %02x %02x %02x:", r[4], r[5], r[6]);
    for (int i = 0; i < 8; i++) if (r[4] & (1 << i)) printf(" %s", right[i]);
    for (int i = 0; i < 8; i++) if (r[5] & (1 << i)) printf(" %s", shared[i]);
    for (int i = 0; i < 8; i++) if (r[6] & (1 << i)) printf(" %s", left[i]);
    printf("\n");
}

static void decode_30(const uint8_t *r, uint16_t len) {
    if (len < 50) return;
    memcpy(last30, r, 50);
    have_last30 = true;
    if (input_sink) input_sink(r, len);
    if (!verbose) return;
    log_button_change(r);
    uint32_t now = to_ms_since_boot(get_absolute_time());
    if (now - last_30_print_ms < 500) return;
    last_30_print_ms = now;

    int lx = r[7] | ((r[8] & 0x0F) << 8), ly = (r[8] >> 4) | (r[9] << 4);
    int rx = r[10] | ((r[11] & 0x0F) << 8), ry = (r[11] >> 4) | (r[12] << 4);
    int16_t acc[3], gyr[3];
    for (int i = 0; i < 3; i++) {
        acc[i] = (int16_t)(r[14 + 2 * i] | (r[15 + 2 * i] << 8));
        gyr[i] = (int16_t)(r[20 + 2 * i] | (r[21 + 2 * i] << 8));
    }
    printf("0x30 t=%02x bat=%x btn=%02x%02x%02x L(%d,%d)%% R(%d,%d)%% acc(%d,%d,%d) gyr(%d,%d,%d)\n",
           r[2], r[3] >> 4, r[4], r[5], r[6],
           norm(lx, cal_l.cx, cal_l.minx, cal_l.maxx), norm(ly, cal_l.cy, cal_l.miny, cal_l.maxy),
           norm(rx, cal_r.cx, cal_r.minx, cal_r.maxx), norm(ry, cal_r.cy, cal_r.miny, cal_r.maxy),
           acc[0], acc[1], acc[2], gyr[0], gyr[1], gyr[2]);
}

// ---- repasse PC -> controle (vibracao / LED), seguro entre threads ---------------------------
static critical_section_t fwd_cs;
static bool fwd_inited;
static uint8_t fwd_rumble[8];
static bool fwd_rumble_dirty, fwd_lights_dirty, fwd_pending;
static uint8_t fwd_lights;
static btstack_context_callback_registration_t fwd_reg;

static void fwd_pump(void *ctx) {
    (void)ctx;
    uint8_t rumble[8], lights = 0;
    bool do_r, do_l;
    critical_section_enter_blocking(&fwd_cs);
    do_r = fwd_rumble_dirty; do_l = fwd_lights_dirty;
    memcpy(rumble, fwd_rumble, 8); lights = fwd_lights;
    fwd_rumble_dirty = fwd_lights_dirty = false;
    fwd_pending = false;
    critical_section_exit(&fwd_cs);

    if (!done || !connected) { printf("FWD descartado (done=%d connected=%d)\n", done, connected); return; }
    if (do_l) send_subcommand(0x30, &lights, 1);
    if (do_r) {
        uint8_t b[9];
        b[0] = counter++ & 0x0F;
        memcpy(&b[1], rumble, 8);
        uint8_t st = send_out(RID_RUMBLE, b, sizeof(b));
        static unsigned nlog;
        if (nlog < 60) { nlog++; printf("FWD rumble -> BT status 0x%02x\n", st); }
    }
}

static void fwd_post(void) {
    // chamado com fwd_cs fora; agenda o pump no contexto do BTstack
    bool post = false;
    critical_section_enter_blocking(&fwd_cs);
    if (!fwd_pending) { fwd_pending = true; post = true; }
    critical_section_exit(&fwd_cs);
    if (post) {
        fwd_reg.callback = fwd_pump;
        fwd_reg.context = NULL;
        btstack_run_loop_execute_on_main_thread(&fwd_reg);
    }
}

void procon_forward_rumble(const uint8_t rumble8[8]) {
    if (!fwd_inited) return;
    critical_section_enter_blocking(&fwd_cs);
    memcpy(fwd_rumble, rumble8, 8);
    fwd_rumble_dirty = true;
    critical_section_exit(&fwd_cs);
    fwd_post();
}

void procon_forward_player_lights(uint8_t bits) {
    if (!fwd_inited) return;
    critical_section_enter_blocking(&fwd_cs);
    fwd_lights = bits;
    fwd_lights_dirty = true;
    critical_section_exit(&fwd_cs);
    fwd_post();
}

// ---- API -------------------------------------------------------------------------------
void procon_init(procon_send_fn send) {
    send_out = send;
    memset(spi60, 0xFF, sizeof(spi60));
    memset(spi80, 0xFF, sizeof(spi80));
    critical_section_init(&fwd_cs);
    fwd_inited = true;
}

void procon_set_input_sink(procon_input_sink_fn fn) { input_sink = fn; }
void procon_set_verbose(bool v) { verbose = v; }
bool procon_verbose(void) { return verbose; }
bool procon_ready(void) { return connected && done; }

void procon_get_dev_info(uint8_t out[12]) { memcpy(out, dev_info, sizeof(dev_info)); }

static void start_setup(btstack_timer_source_t *ts) {
    (void)ts;
    if (connected) send_step();
}

void procon_on_connected(void) {
    connected = true; done = false; step = 0; retries = 0; counter = 0;
    n_total = n_total_last = n_30 = n_30_last = 0;
    last_rate_ms = last_30_print_ms = 0;
    // espera 1,5 s para o controle terminar de abrir os canais e enviar o descritor
    btstack_run_loop_remove_timer(&step_timer);
    btstack_run_loop_set_timer_handler(&step_timer, start_setup);
    btstack_run_loop_set_timer(&step_timer, 1500);
    btstack_run_loop_add_timer(&step_timer);
}

// Relatorio "solto": botoes em zero e sticks no centro calibrado. Evita botao preso no PC quando o controle some.
static void push_neutral_report(void) {
    if (!have_last30 || !input_sink) return;
    uint8_t r[64];
    memcpy(r, last30, 50);
    r[4] = r[5] = r[6] = 0;
    int lx = cal_l.cx, ly = cal_l.cy, rx = cal_r.cx, ry = cal_r.cy;
    r[7]  = lx & 0xFF; r[8]  = ((lx >> 8) & 0x0F) | ((ly & 0x0F) << 4); r[9]  = ly >> 4;
    r[10] = rx & 0xFF; r[11] = ((rx >> 8) & 0x0F) | ((ry & 0x0F) << 4); r[12] = ry >> 4;
    memset(&r[13], 0, 37);                    // vibrador e IMU zerados
    input_sink(r, 50);
}

void procon_on_disconnected(void) {
    if (done) push_neutral_report();
    have_last30 = false;
    connected = false; done = false;
    btstack_run_loop_remove_timer(&step_timer);
    btstack_run_loop_remove_timer(&rumble_timer);
}

void procon_on_report(const uint8_t *r, uint16_t len) {
    if (len < 2) return;
    n_total++;
    switch (r[1]) {
        case 0x30: n_30++; decode_30(r, len); break;
        case 0x21: handle_reply(r, len); break;
        case 0x3F: break;                 // simples: ignorado
        default: if (verbose) log_hex("RX", r, len > 24 ? 24 : len); break;
    }
}

void procon_tick(uint32_t now_ms) {
    if (!connected || !verbose) return;
    if (now_ms - last_rate_ms >= 1000) {
        printf("rate: total %u/s, 0x30 %u/s\n", (unsigned)(n_total - n_total_last), (unsigned)(n_30 - n_30_last));
        n_total_last = n_total; n_30_last = n_30; last_rate_ms = now_ms;
    }
}

static void rumble_off(btstack_timer_source_t *ts) {
    (void)ts;
    if (!connected) return;
    uint8_t b[9];
    b[0] = counter++ & 0x0F;
    fill_neutral_rumble(&b[1]);
    send_out(RID_RUMBLE, b, sizeof(b));
}

void procon_rumble_pulse(void) {
    if (!procon_ready()) { printf("rumble: controle nao esta pronto\n"); return; }
    // 160 Hz, amplitude ~0.55 nos dois lados (seguro; nunca usar amplitude > ~1.0)
    static const uint8_t on[4] = {0x80, 0x90, 0x40, 0x64};
    uint8_t b[9];
    b[0] = counter++ & 0x0F;
    memcpy(&b[1], on, 4);
    memcpy(&b[5], on, 4);
    uint8_t st = send_out(RID_RUMBLE, b, sizeof(b));
    printf("rumble on: 0x%02x\n", st);
    btstack_run_loop_remove_timer(&rumble_timer);
    btstack_run_loop_set_timer_handler(&rumble_timer, rumble_off);
    btstack_run_loop_set_timer(&rumble_timer, 250);
    btstack_run_loop_add_timer(&rumble_timer);
}

void procon_dump(void) {
    if (!spi_loaded) { printf("SPI ainda nao carregada\n"); return; }
    printf("Serial: ");
    for (int i = 0; i < 16; i++) printf("%02x", spi60[i]);
    printf("\nCores corpo #%02x%02x%02x botoes #%02x%02x%02x (flag 0x601B=%02x)\n",
           spi60[0x50], spi60[0x51], spi60[0x52], spi60[0x53], spi60[0x54], spi60[0x55], spi60[0x1B]);
    log_hex("cal IMU 0x6020", &spi60[0x20], 24);
    log_hex("cal sticks 0x603D", &spi60[0x3D], 18);
    log_hex("params 0x6080", &spi60[0x80], 24);
    log_hex("usuario 0x8010", &spi80[0x10], 22);
    log_hex("usuario IMU 0x8026", &spi80[0x26], 26);
    log_hex("info 0x02", dev_info, 12);
}
