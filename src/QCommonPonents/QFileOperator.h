#ifndef _FILE_OPERATOR_H_
#define _FILE_OPERATOR_H_

#include <QList>
#include <QStringList>
#include <QString>
#include <QChar>

// 表格行对应结构体，匹配Excel/CSV表头
struct ParaItem {
    // 11列表头一一对应
    QString serialNum;      // 序号
    QString saeParamId;     // SAE1939参数ID
    QString objDictName;    // 对象字典名称
    QString indexNum;       // 索引号
    QString subIndex;       // 子索引
    QString dataType;       // 数据类型
    QString paramValue;     // 参数值
    QString rwDesc;         // 读写说明
    QString funcDesc;       // 功能说明
    QString objType;        // 参数对象类型
    QString remark;         // 备注

    // 构造初始化空值
    ParaItem() = default;
    // 从一行QStringList填充结构体
    bool fromRow(const QStringList& row);
};

class QFileOperator
{
public:
    static QFileOperator* GetInstance() {
        static QFileOperator *instance = nullptr;
        if(instance == nullptr) {
            instance = new QFileOperator();
        }
        return instance;
    }

    // 根据文件扩展名自动调用对应的打开/保存方法
    bool openFile(const QString &filePath);
    bool saveFile(const QString &filePath);

    // CSV 专用操作
    bool openCSV(const QString &filePath, QChar delimiter = ',');
    bool saveCSV(const QString &filePath, QChar delimiter = ',');

    // Excel 操作（需要第三方库支持，此处提供框架）
    bool openExcel(const QString &filePath);
    bool ParseExcel(const QString &filePath);
    bool saveExcel(const QString &filePath);
    bool SaveModifyValueToLastFile(const QList<QString> &paraList);
    QString GetLastLoadFile() {return m_last_load_file; }

    // 获取全部表格数据转为结构体数组
    QList<ParaItem> getTableItems() const;
    // 用结构体数组覆盖原有表格数据
    void setTableItems(const QList<ParaItem>& itemList);

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
    QFileOperator(){};
    ~QFileOperator(){};

private:
    QList<QStringList> m_data;  // 存储表格数据，每行为QStringList
    QString m_last_load_file;

    // CSV 解析辅助函数
    QStringList parseCSVLine(const QString &line, QChar delimiter);
    QString escapeCSVField(const QString &field, QChar delimiter);
};



#endif // _FILE_OPERATOR_H_