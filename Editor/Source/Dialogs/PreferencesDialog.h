#pragma once

#include <QDialog>

class QComboBox;

namespace Horse {

class PreferencesDialog : public QDialog {
  Q_OBJECT

public:
  explicit PreferencesDialog(QWidget *parent = nullptr);

private slots:
  void OnAppearanceChanged(int index);

private:
  QComboBox *m_AppearanceCombo;
};

} // namespace Horse
