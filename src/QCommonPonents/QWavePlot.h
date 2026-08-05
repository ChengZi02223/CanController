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

    void setupAxis(const AxisConfig& cfg);

    int addCurve(const QString& name, const QPen& pen, bool useRightY);

    void appendData(int curveIndex, double time, double value);

    void setMaxPointCount(int cnt);

    void setTimeWindow(double sec);

    void clearAll();

    void setLeftYRange(double min, double max);
    void setRightYRange(double min, double max);
    void setAutoY(bool enable);

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
    const int m_marginBottom = 45;

    // tMin固定=0，tMax = min(最新时间, m_timeWindow)；超过窗口后tMax=m_timeWindow，画面向左滚
    double timeToX(double t, double tMax, double plotWidth);
    double valueToY(double val, double yMin, double yMax, int plotHeight);

    void calcDataRange(double& tLatest,
                       double& leftMin, double& leftMax,
                       double& rightMin, double& rightMax);

    QVector<double> genTicks(double min, double max, int tickCnt);
};

#endif // QWAVEPLOTWIDGET_H