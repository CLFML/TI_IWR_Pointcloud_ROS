#ifndef PUBLISH_NODE_HPP
#define PUBLISH_NODE_HPP

#include <packet_parser/packet_parser.hpp>
#include <sensor_msgs/msg/point_cloud2.hpp>
#include <sensor_msgs/point_cloud2_iterator.hpp>
#include <rclcpp/rclcpp.hpp>

class PublishNode : public rclcpp::Node {
  public:
    PublishNode();
    ~PublishNode();
  
  private:
    void radar_frame_cb(const radar_frame_t& radar_points);
    void publish_pointcloud(const radar_frame_t& radar_points);
  
    std::unique_ptr<PacketParser> _packet_parser;
    std::thread _processing_thread;
    bool _running = true;
  
    std::queue<radar_frame_t> frame_queue;
    std::mutex queue_mutex;
    std::mutex _mutex;
    std::condition_variable _cv;
  
    rclcpp::Publisher<sensor_msgs::msg::PointCloud2>::SharedPtr radar_publisher_;
  };
  

#endif /* PUBLISH_NODE_HPP */