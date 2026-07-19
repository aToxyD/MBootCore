#include <QTest>
#include <QApplication>
#include "gui/diagnostics/DiagnosticsWidget.hpp"

class DiagnosticsTest : public QObject {
    Q_OBJECT
private slots:
    void testDiagnosticsWidget();
};

void DiagnosticsTest::testDiagnosticsWidget()
{
    DiagnosticsWidget w;
    w.runAll();

    gui::runtime::RuntimeDiagnosticsView diagnostics;
    QString report = DiagnosticsWidget::buildReport(diagnostics);
    QVERIFY(!report.isEmpty());
    QVERIFY(report.contains("MBoot Studio Diagnostics Report"));
}

QTEST_MAIN(DiagnosticsTest)
#include "diagnostics_test.moc"
