#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMap>
#include <QTextEdit>
#include <QString>
#include <QVBoxLayout>
#include "ganttwidget.h"
#include "scheduler.h"
#include "threadedscheduler.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_runButton_clicked();
    void on_saveButton_clicked();

private:
    Ui::MainWindow *ui;
    QMap<QString, QTextEdit*> logs_basic, logs_threaded;
    QMap<QString, GanttWidget*> gantts_basic, gantts_threaded;
    
    void createAlgoTab(const QString &name, QTabWidget *parentTabs,
        QMap<QString, QTextEdit*> &logsMap,
        QMap<QString, GanttWidget*> &ganttMap);

    void logToAlgoTab(const QString &name, const QString &message,
       QMap<QString, QTextEdit*> &logsMap);
};

#endif // MAINWINDOW_H
