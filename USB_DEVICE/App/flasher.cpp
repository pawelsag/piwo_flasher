extern "C" {
#include "flasher.h"
}
#include "global.hpp"

#include "proto.hpp"

stm32_config stm_configuration;

template<typename... Args>
inline void usb_transmit_msg(const char *format, Args... args)
{
  uint8_t packet_buf[max_packet_size];
  uint8_t payload[flash_msg_payload_length];

  int written = snprintf(reinterpret_cast<char*>(payload), max_packet_size, format, args...);
  if(written < 0 || written == max_packet_size)
    return;

  raw_packet packet(packet_buf, max_packet_size);

  auto flash_msg_builder_opt = flash_msg_builder::make_flash_msg_builder(packet);
  if(!flash_msg_builder_opt.has_value())
    return;

  auto flash_msg_builder = *flash_msg_builder_opt;

  if(!flash_msg_builder.copy_msg(payload, written))
    return;

  CDC_Transmit_FS(packet_buf, max_packet_size);
};


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
stm32_init()
{
  uint8_t init_cmd = STM32_CMD_INIT;
  uint8_t get_cmd[2] = {STM32_CMD_GET, STM32_CMD_GET^0xff};
  char response;

  if(stm32_write(&init_cmd, 1) != HAL_OK)
  {
      usb_transmit_msg("During stm32 config init write the uart error occured");
      return -1;
  }

  if(stm32_read(&response,1) != HAL_OK)
  {
      usb_transmit_msg("During stm32 config init read the uart error occured");
      return -1;
  }

  if(response != STM32_ACK)
  {
      usb_transmit_msg("Stm32 config error. Waiting for ACK failed %d != %d", STM32_ACK, response);
      return -1;
  }

  if(stm32_write(get_cmd, 2) != HAL_OK)
  {
    usb_transmit_msg("During stm32 config get command uart failed");
    return -1;
  }

  stm32_read(&response, 1);
  if(response != 12)
  {
    usb_transmit_msg("STM cmd length incorrect, 12 != %d", response);
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
    usb_transmit_msg("Reading STM configuation commands failed");
    return -1;
  }

  usb_transmit_msg("STM configuation commands done");
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
  const uint8_t er_cmd[2] = {stm_configuration.er, (uint8_t)(stm_configuration.er^0xff)};
  const uint8_t er_all[2] = {num_of_pages, 0x00};
  uint8_t response;

  usb_transmit_msg("Erasing STM pages");

  stm32_write(er_cmd, 2);
  stm32_read(&response, 1);

  if(response != STM32_ACK)
    return -1;

  stm32_write(er_all, 2);
  stm32_read(&response, 1);

  if(response != STM32_ACK)
    return -1;

  usb_transmit_msg("Erasing STM pages done");

  return 0;
}

int
stm32_send_data(const addr_raw_t &addr, const uint8_t *payload, uint8_t size, uint8_t chksum)
{
  const uint8_t cmd[2] = {stm_configuration.wm, (uint8_t)(stm_configuration.wm^0xff)};
  const uint8_t addr_chksum =  addr[0] ^ addr[1] ^ addr[2] ^ addr[3];
  uint8_t response;

  //send command
  stm32_write(cmd, 2);
  stm32_read(&response, 1);

  if(response != STM32_ACK)
  {
    usb_transmit_msg("Sending payload command failed. ACK not received");
    return -1;
  }

  // send addr
  stm32_write(addr.data(), addr.size());
  // send addr checksum
  stm32_write(&addr_chksum, 1);

  stm32_read(&response, 1);
  if(response != STM32_ACK)
  {
    usb_transmit_msg("Sending payload address failed. ACK not received");
    return -1;
  }

  // send payload size
  stm32_write(&size, 1);
  // send payload
  stm32_write(payload, size);
  // send addr checksum
  stm32_write(&chksum, 1);

  stm32_read(&response, 1);
  if(response != STM32_ACK)
  {
    usb_transmit_msg("Sending payload failed. ACK not received");
    return -1;
  }

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

static void
reset_transfer()
{
  HAL_GPIO_WritePin(GPIOC, Boot_Pin, GPIO_PIN_RESET);
  HAL_Delay(10);
  HAL_GPIO_WritePin(GPIOC, Reset_Pin, GPIO_PIN_SET);
  HAL_Delay(5);
  HAL_GPIO_WritePin(GPIOC, Reset_Pin, GPIO_PIN_RESET);
}


void
handle_command(uint8_t *data, uint32_t size)
{
  raw_packet packet(data, size);

  auto packet_opt = packet.get_type();

  if(!packet_opt.has_value())
  {
      usb_transmit_msg("Incorrect frame type received");
      return;
  }

  switch(packet_opt.value())
  {
      case packet_type::INIT:
        {
          auto flash_init_opt = flash_init::make_flash_init(packet);
          if(!flash_init_opt.has_value())
          {
            usb_transmit_msg("Received init packet incorrect");
            return;
          }

          auto flash_init = flash_init_opt.value();

          init_transfer();
          stm32_init();
          stm32_erase_flash();
        }
        break;
      case packet_type::FRAME:
        {
          auto flash_frame_opt = flash_frame::make_flash_frame(packet);
          if(!flash_frame_opt.has_value())
          {
            usb_transmit_msg("Received frame packet incorrect");
            return;
          }
          auto flash_frame = flash_frame_opt.value();
          auto flash_address = flash_frame.get_addr_raw();
          stm32_send_data(flash_address,
                          flash_frame.get_payload(),
                          flash_frame.get_payload_size(),
                          flash_frame.get_checksum());

        }
        break;
      case packet_type::RESET:
        {
          auto flash_init_opt = flash_init::make_flash_init(packet);
          if(!flash_init_opt.has_value())
          {
            usb_transmit_msg("Received reset packet incorrect");
            return;
          }
          reset_transfer();

          uint8_t reset_done_buf[flash_reset_done_length];
          raw_packet raw_packet(reset_done_buf, flash_reset_done_length);

          auto flash_reset_done_builder_opt = flash_reset_done_builder::make_flash_reset_done_builder(raw_packet);
          if(!flash_reset_done_builder_opt.has_value())
          {
           usb_transmit_msg("Packet reset done creation failed");
           return;
          }
          auto flash_reset_done_builder = *flash_reset_done_builder_opt;
          flash_reset_done flash_reset_done_packet(flash_reset_done_builder);
          CDC_Transmit_FS(flash_reset_done_packet.data(), flash_reset_done_packet.size());
        }
        break;
      default:
        break;
  }

}
