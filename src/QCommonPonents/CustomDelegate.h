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

#endif // CUSTOMDELEGATE_H
