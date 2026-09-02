#ifndef _SETTING_PAGE_H_
#define _SETTING_PAGE_H_

#include "QtWidgets.h"
#include "BasicInfoBar.h"
#include <map>

class BasicInfoBar;
class SettingInfoTable;
class FunctionBtnArea;
class ProgressDialog;

struct BasicInfo;
struct can_frame;

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

public slots:
    void OnValueChanged(QTableWidgetItem *item);

private:
    void InitPage();

private:
    QVBoxLayout* main_layout_ = nullptr;
    BasicInfoBar* basic_info_bar_ = nullptr;
    SettingInfoTable* setting_info_table_ = nullptr;
    FunctionBtnArea* function_btn_area_ = nullptr;

    QTableWidgetItem *last_item_ = nullptr;

    QString old_item_value_ = "";
};

class SettingInfoTable : public QTableWidget {
    Q_OBJECT

public:
    SettingInfoTable(QWidget* parent = nullptr);
    ~SettingInfoTable(){}

    std::vector<uint8_t> GetRowCMD(int row);
    ParamType GetRowParamType(int row);

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
    void OnSetRowValue(QString value, QString idx, QString sub_idx = "");

private:
    void InsterRow(ParaItem item);
    std::vector<uint8_t> CreateRowCmd(ParaItem item);
    void ReloadDefaultValue();

    void UpdateParams();

private:
    std::map<int, QString> default_values_;
    std::map<int, QString> user_values_;
    std::map<int, ParamType> p_item_type_;

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
    bool StartReadFromEPROM();
    void StopReadFromEPROM();
    void ParseEPROMFrame(const can_frame &frame);
    bool TestEPROMSenCmd(can_frame &frame); // for test mode
    void ClearStringFragmentCache();

signals:
    void SendLoadSettings();
    void SendSaveSettings();
    void SendInputMode(InputMode mode);
    void SendClearModifyValue();
    void SendSaveDefaultValue();
    void SendSaveToEPROM();
    void SendReadValue(QString value, QString idx, QString sub_idx);
    // void SendConfirmValues();
    void updateProgress(int percent);
    void SendReadFinished();

private slots:
    void OnLoadSettingBtnClicked();
    void OnSaveSettingBtnClicked();
    void OnModeChangeBtnClicked();
    // void OnSaveDefaultBtnClicked();
    // void OnSaveEepromBtnClicked();
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

    ProgressDialog* progress_dialog_ = nullptr;

    InputMode input_mode_ = kHand;

    std::atomic<bool> parser_running_{false};
    QThread *parser_thread_ = nullptr;
    std::mutex m_mtx_;
    int read_index_ = 0;
    int parse_count_ = 0;

    std::map<QString,QString> str_fragment_cache_;
};


#endif // _SETTING_PAGE_H_