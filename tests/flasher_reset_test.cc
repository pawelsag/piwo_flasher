#include "proto.hpp"
#include <cstdint>

#include <gtest/gtest.h>

namespace
{
constexpr uint8_t COMMON_FLASH_RESET_LENGTH_POS = 0;
constexpr uint8_t COMMON_FLASH_RESET_TYPE_POS   = 1;

constexpr size_t  FLASH_RESET_SIZE          = 2;
constexpr uint8_t FLASH_RESET_TYPE          = 0x02;

} // namespace

TEST(FlashresetTest, build_and_make_flash_reset_success)
{
  usb_byte_t buffer[FLASH_RESET_SIZE];

  raw_packet raw_packet(buffer, FLASH_RESET_SIZE);

 auto flash_reset_builder_opt = flash_reset_builder::make_flash_reset_builder(raw_packet);

 ASSERT_TRUE(flash_reset_builder_opt.has_value());
 EXPECT_EQ(buffer[COMMON_FLASH_RESET_LENGTH_POS], usb_byte_t{ FLASH_RESET_SIZE });
 EXPECT_EQ(buffer[COMMON_FLASH_RESET_TYPE_POS], usb_byte_t{ FLASH_RESET_TYPE });

 auto flash_reset_builder = *flash_reset_builder_opt;

 flash_reset flash_reset_packet(flash_reset_builder);

 // Pointers must be the same
 EXPECT_EQ(flash_reset_packet.cdata(), buffer);
 EXPECT_EQ(flash_reset_packet.data(), buffer);
 EXPECT_EQ(flash_reset_packet.cdata(), buffer);
 EXPECT_EQ(flash_reset_packet.begin(), buffer);
 EXPECT_EQ(flash_reset_packet.cbegin(), buffer);
 EXPECT_EQ(flash_reset_packet.end(), buffer + FLASH_RESET_SIZE);
 EXPECT_EQ(flash_reset_packet.cend(), buffer + FLASH_RESET_SIZE);

 EXPECT_EQ(flash_reset_packet.size(), FLASH_RESET_SIZE);
 EXPECT_EQ(flash_reset_packet.data()[COMMON_FLASH_RESET_LENGTH_POS],
           usb_byte_t{ FLASH_RESET_SIZE });

 ASSERT_TRUE(flash_reset_packet.get_type().has_value());

 EXPECT_EQ(uint8_t(flash_reset_packet.get_type().value()), FLASH_RESET_TYPE);
 EXPECT_EQ(flash_reset_packet.data()[COMMON_FLASH_RESET_TYPE_POS],
           usb_byte_t{ FLASH_RESET_TYPE });
}
