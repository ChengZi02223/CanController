#include "CanController.h"
#include "CanConfig.h"
#include "MgrUtil.h"

#include <QApplication>
#include <QFile>
#pragma comment(lib, "user32.lib")

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QFile file(":/new_style.qss");
    file.open(QFile::ReadOnly);
    QString qss = file.readAll();
    file.close();
    a.setStyleSheet(qss);

    qRegisterMetaType<can_frame>("can_frame");

    CAN_MGR_START;
    // CanController *w  = new CanController();
    CanController::GetInstance()->show();
    // CanConfigWin::GetInstance()->show();
    return a.exec();
} 