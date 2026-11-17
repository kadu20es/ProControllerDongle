#ifndef TUSB_CONFIG_H
#define TUSB_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

// Dispositivo USB: Pro Controller (HID) + porta serial de diagnostico (CDC, opcional).
#ifndef PROCON_DEBUG_CDC
#define PROCON_DEBUG_CDC 1
#endif

#define CFG_TUSB_RHPORT0_MODE   (OPT_MODE_DEVICE | OPT_MODE_FULL_SPEED)
#define CFG_TUD_ENDPOINT0_SIZE  64

// PROCON_WAKE=1 adiciona um teclado HID (so envia F15) para o Windows permitir acordar o PC.
#ifndef PROCON_WAKE
#define PROCON_WAKE 1
#endif
#define CFG_TUD_HID             (1 + PROCON_WAKE)
#define CFG_TUD_HID_EP_BUFSIZE  64

#if PROCON_DEBUG_CDC
#define CFG_TUD_CDC             1
#define CFG_TUD_CDC_RX_BUFSIZE  256
#define CFG_TUD_CDC_TX_BUFSIZE  1024
#define CFG_TUD_CDC_EP_BUFSIZE  64
#else
#define CFG_TUD_CDC             0
#endif

#define CFG_TUD_MSC             0
#define CFG_TUD_MIDI            0
#define CFG_TUD_VENDOR          0

#ifdef __cplusplus
}
#endif

#endif
