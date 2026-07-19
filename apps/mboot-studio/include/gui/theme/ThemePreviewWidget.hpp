#pragma once

#include <QWidget>
#include <memory>

class ThemePreviewWidget : public QWidget {
    Q_OBJECT
public:
    explicit ThemePreviewWidget(QWidget *parent = nullptr);
    void setThemeName(const QString &name);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QString m_themeName;
};
