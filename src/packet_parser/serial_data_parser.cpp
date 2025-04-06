#include "packet_parser.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

void PacketParser::serial_loop() {
  constexpr size_t kReadSize = 2048;

  while (_running) {
    std::vector<uint8_t> chunk;
    size_t bytes_read = _serial_data.read(chunk, kReadSize);
    if (bytes_read == 0) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      continue;
    }
    _buffer.insert(_buffer.end(), chunk.begin(), chunk.begin() + bytes_read);

    process_buffer();
  }
}

size_t PacketParser::find_magic_word(const std::vector<uint8_t> &buf,
                                     size_t offset) {
  if (buf.size() < offset + _magic_word.size())
    return std::string::npos;

  auto it = std::search(buf.begin() + offset, buf.end(), _magic_word.begin(),
                        _magic_word.end());

  return (it != buf.end()) ? std::distance(buf.begin(), it) : std::string::npos;
}

void PacketParser::process_buffer() {
  while (true) {
    size_t start = find_magic_word(_buffer, _read_offset);
    if (start == std::string::npos) {
      // Too much stale data? Clean up
      if (_buffer.size() > 4096) {
        _buffer.erase(_buffer.begin(), _buffer.end() - _magic_word.size());
      }
      _read_offset = 0;
      return;
    }

    // Minimum valid header size
    if (_buffer.size() < start + 36)
      return;

    uint32_t packet_length = 0;
    std::memcpy(&packet_length, &_buffer[start + 12], sizeof(uint32_t));

    if (packet_length < 36 || packet_length > 65536) {
      std::cerr << "Suspicious packet length: " << packet_length
                << ", skipping magic.\n";
      _read_offset = start + _magic_word.size();
      continue;
    }

    if (_buffer.size() < start + packet_length)
      return;

    std::vector<uint8_t> frame(_buffer.begin() + start,
                               _buffer.begin() + start + packet_length);
    _read_offset = start + packet_length;

    try {
      radar_frame_t parsed = parse_radar_frame(frame);
      _cfg.cbfunc(parsed);
    } catch (const std::exception &e) {
      std::cerr << "Frame parse error: " << e.what() << '\n';
    }

    // Compact buffer
    if (_read_offset > 1024 || _read_offset > _buffer.size() / 2) {
      size_t remaining = _buffer.size() - _read_offset;
      std::memmove(_buffer.data(), _buffer.data() + _read_offset, remaining);
      _buffer.resize(remaining);
      _read_offset = 0;
    }
  }
}
