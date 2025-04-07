#include "packet_parser.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

const std::vector<std::string>
PacketParser::tokenize_line(const std::string &line,
                            const size_t minimum_tokens) {
  std::istringstream iss(line);
  std::vector<std::string> tokens;
  std::string token;

  // Tokenize by whitespace
  while (iss >> token) {
    tokens.push_back(token);
  }
  // Ensure all required fields are present
  if (tokens.size() <= minimum_tokens) {
    throw std::runtime_error(
        "Incomplete 'profileCfg' line: expected at least " +
        std::to_string(minimum_tokens) + " tokens");
  }

  return tokens;
}

bool PacketParser::line_contains_keyword(const std::string &line,
                                         const std::string keyword) {
  size_t pos = line.find(keyword);
  if (pos != std::string::npos) {
    size_t end_of_word = pos + std::string(keyword).length();

    // Check for space after the word
    if (end_of_word < line.length() && line[end_of_word] == ' ') {
      return true;
    }
  }
  return false;
}

bool PacketParser::set_profileconfig_from_cfg_line(const std::string &line) {
  try {
    std::vector<std::string> tokens = tokenize_line(line, 12);
    _profile_settings.start_freq = std::stof(tokens[2]);
    _profile_settings.idle_time = std::stof(tokens[3]);
    _profile_settings.adc_start_time = std::stof(tokens[4]);
    _profile_settings.ramp_end_time = std::stof(tokens[5]);
    _profile_settings.freq_slope_const = std::stof(tokens[8]);
    _profile_settings.num_adc_samples = std::stoi(tokens[10]);
    _profile_settings.dig_out_sample_rate = std::stof(tokens[11]);
  } catch (const std::exception &e) {
    throw std::runtime_error("Failed to parse values from 'profileCfg' line: " +
                             std::string(e.what()));
    return false;
  }
  return true;
}

bool PacketParser::set_frameconfig_from_cfg_line(const std::string &line) {
  try {
    std::vector<std::string> tokens = tokenize_line(line, 6);
    _frame_settings.ntx = std::stof(tokens[2]) - std::stof(tokens[1]) + 1;
    _frame_settings.num_chirp_loop = std::stof(tokens[3]);
    _frame_settings.ms_per_frame = std::stof(tokens[5]);
    std::cout << "ntx: " << _frame_settings.ntx
              << " num_chirp_loop: " << _frame_settings.num_chirp_loop
              << " ms_per_frame: " << _frame_settings.ms_per_frame << '\n';
  } catch (const std::exception &e) {
    throw std::runtime_error("Failed to parse values from 'profileCfg' line: " +
                             std::string(e.what()));
    return false;
  }
  return true;
}

int PacketParser::configure_radar_with_cfg_file() {
  std::fstream cfg;
  cfg.open(_cfg_file, std::ios_base::in);
  if (!cfg.is_open()) {
    std::cerr << "Failed to open file: " << _cfg_file << std::endl;
    return 1;
  }

  std::vector<std::string> ti_config;
  std::string line;

  bool valid_frame_config = false;
  bool valid_profile_config = false;
  _radar_cfg_valid = false;
  while (std::getline(cfg, line)) {
    // Remove trailing \r and \n
    line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
    line.erase(std::remove(line.begin(), line.end(), '\n'), line.end());

    if (!line.empty() && line[0] != '%') {
      ti_config.push_back(line);
    }

    if (line_contains_keyword(line, "profileCfg")) {
      valid_profile_config = set_profileconfig_from_cfg_line(line);
    } else if (line_contains_keyword(line, "frameCfg")) {
      valid_frame_config = set_frameconfig_from_cfg_line(line);
    }
  }

  // Check if filename (without extension) contains "tracking"
  _cfg_mode = _cfg_file.stem().string().find("tracking") != std::string::npos;
  if (valid_frame_config && valid_profile_config) {
    _radar_cfg_valid = true;
    for (const auto &cmd : ti_config) {
      _serial_cmd.write(cmd + "\n");
      _serial_cmd.flush();
      std::cout << _serial_cmd.readline() << '\n';
      std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
  }
  _session_params.adc_duration = _profile_settings.num_adc_samples /
                                 (_profile_settings.dig_out_sample_rate * 1e3);
  _session_params.bandwidth =
      _session_params.adc_duration * _profile_settings.freq_slope_const * 1e12;
  _session_params.pulse_repetition_interval =
      (_profile_settings.idle_time + _profile_settings.ramp_end_time) * 1e-6;
  _session_params.range_fft_size = static_cast<int>(
      std::pow(2, std::ceil(std::log2(_profile_settings.num_adc_samples))));
  _session_params.range_doppler_size = static_cast<int>(
      std::pow(2, std::ceil(std::log2(_frame_settings.num_chirp_loop))));
  _session_params.range_resolution =
      _speed_of_light / (2 * _session_params.bandwidth);
  _session_params.range_max =
      _session_params.range_resolution * _session_params.range_fft_size;
  double center_freq_hz = _profile_settings.start_freq * 1e9 +
                          (_profile_settings.freq_slope_const * 1e12) *
                              (_profile_settings.adc_start_time * 1e-6 +
                               _session_params.adc_duration / 2);

  _session_params.vel_max =
      _speed_of_light /
      (2 * center_freq_hz * _session_params.pulse_repetition_interval) /
      _frame_settings.ntx;

  _session_params.vel_abs_max = _session_params.vel_max / 2;
  _session_params.vel_resolution =
      _session_params.vel_max / _session_params.range_doppler_size;
  return 0;
}
