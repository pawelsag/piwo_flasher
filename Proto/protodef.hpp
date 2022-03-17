#pragma once
#include <cstdint>
#include <algorithm>

using usb_byte_t = uint8_t;

enum class packet_type : uint8_t
{
  INIT,
  FRAME,
  RESET, // also send when flashing is done
  RESET_DONE,
};

constexpr int common_length_pos = 0x0;
constexpr int common_type_pos   = 0x1;

// flasher init the 
constexpr int flash_init_length = 6;
constexpr int flash_init_addr_size   = 0x4;
constexpr int flash_init_addr_pos = common_type_pos + 1; // MSB -> LSB ex. 0x80 0x00 0x00 0x00
  // size must devide by 4
  // precalculate the crc of data
constexpr int flash_frame_data_size   = 0x1;
constexpr int flash_frame_header_length   = flash_frame_data_size + 2;
constexpr int flash_frame_max_data_length   = 256 - flash_frame_header_length;
constexpr int flash_frame_max_length   = 256;

constexpr int flash_frame_payload_size_pos = common_type_pos + 1;
constexpr int flash_frame_payload_pos      = common_type_pos + 2;

// flash reset request
constexpr int flash_reset_length = 2;
// flash reset response
constexpr int flash_reset_done_length = 2;

constexpr int max_packet_size = std::max({ flash_init_length,
                                           flash_frame_max_length,
                                           flash_reset_length,
                                           flash_reset_done_length});
