#include "publish_node.hpp"

PublishNode::PublishNode() : Node("publish_node") {
  RCLCPP_INFO(this->get_logger(), "Publish node has been started.");

  _running = true;

  // Declare parameters with default values
  this->declare_parameter<std::string>("cfg_path", "../radar/cfg/cfg_default_30fps.cfg");
  this->declare_parameter<std::string>("cli_port", "/dev/pts/2");
  this->declare_parameter<int>("cli_baudrate", 115200);
  this->declare_parameter<std::string>("data_port", "/dev/pts/6");
  this->declare_parameter<int>("data_baudrate", 921600);
  this->declare_parameter<std::string>("publish_topic", "/radar_3d_points");

  // Retrieve parameter values
  packet_parser_cfg_t cfg;
  this->get_parameter("cfg_path", cfg.cfg_path);
  this->get_parameter("cli_port", cfg.cli_port);
  this->get_parameter("cli_baudrate", cfg.cli_baudrate);
  this->get_parameter("data_port", cfg.data_port);
  this->get_parameter("data_baudrate", cfg.data_baudrate);
  cfg.cbfunc = std::bind(&PublishNode::radar_frame_cb, this, std::placeholders::_1);

  std::string publish_topic;
  this->get_parameter("publish_topic", publish_topic);

  try {
    _packet_parser = std::make_unique<PacketParser>(cfg);
  } catch (const std::exception& e) {
    std::cerr << "[PublishNode]: ERROR! Cannot open serial ports: " << e.what() << '\n';
    exit(1);
  }

  radar_publisher_ = this->create_publisher<sensor_msgs::msg::PointCloud2>(publish_topic, 10);

  _processing_thread = std::thread([this]() {
    while (_running) {
      std::unique_lock<std::mutex> lock(_mutex);
      _cv.wait(lock, [this] {
        std::lock_guard<std::mutex> qlock(queue_mutex);
        return !_running || !frame_queue.empty();
      });

      if (!_running) break;

      radar_frame_t frame;
      {
        std::lock_guard<std::mutex> qlock(queue_mutex);
        if (!frame_queue.empty()) {
          frame = frame_queue.front();
          frame_queue.pop();
        } else {
          continue;
        }
      }

      lock.unlock();
      publish_pointcloud(frame);
    }
  });
}


PublishNode::~PublishNode() {
  _running = false;
  _cv.notify_one();
  if (_processing_thread.joinable()) {
    _processing_thread.join();
  }
}

void PublishNode::radar_frame_cb(const radar_frame_t &radar_points) {
  {
    std::lock_guard<std::mutex> lock(queue_mutex);
    frame_queue.push(radar_points);
  }
  _cv.notify_one();
}

void PublishNode::publish_pointcloud(const radar_frame_t &radar_points) {
  sensor_msgs::msg::PointCloud2 cloud_msg;
  cloud_msg.header.stamp = this->get_clock()->now();
  cloud_msg.header.frame_id = "radar_frame";

  cloud_msg.height = 1;
  cloud_msg.width = radar_points.points.size();
  cloud_msg.is_dense = false;
  cloud_msg.is_bigendian = false;

  sensor_msgs::PointCloud2Modifier modifier(cloud_msg);
  modifier.setPointCloud2Fields(4, // number of fields
                                "x", 1, sensor_msgs::msg::PointField::FLOAT32,
                                "y", 1, sensor_msgs::msg::PointField::FLOAT32,
                                "z", 1, sensor_msgs::msg::PointField::FLOAT32,
                                "intensity", 1,
                                sensor_msgs::msg::PointField::FLOAT32);

  sensor_msgs::PointCloud2Iterator<float> iter_x(cloud_msg, "x");
  sensor_msgs::PointCloud2Iterator<float> iter_y(cloud_msg, "y");
  sensor_msgs::PointCloud2Iterator<float> iter_z(cloud_msg, "z");
  sensor_msgs::PointCloud2Iterator<float> iter_intensity(cloud_msg,
                                                         "intensity");

  for (const auto &point : radar_points.points) {
    *iter_x = point.x;
    *iter_y = point.y;
    *iter_z = point.z;
    *iter_intensity = point.snr; // or use point.intensity if you rename
    ++iter_x;
    ++iter_y;
    ++iter_z;
    ++iter_intensity;
  }
  radar_publisher_->publish(cloud_msg);
}

int main(int argc, char *argv[]) {
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<PublishNode>());
  rclcpp::shutdown();
  return 0;
}
