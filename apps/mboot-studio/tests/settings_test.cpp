#include <QTest>
#include <QApplication>
#include "gui/settings/SettingsDialog.hpp"

class SettingsTest : public QObject {
    Q_OBJECT
private slots:
    void testSettingsDialog();
};

void SettingsTest::testSettingsDialog()
{
    SettingsDialog dlg;
    dlg.loadSettings();
    dlg.saveSettings();
}

QTEST_MAIN(SettingsTest)
#include "settings_test.moc"
