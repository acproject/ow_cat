#include "platform/linux/linux_ime_adapter.h"
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <iostream>
#include <algorithm>

#ifdef __linux__
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <cairo.h>
#include <pango/pango.h>
#include <X11/Xlib.h>
#include <cstring>
#include <cstdlib>
#endif

namespace owcat {

// Implementation structure for LinuxCandidateWindow
struct LinuxCandidateWindow::Impl {
#ifdef __linux__
    GtkWidget* window;
    GtkWidget* drawingArea;
    GtkWidget* vbox;
    std::vector<GtkWidget*> candidateLabels;
    PangoFontDescription* fontDesc;
    GdkRGBA backgroundColor;
    GdkRGBA textColor;
    GdkRGBA selectedColor;
    GdkRGBA borderColor;
#endif
    
    // Callbacks
    std::function<void(const std::string&)> selectionCallback;
    std::function<void(int)> highlightCallback;
    
    // State
    bool isInitialized;
    bool isVisible;
    std::vector<std::string> candidates;
    int selectedIndex;
    int pageSize;
    int currentPage;
    
    // Position and size
    int x, y;
    int width, height;
    int itemHeight;
    int padding;
    int borderWidth;
    
    // Style
    std::string fontFamily;
    int fontSize;
    bool showNumbers;
    bool showBorder;
    
    Impl() {
#ifdef __linux__
        window = nullptr;
        drawingArea = nullptr;
        vbox = nullptr;
        fontDesc = nullptr;
#endif
        isInitialized = false;
        isVisible = false;
        selectedIndex = 0;
        pageSize = 10;
        currentPage = 0;
        
        x = 0;
        y = 0;
        width = 200;
        height = 100;
        itemHeight = 25;
        padding = 5;
        borderWidth = 1;
        
        fontFamily = "Sans";
        fontSize = 12;
        showNumbers = true;
        showBorder = true;
        
#ifdef __linux__
        // Initialize colors
        backgroundColor = {1.0, 1.0, 1.0, 0.95}; // White with slight transparency
        textColor = {0.0, 0.0, 0.0, 1.0};        // Black
        selectedColor = {0.2, 0.4, 0.8, 1.0};    // Blue
        borderColor = {0.5, 0.5, 0.5, 1.0};     // Gray
#endif
    }
    
    ~Impl() {
#ifdef __linux__
        if (fontDesc) {
            pango_font_description_free(fontDesc);
        }
        
        if (window) {
            gtk_widget_destroy(window);
        }
#endif
    }
};

LinuxCandidateWindow::LinuxCandidateWindow() : pImpl(std::make_unique<Impl>()) {}

LinuxCandidateWindow::~LinuxCandidateWindow() = default;

bool LinuxCandidateWindow::initialize() {
#ifdef __linux__
    if (pImpl->isInitialized) {
        return true;
    }
    
    // Create window
    pImpl->window = gtk_window_new(GTK_WINDOW_POPUP);
    if (!pImpl->window) {
        std::cerr << "Failed to create candidate window" << std::endl;
        return false;
    }
    
    // Set window properties
    gtk_window_set_decorated(GTK_WINDOW(pImpl->window), FALSE);
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(pImpl->window), TRUE);
    gtk_window_set_skip_pager_hint(GTK_WINDOW(pImpl->window), TRUE);
    gtk_window_set_type_hint(GTK_WINDOW(pImpl->window), GDK_WINDOW_TYPE_HINT_POPUP_MENU);
    gtk_window_set_accept_focus(GTK_WINDOW(pImpl->window), FALSE);
    gtk_window_set_focus_on_map(GTK_WINDOW(pImpl->window), FALSE);
    gtk_window_set_resizable(GTK_WINDOW(pImpl->window), FALSE);
    
    // Create vertical box container
    pImpl->vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(pImpl->window), pImpl->vbox);
    
    // Create drawing area for custom rendering
    pImpl->drawingArea = gtk_drawing_area_new();
    gtk_box_pack_start(GTK_BOX(pImpl->vbox), pImpl->drawingArea, TRUE, TRUE, 0);
    
    // Connect drawing signal
    g_signal_connect(pImpl->drawingArea, "draw", G_CALLBACK(onDraw), this);
    
    // Connect mouse events
    gtk_widget_add_events(pImpl->drawingArea, 
                         GDK_BUTTON_PRESS_MASK | 
                         GDK_BUTTON_RELEASE_MASK | 
                         GDK_POINTER_MOTION_MASK);
    
    g_signal_connect(pImpl->drawingArea, "button-press-event", G_CALLBACK(onButtonPress), this);
    g_signal_connect(pImpl->drawingArea, "motion-notify-event", G_CALLBACK(onMotionNotify), this);
    
    // Initialize font
    initializeFont();
    
    // Set initial size
    updateWindowSize();
    
    pImpl->isInitialized = true;
    std::cout << "Linux candidate window initialized successfully" << std::endl;
    return true;
#else
    std::cerr << "Linux candidate window not supported on this platform" << std::endl;
    return false;
#endif
}

void LinuxCandidateWindow::shutdown() {
#ifdef __linux__
    if (!pImpl->isInitialized) {
        return;
    }
    
    hide();
    
    if (pImpl->fontDesc) {
        pango_font_description_free(pImpl->fontDesc);
        pImpl->fontDesc = nullptr;
    }
    
    if (pImpl->window) {
        gtk_widget_destroy(pImpl->window);
        pImpl->window = nullptr;
    }
    
    pImpl->isInitialized = false;
    std::cout << "Linux candidate window shut down" << std::endl;
#endif
}

void LinuxCandidateWindow::show(const std::vector<std::string>& candidates, int selectedIndex, int x, int y) {
    if (!pImpl->isInitialized) {
        return;
    }
    
    pImpl->candidates = candidates;
    pImpl->selectedIndex = selectedIndex;
    pImpl->x = x;
    pImpl->y = y;
    pImpl->currentPage = selectedIndex / pImpl->pageSize;
    
    updateWindowSize();
    updatePosition();
    
#ifdef __linux__
    if (pImpl->window) {
        gtk_widget_show_all(pImpl->window);
        gtk_window_present(GTK_WINDOW(pImpl->window));
    }
#endif
    
    pImpl->isVisible = true;
}

void LinuxCandidateWindow::hide() {
    if (!pImpl->isInitialized || !pImpl->isVisible) {
        return;
    }
    
#ifdef __linux__
    if (pImpl->window) {
        gtk_widget_hide(pImpl->window);
    }
#endif
    
    pImpl->isVisible = false;
}

void LinuxCandidateWindow::updateCandidates(const std::vector<std::string>& candidates, int selectedIndex) {
    if (!pImpl->isInitialized) {
        return;
    }
    
    pImpl->candidates = candidates;
    pImpl->selectedIndex = selectedIndex;
    pImpl->currentPage = selectedIndex / pImpl->pageSize;
    
    updateWindowSize();
    
#ifdef __linux__
    if (pImpl->drawingArea) {
        gtk_widget_queue_draw(pImpl->drawingArea);
    }
#endif
}

void LinuxCandidateWindow::updateSelection(int selectedIndex) {
    if (!pImpl->isInitialized) {
        return;
    }
    
    pImpl->selectedIndex = selectedIndex;
    pImpl->currentPage = selectedIndex / pImpl->pageSize;
    
#ifdef __linux__
    if (pImpl->drawingArea) {
        gtk_widget_queue_draw(pImpl->drawingArea);
    }
#endif
}

void LinuxCandidateWindow::setPosition(int x, int y) {
    pImpl->x = x;
    pImpl->y = y;
    updatePosition();
}

void LinuxCandidateWindow::setSize(int width, int height) {
    pImpl->width = width;
    pImpl->height = height;
    
#ifdef __linux__
    if (pImpl->window) {
        gtk_window_resize(GTK_WINDOW(pImpl->window), width, height);
    }
#endif
}

void LinuxCandidateWindow::setPageSize(int pageSize) {
    pImpl->pageSize = pageSize;
    updateWindowSize();
}

void LinuxCandidateWindow::setFont(const std::string& fontFamily, int fontSize) {
    pImpl->fontFamily = fontFamily;
    pImpl->fontSize = fontSize;
    initializeFont();
    updateWindowSize();
}

void LinuxCandidateWindow::setColors(const std::string& background, const std::string& text, 
                                   const std::string& selected, const std::string& border) {
#ifdef __linux__
    gdk_rgba_parse(&pImpl->backgroundColor, background.c_str());
    gdk_rgba_parse(&pImpl->textColor, text.c_str());
    gdk_rgba_parse(&pImpl->selectedColor, selected.c_str());
    gdk_rgba_parse(&pImpl->borderColor, border.c_str());
    
    if (pImpl->drawingArea) {
        gtk_widget_queue_draw(pImpl->drawingArea);
    }
#endif
}

void LinuxCandidateWindow::setSelectionCallback(std::function<void(const std::string&)> callback) {
    pImpl->selectionCallback = callback;
}

void LinuxCandidateWindow::setHighlightCallback(std::function<void(int)> callback) {
    pImpl->highlightCallback = callback;
}

bool LinuxCandidateWindow::isVisible() const {
    return pImpl->isVisible;
}

std::vector<std::string> LinuxCandidateWindow::getCandidates() const {
    return pImpl->candidates;
}

int LinuxCandidateWindow::getSelectedIndex() const {
    return pImpl->selectedIndex;
}

int LinuxCandidateWindow::getPageSize() const {
    return pImpl->pageSize;
}

int LinuxCandidateWindow::getCurrentPage() const {
    return pImpl->currentPage;
}

// Private helper methods
void LinuxCandidateWindow::updateWindowSize() {
#ifdef __linux__
    if (!pImpl->isInitialized || pImpl->candidates.empty()) {
        return;
    }
    
    // Calculate required size based on candidates
    int visibleCandidates = std::min(pImpl->pageSize, static_cast<int>(pImpl->candidates.size()) - pImpl->currentPage * pImpl->pageSize);
    
    // Calculate text dimensions
    int maxWidth = 0;
    if (pImpl->fontDesc) {
        PangoLayout* layout = pango_cairo_create_layout(nullptr);
        if (layout) {
            pango_layout_set_font_description(layout, pImpl->fontDesc);
            
            for (int i = 0; i < visibleCandidates; i++) {
                int candidateIndex = pImpl->currentPage * pImpl->pageSize + i;
                if (candidateIndex < static_cast<int>(pImpl->candidates.size())) {
                    std::string text = formatCandidateText(candidateIndex);
                    pango_layout_set_text(layout, text.c_str(), -1);
                    
                    int textWidth, textHeight;
                    pango_layout_get_pixel_size(layout, &textWidth, &textHeight);
                    maxWidth = std::max(maxWidth, textWidth);
                }
            }
            
            g_object_unref(layout);
        }
    }
    
    // Calculate window dimensions
    pImpl->width = maxWidth + 2 * pImpl->padding + 2 * pImpl->borderWidth;
    pImpl->height = visibleCandidates * pImpl->itemHeight + 2 * pImpl->padding + 2 * pImpl->borderWidth;
    
    // Ensure minimum size
    pImpl->width = std::max(pImpl->width, 100);
    pImpl->height = std::max(pImpl->height, 50);
    
    if (pImpl->window) {
        gtk_window_resize(GTK_WINDOW(pImpl->window), pImpl->width, pImpl->height);
        gtk_widget_set_size_request(pImpl->drawingArea, pImpl->width, pImpl->height);
    }
#endif
}

void LinuxCandidateWindow::updatePosition() {
#ifdef __linux__
    if (!pImpl->window) {
        return;
    }
    
    // Get screen dimensions
    GdkScreen* screen = gtk_window_get_screen(GTK_WINDOW(pImpl->window));
    int screenWidth = gdk_screen_get_width(screen);
    int screenHeight = gdk_screen_get_height(screen);
    
    // Adjust position to keep window on screen
    int adjustedX = pImpl->x;
    int adjustedY = pImpl->y;
    
    if (adjustedX + pImpl->width > screenWidth) {
        adjustedX = screenWidth - pImpl->width;
    }
    if (adjustedX < 0) {
        adjustedX = 0;
    }
    
    if (adjustedY + pImpl->height > screenHeight) {
        adjustedY = pImpl->y - pImpl->height - 5; // Show above cursor
    }
    if (adjustedY < 0) {
        adjustedY = 0;
    }
    
    gtk_window_move(GTK_WINDOW(pImpl->window), adjustedX, adjustedY);
#endif
}

void LinuxCandidateWindow::initializeFont() {
#ifdef __linux__
    if (pImpl->fontDesc) {
        pango_font_description_free(pImpl->fontDesc);
    }
    
    pImpl->fontDesc = pango_font_description_new();
    pango_font_description_set_family(pImpl->fontDesc, pImpl->fontFamily.c_str());
    pango_font_description_set_size(pImpl->fontDesc, pImpl->fontSize * PANGO_SCALE);
#endif
}

std::string LinuxCandidateWindow::formatCandidateText(int index) const {
    if (index < 0 || index >= static_cast<int>(pImpl->candidates.size())) {
        return "";
    }
    
    std::string text;
    if (pImpl->showNumbers) {
        int displayIndex = (index % pImpl->pageSize) + 1;
        if (displayIndex == 10) displayIndex = 0; // Show 0 for 10th item
        text = std::to_string(displayIndex) + ". " + pImpl->candidates[index];
    } else {
        text = pImpl->candidates[index];
    }
    
    return text;
}

int LinuxCandidateWindow::getCandidateIndexAtPosition(int x, int y) const {
    if (x < pImpl->padding || x > pImpl->width - pImpl->padding ||
        y < pImpl->padding || y > pImpl->height - pImpl->padding) {
        return -1;
    }
    
    int itemIndex = (y - pImpl->padding) / pImpl->itemHeight;
    int candidateIndex = pImpl->currentPage * pImpl->pageSize + itemIndex;
    
    if (candidateIndex >= 0 && candidateIndex < static_cast<int>(pImpl->candidates.size())) {
        return candidateIndex;
    }
    
    return -1;
}

void LinuxCandidateWindow::selectCandidate(int index) {
    if (index >= 0 && index < static_cast<int>(pImpl->candidates.size())) {
        if (pImpl->selectionCallback) {
            pImpl->selectionCallback(pImpl->candidates[index]);
        }
    }
}

void LinuxCandidateWindow::highlightCandidate(int index) {
    if (index != pImpl->selectedIndex) {
        pImpl->selectedIndex = index;
        
#ifdef __linux__
        if (pImpl->drawingArea) {
            gtk_widget_queue_draw(pImpl->drawingArea);
        }
#endif
        
        if (pImpl->highlightCallback) {
            pImpl->highlightCallback(index);
        }
    }
}

// GTK+ event handlers
#ifdef __linux__
gboolean LinuxCandidateWindow::onDraw(GtkWidget* widget, cairo_t* cr, gpointer userData) {
    LinuxCandidateWindow* window = static_cast<LinuxCandidateWindow*>(userData);
    return window->handleDraw(cr);
}

gboolean LinuxCandidateWindow::onButtonPress(GtkWidget* widget, GdkEventButton* event, gpointer userData) {
    LinuxCandidateWindow* window = static_cast<LinuxCandidateWindow*>(userData);
    return window->handleButtonPress(event);
}

gboolean LinuxCandidateWindow::onMotionNotify(GtkWidget* widget, GdkEventMotion* event, gpointer userData) {
    LinuxCandidateWindow* window = static_cast<LinuxCandidateWindow*>(userData);
    return window->handleMotionNotify(event);
}

gboolean LinuxCandidateWindow::handleDraw(cairo_t* cr) {
    if (pImpl->candidates.empty()) {
        return FALSE;
    }
    
    // Clear background
    cairo_set_source_rgba(cr, pImpl->backgroundColor.red, pImpl->backgroundColor.green, 
                          pImpl->backgroundColor.blue, pImpl->backgroundColor.alpha);
    cairo_paint(cr);
    
    // Draw border
    if (pImpl->showBorder) {
        cairo_set_source_rgba(cr, pImpl->borderColor.red, pImpl->borderColor.green, 
                              pImpl->borderColor.blue, pImpl->borderColor.alpha);
        cairo_set_line_width(cr, pImpl->borderWidth);
        cairo_rectangle(cr, 0.5, 0.5, pImpl->width - 1, pImpl->height - 1);
        cairo_stroke(cr);
    }
    
    // Create Pango layout
    PangoLayout* layout = pango_cairo_create_layout(cr);
    if (!layout) {
        return FALSE;
    }
    
    pango_layout_set_font_description(layout, pImpl->fontDesc);
    
    // Draw candidates
    int visibleCandidates = std::min(pImpl->pageSize, static_cast<int>(pImpl->candidates.size()) - pImpl->currentPage * pImpl->pageSize);
    
    for (int i = 0; i < visibleCandidates; i++) {
        int candidateIndex = pImpl->currentPage * pImpl->pageSize + i;
        if (candidateIndex >= static_cast<int>(pImpl->candidates.size())) {
            break;
        }
        
        int itemY = pImpl->padding + i * pImpl->itemHeight;
        
        // Draw selection background
        if (candidateIndex == pImpl->selectedIndex) {
            cairo_set_source_rgba(cr, pImpl->selectedColor.red, pImpl->selectedColor.green, 
                                  pImpl->selectedColor.blue, 0.3);
            cairo_rectangle(cr, pImpl->padding, itemY, 
                           pImpl->width - 2 * pImpl->padding, pImpl->itemHeight);
            cairo_fill(cr);
        }
        
        // Draw candidate text
        std::string text = formatCandidateText(candidateIndex);
        pango_layout_set_text(layout, text.c_str(), -1);
        
        if (candidateIndex == pImpl->selectedIndex) {
            cairo_set_source_rgba(cr, pImpl->selectedColor.red, pImpl->selectedColor.green, 
                                  pImpl->selectedColor.blue, pImpl->selectedColor.alpha);
        } else {
            cairo_set_source_rgba(cr, pImpl->textColor.red, pImpl->textColor.green, 
                                  pImpl->textColor.blue, pImpl->textColor.alpha);
        }
        
        cairo_move_to(cr, pImpl->padding + 5, itemY + (pImpl->itemHeight - pImpl->fontSize) / 2);
        pango_cairo_show_layout(cr, layout);
    }
    
    g_object_unref(layout);
    return TRUE;
}

gboolean LinuxCandidateWindow::handleButtonPress(GdkEventButton* event) {
    if (event->button == 1) { // Left click
        int candidateIndex = getCandidateIndexAtPosition(static_cast<int>(event->x), static_cast<int>(event->y));
        if (candidateIndex >= 0) {
            selectCandidate(candidateIndex);
            return TRUE;
        }
    }
    return FALSE;
}

gboolean LinuxCandidateWindow::handleMotionNotify(GdkEventMotion* event) {
    int candidateIndex = getCandidateIndexAtPosition(static_cast<int>(event->x), static_cast<int>(event->y));
    if (candidateIndex >= 0) {
        highlightCandidate(candidateIndex);
    }
    return FALSE;
}
#endif

} // namespace owcat