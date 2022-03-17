#include "global.hpp"

UART_HandleTypeDef huartx;

/**
  * @brief UART Initialization Function
  * @param None
  * @retval None
  */
void uart_init(const USBD_CDC_LineCodingTypeDef *cdc_uart_config)
{
  huartx.Instance = CURRENT_UART;
  huartx.Init.BaudRate = cdc_uart_config->bitrate;
  huartx.Init.WordLength = cdc_uart_config->datatype;
  huartx.Init.StopBits = cdc_uart_config->format;
  huartx.Init.Parity = cdc_uart_config->paritytype;
  huartx.Init.Mode = UART_MODE_TX_RX;
  huartx.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huartx.Init.OverSampling = UART_OVERSAMPLING_16;
  huartx.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huartx.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huartx) != HAL_OK)
  {
    Error_Handler();
  }
}
