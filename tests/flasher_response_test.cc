#include "proto.hpp"
#include <cstdint>

#include <gtest/gtest.h>

namespace
{
constexpr uint8_t COMMON_FLASH_RESPONSE_LENGTH_POS = 0;
constexpr uint8_t COMMON_FLASH_RESPONSE_TYPE_POS   = 1;
constexpr uint8_t FLASH_RESPONSE_TYPE_POS   = 2;

constexpr size_t  FLASH_RESPONSE_SIZE          = 3;
constexpr uint8_t FLASH_RESPONSE_TYPE          = 0x03;


} // namespace

TEST(FlashresponseTest, build_and_make_flash_response_ack_success)
{
  usb_byte_t buffer[FLASH_RESPONSE_SIZE];

  raw_packet raw_packet(buffer, FLASH_RESPONSE_SIZE);

 auto flash_response_builder_opt = flash_response_builder::make_flash_response_builder(raw_packet);

 ASSERT_TRUE(flash_response_builder_opt.has_value());
 EXPECT_EQ(buffer[COMMON_FLASH_RESPONSE_LENGTH_POS], usb_byte_t{ FLASH_RESPONSE_SIZE });
 EXPECT_EQ(buffer[COMMON_FLASH_RESPONSE_TYPE_POS], usb_byte_t{ FLASH_RESPONSE_TYPE });

 auto flash_response_builder = *flash_response_builder_opt;
 flash_response_builder.set_response(flash_response_type::ACK);

 flash_response flash_response_packet(flash_response_builder);

 // Pointers must be the same
 EXPECT_EQ(flash_response_packet.cdata(), buffer);
 EXPECT_EQ(flash_response_packet.data(), buffer);
 EXPECT_EQ(flash_response_packet.cdata(), buffer);
 EXPECT_EQ(flash_response_packet.begin(), buffer);
 EXPECT_EQ(flash_response_packet.cbegin(), buffer);
 EXPECT_EQ(flash_response_packet.end(), buffer + FLASH_RESPONSE_SIZE);
 EXPECT_EQ(flash_response_packet.cend(), buffer + FLASH_RESPONSE_SIZE);

 EXPECT_EQ(flash_response_packet.size(), FLASH_RESPONSE_SIZE);
 EXPECT_EQ(flash_response_packet.data()[COMMON_FLASH_RESPONSE_LENGTH_POS],
           usb_byte_t{ FLASH_RESPONSE_SIZE });

 ASSERT_TRUE(flash_response_packet.get_type().has_value());

 EXPECT_EQ(uint8_t(flash_response_packet.get_type().value()), FLASH_RESPONSE_TYPE);
 EXPECT_EQ(flash_response_packet.data()[COMMON_FLASH_RESPONSE_TYPE_POS],
           usb_byte_t{ FLASH_RESPONSE_TYPE });

 EXPECT_EQ(flash_response_packet.get_response(), flash_response_type::ACK);
}

TEST(FlashresponseTest, build_and_make_flash_response_nack_success)
{
  usb_byte_t buffer[FLASH_RESPONSE_SIZE];

  raw_packet raw_packet(buffer, FLASH_RESPONSE_SIZE);

 auto flash_response_builder_opt = flash_response_builder::make_flash_response_builder(raw_packet);

 ASSERT_TRUE(flash_response_builder_opt.has_value());
 EXPECT_EQ(buffer[COMMON_FLASH_RESPONSE_LENGTH_POS], usb_byte_t{ FLASH_RESPONSE_SIZE });
 EXPECT_EQ(buffer[COMMON_FLASH_RESPONSE_TYPE_POS], usb_byte_t{ FLASH_RESPONSE_TYPE });

 auto flash_response_builder = *flash_response_builder_opt;
 flash_response_builder.set_response(flash_response_type::NACK);

 flash_response flash_response_packet(flash_response_builder);

 // Pointers must be the same
 EXPECT_EQ(flash_response_packet.cdata(), buffer);
 EXPECT_EQ(flash_response_packet.data(), buffer);
 EXPECT_EQ(flash_response_packet.cdata(), buffer);
 EXPECT_EQ(flash_response_packet.begin(), buffer);
 EXPECT_EQ(flash_response_packet.cbegin(), buffer);
 EXPECT_EQ(flash_response_packet.end(), buffer + FLASH_RESPONSE_SIZE);
 EXPECT_EQ(flash_response_packet.cend(), buffer + FLASH_RESPONSE_SIZE);

 EXPECT_EQ(flash_response_packet.size(), FLASH_RESPONSE_SIZE);
 EXPECT_EQ(flash_response_packet.data()[COMMON_FLASH_RESPONSE_LENGTH_POS],
           usb_byte_t{ FLASH_RESPONSE_SIZE });

 ASSERT_TRUE(flash_response_packet.get_type().has_value());

 EXPECT_EQ(uint8_t(flash_response_packet.get_type().value()), FLASH_RESPONSE_TYPE);
 EXPECT_EQ(flash_response_packet.data()[COMMON_FLASH_RESPONSE_TYPE_POS],
           usb_byte_t{ FLASH_RESPONSE_TYPE });

 EXPECT_EQ(flash_response_packet.get_response(), flash_response_type::NACK);
}
