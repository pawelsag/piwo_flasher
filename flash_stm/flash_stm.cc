#include "protodef.hpp"
#include <stdio.h>
#include "proto.hpp"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <thread>
#include <termios.h>
#include <string.h>
#include <atomic>
#include <array>
#include <condition_variable>
#include <chrono>
#include<spdlog/spdlog.h>

using namespace std::chrono_literals;

/* TODO change the max size to  flash_frame_max_data_lengt
 * when STM will reasemble the packets received via the USB CDC
 */
constexpr uint8_t max_usb_cdc_transfer_size = 64;
const std::string usage =
"\n[USAGE]./flash_stm  device binary [debug_level]\n"
"\t device - path to device file in /dev directory usually: /dev/ttyACM0\n"
"\t binary - path to binary to flash\n"
"\t debug_level - one of: info, debug, trace\n";

constexpr uint32_t start_flash_addr = 0x8000000;

std::condition_variable cv_response;
std::mutex mtx_response;

flash_response_type response = flash_response_type::NONE;

std::atomic_bool finish = false;

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
  raw_packet packet(buf, max_packet_size);
  
  while(!finish)
  {
    // read proto header
    read_some(device, buf, 2);

    switch(buf[common_type_pos])
    {
      case (uint8_t)packet_type::RESPONSE:
        {
         std::unique_lock<std::mutex> lk(mtx_response);
         read_some(device, buf + flash_response_type_pos, 1);
         auto flash_response_packet_opt = flash_response::make_flash_response(packet);
         if(!flash_response_packet_opt.has_value())
           return;

         auto flash_response_packet = flash_response_packet_opt.value();
         response = flash_response_packet.get_response();
         cv_response.notify_all();
         spdlog::debug("[STM32 RESPONSE] {}", flash_response_packet.get_response() == flash_response_type::ACK ? "ACK" : "NACK");
        }
        break;

      case (uint8_t)packet_type::MSG:
      {
        read_some(device, buf + flash_msg_payload_size_pos, 1);
        read_some(device, buf + flash_msg_payload_pos, buf[flash_msg_payload_size_pos]);
        auto flash_msg_packet_opt = flash_msg::make_flash_msg(packet);
        if(!flash_msg_packet_opt.has_value())
          return;

        auto flash_msg_packet = flash_msg_packet_opt.value();
        spdlog::debug("[STM32 MSG] {}", flash_msg_packet.get_msg());
      }
      break;
      default:
      spdlog::error("[FLASHER] Incorrect frame {} {}", buf[0], buf[1]);
        continue;
    }
  }
}
// There are two possible responses
// ACK when the transation was performed correctly
// NACK when transaction failed
//
// return true when we receive ACK
// otherwise return NACK
bool
wait_for_response()
{
  std::unique_lock<std::mutex> lk(mtx_response);
  if(!cv_response.wait_for(lk, 10000ms, []{return response != flash_response_type::NONE;})) 
    return false;
  auto rsp = response;
  response = flash_response_type::NONE;
  return rsp == flash_response_type::ACK;
}

static bool
send_init_packet(int fd)
{
  uint8_t buf[flash_init_length];
  raw_packet raw_packet(buf, flash_init_length);
  auto flash_init_builder_opt = flash_init_builder::make_flash_init_builder(raw_packet);
  if(!flash_init_builder_opt.has_value())
  {
    spdlog::error("[FLASHER] Creating init packet failed\n");
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
  raw_packet raw_packet(buf, max_usb_cdc_transfer_size);
  auto flash_frame_builder_opt = flash_frame_builder::make_flash_frame_builder(raw_packet);

  if(!flash_frame_builder_opt.has_value())
  {
    spdlog::error("[FLASHER] Creating frame packet failed. Can't flash device");
    return false;
  }

  auto flash_frame_builder = *flash_frame_builder_opt;

  flash_frame_builder.set_flash_addr(addr);

  if(!flash_frame_builder.set_data(payload, payload_size))
  {
    spdlog::error("[FLASHER] Adding payload to frame failed. Can't flash device");
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
    spdlog::error("[FLASHER] Creating init packet failed");
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
    spdlog::error("[FLASHER] Can't get tty params. tcgetattr failed, err={}", errno);
    return false;
  }

  /* Setting other Port Stuff */
  tty.c_cflag     |= ~PARENB;              /* disable parity */
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
  tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl
  tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL); // Disable any special handling of received bytes

  tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
  tty.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed

  tty.c_cc[VTIME] = 10;    // Wait for up to 1s (10 deciseconds), returning as soon as any data is received.
  tty.c_cc[VMIN] = 0;

  /* Set Baud Rate */
  cfsetspeed(&tty, (speed_t)B115200);

  if ( tcsetattr ( fd, TCSANOW, &tty ) != 0) {
    spdlog::error("[FLASHER] Setting device attributes failed, err={}", errno);
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

  int device, binary;
  uint8_t packet_buf[max_packet_size];
  uint8_t file_buf[flash_frame_max_data_length];
  size_t file_size = 0, total_bytes_read = 0;
  size_t bytes_read = 0, next_read = 0, to_send = 0;
  struct stat st;
  constexpr uint8_t max_payload = max_usb_cdc_transfer_size - flash_frame_header_length;

  constexpr uint8_t progres_bar_width = 25;
  std::array<char, progres_bar_width> progress_bar;
  progress_bar.fill(' ');
  bool disable_proggress = false;

  if(argc < 3)
  {
    spdlog::info("{}", usage);
    return -1;
  }

  if(argc == 4)
  {
    if(strcmp(argv[3], "info") == 0)
    {
      spdlog::set_level(spdlog::level::info);
    }else if(strcmp(argv[3], "debug") == 0)
    {
      spdlog::set_level(spdlog::level::debug);
      disable_proggress = true;
    }else if(strcmp(argv[3], "trace") == 0)
    {
      spdlog::set_level(spdlog::level::trace);
      disable_proggress = true;
    }else{
      spdlog::error("Invalid logging category level");
      spdlog::info("{}", usage);
      return -1;
    }
  }

  device = open(argv[1], O_RDWR);
  if(device == -1)
  {
    spdlog::error("[FLASHER] Can't open device");
    return -2;
  }
  std::thread(receive_msg, device).detach();

  if(!set_device_params(device))
  {
    spdlog::error("[FLASHER] Setting device params, failed");
    return -3;
  }

  binary = open(argv[2], O_RDONLY);
  if (binary == -1)
  {
    spdlog::error("[FLASHER] Can't open file.");
    return -2;
  }

  stat(argv[2], &st);
  file_size = st.st_size;
  spdlog::info("[FLASHER] Flashing binary {} of size {}", argv[2], file_size);

  // init packet
  if(!send_init_packet(device))
  {
    spdlog::error("[FLASHER] Sending init packet fail");
    finish = true;
    return -1;
  }

  if(!wait_for_response())
  {
    spdlog::error("[FLASHER] Waiting for init response failed");
    finish = true;
    return -1;
  }
  next_read = max_payload; 
  while(total_bytes_read < file_size)
  {
    bytes_read = read(binary, file_buf + to_send, next_read);
    spdlog::trace("[FLASHER] Read bytes {}", bytes_read);
    to_send += bytes_read;

    if(bytes_read == 0)
    {
      // we can get here only when the payload size is not alligned to 4
      if(to_send != 0)
      {
        while(to_send & 0b11){
          file_buf[to_send] = 0x0;
          to_send++;
        }
        if(!send_frame_with_correct_endian(device, flash_address, file_buf, to_send))
        {
          spdlog::error("[FLASHER] Sending frame packet failed");
          finish = true;
          return -4;
        }

        if(!wait_for_response())
        {
          spdlog::error("[FLASHER] Waiting for frame packet response failed");
          finish = true;
          return -1;
        }
      }
      break;
    }

    if(bytes_read < 0)
    {
      spdlog::error("[FLASHER] Data read general error {}", errno);
      if(!send_reset_packet(device))
      {
        spdlog::error("[FLASHER] Sending reset packet failed");
        finish = true;
        return -4;
      }
      if(!wait_for_response())
      {
        spdlog::error("[FLASHER] Waiting for reset packet response failed\n");
        finish = true;
        return -1;
      }
      return -2;
    }

   flash_address = start_flash_addr + total_bytes_read;
   total_bytes_read += bytes_read;

    if(bytes_read & 0b11) {
      next_read = next_read - bytes_read;
      continue;
    }
    else
      next_read = max_payload;

   if(!send_frame_with_correct_endian(device, flash_address, file_buf, to_send))
   {
     spdlog::error("[FLASHER] Sending frame packet failed");
     return  -4;
   }

   spdlog::trace("[FLASHER] Waiting for response...");
   if(!wait_for_response())
   {
     spdlog::error("[FLASHER] Waiting for frame packet response failed");
     finish = true;
     return -1;
   }
   if(!disable_proggress)
   {
     const uint8_t progress = static_cast<uint8_t>(total_bytes_read/static_cast<double>(file_size)*100);
     const uint8_t bar_percent = static_cast<uint8_t>(total_bytes_read/static_cast<double>(file_size)*25);
     std::fill_n(progress_bar.begin(), bar_percent, '#');
     printf("Progress[%25s] [%d%]\r", progress_bar.data(), progress);
     fflush(stdout);
   }
   to_send = 0;
  }

  if(!send_reset_packet(device))
  {
    spdlog::error("[FLASHER] Sending reset packet failed");
    finish = true;
    return -4;
  }

  if(!wait_for_response())
  {
    spdlog::error("[FLASHER] Waiting for reset packet response failed");
    finish = true;
    return -1;
  }

  spdlog::info("[FLASHER] Job Completed. Binary {} with size {} flashed", argv[2], file_size);
  finish = true;
  close(device);
}
