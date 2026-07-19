#pragma once

#include <QWidget>
#include <QSplitter>
#include <memory>

class GPTViewer;
class PartitionEditor;
class QPushButton;
class QLabel;

class PartitionTableWidget : public QWidget {
    Q_OBJECT
public:
    explicit PartitionTableWidget(QWidget *parent = nullptr);
    void loadTable(const QString &path);
    void saveTable(const QString &path);

signals:
    void backupRequested();
    void restoreRequested();
    void compareRequested();

private:
    void setupUi();
    QSplitter *m_splitter;
    GPTViewer *m_gptViewer;
    PartitionEditor *m_editor;
    QPushButton *m_backupBtn, *m_restoreBtn, *m_compareBtn, *m_refreshBtn;
};
