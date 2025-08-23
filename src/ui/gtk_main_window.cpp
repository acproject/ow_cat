#include "ui/gtk_ui.h"
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <sstream>
#include <functional>

#ifdef OWCAT_USE_GTK
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <glib.h>
#endif

namespace owcat {

// Implementation structure for GtkMainWindow
struct GtkMainWindow::Impl {
#ifdef OWCAT_USE_GTK
    GtkApplication* application;
    GtkWidget* window;
    GtkWidget* headerBar;
    GtkWidget* notebook;
    GtkWidget* statusbar;
    GtkWidget* menuBar;
    GtkWidget* toolbar;
    
    // Status page widgets
    GtkWidget* statusLabel;
    GtkWidget* statisticsGrid;
    
    // History page widgets
    GtkWidget* historyTreeView;
    GtkListStore* historyListStore;
    GtkWidget* clearHistoryButton;
    
    // Dictionary page widgets
    GtkWidget* dictionaryTreeView;
    GtkListStore* dictionaryListStore;
    GtkWidget* addDictionaryButton;
    GtkWidget* removeDictionaryButton;
    
    // Settings page widgets
    GtkWidget* settingsGrid;
    GtkWidget* enabledCheckButton;
    GtkWidget* autoStartCheckButton;
    GtkWidget* showNotificationsCheckButton;
    
    // Menu items
    GtkWidget* fileMenu;
    GtkWidget* editMenu;
    GtkWidget* viewMenu;
    GtkWidget* helpMenu;
#endif
    
    // State
    bool isVisible;
    std::string currentStatus;
    std::map<std::string, std::string> statistics;
    std::vector<std::string> inputHistory;
    std::vector<std::map<std::string, std::string>> dictionaries;
    
    // Callbacks
    std::function<void()> onClosed;
    std::function<void()> onSettingsRequested;
    std::function<void()> onAboutRequested;
    std::function<void(const std::string&)> onDictionarySelected;
    std::function<void()> onClearHistoryRequested;
    
    Impl() {
#ifdef OWCAT_USE_GTK
        application = nullptr;
        window = nullptr;
        headerBar = nullptr;
        notebook = nullptr;
        statusbar = nullptr;
        menuBar = nullptr;
        toolbar = nullptr;
        
        statusLabel = nullptr;
        statisticsGrid = nullptr;
        
        historyTreeView = nullptr;
        historyListStore = nullptr;
        clearHistoryButton = nullptr;
        
        dictionaryTreeView = nullptr;
        dictionaryListStore = nullptr;
        addDictionaryButton = nullptr;
        removeDictionaryButton = nullptr;
        
        settingsGrid = nullptr;
        enabledCheckButton = nullptr;
        autoStartCheckButton = nullptr;
        showNotificationsCheckButton = nullptr;
        
        fileMenu = nullptr;
        editMenu = nullptr;
        viewMenu = nullptr;
        helpMenu = nullptr;
#endif
        isVisible = false;
        currentStatus = "Ready";
    }
};

GtkMainWindow::GtkMainWindow() : pImpl(std::make_unique<Impl>()) {}

GtkMainWindow::~GtkMainWindow() = default;

bool GtkMainWindow::create(GtkApplication* app) {
#ifdef OWCAT_USE_GTK
    if (pImpl->window) {
        return true; // Already created
    }
    
    pImpl->application = app;
    
    // Create main window
    pImpl->window = gtk_application_window_new(app);
    if (!pImpl->window) {
        std::cerr << "Failed to create main window" << std::endl;
        return false;
    }
    
    // Set window properties
    gtk_window_set_title(GTK_WINDOW(pImpl->window), "OwCat IME");
    gtk_window_set_default_size(GTK_WINDOW(pImpl->window), 800, 600);
    gtk_window_set_position(GTK_WINDOW(pImpl->window), GTK_WIN_POS_CENTER);
    
    // Set window icon
    gtk_window_set_icon_name(GTK_WINDOW(pImpl->window), "owcat");
    
    // Create header bar
    createHeaderBar();
    
    // Create menu bar
    createMenuBar();
    
    // Create main content
    createMainContent();
    
    // Create status bar
    createStatusBar();
    
    // Set up layout
    setupLayout();
    
    // Connect signals
    connectSignals();
    
    std::cout << "Main window created successfully" << std::endl;
    return true;
#else
    return false;
#endif
}

void GtkMainWindow::destroy() {
#ifdef OWCAT_USE_GTK
    if (pImpl->window) {
        gtk_widget_destroy(pImpl->window);
        pImpl->window = nullptr;
    }
#endif
}

bool GtkMainWindow::show() {
#ifdef OWCAT_USE_GTK
    if (pImpl->window) {
        gtk_widget_show_all(pImpl->window);
        gtk_window_present(GTK_WINDOW(pImpl->window));
        pImpl->isVisible = true;
        return true;
    }
#endif
    return false;
}

bool GtkMainWindow::hide() {
#ifdef OWCAT_USE_GTK
    if (pImpl->window) {
        gtk_widget_hide(pImpl->window);
        pImpl->isVisible = false;
        return true;
    }
#endif
    return false;
}

bool GtkMainWindow::isVisible() const {
    return pImpl->isVisible;
}

void GtkMainWindow::setTitle(const std::string& title) {
#ifdef OWCAT_USE_GTK
    if (pImpl->window) {
        gtk_window_set_title(GTK_WINDOW(pImpl->window), title.c_str());
    }
#endif
}

void GtkMainWindow::setSize(int width, int height) {
#ifdef OWCAT_USE_GTK
    if (pImpl->window) {
        gtk_window_resize(GTK_WINDOW(pImpl->window), width, height);
    }
#endif
}

void GtkMainWindow::setPosition(int x, int y) {
#ifdef OWCAT_USE_GTK
    if (pImpl->window) {
        gtk_window_move(GTK_WINDOW(pImpl->window), x, y);
    }
#endif
}

void GtkMainWindow::center() {
#ifdef OWCAT_USE_GTK
    if (pImpl->window) {
        gtk_window_set_position(GTK_WINDOW(pImpl->window), GTK_WIN_POS_CENTER);
    }
#endif
}

// Content management
void GtkMainWindow::updateStatus(const std::string& status) {
    pImpl->currentStatus = status;
    
#ifdef OWCAT_USE_GTK
    if (pImpl->statusLabel) {
        gtk_label_set_text(GTK_LABEL(pImpl->statusLabel), status.c_str());
    }
    
    if (pImpl->statusbar) {
        gtk_statusbar_pop(GTK_STATUSBAR(pImpl->statusbar), 0);
        gtk_statusbar_push(GTK_STATUSBAR(pImpl->statusbar), 0, status.c_str());
    }
#endif
}

void GtkMainWindow::updateStatistics(const std::map<std::string, std::string>& stats) {
    pImpl->statistics = stats;
    
#ifdef OWCAT_USE_GTK
    if (pImpl->statisticsGrid) {
        // Clear existing statistics
        GList* children = gtk_container_get_children(GTK_CONTAINER(pImpl->statisticsGrid));
        for (GList* iter = children; iter != nullptr; iter = g_list_next(iter)) {
            gtk_widget_destroy(GTK_WIDGET(iter->data));
        }
        g_list_free(children);
        
        // Add new statistics
        int row = 0;
        for (const auto& stat : stats) {
            GtkWidget* keyLabel = gtk_label_new(stat.first.c_str());
            GtkWidget* valueLabel = gtk_label_new(stat.second.c_str());
            
            gtk_widget_set_halign(keyLabel, GTK_ALIGN_START);
            gtk_widget_set_halign(valueLabel, GTK_ALIGN_END);
            
            gtk_grid_attach(GTK_GRID(pImpl->statisticsGrid), keyLabel, 0, row, 1, 1);
            gtk_grid_attach(GTK_GRID(pImpl->statisticsGrid), valueLabel, 1, row, 1, 1);
            
            row++;
        }
        
        gtk_widget_show_all(pImpl->statisticsGrid);
    }
#endif
}

void GtkMainWindow::updateInputHistory(const std::vector<std::string>& history) {
    pImpl->inputHistory = history;
    
#ifdef OWCAT_USE_GTK
    if (pImpl->historyListStore) {
        gtk_list_store_clear(pImpl->historyListStore);
        
        for (const auto& item : history) {
            GtkTreeIter iter;
            gtk_list_store_append(pImpl->historyListStore, &iter);
            gtk_list_store_set(pImpl->historyListStore, &iter, 0, item.c_str(), -1);
        }
    }
#endif
}

void GtkMainWindow::updateDictionaries(const std::vector<std::map<std::string, std::string>>& dictionaries) {
    pImpl->dictionaries = dictionaries;
    
#ifdef OWCAT_USE_GTK
    if (pImpl->dictionaryListStore) {
        gtk_list_store_clear(pImpl->dictionaryListStore);
        
        for (const auto& dict : dictionaries) {
            GtkTreeIter iter;
            gtk_list_store_append(pImpl->dictionaryListStore, &iter);
            
            auto nameIt = dict.find("name");
            auto pathIt = dict.find("path");
            auto sizeIt = dict.find("size");
            
            gtk_list_store_set(pImpl->dictionaryListStore, &iter,
                              0, nameIt != dict.end() ? nameIt->second.c_str() : "Unknown",
                              1, pathIt != dict.end() ? pathIt->second.c_str() : "Unknown",
                              2, sizeIt != dict.end() ? sizeIt->second.c_str() : "0",
                              -1);
        }
    }
#endif
}

// Callbacks
void GtkMainWindow::setOnClosed(std::function<void()> callback) {
    pImpl->onClosed = callback;
}

void GtkMainWindow::setOnSettingsRequested(std::function<void()> callback) {
    pImpl->onSettingsRequested = callback;
}

void GtkMainWindow::setOnAboutRequested(std::function<void()> callback) {
    pImpl->onAboutRequested = callback;
}

void GtkMainWindow::setOnDictionarySelected(std::function<void(const std::string&)> callback) {
    pImpl->onDictionarySelected = callback;
}

void GtkMainWindow::setOnClearHistoryRequested(std::function<void()> callback) {
    pImpl->onClearHistoryRequested = callback;
}

// Widget access
GtkWidget* GtkMainWindow::getWidget() const {
#ifdef OWCAT_USE_GTK
    return pImpl->window;
#else
    return nullptr;
#endif
}

GtkWindow* GtkMainWindow::getWindow() const {
#ifdef OWCAT_USE_GTK
    return GTK_WINDOW(pImpl->window);
#else
    return nullptr;
#endif
}

// Private helper methods
void GtkMainWindow::createHeaderBar() {
#ifdef OWCAT_USE_GTK
    pImpl->headerBar = gtk_header_bar_new();
    gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(pImpl->headerBar), TRUE);
    gtk_header_bar_set_title(GTK_HEADER_BAR(pImpl->headerBar), "OwCat IME");
    gtk_header_bar_set_subtitle(GTK_HEADER_BAR(pImpl->headerBar), "Chinese Input Method");
    
    // Add header bar buttons
    GtkWidget* settingsButton = gtk_button_new_from_icon_name("preferences-system", GTK_ICON_SIZE_BUTTON);
    gtk_widget_set_tooltip_text(settingsButton, "Settings");
    gtk_header_bar_pack_end(GTK_HEADER_BAR(pImpl->headerBar), settingsButton);
    
    g_signal_connect(settingsButton, "clicked", G_CALLBACK(+[](GtkButton* button, gpointer userData) {
        GtkMainWindow* window = static_cast<GtkMainWindow*>(userData);
        if (window->pImpl->onSettingsRequested) {
            window->pImpl->onSettingsRequested();
        }
    }), this);
    
    GtkWidget* aboutButton = gtk_button_new_from_icon_name("help-about", GTK_ICON_SIZE_BUTTON);
    gtk_widget_set_tooltip_text(aboutButton, "About");
    gtk_header_bar_pack_end(GTK_HEADER_BAR(pImpl->headerBar), aboutButton);
    
    g_signal_connect(aboutButton, "clicked", G_CALLBACK(+[](GtkButton* button, gpointer userData) {
        GtkMainWindow* window = static_cast<GtkMainWindow*>(userData);
        if (window->pImpl->onAboutRequested) {
            window->pImpl->onAboutRequested();
        }
    }), this);
    
    gtk_window_set_titlebar(GTK_WINDOW(pImpl->window), pImpl->headerBar);
#endif
}

void GtkMainWindow::createMenuBar() {
#ifdef OWCAT_USE_GTK
    pImpl->menuBar = gtk_menu_bar_new();
    
    // File menu
    pImpl->fileMenu = gtk_menu_new();
    GtkWidget* fileMenuItem = gtk_menu_item_new_with_label("File");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(fileMenuItem), pImpl->fileMenu);
    gtk_menu_shell_append(GTK_MENU_SHELL(pImpl->menuBar), fileMenuItem);
    
    // Add file menu items
    GtkWidget* quitMenuItem = gtk_menu_item_new_with_label("Quit");
    gtk_menu_shell_append(GTK_MENU_SHELL(pImpl->fileMenu), quitMenuItem);
    g_signal_connect(quitMenuItem, "activate", G_CALLBACK(+[](GtkMenuItem* item, gpointer userData) {
        GtkMainWindow* window = static_cast<GtkMainWindow*>(userData);
        if (window->pImpl->onClosed) {
            window->pImpl->onClosed();
        }
    }), this);
    
    // Edit menu
    pImpl->editMenu = gtk_menu_new();
    GtkWidget* editMenuItem = gtk_menu_item_new_with_label("Edit");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(editMenuItem), pImpl->editMenu);
    gtk_menu_shell_append(GTK_MENU_SHELL(pImpl->menuBar), editMenuItem);
    
    // Add edit menu items
    GtkWidget* preferencesMenuItem = gtk_menu_item_new_with_label("Preferences");
    gtk_menu_shell_append(GTK_MENU_SHELL(pImpl->editMenu), preferencesMenuItem);
    g_signal_connect(preferencesMenuItem, "activate", G_CALLBACK(+[](GtkMenuItem* item, gpointer userData) {
        GtkMainWindow* window = static_cast<GtkMainWindow*>(userData);
        if (window->pImpl->onSettingsRequested) {
            window->pImpl->onSettingsRequested();
        }
    }), this);
    
    // Help menu
    pImpl->helpMenu = gtk_menu_new();
    GtkWidget* helpMenuItem = gtk_menu_item_new_with_label("Help");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(helpMenuItem), pImpl->helpMenu);
    gtk_menu_shell_append(GTK_MENU_SHELL(pImpl->menuBar), helpMenuItem);
    
    // Add help menu items
    GtkWidget* aboutMenuItem = gtk_menu_item_new_with_label("About");
    gtk_menu_shell_append(GTK_MENU_SHELL(pImpl->helpMenu), aboutMenuItem);
    g_signal_connect(aboutMenuItem, "activate", G_CALLBACK(+[](GtkMenuItem* item, gpointer userData) {
        GtkMainWindow* window = static_cast<GtkMainWindow*>(userData);
        if (window->pImpl->onAboutRequested) {
            window->pImpl->onAboutRequested();
        }
    }), this);
#endif
}

void GtkMainWindow::createMainContent() {
#ifdef OWCAT_USE_GTK
    // Create notebook for tabbed interface
    pImpl->notebook = gtk_notebook_new();
    gtk_notebook_set_tab_pos(GTK_NOTEBOOK(pImpl->notebook), GTK_POS_TOP);
    
    // Create status page
    createStatusPage();
    
    // Create history page
    createHistoryPage();
    
    // Create dictionary page
    createDictionaryPage();
    
    // Create settings page
    createSettingsPage();
#endif
}

void GtkMainWindow::createStatusPage() {
#ifdef OWCAT_USE_GTK
    GtkWidget* statusPage = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(statusPage), 10);
    
    // Status label
    pImpl->statusLabel = gtk_label_new(pImpl->currentStatus.c_str());
    gtk_widget_set_halign(pImpl->statusLabel, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(statusPage), pImpl->statusLabel, FALSE, FALSE, 0);
    
    // Statistics grid
    pImpl->statisticsGrid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(pImpl->statisticsGrid), 5);
    gtk_grid_set_column_spacing(GTK_GRID(pImpl->statisticsGrid), 10);
    gtk_box_pack_start(GTK_BOX(statusPage), pImpl->statisticsGrid, TRUE, TRUE, 0);
    
    // Add tab
    GtkWidget* statusLabel = gtk_label_new("Status");
    gtk_notebook_append_page(GTK_NOTEBOOK(pImpl->notebook), statusPage, statusLabel);
#endif
}

void GtkMainWindow::createHistoryPage() {
#ifdef OWCAT_USE_GTK
    GtkWidget* historyPage = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(historyPage), 10);
    
    // Create tree view for history
    pImpl->historyListStore = gtk_list_store_new(1, G_TYPE_STRING);
    pImpl->historyTreeView = gtk_tree_view_new_with_model(GTK_TREE_MODEL(pImpl->historyListStore));
    
    // Add column
    GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn* column = gtk_tree_view_column_new_with_attributes("Input History", renderer, "text", 0, nullptr);
    gtk_tree_view_append_column(GTK_TREE_VIEW(pImpl->historyTreeView), column);
    
    // Add scrolled window
    GtkWidget* scrolledWindow = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledWindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scrolledWindow), pImpl->historyTreeView);
    gtk_box_pack_start(GTK_BOX(historyPage), scrolledWindow, TRUE, TRUE, 0);
    
    // Clear history button
    pImpl->clearHistoryButton = gtk_button_new_with_label("Clear History");
    gtk_box_pack_start(GTK_BOX(historyPage), pImpl->clearHistoryButton, FALSE, FALSE, 0);
    
    g_signal_connect(pImpl->clearHistoryButton, "clicked", G_CALLBACK(+[](GtkButton* button, gpointer userData) {
        GtkMainWindow* window = static_cast<GtkMainWindow*>(userData);
        if (window->pImpl->onClearHistoryRequested) {
            window->pImpl->onClearHistoryRequested();
        }
    }), this);
    
    // Add tab
    GtkWidget* historyLabel = gtk_label_new("History");
    gtk_notebook_append_page(GTK_NOTEBOOK(pImpl->notebook), historyPage, historyLabel);
#endif
}

void GtkMainWindow::createDictionaryPage() {
#ifdef OWCAT_USE_GTK
    GtkWidget* dictionaryPage = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(dictionaryPage), 10);
    
    // Create tree view for dictionaries
    pImpl->dictionaryListStore = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    pImpl->dictionaryTreeView = gtk_tree_view_new_with_model(GTK_TREE_MODEL(pImpl->dictionaryListStore));
    
    // Add columns
    GtkCellRenderer* renderer1 = gtk_cell_renderer_text_new();
    GtkTreeViewColumn* nameColumn = gtk_tree_view_column_new_with_attributes("Name", renderer1, "text", 0, nullptr);
    gtk_tree_view_append_column(GTK_TREE_VIEW(pImpl->dictionaryTreeView), nameColumn);
    
    GtkCellRenderer* renderer2 = gtk_cell_renderer_text_new();
    GtkTreeViewColumn* pathColumn = gtk_tree_view_column_new_with_attributes("Path", renderer2, "text", 1, nullptr);
    gtk_tree_view_append_column(GTK_TREE_VIEW(pImpl->dictionaryTreeView), pathColumn);
    
    GtkCellRenderer* renderer3 = gtk_cell_renderer_text_new();
    GtkTreeViewColumn* sizeColumn = gtk_tree_view_column_new_with_attributes("Size", renderer3, "text", 2, nullptr);
    gtk_tree_view_append_column(GTK_TREE_VIEW(pImpl->dictionaryTreeView), sizeColumn);
    
    // Add scrolled window
    GtkWidget* scrolledWindow = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledWindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scrolledWindow), pImpl->dictionaryTreeView);
    gtk_box_pack_start(GTK_BOX(dictionaryPage), scrolledWindow, TRUE, TRUE, 0);
    
    // Button box
    GtkWidget* buttonBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    
    pImpl->addDictionaryButton = gtk_button_new_with_label("Add Dictionary");
    gtk_box_pack_start(GTK_BOX(buttonBox), pImpl->addDictionaryButton, FALSE, FALSE, 0);
    
    pImpl->removeDictionaryButton = gtk_button_new_with_label("Remove Dictionary");
    gtk_box_pack_start(GTK_BOX(buttonBox), pImpl->removeDictionaryButton, FALSE, FALSE, 0);
    
    gtk_box_pack_start(GTK_BOX(dictionaryPage), buttonBox, FALSE, FALSE, 0);
    
    // Connect signals
    GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(pImpl->dictionaryTreeView));
    g_signal_connect(selection, "changed", G_CALLBACK(+[](GtkTreeSelection* selection, gpointer userData) {
        GtkMainWindow* window = static_cast<GtkMainWindow*>(userData);
        GtkTreeIter iter;
        GtkTreeModel* model;
        
        if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
            gchar* name;
            gtk_tree_model_get(model, &iter, 0, &name, -1);
            
            if (window->pImpl->onDictionarySelected && name) {
                window->pImpl->onDictionarySelected(name);
            }
            
            g_free(name);
        }
    }), this);
    
    // Add tab
    GtkWidget* dictionaryLabel = gtk_label_new("Dictionaries");
    gtk_notebook_append_page(GTK_NOTEBOOK(pImpl->notebook), dictionaryPage, dictionaryLabel);
#endif
}

void GtkMainWindow::createSettingsPage() {
#ifdef OWCAT_USE_GTK
    GtkWidget* settingsPage = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(settingsPage), 10);
    
    // Settings grid
    pImpl->settingsGrid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(pImpl->settingsGrid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(pImpl->settingsGrid), 10);
    
    // Enable IME checkbox
    pImpl->enabledCheckButton = gtk_check_button_new_with_label("Enable OwCat IME");
    gtk_grid_attach(GTK_GRID(pImpl->settingsGrid), pImpl->enabledCheckButton, 0, 0, 2, 1);
    
    // Auto start checkbox
    pImpl->autoStartCheckButton = gtk_check_button_new_with_label("Start with system");
    gtk_grid_attach(GTK_GRID(pImpl->settingsGrid), pImpl->autoStartCheckButton, 0, 1, 2, 1);
    
    // Show notifications checkbox
    pImpl->showNotificationsCheckButton = gtk_check_button_new_with_label("Show notifications");
    gtk_grid_attach(GTK_GRID(pImpl->settingsGrid), pImpl->showNotificationsCheckButton, 0, 2, 2, 1);
    
    gtk_box_pack_start(GTK_BOX(settingsPage), pImpl->settingsGrid, FALSE, FALSE, 0);
    
    // Add tab
    GtkWidget* settingsLabel = gtk_label_new("Settings");
    gtk_notebook_append_page(GTK_NOTEBOOK(pImpl->notebook), settingsPage, settingsLabel);
#endif
}

void GtkMainWindow::createStatusBar() {
#ifdef OWCAT_USE_GTK
    pImpl->statusbar = gtk_statusbar_new();
    gtk_statusbar_push(GTK_STATUSBAR(pImpl->statusbar), 0, pImpl->currentStatus.c_str());
#endif
}

void GtkMainWindow::setupLayout() {
#ifdef OWCAT_USE_GTK
    // Create main vertical box
    GtkWidget* mainVBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    
    // Add menu bar
    gtk_box_pack_start(GTK_BOX(mainVBox), pImpl->menuBar, FALSE, FALSE, 0);
    
    // Add notebook
    gtk_box_pack_start(GTK_BOX(mainVBox), pImpl->notebook, TRUE, TRUE, 0);
    
    // Add status bar
    gtk_box_pack_start(GTK_BOX(mainVBox), pImpl->statusbar, FALSE, FALSE, 0);
    
    // Add to window
    gtk_container_add(GTK_CONTAINER(pImpl->window), mainVBox);
#endif
}

void GtkMainWindow::connectSignals() {
#ifdef OWCAT_USE_GTK
    // Window close signal
    g_signal_connect(pImpl->window, "delete-event", G_CALLBACK(+[](GtkWidget* widget, GdkEvent* event, gpointer userData) -> gboolean {
        GtkMainWindow* window = static_cast<GtkMainWindow*>(userData);
        if (window->pImpl->onClosed) {
            window->pImpl->onClosed();
        }
        return FALSE; // Allow window to close
    }), this);
    
    // Window destroy signal
    g_signal_connect(pImpl->window, "destroy", G_CALLBACK(+[](GtkWidget* widget, gpointer userData) {
        GtkMainWindow* window = static_cast<GtkMainWindow*>(userData);
        window->pImpl->isVisible = false;
    }), this);
#endif
}

} // namespace owcat