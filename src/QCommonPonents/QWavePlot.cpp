#include "QWavePlot.h"
#include <algorithm>
#include <QFont>

QWavePlotWidget::QWavePlotWidget(QWidget *parent)
    : QWidget(parent)
{
    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);
}

QVector<WaveDataPoint> QWavePlotWidget::getCurvePoints(int idx) const
{
    if (idx < 0 || idx >= m_curves.size())
        return {};
    return m_curves[idx].points;
}

void QWavePlotWidget::setupAxis(const QWavePlotWidget::AxisConfig &cfg)
{
    m_axisCfg = cfg;
    update();
}

int QWavePlotWidget::addCurve(const QString &name, const QPen &pen, bool useRightY)
{
    WaveCurve c;
    c.name = name;
    c.pen = pen;
    c.useRightY = useRightY;
    c.visible = true;
    m_curves.append(c);
    update();
    return m_curves.size()-1;
}

QString QWavePlotWidget::curveName(int idx) const
{
    if(idx <0 || idx >= m_curves.size()) return "";
    return m_curves[idx].name;
}

QPen QWavePlotWidget::curvePen(int idx) const
{
    if(idx <0 || idx >= m_curves.size()) return QPen(Qt::black);
    return m_curves[idx].pen;
}

void QWavePlotWidget::appendData(int curveIndex, double time, double value)
{
    if(curveIndex <0 || curveIndex >= m_curves.size()) return;
    auto& curve = m_curves[curveIndex];
    bool wasEmpty = curve.points.isEmpty();
    curve.points.push_back(WaveDataPoint(time, value));
    while(curve.points.size() > m_maxPoints)
    {
        curve.points.removeFirst();
    }
    if (wasEmpty && !curve.points.isEmpty())
    {
        emit sigCurveFirstData(curveIndex);
    }
    update();
}

void QWavePlotWidget::setMaxPointCount(int cnt)
{
    m_maxPoints = cnt;
}

void QWavePlotWidget::setTimeWindow(double sec)
{
    m_timeWindow = sec;
}

void QWavePlotWidget::clearAll()
{
    for(auto& c : m_curves)
    {
        c.points.clear();
    }
    update();
}

void QWavePlotWidget::setLeftYRange(double min, double max)
{
    m_autoY = false;
    m_leftYMin = min;
    m_leftYMax = max;
}

void QWavePlotWidget::setRightYRange(double min, double max)
{
    m_autoY = false;
    m_rightYMin = min;
    m_rightYMax = max;
}

void QWavePlotWidget::setAutoY(bool enable)
{
    m_autoY = enable;
}

void QWavePlotWidget::setCurveVisible(int curveIndex, bool visible)
{
    if(curveIndex >=0 && curveIndex < m_curves.size())
    {
        m_curves[curveIndex].visible = visible;
        update();
    }
}

bool QWavePlotWidget::isCurveVisible(int curveIndex) const
{
    if(curveIndex <0 || curveIndex >= m_curves.size())
        return false;
    return m_curves[curveIndex].visible;
}

void QWavePlotWidget::setAllCurveVisible(bool visible)
{
    for(auto& crv : m_curves)
        crv.visible = visible;
    update();
}

double QWavePlotWidget::timeToX(double t, double tMaxView, double plotWidth)
{
    if(tMaxView <= 1e-9) return m_marginLeft;
    return m_marginLeft + (t / tMaxView) * plotWidth;
}

double QWavePlotWidget::valueToY(double val, double yMin, double yMax, int plotHeight)
{
    if(yMax <= yMin) return m_marginTop;
    double ratio = (val - yMin) / (yMax - yMin);
    return m_marginTop + plotHeight * (1.0 - ratio);
}

QVector<double> QWavePlotWidget::genTicks(double min, double max, int tickCnt)
{
    QVector<double> res;
    if(tickCnt <= 1 || max <= min)
    {
        res << min << max;
        return res;
    }
    double step = (max - min) / (tickCnt -1);
    for(int i=0;i<tickCnt;i++)
    {
        res << min + i*step;
    }
    return res;
}

void QWavePlotWidget::calcDataRange(double &tLatest, double &leftMin, double &leftMax, double &rightMin, double &rightMax)
{
    tLatest = 0.0;
    leftMin = 1e20; leftMax = -1e20;
    rightMin =1e20; rightMax =-1e20;
    for(auto& crv : m_curves)
    {
        for(auto& p : crv.points)
        {
            tLatest = std::max(tLatest, p.t);
            if(crv.useRightY)
            {
                rightMin = std::min(rightMin, p.val);
                rightMax = std::max(rightMax, p.val);
            }
            else
            {
                leftMin = std::min(leftMin, p.val);
                leftMax = std::max(leftMax, p.val);
            }
        }
    }
}

void QWavePlotWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    QFont font = painter.font();
    font.setPointSize(9);
    painter.setFont(font);

    int w = width();
    int h = height();
    int plotW = w - m_marginLeft - m_marginRight;
    int plotH = h - m_marginTop - m_marginBottom;

    double tLatest;
    double leftMin, leftMax, rightMin, rightMax;
    calcDataRange(tLatest, leftMin, leftMax, rightMin, rightMax);

    double tViewMax = tLatest;
    if(tViewMax > m_timeWindow)
        tViewMax = m_timeWindow;

    if(m_autoY)
    {
        if(leftMin < leftMax)
        {
            m_leftYMin = leftMin;
            m_leftYMax = leftMax;
        }
        if(rightMin < rightMax)
        {
            m_rightYMin = rightMin;
            m_rightYMax = rightMax;
        }
    }

    painter.drawRect(m_marginLeft, m_marginTop, plotW, plotH);

    // X轴
    painter.setPen(QPen(Qt::black,1));
    auto xTicks = genTicks(0.0, tViewMax, 6);
    for(double tick : xTicks)
    {
        double x = timeToX(tick, tViewMax, plotW);
        painter.drawLine(QPointF(x, m_marginTop+plotH), QPointF(x, m_marginTop+plotH+6));
        painter.drawText(QRectF(x-25, m_marginTop+plotH+8,50,20), Qt::AlignHCenter, QString::number(tick, 'f',1));
    }
    painter.drawText(QRect(m_marginLeft, h - m_marginBottom + 5, plotW,30), Qt::AlignHCenter, m_axisCfg.xLabel);

    // 左Y
    auto leftYTicks = genTicks(m_leftYMin, m_leftYMax, 6);
    for(double tick : leftYTicks)
    {
        double y = valueToY(tick, m_leftYMin, m_leftYMax, plotH);
        painter.drawLine(QPointF(m_marginLeft-6, y), QPointF(m_marginLeft, y));
        painter.drawText(QRectF(0, y-10, m_marginLeft-8,20), Qt::AlignRight|Qt::AlignVCenter, QString::number(tick, 'f',2));
    }
    painter.drawText(QRect(2, m_marginTop, m_marginLeft-10, plotH), Qt::AlignVCenter|Qt::AlignRight, m_axisCfg.leftYLabel);

    // 右Y
    auto rightYTicks = genTicks(m_rightYMin, m_rightYMax,6);
    for(double tick : rightYTicks)
    {
        double y = valueToY(tick, m_rightYMin, m_rightYMax, plotH);
        painter.drawLine(QPointF(m_marginLeft+plotW, y), QPointF(m_marginLeft+plotW+6, y));
        painter.drawText(QRectF(w-m_marginRight+8, y-10, m_marginRight-10,20), Qt::AlignLeft|Qt::AlignVCenter, QString::number(tick, 'f',2));
    }
    painter.drawText(QRect(w-m_marginRight+8, m_marginTop, m_marginRight-10, plotH), Qt::AlignVCenter, m_axisCfg.rightYLabel);

    //网格
    painter.setPen(QPen(QColor(210,210,210),1,Qt::DotLine));
    for(double tick : xTicks)
    {
        double x = timeToX(tick, tViewMax, plotW);
        painter.drawLine(QPointF(x,m_marginTop), QPointF(x, m_marginTop+plotH));
    }
    for(double tick : leftYTicks)
    {
        double y = valueToY(tick, m_leftYMin, m_leftYMax, plotH);
        painter.drawLine(QPointF(m_marginLeft,y), QPointF(m_marginLeft+plotW, y));
    }

    //绘制曲线，跳过不可见
    for(auto& crv : m_curves)
    {
        if(!crv.visible || crv.points.size() <2) continue;
        painter.setPen(crv.pen);
        QPointF prevPt;
        bool first = true;
        for(auto& dp : crv.points)
        {
            if(dp.t < 0 || dp.t > tViewMax) continue;
            double x = timeToX(dp.t, tViewMax, plotW);
            double y;
            if(crv.useRightY)
                y = valueToY(dp.val, m_rightYMin, m_rightYMax, plotH);
            else
                y = valueToY(dp.val, m_leftYMin, m_leftYMax, plotH);

            QPointF curr(x,y);
            if(!first)
                painter.drawLine(prevPt, curr);
            prevPt = curr;
            first = false;
        }
    }
}
