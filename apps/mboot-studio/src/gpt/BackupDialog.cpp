#include "gui/gpt/BackupDialog.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QFileDialog>
#include <QLabel>

BackupDialog::BackupDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Backup GPT"));
    auto *layout = new QVBoxLayout(this);

    auto *pathLayout = new QHBoxLayout();
    auto *pathEdit = new QLineEdit(this);
    pathEdit->setPlaceholderText(tr("Backup path..."));
    pathLayout->addWidget(pathEdit);

    auto *browseBtn = new QPushButton(tr("Browse..."), this);
    pathLayout->addWidget(browseBtn);
    layout->addLayout(pathLayout);

    auto *userDataCheck = new QCheckBox(tr("Include user data"), this);
    userDataCheck->setProperty("includeUserData", true);
    layout->addWidget(userDataCheck);

    auto *btnLayout = new QHBoxLayout();
    auto *okBtn = new QPushButton(tr("OK"), this);
    auto *cancelBtn = new QPushButton(tr("Cancel"), this);
    btnLayout->addWidget(okBtn);
    btnLayout->addWidget(cancelBtn);
    layout->addLayout(btnLayout);

    connect(browseBtn, &QPushButton::clicked, this, [this, pathEdit]() {
        QString path = QFileDialog::getSaveFileName(this, tr("Select Backup File"));
        if (!path.isEmpty()) pathEdit->setText(path);
    });
    connect(okBtn, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

QString BackupDialog::backupPath() const
{
    auto *edit = findChild<QLineEdit*>();
    return edit ? edit->text() : QString();
}

bool BackupDialog::includeUserData() const
{
    auto *check = findChild<QCheckBox*>();
    return check && check->isChecked();
}
