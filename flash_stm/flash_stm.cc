#include "protodef.hpp"
#include <stdio.h>
#include "proto.hpp"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <thread>
#include <termios.h>
#include <string.h>

const char * usage = "./flash_stm  /path/to/device /path/to/flash/binary\n";
constexpr uint32_t start_flash_addr = 0x80000000;
constexpr size_t buffer_size = 256;
int device, binary;
bool finish = false;

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

void
read_some(int fd, uint8_t *buf, int to_read)
{
  int cnt = 0;
  while(cnt < to_read)
    if(read(fd, buf+cnt, 1) > 0 || finish)
      cnt++;
}

void receive_msg(int device)
{
  uint8_t buf[max_packet_size];
  int bytes_to_rcv=0;
  raw_packet packet(buf, flash_msg_size);
  
  while(!finish)
  {
    // read proto header
    read_some(device, buf, 2);

    switch(buf[common_type_pos])
    {
      case (uint8_t)packet_type::RESET_DONE:
        finish = true;
        printf("Reset done received. Flashing done\n");
        return;

      case (uint8_t)packet_type::MSG:
      {
        read_some(device, buf + flash_msg_payload_size_pos, 1);
        read_some(device, buf + flash_msg_payload_pos, buf[flash_msg_payload_size_pos]);
        auto flash_msg_packet_opt = flash_msg::make_flash_msg(packet);
        if(!flash_msg_packet_opt.has_value())
          return;

        auto flash_msg_packet = flash_msg_packet_opt.value();
        printf("%s\n", flash_msg_packet.get_msg());
      }
      break;
      default:
        printf("Incorrect frame %d %d\n", buf[0], buf[1]);
        continue;
    }
  }
}

static bool
send_init_packet(int fd)
{
  uint8_t buf[flash_init_length];
  raw_packet raw_packet(buf, flash_init_length);
  auto flash_init_builder_opt = flash_init_builder::make_flash_init_builder(raw_packet);
  if(!flash_init_builder_opt.has_value())
  {
    printf("Creating init packet failed\n");
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
    printf("Creating frame packet failed. Can't flash device\n");
    return false;
  }

  auto flash_frame_builder = *flash_frame_builder_opt;

  flash_frame_builder.set_flash_addr(addr);

  if(!flash_frame_builder.append_data(payload, payload_size))
  {
    printf("Adding payload to frame failed. Can't flash device\n");
    return false;
  }
  flash_frame flash_frame_packet(flash_frame_builder);

  return write_all(fd, flash_frame_packet.data(), flash_frame_packet.size());
}

static bool
send_frame_with_correct_endian(int fd, uint32_t addr, uint8_t *payload, size_t payload_size)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  return send_frame(fd, __builtin_bswap32(addr), payload, payload_size);
#else
  return send_frame(fd, addr, payload, payload_size);
#endif
}

static bool
send_reset_packet(int fd)
{
  uint8_t buf[flash_reset_length];
  raw_packet raw_packet(buf, flash_reset_length);
  auto flash_reset_builder_opt = flash_reset_builder::make_flash_reset_builder(raw_packet);
  if(!flash_reset_builder_opt.has_value())
  {
    printf("Creating init packet failed\n");
    return false;
  }

  auto flash_reset_builder = *flash_reset_builder_opt;
  flash_reset flash_reset_packet(flash_reset_builder);

  return write_all(fd, flash_reset_packet.begin(), flash_reset_packet.size()); 
}

static bool
set_device_params(int fd)
{
  struct termios tty;

  if ( tcgetattr (fd, &tty) != 0 ) {
    printf("Can't get tty params. tcgetattr failed, err=%d\n", errno);
    return false;
  }

  /* Setting other Port Stuff */
  tty.c_cflag     &=  ~PARENB;            // No parity
  tty.c_cflag     &=  ~CSTOPB;            // One stop bit
  tty.c_cflag     &=  ~CSIZE;
  tty.c_cflag     |=  CS8;                // Set 8 bits per byte

  tty.c_cflag     &=  ~CRTSCTS;           // no flow control
  tty.c_cflag     |=  CREAD | CLOCAL;     // turn on READ & ignore ctrl lines

  tty.c_lflag     &= ~ICANON;             // disable canonical mode
  tty.c_lflag     &= ~ECHO; // Disable echo
  tty.c_lflag     &= ~ECHOE; // Disable erasure
  tty.c_lflag     &= ~ECHONL; // Disable new-line echo
  tty.c_lflag     &= ~ISIG;   // Disable interpretation of INTR, QUIT and SUSP
                              //
  tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl
  tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL); // Disable any special handling of received bytes

  tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
  tty.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed

  tty.c_cc[VTIME] = 10;    // Wait for up to 1s (10 deciseconds), returning as soon as any data is received.
  tty.c_cc[VMIN] = 0;

  /* Set Baud Rate */
  cfsetspeed(&tty, (speed_t)B115200);

  if ( tcsetattr ( fd, TCSANOW, &tty ) != 0) {
    printf("Setting device attributes failed, err=%d\n", errno);
    return false;
  }
  return true;
}

int main(int argc, char* argv[])
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  uint32_t flash_address = __builtin_bswap32(start_flash_addr);
#else
  uint32_t flash_address = start_flash_addr;
#endif

  uint8_t packet_buf[max_packet_size];
  uint8_t file_buf[flash_frame_max_data_length];
  raw_packet packet(packet_buf, buffer_size);
  size_t file_size = 0, total_bytes_read = 0;
  size_t bytes_read = 0, next_read = 0, to_send = 0;
  struct stat st;

  if(argc != 3)
  {
    printf("%s", usage);
    return -1;
  }

  device = open(argv[1], O_RDWR);
  if(device == -1)
  {
    printf("Can't open device.\n");
    return -2;
  }
  std::thread recevier(receive_msg, device);

  if(!set_device_params(device))
  {
    printf("Setting device params, failed\n");
    return -3;
  }
  
  binary = open(argv[2], O_RDWR);
  if (binary == -1)
  {
    printf("Can't open file.\n");
    return -2;
  }


  stat(argv[2], &st);
  file_size = st.st_size;
  printf("Flashing binary %s of size %d \n", argv[2], file_size);

  // init packet
  if(!send_init_packet(device))
  {
    printf("Sending init packet fail\n");
    return -1;
  }

  //next_read = flash_frame_max_data_length; 
  //while(total_bytes_read < file_size)
  //{
  //  bytes_read = read(binary, file_buf + to_send, next_read);
  //  to_send += bytes_read;

  //  if(bytes_read == 0)
  //  {
  //    // we can get here only when the payload size is not alligned to 4
  //    if(to_send != 0)
  //    {
  //      while(to_send & 0x11){
  //        file_buf[to_send] = 0x0;
  //        to_send++;
  //      }
  //      if(!send_frame_with_correct_endian(device, flash_address, file_buf, to_send))
  //      {
  //        printf("Sending frame packet failed");
  //        return -4;
  //      }
  //    }

  //    printf("Bytes %d flashed\n", file_size);
  //    send_reset_packet(device);
  //    break;
  //  }

  //  if(bytes_read < 0)
  //  {
  //    printf("Data read general error %d\n", errno);
  //    send_reset_packet(device);
  //    break;
  //  }

  // flash_address = start_flash_addr + total_bytes_read;
  // total_bytes_read += bytes_read;

  //  if(bytes_read & 0x11) {
  //    next_read = next_read - bytes_read;
  //    continue;
  //  }
  //  else
  //    next_read = flash_frame_max_data_length;

  // if(send_frame_with_correct_endian(device, flash_address, file_buf, to_send))
  // {
  //   printf("Sending frame packet failed");
  //   return  -4;
  // }
  // to_send = 0;
  //}
  int end = 0;
  while(!end)
  {
    scanf("%d", &end);
  }

  finish = true;
  recevier.join();
  close(device);
}
