#ifndef _SETTING_PAGE_H_
#define _SETTING_PAGE_H_

#include "QtWidgets.h"
#include "BasicInfoBar.h"
#include <map>

class BasicInfoBar;
class SettingInfoTable;
class FunctionBtnArea;

struct BasicInfo;

enum InputMode {kHand, kAuto};

enum ParamType {kError, kUINT8, kUINT16, kUINT32, kSTRING};
enum ReadWriteType {kNone, kRead, kWirte, kReadOnly, kWriteOnly, kReadWrite};

class SettingPage : public QWidget {
    Q_OBJECT    

public:
    SettingPage(QWidget* parent = nullptr);
    ~SettingPage(){}
    void InitBasicInfo(BasicInfo info);

signals:
    void SendInputMode(InputMode mode);
    void SendInfoChanged(InfoType type, QString value);
    void SendRowValue(QString value, QString idx, QString sub_idx);

private slots:
    void OnValueChanged(QTableWidgetItem *item);

private:
    void InitPage();

private:
    QVBoxLayout* main_layout_ = nullptr;
    BasicInfoBar* basic_info_bar_ = nullptr;
    SettingInfoTable* setting_info_table_ = nullptr;
    FunctionBtnArea* function_btn_area_ = nullptr;

    QTableWidgetItem *last_item_ = nullptr;
};

class SettingInfoTable : public QTableWidget {
    Q_OBJECT

public:
    SettingInfoTable(QWidget* parent = nullptr);
    ~SettingInfoTable(){}

    std::vector<uint8_t> GetRowCMD(int row);

protected:
    void resizeEvent(QResizeEvent* event) override;

public slots:
    void OnLoadSettings();
    void OnSaveSettings();
    void OnChangeInputMode(InputMode mode);
    void OnClearModifyValues();
    void OnConfirmAllValues();
    void OnSaveDefaultValue();
    void OnSaveToEPROM();
    void OnReadFromEPROM();
    void OnSetRowValue(QString value, QString idx, QString sub_idx = "");

private:
    void InsterRow(ParaItem item);
    std::vector<uint8_t> CreateRowCmd(ParaItem item);
    void ReloadDefaultValue();

    void UpdateParams();

private:
    std::map<int, QString> default_values_;
    std::map<int, QString> user_values_;
    // std::vector<RowItemIndex> row_items_;

    std::map<int, std::vector<uint8_t>> row_cmd_map_;
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
    void SendLoadSettings();
    void SendSaveSettings();
    void SendInputMode(InputMode mode);
    void SendClearModifyValue();
    void SendSaveDefaultValue();
    void SendSaveToEPROM();
    void SendReadFromEPROM();
    // void SendConfirmValues();

private slots:
    void OnLoadSettingBtnClicked();
    void OnSaveSettingBtnClicked();
    void OnModeChangeBtnClicked();
    // void OnSaveDefaultBtnClicked();
    // void OnSaveEepromBtnClicked();
    // void OnLoadToTableBtnClicked();

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