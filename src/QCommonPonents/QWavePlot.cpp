#include "QWavePlot.h"
#include <algorithm>
#include <QFont>
#include <QWheelEvent>
#include <QDebug>

inline QString GetAxisUnit(AxisUnit unit) {
    switch (unit)
    {
    case AxisUnit::kmA:
        return QString("(mA)");
    case AxisUnit::kmm:
        return QString("(mm)");
    case AxisUnit::kms:
        return QString("(ms)");
    case AxisUnit::ks:
        return QString("(s)");
    case AxisUnit::kp:
        return QString("(%)");
    default:
        return QString();
    }
}

QWavePlotWidget::QWavePlotWidget(QWidget *parent)
    : QWidget(parent), m_viewTimeOffset(0.0), m_followLatest(true)
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

void QWavePlotWidget::setupAxis(const AxisConfig &cfg)
{
    m_axisCfg = cfg;
    update();
}

void QWavePlotWidget::updateAxis(AxisName name, QString label, AxisUnit unit) {
    switch(name) {
        case AxisName::kLeftY:
            m_axisCfg.left_y = label;
            m_axisCfg.left_y_unit = unit;
            break;
        case AxisName::kRightY:
            m_axisCfg.right_y = label;
            m_axisCfg.right_y_unit = unit;
            break;
        case AxisName::kBottomX:
            m_axisCfg.bottom_x = label;
            m_axisCfg.bottom_x_unit = unit;
            break;  
        default:
            break;     
    }
    update();
}


// 废弃
int QWavePlotWidget::addCurve(const QString &name, const QPen &pen, bool useRightY, WaveCurveType type, bool visible)
{
    WaveCurve c;
    c.name = name;
    c.pen = pen;
    c.useRightY = useRightY;
    c.visible = visible;
    c.type = type;
    m_curves.append(c);
    update();
    return m_curves.size()-1;
}

int QWavePlotWidget::addCurve(WaveCurve curve) {
    m_curves.append(curve);
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

void QWavePlotWidget::appendData(int side, WaveCurveType type, double time, double value) {
    for(auto& curr_c : m_curves) {
        if(curr_c.side == side && curr_c.type == type){
            bool wasEmpty = curr_c.points.isEmpty();
            curr_c.points.push_back(WaveDataPoint(time, value));
            if (wasEmpty && !curr_c.points.isEmpty()) {
                emit sigCurveFirstData(1);
            }
        }
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
// 清空数据同时重置滚动偏移、恢复自动跟随
    m_viewTimeOffset = 0.0;
    m_followLatest = true;
    update();
}

void QWavePlotWidget::setLeftYRange(double min, double max)
{
    m_autoY = false;
    m_leftYMin = min;
    m_leftYMax = max;
    update();
}

void QWavePlotWidget::setRightYRange(double min, double max)
{
    m_autoY = false;
    m_rightYMin = min;
    m_rightYMax = max;
    update();
}

void QWavePlotWidget::setAutoY(bool enable)
{
    m_autoY = enable;
    update();
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

void QWavePlotWidget::showCurveType(bool use_right, WaveCurveType type) {
    for(auto& crv : m_curves) {
        if(crv.useRightY != use_right) {
            continue;
        }
        crv.visible = crv.type == type;
    }
    update();
}

// double QWavePlotWidget::timeToX(double t, double tMaxView, double plotWidth)
// {
//     if(tMaxView <= 1e-9) return m_marginLeft;
//     return m_marginLeft + (t / tMaxView) * plotWidth;
// }

double QWavePlotWidget::timeToX(double t, double tViewMin, double tViewMax, double plotWidth)
{
    double viewSpan = tViewMax - tViewMin;
    if (viewSpan <= 1e-9)
        return m_marginLeft;
    double rel = (t - tViewMin) / viewSpan;
    return m_marginLeft + rel * plotWidth;
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
        if(!crv.visible) continue;
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
    font.setPointSize(10);
    painter.setFont(font);
    int w = width();
    int h = height();
    int plotW = w - m_marginLeft - m_marginRight;
    int plotH = h - m_marginTop - m_marginBottom;

    double tLatest;
    double leftMin, leftMax, rightMin, rightMax;
    calcDataRange(tLatest, leftMin, leftMax, rightMin, rightMax);

    // ======== 新增：计算当前可视10秒窗口 ========
    double viewMin, viewMax;
    getViewTimeRange(tLatest, viewMin, viewMax);
    // ===========================================

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

    // 绘制外框
    painter.drawRect(m_marginLeft, m_marginTop, plotW, plotH);

    // ========= X轴（使用viewMin ~ viewMax） =========
    painter.setPen(QPen(Qt::black,1));
    auto xTicks = genTicks(viewMin, viewMax, 6);
    for(double tick : xTicks)
    {
        double x = timeToX(tick, viewMin, viewMax, plotW);
        painter.drawLine(QPointF(x, m_marginTop+plotH), QPointF(x, m_marginTop+plotH+6));
        if(m_axisCfg.bottom_x_unit == AxisUnit::kms){
            painter.drawText(QRectF(x-25, m_marginTop+plotH+8,50,20), Qt::AlignHCenter, QString::number(static_cast<int>(tick * 1000)));  // ms
        } else {
            painter.drawText(QRectF(x-25, m_marginTop+plotH+8,50,20), Qt::AlignHCenter, QString::number(tick, 'f',1));
        }  
    }
    painter.drawText(QRect(m_marginLeft, h - m_marginBottom + 5, plotW,30), Qt::AlignHCenter, m_axisCfg.bottom_x + GetAxisUnit(m_axisCfg.bottom_x_unit));
    
    // 左Y轴
    auto leftYTicks = genTicks(m_leftYMin, m_leftYMax, 6);
    for(double tick : leftYTicks)
    {
        double y = valueToY(tick, m_leftYMin, m_leftYMax, plotH);
        painter.drawLine(QPointF(m_marginLeft-6, y), QPointF(m_marginLeft, y));
        painter.drawText(QRectF(0, y-10, m_marginLeft-8,20), Qt::AlignRight|Qt::AlignVCenter, QString::number(tick, 'f',2));
    }
    QRect left_rect(-25, m_marginTop, m_marginLeft + 90, plotH + 10);
    painter.save();
    // 1. 将坐标系原点移动到矩形中心
    painter.translate(left_rect.center());
    // 2. 逆时针旋转90度，文字竖排（从上往下读）；-90则从下往上读
    painter.rotate(-90);
    // 3. 移回原点
    painter.translate(-left_rect.center());

    // 在旋转后的坐标系绘制文本
    painter.drawText(left_rect, Qt::AlignCenter, m_axisCfg.left_y + GetAxisUnit(m_axisCfg.left_y_unit));

    painter.restore();
    // painter.drawText(QRect(2, m_marginTop, m_marginLeft-10, plotH), Qt::AlignVCenter|Qt::AlignRight, m_axisCfg.left_y);

    // 右Y轴
    auto rightYTicks = genTicks(m_rightYMin, m_rightYMax,6);
    for(double tick : rightYTicks)
    {
        double y = valueToY(tick, m_rightYMin, m_rightYMax, plotH);
        painter.drawLine(QPointF(m_marginLeft+plotW, y), QPointF(m_marginLeft+plotW+6, y));
        painter.drawText(QRectF(w-m_marginRight+8, y-10, m_marginRight-10,20), Qt::AlignLeft|Qt::AlignVCenter, QString::number(tick, 'f',2));
    }
    // painter.drawText(QRect(w-m_marginRight+8, m_marginTop, m_marginRight-10, plotH), Qt::AlignVCenter, m_axisCfg.right_y);
    QRect right_rect(w - m_marginRight - 60, m_marginTop, m_marginRight+90, plotH + 10);
    painter.save();
    // 1. 将坐标系原点移动到矩形中心
    painter.translate(right_rect.center());
    // 2. 逆时针旋转90度，文字竖排（从上往下读）；-90则从下往上读
    painter.rotate(90);
    // 3. 移回原点
    painter.translate(-right_rect.center());

    // 在旋转后的坐标系绘制文本
    painter.drawText(right_rect, Qt::AlignCenter, m_axisCfg.right_y + GetAxisUnit(m_axisCfg.right_y_unit));

    painter.restore();

    // 网格线
    painter.setPen(QPen(QColor(210,210,210),1,Qt::DotLine));
    for(double tick : xTicks)
    {
        double x = timeToX(tick, viewMin, viewMax, plotW);
        painter.drawLine(QPointF(x,m_marginTop), QPointF(x, m_marginTop+plotH));
    }
    for(double tick : leftYTicks)
    {
        double y = valueToY(tick, m_leftYMin, m_leftYMax, plotH);
        painter.drawLine(QPointF(m_marginLeft,y), QPointF(m_marginLeft+plotW, y));
    }

    // 绘制曲线（只渲染 [viewMin, viewMax] 区间内的点）
    for(auto& crv : m_curves)
    {
        if(!crv.visible || crv.points.size() <2) continue;
        painter.setPen(crv.pen);
        QPointF prevPt;
        bool first = true;
        double dp_v, y;
        for(auto& dp : crv.points)
        {
            // 过滤不在当前可视窗口的点
            if(dp.t < viewMin || dp.t > viewMax)
                continue;

            double x = timeToX(dp.t, viewMin, viewMax, plotW);
            dp_v = dp.val;
            if(crv.useRightY) {
                y = valueToY(dp_v, m_rightYMin, m_rightYMax, plotH);
            } else {
                y = valueToY(dp_v, m_leftYMin, m_leftYMax, plotH);
            }
            QPointF curr(x,y);
            if(!first)
                painter.drawLine(prevPt, curr);
            prevPt = curr;
            first = false;
        }
        if(crv.useRightY) {
            painter.drawLine(QPointF(m_marginLeft+plotW,y), QPointF(m_marginLeft+plotW + 5, y));
            painter.drawText(QRectF(w-m_marginRight+8, y, m_marginRight-10,20), Qt::AlignLeft|Qt::AlignVCenter, QString::number(dp_v, 'f',2));
        } else {
            painter.drawLine(QPointF(m_marginLeft - 5 ,y), QPointF(m_marginLeft, y));
            painter.drawText(QRectF(0, y - 10, m_marginLeft-8,20), Qt::AlignRight|Qt::AlignVCenter, QString::number(dp_v, 'f',2));
        }        
    }
}


void QWavePlotWidget::resetViewFollowLatest()
{
    m_viewTimeOffset = 0.0;
    m_followLatest = true;
    update();
}


void QWavePlotWidget::getViewTimeRange(double tLatest, double &viewMin, double &viewMax)
{
    if (m_followLatest)
    {
        // 自动跟随：窗口永远是最新10秒
        viewMax = tLatest;
        viewMin = tLatest - m_timeWindow;
    }
    else
    {
        // 手动滚动偏移：窗口整体左移 m_viewTimeOffset 秒
        viewMax = tLatest - m_viewTimeOffset;
        viewMin = viewMax - m_timeWindow;
    }
    // 边界保护：最小时间不能小于0
    if (viewMin < 0.0)
    {
        viewMin = 0.0;
        viewMax = viewMin + m_timeWindow;
    }
}

void QWavePlotWidget::wheelEvent(QWheelEvent *event)
{
    // delta>0 滚轮向上(往左翻历史)；delta<0 滚轮向下(往右回最新)
    int delta = event->angleDelta().y();
    if (delta == 0)
    {
        QWidget::wheelEvent(event);
        return;
    }
    // 只要滚动，关闭自动跟随
    m_followLatest = false;

    if (delta > 0)
    {
        // 上滚：向左查看更早数据，偏移增加
        m_viewTimeOffset += m_scrollStepSec;
    }
    else
    {
        // 下滚：向右靠近最新，偏移减少
        m_viewTimeOffset -= m_scrollStepSec;
        // 偏移不能小于0，小于0说明已经回到最新区间
        if (m_viewTimeOffset <= 0.0)
        {
            m_viewTimeOffset = 0.0;
            m_followLatest = true; // 回到末尾恢复自动跟随
        }
    }
    update();
    event->accept();
}