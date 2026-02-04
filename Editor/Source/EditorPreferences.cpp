#include "EditorPreferences.h"
#include <QSettings>

namespace Horse {

EditorPreferences &EditorPreferences::Get() {
  static EditorPreferences instance;
  return instance;
}

EditorPreferences::EditorPreferences() { Load(); }

void EditorPreferences::SetAppearance(AppearanceMode mode) {
  m_Appearance = mode;
  Save();
}

void EditorPreferences::Load() {
  QSettings settings("HorseEngine", "Editor");
  int mode = settings.value("Appearance", 0).toInt();
  m_Appearance = static_cast<AppearanceMode>(mode);
}

void EditorPreferences::Save() {
  QSettings settings("HorseEngine", "Editor");
  settings.setValue("Appearance", static_cast<int>(m_Appearance));
}

} // namespace Horse
