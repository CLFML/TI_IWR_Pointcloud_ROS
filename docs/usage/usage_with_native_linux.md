# 🐧 Getting Started with Native ROS

Prefer a traditional system-wide install? Use the prebuilt `.deb` package for **Ubuntu Noble / ROS 2 Jazzy**.

---

## 📦 Install package via `.deb`

Install the latest `.deb` package directly from [Releases](https://github.com/CLFML/TI_IWR_Pointcloud_ROS/releases):

```bash
curl -s https://api.github.com/repos/CLFML/TI_IWR_Pointcloud_ROS/releases/latest \
  | grep "browser_download_url.*deb" \
  | cut -d : -f 2,3 \
  | tr -d \" \
  | wget -qi -
sudo dpkg -i ./ros-jazzy-*.deb
```

---

## ✅ Run the Node

Make sure ROS 2 is sourced:

```bash
source /opt/ros/jazzy/setup.sh
```

Then launch the radar point cloud publisher:

```bash
ros2 run ti_iwr_pointcloud publish_node
```

---
