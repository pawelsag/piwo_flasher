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
constexpr uint8_t COMMON_FLASH_FRAME_LENGTH_POS    = 0;
constexpr uint8_t COMMON_FLASH_FRAME_TYPE_POS      = 1;
constexpr uint8_t COMMON_FLASH_FRAME_DATA_SIZE_POS = 2;
constexpr uint8_t COMMON_FLASH_FRAME_DATA_POS      = 3;

constexpr uint8_t FLASH_FRAME_TYPE          = 0x01;
constexpr uint8_t FLASH_FRAME_HEADER_SIZE   = 0x03;

} // namespace

TEST(FlashFrameTest, build_and_make_flash_frame_success)
{
  constexpr size_t  FLASH_FRAME_SIZE          = 8;
  constexpr uint8_t FLASH_FRAME_DATA_SIZE_MAX = FLASH_FRAME_SIZE - FLASH_FRAME_HEADER_SIZE;
  constexpr uint8_t EXPECTED_PAYLOAD_ADDED    = 4;
  constexpr uint8_t FLASH_FRAME_DATA[FLASH_FRAME_DATA_SIZE_MAX] = {0x10, 0xa, 0xff, 0x00, 0xbb};
  usb_byte_t buffer[FLASH_FRAME_SIZE];

  raw_packet raw_packet(buffer, FLASH_FRAME_SIZE);

  auto flash_frame_builder_opt = flash_frame_builder::make_flash_frame_builder(raw_packet);

  ASSERT_TRUE(flash_frame_builder_opt.has_value());
  EXPECT_EQ(buffer[COMMON_FLASH_FRAME_LENGTH_POS], usb_byte_t{ FLASH_FRAME_SIZE });
  EXPECT_EQ(buffer[COMMON_FLASH_FRAME_TYPE_POS], usb_byte_t{ FLASH_FRAME_TYPE });
 
  auto flash_frame_builder = *flash_frame_builder_opt;

  EXPECT_FALSE(flash_frame_builder.append_data(FLASH_FRAME_DATA, 0x5));
  EXPECT_TRUE(flash_frame_builder.append_data(FLASH_FRAME_DATA, 0x4));
 
  flash_frame flash_frame_packet(flash_frame_builder);
 
  // Pointers must be the same
  EXPECT_EQ(flash_frame_packet.cdata(), buffer);
  EXPECT_EQ(flash_frame_packet.data(), buffer);
  EXPECT_EQ(flash_frame_packet.cdata(), buffer);
  EXPECT_EQ(flash_frame_packet.begin(), buffer);
  EXPECT_EQ(flash_frame_packet.cbegin(), buffer);
  EXPECT_EQ(flash_frame_packet.end(), buffer + FLASH_FRAME_SIZE);
  EXPECT_EQ(flash_frame_packet.cend(), buffer + FLASH_FRAME_SIZE);
  ASSERT_EQ(flash_frame_packet.get_lenght(), FLASH_FRAME_SIZE);
  ASSERT_EQ(flash_frame_packet.get_payload_size(), EXPECTED_PAYLOAD_ADDED);
 
  EXPECT_EQ(flash_frame_packet.size(), FLASH_FRAME_SIZE);
  EXPECT_EQ(flash_frame_packet.data()[COMMON_FLASH_FRAME_LENGTH_POS],
            usb_byte_t{ FLASH_FRAME_SIZE });
 
  ASSERT_TRUE(flash_frame_packet.get_type().has_value());
 
  EXPECT_EQ(uint8_t(flash_frame_packet.get_type().value()), FLASH_FRAME_TYPE);
  EXPECT_EQ(flash_frame_packet.data()[COMMON_FLASH_FRAME_TYPE_POS],
            usb_byte_t{ FLASH_FRAME_TYPE });
  
  for(int i =0 ;i <flash_frame_packet.get_payload_size(); i++)
  {
    EXPECT_EQ(FLASH_FRAME_DATA[i], flash_frame_packet.get_payload()[i]) << FLASH_FRAME_DATA[i];
  }
}

TEST(FlashFrameTest, build_and_make_flash_frame_append_payload_mulitple_times_success)
{
  constexpr size_t  FLASH_FRAME_SIZE          = 16;
  constexpr uint8_t FLASH_FRAME_DATA_SIZE_MAX = FLASH_FRAME_SIZE - FLASH_FRAME_HEADER_SIZE;
  constexpr uint8_t EXPECTED_PAYLOAD_ADDED    = 8;
  constexpr uint8_t FLASH_FRAME_DATA[FLASH_FRAME_DATA_SIZE_MAX] = {0x10, 0xa, 0xff, 0x00, 0xbb, 0xcc, 0xdd, 0xee};
  usb_byte_t buffer[FLASH_FRAME_SIZE];

  raw_packet raw_packet(buffer, FLASH_FRAME_SIZE);

  auto flash_frame_builder_opt = flash_frame_builder::make_flash_frame_builder(raw_packet);

  ASSERT_TRUE(flash_frame_builder_opt.has_value());
  EXPECT_EQ(buffer[COMMON_FLASH_FRAME_LENGTH_POS], usb_byte_t{ FLASH_FRAME_SIZE });
  EXPECT_EQ(buffer[COMMON_FLASH_FRAME_TYPE_POS], usb_byte_t{ FLASH_FRAME_TYPE });
 
  auto flash_frame_builder = *flash_frame_builder_opt;

  ASSERT_TRUE(flash_frame_builder.append_data(FLASH_FRAME_DATA, 0x4));
  ASSERT_FALSE(flash_frame_builder.append_data(FLASH_FRAME_DATA, 0x5));
  ASSERT_TRUE(flash_frame_builder.append_data(FLASH_FRAME_DATA+0x4, 0x4));
 
  flash_frame flash_frame_packet(flash_frame_builder);
 
  // Pointers must be the same
  EXPECT_EQ(flash_frame_packet.cdata(), buffer);
  EXPECT_EQ(flash_frame_packet.data(), buffer);
  EXPECT_EQ(flash_frame_packet.cdata(), buffer);
  EXPECT_EQ(flash_frame_packet.begin(), buffer);
  EXPECT_EQ(flash_frame_packet.cbegin(), buffer);
  EXPECT_EQ(flash_frame_packet.end(), buffer + FLASH_FRAME_SIZE);
  EXPECT_EQ(flash_frame_packet.cend(), buffer + FLASH_FRAME_SIZE);
  ASSERT_EQ(flash_frame_packet.get_lenght(), FLASH_FRAME_SIZE);
  ASSERT_EQ(flash_frame_packet.get_payload_size(), EXPECTED_PAYLOAD_ADDED);
 
  EXPECT_EQ(flash_frame_packet.size(), FLASH_FRAME_SIZE);
  EXPECT_EQ(flash_frame_packet.data()[COMMON_FLASH_FRAME_LENGTH_POS],
            usb_byte_t{ FLASH_FRAME_SIZE });
 
  ASSERT_TRUE(flash_frame_packet.get_type().has_value());
 
  EXPECT_EQ(uint8_t(flash_frame_packet.get_type().value()), FLASH_FRAME_TYPE);
  EXPECT_EQ(flash_frame_packet.data()[COMMON_FLASH_FRAME_TYPE_POS],
            usb_byte_t{ FLASH_FRAME_TYPE });
  
  for(int i =0 ;i <flash_frame_packet.get_payload_size(); i++)
  {
    EXPECT_EQ(FLASH_FRAME_DATA[i], flash_frame_packet.get_payload()[i]) << FLASH_FRAME_DATA[i];
  }
}
