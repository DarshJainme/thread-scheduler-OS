#include "ganttwidget.h"

GanttWidget::GanttWidget(QWidget *parent) : QGraphicsView(parent) {
    scene = new QGraphicsScene(this);
    setScene(scene);
}
