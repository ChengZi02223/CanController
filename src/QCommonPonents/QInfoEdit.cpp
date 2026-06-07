// QInfoEdit.cpp
#include "QInfoEdit.h"
#include <QKeyEvent>
#include <QClipboard>
#include <QApplication>
#include <QDebug>

QInfoEdit::QInfoEdit(QWidget *parent)
    : QLineEdit(parent)
{
    setAlignment(Qt::AlignCenter);
    updateDisplayText();

    connect(this, &QLineEdit::cursorPositionChanged,
            this, &QInfoEdit::onCursorPositionChanged);
    connect(this, &QLineEdit::textChanged,
            this, &QInfoEdit::onTextChanged);
}

QInfoEdit::QInfoEdit(const QString &prefix, QWidget *parent)
    : QInfoEdit(parent)
{
    setPrefix(prefix);
}

QString QInfoEdit::infor() const
{
    return m_infor_;
}

void QInfoEdit::setInfor(const QString &infor)
{
    if (m_infor_ != infor) {
        m_infor_ = infor;
        updateDisplayText();
    }
}

void QInfoEdit::setPrefix(const QString &prefix) {
    if (m_prefix_ != prefix) {
        m_prefix_ = prefix;
        updateDisplayText();
    }
}

void QInfoEdit::updateDisplayText()
{
    // 避免触发 textChanged 循环
    bool blocked = blockSignals(true);
    setText(m_prefix_ + m_infor_);
    blockSignals(blocked);
    // 确保光标有效位置（如果光标超出范围，自动修正）
    int curPos = cursorPosition();
    if (curPos < m_prefix_.length())
        setCursorPosition(m_prefix_.length());
}

void QInfoEdit::ensurePrefixAndCursor()
{
    QString current = text();
    if (!current.startsWith(m_prefix_)) {
        // 恢复正确内容
        updateDisplayText();
        return;
    }
    // 提取版本号（可能用户修改了前缀后的内容）
    QString newVersion = current.mid(m_prefix_.length());
    if (newVersion != m_infor_) {
        m_infor_ = newVersion;
        emit textChanged(current); // 手动通知变化，但已由信号处理，此处避免递归
    }
    // 光标位置修正
    if (cursorPosition() < m_prefix_.length())
        setCursorPosition(m_prefix_.length());
}

void QInfoEdit::fixSelection()
{
    int selStart = selectionStart();
    int selEnd = selStart + selectedText().length();
    if (selStart == -1)
        return;

    // 不允许选区覆盖前缀区域
    if (selStart < m_prefix_.length()) {
        selStart = m_prefix_.length();
        if (selEnd < selStart)
            selEnd = selStart;
        // 重新设置选区
        setSelection(selStart, selEnd - selStart);
    }
}

void QInfoEdit::insertSafely(const QString &text)
{
    int curPos = cursorPosition();
    if (curPos < m_prefix_.length()) {
        // 光标在前缀区，移动到前缀末尾再插入
        curPos = m_prefix_.length();
        setCursorPosition(curPos);
    }
    // 如果存在选区且选区包含前缀，则先修正选区
    if (hasSelectedText()) {
        int selStart = selectionStart();
        if (selStart < m_prefix_.length()) {
            // 选区包含前缀 -> 移除整个选区，但保留前缀部分不变
            QString fullText = text;
            QString newSuffix = fullText.mid(m_prefix_.length());
            int selEnd = selStart + selectedText().length();
            int suffixStart = m_prefix_.length();
            int removedStart = std::max(selStart, suffixStart);
            int removedLen = selEnd - removedStart;
            if (removedLen > 0) {
                newSuffix.remove(removedStart - suffixStart, removedLen);
                m_infor_ = newSuffix;
                updateDisplayText();
                setCursorPosition(suffixStart + (removedStart - suffixStart));
            }
        }
    }
    // 正常插入到后缀区
    // QLineEdit::insert(text);
    insertSafely(text);
}

void QInfoEdit::keyPressEvent(QKeyEvent *event)
{
    int curPos = cursorPosition();
    int prefixLen = m_prefix_.length();

    // 处理 Backspace 键（删除光标前一个字符）
    if (event->key() == Qt::Key_Backspace) {
        if (curPos <= prefixLen) {
            // 不可删除前缀
            event->accept();
            return;
        }
        // 如果有选区且选区覆盖前缀，不允许
        if (hasSelectedText()) {
            int selStart = selectionStart();
            if (selStart < prefixLen) {
                event->accept();
                return;
            }
        }
        QLineEdit::keyPressEvent(event);
        return;
    }

    // 处理 Delete 键（删除光标后一个字符）
    if (event->key() == Qt::Key_Delete) {
        if (curPos < prefixLen) {
            // 光标在前缀内，不允许删除
            event->accept();
            return;
        }
        if (hasSelectedText()) {
            int selStart = selectionStart();
            if (selStart < prefixLen) {
                event->accept();
                return;
            }
        }
        QLineEdit::keyPressEvent(event);
        return;
    }

    // 处理方向键：阻止光标移到前缀区内
    if (event->key() == Qt::Key_Left || event->key() == Qt::Key_Home) {
        QLineEdit::keyPressEvent(event);
        if (cursorPosition() < prefixLen)
            setCursorPosition(prefixLen);
        return;
    }

    if (event->key() == Qt::Key_Right || event->key() == Qt::Key_End) {
        QLineEdit::keyPressEvent(event);
        // 右方向不需要特别限制，但确保不会越界
        return;
    }

    // 处理普通字符输入
    if (!event->text().isEmpty() && event->text()[0].isPrint()) {
        // 如果光标在前缀区，强行移动到前缀后
        if (curPos < prefixLen) {
            setCursorPosition(prefixLen);
        }
        // 如果有选区且包含前缀，则先修正选区
        if (hasSelectedText()) {
            int selStart = selectionStart();
            if (selStart < prefixLen) {
                // 选区包含前缀，只删除后缀部分
                int selEnd = selStart + selectedText().length();
                int suffixStart = prefixLen;
                QString newSuffix = text().mid(suffixStart);
                int removedStart = std::max(selStart, suffixStart);
                int removedLen = selEnd - removedStart;
                if (removedLen > 0) {
                    newSuffix.remove(removedStart - suffixStart, removedLen);
                    m_infor_ = newSuffix;
                    updateDisplayText();
                    setCursorPosition(suffixStart + (removedStart - suffixStart));
                }
                event->accept();
                // 接着插入新字符
                insertSafely(event->text());
                return;
            }
        }
        QLineEdit::keyPressEvent(event);
        return;
    }

    // 处理 Ctrl+V（粘贴）
    if (event->matches(QKeySequence::Paste)) {
        QClipboard *clipboard = QApplication::clipboard();
        QString clipText = clipboard->text();
        if (!clipText.isEmpty()) {
            // 允许粘贴到后缀区，但要过滤掉可能破坏前缀的内容
            insertSafely(clipText);
        }
        event->accept();
        return;
    }

    // 其他按键（如 Ctrl+C, Ctrl+X 等）
    if (event->key() == Qt::Key_X && event->modifiers() == Qt::ControlModifier) {
        // 如果选区包含前缀，不允许剪切
        if (hasSelectedText()) {
            int selStart = selectionStart();
            if (selStart < prefixLen) {
                event->accept();
                return;
            }
        }
    }
    QLineEdit::keyPressEvent(event);
}

void QInfoEdit::mousePressEvent(QMouseEvent *event)
{
    QLineEdit::mousePressEvent(event);
    // 如果光标点击后落到了前缀区，强行移动到前缀后
    if (cursorPosition() < m_prefix_.length())
        setCursorPosition(m_prefix_.length());
    fixSelection();
}

void QInfoEdit::mouseMoveEvent(QMouseEvent *event)
{
    QLineEdit::mouseMoveEvent(event);
    fixSelection();
}

void QInfoEdit::mouseReleaseEvent(QMouseEvent *event)
{
    QLineEdit::mouseReleaseEvent(event);
    fixSelection();
}

// void QInfoEdit::insert(const QString &text)
// {
//     // 重写 insert，确保只能插入到后缀区
//     insertSafely(text);
// }

void QInfoEdit::onCursorPositionChanged(int /*oldPos*/, int newPos)
{
    if (newPos < m_prefix_.length())
        setCursorPosition(m_prefix_.length());
    fixSelection();
}

void QInfoEdit::onTextChanged(const QString &text)
{
    // 防御：确保文本仍以前缀开头，否则恢复
    if (!text.startsWith(m_prefix_)) {
        updateDisplayText();
        return;
    }
    // 提取版本号部分
    QString newVersion = text.mid(m_prefix_.length());
    if (newVersion != m_infor_) {
        m_infor_ = newVersion;
    }
}