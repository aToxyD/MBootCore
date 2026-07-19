#pragma once

#include <QObject>
#include <QString>
#include <QPalette>
#include <QStyle>
#include <memory>

class ThemeManager : public QObject {
    Q_OBJECT
public:
    enum Theme { Dark, Light, Classic, HighContrast };
    Q_ENUM(Theme)

    explicit ThemeManager(QObject *parent = nullptr);

    void applyTheme(Theme theme);
    void applyTheme(const QString &name);
    void setAccentColor(const QColor &color);
    void setFontScale(qreal scale);

    Theme currentTheme() const { return m_currentTheme; }
    QColor accentColor() const { return m_accentColor; }
    qreal fontScale() const { return m_fontScale; }
    QStringList availableThemes() const;
    QString currentThemeName() const;

    QPalette darkPalette() const;
    QPalette lightPalette() const;
    QPalette classicPalette() const;
    QPalette highContrastPalette() const;

signals:
    void themeApplied(const QString &name);
    void accentColorChanged(const QColor &color);
    void fontScaleChanged(qreal scale);

private:
    void applyPalette(const QPalette &palette);
    void applyStyleSheet(Theme theme);
    QString themeStylesheet(Theme theme) const;
    QString baseStylesheet() const;

    Theme m_currentTheme{Dark};
    QColor m_accentColor{QColor("#00aaff")};
    qreal m_fontScale{1.0};
};
