#include "SettingPage.h"

#include "QFileOperator.h"
#include "CanDriver.h"
#include "can_cmd.h"
#include "Utils.h"
#include "CanManager.h"

#define INFO_TABLE_PARAM_COLUMN 0
#define INFO_TABLE_OBJ_COLUMN 1
#define INFO_TABLE_IDX_COLUMN 2
#define INFO_TABLE_SAVE_COLUMN 3
#define INFO_TABLE_MODIFY_COLUMN 4

#define BUTTON_WIDTH 150
#define BUTTON_HEIGHT 40

#define ROW_COUNT 12
#define COLUMN_COUNT 7
#define ROW_HEIGHT 42

#define TEST_CONFIG_FILE "D:/Desktop/yc/PartTimeJobs/Windows/CanController_Docs/CANopen对象字典功能说明V1.xlsx"

// enum ParamType {kNone, kUINT8, kUINT16, kUINT32, kSTRING};
inline ParamType GetParamType(const QString &text) {
    QString s = text.trimmed();
    if (s.isEmpty()) {
        return ParamType::kError;
    }

    if(s.contains("32")) {
        return  ParamType::kUINT32;
    } else if(s.contains("16")) {
        return  ParamType::kUINT16;
    } else if(s.contains("8")) {
        return  ParamType::kUINT8;
    } else if(s.toLower().contains("string")) {
        return  ParamType::kSTRING;
    }
    return ParamType::kError;
}

inline uint8_t GetCmdHeadByType(QString text) {
    auto type = GetParamType(text);
    if(type == ParamType::kUINT8) {
        return 0x2F;
    } else if(type == ParamType::kUINT16) {
        return 0x2B;
    } else if(type == ParamType::kUINT32) {
        return 0x23;
    } else if(type == ParamType::kSTRING) {
        return 0x00; // need define
    }
    return 0x00;
}

inline ReadWriteType GetReadWriteType(const QString &text) {
    QString s = text.trimmed();
    // 空、"-" 返回kNone
    if (s.isEmpty() || s == "-")
    {
        return ReadWriteType::kNone;
    }

    QString lower = s.toLower();

    // ========== 只读场景：r、ro、中文"只读" ==========
    if (lower == "r" || lower == "ro" || s.contains("只读"))
    {
        return ReadWriteType::kReadOnly;
    }
    // ========== 只写场景：w、wo、中文"只写" ==========
    if (lower == "w" || lower == "wo" || s.contains("只写"))
    {
        return ReadWriteType::kWriteOnly;
    }
    // ========== 读写场景：rw ==========
    if (lower == "rw")
    {
        return ReadWriteType::kReadWrite;
    }

    // 单独读、单独写（如果业务需要区分kRead / kWrite，例如纯"r"代表仅读，纯"w"仅写）
    // 上面已经把 r/ro 归为kReadOnly，w/wo归为kWriteOnly；
    // 如果希望区分 kRead(仅读) 和 kReadOnly(只读禁止修改)，可以调整逻辑。

    // 未知文本返回kNone
    return ReadWriteType::kNone;
    
}

static const QStringList info_table_h_head({"参数名称", "对象字典", "索引", "保存值", "修改值", "下限", "上限"});
// static const QStringList info_table_v_head({"最大电流", "额定电压", "响应时间", "增益系数", "滤波深度", "死区时间", "保护阈值", "校准偏移", "过流保护", "温度补偿", "采样周期", "通讯超时"});

SettingPage::SettingPage(QWidget* parent)
    : QWidget(parent) {
    InitPage();
}

void SettingPage::InitPage() {
    main_layout_ = new QVBoxLayout(this);

    basic_info_bar_ = new BasicInfoBar(kHar, this);

    auto sub_layout = new QHBoxLayout();
    setting_info_table_ = new SettingInfoTable(this);
    function_btn_area_ = new FunctionBtnArea(this);
    setting_info_table_->setEnabled(false);
    function_btn_area_->setEnabled(false);

    sub_layout->addWidget(setting_info_table_);
    sub_layout->addWidget(function_btn_area_);

    main_layout_->addWidget(basic_info_bar_);
    main_layout_->addLayout(sub_layout);


    
    connect(function_btn_area_, &FunctionBtnArea::SendLoadSettings, setting_info_table_, &SettingInfoTable::OnLoadSettings);
    connect(function_btn_area_, &FunctionBtnArea::SendSaveSettings, setting_info_table_, &SettingInfoTable::OnSaveSettings);

    connect(function_btn_area_, &FunctionBtnArea::SendInputMode, setting_info_table_, &SettingInfoTable::OnChangeInputMode);
    connect(function_btn_area_, &FunctionBtnArea::SendClearModifyValue, setting_info_table_, &SettingInfoTable::OnClearModifyValues);
    // connect(function_btn_area_, &FunctionBtnArea::SendConfirmValues, setting_info_table_, &SettingInfoTable::OnConfirmAllValues);
    connect(function_btn_area_, &FunctionBtnArea::SendSaveDefaultValue, setting_info_table_, &SettingInfoTable::OnSaveDefaultValue);
    connect(function_btn_area_, &FunctionBtnArea::SendSaveToEPROM, setting_info_table_, &SettingInfoTable::OnSaveToEPROM);
    connect(function_btn_area_, &FunctionBtnArea::SendReadFromEPROM, setting_info_table_, &SettingInfoTable::OnReadFromEPROM);
    // connect(function_btn_area_, &FunctionBtnArea::SendInputMode, this, &SettingPage::SendInputMode);

    connect(basic_info_bar_, &BasicInfoBar::SendTestState, [=](TestState state){
        setting_info_table_->setEnabled(state == kTestStart);
        function_btn_area_->setEnabled(state == kTestStart);
    });
    connect(basic_info_bar_, &BasicInfoBar::SendInfoChanged, this, &SettingPage::SendInfoChanged);
    connect(setting_info_table_, &QTableWidget::itemChanged, this, &SettingPage::OnValueChanged);
    connect(this, &SettingPage::SendRowValue, setting_info_table_, &SettingInfoTable::OnSetRowValue);
    connect(CanManager::GetInstance(), &CanManager::SendRowValue, setting_info_table_, &SettingInfoTable::OnSetRowValue);
}

void SettingPage::InitBasicInfo(BasicInfo info) { basic_info_bar_->InitData(info); }

void SettingPage::OnValueChanged(QTableWidgetItem *item) {
    // if(last_item_ != nullptr && last_item_ == item) {
    //     return;
    // } 
    // last_item_ = item;

    // qDebug() << item->text() << ": "<<item->row() << "--" << item->column();

    // auto cmd = setting_info_table_->GetRowCMD(item->row());
    // if(cmd.empty()) {
    //     return;
    // }

    // CanDriver::GetInstance()->SendCmd(SDO_COB_ID, cmd, kCmdTimeOut);
    
}

SettingInfoTable::SettingInfoTable(QWidget* parent) : QTableWidget(parent) {
    setEditTriggers(QAbstractItemView::AllEditTriggers);
    connect(this, &QTableWidget::cellChanged, [this](int row, int col){
        if(col != INFO_TABLE_MODIFY_COLUMN) {
            return;
        }
        auto save_item = item(row, INFO_TABLE_SAVE_COLUMN);
        auto change_item = item(row, INFO_TABLE_MODIFY_COLUMN);
        if(save_item->text() != change_item->text()) {
            change_item->setForeground(QBrush(Qt::red));
        }
    });
    setObjectName("SettingInfoTable");
    setColumnCount(COLUMN_COUNT);
    setHorizontalHeaderLabels(info_table_h_head);
    verticalHeader()->setVisible(false);
}

void SettingInfoTable::InsterRow(ParaItem item) {
    bool ok;
    int serialNum = item.serialNum.toInt(&ok);
    if(!ok || serialNum < 0) return;
    if(item.objDictName.trimmed().isEmpty()) return;  // 跳过空行(序号33)
    int row = rowCount();          // 顺序追加，不用序号算
    insertRow(row);
    setRowHeight(row, ROW_HEIGHT);

    auto param_item = new QTableWidgetItem(item.objDictName);
    param_item->setFlags(param_item->flags() & ~Qt::ItemIsEditable);
    param_item->setTextAlignment(Qt::AlignCenter);
    param_item->setToolTip(item.objDictName);
    setItem(row, INFO_TABLE_PARAM_COLUMN, param_item);

    // 对象字典
    auto obj_item = new QTableWidgetItem(item.indexNum);
    obj_item->setFlags(obj_item->flags() & ~Qt::ItemIsEditable);
    obj_item->setTextAlignment(Qt::AlignCenter);
    setItem(row, INFO_TABLE_OBJ_COLUMN, obj_item);
    // 子索引
    auto idx_item = new QTableWidgetItem(item.subIndex);
    idx_item->setFlags(idx_item->flags() & ~Qt::ItemIsEditable);
    idx_item->setTextAlignment(Qt::AlignCenter);
    setItem(row, INFO_TABLE_IDX_COLUMN, idx_item);
    // 保存值
    auto save_item = new QTableWidgetItem(item.paramValue);
    save_item->setFlags(save_item->flags() & ~Qt::ItemIsEditable);
    save_item->setTextAlignment(Qt::AlignCenter);
    setItem(row, INFO_TABLE_SAVE_COLUMN, save_item);    
    // 参数值
    auto p_item = new QTableWidgetItem(item.paramValue);
    if(GetReadWriteType(item.rwDesc) == ReadWriteType::kReadOnly) {
        p_item->setFlags(p_item->flags() & ~Qt::ItemIsEditable);
    } else {
        p_item->setFlags(p_item->flags() | Qt::ItemIsEditable);
    }
    
    p_item->setTextAlignment(Qt::AlignCenter);
    setItem(row, INFO_TABLE_MODIFY_COLUMN, p_item);

    default_values_[row] = item.paramValue;
    // 确定标定按钮
    // auto enter_btn = new QPushButton("确认", this);
    // enter_btn->setObjectName("EnterBtn");
    // enter_btn->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    // setCellWidget(row, COLUMN_COUNT - 1, enter_btn);
    // connect(enter_btn, &QPushButton::clicked, this, [this, row](){
    //     OnEnterBtnClicked(row);
    // });
    row_cmd_map_[row] = CreateRowCmd(item);
    // row_items_.push_back({row, item.indexNum, item.subIndex, item.paramValue});
}

std::vector<uint8_t> SettingInfoTable::GetRowCMD(int row) {
    return row_cmd_map_[row];
}

void SettingInfoTable::OnSetRowValue(QString value, QString idx, QString sub_idx) {
    if(idx.isEmpty()) {
        return;
    }
    qDebug() << "OnSetRowValue: " << value << " " << idx << " " << sub_idx;
    QString targetIndex = idx.toUpper();
    for(int row = 0; row < rowCount(); row++) {
        QTableWidgetItem* objItem = item(row, INFO_TABLE_OBJ_COLUMN);
        if(!objItem) continue;

        // 表格单元格文本统一大写，比对
        QString tableIndex = objItem->text().toUpper();
        if(tableIndex != targetIndex)
            continue;

        // 对象字典索引匹配成功，校验子索引
        bool subMatch = false;
        if(sub_idx.isEmpty() || sub_idx == "--") {
            subMatch = true;
        } else {
            QTableWidgetItem* subItem = item(row, INFO_TABLE_IDX_COLUMN);
            if(subItem != nullptr) {
                QString tableSub = subItem->text().toUpper();
                QString targetSub = sub_idx.toUpper();
                if(tableSub == targetSub) {
                    subMatch = true;
                }
            }
        }

        if(subMatch) {
            QTableWidgetItem* modItem = item(row, INFO_TABLE_MODIFY_COLUMN);
            if(modItem) {
                modItem->setText(value);
                qDebug()<<"成功更新行"<<row<<" value="<<value;
            }
        }
    }
}

std::vector<uint8_t> SettingInfoTable::CreateRowCmd(ParaItem item) {
    std::array<uint8_t, 8> command = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    command[0] = GetCmdHeadByType(item.dataType);
    FillValueToCmd(QStringToUint16(item.indexNum), command, 1, 2);

    std::vector<uint8_t> vec_cmd(command.begin(), command.end());
    return vec_cmd;
}

void SettingInfoTable::resizeEvent(QResizeEvent* event)  {
    if(columnCount() < 0){
        return; 
    }
    auto col_width = (width() - 5) / columnCount();
    for(int i = 0; i < columnCount(); ++i) {
        setColumnWidth(i, col_width);
    }
}

void SettingInfoTable::OnLoadSettings() {
    QSignalBlocker blocker(this);
    auto file_op = QFileOperator::GetInstance();
    QList<ParaItem> tableDataList = file_op->getTableItems();
    if(tableDataList.empty()){
        return;
    }
    clearContents();
    setRowCount(0);
    // row_items_.clear();
    default_values_.clear();
    for(auto item : tableDataList) {
        InsterRow(item);
    }

    for(int i = 0; i < rowCount(); ++i) {
        setRowHeight(i,ROW_HEIGHT); 
        for(int j = 5; j < COLUMN_COUNT; ++j) {
            auto value_item = new QTableWidgetItem();
            value_item->setTextAlignment(Qt::AlignCenter);
            value_item->setFlags(value_item->flags() & ~Qt::ItemIsEditable);
            setItem(i, j, value_item);
        }
    }
}

void SettingInfoTable::OnSaveSettings() {
    // todo
    // qDebug() << "OnSaveSettings";
    QList<QString> para_list;
    for(int i = 0; i < rowCount(); ++i) {
        auto modify_item = item(i, INFO_TABLE_MODIFY_COLUMN);
        para_list << modify_item->text();
    }
    if(para_list.isEmpty()){
        return;
    }
    auto file_op = QFileOperator::GetInstance();
    bool res = file_op->SaveModifyValueToLastFile(para_list);
    if(res) {
        QMessageBox::information(this, "提示", tr("修改值已经成功保存到配置文件中：%1").arg(file_op->GetLastLoadFile()));
    }
    OnConfirmAllValues();
}

void SettingInfoTable::OnChangeInputMode(InputMode mode) {
    bool editable = (mode == kHand);

    for(int i = 0; i < rowCount(); ++i) {
        auto modify_item = item(i, INFO_TABLE_MODIFY_COLUMN);
        if(editable) {
            modify_item->setFlags(modify_item->flags() | Qt::ItemIsEditable);
        } else {
            modify_item->setFlags(modify_item->flags() &~ Qt::ItemIsEditable);
        }
    }
}

void SettingInfoTable::OnClearModifyValues() {
    for(int i = 0; i < rowCount(); ++i) {
        auto modify_item = item(i, INFO_TABLE_MODIFY_COLUMN);
        modify_item->setText("");
    }
}

void SettingInfoTable::OnConfirmAllValues() {
    for(int i = 0; i < rowCount(); ++i) {
        auto save_item = item(i, INFO_TABLE_SAVE_COLUMN);
        auto change_item = item(i, INFO_TABLE_MODIFY_COLUMN);
        save_item->setText(change_item->text());
        change_item->setForeground(QBrush(Qt::green));
    }
}

void SettingInfoTable::ReloadDefaultValue() {
    for(int i = 0; i < rowCount(); ++i) {
        auto save_item = item(i, INFO_TABLE_SAVE_COLUMN);
        auto change_item = item(i, INFO_TABLE_MODIFY_COLUMN);
        auto default_value = default_values_[i];
        save_item->setText(default_value);
        change_item->setText(default_value);
        change_item->setForeground(QBrush(Qt::black));
    }
}

void SettingInfoTable::OnSaveDefaultValue() {
    UpdateParams();
    qDebug() << "保存默认参数";
    CanDriver::GetInstance()->SendCmd(SDO_COB_ID, SDO_SAVE_DEFAULT_CMD, kCmdTimeOut);
    ReloadDefaultValue();
    update();
}

void SettingInfoTable::OnSaveToEPROM() {
    UpdateParams();
    qDebug() << "保存到EPROM";
    CanDriver::GetInstance()->SendCmd(SDO_COB_ID, SDO_SAVE_USER_SETTING_CMD, kCmdTimeOut);

    for(int i = 0; i < rowCount(); ++i) {
        user_values_[i] = item(i, INFO_TABLE_MODIFY_COLUMN)->text();
    }
}

void SettingInfoTable::OnReadFromEPROM() {
    qDebug() << "读取参数到表格";
    CanManager::GetInstance()->Start();
    bool ret =  CanManager::GetInstance()->SendFrame(SDO_COB_ID, SDO_READ_PARAM_TO_TABLE);
    qDebug()<<"[CanManager]发送0x640批量读指令 "<<(ret?"成功":"失败");
}

void SettingInfoTable::UpdateParams() {

}

FunctionBtnArea::FunctionBtnArea(QWidget* parent) : QGroupBox(parent) {
    setTitle("功能按钮区");
    InitButtons();
}

void FunctionBtnArea::InitButtons() {
    auto main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(10, 10, 10, 10);
    main_layout->setSpacing(30);

    load_setting_btn_ = new QPushButton("读取配置参数文件", this);
    load_setting_btn_->setMinimumSize(BUTTON_WIDTH, BUTTON_HEIGHT);
    load_setting_btn_->setObjectName("FuncBtn");
    save_setting_btn_ = new QPushButton("保存配置参数文件", this);
    save_setting_btn_->setMinimumSize(BUTTON_WIDTH, BUTTON_HEIGHT);
    save_setting_btn_->setObjectName("FuncBtn");
    mode_change_btn_ = new QPushButton("手输模式", this);
    mode_change_btn_->setMinimumSize(BUTTON_WIDTH, BUTTON_HEIGHT);
    mode_change_btn_->setObjectName("FuncBtn");
    save_default_btn_ = new QPushButton("保存默认参数", this);
    save_default_btn_->setMinimumSize(BUTTON_WIDTH, BUTTON_HEIGHT);
    save_default_btn_->setObjectName("FuncBtn");
    save_eeprom_btn_ = new QPushButton("保存到EEPROM", this);
    save_eeprom_btn_->setMinimumSize(BUTTON_WIDTH, BUTTON_HEIGHT);
    save_eeprom_btn_->setObjectName("FuncBtn");
    load_to_table_btn_ = new QPushButton("读取参数到表格", this);
    load_to_table_btn_->setMinimumSize(BUTTON_WIDTH, BUTTON_HEIGHT);
    load_to_table_btn_->setObjectName("FuncBtn");
    clear_setting_btn_ = new QPushButton("清空配置参数", this);
    clear_setting_btn_->setMinimumSize(BUTTON_WIDTH, BUTTON_HEIGHT);
    clear_setting_btn_->setObjectName("FuncBtn");
    // confirm_btn_ = new QPushButton("一键确认", this);
    // confirm_btn_->setMinimumSize(BUTTON_WIDTH, BUTTON_HEIGHT);
    // confirm_btn_->setObjectName("FuncBtn");

    main_layout->addWidget(load_setting_btn_);
    main_layout->addWidget(save_setting_btn_);
    main_layout->addWidget(mode_change_btn_);
    main_layout->addWidget(save_default_btn_);
    main_layout->addWidget(save_eeprom_btn_);
    main_layout->addWidget(load_to_table_btn_);
    main_layout->addWidget(clear_setting_btn_);
    main_layout->addStretch();
    // main_layout->addWidget(confirm_btn_, 0, Qt::AlignBottom);

    ConnectSignles();
}

void FunctionBtnArea::ConnectSignles() {
    connect(load_setting_btn_, &QPushButton::clicked, this, &FunctionBtnArea::OnLoadSettingBtnClicked);
    connect(save_setting_btn_, &QPushButton::clicked, this, &FunctionBtnArea::SendSaveSettings);
    connect(mode_change_btn_, &QPushButton::clicked, this, &FunctionBtnArea::OnModeChangeBtnClicked);
    connect(save_default_btn_, &QPushButton::clicked, this, &FunctionBtnArea::SendSaveDefaultValue);
    connect(save_eeprom_btn_, &QPushButton::clicked, this, &FunctionBtnArea::SendSaveToEPROM);
    connect(load_to_table_btn_, &QPushButton::clicked, this, &FunctionBtnArea::SendReadFromEPROM);
    connect(clear_setting_btn_, &QPushButton::clicked, this, &FunctionBtnArea::SendClearModifyValue);
    // connect(confirm_btn_, &QPushButton::clicked, this, &FunctionBtnArea::SendConfirmValues);
}

void FunctionBtnArea::OnLoadSettingBtnClicked() {
#ifdef ON_TEST_MODE
    QString file_path(TEST_CONFIG_FILE);
#else
    QString file_path = QFileDialog::getOpenFileName(nullptr, "Open File", "", "(*)");
#endif
    auto file_op = QFileOperator::GetInstance();
    
    qDebug() << "Open File:" << file_path;
    if(file_op->openFile(file_path)) {
        emit SendLoadSettings();
    }
}

void FunctionBtnArea::OnSaveSettingBtnClicked() {
    // QString save_path = QFileDialog::getSaveFileName(nullptr, "Save File", "", "(*)");
    // qDebug() << "Save File:" << save_path;
    emit SendSaveSettings();
}

void FunctionBtnArea::OnModeChangeBtnClicked() {
    QString mode = "";
    if(input_mode_ == kHand) {
        mode = QString("禁止手输");
        input_mode_ = kAuto;
    } else if (input_mode_ == kAuto) {
        mode = QString("手输模式");
        input_mode_ = kHand;
    }
    emit SendInputMode(input_mode_);
    mode_change_btn_->setText(mode);
}