#include "proto.hpp"
#include <cstdint>

#include <gtest/gtest.h>

namespace
{
constexpr uint8_t COMMON_FLASH_INIT_LENGTH_POS = 0;
constexpr uint8_t COMMON_FLASH_INIT_TYPE_POS   = 1;

constexpr size_t  FLASH_INIT_SIZE          = 2;
constexpr uint8_t FLASH_INIT_TYPE          = 0x00;

} // namespace

TEST(FlashInitTest, build_and_make_flash_init_success)
{
 usb_byte_t buffer[FLASH_INIT_SIZE];

 raw_packet raw_packet(buffer, FLASH_INIT_SIZE);

 auto flash_init_builder_opt = flash_init_builder::make_flash_init_builder(raw_packet);

 ASSERT_TRUE(flash_init_builder_opt.has_value());
 EXPECT_EQ(buffer[COMMON_FLASH_INIT_LENGTH_POS], usb_byte_t{ FLASH_INIT_SIZE });
 EXPECT_EQ(buffer[COMMON_FLASH_INIT_TYPE_POS], usb_byte_t{ FLASH_INIT_TYPE });

 auto flash_init_builder = *flash_init_builder_opt;

 flash_init flash_init_packet(flash_init_builder);

 // Pointers must be the same
 EXPECT_EQ(flash_init_packet.cdata(), buffer);
 EXPECT_EQ(flash_init_packet.data(), buffer);
 EXPECT_EQ(flash_init_packet.cdata(), buffer);
 EXPECT_EQ(flash_init_packet.begin(), buffer);
 EXPECT_EQ(flash_init_packet.cbegin(), buffer);
 EXPECT_EQ(flash_init_packet.end(), buffer + FLASH_INIT_SIZE);
 EXPECT_EQ(flash_init_packet.cend(), buffer + FLASH_INIT_SIZE);

 EXPECT_EQ(flash_init_packet.size(), FLASH_INIT_SIZE);
 EXPECT_EQ(flash_init_packet.data()[COMMON_FLASH_INIT_LENGTH_POS],
           usb_byte_t{ FLASH_INIT_SIZE });

 ASSERT_TRUE(flash_init_packet.get_type().has_value());

 EXPECT_EQ(uint8_t(flash_init_packet.get_type().value()), FLASH_INIT_TYPE);
 EXPECT_EQ(flash_init_packet.data()[COMMON_FLASH_INIT_TYPE_POS],
           usb_byte_t{ FLASH_INIT_TYPE });
}
