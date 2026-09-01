#include "ProgressDialog.h"

ProgressDialog::ProgressDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setMinimumSize(350,140);

    m_labelTitle = new QLabel(this);
    m_labelTitle->setAlignment(Qt::AlignCenter);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0,100);
    m_progressBar->setValue(0);

    m_btnOperate = new QPushButton(this);
    m_btnOperate->setVisible(false);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setSpacing(12);
    layout->setContentsMargins(20,20,20,20);

    layout->addWidget(m_labelTitle);
    layout->addWidget(m_progressBar);
    layout->addWidget(m_btnOperate,0,Qt::AlignCenter);

    setLayout(layout);

    connect(m_btnOperate,&QPushButton::clicked,this,[this](){
        close();
    });
}

void ProgressDialog::OnEndProgress() {
    m_btnOperate->setVisible(true);
    setTitleText("参数读取完成！");
}

void ProgressDialog::setTitleText(const QString &text)
{
    m_labelTitle->setText(text);
}

void ProgressDialog::setProgressValue(int value)
{
    m_progressBar->setValue(value);
}

void ProgressDialog::setButtonText(const QString &text)
{
    m_btnOperate->setText(text);
}
