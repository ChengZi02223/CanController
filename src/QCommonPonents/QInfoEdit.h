// QInfoEdit.h
#pragma once

#include <QLineEdit>

class QInfoEdit : public QLineEdit
{
    Q_OBJECT
    Q_PROPERTY(QString infor READ infor WRITE setInfor)

public:
    explicit QInfoEdit(QWidget *parent = nullptr);
    explicit QInfoEdit(const QString &prefix, QWidget *parent = nullptr);

    // 获取版本号（不含前缀）
    QString infor() const;
    // 设置版本号（自动添加前缀）
    void setInfor(const QString &infor);
    void setPrefix(const QString &prefix);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    // void insert(const QString &text) override;

private slots:
    void onCursorPositionChanged(int oldPos, int newPos);
    void onTextChanged(const QString &text);

private:
    QString m_prefix_ = "";
    QString m_infor_;           // 不包含前缀的版本字符串

    void updateDisplayText();    // 根据 m_infor_ 刷新显示文本
    void ensurePrefixAndCursor();// 确保文本以前缀开头，光标不在前缀内
    void fixSelection();         // 限制选区不覆盖前缀
    void insertSafely(const QString &text); // 安全地在光标位置插入文本（仅操作后缀）
};