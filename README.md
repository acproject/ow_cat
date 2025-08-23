# OW Cat 跨平台智能输入法

一款基于AI预测的跨平台中文输入法，支持Windows、macOS和Linux系统。

## 特性

- 🚀 **跨平台支持**: Windows、macOS、Linux
- 🧠 **AI智能预测**: 基于Qwen0.6B模型的智能词汇预测
- 📚 **智能词库**: 支持用户词汇学习和频率调整
- 🎨 **现代UI**: 基于GTK的美观用户界面
- 🔒 **本地运行**: 无需网络连接，保护隐私
- ⚡ **高性能**: 优化的C++实现，响应迅速

## 系统架构

```
┌─────────────────┐
│   用户界面层     │  GTK-based UI
├─────────────────┤
│   平台适配层     │  Windows IME / macOS IMK / Linux IBus/Fcitx
├─────────────────┤
│   核心引擎层     │  拼音转换 / 词库管理 / AI预测
└─────────────────┘
```

### 核心组件

- **核心引擎**: 拼音转换、词库管理、AI预测算法
- **平台适配**: 各平台原生输入法API集成
- **用户界面**: GTK实现的候选词窗口和设置界面
- **AI预测**: 基于llama.cpp的Qwen0.6B模型集成

## 构建要求

### 系统要求
- Windows 10+ / macOS 10.15+ / Linux (Ubuntu 20.04+)
- CMake 3.20+
- C++17兼容编译器
- vcpkg包管理器

### 依赖库
- GTK3 - 用户界面
- SQLite3 - 词库存储
- spdlog - 日志系统
- nlohmann/json - JSON处理
- Boost - 系统功能
- llama.cpp - AI模型推理

## 快速开始

### 1. 安装vcpkg

```bash
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat  # Windows
# ./bootstrap-vcpkg.sh   # Linux/macOS
```

设置环境变量:
```bash
set VCPKG_ROOT=C:\path\to\vcpkg  # Windows
# export VCPKG_ROOT=/path/to/vcpkg  # Linux/macOS
```

### 2. 安装依赖

```bash
vcpkg install gtk3 sqlite3 spdlog nlohmann-json boost-filesystem boost-system boost-thread
```

### 3. 构建项目

#### Windows
```powershell
.\build.ps1 Release
```

#### Linux/macOS
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake
cmake --build . --parallel
```

### 4. 运行

```bash
.\build\bin\ow_cat.exe  # Windows
# ./build/bin/ow_cat     # Linux/macOS
```

## 项目结构

```
ow_cat/
├── CMakeLists.txt          # 主CMake配置
├── vcpkg.json             # vcpkg依赖配置
├── build.ps1              # Windows构建脚本
├── README.md              # 项目说明
├── include/               # 头文件
│   ├── core/             # 核心引擎头文件
│   ├── platform/         # 平台适配头文件
│   ├── ui/               # UI层头文件
│   └── app/              # 应用层头文件
├── src/                   # 源代码
│   ├── main.cpp          # 程序入口
│   ├── core/             # 核心引擎实现
│   ├── platform/         # 平台适配实现
│   ├── ui/               # UI层实现
│   └── app/              # 应用层实现
├── data/                  # 数据文件
│   └── dictionary.db     # 词库数据库
├── models/                # AI模型文件
│   └── qwen0.6b.gguf     # Qwen模型文件
└── resources/             # 资源文件
    ├── icons/            # 图标文件
    └── themes/           # 主题文件
```

## 开发指南

### 添加新的平台支持

1. 在 `include/platform/` 下创建平台特定头文件
2. 在 `src/platform/` 下实现平台适配代码
3. 更新 `src/platform/CMakeLists.txt` 添加新平台的构建配置

### 自定义AI模型

1. 将GGUF格式的模型文件放入 `models/` 目录
2. 更新配置文件中的模型路径
3. 根据需要调整预测参数

### 扩展词库

1. 词库使用SQLite数据库存储
2. 支持导入TXT、CSV、JSON格式的词库文件
3. 用户词汇会自动学习和更新频率

## 配置选项

主要配置选项在 `EngineConfig` 结构中定义:

- `dictionary_path`: 词库数据库路径
- `model_path`: AI模型文件路径
- `max_candidates`: 最大候选词数量
- `enable_prediction`: 是否启用AI预测
- `enable_learning`: 是否启用用户学习
- `prediction_threshold`: AI预测阈值

## 许可证

本项目采用MIT许可证 - 详见 [LICENSE](LICENSE) 文件

## 贡献

欢迎提交Issue和Pull Request来改进这个项目！

## 致谢

- [llama.cpp](https://github.com/ggerganov/llama.cpp) - AI模型推理引擎
- [GTK](https://www.gtk.org/) - 跨平台UI框架
- [vcpkg](https://github.com/Microsoft/vcpkg) - C++包管理器
- [Qwen](https://github.com/QwenLM/Qwen) - 预训练语言模型