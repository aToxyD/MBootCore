#include <QTest>
#include <QApplication>
#include "gui/theme/ThemePreviewWidget.hpp"

class ThemeTest : public QObject {
    Q_OBJECT
private slots:
    void testThemePreviewWidget();
};

void ThemeTest::testThemePreviewWidget()
{
    ThemePreviewWidget w;
    w.setThemeName("Dark");
    w.resize(400, 300);
}

QTEST_MAIN(ThemeTest)
#include "theme_test.moc"
