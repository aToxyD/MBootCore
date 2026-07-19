#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <memory>

class IntegrityChecker : public QWidget {
    Q_OBJECT
public:
    explicit IntegrityChecker(QWidget *parent = nullptr);
    void verify(const QString &path);

signals:
    void verificationComplete(bool valid, const QString &details);

private:
    void setupUi();
    QLabel *m_statusLabel, *m_hashLabel, *m_signatureLabel;
    QPushButton *m_verifyBtn;
};
