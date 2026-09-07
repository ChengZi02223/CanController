#ifndef MESSAGEBOX_H
#define MESSAGEBOX_H

#include <QDialog>
#include <QTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QColor>

class MsgBox : public QDialog {
    Q_OBJECT
public:
    static void info(const QString &text);
    static void warning(const QString &text);
    static void clear();

private slots:
    void onFinished();

private:
    explicit MsgBox(QWidget *parent = nullptr);
    static MsgBox* instance();
    void append(const QString &text, const QColor &color);

private:
    QTextEdit* m_textEdit = nullptr;
    static MsgBox* m_self;
};

#endif // MESSAGEBOX_H
