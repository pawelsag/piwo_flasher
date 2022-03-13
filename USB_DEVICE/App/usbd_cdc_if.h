/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : usbd_cdc_if.h
  * @version        : v2.0_Cube
  * @brief          : Header for usbd_cdc_if.c file.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USBD_CDC_IF_H__
#define __USBD_CDC_IF_H__

#ifdef __cplusplus
 extern "C" {
#endif

#include "config.h"
#include <string.h>

extern USBD_CDC_ItfTypeDef USBD_Interface_fops_FS;

extern UART_HandleTypeDef huartx;

uint8_t CDC_Transmit_FS(uint8_t* Buf, uint16_t Len);

#define USB_TRANSMIT(data, len) CDC_Transmit_FS((uint8_t*)data, len)
#define USB_TRANSMIT_STR(str) CDC_Transmit_FS((uint8_t*)str, strlen(str))


#ifdef __cplusplus
}
#endif

#endif /* __USBD_CDC_IF_H__ */

