#include "proto.hpp"
#include <cstdint>
#include <vector>
#include <array>
#include <iostream>

#include <gmock/gmock-actions.h>
#include <gmock/gmock-cardinalities.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace
{
constexpr uint8_t COMMON_FLASH_MSG_LENGTH_POS    = 0;
constexpr uint8_t COMMON_FLASH_MSG_TYPE_POS      = 1;
constexpr uint8_t COMMON_FLASH_MSG_PAYLOAD_SIZE_POS = 2;
constexpr uint8_t COMMON_FLASH_MSG_PAYLOAD_POS      = 3;

constexpr uint8_t FLASH_MSG_TYPE          = 0x04;
constexpr uint8_t FLASH_MSG_HEADER_SIZE   = 0x03;

} // namespace

TEST(FlashmsgTest, build_and_make_flash_msg_success)
{
  constexpr size_t  FLASH_MSG_SIZE          = 20;
  constexpr uint8_t FLASH_MSG_DATA_SIZE = 5;
  constexpr uint8_t EXPECTED_PAYLOAD_SIZE    = 5;

  constexpr uint8_t FLASH_MSG_DATA[FLASH_MSG_DATA_SIZE] = {'H', 'E', 'L', 'L', 'O'};
  usb_byte_t buffer[FLASH_MSG_SIZE];

  raw_packet raw_packet(buffer, FLASH_MSG_SIZE);

  auto flash_msg_builder_opt = flash_msg_builder::make_flash_msg_builder(raw_packet);

  ASSERT_TRUE(flash_msg_builder_opt.has_value());
  EXPECT_EQ(buffer[COMMON_FLASH_MSG_LENGTH_POS], usb_byte_t{ FLASH_MSG_SIZE });
  EXPECT_EQ(buffer[COMMON_FLASH_MSG_TYPE_POS], usb_byte_t{ FLASH_MSG_TYPE });
 
  auto flash_msg_builder = *flash_msg_builder_opt;

  EXPECT_TRUE(flash_msg_builder.copy_msg(FLASH_MSG_DATA, FLASH_MSG_DATA_SIZE));
 
  flash_msg flash_msg_packet(flash_msg_builder);
 
  // Pointers must be the same
  EXPECT_EQ(flash_msg_packet.cdata(), buffer);
  EXPECT_EQ(flash_msg_packet.data(), buffer);
  EXPECT_EQ(flash_msg_packet.cdata(), buffer);
  EXPECT_EQ(flash_msg_packet.begin(), buffer);
  EXPECT_EQ(flash_msg_packet.cbegin(), buffer);
  EXPECT_EQ(flash_msg_packet.end(), buffer + FLASH_MSG_SIZE);
  EXPECT_EQ(flash_msg_packet.cend(), buffer + FLASH_MSG_SIZE);
  ASSERT_EQ(flash_msg_packet.get_lenght(), FLASH_MSG_SIZE);
  ASSERT_EQ(flash_msg_packet.get_msg_size(), EXPECTED_PAYLOAD_SIZE);
 
  EXPECT_EQ(flash_msg_packet.size(), FLASH_MSG_SIZE);
  EXPECT_EQ(flash_msg_packet.data()[COMMON_FLASH_MSG_LENGTH_POS],
            usb_byte_t{ FLASH_MSG_SIZE });
 
  ASSERT_TRUE(flash_msg_packet.get_type().has_value());
 
  EXPECT_EQ(uint8_t(flash_msg_packet.get_type().value()), FLASH_MSG_TYPE);
  EXPECT_EQ(flash_msg_packet.data()[COMMON_FLASH_MSG_TYPE_POS],
            usb_byte_t{ FLASH_MSG_TYPE });
  
  for(int i =0 ;i <flash_msg_packet.get_msg_size(); i++)
  {
    EXPECT_EQ(FLASH_MSG_DATA[i], flash_msg_packet.get_msg()[i]) << FLASH_MSG_DATA[i];
  }
}
