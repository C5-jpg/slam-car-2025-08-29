<div align="center">

# 🏎️ SLAM-Car — PC 版激光雷达 SLAM 系统

**CoreSLAM · YDLIDAR X2 · 实时建图 · 数据包验证**

[![C](https://img.shields.io/badge/C-99-A8B9CC.svg)](https://en.cppreference.com/w/c)
[![CMake](https://img.shields.io/badge/CMake-3.10+-064F8C.svg)](https://cmake.org/)
[![License](https://img.shields.io/badge/License-MIT-green.svg)]()

**PC 原生运行的 SLAM 系统，含 YDLIDAR X2 数据包解析与验证工具**

</div>

---

## 📋 项目概述

本项目是 SLAM 自主导航小车的 **PC 原生版本**，基于 CoreSLAM (TinySLAM) 算法实现实时建图。相较于 WSL 版本，此版本专注于桌面端运行，并提供了独立的 **YDLIDAR X2 数据包验证工具**，用于调试和诊断激光雷达串口通信。

### 🔑 核心组件

| 组件 | 描述 |
|:---:|------|
| **CoreSLAM 库** | TinySLAM 算法实现，含回环检测 |
| **slam_app_pc** | PC 端 SLAM 主程序 |
| **ydlidar_x2_packet_validator** | YDLIDAR X2 串口数据包解析/校验工具 |

---

## 🚀 快速开始

```bash
# 编译
cmake -B build && cmake --build build

# 运行 SLAM
./slam_app_pc

# 验证 LiDAR 数据包
./ydlidar_x2_packet_validator /dev/ttyUSB0
```

---

## 🛠️ 技术栈

- **语言**: C (C99)
- **算法**: CoreSLAM / TinySLAM
- **硬件**: YDLIDAR X2 激光雷达
- **通信**: Serial (termios, 115200 baud)
- **构建**: CMake + Autotools
- **可视化**: SDL 1.x

---

<div align="center">

**⭐ 如果这个项目对您有帮助，请给个 Star！**

</div>
