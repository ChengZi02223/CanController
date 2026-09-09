#include "CustomDelegate.h"
#include <QLineEdit>
#include <QEvent>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QPainter>
#include <QStyle>
#include <QStyleOptionButton>
#include <QApplication>
#include <QMouseEvent>

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


ButtonDelegate::ButtonDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void ButtonDelegate::setButtonColumns(const QList<int> &columns)
{
    m_buttonColumns = columns;
}

void ButtonDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                           const QModelIndex &index) const
{
    if (!m_buttonColumns.contains(index.column())) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    // 获取按钮文字，用于判断 checked 状态
    QString text = index.data(Qt::DisplayRole).toString();
    bool isChecked = (text == "结束标定" || text == "验证中");   // 与业务逻辑匹配

    // 确定当前状态
    bool isEnabled = (option.state & QStyle::State_Enabled);
    bool isHover = (option.state & QStyle::State_MouseOver);
    bool isPressed = (option.state & QStyle::State_Sunken);

    // 选择背景颜色或渐变
    QBrush background;
    QColor textColor = Qt::white;

    if (!isEnabled) {
        // 禁用：灰底灰字
        background = QBrush(QColor(253, 186, 116));   // #fdba74
        textColor = QColor(160, 122, 74);            // #a07a4a
    } else if (isPressed) {
        // pressed：纯色深橙
        background = QBrush(QColor(194, 65, 12));     // #c2410c
    } else if (isChecked) {
        // checked：蓝色渐变
        QLinearGradient grad(option.rect.topLeft(), option.rect.bottomLeft());
        grad.setColorAt(0, QColor(37, 99, 235));      // #2563eb
        grad.setColorAt(1, QColor(29, 78, 216));      // #1d4ed8
        background = grad;
    } else if (isHover) {
        // hover：亮橙色渐变
        QLinearGradient grad(option.rect.topLeft(), option.rect.bottomLeft());
        grad.setColorAt(0, QColor(251, 146, 60));     // #fb923c
        grad.setColorAt(1, QColor(249, 115, 22));     // #f97316
        background = grad;
    } else {
        // 正常：橙色渐变
        QLinearGradient grad(option.rect.topLeft(), option.rect.bottomLeft());
        grad.setColorAt(0, QColor(249, 115, 22));     // #f97316
        grad.setColorAt(1, QColor(234, 88, 12));      // #ea580c
        background = grad;
    }

    painter->save();

    // 绘制圆角背景（圆角半径12px）
    painter->setPen(Qt::NoPen);
    painter->setBrush(background);
    painter->drawRoundedRect(option.rect, 12, 12);

    // 绘制文字（加 padding：左右16px，上下6px）
    QRect textRect = option.rect.adjusted(30, 15, -30, -15);
    // 如果矩形太小，取消 padding 以防文字被裁切
    if (textRect.width() < 20 || textRect.height() < 10) {
        textRect = option.rect;
    }
    painter->setPen(textColor);
    painter->drawText(textRect, Qt::AlignCenter, text);

    painter->restore();
}

bool ButtonDelegate::editorEvent(QEvent *event, QAbstractItemModel *model,
                                 const QStyleOptionViewItem &option,
                                 const QModelIndex &index)
{
    Q_UNUSED(model);
    if (!m_buttonColumns.contains(index.column()))
        return QStyledItemDelegate::editorEvent(event, model, option, index);

    if (event->type() == QEvent::MouseButtonRelease) {
        QMouseEvent *me = static_cast<QMouseEvent*>(event);
        if (option.rect.contains(me->pos())) {
            emit buttonClicked(index.row(), index.column());
            return true;   // 阻止表格默认行为（滚动/选中）
        }
    }
    return QStyledItemDelegate::editorEvent(event, model, option, index);
}