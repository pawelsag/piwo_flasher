#include <stdio.h>
#include "proto.hpp"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

const char * usage = "./flash_stm  /path/to/device /path/to/flash/binary\n";
constexpr size_t buffer_size = 256;
int device, binary;

bool
write_all(int fd, uint8_t *buf, size_t size)
{
  size_t written;
  while(size)
  {
    written = write(fd, buf, size);
    if(written == -1)
      return false;
    size -= written;
  }
  return true;
}

static bool
send_init_packet(int fd)
{
  uint8_t buf[flash_init_length];
  raw_packet raw_packet(buf, flash_init_length);
  auto flash_init_builder_opt = flash_init_builder::make_flash_init_builder(raw_packet);
  if(!flash_init_builder_opt.has_value())
  {
    printf("Creating init packet failed");
    return false;
  }

  auto flash_init_builder = *flash_init_builder_opt;
  flash_init flash_init_packet(flash_init_builder);

  return write_all(fd, flash_init_packet.begin(), flash_init_packet.size()); 
}

// data passed to buffer should always deivde by 4
static bool
send_frame(int fd, uint32_t addr, uint8_t *payload, size_t payload_size)
{
  uint8_t buf[flash_frame_max_length];
  raw_packet raw_packet(buf, flash_frame_max_length);
  auto flash_frame_builder_opt = flash_frame_builder::make_flash_frame_builder(raw_packet);

  if(!flash_frame_builder_opt.has_value())
  {
    printf("Creating frame packet failed. Can't flash device");
    return false;
  }

  auto flash_frame_builder = *flash_frame_builder_opt;

  flash_frame_builder.set_flash_addr(addr);

  if(!flash_frame_builder.append_data(payload,payload_size))
  {
    printf("Adding payload to frame failed. Can't flash device");
    return false;
  }
  flash_frame flash_frame_packet(flash_frame_builder);

  return write_all(fd, flash_frame_packet.data(), flash_frame_packet.size());
}

static bool
send_reset_packet(int fd)
{
  uint8_t buf[flash_reset_length];
  raw_packet raw_packet(buf, flash_reset_length);
  auto flash_reset_builder_opt = flash_reset_builder::make_flash_reset_builder(raw_packet);
  if(!flash_reset_builder_opt.has_value())
  {
    printf("Creating init packet failed");
    return false;
  }

  auto flash_reset_builder = *flash_reset_builder_opt;
  flash_reset flash_reset_packet(flash_reset_builder);

  return write_all(fd, flash_reset_packet.begin(), flash_reset_packet.size()); 
}

int main(int argc, char* argv[])
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  uint32_t flash_address = __builtin_bswap32(0x80000000);
#else
  uint32_t flash_address = 0x80000000;
#endif

  uint8_t packet_buf[max_packet_size];
  uint8_t file_buf[flash_frame_max_data_length];
  raw_packet packet(packet_buf, buffer_size);
  size_t file_size, total_bytes_read = 0;
  size_t bytes_read;
  struct stat st;

  if(argc != 3)
  {
    printf("%s", usage);
    return -1;
  }

  device = open(argv[1], O_RDWR);
  if(device == -1)
  {
    printf("Can't open device.");
    return -2;
  }
  
  binary = open(argv[2], O_RDWR);
  if (binary == -1)
  {
    printf("Can't open file.");
    return -2;
  }

  stat(argv[2], &st);
  file_size = st.st_size;
  // init packet
  if(!send_init_packet(device))
  {
    printf("Sending init packet fail");
    return -1;
  }
  
  while(total_bytes_read < file_size)
  {
    bytes_read = read(binary, file_buf, flash_frame_max_data_length);
    if(bytes_read == 0)
    {
      // end of file
      // create reset packet wait for reset done packet
      // then break
      break;
    }
    if(bytes_read < 0)
    {
      // binary read error
      // create reset packet wait for reset done packet
      // then break
    }
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  flash_address = __builtin_bswap32(flash_address + total_bytes_read);
#else
  flash_address = flash_address + total_bytes_read;
#endif
    // send

  }
}
