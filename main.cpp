#include "CanController.h"

#include <QApplication>
#include <QFile>
#pragma comment(lib, "user32.lib")

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QFile file(":/style.qss");
    file.open(QFile::ReadOnly);
    QString qss = file.readAll();
    file.close();
    a.setStyleSheet(qss);

    CanController *w  = new CanController();
    w->show();
    return a.exec();
} 