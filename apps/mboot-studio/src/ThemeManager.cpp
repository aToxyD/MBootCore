#include "gui/framework/ThemeManager.hpp"
#include <QApplication>
#include <QStyleFactory>
#include <QFile>
#include <QPalette>
#include <QFont>

ThemeManager::ThemeManager(QObject *parent) : QObject(parent) {}

void ThemeManager::applyTheme(Theme theme)
{
    m_currentTheme = theme;
    QPalette palette;
    switch (theme) {
    case Dark: palette = darkPalette(); break;
    case Light: palette = lightPalette(); break;
    case Classic: palette = classicPalette(); break;
    case HighContrast: palette = highContrastPalette(); break;
    }
    applyPalette(palette);
    applyStyleSheet(theme);
    emit themeApplied(currentThemeName());
}

void ThemeManager::applyTheme(const QString &name)
{
    QString n = name.trimmed().toLower();
    if (n == "dark") applyTheme(Dark);
    else if (n == "light") applyTheme(Light);
    else if (n == "classic") applyTheme(Classic);
    else if (n == "high contrast" || n == "highcontrast" || n == "high-contrast") applyTheme(HighContrast);
}

void ThemeManager::setAccentColor(const QColor &color)
{
    m_accentColor = color;
    applyTheme(m_currentTheme);
    emit accentColorChanged(color);
}

void ThemeManager::setFontScale(qreal scale)
{
    m_fontScale = scale;
    QFont defaultFont = QApplication::font();
    defaultFont.setPointSizeF(defaultFont.pointSizeF() * scale);
    QApplication::setFont(defaultFont);
    emit fontScaleChanged(scale);
}

QStringList ThemeManager::availableThemes() const
{
    return {"Dark", "Light", "Classic", "High Contrast"};
}

QString ThemeManager::currentThemeName() const
{
    switch (m_currentTheme) {
    case Dark: return "Dark";
    case Light: return "Light";
    case Classic: return "Classic";
    case HighContrast: return "High Contrast";
    }
    return "Dark";
}

QPalette ThemeManager::darkPalette() const
{
    QPalette p;
    p.setColor(QPalette::Window, QColor(30, 30, 30));
    p.setColor(QPalette::WindowText, QColor(208, 208, 208));
    p.setColor(QPalette::Base, QColor(42, 42, 42));
    p.setColor(QPalette::AlternateBase, QColor(50, 50, 50));
    p.setColor(QPalette::ToolTipBase, QColor(42, 42, 42));
    p.setColor(QPalette::ToolTipText, QColor(208, 208, 208));
    p.setColor(QPalette::Text, QColor(208, 208, 208));
    p.setColor(QPalette::Button, QColor(50, 50, 50));
    p.setColor(QPalette::ButtonText, QColor(208, 208, 208));
    p.setColor(QPalette::BrightText, Qt::red);
    p.setColor(QPalette::Link, m_accentColor);
    p.setColor(QPalette::Highlight, m_accentColor);
    p.setColor(QPalette::HighlightedText, Qt::white);
    p.setColor(QPalette::Disabled, QPalette::Text, QColor(128, 128, 128));
    p.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(128, 128, 128));
    return p;
}

QPalette ThemeManager::lightPalette() const
{
    QPalette p;
    p.setColor(QPalette::Window, QColor(240, 240, 240));
    p.setColor(QPalette::WindowText, QColor(30, 30, 30));
    p.setColor(QPalette::Base, QColor(255, 255, 255));
    p.setColor(QPalette::AlternateBase, QColor(245, 245, 245));
    p.setColor(QPalette::ToolTipBase, QColor(255, 255, 255));
    p.setColor(QPalette::ToolTipText, QColor(30, 30, 30));
    p.setColor(QPalette::Text, QColor(30, 30, 30));
    p.setColor(QPalette::Button, QColor(240, 240, 240));
    p.setColor(QPalette::ButtonText, QColor(30, 30, 30));
    p.setColor(QPalette::BrightText, Qt::red);
    p.setColor(QPalette::Link, m_accentColor);
    p.setColor(QPalette::Highlight, m_accentColor);
    p.setColor(QPalette::HighlightedText, Qt::white);
    return p;
}

QPalette ThemeManager::classicPalette() const
{
    return QApplication::style()->standardPalette();
}

QPalette ThemeManager::highContrastPalette() const
{
    QPalette p;
    p.setColor(QPalette::Window, Qt::black);
    p.setColor(QPalette::WindowText, Qt::white);
    p.setColor(QPalette::Base, Qt::black);
    p.setColor(QPalette::AlternateBase, QColor(20, 20, 20));
    p.setColor(QPalette::ToolTipBase, Qt::white);
    p.setColor(QPalette::ToolTipText, Qt::black);
    p.setColor(QPalette::Text, Qt::white);
    p.setColor(QPalette::Button, QColor(40, 40, 40));
    p.setColor(QPalette::ButtonText, Qt::white);
    p.setColor(QPalette::BrightText, Qt::yellow);
    p.setColor(QPalette::Link, QColor(0, 255, 255));
    p.setColor(QPalette::Highlight, Qt::white);
    p.setColor(QPalette::HighlightedText, Qt::black);
    return p;
}

void ThemeManager::applyPalette(const QPalette &palette)
{
    QApplication::setPalette(palette);
}

void ThemeManager::applyStyleSheet(Theme theme)
{
    qApp->setStyleSheet(themeStylesheet(theme));
}

QString ThemeManager::themeStylesheet(Theme theme) const
{
    QString accent = m_accentColor.name();
    QString bg = (theme == Dark) ? "#1e1e1e" : (theme == Light) ? "#f0f0f0" : (theme == HighContrast) ? "#000000" : "#f0f0f0";
    QString fg = (theme == Dark) ? "#d0d0d0" : (theme == Light) ? "#1e1e1e" : (theme == HighContrast) ? "#ffffff" : "#1e1e1e";
    QString base = (theme == Dark) ? "#2a2a2a" : (theme == Light) ? "#ffffff" : (theme == HighContrast) ? "#000000" : "#ffffff";

    return QString(R"(
        QMainWindow { background-color: %1; color: %2; }
        QDockWidget { background-color: %1; color: %2; titlebar-close-icon: url(none); }
        QDockWidget::title { background-color: %3; color: %2; padding: 6px; }
        QMenuBar { background-color: %3; color: %2; border-bottom: 1px solid %4; }
        QMenuBar::item:selected { background-color: %4; }
        QMenu { background-color: %3; color: %2; border: 1px solid %4; }
        QMenu::item:selected { background-color: %4; color: white; }
        QToolBar { background-color: %1; border: none; spacing: 4px; padding: 2px; }
        QStatusBar { background-color: %3; color: %2; border-top: 1px solid %4; }
        QTreeView { background-color: %3; color: %2; alternate-background-color: %1; border: 1px solid %4; }
        QListView { background-color: %3; color: %2; alternate-background-color: %1; border: 1px solid %4; }
        QTableView { background-color: %3; color: %2; alternate-background-color: %1; border: 1px solid %4; }
        QHeaderView::section { background-color: %1; color: %2; padding: 4px; border: 1px solid %4; }
        QTabWidget::pane { background-color: %3; border: 1px solid %4; }
        QTabBar::tab { background-color: %1; color: %2; padding: 8px 16px; border: 1px solid %4; }
        QTabBar::tab:selected { background-color: %3; border-bottom: 2px solid %5; }
        QPushButton { background-color: %3; color: %2; border: 1px solid %4; padding: 6px 16px; border-radius: 4px; }
        QPushButton:hover { background-color: %4; }
        QPushButton:pressed { background-color: %5; }
        QLineEdit { background-color: %3; color: %2; border: 1px solid %4; padding: 4px; border-radius: 3px; }
        QTextEdit { background-color: %3; color: %2; border: 1px solid %4; }
        QPlainTextEdit { background-color: %3; color: %2; border: 1px solid %4; }
        QComboBox { background-color: %3; color: %2; border: 1px solid %4; padding: 4px; }
        QComboBox::drop-down { border: none; }
        QGroupBox { border: 1px solid %4; margin-top: 12px; padding-top: 12px; }
        QGroupBox::title { color: %2; }
        QProgressBar { background-color: %1; border: 1px solid %4; text-align: center; color: %2; }
        QProgressBar::chunk { background-color: %5; }
        QSplitter::handle { background-color: %4; }
        QScrollBar:vertical { background-color: %1; width: 12px; }
        QScrollBar::handle:vertical { background-color: %4; min-height: 20px; border-radius: 4px; }
        QScrollBar:horizontal { background-color: %1; height: 12px; }
        QScrollBar::handle:horizontal { background-color: %4; min-width: 20px; border-radius: 4px; }
        QLabel { color: %2; }
    )").arg(bg, fg, base, QColor(60,60,60).name(), accent);
}

QString ThemeManager::baseStylesheet() const
{
    return themeStylesheet(m_currentTheme);
}
