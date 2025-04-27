// mainwindow.cpp (updated for Basic and Threaded Schedulers tabs)
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QString>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QPainter>
#include <QImage>
#include "scheduler.h"
#include "threadedscheduler.h"
#include "analysis.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::on_runButton_clicked() {
    ui->basicSchedulerTabs->clear();
    ui->threadedSchedulerTabs->clear();

    QStringList basicAlgos = {"FCFS", "RR", "PRIORITY", "SJF", "MLQ", "MLFQ", "EDF", "CFS"};

    QStringList threadedAlgos = {"T_FCFS", "T_RR", "T_PRIORITY", "T_MLFQ", "T_CFS"};

    for (int alg = FCFS; alg <= CFS; ++alg) {
        QString name = basicAlgos[alg];
        createAlgoTab(name, ui->basicSchedulerTabs, logs_basic, gantts_basic);
        
        Scheduler s((Algorithm)alg, 100, [=](const std::string &m) {
            logToAlgoTab(name, QString::fromStdString(m), logs_basic);
        });
        s.run();
        gantts_basic[name]->drawTimeline(s.timeline(), name);
    }
    
    for (int alg = T_FCFS; alg <=T_CFS; ++alg) {
        QString name = threadedAlgos[alg];
        createAlgoTab(name, ui->threadedSchedulerTabs, logs_threaded, gantts_threaded);

        ThreadedScheduler ts((ThreadedAlgorithm)alg, 100, [=](const std::string &m) {
            logToAlgoTab(name, QString::fromStdString(m), logs_threaded);
        });
        ts.run();
        gantts_threaded[name]->drawTimeline(ts.timeline(), name);
    }

    analyzeAlgorithms();
}

void MainWindow::createAlgoTab(const QString &name, QTabWidget *parentTabs,
    QMap<QString, QTextEdit*> &logsMap,
    QMap<QString, GanttWidget*> &ganttMap) {
    QWidget *tab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(tab);
    GanttWidget *gantt = new GanttWidget(this);
    QTextEdit *logBox = new QTextEdit(this);
    logBox->setReadOnly(true);
    layout->addWidget(gantt);
    layout->addWidget(logBox);
    tab->setLayout(layout);

    parentTabs->addTab(tab, name);
    logsMap[name] = logBox;
    ganttMap[name] = gantt;
}


void MainWindow::logToAlgoTab(const QString &name, const QString &message,
    QMap<QString, QTextEdit*> &logsMap) {
    if (logsMap.contains(name)) {
    logsMap[name]->append(message);
    }
}

void MainWindow::on_saveButton_clicked() {
    QString folder = QFileDialog::getExistingDirectory(this, "Select Folder to Save Reports");
    if (folder.isEmpty()) return;

    auto saveGroup = [&](const QMap<QString, QTextEdit*> &logs,
                         const QMap<QString, GanttWidget*> &gantts,
                         const QString &prefix) {
        for (const QString& name : logs.keys()) {
            QFile logFile(folder + "/" + prefix + "_" + name + "_log.txt");
            if (logFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream out(&logFile);
                out << logs[name]->toPlainText();
                logFile.close();
            }

            QPixmap pixmap(gantts[name]->size());
            QPainter painter(&pixmap);
            gantts[name]->render(&painter);
            pixmap.save(folder + "/" + prefix + "_" + name + "_gantt.png");
        }
    };

    saveGroup(logs_basic, gantts_basic, "basic");
    saveGroup(logs_threaded, gantts_threaded, "threaded");
}