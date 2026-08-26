// QRangeSlider.cpp
#include "QRangeSlider.h"
#include <QApplication>

QRangeSlider::QRangeSlider(QWidget *parent)
    : QWidget(parent)
{
    // 创建控件
    m_slider = new QSlider(Qt::Horizontal);
    m_slider->setTickPosition(QSlider::TicksBelow);
    m_slider->setTickInterval(10);

    m_leftBtn  = new QPushButton("-");
    m_rightBtn = new QPushButton("+");

    // 设置按钮的自动重复（长按时持续触发 clicked 信号）
    m_leftBtn->setAutoRepeat(true);
    m_leftBtn->setAutoRepeatDelay(300);  // 长按 300ms 后开始重复
    m_leftBtn->setAutoRepeatInterval(50); // 每 50ms 触发一次

    m_rightBtn->setAutoRepeat(true);
    m_rightBtn->setAutoRepeatDelay(300);
    m_rightBtn->setAutoRepeatInterval(50);

    // 布局
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->addWidget(m_leftBtn);
    layout->addWidget(m_slider);
    layout->addWidget(m_rightBtn);
    setLayout(layout);

    // 信号槽连接
    connect(m_leftBtn, &QPushButton::clicked, this, &QRangeSlider::onLeftClicked);
    connect(m_rightBtn, &QPushButton::clicked, this, &QRangeSlider::onRightClicked);
    connect(m_slider, &QSlider::valueChanged, this, &QRangeSlider::onSliderMoved);
}

int QRangeSlider::value() const
{
    return m_slider->value();
}

void QRangeSlider::SetRange(int min, int max) {
    m_slider->setRange(min, max);
    m_slider->setValue(min);
    min_value_ = min;
    max_value_ = max;
}

void QRangeSlider::setValue(int val)
{
    val = qBound(min_value_, val, max_value_);
    if (val != m_slider->value()) {
        m_slider->setValue(val);
        // valueChanged 信号会在 slider 的 valueChanged 中发出，无需重复发送
    }
}

void QRangeSlider::onLeftClicked()
{
    int newVal = m_slider->value() - 1;
    if (newVal >= min_value_) {
        m_slider->setValue(newVal);
    }
}

void QRangeSlider::onRightClicked()
{
    int newVal = m_slider->value() + 1;
    if (newVal <= max_value_) {
        m_slider->setValue(newVal);
    }
}

void QRangeSlider::onSliderMoved(int val)
{
    // 将滑动条的值变化转发给外部（同时按钮加减也会经过这里）
    emit valueChanged(val);
}