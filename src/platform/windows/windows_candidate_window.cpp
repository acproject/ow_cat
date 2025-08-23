#ifdef _WIN32

#include "platform/windows/windows_ime_adapter.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <sstream>

namespace owcat {
namespace platform {
namespace windows {

// 窗口类名
static const wchar_t* CANDIDATE_WINDOW_CLASS = L"OwCatCandidateWindow";

// 窗口过程
LRESULT CALLBACK CandidateWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    WindowsCandidateWindow* window = reinterpret_cast<WindowsCandidateWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    
    switch (msg) {
        case WM_CREATE:
            {
                CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
                SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
            }
            return 0;
            
        case WM_PAINT:
            if (window) {
                window->onPaint();
            }
            return 0;
            
        case WM_LBUTTONDOWN:
            if (window) {
                int x = LOWORD(lParam);
                int y = HIWORD(lParam);
                window->onMouseClick(x, y);
            }
            return 0;
            
        case WM_MOUSEMOVE:
            if (window) {
                int x = LOWORD(lParam);
                int y = HIWORD(lParam);
                window->onMouseMove(x, y);
            }
            return 0;
            
        case WM_KILLFOCUS:
            if (window) {
                window->hide();
            }
            return 0;
            
        case WM_DESTROY:
            return 0;
            
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

// WindowsCandidateWindow::Impl 实现
class WindowsCandidateWindow::Impl {
public:
    Impl() : hwnd_(nullptr), visible_(false), selected_index_(0),
             item_height_(25), margin_(5), font_(nullptr) {
        // 初始化默认样式
        background_color_ = RGB(255, 255, 255);
        text_color_ = RGB(0, 0, 0);
        selected_color_ = RGB(0, 120, 215);
        selected_text_color_ = RGB(255, 255, 255);
        border_color_ = RGB(128, 128, 128);
    }
    
    ~Impl() {
        destroy();
    }
    
    bool create() {
        if (hwnd_) {
            return true;
        }
        
        spdlog::info("Creating Windows candidate window");
        
        // 注册窗口类
        if (!registerWindowClass()) {
            spdlog::error("Failed to register candidate window class");
            return false;
        }
        
        // 创建字体
        createFont();
        
        // 创建窗口
        hwnd_ = CreateWindowExW(
            WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
            CANDIDATE_WINDOW_CLASS,
            L"Candidate Window",
            WS_POPUP | WS_BORDER,
            0, 0, 200, 100,
            nullptr, nullptr, GetModuleHandle(nullptr), this
        );
        
        if (!hwnd_) {
            spdlog::error("Failed to create candidate window: {}", GetLastError());
            return false;
        }
        
        spdlog::info("Windows candidate window created successfully");
        return true;
    }
    
    void destroy() {
        if (hwnd_) {
            DestroyWindow(hwnd_);
            hwnd_ = nullptr;
        }
        
        if (font_) {
            DeleteObject(font_);
            font_ = nullptr;
        }
        
        visible_ = false;
    }
    
    void show(const core::CandidateList& candidates, const CandidateWindowPosition& position) {
        if (!hwnd_) {
            spdlog::error("Candidate window not created");
            return;
        }
        
        candidates_ = candidates;
        selected_index_ = 0;
        
        // 计算窗口大小
        calculateWindowSize();
        
        // 设置窗口位置
        int x = position.x;
        int y = position.y;
        
        // 确保窗口在屏幕范围内
        adjustWindowPosition(x, y);
        
        // 移动并显示窗口
        SetWindowPos(hwnd_, HWND_TOPMOST, x, y, window_width_, window_height_,
                    SWP_NOACTIVATE | SWP_SHOWWINDOW);
        
        visible_ = true;
        
        // 重绘窗口
        InvalidateRect(hwnd_, nullptr, TRUE);
        
        spdlog::debug("Candidate window shown with {} candidates at ({}, {})", 
                     candidates.candidates.size(), x, y);
    }
    
    void hide() {
        if (hwnd_ && visible_) {
            ShowWindow(hwnd_, SW_HIDE);
            visible_ = false;
            spdlog::debug("Candidate window hidden");
        }
    }
    
    void updateSelection(int selected_index) {
        if (selected_index >= 0 && selected_index < static_cast<int>(candidates_.candidates.size())) {
            selected_index_ = selected_index;
            
            if (visible_) {
                InvalidateRect(hwnd_, nullptr, TRUE);
            }
            
            spdlog::debug("Candidate selection updated to index {}", selected_index);
        }
    }
    
    void setWindowStyle(const std::map<std::string, std::string>& style) {
        // 解析样式配置
        auto it = style.find("background_color");
        if (it != style.end()) {
            background_color_ = parseColor(it->second);
        }
        
        it = style.find("text_color");
        if (it != style.end()) {
            text_color_ = parseColor(it->second);
        }
        
        it = style.find("selected_color");
        if (it != style.end()) {
            selected_color_ = parseColor(it->second);
        }
        
        it = style.find("selected_text_color");
        if (it != style.end()) {
            selected_text_color_ = parseColor(it->second);
        }
        
        it = style.find("border_color");
        if (it != style.end()) {
            border_color_ = parseColor(it->second);
        }
        
        it = style.find("font_size");
        if (it != style.end()) {
            try {
                int font_size = std::stoi(it->second);
                if (font_size > 0 && font_size <= 72) {
                    font_size_ = font_size;
                    createFont(); // 重新创建字体
                }
            } catch (const std::exception&) {
                spdlog::warn("Invalid font size: {}", it->second);
            }
        }
        
        // 如果窗口可见，重绘
        if (visible_) {
            InvalidateRect(hwnd_, nullptr, TRUE);
        }
    }
    
    void onPaint() {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd_, &ps);
        
        if (!hdc) {
            return;
        }
        
        // 设置背景模式
        SetBkMode(hdc, TRANSPARENT);
        
        // 选择字体
        HFONT old_font = static_cast<HFONT>(SelectObject(hdc, font_));
        
        // 填充背景
        HBRUSH bg_brush = CreateSolidBrush(background_color_);
        FillRect(hdc, &ps.rcPaint, bg_brush);
        DeleteObject(bg_brush);
        
        // 绘制边框
        HPEN border_pen = CreatePen(PS_SOLID, 1, border_color_);
        HPEN old_pen = static_cast<HPEN>(SelectObject(hdc, border_pen));
        
        RECT window_rect;
        GetClientRect(hwnd_, &window_rect);
        Rectangle(hdc, window_rect.left, window_rect.top, window_rect.right, window_rect.bottom);
        
        SelectObject(hdc, old_pen);
        DeleteObject(border_pen);
        
        // 绘制候选项
        int y = margin_;
        for (size_t i = 0; i < candidates_.candidates.size(); ++i) {
            const auto& candidate = candidates_.candidates[i];
            
            RECT item_rect = {
                margin_,
                y,
                window_width_ - margin_,
                y + item_height_
            };
            
            // 绘制选中背景
            if (static_cast<int>(i) == selected_index_) {
                HBRUSH selected_brush = CreateSolidBrush(selected_color_);
                FillRect(hdc, &item_rect, selected_brush);
                DeleteObject(selected_brush);
                
                SetTextColor(hdc, selected_text_color_);
            } else {
                SetTextColor(hdc, text_color_);
            }
            
            // 绘制候选文本
            std::wstring display_text = formatCandidateText(i + 1, candidate);
            
            RECT text_rect = item_rect;
            text_rect.left += 5;
            text_rect.right -= 5;
            
            DrawTextW(hdc, display_text.c_str(), -1, &text_rect, 
                     DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
            
            y += item_height_;
        }
        
        // 恢复字体
        SelectObject(hdc, old_font);
        
        EndPaint(hwnd_, &ps);
    }
    
    void onMouseClick(int x, int y) {
        // 计算点击的候选项索引
        int item_index = (y - margin_) / item_height_;
        
        if (item_index >= 0 && item_index < static_cast<int>(candidates_.candidates.size())) {
            selected_index_ = item_index;
            
            // 通知选择变化
            if (selection_callback_) {
                selection_callback_(selected_index_);
            }
            
            // 重绘
            InvalidateRect(hwnd_, nullptr, TRUE);
            
            spdlog::debug("Candidate {} clicked", item_index);
        }
    }
    
    void onMouseMove(int x, int y) {
        // 计算鼠标悬停的候选项索引
        int item_index = (y - margin_) / item_height_;
        
        if (item_index >= 0 && item_index < static_cast<int>(candidates_.candidates.size()) &&
            item_index != selected_index_) {
            selected_index_ = item_index;
            InvalidateRect(hwnd_, nullptr, TRUE);
        }
    }
    
    void setSelectionCallback(std::function<void(int)> callback) {
        selection_callback_ = callback;
    }
    
private:
    bool registerWindowClass() {
        WNDCLASSEXW wc = {0};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = CandidateWindowProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH));
        wc.lpszClassName = CANDIDATE_WINDOW_CLASS;
        
        ATOM result = RegisterClassExW(&wc);
        if (result == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
            return false;
        }
        
        return true;
    }
    
    void createFont() {
        if (font_) {
            DeleteObject(font_);
        }
        
        font_ = CreateFontW(
            -font_size_,                    // 字体高度
            0,                              // 字体宽度
            0,                              // 角度
            0,                              // 基线角度
            FW_NORMAL,                      // 字体粗细
            FALSE,                          // 斜体
            FALSE,                          // 下划线
            FALSE,                          // 删除线
            DEFAULT_CHARSET,                // 字符集
            OUT_DEFAULT_PRECIS,             // 输出精度
            CLIP_DEFAULT_PRECIS,            // 裁剪精度
            DEFAULT_QUALITY,                // 输出质量
            DEFAULT_PITCH | FF_DONTCARE,    // 字体间距和族
            L"Microsoft YaHei"              // 字体名称
        );
    }
    
    void calculateWindowSize() {
        if (candidates_.candidates.empty()) {
            window_width_ = 100;
            window_height_ = 50;
            return;
        }
        
        // 计算最大文本宽度
        HDC hdc = GetDC(hwnd_);
        HFONT old_font = static_cast<HFONT>(SelectObject(hdc, font_));
        
        int max_width = 0;
        for (size_t i = 0; i < candidates_.candidates.size(); ++i) {
            std::wstring text = formatCandidateText(i + 1, candidates_.candidates[i]);
            
            SIZE text_size;
            GetTextExtentPoint32W(hdc, text.c_str(), text.length(), &text_size);
            max_width = std::max(max_width, static_cast<int>(text_size.cx));
        }
        
        SelectObject(hdc, old_font);
        ReleaseDC(hwnd_, hdc);
        
        // 设置窗口大小
        window_width_ = max_width + margin_ * 2 + 10; // 额外的边距
        window_height_ = static_cast<int>(candidates_.candidates.size()) * item_height_ + margin_ * 2;
        
        // 限制最大尺寸
        window_width_ = std::min(window_width_, 400);
        window_height_ = std::min(window_height_, 300);
    }
    
    void adjustWindowPosition(int& x, int& y) {
        // 获取屏幕尺寸
        int screen_width = GetSystemMetrics(SM_CXSCREEN);
        int screen_height = GetSystemMetrics(SM_CYSCREEN);
        
        // 确保窗口不超出屏幕边界
        if (x + window_width_ > screen_width) {
            x = screen_width - window_width_;
        }
        if (x < 0) {
            x = 0;
        }
        
        if (y + window_height_ > screen_height) {
            y = y - window_height_ - 30; // 显示在光标上方
        }
        if (y < 0) {
            y = 0;
        }
    }
    
    std::wstring formatCandidateText(int index, const core::Candidate& candidate) {
        std::wstringstream ss;
        ss << index << L". " << utf8ToWstring(candidate.text);
        
        if (!candidate.comment.empty()) {
            ss << L" (" << utf8ToWstring(candidate.comment) << L")";
        }
        
        return ss.str();
    }
    
    COLORREF parseColor(const std::string& color_str) {
        // 支持 #RRGGBB 格式
        if (color_str.length() == 7 && color_str[0] == '#') {
            try {
                unsigned long color = std::stoul(color_str.substr(1), nullptr, 16);
                return RGB((color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF);
            } catch (const std::exception&) {
                spdlog::warn("Invalid color format: {}", color_str);
            }
        }
        
        // 默认颜色
        return RGB(0, 0, 0);
    }
    
public:
    HWND hwnd_;
    bool visible_;
    core::CandidateList candidates_;
    int selected_index_;
    
    int window_width_;
    int window_height_;
    int item_height_;
    int margin_;
    int font_size_ = 14;
    
    HFONT font_;
    COLORREF background_color_;
    COLORREF text_color_;
    COLORREF selected_color_;
    COLORREF selected_text_color_;
    COLORREF border_color_;
    
    std::function<void(int)> selection_callback_;
};

// WindowsCandidateWindow 实现
WindowsCandidateWindow::WindowsCandidateWindow()
    : pImpl(std::make_unique<Impl>()) {
}

WindowsCandidateWindow::~WindowsCandidateWindow() = default;

bool WindowsCandidateWindow::create() {
    return pImpl->create();
}

void WindowsCandidateWindow::destroy() {
    pImpl->destroy();
}

void WindowsCandidateWindow::show(const core::CandidateList& candidates,
                                const CandidateWindowPosition& position) {
    pImpl->show(candidates, position);
}

void WindowsCandidateWindow::hide() {
    pImpl->hide();
}

void WindowsCandidateWindow::updateSelection(int selected_index) {
    pImpl->updateSelection(selected_index);
}

void WindowsCandidateWindow::setWindowStyle(const std::map<std::string, std::string>& style) {
    pImpl->setWindowStyle(style);
}

void WindowsCandidateWindow::onPaint() {
    pImpl->onPaint();
}

void WindowsCandidateWindow::onMouseClick(int x, int y) {
    pImpl->onMouseClick(x, y);
}

void WindowsCandidateWindow::onMouseMove(int x, int y) {
    pImpl->onMouseMove(x, y);
}

void WindowsCandidateWindow::setSelectionCallback(std::function<void(int)> callback) {
    pImpl->setSelectionCallback(callback);
}

} // namespace windows
} // namespace platform
} // namespace owcat

#endif // _WIN32