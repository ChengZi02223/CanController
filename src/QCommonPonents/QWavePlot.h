#ifndef QWAVEPLOTWIDGET_H
#define QWAVEPLOTWIDGET_H
#include <QWidget>
#include <QPainter>
#include <QVector>
#include <QPen>

struct WaveDataPoint
{
    double t = 0.0;
    double val = 0.0;
    WaveDataPoint() = default;
    WaveDataPoint(double t_, double v_) : t(t_), val(v_) {}
};

struct WaveCurve
{
    QString name;
    QPen pen;
    bool useRightY{false};
    bool visible{true};
    QVector<WaveDataPoint> points;
};

class QWavePlotWidget : public QWidget
{
    Q_OBJECT
public:
    struct AxisConfig
    {
        QString xLabel = "时间(s)";
        QString leftYLabel = "流量";
        QString rightYLabel = "位移";
    };
    explicit QWavePlotWidget(QWidget *parent = nullptr);

    QVector<WaveDataPoint> getCurvePoints(int idx) const;

    void setupAxis(const AxisConfig& cfg);
    int addCurve(const QString& name, const QPen& pen, bool useRightY = false);
    void appendData(int curveIndex, double time, double value);
    void setMaxPointCount(int cnt);
    void setTimeWindow(double sec);
    void clearAll();
    void setLeftYRange(double min, double max);
    void setRightYRange(double min, double max);
    void setAutoY(bool enable);

    // 可见性接口，供外部QCheckBox调用
    void setCurveVisible(int curveIndex, bool visible);
    bool isCurveVisible(int curveIndex) const;
    void setAllCurveVisible(bool visible);
    int curveCount() const { return m_curves.size(); }
    QString curveName(int idx) const;
    QPen curvePen(int idx) const;

signals:
    // 当某条曲线第一次写入数据时触发
    void sigCurveFirstData(int curveIdx);
    
protected:
    void paintEvent(QPaintEvent *event) override;

private:
    AxisConfig m_axisCfg;
    QVector<WaveCurve> m_curves;
    int m_maxPoints = 3000;
    double m_timeWindow = 10.0;
    bool m_autoY = true;
    double m_leftYMin{-1.0}, m_leftYMax{1.0};
    double m_rightYMin{-1.0}, m_rightYMax{1.0};

    const int m_marginLeft = 70;
    const int m_marginRight = 70;
    const int m_marginTop = 30;
    const int m_marginBottom = 45; // 恢复原来边距，图例交给外部控件

    double timeToX(double t, double tMax, double plotWidth);
    double valueToY(double val, double yMin, double yMax, int plotHeight);
    void calcDataRange(double& tLatest,
                       double& leftMin, double& leftMax,
                       double& rightMin, double& rightMax);
    QVector<double> genTicks(double min, double max, int tickCnt);
};
#endif // QWAVEPLOTWIDGET_H
