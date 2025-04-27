// main.cpp

#include <windows.h>         
#include <QApplication>
#include <QDebug>
#include "mainwindow.h"

extern LPVOID scheduler_fiber;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    scheduler_fiber = ConvertThreadToFiber(nullptr);
    if (!scheduler_fiber) {
        qFatal("ConvertThreadToFiber failed, error %u", GetLastError());
        return -1;
    }
    MainWindow w;
    w.show();
    int result = a.exec();
    if (!ConvertFiberToThread()) {
        qWarning() << "ConvertFiberToThread failed, error" << GetLastError();
    }

    return result;
}
