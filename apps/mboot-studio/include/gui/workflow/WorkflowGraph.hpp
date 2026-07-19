#pragma once

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QMap>

class WorkflowGraph : public QGraphicsView {
    Q_OBJECT
public:
    explicit WorkflowGraph(QWidget *parent = nullptr);
    void setSteps(const QVariantList &steps);
    void highlightStep(const QString &stepId);
    void clear();

signals:
    void stepSelected(const QString &stepId);

private:
    void setupScene();
    QGraphicsScene *m_scene;
    QMap<QString, QGraphicsItem *> m_stepItems;
};
