#include "ui/gtk_ui.h"
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>

#ifdef OWCAT_USE_GTK
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <glib.h>
#endif

namespace owcat {

// Implementation structure for GtkThemeManager
struct GtkThemeManager::Impl {
#ifdef OWCAT_USE_GTK
    GtkCssProvider* cssProvider;
    GdkScreen* screen;
#endif
    
    // State
    std::string currentTheme;
    std::map<std::string, std::string> themes;
    std::string customCss;
    bool isDarkMode;
    
    // Callbacks
    std::function<void(const std::string&)> onThemeChanged;
    std::function<void(bool)> onDarkModeChanged;
    
    Impl() {
#ifdef OWCAT_USE_GTK
        cssProvider = nullptr;
        screen = nullptr;
#endif
        
        currentTheme = "default";
        isDarkMode = false;
        
        // Initialize built-in themes
        initializeThemes();
    }
    
    void initializeThemes() {
        // Default theme
        themes["default"] = R"(
/* Default OwCat Theme */
.owcat-window {
    background-color: #ffffff;
    color: #000000;
    border: 1px solid #cccccc;
}

.owcat-candidate-window {
    background-color: #ffffff;
    color: #000000;
    border: 1px solid #cccccc;
    border-radius: 4px;
    box-shadow: 0 2px 8px rgba(0, 0, 0, 0.15);
}

.owcat-candidate {
    padding: 4px 8px;
    margin: 1px;
}

.owcat-candidate:hover {
    background-color: #f0f0f0;
}

.owcat-candidate.selected {
    background-color: #3584e4;
    color: #ffffff;
}

.owcat-candidate-number {
    color: #666666;
    font-weight: bold;
    margin-right: 4px;
}

.owcat-candidate.selected .owcat-candidate-number {
    color: #ffffff;
}

.owcat-preedit {
    background-color: #ffffcc;
    color: #000000;
    border-bottom: 2px solid #3584e4;
}

.owcat-button {
    background: linear-gradient(to bottom, #ffffff, #f0f0f0);
    border: 1px solid #cccccc;
    border-radius: 3px;
    padding: 6px 12px;
    color: #000000;
}

.owcat-button:hover {
    background: linear-gradient(to bottom, #f8f8f8, #e8e8e8);
    border-color: #999999;
}

.owcat-button:active {
    background: linear-gradient(to bottom, #e8e8e8, #d8d8d8);
    border-color: #666666;
}

.owcat-entry {
    background-color: #ffffff;
    color: #000000;
    border: 1px solid #cccccc;
    border-radius: 3px;
    padding: 4px 8px;
}

.owcat-entry:focus {
    border-color: #3584e4;
    box-shadow: 0 0 0 2px rgba(53, 132, 228, 0.3);
}
)";
        
        // Dark theme
        themes["dark"] = R"(
/* Dark OwCat Theme */
.owcat-window {
    background-color: #2d2d2d;
    color: #ffffff;
    border: 1px solid #555555;
}

.owcat-candidate-window {
    background-color: #2d2d2d;
    color: #ffffff;
    border: 1px solid #555555;
    border-radius: 4px;
    box-shadow: 0 2px 8px rgba(0, 0, 0, 0.5);
}

.owcat-candidate {
    padding: 4px 8px;
    margin: 1px;
}

.owcat-candidate:hover {
    background-color: #404040;
}

.owcat-candidate.selected {
    background-color: #3584e4;
    color: #ffffff;
}

.owcat-candidate-number {
    color: #aaaaaa;
    font-weight: bold;
    margin-right: 4px;
}

.owcat-candidate.selected .owcat-candidate-number {
    color: #ffffff;
}

.owcat-preedit {
    background-color: #404040;
    color: #ffffff;
    border-bottom: 2px solid #3584e4;
}

.owcat-button {
    background: linear-gradient(to bottom, #404040, #353535);
    border: 1px solid #555555;
    border-radius: 3px;
    padding: 6px 12px;
    color: #ffffff;
}

.owcat-button:hover {
    background: linear-gradient(to bottom, #454545, #3a3a3a);
    border-color: #777777;
}

.owcat-button:active {
    background: linear-gradient(to bottom, #353535, #2a2a2a);
    border-color: #999999;
}

.owcat-entry {
    background-color: #404040;
    color: #ffffff;
    border: 1px solid #555555;
    border-radius: 3px;
    padding: 4px 8px;
}

.owcat-entry:focus {
    border-color: #3584e4;
    box-shadow: 0 0 0 2px rgba(53, 132, 228, 0.3);
}
)";
        
        // Light theme
        themes["light"] = R"(
/* Light OwCat Theme */
.owcat-window {
    background-color: #fafafa;
    color: #2e2e2e;
    border: 1px solid #e0e0e0;
}

.owcat-candidate-window {
    background-color: #fafafa;
    color: #2e2e2e;
    border: 1px solid #e0e0e0;
    border-radius: 6px;
    box-shadow: 0 4px 12px rgba(0, 0, 0, 0.1);
}

.owcat-candidate {
    padding: 6px 10px;
    margin: 1px;
    border-radius: 3px;
}

.owcat-candidate:hover {
    background-color: #f0f0f0;
}

.owcat-candidate.selected {
    background-color: #4285f4;
    color: #ffffff;
}

.owcat-candidate-number {
    color: #757575;
    font-weight: bold;
    margin-right: 6px;
}

.owcat-candidate.selected .owcat-candidate-number {
    color: #ffffff;
}

.owcat-preedit {
    background-color: #fff3cd;
    color: #856404;
    border-bottom: 2px solid #4285f4;
}

.owcat-button {
    background: linear-gradient(to bottom, #ffffff, #f8f9fa);
    border: 1px solid #dadce0;
    border-radius: 4px;
    padding: 8px 16px;
    color: #3c4043;
}

.owcat-button:hover {
    background: linear-gradient(to bottom, #f8f9fa, #f1f3f4);
    border-color: #c4c7c5;
}

.owcat-button:active {
    background: linear-gradient(to bottom, #f1f3f4, #e8eaed);
    border-color: #9aa0a6;
}

.owcat-entry {
    background-color: #ffffff;
    color: #3c4043;
    border: 1px solid #dadce0;
    border-radius: 4px;
    padding: 6px 10px;
}

.owcat-entry:focus {
    border-color: #4285f4;
    box-shadow: 0 0 0 2px rgba(66, 133, 244, 0.2);
}
)";
        
        // High contrast theme
        themes["high-contrast"] = R"(
/* High Contrast OwCat Theme */
.owcat-window {
    background-color: #000000;
    color: #ffffff;
    border: 2px solid #ffffff;
}

.owcat-candidate-window {
    background-color: #000000;
    color: #ffffff;
    border: 2px solid #ffffff;
    border-radius: 0;
}

.owcat-candidate {
    padding: 6px 12px;
    margin: 2px;
    border: 1px solid #ffffff;
}

.owcat-candidate:hover {
    background-color: #333333;
}

.owcat-candidate.selected {
    background-color: #ffffff;
    color: #000000;
    border: 2px solid #000000;
}

.owcat-candidate-number {
    color: #ffff00;
    font-weight: bold;
    margin-right: 8px;
}

.owcat-candidate.selected .owcat-candidate-number {
    color: #000000;
}

.owcat-preedit {
    background-color: #ffff00;
    color: #000000;
    border-bottom: 3px solid #ffffff;
}

.owcat-button {
    background-color: #000000;
    border: 2px solid #ffffff;
    border-radius: 0;
    padding: 8px 16px;
    color: #ffffff;
    font-weight: bold;
}

.owcat-button:hover {
    background-color: #ffffff;
    color: #000000;
}

.owcat-button:active {
    background-color: #ffff00;
    color: #000000;
}

.owcat-entry {
    background-color: #000000;
    color: #ffffff;
    border: 2px solid #ffffff;
    border-radius: 0;
    padding: 6px 12px;
    font-weight: bold;
}

.owcat-entry:focus {
    border-color: #ffff00;
    background-color: #333333;
}
)";
    }
};

GtkThemeManager::GtkThemeManager() : pImpl(std::make_unique<Impl>()) {}

GtkThemeManager::~GtkThemeManager() = default;

bool GtkThemeManager::initialize() {
#ifdef OWCAT_USE_GTK
    // Create CSS provider
    pImpl->cssProvider = gtk_css_provider_new();
    if (!pImpl->cssProvider) {
        std::cerr << "Failed to create CSS provider" << std::endl;
        return false;
    }
    
    // Get default screen
    pImpl->screen = gdk_screen_get_default();
    if (!pImpl->screen) {
        std::cerr << "Failed to get default screen" << std::endl;
        return false;
    }
    
    // Add CSS provider to screen
    gtk_style_context_add_provider_for_screen(pImpl->screen,
                                              GTK_STYLE_PROVIDER(pImpl->cssProvider),
                                              GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    
    // Load default theme
    loadTheme(pImpl->currentTheme);
    
    std::cout << "Theme manager initialized successfully" << std::endl;
    return true;
#else
    std::cerr << "GTK support not available" << std::endl;
    return false;
#endif
}

void GtkThemeManager::shutdown() {
#ifdef OWCAT_USE_GTK
    if (pImpl->cssProvider && pImpl->screen) {
        gtk_style_context_remove_provider_for_screen(pImpl->screen,
                                                     GTK_STYLE_PROVIDER(pImpl->cssProvider));
    }
    
    if (pImpl->cssProvider) {
        g_object_unref(pImpl->cssProvider);
        pImpl->cssProvider = nullptr;
    }
    
    pImpl->screen = nullptr;
#endif
}

// Theme management
bool GtkThemeManager::loadTheme(const std::string& themeName) {
    auto it = pImpl->themes.find(themeName);
    if (it == pImpl->themes.end()) {
        std::cerr << "Theme not found: " << themeName << std::endl;
        return false;
    }
    
    return loadCss(it->second);
}

bool GtkThemeManager::loadThemeFromFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Failed to open theme file: " << filePath << std::endl;
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string css = buffer.str();
    
    return loadCss(css);
}

bool GtkThemeManager::loadCss(const std::string& css) {
#ifdef OWCAT_USE_GTK
    if (!pImpl->cssProvider) {
        std::cerr << "CSS provider not initialized" << std::endl;
        return false;
    }
    
    GError* error = nullptr;
    gboolean success = gtk_css_provider_load_from_data(pImpl->cssProvider,
                                                      css.c_str(),
                                                      css.length(),
                                                      &error);
    
    if (!success) {
        std::cerr << "Failed to load CSS: ";
        if (error) {
            std::cerr << error->message << std::endl;
            g_error_free(error);
        } else {
            std::cerr << "Unknown error" << std::endl;
        }
        return false;
    }
    
    std::cout << "CSS loaded successfully" << std::endl;
    return true;
#else
    return false;
#endif
}

void GtkThemeManager::setCurrentTheme(const std::string& themeName) {
    if (pImpl->currentTheme != themeName) {
        std::string oldTheme = pImpl->currentTheme;
        pImpl->currentTheme = themeName;
        
        if (loadTheme(themeName)) {
            if (pImpl->onThemeChanged) {
                pImpl->onThemeChanged(themeName);
            }
        } else {
            // Revert on failure
            pImpl->currentTheme = oldTheme;
        }
    }
}

std::string GtkThemeManager::getCurrentTheme() const {
    return pImpl->currentTheme;
}

std::vector<std::string> GtkThemeManager::getAvailableThemes() const {
    std::vector<std::string> themes;
    for (const auto& pair : pImpl->themes) {
        themes.push_back(pair.first);
    }
    return themes;
}

bool GtkThemeManager::hasTheme(const std::string& themeName) const {
    return pImpl->themes.find(themeName) != pImpl->themes.end();
}

// Custom CSS
void GtkThemeManager::setCustomCss(const std::string& css) {
    pImpl->customCss = css;
    
    // Combine current theme with custom CSS
    std::string combinedCss;
    auto it = pImpl->themes.find(pImpl->currentTheme);
    if (it != pImpl->themes.end()) {
        combinedCss = it->second;
    }
    
    if (!pImpl->customCss.empty()) {
        combinedCss += "\n\n/* Custom CSS */\n" + pImpl->customCss;
    }
    
    loadCss(combinedCss);
}

std::string GtkThemeManager::getCustomCss() const {
    return pImpl->customCss;
}

void GtkThemeManager::addCustomCss(const std::string& css) {
    if (!pImpl->customCss.empty()) {
        pImpl->customCss += "\n";
    }
    pImpl->customCss += css;
    setCustomCss(pImpl->customCss);
}

void GtkThemeManager::clearCustomCss() {
    pImpl->customCss.clear();
    loadTheme(pImpl->currentTheme);
}

// Dark mode
void GtkThemeManager::setDarkMode(bool enabled) {
    if (pImpl->isDarkMode != enabled) {
        pImpl->isDarkMode = enabled;
        
#ifdef OWCAT_USE_GTK
        // Set GTK dark theme preference
        GtkSettings* settings = gtk_settings_get_default();
        if (settings) {
            g_object_set(settings, "gtk-application-prefer-dark-theme", enabled, nullptr);
        }
#endif
        
        // Switch to appropriate theme
        if (enabled && hasTheme("dark")) {
            setCurrentTheme("dark");
        } else if (!enabled && hasTheme("light")) {
            setCurrentTheme("light");
        }
        
        if (pImpl->onDarkModeChanged) {
            pImpl->onDarkModeChanged(enabled);
        }
    }
}

bool GtkThemeManager::isDarkMode() const {
    return pImpl->isDarkMode;
}

void GtkThemeManager::toggleDarkMode() {
    setDarkMode(!pImpl->isDarkMode);
}

// Theme registration
void GtkThemeManager::registerTheme(const std::string& name, const std::string& css) {
    pImpl->themes[name] = css;
}

void GtkThemeManager::unregisterTheme(const std::string& name) {
    auto it = pImpl->themes.find(name);
    if (it != pImpl->themes.end()) {
        // Don't allow removing built-in themes
        if (name != "default" && name != "dark" && name != "light" && name != "high-contrast") {
            pImpl->themes.erase(it);
        }
    }
}

// Utility methods
void GtkThemeManager::applyClassToWidget(GtkWidget* widget, const std::string& className) {
#ifdef OWCAT_USE_GTK
    if (widget) {
        GtkStyleContext* context = gtk_widget_get_style_context(widget);
        gtk_style_context_add_class(context, className.c_str());
    }
#endif
}

void GtkThemeManager::removeClassFromWidget(GtkWidget* widget, const std::string& className) {
#ifdef OWCAT_USE_GTK
    if (widget) {
        GtkStyleContext* context = gtk_widget_get_style_context(widget);
        gtk_style_context_remove_class(context, className.c_str());
    }
#endif
}

void GtkThemeManager::refreshTheme() {
    loadTheme(pImpl->currentTheme);
}

// Callbacks
void GtkThemeManager::setOnThemeChanged(std::function<void(const std::string&)> callback) {
    pImpl->onThemeChanged = callback;
}

void GtkThemeManager::setOnDarkModeChanged(std::function<void(bool)> callback) {
    pImpl->onDarkModeChanged = callback;
}

// Widget access
GtkCssProvider* GtkThemeManager::getCssProvider() const {
#ifdef OWCAT_USE_GTK
    return pImpl->cssProvider;
#else
    return nullptr;
#endif
}

} // namespace owcat