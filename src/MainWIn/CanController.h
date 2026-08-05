#pragma once
// #include "ui_CanController.h"
#include "QtWidgets.h"

class SettingPage;
class CalibrationPage;

class CanController : public QWidget {
    Q_OBJECT
    
public:
    static CanController* GetInstance(QWidget* parent = nullptr) {
        static CanController instance(parent);
        return &instance;
    }

private:
    CanController(QWidget* parent = nullptr);
    ~CanController();
    void InitWindow();
    
    void InitData();
    void InitBasicInfo();
    
    void CreateTabButtons();

private:
    QVBoxLayout* main_layout_ = nullptr;
    QStackedWidget* stacked_widget_ = nullptr;
    SettingPage* setting_page_ = nullptr;
    CalibrationPage* calibration_page_ = nullptr;
};