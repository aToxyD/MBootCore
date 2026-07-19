#pragma once

#include <QWidget>
#include <QTableWidget>
#include <memory>

#include "DeviceListModel.hpp"

class DevicePropertiesWidget : public QWidget {
    Q_OBJECT
public:
    explicit DevicePropertiesWidget(QWidget *parent = nullptr);
    void showProperties(const DeviceInfo &device);
    void clear();

private:
    void setupUi();
    QTableWidget *m_table;
};
