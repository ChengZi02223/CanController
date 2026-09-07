#include "CustomDelegate.h"
#include <QLineEdit>
#include <QEvent>
#include <QFocusEvent>
#include <QKeyEvent>

CustomDelegate::CustomDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

QWidget *CustomDelegate::createEditor(QWidget *parent,
                                      const QStyleOptionViewItem &option,
                                      const QModelIndex &index) const
{
    QWidget* editor = QStyledItemDelegate::createEditor(parent, option, index);

    // 保存原始值、提交标记
    QVariant oldVal = index.data(Qt::EditRole);
    editor->setProperty("oldValue", oldVal);
    editor->setProperty("isCommitted", false);

    editor->installEventFilter(const_cast<CustomDelegate*>(this));

    QLineEdit* lineEdit = qobject_cast<QLineEdit*>(editor);
    if(lineEdit)
    {
        CustomDelegate* nonConstThis = const_cast<CustomDelegate*>(this);
        int row = index.row();
        int col = index.column();

        connect(lineEdit, &QLineEdit::returnPressed, nonConstThis, [editor, nonConstThis, row, col, lineEdit](){
            // 回车标记已经提交，setModelData看到标记就不拦截
            editor->setProperty("isCommitted", true);
            emit nonConstThis->cellEditReturnPressed(row, col, lineEdit->text());
            lineEdit->clearFocus();
        });
    }
    return editor;
}

// 核心拦截：Qt要把编辑器内容写入model时会调用这个虚函数
void CustomDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    bool committed = editor->property("isCommitted").toBool();
    if (committed)
    {
        // 已经回车提交，但是！我们是外部代码写表格，代理这里也不写model
        Q_UNUSED(model);
        Q_UNUSED(index);
        return;
    }
    // === 点击其他cell会走到这里：isCommitted=false，直接不调用父类setModelData，拒绝保存新值 ===
    // 读取旧值，写回model，还原
    QVariant oldVal = editor->property("oldValue");
    model->setData(index, oldVal, Qt::EditRole);
}

bool CustomDelegate::eventFilter(QObject *obj, QEvent *event)
{
    if (!obj) // ✅增加判断对象是否已经标记待删除
    {
        return QStyledItemDelegate::eventFilter(obj, event);
    }
    QWidget* editor = qobject_cast<QWidget*>(obj);
    if (!editor)
    {
        return QStyledItemDelegate::eventFilter(obj, event);
    }

    // ESC按键还原
    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent* keyEv = static_cast<QKeyEvent*>(event);
        if(keyEv->key() == Qt::Key_Escape)
        {
            QVariant oldVar = editor->property("oldValue");
            QVariant idxVar = editor->property("editIndex");
            QModelIndex idx = idxVar.value<QModelIndex>();
            if(idx.isValid())
            {
                QAbstractItemModel* model = const_cast<QAbstractItemModel*>(idx.model());
                model->setData(idx, oldVar, Qt::EditRole);
            }
        }
    }
    return QStyledItemDelegate::eventFilter(obj, event);
}
