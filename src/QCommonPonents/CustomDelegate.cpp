#include "CustomDelegate.h"
#include <QLineEdit>

CustomDelegate::CustomDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

QWidget *CustomDelegate::createEditor(QWidget *parent,
                                      const QStyleOptionViewItem &option,
                                      const QModelIndex &index) const
{
    QWidget* editor = QStyledItemDelegate::createEditor(parent, option, index);
    QLineEdit* lineEdit = qobject_cast<QLineEdit*>(editor);
    if(lineEdit)
    {
        //关键点：const函数内，用 const_cast 获取非const this
        CustomDelegate* nonConstThis = const_cast<CustomDelegate*>(this);
        int row = index.row();
        int col = index.column();

        connect(lineEdit, &QLineEdit::returnPressed, nonConstThis, [nonConstThis, row, col, lineEdit](){
            QString text = lineEdit->text();
            emit nonConstThis->cellEditReturnPressed(row, col, text);
        });
    }
    return editor;
}

