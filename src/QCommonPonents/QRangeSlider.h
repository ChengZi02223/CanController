#ifndef _RANGE_SLIDER_H_
#define _RANGE_SLIDER_H_

#include <QWidget>
#include <QSlider>
#include <QPushButton>
#include <QHBoxLayout>

class QRangeSlider : public QWidget
{
    Q_OBJECT
public:
    explicit QRangeSlider(QWidget *parent = nullptr);

    // 获取当前数值
    int value() const;

public slots:
    // 设置数值（自动限制范围并同步控件）
    void setValue(int val);

signals:
    // 数值改变信号
    void valueChanged(int value);

private slots:
    void onLeftClicked();
    void onRightClicked();
    void onSliderMoved(int val);

private:
    QSlider    *m_slider;
    QPushButton *m_leftBtn;
    QPushButton *m_rightBtn;
};

#endif // _RANGE_SLIDER_H_