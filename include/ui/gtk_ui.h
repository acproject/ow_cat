#pragma once

#include "core/platform_manager.h"
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <functional>

#ifdef OWCAT_USE_GTK
// Forward declarations for GTK types
typedef struct _GtkWidget GtkWidget;
typedef struct _GtkWindow GtkWindow;
typedef struct _GtkApplication GtkApplication;
typedef struct _GtkBuilder GtkBuilder;
typedef struct _GdkPixbuf GdkPixbuf;
typedef struct _GtkCssProvider GtkCssProvider;
typedef struct _GtkStyleContext GtkStyleContext;
typedef struct _GtkTreeView GtkTreeView;
typedef struct _GtkListStore GtkListStore;
typedef struct _GtkTreeSelection GtkTreeSelection;
typedef struct _GtkNotebook GtkNotebook;
typedef struct _GtkStatusbar GtkStatusbar;
typedef struct _GtkMenuBar GtkMenuBar;
typedef struct _GtkToolbar GtkToolbar;
typedef struct _GtkEntry GtkEntry;
typedef struct _GtkTextView GtkTextView;
typedef struct _GtkTextBuffer GtkTextBuffer;
typedef struct _GtkLabel GtkLabel;
typedef struct _GtkButton GtkButton;
typedef struct _GtkCheckButton GtkCheckButton;
typedef struct _GtkSpinButton GtkSpinButton;
typedef struct _GtkComboBox GtkComboBox;
typedef struct _GtkScale GtkScale;
typedef struct _GtkProgressBar GtkProgressBar;
typedef struct _GtkDialog GtkDialog;
typedef struct _GtkFileChooserDialog GtkFileChooserDialog;
typedef struct _GtkAboutDialog GtkAboutDialog;
#endif

namespace owcat {

// Forward declarations
class GtkMainWindow;
class GtkCandidateWindow;
class GtkSettingsDialog;
class GtkAboutDialog;
class GtkStatusIcon;
class GtkThemeManager;

/**
 * @brief GTK-based user interface manager
 * 
 * Provides a cross-platform GUI using GTK+ for the OwCat IME.
 * Manages main window, candidate window, settings dialog, and system tray integration.
 */
class GtkUI {
public:
    GtkUI();
    ~GtkUI();

    // Core lifecycle
    bool initialize(int argc, char* argv[]);
    void shutdown();
    void run();
    void quit();

    // Window management
    bool showMainWindow();
    bool hideMainWindow();
    bool isMainWindowVisible() const;
    
    bool showCandidateWindow(const std::vector<std::string>& candidates, int x, int y);
    bool hideCandidateWindow();
    bool updateCandidateWindow(const std::vector<std::string>& candidates);
    bool setCandidateSelection(int index);
    
    bool showSettingsDialog();
    bool hideSettingsDialog();
    
    bool showAboutDialog();
    
    // System tray
    bool createSystemTrayIcon();
    bool removeSystemTrayIcon();
    bool updateSystemTrayIcon(const std::string& iconName, const std::string& tooltip);
    
    // Theme and styling
    bool loadTheme(const std::string& themeName);
    bool setCustomCSS(const std::string& css);
    std::vector<std::string> getAvailableThemes() const;
    
    // Callbacks
    void setOnCandidateSelected(std::function<void(int)> callback);
    void setOnCandidateHighlighted(std::function<void(int)> callback);
    void setOnSettingsChanged(std::function<void(const std::map<std::string, std::string>&)> callback);
    void setOnMainWindowClosed(std::function<void()> callback);
    void setOnSystemTrayActivated(std::function<void()> callback);
    void setOnQuitRequested(std::function<void()> callback);
    
    // Configuration
    void setConfiguration(const std::map<std::string, std::string>& config);
    std::map<std::string, std::string> getConfiguration() const;
    
    // Utility
    void showNotification(const std::string& title, const std::string& message);
    void showErrorDialog(const std::string& title, const std::string& message);
    bool showConfirmDialog(const std::string& title, const std::string& message);
    std::string showFileDialog(const std::string& title, bool save = false, const std::string& filter = "");
    
    // Accessibility
    void setAccessibilityEnabled(bool enabled);
    bool isAccessibilityEnabled() const;
    
    // Internationalization
    void setLanguage(const std::string& language);
    std::string getCurrentLanguage() const;
    std::vector<std::string> getAvailableLanguages() const;
    
    // Platform integration
    void setPlatformManager(std::shared_ptr<PlatformManager> manager);
    std::shared_ptr<PlatformManager> getPlatformManager() const;

private:
    struct Impl;
    std::unique_ptr<Impl> pImpl;
};

/**
 * @brief Main application window
 * 
 * Provides the main interface for OwCat IME configuration and monitoring.
 */
class GtkMainWindow {
public:
    GtkMainWindow();
    ~GtkMainWindow();

    bool create(GtkApplication* app);
    void destroy();
    
    bool show();
    bool hide();
    bool isVisible() const;
    
    void setTitle(const std::string& title);
    void setSize(int width, int height);
    void setPosition(int x, int y);
    void center();
    
    // Content management
    void updateStatus(const std::string& status);
    void updateStatistics(const std::map<std::string, std::string>& stats);
    void updateInputHistory(const std::vector<std::string>& history);
    void updateDictionaries(const std::vector<std::map<std::string, std::string>>& dictionaries);
    
    // Callbacks
    void setOnClosed(std::function<void()> callback);
    void setOnSettingsRequested(std::function<void()> callback);
    void setOnAboutRequested(std::function<void()> callback);
    void setOnDictionarySelected(std::function<void(const std::string&)> callback);
    void setOnClearHistoryRequested(std::function<void()> callback);
    
    // Widget access
    GtkWidget* getWidget() const;
    GtkWindow* getWindow() const;

private:
    struct Impl;
    std::unique_ptr<Impl> pImpl;
};

/**
 * @brief Candidate selection window
 * 
 * Displays input candidates and handles user selection.
 */
class GtkCandidateWindow {
public:
    GtkCandidateWindow();
    ~GtkCandidateWindow();

    bool create();
    void destroy();
    
    bool show(const std::vector<std::string>& candidates, int x, int y);
    bool hide();
    bool isVisible() const;
    
    bool updateCandidates(const std::vector<std::string>& candidates);
    bool setSelection(int index);
    int getSelection() const;
    
    void setPosition(int x, int y);
    void setSize(int width, int height);
    void setFont(const std::string& fontName, int fontSize);
    void setColors(const std::string& background, const std::string& foreground, const std::string& selection);
    
    // Callbacks
    void setOnCandidateSelected(std::function<void(int)> callback);
    void setOnCandidateHighlighted(std::function<void(int)> callback);
    void setOnWindowClosed(std::function<void()> callback);
    
    // Widget access
    GtkWidget* getWidget() const;
    GtkWindow* getWindow() const;

private:
    struct Impl;
    std::unique_ptr<Impl> pImpl;
};

/**
 * @brief Settings configuration dialog
 * 
 * Provides interface for configuring OwCat IME settings.
 */
class GtkSettingsDialog {
public:
    GtkSettingsDialog();
    ~GtkSettingsDialog();

    bool create(GtkWindow* parent);
    void destroy();
    
    bool show();
    bool hide();
    bool isVisible() const;
    
    void setConfiguration(const std::map<std::string, std::string>& config);
    std::map<std::string, std::string> getConfiguration() const;
    
    // Callbacks
    void setOnConfigurationChanged(std::function<void(const std::map<std::string, std::string>&)> callback);
    void setOnDialogClosed(std::function<void()> callback);
    
    // Widget access
    GtkWidget* getWidget() const;
    GtkDialog* getDialog() const;

private:
    struct Impl;
    std::unique_ptr<Impl> pImpl;
};

/**
 * @brief About dialog
 * 
 * Displays application information and credits.
 */
class GtkAboutDialog {
public:
    GtkAboutDialog();
    ~GtkAboutDialog();

    bool create(GtkWindow* parent);
    void destroy();
    
    bool show();
    
    void setApplicationInfo(const std::string& name, const std::string& version, 
                           const std::string& description, const std::string& website);
    void setAuthors(const std::vector<std::string>& authors);
    void setLicense(const std::string& license);
    void setCopyright(const std::string& copyright);
    void setLogo(const std::string& logoPath);
    
    // Widget access
    GtkWidget* getWidget() const;
    GtkAboutDialog* getAboutDialog() const;

private:
    struct Impl;
    std::unique_ptr<Impl> pImpl;
};

/**
 * @brief System tray icon
 * 
 * Provides system tray integration for quick access.
 */
class GtkStatusIcon {
public:
    GtkStatusIcon();
    ~GtkStatusIcon();

    bool create();
    void destroy();
    
    bool show();
    bool hide();
    bool isVisible() const;
    
    void setIcon(const std::string& iconName);
    void setTooltip(const std::string& tooltip);
    void setMenu(GtkWidget* menu);
    
    // Callbacks
    void setOnActivated(std::function<void()> callback);
    void setOnPopupMenu(std::function<void(int, int)> callback);
    
private:
    struct Impl;
    std::unique_ptr<Impl> pImpl;
};

/**
 * @brief Theme and styling manager
 * 
 * Manages GTK themes and custom CSS styling.
 */
class GtkThemeManager {
public:
    GtkThemeManager();
    ~GtkThemeManager();

    bool initialize();
    void shutdown();
    
    bool loadTheme(const std::string& themeName);
    bool setCustomCSS(const std::string& css);
    bool loadCSSFromFile(const std::string& filePath);
    
    std::vector<std::string> getAvailableThemes() const;
    std::string getCurrentTheme() const;
    
    // Color scheme
    void setColorScheme(const std::map<std::string, std::string>& colors);
    std::map<std::string, std::string> getColorScheme() const;
    
    // Font settings
    void setDefaultFont(const std::string& fontName, int fontSize);
    std::pair<std::string, int> getDefaultFont() const;
    
    // Dark mode
    void setDarkMode(bool enabled);
    bool isDarkMode() const;
    
private:
    struct Impl;
    std::unique_ptr<Impl> pImpl;
};

// Utility functions
namespace gtk_utils {
    // GTK initialization helpers
    bool initializeGTK(int argc, char* argv[]);
    void shutdownGTK();
    
    // Widget utilities
    GtkWidget* createButton(const std::string& label, std::function<void()> callback = nullptr);
    GtkWidget* createLabel(const std::string& text);
    GtkWidget* createEntry(const std::string& placeholder = "");
    GtkWidget* createCheckButton(const std::string& label, bool checked = false);
    GtkWidget* createComboBox(const std::vector<std::string>& items);
    
    // Layout utilities
    GtkWidget* createVBox(int spacing = 5);
    GtkWidget* createHBox(int spacing = 5);
    GtkWidget* createGrid(int rowSpacing = 5, int columnSpacing = 5);
    
    // Dialog utilities
    void showMessageDialog(GtkWindow* parent, const std::string& title, const std::string& message);
    bool showConfirmDialog(GtkWindow* parent, const std::string& title, const std::string& message);
    std::string showFileChooserDialog(GtkWindow* parent, const std::string& title, bool save = false);
    
    // Icon utilities
    GdkPixbuf* loadIcon(const std::string& iconName, int size = 24);
    GdkPixbuf* loadIconFromFile(const std::string& filePath, int size = 24);
    
    // CSS utilities
    bool applyCSSToWidget(GtkWidget* widget, const std::string& css);
    bool loadCSSFromFile(const std::string& filePath);
    
    // Screen utilities
    void getScreenSize(int& width, int& height);
    void getCursorPosition(int& x, int& y);
    
    // String utilities
    std::string escapeMarkup(const std::string& text);
    std::string formatMarkup(const std::string& text, const std::string& format);
}

} // namespace owcat