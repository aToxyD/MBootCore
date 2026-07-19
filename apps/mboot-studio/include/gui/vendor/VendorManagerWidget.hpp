#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <memory>

class VendorManagerWidget : public QWidget {
    Q_OBJECT
public:
    explicit VendorManagerWidget(QWidget *parent = nullptr);
    void loadVendors();
    void showVendorDetails(const QString &vendorId);

private:
    void setupUi();
    QTableWidget *m_table;
    QPushButton *m_detailsBtn, *m_refreshBtn;
    QLabel *m_statusLabel;
};
