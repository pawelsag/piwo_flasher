extern "C" {
#include "flasher.h"
}
#include "global.hpp"
#include <stdarg.h>

#include "proto.hpp"
constexpr int uart_timeout_ms = 1000;
stm32_config stm_configuration;

void usb_transmit_msg(const char *format, ...)
{
  const int attemps = 10;
  int attempt = 0;

  uint8_t packet_buf[flash_msg_length];
  uint8_t payload[flash_msg_payload_length];
  va_list args;
  va_start(args, format);
  int written = vsnprintf(reinterpret_cast<char*>(payload), flash_msg_length, format, args);
  va_end(args);

  if(written < 0 || written == max_packet_size)
    return;

  raw_packet packet(packet_buf, max_packet_size);

  auto flash_msg_builder_opt = flash_msg_builder::make_flash_msg_builder(packet);
  if(!flash_msg_builder_opt.has_value())
    return; // this should never heppen

  auto flash_msg_builder = *flash_msg_builder_opt;

  if(!flash_msg_builder.copy_msg(payload, written+1))
    return;

  flash_msg msg = flash_msg(flash_msg_builder);

  while(CDC_Transmit_FS(msg.data(), msg.get_msg_size() + flash_msg_header_length) != USBD_OK && attempt < attemps)
  {
    HAL_Delay(50);
    attempt++;
  }
};

void
usb_transmit_cmd_response(flash_response_type response)
{
  const int attemps = 10;
  int attempt = 0;

  uint8_t packet_buf[flash_response_length];
  raw_packet raw_packet(packet_buf, flash_response_length);

  auto flash_response_builder_opt = flash_response_builder::make_flash_response_builder(raw_packet);
  if(!flash_response_builder_opt.has_value())
  {
    usb_transmit_msg("Sending response failed");
    return; // this should never heppen
  }

  auto flash_response_builder = *flash_response_builder_opt;
  flash_response_builder.set_response(response);

  flash_response response_packet(flash_response_builder);

  while(CDC_Transmit_FS(response_packet.data(), response_packet.size()) != USBD_OK && attempt < attemps)
  {
    HAL_Delay(50);
    attempt++;
  }
}

int
stm32_read(uint8_t* buf, uint32_t count)
{
  return HAL_UART_Receive(&huartx, (uint8_t*)buf, count, uart_timeout_ms);
}

int
stm32_write(const uint8_t* buf, uint32_t count)
{
  return HAL_UART_Transmit(&huartx, (uint8_t*)buf, count, uart_timeout_ms);
}

bool
stm32_init()
{
  constexpr int cmds_res_size = 12;
  uint8_t commands[cmds_res_size];
  uint8_t init_cmd = STM32_CMD_INIT;
  uint8_t get_cmd[2] = {STM32_CMD_GET, STM32_CMD_GET^0xff};
  uint8_t response=0x0;

  if(stm32_write(&init_cmd, 1) != HAL_OK)
  {
      usb_transmit_msg("During stm32 config init write the uart error occured");
      return false;
  }

  if(stm32_read(&response, 1) != HAL_OK)
  {
      usb_transmit_msg("During stm32 config init read the uart error occured");
      return false;
  }

  if(response != STM32_ACK)
  {
      usb_transmit_msg("Stm32 config error. Waiting for ACK failed %d != %d", STM32_ACK, response);
      return false;
  }
  response = 0x0;

  if(stm32_write(get_cmd, 2) != HAL_OK)
  {
    usb_transmit_msg("During stm32 config get command uart failed");
    return false;
  }

  if(stm32_read(&response, 1) != HAL_OK)
  {
      usb_transmit_msg("During stm32 config init read the uart error occured");
      return false;
  }

  if(response != STM32_ACK)
  {
      usb_transmit_msg("Stm32 config error. Waiting for ACK failed %d != %d", STM32_ACK, response);
      return false;
  }
  response = 0x0;

  stm32_read(&response, 1);
  if(response != cmds_res_size)
  {
    usb_transmit_msg("STM cmd length incorrect, 12 != %d", response);
    return false;
  }

  stm32_read(commands, cmds_res_size);

  stm_configuration.version = commands[0];
  stm_configuration.get     = commands[1];
  stm_configuration.gvr     = commands[2];
  stm_configuration.gid     = commands[3];
  stm_configuration.rm      = commands[4];
  stm_configuration.go      = commands[5];
  stm_configuration.wm      = commands[6];
  stm_configuration.er      = commands[7];
  stm_configuration.wp      = commands[8];
  stm_configuration.uw      = commands[9];
  stm_configuration.rp      = commands[10];
  stm_configuration.ur      = commands[11];

  if(stm_configuration.version == 0x33)
  {
    stm32_read(&stm_configuration.ch, 1);
  }

  if(stm32_read(&response, 1) != HAL_OK)
  {
      return false;
  }

  if(response != STM32_ACK)
  {
    usb_transmit_msg("Reading STM configuation commands failed");
    return false;
  }

  usb_transmit_msg("STM configuation commands done");
  return true;
}

stm32_config
get_config()
{
  return stm_configuration;
}

bool
stm32_erase_flash()
{
  constexpr uint8_t num_of_pages = 0xff;
  const uint8_t er_cmd[2] = {stm_configuration.er, (uint8_t)(stm_configuration.er^0xff)};
  const uint8_t er_all[2] = {num_of_pages, 0x00};
  uint8_t response=0x0;

  usb_transmit_msg("Erasing STM pages");

  stm32_write(er_cmd, 2);
  stm32_read(&response, 1);

  if(response != STM32_ACK)
    return false;
  response = 0x0;

  stm32_write(er_all, 2);
  stm32_read(&response, 1);

  if(response != STM32_ACK)
    return false;

  usb_transmit_msg("Erasing STM pages done");

  return true;
}

bool
stm32_send_data(const addr_raw_t &addr, const uint8_t *payload, uint8_t size, uint8_t chksum)
{
  const uint8_t cmd[2] = {stm_configuration.wm, (uint8_t)(stm_configuration.wm^0xff)};
  const uint8_t addr_chksum =  addr[0] ^ addr[1] ^ addr[2] ^ addr[3];
  uint8_t response=0x0;

  //send command
  stm32_write(cmd, 2);
  stm32_read(&response, 1);

  if(response != STM32_ACK)
  {
    usb_transmit_msg("Sending payload command failed. ACK not received %x != 0x79", response);
    return false;
  }
  response = 0x0;

  // send addr
  stm32_write(addr.data(), addr.size());
  // send addr checksum
  stm32_write(&addr_chksum, 1);

  stm32_read(&response, 1);
  if(response != STM32_ACK)
  {
    usb_transmit_msg("Sending payload address failed. ACK not received %x != 0x79", response);
    return false;
  }
  response = 0x0;

  // we send N as a number of bytes to receive and than N+1 bytes
  const uint8_t real_size = size-1;
  // send payload size
  stm32_write(&real_size, 1);
  // send payload
  stm32_write(payload, size);
  // send addr checksum
  stm32_write(&chksum, 1);

  stm32_read(&response, 1);
  if(response != STM32_ACK)
  {
    usb_transmit_msg("Sending payload frame failed. ACK not received %x != 0x79", response);
    return false;
  }

  return true;
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

  auto packet_type_opt = packet.get_type();

  if(!packet_type_opt.has_value())
  {
      usb_transmit_msg("Incorrect frame type received");
      usb_transmit_cmd_response(flash_response_type::NACK);
      return;
  }

  usb_transmit_msg("Handle command %d with size %d %d", (uint8_t)packet_type_opt.value(), data[0], size);

  switch(packet_type_opt.value())
  {
      case packet_type::INIT:
        {
          auto flash_init_opt = flash_init::make_flash_init(packet);
          usb_transmit_msg("Handle init packet");
          if(!flash_init_opt.has_value())
          {
            usb_transmit_msg("Received init packet incorrect");
            usb_transmit_cmd_response(flash_response_type::NACK);
            return;
          }

          auto flash_init = flash_init_opt.value();
          init_transfer();

          if(!stm32_init())
          {
            usb_transmit_msg("Init cmd failed");
            usb_transmit_cmd_response(flash_response_type::NACK);
            return;
          }

          if(!stm32_erase_flash())
          {
            usb_transmit_msg("Erase cmd failed");
            usb_transmit_cmd_response(flash_response_type::NACK);
            return;
          }
          usb_transmit_cmd_response(flash_response_type::ACK);
        }
        break;
      case packet_type::FRAME:
        {
          auto flash_frame_opt = flash_frame::make_flash_frame(packet);
          if(!flash_frame_opt.has_value())
          {
            usb_transmit_msg("Received frame packet incorrect");
            usb_transmit_cmd_response(flash_response_type::NACK);
            return;
          }
          auto flash_frame = flash_frame_opt.value();
          auto flash_address = flash_frame.get_addr_raw();
          if(!stm32_send_data(flash_address,
                          flash_frame.get_payload(),
                          flash_frame.get_payload_size(),
                          flash_frame.get_checksum()))
          {
            usb_transmit_cmd_response(flash_response_type::NACK);
            return;
          }

          usb_transmit_cmd_response(flash_response_type::ACK);
        }
        break;
      case packet_type::RESET:
        {
          auto flash_init_opt = flash_reset::make_flash_reset(packet);
          if(!flash_init_opt.has_value())
          {
            usb_transmit_msg("Received reset packet incorrect");
            usb_transmit_cmd_response(flash_response_type::NACK);
            return;
          }

          reset_transfer();
          usb_transmit_msg("Reset done");
#if defined(WITH_SIMULATION)
          uint8_t simulation_resest_cmd[2] = {0x3, 0xfc};
          stm32_write(simulation_resest_cmd, 2);
#endif
          usb_transmit_cmd_response(flash_response_type::ACK);
        }
        break;
      default:
            usb_transmit_msg("Handler failed");
        break;
  }
}
