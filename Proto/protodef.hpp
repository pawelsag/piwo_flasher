#pragma once
#include <array>
#include <cstdint>
#include <algorithm>
#include <array>

using usb_byte_t = uint8_t;
using addr_raw_t = std::array<uint8_t, 4>;

enum class packet_type : uint8_t
{
  INIT,
  FRAME,
  RESET, // also send when flashing is done
  RESET_DONE,
  MSG
};

constexpr int common_length_pos = 0x0;
constexpr int common_type_pos   = 0x1;

// flash init
constexpr int flash_init_length = 2;
// flash frame
// payload size must devide by 4
constexpr int flash_frame_data_size       = 0x1;
constexpr int flash_frame_checksum_size   = 0x1;
constexpr int flash_frame_addr_size       = 0x4;
constexpr int flash_frame_header_length   = flash_frame_data_size + flash_frame_checksum_size + flash_frame_addr_size + 2;
constexpr int flash_frame_max_data_length = 256 - flash_frame_header_length;
static_assert(flash_frame_max_data_length % 4 == 0);
constexpr int flash_frame_max_length      = 256;

constexpr int flash_frame_addr_pos           = common_type_pos + 1;
constexpr int flash_frame_payload_size_pos  = common_type_pos + 5;
constexpr int flash_frame_checksum_pos      = common_type_pos + 6;
constexpr int flash_frame_payload_pos       = common_type_pos + 7;

// flash reset request
constexpr int flash_reset_length = 2;
// flash reset response
constexpr int flash_reset_done_length = 2;
// flash msg
constexpr int flash_msg_size = 256;
constexpr int flash_msg_payload_size_length = 1;
constexpr int flash_msg_payload_length = 253;
constexpr int flash_msg_payload_size_pos = common_type_pos + 1;
constexpr int flash_msg_payload_pos      = common_type_pos + 2;
constexpr int flash_msg_header_length   = flash_msg_payload_size_length + 2;

constexpr int max_packet_size = std::max({ flash_init_length,
                                           flash_frame_max_length,
                                           flash_msg_size,
                                           flash_reset_length,
                                           flash_reset_done_length});
