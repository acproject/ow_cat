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

// Implementation structure for GtkSettingsDialog
struct GtkSettingsDialog::Impl {
#ifdef OWCAT_USE_GTK
    GtkWidget* dialog;
    GtkWidget* notebook;
    GtkWidget* buttonBox;
    GtkWidget* okButton;
    GtkWidget* cancelButton;
    GtkWidget* applyButton;
    GtkWidget* resetButton;
    
    // General settings page
    GtkWidget* generalPage;
    GtkWidget* enabledCheckButton;
    GtkWidget* autoStartCheckButton;
    GtkWidget* showNotificationsCheckButton;
    GtkWidget* languageComboBox;
    GtkWidget* themeComboBox;
    
    // Input settings page
    GtkWidget* inputPage;
    GtkWidget* pinyinModeComboBox;
    GtkWidget* fuzzyPinyinCheckButton;
    GtkWidget* autoCommitCheckButton;
    GtkWidget* pageUpKeyEntry;
    GtkWidget* pageDownKeyEntry;
    GtkWidget* candidateKeysEntry;
    
    // Appearance settings page
    GtkWidget* appearancePage;
    GtkWidget* fontButton;
    GtkWidget* candidateWindowOpacityScale;
    GtkWidget* showCandidateNumbersCheckButton;
    GtkWidget* verticalCandidateListCheckButton;
    GtkWidget* candidateWindowColorButton;
    GtkWidget* selectedCandidateColorButton;
    
    // Advanced settings page
    GtkWidget* advancedPage;
    GtkWidget* dictionaryPathEntry;
    GtkWidget* browseDictionaryButton;
    GtkWidget* userDictionaryPathEntry;
    GtkWidget* browseUserDictionaryButton;
    GtkWidget* logLevelComboBox;
    GtkWidget* enableLoggingCheckButton;
    GtkWidget* maxHistorySpinButton;
    
    // Hotkeys settings page
    GtkWidget* hotkeysPage;
    GtkWidget* toggleKeyEntry;
    GtkWidget* switchModeKeyEntry;
    GtkWidget* temporaryEnglishKeyEntry;
    GtkWidget* fullWidthKeyEntry;
    GtkWidget* punctuationKeyEntry;
#endif
    
    // State
    bool isVisible;
    std::map<std::string, std::string> currentSettings;
    std::map<std::string, std::string> originalSettings;
    
    // Callbacks
    std::function<void(const std::map<std::string, std::string>&)> onSettingsChanged;
    std::function<void()> onDialogClosed;
    std::function<void()> onResetRequested;
    
    Impl() {
#ifdef OWCAT_USE_GTK
        dialog = nullptr;
        notebook = nullptr;
        buttonBox = nullptr;
        okButton = nullptr;
        cancelButton = nullptr;
        applyButton = nullptr;
        resetButton = nullptr;
        
        generalPage = nullptr;
        enabledCheckButton = nullptr;
        autoStartCheckButton = nullptr;
        showNotificationsCheckButton = nullptr;
        languageComboBox = nullptr;
        themeComboBox = nullptr;
        
        inputPage = nullptr;
        pinyinModeComboBox = nullptr;
        fuzzyPinyinCheckButton = nullptr;
        autoCommitCheckButton = nullptr;
        pageUpKeyEntry = nullptr;
        pageDownKeyEntry = nullptr;
        candidateKeysEntry = nullptr;
        
        appearancePage = nullptr;
        fontButton = nullptr;
        candidateWindowOpacityScale = nullptr;
        showCandidateNumbersCheckButton = nullptr;
        verticalCandidateListCheckButton = nullptr;
        candidateWindowColorButton = nullptr;
        selectedCandidateColorButton = nullptr;
        
        advancedPage = nullptr;
        dictionaryPathEntry = nullptr;
        browseDictionaryButton = nullptr;
        userDictionaryPathEntry = nullptr;
        browseUserDictionaryButton = nullptr;
        logLevelComboBox = nullptr;
        enableLoggingCheckButton = nullptr;
        maxHistorySpinButton = nullptr;
        
        hotkeysPage = nullptr;
        toggleKeyEntry = nullptr;
        switchModeKeyEntry = nullptr;
        temporaryEnglishKeyEntry = nullptr;
        fullWidthKeyEntry = nullptr;
        punctuationKeyEntry = nullptr;
#endif
        
        isVisible = false;
    }
};

GtkSettingsDialog::GtkSettingsDialog() : pImpl(std::make_unique<Impl>()) {}

GtkSettingsDialog::~GtkSettingsDialog() = default;

bool GtkSettingsDialog::create(GtkWindow* parent) {
#ifdef OWCAT_USE_GTK
    if (pImpl->dialog) {
        return true; // Already created
    }
    
    // Create dialog
    pImpl->dialog = gtk_dialog_new();
    if (!pImpl->dialog) {
        std::cerr << "Failed to create settings dialog" << std::endl;
        return false;
    }
    
    // Set dialog properties
    gtk_window_set_title(GTK_WINDOW(pImpl->dialog), "OwCat Settings");
    gtk_window_set_default_size(GTK_WINDOW(pImpl->dialog), 600, 500);
    gtk_window_set_modal(GTK_WINDOW(pImpl->dialog), TRUE);
    gtk_window_set_resizable(GTK_WINDOW(pImpl->dialog), TRUE);
    
    if (parent) {
        gtk_window_set_transient_for(GTK_WINDOW(pImpl->dialog), parent);
        gtk_window_set_position(GTK_WINDOW(pImpl->dialog), GTK_WIN_POS_CENTER_ON_PARENT);
    } else {
        gtk_window_set_position(GTK_WINDOW(pImpl->dialog), GTK_WIN_POS_CENTER);
    }
    
    // Create content
    createContent();
    
    // Create buttons
    createButtons();
    
    // Connect signals
    connectSignals();
    
    std::cout << "Settings dialog created successfully" << std::endl;
    return true;
#else
    return false;
#endif
}

void GtkSettingsDialog::destroy() {
#ifdef OWCAT_USE_GTK
    if (pImpl->dialog) {
        gtk_widget_destroy(pImpl->dialog);
        pImpl->dialog = nullptr;
    }
#endif
}

bool GtkSettingsDialog::show() {
#ifdef OWCAT_USE_GTK
    if (pImpl->dialog) {
        // Store original settings
        pImpl->originalSettings = pImpl->currentSettings;
        
        // Load current settings into UI
        loadSettings();
        
        gtk_widget_show_all(pImpl->dialog);
        pImpl->isVisible = true;
        return true;
    }
#endif
    return false;
}

bool GtkSettingsDialog::hide() {
#ifdef OWCAT_USE_GTK
    if (pImpl->dialog) {
        gtk_widget_hide(pImpl->dialog);
        pImpl->isVisible = false;
        return true;
    }
#endif
    return false;
}

bool GtkSettingsDialog::isVisible() const {
    return pImpl->isVisible;
}

void GtkSettingsDialog::setSettings(const std::map<std::string, std::string>& settings) {
    pImpl->currentSettings = settings;
    if (pImpl->isVisible) {
        loadSettings();
    }
}

std::map<std::string, std::string> GtkSettingsDialog::getSettings() const {
    if (pImpl->isVisible) {
        // Get current values from UI
        const_cast<GtkSettingsDialog*>(this)->saveSettings();
    }
    return pImpl->currentSettings;
}

void GtkSettingsDialog::resetToDefaults() {
    // Set default values
    pImpl->currentSettings.clear();
    pImpl->currentSettings["enabled"] = "true";
    pImpl->currentSettings["auto_start"] = "false";
    pImpl->currentSettings["show_notifications"] = "true";
    pImpl->currentSettings["language"] = "zh_CN";
    pImpl->currentSettings["theme"] = "default";
    pImpl->currentSettings["pinyin_mode"] = "full";
    pImpl->currentSettings["fuzzy_pinyin"] = "true";
    pImpl->currentSettings["auto_commit"] = "false";
    pImpl->currentSettings["page_up_key"] = "minus";
    pImpl->currentSettings["page_down_key"] = "equal";
    pImpl->currentSettings["candidate_keys"] = "1234567890";
    pImpl->currentSettings["font"] = "Sans 12";
    pImpl->currentSettings["candidate_window_opacity"] = "0.9";
    pImpl->currentSettings["show_candidate_numbers"] = "true";
    pImpl->currentSettings["vertical_candidate_list"] = "false";
    pImpl->currentSettings["candidate_window_color"] = "#FFFFFF";
    pImpl->currentSettings["selected_candidate_color"] = "#3584E4";
    pImpl->currentSettings["dictionary_path"] = "";
    pImpl->currentSettings["user_dictionary_path"] = "";
    pImpl->currentSettings["log_level"] = "info";
    pImpl->currentSettings["enable_logging"] = "true";
    pImpl->currentSettings["max_history"] = "1000";
    pImpl->currentSettings["toggle_key"] = "ctrl+space";
    pImpl->currentSettings["switch_mode_key"] = "shift";
    pImpl->currentSettings["temporary_english_key"] = "shift";
    pImpl->currentSettings["full_width_key"] = "shift+space";
    pImpl->currentSettings["punctuation_key"] = "ctrl+period";
    
    if (pImpl->isVisible) {
        loadSettings();
    }
    
    if (pImpl->onResetRequested) {
        pImpl->onResetRequested();
    }
}

// Callbacks
void GtkSettingsDialog::setOnSettingsChanged(std::function<void(const std::map<std::string, std::string>&)> callback) {
    pImpl->onSettingsChanged = callback;
}

void GtkSettingsDialog::setOnDialogClosed(std::function<void()> callback) {
    pImpl->onDialogClosed = callback;
}

void GtkSettingsDialog::setOnResetRequested(std::function<void()> callback) {
    pImpl->onResetRequested = callback;
}

// Widget access
GtkWidget* GtkSettingsDialog::getWidget() const {
#ifdef OWCAT_USE_GTK
    return pImpl->dialog;
#else
    return nullptr;
#endif
}

GtkWindow* GtkSettingsDialog::getWindow() const {
#ifdef OWCAT_USE_GTK
    return GTK_WINDOW(pImpl->dialog);
#else
    return nullptr;
#endif
}

// Private helper methods
void GtkSettingsDialog::createContent() {
#ifdef OWCAT_USE_GTK
    GtkWidget* contentArea = gtk_dialog_get_content_area(GTK_DIALOG(pImpl->dialog));
    
    // Create notebook
    pImpl->notebook = gtk_notebook_new();
    gtk_notebook_set_tab_pos(GTK_NOTEBOOK(pImpl->notebook), GTK_POS_TOP);
    gtk_container_set_border_width(GTK_CONTAINER(pImpl->notebook), 10);
    gtk_box_pack_start(GTK_BOX(contentArea), pImpl->notebook, TRUE, TRUE, 0);
    
    // Create pages
    createGeneralPage();
    createInputPage();
    createAppearancePage();
    createAdvancedPage();
    createHotkeysPage();
#endif
}

void GtkSettingsDialog::createGeneralPage() {
#ifdef OWCAT_USE_GTK
    pImpl->generalPage = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(pImpl->generalPage), 10);
    gtk_grid_set_column_spacing(GTK_GRID(pImpl->generalPage), 10);
    gtk_container_set_border_width(GTK_CONTAINER(pImpl->generalPage), 10);
    
    int row = 0;
    
    // Enable IME
    pImpl->enabledCheckButton = gtk_check_button_new_with_label("Enable OwCat IME");
    gtk_grid_attach(GTK_GRID(pImpl->generalPage), pImpl->enabledCheckButton, 0, row++, 2, 1);
    
    // Auto start
    pImpl->autoStartCheckButton = gtk_check_button_new_with_label("Start with system");
    gtk_grid_attach(GTK_GRID(pImpl->generalPage), pImpl->autoStartCheckButton, 0, row++, 2, 1);
    
    // Show notifications
    pImpl->showNotificationsCheckButton = gtk_check_button_new_with_label("Show notifications");
    gtk_grid_attach(GTK_GRID(pImpl->generalPage), pImpl->showNotificationsCheckButton, 0, row++, 2, 1);
    
    // Language
    GtkWidget* languageLabel = gtk_label_new("Language:");
    gtk_widget_set_halign(languageLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(pImpl->generalPage), languageLabel, 0, row, 1, 1);
    
    pImpl->languageComboBox = gtk_combo_box_text_new();
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(pImpl->languageComboBox), "zh_CN", "简体中文");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(pImpl->languageComboBox), "zh_TW", "繁體中文");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(pImpl->languageComboBox), "en_US", "English");
    gtk_grid_attach(GTK_GRID(pImpl->generalPage), pImpl->languageComboBox, 1, row++, 1, 1);
    
    // Theme
    GtkWidget* themeLabel = gtk_label_new("Theme:");
    gtk_widget_set_halign(themeLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(pImpl->generalPage), themeLabel, 0, row, 1, 1);
    
    pImpl->themeComboBox = gtk_combo_box_text_new();
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(pImpl->themeComboBox), "default", "Default");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(pImpl->themeComboBox), "dark", "Dark");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(pImpl->themeComboBox), "light", "Light");
    gtk_grid_attach(GTK_GRID(pImpl->generalPage), pImpl->themeComboBox, 1, row++, 1, 1);
    
    // Add tab
    GtkWidget* generalLabel = gtk_label_new("General");
    gtk_notebook_append_page(GTK_NOTEBOOK(pImpl->notebook), pImpl->generalPage, generalLabel);
#endif
}

void GtkSettingsDialog::createInputPage() {
#ifdef OWCAT_USE_GTK
    pImpl->inputPage = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(pImpl->inputPage), 10);
    gtk_grid_set_column_spacing(GTK_GRID(pImpl->inputPage), 10);
    gtk_container_set_border_width(GTK_CONTAINER(pImpl->inputPage), 10);
    
    int row = 0;
    
    // Pinyin mode
    GtkWidget* pinyinModeLabel = gtk_label_new("Pinyin Mode:");
    gtk_widget_set_halign(pinyinModeLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(pImpl->inputPage), pinyinModeLabel, 0, row, 1, 1);
    
    pImpl->pinyinModeComboBox = gtk_combo_box_text_new();
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(pImpl->pinyinModeComboBox), "full", "Full Pinyin");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(pImpl->pinyinModeComboBox), "double", "Double Pinyin");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(pImpl->pinyinModeComboBox), "fuzzy", "Fuzzy Pinyin");
    gtk_grid_attach(GTK_GRID(pImpl->inputPage), pImpl->pinyinModeComboBox, 1, row++, 1, 1);
    
    // Fuzzy pinyin
    pImpl->fuzzyPinyinCheckButton = gtk_check_button_new_with_label("Enable fuzzy pinyin");
    gtk_grid_attach(GTK_GRID(pImpl->inputPage), pImpl->fuzzyPinyinCheckButton, 0, row++, 2, 1);
    
    // Auto commit
    pImpl->autoCommitCheckButton = gtk_check_button_new_with_label("Auto commit single candidate");
    gtk_grid_attach(GTK_GRID(pImpl->inputPage), pImpl->autoCommitCheckButton, 0, row++, 2, 1);
    
    // Page up key
    GtkWidget* pageUpKeyLabel = gtk_label_new("Page Up Key:");
    gtk_widget_set_halign(pageUpKeyLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(pImpl->inputPage), pageUpKeyLabel, 0, row, 1, 1);
    
    pImpl->pageUpKeyEntry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(pImpl->pageUpKeyEntry), "minus");
    gtk_grid_attach(GTK_GRID(pImpl->inputPage), pImpl->pageUpKeyEntry, 1, row++, 1, 1);
    
    // Page down key
    GtkWidget* pageDownKeyLabel = gtk_label_new("Page Down Key:");
    gtk_widget_set_halign(pageDownKeyLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(pImpl->inputPage), pageDownKeyLabel, 0, row, 1, 1);
    
    pImpl->pageDownKeyEntry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(pImpl->pageDownKeyEntry), "equal");
    gtk_grid_attach(GTK_GRID(pImpl->inputPage), pImpl->pageDownKeyEntry, 1, row++, 1, 1);
    
    // Candidate keys
    GtkWidget* candidateKeysLabel = gtk_label_new("Candidate Keys:");
    gtk_widget_set_halign(candidateKeysLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(pImpl->inputPage), candidateKeysLabel, 0, row, 1, 1);
    
    pImpl->candidateKeysEntry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(pImpl->candidateKeysEntry), "1234567890");
    gtk_grid_attach(GTK_GRID(pImpl->inputPage), pImpl->candidateKeysEntry, 1, row++, 1, 1);
    
    // Add tab
    GtkWidget* inputLabel = gtk_label_new("Input");
    gtk_notebook_append_page(GTK_NOTEBOOK(pImpl->notebook), pImpl->inputPage, inputLabel);
#endif
}

void GtkSettingsDialog::createAppearancePage() {
#ifdef OWCAT_USE_GTK
    pImpl->appearancePage = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(pImpl->appearancePage), 10);
    gtk_grid_set_column_spacing(GTK_GRID(pImpl->appearancePage), 10);
    gtk_container_set_border_width(GTK_CONTAINER(pImpl->appearancePage), 10);
    
    int row = 0;
    
    // Font
    GtkWidget* fontLabel = gtk_label_new("Font:");
    gtk_widget_set_halign(fontLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(pImpl->appearancePage), fontLabel, 0, row, 1, 1);
    
    pImpl->fontButton = gtk_font_button_new();
    gtk_font_button_set_font_name(GTK_FONT_BUTTON(pImpl->fontButton), "Sans 12");
    gtk_grid_attach(GTK_GRID(pImpl->appearancePage), pImpl->fontButton, 1, row++, 1, 1);
    
    // Candidate window opacity
    GtkWidget* opacityLabel = gtk_label_new("Window Opacity:");
    gtk_widget_set_halign(opacityLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(pImpl->appearancePage), opacityLabel, 0, row, 1, 1);
    
    pImpl->candidateWindowOpacityScale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.1, 1.0, 0.1);
    gtk_scale_set_value(GTK_SCALE(pImpl->candidateWindowOpacityScale), 0.9);
    gtk_scale_set_digits(GTK_SCALE(pImpl->candidateWindowOpacityScale), 1);
    gtk_grid_attach(GTK_GRID(pImpl->appearancePage), pImpl->candidateWindowOpacityScale, 1, row++, 1, 1);
    
    // Show candidate numbers
    pImpl->showCandidateNumbersCheckButton = gtk_check_button_new_with_label("Show candidate numbers");
    gtk_grid_attach(GTK_GRID(pImpl->appearancePage), pImpl->showCandidateNumbersCheckButton, 0, row++, 2, 1);
    
    // Vertical candidate list
    pImpl->verticalCandidateListCheckButton = gtk_check_button_new_with_label("Vertical candidate list");
    gtk_grid_attach(GTK_GRID(pImpl->appearancePage), pImpl->verticalCandidateListCheckButton, 0, row++, 2, 1);
    
    // Candidate window color
    GtkWidget* candidateWindowColorLabel = gtk_label_new("Window Color:");
    gtk_widget_set_halign(candidateWindowColorLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(pImpl->appearancePage), candidateWindowColorLabel, 0, row, 1, 1);
    
    pImpl->candidateWindowColorButton = gtk_color_button_new();
    GdkRGBA white = {1.0, 1.0, 1.0, 1.0};
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(pImpl->candidateWindowColorButton), &white);
    gtk_grid_attach(GTK_GRID(pImpl->appearancePage), pImpl->candidateWindowColorButton, 1, row++, 1, 1);
    
    // Selected candidate color
    GtkWidget* selectedCandidateColorLabel = gtk_label_new("Selection Color:");
    gtk_widget_set_halign(selectedCandidateColorLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(pImpl->appearancePage), selectedCandidateColorLabel, 0, row, 1, 1);
    
    pImpl->selectedCandidateColorButton = gtk_color_button_new();
    GdkRGBA blue = {0.21, 0.52, 0.89, 1.0};
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(pImpl->selectedCandidateColorButton), &blue);
    gtk_grid_attach(GTK_GRID(pImpl->appearancePage), pImpl->selectedCandidateColorButton, 1, row++, 1, 1);
    
    // Add tab
    GtkWidget* appearanceLabel = gtk_label_new("Appearance");
    gtk_notebook_append_page(GTK_NOTEBOOK(pImpl->notebook), pImpl->appearancePage, appearanceLabel);
#endif
}

void GtkSettingsDialog::createAdvancedPage() {
#ifdef OWCAT_USE_GTK
    pImpl->advancedPage = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(pImpl->advancedPage), 10);
    gtk_grid_set_column_spacing(GTK_GRID(pImpl->advancedPage), 10);
    gtk_container_set_border_width(GTK_CONTAINER(pImpl->advancedPage), 10);
    
    int row = 0;
    
    // Dictionary path
    GtkWidget* dictionaryPathLabel = gtk_label_new("Dictionary Path:");
    gtk_widget_set_halign(dictionaryPathLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(pImpl->advancedPage), dictionaryPathLabel, 0, row, 1, 1);
    
    GtkWidget* dictionaryBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    pImpl->dictionaryPathEntry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(dictionaryBox), pImpl->dictionaryPathEntry, TRUE, TRUE, 0);
    
    pImpl->browseDictionaryButton = gtk_button_new_with_label("Browse...");
    gtk_box_pack_start(GTK_BOX(dictionaryBox), pImpl->browseDictionaryButton, FALSE, FALSE, 0);
    gtk_grid_attach(GTK_GRID(pImpl->advancedPage), dictionaryBox, 1, row++, 1, 1);
    
    // User dictionary path
    GtkWidget* userDictionaryPathLabel = gtk_label_new("User Dictionary Path:");
    gtk_widget_set_halign(userDictionaryPathLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(pImpl->advancedPage), userDictionaryPathLabel, 0, row, 1, 1);
    
    GtkWidget* userDictionaryBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    pImpl->userDictionaryPathEntry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(userDictionaryBox), pImpl->userDictionaryPathEntry, TRUE, TRUE, 0);
    
    pImpl->browseUserDictionaryButton = gtk_button_new_with_label("Browse...");
    gtk_box_pack_start(GTK_BOX(userDictionaryBox), pImpl->browseUserDictionaryButton, FALSE, FALSE, 0);
    gtk_grid_attach(GTK_GRID(pImpl->advancedPage), userDictionaryBox, 1, row++, 1, 1);
    
    // Log level
    GtkWidget* logLevelLabel = gtk_label_new("Log Level:");
    gtk_widget_set_halign(logLevelLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(pImpl->advancedPage), logLevelLabel, 0, row, 1, 1);
    
    pImpl->logLevelComboBox = gtk_combo_box_text_new();
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(pImpl->logLevelComboBox), "debug", "Debug");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(pImpl->logLevelComboBox), "info", "Info");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(pImpl->logLevelComboBox), "warning", "Warning");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(pImpl->logLevelComboBox), "error", "Error");
    gtk_grid_attach(GTK_GRID(pImpl->advancedPage), pImpl->logLevelComboBox, 1, row++, 1, 1);
    
    // Enable logging
    pImpl->enableLoggingCheckButton = gtk_check_button_new_with_label("Enable logging");
    gtk_grid_attach(GTK_GRID(pImpl->advancedPage), pImpl->enableLoggingCheckButton, 0, row++, 2, 1);
    
    // Max history
    GtkWidget* maxHistoryLabel = gtk_label_new("Max History Items:");
    gtk_widget_set_halign(maxHistoryLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(pImpl->advancedPage), maxHistoryLabel, 0, row, 1, 1);
    
    pImpl->maxHistorySpinButton = gtk_spin_button_new_with_range(100, 10000, 100);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(pImpl->maxHistorySpinButton), 1000);
    gtk_grid_attach(GTK_GRID(pImpl->advancedPage), pImpl->maxHistorySpinButton, 1, row++, 1, 1);
    
    // Add tab
    GtkWidget* advancedLabel = gtk_label_new("Advanced");
    gtk_notebook_append_page(GTK_NOTEBOOK(pImpl->notebook), pImpl->advancedPage, advancedLabel);
#endif
}

void GtkSettingsDialog::createHotkeysPage() {
#ifdef OWCAT_USE_GTK
    pImpl->hotkeysPage = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(pImpl->hotkeysPage), 10);
    gtk_grid_set_column_spacing(GTK_GRID(pImpl->hotkeysPage), 10);
    gtk_container_set_border_width(GTK_CONTAINER(pImpl->hotkeysPage), 10);
    
    int row = 0;
    
    // Toggle key
    GtkWidget* toggleKeyLabel = gtk_label_new("Toggle IME:");
    gtk_widget_set_halign(toggleKeyLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(pImpl->hotkeysPage), toggleKeyLabel, 0, row, 1, 1);
    
    pImpl->toggleKeyEntry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(pImpl->toggleKeyEntry), "ctrl+space");
    gtk_grid_attach(GTK_GRID(pImpl->hotkeysPage), pImpl->toggleKeyEntry, 1, row++, 1, 1);
    
    // Switch mode key
    GtkWidget* switchModeKeyLabel = gtk_label_new("Switch Mode:");
    gtk_widget_set_halign(switchModeKeyLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(pImpl->hotkeysPage), switchModeKeyLabel, 0, row, 1, 1);
    
    pImpl->switchModeKeyEntry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(pImpl->switchModeKeyEntry), "shift");
    gtk_grid_attach(GTK_GRID(pImpl->hotkeysPage), pImpl->switchModeKeyEntry, 1, row++, 1, 1);
    
    // Temporary English key
    GtkWidget* temporaryEnglishKeyLabel = gtk_label_new("Temporary English:");
    gtk_widget_set_halign(temporaryEnglishKeyLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(pImpl->hotkeysPage), temporaryEnglishKeyLabel, 0, row, 1, 1);
    
    pImpl->temporaryEnglishKeyEntry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(pImpl->temporaryEnglishKeyEntry), "shift");
    gtk_grid_attach(GTK_GRID(pImpl->hotkeysPage), pImpl->temporaryEnglishKeyEntry, 1, row++, 1, 1);
    
    // Full width key
    GtkWidget* fullWidthKeyLabel = gtk_label_new("Full Width:");
    gtk_widget_set_halign(fullWidthKeyLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(pImpl->hotkeysPage), fullWidthKeyLabel, 0, row, 1, 1);
    
    pImpl->fullWidthKeyEntry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(pImpl->fullWidthKeyEntry), "shift+space");
    gtk_grid_attach(GTK_GRID(pImpl->hotkeysPage), pImpl->fullWidthKeyEntry, 1, row++, 1, 1);
    
    // Punctuation key
    GtkWidget* punctuationKeyLabel = gtk_label_new("Punctuation:");
    gtk_widget_set_halign(punctuationKeyLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(pImpl->hotkeysPage), punctuationKeyLabel, 0, row, 1, 1);
    
    pImpl->punctuationKeyEntry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(pImpl->punctuationKeyEntry), "ctrl+period");
    gtk_grid_attach(GTK_GRID(pImpl->hotkeysPage), pImpl->punctuationKeyEntry, 1, row++, 1, 1);
    
    // Add tab
    GtkWidget* hotkeysLabel = gtk_label_new("Hotkeys");
    gtk_notebook_append_page(GTK_NOTEBOOK(pImpl->notebook), pImpl->hotkeysPage, hotkeysLabel);
#endif
}

void GtkSettingsDialog::createButtons() {
#ifdef OWCAT_USE_GTK
    // Create button box
    pImpl->buttonBox = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(pImpl->buttonBox), GTK_BUTTONBOX_END);
    gtk_box_set_spacing(GTK_BOX(pImpl->buttonBox), 5);
    gtk_container_set_border_width(GTK_CONTAINER(pImpl->buttonBox), 10);
    
    // Reset button
    pImpl->resetButton = gtk_button_new_with_label("Reset to Defaults");
    gtk_box_pack_start(GTK_BOX(pImpl->buttonBox), pImpl->resetButton, FALSE, FALSE, 0);
    
    // Cancel button
    pImpl->cancelButton = gtk_button_new_with_label("Cancel");
    gtk_box_pack_end(GTK_BOX(pImpl->buttonBox), pImpl->cancelButton, FALSE, FALSE, 0);
    
    // Apply button
    pImpl->applyButton = gtk_button_new_with_label("Apply");
    gtk_box_pack_end(GTK_BOX(pImpl->buttonBox), pImpl->applyButton, FALSE, FALSE, 0);
    
    // OK button
    pImpl->okButton = gtk_button_new_with_label("OK");
    gtk_box_pack_end(GTK_BOX(pImpl->buttonBox), pImpl->okButton, FALSE, FALSE, 0);
    
    // Add to dialog
    GtkWidget* contentArea = gtk_dialog_get_content_area(GTK_DIALOG(pImpl->dialog));
    gtk_box_pack_end(GTK_BOX(contentArea), pImpl->buttonBox, FALSE, FALSE, 0);
#endif
}

void GtkSettingsDialog::connectSignals() {
#ifdef OWCAT_USE_GTK
    // OK button
    g_signal_connect(pImpl->okButton, "clicked", G_CALLBACK(+[](GtkButton* button, gpointer userData) {
        GtkSettingsDialog* dialog = static_cast<GtkSettingsDialog*>(userData);
        dialog->saveSettings();
        if (dialog->pImpl->onSettingsChanged) {
            dialog->pImpl->onSettingsChanged(dialog->pImpl->currentSettings);
        }
        dialog->hide();
    }), this);
    
    // Cancel button
    g_signal_connect(pImpl->cancelButton, "clicked", G_CALLBACK(+[](GtkButton* button, gpointer userData) {
        GtkSettingsDialog* dialog = static_cast<GtkSettingsDialog*>(userData);
        // Restore original settings
        dialog->pImpl->currentSettings = dialog->pImpl->originalSettings;
        dialog->hide();
    }), this);
    
    // Apply button
    g_signal_connect(pImpl->applyButton, "clicked", G_CALLBACK(+[](GtkButton* button, gpointer userData) {
        GtkSettingsDialog* dialog = static_cast<GtkSettingsDialog*>(userData);
        dialog->saveSettings();
        if (dialog->pImpl->onSettingsChanged) {
            dialog->pImpl->onSettingsChanged(dialog->pImpl->currentSettings);
        }
    }), this);
    
    // Reset button
    g_signal_connect(pImpl->resetButton, "clicked", G_CALLBACK(+[](GtkButton* button, gpointer userData) {
        GtkSettingsDialog* dialog = static_cast<GtkSettingsDialog*>(userData);
        dialog->resetToDefaults();
    }), this);
    
    // Browse dictionary button
    g_signal_connect(pImpl->browseDictionaryButton, "clicked", G_CALLBACK(+[](GtkButton* button, gpointer userData) {
        GtkSettingsDialog* dialog = static_cast<GtkSettingsDialog*>(userData);
        dialog->browseDictionaryPath();
    }), this);
    
    // Browse user dictionary button
    g_signal_connect(pImpl->browseUserDictionaryButton, "clicked", G_CALLBACK(+[](GtkButton* button, gpointer userData) {
        GtkSettingsDialog* dialog = static_cast<GtkSettingsDialog*>(userData);
        dialog->browseUserDictionaryPath();
    }), this);
    
    // Dialog close signal
    g_signal_connect(pImpl->dialog, "delete-event", G_CALLBACK(+[](GtkWidget* widget, GdkEvent* event, gpointer userData) -> gboolean {
        GtkSettingsDialog* dialog = static_cast<GtkSettingsDialog*>(userData);
        dialog->hide();
        if (dialog->pImpl->onDialogClosed) {
            dialog->pImpl->onDialogClosed();
        }
        return TRUE; // Prevent default close
    }), this);
#endif
}

void GtkSettingsDialog::loadSettings() {
#ifdef OWCAT_USE_GTK
    // Load general settings
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pImpl->enabledCheckButton), 
                                pImpl->currentSettings["enabled"] == "true");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pImpl->autoStartCheckButton), 
                                pImpl->currentSettings["auto_start"] == "true");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pImpl->showNotificationsCheckButton), 
                                pImpl->currentSettings["show_notifications"] == "true");
    
    gtk_combo_box_set_active_id(GTK_COMBO_BOX(pImpl->languageComboBox), 
                                pImpl->currentSettings["language"].c_str());
    gtk_combo_box_set_active_id(GTK_COMBO_BOX(pImpl->themeComboBox), 
                                pImpl->currentSettings["theme"].c_str());
    
    // Load input settings
    gtk_combo_box_set_active_id(GTK_COMBO_BOX(pImpl->pinyinModeComboBox), 
                                pImpl->currentSettings["pinyin_mode"].c_str());
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pImpl->fuzzyPinyinCheckButton), 
                                pImpl->currentSettings["fuzzy_pinyin"] == "true");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pImpl->autoCommitCheckButton), 
                                pImpl->currentSettings["auto_commit"] == "true");
    
    gtk_entry_set_text(GTK_ENTRY(pImpl->pageUpKeyEntry), 
                      pImpl->currentSettings["page_up_key"].c_str());
    gtk_entry_set_text(GTK_ENTRY(pImpl->pageDownKeyEntry), 
                      pImpl->currentSettings["page_down_key"].c_str());
    gtk_entry_set_text(GTK_ENTRY(pImpl->candidateKeysEntry), 
                      pImpl->currentSettings["candidate_keys"].c_str());
    
    // Load appearance settings
    gtk_font_button_set_font_name(GTK_FONT_BUTTON(pImpl->fontButton), 
                                 pImpl->currentSettings["font"].c_str());
    
    double opacity = std::stod(pImpl->currentSettings["candidate_window_opacity"]);
    gtk_range_set_value(GTK_RANGE(pImpl->candidateWindowOpacityScale), opacity);
    
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pImpl->showCandidateNumbersCheckButton), 
                                pImpl->currentSettings["show_candidate_numbers"] == "true");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pImpl->verticalCandidateListCheckButton), 
                                pImpl->currentSettings["vertical_candidate_list"] == "true");
    
    // Load advanced settings
    gtk_entry_set_text(GTK_ENTRY(pImpl->dictionaryPathEntry), 
                      pImpl->currentSettings["dictionary_path"].c_str());
    gtk_entry_set_text(GTK_ENTRY(pImpl->userDictionaryPathEntry), 
                      pImpl->currentSettings["user_dictionary_path"].c_str());
    
    gtk_combo_box_set_active_id(GTK_COMBO_BOX(pImpl->logLevelComboBox), 
                                pImpl->currentSettings["log_level"].c_str());
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pImpl->enableLoggingCheckButton), 
                                pImpl->currentSettings["enable_logging"] == "true");
    
    int maxHistory = std::stoi(pImpl->currentSettings["max_history"]);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(pImpl->maxHistorySpinButton), maxHistory);
    
    // Load hotkey settings
    gtk_entry_set_text(GTK_ENTRY(pImpl->toggleKeyEntry), 
                      pImpl->currentSettings["toggle_key"].c_str());
    gtk_entry_set_text(GTK_ENTRY(pImpl->switchModeKeyEntry), 
                      pImpl->currentSettings["switch_mode_key"].c_str());
    gtk_entry_set_text(GTK_ENTRY(pImpl->temporaryEnglishKeyEntry), 
                      pImpl->currentSettings["temporary_english_key"].c_str());
    gtk_entry_set_text(GTK_ENTRY(pImpl->fullWidthKeyEntry), 
                      pImpl->currentSettings["full_width_key"].c_str());
    gtk_entry_set_text(GTK_ENTRY(pImpl->punctuationKeyEntry), 
                      pImpl->currentSettings["punctuation_key"].c_str());
#endif
}

void GtkSettingsDialog::saveSettings() {
#ifdef OWCAT_USE_GTK
    // Save general settings
    pImpl->currentSettings["enabled"] = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pImpl->enabledCheckButton)) ? "true" : "false";
    pImpl->currentSettings["auto_start"] = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pImpl->autoStartCheckButton)) ? "true" : "false";
    pImpl->currentSettings["show_notifications"] = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pImpl->showNotificationsCheckButton)) ? "true" : "false";
    
    gchar* language = gtk_combo_box_get_active_id(GTK_COMBO_BOX(pImpl->languageComboBox));
    if (language) {
        pImpl->currentSettings["language"] = language;
        g_free(language);
    }
    
    gchar* theme = gtk_combo_box_get_active_id(GTK_COMBO_BOX(pImpl->themeComboBox));
    if (theme) {
        pImpl->currentSettings["theme"] = theme;
        g_free(theme);
    }
    
    // Save input settings
    gchar* pinyinMode = gtk_combo_box_get_active_id(GTK_COMBO_BOX(pImpl->pinyinModeComboBox));
    if (pinyinMode) {
        pImpl->currentSettings["pinyin_mode"] = pinyinMode;
        g_free(pinyinMode);
    }
    
    pImpl->currentSettings["fuzzy_pinyin"] = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pImpl->fuzzyPinyinCheckButton)) ? "true" : "false";
    pImpl->currentSettings["auto_commit"] = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pImpl->autoCommitCheckButton)) ? "true" : "false";
    
    pImpl->currentSettings["page_up_key"] = gtk_entry_get_text(GTK_ENTRY(pImpl->pageUpKeyEntry));
    pImpl->currentSettings["page_down_key"] = gtk_entry_get_text(GTK_ENTRY(pImpl->pageDownKeyEntry));
    pImpl->currentSettings["candidate_keys"] = gtk_entry_get_text(GTK_ENTRY(pImpl->candidateKeysEntry));
    
    // Save appearance settings
    pImpl->currentSettings["font"] = gtk_font_button_get_font_name(GTK_FONT_BUTTON(pImpl->fontButton));
    
    double opacity = gtk_range_get_value(GTK_RANGE(pImpl->candidateWindowOpacityScale));
    pImpl->currentSettings["candidate_window_opacity"] = std::to_string(opacity);
    
    pImpl->currentSettings["show_candidate_numbers"] = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pImpl->showCandidateNumbersCheckButton)) ? "true" : "false";
    pImpl->currentSettings["vertical_candidate_list"] = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pImpl->verticalCandidateListCheckButton)) ? "true" : "false";
    
    // Save advanced settings
    pImpl->currentSettings["dictionary_path"] = gtk_entry_get_text(GTK_ENTRY(pImpl->dictionaryPathEntry));
    pImpl->currentSettings["user_dictionary_path"] = gtk_entry_get_text(GTK_ENTRY(pImpl->userDictionaryPathEntry));
    
    gchar* logLevel = gtk_combo_box_get_active_id(GTK_COMBO_BOX(pImpl->logLevelComboBox));
    if (logLevel) {
        pImpl->currentSettings["log_level"] = logLevel;
        g_free(logLevel);
    }
    
    pImpl->currentSettings["enable_logging"] = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pImpl->enableLoggingCheckButton)) ? "true" : "false";
    
    int maxHistory = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(pImpl->maxHistorySpinButton));
    pImpl->currentSettings["max_history"] = std::to_string(maxHistory);
    
    // Save hotkey settings
    pImpl->currentSettings["toggle_key"] = gtk_entry_get_text(GTK_ENTRY(pImpl->toggleKeyEntry));
    pImpl->currentSettings["switch_mode_key"] = gtk_entry_get_text(GTK_ENTRY(pImpl->switchModeKeyEntry));
    pImpl->currentSettings["temporary_english_key"] = gtk_entry_get_text(GTK_ENTRY(pImpl->temporaryEnglishKeyEntry));
    pImpl->currentSettings["full_width_key"] = gtk_entry_get_text(GTK_ENTRY(pImpl->fullWidthKeyEntry));
    pImpl->currentSettings["punctuation_key"] = gtk_entry_get_text(GTK_ENTRY(pImpl->punctuationKeyEntry));
#endif
}

void GtkSettingsDialog::browseDictionaryPath() {
#ifdef OWCAT_USE_GTK
    GtkWidget* fileChooser = gtk_file_chooser_dialog_new("Select Dictionary File",
                                                         GTK_WINDOW(pImpl->dialog),
                                                         GTK_FILE_CHOOSER_ACTION_OPEN,
                                                         "Cancel", GTK_RESPONSE_CANCEL,
                                                         "Open", GTK_RESPONSE_ACCEPT,
                                                         nullptr);
    
    // Add file filters
    GtkFileFilter* filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Dictionary Files");
    gtk_file_filter_add_pattern(filter, "*.dict");
    gtk_file_filter_add_pattern(filter, "*.txt");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(fileChooser), filter);
    
    if (gtk_dialog_run(GTK_DIALOG(fileChooser)) == GTK_RESPONSE_ACCEPT) {
        gchar* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fileChooser));
        if (filename) {
            gtk_entry_set_text(GTK_ENTRY(pImpl->dictionaryPathEntry), filename);
            g_free(filename);
        }
    }
    
    gtk_widget_destroy(fileChooser);
#endif
}

void GtkSettingsDialog::browseUserDictionaryPath() {
#ifdef OWCAT_USE_GTK
    GtkWidget* fileChooser = gtk_file_chooser_dialog_new("Select User Dictionary File",
                                                         GTK_WINDOW(pImpl->dialog),
                                                         GTK_FILE_CHOOSER_ACTION_SAVE,
                                                         "Cancel", GTK_RESPONSE_CANCEL,
                                                         "Save", GTK_RESPONSE_ACCEPT,
                                                         nullptr);
    
    // Add file filters
    GtkFileFilter* filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Dictionary Files");
    gtk_file_filter_add_pattern(filter, "*.dict");
    gtk_file_filter_add_pattern(filter, "*.txt");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(fileChooser), filter);
    
    if (gtk_dialog_run(GTK_DIALOG(fileChooser)) == GTK_RESPONSE_ACCEPT) {
        gchar* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fileChooser));
        if (filename) {
            gtk_entry_set_text(GTK_ENTRY(pImpl->userDictionaryPathEntry), filename);
            g_free(filename);
        }
    }
    
    gtk_widget_destroy(fileChooser);
#endif
}

} // namespace owcat