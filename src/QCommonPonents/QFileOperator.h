#ifndef _FILE_OPERATOR_H_
#define _FILE_OPERATOR_H_

#include <QList>
#include <QStringList>
#include <QString>
#include <QChar>

class QFileOperator
{
public:
    QFileOperator();
    ~QFileOperator();

    // 根据文件扩展名自动调用对应的打开/保存方法
    bool openFile(const QString &filePath);
    bool saveFile(const QString &filePath);

    // CSV 专用操作
    bool openCSV(const QString &filePath, QChar delimiter = ',');
    bool saveCSV(const QString &filePath, QChar delimiter = ',');

    // Excel 操作（需要第三方库支持，此处提供框架）
    bool openExcel(const QString &filePath);
    bool saveExcel(const QString &filePath);

    // 数据访问
    QList<QStringList> getData() const;
    void setData(const QList<QStringList> &data);
    QStringList getRow(int row) const;
    QString getCell(int row, int col) const;
    bool setCell(int row, int col, const QString &value);
    void clearData();
    int rowCount() const;
    int columnCount() const;

private:
    QList<QStringList> m_data;  // 存储表格数据，每行为QStringList

    // CSV 解析辅助函数
    QStringList parseCSVLine(const QString &line, QChar delimiter);
    QString escapeCSVField(const QString &field, QChar delimiter);
};



#endif // _FILE_OPERATOR_H_