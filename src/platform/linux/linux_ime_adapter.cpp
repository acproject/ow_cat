#include "platform/linux/linux_ime_adapter.h"
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <chrono>

#ifdef __linux__
#include <ibus.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <cstring>
#include <cstdlib>
#endif

namespace owcat {

// Implementation structure
struct LinuxImeAdapter::Impl {
#ifdef __linux__
    IBusBus* bus;
    IBusEngine* engine;
    GMainLoop* mainLoop;
    std::thread* mainLoopThread;
#endif
    
    // Components
    std::unique_ptr<LinuxInputEngine> inputEngine;
    std::unique_ptr<LinuxCandidateWindow> candidateWindow;
    std::unique_ptr<LinuxSystemIntegration> systemIntegration;
    
    // Callbacks
    KeyEventCallback keyEventCallback;
    StateChangeCallback stateChangeCallback;
    FocusChangeCallback focusChangeCallback;
    
    // State
    bool isInitialized;
    bool isEnabled;
    bool isActive;
    bool hasFocus;
    std::string engineName;
    std::string displayName;
    std::string currentComposition;
    int compositionCursor;
    
    // Configuration
    std::map<std::string, std::string> config;
    
    Impl() {
#ifdef __linux__
        bus = nullptr;
        engine = nullptr;
        mainLoop = nullptr;
        mainLoopThread = nullptr;
#endif
        isInitialized = false;
        isEnabled = false;
        isActive = false;
        hasFocus = false;
        compositionCursor = 0;
        engineName = "owcat";
        displayName = "OwCat IME";
    }
    
    ~Impl() {
#ifdef __linux__
        if (mainLoopThread && mainLoopThread->joinable()) {
            if (mainLoop) {
                g_main_loop_quit(mainLoop);
            }
            mainLoopThread->join();
            delete mainLoopThread;
        }
        
        if (mainLoop) {
            g_main_loop_unref(mainLoop);
        }
        
        if (engine) {
            g_object_unref(engine);
        }
        
        if (bus) {
            g_object_unref(bus);
        }
#endif
    }
};

LinuxImeAdapter::LinuxImeAdapter() : pImpl(std::make_unique<Impl>()) {
    pImpl->inputEngine = std::make_unique<LinuxInputEngine>();
    pImpl->candidateWindow = std::make_unique<LinuxCandidateWindow>();
    pImpl->systemIntegration = std::make_unique<LinuxSystemIntegration>();
}

LinuxImeAdapter::~LinuxImeAdapter() = default;

bool LinuxImeAdapter::initialize() {
#ifdef __linux__
    if (pImpl->isInitialized) {
        return true;
    }
    
    // Initialize GTK if not already initialized
    if (!gtk_init_check(nullptr, nullptr)) {
        std::cerr << "Failed to initialize GTK" << std::endl;
        return false;
    }
    
    // Initialize IBus
    ibus_init();
    
    if (!initializeIBus()) {
        std::cerr << "Failed to initialize IBus" << std::endl;
        return false;
    }
    
    // Initialize components
    if (!pImpl->inputEngine->initialize()) {
        std::cerr << "Failed to initialize input engine" << std::endl;
        return false;
    }
    
    if (!pImpl->candidateWindow->initialize()) {
        std::cerr << "Failed to initialize candidate window" << std::endl;
        return false;
    }
    
    if (!pImpl->systemIntegration->initialize()) {
        std::cerr << "Failed to initialize system integration" << std::endl;
        return false;
    }
    
    // Set up callbacks
    initializeCallbacks();
    
    // Load configuration
    loadConfiguration();
    
    pImpl->isInitialized = true;
    std::cout << "Linux IME adapter initialized successfully" << std::endl;
    return true;
#else
    std::cerr << "Linux IME adapter not supported on this platform" << std::endl;
    return false;
#endif
}

void LinuxImeAdapter::shutdown() {
#ifdef __linux__
    if (!pImpl->isInitialized) {
        return;
    }
    
    // Save configuration
    saveConfiguration();
    
    // Shutdown components
    if (pImpl->candidateWindow) {
        pImpl->candidateWindow->shutdown();
    }
    
    if (pImpl->inputEngine) {
        pImpl->inputEngine->shutdown();
    }
    
    if (pImpl->systemIntegration) {
        pImpl->systemIntegration->shutdown();
    }
    
    // Shutdown IBus
    shutdownIBus();
    
    pImpl->isInitialized = false;
    std::cout << "Linux IME adapter shut down" << std::endl;
#endif
}

bool LinuxImeAdapter::registerInputMethod(const std::string& id, const std::string& name) {
#ifdef __linux__
    pImpl->engineName = id;
    pImpl->displayName = name;
    
    return registerWithIBus(id, name);
#else
    return false;
#endif
}

bool LinuxImeAdapter::unregisterInputMethod(const std::string& id) {
#ifdef __linux__
    return pImpl->systemIntegration->unregisterIME(id);
#else
    return false;
#endif
}

bool LinuxImeAdapter::enableInputMethod() {
    pImpl->isEnabled = true;
    
    if (pImpl->stateChangeCallback) {
        pImpl->stateChangeCallback(PlatformInputState::Enabled);
    }
    
    return true;
}

bool LinuxImeAdapter::disableInputMethod() {
    pImpl->isEnabled = false;
    
    // Hide candidate window
    hideCandidateWindow();
    
    // End any active composition
    if (!pImpl->currentComposition.empty()) {
        endComposition();
    }
    
    if (pImpl->stateChangeCallback) {
        pImpl->stateChangeCallback(PlatformInputState::Disabled);
    }
    
    return true;
}

bool LinuxImeAdapter::activateInputMethod() {
    pImpl->isActive = true;
    
    if (pImpl->stateChangeCallback) {
        pImpl->stateChangeCallback(PlatformInputState::Active);
    }
    
    return true;
}

void LinuxImeAdapter::startComposition() {
    pImpl->currentComposition.clear();
    pImpl->compositionCursor = 0;
    
#ifdef __linux__
    if (pImpl->inputEngine) {
        pImpl->inputEngine->showPreeditText();
    }
#endif
}

void LinuxImeAdapter::updateComposition(const std::string& text, int cursorPos) {
    pImpl->currentComposition = text;
    pImpl->compositionCursor = cursorPos;
    
#ifdef __linux__
    if (pImpl->inputEngine) {
        pImpl->inputEngine->updatePreeditText(text, cursorPos, true);
    }
#endif
}

void LinuxImeAdapter::endComposition() {
    pImpl->currentComposition.clear();
    pImpl->compositionCursor = 0;
    
#ifdef __linux__
    if (pImpl->inputEngine) {
        pImpl->inputEngine->hidePreeditText();
    }
#endif
}

void LinuxImeAdapter::commitText(const std::string& text) {
#ifdef __linux__
    if (pImpl->inputEngine) {
        pImpl->inputEngine->commitText(text);
    }
#endif
    
    // End composition after committing
    endComposition();
}

void LinuxImeAdapter::showCandidateWindow(const std::vector<std::string>& candidates, int selectedIndex, int x, int y) {
    if (pImpl->candidateWindow) {
        pImpl->candidateWindow->show(candidates, selectedIndex, x, y);
    }
    
#ifdef __linux__
    if (pImpl->inputEngine) {
        pImpl->inputEngine->updateLookupTable(candidates, selectedIndex, true);
    }
#endif
}

void LinuxImeAdapter::hideCandidateWindow() {
    if (pImpl->candidateWindow) {
        pImpl->candidateWindow->hide();
    }
    
#ifdef __linux__
    if (pImpl->inputEngine) {
        pImpl->inputEngine->hideLookupTable();
    }
#endif
}

void LinuxImeAdapter::updateCandidateWindow(const std::vector<std::string>& candidates, int selectedIndex) {
    if (pImpl->candidateWindow) {
        pImpl->candidateWindow->updateCandidates(candidates, selectedIndex);
    }
    
#ifdef __linux__
    if (pImpl->inputEngine) {
        pImpl->inputEngine->updateLookupTable(candidates, selectedIndex, true);
    }
#endif
}

CursorPosition LinuxImeAdapter::getCursorPosition() {
    CursorPosition pos = {0, 0};
    
#ifdef __linux__
    // Get cursor position from X11
    Display* display = XOpenDisplay(nullptr);
    if (display) {
        Window root, child;
        int root_x, root_y, win_x, win_y;
        unsigned int mask;
        
        if (XQueryPointer(display, DefaultRootWindow(display), &root, &child, 
                         &root_x, &root_y, &win_x, &win_y, &mask)) {
            pos.x = root_x;
            pos.y = root_y;
        }
        
        XCloseDisplay(display);
    }
#endif
    
    return pos;
}

ApplicationInfo LinuxImeAdapter::getCurrentApplication() {
    ApplicationInfo info;
    info.name = "Unknown";
    info.processId = 0;
    info.windowTitle = "";
    
#ifdef __linux__
    // Get current application info from system integration
    if (pImpl->systemIntegration) {
        auto processInfo = pImpl->systemIntegration->getCurrentProcessInfo();
        if (!processInfo.empty()) {
            info.name = processInfo["name"];
            info.processId = std::stoi(processInfo["pid"]);
            info.windowTitle = processInfo["window_title"];
        }
    }
#endif
    
    return info;
}

void LinuxImeAdapter::setKeyEventCallback(KeyEventCallback callback) {
    pImpl->keyEventCallback = callback;
}

void LinuxImeAdapter::setStateChangeCallback(StateChangeCallback callback) {
    pImpl->stateChangeCallback = callback;
}

void LinuxImeAdapter::setFocusChangeCallback(FocusChangeCallback callback) {
    pImpl->focusChangeCallback = callback;
}

std::string LinuxImeAdapter::getPlatformName() const {
    return "Linux";
}

std::string LinuxImeAdapter::getPlatformVersion() const {
    if (pImpl->systemIntegration) {
        return pImpl->systemIntegration->getSystemVersion();
    }
    return "Unknown";
}

std::vector<std::string> LinuxImeAdapter::getSupportedFeatures() const {
    return {
        "composition",
        "candidates",
        "preedit",
        "lookup_table",
        "properties",
        "focus_events",
        "key_events",
        "ibus_integration"
    };
}

std::map<std::string, std::string> LinuxImeAdapter::getPlatformConfiguration() const {
    std::map<std::string, std::string> config;
    
    config["platform"] = "Linux";
    config["framework"] = "IBus";
    config["engine_name"] = pImpl->engineName;
    config["display_name"] = pImpl->displayName;
    config["enabled"] = pImpl->isEnabled ? "true" : "false";
    config["active"] = pImpl->isActive ? "true" : "false";
    config["has_focus"] = pImpl->hasFocus ? "true" : "false";
    
#ifdef __linux__
    if (pImpl->systemIntegration) {
        config["desktop_environment"] = pImpl->systemIntegration->getDesktopEnvironment();
        config["display_server"] = pImpl->systemIntegration->getDisplayServer();
        config["ibus_version"] = pImpl->systemIntegration->getIBusVersion();
        config["ibus_running"] = pImpl->systemIntegration->isIBusRunning() ? "true" : "false";
    }
#endif
    
    return config;
}

// Linux-specific methods
bool LinuxImeAdapter::initializeIBus() {
#ifdef __linux__
    // Create IBus bus connection
    pImpl->bus = ibus_bus_new();
    if (!pImpl->bus) {
        std::cerr << "Failed to create IBus bus connection" << std::endl;
        return false;
    }
    
    // Check if IBus daemon is running
    if (!ibus_bus_is_connected(pImpl->bus)) {
        std::cerr << "IBus daemon is not running" << std::endl;
        
        // Try to start IBus daemon
        if (pImpl->systemIntegration && pImpl->systemIntegration->startIBusDaemon()) {
            // Wait a bit for daemon to start
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            
            if (!ibus_bus_is_connected(pImpl->bus)) {
                std::cerr << "Failed to connect to IBus daemon after starting it" << std::endl;
                return false;
            }
        } else {
            return false;
        }
    }
    
    // Set up main loop for IBus events
    pImpl->mainLoop = g_main_loop_new(nullptr, FALSE);
    if (!pImpl->mainLoop) {
        std::cerr << "Failed to create GMainLoop" << std::endl;
        return false;
    }
    
    // Start main loop in separate thread
    pImpl->mainLoopThread = new std::thread([this]() {
        g_main_loop_run(pImpl->mainLoop);
    });
    
    std::cout << "IBus initialized successfully" << std::endl;
    return true;
#else
    return false;
#endif
}

void LinuxImeAdapter::shutdownIBus() {
#ifdef __linux__
    // Stop main loop
    if (pImpl->mainLoop) {
        g_main_loop_quit(pImpl->mainLoop);
    }
    
    // Wait for main loop thread to finish
    if (pImpl->mainLoopThread && pImpl->mainLoopThread->joinable()) {
        pImpl->mainLoopThread->join();
        delete pImpl->mainLoopThread;
        pImpl->mainLoopThread = nullptr;
    }
    
    // Clean up IBus resources
    if (pImpl->engine) {
        g_object_unref(pImpl->engine);
        pImpl->engine = nullptr;
    }
    
    if (pImpl->bus) {
        g_object_unref(pImpl->bus);
        pImpl->bus = nullptr;
    }
    
    if (pImpl->mainLoop) {
        g_main_loop_unref(pImpl->mainLoop);
        pImpl->mainLoop = nullptr;
    }
#endif
}

bool LinuxImeAdapter::registerWithIBus(const std::string& engineName, const std::string& displayName) {
#ifdef __linux__
    return pImpl->systemIntegration->registerIME(engineName, displayName, "OwCat Chinese Input Method");
#else
    return false;
#endif
}

bool LinuxImeAdapter::connectToIBusDaemon() {
#ifdef __linux__
    if (!pImpl->bus) {
        return false;
    }
    
    return ibus_bus_is_connected(pImpl->bus);
#else
    return false;
#endif
}

void LinuxImeAdapter::disconnectFromIBusDaemon() {
#ifdef __linux__
    if (pImpl->bus) {
        // IBus will handle disconnection automatically
    }
#endif
}

void LinuxImeAdapter::setInputEngine(LinuxInputEngine* engine) {
    // This would typically be set during initialization
}

LinuxInputEngine* LinuxImeAdapter::getInputEngine() const {
    return pImpl->inputEngine.get();
}

void LinuxImeAdapter::setCandidateWindow(LinuxCandidateWindow* window) {
    // This would typically be set during initialization
}

LinuxCandidateWindow* LinuxImeAdapter::getCandidateWindow() const {
    return pImpl->candidateWindow.get();
}

void LinuxImeAdapter::setSystemIntegration(LinuxSystemIntegration* integration) {
    // This would typically be set during initialization
}

LinuxSystemIntegration* LinuxImeAdapter::getSystemIntegration() const {
    return pImpl->systemIntegration.get();
}

// IBus event handlers
void LinuxImeAdapter::handleIBusConnected() {
    std::cout << "Connected to IBus daemon" << std::endl;
}

void LinuxImeAdapter::handleIBusDisconnected() {
    std::cout << "Disconnected from IBus daemon" << std::endl;
}

void LinuxImeAdapter::handleEngineEnabled() {
    pImpl->isEnabled = true;
    
    if (pImpl->stateChangeCallback) {
        pImpl->stateChangeCallback(PlatformInputState::Enabled);
    }
}

void LinuxImeAdapter::handleEngineDisabled() {
    pImpl->isEnabled = false;
    
    if (pImpl->stateChangeCallback) {
        pImpl->stateChangeCallback(PlatformInputState::Disabled);
    }
}

void LinuxImeAdapter::handleEngineFocusIn() {
    pImpl->hasFocus = true;
    
    if (pImpl->focusChangeCallback) {
        pImpl->focusChangeCallback(true);
    }
}

void LinuxImeAdapter::handleEngineFocusOut() {
    pImpl->hasFocus = false;
    
    // Hide candidate window when losing focus
    hideCandidateWindow();
    
    // End composition when losing focus
    if (!pImpl->currentComposition.empty()) {
        endComposition();
    }
    
    if (pImpl->focusChangeCallback) {
        pImpl->focusChangeCallback(false);
    }
}

void LinuxImeAdapter::handleEngineReset() {
    // Reset composition state
    endComposition();
    hideCandidateWindow();
}

// Input processing
bool LinuxImeAdapter::processKeyEvent(unsigned int keyval, unsigned int keycode, unsigned int state) {
    if (!pImpl->isEnabled || !pImpl->hasFocus) {
        return false;
    }
    
    // Create key event
    PlatformKeyEvent keyEvent;
    keyEvent.keyCode = keycode;
    keyEvent.virtualKey = keyval;
    keyEvent.scanCode = 0; // Not used on Linux
    keyEvent.isPressed = true; // Assume key press for now
    keyEvent.modifiers = 0;
    
#ifdef __linux__
    // Convert state to modifiers
    if (state & GDK_SHIFT_MASK) keyEvent.modifiers |= 0x01;
    if (state & GDK_CONTROL_MASK) keyEvent.modifiers |= 0x02;
    if (state & GDK_MOD1_MASK) keyEvent.modifiers |= 0x04; // Alt
#endif
    
    // Call key event callback
    if (pImpl->keyEventCallback) {
        return pImpl->keyEventCallback(keyEvent);
    }
    
    return false;
}

void LinuxImeAdapter::updatePreeditText(const std::string& text, int cursorPos) {
    updateComposition(text, cursorPos);
}

void LinuxImeAdapter::commitString(const std::string& text) {
    commitText(text);
}

void LinuxImeAdapter::updateLookupTable(const std::vector<std::string>& candidates, int selectedIndex, bool visible) {
    if (visible) {
        auto pos = getCursorPosition();
        showCandidateWindow(candidates, selectedIndex, pos.x, pos.y + 20);
    } else {
        hideCandidateWindow();
    }
}

// Configuration
void LinuxImeAdapter::loadConfiguration() {
    // Load configuration from file
    std::string configFile = pImpl->systemIntegration->getConfigDirectory() + "/owcat/config.ini";
    std::ifstream file(configFile);
    
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            size_t pos = line.find('=');
            if (pos != std::string::npos) {
                std::string key = line.substr(0, pos);
                std::string value = line.substr(pos + 1);
                pImpl->config[key] = value;
            }
        }
        file.close();
    }
}

void LinuxImeAdapter::saveConfiguration() {
    // Save configuration to file
    std::string configDir = pImpl->systemIntegration->getConfigDirectory() + "/owcat";
    std::string configFile = configDir + "/config.ini";
    
    // Create directory if it doesn't exist
    system(("mkdir -p " + configDir).c_str());
    
    std::ofstream file(configFile);
    if (file.is_open()) {
        for (const auto& pair : pImpl->config) {
            file << pair.first << "=" << pair.second << std::endl;
        }
        file.close();
    }
}

std::string LinuxImeAdapter::getConfigValue(const std::string& key, const std::string& defaultValue) const {
    auto it = pImpl->config.find(key);
    return (it != pImpl->config.end()) ? it->second : defaultValue;
}

void LinuxImeAdapter::setConfigValue(const std::string& key, const std::string& value) {
    pImpl->config[key] = value;
}

// Private helper methods
void LinuxImeAdapter::initializeCallbacks() {
    // Set up callbacks between components
    if (pImpl->candidateWindow) {
        pImpl->candidateWindow->setSelectionCallback([this](const std::string& candidate) {
            commitText(candidate);
        });
    }
    
    if (pImpl->inputEngine) {
        pImpl->inputEngine->setProcessKeyEventCallback([this](unsigned int keyval, unsigned int keycode, unsigned int state) {
            return processKeyEvent(keyval, keycode, state);
        });
        
        pImpl->inputEngine->setFocusInCallback([this]() {
            handleEngineFocusIn();
        });
        
        pImpl->inputEngine->setFocusOutCallback([this]() {
            handleEngineFocusOut();
        });
        
        pImpl->inputEngine->setResetCallback([this]() {
            handleEngineReset();
        });
        
        pImpl->inputEngine->setEnableCallback([this]() {
            handleEngineEnabled();
        });
        
        pImpl->inputEngine->setDisableCallback([this]() {
            handleEngineDisabled();
        });
    }
}

void LinuxImeAdapter::setupIBusEngine() {
#ifdef __linux__
    // This would be called when IBus creates an engine instance
    // The actual engine setup is handled by the IBus framework
#endif
}

void LinuxImeAdapter::registerEngineWithIBus() {
#ifdef __linux__
    // Register the engine component with IBus
    registerWithIBus(pImpl->engineName, pImpl->displayName);
#endif
}

void LinuxImeAdapter::handleKeyEventInternal(unsigned int keyval, unsigned int keycode, unsigned int state) {
    processKeyEvent(keyval, keycode, state);
}

void LinuxImeAdapter::updateClientInfo() {
    // Update information about the current client application
    // This is called when focus changes or when a new application becomes active
}

} // namespace owcat