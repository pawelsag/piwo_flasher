#include "protodef.hpp"

#include <optional>

class raw_packet
{
public:
  explicit raw_packet(usb_byte_t* packet, const size_t& size)
    : _packet(packet)
    , _size(size)
  {
  }

  raw_packet(const raw_packet& packet)
    : _packet(packet._packet)
    , _size(packet._size)
  {
  }

  size_t
  size() const
  {
    return _size;
  }

  usb_byte_t*
  data() const
  {
    return _packet;
  }

  usb_byte_t*
  begin() const
  {
    return _packet;
  }

  usb_byte_t*
  end() const
  {
    return _packet + _size;
  }

  usb_byte_t*
  cdata() const
  {
    return _packet;
  }

  const usb_byte_t*
  cbegin() const
  {
    return _packet;
  }

  const usb_byte_t*
  cend() const
  {
    return _packet + _size;
  }

  std::optional<packet_type>
  get_type() const noexcept
  {
    switch (static_cast<packet_type>(_packet[common_type_pos]))
    {
      case packet_type::INIT:
      case packet_type::FRAME:
      case packet_type::RESET:
      case packet_type::RESPONSE:
      case packet_type::MSG:
        return static_cast<packet_type>(_packet[common_type_pos]);
      default:
        return std::nullopt;
        break;
    }
    return std::nullopt;
  }

protected:
  usb_byte_t* _packet;
  size_t      _size;
};

class flash_init_builder
{
public:
  friend class flash_init;

  static std::optional<flash_init_builder>
  make_flash_init_builder(raw_packet packet)
  {
    auto packet_buffer_size = static_cast<size_t>(packet.size());

    if (packet_buffer_size < flash_init_length)
    {
      return std::nullopt;
    }

    packet.data()[common_length_pos] = usb_byte_t{ flash_init_length };
    packet.data()[common_type_pos] =
      usb_byte_t{ static_cast<uint8_t>(packet_type::INIT) };

    return flash_init_builder(packet);
  }

private:
  explicit flash_init_builder(raw_packet packet) noexcept
    : _raw_packet(packet)
  {
  }

  raw_packet _raw_packet;
};

class flash_init
  : public raw_packet
{
public:
  explicit flash_init(flash_init_builder builder)
    : raw_packet(builder._raw_packet)
  {
  }

  size_t
  get_lenght() const noexcept
  {
    return static_cast<size_t>(this->cdata()[common_length_pos]);
  }

  size_t
  size() const
  {
    return flash_init_length;
  }

  usb_byte_t*
  end() const
  {
    return this->cdata() + flash_init_length;
  }

  const usb_byte_t*
  cend() const
  {
    return this->cdata() + flash_init_length;
  }

  static std::optional<flash_init>
  make_flash_init(raw_packet packet)
  {
    auto packet_buffer_size = static_cast<size_t>(packet.size());

    if (packet_buffer_size < flash_init_length ||
        static_cast<uint8_t>(packet.cdata()[common_length_pos]) != flash_init_length ||
        !packet.get_type().has_value() || packet.get_type() != packet_type::INIT)
    {
      return std::nullopt;
    }
    return flash_init(packet);
  }

private:
  explicit flash_init(raw_packet packet) noexcept
    : raw_packet(packet)
  {
  }
};

class flash_frame_builder
{
public:
  friend class flash_frame;

  bool
  append_data(const uint8_t *data, uint8_t size) noexcept
  {
    // cannot append data with size that don't divied by 4
    if(size & 0b11)
      return false;

    if(flash_frame_max_data_length < current_size+size)
      return false;
    // copy new data
    std::copy_n(reinterpret_cast<const usb_byte_t*>(data),
                size,
                this->_raw_packet.begin() + current_size + flash_frame_header_length);
    // calculate checksum
    for(uint8_t i =0; i < size; i++) 
      this->_raw_packet.data()[flash_frame_checksum_pos] ^= data[i];

    // calculate new size and assign it
    current_size += size;

    std::copy_n(reinterpret_cast<const usb_byte_t*>(&current_size),
                flash_frame_data_size,
                this->_raw_packet.begin() + flash_frame_payload_size_pos);

    return true;
  }

  void
  set_flash_addr(const uint32_t addr) noexcept
  {
    std::copy_n(reinterpret_cast<const usb_byte_t*>(&addr),
                flash_frame_addr_size,
                this->_raw_packet.begin() + flash_frame_addr_pos);
  }


  static std::optional<flash_frame_builder>
  make_flash_frame_builder(raw_packet packet)
  {
    auto packet_buffer_size = static_cast<size_t>(packet.size());

    if (packet_buffer_size < flash_frame_header_length ||  packet_buffer_size > flash_frame_max_length)
    {
      return std::nullopt;
    }

    packet.data()[common_length_pos] = static_cast<usb_byte_t>( packet_buffer_size );
    packet.data()[common_type_pos] =
      usb_byte_t{ static_cast<uint8_t>(packet_type::FRAME) };

    packet.data()[flash_frame_checksum_pos] = usb_byte_t{0};

    return flash_frame_builder(packet);
  }

private:
  explicit flash_frame_builder(raw_packet packet) noexcept
    : _raw_packet(packet)
  {
  }

  raw_packet _raw_packet;
  size_t current_size = 0;
};

class flash_frame
  : public raw_packet
{
public:
  explicit flash_frame(flash_frame_builder builder)
    : raw_packet(builder._raw_packet)
  {
  }

  size_t
  get_lenght() const noexcept
  {
    return static_cast<size_t>(this->cdata()[common_length_pos]);
  }

  const usb_byte_t*
  get_payload() const noexcept
  {
    return this->cdata() + flash_frame_payload_pos;
  }

  uint8_t
  get_payload_size()
  {
    return static_cast<uint8_t>(this->cdata()[flash_frame_payload_size_pos]);
  }

  uint8_t
  get_checksum()
  {
    return static_cast<uint8_t>(this->cdata()[flash_frame_checksum_pos]);
  }

  uint32_t
  get_addr() const noexcept
  {
    uint32_t addr =0;
    std::copy_n(this->cdata() + flash_frame_addr_pos,
                flash_frame_addr_size,
                reinterpret_cast<usb_byte_t*>(&addr));
    return addr;
  }

  addr_raw_t
  get_addr_raw() const noexcept
  {
    addr_raw_t addr;
    std::copy_n(this->cdata() + flash_frame_addr_pos,
                flash_frame_addr_size,
                reinterpret_cast<usb_byte_t*>(&addr));
    return addr;
  }

  size_t
  size() const
  {
    return static_cast<size_t>(this->cdata()[common_length_pos]);
  }

  usb_byte_t*
  end() const
  {
    uint8_t length = this->get_lenght();
    return this->cdata() + length;
  }

  const usb_byte_t*
  cend() const
  {
    uint8_t length = this->get_lenght();
    return this->cdata() + length;
  }

  static std::optional<flash_frame>
  make_flash_frame(raw_packet packet)
  {
    auto packet_buffer_size = static_cast<size_t>(packet.size());

    if (packet_buffer_size < flash_frame_header_length ||
        !packet.get_type().has_value() || packet.get_type() != packet_type::FRAME)
    {
      return std::nullopt;
    }
    return flash_frame(packet);
  }

private:
  explicit flash_frame(raw_packet packet) noexcept
    : raw_packet(packet)
  {
  }
};

class flash_reset_builder
{
public:
  friend class flash_reset;

  static std::optional<flash_reset_builder>
  make_flash_reset_builder(raw_packet packet)
  {
    auto packet_buffer_size = static_cast<size_t>(packet.size());

    if (packet_buffer_size < flash_reset_length)
    {
      return std::nullopt;
    }

    packet.data()[common_length_pos] = usb_byte_t{ flash_reset_length };
    packet.data()[common_type_pos] =
      usb_byte_t{ static_cast<uint8_t>(packet_type::RESET) };

    return flash_reset_builder(packet);
  }

private:
  explicit flash_reset_builder(raw_packet packet) noexcept
    : _raw_packet(packet)
  {
  }

  raw_packet _raw_packet;
};

class flash_reset
  : public raw_packet
{
public:
  explicit flash_reset(flash_reset_builder builder)
    : raw_packet(builder._raw_packet)
  {
  }

  size_t
  get_lenght() const noexcept
  {
    return static_cast<size_t>(this->cdata()[common_length_pos]);
  }

  size_t
  size() const
  {
    return flash_reset_length;
  }

  usb_byte_t*
  end() const
  {
    return this->cdata() + flash_reset_length;
  }

  const usb_byte_t*
  cend() const
  {
    return this->cdata() + flash_reset_length;
  }

  static std::optional<flash_reset>
  make_flash_reset(raw_packet packet)
  {
    auto packet_buffer_size = static_cast<size_t>(packet.size());

    if (packet_buffer_size < flash_reset_length ||
        static_cast<uint8_t>(packet.cdata()[common_length_pos]) != flash_reset_length ||
        !packet.get_type().has_value() || packet.get_type() != packet_type::RESET)
    {
      return std::nullopt;
    }
    return flash_reset(packet);
  }

private:
  explicit flash_reset(raw_packet packet) noexcept
    : raw_packet(packet)
  {
  }
};

class flash_response_builder
{
public:
  friend class flash_response;

  static std::optional<flash_response_builder>
  make_flash_response_builder(raw_packet packet)
  {
    auto packet_buffer_size = static_cast<size_t>(packet.size());

    if (packet_buffer_size < flash_response_length)
    {
      return std::nullopt;
    }

    packet.data()[common_length_pos] = usb_byte_t{ flash_response_length };
    packet.data()[common_type_pos] =
      usb_byte_t{ static_cast<uint8_t>(packet_type::RESPONSE) };

    return flash_response_builder(packet);
  }

  void
  set_response(const flash_response_type response) noexcept
  {
    this->_raw_packet.data()[flash_response_type_pos] =
      usb_byte_t{ static_cast<uint8_t>(response) };
  }

private:
  explicit flash_response_builder(raw_packet packet) noexcept
    : _raw_packet(packet)
  {
  }

  raw_packet _raw_packet;
};

class flash_response
  : public raw_packet
{
public:
  explicit flash_response(flash_response_builder builder)
    : raw_packet(builder._raw_packet)
  {
  }

  size_t
  get_lenght() const noexcept
  {
    return static_cast<size_t>(this->cdata()[common_length_pos]);
  }

  size_t
  size() const
  {
    return flash_response_length;
  }

  usb_byte_t*
  end() const
  {
    return this->cdata() + flash_response_length;
  }

  const usb_byte_t*
  cend() const
  {
    return this->cdata() + flash_response_length;
  }

  flash_response_type
  get_response() noexcept
  {
    return static_cast<flash_response_type>(this->data()[flash_response_type_pos]);
  }

  static std::optional<flash_response>
  make_flash_response(raw_packet packet)
  {
    auto packet_buffer_size = static_cast<size_t>(packet.size());

    if (packet_buffer_size < flash_response_length ||
        static_cast<uint8_t>(packet.cdata()[common_length_pos]) != flash_response_length ||
        !packet.get_type().has_value() || packet.get_type() != packet_type::RESPONSE)
    {
      return std::nullopt;
    }
    return flash_response(packet);
  }

private:
  explicit flash_response(raw_packet packet) noexcept
    : raw_packet(packet)
  {
  }
};

class flash_msg_builder
{
public:
  friend class flash_msg;

  bool
  copy_msg(const uint8_t *data, uint8_t size) noexcept
  {
    if(flash_msg_payload_length < size)
      return false;

    std::copy_n(reinterpret_cast<const usb_byte_t*>(data),
                size,
                this->_raw_packet.begin() + flash_msg_payload_pos);

    this->_raw_packet.data()[flash_msg_payload_size_pos] = usb_byte_t{size};

    return true;
  }

  static std::optional<flash_msg_builder>
  make_flash_msg_builder(raw_packet packet)
  {
    auto packet_buffer_size = static_cast<size_t>(packet.size());

    if (packet_buffer_size < flash_msg_header_length ||  packet_buffer_size > flash_msg_length)
    {
      return std::nullopt;
    }

    packet.data()[common_length_pos] = static_cast<usb_byte_t>( packet_buffer_size );
    packet.data()[common_type_pos] =
      usb_byte_t{ static_cast<uint8_t>(packet_type::MSG) };

    return flash_msg_builder(packet);
  }

private:
  explicit flash_msg_builder(raw_packet packet) noexcept
    : _raw_packet(packet)
  {
  }

  raw_packet _raw_packet;
};

class flash_msg
  : public raw_packet
{
public:
  explicit flash_msg(flash_msg_builder builder)
    : raw_packet(builder._raw_packet)
  {
  }

  size_t
  get_lenght() const noexcept
  {
    return static_cast<size_t>(this->cdata()[common_length_pos]);
  }

  const usb_byte_t*
  get_msg() const noexcept
  {
    return this->cdata() + flash_msg_payload_pos;
  }

  uint8_t
  get_msg_size()
  {
    return static_cast<uint8_t>(this->cdata()[flash_msg_payload_size_pos]);
  }

  size_t
  size() const
  {
    return static_cast<size_t>(this->cdata()[common_length_pos]);
  }

  usb_byte_t*
  end() const
  {
    uint8_t length = this->get_lenght();
    return this->cdata() + length;
  }

  const usb_byte_t*
  cend() const
  {
    uint8_t length = this->get_lenght();
    return this->cdata() + length;
  }

  static std::optional<flash_msg>
  make_flash_msg(raw_packet packet)
  {
    auto packet_buffer_size = static_cast<size_t>(packet.size());

    if (packet_buffer_size < flash_msg_header_length ||
        !packet.get_type().has_value() || packet.get_type() != packet_type::MSG)
    {
      return std::nullopt;
    }
    return flash_msg(packet);
  }

private:
  explicit flash_msg(raw_packet packet) noexcept
    : raw_packet(packet)
  {
  }
};
