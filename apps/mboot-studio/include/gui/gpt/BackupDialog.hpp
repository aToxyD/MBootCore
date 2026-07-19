#pragma once

#include <QDialog>

class BackupDialog : public QDialog {
    Q_OBJECT
public:
    explicit BackupDialog(QWidget *parent = nullptr);
    QString backupPath() const;
    bool includeUserData() const;
};
