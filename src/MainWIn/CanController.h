#pragma once
// #include "ui_CanController.h"
#include "QtWidgets.h"

class SettingPage;
class CalibrationPage;

class CanController : public QWidget {
    Q_OBJECT
    
public:
    CanController(QWidget* parent = nullptr);
    ~CanController();

private:
    void InitWindow();
    void CreateTabButtons();

private:
    QVBoxLayout* main_layout_ = nullptr;
    QStackedWidget* stacked_widget_ = nullptr;
    SettingPage* setting_page_ = nullptr;
    CalibrationPage* calibration_page_ = nullptr;
};