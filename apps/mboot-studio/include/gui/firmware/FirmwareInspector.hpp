#pragma once

#include <QWidget>
#include <QLabel>
#include <QTabWidget>
#include <memory>

class FirmwareInspector : public QWidget {
    Q_OBJECT
public:
    explicit FirmwareInspector(QWidget *parent = nullptr);
    void inspect(const QString &path);

private:
    void setupUi();
    QLabel *m_nameLabel, *m_versionLabel, *m_sizeLabel, *m_typeLabel;
    QLabel *m_hashLabel, *m_signatureLabel, *m_compressionLabel;
    QTabWidget *m_tabs;
};
