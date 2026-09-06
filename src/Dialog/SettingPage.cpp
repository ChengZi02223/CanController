#include "SettingPage.h"

#include "QFileOperator.h"
#include "CanDriver.h"
#include "can_cmd.h"
#include "Utils.h"
#include "CanManager.h"
#include "ProgressDialog.h"
#include "CustomDelegate.h"
#include "CanConfig.h"

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

static int info_table_count = 0;

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
    } else if(s.contains("8") || s == "RECORD") {
        return  ParamType::kUINT8;
    } else if(s.toLower().contains("string")) {
        return  ParamType::kSTRING;
    }
    return ParamType::kError;
}

inline int GetParamTypeMaxValue(ParamType type) {
    switch(type) {
        case ParamType::kUINT8:
            return 0xFF;
        case ParamType::kUINT16:
            return 0xFFFF;
        case ParamType::kUINT32:
            return 0xFFFFFFFF;
        default:
            return 0;
    }
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
    connect(function_btn_area_, &FunctionBtnArea::SendReadValue, setting_info_table_, &SettingInfoTable::OnSetRowValue);
    // connect(function_btn_area_, &FunctionBtnArea::SendInputMode, this, &SettingPage::SendInputMode);

    connect(basic_info_bar_, &BasicInfoBar::SendTestState, [=](TestState state){
        setting_info_table_->setEnabled(state == kTestStart);
        function_btn_area_->setEnabled(state == kTestStart);
    });
    connect(basic_info_bar_, &BasicInfoBar::SendInfoChanged, this, &SettingPage::SendInfoChanged);
    // connect(setting_info_table_, &QTableWidget::itemChanged, this, &SettingPage::OnValueChanged);
    connect(this, &SettingPage::SendRowValue, setting_info_table_, &SettingInfoTable::OnSetRowValue);
    CustomDelegate* m_delegate = new CustomDelegate(this);
    setting_info_table_->setItemDelegate(m_delegate);
    connect(m_delegate, &CustomDelegate::cellEditReturnPressed,
            this, [this](int row, int col, const QString &newText){
        auto item = setting_info_table_->item(row, col);
        if(item) {
            qDebug() << "Old value: " << item->text() << ", New value: " << newText;
            old_item_value_ = item->text();
            item->setText(newText);
            OnValueChanged(item);
        }
    });
}

void SettingPage::InitBasicInfo(BasicInfo info) { basic_info_bar_->InitData(info); }

void SettingPage::OnValueChanged(QTableWidgetItem *item) {
    setting_info_table_->ChangRowValue(item, old_item_value_);
}

SettingInfoTable::SettingInfoTable(QWidget* parent) : QTableWidget(parent) {
    setEditTriggers(QAbstractItemView::AllEditTriggers);
    connect(this, &QTableWidget::cellChanged, [this](int row, int col){
        if(col != INFO_TABLE_MODIFY_COLUMN) {
            return;
        }
        auto save_item = item(row, INFO_TABLE_SAVE_COLUMN);
        auto change_item = item(row, INFO_TABLE_MODIFY_COLUMN);
        // qDebug() << "Cell changed: " << save_item->text() << " --- " << change_item->text();
        if(save_item->text().trimmed() != change_item->text().trimmed()) {
            change_item->setForeground(QBrush(Qt::red));
        } else {
            change_item->setForeground(QBrush(Qt::black));
        }
    });
    setObjectName("SettingInfoTable");
    setColumnCount(COLUMN_COUNT);
    setHorizontalHeaderLabels(info_table_h_head);
    verticalHeader()->setVisible(false);

    save_progress_dialog_ = new ProgressDialog(this);
    save_progress_dialog_->setVisible(false);
    save_progress_dialog_->setButtonText("OK");
    connect(this, &SettingInfoTable::updateProgress, save_progress_dialog_, &ProgressDialog::setProgressValue);
    connect(this, &SettingInfoTable::SendReadFinished, save_progress_dialog_, &ProgressDialog::OnEndProgress);
    connect(save_progress_dialog_, &ProgressDialog::SendClose, this, [this](){
        if(save_thread_ != nullptr) {
            save_running_.store(false);
            save_thread_->quit();
            save_thread_->wait();
            delete save_thread_;
            save_thread_ = nullptr;
        }
    });

    qRegisterMetaType<QVector<int>>("QVector<int>");
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
    p_item_type_[row] = GetParamType(item.dataType);
}

std::vector<uint8_t> SettingInfoTable::GetRowCMD(int row) {
    return row_cmd_map_[row];
}

ParamType SettingInfoTable::GetRowParamType(int row) {
    if(row < 0 || row >= rowCount()) {
        return ParamType::kError;
    }
    auto p_item = item(row, INFO_TABLE_PARAM_COLUMN);
    if(!p_item) {
        return ParamType::kError;
    }
    return p_item_type_.at(row);
}

void SettingInfoTable::OnSetRowValue(QString value, QString idx, QString sub_idx) {
    if(idx.isEmpty()) {
        return;
    }
    // qDebug() << "OnSetRowValue: " << value << " " << idx << " " << sub_idx;
    QString targetIndex = idx.toUpper();
    for(int row = 0; row < rowCount(); row++) {
        QTableWidgetItem* objItem = item(row, INFO_TABLE_OBJ_COLUMN);
        if(!objItem) continue;

        // 表格单元格文本统一大写，比对
        QString tableIndex = objItem->text().toUpper();
        if(tableIndex != targetIndex)
            continue;

        // 对象字典索引匹配成功，校验子索引
        QTableWidgetItem* sub_idx_item = item(row, INFO_TABLE_IDX_COLUMN);
        if(!sub_idx_item) continue;

        // 表格单元格文本统一大写，比对
        QString sub_index = sub_idx_item->text().toUpper();
        if(sub_index == "——") {
            sub_index = "0X00";
        } 
        
        if(sub_index != sub_idx && !sub_idx.contains(sub_index)){
            continue;
        }

        // 匹配成功，更新修改值
        QTableWidgetItem* modify_item = item(row, INFO_TABLE_MODIFY_COLUMN);
        if(!modify_item) continue;
        modify_item->setText(value);
        // qDebug() << "成功更新行: "<<row << value << " " << idx << " " << sub_idx  <<" == " <<sub_index ;
    }
}

std::vector<uint8_t> SettingInfoTable::CreateRowCmd(ParaItem item) {
    // qDebug() << "CreateRowCmd: " << item.objDictName << " " << item.indexNum << " " << item.subIndex << " " << item.paramValue;
    std::vector<uint8_t> command = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    command[0] = GetCmdHeadByType(item.dataType);
    Fill16ValueToCmd(QStringToUint16Hex(item.indexNum), command, 1);
    auto subIndex = item.subIndex.trimmed();
    if(subIndex.isEmpty() || subIndex == "——") {
        command[3] = 0x00;
    } else {
        command[3] = QStringToUint8Hex(item.subIndex);
    }

    std::vector<uint8_t> vec_cmd(command.begin(), command.end());

    // PrintCmd(SDO_COB_ID, vec_cmd, "Create Row CMD: ");

    return vec_cmd;
}

void SettingInfoTable::ChangRowValue(QTableWidgetItem *item, QString old_value) {
    auto cmd = GetRowCMD(item->row());
    if(cmd.empty()) {
        return;
    }

    if(IsItemReadOnly(item)) {
        return;
    }
    auto type = GetRowParamType(item->row());
    if(!old_value.isEmpty() && GetParamTypeMaxValue(type) < item->text().toULongLong()) {
        QMessageBox::warning(this, "Warning", "修改值超出范围!");
        item->setText(old_value);
        return;
    }
    // qDebug() << "OnValueChanged: " << item->text() << " type: " << static_cast<int>(type);
    if(type == ParamType::kUINT8) {
        cmd[4] = QStringToUint8Dec(item->text());
    } else if(type == ParamType::kUINT16) {
        Fill16ValueToCmd(QStringToUint16Dec(item->text()), cmd, 4);
    } else if(type == ParamType::kUINT32) {
        Fill32ValueToCmd(QStringToUint32Dec(item->text()), cmd, 4);
    } 

    CanDriver::GetInstance()->SendCmd(SDO_COB_ID, cmd, kCmdTimeOut);
}

bool SettingInfoTable::IsItemReadOnly(QTableWidgetItem *item) {
    if(!item) {
        return true;
    }
    auto flags = item->flags();
    return !(flags & Qt::ItemIsEditable);
}

void SettingInfoTable::resizeEvent(QResizeEvent* event)  {
    if(columnCount() < 0){
        return; 
    }
    auto col_width = (width() - 10) / columnCount();
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
    p_item_type_.clear();
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
    info_table_count = rowCount();
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
    CanDriver::GetInstance()->SendCmd(SDO_COB_ID, SDO_SAVE_DEFAULT_CMD, kCmdTimeOut);
    OnConfirmAllValues();
    update();
}

void SettingInfoTable::OnSaveToEPROM() {
    UpdateParams();
    CanDriver::GetInstance()->SendCmd(SDO_COB_ID, SDO_SAVE_USER_SETTING_CMD, kCmdTimeOut);

    for(int i = 0; i < rowCount(); ++i) {
        user_values_[i] = item(i, INFO_TABLE_MODIFY_COLUMN)->text();
    }
}

void SettingInfoTable::UpdateParams() {
    if(save_thread_ != nullptr){
        return; 
    }

    save_running_.store(true);
    save_thread_ = QThread::create([this](){
        for(int i = 0; i < rowCount(); ++i) {
            auto change_item = item(i, INFO_TABLE_MODIFY_COLUMN);
            ChangRowValue(change_item);
            change_item->setForeground(QBrush(Qt::black));
            emit updateProgress((i + 1) * 100 / rowCount());
            QThread::msleep(200); 
        }
        emit SendReadFinished();
    });
    save_thread_->start();
    save_progress_dialog_->setTitleText("正在保存参数到EPROM，请稍候...");
    save_progress_dialog_->exec();
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
    progress_dialog_ = new ProgressDialog(this);
    progress_dialog_->setVisible(false);
    progress_dialog_->setButtonText("OK");
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
    connect(load_to_table_btn_, &QPushButton::clicked, this, &FunctionBtnArea::OnLoadToTableBtnClicked);
    connect(clear_setting_btn_, &QPushButton::clicked, this, &FunctionBtnArea::SendClearModifyValue);
    // connect(confirm_btn_, &QPushButton::clicked, this, &FunctionBtnArea::SendConfirmValues);

    connect(CanConfigWin::GetInstance(), &CanConfigWin::SendReadFromEPROM, this, &FunctionBtnArea::OnReadFromEPROM);
    connect(this, &FunctionBtnArea::updateProgress, progress_dialog_, &ProgressDialog::setProgressValue);
    connect(this, &FunctionBtnArea::SendReadFinished, progress_dialog_, &ProgressDialog::OnEndProgress);
    connect(progress_dialog_, &ProgressDialog::SendClose, this, [this](){
        // StopReadFromEPROM();
        start_read_eprom_ = false;
        parse_count_ = 0;
        ClearStringFragmentCache();
    });
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

void FunctionBtnArea::OnLoadToTableBtnClicked() {
    qDebug() << "读取参数到表格";
    can_frame frame{};
    bool ret = CanDriver::GetInstance()->ExecCmd(SDO_COB_ID, SDO_READ_PARAM_TO_TABLE, frame, kCmdTimeOut);
#ifndef ON_TEST_MODE
    if (!ret || frame.can_id != 0x5C0) {
        QMessageBox::warning(this, "警告", "读取参数到表格失败，未接收到开始读取指令！");
        return;
    }
#endif
    // StartReadFromEPROM();
    start_read_eprom_ = true;
    progress_dialog_->setTitleText("正在读取参数，请稍候...");
    progress_dialog_->exec();
}

bool FunctionBtnArea::TestEPROMSenCmd(can_frame &frame) {
    if(read_index_ >= kBatchReadTestFrames.size()){
        return false;
    }
    auto test_frame = kBatchReadTestFrames[read_index_];
    frame.can_id = test_frame.cobId;
    frame.can_dlc = 8;
    std::copy(test_frame.data, test_frame.data + 8, frame.data);
    // PrintCmd(frame.can_id, std::vector<uint8_t>(frame.data, frame.data + frame.can_dlc), "Receive: ");
    read_index_ ++;
    return true;
}

#ifdef tt
bool FunctionBtnArea::StartReadFromEPROM() {
    if(parser_thread_ != nullptr){
        return false; 
    }
    parse_count_ = 0;
    read_index_ = 0;
    parser_running_.store(true);
    parser_thread_ = QThread::create([this](){
        while(parser_running_.load()) {
            can_frame frame{};
#ifdef ON_TEST_MODE
            if (TestEPROMSenCmd(frame)) {
                ParseEPROMFrame(frame);
            }
#else
            if (CanDriver::GetInstance()->receive(frame, kCmdTimeOut)) {
                ParseEPROMFrame(frame);
            }
#endif
            QThread::msleep(kSleepTimeOut);
        }
    });
    parser_thread_->start();
    qDebug()<< "读取线程开始";
}

void FunctionBtnArea::StopReadFromEPROM() {
    if(parser_thread_ == nullptr) {
        return;
    }
    
    parser_running_.store(false); //退出循环条件
    parser_thread_->quit();
    parser_thread_->wait(); //阻塞等待线程安全结束
    delete parser_thread_;
    parser_thread_ = nullptr;
    parse_count_ = 0;
    disconnect(this, &FunctionBtnArea::updateProgress, progress_dialog_, &ProgressDialog::setProgressValue);
    ClearStringFragmentCache();
    qDebug()<< "读取线程已结束";
}
#endif

void FunctionBtnArea::ClearStringFragmentCache() {
    // std::lock_guard<std::mutex> lk(m_mtx_);
    str_fragment_cache_.clear();
}

void FunctionBtnArea::OnReadFromEPROM(can_frame frame) {
    if(!start_read_eprom_){
        return;
    }
    ParseEPROMFrame(frame);
}

void FunctionBtnArea::ParseEPROMFrame(const can_frame &frame) {
    int local_parse_count;
    bool is_string_final_segment = false;
    QString final_str_value;
    QString final_index;
    QString final_subindex;

    {
        // std::lock_guard<std::mutex> lk(m_mtx_);
        if(frame.can_dlc < 8) {
            qDebug()<<"[frame]数据长度不足8字节";
            return;
        }
        if(frame.data[0] == 0xFF) {
            emit SendReadFinished();
            return;
        }

        uint16_t indexRaw = (static_cast<uint16_t>(frame.data[2]) << 8) | static_cast<uint16_t>(frame.data[1]);
        uint8_t subIndexRaw = frame.data[3];
        QString strIndex = QString("0x%1").arg(indexRaw,4,16,QChar('0')).toUpper();
        QString strSubIndex = QString("0x%1").arg(subIndexRaw,2,16,QChar('0')).toUpper();

        // ========== 判断是否字符串分片帧 data[0]==0x04 ==========
        if(frame.data[0] == 0x04) {
            //分片号，bit7标记是否结束分片
            uint8_t seg_flag = frame.data[4];
            bool is_last_seg = ((seg_flag & 0x80) != 0);
            QString cache_key = QString("%1,%2").arg(strIndex).arg(strSubIndex);

            //取出本分片3个字节 data5 data6 data7，转为char追加
            QByteArray seg_bytes;
            seg_bytes.append(static_cast<char>(frame.data[5]));
            seg_bytes.append(static_cast<char>(frame.data[6]));
            seg_bytes.append(static_cast<char>(frame.data[7]));

            //追加到缓存
            str_fragment_cache_[cache_key].append(QString::fromLocal8Bit(seg_bytes));

            if(is_last_seg)
            {
                //最后分片，取出完整字符串，拷贝到局部变量，锁外emit
                final_str_value = str_fragment_cache_[cache_key];
                final_index = strIndex;
                final_subindex = strSubIndex;
                //清除该key缓存
                str_fragment_cache_.erase(cache_key);
                is_string_final_segment = true;

                parse_count_++;
                local_parse_count = parse_count_;
            }
            else
            {
                //中间分片，只缓存，不发送信号，不增加parse_count
                return;
            }
        } else {
            //====普通数值帧，原有逻辑不变====
            uint32_t valueRaw = (static_cast<uint32_t>(frame.data[7])<<24)
                                | (static_cast<uint32_t>(frame.data[6])<<16)
                                | (static_cast<uint32_t>(frame.data[5])<<8)
                                | static_cast<uint32_t>(frame.data[4]);
            QString strValue = QString::number(valueRaw);

            parse_count_++;
            local_parse_count = parse_count_;

            //锁内只拷贝，emit放到锁外面
            final_str_value = strValue;
            final_index = strIndex;
            final_subindex = strSubIndex;
            is_string_final_segment = false;
        }
    } //锁释放

    //========锁外面发送信号，避免锁内emit死锁风险========
    emit SendReadValue(final_str_value, final_index, final_subindex);

    int percent = 0;
    if(info_table_count > 0) {
        percent = static_cast<int>(100.0 * local_parse_count / info_table_count);
    }
    emit updateProgress(percent);
}
