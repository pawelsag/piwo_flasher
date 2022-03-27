/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : usbd_cdc_if.c
  * @version        : v2.0_Cube
  * @brief          : Usb device for Virtual Com Port.
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

/* Includes ------------------------------------------------------------------*/
#include "usbd_cdc_if.h"

#include <string.h>
#include <stdio.h>

#define APP_RX_DATA_SIZE  256
#define APP_TX_DATA_SIZE  256

uint8_t UserRxBufferFS[APP_RX_DATA_SIZE];

uint8_t UserTxBufferFS[APP_TX_DATA_SIZE];

extern USBD_HandleTypeDef hUsbDeviceFS;

static int8_t CDC_Init_FS(void);
static int8_t CDC_DeInit_FS(void);
static int8_t CDC_Control_FS(uint8_t cmd, uint8_t* pbuf, uint16_t length);
static int8_t CDC_Receive_FS(uint8_t* pbuf, uint32_t *Len);

uint32_t cdc_parity_to_hal_parity(uint8_t parity)
{
  switch (parity)
  {
  case 0:
    return UART_PARITY_NONE;
  case 1:
    return UART_PARITY_ODD;
  case 2:
    return UART_PARITY_EVEN;
  default:
    Error_Handler();
  }
  __builtin_unreachable();
}

uint32_t cdc_stopbits_to_hal_stopbits(uint8_t stopbits)
{
  switch (stopbits)
  {
  case 0:
    return UART_STOPBITS_1;
    break;
  case 1:
    return UART_STOPBITS_1_5;
  case 2:
    return UART_STOPBITS_2;
  default:
    Error_Handler();
  }
  __builtin_unreachable();
}

uint32_t cdc_wordwidth_to_hal_wordwidth(uint8_t wordwidth)
{
  switch (wordwidth)
  {
  case 0x07:
    Error_Handler();
  case 0x08:
    return UART_WORDLENGTH_8B;
  case 0x09:
    return UART_WORDLENGTH_9B;
  default:
    Error_Handler();
  }
  __builtin_unreachable();
}

USBD_CDC_ItfTypeDef USBD_Interface_fops_FS =
{
  CDC_Init_FS,
  CDC_DeInit_FS,
  CDC_Control_FS,
  CDC_Receive_FS
};

static int8_t CDC_Init_FS(void)
{
  const USBD_CDC_LineCodingTypeDef uart_line_config = {
    .bitrate = 115200,
    .format = UART_STOPBITS_1,
    .paritytype = UART_PARITY_NONE,
    .datatype = UART_WORDLENGTH_8B
  };

  USBD_CDC_SetTxBuffer(&hUsbDeviceFS, UserTxBufferFS, 0);
  USBD_CDC_SetRxBuffer(&hUsbDeviceFS, UserRxBufferFS);
  uart_init(&uart_line_config);
  return (USBD_OK);
}

/**
  * @brief  DeInitializes the CDC media low layer
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_DeInit_FS(void)
{
  if(HAL_UART_DeInit(&huartx) != HAL_OK)
  {
    Error_Handler();
  }
  return (USBD_OK);
}

/**
  * @brief  Manage the CDC class requests
  * @param  cmd: Command code
  * @param  pbuf: Buffer containing command data (request parameters)
  * @param  length: Number of data to be sent (in bytes)
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_Control_FS(uint8_t cmd, uint8_t* pbuf, uint16_t length)
{
  /* USER CODE BEGIN 5 */
  switch(cmd)
  {
    case CDC_SEND_ENCAPSULATED_COMMAND:

    break;

    case CDC_GET_ENCAPSULATED_RESPONSE:

    break;

    case CDC_SET_COMM_FEATURE:

    break;

    case CDC_GET_COMM_FEATURE:

    break;

    case CDC_CLEAR_COMM_FEATURE:

    break;

  /*******************************************************************************/
  /* Line Coding Structure                                                       */
  /*-----------------------------------------------------------------------------*/
  /* Offset | Field       | Size | Value  | Description                          */
  /* 0      | dwDTERate   |   4  | Number |Data terminal rate, in bits per second*/
  /* 4      | bCharFormat |   1  | Number | Stop bits                            */
  /*                                        0 - 1 Stop bit                       */
  /*                                        1 - 1.5 Stop bits                    */
  /*                                        2 - 2 Stop bits                      */
  /* 5      | bParityType |  1   | Number | Parity                               */
  /*                                        0 - None                             */
  /*                                        1 - Odd                              */
  /*                                        2 - Even                             */
  /*                                        3 - Mark                             */
  /*                                        4 - Space                            */
  /* 6      | bDataBits  |   1   | Number Data bits (5, 6, 7, 8 or 16).          */
  /*******************************************************************************/
    case CDC_SET_LINE_CODING:
    {
      const USBD_CDC_LineCodingTypeDef uart_line_config = {
        .bitrate = (uint32_t)pbuf[0] | (uint32_t)(pbuf[1]<<8) | (uint32_t)(pbuf[2]<<16) | (uint32_t)(pbuf[3]<<24),
        .format = cdc_stopbits_to_hal_stopbits(pbuf[4]),
        .paritytype = cdc_parity_to_hal_parity(pbuf[5]),
        .datatype = cdc_wordwidth_to_hal_wordwidth(pbuf[6]) 
      };

      if(HAL_UART_DeInit(&huartx) != HAL_OK)
      {
        Error_Handler();
      }
      uart_init(&uart_line_config);
    }
    break;

    case CDC_GET_LINE_CODING:
    pbuf[0] = (uint8_t)huartx.Init.BaudRate;
    pbuf[1] = (uint8_t)(huartx.Init.BaudRate>>8);
    pbuf[2] = (uint8_t)(huartx.Init.BaudRate>>16);
    pbuf[3] = (uint8_t)(huartx.Init.BaudRate>>24);
    pbuf[4] = huartx.Init.StopBits;
    pbuf[5] = huartx.Init.Parity;
    pbuf[6] = huartx.Init.WordLength;
    break;

    case CDC_SET_CONTROL_LINE_STATE:

    break;

    case CDC_SEND_BREAK:

    break;

  default:
    break;
  }

  return (USBD_OK);
  /* USER CODE END 5 */
}

/**
  * @brief  Data received over USB OUT endpoint are sent over CDC interface
  *         through this function.
  *
  *         @note
  *         This function will issue a NAK packet on any OUT packet received on
  *         USB endpoint until exiting this function. If you exit this function
  *         before transfer is complete on CDC interface (ie. using DMA controller)
  *         it will result in receiving more data while previous ones are still
  *         not sent.
  *
  * @param  Buf: Buffer of data to be received
  * @param  Len: Number of data received (in bytes)
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_Receive_FS(uint8_t* Buf, uint32_t *Len)
{
  /* USER CODE BEGIN 6 */
  if(usb_event_rx == 1)
    return USBD_BUSY;
  memcpy(usb_rx.buf, Buf, *Len);
  usb_rx.len = *Len;
  usb_event_rx = 1;

  USBD_CDC_ReceivePacket(&hUsbDeviceFS);

  return (USBD_OK);
  /* USER CODE END 6 */
}

/**
  * @brief  CDC_Transmit_FS
  *         Data to send over USB IN endpoint are sent over CDC interface
  *         through this function.
  *         @note
  *
  *
  * @param  Buf: Buffer of data to be sent
  * @param  Len: Number of data to be sent (in bytes)
  * @retval USBD_OK if all operations are OK else USBD_FAIL or USBD_BUSY
  */
uint8_t CDC_Transmit_FS(uint8_t* Buf, uint16_t Len)
{
  uint8_t result = USBD_OK;
  /* USER CODE BEGIN 7 */
  USBD_CDC_HandleTypeDef *hcdc = (USBD_CDC_HandleTypeDef*)hUsbDeviceFS.pClassData;
  if (hcdc->TxState != 0){
    return USBD_BUSY;
  }

  USBD_CDC_SetTxBuffer(&hUsbDeviceFS, Buf, Len);
  result = USBD_CDC_TransmitPacket(&hUsbDeviceFS);
  /* USER CODE END 7 */
  return result;
}
