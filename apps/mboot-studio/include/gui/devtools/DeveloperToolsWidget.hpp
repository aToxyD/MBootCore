#pragma once

#include <QWidget>
#include <QTabWidget>
#include <memory>

class QPushButton;

class DeveloperToolsWidget : public QWidget {
    Q_OBJECT
public:
    explicit DeveloperToolsWidget(QWidget *parent = nullptr);

private:
    void setupUi();
    QTabWidget *m_tabs;
    QPushButton *m_refreshBtn;
};
