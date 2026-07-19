#pragma once

#include <QWidget>
#include <QTreeView>
#include <memory>

class DependencyViewer : public QWidget {
    Q_OBJECT
public:
    explicit DependencyViewer(QWidget *parent = nullptr);
    void setDependencies(const QVariantMap &deps);
    void clear();

private:
    void setupUi();
    QTreeView *m_treeView;
};
