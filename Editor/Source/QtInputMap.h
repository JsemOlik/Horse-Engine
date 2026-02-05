#pragma once

#include "HorseEngine/Core/Input.h"
#include <QKeySequence>


namespace Horse {

inline int QtKeyToHorseKey(int qtKey) {
  switch (qtKey) {
  case Qt::Key_Space:
    return KEY_SPACE;
  case Qt::Key_W:
    return KEY_W;
  case Qt::Key_S:
    return KEY_S;
  case Qt::Key_A:
    return KEY_A;
  case Qt::Key_D:
    return KEY_D;
  case Qt::Key_Q:
    return KEY_Q;
  case Qt::Key_E:
    return KEY_E;
  case Qt::Key_R:
    return KEY_R;
  case Qt::Key_T:
    return KEY_T;
  case Qt::Key_F:
    return KEY_F;
  case Qt::Key_G:
    return KEY_G;
  case Qt::Key_Z:
    return KEY_Z;
  case Qt::Key_X:
    return KEY_X;
  case Qt::Key_C:
    return KEY_C;
  case Qt::Key_V:
    return KEY_V;
  case Qt::Key_Escape:
    return KEY_ESCAPE;
  case Qt::Key_Shift:
    return KEY_SHIFT;
  case Qt::Key_Control:
    return KEY_CONTROL;
  case Qt::Key_Alt:
    return KEY_MENU;
  case Qt::Key_Return:
    return KEY_RETURN;
  case Qt::Key_Tab:
    return KEY_TAB;
  case Qt::Key_Backspace:
    return KEY_BACK;
  case Qt::Key_Left:
    return KEY_LEFT;
  case Qt::Key_Right:
    return KEY_RIGHT;
  case Qt::Key_Up:
    return KEY_UP;
  case Qt::Key_Down:
    return KEY_DOWN;
  // Add more as needed
  default:
    return 0;
  }
}

} // namespace Horse
