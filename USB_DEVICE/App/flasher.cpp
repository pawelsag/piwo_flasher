extern "C" {
#include "flasher.h"
}

#include "global.hpp"

#include "proto.hpp"

stm32_config stm_configuration;

template<typename T>
int
stm32_read(T* buf, uint32_t count)
{
  return HAL_UART_Receive(&huartx, (uint8_t*)buf, sizeof(T)*count, HAL_MAX_DELAY);
}

template<typename T>
int
stm32_write(const T* buf, uint32_t count)
{
  return HAL_UART_Transmit(&huartx, (uint8_t*)buf, sizeof(T)*count, HAL_MAX_DELAY);
}

int
stm32_fill_config()
{
  uint8_t init_cmd = STM32_CMD_INIT;
  uint8_t get_cmd[2] = {STM32_CMD_GET, STM32_CMD_GET^0xff};
  char response;

  if(stm32_write(&init_cmd, 1) != HAL_OK)
  {
      USB_TRANSMIT_STR("During stm32 config init write the uart error occured");
      return -1;
  }

  if(stm32_read(&response,1) != HAL_OK)
  {
      USB_TRANSMIT_STR("During stm32 config init read the uart error occured");
      return -1;
  }

  if(response != STM32_ACK)
  {
      USB_TRANSMIT_STR("Stm32 config error. Waiting for ACK failed");
      return -1;
  }

  if(stm32_write(get_cmd, 2) != HAL_OK)
  {
    USB_TRANSMIT_STR("During stm32 config get command uart failed");
    return -1;
  }

  stm32_read(&response, 1);
  if(response != 12)
  {
    USB_TRANSMIT_STR("STM cmd length incorrect");
    return -1;
  }

  stm32_read(&stm_configuration.version, 1);
  stm32_read(&stm_configuration.get, 1);
  stm32_read(&stm_configuration.gvr, 1);
  stm32_read(&stm_configuration.gid, 1);
  stm32_read(&stm_configuration.rm, 1);
  stm32_read(&stm_configuration.go, 1);
  stm32_read(&stm_configuration.wm, 1);
  stm32_read(&stm_configuration.er, 1);
  stm32_read(&stm_configuration.wp, 1);
  stm32_read(&stm_configuration.uw, 1);
  stm32_read(&stm_configuration.rp, 1);
  stm32_read(&stm_configuration.ur, 1);

  if(stm32_read(&response,1) != HAL_OK)
  {
      return -1;
  }
  if(response != STM32_ACK)
  {
    USB_TRANSMIT_STR("Reading STM configuation commands failed");
    return -1;
  }

  USB_TRANSMIT_STR("STM configuation commands done");
  return 0;
}

stm32_config
get_config()
{
  return stm_configuration;
}

int
stm32_erase_flash()
{
  constexpr uint8_t num_of_pages = 0xff;
  const uint8_t er_cmd[2] = {stm_configuration.er, (uint8_t)(stm_configuration.er^0xffu)};
  const uint8_t er_all[2] = {num_of_pages, 0x00};
  uint8_t response;

  USB_TRANSMIT_STR("Erasing STM pages");

  stm32_write(er_cmd, 2);
  stm32_read(&response, 1);

  if(response  != STM32_ACK)
    return -1;

  stm32_write(er_all, 2);
  stm32_read(&response, 1);

  if(response != STM32_ACK)
    return -1;

  USB_TRANSMIT_STR("Erasing STM pages done");

  return 0;
}

int
stm32_send_data(uint32_t addr, raw_packet data)
{
  const uint8_t cmd[2] = {stm_configuration.wm, (uint8_t)(stm_configuration.wm^0xff)};
  const uint8_t addr_cmd[4] = { uint8_t(addr>>24),
                            uint8_t(addr>>16),
                            uint8_t(addr>>8),
                            uint8_t(addr)};
  const uint8_t addr_chsum =  addr_cmd[0] ^ addr_cmd[1] ^ addr_cmd[2] ^ addr_cmd[3];
  uint8_t response;
  
  stm32_write(cmd, 2);
  stm32_read(&response, 1);
  if(response != STM32_ACK)
  {
    USB_TRANSMIT_STR("Writting to address failed");
    return -1;
  }

  uint32_t data_to_send = data.size();
  
  stm32_write(data.data(), data.size());
  // TODO send checksum
  //
  return 0;
}



static void
init_transfer()
{
  HAL_GPIO_WritePin(GPIOC, Boot_Pin, GPIO_PIN_SET);
  HAL_Delay(10);
  HAL_GPIO_WritePin(GPIOC, Reset_Pin, GPIO_PIN_SET);
  HAL_Delay(5);
  HAL_GPIO_WritePin(GPIOC, Reset_Pin, GPIO_PIN_RESET);
}




void
handle_command(uint8_t *data, uint32_t size)
{
  raw_packet packet(data, size);

  auto asdf = flash_init_builder::make_flash_init_builder(packet);

  auto packet_opt = packet.get_type();

  if(!packet_opt.has_value())
  {
      USB_TRANSMIT_STR("Incorrect frame type received");
      return;
  }

  switch(packet_opt.value())
  {
      case packet_type::INIT:
        break;
      case packet_type::FRAME:
        break;
      case packet_type::RESET:
        break;
      case packet_type::RESET_DONE:
        break;
  }

}
