#include "ui/gtk_ui.h"
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <sstream>
#include <functional>
#include <algorithm>

#ifdef OWCAT_USE_GTK
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <glib.h>
#include <pango/pango.h>
#include <cairo.h>
#endif

namespace owcat {

// Implementation structure for GtkCandidateWindow
struct GtkCandidateWindow::Impl {
#ifdef OWCAT_USE_GTK
    GtkWidget* window;
    GtkWidget* drawingArea;
    GtkWidget* vbox;
    GtkWidget* scrolledWindow;
#endif
    
    // State
    bool isVisible;
    std::vector<std::string> candidates;
    int selectedIndex;
    int pageSize;
    int currentPage;
    
    // Position and size
    int x, y;
    int width, height;
    int minWidth, minHeight;
    int maxWidth, maxHeight;
    
    // Appearance
    std::string fontFamily;
    int fontSize;
    std::string backgroundColor;
    std::string textColor;
    std::string selectedBackgroundColor;
    std::string selectedTextColor;
    std::string borderColor;
    int borderWidth;
    int padding;
    int itemHeight;
    
    // Callbacks
    std::function<void(int)> onCandidateSelected;
    std::function<void(int)> onCandidateHighlighted;
    std::function<void()> onWindowClosed;
    
    Impl() {
#ifdef OWCAT_USE_GTK
        window = nullptr;
        drawingArea = nullptr;
        vbox = nullptr;
        scrolledWindow = nullptr;
#endif
        
        isVisible = false;
        selectedIndex = -1;
        pageSize = 9;
        currentPage = 0;
        
        x = 0;
        y = 0;
        width = 300;
        height = 200;
        minWidth = 200;
        minHeight = 100;
        maxWidth = 600;
        maxHeight = 400;
        
        fontFamily = "Sans";
        fontSize = 12;
        backgroundColor = "#FFFFFF";
        textColor = "#000000";
        selectedBackgroundColor = "#3584E4";
        selectedTextColor = "#FFFFFF";
        borderColor = "#CCCCCC";
        borderWidth = 1;
        padding = 5;
        itemHeight = 25;
    }
};

GtkCandidateWindow::GtkCandidateWindow() : pImpl(std::make_unique<Impl>()) {}

GtkCandidateWindow::~GtkCandidateWindow() = default;

bool GtkCandidateWindow::create() {
#ifdef OWCAT_USE_GTK
    if (pImpl->window) {
        return true; // Already created
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
    gtk_window_set_resizable(GTK_WINDOW(pImpl->window), FALSE);
    gtk_window_set_keep_above(GTK_WINDOW(pImpl->window), TRUE);
    
    // Create drawing area
    pImpl->drawingArea = gtk_drawing_area_new();
    gtk_widget_set_size_request(pImpl->drawingArea, pImpl->width, pImpl->height);
    
    // Add drawing area to window
    gtk_container_add(GTK_CONTAINER(pImpl->window), pImpl->drawingArea);
    
    // Connect signals
    connectSignals();
    
    // Apply styling
    applyStyle();
    
    std::cout << "Candidate window created successfully" << std::endl;
    return true;
#else
    return false;
#endif
}

void GtkCandidateWindow::destroy() {
#ifdef OWCAT_USE_GTK
    if (pImpl->window) {
        gtk_widget_destroy(pImpl->window);
        pImpl->window = nullptr;
        pImpl->drawingArea = nullptr;
    }
#endif
}

bool GtkCandidateWindow::show() {
#ifdef OWCAT_USE_GTK
    if (pImpl->window && !pImpl->candidates.empty()) {
        gtk_widget_show_all(pImpl->window);
        pImpl->isVisible = true;
        return true;
    }
#endif
    return false;
}

bool GtkCandidateWindow::hide() {
#ifdef OWCAT_USE_GTK
    if (pImpl->window) {
        gtk_widget_hide(pImpl->window);
        pImpl->isVisible = false;
        return true;
    }
#endif
    return false;
}

bool GtkCandidateWindow::isVisible() const {
    return pImpl->isVisible;
}

void GtkCandidateWindow::setCandidates(const std::vector<std::string>& candidates) {
    pImpl->candidates = candidates;
    pImpl->selectedIndex = candidates.empty() ? -1 : 0;
    pImpl->currentPage = 0;
    
    updateSize();
    redraw();
}

std::vector<std::string> GtkCandidateWindow::getCandidates() const {
    return pImpl->candidates;
}

void GtkCandidateWindow::setSelectedIndex(int index) {
    if (index >= 0 && index < static_cast<int>(pImpl->candidates.size())) {
        pImpl->selectedIndex = index;
        pImpl->currentPage = index / pImpl->pageSize;
        redraw();
    }
}

int GtkCandidateWindow::getSelectedIndex() const {
    return pImpl->selectedIndex;
}

void GtkCandidateWindow::selectNext() {
    if (!pImpl->candidates.empty()) {
        int newIndex = (pImpl->selectedIndex + 1) % pImpl->candidates.size();
        setSelectedIndex(newIndex);
    }
}

void GtkCandidateWindow::selectPrevious() {
    if (!pImpl->candidates.empty()) {
        int newIndex = (pImpl->selectedIndex - 1 + pImpl->candidates.size()) % pImpl->candidates.size();
        setSelectedIndex(newIndex);
    }
}

void GtkCandidateWindow::nextPage() {
    int totalPages = (pImpl->candidates.size() + pImpl->pageSize - 1) / pImpl->pageSize;
    if (pImpl->currentPage < totalPages - 1) {
        pImpl->currentPage++;
        pImpl->selectedIndex = pImpl->currentPage * pImpl->pageSize;
        redraw();
    }
}

void GtkCandidateWindow::previousPage() {
    if (pImpl->currentPage > 0) {
        pImpl->currentPage--;
        pImpl->selectedIndex = pImpl->currentPage * pImpl->pageSize;
        redraw();
    }
}

// Position and size
void GtkCandidateWindow::setPosition(int x, int y) {
    pImpl->x = x;
    pImpl->y = y;
    
#ifdef OWCAT_USE_GTK
    if (pImpl->window) {
        gtk_window_move(GTK_WINDOW(pImpl->window), x, y);
    }
#endif
}

void GtkCandidateWindow::getPosition(int& x, int& y) const {
    x = pImpl->x;
    y = pImpl->y;
}

void GtkCandidateWindow::setSize(int width, int height) {
    pImpl->width = std::max(pImpl->minWidth, std::min(width, pImpl->maxWidth));
    pImpl->height = std::max(pImpl->minHeight, std::min(height, pImpl->maxHeight));
    
#ifdef OWCAT_USE_GTK
    if (pImpl->drawingArea) {
        gtk_widget_set_size_request(pImpl->drawingArea, pImpl->width, pImpl->height);
    }
    if (pImpl->window) {
        gtk_window_resize(GTK_WINDOW(pImpl->window), pImpl->width, pImpl->height);
    }
#endif
}

void GtkCandidateWindow::getSize(int& width, int& height) const {
    width = pImpl->width;
    height = pImpl->height;
}

void GtkCandidateWindow::setSizeConstraints(int minWidth, int minHeight, int maxWidth, int maxHeight) {
    pImpl->minWidth = minWidth;
    pImpl->minHeight = minHeight;
    pImpl->maxWidth = maxWidth;
    pImpl->maxHeight = maxHeight;
}

// Appearance
void GtkCandidateWindow::setFont(const std::string& family, int size) {
    pImpl->fontFamily = family;
    pImpl->fontSize = size;
    redraw();
}

void GtkCandidateWindow::setColors(const std::string& background, const std::string& text,
                                  const std::string& selectedBackground, const std::string& selectedText) {
    pImpl->backgroundColor = background;
    pImpl->textColor = text;
    pImpl->selectedBackgroundColor = selectedBackground;
    pImpl->selectedTextColor = selectedText;
    redraw();
}

void GtkCandidateWindow::setBorder(const std::string& color, int width) {
    pImpl->borderColor = color;
    pImpl->borderWidth = width;
    redraw();
}

void GtkCandidateWindow::setPadding(int padding) {
    pImpl->padding = padding;
    updateSize();
    redraw();
}

void GtkCandidateWindow::setItemHeight(int height) {
    pImpl->itemHeight = height;
    updateSize();
    redraw();
}

void GtkCandidateWindow::setPageSize(int size) {
    pImpl->pageSize = std::max(1, size);
    pImpl->currentPage = pImpl->selectedIndex / pImpl->pageSize;
    updateSize();
    redraw();
}

// Callbacks
void GtkCandidateWindow::setOnCandidateSelected(std::function<void(int)> callback) {
    pImpl->onCandidateSelected = callback;
}

void GtkCandidateWindow::setOnCandidateHighlighted(std::function<void(int)> callback) {
    pImpl->onCandidateHighlighted = callback;
}

void GtkCandidateWindow::setOnWindowClosed(std::function<void()> callback) {
    pImpl->onWindowClosed = callback;
}

// Widget access
GtkWidget* GtkCandidateWindow::getWidget() const {
#ifdef OWCAT_USE_GTK
    return pImpl->window;
#else
    return nullptr;
#endif
}

GtkWindow* GtkCandidateWindow::getWindow() const {
#ifdef OWCAT_USE_GTK
    return GTK_WINDOW(pImpl->window);
#else
    return nullptr;
#endif
}

// Private helper methods
void GtkCandidateWindow::updateSize() {
    if (pImpl->candidates.empty()) {
        return;
    }
    
    int visibleItems = std::min(pImpl->pageSize, static_cast<int>(pImpl->candidates.size()) - pImpl->currentPage * pImpl->pageSize);
    int newHeight = visibleItems * pImpl->itemHeight + 2 * pImpl->padding + 2 * pImpl->borderWidth;
    
    // Calculate width based on longest candidate text
    int maxTextWidth = 0;
#ifdef OWCAT_USE_GTK
    if (pImpl->drawingArea) {
        PangoContext* context = gtk_widget_get_pango_context(pImpl->drawingArea);
        PangoFontDescription* fontDesc = pango_font_description_new();
        pango_font_description_set_family(fontDesc, pImpl->fontFamily.c_str());
        pango_font_description_set_size(fontDesc, pImpl->fontSize * PANGO_SCALE);
        
        int startIndex = pImpl->currentPage * pImpl->pageSize;
        int endIndex = std::min(startIndex + pImpl->pageSize, static_cast<int>(pImpl->candidates.size()));
        
        for (int i = startIndex; i < endIndex; i++) {
            PangoLayout* layout = pango_layout_new(context);
            pango_layout_set_font_description(layout, fontDesc);
            
            std::string displayText = std::to_string(i - startIndex + 1) + ". " + pImpl->candidates[i];
            pango_layout_set_text(layout, displayText.c_str(), -1);
            
            int textWidth, textHeight;
            pango_layout_get_pixel_size(layout, &textWidth, &textHeight);
            maxTextWidth = std::max(maxTextWidth, textWidth);
            
            g_object_unref(layout);
        }
        
        pango_font_description_free(fontDesc);
    }
#endif
    
    int newWidth = maxTextWidth + 2 * pImpl->padding + 2 * pImpl->borderWidth;
    setSize(newWidth, newHeight);
}

void GtkCandidateWindow::redraw() {
#ifdef OWCAT_USE_GTK
    if (pImpl->drawingArea) {
        gtk_widget_queue_draw(pImpl->drawingArea);
    }
#endif
}

void GtkCandidateWindow::applyStyle() {
#ifdef OWCAT_USE_GTK
    if (pImpl->window) {
        // Apply CSS styling
        GtkCssProvider* cssProvider = gtk_css_provider_new();
        std::string css = ".candidate-window { "
                         "background-color: " + pImpl->backgroundColor + "; "
                         "border: " + std::to_string(pImpl->borderWidth) + "px solid " + pImpl->borderColor + "; "
                         "}";
        
        gtk_css_provider_load_from_data(cssProvider, css.c_str(), -1, nullptr);
        
        GtkStyleContext* context = gtk_widget_get_style_context(pImpl->window);
        gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(cssProvider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        gtk_style_context_add_class(context, "candidate-window");
        
        g_object_unref(cssProvider);
    }
#endif
}

void GtkCandidateWindow::connectSignals() {
#ifdef OWCAT_USE_GTK
    if (pImpl->drawingArea) {
        // Draw signal
        g_signal_connect(pImpl->drawingArea, "draw", G_CALLBACK(+[](GtkWidget* widget, cairo_t* cr, gpointer userData) -> gboolean {
            GtkCandidateWindow* window = static_cast<GtkCandidateWindow*>(userData);
            window->onDraw(cr);
            return FALSE;
        }), this);
        
        // Mouse events
        gtk_widget_add_events(pImpl->drawingArea, GDK_BUTTON_PRESS_MASK | GDK_POINTER_MOTION_MASK);
        
        g_signal_connect(pImpl->drawingArea, "button-press-event", G_CALLBACK(+[](GtkWidget* widget, GdkEventButton* event, gpointer userData) -> gboolean {
            GtkCandidateWindow* window = static_cast<GtkCandidateWindow*>(userData);
            return window->onButtonPress(event);
        }), this);
        
        g_signal_connect(pImpl->drawingArea, "motion-notify-event", G_CALLBACK(+[](GtkWidget* widget, GdkEventMotion* event, gpointer userData) -> gboolean {
            GtkCandidateWindow* window = static_cast<GtkCandidateWindow*>(userData);
            return window->onMotionNotify(event);
        }), this);
    }
    
    if (pImpl->window) {
        // Focus out event
        g_signal_connect(pImpl->window, "focus-out-event", G_CALLBACK(+[](GtkWidget* widget, GdkEventFocus* event, gpointer userData) -> gboolean {
            GtkCandidateWindow* window = static_cast<GtkCandidateWindow*>(userData);
            window->hide();
            if (window->pImpl->onWindowClosed) {
                window->pImpl->onWindowClosed();
            }
            return FALSE;
        }), this);
    }
#endif
}

#ifdef OWCAT_USE_GTK
void GtkCandidateWindow::onDraw(cairo_t* cr) {
    if (pImpl->candidates.empty()) {
        return;
    }
    
    // Get widget size
    int width = gtk_widget_get_allocated_width(pImpl->drawingArea);
    int height = gtk_widget_get_allocated_height(pImpl->drawingArea);
    
    // Clear background
    GdkRGBA bgColor;
    gdk_rgba_parse(&bgColor, pImpl->backgroundColor.c_str());
    cairo_set_source_rgba(cr, bgColor.red, bgColor.green, bgColor.blue, bgColor.alpha);
    cairo_rectangle(cr, 0, 0, width, height);
    cairo_fill(cr);
    
    // Draw border
    if (pImpl->borderWidth > 0) {
        GdkRGBA borderColor;
        gdk_rgba_parse(&borderColor, pImpl->borderColor.c_str());
        cairo_set_source_rgba(cr, borderColor.red, borderColor.green, borderColor.blue, borderColor.alpha);
        cairo_set_line_width(cr, pImpl->borderWidth);
        cairo_rectangle(cr, pImpl->borderWidth / 2.0, pImpl->borderWidth / 2.0, 
                       width - pImpl->borderWidth, height - pImpl->borderWidth);
        cairo_stroke(cr);
    }
    
    // Create Pango layout
    PangoLayout* layout = pango_cairo_create_layout(cr);
    PangoFontDescription* fontDesc = pango_font_description_new();
    pango_font_description_set_family(fontDesc, pImpl->fontFamily.c_str());
    pango_font_description_set_size(fontDesc, pImpl->fontSize * PANGO_SCALE);
    pango_layout_set_font_description(layout, fontDesc);
    
    // Draw candidates
    int startIndex = pImpl->currentPage * pImpl->pageSize;
    int endIndex = std::min(startIndex + pImpl->pageSize, static_cast<int>(pImpl->candidates.size()));
    
    for (int i = startIndex; i < endIndex; i++) {
        int itemY = pImpl->borderWidth + pImpl->padding + (i - startIndex) * pImpl->itemHeight;
        
        // Draw selection background
        if (i == pImpl->selectedIndex) {
            GdkRGBA selBgColor;
            gdk_rgba_parse(&selBgColor, pImpl->selectedBackgroundColor.c_str());
            cairo_set_source_rgba(cr, selBgColor.red, selBgColor.green, selBgColor.blue, selBgColor.alpha);
            cairo_rectangle(cr, pImpl->borderWidth, itemY, 
                           width - 2 * pImpl->borderWidth, pImpl->itemHeight);
            cairo_fill(cr);
        }
        
        // Draw text
        std::string displayText = std::to_string(i - startIndex + 1) + ". " + pImpl->candidates[i];
        pango_layout_set_text(layout, displayText.c_str(), -1);
        
        GdkRGBA textColor;
        if (i == pImpl->selectedIndex) {
            gdk_rgba_parse(&textColor, pImpl->selectedTextColor.c_str());
        } else {
            gdk_rgba_parse(&textColor, pImpl->textColor.c_str());
        }
        
        cairo_set_source_rgba(cr, textColor.red, textColor.green, textColor.blue, textColor.alpha);
        cairo_move_to(cr, pImpl->borderWidth + pImpl->padding, 
                     itemY + (pImpl->itemHeight - pImpl->fontSize) / 2);
        pango_cairo_show_layout(cr, layout);
    }
    
    // Cleanup
    pango_font_description_free(fontDesc);
    g_object_unref(layout);
}

gboolean GtkCandidateWindow::onButtonPress(GdkEventButton* event) {
    if (event->button == 1) { // Left click
        int clickedIndex = getIndexAtPosition(event->x, event->y);
        if (clickedIndex >= 0) {
            setSelectedIndex(clickedIndex);
            if (pImpl->onCandidateSelected) {
                pImpl->onCandidateSelected(clickedIndex);
            }
        }
        return TRUE;
    }
    return FALSE;
}

gboolean GtkCandidateWindow::onMotionNotify(GdkEventMotion* event) {
    int hoveredIndex = getIndexAtPosition(event->x, event->y);
    if (hoveredIndex >= 0 && hoveredIndex != pImpl->selectedIndex) {
        setSelectedIndex(hoveredIndex);
        if (pImpl->onCandidateHighlighted) {
            pImpl->onCandidateHighlighted(hoveredIndex);
        }
    }
    return FALSE;
}

int GtkCandidateWindow::getIndexAtPosition(double x, double y) {
    if (pImpl->candidates.empty()) {
        return -1;
    }
    
    int itemY = y - pImpl->borderWidth - pImpl->padding;
    if (itemY < 0) {
        return -1;
    }
    
    int itemIndex = itemY / pImpl->itemHeight;
    int absoluteIndex = pImpl->currentPage * pImpl->pageSize + itemIndex;
    
    if (absoluteIndex >= 0 && absoluteIndex < static_cast<int>(pImpl->candidates.size())) {
        return absoluteIndex;
    }
    
    return -1;
}
#endif

} // namespace owcat