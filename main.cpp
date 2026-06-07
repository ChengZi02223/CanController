#include "CanController.h"

#include <QApplication>
#pragma comment(lib, "user32.lib")

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    CanController *w  = new CanController();
    w->show();
    return a.exec();
} 