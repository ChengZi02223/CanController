#include "QFileOperator.h"

#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QFileInfo>

// ==================== 构造/析构 ====================
QFileOperator::QFileOperator()
{
}

QFileOperator::~QFileOperator()
{
}

// ==================== 自动打开/保存 ====================
bool QFileOperator::openFile(const QString &filePath)
{
    QFileInfo info(filePath);
    QString suffix = info.suffix().toLower();

    if (suffix == "csv") {
        return openCSV(filePath);
    } else if (suffix == "xlsx" || suffix == "xls") {
        return openExcel(filePath);
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

// #define USE_QXLSX
#ifdef USE_QXLSX
#include "xlsxdocument.h"
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
    int row = 1;
    while (true) {
        QStringList rowData;
        int col = 1;
        while (true) {
            QXlsx::Cell *cell = doc.cellAt(row, col);
            if (!cell) break;
            QVariant value = cell->value();
            rowData.append(value.toString());
            col++;
        }
        if (rowData.isEmpty() && row > 1) break; // 连续空行停止（简单启发）
        if (!rowData.isEmpty()) m_data.append(rowData);
        row++;
    }
    return true;
#else
    Q_UNUSED(filePath)
    qWarning() << "Excel support not compiled. Please integrate QXlsx library and define USE_QXLSX.";
    return false;
#endif
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