#pragma once

#include <QWidget>
#include <QTableWidget>
#include <memory>

class MetadataViewer : public QWidget {
    Q_OBJECT
public:
    explicit MetadataViewer(QWidget *parent = nullptr);
    void setMetadata(const QVariantMap &metadata);
    void clear();

private:
    void setupUi();
    QTableWidget *m_table;
};
