#include <QTest>
#include <QApplication>
#include <QSignalSpy>
#include "gui/gpt/PartitionTableWidget.hpp"
#include "gui/gpt/GPTViewer.hpp"
#include "gui/gpt/PartitionEditor.hpp"
#include "gui/gpt/BackupDialog.hpp"
#include "gui/gpt/RestoreDialog.hpp"
#include "gui/gpt/CompareDialog.hpp"

class GptTest : public QObject {
    Q_OBJECT
private slots:
    void testPartitionTableWidget();
    void testGPTViewer();
    void testPartitionEditor();
    void testBackupDialog();
    void testRestoreDialog();
    void testCompareDialog();
};

void GptTest::testPartitionTableWidget()
{
    PartitionTableWidget w;
    QVERIFY(w.width() > 0);
}

void GptTest::testGPTViewer()
{
    GPTViewer viewer;
    QVariantList parts;
    QVariantMap p; p["name"] = "boot"; p["size"] = 4096;
    parts << p;
    viewer.setPartitions(parts);
}

void GptTest::testPartitionEditor()
{
    PartitionEditor editor;
    QVariantMap p;
    p["name"] = "boot";
    p["guid"] = "{1234-5678}";
    editor.loadPartition(p);
    editor.clear();
}

void GptTest::testBackupDialog()
{
    BackupDialog dlg;
    QVERIFY(dlg.backupPath().isEmpty());
}

void GptTest::testRestoreDialog()
{
    RestoreDialog dlg;
    QVERIFY(dlg.restorePath().isEmpty());
}

void GptTest::testCompareDialog()
{
    CompareDialog dlg;
    dlg.compare("/path/1.bin", "/path/2.bin");
}

QTEST_MAIN(GptTest)
#include "gpt_test.moc"
