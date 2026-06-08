#ifndef _CALIBRATION_PAGE_H_
#define _CALIBRATION_PAGE_H_

#include "QtWidgets.h"
#include <map>

enum CalibState {kEnd, kStart, kConfirm};
enum CalibStatus {kOnCalib, kSaveCalib, kConfirmCalib};

class BasicInfoBar;
class CalibrationPage : public QWidget {
    Q_OBJECT    

public:
    CalibrationPage(QWidget* parent = nullptr);
    ~CalibrationPage(){}

private:
    void InitPage();
    QWidget* CreateControlArea();
    QWidget* CreatePIDSettingArea();
    QWidget* CreateDisplacementArea();
    QWidget* CreateSignalResponseArea();

    QWidget* CreateWaveformArea();

private slots:
    void OnControl1BtnClicked();
    void OnControl2BtnClicked();
    void OnCycleBtnClicked();

    void OnPIDStepBtnClicked();
    void OnPIDRampBtnClicked();
    void OnPIDMotionBtnClicked();
    void OnPIDSaveBtnClicked();

    void OnSineWaveBtnClicked();
    void OnSawtoothWaveBtnClicked();

private:
    QHBoxLayout* main_layout_ = nullptr;
    BasicInfoBar* basic_info_bar_ = nullptr;
    std::map<QPushButton*, CalibState> states_map_;
    CalibStatus calib_state_ = kOnCalib;
};



#endif // _CALIBRATION_PAGE_H_