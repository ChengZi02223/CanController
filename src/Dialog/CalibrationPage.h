#ifndef _CALIBRATION_PAGE_H_
#define _CALIBRATION_PAGE_H_

#include "QtWidgets.h"
#include <map>
#include "QWavePlot.h"
#include <thread>
#include <atomic>
#include <iostream>

enum CalibState {kEnd, kStart, kConfirm};
enum CalibStatus {kOnCalib, kStopCalib, kConfirmCalib};

enum LoopMode { kOpenLoop, kClosedLoop };

struct DrawStayInfo {
    int side; // 1 | 2 侧
    double time; // 时间
    double value; 
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

    bool StartOpenLoop();
    void StopOpenLoop();
    void ExecuteOpenLoopCycle();
    int ReadCurrentStay(int side);

    // PID
    void SetPIDParam();

signals:
    void SendDrawStayFaInfo(const DrawStayInfo &info);
    void SendOpenLoopFinished();

private slots:
    void OnControl1BtnClicked(bool checked);
    void OnControl2BtnClicked(bool checked);
    void OnCycleBtnClicked(bool checked);

    void OnPIDStepBtnClicked(bool checked);
    void OnPIDRampBtnClicked(bool checked);
    void OnPIDMotionBtnClicked();
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
    bool is_on_work_stay_time_ = false;

    QPushButton* control_1_btn_ = nullptr;
    QPushButton* control_2_btn_ = nullptr;
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
    QComboBox* side_combo_ = nullptr;

    QPushButton *step_btn_ = nullptr;
    QPushButton *ramp_btn_ = nullptr;

    QTableWidget* displace_table_ = nullptr;
    QRangeSlider* range_slider_ = nullptr;
    QLabel* info_label_ = nullptr;
    int select_calib_ = 0;

    std::map<QPushButton*, CalibState> states_map_;
    std::vector<QPushButton*> calib_btns_;
    CalibState calib_state_ = kEnd; //标定按钮
    CalibStatus calib_status_ = kStopCalib;   // 标定状态按钮

    QWavePlotWithLegendWidget* m_wavePlot = nullptr;
    int idxSine_1;
    int idxSine_2;
    int idxSaw_1;
    int idxSaw_2;
    std::mutex m_time_mtx_;
    double m_time = 0.0;
    // WavePlotTool* m_waveTool{nullptr};
    // QCPGraph* m_graphSine{nullptr};    // 正弦波 绑定左Y（流量）
    // QCPGraph* m_graphSaw{nullptr};     // 锯齿波 绑定右Y（位移）
};



#endif // _CALIBRATION_PAGE_H_