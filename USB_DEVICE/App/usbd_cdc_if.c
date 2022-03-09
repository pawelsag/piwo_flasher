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
/* USER CODE BEGIN INCLUDE */

/* USER CODE END INCLUDE */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/

/* USER CODE END PV */

/** @addtogroup STM32_USB_OTG_DEVICE_LIBRARY
  * @brief Usb device library.
  * @{
  */

/** @addtogroup USBD_CDC_IF
  * @{
  */

/** @defgroup USBD_CDC_IF_Private_TypesDefinitions USBD_CDC_IF_Private_TypesDefinitions
  * @brief Private types.
  * @{
  */

/* USER CODE BEGIN PRIVATE_TYPES */

/* USER CODE END PRIVATE_TYPES */

/**
  * @}
  */

/** @defgroup USBD_CDC_IF_Private_Defines USBD_CDC_IF_Private_Defines
  * @brief Private defines.
  * @{
  */

/* USER CODE BEGIN PRIVATE_DEFINES */
/* USER CODE END PRIVATE_DEFINES */

/**
  * @}
  */

/** @defgroup USBD_CDC_IF_Private_Macros USBD_CDC_IF_Private_Macros
  * @brief Private macros.
  * @{
  */

/* USER CODE BEGIN PRIVATE_MACRO */

/* USER CODE END PRIVATE_MACRO */

/**
  * @}
  */

/** @defgroup USBD_CDC_IF_Private_Variables USBD_CDC_IF_Private_Variables
  * @brief Private variables.
  * @{
  */
/* Create buffer for reception and transmission           */
/* It's up to user to redefine and/or remove those define */
/** Received data over USB are stored in this buffer      */
uint8_t UserRxBufferFS[APP_RX_DATA_SIZE];

/** Data to send over USB CDC are stored in this buffer   */
uint8_t UserTxBufferFS[APP_TX_DATA_SIZE];

/* USER CODE BEGIN PRIVATE_VARIABLES */
extern UART_HandleTypeDef huart4;
UART_HandleTypeDef *uartx = &huart4;

/* USER CODE END PRIVATE_VARIABLES */

/**
  * @}
  */

/** @defgroup USBD_CDC_IF_Exported_Variables USBD_CDC_IF_Exported_Variables
  * @brief Public variables.
  * @{
  */

extern USBD_HandleTypeDef hUsbDeviceFS;

/* USER CODE BEGIN EXPORTED_VARIABLES */

/* USER CODE END EXPORTED_VARIABLES */

/**
  * @}
  */

/** @defgroup USBD_CDC_IF_Private_FunctionPrototypes USBD_CDC_IF_Private_FunctionPrototypes
  * @brief Private functions declaration.
  * @{
  */

static int8_t CDC_Init_FS(void);
static int8_t CDC_DeInit_FS(void);
static int8_t CDC_Control_FS(uint8_t cmd, uint8_t* pbuf, uint16_t length);
static int8_t CDC_Receive_FS(uint8_t* pbuf, uint32_t *Len);

/* USER CODE BEGIN PRIVATE_FUNCTIONS_DECLARATION */

/**
  * @brief UART4 Initialization Function
  * @param None
  * @retval None
  */
static void UART_Init(const USBD_CDC_LineCodingTypeDef *cdc_uart_config)
{

  /* USER CODE BEGIN UART4_Init 0 */

  /* USER CODE END UART4_Init 0 */

  /* USER CODE BEGIN UART4_Init 1 */

  /* USER CODE END UART4_Init 1 */
  uartx->Instance = UART4;
  uartx->Init.BaudRate = cdc_uart_config->bitrate;
  uartx->Init.WordLength = cdc_uart_config->datatype;
  uartx->Init.StopBits = cdc_uart_config->format;
  uartx->Init.Parity = cdc_uart_config->paritytype;
  uartx->Init.Mode = UART_MODE_TX_RX;
  uartx->Init.HwFlowCtl = UART_HWCONTROL_NONE;
  uartx->Init.OverSampling = UART_OVERSAMPLING_16;
  uartx->Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  uartx->AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(uartx) != HAL_OK)
  {
    Error_Handler();
  }
}

int cdc_parity_to_hal_parity(int parity)
{
  switch (parity)
  {
  case 0:
    return UART_PARITY_NONE;
  case 1:
    return UART_PARITY_ODD;
  case 2:
    return UART_PARITY_EVEN;
  default :
    Error_Handler();
  }
  __builtin_unreachable();
}

int cdc_stopbits_to_hal_stopbits(int stopbits)
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
  default :
    Error_Handler();
  }
  __builtin_unreachable();
}

int cdc_wordwidth_to_hal_wordwidth(int wordwidth)
{
  switch (wordwidth)
  {
  case 0x07:
    Error_Handler();
  case 0x08:
    return UART_WORDLENGTH_8B;
  case 0x09:
    return UART_WORDLENGTH_9B;
  default :
    Error_Handler();
  }
  __builtin_unreachable();
}

/* USER CODE END PRIVATE_FUNCTIONS_DECLARATION */

/**
  * @}
  */

USBD_CDC_ItfTypeDef USBD_Interface_fops_FS =
{
  CDC_Init_FS,
  CDC_DeInit_FS,
  CDC_Control_FS,
  CDC_Receive_FS
};

/* Private functions ---------------------------------------------------------*/
/**
  * @brief  Initializes the CDC media low layer over the FS USB IP
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_Init_FS(void)
{
  /* USER CODE BEGIN 3 */
  const USBD_CDC_LineCodingTypeDef uart_line_config = {
    .bitrate = 115200,
    .format = UART_STOPBITS_1,
    .paritytype = UART_PARITY_NONE,
    .datatype = UART_WORDLENGTH_8B
  };

  /* Set Application Buffers */
  USBD_CDC_SetTxBuffer(&hUsbDeviceFS, UserTxBufferFS, 0);
  USBD_CDC_SetRxBuffer(&hUsbDeviceFS, UserRxBufferFS);
  /* configure uart */
  UART_Init(&uart_line_config);
  return (USBD_OK);
  /* USER CODE END 3 */
}

/**
  * @brief  DeInitializes the CDC media low layer
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_DeInit_FS(void)
{
  /* USER CODE BEGIN 4 */
  if(HAL_UART_DeInit(uartx) != HAL_OK)
  {
    Error_Handler();
  }
  return (USBD_OK);
  /* USER CODE END 4 */
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
        .bitrate = pbuf[0] | (pbuf[1]<<8) | (pbuf[2]<<16) | (pbuf[3]<<24),
        .format = cdc_stopbits_to_hal_stopbits(pbuf[4]),
        .paritytype = cdc_parity_to_hal_parity(pbuf[5]),
        .datatype = cdc_wordwidth_to_hal_wordwidth(pbuf[6]) 
      };

      if(HAL_UART_DeInit(uartx) != HAL_OK)
      {
        Error_Handler();
      }
      UART_Init(&uart_line_config);
      char arr [80];
      sprintf(arr, "Conf set speed %lu \n", uart_line_config.bitrate); 
      HAL_UART_Transmit(uartx, (uint8_t*)arr, strlen(arr), HAL_MAX_DELAY);
    }
    break;

    case CDC_GET_LINE_CODING:
    pbuf[0] = (uint8_t)uartx->Init.BaudRate;
    pbuf[1] = (uint8_t)(uartx->Init.BaudRate>>8);
    pbuf[2] = (uint8_t)(uartx->Init.BaudRate>>16);
    pbuf[3] = (uint8_t)(uartx->Init.BaudRate>>24);
    pbuf[4] = uartx->Init.StopBits;
    pbuf[5] = uartx->Init.Parity;
    pbuf[6] = uartx->Init.WordLength;
    break;
    char arr [80];
    sprintf(arr, "Conf get speed %lu \n", uartx->Init.BaudRate); 
    HAL_UART_Transmit(uartx, (uint8_t*)arr, strlen(arr), HAL_MAX_DELAY);

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
  HAL_UART_Transmit(uartx, Buf, *Len, HAL_MAX_DELAY);
  USBD_CDC_SetRxBuffer(&hUsbDeviceFS, &Buf[0]);
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

/* USER CODE BEGIN PRIVATE_FUNCTIONS_IMPLEMENTATION */

/* USER CODE END PRIVATE_FUNCTIONS_IMPLEMENTATION */

/**
  * @}
  */

/**
  * @}
  */
