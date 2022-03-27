#pragma once

#ifdef __cplusplus
 extern "C" {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wvolatile"
#endif

#include "usbd_cdc.h"
#include "flasher.h"

#define CURRENT_UART USART1

struct usb_data
{
  uint8_t buf [256];
  uint8_t len;
};

extern int usb_event_rx;
extern struct usb_data usb_rx;
extern USBD_CDC_ItfTypeDef USBD_Interface_fops_FS;
extern UART_HandleTypeDef huartx;


void uart_init(const USBD_CDC_LineCodingTypeDef *cdc_uart_config);

#ifdef __cplusplus
#pragma GCC diagnostic pop
}
#endif
