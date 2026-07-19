#include "gui/firmware/IntegrityChecker.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>

IntegrityChecker::IntegrityChecker(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

void IntegrityChecker::verify(const QString &path)
{
    Q_UNUSED(path)
    bool valid = false;
    QString details;
    emit verificationComplete(valid, details);
}

void IntegrityChecker::setupUi()
{
    auto *layout = new QVBoxLayout(this);

    auto *form = new QVBoxLayout();
    m_statusLabel = new QLabel(tr("Status: Not verified"), this);
    m_hashLabel = new QLabel(tr("Hash: -"), this);
    m_signatureLabel = new QLabel(tr("Signature: -"), this);

    form->addWidget(m_statusLabel);
    form->addWidget(m_hashLabel);
    form->addWidget(m_signatureLabel);
    layout->addLayout(form);

    m_verifyBtn = new QPushButton(tr("Verify Integrity"), this);
    layout->addWidget(m_verifyBtn);

    connect(m_verifyBtn, &QPushButton::clicked, this, [this]() {
        verify(QString());
    });
}
