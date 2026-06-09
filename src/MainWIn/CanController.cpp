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
    setting_page_btn->setObjectName("SettingPageTabBtn");
    setting_page_btn->setCheckable(true);
    setting_page_btn->setChecked(true);
    auto test_page_btn = new QPushButton("测试标定状态", this);
    test_page_btn->setObjectName("TestPageTabBtn");
    test_page_btn->setCheckable(true);
    test_page_btn->setChecked(false);

    auto btn_layout = new QHBoxLayout();
    btn_layout->addWidget(setting_page_btn);
    btn_layout->addWidget(test_page_btn);
    main_layout_->addLayout(btn_layout);

    auto btn_group = new QButtonGroup(this);
    btn_group->addButton(setting_page_btn);
    btn_group->addButton(test_page_btn);
    btn_group->setExclusive(true); // 开启互斥

    // 2. 点击事件只需要处理页面切换，不用手动取消选中
    connect(setting_page_btn, &QPushButton::clicked, this, [this]() {
        stacked_widget_->setCurrentIndex(0);
    });

    connect(test_page_btn, &QPushButton::clicked, this, [this]() {
        stacked_widget_->setCurrentIndex(1);
    });
}
