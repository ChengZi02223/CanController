#include "CalibrationPage.h"

#include "BasicInfoBar.h"
#include "CmdManager.h"
#include "CanDriver.h"
#include "can_cmd.h"

#include <QDebug>
#include <iostream>


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

    main_layout_->addLayout(left_layout, 0.5);
    main_layout_->addLayout(right_layout, 0.5);

    InitPageValue();
}

void CalibrationPage::InitPageValue() {
// control area

}

void CalibrationPage::resizeEvent(QResizeEvent* event)  {
    auto left_size = (width() - 40) / 2;
    basic_info_bar_->setFixedWidth(left_size);
    control_group_->setFixedWidth(left_size);
    pid_group_->setFixedWidth(left_size);
    displacement_group_->setFixedWidth(left_size);
    signal_group_->setFixedWidth(left_size);
    // waveform_group_->setFixedWidth(left_size);
    int table_col_width = (left_size - 100) / 3;
    for(int i = 0; i < displace_table_->columnCount(); i++) {
        displace_table_->setColumnWidth(i, table_col_width);
    }
}

// Implementation for creating control area
QWidget* CalibrationPage::CreateControlArea() {
    control_group_ = new QGroupBox("开环控制指令区", this);
    control_group_->setObjectName("ControlGroup");

    auto main_layout = new QVBoxLayout(control_group_);

    auto output_cycle_1_label = new QLabel("1 侧输出占空比(%)");
    auto output_cycle_2_label = new QLabel("2 侧输出占空比(%)");
    auto cycle_count_label = new QLabel("循环次数");
    auto neutral_time_label = new QLabel("中位停留时间(s)");
    auto work_time_label = new QLabel("工作位停留时间(s)");

    output_cycle_1_edit_ = new QLineEdit("50");
    output_cycle_1_edit_->setAlignment(Qt::AlignCenter);
    output_cycle_2_edit_ = new QLineEdit("50");
    output_cycle_2_edit_->setAlignment(Qt::AlignCenter);
    cycle_count_edit_ = new QLineEdit("10");
    cycle_count_edit_->setAlignment(Qt::AlignCenter);
    neutral_time_edit_ = new QLineEdit("1000");
    neutral_time_edit_->setAlignment(Qt::AlignCenter);
    work_time_edit_ = new QLineEdit("2000");
    work_time_edit_->setAlignment(Qt::AlignCenter);

    auto control_1_btn = new QPushButton("1 侧开环控制");
    control_1_btn->setFixedHeight(30);
    control_1_btn->setMinimumWidth(150);
    control_1_btn->setObjectName("ControlBtn");
    control_1_btn->setCheckable(true);
    auto control_2_btn = new QPushButton("2 侧开环控制");
    control_2_btn->setFixedHeight(30);
    control_2_btn->setMinimumWidth(150);
    control_2_btn->setObjectName("ControlBtn");
    cycle_btn_ = new QPushButton("开环循环动作");
    cycle_btn_->setObjectName("CycleBtn");
    cycle_btn_->setMinimumWidth(300);
    cycle_btn_->setFixedHeight(30);
    cycle_btn_->setCheckable(true);
    cycle_btn_->setDisabled(true);

    auto grid_layout = new QGridLayout();

    grid_layout->addWidget(output_cycle_1_label, 0, 0, Qt::AlignCenter);
    grid_layout->addWidget(output_cycle_2_label, 0, 1, Qt::AlignCenter);
    grid_layout->addWidget(cycle_count_label, 0, 2, Qt::AlignCenter);
    grid_layout->addWidget(neutral_time_label, 0, 3, Qt::AlignCenter);
    grid_layout->addWidget(work_time_label, 0, 4, Qt::AlignCenter);
    grid_layout->addWidget(output_cycle_1_edit_, 1, 0, Qt::AlignCenter);
    grid_layout->addWidget(output_cycle_2_edit_, 1, 1, Qt::AlignCenter);
    grid_layout->addWidget(cycle_count_edit_, 1, 2, Qt::AlignCenter);
    grid_layout->addWidget(neutral_time_edit_, 1, 3, Qt::AlignCenter);
    grid_layout->addWidget(work_time_edit_, 1, 4, Qt::AlignCenter);
    grid_layout->addWidget(control_1_btn, 2, 0, Qt::AlignCenter);
    grid_layout->addWidget(control_2_btn, 2, 1, Qt::AlignCenter);
    grid_layout->addWidget(cycle_btn_, 2, 2, 1, 3, Qt::AlignCenter);    

    main_layout->addLayout(grid_layout);

    connect(control_1_btn, &QPushButton::clicked, this, &CalibrationPage::OnControl1BtnClicked);
    connect(control_2_btn, &QPushButton::clicked, this, &CalibrationPage::OnControl2BtnClicked);
    connect(cycle_btn_, &QPushButton::clicked, this, &CalibrationPage::OnCycleBtnClicked);

    return control_group_;
}

// Implementation for control 1 button click
void CalibrationPage::OnControl1BtnClicked(bool checked) {
    std::cout << "Control 1 button clicked, checked:" << checked <<std::endl;
    cycle_btn_->setDisabled(!checked);
    if(!checked) {
        CanDriver::GetInstance()->ExecCmd(SDO_COB_ID, SDO_WRITE_CLOSE_CMD, 200);
        return;
    }

    bool ok = false;
    // 1. 读取输入框文本，转数字 100 → 1000（你业务规则：百分比 ×10）
    int percent = output_cycle_1_edit_->text().toInt(&ok);
    if(!ok) {
        std::cout << "输入数值非法"<<std::endl;
        return;
    }
    uint16_t value = static_cast<uint16_t>(percent * 10); // 100 → 1000

    // 2. 复制模板生成待发送指令
    std::vector<uint8_t> cmd(SDO_WRITE_OPEN_VALUE_CMD.begin(), SDO_WRITE_OPEN_VALUE_CMD.end());

    // 3. 【小端模式】填充第5、6字节（下标4、5）
    cmd[4] = static_cast<uint8_t>(value & 0xFF);        // 低字节 0xE8
    cmd[5] = static_cast<uint8_t>((value >> 8) & 0xFF); // 高字节 0x03

    // 此时 cmd = {0x2B,0x00,0x63,0x00,0xE8,0x03,0x00,0x00}

    // for (auto byte : cmd) {
    //     std::cout << "byte:" << QString("0x%1").arg(byte, 2, 16, QChar('0')).toUpper();
    // }
    bool ret = CanDriver::GetInstance()->ExecCmd(SDO_COB_ID, cmd, 200);
    if(!ret){
        qDebug().noquote() << "开阀指令发送失败";
    }
    

}

// Implementation for control 2 button click
void CalibrationPage::OnControl2BtnClicked() {
    
}

// Implementation for cycle button click
void CalibrationPage::OnCycleBtnClicked(bool checked) {
    std::cout << "cycle clicked, checked:" << checked<<std::endl;
    if(!checked) {
        CanDriver::GetInstance()->ExecCmd(NMT_COB_ID, NMT_CLOSE_READ_CMD, 200);
        data_timer_.stop();
        return;
    }

    CanDriver::GetInstance()->ExecCmd(NMT_COB_ID, NMT_READ_VALUE_CMD, 200);
    data_timer_.start(100);
}

// Implementation for creating PID setting area
QWidget* CalibrationPage::CreatePIDSettingArea() {
    pid_group_ = new QGroupBox("PID参数调试区", this);
    pid_group_->setObjectName("PIDGroup");

    auto main_layout = new QVBoxLayout(pid_group_);

    auto p_label = new QLabel("P 比例参数");
    auto i_label = new QLabel("I 积分参数");
    auto d_label = new QLabel("D 微分参数");
    auto target_label = new QLabel("目标值");
    auto ramp_label = new QLabel("斜坡时间");

    p_edit_ = new QLineEdit();
    i_edit_ = new QLineEdit();
    d_edit_ = new QLineEdit();
    target_edit_ = new QLineEdit();
    ramp_edit_ = new QLineEdit();

    auto grid_layout = new QGridLayout();

    grid_layout->addWidget(p_label, 0, 0, Qt::AlignCenter);
    grid_layout->addWidget(i_label, 0, 1, Qt::AlignCenter);
    grid_layout->addWidget(d_label, 0, 2, Qt::AlignCenter);
    grid_layout->addWidget(target_label, 0, 3, Qt::AlignCenter);
    grid_layout->addWidget(ramp_label, 0, 4, Qt::AlignCenter);
    grid_layout->addWidget(p_edit_, 1, 0, Qt::AlignCenter);
    grid_layout->addWidget(i_edit_, 1, 1, Qt::AlignCenter);
    grid_layout->addWidget(d_edit_, 1, 2, Qt::AlignCenter);
    grid_layout->addWidget(target_edit_, 1, 3, Qt::AlignCenter);
    grid_layout->addWidget(ramp_edit_, 1, 4, Qt::AlignCenter);

    auto button_layout = new QHBoxLayout();
    auto side_combo = new QComboBox();
    side_combo->setFixedHeight(30);
    side_combo->addItems({"1侧","2侧"});
    auto step_btn = new QPushButton("闭环阶跃响应");
    step_btn->setObjectName("ResponceBtn");
    step_btn->setFixedHeight(30);
    auto ramp_btn = new QPushButton("闭环斜坡响应");
    ramp_btn->setObjectName("ResponceBtn");
    ramp_btn->setFixedHeight(30);
    auto motion_btn = new QPushButton("往复动作");
    motion_btn->setObjectName("CycleBtn");
    motion_btn->setFixedHeight(30);
    auto save_btn = new QPushButton("保存PID参数");
    save_btn->setObjectName("CycleBtn");
    save_btn->setFixedHeight(30);

    button_layout->addWidget(side_combo);
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

    return pid_group_;
}

void CalibrationPage::OnPIDStepBtnClicked() {
    
}

void CalibrationPage::OnPIDRampBtnClicked() {
    
}

void CalibrationPage::OnPIDMotionBtnClicked() {
    std::cout<<"OnPIDMotionBtnClicked"<<std::endl;
}

void CalibrationPage::OnPIDSaveBtnClicked() {
    
}

// Implementation for creating displacement area
QWidget* CalibrationPage::CreateDisplacementArea() {
    displacement_group_ = new QGroupBox("位移流量标定区", this);
    displacement_group_->setObjectName("DisplacGroup");

    auto main_layout = new QVBoxLayout(displacement_group_);

    displace_table_ = new QTableWidget(11, 3, this);
    displace_table_->setObjectName("DisplaceTable");
    // save_item->setForeground(QBrush(Qt::green));

    QStringList h_headers;
    for (int i = 0; i < 11; ++i) {
        h_headers << QString("点%1").arg(i + 1);
        // displace_table_->setColumnWidth(i, 90);

        //标定值
        auto calib_value_item = new QTableWidgetItem(QString::number(i*10));
        calib_value_item->setTextAlignment(Qt::AlignCenter);
        displace_table_->setItem(i, 0, calib_value_item);

        //控制值
        auto control_value_item = new QTableWidgetItem(QString::number(i*10));
        control_value_item->setTextAlignment(Qt::AlignCenter);
        displace_table_->setItem(i, 1, control_value_item);

        // 标定 
        auto calib_btn = new QPushButton("结束标定");
        calib_btn->setMinimumWidth(90);
        calib_btn->setObjectName("CalibBtn");
        displace_table_->setCellWidget(i, 2, calib_btn);
        calib_btns_.push_back(calib_btn);
        connect(calib_btn, &QPushButton::clicked, this, [this, i, calib_btn](){
            select_calib_ = i;
            InitCalibState(calib_btn);
            switch(calib_state_) {
                case kEnd:
                    calib_btn->setText("开始标定");
                    calib_state_ = kStart;
                    break;
                case kStart:
                    calib_btn->setText("确认标定");
                    calib_state_ = kConfirm;
                    displace_table_->item(i, 0)->setText(displace_table_->item(i, 1)->text());
                    break;
                case kConfirm:
                    calib_btn->setText("结束标定");
                    calib_state_ = kEnd;
                    SetRowCalib(i, false);
                    UpdateCalibInfo();
                    return;
                default:
                    break;
            }
            UpdateCalibInfo();
            InitCalibValues(i);
        });
    }
    displace_table_->setVerticalHeaderLabels(h_headers);
    displace_table_->setHorizontalHeaderLabels({"标定值", "控制值", "标定"});

    info_label_ = new QLabel("当前标定：");

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

    auto state_btn = new QPushButton("标定终止");
    state_btn->setObjectName("StateBtn");
    state_btn->setFixedHeight(30);
    state_btn->setMinimumWidth(100);
    connect(state_btn, &QPushButton::clicked, this, [this, state_btn](){
        switch(calib_status_) {
            case kStopCalib:
                state_btn->setText("标定进行中");
                calib_status_ = kOnCalib;
                break;            
            case kOnCalib:
                state_btn->setText("验证标定值");
                calib_status_ = kConfirmCalib;
                break;
            case kConfirmCalib:
                state_btn->setText("标定终止");
                calib_status_ = kStopCalib;
                break;
            default:
                break;
        }
    });

    auto save_btn = new QPushButton("保存标定值");
    save_btn->setObjectName("StateBtn");
    save_btn->setFixedHeight(30);
    save_btn->setMinimumWidth(100);

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
    sub_layout->addWidget(save_btn, Qt::AlignRight);

    main_layout->addWidget(info_label_, 0, Qt::AlignRight);    
    main_layout->addWidget(displace_table_, 1);
    main_layout->addStretch();
    main_layout->addLayout(sub_layout);

    range_slider_ = new QRangeSlider(this);
    main_layout->addWidget(range_slider_);
    
    connect(range_slider_, &QRangeSlider::valueChanged, [=](int value){
        if(calib_state_ != kStart) {
            return;
        }
        displace_target_edit->setText(QString::number(value));
        // auto select_items = displace_table_->selectedItems();
        // if(select_items.isEmpty()) {
        //     return;
        // }
        // auto it = std::find_if(select_items.begin(), select_items.end(), [=](QTableWidgetItem *item){
        //     return item && (item->row() == 1);
        // });
        // if(it != select_items.end()) {
        //     (*it)->setText(QString::number(value));
        //     real_flow_edit->setText(QString::number(value));
        // }
        auto selected_calib_item = displace_table_->item(select_calib_, 1);
        selected_calib_item->setText(QString::number(value));
        real_flow_edit->setText(QString::number(value));
        UpdateCalibInfo();
    });
    // connect(displace_table_, &QTableWidget::cellPressed, [=](int row, int col) {
    //     if(col != 1) {
    //         return;
    //     }
    //     auto control_value = displace_table_->item(row, col)->text();
    //     displace_target_edit->setText(control_value);
    // });
    connect(save_btn, &QPushButton::clicked , this, CalibrationPage::OnSaveCalibValueBtnCLicked);

    return displacement_group_;
}

void CalibrationPage::UpdateCalibInfo() {
    auto v_head = displace_table_->verticalHeaderItem(select_calib_)->text();
    auto calib_item = displace_table_->item(select_calib_, 0);
    auto control_item = displace_table_->item(select_calib_, 1);
    QString state_str = "";
    if(calib_state_ == kStart) {
        state_str = "开始标定";
    } else if (calib_state_ == kConfirm) {
        state_str = "确认标定";
    } else if (calib_state_ == kEnd) {
        state_str = "结束标定";
    }
    auto text = QString("当前标定：%1 | %2 | %3 | %4").arg(v_head).arg(calib_item->text()).arg(control_item->text()).arg(state_str);
    info_label_->setText(text);
}

void CalibrationPage::OnSaveCalibValueBtnCLicked() {
    for(auto btn : calib_btns_) {
        btn->setText("结束标定");
    }
    calib_state_ = kEnd;
    for(int i = 0; i < displace_table_->rowCount(); i++) {
        SetRowCalib(i, false);
    }
}

void CalibrationPage::InitCalibState(QPushButton *calib_btn) {
    if(calib_btns_.empty()) {
        return;
    }
    int not_on_end = 0;
    for(auto btn : calib_btns_) {
        if(btn == calib_btn) {
            continue;
        }
        if(btn->text() != "结束标定") {
            not_on_end ++;
        }
        btn->setText("结束标定");
    }
    if(not_on_end > 0) {
        calib_state_ = kEnd;
    }
}

void CalibrationPage::InitCalibValues(int row) {
    if(row < 0 || row > displace_table_->rowCount() - 1) {
        return;
    }
    for(int i = 0; i < displace_table_->rowCount(); i++) {
        auto control_item = displace_table_->item(i, 1); 
        if(i == row) {
            range_slider_->setValue(control_item->text().toInt());
        }
        SetRowCalib(i, i == row);
    }
}

void CalibrationPage::SetRowCalib(int row, bool calib) {
    if(row < 0 || row > displace_table_->rowCount() - 1) {
        return;
    }
    auto calib_item = displace_table_->item(row, 0);
    auto control_item = displace_table_->item(row, 1);
    QColor backgd_color = calib ? Qt::blue : Qt::transparent; 
    QColor foregd_color = calib ? Qt::white : Qt::black; 
    calib_item->setBackground(QBrush(backgd_color));
    calib_item->setForeground(QBrush(foregd_color));
    control_item->setBackground(QBrush(backgd_color));
    control_item->setForeground(QBrush(foregd_color));
}

// Implementation for creating signal response area
QWidget* CalibrationPage::CreateSignalResponseArea() {
    signal_group_ = new QGroupBox("周期信号响应区", this);
    signal_group_->setObjectName("SignalGroup");

    auto main_layout = new QVBoxLayout(signal_group_);

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
    // mode_select_combo->setMinimumWidth(300);
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
    sine_wave_btn->setMinimumWidth(150);
    sine_wave_btn->setObjectName("WaveBtn");
    sine_wave_btn->setFixedHeight(30);
    sine_wave_btn->setEnabled(false);
    auto sawtooth_wave_btn = new QPushButton("锯齿波");
    sawtooth_wave_btn->setMinimumWidth(150);
    sawtooth_wave_btn->setObjectName("WaveBtn");
    sawtooth_wave_btn->setFixedHeight(30);

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

    return signal_group_;
}

void CalibrationPage::OnSineWaveBtnClicked() {
    // Implementation for sine wave button click
}

void CalibrationPage::OnSawtoothWaveBtnClicked() {
    // Implementation for sawtooth wave button click
}

// Implementation for creating waveform area
QWidget* CalibrationPage::CreateWaveformArea() {
    waveform_group_ = new QGroupBox("特性曲线显示图", this);

    auto main_layout = new QVBoxLayout(waveform_group_);

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
    sub_layout->addStretch();

    #ifdef ENABLE_WAVEFORM_DISPLAY
        std::cout << "Creating waveform display widget...";
        // auto waveform_display = new QWidget(); // Placeholder for actual waveform display widget
        QPerfCurve *waveform_display = new QPerfCurve();
        // 初始化曲线
        QVector<double> time = {0};
        QVector<double> displacement = {0};
        QVector<double> flow = {0};

        waveform_display->setTimeData(time);
        waveform_display->addCurve("Displacement", displacement, AxisType::Left);
        waveform_display->addCurve("Flow", flow, AxisType::Right);

        // 设置最大数据点数（保留最近100个点）
        waveform_display->setMaxDataPoints(100);
        
        // 启用自动刷新（每50ms刷新一次）
        waveform_display->setAutoRefreshInterval(50);
        waveform_display->setAutoRefreshEnabled(true);

        waveform_display->resize(800, 500);
        waveform_display->show();       
        // 模拟动态数据生成
        // QTimer dataTimer;
        double t = 0;
        QObject::connect(&data_timer_, &QTimer::timeout, [&]() {
            t += 0.1;
            double disp = 10 * sin(t) + 5 * sin(0.5 * t);
            double flow = 8 * cos(t * 0.7) + 3 * sin(t * 1.2);
            
            QMap<QString, double> dataPoint;
            dataPoint["Displacement"] = disp;
            dataPoint["Flow"] = flow;
            std::cout << "Appending data point at time:" << t << "Displacement:" << disp << "Flow:" << flow;
            
            waveform_display->appendDataPoint(t, dataPoint);
        });
        // data_timer_.start(100);
#endif

    // MainWindow构造
    m_wavePlot = new QWavePlotWidget(this);

    QWavePlotWidget::AxisConfig cfg;
    cfg.xLabel = "时间(s)";
    cfg.leftYLabel = "流量";
    cfg.rightYLabel = "位移";
    m_wavePlot->setupAxis(cfg);
    m_wavePlot->setMaxPointCount(3000);
    m_wavePlot->setTimeWindow(20.0);

    // 添加曲线：正弦波绑定左Y；锯齿波绑定右Y
    idxSine = m_wavePlot->addCurve("正弦波(流量)", QPen(Qt::blue,2), false);
    idxSaw = m_wavePlot->addCurve("锯齿波(位移)", QPen(Qt::red,2), true);
    connect(&data_timer_, &QTimer::timeout, [&](){
        m_time +=0.05;
        double sineVal = 10 * sin(2*M_PI*0.5*m_time);
        double sawVal = fmod(m_time*20,40)-20;

        // std::cout << "Appending data point at time:" << m_time << "Sine:" << sineVal << "Saw:" << sawVal;
        // can_frame frame;
        // bool ret = CanDriver::GetInstance()->receive(frame, 100);
        // if(!ret) {
        //     std::cout << "Failed to read CAN frame!!"<<std::endl;
        //     return;
        // }
        // if(frame.can_id != 0x1C0) {
        //     std::cout << "Unexpected CAN ID:" << frame.can_id<<std::endl;
        //     return;
        // }
        // if(frame.can_dlc != 8) {
        //     std::cout << "CAN frame data length too short:" << frame.can_dlc<<std::endl;
        //     return;
        // }

        // uint16_t raw = (static_cast<uint16_t>(frame.data[3]) << 8) | frame.data[2];
        // double sawVal = static_cast<double>(raw) * 170.0 / 1000.0;
        // m_wavePlot->appendData(idxSine, m_time, sineVal);
        // std::cout << "Appending data point at time:" << m_time << "Saw:" << sawVal;
        m_wavePlot->appendData(idxSaw, m_time, sawVal);
    });

    main_layout->addLayout(sub_layout);
    main_layout->addWidget(m_wavePlot);   


    return waveform_group_;
}