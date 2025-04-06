#include "packet_parser.hpp"
#include <iostream>

template <typename T>
T unpack(const std::vector<uint8_t> &buffer, size_t &offset) {
  if (offset + sizeof(T) > buffer.size())
    throw std::runtime_error("Buffer overrun");
  T value;
  std::memcpy(&value, &buffer[offset], sizeof(T));
  offset += sizeof(T);
  return value;
}

void PacketParser::parse_detected_points_tlv(const std::vector<uint8_t> &data,
                                             radar_frame_t &frame) {
  size_t offset = 0;
  size_t point_struct_size = 4 * sizeof(float); // x, y, z, velocity
  size_t num_points = data.size() / point_struct_size;

  for (size_t i = 0; i < num_points; ++i) {
    radar_point_t pt;
    pt.x = unpack<float>(data, offset);
    pt.y = unpack<float>(data, offset);
    pt.z = unpack<float>(data, offset);
    pt.velocity = unpack<float>(data, offset);
    frame.points.push_back(pt);
  }
}

void PacketParser::parse_side_info_tlv(const std::vector<uint8_t> &data,
                                       radar_frame_t &frame) {
  size_t offset = 0;
  size_t num_points = frame.points.size();

  for (size_t i = 0;
       i < num_points && offset + sizeof(uint16_t) * 2 <= data.size(); ++i) {
    uint16_t snr = unpack<uint16_t>(data, offset);
    uint16_t noise = unpack<uint16_t>(data, offset);
    frame.points[i].snr = static_cast<float>(snr) / 10.0f;     // SNR in dB
    frame.points[i].noise = static_cast<float>(noise) / 10.0f; // SNR in dB
  }
}

radar_frame_t
PacketParser::parse_radar_frame(const std::vector<uint8_t> &buffer) {
  radar_frame_t frame;

  if (buffer.size() < 44) {
    std::cerr << "Frame too small to be valid\n";
    return frame;
  }

  size_t offset = 8; // Skip magic word

#if DEBUG
  uint32_t version = unpack<uint32_t>(buffer, offset);
  uint32_t packet_length = unpack<uint32_t>(buffer, offset);
  uint32_t platform = unpack<uint32_t>(buffer, offset);
  uint32_t frame_number = unpack<uint32_t>(buffer, offset);
  uint32_t cpu_cycles = unpack<uint32_t>(buffer, offset);
  uint32_t num_detected_objs = unpack<uint32_t>(buffer, offset);
  uint32_t num_tlv = unpack<uint32_t>(buffer, offset);
  uint32_t subframe_idx = unpack<uint32_t>(buffer, offset);

  std::cout << "Header Info -- "
            << "Version: " << version << ", Packet Length: " << packet_length
            << ", Platform: " << platform << ", Frame #: " << frame_number
            << ", CPU Cycles: " << cpu_cycles
            << ", Detected objs: " << num_detected_objs
            << ", Num tlv: " << num_tlv << ", Subframe idx: " << subframe_idx
            << '\n';
#else
  offset += 24;
  uint32_t num_tlv = unpack<uint32_t>(buffer, offset);
  offset += 4;
#endif
  for (uint32_t i = 0; i < num_tlv; ++i) {
    if (offset + 8 > buffer.size()) {
      std::cerr << "Incomplete TLV header at TLV " << i << '\n';
      break;
    }
    // Read TLV type and length
    uint32_t tlv_type = unpack<uint32_t>(buffer, offset);
    uint32_t tlv_length = unpack<uint32_t>(buffer, offset);

#if DEBUG
    std::cout << "TLV #" << i << " | Type: " << tlv_type
              << ", Length: " << tlv_length << '\n';
#endif
    // Check if there's enough data for the value
    if (offset + (tlv_length) > buffer.size()) {
      std::cerr << "TLV value extends beyond buffer size!\n";
      break;
    }

    std::vector<uint8_t> tlv_data(buffer.begin() + offset,
                                  buffer.begin() + offset + (tlv_length));

    switch (tlv_type) {
    case 1020:
      parse_detected_points_tlv(tlv_data, frame);
      break;
    case 1021:
      parse_side_info_tlv(tlv_data, frame);
      break;
    default:
      std::cerr << "Unknown TLV type: " << tlv_type << '\n';
      break;
    }
    offset += tlv_length;
  }

  return frame;
}