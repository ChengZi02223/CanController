#ifndef _CALIBRATION_PAGE_H_
#define _CALIBRATION_PAGE_H_

#include "QtWidgets.h"
#include <map>
#include "QWavePlot.h"
#include <thread>
#include <atomic>
#include <iostream>
#include <QDateTime>

enum CalibState {kEnd, kStart, kConfirm};
enum CalibStatus {kOnCalib, kStopCalib, kConfirmCalib};

enum LoopMode { kOpenLoop, kClosedLoop };

struct DrawStayInfo {
    int side; // 1 | 2 侧
    double time; // 时间
    double pos_mm; // 实际位移
    double deviation; // 控制偏差
    double demand_value; // 最终需求值
};

struct DrawCurrInfo {
    int side; // 1 | 2 侧
    double time; // 时间
    double pwm_mm; // PWM输出
    double deviation; // 控制偏差
    double demand_value; // 最终需求值
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
    int16_t GetPwmAbs() const {
        return static_cast<int16_t>(std::abs(static_cast<int>(pwmOutput)));
    }
    int16_t GetActualCurrentAbsMa() const {
        return static_cast<int16_t>(std::abs(static_cast<int>(actualCurrentMa)));
    }
    int16_t GetTargetCurrentAbsMa() const {
        return static_cast<int16_t>(std::abs(static_cast<int>(targetCurrentMa)));
    }
};

Q_DECLARE_METATYPE(DrawStayInfo)
struct BasicInfo;
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
    void InitPageValue();
    QWidget* CreateControlArea();
    QWidget* CreatePIDSettingArea();
    QWidget* CreateDisplacementArea();
    void InitCalibState(QPushButton *calib_btn);
    void InitCalibValues(int row);
    void SetRowCalib(int row, bool calib);
    void UpdateCalibInfo();
    QWidget* CreateSignalResponseArea();

    QWidget* CreateWaveformArea();

    bool IsOnSideControl1();

    void ChangeLoopMode(LoopMode mode);

    // 开环
    void StartControl1Loop();
    void StartControl2Loop();
    void StopControl1Loop();
    void StopControl2Loop();
    void DrawStay(const DrawStayInfo &info);

    bool StartLoopCycle();
    void StopLoopCycle();
    void ExecuteLoopCycle();
    Tpdo2PositionInfo ReadTpdo2Position(int side) const;
    Tpdo3CurrentInfo ReadTpdo3Current(int side) const;

    // PID
    void SetPIDParam();

signals:
    void SendDrawStayFaInfo(const DrawStayInfo &info);
    void SendOpenLoopFinished();

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

    void OnSaveCalibValueBtnCLicked();

    void OnSineWaveBtnClicked();
    void OnSawtoothWaveBtnClicked();

    // 位移曲线绘制
    void OnDrawStayFa1();
    void OnDrawStayFa2();

private:

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
    // 开环循环动作线程
    bool stay_1_running_ = false;
    QThread *stay_thread_1_ = nullptr;
    bool stay_2_running_ = false;
    QThread *stay_thread_2_ = nullptr;

    std::atomic<bool> is_open_running_{false};
    std::thread open_loop_thread_;
    bool is_on_cycle_ = false;

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
    CalibState calib_state_ = kEnd; //标定按钮
    CalibStatus calib_status_ = kStopCalib;   // 标定状态按钮

    //周期信号响应区
    QLineEdit* cycle_signal_count_edit_ = nullptr;
    QLineEdit* signal_target_value_edit_ = nullptr;
    QLineEdit* up_time_edit_ = nullptr;
    QLineEdit* down_time_edit_ = nullptr;
    QLineEdit* stop_time_edit_ = nullptr;

    QWavePlotWithLegendWidget* m_wavePlot = nullptr;
    // 位移相关曲线
    int idxSaw_stay_1;
    int idxSaw_devia_1;
    int idxSaw_value_1;
    int idxSaw_stay_2;
    int idxSaw_devia_2;
    int idxSaw_value_2;

    // 电流相关曲线
    int idxSine_pwm_1;
    int idxSine_curr_1;
    int idxSine_target_1;
    int idxSine_pwm_2;
    int idxSine_curr_2;
    int idxSine_target_2;
    std::mutex m_time_mtx_;
    double m_time = 0.0;
    QDateTime m_time_;
    // WavePlotTool* m_waveTool{nullptr};
    // QCPGraph* m_graphSine{nullptr};    // 正弦波 绑定左Y（流量）
    // QCPGraph* m_graphSaw{nullptr};     // 锯齿波 绑定右Y（位移）
};



#endif // _CALIBRATION_PAGE_H_