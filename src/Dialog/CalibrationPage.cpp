#include "CalibrationPage.h"


#include "TaskMgr.h"
#include "can_cmd.h"
#include "Utils.h"
#include "MgrUtil.h"
#include "QWavePlotWithLegendWidget.h"
#include "CustomDelegate.h"
#include <QDebug>
#include <iostream>
#include <QThread>
#include <mutex>
#include <map>
#include <QIntValidator>
#include <vector>
#include <utility>

// #define kCalibratScale 10

CalibrationPage::CalibrationPage(QWidget* parent)
    : QWidget(parent) {

    InitPage();
    qRegisterMetaType<DrawCurveInfo>("DrawCurveInfo");
    

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
    connect(this, &CalibrationPage::SendInfoChanged, basic_info_bar_, &BasicInfoBar::OnChangeInfo);
    connect(CanManager::GetInstance(), &CanManager::SendTpdo2Position, this, &CalibrationPage::OnReadTpdo2Position);
    connect(CanManager::GetInstance(), &CanManager::SendTpdo3Current, this, &CalibrationPage::OnReadTpdo3Current);
    connect(this, &CalibrationPage::SendStopReadNMTCmd, CanManager::GetInstance(), &CanManager::OnStopNMTRead);

    InitPageTimer();
}

void CalibrationPage::InitBasicInfo(BasicInfo info) { basic_info_bar_->InitData(info); }

void CalibrationPage::InitPageTimer() {
    stay_timer_1_ = new QTimer();
    stay_timer_2_ = new QTimer();
    connect(stay_timer_1_, &QTimer::timeout, this, &CalibrationPage::OnTimePushCmd);
    connect(stay_timer_2_, &QTimer::timeout, this, &CalibrationPage::OnTimePushCmd);
}

void CalibrationPage::OnTimePushCmd() {
    if(curr_side_ == kSideNone) {
        return;
    }
    switch (curr_side_) {
    case kSideOne:
        CAN_MGR_PUSH_CMD(SEND_COB_ID, cur_fa_val_1_cmd_);
        break;
    case kSideTwo:
        CAN_MGR_PUSH_CMD(SEND_COB_ID, cur_fa_val_2_cmd_);
        break;
    break;    
    default:
        break;
    }
}

void CalibrationPage::OpenTimer(LoopSide side) {
    curr_side_ = side;
    switch (side) {
    case kSideOne:
        stay_timer_1_->start(500);
        break;
    case kSideTwo:
        stay_timer_2_->start(500);
        break;
    break;    
    default:
        break;
    }
}

void CalibrationPage::CloseTimer(LoopSide side) {
    auto CloseT = [this](QTimer *timer){
        if(timer == nullptr) return;
        if(timer->isActive()) {
            timer->stop();
        }
    };

    switch (side) {
    case kSideOne:
        CloseT(stay_timer_1_);
        break;
    case kSideTwo:
        CloseT(stay_timer_2_);
        break;
    break;    
    default:
        break;
    }
    curr_side_ = kSideNone;
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
    auto output_cycle_2_label = new QLabel("实际电流值(mA)");
    auto cycle_count_label = new QLabel("循环次数");
    auto neutral_time_label = new QLabel("中位停留时间(ms)");
    auto work_time_label = new QLabel("工作位停留时间(ms)");

    output_cycle_1_edit_ = new QLineEdit("10");
    output_cycle_1_edit_->setAlignment(Qt::AlignCenter);
    output_cycle_2_edit_ = new QLineEdit("300");
    output_cycle_2_edit_->setAlignment(Qt::AlignCenter);
    QIntValidator *intVal = new QIntValidator(0, 630, output_cycle_2_edit_);
    output_cycle_2_edit_->setValidator(intVal);
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
    CAN_MGR_PUSH_CMD(SDO_COB_ID, cmd);
    cur_loop_mode_ = mode;
}

void CalibrationPage::HandleControlEvent(bool checked, const ControlContext& ctx)
{
    // 记录当前模式，后续 OnTimePushCmd / 停止分支都会用到
    cur_loop_mode_ = ctx.loop_mode;

    CanDriver::GetInstance()->FlushBuffers();

    // ---------------- 停止 ----------------
    if (!checked) {
        CloseTimer(ctx.side);
        CanManager::GetInstance()->ClearCommands();
        if (ctx.side == kSideOne) {
            cur_fa_val_1_cmd_ = ctx.close_cmd;
        } else if (ctx.side == kSideTwo) {
            cur_fa_val_2_cmd_ = ctx.close_cmd;
        }

        CAN_MGR_PUSH_CMD(SEND_COB_ID, ctx.close_cmd);
        QThread::msleep(200);
        // CAN_MGR_PUSH_CMD(NMT_COB_ID, NMT_CLOSE_READ_CMD);
        emit SendStopReadNMTCmd();
        StopDrawThread();
        // emit SendStopReadNMTCmd();
        return;
    }

    // ---------------- 启动 ----------------
    CAN_MGR_PUSH_CMD(NMT_COB_ID, NMT_READ_VALUE_CMD);

    // 闭环时下发 PID
    if (ctx.need_set_pid) {
        SetPIDParam();
    }

    // 斜坡响应：先下发斜坡时间
    if (ctx.need_ramp) {
        bool ok = false;
        int ramp_time = ramp_edit_->text().toInt(&ok);
        if (!ok) {
            std::cout << "输入数值非法" << std::endl;
            return;
        }
        CAN_MGR_PUSH_CMD(RPDO2_COB_ID,
            RampTimeCMDConfig(SDO_RPDO2_RAMP_TIME_CMD, ramp_time));
    }

    // 保存命令帧
    if (ctx.side == kSideOne) {
        cur_fa_val_1_cmd_ = SetTargetCMDValue(ctx.target_cmd, ctx.value, ctx.factor);
    } else if (ctx.side == kSideTwo) {
        cur_fa_val_2_cmd_ = SetTargetCMDValue(ctx.target_cmd, ctx.value, ctx.factor);
    }

    StartDrawThread();
    OpenTimer(ctx.side);
}

// Implementation for control 1 button click
void CalibrationPage::OnControl1BtnClicked(bool checked) {
    if (checked && control_2_btn_->isChecked()) {
        control_2_btn_->setChecked(false);
        OnControl2BtnClicked(false);
    }

    ControlContext ctx;
    ctx.side = kSideOne;
    ctx.value = output_cycle_1_edit_->text().toInt();
    ctx.target_cmd = SDO_PWM_OPEN_1_VALUE_CMD;
    ctx.close_cmd = SDO_WRITE_CLOSE_1_CMD;
    ctx.factor = 10;
    ctx.loop_mode = kOpenLoop;

    HandleControlEvent(checked, ctx);
}

// Implementation for control 2 button click
void CalibrationPage::OnControl2BtnClicked(bool checked) {
    if (checked && control_1_btn_->isChecked()) {
        control_1_btn_->setChecked(false);
        OnControl1BtnClicked(false);
    }

    ControlContext ctx;
    ctx.side = kSideTwo;
    ctx.value = output_cycle_1_edit_->text().toInt();
    ctx.target_cmd = SDO_PWM_OPEN_2_VALUE_CMD;
    ctx.close_cmd = SDO_WRITE_CLOSE_2_CMD;
    ctx.factor = 10;
    ctx.loop_mode = kOpenLoop;

    HandleControlEvent(checked, ctx);
}

// 1侧流量开环控制
void CalibrationPage::OnControlCur1BtnClicked(bool checked) {
    if (checked && control_cur_2_btn_->isChecked()) {
        control_cur_2_btn_->setChecked(false);
        OnControlCur2BtnClicked(false);
    }

    ControlContext ctx;
    ctx.side = kSideOne;
    ctx.value = output_cycle_2_edit_->text().toInt();
    ctx.target_cmd = SDO_CUR_OPEN_1_VALUE_CMD;
    ctx.close_cmd = SDO_WRITE_CLOSE_1_CMD;
    ctx.factor = 1;
    ctx.loop_mode = kOpenLoop;

    HandleControlEvent(checked, ctx);
}

// 2侧流量开环控制
void CalibrationPage::OnControlCur2BtnClicked(bool checked) {
    if (checked && control_cur_1_btn_->isChecked()) {
        control_cur_1_btn_->setChecked(false);
        OnControlCur1BtnClicked(false);
    }

    ControlContext ctx;
    ctx.side = kSideTwo;
    ctx.value = output_cycle_2_edit_->text().toInt();
    ctx.target_cmd = SDO_CUR_OPEN_2_VALUE_CMD;
    ctx.close_cmd = SDO_WRITE_CLOSE_2_CMD;
    ctx.factor = 1;
    ctx.loop_mode = kOpenLoop;

    HandleControlEvent(checked, ctx);
}

// Implementation for cycle button click
void CalibrationPage::OnCycleBtnClicked(bool checked) {
    cur_loop_mode_ = kOpenLoop;
    std::cout << "cycle clicked, checked:" << checked << std::endl;
    if (!checked) {
        StopLoopCycle();
        // CAN_MGR_PUSH_CMD(NMT_COB_ID, NMT_CLOSE_READ_CMD);
        emit SendStopReadNMTCmd();
        return;
    } else {
        // ChangeLoopMode(kOpenLoop);
        // CAN_MGR_PUSH_CMD(SDO_COB_ID, SDO_OPEN_LOOP_MODE_CMD);
    }

    if (!StartLoopCycle()) {
        // 启动失败，将按钮恢复为未选中，避免状态误导
        cycle_btn_->setChecked(false);
    }
}

bool CalibrationPage::StartLoopCycle() {

    // 已经在运行，禁止重复启动
    if (is_open_running_.load() || open_loop_thread_ != nullptr) {
        return false;
    }
    is_open_running_.store(true);
    open_loop_thread_ = QThread::create([this]() {
        ExecuteLoopCycle();
    });
    open_loop_thread_->start();

    return true;
}

void CalibrationPage::StopLoopCycle() {
    if (is_stopping_.exchange(true)) return;

    stop_requested_.store(true);            // 原子写
    is_open_running_.store(false);
    cv_.notify_all();                       // 立刻唤醒 WaitAndResend

    if(draw_curve_running_.load()) {
        StopDrawThread();
    }

    if (open_loop_thread_ != nullptr) {
        open_loop_thread_->wait();
        delete open_loop_thread_;
        open_loop_thread_ = nullptr;
    }

    stop_requested_.store(false);           // 原子写，为下次启动复位
    is_stopping_.store(false);
}

void CalibrationPage::ExecuteLoopCycle() {
// TODO: 
    int percent_1, percent_2, cycle_count, neutral_stay_time, work_stay_time, factor;
    std::array<uint8_t, 8> send_target_1_cmd, send_target_2_cmd;
    if(cur_loop_mode_ == kOpenLoop) {
        percent_1 = output_cycle_1_edit_->text().toInt();
        percent_2 = output_cycle_1_edit_->text().toInt();
        cycle_count = cycle_count_edit_->text().toInt();
        neutral_stay_time = neutral_time_edit_->text().toInt();
        work_stay_time = work_time_edit_->text().toInt();
        send_target_1_cmd = SDO_PWM_OPEN_1_VALUE_CMD;
        send_target_2_cmd = SDO_PWM_OPEN_2_VALUE_CMD;
        cur_fa_val_1_cmd_ = SDO_WRITE_CLOSE_1_CMD;
        cur_fa_val_2_cmd_ = SDO_WRITE_CLOSE_1_CMD;   // 初始状态，公用一个传感器，只能发1不发2，发2不发1     
        factor = 10;
    } else {
        percent_1 = target_edit_->text().toInt();
        percent_2 = target_edit_->text().toInt();
        cycle_count = 1;
        neutral_stay_time = kCloseCycleWaitTime * 1000;
        work_stay_time = kCloseCycleWaitTime * 1000;
        send_target_1_cmd = SDO_SEND_TARGET_1_VALUE_CMD;
        send_target_2_cmd = SDO_SEND_TARGET_2_VALUE_CMD;
        cur_fa_val_1_cmd_ = SDO_STOP_1_CMD;
        cur_fa_val_2_cmd_ = SDO_STOP_1_CMD; // 初始状态，公用一个传感器，只能发1不发2，发2不发1  
        factor = 1;
    }
    int target_value_1 = percent_1 * 10; // = percent_1 / 100 * 1000;
    int target_value_2 = percent_2 * 10; // = percent_2 / 100 * 1000;

    // 循环次数
    int loop_count = 0;
    CAN_MGR_PUSH_CMD(NMT_COB_ID, NMT_READ_VALUE_CMD);
    if(!draw_curve_running_.load()) {
        StartDrawThread();
    }
    while(is_open_running_.load() && loop_count < cycle_count) {
        if(stop_requested_) {
            break;
        }
        if (!RunOpenLoopSide(kSideOne, percent_1, send_target_1_cmd,
                             neutral_stay_time, work_stay_time, factor)) break;
        if (!RunOpenLoopSide(kSideTwo, percent_2, send_target_2_cmd,
                             neutral_stay_time, work_stay_time, factor)) break;

        loop_count++;
        qDebug() << "-------一个循环结束-------";
    }
    qDebug() << "loop count: "<< loop_count;
    // is_on_work_stay_time_ = false;
    if (is_open_running_.load()) {   // 正常完成
        emit SendOpenLoopFinished();
    }
    // CAN_CLEAR_BUFF
    // CanDriver::GetInstance()->FlushRxBuffer();
}

// ------------------------------------------------------------------
// 发送一帧开环指令
// ------------------------------------------------------------------
void CalibrationPage::PushOpenLoopCmd(LoopSide side,
                                      const std::array<uint8_t, 8>& target_cmd,
                                      int percent, int factor)
{
    if (side == kSideOne) {
        cur_fa_val_1_cmd_ = SetTargetCMDValue(target_cmd, percent, factor);
        CAN_MGR_PUSH_CMD(SEND_COB_ID, cur_fa_val_1_cmd_);
    } else if (side == kSideTwo) {
        cur_fa_val_2_cmd_ = SetTargetCMDValue(target_cmd, percent, factor);
        CAN_MGR_PUSH_CMD(SEND_COB_ID, cur_fa_val_2_cmd_);
    }
}

// ------------------------------------------------------------------
// 边等待、边重发
// ------------------------------------------------------------------
bool CalibrationPage::WaitAndResend(int total_ms, LoopSide side,
                                    const std::array<uint8_t, 8>& target_cmd,
                                    int percent, int factor)
{
    using clock = std::chrono::steady_clock;
    const auto deadline = clock::now() + std::chrono::milliseconds(total_ms);

    while (!stop_requested_.load() && is_open_running_.load()) {
        // 1) 先发一帧（保证 t=0 时刻命令已下发）
        PushOpenLoopCmd(side, target_cmd, percent, factor);

        // 2) 到时间就退出
        const auto now = clock::now();
        if (now >= deadline) break;

        const auto remain_ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now).count();
        if (remain_ms <= 0) break;

        // 3) 只睡 min(剩余, 重发间隔)，保证能及时醒来重发
        const auto wait_ms = std::min<long long>(remain_ms, 500);

        // 注意：CAN 发送在锁外完成，持锁期间只做 wait
        std::unique_lock<std::mutex> lock(cv_mtx_);
        cv_.wait_for(lock, std::chrono::milliseconds(wait_ms),
                     [this] {
                         return stop_requested_.load()
                             || !is_open_running_.load();
                     });
    }

    return !stop_requested_.load() && is_open_running_.load();
}

// ------------------------------------------------------------------
// 单侧完整时序：中位 → 工作位 → 中位
// ------------------------------------------------------------------
bool CalibrationPage::RunOpenLoopSide(LoopSide side, int percent,
                                      const std::array<uint8_t, 8>& target_cmd,
                                      int neutral_ms, int work_ms, int factor)
{
    curr_side_ = side;
    const char *tag = (side == kSideOne) ? "1" : "2";

    qDebug() << tag << " 中位停留： " << neutral_ms / 1000 << "s";
    if (!WaitAndResend(neutral_ms, side, target_cmd, 0, factor)) return false;

    qDebug() << tag << " 工作位停留： " << work_ms / 1000 << "s";
    if (!WaitAndResend(work_ms, side, target_cmd, percent, factor)) return false;


    qDebug() << tag << " 中位停留： " << neutral_ms / 1000 << "s";
    if (!WaitAndResend(neutral_ms, side, target_cmd, 0, factor)) return false;

    emit SendMarkGapAllCurves(); 

    return true;
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
    CAN_MGR_PUSH_CMDS(cmds);
}

void CalibrationPage::OnPIDStepBtnClicked(bool checked) {
    qDebug() << "OnPIDStepBtnClicked: "<<checked;
    if (checked && ramp_btn_->isChecked()) {
        ramp_btn_->setChecked(false);
        OnPIDRampBtnClicked(false);
    }

    ControlContext ctx;
    ctx.side        = IsOnSideControl1() ? kSideOne : kSideTwo;
    ctx.value       = target_edit_->text().toInt();
    ctx.target_cmd  = (ctx.side == kSideOne) ? SDO_SEND_TARGET_1_VALUE_CMD
                                             : SDO_SEND_TARGET_2_VALUE_CMD;
    ctx.close_cmd = (ctx.side == kSideOne) ? SDO_STOP_1_CMD
                                           : SDO_STOP_2_CMD;
    ctx.factor      = 1;
    ctx.loop_mode   = kClosedLoop;
    ctx.need_set_pid = true;

    HandleControlEvent(checked, ctx);
}

void CalibrationPage::OnPIDRampBtnClicked(bool checked) {
    qDebug() << "OnPIDRampBtnClicked: "<<checked;
    if (checked && step_btn_->isChecked()) {
        step_btn_->setChecked(false);
        OnPIDStepBtnClicked(false);
    }

    ControlContext ctx;
    ctx.side        = IsOnSideControl1() ? kSideOne : kSideTwo;
    ctx.value       = target_edit_->text().toInt();
    ctx.target_cmd  = (ctx.side == kSideOne) ? SDO_SEND_TARGET_1_VALUE_CMD
                                             : SDO_SEND_TARGET_2_VALUE_CMD;
    ctx.close_cmd = (ctx.side == kSideOne) ? SDO_STOP_1_CMD
                                           : SDO_STOP_2_CMD;
    ctx.factor      = 1;
    ctx.loop_mode   = kClosedLoop;
    ctx.need_set_pid = true;
    ctx.need_ramp    = true;

    HandleControlEvent(checked, ctx);
}

void CalibrationPage::OnPIDMotionBtnClicked(bool checked) {
    cur_loop_mode_ = kClosedLoop;
    std::cout << "close cycle clicked, checked:" << checked << std::endl;
    if (!checked) {
        // CAN_MGR_PUSH_CMD(NMT_COB_ID, NMT_CLOSE_READ_CMD);
        emit SendStopReadNMTCmd();
        StopLoopCycle();
        return;
    } else {
        // CAN_MGR_PUSH_CMD(SDO_COB_ID, SDO_RAMP_MODE_CMD);
        SetPIDParam();
    }

    bool ok = false;
    int ramp_time = ramp_edit_->text().toInt(&ok);
    if(!ok) {
        std::cout << "输入数值非法"<<std::endl;
        return;
    }

    CAN_MGR_PUSH_CMD(RPDO2_COB_ID, RampTimeCMDConfig(SDO_RPDO2_RAMP_TIME_CMD, ramp_time));

    StartLoopCycle();
}

void CalibrationPage::OnPIDSaveBtnClicked() {
    QString p_sub_idx = IsOnSideControl1() ? "0x02" : "0x06";
    QString i_sub_idx = IsOnSideControl1() ? "0x03" : "0x07";
    QString d_sub_idx = IsOnSideControl1() ? "0x04" : "0x08";

    QString p_v = QString::number(static_cast<int>(p_edit_->text().toDouble() * 1000));
    QString i_v = QString::number(static_cast<int>(i_edit_->text().toDouble() * 1000));
    QString d_v = QString::number(static_cast<int>(d_edit_->text().toDouble() * 1000));

    emit SendRowValue(p_v, "0x2020", p_sub_idx);
    emit SendRowValue(i_v, "0x2020", i_sub_idx);
    emit SendRowValue(d_v, "0x2020", d_sub_idx);
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

    QStringList h_headers = {"100%", "50%", "25%", "10%", "0%", "中位", "0%", "10%", "25%", "50%", "100%"};
    for (int i = 0; i < 11; ++i) {
        // displace_table_->setColumnWidth(i, 90);
        displace_table_->setRowHeight(i, 40);

        //位移标定值
        auto calib_stay_item = new QTableWidgetItem(QString::number(GetCalibratYuGuValue(i)));
        calib_stay_item->setTextAlignment(Qt::AlignCenter);
        displace_table_->setItem(i, 0, calib_stay_item);

        //电流标定值
        auto calib_current_item = new QTableWidgetItem("0");
        calib_current_item->setTextAlignment(Qt::AlignCenter);
        displace_table_->setItem(i, 1, calib_current_item);

        //标定预估值
        auto control_value_item = new QTableWidgetItem(QString::number(GetCalibratYuGuValue(i)));
        control_value_item->setTextAlignment(Qt::AlignCenter);
        displace_table_->setItem(i, 2, control_value_item);

        // 标定 
        auto calibItem = new QTableWidgetItem("开始标定");
        calibItem->setTextAlignment(Qt::AlignCenter);
        displace_table_->setItem(i, 3, calibItem);
        // 验证标定
        auto verifyItem = new QTableWidgetItem("验证标定");
        verifyItem->setTextAlignment(Qt::AlignCenter);
        displace_table_->setItem(i, 4, verifyItem);
    }

    ButtonDelegate *btnDelegate = new ButtonDelegate(this);
    btnDelegate->setButtonColumns({3, 4});    // 第3、4列为按钮列
    displace_table_->setItemDelegateForColumn(3, btnDelegate);
    displace_table_->setItemDelegateForColumn(4, btnDelegate);

    connect(btnDelegate, &ButtonDelegate::buttonClicked,
            this, &CalibrationPage::OnCalibButtonClicked);

    displace_table_->setVerticalHeaderLabels(h_headers);
    displace_table_->setHorizontalHeaderLabels({"位移标定值", "电流标定值", "标定预估值", "标定", "验证"});

    info_label_ = new QLabel("当前标定：");

    auto sub_layout = new QHBoxLayout();
    auto displace_target_label = new QLabel("位移目标值:");
    displace_target_edit_ = new QLineEdit();
    displace_target_edit_->setAlignment(Qt::AlignCenter);
    auto actual_value_label = new QLabel("位移实际值:");
    actual_value_edit_ = new QLineEdit();
    actual_value_edit_->setAlignment(Qt::AlignCenter);
    actual_value_edit_->setReadOnly(true);
    auto real_flow_label = new QLabel("实时流量:");
    auto real_flow_edit = new QLineEdit();
    real_flow_edit->setAlignment(Qt::AlignCenter);
    auto target_flow_label = new QLabel("目标流量:");
    target_flow_edit_ = new QLineEdit();
    target_flow_edit_->setAlignment(Qt::AlignCenter);

    auto save_btn = new QPushButton("保存标定值");
    save_btn->setObjectName("StateBtn");
    save_btn->setFixedHeight(30);
    save_btn->setMinimumWidth(100);

    sub_layout->addWidget(displace_target_label);
    sub_layout->addWidget(displace_target_edit_);
    sub_layout->addStretch();
    sub_layout->addWidget(actual_value_label);
    sub_layout->addWidget(actual_value_edit_);
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
    range_slider_->SetRange(0, 170);
    main_layout->addWidget(range_slider_);
    
    connect(range_slider_, &QRangeSlider::valueChanged, [=](int value){
        if(!on_calibrat_) {
            return;
        }        
        displace_target_edit_->setText(QString::number(value));
        auto step_item = displace_table_->item(select_calib_, 0);
        step_item->setText(QString::number(value));
        auto selected_calib_item = displace_table_->item(select_calib_, 2);
        selected_calib_item->setText(QString::number(value));
        int yugu_v = CalcDisplacement(value, select_calib_);
        // qDebug() << "预估： "<<yugu_v;
        if(select_calib_ < 6) {
            cur_fa_val_1_cmd_ = SetTargetCMDValue(SDO_PWM_OPEN_1_VALUE_CMD, yugu_v);
        }else {
            cur_fa_val_2_cmd_ = SetTargetCMDValue(SDO_PWM_OPEN_2_VALUE_CMD, yugu_v);
        }

        UpdateCalibInfo();
    });
    connect(displace_table_, &QTableWidget::cellClicked, [this](int row, int col){
        select_calib_ = row;
        if(!on_calibrat_) return;
        auto control_item = displace_table_->item(row, 2); 
        range_slider_->setValue(control_item->text().toInt());
    });
    connect(save_btn, &QPushButton::clicked , this, CalibrationPage::OnSaveCalibValueBtnCLicked);

    connect(this, &CalibrationPage::SendCalibCurrentValue, this, [this](int value){
        if(!on_calibrat_) return;
        auto control_item = displace_table_->item(select_calib_, 1);
        control_item->setText(QString::number(value));
        UpdateCalibInfo();
    });

    return displacement_group_;
}

void CalibrationPage::OnCalibButtonClicked(int row, int column) {
    if (column == 3) {   // 标定列
        // 原 lambda 逻辑，但将 calib_btn 操作替换为表格项操作
        select_calib_ = row;
        InitCalibState(row);   // 改为行索引

        // 获取当前标定状态（通过表格项文本判断）
        QTableWidgetItem *item = displace_table_->item(row, 3);
        bool isCalibrating = (item->text() == "结束标定");

        if (!isCalibrating) {
            // 开始标定
            item->setText("结束标定");
            calib_state_ = kStart;
            on_calibrat_ = true;
            // ... 原有开始标定逻辑
            InitCalibValues(row);
            target_flow_edit_->setText(QString::number(GetTargetFlow(row)));
            CAN_MGR_PUSH_CMD(NMT_COB_ID, NMT_READ_VALUE_CMD);
            int value = GetCalibratValue(row);
            if (row < 6) {
                cur_fa_val_1_cmd_ = SetTargetCMDValue(SDO_PWM_OPEN_1_VALUE_CMD, value);
            } else {
                cur_fa_val_2_cmd_ = SetTargetCMDValue(SDO_PWM_OPEN_2_VALUE_CMD, value);
            }
            LoopSide side = row < 6 ? kSideOne : kSideTwo;
            OpenTimer(side);
            StartDrawThread();
        } else {
            // 结束标定
            item->setText("开始标定");
            calib_state_ = kEnd;
            on_calibrat_ = false;
            SetRowCalib(row, false);
            if (!already_on_calib_) {
                if (row < 6) {
                    CAN_MGR_PUSH_CMD(SEND_COB_ID, SDO_WRITE_CLOSE_1_CMD);
                } else {
                    CAN_MGR_PUSH_CMD(SEND_COB_ID, SDO_WRITE_CLOSE_2_CMD);
                }
                LoopSide side = row < 6 ? kSideOne : kSideTwo;
                StopDrawThread();
                CloseTimer(side);
                // CAN_MGR_PUSH_CMD(NMT_COB_ID, NMT_CLOSE_READ_CMD);
                emit SendStopReadNMTCmd();
            }
        }
        // 更新高亮（选中前三列）
        for (int c = 0; c < 3; ++c) {
            QTableWidgetItem *it = displace_table_->item(row, c);
            if (it) it->setSelected(on_calibrat_);
        }
        UpdateCalibInfo();  // 需修改此函数以适应新状态

    } else if (column == 4) {  // 验证列
        // 验证按钮逻辑
        QTableWidgetItem *item = displace_table_->item(row, 4);
        // 判断当前是否正在验证（通过文本或额外标志，此处用文本）
        bool isVerifying = (item->text() == "验证中");  // 可自行定义
        if (!isVerifying) {
            item->setText("验证中");
            // 执行启动验证代码（原 checked==true 分支）
            int yugu_v = CalcDisplacement(displace_table_->item(row, 2)->text().toInt(), row);
            CAN_MGR_PUSH_CMD(NMT_COB_ID, NMT_READ_VALUE_CMD);
            if (row < 6) {
                cur_fa_val_1_cmd_ = SetTargetCMDValue(SDO_PWM_OPEN_1_VALUE_CMD, yugu_v);
            } else {
                cur_fa_val_2_cmd_ = SetTargetCMDValue(SDO_PWM_OPEN_2_VALUE_CMD, yugu_v);
            }
            LoopSide side = row < 6 ? kSideOne : kSideTwo;
            OpenTimer(side);
            StartDrawThread();
            // 将其他验证按钮置为“验证标定”
            for (int r = 0; r < displace_table_->rowCount(); ++r) {
                if (r != row) {
                    auto otherItem = displace_table_->item(r, 4);
                    if (otherItem && otherItem->text() == "验证中")
                        otherItem->setText("验证标定");
                }
            }
        } else {
            // 停止验证
            item->setText("验证标定");
            if (row < 6) {
                CAN_MGR_PUSH_CMD(SEND_COB_ID, SDO_WRITE_CLOSE_1_CMD);
            } else {
                CAN_MGR_PUSH_CMD(SEND_COB_ID, SDO_WRITE_CLOSE_2_CMD);
            }
            LoopSide side = row < 6 ? kSideOne : kSideTwo;
            StopDrawThread();
            CloseTimer(side);
            // CAN_MGR_PUSH_CMD(NMT_COB_ID, NMT_CLOSE_READ_CMD);
            emit SendStopReadNMTCmd();
        }
    }
}

void CalibrationPage::UpdateCalibInfo() {
    auto side = select_calib_ < 6 ? "1侧" : "2侧";
    side = select_calib_ == 5 ? "中位" : side;
    auto v_head = displace_table_->verticalHeaderItem(select_calib_)->text();
    auto calib_item = displace_table_->item(select_calib_, 0);
    auto control_item = displace_table_->item(select_calib_, 2);
    QString state_str = on_calibrat_? "标定中" : "结束";
    auto text = QString("当前标定：%1 | %2 | %3 | %4 | %5").arg(side).arg(v_head).arg(calib_item->text()).arg(control_item->text()).arg(state_str);
    info_label_->setText(text);
}

void CalibrationPage::OnSaveCalibValueBtnCLicked() {
    if(on_calibrat_ && draw_curve_running_.load()) {
        StopDrawThread();
    }
    for (int r = 0; r < displace_table_->rowCount(); ++r) {
        auto item = displace_table_->item(r, 3);
        if (item) {
            item->setText("开始标定");
            // 取消选中
            item->setSelected(false);
        }
        // 同时清除验证按钮状态
        auto vItem = displace_table_->item(r, 4);
        if (vItem) vItem->setText("验证标定");
        // 取消高亮前三列
        for (int c = 0; c < 3; ++c) {
            auto it = displace_table_->item(r, c);
            if (it) it->setSelected(false);
        }
        SetRowCalib(r, false);
    }
    on_calibrat_ = false;
    calib_state_ = kEnd;
    for(int i = 0; i < displace_table_->rowCount(); i++) {
        SetRowCalib(i, false);
        for(int c = 0; c < 3; c++) {
            QTableWidgetItem* item = displace_table_->item(i, c);
            if(item) item->setSelected(false);
        }
        QTableWidgetItem* yugu_item = displace_table_->item(i, 2);
        auto sub_index = QString("0x%1").arg(GetYuGuValueSubIdx(i),2,16,QLatin1Char('0')).toUpper();
        emit SendRowValue(yugu_item->text(), "0x2003", sub_index);

        QTableWidgetItem* curr_item = displace_table_->item(i, 1);
        sub_index = QString("0x%1").arg(GetCurrValueSubIdx(i),2,16,QLatin1Char('0')).toUpper();
        emit SendRowValue(curr_item->text(), "0x2003", sub_index);        
    }
}

void CalibrationPage::InitCalibState(int row) {
   int not_on_end = 0;
    for (int r = 0; r < displace_table_->rowCount(); ++r) {
        if (r == row) continue;
        QTableWidgetItem *item = displace_table_->item(r, 3);
        if (item && item->text() != "开始标定") {
            not_on_end++;
        }
        // 将其他行的标定按钮文本重置为"开始标定"（并清除选中状态）
        if (item) {
            item->setText("开始标定");
        }
        // 同时取消高亮
        for (int c = 0; c < 3; ++c) {
            QTableWidgetItem *it = displace_table_->item(r, c);
            if (it) it->setSelected(false);
        }
        SetRowCalib(r, false);
    }
    already_on_calib_ = (not_on_end > 0);
}


void CalibrationPage::InitCalibValues(int row) {
    if(row < 0 || row > displace_table_->rowCount() - 1) {
        return;
    }
    for(int i = 0; i < displace_table_->rowCount(); i++) {
        auto control_item = displace_table_->item(i, 2); 
        if(i == row) {
            auto value = control_item->text().toInt();
            range_slider_->setValue(value);
            displace_target_edit_->setText(QString::number(value));
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

    cycle_signal_count_edit_ = new QLineEdit("5");
    cycle_signal_count_edit_->setAlignment(Qt::AlignCenter);
    signal_target_value_edit_ = new QLineEdit("100");
    signal_target_value_edit_->setAlignment(Qt::AlignCenter);
    up_time_edit_ = new QLineEdit("1000");
    up_time_edit_->setAlignment(Qt::AlignCenter);
    down_time_edit_ = new QLineEdit("1000");
    down_time_edit_->setAlignment(Qt::AlignCenter);
    stop_time_edit_ = new QLineEdit("500");
    stop_time_edit_->setAlignment(Qt::AlignCenter);

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
    grid_layout->addWidget(cycle_signal_count_edit_, 1, 0, Qt::AlignCenter);
    grid_layout->addWidget(signal_target_value_edit_, 1, 1, Qt::AlignCenter);
    grid_layout->addWidget(up_time_edit_, 1, 2, Qt::AlignCenter);
    grid_layout->addWidget(down_time_edit_, 1, 3, Qt::AlignCenter);
    grid_layout->addWidget(stop_time_edit_, 1, 4, Qt::AlignCenter);
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

static const QStringList y_axis_labels = {"阀芯实时位移", "位移闭环控制偏差", "最终需求值", "PWM输出占空比", "电磁铁实际电流", "电磁铁目标电流"};
static const QMap<QString, AxisUnit> axis_label_unit_map = {
    {"阀芯实时位移", AxisUnit::kmm},
    {"位移闭环控制偏差", AxisUnit::kmm},
    {"最终需求值", AxisUnit::kmm},
    {"PWM输出占空比", AxisUnit::kp},
    {"电磁铁实际电流", AxisUnit::kmA},
    {"电磁铁目标电流", AxisUnit::kmA}
};

    QString name;
    int side;
    QPen pen;
    bool useRightY;
    bool visible;
    WaveCurveType type;
    QVector<WaveDataPoint> points;
static const std::vector<WaveCurve> wave_curves = {
    // ========== 左侧Y轴 useRightY = false ==========
    //          name            side    pen           right_y visible           type              points
    WaveCurve{"阀1PWM输出占空比", 1, QPen(Qt::blue,2),  false, true,  WaveCurveType::kPWMRatio,      {}}, // 默认可见
    WaveCurve{"阀1目标电流",     1, QPen(Qt::blue,2),  false, false, WaveCurveType::kTargetCurrent, {}},
    WaveCurve{"阀1实际电流",     1, QPen(Qt::blue,2),  false, false, WaveCurveType::kRealCurrent,   {}},
    WaveCurve{"阀1实时位移",     1, QPen(Qt::blue,2),  false, false, WaveCurveType::kRealDisp,      {}},
    WaveCurve{"阀1闭环控制偏差", 1, QPen(Qt::blue,2),  false, false, WaveCurveType::kCloseLoopErr,  {}},
    WaveCurve{"阀1最终需求值",   1, QPen(Qt::blue,2),  false, false, WaveCurveType::kDemandVal,     {}},

    WaveCurve{"阀2PWM输出占空比", 2, QPen(Qt::black,2), false, true,  WaveCurveType::kPWMRatio,      {}}, // 默认可见
    WaveCurve{"阀2目标电流",     2, QPen(Qt::black,2), false, false, WaveCurveType::kTargetCurrent, {}},
    WaveCurve{"阀2实际电流",     2, QPen(Qt::black,2), false, false, WaveCurveType::kRealCurrent,   {}},
    WaveCurve{"阀2实时位移",     2, QPen(Qt::black,2), false, false, WaveCurveType::kRealDisp,      {}},
    WaveCurve{"阀2闭环控制偏差", 2, QPen(Qt::black,2), false, false, WaveCurveType::kCloseLoopErr,  {}},
    WaveCurve{"阀2最终需求值",   2, QPen(Qt::black,2), false, false, WaveCurveType::kDemandVal,     {}},

    // ========== 右侧Y轴 useRightY = true ==========
    WaveCurve{"阀1PWM输出占空比", 1, QPen(Qt::red,2),    true, false, WaveCurveType::kPWMRatio,      {}},
    WaveCurve{"阀1目标电流",     1, QPen(Qt::red,2),    true, false, WaveCurveType::kTargetCurrent, {}},
    WaveCurve{"阀1实际电流",     1, QPen(Qt::red,2),    true, false, WaveCurveType::kRealCurrent,   {}},
    WaveCurve{"阀1实时位移",     1, QPen(Qt::red,2),    true, true,  WaveCurveType::kRealDisp,      {}}, // 默认可见
    WaveCurve{"阀1闭环控制偏差", 1, QPen(Qt::red,2),    true, false, WaveCurveType::kCloseLoopErr,  {}},
    WaveCurve{"阀1最终需求值",   1, QPen(Qt::red,2),    true, false, WaveCurveType::kDemandVal,     {}},

    WaveCurve{"阀2PWM输出占空比", 2, QPen(Qt::green,2),  true, false, WaveCurveType::kPWMRatio,      {}},
    WaveCurve{"阀2目标电流",     2, QPen(Qt::green,2),  true, false, WaveCurveType::kTargetCurrent, {}},
    WaveCurve{"阀2实际电流",     2, QPen(Qt::green,2),  true, false, WaveCurveType::kRealCurrent,   {}},
    WaveCurve{"阀2实时位移",     2, QPen(Qt::green,2),  true, true,  WaveCurveType::kRealDisp,      {}}, // 默认可见
    WaveCurve{"阀2闭环控制偏差", 2, QPen(Qt::green,2),  true, false, WaveCurveType::kCloseLoopErr,  {}},
    WaveCurve{"阀2最终需求值",   2, QPen(Qt::green,2),  true, false, WaveCurveType::kDemandVal,     {}}
};


inline AxisUnit GetAxisUnit(const QString &label) {
    if(axis_label_unit_map.contains(label)){
        return axis_label_unit_map[label];
    }
    return AxisUnit::knone;
}

inline WaveCurveType GetWaveCurveType(const QString &label) {
    return static_cast<WaveCurveType>(y_axis_labels.indexOf(label));
}

// Implementation for creating waveform area
QWidget* CalibrationPage::CreateWaveformArea() {
    waveform_group_ = new QGroupBox("特性曲线显示图", this);

    auto main_layout = new QVBoxLayout(waveform_group_);

    auto sub_layout = new QHBoxLayout();
    auto left_agix_label = new QLabel("左轴:");
    auto left_agix_combo = new QComboBox();
    left_agix_combo->addItems(y_axis_labels);
    left_agix_combo->setCurrentIndex(3);
    auto right_agix_label = new QLabel("右轴:");
    auto right_agix_combo = new QComboBox();
    right_agix_combo->addItems(y_axis_labels);
    right_agix_combo->setCurrentIndex(0);
    auto bottom_agix_label = new QLabel("底轴:");
    auto bottom_agix_combo = new QComboBox();
    bottom_agix_combo->addItems({"时间(s)", "时间(ms)"});

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

        waveform_display->resize(800, kCmdTimeOut);
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

    AxisConfig cfg;
    cfg.left_y = "PWM输出占空比";
    cfg.right_y = "阀芯实时位移";
    cfg.bottom_x = "时间";

    cfg.left_y_unit = AxisUnit::kp;
    cfg.right_y_unit = AxisUnit::kmm;
    cfg.bottom_x_unit = AxisUnit::ks;

    m_wavePlot->setupAxis(cfg);
    // m_wavePlot->setAutoY(true);

    for(auto curve : wave_curves) {
        auto index = m_wavePlot->addCurve(curve);
    }

    main_layout->addLayout(sub_layout);
    main_layout->addWidget(m_wavePlot);

    connect(this, &CalibrationPage::SendDrawStayFaInfo, this, CalibrationPage::DrawStay);
    connect(clear_btn, &QPushButton::clicked, [this] (){
        m_wavePlot->clearAll();
        m_time_ = QDateTime::currentDateTime();
        m_time = 0.0;
    });
    connect(left_agix_combo, &QComboBox::currentTextChanged, this, [=](const QString &text){
        m_wavePlot->updateAxis(AxisName::kLeftY, text, GetAxisUnit(text));
        m_wavePlot->showCurveType(false, GetWaveCurveType(text));
    });
    connect(right_agix_combo, &QComboBox::currentTextChanged, this, [=](const QString &text){
        m_wavePlot->updateAxis(AxisName::kRightY, text, GetAxisUnit(text));
        m_wavePlot->showCurveType(true, GetWaveCurveType(text));
    });
    connect(bottom_agix_combo, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, [=](int index){
        AxisUnit uint = index == 0? AxisUnit::ks : AxisUnit::kms;
        m_wavePlot->updateAxis(AxisName::kBottomX, QString("时间"), uint);
    });
    connect(this, &CalibrationPage::SendMarkGapAllCurves, this, [this](){
        m_wavePlot->markGapAllCurves();
    });
    return waveform_group_;
}

void CalibrationPage::StartDrawThread() {
    if(draw_curve_thread_ != nullptr){
        return; //已经启动，防止重复创建
    }
    if (m_wavePlot) {
        m_wavePlot->markGapAllCurves();
    }
    draw_curve_running_.store(true);
    draw_curve_thread_ = QThread::create([this](){
        while(draw_curve_running_.load()) {
            OnDrawStayFa();
            QThread::msleep(kSleepTimeOut);
        }
    });
    draw_curve_thread_->start();
}

void CalibrationPage::StopDrawThread() {
    if(draw_curve_thread_ == nullptr) {
        return;
    }
    // CAN_CLEAR_BUFF
    draw_curve_running_.store(false);//退出循环条件
    draw_curve_thread_->wait();
    delete draw_curve_thread_;
    draw_curve_thread_ = nullptr;

    if (m_wavePlot) {
        m_wavePlot->markGapAllCurves();
    }
}

void CalibrationPage::OnReadTpdo2Position(can_frame frame) {
    int cur_side = static_cast<int>(curr_side_);
    if (curr_side_ == kSideNone) {
        return;
    }
    // B0字节解析：高4位运行模式，低4位阀ID
    uint8_t b0 = frame.data[0];
    int valveId = static_cast<int>(b0 & 0x0F);
    uint8_t runMode = (b0 >> 4) & 0x0F;
    
    if(valveId != cur_side) {
        return;
    }

    Tpdo2PositionInfo info{};
    info.valid = true;
    info.valveId = valveId;
    info.runMode = runMode;

    // B1: 0x6374 窗口监控状态
    info.windowMonitorStatus = frame.data[1];
    const uint8_t statusByte = info.windowMonitorStatus;
    info.bit0_ReachDelay    = (statusByte & (1 << 0)) != 0;
    info.bit1_InWindow      = (statusByte & (1 << 1)) != 0;
    info.bit2_MonitorEnable = (statusByte & (1 << 2)) != 0;
    info.bit3_WindowErr     = (statusByte & (1 << 3)) != 0;

    // B2‑B3 0x6301 阀芯位置反馈
    info.rawPosition = ExtractFromDataList(frame.data, 2, 3);
    info.posMm = static_cast<double>(info.rawPosition) * 170.0 / 1000.0;

    // B4‑B5 0x6350 闭环控制偏差
    info.ctrlDeviation = ExtractFromDataList(frame.data, 4, 5);
    // 【修正】原来用 rawPosition 算，明显是笔误
    info.deviationMm = static_cast<double>(info.rawPosition) * 170.0 / 1000.0;

    // B6‑B7 0x6310 最终需求值
    info.demandValue = ExtractFromDataList(frame.data, 6, 7);
    // 【修正】同上
    info.demandValueMm = static_cast<double>(info.rawPosition) * 170.0 / 1000.0;

    // 一次性写共享结构体
    {
        std::lock_guard<std::mutex> lk(tpdo_mtx_);
        tpdo_2_info_ = info;
    }   

    // std::cout << "\n==== TPDO2(0x2C0) Parse Result ====" << std::endl;
    // std::cout << "valveId      :" << tpdo_2_info_.valveId << std::endl;
    // std::cout << "runMode      :" << (int)tpdo_2_info_.runMode << std::endl;
    // std::cout << "winMonitorSt :" << (int)tpdo_2_info_.windowMonitorStatus << std::endl;
    // std::cout << "rawPosition  :" << tpdo_2_info_.rawPosition << std::endl;
    // std::cout << "pos(mm)      :" << tpdo_2_info_.posMm << std::endl;
    // std::cout << "ctrlDeviation:" << tpdo_2_info_.ctrlDeviation << std::endl;
    // std::cout << "demandValue  :" << tpdo_2_info_.demandValue << std::endl;
    // std::cout << "Bit0到位延时:" << tpdo_2_info_.bit0_ReachDelay << " Bit1窗口内:" << tpdo_2_info_.bit1_InWindow << std::endl;
    // std::cout << "Bit2监控开启:" << tpdo_2_info_.bit2_MonitorEnable << " Bit3窗口异常:" << tpdo_2_info_.bit3_WindowErr << std::endl;
    // std::cout << "====================================\n" << std::endl;
}

// 位移曲线绘制
void CalibrationPage::OnDrawStayFa() {
    DrawCurveInfo info;
    info.side = static_cast<int>(curr_side_);

    // 先在锁里把数据拷出来，避免长时间持锁
    {
        std::lock_guard<std::mutex> lk(tpdo_mtx_);
#ifdef ON_TEST_MODE
        info.pos_mm = fmod(m_time*10,10);
        info.deviation = fmod(m_time*10,20);
        info.demand_value = fmod(m_time*10,30);
        info.pwm_ratio = fmod(m_time*10,40);
        info.real_curr = fmod(m_time*10,50);
        info.target_curr = fmod(m_time*10,60);
#else
        if (tpdo_2_info_.valid) {
            info.pos_mm       = tpdo_2_info_.posMm;
            info.deviation    = tpdo_2_info_.deviationMm;
            info.demand_value = tpdo_2_info_.demandValueMm;
        }
        if(tpdo_3_info_.valid) {
            info.pwm_ratio    = tpdo_3_info_.GetPwmAbs();
            info.real_curr    = tpdo_3_info_.GetActualCurrentAbsMa();
            info.target_curr  = tpdo_3_info_.GetTargetCurrentAbsMa();            
        }
#endif
    }

    // 时间累计
    {
        std::lock_guard<std::mutex> lk(m_time_mtx_);
        const auto now = QDateTime::currentDateTime();
        const int diss = m_time_.msecsTo(now);
        m_time += static_cast<double>(diss) / 1000.0;
        info.time = m_time;
        m_time_ = now;
    }

    emit SendDrawStayFaInfo(info);
    emit SendCalibCurrentValue(static_cast<int>(info.real_curr));
}

void CalibrationPage::DrawStay(const DrawCurveInfo &info) {
    int control_side = info.side;
    if(control_side != 1 && control_side != 2) {
        return;
    }
    m_wavePlot->appendData(control_side, WaveCurveType::kRealDisp, info.time, info.pos_mm);
    m_wavePlot->appendData(control_side, WaveCurveType::kCloseLoopErr, info.time, info.deviation);
    m_wavePlot->appendData(control_side, WaveCurveType::kDemandVal, info.time, info.demand_value);
    m_wavePlot->appendData(control_side, WaveCurveType::kPWMRatio, info.time, info.pwm_ratio);
    m_wavePlot->appendData(control_side, WaveCurveType::kRealCurrent, info.time, info.real_curr);
    m_wavePlot->appendData(control_side, WaveCurveType::kTargetCurrent, info.time, info.target_curr);

    if(on_calibrat_) {
        actual_value_edit_->setText(QString::number(info.pos_mm));
    }
}

// 电流
void CalibrationPage::OnReadTpdo3Current(can_frame frame) {
    const int cur_side = static_cast<int>(curr_side_);
    if (curr_side_ == kSideNone) {
        return;
    }

    const int16_t pwmOut = static_cast<int16_t>(ExtractFromDataList(frame.data, 0, 1));
    const int frameValveId = (pwmOut >= 0) ? 1 : 2;

    if (frameValveId != cur_side) {
        return;                       // 静默丢弃
    }

    Tpdo3CurrentInfo info{};
    info.valid = true;
    info.pwmOutput = pwmOut;
    info.actualCurrentMa = static_cast<int16_t>(ExtractFromDataList(frame.data, 2, 3));
    info.targetCurrentMa = static_cast<int16_t>(ExtractFromDataList(frame.data, 4, 5));
    info.reserved        = static_cast<uint16_t>(ExtractFromDataList(frame.data, 6, 7));

    {
        std::lock_guard<std::mutex> lk(tpdo_mtx_);
        tpdo_3_info_ = info;
    }

    // 调试打印，对齐文档示例输出
    // std::cout << "\n==== TPDO3(0x3C0) Parse Result ====" << std::endl;
    // std::cout << "valveId        :" << tpdo_3_info_.GetValveId() << std::endl;
    // std::cout << "PWM Output(raw):" << tpdo_3_info_.pwmOutput << "  | 物理PWM(abs):" << tpdo_3_info_.GetPwmAbs() << std::endl;
    // std::cout << "ActualCurrent(raw mA):" << tpdo_3_info_.actualCurrentMa << " | 物理电流:" << tpdo_3_info_.GetActualCurrentAbsMa() << "mA" << std::endl;
    // std::cout << "TargetCurrent(raw mA):" << tpdo_3_info_.targetCurrentMa << " | 物理电流:" << tpdo_3_info_.GetTargetCurrentAbsMa() << "mA" << std::endl;
    // std::cout << "Reserved       :0x" << std::hex << tpdo_3_info_.reserved << std::dec << std::endl;
    // std::cout << "====================================\n" << std::endl;
}