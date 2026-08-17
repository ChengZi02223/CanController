#include "CalibrationPage.h"

#include "BasicInfoBar.h"
#include "CanDriver.h"
#include "TaskMgr.h"
#include "can_cmd.h"
#include "Utils.h"
#include "QWavePlotWithLegendWidget.h"

#include <QDebug>
#include <iostream>
#include <QThread>
#include <mutex>

#define kFaMaxFlow 80 // L/Min
#ifdef ON_TEST_MODE
    #define kCloseCycleWaitTime 2  //s
#else
    #define kCloseCycleWaitTime 15 //s
#endif
static const std::vector<double> flow_table = {1.0, 0.9, 0.5, 0.1, 0, 0, 0, 0.1, 0.5, 0.9, 1.0};

inline int32_t GetTargetFlow(int i) {
    if(i >= 0 || i < flow_table.size()) {
        return flow_table[i] * kFaMaxFlow;
    }
} 

CalibrationPage::CalibrationPage(QWidget* parent)
    : QWidget(parent) {

    InitPage();
    qRegisterMetaType<DrawStayInfo>("DrawStayInfo");

    connect(this, &CalibrationPage::SendOpenLoopFinished, this, [this](){
        if(cur_loop_mode_ == kOpenLoop) {
            cycle_btn_->setChecked(false);
            OnCycleBtnClicked(false);            
        } else {
            motion_btn_->setChecked(false);
            OnPIDMotionBtnClicked(false);
        }

    });
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

void CalibrationPage::InitBasicInfo(BasicInfo info) { basic_info_bar_->InitData(info); }

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
    int table_col_width = (left_size - 100) / 5;
    for(int i = 0; i < displace_table_->columnCount(); i++) {
        displace_table_->setColumnWidth(i, table_col_width);
    }

    // 开环控制区

    auto btn_width = table_col_width - 5;
    output_cycle_1_edit_->setFixedWidth(btn_width);
    output_cycle_2_edit_->setFixedWidth(btn_width);
    cycle_count_edit_->setFixedWidth(btn_width);
    neutral_time_edit_->setFixedWidth(btn_width);
    work_time_edit_->setFixedWidth(btn_width);

    control_1_btn_->setFixedWidth(btn_width);
    control_2_btn_->setFixedWidth(btn_width);
    control_cur_1_btn_->setFixedWidth(btn_width);
    control_cur_2_btn_->setFixedWidth(btn_width);
    cycle_btn_->setFixedWidth(btn_width);

    // PID验证区
    p_edit_->setFixedWidth(btn_width);
    i_edit_->setFixedWidth(btn_width);
    d_edit_->setFixedWidth(btn_width);
    target_edit_->setFixedWidth(btn_width);
    ramp_edit_->setFixedWidth(btn_width);

    side_btn_->setFixedWidth(btn_width);
    step_btn_->setFixedWidth(btn_width);
    ramp_btn_->setFixedWidth(btn_width);
    motion_btn_->setFixedWidth(btn_width);
    save_pid_btn_->setFixedWidth(btn_width);
}

// Implementation for creating control area
QWidget* CalibrationPage::CreateControlArea() {
    control_group_ = new QGroupBox("一/二侧电流开环", this);
    control_group_->setObjectName("ControlGroup");

    auto main_layout = new QVBoxLayout(control_group_);

    auto output_cycle_1_label = new QLabel("位移输出占空比(%)");
    auto output_cycle_2_label = new QLabel("电流输出占空比(%)");
    auto cycle_count_label = new QLabel("循环次数");
    auto neutral_time_label = new QLabel("中位停留时间(ms)");
    auto work_time_label = new QLabel("工作位停留时间(ms)");

    output_cycle_1_edit_ = new QLineEdit("10");
    output_cycle_1_edit_->setAlignment(Qt::AlignCenter);
    output_cycle_2_edit_ = new QLineEdit("10");
    output_cycle_2_edit_->setAlignment(Qt::AlignCenter);
    cycle_count_edit_ = new QLineEdit("2");
    cycle_count_edit_->setAlignment(Qt::AlignCenter);
    neutral_time_edit_ = new QLineEdit("1000");
    neutral_time_edit_->setAlignment(Qt::AlignCenter);
    work_time_edit_ = new QLineEdit("2000");
    work_time_edit_->setAlignment(Qt::AlignCenter);

    control_1_btn_ = new QPushButton("1 侧开环控制");
    control_1_btn_->setFixedHeight(30);
    control_1_btn_->setObjectName("ControlBtn");
    control_1_btn_->setCheckable(true);
    control_2_btn_ = new QPushButton("2 侧开环控制");
    control_2_btn_->setFixedHeight(30);
    control_2_btn_->setObjectName("ControlBtn");
    control_2_btn_->setCheckable(true);

    control_cur_1_btn_ = new QPushButton("1 侧电流开环控制");
    control_cur_1_btn_->setFixedHeight(30);
    control_cur_1_btn_->setObjectName("ControlBtn");
    control_cur_1_btn_->setCheckable(true);
    control_cur_2_btn_ = new QPushButton("2 侧电流开环控制");
    control_cur_2_btn_->setFixedHeight(30);
    control_cur_2_btn_->setMinimumWidth(150);
    control_cur_2_btn_->setObjectName("ControlBtn");
    control_cur_2_btn_->setCheckable(true);

    cycle_btn_ = new QPushButton("开环循环动作");
    cycle_btn_->setObjectName("CycleBtn");
    cycle_btn_->setFixedHeight(30);
    cycle_btn_->setCheckable(true);
    // cycle_btn_->setDisabled(true);

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
    grid_layout->addWidget(control_1_btn_, 2, 0, Qt::AlignCenter);
    grid_layout->addWidget(control_2_btn_, 2, 1, Qt::AlignCenter);
    grid_layout->addWidget(control_cur_1_btn_, 2, 2, Qt::AlignCenter);
    grid_layout->addWidget(control_cur_2_btn_, 2, 3, Qt::AlignCenter);
    grid_layout->addWidget(cycle_btn_, 2, 4, Qt::AlignCenter);    

    main_layout->addLayout(grid_layout);

    connect(control_1_btn_, &QPushButton::clicked, this, &CalibrationPage::OnControl1BtnClicked);
    connect(control_2_btn_, &QPushButton::clicked, this, &CalibrationPage::OnControl2BtnClicked);
    connect(control_cur_1_btn_, &QPushButton::clicked, this, &CalibrationPage::OnControlCur1BtnClicked);
    connect(control_cur_2_btn_, &QPushButton::clicked, this, &CalibrationPage::OnControlCur2BtnClicked);
    connect(cycle_btn_, &QPushButton::clicked, this, &CalibrationPage::OnCycleBtnClicked);

    return control_group_;
}

void CalibrationPage::ChangeLoopMode(LoopMode mode) {
    if(mode == cur_loop_mode_) {
        return;
    }
    std::vector<uint8_t> cmd;
    if(mode == kOpenLoop) {
        cmd = SDO_OPEN_LOOP_MODE_CMD;
    } else {
        cmd = SDO_CLOSE_LOOP_MODE_CMD;
    }
    CanDriver::GetInstance()->ExecCmd(SDO_COB_ID, cmd, 200);
    cur_loop_mode_ = mode;
}

// Implementation for control 1 button click
void CalibrationPage::OnControl1BtnClicked(bool checked) {
    std::cout << "Control 1 button clicked, checked:" << checked <<std::endl;
    cur_loop_mode_ = kOpenLoop;
    if(checked && control_2_btn_->isChecked()){
        control_2_btn_->setChecked(false);
        OnControl2BtnClicked(false);
    }
    if(!checked) {
        CanDriver::GetInstance()->ExecCmd(NMT_COB_ID, NMT_CLOSE_READ_CMD, 500);
        CanDriver::GetInstance()->ExecCmd(SDO_COB_ID, SDO_WRITE_CLOSE_1_CMD, 500);
        StopControl1Loop();
        return;
    } else {
        CanDriver::GetInstance()->ExecCmd(SDO_COB_ID, SDO_OPEN_LOOP_MODE_CMD, 200);
    }
    
    bool ok = false;
    // 1. 读取输入框文本，转数字 100 → 1000（你业务规则：百分比 ×10）
    int percent = output_cycle_1_edit_->text().toInt(&ok);
    if(!ok) {
        std::cout << "输入数值非法"<<std::endl;
        return;
    }
    cur_fa_val_1_cmd_ = SetTargetCMDValue(SDO_WRITE_OPEN_1_VALUE_CMD, percent);

    // {0x01, 0x40}
    CanDriver::GetInstance()->ExecCmd(NMT_COB_ID, NMT_READ_VALUE_CMD, 200);
    StartControl1Loop();
}

// Implementation for control 2 button click
void CalibrationPage::OnControl2BtnClicked(bool checked) {
    cur_loop_mode_ = kOpenLoop;
    std::cout << "Control 2 button clicked, checked:" << checked <<std::endl;
    if(checked && control_1_btn_->isChecked()){
        control_1_btn_->setChecked(false);
        OnControl1BtnClicked(false);
    }
    if(!checked) {
        CanDriver::GetInstance()->ExecCmd(NMT_COB_ID, NMT_CLOSE_READ_CMD, 500);
        CanDriver::GetInstance()->ExecCmd(SDO_COB_ID, SDO_WRITE_CLOSE_2_CMD, 500);
        StopControl2Loop();
        return;
    } else {
        // ChangeLoopMode(kOpenLoop);
        CanDriver::GetInstance()->ExecCmd(SDO_COB_ID, SDO_OPEN_LOOP_MODE_CMD, 200);
    }
    
    bool ok = false;
    // 1. 读取输入框文本，转数字 100 → 1000（你业务规则：百分比 ×10）
    int percent = output_cycle_1_edit_->text().toInt(&ok);
    if(!ok) {
        std::cout << "输入数值非法"<<std::endl;
        return;
    }
    // 此时 {0x2B,0x03,0x63,0x00,0xE8,0x03,0x00,0x00}
    cur_fa_val_2_cmd_ = SetTargetCMDValue(SDO_WRITE_OPEN_2_VALUE_CMD, percent);

    // {0x01, 0x40}
    CanDriver::GetInstance()->ExecCmd(NMT_COB_ID, NMT_READ_VALUE_CMD, 200);
    StartControl2Loop();
}

// 1侧流量开环控制
void CalibrationPage::OnControlCur1BtnClicked(bool checked) {
    cur_loop_mode_ = kOpenLoop;
}

// 2侧流量开环控制
void CalibrationPage::OnControlCur2BtnClicked(bool checked) {
    cur_loop_mode_ = kOpenLoop;
}

// Implementation for cycle button click
void CalibrationPage::OnCycleBtnClicked(bool checked) {
    cur_loop_mode_ = kOpenLoop;
    std::cout << "cycle clicked, checked:" << checked << std::endl;
    if (!checked) {
        StopLoopCycle();
        CanDriver::GetInstance()->ExecCmd(NMT_COB_ID, NMT_CLOSE_READ_CMD, 500);
        return;
    } else {
        // ChangeLoopMode(kOpenLoop);
        CanDriver::GetInstance()->ExecCmd(SDO_COB_ID, SDO_OPEN_LOOP_MODE_CMD, 200);
    }

    StartLoopCycle();
}

bool CalibrationPage::StartLoopCycle() {
    // 已经在运行，禁止重复启动
    if(is_open_running_.load()){
        return false;
    }
    is_open_running_.store(true);
    if(open_loop_thread_.joinable()) {
        open_loop_thread_.join();
    }
    open_loop_thread_ = std::thread([this]() {
        ExecuteLoopCycle();
    });

    return true;
}

void CalibrationPage::StopLoopCycle() {
    // 1.通知线程业务循环退出
    is_open_running_.store(false);

    // 2.如果线程有效，join阻塞等待子线程执行完毕
    if(open_loop_thread_.joinable()) {
        open_loop_thread_.join();
    }
    OnControl1BtnClicked(false);
    OnControl2BtnClicked(false);
}

void CalibrationPage::ExecuteLoopCycle() {
// TODO: 
    int percent_1, percent_2, cycle_count, neutral_stay_time, work_stay_time;
    if(cur_loop_mode_ == kOpenLoop) {
        percent_1 = output_cycle_1_edit_->text().toInt();
        percent_2 = output_cycle_2_edit_->text().toInt();
        cycle_count = cycle_count_edit_->text().toInt();
        neutral_stay_time = neutral_time_edit_->text().toInt();
        work_stay_time = work_time_edit_->text().toInt();
    } else {
        percent_1 = target_edit_->text().toInt();
        percent_2 = target_edit_->text().toInt();
        cycle_count = 1;
        neutral_stay_time = kCloseCycleWaitTime * 1000;
        work_stay_time = kCloseCycleWaitTime * 1000;
    }
    int target_value_1 = percent_1 * 10; // = percent_1 / 100 * 1000;
    int target_value_2 = percent_2 * 10; // = percent_2 / 100 * 1000;

    // 循环次数
    int loop_count = 0;
    bool one_loop_off_1_ = false;
    bool one_loop_off_2_ = false;
    cur_fa_val_1_cmd_ = SDO_WRITE_CLOSE_1_CMD;
    cur_fa_val_2_cmd_ = SDO_WRITE_CLOSE_2_CMD;
    CanDriver::GetInstance()->ExecCmd(NMT_COB_ID, NMT_READ_VALUE_CMD, 500);
    if(!stay_1_running_) {
        StartControl1Loop();
    }
    if(!stay_2_running_) {
        StartControl2Loop();
    }
    while(is_open_running_.load()) {
        if(loop_count == cycle_count) {
            break;
        }
        // is_on_work_stay_time_ = false;
        // 1侧开环控制
        cur_fa_val_1_cmd_ = SetTargetCMDValue(SDO_WRITE_OPEN_1_VALUE_CMD, 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(neutral_stay_time));
        qDebug() << "1 中位停留： " << neutral_stay_time / 1000 << "s";

        // is_on_work_stay_time_ = true;
        cur_fa_val_1_cmd_ = SetTargetCMDValue(SDO_WRITE_OPEN_1_VALUE_CMD, percent_1);
        std::this_thread::sleep_for(std::chrono::milliseconds(work_stay_time));
        qDebug() << "1 工作位停留： " << work_stay_time / 1000 << "s";

        // is_on_work_stay_time_ = false;
        cur_fa_val_1_cmd_ = SetTargetCMDValue(SDO_WRITE_OPEN_1_VALUE_CMD, 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(neutral_stay_time));
        qDebug() << "1 中位停留： " << neutral_stay_time / 1000 << "s";

        // 2侧开环控制
        // is_on_work_stay_time_ = false;
        cur_fa_val_2_cmd_ = SetTargetCMDValue(SDO_WRITE_OPEN_2_VALUE_CMD, 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(neutral_stay_time));
        qDebug() << "2 中位停留： " << neutral_stay_time / 1000 << "s";

        // is_on_work_stay_time_ = true;
        cur_fa_val_2_cmd_ = SetTargetCMDValue(SDO_WRITE_OPEN_2_VALUE_CMD, percent_2);
        std::this_thread::sleep_for(std::chrono::milliseconds(work_stay_time));
        qDebug() << "2 工作位停留： " << work_stay_time / 1000 << "s";

        // is_on_work_stay_time_ = false;
        cur_fa_val_2_cmd_ = SetTargetCMDValue(SDO_WRITE_OPEN_2_VALUE_CMD, 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(neutral_stay_time));
        qDebug() << "2 中位停留： " << neutral_stay_time / 1000 << "s";

        loop_count++;
        qDebug() << "-------一个循环结束-------";
    }
    qDebug() << "loop count: "<< loop_count;
    // is_on_work_stay_time_ = false;
    emit SendOpenLoopFinished();
}

static int x_ = 0;
static int y_ = 0;
int CalibrationPage::ReadCurrentStay(int side) {
    if(side != 1 && side != 2) {
        return -1;
    }
#ifdef ON_TEST_MODE
    if (side == 1) {
        int value_x = x_ % 200;
        x_++;
        return value_x <= 100 ? value_x : 200 - value_x;    
    }
    int value_y = y_ % 200;
    y_++;
    return value_y <= 100 ? value_y : 200 - value_y; 
#else
    can_frame ret_frame;
    bool ret = false;
    if(side == 1) {  
        // {0x40, 0x01, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00}
        ret = CanDriver::GetInstance()->ExecCmd(SDO_COB_ID, SDO_READ_STAY_1_CMD, ret_frame, 200);
    } else {
        // {0x40, 0x04, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00}
        ret = CanDriver::GetInstance()->ExecCmd(SDO_COB_ID, SDO_READ_STAY_2_CMD, ret_frame, 200);
    }
    if(!ret){
        std::cout << "开环循环动作 指令发送失败" << std::endl;
        return -1;
    }
    if(ret_frame.can_id != CUR_STAY_ID) {
        std::cout << "Unexpected CAN ID:" << ret_frame.can_id << std::endl;
        return -1;
    }
    if(ret_frame.can_dlc != 8) {
        std::cout << "CAN frame data length too short:" << ret_frame.can_dlc << std::endl;
        return -1;
    }
    if(side == 1) { 
        if(!CheckAnswerHead(ret_frame.data, SDO_CUR_STAY_1_CMD_HEAD)) {
            return -1;
        }        
    }else {
        if(!CheckAnswerHead(ret_frame.data, SDO_CUR_STAY_2_CMD_HEAD)) {
            return -1;
        }  
    }
    uint16_t saw = ExtractFromDataList(ret_frame.data, 5, 4);
    return static_cast<int>(saw);
#endif
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
    auto ramp_label = new QLabel("斜坡时间(ms)");

    p_edit_ = new QLineEdit("1.0");
    p_edit_->setAlignment(Qt::AlignCenter);
    i_edit_ = new QLineEdit("0.1");
    i_edit_->setAlignment(Qt::AlignCenter);
    d_edit_ = new QLineEdit("0.01");
    d_edit_->setAlignment(Qt::AlignCenter);
    target_edit_ = new QLineEdit("100");
    target_edit_->setAlignment(Qt::AlignCenter);
    ramp_edit_ = new QLineEdit("500");
    ramp_edit_->setAlignment(Qt::AlignCenter);

    side_btn_ = new QPushButton("1 侧");
    side_btn_->setObjectName("ResponceBtn");
    side_btn_->setFixedHeight(30);
    step_btn_ = new QPushButton("闭环阶跃响应");
    step_btn_->setObjectName("ResponceBtn");
    step_btn_->setFixedHeight(30);
    step_btn_->setCheckable(true);
    ramp_btn_ = new QPushButton("闭环斜坡响应");
    ramp_btn_->setObjectName("ResponceBtn");
    ramp_btn_->setFixedHeight(30);
    ramp_btn_->setCheckable(true);
    motion_btn_ = new QPushButton("往复动作");
    motion_btn_->setObjectName("CycleBtn");
    motion_btn_->setFixedHeight(30);
    motion_btn_->setCheckable(true);
    save_pid_btn_ = new QPushButton("保存PID参数");
    save_pid_btn_->setObjectName("CycleBtn");
    save_pid_btn_->setFixedHeight(30);

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
    grid_layout->addWidget(side_btn_, 2, 0, Qt::AlignCenter);
    grid_layout->addWidget(step_btn_, 2, 1, Qt::AlignCenter);
    grid_layout->addWidget(ramp_btn_, 2, 2, Qt::AlignCenter);
    grid_layout->addWidget(motion_btn_, 2, 3, Qt::AlignCenter);
    grid_layout->addWidget(save_pid_btn_, 2, 4, Qt::AlignCenter);

    main_layout->addLayout(grid_layout);

    connect(side_btn_, &QPushButton::clicked, this, &CalibrationPage::OnPIDSideBtnClicked);
    connect(step_btn_, &QPushButton::clicked, this, &CalibrationPage::OnPIDStepBtnClicked);
    connect(ramp_btn_, &QPushButton::clicked, this, &CalibrationPage::OnPIDRampBtnClicked);
    connect(motion_btn_, &QPushButton::clicked, this, &CalibrationPage::OnPIDMotionBtnClicked);
    connect(save_pid_btn_, &QPushButton::clicked, this, &CalibrationPage::OnPIDSaveBtnClicked);

    return pid_group_;
}

bool CalibrationPage::IsOnSideControl1() {
    return on_side_1_;
}

void CalibrationPage::OnPIDSideBtnClicked() {
    on_side_1_ = !on_side_1_;
    auto txt = on_side_1_ ? QString("1 侧") : QString("2 侧");
    side_btn_->setText(txt);
}

void CalibrationPage::SetPIDParam() {
    auto driver = CanDriver::GetInstance();
    double p_v = p_edit_->text().toDouble();
    double i_v = i_edit_->text().toDouble();
    double d_v = d_edit_->text().toDouble();
    std::vector<CanCmdItem> cmds;
    if(IsOnSideControl1()) {
        cmds = {
            {SDO_COB_ID, SetPIDCMDValue(SDO_WRITE_PID_1_P_CMD, p_v)},
            {SDO_COB_ID, SetPIDCMDValue(SDO_WRITE_PID_1_I_CMD, i_v)},
            {SDO_COB_ID, SetPIDCMDValue(SDO_WRITE_PID_1_D_CMD, d_v)}
        };
    } else {
        cmds = {
            {SDO_COB_ID, SetPIDCMDValue(SDO_WRITE_PID_2_P_CMD, p_v)},
            {SDO_COB_ID, SetPIDCMDValue(SDO_WRITE_PID_2_I_CMD, i_v)},
            {SDO_COB_ID, SetPIDCMDValue(SDO_WRITE_PID_2_D_CMD, d_v)}
        };
    }
    CanDriver::GetInstance()->ExecCmds(cmds);
}

void CalibrationPage::OnPIDStepBtnClicked(bool checked) {
    cur_loop_mode_ = kClosedLoop;
    auto driver = CanDriver::GetInstance();
    std::cout << "Step button clicked, checked:" << checked <<std::endl;
    if(checked && ramp_btn_->isChecked()){
        ramp_btn_->setChecked(false);
        OnPIDRampBtnClicked(false);      
    }
    if(!checked) {
        // {0x02, 0x40}
        driver->ExecCmd(NMT_COB_ID, NMT_CLOSE_READ_CMD, 500);
        // {0x2B, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00}
        if(IsOnSideControl1()) {
            driver->ExecCmd(SDO_COB_ID, SDO_WRITE_CLOSE_1_CMD, 500);
            StopControl1Loop();
        } else {
            driver->ExecCmd(SDO_COB_ID, SDO_WRITE_CLOSE_2_CMD, 500);
            StopControl2Loop();
        }
        return;
    } else {
        // ChangeLoopMode(kClosedLoop);
        std::vector<CanCmdItem> cmds = {
            {SDO_COB_ID, SDO_CLOSE_LOOP_MODE_CMD},      // 闭环模式
            {SDO_COB_ID, SDO_STEP_MODE_CMD}            // 阶跃模式
        };
        driver->ExecCmds(cmds);
        SetPIDParam();
    }
    
    bool ok = false;
    // 阀1目标流量=40L/min (500=50%×80L) ===
    int target_value = target_edit_->text().toInt(&ok);
    if(!ok) {
        std::cout << "输入数值非法"<<std::endl;
        return;
    }

    std::vector<uint8_t> cmd;
    if(IsOnSideControl1()) {
        cur_fa_val_1_cmd_ = SetTargetCMDValue(SDO_WRITE_OPEN_1_VALUE_CMD, target_value, 1);
        cmd = cur_fa_val_1_cmd_;
    } else {
        cur_fa_val_2_cmd_ = SetTargetCMDValue(SDO_WRITE_OPEN_2_VALUE_CMD, target_value, 1);
        cmd = cur_fa_val_2_cmd_;
    }

    // {0x01, 0x40}
    driver->ExecCmd(NMT_COB_ID, NMT_READ_VALUE_CMD, 200);
    if(IsOnSideControl1()) {
        StartControl1Loop();
    } else {
        StartControl2Loop();
    }
    
}

void CalibrationPage::OnPIDRampBtnClicked(bool checked) {
    cur_loop_mode_ = kClosedLoop;
    auto driver = CanDriver::GetInstance();

    std::cout << "Step button clicked, checked:" << checked <<std::endl;
    if(checked && step_btn_->isChecked()){
        step_btn_->setChecked(false);
        OnPIDStepBtnClicked(false);
    }

    if(!checked) {
        // {0x02, 0x40}
        driver->ExecCmd(NMT_COB_ID, NMT_CLOSE_READ_CMD, 500);
        // {0x2B, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00}
        if(IsOnSideControl1()) {
            driver->ExecCmd(SDO_COB_ID, SDO_WRITE_CLOSE_1_CMD, 500);
            StopControl1Loop();
        } else {
            driver->ExecCmd(SDO_COB_ID, SDO_WRITE_CLOSE_2_CMD, 500);
            StopControl2Loop();
        }
        return;
    } else {
        // ChangeLoopMode(kClosedLoop);// 闭环模式
        std::vector<CanCmdItem> cmds = {
            {SDO_COB_ID, SDO_CLOSE_LOOP_MODE_CMD},      // 闭环模式
            {SDO_COB_ID, SDO_RAMP_MODE_CMD},            // 阶跃模式
            {SDO_COB_ID, SDO_RAMP_EXTEND_TIME_CMD}, // 伸出激活=500ms
            {SDO_COB_ID, SDO_RAMP_RETRACT_TIME_CMD} // 缩回激活=200ms
        };
        driver->ExecCmds(cmds);
        SetPIDParam();
    }
    
    bool ok = false;
    int ramp_time = ramp_edit_->text().toInt(&ok);
    if(!ok) {
        std::cout << "输入数值非法"<<std::endl;
        return;
    }

    std::vector<uint8_t> cmd;
    if(IsOnSideControl1()) {
        cur_fa_val_1_cmd_ = SetTargetCMDValue(SDO_WRITE_OPEN_1_VALUE_CMD, ramp_time, 1);
        cmd = cur_fa_val_1_cmd_;
    } else {
        cur_fa_val_2_cmd_ = SetTargetCMDValue(SDO_WRITE_OPEN_2_VALUE_CMD, ramp_time, 1);
        cmd = cur_fa_val_2_cmd_;
    }

    // {0x01, 0x40}
    driver->ExecCmd(NMT_COB_ID, NMT_READ_VALUE_CMD, 200);
    if(IsOnSideControl1()) {
        StartControl1Loop();
    } else {
        StartControl2Loop();
    }
}

void CalibrationPage::OnPIDMotionBtnClicked(bool checked) {
    cur_loop_mode_ = kClosedLoop;
    std::cout << "close cycle clicked, checked:" << checked << std::endl;
    auto driver = CanDriver::GetInstance();
    if (!checked) {
        StopLoopCycle();
        return;
    } else {
            std::vector<CanCmdItem> cmds = {
            {SDO_COB_ID, SDO_CLOSE_LOOP_MODE_CMD},      // 闭环模式
            {SDO_COB_ID, SDO_RAMP_MODE_CMD}             // 阶跃模式
        };
        driver->ExecCmds(cmds);
    }

    bool ok = false;
    int ramp_time = ramp_edit_->text().toInt(&ok);
    if(!ok) {
        std::cout << "输入数值非法"<<std::endl;
        return;
    }

    std::vector<uint8_t> cmd = SetTargetCMDValue(SDO_SET_RAMP_EXTEND_TIME_CMD, ramp_time, 1);
    driver->ExecCmd(SDO_COB_ID, cmd, 500);
    cmd = SetTargetCMDValue(SDO_SET_RAMP_RETRACT_TIME_CMD, ramp_time, 1);
    driver->ExecCmd(SDO_COB_ID, cmd, 500);

    StartLoopCycle();
}

void CalibrationPage::OnPIDSaveBtnClicked() {
    
}

// Implementation for creating displacement area
QWidget* CalibrationPage::CreateDisplacementArea() {
    displacement_group_ = new QGroupBox("位移流量标定区", this);
    displacement_group_->setObjectName("DisplacGroup");

    auto main_layout = new QVBoxLayout(displacement_group_);

    displace_table_ = new QTableWidget(11, 5, this);
    displace_table_->setObjectName("DisplaceTable");
    // displace_table_->setSelectionBehavior(QAbstractItemView::SelectRows); //SelectRows
    displace_table_->setSelectionMode(QAbstractItemView::NoSelection);
    // save_item->setForeground(QBrush(Qt::green));

    QStringList h_headers = {"100%", "90%", "50%", "10%", "0%", "中位", "0%", "10%", "50%", "90%", "100%"};
    for (int i = 0; i < 11; ++i) {
        // displace_table_->setColumnWidth(i, 90);
        displace_table_->setRowHeight(i, 40);

        //位移标定值
        auto calib_stay_item = new QTableWidgetItem(QString::number(i*10));
        calib_stay_item->setTextAlignment(Qt::AlignCenter);
        displace_table_->setItem(i, 0, calib_stay_item);

        //电流标定值
        auto calib_current_item = new QTableWidgetItem(QString::number(i*10));
        calib_current_item->setTextAlignment(Qt::AlignCenter);
        displace_table_->setItem(i, 1, calib_current_item);

        //标定预估值
        auto control_value_item = new QTableWidgetItem(QString::number(i*10));
        control_value_item->setTextAlignment(Qt::AlignCenter);
        displace_table_->setItem(i, 2, control_value_item);

        // 标定 
        auto calib_btn = new QPushButton("结束标定");
        calib_btn->setMinimumWidth(90);
        calib_btn->setObjectName("CalibBtn");
        displace_table_->setCellWidget(i, 3, calib_btn);
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
                    calib_btn->setText("结束标定");
                    calib_state_ = kEnd;
                    SetRowCalib(i, false);
                    break;
                default:
                    break;
            }
            UpdateCalibInfo();
            bool start = (calib_state_ == kStart);
            for(int c = 0; c < 3; c++) {
                QTableWidgetItem* item = displace_table_->item(i, c);
                if(item){
                    item->setSelected(start);
                }
            }
            if(start) {
                InitCalibValues(i);
                target_flow_edit_->setText(QString::number(GetTargetFlow(i)));
            } else {
                target_flow_edit_->setText("");
            }
        });

        auto verify_btn = new QPushButton("验证标定");
        verify_btn->setMinimumWidth(40);
        verify_btn->setObjectName("CalibBtn");
        displace_table_->setCellWidget(i, 4, verify_btn);
        connect(verify_btn, &QPushButton::clicked, this, [this, i, verify_btn](){
        });

    }
    displace_table_->setVerticalHeaderLabels(h_headers);
    displace_table_->setHorizontalHeaderLabels({"位移标定值", "电流标定值", "标定预估值", "标定", "验证"});

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
    target_flow_edit_ = new QLineEdit();
    target_flow_edit_->setAlignment(Qt::AlignCenter);

#if 0
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
#endif

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
    sub_layout->addWidget(target_flow_edit_);
    sub_layout->addStretch();
#if 0
    sub_layout->addWidget(state_btn, Qt::AlignRight);
#endif
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
        auto selected_calib_item = displace_table_->item(select_calib_, 2);
        selected_calib_item->setText(QString::number(value));
        real_flow_edit->setText(QString::number(value));
        UpdateCalibInfo();
    });
    connect(displace_table_, &QTableWidget::cellClicked, [this](int row, int col){
        if(calib_state_ == kStart && row != select_calib_) return;
        auto control_item = displace_table_->item(row, 2); 
        range_slider_->setValue(control_item->text().toInt());
    });
    connect(save_btn, &QPushButton::clicked , this, CalibrationPage::OnSaveCalibValueBtnCLicked);

    return displacement_group_;
}

void CalibrationPage::UpdateCalibInfo() {
    auto v_head = displace_table_->verticalHeaderItem(select_calib_)->text();
    auto calib_item = displace_table_->item(select_calib_, 0);
    auto control_item = displace_table_->item(select_calib_, 2);
    QString state_str = "";
    if(calib_state_ == kStart) {
        state_str = "开始标定";
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
        for(int c = 0; c < 3; c++) {
            QTableWidgetItem* item = displace_table_->item(i, c);
            if(item) item->setSelected(false);
        }
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
        auto control_item = displace_table_->item(i, 2); 
        if(i == row) {
            range_slider_->setValue(control_item->text().toInt());
        }
        SetRowCalib(i, i == row);
        for(int c = 0; c < 3; c++) {
            QTableWidgetItem* item = displace_table_->item(i, c);
            if(item) item->setSelected(i == row);
        }
    }
}

void CalibrationPage::SetRowCalib(int row, bool calib) {
    if(row < 0 || row > displace_table_->rowCount() - 1) {
        return;
    }
    auto calib_item = displace_table_->item(row, 0);
    auto control_item = displace_table_->item(row, 2);
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
    auto up_time_label = new QLabel("斜坡时间(ms)");
    auto down_time_label = new QLabel("工作位停留时间(ms)");
    auto stop_time_label = new QLabel("中位停留时间(ms)");

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
    left_agix_combo->addItem("流量");
    auto right_agix_label = new QLabel("右轴:");
    auto right_agix_combo = new QComboBox();
    right_agix_combo->addItem("位移");
    auto bottom_agix_label = new QLabel("底轴:");
    auto bottom_agix_combo = new QComboBox();
    bottom_agix_combo->addItem("时间(s)");

    auto clear_btn = new QPushButton("清除");

    sub_layout->addWidget(left_agix_label);
    sub_layout->addWidget(left_agix_combo);
    sub_layout->addWidget(right_agix_label);
    sub_layout->addWidget(right_agix_combo);
    sub_layout->addWidget(bottom_agix_label);
    sub_layout->addWidget(bottom_agix_combo);
    sub_layout->addStretch();
    sub_layout->addWidget(clear_btn, 0, Qt::AlignRight);

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
    m_wavePlot = new QWavePlotWithLegendWidget(this);

    QWavePlotWidget::AxisConfig cfg;
    cfg.xLabel = "时间(s)";
    cfg.leftYLabel = "流量";
    cfg.rightYLabel = "位移";
    m_wavePlot->setupAxis(cfg);

    // 添加曲线：正弦波绑定左Y；锯齿波绑定右Y
    idxSine_1 = m_wavePlot->addCurve("正弦波1(流量)", QPen(Qt::blue,2), false);
    idxSine_2 = m_wavePlot->addCurve("正弦波2(流量)", QPen(Qt::black,2), false);
    idxSaw_1 = m_wavePlot->addCurve("锯齿波 1侧(位移)", QPen(Qt::red,2), true);
    idxSaw_2 = m_wavePlot->addCurve("锯齿波 2侧(位移)", QPen(Qt::green,2), true);
    main_layout->addLayout(sub_layout);
    main_layout->addWidget(m_wavePlot);

    connect(this, &CalibrationPage::SendDrawStayFaInfo, this, CalibrationPage::DrawStay);
    connect(clear_btn, &QPushButton::clicked, [this] (){
        m_wavePlot->clearAll();
        m_time_ = QDateTime::currentDateTime();
        m_time = 0.0;
    });
    return waveform_group_;
}

void CalibrationPage::StartControl1Loop() {
    if(stay_thread_1_ != nullptr){
        return; //已经启动，防止重复创建
    }

    stay_1_running_ = true;
    if(!stay_2_running_) m_time_ = QDateTime::currentDateTime();
    stay_thread_1_ = QThread::create([this](){
        // 子线程循环，等价原来定时器不断触发OnDrawStayFa1
        while(stay_1_running_) {
            OnDrawStayFa1(); //执行你的业务函数
            QThread::msleep(200);
        }
    });
    stay_thread_1_->start();
}

void CalibrationPage::StartControl2Loop() {
    if(stay_thread_2_ != nullptr){
        return; //已经启动，防止重复创建
    }

    stay_2_running_ = true;
    if(!stay_1_running_) m_time_ = QDateTime::currentDateTime();
    stay_thread_2_ = QThread::create([this](){
        // 子线程循环，等价原来定时器不断触发OnDrawStayFa1
        while(stay_2_running_) {
            OnDrawStayFa2(); //执行你的业务函数
            QThread::msleep(200);
        }
    });
    stay_thread_2_->start();
}

void CalibrationPage::StopControl1Loop() {
    if(stay_thread_1_ == nullptr) {
        return;
    }

    stay_1_running_ = false; //退出循环条件
    stay_thread_1_->quit();
    stay_thread_1_->wait(); //阻塞等待线程安全结束
    delete stay_thread_1_;
    stay_thread_1_ = nullptr;
}

void CalibrationPage::StopControl2Loop() {
    if(stay_thread_2_ == nullptr) {
        return;
    }

    stay_2_running_ = false; //退出循环条件
    stay_thread_2_->quit();
    stay_thread_2_->wait(); //阻塞等待线程安全结束
    delete stay_thread_2_;
    stay_thread_2_ = nullptr;
}

// double sineVal_1 = 10 * sin(2*M_PI*0.5*m_time);
// double sineVal_2 = 5 * sin(3*M_PI*0.5*m_time);
// double sawVal_1 = fmod(m_time*20,40)-20;
// double sawVal_2 = fmod(m_time*10,20)-10;
// 位移曲线绘制
void CalibrationPage::OnDrawStayFa1() {
    
    CanDriver::GetInstance()->ExecCmd(SDO_COB_ID, cur_fa_val_1_cmd_, 100);
    std::lock_guard<std::mutex> lk(m_time_mtx_);
    DrawStayInfo info;
    info.side = 1;
    info.time = m_time;
#ifdef ON_TEST_MODE
    info.value = fmod(m_time*10,10);
#else
    can_frame frame{};
    bool ret = false;
    qint64 startMs = QDateTime::currentMSecsSinceEpoch();
    const qint64 timeoutMs = 100;

    while(true)
    {
        // 剩余时间
        qint64 elapsed = QDateTime::currentMSecsSinceEpoch() - startMs;
        if(elapsed >= timeoutMs)
        {
            std::cout << "receive 100ms timeout, no 0x1C0 frame" << std::endl;
            return;
        }
        // 剩余时间作为阻塞超时，不要固定写死100
        ret = CanDriver::GetInstance()->receive(frame, static_cast<int>(timeoutMs - elapsed));
        if(!ret)
        {
            std::cout << "CAN receive fail" << std::endl;
            return;
        }
        // 找到目标ID才跳出循环
        if(frame.can_id == 0x1C0)
        {
            break;
        }
        // 其他ID，继续循环接收下一条，不return
        std::cout << "discard frame id:" << frame.can_id << std::endl;
    }

    // 到这里：一定是can_id ==0x1C0
    if(frame.can_dlc != 8)
    {
        std::cout << "CAN frame data length too short:" << frame.can_dlc << std::endl;
        return;
    }

    uint16_t raw = ExtractFromDataList(frame.data,3,2);

    // ✅增加数值合理性校验，过滤乱码
    // const uint16_t rawMin = 0;
    // const uint16_t rawMax = 1000; // 根据实际物理范围设置
    // if(raw < rawMin || raw > rawMax)
    // {
    //     std::cout << "raw value out of range! raw=" << raw << std::endl;
    //     return;
    // }
    info.value = static_cast<double>(raw) *170.0 / 1000.0;
#endif
    auto now = QDateTime::currentDateTime();
    int diss = m_time_.msecsTo(now);
    m_time += static_cast<double>(diss) / 1000.0;
    SendDrawStayFaInfo(info);
    m_time_ = QDateTime::currentDateTime();
}

void CalibrationPage::OnDrawStayFa2() {

    CanDriver::GetInstance()->ExecCmd(SDO_COB_ID, cur_fa_val_2_cmd_, 100);
    std::lock_guard<std::mutex> lk(m_time_mtx_);
    DrawStayInfo info;
    info.side = 2;
    info.time = m_time;    
#ifdef ON_TEST_MODE
    info.value = fmod(m_time*10,10);
#else
    can_frame frame{};
    bool ret = false;
    qint64 startMs = QDateTime::currentMSecsSinceEpoch();
    const qint64 timeoutMs = 100;

    while(true)
    {
        // 剩余时间
        qint64 elapsed = QDateTime::currentMSecsSinceEpoch() - startMs;
        if(elapsed >= timeoutMs)
        {
            std::cout << "receive 100ms timeout, no 0x1C0 frame" << std::endl;
            return;
        }
        // 剩余时间作为阻塞超时，不要固定写死100
        ret = CanDriver::GetInstance()->receive(frame, static_cast<int>(timeoutMs - elapsed));
        if(!ret)
        {
            std::cout << "CAN receive fail" << std::endl;
            return;
        }
        // 找到目标ID才跳出循环
        if(frame.can_id == 0x2C0)
        {
            break;
        }
        // 其他ID，继续循环接收下一条，不return
        std::cout << "discard frame id:" << frame.can_id << std::endl;
    }

    // 到这里：一定是can_id ==0x1C0
    if(frame.can_dlc != 8)
    {
        std::cout << "CAN frame data length too short:" << frame.can_dlc << std::endl;
        return;
    }

    uint16_t raw = ExtractFromDataList(frame.data,3,2);

    // ✅增加数值合理性校验，过滤乱码
    // const uint16_t rawMin = 0;
    // const uint16_t rawMax = 1000; // 根据实际物理范围设置
    // if(raw < rawMin || raw > rawMax)
    // {
    //     std::cout << "raw value out of range! raw=" << raw << std::endl;
    //     return;
    // }
    info.value = static_cast<double>(raw) *170.0 / 1000.0;
#endif
    m_time += static_cast<double>(m_time_.msecsTo(QDateTime::currentDateTime())) / 1000.0;
    SendDrawStayFaInfo(info);
    m_time_ = QDateTime::currentDateTime();
}

void CalibrationPage::DrawStay(const DrawStayInfo &info) {
    int control_side = info.side;
    if(control_side != 1 && control_side != 2) {
        return;
    }
    // qDebug() << "DrawStay("<<control_side << "): "<< info.time << " - "<< info.value;
    if(control_side == 1) {
        m_wavePlot->appendData(idxSaw_1, info.time, info.value);
    } else {
        m_wavePlot->appendData(idxSaw_2, info.time, info.value);
    }
}