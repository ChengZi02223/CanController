#include "CalibrationPage.h"

#include "BasicInfoBar.h"



CalibrationPage::CalibrationPage(QWidget* parent)
    : QWidget(parent) {

    InitPage();
}

void CalibrationPage::InitPage() {
    main_layout_ = new QHBoxLayout(this);

    auto left_layout = new QVBoxLayout();
    basic_info_bar_ = new BasicInfoBar(kVal, this);
    basic_info_bar_->setEnabled(false);

    left_layout->addWidget(basic_info_bar_);
    left_layout->addWidget(CreateControlArea());
    left_layout->addWidget(CreatePIDSettingArea());
    left_layout->addWidget(CreateDisplacementArea());
    left_layout->addWidget(CreateSignalResponseArea());

    auto right_layout = new QVBoxLayout();
    right_layout->addWidget(CreateWaveformArea());

    main_layout_->addLayout(left_layout);
    main_layout_->addLayout(right_layout);
}

// Implementation for creating control area
QWidget* CalibrationPage::CreateControlArea() {
    auto control_group = new QGroupBox("开环控制指令区", this);

    auto main_layout = new QVBoxLayout(control_group);

    auto output_cycle_1_label = new QLabel("1 侧输出占空比");
    auto output_cycle_2_label = new QLabel("2 侧输出占空比");
    auto cycle_count_label = new QLabel("循环次数");
    auto neutral_time_label = new QLabel("中位停留时间");
    auto work_time_label = new QLabel("工作位停留时间");

    auto output_cycle_1_edit = new QLineEdit();
    output_cycle_1_edit->setMinimumWidth(150);
    auto output_cycle_2_edit = new QLineEdit();
    output_cycle_2_edit->setMinimumWidth(150);
    auto cycle_count_edit = new QLineEdit();
    cycle_count_edit->setMinimumWidth(150);
    auto neutral_time_edit = new QLineEdit();
    neutral_time_edit->setMinimumWidth(150);
    auto work_time_edit = new QLineEdit();
    work_time_edit->setMinimumWidth(150);

    auto control_1_btn = new QPushButton("1 侧开环控制");
    control_1_btn->setMinimumWidth(150);
    auto control_2_btn = new QPushButton("2 侧开环控制");
    control_2_btn->setMinimumWidth(150);
    auto cycle_btn = new QPushButton("开环循环动作");
    cycle_btn->setMinimumWidth(400);

    auto grid_layout = new QGridLayout();

    grid_layout->addWidget(output_cycle_1_label, 0, 0, Qt::AlignCenter);
    grid_layout->addWidget(output_cycle_2_label, 0, 1, Qt::AlignCenter);
    grid_layout->addWidget(cycle_count_label, 0, 2, Qt::AlignCenter);
    grid_layout->addWidget(neutral_time_label, 0, 3, Qt::AlignCenter);
    grid_layout->addWidget(work_time_label, 0, 4, Qt::AlignCenter);
    grid_layout->addWidget(output_cycle_1_edit, 1, 0, Qt::AlignCenter);
    grid_layout->addWidget(output_cycle_2_edit, 1, 1, Qt::AlignCenter);
    grid_layout->addWidget(cycle_count_edit, 1, 2, Qt::AlignCenter);
    grid_layout->addWidget(neutral_time_edit, 1, 3, Qt::AlignCenter);
    grid_layout->addWidget(work_time_edit, 1, 4, Qt::AlignCenter);
    grid_layout->addWidget(control_1_btn, 2, 0, Qt::AlignCenter);
    grid_layout->addWidget(control_2_btn, 2, 1, Qt::AlignCenter);
    grid_layout->addWidget(cycle_btn, 2, 2, 1, 3, Qt::AlignCenter);    

    main_layout->addLayout(grid_layout);

    connect(control_1_btn, &QPushButton::clicked, this, &CalibrationPage::OnControl1BtnClicked);
    connect(control_2_btn, &QPushButton::clicked, this, &CalibrationPage::OnControl2BtnClicked);
    connect(cycle_btn, &QPushButton::clicked, this, &CalibrationPage::OnCycleBtnClicked);

    return control_group;
}

// Implementation for control 1 button click
void CalibrationPage::OnControl1BtnClicked() {

}

// Implementation for control 2 button click
void CalibrationPage::OnControl2BtnClicked() {
    
}

// Implementation for cycle button click
void CalibrationPage::OnCycleBtnClicked() {
    
}

// Implementation for creating PID setting area
QWidget* CalibrationPage::CreatePIDSettingArea() {
    auto pid_group = new QGroupBox("PID参数调试区", this);

    auto main_layout = new QVBoxLayout(pid_group);

    auto p_label = new QLabel("P 比例参数");
    auto i_label = new QLabel("I 积分参数");
    auto d_label = new QLabel("D 微分参数");
    auto target_label = new QLabel("目标值");
    auto ramp_label = new QLabel("斜坡时间");

    auto p_edit = new QLineEdit();
    auto i_edit = new QLineEdit();
    auto d_edit = new QLineEdit();
    auto target_edit = new QLineEdit();
    auto ramp_edit = new QLineEdit();

    auto grid_layout = new QGridLayout();

    grid_layout->addWidget(p_label, 0, 0, Qt::AlignCenter);
    grid_layout->addWidget(i_label, 0, 1, Qt::AlignCenter);
    grid_layout->addWidget(d_label, 0, 2, Qt::AlignCenter);
    grid_layout->addWidget(target_label, 0, 3, Qt::AlignCenter);
    grid_layout->addWidget(ramp_label, 0, 4, Qt::AlignCenter);
    grid_layout->addWidget(p_edit, 1, 0, Qt::AlignCenter);
    grid_layout->addWidget(i_edit, 1, 1, Qt::AlignCenter);
    grid_layout->addWidget(d_edit, 1, 2, Qt::AlignCenter);
    grid_layout->addWidget(target_edit, 1, 3, Qt::AlignCenter);
    grid_layout->addWidget(ramp_edit, 1, 4, Qt::AlignCenter);

    auto button_layout = new QHBoxLayout();
    auto step_btn = new QPushButton("闭环阶跃响应");
    auto ramp_btn = new QPushButton("闭环斜坡响应");
    auto motion_btn = new QPushButton("往复动作");
    auto save_btn = new QPushButton("保存PID参数");

    button_layout->addWidget(step_btn);
    button_layout->addWidget(ramp_btn);
    button_layout->addWidget(motion_btn);
    button_layout->addWidget(save_btn);

    main_layout->addLayout(grid_layout);
    main_layout->addLayout(button_layout);

    connect(step_btn, &QPushButton::clicked, this, &CalibrationPage::OnPIDStepBtnClicked);
    connect(ramp_btn, &QPushButton::clicked, this, &CalibrationPage::OnPIDRampBtnClicked);
    connect(motion_btn, &QPushButton::clicked, this, &CalibrationPage::OnPIDMotionBtnClicked);
    connect(save_btn, &QPushButton::clicked, this, &CalibrationPage::OnPIDSaveBtnClicked);

    return pid_group;
}

void CalibrationPage::OnPIDStepBtnClicked() {
    
}

void CalibrationPage::OnPIDRampBtnClicked() {
    
}

void CalibrationPage::OnPIDMotionBtnClicked() {
    
}

void CalibrationPage::OnPIDSaveBtnClicked() {
    
}

// Implementation for creating displacement area
QWidget* CalibrationPage::CreateDisplacementArea() {
    auto displacement_group = new QGroupBox("位移流量标定区", this);

    auto main_layout = new QVBoxLayout(displacement_group);

    auto displace_table = new QTableWidget(3, 11, this);

    QStringList h_headers;
    for (int i = 0; i < 11; ++i) {
        h_headers << QString("点%1").arg(i + 1);
        displace_table->setColumnWidth(i, 90);

        //标定值
        auto calib_value_item = new QTableWidgetItem(QString::number(i*10));
        calib_value_item->setTextAlignment(Qt::AlignCenter);
        displace_table->setItem(0, i, calib_value_item);

        //控制值
        auto control_value_item = new QTableWidgetItem(QString::number(i*10));
        control_value_item->setTextAlignment(Qt::AlignCenter);
        displace_table->setItem(1, i, control_value_item);

        // 标定 
        auto calib_btn = new QPushButton("结束标定");
        calib_btn->setMinimumWidth(90);
        displace_table->setCellWidget(2, i, calib_btn);
        states_map_[calib_btn] = kEnd;
        connect(calib_btn, &QPushButton::clicked, this, [this, i, calib_btn](){
            auto state = states_map_[calib_btn];
            switch(state) {
                case kEnd:
                    calib_btn->setText("开始标定");
                    state = kStart;
                    break;
                case kStart:
                    calib_btn->setText("确认标定");
                    state = kConfirm;
                    break;
                case kConfirm:
                    calib_btn->setText("结束标定");
                    state = kEnd;
                    break;
                default:
                    break;
            }
            states_map_[calib_btn] = state;
        });
    }
    displace_table->setHorizontalHeaderLabels(h_headers);
    displace_table->setVerticalHeaderLabels({"标定值", "控制值", "标定"});

    auto sub_layout = new QHBoxLayout();
    auto displace_target_label = new QLabel("位移目标值:");
    auto displace_target_edit = new QLineEdit();
    displace_target_edit->setAlignment(Qt::AlignCenter);
    auto actual_value_label = new QLabel("位移实际值:");
    auto actual_value_edit = new QLineEdit();
    actual_value_edit->setAlignment(Qt::AlignCenter);
    auto real_flow_label = new QLabel("实时流量:");
    auto real_flow_edit = new QLineEdit();
    real_flow_edit->setAlignment(Qt::AlignCenter);
    auto target_flow_label = new QLabel("目标流量:");
    auto target_flow_edit = new QLineEdit();
    target_flow_edit->setAlignment(Qt::AlignCenter);

    auto state_btn = new QPushButton("标定进行中");
    connect(state_btn, &QPushButton::clicked, this, [this, state_btn](){
        switch(calib_state_) {
            case kOnCalib:
                state_btn->setText("保存标定值");
                calib_state_ = kSaveCalib;
                break;
            case kSaveCalib:
                state_btn->setText("验证标定值");
                calib_state_ = kConfirmCalib;
                break;
            case kConfirmCalib:
                state_btn->setText("标定进行中");
                calib_state_ = kOnCalib;
                break;
            default:
                break;
        }
    });
    sub_layout->addWidget(displace_target_label);
    sub_layout->addWidget(displace_target_edit);
    sub_layout->addStretch();
    sub_layout->addWidget(actual_value_label);
    sub_layout->addWidget(actual_value_edit);
    sub_layout->addStretch();
    sub_layout->addWidget(real_flow_label);
    sub_layout->addWidget(real_flow_edit);
    sub_layout->addStretch();
    sub_layout->addWidget(target_flow_label);
    sub_layout->addWidget(target_flow_edit);
    sub_layout->addStretch();
    sub_layout->addWidget(state_btn, Qt::AlignRight);

    main_layout->addWidget(displace_table);
    main_layout->addStretch();
    main_layout->addLayout(sub_layout);

    auto range_slider = new QRangeSlider(this);
    main_layout->addWidget(range_slider);
    
    connect(range_slider, &QRangeSlider::valueChanged, [=](int value){
        displace_target_edit->setText(QString::number(value));
        auto select_items = displace_table->selectedItems();
        if(select_items.isEmpty()) {
            return;
        }
        auto it = std::find_if(select_items.begin(), select_items.end(), [=](QTableWidgetItem *item){
            return item && (item->row() == 1);
        });
        if(it != select_items.end()) {
            (*it)->setText(QString::number(value));
            real_flow_edit->setText(QString::number(value));
        }
    });
    connect(displace_table, &QTableWidget::cellPressed, [=](int row, int col) {
        if(row != 1) {
            return;
        }
        auto control_value = displace_table->item(row, col)->text();
        real_flow_edit->setText(control_value);
    });

    return displacement_group;
}

// Implementation for creating signal response area
QWidget* CalibrationPage::CreateSignalResponseArea() {
    auto signal_group = new QGroupBox("周期信号响应区", this);

    auto main_layout = new QVBoxLayout(signal_group);

    auto cycle_count_label = new QLabel("循环次数");
    auto target_value_label = new QLabel("目标值");
    auto up_time_label = new QLabel("上升时间(s)");
    auto down_time_label = new QLabel("下降时间(s)");
    auto stop_time_label = new QLabel("中位滞留时间(s)");

    auto cycle_count_edit = new QLineEdit();
    auto target_value_edit = new QLineEdit();
    auto up_time_edit = new QLineEdit();
    auto down_time_edit = new QLineEdit();
    auto stop_time_edit = new QLineEdit();

    auto mode_select_label = new QLabel("模式选择:");
    auto mode_select_combo = new QComboBox();
    mode_select_combo->setFixedHeight(30);
    mode_select_combo->setMinimumWidth(300);
    mode_select_combo->addItems({"1侧曲线运动", "2侧曲线运动", "双侧曲线运动"});

    auto grid_layout = new QGridLayout();

    grid_layout->addWidget(cycle_count_label, 0, 0, Qt::AlignCenter);
    grid_layout->addWidget(target_value_label, 0, 1, Qt::AlignCenter);
    grid_layout->addWidget(up_time_label, 0, 2, Qt::AlignCenter);
    grid_layout->addWidget(down_time_label, 0, 3, Qt::AlignCenter);
    grid_layout->addWidget(stop_time_label, 0, 4, Qt::AlignCenter);
    grid_layout->addWidget(cycle_count_edit, 1, 0, Qt::AlignCenter);
    grid_layout->addWidget(target_value_edit, 1, 1, Qt::AlignCenter);
    grid_layout->addWidget(up_time_edit, 1, 2, Qt::AlignCenter);
    grid_layout->addWidget(down_time_edit, 1, 3, Qt::AlignCenter);
    grid_layout->addWidget(stop_time_edit, 1, 4, Qt::AlignCenter);
    // grid_layout->addWidget(mode_select_label, 2, 0, Qt::AlignCenter);

    auto button_layout = new QHBoxLayout();
    auto sine_wave_btn = new QPushButton("正弦波");
    sine_wave_btn->setMinimumWidth(300);
    sine_wave_btn->setEnabled(false);
    auto sawtooth_wave_btn = new QPushButton("锯齿波");
    sawtooth_wave_btn->setMinimumWidth(300);

    button_layout->addWidget(mode_select_label);
    button_layout->addWidget(mode_select_combo);
    button_layout->addStretch();
    button_layout->addWidget(sine_wave_btn);
    button_layout->addWidget(sawtooth_wave_btn);

    main_layout->addLayout(grid_layout);
    main_layout->addLayout(button_layout);

    connect(sine_wave_btn, &QPushButton::clicked, this, &CalibrationPage::OnSineWaveBtnClicked);
    connect(sawtooth_wave_btn, &QPushButton::clicked, this, &CalibrationPage::OnSawtoothWaveBtnClicked);
    connect(mode_select_combo, &QComboBox::currentTextChanged, this, [=](const QString &text){
        sine_wave_btn->setEnabled(text == "双侧曲线运动");
    });

    return signal_group;
}

void CalibrationPage::OnSineWaveBtnClicked() {
    // Implementation for sine wave button click
}

void CalibrationPage::OnSawtoothWaveBtnClicked() {
    // Implementation for sawtooth wave button click
}

// Implementation for creating waveform area
QWidget* CalibrationPage::CreateWaveformArea() {
    auto waveform_group = new QGroupBox("特性曲线显示图", this);

    auto main_layout = new QVBoxLayout(waveform_group);

    auto sub_layout = new QHBoxLayout();
    auto left_agix_label = new QLabel("左轴:");
    auto left_agix_combo = new QComboBox();
    auto right_agix_label = new QLabel("右轴:");
    auto right_agix_combo = new QComboBox();
    auto bottom_agix_label = new QLabel("底轴:");
    auto bottom_agix_combo = new QComboBox();
    sub_layout->addWidget(left_agix_label);
    sub_layout->addWidget(left_agix_combo);
    sub_layout->addWidget(right_agix_label);
    sub_layout->addWidget(right_agix_combo);
    sub_layout->addWidget(bottom_agix_label);
    sub_layout->addWidget(bottom_agix_combo);

    auto waveform_display = new QWidget(); // Placeholder for actual waveform display widget

    main_layout->addLayout(sub_layout);
    main_layout->addWidget(waveform_display);


    return waveform_group;
}