#pragma once

#include <QWidget>
#include <QTreeView>
#include <memory>

class FlashPlanViewer : public QWidget {
    Q_OBJECT
public:
    explicit FlashPlanViewer(QWidget *parent = nullptr);
    void setPlan(const QVariantMap &plan);
    void clear();

private:
    void setupUi();
    QTreeView *m_treeView;
};
