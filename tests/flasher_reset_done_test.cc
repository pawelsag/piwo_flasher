#include "proto.hpp"
#include <cstdint>

#include <gtest/gtest.h>

namespace
{
constexpr uint8_t COMMON_FLASH_RESET_DONE_LENGTH_POS = 0;
constexpr uint8_t COMMON_FLASH_RESET_DONE_TYPE_POS   = 1;

constexpr size_t  FLASH_RESET_DONE_SIZE          = 2;
constexpr uint8_t FLASH_RESET_DONE_TYPE          = 0x03;

} // namespace

TEST(Flashreset_doneTest, build_and_make_flash_reset_done_success)
{
  usb_byte_t buffer[FLASH_RESET_DONE_SIZE];

  raw_packet raw_packet(buffer, FLASH_RESET_DONE_SIZE);

 auto flash_reset_done_builder_opt = flash_reset_done_builder::make_flash_reset_done_builder(raw_packet);

 ASSERT_TRUE(flash_reset_done_builder_opt.has_value());
 EXPECT_EQ(buffer[COMMON_FLASH_RESET_DONE_LENGTH_POS], usb_byte_t{ FLASH_RESET_DONE_SIZE });
 EXPECT_EQ(buffer[COMMON_FLASH_RESET_DONE_TYPE_POS], usb_byte_t{ FLASH_RESET_DONE_TYPE });

 auto flash_reset_done_builder = *flash_reset_done_builder_opt;

 flash_reset_done flash_reset_done_packet(flash_reset_done_builder);

 // Pointers must be the same
 EXPECT_EQ(flash_reset_done_packet.cdata(), buffer);
 EXPECT_EQ(flash_reset_done_packet.data(), buffer);
 EXPECT_EQ(flash_reset_done_packet.cdata(), buffer);
 EXPECT_EQ(flash_reset_done_packet.begin(), buffer);
 EXPECT_EQ(flash_reset_done_packet.cbegin(), buffer);
 EXPECT_EQ(flash_reset_done_packet.end(), buffer + FLASH_RESET_DONE_SIZE);
 EXPECT_EQ(flash_reset_done_packet.cend(), buffer + FLASH_RESET_DONE_SIZE);

 EXPECT_EQ(flash_reset_done_packet.size(), FLASH_RESET_DONE_SIZE);
 EXPECT_EQ(flash_reset_done_packet.data()[COMMON_FLASH_RESET_DONE_LENGTH_POS],
           usb_byte_t{ FLASH_RESET_DONE_SIZE });

 ASSERT_TRUE(flash_reset_done_packet.get_type().has_value());

 EXPECT_EQ(uint8_t(flash_reset_done_packet.get_type().value()), FLASH_RESET_DONE_TYPE);
 EXPECT_EQ(flash_reset_done_packet.data()[COMMON_FLASH_RESET_DONE_TYPE_POS],
           usb_byte_t{ FLASH_RESET_DONE_TYPE });
}
