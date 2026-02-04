#pragma once

#include <memory>
#include <string>

namespace Horse {

enum class AppearanceMode { Light = 0, Dark = 1, DarkModern = 2 };

class EditorPreferences {
public:
  static EditorPreferences &Get();

  AppearanceMode GetAppearance() const { return m_Appearance; }
  void SetAppearance(AppearanceMode mode);

  void Load();
  void Save();

private:
  EditorPreferences();
  ~EditorPreferences() = default;

  AppearanceMode m_Appearance = AppearanceMode::Light;
};

} // namespace Horse
