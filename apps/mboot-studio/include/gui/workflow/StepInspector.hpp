#pragma once

#include <QWidget>
#include <QLabel>
#include <QTextEdit>
#include <memory>

class StepInspector : public QWidget {
    Q_OBJECT
public:
    explicit StepInspector(QWidget *parent = nullptr);
    void inspectStep(const QVariantMap &step);
    void clear();

private:
    void setupUi();
    QLabel *m_nameLabel, *m_typeLabel, *m_statusLabel;
    QTextEdit *m_detailsView;
};
