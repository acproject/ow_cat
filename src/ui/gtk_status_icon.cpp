#include "ui/gtk_ui.h"
#include <memory>
#include <string>
#include <vector>
#include <iostream>

#ifdef OWCAT_USE_GTK
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <glib.h>
#endif

namespace owcat {

// Implementation structure for GtkStatusIcon
struct GtkStatusIcon::Impl {
#ifdef OWCAT_USE_GTK
    GtkStatusIcon* statusIcon;
    GtkWidget* menu;
    GtkWidget* enabledMenuItem;
    GtkWidget* modeMenuItem;
    GtkWidget* settingsMenuItem;
    GtkWidget* aboutMenuItem;
    GtkWidget* separatorMenuItem1;
    GtkWidget* separatorMenuItem2;
    GtkWidget* quitMenuItem;
#endif
    
    // State
    bool isVisible;
    bool isEnabled;
    std::string currentMode;
    std::string tooltip;
    std::string iconName;
    
    // Callbacks
    std::function<void()> onActivated;
    std::function<void()> onToggleEnabled;
    std::function<void()> onShowSettings;
    std::function<void()> onShowAbout;
    std::function<void()> onQuit;
    std::function<void(int, int)> onPopupMenu;
    
    Impl() {
#ifdef OWCAT_USE_GTK
        statusIcon = nullptr;
        menu = nullptr;
        enabledMenuItem = nullptr;
        modeMenuItem = nullptr;
        settingsMenuItem = nullptr;
        aboutMenuItem = nullptr;
        separatorMenuItem1 = nullptr;
        separatorMenuItem2 = nullptr;
        quitMenuItem = nullptr;
#endif
        
        isVisible = false;
        isEnabled = true;
        currentMode = "Chinese";
        tooltip = "OwCat Input Method";
        iconName = "input-keyboard";
    }
};

GtkStatusIcon::GtkStatusIcon() : pImpl(std::make_unique<Impl>()) {}

GtkStatusIcon::~GtkStatusIcon() = default;

bool GtkStatusIcon::create() {
#ifdef OWCAT_USE_GTK
    if (pImpl->statusIcon) {
        return true; // Already created
    }
    
    // Create status icon
    pImpl->statusIcon = gtk_status_icon_new_from_icon_name(pImpl->iconName.c_str());
    if (!pImpl->statusIcon) {
        std::cerr << "Failed to create status icon" << std::endl;
        return false;
    }
    
    // Set properties
    gtk_status_icon_set_tooltip_text(pImpl->statusIcon, pImpl->tooltip.c_str());
    gtk_status_icon_set_visible(pImpl->statusIcon, TRUE);
    
    // Create context menu
    createMenu();
    
    // Connect signals
    connectSignals();
    
    pImpl->isVisible = true;
    std::cout << "Status icon created successfully" << std::endl;
    return true;
#else
    std::cerr << "GTK support not available" << std::endl;
    return false;
#endif
}

void GtkStatusIcon::destroy() {
#ifdef OWCAT_USE_GTK
    if (pImpl->menu) {
        gtk_widget_destroy(pImpl->menu);
        pImpl->menu = nullptr;
    }
    
    if (pImpl->statusIcon) {
        g_object_unref(pImpl->statusIcon);
        pImpl->statusIcon = nullptr;
    }
    
    pImpl->isVisible = false;
#endif
}

bool GtkStatusIcon::show() {
#ifdef OWCAT_USE_GTK
    if (pImpl->statusIcon) {
        gtk_status_icon_set_visible(pImpl->statusIcon, TRUE);
        pImpl->isVisible = true;
        return true;
    }
#endif
    return false;
}

bool GtkStatusIcon::hide() {
#ifdef OWCAT_USE_GTK
    if (pImpl->statusIcon) {
        gtk_status_icon_set_visible(pImpl->statusIcon, FALSE);
        pImpl->isVisible = false;
        return true;
    }
#endif
    return false;
}

bool GtkStatusIcon::isVisible() const {
    return pImpl->isVisible;
}

// Icon properties
void GtkStatusIcon::setIcon(const std::string& iconName) {
    pImpl->iconName = iconName;
#ifdef OWCAT_USE_GTK
    if (pImpl->statusIcon) {
        gtk_status_icon_set_from_icon_name(pImpl->statusIcon, iconName.c_str());
    }
#endif
}

void GtkStatusIcon::setTooltip(const std::string& tooltip) {
    pImpl->tooltip = tooltip;
#ifdef OWCAT_USE_GTK
    if (pImpl->statusIcon) {
        gtk_status_icon_set_tooltip_text(pImpl->statusIcon, tooltip.c_str());
    }
#endif
}

std::string GtkStatusIcon::getIcon() const {
    return pImpl->iconName;
}

std::string GtkStatusIcon::getTooltip() const {
    return pImpl->tooltip;
}

// State management
void GtkStatusIcon::setEnabled(bool enabled) {
    pImpl->isEnabled = enabled;
    updateMenu();
    
    // Update icon based on state
    if (enabled) {
        setIcon("input-keyboard");
    } else {
        setIcon("input-keyboard-symbolic");
    }
}

void GtkStatusIcon::setMode(const std::string& mode) {
    pImpl->currentMode = mode;
    updateMenu();
    
    // Update tooltip
    std::string newTooltip = "OwCat Input Method - " + mode;
    setTooltip(newTooltip);
}

bool GtkStatusIcon::isEnabled() const {
    return pImpl->isEnabled;
}

std::string GtkStatusIcon::getMode() const {
    return pImpl->currentMode;
}

// Menu management
void GtkStatusIcon::showMenu(int x, int y) {
#ifdef OWCAT_USE_GTK
    if (pImpl->menu) {
        gtk_menu_popup_at_pointer(GTK_MENU(pImpl->menu), nullptr);
    }
#endif
}

void GtkStatusIcon::updateMenu() {
#ifdef OWCAT_USE_GTK
    if (!pImpl->menu) {
        return;
    }
    
    // Update enabled menu item
    if (pImpl->enabledMenuItem) {
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(pImpl->enabledMenuItem), pImpl->isEnabled);
    }
    
    // Update mode menu item
    if (pImpl->modeMenuItem) {
        std::string modeText = "Mode: " + pImpl->currentMode;
        gtk_menu_item_set_label(GTK_MENU_ITEM(pImpl->modeMenuItem), modeText.c_str());
    }
#endif
}

// Callbacks
void GtkStatusIcon::setOnActivated(std::function<void()> callback) {
    pImpl->onActivated = callback;
}

void GtkStatusIcon::setOnToggleEnabled(std::function<void()> callback) {
    pImpl->onToggleEnabled = callback;
}

void GtkStatusIcon::setOnShowSettings(std::function<void()> callback) {
    pImpl->onShowSettings = callback;
}

void GtkStatusIcon::setOnShowAbout(std::function<void()> callback) {
    pImpl->onShowAbout = callback;
}

void GtkStatusIcon::setOnQuit(std::function<void()> callback) {
    pImpl->onQuit = callback;
}

void GtkStatusIcon::setOnPopupMenu(std::function<void(int, int)> callback) {
    pImpl->onPopupMenu = callback;
}

// Widget access
GtkStatusIcon* GtkStatusIcon::getStatusIcon() const {
#ifdef OWCAT_USE_GTK
    return pImpl->statusIcon;
#else
    return nullptr;
#endif
}

GtkWidget* GtkStatusIcon::getMenu() const {
#ifdef OWCAT_USE_GTK
    return pImpl->menu;
#else
    return nullptr;
#endif
}

// Private helper methods
void GtkStatusIcon::createMenu() {
#ifdef OWCAT_USE_GTK
    pImpl->menu = gtk_menu_new();
    
    // Enabled toggle
    pImpl->enabledMenuItem = gtk_check_menu_item_new_with_label("Enabled");
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(pImpl->enabledMenuItem), pImpl->isEnabled);
    gtk_menu_shell_append(GTK_MENU_SHELL(pImpl->menu), pImpl->enabledMenuItem);
    
    // Mode display
    std::string modeText = "Mode: " + pImpl->currentMode;
    pImpl->modeMenuItem = gtk_menu_item_new_with_label(modeText.c_str());
    gtk_widget_set_sensitive(pImpl->modeMenuItem, FALSE); // Read-only
    gtk_menu_shell_append(GTK_MENU_SHELL(pImpl->menu), pImpl->modeMenuItem);
    
    // Separator
    pImpl->separatorMenuItem1 = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(pImpl->menu), pImpl->separatorMenuItem1);
    
    // Settings
    pImpl->settingsMenuItem = gtk_menu_item_new_with_label("Settings...");
    gtk_menu_shell_append(GTK_MENU_SHELL(pImpl->menu), pImpl->settingsMenuItem);
    
    // About
    pImpl->aboutMenuItem = gtk_menu_item_new_with_label("About...");
    gtk_menu_shell_append(GTK_MENU_SHELL(pImpl->menu), pImpl->aboutMenuItem);
    
    // Separator
    pImpl->separatorMenuItem2 = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(pImpl->menu), pImpl->separatorMenuItem2);
    
    // Quit
    pImpl->quitMenuItem = gtk_menu_item_new_with_label("Quit");
    gtk_menu_shell_append(GTK_MENU_SHELL(pImpl->menu), pImpl->quitMenuItem);
    
    // Show all menu items
    gtk_widget_show_all(pImpl->menu);
    
    // Connect menu item signals
    connectMenuSignals();
#endif
}

void GtkStatusIcon::connectSignals() {
#ifdef OWCAT_USE_GTK
    if (!pImpl->statusIcon) {
        return;
    }
    
    // Activate signal (left click)
    g_signal_connect(pImpl->statusIcon, "activate", G_CALLBACK(+[](GtkStatusIcon* statusIcon, gpointer userData) {
        GtkStatusIcon* self = static_cast<GtkStatusIcon*>(userData);
        if (self->pImpl->onActivated) {
            self->pImpl->onActivated();
        }
    }), this);
    
    // Popup menu signal (right click)
    g_signal_connect(pImpl->statusIcon, "popup-menu", G_CALLBACK(+[](GtkStatusIcon* statusIcon, guint button, guint activateTime, gpointer userData) {
        GtkStatusIcon* self = static_cast<GtkStatusIcon*>(userData);
        
        if (self->pImpl->menu) {
            gtk_menu_popup_at_pointer(GTK_MENU(self->pImpl->menu), nullptr);
        }
        
        if (self->pImpl->onPopupMenu) {
            self->pImpl->onPopupMenu(0, 0); // GTK handles positioning
        }
    }), this);
#endif
}

void GtkStatusIcon::connectMenuSignals() {
#ifdef OWCAT_USE_GTK
    // Enabled toggle
    if (pImpl->enabledMenuItem) {
        g_signal_connect(pImpl->enabledMenuItem, "toggled", G_CALLBACK(+[](GtkCheckMenuItem* menuItem, gpointer userData) {
            GtkStatusIcon* self = static_cast<GtkStatusIcon*>(userData);
            bool active = gtk_check_menu_item_get_active(menuItem);
            self->pImpl->isEnabled = active;
            
            if (self->pImpl->onToggleEnabled) {
                self->pImpl->onToggleEnabled();
            }
            
            // Update icon
            if (active) {
                self->setIcon("input-keyboard");
            } else {
                self->setIcon("input-keyboard-symbolic");
            }
        }), this);
    }
    
    // Settings
    if (pImpl->settingsMenuItem) {
        g_signal_connect(pImpl->settingsMenuItem, "activate", G_CALLBACK(+[](GtkMenuItem* menuItem, gpointer userData) {
            GtkStatusIcon* self = static_cast<GtkStatusIcon*>(userData);
            if (self->pImpl->onShowSettings) {
                self->pImpl->onShowSettings();
            }
        }), this);
    }
    
    // About
    if (pImpl->aboutMenuItem) {
        g_signal_connect(pImpl->aboutMenuItem, "activate", G_CALLBACK(+[](GtkMenuItem* menuItem, gpointer userData) {
            GtkStatusIcon* self = static_cast<GtkStatusIcon*>(userData);
            if (self->pImpl->onShowAbout) {
                self->pImpl->onShowAbout();
            }
        }), this);
    }
    
    // Quit
    if (pImpl->quitMenuItem) {
        g_signal_connect(pImpl->quitMenuItem, "activate", G_CALLBACK(+[](GtkMenuItem* menuItem, gpointer userData) {
            GtkStatusIcon* self = static_cast<GtkStatusIcon*>(userData);
            if (self->pImpl->onQuit) {
                self->pImpl->onQuit();
            }
        }), this);
    }
#endif
}

} // namespace owcat