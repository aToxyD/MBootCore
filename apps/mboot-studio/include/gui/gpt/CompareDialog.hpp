#pragma once

#include <QDialog>
#include <QTableWidget>
#include <memory>

class CompareDialog : public QDialog {
    Q_OBJECT
public:
    explicit CompareDialog(QWidget *parent = nullptr);
    void compare(const QString &path1, const QString &path2);

private:
    void setupUi();
    QTableWidget *m_diffTable;
};
