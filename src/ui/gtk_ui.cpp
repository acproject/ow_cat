#include "ui/gtk_ui.h"
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <functional>

#ifdef OWCAT_USE_GTK
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <glib.h>
#include <gio/gio.h>
#endif

namespace owcat {

// Implementation structure for GtkUI
struct GtkUI::Impl {
#ifdef OWCAT_USE_GTK
    GtkApplication* application;
    GMainLoop* mainLoop;
#endif
    
    // Components
    std::unique_ptr<GtkMainWindow> mainWindow;
    std::unique_ptr<GtkCandidateWindow> candidateWindow;
    std::unique_ptr<GtkSettingsDialog> settingsDialog;
    std::unique_ptr<GtkAboutDialog> aboutDialog;
    std::unique_ptr<GtkStatusIcon> statusIcon;
    std::unique_ptr<GtkThemeManager> themeManager;
    
    // State
    bool isInitialized;
    bool isRunning;
    std::map<std::string, std::string> configuration;
    std::string currentLanguage;
    bool accessibilityEnabled;
    
    // Platform integration
    std::shared_ptr<PlatformManager> platformManager;
    
    // Callbacks
    std::function<void(int)> onCandidateSelected;
    std::function<void(int)> onCandidateHighlighted;
    std::function<void(const std::map<std::string, std::string>&)> onSettingsChanged;
    std::function<void()> onMainWindowClosed;
    std::function<void()> onSystemTrayActivated;
    std::function<void()> onQuitRequested;
    
    Impl() {
#ifdef OWCAT_USE_GTK
        application = nullptr;
        mainLoop = nullptr;
#endif
        isInitialized = false;
        isRunning = false;
        currentLanguage = "en";
        accessibilityEnabled = false;
    }
    
    ~Impl() {
#ifdef OWCAT_USE_GTK
        if (application) {
            g_object_unref(application);
        }
        if (mainLoop) {
            g_main_loop_unref(mainLoop);
        }
#endif
    }
};

GtkUI::GtkUI() : pImpl(std::make_unique<Impl>()) {}

GtkUI::~GtkUI() = default;

bool GtkUI::initialize(int argc, char* argv[]) {
#ifdef OWCAT_USE_GTK
    if (pImpl->isInitialized) {
        return true;
    }
    
    // Initialize GTK
    if (!gtk_utils::initializeGTK(argc, argv)) {
        std::cerr << "Failed to initialize GTK" << std::endl;
        return false;
    }
    
    // Create GTK application
    pImpl->application = gtk_application_new("org.owcat.ime", G_APPLICATION_FLAGS_NONE);
    if (!pImpl->application) {
        std::cerr << "Failed to create GTK application" << std::endl;
        return false;
    }
    
    // Create main loop
    pImpl->mainLoop = g_main_loop_new(nullptr, FALSE);
    if (!pImpl->mainLoop) {
        std::cerr << "Failed to create main loop" << std::endl;
        return false;
    }
    
    // Initialize components
    pImpl->mainWindow = std::make_unique<GtkMainWindow>();
    pImpl->candidateWindow = std::make_unique<GtkCandidateWindow>();
    pImpl->settingsDialog = std::make_unique<GtkSettingsDialog>();
    pImpl->aboutDialog = std::make_unique<GtkAboutDialog>();
    pImpl->statusIcon = std::make_unique<GtkStatusIcon>();
    pImpl->themeManager = std::make_unique<GtkThemeManager>();
    
    // Initialize theme manager
    if (!pImpl->themeManager->initialize()) {
        std::cerr << "Failed to initialize theme manager" << std::endl;
    }
    
    // Set up application callbacks
    g_signal_connect(pImpl->application, "activate", G_CALLBACK(+[](GtkApplication* app, gpointer userData) {
        GtkUI* ui = static_cast<GtkUI*>(userData);
        ui->pImpl->mainWindow->create(app);
    }), this);
    
    // Set up component callbacks
    setupCallbacks();
    
    pImpl->isInitialized = true;
    std::cout << "GTK UI initialized successfully" << std::endl;
    return true;
#else
    std::cerr << "GTK UI not supported on this platform" << std::endl;
    return false;
#endif
}

void GtkUI::shutdown() {
#ifdef OWCAT_USE_GTK
    if (!pImpl->isInitialized) {
        return;
    }
    
    // Stop main loop if running
    if (pImpl->isRunning && pImpl->mainLoop) {
        g_main_loop_quit(pImpl->mainLoop);
        pImpl->isRunning = false;
    }
    
    // Destroy components
    if (pImpl->statusIcon) {
        pImpl->statusIcon->destroy();
    }
    if (pImpl->aboutDialog) {
        pImpl->aboutDialog->destroy();
    }
    if (pImpl->settingsDialog) {
        pImpl->settingsDialog->destroy();
    }
    if (pImpl->candidateWindow) {
        pImpl->candidateWindow->destroy();
    }
    if (pImpl->mainWindow) {
        pImpl->mainWindow->destroy();
    }
    
    // Shutdown theme manager
    if (pImpl->themeManager) {
        pImpl->themeManager->shutdown();
    }
    
    // Shutdown GTK
    gtk_utils::shutdownGTK();
    
    pImpl->isInitialized = false;
    std::cout << "GTK UI shut down" << std::endl;
#endif
}

void GtkUI::run() {
#ifdef OWCAT_USE_GTK
    if (!pImpl->isInitialized || pImpl->isRunning) {
        return;
    }
    
    pImpl->isRunning = true;
    
    // Run GTK application
    g_application_run(G_APPLICATION(pImpl->application), 0, nullptr);
    
    pImpl->isRunning = false;
#endif
}

void GtkUI::quit() {
#ifdef OWCAT_USE_GTK
    if (pImpl->isRunning && pImpl->application) {
        g_application_quit(G_APPLICATION(pImpl->application));
    }
    
    if (pImpl->onQuitRequested) {
        pImpl->onQuitRequested();
    }
#endif
}

// Window management
bool GtkUI::showMainWindow() {
    if (pImpl->mainWindow) {
        return pImpl->mainWindow->show();
    }
    return false;
}

bool GtkUI::hideMainWindow() {
    if (pImpl->mainWindow) {
        return pImpl->mainWindow->hide();
    }
    return false;
}

bool GtkUI::isMainWindowVisible() const {
    if (pImpl->mainWindow) {
        return pImpl->mainWindow->isVisible();
    }
    return false;
}

bool GtkUI::showCandidateWindow(const std::vector<std::string>& candidates, int x, int y) {
    if (pImpl->candidateWindow) {
        return pImpl->candidateWindow->show(candidates, x, y);
    }
    return false;
}

bool GtkUI::hideCandidateWindow() {
    if (pImpl->candidateWindow) {
        return pImpl->candidateWindow->hide();
    }
    return false;
}

bool GtkUI::updateCandidateWindow(const std::vector<std::string>& candidates) {
    if (pImpl->candidateWindow) {
        return pImpl->candidateWindow->updateCandidates(candidates);
    }
    return false;
}

bool GtkUI::setCandidateSelection(int index) {
    if (pImpl->candidateWindow) {
        return pImpl->candidateWindow->setSelection(index);
    }
    return false;
}

bool GtkUI::showSettingsDialog() {
    if (pImpl->settingsDialog) {
        return pImpl->settingsDialog->show();
    }
    return false;
}

bool GtkUI::hideSettingsDialog() {
    if (pImpl->settingsDialog) {
        return pImpl->settingsDialog->hide();
    }
    return false;
}

bool GtkUI::showAboutDialog() {
    if (pImpl->aboutDialog) {
        return pImpl->aboutDialog->show();
    }
    return false;
}

// System tray
bool GtkUI::createSystemTrayIcon() {
    if (pImpl->statusIcon) {
        return pImpl->statusIcon->create();
    }
    return false;
}

bool GtkUI::removeSystemTrayIcon() {
    if (pImpl->statusIcon) {
        pImpl->statusIcon->destroy();
        return true;
    }
    return false;
}

bool GtkUI::updateSystemTrayIcon(const std::string& iconName, const std::string& tooltip) {
    if (pImpl->statusIcon) {
        pImpl->statusIcon->setIcon(iconName);
        pImpl->statusIcon->setTooltip(tooltip);
        return true;
    }
    return false;
}

// Theme and styling
bool GtkUI::loadTheme(const std::string& themeName) {
    if (pImpl->themeManager) {
        return pImpl->themeManager->loadTheme(themeName);
    }
    return false;
}

bool GtkUI::setCustomCSS(const std::string& css) {
    if (pImpl->themeManager) {
        return pImpl->themeManager->setCustomCSS(css);
    }
    return false;
}

std::vector<std::string> GtkUI::getAvailableThemes() const {
    if (pImpl->themeManager) {
        return pImpl->themeManager->getAvailableThemes();
    }
    return {};
}

// Callbacks
void GtkUI::setOnCandidateSelected(std::function<void(int)> callback) {
    pImpl->onCandidateSelected = callback;
}

void GtkUI::setOnCandidateHighlighted(std::function<void(int)> callback) {
    pImpl->onCandidateHighlighted = callback;
}

void GtkUI::setOnSettingsChanged(std::function<void(const std::map<std::string, std::string>&)> callback) {
    pImpl->onSettingsChanged = callback;
}

void GtkUI::setOnMainWindowClosed(std::function<void()> callback) {
    pImpl->onMainWindowClosed = callback;
}

void GtkUI::setOnSystemTrayActivated(std::function<void()> callback) {
    pImpl->onSystemTrayActivated = callback;
}

void GtkUI::setOnQuitRequested(std::function<void()> callback) {
    pImpl->onQuitRequested = callback;
}

// Configuration
void GtkUI::setConfiguration(const std::map<std::string, std::string>& config) {
    pImpl->configuration = config;
    
    // Apply configuration to components
    if (pImpl->settingsDialog) {
        pImpl->settingsDialog->setConfiguration(config);
    }
    
    // Apply theme if specified
    auto themeIt = config.find("theme");
    if (themeIt != config.end()) {
        loadTheme(themeIt->second);
    }
    
    // Apply language if specified
    auto langIt = config.find("language");
    if (langIt != config.end()) {
        setLanguage(langIt->second);
    }
    
    // Apply accessibility settings
    auto a11yIt = config.find("accessibility");
    if (a11yIt != config.end()) {
        setAccessibilityEnabled(a11yIt->second == "true");
    }
}

std::map<std::string, std::string> GtkUI::getConfiguration() const {
    auto config = pImpl->configuration;
    
    // Get current settings from components
    if (pImpl->settingsDialog) {
        auto dialogConfig = pImpl->settingsDialog->getConfiguration();
        config.insert(dialogConfig.begin(), dialogConfig.end());
    }
    
    // Add current UI state
    if (pImpl->themeManager) {
        config["theme"] = pImpl->themeManager->getCurrentTheme();
        config["dark_mode"] = pImpl->themeManager->isDarkMode() ? "true" : "false";
    }
    
    config["language"] = pImpl->currentLanguage;
    config["accessibility"] = pImpl->accessibilityEnabled ? "true" : "false";
    
    return config;
}

// Utility
void GtkUI::showNotification(const std::string& title, const std::string& message) {
#ifdef OWCAT_USE_GTK
    // Create notification using GNotification
    GNotification* notification = g_notification_new(title.c_str());
    g_notification_set_body(notification, message.c_str());
    g_notification_set_icon(notification, g_themed_icon_new("owcat"));
    
    if (pImpl->application) {
        g_application_send_notification(G_APPLICATION(pImpl->application), nullptr, notification);
    }
    
    g_object_unref(notification);
#endif
}

void GtkUI::showErrorDialog(const std::string& title, const std::string& message) {
#ifdef OWCAT_USE_GTK
    GtkWindow* parent = pImpl->mainWindow ? pImpl->mainWindow->getWindow() : nullptr;
    gtk_utils::showMessageDialog(parent, title, message);
#endif
}

bool GtkUI::showConfirmDialog(const std::string& title, const std::string& message) {
#ifdef OWCAT_USE_GTK
    GtkWindow* parent = pImpl->mainWindow ? pImpl->mainWindow->getWindow() : nullptr;
    return gtk_utils::showConfirmDialog(parent, title, message);
#else
    return false;
#endif
}

std::string GtkUI::showFileDialog(const std::string& title, bool save, const std::string& filter) {
#ifdef OWCAT_USE_GTK
    GtkWindow* parent = pImpl->mainWindow ? pImpl->mainWindow->getWindow() : nullptr;
    return gtk_utils::showFileChooserDialog(parent, title, save);
#else
    return "";
#endif
}

// Accessibility
void GtkUI::setAccessibilityEnabled(bool enabled) {
    pImpl->accessibilityEnabled = enabled;
    
#ifdef OWCAT_USE_GTK
    // Enable/disable accessibility features
    GtkSettings* settings = gtk_settings_get_default();
    if (settings) {
        g_object_set(settings, "gtk-enable-accels", enabled, nullptr);
        g_object_set(settings, "gtk-enable-mnemonics", enabled, nullptr);
    }
#endif
}

bool GtkUI::isAccessibilityEnabled() const {
    return pImpl->accessibilityEnabled;
}

// Internationalization
void GtkUI::setLanguage(const std::string& language) {
    pImpl->currentLanguage = language;
    
    // Set locale for GTK
#ifdef OWCAT_USE_GTK
    setlocale(LC_ALL, language.c_str());
    
    // Update text domain
    bindtextdomain("owcat", "/usr/share/locale");
    textdomain("owcat");
#endif
}

std::string GtkUI::getCurrentLanguage() const {
    return pImpl->currentLanguage;
}

std::vector<std::string> GtkUI::getAvailableLanguages() const {
    // Return list of supported languages
    return {"en", "zh_CN", "zh_TW", "ja", "ko", "fr", "de", "es", "ru"};
}

// Platform integration
void GtkUI::setPlatformManager(std::shared_ptr<PlatformManager> manager) {
    pImpl->platformManager = manager;
}

std::shared_ptr<PlatformManager> GtkUI::getPlatformManager() const {
    return pImpl->platformManager;
}

// Private helper methods
void GtkUI::setupCallbacks() {
    // Set up main window callbacks
    if (pImpl->mainWindow) {
        pImpl->mainWindow->setOnClosed([this]() {
            if (pImpl->onMainWindowClosed) {
                pImpl->onMainWindowClosed();
            }
        });
        
        pImpl->mainWindow->setOnSettingsRequested([this]() {
            showSettingsDialog();
        });
        
        pImpl->mainWindow->setOnAboutRequested([this]() {
            showAboutDialog();
        });
    }
    
    // Set up candidate window callbacks
    if (pImpl->candidateWindow) {
        pImpl->candidateWindow->setOnCandidateSelected([this](int index) {
            if (pImpl->onCandidateSelected) {
                pImpl->onCandidateSelected(index);
            }
        });
        
        pImpl->candidateWindow->setOnCandidateHighlighted([this](int index) {
            if (pImpl->onCandidateHighlighted) {
                pImpl->onCandidateHighlighted(index);
            }
        });
    }
    
    // Set up settings dialog callbacks
    if (pImpl->settingsDialog) {
        pImpl->settingsDialog->setOnConfigurationChanged([this](const std::map<std::string, std::string>& config) {
            pImpl->configuration = config;
            if (pImpl->onSettingsChanged) {
                pImpl->onSettingsChanged(config);
            }
        });
    }
    
    // Set up status icon callbacks
    if (pImpl->statusIcon) {
        pImpl->statusIcon->setOnActivated([this]() {
            if (pImpl->onSystemTrayActivated) {
                pImpl->onSystemTrayActivated();
            }
        });
    }
}

// GTK utility functions implementation
namespace gtk_utils {

bool initializeGTK(int argc, char* argv[]) {
#ifdef OWCAT_USE_GTK
    // GTK4 doesn't need explicit initialization
    // gtk_init is called automatically when creating GtkApplication
    return true;
#else
    return false;
#endif
}

void shutdownGTK() {
#ifdef OWCAT_USE_GTK
    // GTK doesn't have an explicit shutdown function
    // Resources are cleaned up automatically
#endif
}

GtkWidget* createButton(const std::string& label, std::function<void()> callback) {
#ifdef OWCAT_USE_GTK
    GtkWidget* button = gtk_button_new_with_label(label.c_str());
    
    if (callback) {
        g_signal_connect(button, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer userData) {
            auto* cb = static_cast<std::function<void()>*>(userData);
            (*cb)();
        }), new std::function<void()>(callback));
        
        // Clean up callback when button is destroyed
        g_signal_connect(button, "destroy", G_CALLBACK(+[](GtkWidget* widget, gpointer userData) {
            delete static_cast<std::function<void()>*>(userData);
        }), nullptr);
    }
    
    return button;
#else
    return nullptr;
#endif
}

GtkWidget* createLabel(const std::string& text) {
#ifdef OWCAT_USE_GTK
    return gtk_label_new(text.c_str());
#else
    return nullptr;
#endif
}

GtkWidget* createEntry(const std::string& placeholder) {
#ifdef OWCAT_USE_GTK
    GtkWidget* entry = gtk_entry_new();
    if (!placeholder.empty()) {
        gtk_entry_set_placeholder_text(GTK_ENTRY(entry), placeholder.c_str());
    }
    return entry;
#else
    return nullptr;
#endif
}

GtkWidget* createCheckButton(const std::string& label, bool checked) {
#ifdef OWCAT_USE_GTK
    GtkWidget* checkButton = gtk_check_button_new_with_label(label.c_str());
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkButton), checked);
    return checkButton;
#else
    return nullptr;
#endif
}

GtkWidget* createComboBox(const std::vector<std::string>& items) {
#ifdef OWCAT_USE_GTK
    GtkWidget* comboBox = gtk_combo_box_text_new();
    
    for (const auto& item : items) {
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(comboBox), item.c_str());
    }
    
    if (!items.empty()) {
        gtk_combo_box_set_active(GTK_COMBO_BOX(comboBox), 0);
    }
    
    return comboBox;
#else
    return nullptr;
#endif
}

GtkWidget* createVBox(int spacing) {
#ifdef OWCAT_USE_GTK
    return gtk_box_new(GTK_ORIENTATION_VERTICAL, spacing);
#else
    return nullptr;
#endif
}

GtkWidget* createHBox(int spacing) {
#ifdef OWCAT_USE_GTK
    return gtk_box_new(GTK_ORIENTATION_HORIZONTAL, spacing);
#else
    return nullptr;
#endif
}

GtkWidget* createGrid(int rowSpacing, int columnSpacing) {
#ifdef OWCAT_USE_GTK
    GtkWidget* grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), rowSpacing);
    gtk_grid_set_column_spacing(GTK_GRID(grid), columnSpacing);
    return grid;
#else
    return nullptr;
#endif
}

void showMessageDialog(GtkWindow* parent, const std::string& title, const std::string& message) {
#ifdef OWCAT_USE_GTK
    GtkWidget* dialog = gtk_message_dialog_new(parent,
                                               GTK_DIALOG_MODAL,
                                               GTK_MESSAGE_INFO,
                                               GTK_BUTTONS_OK,
                                               "%s", message.c_str());
    
    gtk_window_set_title(GTK_WINDOW(dialog), title.c_str());
    
    // GTK4: Use gtk_window_present instead of gtk_dialog_run
    g_signal_connect(dialog, "response", G_CALLBACK(gtk_window_destroy), nullptr);
    gtk_window_present(GTK_WINDOW(dialog));
#endif
}

bool showConfirmDialog(GtkWindow* parent, const std::string& title, const std::string& message) {
#ifdef OWCAT_USE_GTK
    // Note: GTK4 requires async dialogs, this is a simplified synchronous version
    // For full GTK4 compatibility, this should be refactored to use callbacks
    GtkWidget* dialog = gtk_message_dialog_new(parent,
                                               GTK_DIALOG_MODAL,
                                               GTK_MESSAGE_QUESTION,
                                               GTK_BUTTONS_YES_NO,
                                               "%s", message.c_str());
    
    gtk_window_set_title(GTK_WINDOW(dialog), title.c_str());
    
    // GTK4: This is a temporary workaround - should be refactored to async
    bool result = false;
    g_signal_connect(dialog, "response", G_CALLBACK(+[](GtkDialog* dlg, int response, gpointer data) {
        bool* result_ptr = static_cast<bool*>(data);
        *result_ptr = (response == GTK_RESPONSE_YES);
        gtk_window_destroy(GTK_WINDOW(dlg));
    }), &result);
    
    gtk_window_present(GTK_WINDOW(dialog));
    return result;
#else
    return false;
#endif
}

std::string showFileChooserDialog(GtkWindow* parent, const std::string& title, bool save) {
#ifdef OWCAT_USE_GTK
    // Note: GTK4 requires async file dialogs, this is a simplified version
    // For full GTK4 compatibility, this should be refactored to use callbacks
    GtkFileChooserAction action = save ? GTK_FILE_CHOOSER_ACTION_SAVE : GTK_FILE_CHOOSER_ACTION_OPEN;
    const char* buttonText = save ? "_Save" : "_Open";
    
    GtkWidget* dialog = gtk_file_chooser_dialog_new(title.c_str(),
                                                    parent,
                                                    action,
                                                    "_Cancel", GTK_RESPONSE_CANCEL,
                                                    buttonText, GTK_RESPONSE_ACCEPT,
                                                    nullptr);
    
    std::string filename;
    // GTK4: This is a temporary workaround - should be refactored to async
    g_signal_connect(dialog, "response", G_CALLBACK(+[](GtkDialog* dlg, int response, gpointer data) {
        std::string* filename_ptr = static_cast<std::string*>(data);
        if (response == GTK_RESPONSE_ACCEPT) {
            GFile* file = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(dlg));
            if (file) {
                char* path = g_file_get_path(file);
                if (path) {
                    *filename_ptr = path;
                    g_free(path);
                }
                g_object_unref(file);
            }
        }
        gtk_window_destroy(GTK_WINDOW(dlg));
    }), &filename);
    
    gtk_window_present(GTK_WINDOW(dialog));
    return filename;
#else
    return "";
#endif
}

void getScreenSize(int& width, int& height) {
#ifdef OWCAT_USE_GTK
    GdkDisplay* display = gdk_display_get_default();
    if (display) {
        GdkMonitor* monitor = gdk_display_get_primary_monitor(display);
        if (monitor) {
            GdkRectangle geometry;
            gdk_monitor_get_geometry(monitor, &geometry);
            width = geometry.width;
            height = geometry.height;
            return;
        }
    }
#endif
    width = 1920;
    height = 1080;
}

void getCursorPosition(int& x, int& y) {
#ifdef OWCAT_USE_GTK
    GdkDisplay* display = gdk_display_get_default();
    if (display) {
        GdkSeat* seat = gdk_display_get_default_seat(display);
        if (seat) {
            GdkDevice* pointer = gdk_seat_get_pointer(seat);
            if (pointer) {
                // GTK4: gdk_device_get_position is deprecated
                // Use gdk_device_get_surface_at_position instead
                double dx, dy;
                GdkSurface* surface = gdk_device_get_surface_at_position(pointer, &dx, &dy);
                if (surface) {
                    x = (int)dx;
                    y = (int)dy;
                    return;
                }
            }
        }
    }
#endif
    x = 0;
    y = 0;
}

std::string escapeMarkup(const std::string& text) {
#ifdef OWCAT_USE_GTK
    gchar* escaped = g_markup_escape_text(text.c_str(), -1);
    std::string result(escaped);
    g_free(escaped);
    return result;
#else
    return text;
#endif
}

std::string formatMarkup(const std::string& text, const std::string& format) {
    return "<" + format + ">" + escapeMarkup(text) + "</" + format + ">";
}

} // namespace gtk_utils

} // namespace owcat