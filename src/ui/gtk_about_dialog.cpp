#include "ui/gtk_ui.h"
#include <memory>
#include <string>
#include <iostream>

#ifdef OWCAT_USE_GTK
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <glib.h>
#endif

namespace owcat {

// Implementation structure for GtkAboutDialog
struct GtkAboutDialog::Impl {
#ifdef OWCAT_USE_GTK
    GtkWidget* dialog;
    GtkWidget* contentArea;
    GtkWidget* headerBox;
    GtkWidget* logoImage;
    GtkWidget* titleLabel;
    GtkWidget* versionLabel;
    GtkWidget* descriptionLabel;
    GtkWidget* infoNotebook;
    GtkWidget* aboutPage;
    GtkWidget* authorsPage;
    GtkWidget* licenseePage;
    GtkWidget* creditsPage;
    GtkWidget* buttonBox;
    GtkWidget* closeButton;
    GtkWidget* websiteButton;
    GtkWidget* donateButton;
#endif
    
    // State
    bool isVisible;
    std::string appName;
    std::string version;
    std::string description;
    std::string website;
    std::string copyright;
    std::string license;
    std::vector<std::string> authors;
    std::vector<std::string> credits;
    
    // Callbacks
    std::function<void()> onDialogClosed;
    std::function<void(const std::string&)> onWebsiteClicked;
    std::function<void()> onDonateClicked;
    
    Impl() {
#ifdef OWCAT_USE_GTK
        dialog = nullptr;
        contentArea = nullptr;
        headerBox = nullptr;
        logoImage = nullptr;
        titleLabel = nullptr;
        versionLabel = nullptr;
        descriptionLabel = nullptr;
        infoNotebook = nullptr;
        aboutPage = nullptr;
        authorsPage = nullptr;
        licenseePage = nullptr;
        creditsPage = nullptr;
        buttonBox = nullptr;
        closeButton = nullptr;
        websiteButton = nullptr;
        donateButton = nullptr;
#endif
        
        isVisible = false;
        appName = "OwCat";
        version = "1.0.0";
        description = "A modern Chinese input method powered by AI";
        website = "https://github.com/owcat/owcat";
        copyright = "Copyright © 2024 OwCat Team";
        license = "MIT License";
        
        authors = {
            "OwCat Team",
            "Contributors from the community"
        };
        
        credits = {
            "llama.cpp - LLM inference library",
            "Qwen - Language model",
            "GTK+ - GUI toolkit",
            "IBus - Input Bus framework",
            "vcpkg - Package manager"
        };
    }
};

GtkAboutDialog::GtkAboutDialog() : pImpl(std::make_unique<Impl>()) {}

GtkAboutDialog::~GtkAboutDialog() = default;

bool GtkAboutDialog::create(GtkWindow* parent) {
#ifdef OWCAT_USE_GTK
    if (pImpl->dialog) {
        return true; // Already created
    }
    
    // Create dialog
    pImpl->dialog = gtk_dialog_new();
    if (!pImpl->dialog) {
        std::cerr << "Failed to create about dialog" << std::endl;
        return false;
    }
    
    // Set dialog properties
    gtk_window_set_title(GTK_WINDOW(pImpl->dialog), "About OwCat");
    gtk_window_set_default_size(GTK_WINDOW(pImpl->dialog), 500, 400);
    gtk_window_set_modal(GTK_WINDOW(pImpl->dialog), TRUE);
    gtk_window_set_resizable(GTK_WINDOW(pImpl->dialog), FALSE);
    
    if (parent) {
        gtk_window_set_transient_for(GTK_WINDOW(pImpl->dialog), parent);
        // GTK4: gtk_window_set_position is deprecated, window manager handles positioning
    } else {
        // GTK4: gtk_window_set_position is deprecated, window manager handles positioning
    }
    
    // Create content
    createContent();
    
    // Create buttons
    createButtons();
    
    // Connect signals
    connectSignals();
    
    std::cout << "About dialog created successfully" << std::endl;
    return true;
#else
    return false;
#endif
}

void GtkAboutDialog::destroy() {
#ifdef OWCAT_USE_GTK
    if (pImpl->dialog) {
        // GTK4: Use gtk_window_destroy instead of gtk_widget_destroy
        gtk_window_destroy(GTK_WINDOW(pImpl->dialog));
        pImpl->dialog = nullptr;
    }
#endif
}

bool GtkAboutDialog::show() {
#ifdef OWCAT_USE_GTK
    if (pImpl->dialog) {
        // GTK4: gtk_widget_show_all is deprecated, use gtk_widget_show
        gtk_widget_show(pImpl->dialog);
        pImpl->isVisible = true;
        return true;
    }
#endif
    return false;
}

bool GtkAboutDialog::hide() {
#ifdef OWCAT_USE_GTK
    if (pImpl->dialog) {
        gtk_widget_hide(pImpl->dialog);
        pImpl->isVisible = false;
        return true;
    }
#endif
    return false;
}

bool GtkAboutDialog::isVisible() const {
    return pImpl->isVisible;
}

// Information setters
void GtkAboutDialog::setAppName(const std::string& name) {
    pImpl->appName = name;
    updateContent();
}

void GtkAboutDialog::setVersion(const std::string& version) {
    pImpl->version = version;
    updateContent();
}

void GtkAboutDialog::setDescription(const std::string& description) {
    pImpl->description = description;
    updateContent();
}

void GtkAboutDialog::setWebsite(const std::string& website) {
    pImpl->website = website;
}

void GtkAboutDialog::setCopyright(const std::string& copyright) {
    pImpl->copyright = copyright;
    updateContent();
}

void GtkAboutDialog::setLicense(const std::string& license) {
    pImpl->license = license;
    updateContent();
}

void GtkAboutDialog::setAuthors(const std::vector<std::string>& authors) {
    pImpl->authors = authors;
    updateContent();
}

void GtkAboutDialog::setCredits(const std::vector<std::string>& credits) {
    pImpl->credits = credits;
    updateContent();
}

// Information getters
std::string GtkAboutDialog::getAppName() const {
    return pImpl->appName;
}

std::string GtkAboutDialog::getVersion() const {
    return pImpl->version;
}

std::string GtkAboutDialog::getDescription() const {
    return pImpl->description;
}

std::string GtkAboutDialog::getWebsite() const {
    return pImpl->website;
}

std::string GtkAboutDialog::getCopyright() const {
    return pImpl->copyright;
}

std::string GtkAboutDialog::getLicense() const {
    return pImpl->license;
}

std::vector<std::string> GtkAboutDialog::getAuthors() const {
    return pImpl->authors;
}

std::vector<std::string> GtkAboutDialog::getCredits() const {
    return pImpl->credits;
}

// Callbacks
void GtkAboutDialog::setOnDialogClosed(std::function<void()> callback) {
    pImpl->onDialogClosed = callback;
}

void GtkAboutDialog::setOnWebsiteClicked(std::function<void(const std::string&)> callback) {
    pImpl->onWebsiteClicked = callback;
}

void GtkAboutDialog::setOnDonateClicked(std::function<void()> callback) {
    pImpl->onDonateClicked = callback;
}

// Widget access
GtkWidget* GtkAboutDialog::getWidget() const {
#ifdef OWCAT_USE_GTK
    return pImpl->dialog;
#else
    return nullptr;
#endif
}

GtkWindow* GtkAboutDialog::getWindow() const {
#ifdef OWCAT_USE_GTK
    return GTK_WINDOW(pImpl->dialog);
#else
    return nullptr;
#endif
}

// Private helper methods
void GtkAboutDialog::createContent() {
#ifdef OWCAT_USE_GTK
    pImpl->contentArea = gtk_dialog_get_content_area(GTK_DIALOG(pImpl->dialog));
    gtk_container_set_border_width(GTK_CONTAINER(pImpl->contentArea), 0);
    
    // Create header
    createHeader();
    
    // Create info notebook
    createInfoNotebook();
#endif
}

void GtkAboutDialog::createHeader() {
#ifdef OWCAT_USE_GTK
    pImpl->headerBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(pImpl->headerBox), 20);
    gtk_box_pack_start(GTK_BOX(pImpl->contentArea), pImpl->headerBox, FALSE, FALSE, 0);
    
    // Logo (placeholder)
    pImpl->logoImage = gtk_image_new_from_icon_name("applications-system", GTK_ICON_SIZE_DIALOG);
    gtk_box_pack_start(GTK_BOX(pImpl->headerBox), pImpl->logoImage, FALSE, FALSE, 0);
    
    // Title
    pImpl->titleLabel = gtk_label_new(nullptr);
    std::string titleMarkup = "<span size='x-large' weight='bold'>" + pImpl->appName + "</span>";
    gtk_label_set_markup(GTK_LABEL(pImpl->titleLabel), titleMarkup.c_str());
    gtk_box_pack_start(GTK_BOX(pImpl->headerBox), pImpl->titleLabel, FALSE, FALSE, 0);
    
    // Version
    pImpl->versionLabel = gtk_label_new(nullptr);
    std::string versionMarkup = "<span size='large'>Version " + pImpl->version + "</span>";
    gtk_label_set_markup(GTK_LABEL(pImpl->versionLabel), versionMarkup.c_str());
    gtk_box_pack_start(GTK_BOX(pImpl->headerBox), pImpl->versionLabel, FALSE, FALSE, 0);
    
    // Description
    pImpl->descriptionLabel = gtk_label_new(pImpl->description.c_str());
    gtk_label_set_line_wrap(GTK_LABEL(pImpl->descriptionLabel), TRUE);
    gtk_label_set_justify(GTK_LABEL(pImpl->descriptionLabel), GTK_JUSTIFY_CENTER);
    gtk_box_pack_start(GTK_BOX(pImpl->headerBox), pImpl->descriptionLabel, FALSE, FALSE, 0);
#endif
}

void GtkAboutDialog::createInfoNotebook() {
#ifdef OWCAT_USE_GTK
    pImpl->infoNotebook = gtk_notebook_new();
    gtk_notebook_set_tab_pos(GTK_NOTEBOOK(pImpl->infoNotebook), GTK_POS_TOP);
    gtk_container_set_border_width(GTK_CONTAINER(pImpl->infoNotebook), 10);
    gtk_box_pack_start(GTK_BOX(pImpl->contentArea), pImpl->infoNotebook, TRUE, TRUE, 0);
    
    // Create pages
    createAboutPage();
    createAuthorsPage();
    createLicensePage();
    createCreditsPage();
#endif
}

void GtkAboutDialog::createAboutPage() {
#ifdef OWCAT_USE_GTK
    pImpl->aboutPage = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pImpl->aboutPage), 
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    
    GtkWidget* aboutBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(aboutBox), 10);
    
    // Copyright
    GtkWidget* copyrightLabel = gtk_label_new(pImpl->copyright.c_str());
    gtk_label_set_selectable(GTK_LABEL(copyrightLabel), TRUE);
    gtk_box_pack_start(GTK_BOX(aboutBox), copyrightLabel, FALSE, FALSE, 0);
    
    // Additional info
    std::string aboutText = 
        "OwCat is a modern Chinese input method that leverages artificial intelligence "
        "to provide intelligent text prediction and completion. It supports multiple "
        "platforms including Windows, macOS, and Linux.\n\n"
        "Features:\n"
        "• AI-powered text prediction\n"
        "• Multiple input modes (Pinyin, Double Pinyin, etc.)\n"
        "• Customizable appearance and behavior\n"
        "• Cross-platform compatibility\n"
        "• Open source and free to use";
    
    GtkWidget* aboutTextLabel = gtk_label_new(aboutText.c_str());
    gtk_label_set_line_wrap(GTK_LABEL(aboutTextLabel), TRUE);
    gtk_label_set_selectable(GTK_LABEL(aboutTextLabel), TRUE);
    gtk_label_set_justify(GTK_LABEL(aboutTextLabel), GTK_JUSTIFY_LEFT);
    gtk_widget_set_halign(aboutTextLabel, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(aboutBox), aboutTextLabel, TRUE, TRUE, 0);
    
    gtk_container_add(GTK_CONTAINER(pImpl->aboutPage), aboutBox);
    
    // Add tab
    GtkWidget* aboutLabel = gtk_label_new("About");
    gtk_notebook_append_page(GTK_NOTEBOOK(pImpl->infoNotebook), pImpl->aboutPage, aboutLabel);
#endif
}

void GtkAboutDialog::createAuthorsPage() {
#ifdef OWCAT_USE_GTK
    pImpl->authorsPage = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pImpl->authorsPage), 
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    
    GtkWidget* authorsBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(authorsBox), 10);
    
    // Authors header
    GtkWidget* authorsHeaderLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(authorsHeaderLabel), "<span weight='bold'>Authors:</span>");
    gtk_widget_set_halign(authorsHeaderLabel, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(authorsBox), authorsHeaderLabel, FALSE, FALSE, 0);
    
    // Authors list
    for (const auto& author : pImpl->authors) {
        GtkWidget* authorLabel = gtk_label_new(("• " + author).c_str());
        gtk_label_set_selectable(GTK_LABEL(authorLabel), TRUE);
        gtk_widget_set_halign(authorLabel, GTK_ALIGN_START);
        gtk_box_pack_start(GTK_BOX(authorsBox), authorLabel, FALSE, FALSE, 0);
    }
    
    gtk_container_add(GTK_CONTAINER(pImpl->authorsPage), authorsBox);
    
    // Add tab
    GtkWidget* authorsLabel = gtk_label_new("Authors");
    gtk_notebook_append_page(GTK_NOTEBOOK(pImpl->infoNotebook), pImpl->authorsPage, authorsLabel);
#endif
}

void GtkAboutDialog::createLicensePage() {
#ifdef OWCAT_USE_GTK
    pImpl->licenseePage = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pImpl->licenseePage), 
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    
    GtkWidget* licenseBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(licenseBox), 10);
    
    // License header
    GtkWidget* licenseHeaderLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(licenseHeaderLabel), "<span weight='bold'>License:</span>");
    gtk_widget_set_halign(licenseHeaderLabel, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(licenseBox), licenseHeaderLabel, FALSE, FALSE, 0);
    
    // License text
    std::string licenseText = 
        "MIT License\n\n"
        "Copyright (c) 2024 OwCat Team\n\n"
        "Permission is hereby granted, free of charge, to any person obtaining a copy "
        "of this software and associated documentation files (the \"Software\"), to deal "
        "in the Software without restriction, including without limitation the rights "
        "to use, copy, modify, merge, publish, distribute, sublicense, and/or sell "
        "copies of the Software, and to permit persons to whom the Software is "
        "furnished to do so, subject to the following conditions:\n\n"
        "The above copyright notice and this permission notice shall be included in all "
        "copies or substantial portions of the Software.\n\n"
        "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR "
        "IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, "
        "FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE "
        "AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER "
        "LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, "
        "OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE "
        "SOFTWARE.";
    
    GtkWidget* licenseTextLabel = gtk_label_new(licenseText.c_str());
    gtk_label_set_line_wrap(GTK_LABEL(licenseTextLabel), TRUE);
    gtk_label_set_selectable(GTK_LABEL(licenseTextLabel), TRUE);
    gtk_label_set_justify(GTK_LABEL(licenseTextLabel), GTK_JUSTIFY_LEFT);
    gtk_widget_set_halign(licenseTextLabel, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(licenseBox), licenseTextLabel, TRUE, TRUE, 0);
    
    gtk_container_add(GTK_CONTAINER(pImpl->licenseePage), licenseBox);
    
    // Add tab
    GtkWidget* licenseLabel = gtk_label_new("License");
    gtk_notebook_append_page(GTK_NOTEBOOK(pImpl->infoNotebook), pImpl->licenseePage, licenseLabel);
#endif
}

void GtkAboutDialog::createCreditsPage() {
#ifdef OWCAT_USE_GTK
    pImpl->creditsPage = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pImpl->creditsPage), 
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    
    GtkWidget* creditsBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(creditsBox), 10);
    
    // Credits header
    GtkWidget* creditsHeaderLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(creditsHeaderLabel), "<span weight='bold'>Third-party Libraries and Tools:</span>");
    gtk_widget_set_halign(creditsHeaderLabel, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(creditsBox), creditsHeaderLabel, FALSE, FALSE, 0);
    
    // Credits list
    for (const auto& credit : pImpl->credits) {
        GtkWidget* creditLabel = gtk_label_new(("• " + credit).c_str());
        gtk_label_set_selectable(GTK_LABEL(creditLabel), TRUE);
        gtk_widget_set_halign(creditLabel, GTK_ALIGN_START);
        gtk_box_pack_start(GTK_BOX(creditsBox), creditLabel, FALSE, FALSE, 0);
    }
    
    // Special thanks
    GtkWidget* thanksHeaderLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(thanksHeaderLabel), "<span weight='bold'>Special Thanks:</span>");
    gtk_widget_set_halign(thanksHeaderLabel, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(creditsBox), thanksHeaderLabel, FALSE, FALSE, 10);
    
    std::vector<std::string> thanks = {
        "All contributors to the open source community",
        "Users who provide feedback and bug reports",
        "Translators who help localize the application",
        "Beta testers who help improve quality"
    };
    
    for (const auto& thank : thanks) {
        GtkWidget* thankLabel = gtk_label_new(("• " + thank).c_str());
        gtk_label_set_selectable(GTK_LABEL(thankLabel), TRUE);
        gtk_widget_set_halign(thankLabel, GTK_ALIGN_START);
        gtk_box_pack_start(GTK_BOX(creditsBox), thankLabel, FALSE, FALSE, 0);
    }
    
    gtk_container_add(GTK_CONTAINER(pImpl->creditsPage), creditsBox);
    
    // Add tab
    GtkWidget* creditsLabel = gtk_label_new("Credits");
    gtk_notebook_append_page(GTK_NOTEBOOK(pImpl->infoNotebook), pImpl->creditsPage, creditsLabel);
#endif
}

void GtkAboutDialog::createButtons() {
#ifdef OWCAT_USE_GTK
    // Create button box
    pImpl->buttonBox = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(pImpl->buttonBox), GTK_BUTTONBOX_END);
    gtk_box_set_spacing(GTK_BOX(pImpl->buttonBox), 5);
    gtk_container_set_border_width(GTK_CONTAINER(pImpl->buttonBox), 10);
    
    // Website button
    pImpl->websiteButton = gtk_button_new_with_label("Visit Website");
    gtk_box_pack_start(GTK_BOX(pImpl->buttonBox), pImpl->websiteButton, FALSE, FALSE, 0);
    
    // Donate button
    pImpl->donateButton = gtk_button_new_with_label("Donate");
    gtk_box_pack_start(GTK_BOX(pImpl->buttonBox), pImpl->donateButton, FALSE, FALSE, 0);
    
    // Close button
    pImpl->closeButton = gtk_button_new_with_label("Close");
    gtk_box_pack_end(GTK_BOX(pImpl->buttonBox), pImpl->closeButton, FALSE, FALSE, 0);
    
    // Add to dialog
    gtk_box_pack_end(GTK_BOX(pImpl->contentArea), pImpl->buttonBox, FALSE, FALSE, 0);
#endif
}

void GtkAboutDialog::connectSignals() {
#ifdef OWCAT_USE_GTK
    // Close button
    g_signal_connect(pImpl->closeButton, "clicked", G_CALLBACK(+[](GtkButton* button, gpointer userData) {
        GtkAboutDialog* dialog = static_cast<GtkAboutDialog*>(userData);
        dialog->hide();
    }), this);
    
    // Website button
    g_signal_connect(pImpl->websiteButton, "clicked", G_CALLBACK(+[](GtkButton* button, gpointer userData) {
        GtkAboutDialog* dialog = static_cast<GtkAboutDialog*>(userData);
        if (dialog->pImpl->onWebsiteClicked) {
            dialog->pImpl->onWebsiteClicked(dialog->pImpl->website);
        }
    }), this);
    
    // Donate button
    g_signal_connect(pImpl->donateButton, "clicked", G_CALLBACK(+[](GtkButton* button, gpointer userData) {
        GtkAboutDialog* dialog = static_cast<GtkAboutDialog*>(userData);
        if (dialog->pImpl->onDonateClicked) {
            dialog->pImpl->onDonateClicked();
        }
    }), this);
    
    // Dialog close signal
    g_signal_connect(pImpl->dialog, "delete-event", G_CALLBACK(+[](GtkWidget* widget, GdkEvent* event, gpointer userData) -> gboolean {
        GtkAboutDialog* dialog = static_cast<GtkAboutDialog*>(userData);
        dialog->hide();
        if (dialog->pImpl->onDialogClosed) {
            dialog->pImpl->onDialogClosed();
        }
        return TRUE; // Prevent default close
    }), this);
#endif
}

void GtkAboutDialog::updateContent() {
#ifdef OWCAT_USE_GTK
    if (!pImpl->dialog) {
        return;
    }
    
    // Update title
    if (pImpl->titleLabel) {
        std::string titleMarkup = "<span size='x-large' weight='bold'>" + pImpl->appName + "</span>";
        gtk_label_set_markup(GTK_LABEL(pImpl->titleLabel), titleMarkup.c_str());
    }
    
    // Update version
    if (pImpl->versionLabel) {
        std::string versionMarkup = "<span size='large'>Version " + pImpl->version + "</span>";
        gtk_label_set_markup(GTK_LABEL(pImpl->versionLabel), versionMarkup.c_str());
    }
    
    // Update description
    if (pImpl->descriptionLabel) {
        gtk_label_set_text(GTK_LABEL(pImpl->descriptionLabel), pImpl->description.c_str());
    }
    
    // Update window title
    std::string windowTitle = "About " + pImpl->appName;
    gtk_window_set_title(GTK_WINDOW(pImpl->dialog), windowTitle.c_str());
#endif
}

} // namespace owcat