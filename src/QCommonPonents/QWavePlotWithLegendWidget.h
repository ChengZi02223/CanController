#ifndef QWAVEPLOTWITHLEGENDWIDGET_H
#define QWAVEPLOTWITHLEGENDWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QLabel>
#include <QPen>
#include <QLineEdit>
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
    void setupAxis(const AxisConfig& cfg);
    void updateAxis(AxisName name, QString label, AxisUnit unit);
    int addCurve(const QString& name, const QPen& pen, bool useRightY, WaveCurveType type, bool visible);
    int addCurve(WaveCurve curve);
    void appendData(int curveIndex, double time, double value);
    void appendData(int side, WaveCurveType type, double time, double value);
    void appendDataByType(WaveCurveType type, double time, double value);
    void setMaxPointCount(int cnt);
    void setTimeWindow(double sec);
    void clearAll();
    void setLeftYRange(double min, double max);
    void setRightYRange(double min, double max);
    void setAutoY(bool enable);
    void setCurveVisible(int curveIndex, bool visible);
    bool isCurveVisible(int curveIndex) const;
    void setAllCurveVisible(bool visible);
    void showCurveType(bool use_right, WaveCurveType type);
    void tryRefreshLegend();
    void markGapAllCurves();
    void setLeftYMaxFloor(double maxClamp);
    void setRightYMaxFloor(double maxClamp);

    void setOnOpening(bool opening);
    void setYAxisVisible(int side, bool visible);
    // 目标开度标记透传（开度模式下左轴上/下半区红色刻度）
    void setTargetOpening1(double v);   // 阀1目标开度：上半区
    void setTargetOpening2(double v);   // 阀2目标开度：下半区
    double targetOpening1() const;
    double targetOpening2() const;
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
    QLineEdit *m_left_max_y;
    QLineEdit *m_right_max_y;
    QWidget* m_legendContainer;
    FlowLayout* m_legendFlowLayout;
    QList<LegendItem> m_legendItems;

};

#endif // QWAVEPLOTWITHLEGENDWIDGET_H
