#include "packet_parser.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

PacketParser::PacketParser(packet_parser_cfg_t &cfg)
    : _running(true), _serial_ports_opened(true),
      _serial_cmd(cfg.cli_port, cfg.cli_baudrate),
      _serial_data(cfg.data_port, cfg.data_baudrate), _cfg_file(cfg.cfg_path),
      _cfg(cfg)

{

  if (!std::filesystem::exists(cfg.cfg_path) || !cfg.cfg_path.has_extension() ||
      cfg.cfg_path.extension() != ".cfg") {
    std::cerr << "Invalid config file: " << cfg.cfg_path << '\n';
    return;
  }

  configure_radar_with_cfg_file();
  serial_thread_ = std::thread(&PacketParser::serial_loop, this);
}

PacketParser::~PacketParser() {
  _running = false;
  if (serial_thread_.joinable()) {
    serial_thread_.join();
  }
  _serial_cmd.close();
  _serial_data.close();
}
