#include "PreferencesDialog.h"
#include "../EditorPreferences.h"
#include "../ThemeManager.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace Horse {

PreferencesDialog::PreferencesDialog(QWidget *parent) : QDialog(parent) {
  setWindowTitle("Preferences");
  resize(400, 300);

  QVBoxLayout *mainLayout = new QVBoxLayout(this);

  // Appearance Setting
  QHBoxLayout *appearanceLayout = new QHBoxLayout();
  appearanceLayout->addWidget(new QLabel("Appearance:"));

  m_AppearanceCombo = new QComboBox();
  m_AppearanceCombo->addItem("Light");
  m_AppearanceCombo->addItem("Dark");
  m_AppearanceCombo->addItem("Dark (Modern)");

  AppearanceMode currentMode = EditorPreferences::Get().GetAppearance();
  m_AppearanceCombo->setCurrentIndex(static_cast<int>(currentMode));

  connect(m_AppearanceCombo,
          QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &PreferencesDialog::OnAppearanceChanged);

  appearanceLayout->addWidget(m_AppearanceCombo);
  mainLayout->addLayout(appearanceLayout);

  mainLayout->addStretch();

  // Close button
  QPushButton *closeButton = new QPushButton("Close");
  connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
  mainLayout->addWidget(closeButton, 0, Qt::AlignRight);
}

void PreferencesDialog::OnAppearanceChanged(int index) {
  AppearanceMode mode = static_cast<AppearanceMode>(index);
  EditorPreferences::Get().SetAppearance(mode);
  ThemeManager::ApplyTheme(mode);
}

} // namespace Horse
