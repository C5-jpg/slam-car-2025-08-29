<div align="center">

# 🏎️ SLAM-Car — PC 版激光雷达 SLAM 系统

**CoreSLAM (TinySLAM) · YDLIDAR X2 · 实时建图 · 回环检测 · 蒙特卡洛定位**

[![C](https://img.shields.io/badge/C-99-A8B9CC.svg)](https://en.cppreference.com/w/c)
[![CMake](https://img.shields.io/badge/CMake-3.10+-064F8C.svg)](https://cmake.org/)
[![Linux](https://img.shields.io/badge/Linux-POSIX-FCC624.svg)](https://www.kernel.org/)
[![License](https://img.shields.io/badge/License-MIT-green.svg)]()

**基于激光雷达的自主导航小车 SLAM 系统，PC 原生运行**

</div>

---

## 📋 项目概述

本项目实现了一套完整的 **SLAM（同步定位与建图）** 系统，搭载于自主导航小车，使用 **YDLIDAR X2 激光雷达** 扫描环境。基于 Mines ParisTech 的 **CoreSLAM (TinySLAM)** 算法，通过蒙特卡洛搜索进行实时位姿估计和栅格地图构建，支持回环检测以修正累积漂移。

### 🔑 核心特性

| 特性 | 描述 |
|:---:|------|
| 🗺️ **实时建图** | CoreSLAM 栅格地图构建，PGM 格式输出 |
| 🔄 **回环检测** | 自动识别重访区域并修正地图漂移 |
| 🎲 **蒙特卡洛定位** | 随机粒子搜索实现鲁棒位姿估计 |
| 🔦 **YDLIDAR X2** | 完整的 0xAA55 协议解析与校验 |
| 🔍 **数据包验证器** | 独立工具，用于调试和诊断 LiDAR 串口通信 |

---

## 🏗️ 系统架构

```
┌────────────────────────────────────────────────────────────┐
│                    SLAM 系统流水线                           │
│                                                             │
│  YDLIDAR X2 LiDAR        CoreSLAM Engine        Output     │
│  ┌──────────────┐      ┌─────────────────┐    ┌─────────┐ │
│  │ 激光扫描      │      │ 蒙特卡洛搜索    │    │ 位姿输出 │ │
│  │ /dev/ttyUSB0 │─▶ 0xAA55 ──▶ 地图更新   │──▶ │ PGM 地图│ │
│  │ 115200 baud  │ 协议解析   │ 回环检测    │    │ 终端日志│ │
│  │              │           │ 位姿估计    │    │         │ │
│  └──────────────┘           └─────────────────┘    └─────────┘ │
└────────────────────────────────────────────────────────────┘
```

---

## 📁 项目结构

```
slam-car/
│
├── 🔧 CoreSLAM 核心库
│   ├── CoreSLAM.h / CoreSLAM.c          # SLAM 核心: 地图初始化, 扫描匹配, 蒙特卡洛搜索
│   ├── CoreSLAM_random.c                # 随机数生成 (蒙特卡洛定位)
│   ├── CoreSLAM_state.c                 # 状态管理 (迭代建图)
│   ├── CoreSLAM_loop_closing.c          # 回环检测
│   └── CoreSLAM_ext.c                   # 扩展: PGM导出, RGB渲染
│
├── 🚗 应用程序
│   ├── ydlidar_x2_packet_validator.c    # LiDAR 数据包验证/诊断工具
│   └── slam_app_pc                      # PC 端 SLAM 主程序 (预编译)
│
├── 🗺️ 输出
│   └── live_map_pc.pgm                  # 生成的栅格地图
│
├── 🔨 构建系统
│   ├── CMakeLists.txt                   # CMake
│   └── Makefile.am / configure.ac       # Autotools
│
└── 📦 依赖与测试
    ├── test/                            # 测试数据 (.dat, .pgm, gnuplot脚本)
    └── third_parties/SDL/               # SDL 1.x 可视化库
```

---

## 🚀 快速开始

### 编译

```bash
# 方式一: CMake
cmake -B build && cmake --build build

# 方式二: Autotools
./bootstrap && ./configure && make
```

### 运行

```bash
# 运行 SLAM 建图 (需要连接 YDLIDAR X2)
./slam_app_pc

# 验证 LiDAR 数据包 (诊断工具)
./ydlidar_x2_packet_validator /dev/ttyUSB0
```

### YDLIDAR X2 数据包验证器

独立的诊断工具，用于调试激光雷达串口通信：

- 打开串口 `/dev/ttyUSB0`，波特率 115200
- 解析 0xAA55 协议包头
- 校验数据包完整性
- 解码角度和距离数据（含矫正）
- 输出十六进制诊断信息

---

## 🛠️ 技术栈

<div align="center">

| 类别 | 技术 |
|:---:|:---:|
| 语言 | C (C99) |
| 算法 | CoreSLAM / TinySLAM (蒙特卡洛定位) |
| 硬件 | YDLIDAR X2 激光雷达 |
| 通信 | POSIX Serial (termios) |
| 构建 | CMake + Autotools |
| 可视化 | SDL 1.x |
| 输出 | PGM 栅格地图格式 |

</div>

---

## 🔗 相关仓库

- [wsl-slam-car](https://github.com/C5-jpg/wsl-slam-car-2025-08-27) — WSL/ARM 版本（支持 Luckfox Pico 交叉编译 + TCP Socket + Python 模拟器）

---

<div align="center">

**⭐ 如果这个项目对您有帮助，请给个 Star！**

</div>
