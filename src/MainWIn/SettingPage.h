#ifndef _SETTING_PAGE_H_
#define _SETTING_PAGE_H_

#include "QtWidgets.h"

class BasicInfoBar;
class SettingInfoTable;
class FunctionBtnArea;

struct BasicInfo;

enum InputMode {kHand, kAuto};

class SettingPage : public QWidget {
    Q_OBJECT    

public:
    SettingPage(QWidget* parent = nullptr);
    ~SettingPage(){}
    void InitBasicInfo(BasicInfo info);

signals:
    void SendInputMode(InputMode mode);

private:
    void InitPage();

private:
    QVBoxLayout* main_layout_ = nullptr;
    BasicInfoBar* basic_info_bar_ = nullptr;
    SettingInfoTable* setting_info_table_ = nullptr;
    FunctionBtnArea* function_btn_area_ = nullptr;
};

class SettingInfoTable : public QTableWidget {
    Q_OBJECT

public:
    SettingInfoTable(QWidget* parent = nullptr);
    ~SettingInfoTable(){}

protected:
    void resizeEvent(QResizeEvent* event) override;

public slots:
    void OnChangeInputMode(InputMode mode);
    void OnClearModifyValues();
    void OnConfirmAllValues();

private slots:
    void OnEnterBtnClicked(int row);
    void OnModifyValueChanged(int col, int row);

private:
    void InitTable();
};

class FunctionBtnArea : public QGroupBox {
    Q_OBJECT

public:
    FunctionBtnArea(QWidget* parent = nullptr);
    ~FunctionBtnArea(){}

private:
    void InitButtons();
    void ConnectSignles();

signals:
    void SendInputMode(InputMode mode);
    void SendClearModifyValue();
    void SendConfirmValues();

private slots:
    void OnLoadSettingBtnClicked();
    void OnSaveSettingBtnClicked();
    void OnModeChangeBtnClicked();
    void OnSaveDefaultBtnClicked();
    void OnSaveEepromBtnClicked();
    void OnLoadToTableBtnClicked();

private:
    QPushButton* load_setting_btn_ = nullptr;   //读取配置参数文件
    QPushButton* save_setting_btn_ = nullptr;   //保存配置参数文件
    QPushButton* mode_change_btn_ = nullptr;    //手输/自动切换
    QPushButton* save_default_btn_ = nullptr;    //保存默认参数
    QPushButton* save_eeprom_btn_ = nullptr;    //保存到EEPROM
    QPushButton* load_to_table_btn_ = nullptr;    //读取参数到表格
    QPushButton* clear_setting_btn_ = nullptr;    //清空配置参数
    QPushButton* confirm_btn_ = nullptr;    //一键确认

    InputMode input_mode_ = kHand;
};


#endif // _SETTING_PAGE_H_