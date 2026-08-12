#ifndef QWAVEPLOTWITHLEGENDWIDGET_H
#define QWAVEPLOTWITHLEGENDWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QLabel>
#include <QPen>
#include "QWavePlot.h"
#include "FlowLayout.h"

// 单条图例组合容器：CheckBox + 颜色线条 + 文本
struct LegendItem
{
    QWidget* groupWidget = nullptr; // 整组的容器
    QCheckBox* checkBox = nullptr;
    QLabel* lineLabel = nullptr; // 绘制彩色线段
    int curveIndex = -1;
};

class QWavePlotWithLegendWidget : public QWidget
{
    Q_OBJECT
public:
    explicit QWavePlotWithLegendWidget(QWidget *parent = nullptr);

    // 完全透传绘图控件所有接口
    void setupAxis(const QWavePlotWidget::AxisConfig& cfg);
    int addCurve(const QString& name, const QPen& pen, bool useRightY = false);
    void appendData(int curveIndex, double time, double value);
    void setMaxPointCount(int cnt);
    void setTimeWindow(double sec);
    void clearAll();
    void setLeftYRange(double min, double max);
    void setRightYRange(double min, double max);
    void setAutoY(bool enable);
    void setCurveVisible(int curveIndex, bool visible);
    bool isCurveVisible(int curveIndex) const;
    void setAllCurveVisible(bool visible);
    void tryRefreshLegend();

private:
    // 刷新图例：只保留有数据的曲线图例，无数据则销毁
    void refreshLegend();
    // 销毁全部图例组件
    void clearAllLegendItems();
    // 创建单条图例组合
    LegendItem createLegendItem(int curveIdx);

private:
    QVBoxLayout* m_mainLayout;
    QWavePlotWidget* m_plot;
    QWidget* m_legendContainer;
    FlowLayout* m_legendFlowLayout;
    QList<LegendItem> m_legendItems;
};

#endif // QWAVEPLOTWITHLEGENDWIDGET_H
