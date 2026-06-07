#include "CanController.h"
#include "SettingPage.h"
#include "CalibrationPage.h"

CanController::CanController(QWidget* parent)
    : QWidget(parent) {
    InitWindow();
}

CanController::~CanController() {}

void CanController::InitWindow() {
    main_layout_ = new QVBoxLayout(this);
    CreateTabButtons();
    stacked_widget_ = new QStackedWidget(this);
    setting_page_ = new SettingPage(this);
    calibration_page_ = new CalibrationPage(this);
    stacked_widget_->addWidget(setting_page_);
    stacked_widget_->addWidget(calibration_page_);
    
    main_layout_->addWidget(stacked_widget_);

    connect(setting_page_, &SettingPage::SendInputMode, [this](InputMode mode){
        if(mode == kAuto) {
            stacked_widget_->setCurrentIndex(1);
        }
    });
}

void CanController::CreateTabButtons() {
    auto setting_page_btn = new QPushButton("参数配置状态", this);
    auto test_page_btn = new QPushButton("测试标定状态", this);

    auto btn_layout = new QHBoxLayout();
    btn_layout->addWidget(setting_page_btn);
    btn_layout->addWidget(test_page_btn);
    main_layout_->addLayout(btn_layout);

    connect(setting_page_btn, &QPushButton::clicked, this, [this]() {
        // 切换到参数配置状态
        stacked_widget_->setCurrentIndex(0);
    });
    connect(test_page_btn, &QPushButton::clicked, this, [this]() {
        // 切换到测试标定状态
        stacked_widget_->setCurrentIndex(1);
    });
}
