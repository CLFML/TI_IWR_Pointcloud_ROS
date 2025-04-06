# 🌟 **`publish_node` - Radar PointCloud Publisher**

Publishes 3D radar point cloud data as `sensor_msgs/PointCloud2` by interfacing with a custom radar parser (`PacketParser`). Ideal for integrating Texas Instruments radar sensors with ROS 2.

---

### ✅ Topics

| Topic               | Type                             | Description                             |
|---------------------|----------------------------------|-----------------------------------------|
| `/radar_3d_points`  | `sensor_msgs/msg/PointCloud2`    | 3D radar point cloud with SNR as intensity |

---

### ⚙️ Parameters

| Name             | Type     | Default Value                                  | Description                                |
|------------------|----------|------------------------------------------------|--------------------------------------------|
| `cfg_path`       | string   | `"../radar/cfg/cfg_default_30fps.cfg"`         | Path to the radar configuration file       |
| `cli_port`       | string   | `"/dev/pts/2"`                                 | Serial port for radar CLI control          |
| `cli_baudrate`   | int      | `115200`                                       | Baudrate for the CLI port                  |
| `data_port`      | string   | `"/dev/pts/6"`                                 | Serial port for radar data stream          |
| `data_baudrate`  | int      | `921600`                                       | Baudrate for the data port                 |
| `publish_topic`  | string   | `"/radar_3d_points"`                           | Topic name to publish the point cloud data |

---

### 📦 Message Notes

- `sensor_msgs/msg/PointCloud2` fields:
  - `x`, `y`, `z`: Cartesian coordinates (float32)
  - `intensity`: SNR value (float32), mapped from radar point's `snr`

---

### 🧩 Implementation Notes

- Built using **`rclcpp`**.
- Uses a **separate thread** for processing and publishing radar frames.
- Interfaces with a custom radar parser (`PacketParser`).
- Uses a **condition variable and mutex** for frame queue synchronization.
- Each received radar frame is converted into a ROS 2 `PointCloud2` message.
- The frame is published to a topic specified by a **configurable parameter**.

---

### 🏁 Launch Example

```bash
ros2 run ti_iwr_pointcloud publish_node
```

---

### 🧪 Dependencies

- `rclcpp`
- `sensor_msgs`
- `serial_cpp`
- Custom radar parser (PacketParser)
- Parameter server for dynamic config
- C++ standard threading and synchronization
