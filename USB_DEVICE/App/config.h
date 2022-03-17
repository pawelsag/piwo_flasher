#pragma once

#ifdef __cplusplus
 extern "C" {
#endif
#include "usbd_cdc.h"

#define CURRENT_UART UART4
// TODO add usb handle define

void uart_init(const USBD_CDC_LineCodingTypeDef *cdc_uart_config);

// TODO add usb init function

#ifdef __cplusplus
}
#endif
