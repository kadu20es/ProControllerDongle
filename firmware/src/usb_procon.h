// Lado USB: o dongle se apresenta ao PC como Nintendo Switch Pro Controller (057E:2009).
#ifndef USB_PROCON_H
#define USB_PROCON_H

#include <stdint.h>

void usb_procon_init(void);                                    // stdio de log + TinyUSB
void usb_procon_task(void);                                    // chamar no loop principal
void usb_procon_request_wake(void);                              // acorda o PC se o USB estiver suspenso (qualquer contexto)
void usb_procon_push_input(const uint8_t *r, uint16_t len);    // BT -> USB: r[0]=0xA1, r[1]=0x30 (qualquer contexto)

#endif
