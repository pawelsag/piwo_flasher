#pragma once

#ifdef __cplusplus
 extern "C" {
#endif
#include "usbd_cdc.h"
struct usb_data
{
  uint8_t buf [256];
  uint8_t len;
};

#define CURRENT_UART USART1
// TODO add usb handle define

extern int usb_event_rx;
extern struct usb_data usb_rx;

void uart_init(const USBD_CDC_LineCodingTypeDef *cdc_uart_config);

// TODO add usb init function

#ifdef __cplusplus
}
#endif
