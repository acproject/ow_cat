#include "platform/linux/linux_ime_adapter.h"
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <iostream>
#include <algorithm>

#ifdef __linux__
#include <ibus.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <cstring>
#include <cstdlib>
#endif

namespace owcat {

// Implementation structure for LinuxInputEngine
struct LinuxInputEngine::Impl {
#ifdef __linux__
    IBusEngine* engine;
    IBusText* preeditText;
    IBusLookupTable* lookupTable;
    IBusProperty* statusProperty;
    IBusPropList* propList;
#endif
    
    // Callbacks
    std::function<bool(unsigned int, unsigned int, unsigned int)> processKeyEventCallback;
    std::function<void()> focusInCallback;
    std::function<void()> focusOutCallback;
    std::function<void()> resetCallback;
    std::function<void()> enableCallback;
    std::function<void()> disableCallback;
    
    // State
    bool isInitialized;
    bool isEnabled;
    bool hasFocus;
    std::string currentInput;
    std::vector<std::string> candidates;
    int selectedCandidate;
    bool preeditVisible;
    bool lookupTableVisible;
    
    // Configuration
    std::string engineName;
    std::string displayName;
    std::string description;
    std::string language;
    std::string license;
    std::string author;
    std::string icon;
    std::string layout;
    
    Impl() {
#ifdef __linux__
        engine = nullptr;
        preeditText = nullptr;
        lookupTable = nullptr;
        statusProperty = nullptr;
        propList = nullptr;
#endif
        isInitialized = false;
        isEnabled = false;
        hasFocus = false;
        selectedCandidate = 0;
        preeditVisible = false;
        lookupTableVisible = false;
        
        engineName = "owcat";
        displayName = "OwCat";
        description = "OwCat Chinese Input Method";
        language = "zh";
        license = "MIT";
        author = "OwCat Team";
        icon = "owcat";
        layout = "us";
    }
    
    ~Impl() {
#ifdef __linux__
        if (preeditText) {
            g_object_unref(preeditText);
        }
        if (lookupTable) {
            g_object_unref(lookupTable);
        }
        if (statusProperty) {
            g_object_unref(statusProperty);
        }
        if (propList) {
            g_object_unref(propList);
        }
        if (engine) {
            g_object_unref(engine);
        }
#endif
    }
};

LinuxInputEngine::LinuxInputEngine() : pImpl(std::make_unique<Impl>()) {}

LinuxInputEngine::~LinuxInputEngine() = default;

bool LinuxInputEngine::initialize() {
#ifdef __linux__
    if (pImpl->isInitialized) {
        return true;
    }
    
    // Initialize IBus text objects
    pImpl->preeditText = ibus_text_new_from_string("");
    if (!pImpl->preeditText) {
        std::cerr << "Failed to create preedit text" << std::endl;
        return false;
    }
    
    // Initialize lookup table
    pImpl->lookupTable = ibus_lookup_table_new(10, 0, TRUE, TRUE);
    if (!pImpl->lookupTable) {
        std::cerr << "Failed to create lookup table" << std::endl;
        return false;
    }
    
    // Initialize properties
    initializeProperties();
    
    pImpl->isInitialized = true;
    std::cout << "Linux input engine initialized successfully" << std::endl;
    return true;
#else
    std::cerr << "Linux input engine not supported on this platform" << std::endl;
    return false;
#endif
}

void LinuxInputEngine::shutdown() {
#ifdef __linux__
    if (!pImpl->isInitialized) {
        return;
    }
    
    // Clean up IBus objects
    if (pImpl->preeditText) {
        g_object_unref(pImpl->preeditText);
        pImpl->preeditText = nullptr;
    }
    
    if (pImpl->lookupTable) {
        g_object_unref(pImpl->lookupTable);
        pImpl->lookupTable = nullptr;
    }
    
    if (pImpl->statusProperty) {
        g_object_unref(pImpl->statusProperty);
        pImpl->statusProperty = nullptr;
    }
    
    if (pImpl->propList) {
        g_object_unref(pImpl->propList);
        pImpl->propList = nullptr;
    }
    
    pImpl->isInitialized = false;
    std::cout << "Linux input engine shut down" << std::endl;
#endif
}

bool LinuxInputEngine::processKeyEvent(unsigned int keyval, unsigned int keycode, unsigned int state) {
    if (!pImpl->isEnabled || !pImpl->hasFocus) {
        return false;
    }
    
#ifdef __linux__
    // Handle special keys
    switch (keyval) {
        case XK_Escape:
            if (!pImpl->currentInput.empty()) {
                clearInput();
                return true;
            }
            return false;
            
        case XK_Return:
        case XK_KP_Enter:
            if (!pImpl->currentInput.empty()) {
                commitCurrentInput();
                return true;
            }
            return false;
            
        case XK_BackSpace:
            if (!pImpl->currentInput.empty()) {
                pImpl->currentInput.pop_back();
                updatePreeditAndCandidates();
                return true;
            }
            return false;
            
        case XK_Delete:
            if (!pImpl->currentInput.empty()) {
                clearInput();
                return true;
            }
            return false;
            
        case XK_Up:
        case XK_KP_Up:
            if (pImpl->lookupTableVisible && pImpl->selectedCandidate > 0) {
                pImpl->selectedCandidate--;
                updateLookupTableSelection();
                return true;
            }
            return false;
            
        case XK_Down:
        case XK_KP_Down:
            if (pImpl->lookupTableVisible && pImpl->selectedCandidate < static_cast<int>(pImpl->candidates.size()) - 1) {
                pImpl->selectedCandidate++;
                updateLookupTableSelection();
                return true;
            }
            return false;
            
        case XK_Page_Up:
        case XK_KP_Page_Up:
            if (pImpl->lookupTableVisible) {
                pageUpCandidates();
                return true;
            }
            return false;
            
        case XK_Page_Down:
        case XK_KP_Page_Down:
            if (pImpl->lookupTableVisible) {
                pageDownCandidates();
                return true;
            }
            return false;
            
        case XK_space:
            if (!pImpl->currentInput.empty() && pImpl->lookupTableVisible && !pImpl->candidates.empty()) {
                commitCandidate(pImpl->selectedCandidate);
                return true;
            }
            break;
            
        case XK_1: case XK_2: case XK_3: case XK_4: case XK_5:
        case XK_6: case XK_7: case XK_8: case XK_9: case XK_0:
            if (pImpl->lookupTableVisible && !pImpl->candidates.empty()) {
                int index = (keyval == XK_0) ? 9 : (keyval - XK_1);
                if (index < static_cast<int>(pImpl->candidates.size())) {
                    commitCandidate(index);
                    return true;
                }
            }
            break;
    }
    
    // Handle regular character input
    if (isInputCharacter(keyval, state)) {
        char ch = static_cast<char>(keyval);
        if (ch >= 'a' && ch <= 'z') {
            pImpl->currentInput += ch;
            updatePreeditAndCandidates();
            return true;
        }
    }
#endif
    
    // Call external callback if available
    if (pImpl->processKeyEventCallback) {
        return pImpl->processKeyEventCallback(keyval, keycode, state);
    }
    
    return false;
}

void LinuxInputEngine::focusIn() {
    pImpl->hasFocus = true;
    
    if (pImpl->focusInCallback) {
        pImpl->focusInCallback();
    }
    
#ifdef __linux__
    // Register properties when gaining focus
    if (pImpl->engine && pImpl->propList) {
        ibus_engine_register_properties(pImpl->engine, pImpl->propList);
    }
#endif
}

void LinuxInputEngine::focusOut() {
    pImpl->hasFocus = false;
    
    // Clear input when losing focus
    clearInput();
    
    if (pImpl->focusOutCallback) {
        pImpl->focusOutCallback();
    }
}

void LinuxInputEngine::reset() {
    clearInput();
    
    if (pImpl->resetCallback) {
        pImpl->resetCallback();
    }
}

void LinuxInputEngine::enable() {
    pImpl->isEnabled = true;
    
    if (pImpl->enableCallback) {
        pImpl->enableCallback();
    }
}

void LinuxInputEngine::disable() {
    pImpl->isEnabled = false;
    clearInput();
    
    if (pImpl->disableCallback) {
        pImpl->disableCallback();
    }
}

void LinuxInputEngine::showPreeditText() {
    pImpl->preeditVisible = true;
    
#ifdef __linux__
    if (pImpl->engine && pImpl->preeditText) {
        ibus_engine_show_preedit_text(pImpl->engine);
    }
#endif
}

void LinuxInputEngine::hidePreeditText() {
    pImpl->preeditVisible = false;
    
#ifdef __linux__
    if (pImpl->engine) {
        ibus_engine_hide_preedit_text(pImpl->engine);
    }
#endif
}

void LinuxInputEngine::updatePreeditText(const std::string& text, int cursorPos, bool visible) {
#ifdef __linux__
    if (pImpl->preeditText) {
        g_object_unref(pImpl->preeditText);
    }
    
    pImpl->preeditText = ibus_text_new_from_string(text.c_str());
    
    if (pImpl->engine) {
        ibus_engine_update_preedit_text(pImpl->engine, pImpl->preeditText, cursorPos, visible);
    }
#endif
    
    pImpl->preeditVisible = visible;
}

void LinuxInputEngine::commitText(const std::string& text) {
#ifdef __linux__
    if (pImpl->engine) {
        IBusText* commitText = ibus_text_new_from_string(text.c_str());
        ibus_engine_commit_text(pImpl->engine, commitText);
        g_object_unref(commitText);
    }
#endif
    
    clearInput();
}

void LinuxInputEngine::showLookupTable() {
    pImpl->lookupTableVisible = true;
    
#ifdef __linux__
    if (pImpl->engine) {
        ibus_engine_show_lookup_table(pImpl->engine);
    }
#endif
}

void LinuxInputEngine::hideLookupTable() {
    pImpl->lookupTableVisible = false;
    
#ifdef __linux__
    if (pImpl->engine) {
        ibus_engine_hide_lookup_table(pImpl->engine);
    }
#endif
}

void LinuxInputEngine::updateLookupTable(const std::vector<std::string>& candidates, int selectedIndex, bool visible) {
    pImpl->candidates = candidates;
    pImpl->selectedCandidate = selectedIndex;
    
#ifdef __linux__
    if (pImpl->lookupTable) {
        ibus_lookup_table_clear(pImpl->lookupTable);
        
        for (const auto& candidate : candidates) {
            IBusText* text = ibus_text_new_from_string(candidate.c_str());
            ibus_lookup_table_append_candidate(pImpl->lookupTable, text);
            g_object_unref(text);
        }
        
        ibus_lookup_table_set_cursor_pos(pImpl->lookupTable, selectedIndex);
        
        if (pImpl->engine) {
            ibus_engine_update_lookup_table(pImpl->engine, pImpl->lookupTable, visible);
        }
    }
#endif
    
    pImpl->lookupTableVisible = visible;
}

void LinuxInputEngine::setEngine(void* engine) {
#ifdef __linux__
    pImpl->engine = static_cast<IBusEngine*>(engine);
#endif
}

void* LinuxInputEngine::getEngine() const {
#ifdef __linux__
    return pImpl->engine;
#else
    return nullptr;
#endif
}

// Callback setters
void LinuxInputEngine::setProcessKeyEventCallback(std::function<bool(unsigned int, unsigned int, unsigned int)> callback) {
    pImpl->processKeyEventCallback = callback;
}

void LinuxInputEngine::setFocusInCallback(std::function<void()> callback) {
    pImpl->focusInCallback = callback;
}

void LinuxInputEngine::setFocusOutCallback(std::function<void()> callback) {
    pImpl->focusOutCallback = callback;
}

void LinuxInputEngine::setResetCallback(std::function<void()> callback) {
    pImpl->resetCallback = callback;
}

void LinuxInputEngine::setEnableCallback(std::function<void()> callback) {
    pImpl->enableCallback = callback;
}

void LinuxInputEngine::setDisableCallback(std::function<void()> callback) {
    pImpl->disableCallback = callback;
}

// Configuration
std::string LinuxInputEngine::getEngineName() const {
    return pImpl->engineName;
}

void LinuxInputEngine::setEngineName(const std::string& name) {
    pImpl->engineName = name;
}

std::string LinuxInputEngine::getDisplayName() const {
    return pImpl->displayName;
}

void LinuxInputEngine::setDisplayName(const std::string& name) {
    pImpl->displayName = name;
}

std::string LinuxInputEngine::getDescription() const {
    return pImpl->description;
}

void LinuxInputEngine::setDescription(const std::string& desc) {
    pImpl->description = desc;
}

std::string LinuxInputEngine::getLanguage() const {
    return pImpl->language;
}

void LinuxInputEngine::setLanguage(const std::string& lang) {
    pImpl->language = lang;
}

std::string LinuxInputEngine::getIcon() const {
    return pImpl->icon;
}

void LinuxInputEngine::setIcon(const std::string& iconPath) {
    pImpl->icon = iconPath;
}

std::string LinuxInputEngine::getLayout() const {
    return pImpl->layout;
}

void LinuxInputEngine::setLayout(const std::string& layout) {
    pImpl->layout = layout;
}

// State queries
bool LinuxInputEngine::isEnabled() const {
    return pImpl->isEnabled;
}

bool LinuxInputEngine::hasFocus() const {
    return pImpl->hasFocus;
}

bool LinuxInputEngine::isPreeditVisible() const {
    return pImpl->preeditVisible;
}

bool LinuxInputEngine::isLookupTableVisible() const {
    return pImpl->lookupTableVisible;
}

std::string LinuxInputEngine::getCurrentInput() const {
    return pImpl->currentInput;
}

std::vector<std::string> LinuxInputEngine::getCurrentCandidates() const {
    return pImpl->candidates;
}

int LinuxInputEngine::getSelectedCandidateIndex() const {
    return pImpl->selectedCandidate;
}

// Private helper methods
void LinuxInputEngine::clearInput() {
    pImpl->currentInput.clear();
    pImpl->candidates.clear();
    pImpl->selectedCandidate = 0;
    
    hidePreeditText();
    hideLookupTable();
}

void LinuxInputEngine::commitCurrentInput() {
    if (!pImpl->currentInput.empty()) {
        commitText(pImpl->currentInput);
    }
}

void LinuxInputEngine::commitCandidate(int index) {
    if (index >= 0 && index < static_cast<int>(pImpl->candidates.size())) {
        commitText(pImpl->candidates[index]);
    }
}

void LinuxInputEngine::updatePreeditAndCandidates() {
    if (pImpl->currentInput.empty()) {
        hidePreeditText();
        hideLookupTable();
        return;
    }
    
    // Update preedit text
    updatePreeditText(pImpl->currentInput, pImpl->currentInput.length(), true);
    
    // Generate candidates (simplified - in real implementation, this would use the core engine)
    generateCandidates();
    
    // Update lookup table
    if (!pImpl->candidates.empty()) {
        updateLookupTable(pImpl->candidates, 0, true);
    } else {
        hideLookupTable();
    }
}

void LinuxInputEngine::generateCandidates() {
    // Simplified candidate generation
    // In a real implementation, this would interface with the core engine
    pImpl->candidates.clear();
    
    if (!pImpl->currentInput.empty()) {
        // Generate some example candidates based on input
        std::string input = pImpl->currentInput;
        
        // Simple pinyin to Chinese character mapping (very basic)
        if (input == "ni") {
            pImpl->candidates = {"你", "尼", "泥", "逆"};
        } else if (input == "hao") {
            pImpl->candidates = {"好", "号", "毫", "豪"};
        } else if (input == "ma") {
            pImpl->candidates = {"吗", "妈", "马", "麻"};
        } else {
            // Default: just add the input itself as a candidate
            pImpl->candidates.push_back(input);
        }
    }
    
    pImpl->selectedCandidate = 0;
}

void LinuxInputEngine::updateLookupTableSelection() {
#ifdef __linux__
    if (pImpl->lookupTable && pImpl->engine) {
        ibus_lookup_table_set_cursor_pos(pImpl->lookupTable, pImpl->selectedCandidate);
        ibus_engine_update_lookup_table(pImpl->engine, pImpl->lookupTable, true);
    }
#endif
}

void LinuxInputEngine::pageUpCandidates() {
#ifdef __linux__
    if (pImpl->lookupTable) {
        if (ibus_lookup_table_page_up(pImpl->lookupTable)) {
            pImpl->selectedCandidate = ibus_lookup_table_get_cursor_pos(pImpl->lookupTable);
            if (pImpl->engine) {
                ibus_engine_update_lookup_table(pImpl->engine, pImpl->lookupTable, true);
            }
        }
    }
#endif
}

void LinuxInputEngine::pageDownCandidates() {
#ifdef __linux__
    if (pImpl->lookupTable) {
        if (ibus_lookup_table_page_down(pImpl->lookupTable)) {
            pImpl->selectedCandidate = ibus_lookup_table_get_cursor_pos(pImpl->lookupTable);
            if (pImpl->engine) {
                ibus_engine_update_lookup_table(pImpl->engine, pImpl->lookupTable, true);
            }
        }
    }
#endif
}

bool LinuxInputEngine::isInputCharacter(unsigned int keyval, unsigned int state) {
#ifdef __linux__
    // Check if this is a regular input character
    if (state & (GDK_CONTROL_MASK | GDK_MOD1_MASK)) {
        return false; // Ignore if Ctrl or Alt is pressed
    }
    
    return (keyval >= 'a' && keyval <= 'z') || 
           (keyval >= 'A' && keyval <= 'Z') ||
           (keyval >= '0' && keyval <= '9');
#else
    return false;
#endif
}

void LinuxInputEngine::initializeProperties() {
#ifdef __linux__
    // Create status property
    pImpl->statusProperty = ibus_property_new("status",
                                              IBUS_PROP_TYPE_NORMAL,
                                              ibus_text_new_from_string("EN"),
                                              pImpl->icon.c_str(),
                                              ibus_text_new_from_string("Input Method Status"),
                                              TRUE,
                                              TRUE,
                                              IBUS_PROP_STATE_UNCHECKED,
                                              nullptr);
    
    // Create property list
    pImpl->propList = ibus_prop_list_new();
    if (pImpl->statusProperty) {
        ibus_prop_list_append(pImpl->propList, pImpl->statusProperty);
    }
#endif
}

void LinuxInputEngine::updateStatusProperty(const std::string& status) {
#ifdef __linux__
    if (pImpl->statusProperty && pImpl->engine) {
        IBusText* text = ibus_text_new_from_string(status.c_str());
        ibus_property_set_label(pImpl->statusProperty, text);
        ibus_engine_update_property(pImpl->engine, pImpl->statusProperty);
        g_object_unref(text);
    }
#endif
}

void LinuxInputEngine::handlePropertyActivate(const std::string& propName, unsigned int propState) {
    if (propName == "status") {
        // Toggle input method state
        if (pImpl->isEnabled) {
            disable();
            updateStatusProperty("EN");
        } else {
            enable();
            updateStatusProperty("中");
        }
    }
}

} // namespace owcat