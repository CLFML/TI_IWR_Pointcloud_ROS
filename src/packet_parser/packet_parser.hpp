#ifndef PACKET_PARSER_HPP
#define PACKET_PARSER_HPP

#include <array>
#include <atomic>
#include <deque>
#include <filesystem>
#include <functional>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

#include <serial_cpp/serial.h>

/** 
 * @brief Represents a single radar detection point.
 */
struct radar_point_t {
  float x;       /**< X coordinate (meters) */
  float y;       /**< Y coordinate (meters) */
  float z;       /**< Z coordinate (meters) */
  float snr;     /**< Signal-to-noise ratio */
  float noise;   /**< Noise level */
  float velocity;/**< Velocity (m/s) */
};

/** 
 * @brief Represents a complete radar frame with various matrices and point clouds.
 */
struct radar_frame_t {
  std::vector<radar_point_t> points;              /**< List of detected points */
  std::vector<float> RA_Mat_F;                    /**< Range-Angle Matrix (Forward) */
  std::vector<float> RA_Mat_R;                    /**< Range-Angle Matrix (Rear) */
  std::vector<float> RD_Mat;                      /**< Range-Doppler Matrix */
  std::vector<std::vector<float>> target_list;    /**< List of tracked targets [target][9 values] */
  std::vector<float> target_cov_matrix;           /**< Covariance matrix (4 floats) */
};

/** 
 * @brief Configuration parameters for initializing the PacketParser.
 */
struct packet_parser_cfg_t {
  std::string cli_port;                                         /**< Serial port for CLI communication */
  uint32_t cli_baudrate;                                        /**< CLI port baudrate */
  std::string data_port;                                        /**< Serial port for radar data */
  uint32_t data_baudrate;                                       /**< Data port baudrate */
  std::filesystem::path cfg_path;                               /**< Path to radar configuration file */
  std::function<void(const radar_frame_t &frame)> cbfunc = nullptr; /**< Callback for parsed radar frames */
};

/** 
 * @brief Class responsible for reading and parsing radar data packets.
 */
class PacketParser {
public:
  /** 
   * @brief Constructs a PacketParser with the provided configuration.
   * @param cfg Configuration struct containing serial port info and callback.
   */
  PacketParser(packet_parser_cfg_t &cfg);

  ~PacketParser();

private:
  std::thread serial_thread_;          /**< Thread for reading from serial port */
  std::atomic<bool> _running;         /**< Flag to control reading loop */
  bool _serial_ports_opened = false;  /**< Indicates if serial ports were successfully opened */

  serial_cpp::Serial _serial_cmd; 
  serial_cpp::Serial _serial_data;

  std::filesystem::path _cfg_file;
  packet_parser_cfg_t _cfg;       
  bool _radar_cfg_valid = false;   
  bool _cfg_mode = false;           


  /** 
   * @brief Radar profile settings as extracted from configuration.
   */
  struct radar_profile_settings_t {
    float start_freq;             /**< Start frequency (GHz) */
    float idle_time;              /**< Idle time (us) */
    float adc_start_time;         /**< ADC start time (us) */
    float ramp_end_time;          /**< Ramp end time (us) */
    float freq_slope_const;       /**< Frequency slope constant (MHz/us) */
    int num_adc_samples;          /**< Number of ADC samples */
    float dig_out_sample_rate;    /**< Digital output sample rate (ksps) */
  } _profile_settings;

  /** 
   * @brief Frame configuration settings for the radar.
   */
  struct radar_frame_settings_t {
    int ntx;                      /**< Number of transmit antennas */
    int num_chirp_loop;          /**< Number of chirps per loop */
    int ms_per_frame;            /**< Milliseconds per radar frame */
  } _frame_settings;

  /** 
   * @brief Derived radar session parameters.
   */
  struct radar_session_params_t {
    double adc_duration;              /**< Duration of ADC window */
    double bandwidth;                 /**< Bandwidth (Hz) */
    double pulse_repetition_interval;/**< Time between chirps (s) */
    int range_fft_size;              /**< Size of FFT for range */
    int range_doppler_size;          /**< Size of FFT for Doppler */
    double range_resolution;         /**< Range resolution (m) */
    double range_max;                /**< Maximum detectable range (m) */
    double vel_max;                  /**< Maximum relative velocity (m/s) */
    double vel_abs_max;              /**< Absolute maximum velocity (m/s) */
    double vel_resolution;           /**< Velocity resolution (m/s) */
  } _session_params;


  std::vector<uint8_t> _buffer;       /**< Buffer for incoming serial data */
  size_t _read_offset = 0;            /**< Offset in buffer for parsing */

  static constexpr uint32_t _speed_of_light = 299792458; /**< Speed of light in m/s */
  static constexpr std::array<uint8_t, 8> _magic_word = {
      0x02, 0x01, 0x04, 0x03, 0x06, 0x05, 0x08, 0x07};   /**< Magic word for packet start */


  /** 
   * @brief Applies radar configuration using the provided config file.
   * @return 0 on success, non-zero on failure.
   */
  int configure_radar_with_cfg_file();

  /** 
   * @brief Main loop for reading serial data packets.
   */
  void serial_loop();

  /** 
   * @brief Processes the current buffer for complete radar packets.
   */
  void process_buffer();

  /** 
   * @brief Parses a complete radar frame from a data buffer.
   * @param buffer Raw data buffer containing a radar frame.
   * @return Parsed radar_frame_t object.
   */
  radar_frame_t parse_radar_frame(const std::vector<uint8_t> &buffer);

  /** 
   * @brief Finds the magic word in a buffer starting from an offset.
   * @param buf The input buffer.
   * @param offset Start searching from this offset.
   * @return Index of the magic word or std::string::npos if not found.
   */
  size_t find_magic_word(const std::vector<uint8_t> &buf, size_t offset);

  /** 
   * @brief Tokenizes a line into a vector of strings.
   * @param line Input string line.
   * @param minimum_tokens Minimum number of tokens required.
   * @return Vector of string tokens.
   */
  const std::vector<std::string> tokenize_line(const std::string &line,
                                               size_t minimum_tokens);

  /** 
   * @brief Checks if a line contains a given keyword.
   * @param line The line to inspect.
   * @param keyword The keyword to search for.
   * @return True if keyword is found, false otherwise.
   */
  bool line_contains_keyword(const std::string &line,
                             const std::string keyword);

  /** 
   * @brief Parses and sets profile configuration from a line.
   * @param line Configuration line string.
   * @return True if successfully parsed, false otherwise.
   */
  bool set_profileconfig_from_cfg_line(const std::string &line);

  /** 
   * @brief Parses and sets frame configuration from a line.
   * @param line Configuration line string.
   * @return True if successfully parsed, false otherwise.
   */
  bool set_frameconfig_from_cfg_line(const std::string &line);

  /** 
   * @brief Parses detected points TLV from raw data.
   * @param data Input binary data of TLV.
   * @param frame Output radar frame structure to populate.
   */
  void parse_detected_points_tlv(const std::vector<uint8_t> &data,
                                 radar_frame_t &frame);

  /** 
   * @brief Parses side information TLV from raw data.
   * @param data Input binary data of TLV.
   * @param frame Output radar frame structure to populate.
   */
  void parse_side_info_tlv(const std::vector<uint8_t> &data,
                           radar_frame_t &frame);
};

#endif // PACKET_PARSER_HPP
