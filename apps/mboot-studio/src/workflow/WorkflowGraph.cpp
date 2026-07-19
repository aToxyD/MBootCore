#include "gui/workflow/WorkflowGraph.hpp"
#include <QWheelEvent>
#include <QGraphicsItem>

WorkflowGraph::WorkflowGraph(QWidget *parent)
    : QGraphicsView(parent)
    , m_scene(new QGraphicsScene(this))
{
    setupScene();
}

void WorkflowGraph::setupScene()
{
    setScene(m_scene);
    setRenderHint(QPainter::Antialiasing);
    setDragMode(QGraphicsView::ScrollHandDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
}

void WorkflowGraph::setSteps(const QVariantList &steps)
{
    Q_UNUSED(steps)
    clear();
    qreal x = 0;
    for (int i = 0; i < steps.size(); ++i) {
        auto *rect = m_scene->addRect(x, 0, 100, 60);
        auto *text = m_scene->addText(QString("Step %1").arg(i));
        text->setPos(x + 10, 20);
        m_stepItems[QString::number(i)] = rect;
        x += 140;
    }
}

void WorkflowGraph::highlightStep(const QString &stepId)
{
    if (auto *item = m_stepItems.value(stepId)) {
        item->setSelected(true);
        centerOn(item);
    }
}

void WorkflowGraph::clear()
{
    m_stepItems.clear();
    m_scene->clear();
}
