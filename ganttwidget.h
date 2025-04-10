// ganttwidget.h
#ifndef GANTTWIDGET_H
#define GANTTWIDGET_H

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QGraphicsSimpleTextItem>  
#include <QMap>
#include <QString>
#include <vector>
#include <QPen>
#include <QBrush>

class GanttWidget : public QGraphicsView {
    Q_OBJECT

public:
    GanttWidget(QWidget *parent = nullptr);

    template <typename T>
    void drawTimeline(const std::vector<T> &timeline, const QString &title) {
        scene->clear();
        scene->addText(title)->setPos(0, 0);

        int offsetY = 30, rowH = 25;
        QMap<int, int> rows;
        int nextRow = 0;
        for (auto &e : timeline) {
            if (!rows.contains(e.task_id)) rows[e.task_id] = nextRow++;
            int r = rows[e.task_id];
            int x = e.start_time, w = e.end_time - e.start_time;
            scene->addRect(x, offsetY + r * rowH, w, 20, QPen(), QBrush(Qt::cyan));
            scene->addSimpleText(QString("T%1").arg(e.task_id))->setPos(x + 2, offsetY + r * rowH);
        }
        scene->setSceneRect(0, 0, 800, offsetY + nextRow * rowH + 50);
    }

private:
    QGraphicsScene *scene;
};

#endif // GANTTWIDGET_H