cmake_minimum_required(VERSION 3.1...3.14)
project(ti_iwr_pointcloud C CXX)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(builtin_interfaces REQUIRED)
find_package(rosidl_default_generators REQUIRED)
find_package(sensor_msgs REQUIRED)

include(FetchContent)

FetchContent_Declare(
   Serial
   GIT_REPOSITORY https://github.com/ami-iit/serial_cpp.git
   GIT_TAG v1.3.0
)
FetchContent_MakeAvailable(Serial)


add_executable(publish_node src/publish_node.cpp
                            src/packet_parser/packet_parser.cpp
                            src/packet_parser/serial_data_parser.cpp
                            src/packet_parser/serial_cmd_parser.cpp
                            src/packet_parser/tlv_parser.cpp)

ament_target_dependencies(publish_node
   rclcpp
   sensor_msgs
)
target_link_libraries(publish_node "${cpp_typesupport_target}" serial_cpp::serial_cpp)
target_include_directories(publish_node PUBLIC src/)

if(UNIX AND NOT APPLE)  # Linux only
 target_compile_options(publish_node PRIVATE -Wall)
endif()

install(TARGETS
publish_node
DESTINATION lib/${PROJECT_NAME}) 

ament_package()
