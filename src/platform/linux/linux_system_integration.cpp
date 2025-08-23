#include "platform/linux/linux_ime_adapter.h"
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdlib>
#include <cstring>

#ifdef __linux__
#include <ibus.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <X11/Xlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <pwd.h>
#include <dirent.h>
#include <proc/readproc.h>
#include <proc/sysinfo.h>
#endif

namespace owcat {

// Implementation structure for LinuxSystemIntegration
struct LinuxSystemIntegration::Impl {
#ifdef __linux__
    IBusBus* bus;
    IBusFactory* factory;
    GMainLoop* mainLoop;
#endif
    
    // State
    bool isInitialized;
    std::string homeDirectory;
    std::string configDirectory;
    std::string dataDirectory;
    std::string cacheDirectory;
    
    // System information
    std::string osName;
    std::string osVersion;
    std::string architecture;
    std::string desktopEnvironment;
    std::string displayServer;
    std::string ibusVersion;
    
    Impl() {
#ifdef __linux__
        bus = nullptr;
        factory = nullptr;
        mainLoop = nullptr;
#endif
        isInitialized = false;
        
        // Initialize directories
        const char* home = getenv("HOME");
        if (home) {
            homeDirectory = home;
        } else {
            struct passwd* pw = getpwuid(getuid());
            if (pw) {
                homeDirectory = pw->pw_dir;
            }
        }
        
        configDirectory = homeDirectory + "/.config";
        dataDirectory = homeDirectory + "/.local/share";
        cacheDirectory = homeDirectory + "/.cache";
        
        // Override with XDG directories if available
        const char* xdgConfig = getenv("XDG_CONFIG_HOME");
        if (xdgConfig) {
            configDirectory = xdgConfig;
        }
        
        const char* xdgData = getenv("XDG_DATA_HOME");
        if (xdgData) {
            dataDirectory = xdgData;
        }
        
        const char* xdgCache = getenv("XDG_CACHE_HOME");
        if (xdgCache) {
            cacheDirectory = xdgCache;
        }
    }
    
    ~Impl() {
#ifdef __linux__
        if (factory) {
            g_object_unref(factory);
        }
        if (bus) {
            g_object_unref(bus);
        }
#endif
    }
};

LinuxSystemIntegration::LinuxSystemIntegration() : pImpl(std::make_unique<Impl>()) {}

LinuxSystemIntegration::~LinuxSystemIntegration() = default;

bool LinuxSystemIntegration::initialize() {
#ifdef __linux__
    if (pImpl->isInitialized) {
        return true;
    }
    
    // Initialize IBus
    ibus_init();
    
    // Create IBus bus connection
    pImpl->bus = ibus_bus_new();
    if (!pImpl->bus) {
        std::cerr << "Failed to create IBus bus connection" << std::endl;
        return false;
    }
    
    // Gather system information
    gatherSystemInfo();
    
    // Create necessary directories
    createDirectories();
    
    pImpl->isInitialized = true;
    std::cout << "Linux system integration initialized successfully" << std::endl;
    return true;
#else
    std::cerr << "Linux system integration not supported on this platform" << std::endl;
    return false;
#endif
}

void LinuxSystemIntegration::shutdown() {
#ifdef __linux__
    if (!pImpl->isInitialized) {
        return;
    }
    
    if (pImpl->factory) {
        g_object_unref(pImpl->factory);
        pImpl->factory = nullptr;
    }
    
    if (pImpl->bus) {
        g_object_unref(pImpl->bus);
        pImpl->bus = nullptr;
    }
    
    pImpl->isInitialized = false;
    std::cout << "Linux system integration shut down" << std::endl;
#endif
}

bool LinuxSystemIntegration::registerIME(const std::string& id, const std::string& name, const std::string& description) {
#ifdef __linux__
    if (!pImpl->isInitialized) {
        return false;
    }
    
    // Create component description
    IBusComponent* component = ibus_component_new("org.freedesktop.IBus.OwCat",
                                                  description.c_str(),
                                                  "1.0.0",
                                                  "MIT",
                                                  "OwCat Team",
                                                  "https://github.com/owcat/owcat",
                                                  "/usr/libexec/ibus-engine-owcat",
                                                  "owcat");
    
    if (!component) {
        std::cerr << "Failed to create IBus component" << std::endl;
        return false;
    }
    
    // Create engine description
    IBusEngineDesc* engineDesc = ibus_engine_desc_new(id.c_str(),
                                                       name.c_str(),
                                                       description.c_str(),
                                                       "zh",
                                                       "MIT",
                                                       "OwCat Team",
                                                       "owcat",
                                                       "us");
    
    if (!engineDesc) {
        std::cerr << "Failed to create IBus engine description" << std::endl;
        g_object_unref(component);
        return false;
    }
    
    // Add engine to component
    ibus_component_add_engine(component, engineDesc);
    
    // Register component with IBus
    if (!ibus_bus_register_component(pImpl->bus, component)) {
        std::cerr << "Failed to register IBus component" << std::endl;
        g_object_unref(component);
        return false;
    }
    
    // Create and register factory
    pImpl->factory = ibus_factory_new(ibus_bus_get_connection(pImpl->bus));
    if (!pImpl->factory) {
        std::cerr << "Failed to create IBus factory" << std::endl;
        g_object_unref(component);
        return false;
    }
    
    // Add engine factory
    ibus_factory_add_engine(pImpl->factory, id.c_str(), IBUS_TYPE_ENGINE);
    
    // Write component file
    writeComponentFile(component, id);
    
    g_object_unref(component);
    
    std::cout << "IME registered successfully: " << id << std::endl;
    return true;
#else
    return false;
#endif
}

bool LinuxSystemIntegration::unregisterIME(const std::string& id) {
#ifdef __linux__
    // Remove component file
    std::string componentFile = "/usr/share/ibus/component/" + id + ".xml";
    if (remove(componentFile.c_str()) != 0) {
        // Try user directory
        componentFile = pImpl->dataDirectory + "/ibus/component/" + id + ".xml";
        remove(componentFile.c_str());
    }
    
    std::cout << "IME unregistered: " << id << std::endl;
    return true;
#else
    return false;
#endif
}

std::vector<std::string> LinuxSystemIntegration::getInstalledIMEs() {
    std::vector<std::string> imes;
    
#ifdef __linux__
    if (!pImpl->bus) {
        return imes;
    }
    
    // Get list of engines from IBus
    GList* engines = ibus_bus_list_engines(pImpl->bus);
    for (GList* p = engines; p != nullptr; p = p->next) {
        IBusEngineDesc* desc = IBUS_ENGINE_DESC(p->data);
        if (desc) {
            const char* name = ibus_engine_desc_get_name(desc);
            if (name) {
                imes.push_back(name);
            }
        }
    }
    
    g_list_free(engines);
#endif
    
    return imes;
}

bool LinuxSystemIntegration::isIMEInstalled(const std::string& id) {
    auto imes = getInstalledIMEs();
    return std::find(imes.begin(), imes.end(), id) != imes.end();
}

std::string LinuxSystemIntegration::getCurrentIME() {
#ifdef __linux__
    if (!pImpl->bus) {
        return "";
    }
    
    // Get current input context
    IBusInputContext* context = ibus_bus_create_input_context(pImpl->bus, "owcat");
    if (context) {
        // This is a simplified implementation
        // In practice, you'd need to track the current engine
        g_object_unref(context);
    }
#endif
    
    return "";
}

bool LinuxSystemIntegration::setCurrentIME(const std::string& id) {
#ifdef __linux__
    if (!pImpl->bus) {
        return false;
    }
    
    // Set global engine
    return ibus_bus_set_global_engine(pImpl->bus, id.c_str());
#else
    return false;
#endif
}

std::vector<std::map<std::string, std::string>> LinuxSystemIntegration::getRunningProcesses() {
    std::vector<std::map<std::string, std::string>> processes;
    
#ifdef __linux__
    // Read from /proc directory
    DIR* procDir = opendir("/proc");
    if (!procDir) {
        return processes;
    }
    
    struct dirent* entry;
    while ((entry = readdir(procDir)) != nullptr) {
        // Check if directory name is a number (PID)
        if (strspn(entry->d_name, "0123456789") == strlen(entry->d_name)) {
            std::string pid = entry->d_name;
            std::map<std::string, std::string> processInfo;
            
            // Read process information
            std::string statFile = "/proc/" + pid + "/stat";
            std::ifstream stat(statFile);
            if (stat.is_open()) {
                std::string line;
                std::getline(stat, line);
                
                std::istringstream iss(line);
                std::string token;
                std::vector<std::string> tokens;
                while (iss >> token) {
                    tokens.push_back(token);
                }
                
                if (tokens.size() >= 4) {
                    processInfo["pid"] = tokens[0];
                    processInfo["name"] = tokens[1];
                    processInfo["state"] = tokens[2];
                    processInfo["ppid"] = tokens[3];
                }
                
                stat.close();
            }
            
            // Read command line
            std::string cmdlineFile = "/proc/" + pid + "/cmdline";
            std::ifstream cmdline(cmdlineFile);
            if (cmdline.is_open()) {
                std::string line;
                std::getline(cmdline, line);
                processInfo["cmdline"] = line;
                cmdline.close();
            }
            
            // Read memory info
            std::string statusFile = "/proc/" + pid + "/status";
            std::ifstream status(statusFile);
            if (status.is_open()) {
                std::string line;
                while (std::getline(status, line)) {
                    if (line.find("VmRSS:") == 0) {
                        processInfo["memory"] = line.substr(7);
                        break;
                    }
                }
                status.close();
            }
            
            if (!processInfo.empty()) {
                processes.push_back(processInfo);
            }
        }
    }
    
    closedir(procDir);
#endif
    
    return processes;
}

std::map<std::string, std::string> LinuxSystemIntegration::getCurrentProcessInfo() {
    std::map<std::string, std::string> info;
    
#ifdef __linux__
    pid_t pid = getpid();
    info["pid"] = std::to_string(pid);
    info["name"] = "owcat";
    
    // Get process name from /proc/self/comm
    std::ifstream comm("/proc/self/comm");
    if (comm.is_open()) {
        std::string name;
        std::getline(comm, name);
        info["name"] = name;
        comm.close();
    }
    
    // Get command line
    std::ifstream cmdline("/proc/self/cmdline");
    if (cmdline.is_open()) {
        std::string line;
        std::getline(cmdline, line);
        info["cmdline"] = line;
        cmdline.close();
    }
    
    // Get memory usage
    std::ifstream status("/proc/self/status");
    if (status.is_open()) {
        std::string line;
        while (std::getline(status, line)) {
            if (line.find("VmRSS:") == 0) {
                info["memory"] = line.substr(7);
                break;
            }
        }
        status.close();
    }
    
    // Get current window title (simplified)
    info["window_title"] = "OwCat IME";
#endif
    
    return info;
}

std::string LinuxSystemIntegration::getSystemVersion() {
#ifdef __linux__
    struct utsname unameData;
    if (uname(&unameData) == 0) {
        return std::string(unameData.sysname) + " " + unameData.release;
    }
#endif
    return "Unknown";
}

std::map<std::string, std::string> LinuxSystemIntegration::getSystemInfo() {
    std::map<std::string, std::string> info;
    
#ifdef __linux__
    // OS information
    struct utsname unameData;
    if (uname(&unameData) == 0) {
        info["os_name"] = unameData.sysname;
        info["os_version"] = unameData.release;
        info["architecture"] = unameData.machine;
        info["hostname"] = unameData.nodename;
    }
    
    // Distribution information
    std::ifstream osRelease("/etc/os-release");
    if (osRelease.is_open()) {
        std::string line;
        while (std::getline(osRelease, line)) {
            size_t pos = line.find('=');
            if (pos != std::string::npos) {
                std::string key = line.substr(0, pos);
                std::string value = line.substr(pos + 1);
                
                // Remove quotes
                if (value.front() == '"' && value.back() == '"') {
                    value = value.substr(1, value.length() - 2);
                }
                
                if (key == "NAME") {
                    info["distribution"] = value;
                } else if (key == "VERSION") {
                    info["distribution_version"] = value;
                }
            }
        }
        osRelease.close();
    }
    
    // Memory information
    std::ifstream meminfo("/proc/meminfo");
    if (meminfo.is_open()) {
        std::string line;
        while (std::getline(meminfo, line)) {
            if (line.find("MemTotal:") == 0) {
                info["total_memory"] = line.substr(10);
            } else if (line.find("MemAvailable:") == 0) {
                info["available_memory"] = line.substr(14);
            }
        }
        meminfo.close();
    }
    
    // CPU information
    std::ifstream cpuinfo("/proc/cpuinfo");
    if (cpuinfo.is_open()) {
        std::string line;
        int processorCount = 0;
        while (std::getline(cpuinfo, line)) {
            if (line.find("processor") == 0) {
                processorCount++;
            } else if (line.find("model name") == 0) {
                size_t pos = line.find(':');
                if (pos != std::string::npos) {
                    info["cpu_model"] = line.substr(pos + 2);
                }
            }
        }
        info["cpu_count"] = std::to_string(processorCount);
        cpuinfo.close();
    }
    
    // User information
    struct passwd* pw = getpwuid(getuid());
    if (pw) {
        info["username"] = pw->pw_name;
        info["home_directory"] = pw->pw_dir;
    }
    
    // Desktop environment
    info["desktop_environment"] = getDesktopEnvironment();
    info["display_server"] = getDisplayServer();
    
    // IBus information
    info["ibus_version"] = getIBusVersion();
    info["ibus_running"] = isIBusRunning() ? "true" : "false";
    
    // Directories
    info["config_directory"] = pImpl->configDirectory;
    info["data_directory"] = pImpl->dataDirectory;
    info["cache_directory"] = pImpl->cacheDirectory;
#endif
    
    return info;
}

// Linux-specific methods
std::string LinuxSystemIntegration::getDesktopEnvironment() {
    const char* de = getenv("XDG_CURRENT_DESKTOP");
    if (de) {
        return de;
    }
    
    de = getenv("DESKTOP_SESSION");
    if (de) {
        return de;
    }
    
    // Try to detect based on running processes
    if (getenv("GNOME_DESKTOP_SESSION_ID")) {
        return "GNOME";
    }
    if (getenv("KDE_FULL_SESSION")) {
        return "KDE";
    }
    if (getenv("XFCE_SESSION")) {
        return "XFCE";
    }
    
    return "Unknown";
}

std::string LinuxSystemIntegration::getDisplayServer() {
    const char* waylandDisplay = getenv("WAYLAND_DISPLAY");
    if (waylandDisplay) {
        return "Wayland";
    }
    
    const char* x11Display = getenv("DISPLAY");
    if (x11Display) {
        return "X11";
    }
    
    return "Unknown";
}

std::string LinuxSystemIntegration::getIBusVersion() {
#ifdef __linux__
    // Try to get IBus version
    FILE* pipe = popen("ibus version 2>/dev/null", "r");
    if (pipe) {
        char buffer[128];
        std::string result;
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }
        pclose(pipe);
        
        // Extract version number
        size_t pos = result.find("IBus ");
        if (pos != std::string::npos) {
            return result.substr(pos + 5);
        }
    }
#endif
    return "Unknown";
}

bool LinuxSystemIntegration::isIBusRunning() {
#ifdef __linux__
    if (pImpl->bus) {
        return ibus_bus_is_connected(pImpl->bus);
    }
    
    // Alternative: check if ibus-daemon is running
    FILE* pipe = popen("pgrep ibus-daemon", "r");
    if (pipe) {
        char buffer[128];
        bool running = fgets(buffer, sizeof(buffer), pipe) != nullptr;
        pclose(pipe);
        return running;
    }
#endif
    return false;
}

bool LinuxSystemIntegration::startIBusDaemon() {
#ifdef __linux__
    // Try to start IBus daemon
    int result = system("ibus-daemon -d");
    return result == 0;
#else
    return false;
#endif
}

bool LinuxSystemIntegration::stopIBusDaemon() {
#ifdef __linux__
    // Try to stop IBus daemon
    int result = system("ibus exit");
    return result == 0;
#else
    return false;
#endif
}

bool LinuxSystemIntegration::restartIBusDaemon() {
    return stopIBusDaemon() && startIBusDaemon();
}

std::string LinuxSystemIntegration::getConfigDirectory() const {
    return pImpl->configDirectory;
}

std::string LinuxSystemIntegration::getDataDirectory() const {
    return pImpl->dataDirectory;
}

std::string LinuxSystemIntegration::getCacheDirectory() const {
    return pImpl->cacheDirectory;
}

bool LinuxSystemIntegration::createDirectory(const std::string& path) {
#ifdef __linux__
    return mkdir(path.c_str(), 0755) == 0 || errno == EEXIST;
#else
    return false;
#endif
}

bool LinuxSystemIntegration::fileExists(const std::string& path) {
#ifdef __linux__
    struct stat st;
    return stat(path.c_str(), &st) == 0;
#else
    return false;
#endif
}

bool LinuxSystemIntegration::directoryExists(const std::string& path) {
#ifdef __linux__
    struct stat st;
    return stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
#else
    return false;
#endif
}

// Private helper methods
void LinuxSystemIntegration::gatherSystemInfo() {
#ifdef __linux__
    struct utsname unameData;
    if (uname(&unameData) == 0) {
        pImpl->osName = unameData.sysname;
        pImpl->osVersion = unameData.release;
        pImpl->architecture = unameData.machine;
    }
    
    pImpl->desktopEnvironment = getDesktopEnvironment();
    pImpl->displayServer = getDisplayServer();
    pImpl->ibusVersion = getIBusVersion();
#endif
}

void LinuxSystemIntegration::createDirectories() {
    // Create necessary directories
    std::vector<std::string> dirs = {
        pImpl->configDirectory + "/owcat",
        pImpl->dataDirectory + "/owcat",
        pImpl->dataDirectory + "/ibus/component",
        pImpl->cacheDirectory + "/owcat"
    };
    
    for (const auto& dir : dirs) {
        createDirectory(dir);
    }
}

void LinuxSystemIntegration::writeComponentFile(void* component, const std::string& engineId) {
#ifdef __linux__
    IBusComponent* comp = static_cast<IBusComponent*>(component);
    
    // Generate XML content
    std::string xml = generateComponentXML(engineId);
    
    // Write to user directory first
    std::string userComponentDir = pImpl->dataDirectory + "/ibus/component";
    createDirectory(userComponentDir);
    
    std::string componentFile = userComponentDir + "/" + engineId + ".xml";
    std::ofstream file(componentFile);
    if (file.is_open()) {
        file << xml;
        file.close();
        std::cout << "Component file written: " << componentFile << std::endl;
    }
    
    // Try to write to system directory (requires privileges)
    std::string systemComponentFile = "/usr/share/ibus/component/" + engineId + ".xml";
    std::ofstream systemFile(systemComponentFile);
    if (systemFile.is_open()) {
        systemFile << xml;
        systemFile.close();
        std::cout << "System component file written: " << systemComponentFile << std::endl;
    }
#endif
}

std::string LinuxSystemIntegration::generateComponentXML(const std::string& engineId) {
    std::ostringstream xml;
    
    xml << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    xml << "<component>\n";
    xml << "  <name>org.freedesktop.IBus.OwCat</name>\n";
    xml << "  <description>OwCat Chinese Input Method</description>\n";
    xml << "  <exec>/usr/libexec/ibus-engine-owcat</exec>\n";
    xml << "  <version>1.0.0</version>\n";
    xml << "  <author>OwCat Team</author>\n";
    xml << "  <license>MIT</license>\n";
    xml << "  <homepage>https://github.com/owcat/owcat</homepage>\n";
    xml << "  <textdomain>owcat</textdomain>\n";
    xml << "  <engines>\n";
    xml << "    <engine>\n";
    xml << "      <name>" << engineId << "</name>\n";
    xml << "      <language>zh</language>\n";
    xml << "      <license>MIT</license>\n";
    xml << "      <author>OwCat Team</author>\n";
    xml << "      <icon>owcat</icon>\n";
    xml << "      <layout>us</layout>\n";
    xml << "      <longname>OwCat Chinese Input Method</longname>\n";
    xml << "      <description>OwCat Chinese Input Method with AI-powered prediction</description>\n";
    xml << "      <rank>50</rank>\n";
    xml << "    </engine>\n";
    xml << "  </engines>\n";
    xml << "</component>\n";
    
    return xml.str();
}

} // namespace owcat