#pragma once

#include "../Core.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace Horse {

struct MousePosition {
  float X;
  float Y;
};

enum class CursorMode { Normal = 0, Hidden = 1, Locked = 2 };

enum KeyCode {
  KEY_NONE = 0x00,
  KEY_LBUTTON = 0x01,
  KEY_RBUTTON = 0x02,
  KEY_MBUTTON = 0x04,
  KEY_BACK = 0x08,
  KEY_TAB = 0x09,
  KEY_RETURN = 0x0D,
  KEY_SHIFT = 0x10,
  KEY_CONTROL = 0x11,
  KEY_MENU = 0x12,
  KEY_ESCAPE = 0x1B,
  KEY_SPACE = 0x20,
  KEY_LEFT = 0x25,
  KEY_UP = 0x26,
  KEY_RIGHT = 0x27,
  KEY_DOWN = 0x28,
  KEY_0 = 0x30,
  KEY_1,
  KEY_2,
  KEY_3,
  KEY_4,
  KEY_5,
  KEY_6,
  KEY_7,
  KEY_8,
  KEY_9,
  KEY_A = 0x41,
  KEY_B,
  KEY_C,
  KEY_D,
  KEY_E,
  KEY_F,
  KEY_G,
  KEY_H,
  KEY_I,
  KEY_J,
  KEY_K,
  KEY_L,
  KEY_M,
  KEY_N,
  KEY_O,
  KEY_P,
  KEY_Q,
  KEY_R,
  KEY_S,
  KEY_T,
  KEY_U,
  KEY_V,
  KEY_W,
  KEY_X,
  KEY_Y,
  KEY_Z
};

struct ActionMapping {
  std::string Name;
  std::vector<int> Keys;
};

struct AxisMapping {
  std::string Name;
  std::vector<int> PositiveKeys;
  std::vector<int> NegativeKeys;
};

class HORSE_API Input {
public:
  static bool IsKeyPressed(int keycode);
  static bool IsMouseButtonPressed(int button);
  static MousePosition GetMousePosition();

  static void RegisterAction(const std::string &name,
                             const std::vector<int> &keys);
  static void RegisterAxis(const std::string &name,
                           const std::vector<int> &positiveKeys,
                           const std::vector<int> &negativeKeys);

  static void SetCursorMode(CursorMode mode);
  static CursorMode GetCursorMode();
  static bool IsActionPressed(const std::string &name);
  static float GetAxisValue(const std::string &name);

  // Internal
  static void UpdateKeyState(int keycode, bool pressed);
  static void UpdateMousePosition(float x, float y, bool isSnap = false);
  static void UpdateMouseButtonState(int button, bool pressed);
};

} // namespace Horse
