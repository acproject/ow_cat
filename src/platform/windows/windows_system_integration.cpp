#ifdef _WIN32

#include "platform/windows/windows_ime_adapter.h"
#include <spdlog/spdlog.h>
#include <sstream>
#include <iomanip>
#include <versionhelpers.h>
#include <psapi.h>
#include <tlhelp32.h>

namespace owcat {
namespace platform {
namespace windows {

// WindowsSystemIntegration::Impl 实现
class WindowsSystemIntegration::Impl {
public:
    Impl() {
    }
    
    ~Impl() {
    }
    
    bool registerIme(const std::string& ime_name, const std::string& ime_path) {
        spdlog::info("Registering IME: {} at {}", ime_name, ime_path);
        
        // 在Windows中，注册IME需要管理员权限和复杂的注册表操作
        // 这里提供一个简化的实现，实际项目中可能需要更复杂的处理
        
        if (!checkAdminPrivileges()) {
            spdlog::warn("Administrator privileges required for IME registration");
            return false;
        }
        
        // 检查IME文件是否存在
        if (GetFileAttributesA(ime_path.c_str()) == INVALID_FILE_ATTRIBUTES) {
            spdlog::error("IME file not found: {}", ime_path);
            return false;
        }
        
        // 这里应该实现实际的IME注册逻辑
        // 包括注册表操作、文件复制等
        spdlog::info("IME registration completed (simplified implementation)");
        return true;
    }
    
    bool unregisterIme(const std::string& ime_name) {
        spdlog::info("Unregistering IME: {}", ime_name);
        
        if (!checkAdminPrivileges()) {
            spdlog::warn("Administrator privileges required for IME unregistration");
            return false;
        }
        
        // 这里应该实现实际的IME注销逻辑
        spdlog::info("IME unregistration completed (simplified implementation)");
        return true;
    }
    
    bool checkAdminPrivileges() {
        BOOL is_admin = FALSE;
        PSID admin_group = nullptr;
        SID_IDENTIFIER_AUTHORITY nt_authority = SECURITY_NT_AUTHORITY;
        
        // 创建管理员组的SID
        if (AllocateAndInitializeSid(&nt_authority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                   DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &admin_group)) {
            // 检查当前用户是否在管理员组中
            if (!CheckTokenMembership(nullptr, admin_group, &is_admin)) {
                is_admin = FALSE;
            }
            FreeSid(admin_group);
        }
        
        return is_admin == TRUE;
    }
    
    std::string getWindowsVersion() {
        std::stringstream version;
        
        // 获取Windows版本信息
        if (IsWindows10OrGreater()) {
            version << "Windows 10 or later";
        } else if (IsWindows8Point1OrGreater()) {
            version << "Windows 8.1";
        } else if (IsWindows8OrGreater()) {
            version << "Windows 8";
        } else if (IsWindows7OrGreater()) {
            version << "Windows 7";
        } else if (IsWindowsVistaOrGreater()) {
            version << "Windows Vista";
        } else {
            version << "Windows (Unknown version)";
        }
        
        // 获取详细版本信息
        OSVERSIONINFOEXW osvi;
        ZeroMemory(&osvi, sizeof(OSVERSIONINFOEXW));
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);
        
        // 注意：GetVersionEx在Windows 8.1+中可能返回不准确的信息
        // 实际项目中可能需要使用其他方法获取准确版本
        if (GetVersionExW(reinterpret_cast<OSVERSIONINFOW*>(&osvi))) {
            version << " (" << osvi.dwMajorVersion << "." << osvi.dwMinorVersion
                   << "." << osvi.dwBuildNumber << ")";
        }
        
        return version.str();
    }
    
    std::map<std::string, std::string> getSystemInfo() {
        std::map<std::string, std::string> info;
        
        // 操作系统信息
        info["os_name"] = "Windows";
        info["os_version"] = getWindowsVersion();
        
        // 系统架构
        SYSTEM_INFO sys_info;
        GetSystemInfo(&sys_info);
        
        switch (sys_info.wProcessorArchitecture) {
            case PROCESSOR_ARCHITECTURE_AMD64:
                info["architecture"] = "x64";
                break;
            case PROCESSOR_ARCHITECTURE_INTEL:
                info["architecture"] = "x86";
                break;
            case PROCESSOR_ARCHITECTURE_ARM:
                info["architecture"] = "ARM";
                break;
            case PROCESSOR_ARCHITECTURE_ARM64:
                info["architecture"] = "ARM64";
                break;
            default:
                info["architecture"] = "Unknown";
                break;
        }
        
        // 处理器信息
        info["processor_count"] = std::to_string(sys_info.dwNumberOfProcessors);
        
        // 内存信息
        MEMORYSTATUSEX mem_status;
        mem_status.dwLength = sizeof(mem_status);
        if (GlobalMemoryStatusEx(&mem_status)) {
            info["total_memory"] = std::to_string(mem_status.ullTotalPhys / (1024 * 1024)) + " MB";
            info["available_memory"] = std::to_string(mem_status.ullAvailPhys / (1024 * 1024)) + " MB";
            info["memory_usage"] = std::to_string(mem_status.dwMemoryLoad) + "%";
        }
        
        // 用户信息
        char username[256];
        DWORD username_len = sizeof(username);
        if (GetUserNameA(username, &username_len)) {
            info["username"] = username;
        }
        
        // 计算机名
        char computer_name[256];
        DWORD computer_name_len = sizeof(computer_name);
        if (GetComputerNameA(computer_name, &computer_name_len)) {
            info["computer_name"] = computer_name;
        }
        
        // 管理员权限
        info["admin_privileges"] = checkAdminPrivileges() ? "true" : "false";
        
        return info;
    }
    
    std::vector<std::string> getInstalledImes() {
        std::vector<std::string> imes;
        
        // 获取系统中安装的输入法列表
        UINT ime_count = GetKeyboardLayoutList(0, nullptr);
        if (ime_count > 0) {
            std::vector<HKL> layouts(ime_count);
            GetKeyboardLayoutList(ime_count, layouts.data());
            
            for (HKL layout : layouts) {
                wchar_t ime_name[256];
                if (ImmGetDescriptionW(layout, ime_name, sizeof(ime_name) / sizeof(wchar_t))) {
                    imes.push_back(wstringToUtf8(ime_name));
                } else {
                    // 如果无法获取描述，使用布局ID
                    std::stringstream ss;
                    ss << "Layout_0x" << std::hex << std::uppercase 
                       << reinterpret_cast<uintptr_t>(layout);
                    imes.push_back(ss.str());
                }
            }
        }
        
        return imes;
    }
    
    bool isImeInstalled(const std::string& ime_name) {
        auto imes = getInstalledImes();
        return std::find(imes.begin(), imes.end(), ime_name) != imes.end();
    }
    
    std::string getCurrentIme() {
        HKL current_layout = GetKeyboardLayout(0);
        if (current_layout) {
            wchar_t ime_name[256];
            if (ImmGetDescriptionW(current_layout, ime_name, sizeof(ime_name) / sizeof(wchar_t))) {
                return wstringToUtf8(ime_name);
            }
        }
        return "Unknown";
    }
    
    bool setCurrentIme(const std::string& ime_name) {
        // 获取所有键盘布局
        UINT ime_count = GetKeyboardLayoutList(0, nullptr);
        if (ime_count > 0) {
            std::vector<HKL> layouts(ime_count);
            GetKeyboardLayoutList(ime_count, layouts.data());
            
            for (HKL layout : layouts) {
                wchar_t layout_name[256];
                if (ImmGetDescriptionW(layout, layout_name, sizeof(layout_name) / sizeof(wchar_t))) {
                    if (wstringToUtf8(layout_name) == ime_name) {
                        // 切换到指定的输入法
                        HWND foreground_window = GetForegroundWindow();
                        if (foreground_window) {
                            PostMessage(foreground_window, WM_INPUTLANGCHANGEREQUEST, 0, 
                                      reinterpret_cast<LPARAM>(layout));
                            return true;
                        }
                    }
                }
            }
        }
        
        spdlog::warn("IME not found: {}", ime_name);
        return false;
    }
    
    std::vector<ProcessInfo> getRunningProcesses() {
        std::vector<ProcessInfo> processes;
        
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot == INVALID_HANDLE_VALUE) {
            spdlog::error("Failed to create process snapshot");
            return processes;
        }
        
        PROCESSENTRY32W pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32W);
        
        if (Process32FirstW(snapshot, &pe32)) {
            do {
                ProcessInfo info;
                info.pid = pe32.th32ProcessID;
                info.name = wstringToUtf8(pe32.szExeFile);
                info.parent_pid = pe32.th32ParentProcessID;
                
                // 获取进程句柄以获取更多信息
                HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 
                                           FALSE, pe32.th32ProcessID);
                if (process) {
                    // 获取内存使用情况
                    PROCESS_MEMORY_COUNTERS pmc;
                    if (GetProcessMemoryInfo(process, &pmc, sizeof(pmc))) {
                        info.memory_usage = pmc.WorkingSetSize;
                    }
                    
                    // 获取进程路径
                    wchar_t process_path[MAX_PATH];
                    if (GetModuleFileNameExW(process, nullptr, process_path, MAX_PATH)) {
                        info.path = wstringToUtf8(process_path);
                    }
                    
                    CloseHandle(process);
                }
                
                processes.push_back(info);
            } while (Process32NextW(snapshot, &pe32));
        }
        
        CloseHandle(snapshot);
        return processes;
    }
    
    ProcessInfo getCurrentProcess() {
        ProcessInfo info;
        info.pid = GetCurrentProcessId();
        
        // 获取进程名
        wchar_t process_name[MAX_PATH];
        if (GetModuleFileNameW(nullptr, process_name, MAX_PATH)) {
            std::wstring full_path(process_name);
            size_t last_slash = full_path.find_last_of(L"\\");
            if (last_slash != std::wstring::npos) {
                info.name = wstringToUtf8(full_path.substr(last_slash + 1));
            } else {
                info.name = wstringToUtf8(full_path);
            }
            info.path = wstringToUtf8(full_path);
        }
        
        // 获取父进程ID
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot != INVALID_HANDLE_VALUE) {
            PROCESSENTRY32W pe32;
            pe32.dwSize = sizeof(PROCESSENTRY32W);
            
            if (Process32FirstW(snapshot, &pe32)) {
                do {
                    if (pe32.th32ProcessID == info.pid) {
                        info.parent_pid = pe32.th32ParentProcessID;
                        break;
                    }
                } while (Process32NextW(snapshot, &pe32));
            }
            
            CloseHandle(snapshot);
        }
        
        // 获取内存使用情况
        PROCESS_MEMORY_COUNTERS pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
            info.memory_usage = pmc.WorkingSetSize;
        }
        
        return info;
    }
    
    bool enableDebugPrivilege() {
        HANDLE token;
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token)) {
            return false;
        }
        
        TOKEN_PRIVILEGES tp;
        tp.PrivilegeCount = 1;
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        
        if (!LookupPrivilegeValue(nullptr, SE_DEBUG_NAME, &tp.Privileges[0].Luid)) {
            CloseHandle(token);
            return false;
        }
        
        bool result = AdjustTokenPrivileges(token, FALSE, &tp, sizeof(tp), nullptr, nullptr) != FALSE;
        CloseHandle(token);
        
        return result;
    }
    
    std::string getLastErrorString() {
        DWORD error_code = GetLastError();
        if (error_code == 0) {
            return "No error";
        }
        
        LPSTR message_buffer = nullptr;
        size_t size = FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr, error_code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            reinterpret_cast<LPSTR>(&message_buffer), 0, nullptr
        );
        
        std::string message(message_buffer, size);
        LocalFree(message_buffer);
        
        // 移除末尾的换行符
        while (!message.empty() && (message.back() == '\r' || message.back() == '\n')) {
            message.pop_back();
        }
        
        return "Error " + std::to_string(error_code) + ": " + message;
    }
};

// WindowsSystemIntegration 实现
WindowsSystemIntegration::WindowsSystemIntegration()
    : pImpl(std::make_unique<Impl>()) {
}

WindowsSystemIntegration::~WindowsSystemIntegration() = default;

bool WindowsSystemIntegration::registerIme(const std::string& ime_name, const std::string& ime_path) {
    return pImpl->registerIme(ime_name, ime_path);
}

bool WindowsSystemIntegration::unregisterIme(const std::string& ime_name) {
    return pImpl->unregisterIme(ime_name);
}

bool WindowsSystemIntegration::checkAdminPrivileges() {
    return pImpl->checkAdminPrivileges();
}

std::string WindowsSystemIntegration::getWindowsVersion() {
    return pImpl->getWindowsVersion();
}

std::map<std::string, std::string> WindowsSystemIntegration::getSystemInfo() {
    return pImpl->getSystemInfo();
}

std::vector<std::string> WindowsSystemIntegration::getInstalledImes() {
    return pImpl->getInstalledImes();
}

bool WindowsSystemIntegration::isImeInstalled(const std::string& ime_name) {
    return pImpl->isImeInstalled(ime_name);
}

std::string WindowsSystemIntegration::getCurrentIme() {
    return pImpl->getCurrentIme();
}

bool WindowsSystemIntegration::setCurrentIme(const std::string& ime_name) {
    return pImpl->setCurrentIme(ime_name);
}

std::vector<ProcessInfo> WindowsSystemIntegration::getRunningProcesses() {
    return pImpl->getRunningProcesses();
}

ProcessInfo WindowsSystemIntegration::getCurrentProcess() {
    return pImpl->getCurrentProcess();
}

bool WindowsSystemIntegration::enableDebugPrivilege() {
    return pImpl->enableDebugPrivilege();
}

std::string WindowsSystemIntegration::getLastErrorString() {
    return pImpl->getLastErrorString();
}

} // namespace windows
} // namespace platform
} // namespace owcat

#endif // _WIN32