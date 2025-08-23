#pragma once

#include "platform/platform_manager.h"
#include <memory>
#include <string>
#include <vector>
#include <functional>

// Forward declarations for Linux-specific types
#ifdef __linux__
struct _GDBusConnection;
struct _IBusEngine;
struct _IBusEngineClass;
struct _IBusText;
struct _IBusProperty;
struct _IBusLookupTable;
struct _IBusPropList;
typedef struct _GDBusConnection GDBusConnection;
typedef struct _IBusEngine IBusEngine;
typedef struct _IBusEngineClass IBusEngineClass;
typedef struct _IBusText IBusText;
typedef struct _IBusProperty IBusProperty;
typedef struct _IBusLookupTable IBusLookupTable;
typedef struct _IBusPropList IBusPropList;
#endif

namespace owcat {

// Forward declarations
class LinuxInputEngine;
class LinuxCandidateWindow;
class LinuxSystemIntegration;

/**
 * @brief Linux IME adapter using IBus framework
 * 
 * This class provides Linux-specific IME integration using the IBus
 * (Intelligent Input Bus) framework, which is the standard input method
 * framework for most Linux distributions.
 */
class LinuxImeAdapter : public PlatformManager {
public:
    LinuxImeAdapter();
    ~LinuxImeAdapter() override;

    // PlatformManager interface implementation
    bool initialize() override;
    void shutdown() override;
    
    bool registerInputMethod(const std::string& id, const std::string& name) override;
    bool unregisterInputMethod(const std::string& id) override;
    
    bool enableInputMethod() override;
    bool disableInputMethod() override;
    bool activateInputMethod() override;
    
    void startComposition() override;
    void updateComposition(const std::string& text, int cursorPos) override;
    void endComposition() override;
    void commitText(const std::string& text) override;
    
    void showCandidateWindow(const std::vector<std::string>& candidates, int selectedIndex, int x, int y) override;
    void hideCandidateWindow() override;
    void updateCandidateWindow(const std::vector<std::string>& candidates, int selectedIndex) override;
    
    CursorPosition getCursorPosition() override;
    ApplicationInfo getCurrentApplication() override;
    
    void setKeyEventCallback(KeyEventCallback callback) override;
    void setStateChangeCallback(StateChangeCallback callback) override;
    void setFocusChangeCallback(FocusChangeCallback callback) override;
    
    std::string getPlatformName() const override;
    std::string getPlatformVersion() const override;
    std::vector<std::string> getSupportedFeatures() const override;
    std::map<std::string, std::string> getPlatformConfiguration() const override;

    // Linux-specific methods
    bool initializeIBus();
    void shutdownIBus();
    bool registerWithIBus(const std::string& engineName, const std::string& displayName);
    bool connectToIBusDaemon();
    void disconnectFromIBusDaemon();
    
    // Engine management
    void setInputEngine(LinuxInputEngine* engine);
    LinuxInputEngine* getInputEngine() const;
    
    // Candidate window management
    void setCandidateWindow(LinuxCandidateWindow* window);
    LinuxCandidateWindow* getCandidateWindow() const;
    
    // System integration
    void setSystemIntegration(LinuxSystemIntegration* integration);
    LinuxSystemIntegration* getSystemIntegration() const;
    
    // IBus event handlers
    void handleIBusConnected();
    void handleIBusDisconnected();
    void handleEngineEnabled();
    void handleEngineDisabled();
    void handleEngineFocusIn();
    void handleEngineFocusOut();
    void handleEngineReset();
    
    // Input processing
    bool processKeyEvent(unsigned int keyval, unsigned int keycode, unsigned int state);
    void updatePreeditText(const std::string& text, int cursorPos);
    void commitString(const std::string& text);
    void updateLookupTable(const std::vector<std::string>& candidates, int selectedIndex, bool visible);
    
    // Configuration
    void loadConfiguration();
    void saveConfiguration();
    std::string getConfigValue(const std::string& key, const std::string& defaultValue = "") const;
    void setConfigValue(const std::string& key, const std::string& value);

private:
    struct Impl;
    std::unique_ptr<Impl> pImpl;
    
    // Internal helper methods
    void initializeCallbacks();
    void setupIBusEngine();
    void registerEngineWithIBus();
    void handleKeyEventInternal(unsigned int keyval, unsigned int keycode, unsigned int state);
    void updateClientInfo();
};

/**
 * @brief Linux input engine for IBus integration
 * 
 * This class handles the actual input processing and text generation
 * for the Linux platform using IBus engine interface.
 */
class LinuxInputEngine {
public:
    LinuxInputEngine();
    ~LinuxInputEngine();
    
    bool initialize();
    void shutdown();
    
    // Engine callbacks
    void setProcessKeyEventCallback(std::function<bool(unsigned int, unsigned int, unsigned int)> callback);
    void setFocusInCallback(std::function<void()> callback);
    void setFocusOutCallback(std::function<void()> callback);
    void setResetCallback(std::function<void()> callback);
    void setEnableCallback(std::function<void()> callback);
    void setDisableCallback(std::function<void()> callback);
    void setPageUpCallback(std::function<void()> callback);
    void setPageDownCallback(std::function<void()> callback);
    void setCursorUpCallback(std::function<void()> callback);
    void setCursorDownCallback(std::function<void()> callback);
    void setPropertyActivateCallback(std::function<void(const std::string&, unsigned int)> callback);
    void setPropertyShowCallback(std::function<void(const std::string&)> callback);
    void setPropertyHideCallback(std::function<void(const std::string&)> callback);
    
    // Input processing
    bool handleKeyEvent(unsigned int keyval, unsigned int keycode, unsigned int state);
    void handleFocusIn();
    void handleFocusOut();
    void handleReset();
    void handleEnable();
    void handleDisable();
    void handlePageUp();
    void handlePageDown();
    void handleCursorUp();
    void handleCursorDown();
    void handlePropertyActivate(const std::string& propName, unsigned int propState);
    void handlePropertyShow(const std::string& propName);
    void handlePropertyHide(const std::string& propName);
    
    // Text output
    void commitText(const std::string& text);
    void updatePreeditText(const std::string& text, int cursorPos, bool visible);
    void showPreeditText();
    void hidePreeditText();
    
    // Lookup table (candidate window)
    void updateLookupTable(const std::vector<std::string>& candidates, int selectedIndex, bool visible);
    void showLookupTable();
    void hideLookupTable();
    
    // Properties
    void registerProperties(const std::vector<std::pair<std::string, std::string>>& properties);
    void updateProperty(const std::string& name, const std::string& label, bool visible);
    
    // State management
    bool isEnabled() const;
    bool hasFocus() const;
    std::string getCurrentInputMode() const;
    void setInputMode(const std::string& mode);
    
#ifdef __linux__
    // IBus engine access
    IBusEngine* getIBusEngine() const;
    void setIBusEngine(IBusEngine* engine);
#endif

private:
    struct Impl;
    std::unique_ptr<Impl> pImpl;
};

/**
 * @brief Linux candidate window implementation
 * 
 * This class provides candidate window functionality for Linux
 * using GTK+ for the user interface.
 */
class LinuxCandidateWindow {
public:
    LinuxCandidateWindow();
    ~LinuxCandidateWindow();
    
    bool initialize();
    void shutdown();
    
    void show(const std::vector<std::string>& candidates, int selectedIndex, int x, int y);
    void hide();
    void updateCandidates(const std::vector<std::string>& candidates, int selectedIndex);
    void updateSelection(int selectedIndex);
    void updatePosition(int x, int y);
    
    // Callbacks
    void setSelectionCallback(std::function<void(const std::string&)> callback);
    void setPageUpCallback(std::function<void()> callback);
    void setPageDownCallback(std::function<void()> callback);
    
    // Appearance
    void setFont(const std::string& fontName, int fontSize);
    void setColors(const std::string& backgroundColor, const std::string& textColor, 
                   const std::string& selectedBackgroundColor, const std::string& selectedTextColor);
    void setBorderStyle(const std::string& borderColor, int borderWidth);
    
    // State
    bool isVisible() const;
    std::vector<std::string> getCandidates() const;
    int getSelectedIndex() const;
    
    // Internal event handlers
    void handleMouseClick(int x, int y);
    void handleMouseMove(int x, int y);
    void handleKeyPress(unsigned int keyval, unsigned int state);
    
private:
    struct Impl;
    std::unique_ptr<Impl> pImpl;
    
    void createWindow();
    void destroyWindow();
    void updateWindowSize();
    void renderCandidates();
    int getCandidateIndexAtPosition(int x, int y);
};

/**
 * @brief Linux system integration utilities
 * 
 * This class provides system-level functionality for Linux,
 * including process management, system information, and
 * input method registration.
 */
class LinuxSystemIntegration {
public:
    LinuxSystemIntegration();
    ~LinuxSystemIntegration();
    
    bool initialize();
    void shutdown();
    
    // Input method registration
    bool registerIME(const std::string& engineName, const std::string& displayName, const std::string& description);
    bool unregisterIME(const std::string& engineName);
    bool installIMEComponent(const std::string& componentFile);
    bool uninstallIMEComponent(const std::string& componentFile);
    
    // System information
    std::string getSystemVersion();
    std::map<std::string, std::string> getSystemInfo();
    std::string getDesktopEnvironment();
    std::string getDisplayServer(); // X11, Wayland, etc.
    
    // Process management
    std::vector<std::map<std::string, std::string>> getRunningProcesses();
    std::map<std::string, std::string> getCurrentProcessInfo();
    bool isProcessRunning(const std::string& processName);
    
    // Input method management
    std::vector<std::string> getInstalledIMEs();
    bool isIMEInstalled(const std::string& engineName);
    std::string getCurrentIME();
    bool setCurrentIME(const std::string& engineName);
    
    // IBus specific
    bool isIBusRunning();
    bool startIBusDaemon();
    bool stopIBusDaemon();
    bool restartIBusDaemon();
    std::string getIBusVersion();
    
    // Fcitx specific (for future support)
    bool isFcitxRunning();
    bool startFcitx();
    bool stopFcitx();
    std::string getFcitxVersion();
    
    // Configuration
    std::string getConfigDirectory();
    std::string getDataDirectory();
    std::string getCacheDirectory();
    bool createDirectories();
    
    // Permissions and capabilities
    bool checkPermissions();
    bool hasX11Access();
    bool hasWaylandAccess();
    
    // Error handling
    std::string getLastError();
    
private:
    struct Impl;
    std::unique_ptr<Impl> pImpl;
    
    // Internal helper methods
    void detectDesktopEnvironment();
    void detectDisplayServer();
    bool executeCommand(const std::string& command, std::string& output);
    std::vector<std::string> parseProcessList(const std::string& psOutput);
};

} // namespace owcat

// C interface for IBus integration
extern "C" {
    // Engine factory functions
    void* createLinuxInputEngine();
    void destroyLinuxInputEngine(void* engine);
    
    // IBus engine callbacks
    bool linuxEngineProcessKeyEvent(void* engine, unsigned int keyval, unsigned int keycode, unsigned int state);
    void linuxEngineFocusIn(void* engine);
    void linuxEngineFocusOut(void* engine);
    void linuxEngineReset(void* engine);
    void linuxEngineEnable(void* engine);
    void linuxEngineDisable(void* engine);
    void linuxEnginePageUp(void* engine);
    void linuxEnginePageDown(void* engine);
    void linuxEngineCursorUp(void* engine);
    void linuxEngineCursorDown(void* engine);
    void linuxEnginePropertyActivate(void* engine, const char* propName, unsigned int propState);
    void linuxEnginePropertyShow(void* engine, const char* propName);
    void linuxEnginePropertyHide(void* engine, const char* propName);
}