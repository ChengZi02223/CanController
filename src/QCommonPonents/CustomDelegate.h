#ifndef CUSTOMDELEGATE_H
#define CUSTOMDELEGATE_H
#include <QStyledItemDelegate>
#include <QModelIndex>

class CustomDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit CustomDelegate(QObject *parent = nullptr);

signals:
    void cellEditReturnPressed(int row, int col, const QString &text);

protected:
    QWidget *createEditor(QWidget *parent,
                          const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;

    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const override;

    bool eventFilter(QObject *obj, QEvent *event) override;
};

class ButtonDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit ButtonDelegate(QObject *parent = nullptr);

    // 设置哪些列作为按钮列（可多列）
    void setButtonColumns(const QList<int> &columns);

protected:
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;

    bool editorEvent(QEvent *event, QAbstractItemModel *model,
                     const QStyleOptionViewItem &option,
                     const QModelIndex &index) override;

signals:
    void buttonClicked(int row, int column);

private:
    QList<int> m_buttonColumns;
};

#endif // CUSTOMDELEGATE_H
