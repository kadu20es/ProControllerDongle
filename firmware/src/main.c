// Dongle Pro Controller: pareamento/reconexao Bluetooth, LED de estado e botao BOOTSEL.
// Protocolo do controle em procon.c, lado USB em usb_procon.c.
// Log (build debug): porta serial CDC.
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/gpio.h"
#include "hardware/sync.h"
#include "hardware/structs/ioqspi.h"
#include "hardware/structs/sio.h"
#include "btstack.h"
#include "procon.h"
#include "usb_procon.h"

#define INQUIRY_DURATION_1280MS 24      // ~30 s de janela de pareamento
#define COD_MAJOR_MASK          0x1F00
#define COD_MAJOR_PERIPHERAL    0x0500
#define COD_MINOR_MASK          0x00FC
#define COD_MINOR_GAMEPAD       0x0008

static uint8_t hid_descriptor_storage[512];
static btstack_packet_callback_registration_t hci_cb;
static btstack_timer_source_t tick_timer;

static uint16_t hid_cid;
static bool connected;
static bool scanning;
static bd_addr_t pending_addr;
static bool have_pending;

static void log_hex(const char *tag, const uint8_t *d, uint16_t len) {
    printf("%s [%u]:", tag, len);
    for (uint16_t i = 0; i < len && i < 256; i++) printf(" %02x", d[i]);
    printf("\n");
}

static uint8_t hid_send(uint8_t report_id, const uint8_t *data, uint16_t len) {
    if (!hid_cid) return ERROR_CODE_COMMAND_DISALLOWED;
    return hid_host_send_report(hid_cid, report_id, data, len);
}

static btstack_timer_source_t connect_timer;
static bd_addr_t bonded_addr;
static bool bonded;
static bool connecting;
static btstack_timer_source_t retry_timer;
static void retry_step(btstack_timer_source_t *ts);

static void enter_retry_timer(void) {
    btstack_run_loop_remove_timer(&retry_timer);
    btstack_run_loop_set_timer_handler(&retry_timer, retry_step);
    btstack_run_loop_set_timer(&retry_timer, 4000);
    btstack_run_loop_add_timer(&retry_timer);
}

static void connect_step(btstack_timer_source_t *ts) {
    (void)ts;
    memcpy(bonded_addr, pending_addr, sizeof(bd_addr_t));
    bonded = true;
    connecting = true;
    uint8_t r = hid_host_connect(pending_addr, HID_PROTOCOL_MODE_REPORT, &hid_cid);
    printf("hid_host_connect: 0x%02x\n", r);
    if (r) connecting = false;
    // se esta conexao falhar, o retry_timer insiste a cada 4 s
    enter_retry_timer();
}

static bool lookup_bond(void) {
    btstack_link_key_iterator_t it;
    link_key_t key;
    link_key_type_t type;
    bool found = false;
    if (!gap_link_key_iterator_init(&it)) return false;
    if (gap_link_key_iterator_get_next(&it, bonded_addr, key, &type)) found = true;
    gap_link_key_iterator_done(&it);
    return found;
}

static void start_scan(void) {
    if (connected || scanning) return;
    int st = gap_inquiry_start(INQUIRY_DURATION_1280MS);
    printf("Inquiry start: %d (segure Sync no controle)\n", st);
    scanning = (st == 0);
}

// Com bond guardado: tenta conectar ativamente a cada 4 s enquanto desconectado.
// (Tambem fica connectable, entao uma conexao iniciada pelo controle e aceita.)
static void retry_step(btstack_timer_source_t *ts) {
    if (bonded && !connected && !connecting && !have_pending) {
        connecting = true;
        uint8_t r = hid_host_connect(bonded_addr, HID_PROTOCOL_MODE_REPORT, &hid_cid);
        printf("Reconectar a %s: hid_host_connect 0x%02x\n", bd_addr_to_str(bonded_addr), r);
        if (r) connecting = false;
    }
    btstack_run_loop_set_timer(ts, 4000);
    btstack_run_loop_add_timer(ts);
}

static void enter_reconnect_or_scan(void) {
    bonded = lookup_bond();
    if (bonded) {
        printf("Bond encontrado: %s. Aguardando controle (aperte um botao). 'p' = esquecer e parear de novo\n",
               bd_addr_to_str(bonded_addr));
        enter_retry_timer();
    } else {
        start_scan();
    }
}

// Le o botao BOOTSEL (ligado ao CS da flash). Executa da RAM com interrupcoes desligadas.
static bool __no_inline_not_in_flash_func(bootsel_pressed)(void) {
    const uint cs_index = 1;
    uint32_t flags = save_and_disable_interrupts();
    hw_write_masked(&ioqspi_hw->io[cs_index].ctrl,
                    GPIO_OVERRIDE_LOW << IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_LSB,
                    IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_BITS);
    for (volatile int i = 0; i < 1000; ++i) { }
    bool pressed = !(sio_hw->gpio_hi_in & SIO_GPIO_HI_IN_QSPI_CSN_BITS);   // apertado = nivel baixo
    hw_write_masked(&ioqspi_hw->io[cs_index].ctrl,
                    GPIO_OVERRIDE_NORMAL << IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_LSB,
                    IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_BITS);
    restore_interrupts(flags);
    return pressed;
}

// Esquece o controle atual e abre a busca por um novo (botao BOOTSEL 2 s ou comando 'p').
static void start_pairing(void) {
    printf("Esquecendo bonds e iniciando pareamento\n");
    gap_delete_all_link_keys();
    bonded = false;
    btstack_run_loop_remove_timer(&retry_timer);
    if (scanning) { gap_inquiry_stop(); scanning = false; }
    if (connected && hid_cid) {
        hid_host_disconnect(hid_cid);   // a busca comeca quando o HID fechar (HID_SUBEVENT_CONNECTION_CLOSED)
        return;
    }
    start_scan();
}

static void poll_serial(void) {
    int c = getchar_timeout_us(0);
    if (c == PICO_ERROR_TIMEOUT) return;
    if (c == 'p') {
        start_pairing();
    } else if (c == 'v') {
        procon_set_verbose(!procon_verbose());
        printf("verbose=%d\n", procon_verbose());
    } else if (c == 'r') {
        procon_rumble_pulse();
    } else if (c == 'd') {
        procon_dump();
    } else if (c == 's') {
        printf("estado: connected=%d connecting=%d scanning=%d bonded=%d\n", connected, connecting, scanning, bonded);
    }
}

// Tick de 100 ms: LED de estado, botao BOOTSEL (2 s = parear) e comandos seriais.
//   conectado        -> LED aceso
//   pareando/busca   -> pisca rapido (5 Hz)
//   aguardando       -> pulso curto a cada 2 s (controle ja pareado, esperando um botao)
static void tick(btstack_timer_source_t *ts) {
    static uint32_t n, hold;
    n++;
    poll_serial();
    uint32_t now = to_ms_since_boot(get_absolute_time());
    bool led;
    if (connected) {
        led = true;
        procon_tick(now);
    } else if (scanning || !bonded) {
        led = (n & 1);
    } else {
        led = ((n % 20) == 0);
    }
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led);

    if (bootsel_pressed()) {
        if (hold < 1000) hold++;
        if (hold == 20) start_pairing();       // 20 x 100 ms = 2 s
    } else {
        hold = 0;
    }
    btstack_run_loop_set_timer(ts, 100);
    btstack_run_loop_add_timer(ts);
}

static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    (void)channel; (void)size;
    if (packet_type != HCI_EVENT_PACKET) return;
    bd_addr_t addr;
    switch (hci_event_packet_get_type(packet)) {
        case BTSTACK_EVENT_STATE:
            if (btstack_event_state_get_state(packet) == HCI_STATE_WORKING) {
                bd_addr_t local; gap_local_bd_addr(local);
                printf("Bluetooth pronto. Endereco local: %s\n", bd_addr_to_str(local));
                enter_reconnect_or_scan();
            }
            break;

        case GAP_EVENT_INQUIRY_RESULT: {
            gap_event_inquiry_result_get_bd_addr(packet, addr);
            uint32_t cod = gap_event_inquiry_result_get_class_of_device(packet);
            char name[32] = "?";
            if (gap_event_inquiry_result_get_name_available(packet)) {
                uint8_t n = gap_event_inquiry_result_get_name_len(packet);
                if (n > sizeof(name) - 1) n = sizeof(name) - 1;
                memcpy(name, gap_event_inquiry_result_get_name(packet), n);
                name[n] = 0;
            }
            printf("Inquiry: %s CoD 0x%06" PRIx32 " nome '%s'\n", bd_addr_to_str(addr), cod, name);
            if ((cod & COD_MAJOR_MASK) == COD_MAJOR_PERIPHERAL && (cod & COD_MINOR_MASK) == COD_MINOR_GAMEPAD) {
                printf("Gamepad encontrado, pareando...\n");
                memcpy(pending_addr, addr, sizeof(bd_addr_t));
                have_pending = true;
                gap_inquiry_stop();
            }
            break;
        }

        case GAP_EVENT_INQUIRY_COMPLETE:
            scanning = false;
            if (have_pending) {
                have_pending = false;
                // pareamento dedicado antes de abrir os canais L2CAP
                int st = gap_dedicated_bonding(pending_addr, 0);
                printf("gap_dedicated_bonding: %d\n", st);
            } else if (!connected && !bonded) {
                printf("Inquiry terminou sem gamepad; reiniciando busca\n");
                start_scan();
            }
            break;

        case GAP_EVENT_DEDICATED_BONDING_COMPLETED: {
            uint8_t st = gap_event_dedicated_bonding_completed_get_status(packet);
            printf("Bonding concluido: status 0x%02x\n", st);
            if (st == ERROR_CODE_SUCCESS) {
                // o bonding dedicado derruba o ACL; espera 1 s antes de abrir o HID
                btstack_run_loop_set_timer_handler(&connect_timer, connect_step);
                btstack_run_loop_set_timer(&connect_timer, 1000);
                btstack_run_loop_add_timer(&connect_timer);
            } else {
                start_scan();
            }
            break;
        }

        case HCI_EVENT_CONNECTION_REQUEST:
            hci_event_connection_request_get_bd_addr(packet, addr);
            printf("HCI connection request de %s CoD 0x%06" PRIx32 " tipo %u\n", bd_addr_to_str(addr),
                   hci_event_connection_request_get_class_of_device(packet),
                   hci_event_connection_request_get_link_type(packet));
            break;
        case HCI_EVENT_CONNECTION_COMPLETE:
            printf("HCI connection complete: status 0x%02x\n", hci_event_connection_complete_get_status(packet));
            break;
        case HCI_EVENT_AUTHENTICATION_COMPLETE:
            printf("Authentication complete: status 0x%02x\n", hci_event_authentication_complete_get_status(packet));
            break;
        case HCI_EVENT_SIMPLE_PAIRING_COMPLETE:
            printf("Simple pairing complete: status 0x%02x\n", hci_event_simple_pairing_complete_get_status(packet));
            break;
        case HCI_EVENT_LINK_KEY_REQUEST:
            printf("Link key request\n");
            break;
        case HCI_EVENT_LINK_KEY_NOTIFICATION:
            printf("Link key notification (chave nova guardada)\n");
            break;
        case HCI_EVENT_IO_CAPABILITY_RESPONSE:
            printf("IO capability response do controle\n");
            break;
        case HCI_EVENT_DISCONNECTION_COMPLETE:
            printf("HCI disconnect: status 0x%02x motivo 0x%02x\n",
                   hci_event_disconnection_complete_get_status(packet),
                   hci_event_disconnection_complete_get_reason(packet));
            break;

        case HCI_EVENT_PIN_CODE_REQUEST:
            hci_event_pin_code_request_get_bd_addr(packet, addr);
            printf("PIN legado solicitado - recusando\n");
            gap_pin_code_negative(addr);
            break;

        case HCI_EVENT_USER_CONFIRMATION_REQUEST:
            hci_event_user_confirmation_request_get_bd_addr(packet, addr);
            gap_ssp_confirmation_response(addr);
            break;

        case HCI_EVENT_HID_META:
            switch (hci_event_hid_meta_get_subevent_code(packet)) {
                case HID_SUBEVENT_INCOMING_CONNECTION:
                    printf("Conexao HID de entrada (controle religou)\n");
                    if (hid_subevent_incoming_connection_get_status(packet) == ERROR_CODE_SUCCESS) {
                        connecting = true;
                        hid_host_accept_connection(hid_subevent_incoming_connection_get_hid_cid(packet),
                                                   HID_PROTOCOL_MODE_REPORT);
                    }
                    break;

                case HID_SUBEVENT_CONNECTION_OPENED: {
                    uint8_t st = hid_subevent_connection_opened_get_status(packet);
                    connecting = false;
                    if (st) {
                        printf("Conexao HID falhou: 0x%02x\n", st);
                        connected = false; hid_cid = 0;
                        if (!bonded) start_scan();   // com bond, o retry_timer tenta de novo
                        break;
                    }
                    hid_cid = hid_subevent_connection_opened_get_hid_cid(packet);
                    connected = true; scanning = false;
                    printf("HID conectado (cid 0x%04x)\n", hid_cid);
                    procon_on_connected();
                    usb_procon_request_wake();    // controle conectou (botao apertado): acorda o PC suspenso
                    break;
                }

                case HID_SUBEVENT_DESCRIPTOR_AVAILABLE:
                    printf("Descritor HID: status 0x%02x, %u bytes\n",
                           hid_subevent_descriptor_available_get_status(packet),
                           hid_descriptor_storage_get_descriptor_len(hid_cid));
                    if (hid_descriptor_storage_get_descriptor_len(hid_cid))
                        log_hex("DESC", hid_descriptor_storage_get_descriptor_data(hid_cid),
                                hid_descriptor_storage_get_descriptor_len(hid_cid));
                    break;

                case HID_SUBEVENT_REPORT:
                    procon_on_report(hid_subevent_report_get_report(packet),
                                     hid_subevent_report_get_report_len(packet));
                    break;

                case HID_SUBEVENT_SET_PROTOCOL_RESPONSE:
                    printf("Set protocol: handshake 0x%02x modo %u\n",
                           hid_subevent_set_protocol_response_get_handshake_status(packet),
                           hid_subevent_set_protocol_response_get_protocol_mode(packet));
                    break;

                case HID_SUBEVENT_CONNECTION_CLOSED:
                    printf("HID desconectado\n");
                    connected = false; hid_cid = 0;
                    procon_on_disconnected();
                    if (!bonded) start_scan();   // sem bond (pareamento pedido): procura um novo controle
                    break;
                default: break;
            }
            break;
        default: break;
    }
}

int main(void) {
    usb_procon_init();    // TinyUSB + log serial (CDC)
    printf("\n=== ProControllerDongle fase 4 ===\n");

    if (cyw43_arch_init()) { printf("cyw43_arch_init falhou\n"); return 1; }

    procon_init(hid_send);
    procon_set_input_sink(usb_procon_push_input);
    l2cap_init();
    hid_host_init(hid_descriptor_storage, sizeof(hid_descriptor_storage));
    hid_host_register_packet_handler(packet_handler);

    gap_set_default_link_policy_settings(LM_LINK_POLICY_ENABLE_SNIFF_MODE | LM_LINK_POLICY_ENABLE_ROLE_SWITCH);
    hci_set_master_slave_policy(HCI_ROLE_MASTER);
    gap_set_local_name("ProControllerDongle");
    gap_set_class_of_device(0x2508);
    gap_set_security_level(LEVEL_2);
    gap_set_bondable_mode(1);
    gap_ssp_set_io_capability(SSP_IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
    gap_ssp_set_auto_accept(1);
    gap_connectable_control(1);     // aceita reconexao iniciada pelo controle

    hci_cb.callback = &packet_handler;
    hci_add_event_handler(&hci_cb);

    btstack_run_loop_set_timer_handler(&tick_timer, tick);
    btstack_run_loop_set_timer(&tick_timer, 100);
    btstack_run_loop_add_timer(&tick_timer);

    hci_power_control(HCI_POWER_ON);

    while (true) usb_procon_task();
}
