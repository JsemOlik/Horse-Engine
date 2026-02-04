#pragma once

#include "HorseEngine/Core.h"
#include <string>
#include <functional>

#include <Windows.h>

namespace Horse {

struct WindowDesc {
    std::wstring Title = L"Horse Engine";
    u32 Width = 1280;
    u32 Height = 720;
    bool Resizable = true;
    bool VSync = true;
};

class Window {
public:
    explicit Window(const WindowDesc& desc);
    ~Window();
    
    bool PollEvents();
    
    HWND GetHandle() const { return m_Hwnd; }
    u32 GetWidth() const { return m_Width; }
    u32 GetHeight() const { return m_Height; }
    bool IsVSyncEnabled() const { return m_VSync; }
    
    void SetVSync(bool enabled) { m_VSync = enabled; }
    
private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void OnResize(u32 width, u32 height);
    
    HWND m_Hwnd = nullptr;
    u32 m_Width = 0;
    u32 m_Height = 0;
    bool m_VSync = true;
    bool m_ShouldClose = false;
};

} // namespace Horse
