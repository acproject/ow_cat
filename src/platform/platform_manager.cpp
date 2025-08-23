#include "platform/platform_manager.h"
#include <spdlog/spdlog.h>

#ifdef _WIN32
#include "platform/windows/windows_ime_adapter.h"
#elif defined(__APPLE__)
#include "platform/macos/macos_input_adapter.h"
#elif defined(__linux__)
#include "platform/linux/linux_input_adapter.h"
#endif

namespace owcat {
namespace platform {

std::unique_ptr<PlatformManager> createPlatformManager() {
    spdlog::info("Creating platform manager for: {}", getCurrentPlatform());
    
#ifdef _WIN32
    return std::make_unique<windows::WindowsImeAdapter>();
#elif defined(__APPLE__)
    return std::make_unique<macos::MacOSInputAdapter>();
#elif defined(__linux__)
    return std::make_unique<linux::LinuxInputAdapter>();
#else
    spdlog::error("Unsupported platform");
    return nullptr;
#endif
}

std::string getCurrentPlatform() {
#ifdef _WIN32
    return "Windows";
#elif defined(__APPLE__)
    #ifdef TARGET_OS_MAC
        return "macOS";
    #else
        return "iOS";
    #endif
#elif defined(__linux__)
    return "Linux";
#elif defined(__FreeBSD__)
    return "FreeBSD";
#elif defined(__OpenBSD__)
    return "OpenBSD";
#elif defined(__NetBSD__)
    return "NetBSD";
#else
    return "Unknown";
#endif
}

bool isPlatformSupported() {
    std::string platform = getCurrentPlatform();
    
    // 支持的平台列表
    return platform == "Windows" || 
           platform == "macOS" || 
           platform == "Linux";
}

} // namespace platform
} // namespace owcat