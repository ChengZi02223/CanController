#ifndef _CALIBRATION_PAGE_H_
#define _CALIBRATION_PAGE_H_

#include "QtWidgets.h"
#include <map>
#include "QWavePlot.h"

enum CalibState {kEnd, kStart, kConfirm};
enum CalibStatus {kOnCalib, kStopCalib, kConfirmCalib};

struct BasicInfo;
class BasicInfoBar;
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

private slots:
    void OnControl1BtnClicked(bool checked);
    void OnControl2BtnClicked(bool checked);
    void OnCycleBtnClicked(bool checked);

    void OnPIDStepBtnClicked();
    void OnPIDRampBtnClicked();
    void OnPIDMotionBtnClicked();
    void OnPIDSaveBtnClicked();

    void OnSaveCalibValueBtnCLicked();

    void OnSineWaveBtnClicked();
    void OnSawtoothWaveBtnClicked();

private:
    QHBoxLayout* main_layout_ = nullptr;
    BasicInfoBar* basic_info_bar_ = nullptr;
    QGroupBox* control_group_ = nullptr;
    QLineEdit* output_cycle_1_edit_ = nullptr;
    QLineEdit* output_cycle_2_edit_ = nullptr;
    QLineEdit* cycle_count_edit_ = nullptr;
    QLineEdit* neutral_time_edit_ = nullptr;
    QLineEdit* work_time_edit_ = nullptr;

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

    QTableWidget* displace_table_ = nullptr;
    QRangeSlider* range_slider_ = nullptr;
    QLabel* info_label_ = nullptr;
    int select_calib_ = 0;

    std::map<QPushButton*, CalibState> states_map_;
    std::vector<QPushButton*> calib_btns_;
    CalibState calib_state_ = kEnd; //标定按钮
    CalibStatus calib_status_ = kStopCalib;   // 标定状态按钮

    QTimer data_timer_;
    QWavePlotWidget* m_wavePlot = nullptr;
    int idxSine;
    int idxSaw;
    double m_time = 0.0;
    // WavePlotTool* m_waveTool{nullptr};
    // QCPGraph* m_graphSine{nullptr};    // 正弦波 绑定左Y（流量）
    // QCPGraph* m_graphSaw{nullptr};     // 锯齿波 绑定右Y（位移）
};



#endif // _CALIBRATION_PAGE_H_