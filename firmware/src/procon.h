// Protocolo do Nintendo Switch Pro Controller sobre Bluetooth HID (lado "host").
// Inicializa o controle (info, SPI, IMU, vibracao, modo 0x30), guarda uma imagem da SPI
// e entrega os relatorios 0x30 para quem os consome (ponte USB).
#ifndef PROCON_H
#define PROCON_H

#include <stdbool.h>
#include <stdint.h>

// Envia relatorio de saida pelo canal HID BT (report_id + payload). Retorna 0 se ok.
typedef uint8_t (*procon_send_fn)(uint8_t report_id, const uint8_t *data, uint16_t len);
// Chamado (no contexto do BTstack) a cada relatorio 0x30 recebido: r[0]=0xA1, r[1]=0x30, len>=50.
typedef void (*procon_input_sink_fn)(const uint8_t *r, uint16_t len);

void procon_init(procon_send_fn send);
void procon_set_input_sink(procon_input_sink_fn fn);
void procon_on_connected(void);                        // canal HID aberto
void procon_on_disconnected(void);
void procon_on_report(const uint8_t *r, uint16_t len);  // r[0] = 0xA1, r[1] = report ID
void procon_tick(uint32_t now_ms);                     // ~250 ms
void procon_rumble_pulse(void);                        // vibracao curta de teste
void procon_dump(void);                                // imprime SPI em cache
void procon_set_verbose(bool v);
bool procon_verbose(void);
bool procon_ready(void);                               // setup concluido (modo 0x30 ativo)

// --- usado pela ponte USB (qualquer contexto) ---
// Le da imagem da SPI (0x6000-0x60FF e 0x8000-0x803F); o resto devolve 0xFF.
void procon_spi_read(uint32_t addr, uint8_t len, uint8_t *out);
// 12 bytes de dados da resposta do subcomando 0x02 (fw, tipo, MAC...). Valores padrao antes da conexao.
void procon_get_dev_info(uint8_t out[12]);
// Repassa a vibracao (8 bytes, formato HD) e o LED de jogador para o controle real.
void procon_forward_rumble(const uint8_t rumble8[8]);
void procon_forward_player_lights(uint8_t bits);

#endif
