#include "ThemeManager.h"
#include <QApplication>
#include <QPalette>
#include <QStyleFactory>

namespace Horse {

void ThemeManager::ApplyTheme(AppearanceMode mode) {
  if (mode == AppearanceMode::Light) {
    qApp->setStyleSheet("");
    qApp->setPalette(qApp->style()->standardPalette());
    return;
  }

  // Common Dark Palette setup - used for both Dark and DarkModern
  QPalette darkPalette;
  darkPalette.setColor(QPalette::Window, QColor(30, 30, 30));
  darkPalette.setColor(QPalette::WindowText, QColor(212, 212, 212));
  darkPalette.setColor(QPalette::Base, QColor(25, 25, 26));
  darkPalette.setColor(QPalette::AlternateBase, QColor(30, 30, 30));
  darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
  darkPalette.setColor(QPalette::ToolTipText, Qt::white);
  darkPalette.setColor(QPalette::Text, QColor(212, 212, 212));
  darkPalette.setColor(QPalette::Button, QColor(51, 51, 51));
  darkPalette.setColor(QPalette::ButtonText, QColor(212, 212, 212));
  darkPalette.setColor(QPalette::BrightText, Qt::red);
  darkPalette.setColor(QPalette::Link, QColor(0, 122, 204));
  darkPalette.setColor(QPalette::Highlight, QColor(9, 71, 113));
  darkPalette.setColor(QPalette::HighlightedText, Qt::white);

  // Disable "inactive" states showing white/grey by forcing palette for all
  // groups
  darkPalette.setColor(QPalette::Disabled, QPalette::Window,
                       QColor(30, 30, 30));
  darkPalette.setColor(QPalette::Disabled, QPalette::WindowText,
                       QColor(100, 100, 100));
  darkPalette.setColor(QPalette::Disabled, QPalette::Base, QColor(25, 25, 26));
  darkPalette.setColor(QPalette::Disabled, QPalette::Text,
                       QColor(100, 100, 100));
  darkPalette.setColor(QPalette::Disabled, QPalette::Button,
                       QColor(45, 45, 45));
  darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText,
                       QColor(100, 100, 100));

  qApp->setPalette(darkPalette);

  if (mode == AppearanceMode::Dark) {
    // Strictly color-focused QSS for "Classic" Dark
    // Only targeting elements where the palette is ignored by Qt's native
    // styles
    QString classicDarkStyle = R"(
        QHeaderView::section {
            background-color: #2d2d2d;
            color: #d4d4d4;
        }

        QTabBar::tab {
            background-color: #2d2d2d;
            color: #d4d4d4;
        }

        QTabBar::tab:selected {
            background-color: #1e1e1e;
        }

        QDockWidget::title {
            background-color: #2d2d2d;
            color: #d4d4d4;
        }
    )";
    qApp->setStyleSheet(classicDarkStyle);
    return;
  }

  // Dark Modern Theme - Full custom styling (structural changes)
  QString darkModernStyle = R"(
        QMainWindow, QDialog, QDockWidget {
            background-color: #1e1e1e;
            color: #d4d4d4;
        }

        QWidget {
            background-color: #1e1e1e;
            color: #d4d4d4;
            font-family: "Segoe UI", "Tahoma", sans-serif;
            font-size: 10pt;
        }

        QMenuBar {
            background-color: #252526;
            color: #d4d4d4;
            border-bottom: 1px solid #333333;
        }

        QMenuBar::item:selected {
            background-color: #3e3e3e;
        }

        QMenu {
            background-color: #252526;
            color: #d4d4d4;
            border: 1px solid #454545;
        }

        QMenu::item:selected {
            background-color: #094771;
        }

        QToolBar {
            background-color: #252526;
            border-bottom: 1px solid #333333;
            spacing: 5px;
            padding: 2px;
        }

        QDockWidget::title {
            background-color: #2d2d2d;
            text-align: left;
            padding-left: 10px;
        }

        QGroupBox {
            border: 1px solid #333333;
            margin-top: 1.5ex;
            font-weight: bold;
            border-radius: 4px;
            padding: 10px;
        }

        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            padding: 0 3px;
        }

        QPushButton {
            background-color: #333333;
            border: 1px solid #454545;
            padding: 5px 15px;
            border-radius: 3px;
        }

        QPushButton:hover {
            background-color: #3e3e3e;
        }

        QPushButton:pressed {
            background-color: #094771;
        }

        QLineEdit, QDoubleSpinBox, QComboBox {
            background-color: #2d2d2d;
            border: 1px solid #3e3e3e;
            padding: 3px;
            border-radius: 2px;
            selection-background-color: #094771;
        }

        QLineEdit:focus, QDoubleSpinBox:focus, QComboBox:focus {
            border: 1px solid #007acc;
        }

        QTreeWidget, QListWidget, QTableWidget {
            background-color: #252526;
            border: 1px solid #333333;
            outline: 0;
        }

        QTreeWidget::item:selected {
            background-color: #094771;
        }

        QHeaderView::section {
            background-color: #2d2d2d;
            color: #d4d4d4;
            padding: 4px;
            border: 1px solid #333333;
        }

        QScrollBar:vertical {
            background: #2d2d2d;
            width: 12px;
            margin: 0px;
        }

        QScrollBar::handle:vertical {
            background: #424242;
            min-height: 20px;
            border-radius: 6px;
            margin: 2px;
        }

        QScrollBar::handle:vertical:hover {
            background: #505050;
        }

        QTabWidget::pane {
            border: 1px solid #333333;
        }

        QTabBar::tab {
            background: #2d2d2d;
            padding: 8px 15px;
            margin-right: 2px;
        }

        QTabBar::tab:selected {
            background: #1e1e1e;
            border-bottom: 2px solid #007acc;
        }
    )";

  qApp->setStyleSheet(darkModernStyle);
}

} // namespace Horse
