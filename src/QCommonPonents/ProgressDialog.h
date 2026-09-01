#ifndef PROGRESSDIALOG_H
#define PROGRESSDIALOG_H

#include <QDialog>
#include <QProgressBar>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>

class ProgressDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ProgressDialog(QWidget *parent = nullptr);

    // 设置标题文本
    void setTitleText(const QString& text);
    // 设置按钮文字
    void setButtonText(const QString& text);

signals:
    // 按钮被点击信号
    void SendClose();
public slots:
    void setProgressValue(int value);
    void OnEndProgress();

protected:
    void closeEvent(QCloseEvent *event) override {
        emit SendClose();
        QDialog::closeEvent(event);
    }

private:
    QLabel* m_labelTitle{nullptr};
    QProgressBar* m_progressBar{nullptr};
    QPushButton* m_btnOperate{nullptr};
};

#endif // PROGRESSDIALOG_H
