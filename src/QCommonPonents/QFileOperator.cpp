#include "QFileOperator.h"

#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QFileInfo>

#define EXCEL_DATA_START_ROW 2
#define MODIFY_VALUE_COL 7

// ==================== 自动打开/保存 ====================
bool QFileOperator::openFile(const QString &filePath)
{
    m_last_load_file = filePath;
    QFileInfo info(filePath);
    QString suffix = info.suffix().toLower();

    if (suffix == "csv") {
        return openCSV(filePath);
    } else if (suffix == "xlsx" || suffix == "xls") {
        // return openExcel(filePath);
        return ParseExcel(filePath);
    } else {
        qWarning() << "Unsupported file format:" << suffix;
        return false;
    }
}

bool QFileOperator::saveFile(const QString &filePath)
{
    QFileInfo info(filePath);
    QString suffix = info.suffix().toLower();

    if (suffix == "csv") {
        return saveCSV(filePath);
    } else if (suffix == "xlsx" || suffix == "xls") {
        return saveExcel(filePath);
    } else {
        qWarning() << "Unsupported file format:" << suffix;
        return false;
    }
}

// ==================== CSV 读取 ====================
bool QFileOperator::openCSV(const QString &filePath, QChar delimiter)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Cannot open file for reading:" << filePath;
        return false;
    }

    QTextStream stream(&file);
    stream.setCodec("UTF-8");
    m_data.clear();

    while (!stream.atEnd()) {
        QString line = stream.readLine();
        if (line.isEmpty()) {
            // 空行跳过，可根据需要调整
            continue;
        }
        QStringList row = parseCSVLine(line, delimiter);
        m_data.append(row);
    }

    file.close();
    return true;
}

// 解析CSV一行（处理引号、逗号、换行等）
QStringList QFileOperator::parseCSVLine(const QString &line, QChar delimiter)
{
    QStringList fields;
    QString field;
    bool inQuotes = false;
    int i = 0;
    int len = line.length();

    while (i < len) {
        QChar ch = line[i];

        if (inQuotes) {
            if (ch == '\"') {
                // 处理转义引号 ""
                if (i + 1 < len && line[i+1] == '\"') {
                    field.append('\"');
                    i += 2;
                    continue;
                } else {
                    inQuotes = false;
                    i++;
                    continue;
                }
            } else {
                field.append(ch);
                i++;
            }
        } else {
            if (ch == '\"') {
                inQuotes = true;
                i++;
            } else if (ch == delimiter) {
                fields.append(field);
                field.clear();
                i++;
            } else if (ch == '\r' || ch == '\n') {
                // 行内不应该出现换行符，但若出现则忽略
                i++;
            } else {
                field.append(ch);
                i++;
            }
        }
    }

    fields.append(field);
    return fields;
}

// ==================== CSV 保存 ====================
bool QFileOperator::saveCSV(const QString &filePath, QChar delimiter)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Cannot open file for writing:" << filePath;
        return false;
    }

    QTextStream stream(&file);
    stream.setCodec("UTF-8");

    for (int i = 0; i < m_data.size(); ++i) {
        QStringList row = m_data[i];
        QStringList escapedRow;
        for (const QString &field : row) {
            escapedRow.append(escapeCSVField(field, delimiter));
        }
        stream << escapedRow.join(delimiter);
        if (i != m_data.size() - 1) {
            stream << "\n";
        }
    }

    file.close();
    return true;
}

// 转义CSV字段（必要时加引号并转义内部引号）
QString QFileOperator::escapeCSVField(const QString &field, QChar delimiter)
{
    bool needQuote = field.contains(delimiter) || field.contains('\"') ||
                     field.contains('\n') || field.contains('\r');

    if (needQuote) {
        QString escaped = field;
        escaped.replace("\"", "\"\""); // 双引号转义为两个双引号
        return QString("\"%1\"").arg(escaped);
    } else {
        return field;
    }
}

// ==================== Excel 操作（框架，需要集成第三方库） ====================
// 以下提供了基于 QXlsx 库的示例实现。如需使用，请：
// 1. 下载 QXlsx 库：https://github.com/QtExcel/QXlsx
// 2. 在项目中包含 QXlsx 头文件并链接库
// 3. 取消下面的 #define USE_QXLSX 注释

#define USE_QXLSX
#ifdef USE_QXLSX
#include "xlsxdocument.h"

/**
 * @brief 获取单元格真实值，如果是被合并单元格，返回合并左上角的值
 * @param doc QXlsx文档
 * @param mergeRanges 全部合并单元格缓存
 * @param row 物理行（从1开始）
 * @param col 物理列（从1开始）
 * @return 解析后字符串
 */
static QString getMergedCellValue(QXlsx::Document& doc, const QList<QXlsx::CellRange>& mergeRanges, int row, int col) {
    for(const auto& range : mergeRanges)
    {
        if(row >= range.firstRow() && row <= range.lastRow()
            && col >= range.firstColumn() && col <= range.lastColumn())
        {
            // 命中合并区域，读取左上角单元格
            int realRow = range.firstRow();
            int realCol = range.firstColumn();
            if(auto cell = doc.cellAt(realRow, realCol))
            {
                return cell->value().toString();
            }
            return "";
        }
    }
    // 不在任何合并区域，直接读取本单元格
    if(auto cell = doc.cellAt(row, col))
    {
        return cell->value().toString();
    }
    return "";
}

#endif



bool QFileOperator::openExcel(const QString &filePath)
{
#ifdef USE_QXLSX
    QXlsx::Document doc(filePath);
    if (!doc.load()) {
        qWarning() << "Failed to load Excel file:" << filePath;
        return false;
    }

    m_data.clear();

    int maxCol = doc.dimension().lastColumn();
    if (maxCol <= 0) maxCol = 11; // 兜底，你的表实际是11列

    int row = EXCEL_DATA_START_ROW;
    while (true) {
        QStringList rowData;
        bool hasAnyData = false;
        for (int col = 1; col <= maxCol; col++) {
            QVariant value;
            if (auto cell = doc.cellAt(row, col)) {
                value = cell->value();
                if (!value.toString().isEmpty())
                    hasAnyData = true;
            }
            rowData.append(value.toString()); // 空单元格也补空字符串，保证列对齐
        }
        if (!hasAnyData && row > EXCEL_DATA_START_ROW)
            break;
        if (hasAnyData)
            m_data.append(rowData);
        row++;
        qDebug() << "row: "<< rowData;
    }
    return true;
#else
    Q_UNUSED(filePath)
    qWarning() << "Excel support not compiled. Please integrate QXlsx library and define USE_QXLSX.";
    return false;
#endif
}

bool QFileOperator::ParseExcel(const QString &filePath){
#ifdef USE_QXLSX
    QXlsx::Document doc(filePath);
    if (!doc.load()) {
        qWarning() << "Failed to load Excel file:" << filePath;
        return false;
    }
    m_data.clear();

    auto sheet = doc.currentWorksheet();
    QList<QXlsx::CellRange> mergeRanges = sheet->mergedCells();

    int maxCol = doc.dimension().lastColumn();
    if (maxCol <= 0) maxCol = 11;

    int row = EXCEL_DATA_START_ROW;

    while (true)
    {
        // 判断当前行是否在某个纵向合并块内
        QXlsx::CellRange hitMergeRange;
        bool isMergeStart = false;
        bool inAnyMerge = false;
        for(const auto& r : mergeRanges)
        {
            if(row >= r.firstRow() && row <= r.lastRow() && r.rowCount()>1)
            {
                hitMergeRange = r;
                inAnyMerge = true;
                if(row == r.firstRow()){
                    isMergeStart = true;
                }
                break;
            }
        }

        if(!inAnyMerge)
        {
            // 普通行
            QStringList rowData;
            bool hasAnyData = false;
            for(int col=1; col<=maxCol; col++)
            {
                QString val = getMergedCellValue(doc, mergeRanges, row, col);
                rowData.append(val);
                if(!val.isEmpty()) hasAnyData = true;
            }
            if(!hasAnyData && row > EXCEL_DATA_START_ROW){
                break;
            }
            if(hasAnyData){
                m_data.append(rowData);
            }
            row++;
        }
        else
        {
            // 处于合并区域，只在合并起始行一次性生成所有子行
            if(isMergeStart)
            {
                int mergeFirst = hitMergeRange.firstRow();
                int mergeLast  = hitMergeRange.lastRow();
                // 遍历合并区域每一行，每一列调用getMergedCellValue自动补全所有合并列
                for(int subRow = mergeFirst; subRow <= mergeLast; subRow++)
                {
                    QStringList subRowData;
                    bool hasAnyData = false;
                    for(int col = 1; col <= maxCol; col++)
                    {
                        QString val = getMergedCellValue(doc, mergeRanges, subRow, col);
                        subRowData.append(val);
                        if(!val.isEmpty()) hasAnyData = true;
                    }
                    if(hasAnyData){
                        m_data.append(subRowData);
                    }
                    qDebug()<<"split merge subRow:"<<subRow<<" data:"<<subRowData;
                }
            }
            // 直接跳到合并区域下一行，跳过中间被合并的物理行，避免重复解析
            row = hitMergeRange.lastRow() + 1;
        }
    }
#endif
    return true;
}

bool QFileOperator::saveExcel(const QString &filePath)
{
#ifdef USE_QXLSX
    QXlsx::Document doc;
    for (int row = 0; row < m_data.size(); ++row) {
        for (int col = 0; col < m_data[row].size(); ++col) {
            doc.write(row + 1, col + 1, m_data[row][col]);
        }
    }
    return doc.saveAs(filePath);
#else
    Q_UNUSED(filePath)
    qWarning() << "Excel support not compiled. Please integrate QXlsx library and define USE_QXLSX.";
    return false;
#endif
}

bool QFileOperator::SaveModifyValueToLastFile(const QList<QString> &paraList) {
#ifdef USE_QXLSX
    if(m_last_load_file.isEmpty()) {
        return false;
    }

    QXlsx::Document doc(m_last_load_file);
    if(!doc.load())
    {
        qWarning()<<"打开原文件失败："<<m_last_load_file;
        return false;
    }

    int startExcelRow = 3;
    for(int i = 0; i < paraList.size(); i++) {
        int excelRow = startExcelRow + i;
        QString modifyVal = paraList.at(i);
        //只写入修改值这一列，其他全部保留原文件原样（合并单元格、样式全部不动）
        doc.write(excelRow, MODIFY_VALUE_COL, modifyVal);
    }

    //保存覆盖原文件
    bool ok = doc.saveAs(m_last_load_file);
    if(!ok) {
        qWarning()<<"保存回写文件失败！文件被占用？"<<m_last_load_file;
    }
    return ok;
#else
    qWarning() << "Excel support not compiled. Please integrate QXlsx library and define USE_QXLSX.";
    return false;
#endif
}

// ==================== 数据访问方法 ====================
QList<QStringList> QFileOperator::getData() const
{
    return m_data;
}

void QFileOperator::setData(const QList<QStringList> &data)
{
    m_data = data;
}

QStringList QFileOperator::getRow(int row) const
{
    if (row < 0 || row >= m_data.size())
        return QStringList();
    return m_data[row];
}

QString QFileOperator::getCell(int row, int col) const
{
    if (row < 0 || row >= m_data.size())
        return QString();
    const QStringList &rowData = m_data[row];
    if (col < 0 || col >= rowData.size())
        return QString();
    return rowData[col];
}

bool QFileOperator::setCell(int row, int col, const QString &value)
{
    if (row < 0) return false;
    if (row >= m_data.size()) {
        // 扩充行
        for (int i = m_data.size(); i <= row; ++i)
            m_data.append(QStringList());
    }
    QStringList &rowData = m_data[row];
    if (col < 0) return false;
    if (col >= rowData.size()) {
        // 扩充列
        for (int i = rowData.size(); i <= col; ++i)
            rowData.append(QString());
    }
    rowData[col] = value;
    return true;
}

void QFileOperator::clearData()
{
    m_data.clear();
}

int QFileOperator::rowCount() const
{
    return m_data.size();
}

int QFileOperator::columnCount() const
{
    if (m_data.isEmpty())
        return 0;
    int maxCol = 0;
    for (const QStringList &row : m_data) {
        if (row.size() > maxCol)
            maxCol = row.size();
    }
    return maxCol;
}

// ==================== 结构体 ParaItem 实现 ====================
bool ParaItem::fromRow(const QStringList &row)
{
    // 不足11列自动补空字符串，防止越界
    serialNum    = row.value(0, "");
    saeParamId   = row.value(1, "");
    objDictName  = row.value(2, "");
    indexNum     = row.value(3, "");
    subIndex     = row.value(4, "");
    dataType     = row.value(5, "");
    paramValue   = row.value(6, "");
    rwDesc       = row.value(7, "");
    funcDesc     = row.value(8, "");
    objType      = row.value(9, "");
    remark       = row.value(10, "");
    return true;
}

// ==================== 新增：结构体数组转换接口 ====================
QList<ParaItem> QFileOperator::getTableItems() const
{
    QList<ParaItem> itemList;
    const QList<QStringList>& allRows = m_data;

    // 跳过表头行(第0行)，从第1行开始读取真实数据
    for (int i = 1; i < allRows.size(); ++i)
    {
        const QStringList& oneRow = allRows[i];
        ParaItem item;
        item.fromRow(oneRow);
        itemList.append(item);
    }
    return itemList;
}

void QFileOperator::setTableItems(const QList<ParaItem> &itemList)
{
    m_data.clear();
    // 先写入固定表头行
    QStringList header = {
        "序号",
        "SAE1939参数ID",
        "对象字典名称",
        "索引号",
        "子索引",
        "数据类型",
        "参数值",
        "读写说明",
        "功能说明",
        "参数对象类型",
        "备注"
    };
    m_data.append(header);

    // 遍历结构体，转成字符串行存入m_data
    for (const ParaItem& item : itemList)
    {
        QStringList row;
        row << item.serialNum
            << item.saeParamId
            << item.objDictName
            << item.indexNum
            << item.subIndex
            << item.dataType
            << item.paramValue
            << item.rwDesc
            << item.funcDesc
            << item.objType
            << item.remark;
        m_data.append(row);
    }
}