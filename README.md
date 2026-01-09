# Detectionx
Edge-oriented multi-stream AI video analytics  
_面向边缘的多路 AI 视频分析_

# 🧩 Overview | 概述
DetectionX delivers high-performance edge-oriented multi-stream AI video analytics, supporting YOLO series (v5/v8/v11) for real-time multi-stream recognition and streaming. Detection results are published via MQTT as structured messages, enabling seamless integration with IoT and backend systems, with one-click deployment for rapid adaptation to edge AI scenarios.  

_DetectionX 提供高效的边缘多路视频流 AI 视频分析，支持 YOLO 系列（v5/v8/v11），实现多路视频流实时识别与推流，检测结果通过 MQTT 结构化消息输出，便于 IoT 与后端系统集成，一键部署，快速适配各类边缘 AI 场景。_  

# 🎬 Demo | 演示

## 相关链接
+ Bilibili：https://www.bilibili.com/video/BV1HvUYBEEtz/?vd_source=cf873886c731eb05ae070722ce19f5dc  

![Demo](asset/images/demo.gif)  

# ⚙️ Core Features | 核心特性
## Efficient Detection | 高效检测
+ Support for YOLO series (v5, v8, v11)（_支持 YOLO 系列多版本_）
+ Configurable detection regions (ROI)（_自定义检测区域_）

## Multi-stream & Edge Processing | 多路与边缘处理
+ Multi-stream video input（_支持多路视频流输入_）  
+ Edge-side video streaming（_边缘端视频推流，实时叠加检测结果_）  

## Integration & Deployment | 集成与部署
+ MQTT structured messaging（_结构化消息发布，便于 IoT 与后端系统集成_）  
+ One-click, configuration-driven setup（_一键部署，快速上手_）  

# 💻 Platform Support | 平台支持
| Platform | Status | Acceleration | Notes |
| :--- | :---: | :---: | :--- |
| Rockchip RK3588 | ✅ Supported | RKMPP / RGA | Verified on RKNN SDK |
| NVIDIA Jetson | 🚧 In Progress | NVDEC / NVENC | Verified on JetPack |
| x86 + CUDA GPU | 🚧 In Progress | CUDA / FFmpeg | Under development |

# 🧰 Quick Start ｜快速开始

## Installation & Setup | 安装与配置

### Basic Dependencies (Ubuntu) | Ubuntu 基础依赖
```shell
sudo apt update

sudo apt install build-essential cmake git libgtk2.0-dev pkg-config
```

### Get Pre-compiled Dependencies ｜获取 **mirox** 预编译依赖

> ⚠️ **Pre-compiled Package Notice**  
> The current pre-compiled packages are built on **Ubuntu 20.04 (focal)**.  
> Compatibility with other Ubuntu releases is **under evaluation**.

```shell
sudo add-apt-repository ppa:maixos/vision -y

sudo apt update

sudo apt install libtoolkitx-dev libvcodecx-dev librtspx-dev libinferencex-dev
```

### YAML Configuration | YAML 配置说明

- **id**：摄像头唯一标识，用于结果映射
- **uri**：视频源地址，可为 RTSP/HTTP 或本地文件
- **region**：可选检测区域，用于指定感兴趣区域（ROI）
  - **type**：区域类型，可选 `polygon`（多边形）或 `rectangle`（矩形）
  - **values**：区域顶点坐标列表，按顺序连线形成闭合区域

```yaml
tasks:
  - id: cam00001
    uri: rtsp://192.168.1.110:8554/live1
    region:
      type: polygon
      values: [ [280, 340], [860, 342], [1350, 610], [3, 628], [3, 495], [14, 491] ]

  - id: cam00002
    uri: rtsp://192.168.1.110:8554/live2
    region:
      type: polygon
      values: [ [233, 300], [1000, 296], [1276, 440], [1276, 600], [3, 560], [3, 425] ]

  - id: cam00003
    uri: rtsp://192.168.1.110:8554/live3
    region:
      type: polygon
      values: [ [423, 398], [730, 392], [1045, 616], [190, 630] ]

  ... ...
```

### Build & Run ｜ 编译与运行
```shell
cd /path/to/detectionx

# Build for your target platform
bash build.sh rk3588

# Run example
./bin/test_on_rk3588
```

# 📜 License | 许可协议
This project is licensed under the **Apache License 2.0** – see the [LICENSE](https://github.com/MincoX/ai-streamx?tab=Apache-2.0-1-ov-file#) file for details.  
Copyright © 2025  
**MincoX**  
Part of the **AI Open Series** — A suite of open AI application frameworks.  

# 💬 Contact | 联系方式
**Bilibili**: https://space.bilibili.com/382756182?spm_id_from=333.1007.0.0  
**Email**: maixos@outlook.com  
