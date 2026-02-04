#include "HorseEngine/Platform/Win32Window.h"
#include "HorseEngine/Core/Logging.h"
#include <windowsx.h>

namespace Horse {

static const wchar_t* WINDOW_CLASS_NAME = L"HorseEngineWindowClass";

Window::Window(const WindowDesc& desc)
    : m_Width(desc.Width), m_Height(desc.Height), m_VSync(desc.VSync) {
    
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = WINDOW_CLASS_NAME;
    
    RegisterClassExW(&wc);
    
    DWORD style = WS_OVERLAPPEDWINDOW;
    if (!desc.Resizable) {
        style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    }
    
    RECT windowRect = {0, 0, static_cast<LONG>(desc.Width), static_cast<LONG>(desc.Height)};
    AdjustWindowRect(&windowRect, style, FALSE);
    
    m_Hwnd = CreateWindowExW(
        0,
        WINDOW_CLASS_NAME,
        desc.Title.c_str(),
        style,
        CW_USEDEFAULT, CW_USEDEFAULT,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        nullptr,
        nullptr,
        GetModuleHandle(nullptr),
        this
    );
    
    if (!m_Hwnd) {
        HORSE_LOG_CORE_ERROR("Failed to create window");
        return;
    }
    
    ShowWindow(m_Hwnd, SW_SHOW);
    UpdateWindow(m_Hwnd);
    
    HORSE_LOG_CORE_INFO("Window created: {}x{}", m_Width, m_Height);
}

Window::~Window() {
    if (m_Hwnd) {
        DestroyWindow(m_Hwnd);
        m_Hwnd = nullptr;
    }
    
    UnregisterClassW(WINDOW_CLASS_NAME, GetModuleHandle(nullptr));
}

bool Window::PollEvents() {
    MSG msg = {};
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            m_ShouldClose = true;
            return false;
        }
        
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    
    return !m_ShouldClose;
}

LRESULT CALLBACK Window::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    Window* window = nullptr;
    
    if (msg == WM_NCCREATE) {
        CREATESTRUCTW* pCreate = reinterpret_cast<CREATESTRUCTW*>(lParam);
        window = static_cast<Window*>(pCreate->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
    } else {
        window = reinterpret_cast<Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }
    
    if (!window) {
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    
    switch (msg) {
        case WM_CLOSE:
            window->m_ShouldClose = true;
            return 0;
            
        case WM_SIZE:
        {
            u32 width = LOWORD(lParam);
            u32 height = HIWORD(lParam);
            window->OnResize(width, height);
            return 0;
        }
        
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void Window::OnResize(u32 width, u32 height) {
    if (width != m_Width || height != m_Height) {
        m_Width = width;
        m_Height = height;
        HORSE_LOG_CORE_INFO("Window resized: {}x{}", m_Width, m_Height);
    }
}

} // namespace Horse
