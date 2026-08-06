#include "SettingPage.h"
#include "BasicInfoBar.h"
#include "QFileOperator.h"

#define INFO_TABLE_PARAM_COLUMN 0
#define INFO_TABLE_OBJ_COLUMN 1
#define INFO_TABLE_IDX_COLUMN 2
#define INFO_TABLE_SAVE_COLUMN 3
#define INFO_TABLE_MODIFY_COLUMN 4

#define BUTTON_WIDTH 150
#define BUTTON_HEIGHT 40

#define ROW_COUNT 12
#define COLUMN_COUNT 8

static const QStringList info_table_h_head({"参数名称", "对象字典", "索引", "保存值", "修改值", "下限", "上限", "操作"});
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
    connect(function_btn_area_, &FunctionBtnArea::SendConfirmValues, setting_info_table_, &SettingInfoTable::OnConfirmAllValues);
    connect(function_btn_area_, &FunctionBtnArea::SendInputMode, this, &SettingPage::SendInputMode);
    connect(basic_info_bar_, &BasicInfoBar::SendTestState, [=](TestState state){
        setting_info_table_->setEnabled(state == kTestStart);
        function_btn_area_->setEnabled(state == kTestStart);
    });
}

void SettingPage::InitBasicInfo(BasicInfo info) { basic_info_bar_->InitData(info); }

SettingInfoTable::SettingInfoTable(QWidget* parent) : QTableWidget(parent) {
    setEditTriggers(QAbstractItemView::AllEditTriggers);
    connect(this, &QTableWidget::cellChanged, [this](int row, int col){
        if(col != INFO_TABLE_MODIFY_COLUMN) {
            return;
        }
        auto save_item = item(row, INFO_TABLE_SAVE_COLUMN);
        auto change_item = item(row, INFO_TABLE_MODIFY_COLUMN);
        if(save_item->text() != change_item->text()) {
            save_item->setForeground(QBrush(Qt::black));
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
    int row = serialNum - 1;
    insertRow(row);

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
    auto save_item = new QTableWidgetItem();
    save_item->setFlags(save_item->flags() & ~Qt::ItemIsEditable);
    save_item->setTextAlignment(Qt::AlignCenter);
    setItem(row, INFO_TABLE_SAVE_COLUMN, save_item);    
    // 参数值
    auto p_item = new QTableWidgetItem(item.paramValue);
    p_item->setFlags(p_item->flags() | Qt::ItemIsEditable);
    p_item->setTextAlignment(Qt::AlignCenter);
    setItem(row, INFO_TABLE_MODIFY_COLUMN, p_item);
    // 确定标定按钮
    auto enter_btn = new QPushButton("确认", this);
    enter_btn->setObjectName("EnterBtn");
    setCellWidget(row, COLUMN_COUNT - 1, enter_btn);
    connect(enter_btn, &QPushButton::clicked, this, [this, row](){
        OnEnterBtnClicked(row);
    });
    
}

void SettingInfoTable::resizeEvent(QResizeEvent* event)  {
    if(columnCount() < 0){
        return; 
    }
    auto col_width = width() / columnCount();
    for(int i = 0; i < columnCount(); ++i) {
        setColumnWidth(i, col_width);
    }
}

void SettingInfoTable::OnLoadSettings() {
    auto file_op = QFileOperator::GetInstance();
    QList<ParaItem> tableDataList = file_op->getTableItems();
    if(tableDataList.empty()){
        return;
    }
    for(auto item : tableDataList) {
        InsterRow(item);
    }

    for(int i = 0; i < rowCount(); ++i) {
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
    qDebug() << "OnSaveSettings";
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
        OnEnterBtnClicked(i);
    }
}

void SettingInfoTable::OnEnterBtnClicked(int row) {
    if(row < 0 || row > rowCount()) {
        return;
    }
    auto save_item = item(row, INFO_TABLE_SAVE_COLUMN);
    auto change_item = item(row, INFO_TABLE_MODIFY_COLUMN);
    save_item->setText(change_item->text());
    save_item->setForeground(QBrush(Qt::green));
}

void SettingInfoTable::OnModifyValueChanged(int col, int row) {

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
    confirm_btn_ = new QPushButton("一键确认", this);
    confirm_btn_->setMinimumSize(BUTTON_WIDTH, BUTTON_HEIGHT);
    confirm_btn_->setObjectName("FuncBtn");

    main_layout->addWidget(load_setting_btn_);
    main_layout->addWidget(save_setting_btn_);
    main_layout->addWidget(mode_change_btn_);
    main_layout->addWidget(save_default_btn_);
    main_layout->addWidget(save_eeprom_btn_);
    main_layout->addWidget(load_to_table_btn_);
    main_layout->addWidget(clear_setting_btn_);
    main_layout->addStretch();
    main_layout->addWidget(confirm_btn_, 0, Qt::AlignBottom);

    ConnectSignles();
}

void FunctionBtnArea::ConnectSignles() {
    connect(load_setting_btn_, &QPushButton::clicked, this, &FunctionBtnArea::OnLoadSettingBtnClicked);
    connect(save_setting_btn_, &QPushButton::clicked, this, &FunctionBtnArea::OnSaveSettingBtnClicked);
    connect(mode_change_btn_, &QPushButton::clicked, this, &FunctionBtnArea::OnModeChangeBtnClicked);
    connect(save_default_btn_, &QPushButton::clicked, this, &FunctionBtnArea::OnSaveDefaultBtnClicked);
    connect(save_eeprom_btn_, &QPushButton::clicked, this, &FunctionBtnArea::OnSaveEepromBtnClicked);
    connect(load_to_table_btn_, &QPushButton::clicked, this, &FunctionBtnArea::OnLoadToTableBtnClicked);
    connect(clear_setting_btn_, &QPushButton::clicked, this, &FunctionBtnArea::SendClearModifyValue);
    connect(confirm_btn_, &QPushButton::clicked, this, &FunctionBtnArea::SendConfirmValues);
}

void FunctionBtnArea::OnLoadSettingBtnClicked() {
    QString file_path = QFileDialog::getOpenFileName(nullptr, "Open File", "", "(*)");
    auto file_op = QFileOperator::GetInstance();
    
    qDebug() << "Open File:" << file_path;
    if(file_op->openFile(file_path)) {
        emit SendLoadSettings();
    }
}

void FunctionBtnArea::OnSaveSettingBtnClicked() {
    QString save_path = QFileDialog::getSaveFileName(nullptr, "Save File", "", "(*)");
    qDebug() << "Save File:" << save_path;
    emit SendSaveSettings();
}

void FunctionBtnArea::OnModeChangeBtnClicked() {
    QString mode = "";
    if(input_mode_ == kHand) {
        mode = QString("自动模式");
        input_mode_ = kAuto;
    } else if (input_mode_ == kAuto) {
        mode = QString("手输模式");
        input_mode_ = kHand;
    }
    emit SendInputMode(input_mode_);
    mode_change_btn_->setText(mode);
}

void FunctionBtnArea::OnSaveDefaultBtnClicked() {

}

void FunctionBtnArea::OnSaveEepromBtnClicked() {

}

void FunctionBtnArea::OnLoadToTableBtnClicked() {

}
