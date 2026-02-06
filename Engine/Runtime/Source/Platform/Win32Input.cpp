#include "HorseEngine/Core/Input.h"
#include "HorseEngine/Core/Logging.h"
#include <string>
#include <unordered_map>
#include <windows.h>

namespace Horse {

static std::unordered_map<int, bool> s_KeyStates;
static std::unordered_map<int, bool> s_MouseStates;
static float s_MouseX = 0.0f;
static float s_MouseY = 0.0f;

static std::unordered_map<std::string, ActionMapping> s_Actions;
static std::unordered_map<std::string, AxisMapping> s_Axes;

static CursorMode s_CursorMode = CursorMode::Normal;
static float s_LastRawX = 0.0f;
static float s_LastRawY = 0.0f;
static bool s_FirstMove = true;

bool Input::IsKeyPressed(int keycode) {
  auto it = s_KeyStates.find(keycode);
  bool pressed = (it != s_KeyStates.end()) ? it->second : false;

  return pressed;
}

bool Input::IsMouseButtonPressed(int button) {
  auto it = s_MouseStates.find(button);
  return (it != s_MouseStates.end()) ? it->second : false;
}

MousePosition Input::GetMousePosition() { return {s_MouseX, s_MouseY}; }

void Input::RegisterAction(const std::string &name,
                           const std::vector<int> &keys) {
  HORSE_LOG_CORE_INFO("Input: RegisterAction: {} (ActionsAddr: {})", name,
                      (void *)&s_Actions);
  s_Actions[name] = {name, keys};
}

void Input::RegisterAxis(const std::string &name,
                         const std::vector<int> &positiveKeys,
                         const std::vector<int> &negativeKeys) {
  HORSE_LOG_CORE_INFO("Input: RegisterAxis: {} (AxesAddr: {})", name,
                      (void *)&s_Axes);
  s_Axes[name] = {name, positiveKeys, negativeKeys};
}

bool Input::IsActionPressed(const std::string &name) {
  auto it = s_Actions.find(name);
  if (it == s_Actions.end())
    return false;
  for (int key : it->second.Keys) {
    if (IsKeyPressed(key))
      return true;
  }
  return false;
}

float Input::GetAxisValue(const std::string &name) {
  auto it = s_Axes.find(name);
  if (it == s_Axes.end())
    return 0.0f;
  float value = 0.0f;
  for (int key : it->second.PositiveKeys) {
    if (IsKeyPressed(key))
      value += 1.0f;
  }
  for (int key : it->second.NegativeKeys) {
    if (IsKeyPressed(key))
      value -= 1.0f;
  }
  return value;
}

void Input::UpdateKeyState(int keycode, bool pressed) {
  s_KeyStates[keycode] = pressed;
}

void Input::UpdateMousePosition(float x, float y, bool isSnap) {
  if (isSnap) {
    s_LastRawX = x;
    s_LastRawY = y;
    return;
  }

  if (s_FirstMove) {
    s_LastRawX = x;
    s_LastRawY = y;
    s_FirstMove = false;
  }

  if (s_CursorMode == CursorMode::Locked) {
    float dx = x - s_LastRawX;
    float dy = y - s_LastRawY;

    s_MouseX += dx;
    s_MouseY += dy;
  } else {
    s_MouseX = x;
    s_MouseY = y;
  }

  s_LastRawX = x;
  s_LastRawY = y;
}

void Input::UpdateMouseButtonState(int button, bool pressed) {
  s_MouseStates[button] = pressed;
}

void Input::SetCursorMode(CursorMode mode) {
  s_CursorMode = mode;
  s_FirstMove = true;

  if (mode == CursorMode::Locked) {
    // Hide cursor
    while (ShowCursor(FALSE) >= 0)
      ;

    // Lock to window
    HWND hwnd = GetActiveWindow();
    if (hwnd) {
      RECT rect;
      GetWindowRect(hwnd, &rect);
      ClipCursor(&rect);
    }
  } else if (mode == CursorMode::Hidden) {
    while (ShowCursor(FALSE) >= 0)
      ;
    ClipCursor(nullptr);
  } else {
    // Normal
    while (ShowCursor(TRUE) < 0)
      ;
    ClipCursor(nullptr);
  }
}

CursorMode Input::GetCursorMode() { return s_CursorMode; }

} // namespace Horse
