#include "MessageBox.h"
#include <QTextCursor>

MsgBox* MsgBox::m_self = nullptr;

MsgBox::MsgBox(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("消息提示"));
    setMinimumSize(520, 300);

    QVBoxLayout* layout = new QVBoxLayout(this);

    m_textEdit = new QTextEdit(this);
    m_textEdit->setReadOnly(true);
    layout->addWidget(m_textEdit);

    QPushButton* btnOk = new QPushButton(tr("确定"), this);
    layout->addWidget(btnOk);

    connect(btnOk, &QPushButton::clicked, this, &QDialog::close);
    connect(this, &QDialog::finished, this, &MsgBox::onFinished);
}

void MsgBox::onFinished()
{
    MsgBox::m_self = nullptr;
}

MsgBox* MsgBox::instance()
{
    if (!m_self)
    {
        m_self = new MsgBox(nullptr);
    }
    return m_self;
}

void MsgBox::append(const QString &text, const QColor &color)
{
    QTextCursor cursor = m_textEdit->textCursor();
    QTextCharFormat fmt;
    fmt.setForeground(QBrush(color));

    cursor.insertText(text + "\n", fmt);
    cursor.movePosition(QTextCursor::End);
    m_textEdit->setTextCursor(cursor);

    if (!isVisible())
    {
        exec();
    }
}

void MsgBox::info(const QString &text)
{
    instance()->append(text, Qt::black);
}

void MsgBox::warning(const QString &text)
{
    instance()->append(text, Qt::red);
}

void MsgBox::clear()
{
    if (m_self)
    {
        m_self->m_textEdit->clear();
    }
}
