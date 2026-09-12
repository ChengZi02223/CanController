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
    for(int idx=0; idx<m_curves.size(); idx++) {
        auto& curr_c = m_curves[idx];
        if(curr_c.side == side && curr_c.type == type){
            bool wasEmpty = curr_c.points.isEmpty();
            curr_c.points.push_back(WaveDataPoint(time, value));
            if (wasEmpty && !curr_c.points.isEmpty()) {
                emit sigCurveFirstData(idx); //正确：发射当前曲线idx
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
    font.setPointSize(8);
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

            // 数据最大值和 floor 取大者
            double wanted = std::max(leftMax, m_leftYMaxFloor);
            if (wanted > m_leftYMaxFloor + 1e-9)
            {
                // 数据超过初始下限：抬升 floor，并通知外部同步 UI
                m_leftYMaxFloor = wanted;
                emit sigLeftYMaxChanged(wanted);
            }
            m_leftYMax = wanted;
        }
        if(rightMin < rightMax)
        {
            m_rightYMin = rightMin;

            double wanted = std::max(rightMax, m_rightYMaxFloor);
            if (wanted > m_rightYMaxFloor + 1e-9)
            {
                m_rightYMaxFloor = wanted;
                emit sigRightYMaxChanged(wanted);
            }
            m_rightYMax = wanted;
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
    for(auto& crv : m_curves) {
        if(!crv.visible || crv.points.size() <2)
            continue;
        painter.setPen(crv.pen);

        const auto& pts = crv.points;
        for(int i = 0; i < pts.size()-1; ++i)
        {
            const WaveDataPoint& p0 = pts[i];
            const WaveDataPoint& p1 = pts[i+1];
            if (p0.isGap) continue;
            bool p0In = (p0.t >= viewMin - 1e-12) && (p0.t <= viewMax + 1e-12);
            bool p1In = (p1.t >= viewMin - 1e-12) && (p1.t <= viewMax + 1e-12);

            auto getPixel = [&](const WaveDataPoint& dp)->QPointF{
                double x = timeToX(dp.t, viewMin, viewMax, plotW);
                double y;
                if(crv.useRightY) {
                    y = valueToY(dp.val, m_rightYMin, m_rightYMax, plotH);
                } else {
                    y = valueToY(dp.val, m_leftYMin, m_leftYMax, plotH);
                }
                return QPointF(x,y);
            };

            // 线性插值求t=clipT处的val
            auto interpVal = [](double t0,double v0,double t1,double v1,double clipT)->double{
                double alpha = (clipT - t0)/(t1 - t0);
                return v0 + alpha*(v1-v0);
            };

            QPointF sPt,ePt;
            bool doDraw = false;

            if(p0In && p1In)
            {
                // 两点都在窗口内，直接绘制
                sPt = getPixel(p0);
                ePt = getPixel(p1);
                doDraw = true;
            }
            else if(p0In && !p1In)
            {
                // p0在窗口内，p1超出右边界viewMax，求与viewMax交点
                double clipT = viewMax;
                double clipVal = interpVal(p0.t,p0.val,p1.t,p1.val,clipT);
                WaveDataPoint clipPt(clipT, clipVal);
                sPt = getPixel(p0);
                ePt = getPixel(clipPt);
                doDraw = true;
            }
            else if(!p0In && p1In)
            {
                // p0在窗口左边外，p1进入窗口，求viewMin交点
                double clipT = viewMin;
                double clipVal = interpVal(p0.t,p0.val,p1.t,p1.val,clipT);
                WaveDataPoint clipPt(clipT, clipVal);
                sPt = getPixel(clipPt);
                ePt = getPixel(p1);
                doDraw = true;
            }
            // 两个都在外，doDraw=false，跳过

            if(doDraw){
                painter.drawLine(sPt, ePt);
            }
        }

        // --- 原代码末尾的“在最后一个点旁边输出数值”保留，逻辑不变 ---
        const WaveDataPoint& lastDp = crv.points.back();
        double dp_v = lastDp.val;
        double y;
        if(crv.useRightY) {
            y = valueToY(dp_v, m_rightYMin, m_rightYMax, plotH);
            painter.drawLine(QPointF(m_marginLeft+plotW,y), QPointF(m_marginLeft+plotW + 5, y));
            painter.drawText(QRectF(w-m_marginRight+8, y, m_marginRight-10,20), Qt::AlignLeft|Qt::AlignVCenter, QString::number(dp_v, 'f',2));
        } else {
            y = valueToY(dp_v, m_leftYMin, m_leftYMax, plotH);
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

void QWavePlotWidget::markGapAllCurves()
{
    for (auto& crv : m_curves)
    {
        if (!crv.points.isEmpty())
        {
            crv.points.back().isGap = true;
        }
    }
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
    int delta = event->angleDelta().y();
    if (delta == 0)
    {
        QWidget::wheelEvent(event);
        return;
    }

    // 先拿到当前最新时间，用于计算可滚动的上限
    double tLatest, lMin, lMax, rMin, rMax;
    calcDataRange(tLatest, lMin, lMax, rMin, rMax);

    // 允许的最大偏移：使得 viewMin = tLatest - offset - window >= 0
    double maxOffset = std::max(0.0, tLatest - m_timeWindow);

    m_followLatest = false;

    if (delta > 0)
    {
        m_viewTimeOffset += m_scrollStepSec;
        // 关键：到达最左侧后不再继续累加
        if (m_viewTimeOffset > maxOffset)
        {
            m_viewTimeOffset = maxOffset;
        }
    }
    else
    {
        m_viewTimeOffset -= m_scrollStepSec;
        if (m_viewTimeOffset <= 0.0)
        {
            m_viewTimeOffset = 0.0;
            m_followLatest = true;
        }
    }

    update();
    event->accept();
}

void QWavePlotWidget::setLeftYMaxFloor(double v)
{
    m_leftYMaxFloor = v;
    update();
}

void QWavePlotWidget::setRightYMaxFloor(double v)
{
    m_rightYMaxFloor = v;
    update();
}