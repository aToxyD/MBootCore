#include "gui/framework/AboutDialog.hpp"
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QApplication>

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("About MBoot Studio");
    setFixedSize(400, 300);

    auto *layout = new QVBoxLayout(this);
    auto *title = new QLabel("<h1>MBoot Studio</h1>");
    title->setAlignment(Qt::AlignCenter);
    auto *version = new QLabel(QString("Version %1").arg(QApplication::applicationVersion()));
    version->setAlignment(Qt::AlignCenter);
    auto *desc = new QLabel("Professional BootROM Protocol GUI Platform");
    desc->setAlignment(Qt::AlignCenter);
    desc->setWordWrap(true);
    auto *copyright = new QLabel("Copyright 2026 MBootCore Team");
    copyright->setAlignment(Qt::AlignCenter);
    auto *closeBtn = new QPushButton("Close");
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);

    layout->addStretch();
    layout->addWidget(title);
    layout->addWidget(version);
    layout->addSpacing(10);
    layout->addWidget(desc);
    layout->addSpacing(20);
    layout->addWidget(copyright);
    layout->addStretch();
    layout->addWidget(closeBtn, 0, Qt::AlignCenter);
}
