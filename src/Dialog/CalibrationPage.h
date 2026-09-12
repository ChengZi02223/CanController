#ifndef _CALIBRATION_PAGE_H_
#define _CALIBRATION_PAGE_H_

#include "QtWidgets.h"
#include <map>
#include "QWavePlot.h"
#include <thread>
#include <atomic>
#include <iostream>
#include <QDateTime>
#include "BasicInfoBar.h"
#include "CanDriver.h"
#include <QTimer>

enum CalibState {kEnd, kStart, kConfirm};
enum CalibStatus {kOnCalib, kStopCalib, kConfirmCalib};

enum LoopMode { kOpenLoop, kClosedLoop };

enum LoopSide { kSideNone, kSideOne, kSideTwo};

struct ControlContext {
    LoopSide side = kSideNone;
    int value = 0;                             // 已解析好的目标值
    std::array<uint8_t, 8> target_cmd{};       // SDO_PWM_OPEN_1 / SDO_CUR_OPEN_1 / SDO_SEND_TARGET_1 ...
    std::vector<uint8_t> close_cmd;
    int factor = 1;                            // 10=开环位移，1=电流/闭环
    LoopMode loop_mode = kOpenLoop;            // kOpenLoop / kClosedLoop
    bool need_set_pid = false;                 // 闭环时下发 PID
    bool need_ramp = false;                    // 斜坡响应才需要
};

struct DrawCurveInfo {
    int side; // 1 | 2 侧
    double time = 0.0; // 时间
    double pos_mm = 0.0; // 实际位移
    double deviation = 0.0; // 控制偏差
    double demand_value = 0.0; // 最终需求值

    double pwm_ratio = 0.0; // PWM输出值占空比
    double real_curr = 0.0; // 实际电流
    double target_curr = 0.0; // 目标电流
};

struct Tpdo2PositionInfo {
    bool valid{false};              // 是否解析成功
    int valveId{0};                // 1=阀1，2=阀2
    uint8_t runMode{0};            // B0高4位，0x6042运行模式
    uint8_t windowMonitorStatus{0};// B1 0x6374窗口监控状态
    int16_t rawPosition{0};        // 0x6301阀芯原始位置反馈 0~1000
    int16_t ctrlDeviation{0};      // 0x6350闭环控制偏差
    int16_t demandValue{0};       // 0x6310最终需求值
    double posMm{0.0};             // 换算后物理位移 mm
    double deviationMm{0.0};             // 闭环控制偏差 mm
    double demandValueMm{0.0};             // 最终需求值 mm
    // B1状态位拆解
    bool bit0_ReachDelay{false};   // Bit0:到位且满足延时
    bool bit1_InWindow{false};     // Bit1:瞬时位置到达窗口内
    bool bit2_MonitorEnable{false};// Bit2:监控已开启
    bool bit3_WindowErr{false};    // Bit3:窗口监控异常
};

struct Tpdo3CurrentInfo {
    bool valid{false};               // 解析是否有效
    int16_t pwmOutput{0};           // B0‑1 0x2014 PWM输出，范围±10000
    int16_t actualCurrentMa{0};     // B2‑3 0x2011 实际电流 mA，±12600
    int16_t targetCurrentMa{0};     // B4‑5 0x2012 目标电流 mA，±12600
    uint16_t reserved{0};           // B6‑7 保留字段

    // 从PWM输出符号判断是哪个阀：1阀1，2阀2
    int GetValveId() const {
        if(pwmOutput >= 0)
            return 1;
        else
            return 2;
    }
    double GetPwmAbs() const {
        return static_cast<double>(std::abs(static_cast<int>(pwmOutput)));
    }
    double GetActualCurrentAbsMa() const {
        return static_cast<double>(std::abs(static_cast<int>(actualCurrentMa)));
    }
    double GetTargetCurrentAbsMa() const {
        return static_cast<double>(std::abs(static_cast<int>(targetCurrentMa)));
    }
};

Q_DECLARE_METATYPE(DrawCurveInfo)
struct BasicInfo;
struct can_frame;
class BasicInfoBar;
class QWavePlotWithLegendWidget;
class CalibrationPage : public QWidget {
    Q_OBJECT    

public:
    CalibrationPage(QWidget* parent = nullptr);
    ~CalibrationPage(){}
    void InitBasicInfo(BasicInfo info);

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    void InitPage();
    void InitPageTimer();
    void OpenTimer(LoopSide side);
    void CloseTimer(LoopSide side);

    QWidget* CreateControlArea();
    QWidget* CreatePIDSettingArea();
    QWidget* CreateDisplacementArea();
    void InitCalibState(int row);
    void InitCalibValues(int row);
    void SetRowCalib(int row, bool calib);
    void UpdateCalibInfo();
    QWidget* CreateSignalResponseArea();

    QWidget* CreateWaveformArea();

    bool IsOnSideControl1();

    void ChangeLoopMode(LoopMode mode);

    void HandleControlEvent(bool checked, const ControlContext& ctx);

    void StartDrawThread();
    void StopDrawThread();
    void DrawStay(const DrawCurveInfo &info);

    bool StartLoopCycle();
    void StopLoopCycle();
    void ExecuteLoopCycle();
    // 发送一帧开环指令：更新对应侧的 cur_fa_val_X_cmd_ 并推入 CAN
    void PushOpenLoopCmd(LoopSide side,
                         const std::array<uint8_t, 8>& target_cmd,
                         int percent, int factor);
    // 边等待、边重发：total_ms 内按 resend_interval_ms_ 周期重发；
    // 返回 false 表示被中断（stop_requested_ 或 is_open_running_ 变化）
    bool WaitAndResend(int total_ms, LoopSide side,
                       const std::array<uint8_t, 8>& target_cmd,
                       int percent, int factor);

    // 单侧完整时序：中位 → 工作位 → 中位；返回 false 表示被中断
    bool RunOpenLoopSide(LoopSide side, int percent,
                         const std::array<uint8_t, 8>& target_cmd,
                         int neutral_ms, int work_ms, int factor);
    // PID
    void SetPIDParam();

signals:
    void SendDrawStayFaInfo(const DrawCurveInfo &info);
    void SendOpenLoopFinished();
    void SendInfoChanged(InfoType type, QString value);
    void SendRowValue(QString value, QString idx, QString sub_idx);
    void SendCalibCurrentValue(double value);
    void SendStopReadNMTCmd();
     void SendMarkGapAllCurves(); 

private slots:
    void OnControl1BtnClicked(bool checked);
    void OnControl2BtnClicked(bool checked);
    void OnControlCur1BtnClicked(bool checked);
    void OnControlCur2BtnClicked(bool checked);
    void OnCycleBtnClicked(bool checked);

    void OnPIDSideBtnClicked();
    void OnPIDStepBtnClicked(bool checked);
    void OnPIDRampBtnClicked(bool checked);
    void OnPIDMotionBtnClicked(bool checked);
    void OnPIDSaveBtnClicked();

    void OnCalibButtonClicked(int row, int column);
    void OnSaveCalibValueBtnCLicked();

    void OnSineWaveBtnClicked();
    void OnSawtoothWaveBtnClicked();

    // 位移曲线绘制
    void OnTimePushCmd();
    void OnReadTpdo2Position(can_frame frame);
    void OnReadTpdo3Current(can_frame frame);

    void OnDrawStayFa();

private:
    QTimer *stay_timer_1_ = nullptr;
    QTimer *stay_timer_2_ = nullptr;
    mutable std::mutex tpdo_mtx_;
    Tpdo2PositionInfo tpdo_2_info_{};
    Tpdo3CurrentInfo tpdo_3_info_{};

    std::atomic<bool> draw_curve_running_{false};
    QThread *draw_curve_thread_ = nullptr;

    std::vector<uint8_t> cur_fa_val_1_cmd_; //当前开阀1 cmd
    std::vector<uint8_t> cur_fa_val_2_cmd_;    

    QHBoxLayout* main_layout_ = nullptr;
    BasicInfoBar* basic_info_bar_ = nullptr;
    QGroupBox* control_group_ = nullptr;
    QLineEdit* output_cycle_1_edit_ = nullptr;
    QLineEdit* output_cycle_2_edit_ = nullptr;
    QLineEdit* cycle_count_edit_ = nullptr;
    QLineEdit* neutral_time_edit_ = nullptr;
    QLineEdit* work_time_edit_ = nullptr;

    LoopMode cur_loop_mode_ = kOpenLoop; // 当前模式： 开环 | 闭环

    std::atomic<bool> is_open_running_{false};
    QThread *open_loop_thread_ = nullptr;
    bool is_on_cycle_ = false;
    LoopSide curr_side_ = kSideNone;

    QPushButton* control_1_btn_ = nullptr;
    QPushButton* control_2_btn_ = nullptr;
    QPushButton* control_cur_1_btn_ = nullptr;
    QPushButton* control_cur_2_btn_ = nullptr;
    QPushButton* cycle_btn_ = nullptr;

    QGroupBox* pid_group_ = nullptr;
    QLineEdit* p_edit_ = nullptr;
    QLineEdit* i_edit_ = nullptr;
    QLineEdit* d_edit_ = nullptr;
    QLineEdit* target_edit_ = nullptr;
    QLineEdit* ramp_edit_ = nullptr;
    QGroupBox* displacement_group_ = nullptr;
    QGroupBox* signal_group_ = nullptr;
    QGroupBox* waveform_group_ = nullptr;
    // QComboBox* side_combo_ = nullptr;
    bool on_side_1_ = true;
    QPushButton *side_btn_ = nullptr;
    QPushButton *step_btn_ = nullptr;
    QPushButton *ramp_btn_ = nullptr;
    QPushButton *motion_btn_ = nullptr;
    QPushButton *save_pid_btn_ = nullptr;

    QLineEdit* target_flow_edit_ = nullptr;

    QTableWidget* displace_table_ = nullptr;
    QRangeSlider* range_slider_ = nullptr;
    QLabel* info_label_ = nullptr;
    int select_calib_ = 0;

    std::map<QPushButton*, CalibState> states_map_;
    std::vector<QPushButton*> calib_btns_;
    std::vector<QPushButton*> verify_btns_;
    CalibState calib_state_ = kEnd; //标定按钮
    CalibStatus calib_status_ = kStopCalib;   // 标定状态按钮
    bool already_on_calib_ = false;

    QLineEdit *displace_target_edit_ = nullptr;
    QLineEdit *actual_value_edit_ = nullptr;
    bool on_calibrat_ = false;

    //周期信号响应区
    QLineEdit* cycle_signal_count_edit_ = nullptr;
    QLineEdit* signal_target_value_edit_ = nullptr;
    QLineEdit* up_time_edit_ = nullptr;
    QLineEdit* down_time_edit_ = nullptr;
    QLineEdit* stop_time_edit_ = nullptr;

    QWavePlotWithLegendWidget* m_wavePlot = nullptr;
    // 左侧
    std::condition_variable cv_;
    std::mutex cv_mtx_;
    std::atomic<bool> stop_requested_{false};
    std::atomic<bool> is_stopping_{false};
    std::mutex m_time_mtx_;
    double m_time = 0.0;
    QDateTime m_time_;
    // WavePlotTool* m_waveTool{nullptr};
    // QCPGraph* m_graphSine{nullptr};    // 正弦波 绑定左Y（流量）
    // QCPGraph* m_graphSaw{nullptr};     // 锯齿波 绑定右Y（位移）
};



#endif // _CALIBRATION_PAGE_H_