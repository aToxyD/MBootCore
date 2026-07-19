#pragma once

#include <QDialog>

class RestoreDialog : public QDialog {
    Q_OBJECT
public:
    explicit RestoreDialog(QWidget *parent = nullptr);
    QString restorePath() const;
    bool overwriteExisting() const;
};
