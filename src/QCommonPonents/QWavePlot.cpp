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

void QWavePlotWidget::appendDataByType(WaveCurveType type, double time, double value) {
    for(int idx=0; idx<m_curves.size(); idx++) {
        auto& curr_c = m_curves[idx];
        if(curr_c.type != type) continue;
        bool wasEmpty = curr_c.points.isEmpty();
        curr_c.points.push_back(WaveDataPoint(time, value));
        while(curr_c.points.size() > m_maxPoints)
        {
            curr_c.points.removeFirst();
        }
        if (wasEmpty && !curr_c.points.isEmpty()) {
            emit sigCurveFirstData(idx);
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

// 轴标签 -> 曲线类型（顺序与 CalibrationPage 的 y_axis_labels 一致，即 WaveCurveType 枚举顺序）
static WaveCurveType LabelToCurveType(const QString& label) {
    static const QVector<QPair<QString, WaveCurveType>> kMap = {
        {"阀芯实时位移",     WaveCurveType::kRealDisp},
        {"位移闭环控制偏差", WaveCurveType::kCloseLoopErr},
        {"最终需求值",       WaveCurveType::kDemandVal},
        {"PWM输出占空比",    WaveCurveType::kPWMRatio},
        {"电磁铁实际电流",   WaveCurveType::kRealCurrent},
        {"电磁铁目标电流",   WaveCurveType::kTargetCurrent},
        {"开度",             WaveCurveType::kOpening},
    };
    for (const auto& p : kMap)
        if (p.first == label)
            return p.second;
    return WaveCurveType::kPWMRatio;  // 兜底：默认左轴类型
}

void QWavePlotWidget::setOnOpening(bool opening) {
    on_opening_ = opening;
    if (opening) {
        // 进入开度模式：只显示 kOpening 曲线，其余全部隐藏
        for (auto& c : m_curves)
            c.visible = (c.type == WaveCurveType::kOpening);
    } else {
        // 退出开度模式：按当前左右轴类型恢复显示
        const WaveCurveType left_type = LabelToCurveType(m_axisCfg.left_y);
        const WaveCurveType right_type = LabelToCurveType(m_axisCfg.right_y);
        for (auto& c : m_curves)
            c.visible = c.useRightY ? (c.type == right_type) : (c.type == left_type);
    }

    // 量程：左右轴无条件同步设置（开度可能从任一侧下拉框进入，另一侧轴虽隐藏，
    // 退出时也必须恢复，否则重新显示时量程残留 ±100）
    if(opening) {
        setLeftYRange(-100, 100);
        setRightYRange(-100, 100);
    } else {
        // 从开度切回普通模式：最小值恢复为0，最大值取上方输入框同步的 floor 值
        double l_target = m_leftYMaxFloor > 1e-9 ? m_leftYMaxFloor : m_leftYMax;
        setLeftYRange(0, l_target);
        double r_target = m_rightYMaxFloor > 1e-9 ? m_rightYMaxFloor : m_rightYMax;
        setRightYRange(0, r_target);
    }

    update();
}

void QWavePlotWidget::setYAxisVisible(int side, bool visible) {
    if(side == 1) { // 左侧
        left_y_visible_ = visible;
    } else if(side == 2) {
        right_y_visible_ = visible;
    }
}

void QWavePlotWidget::setTargetOpening1(double v) {
    m_targetOpening1 = v;
    update();
}

void QWavePlotWidget::setTargetOpening2(double v) {
    m_targetOpening2 = v;
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

QVector<double> QWavePlotWidget::genTicks(double min, double max, int tickCnt, bool y_axis)
{
    Q_UNUSED(y_axis);
    QVector<double> res;
    if(tickCnt <= 1 || max <= min)
    {
        res << min << max;
        return res;
    }
    double step = (max - min) / (tickCnt -1);
    for(int i=0;i<tickCnt;i++) {
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
    PaintCtx ctx;
    ctx.painter = &painter;
    ctx.w = width();
    ctx.h = height();
    ctx.plotW = ctx.w - m_marginLeft - m_marginRight;
    ctx.plotH = ctx.h - m_marginTop - m_marginBottom;

    double tLatest;
    double leftMin, leftMax, rightMin, rightMax;
    calcDataRange(tLatest, leftMin, leftMax, rightMin, rightMax);

    // 计算当前可视时间窗口
    getViewTimeRange(tLatest, ctx.viewMin, ctx.viewMax);

    updateAutoYRange(leftMin, leftMax, rightMin, rightMax);

    // 绘制外框
    painter.drawRect(m_marginLeft, m_marginTop, ctx.plotW, ctx.plotH);

    // ========= X轴（使用viewMin ~ viewMax） =========
    auto xTicks = genTicks(ctx.viewMin, ctx.viewMax, 6, false);
    drawXAxis(ctx, xTicks);
    
    // 左右Y轴（刻度、标签、旋转标题）
    const int y_tick_count = on_opening_ ? 9 : 6;
    auto leftYTicks = genTicks(m_leftYMin, m_leftYMax, y_tick_count);
    drawYAxisSide(ctx, m_leftYMin, m_leftYMax, y_tick_count, false);
    drawYAxisSide(ctx, m_rightYMin, m_rightYMax, y_tick_count, true);

    // 目标开度红色刻度标记（左/右轴，开度模式）
    drawOpeningMarkers(ctx);


    // 网格线
    drawGrid(ctx, xTicks, leftYTicks);

    // 绘制曲线（只渲染 [viewMin, viewMax] 区间内的点）
    drawCurves(ctx);
}

// ============ paintEvent 拆分出的绘制子函数 ============

void QWavePlotWidget::updateAutoYRange(double leftMin, double leftMax, double rightMin, double rightMax)
{
    if (!m_autoY)
        return;
    if (leftMin < leftMax)
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
    if (rightMin < rightMax)
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

void QWavePlotWidget::drawXAxis(PaintCtx& ctx, const QVector<double>& xTicks)
{
    QPainter& painter = *ctx.painter;
    painter.setPen(QPen(Qt::black, 1));
    int x_tick_h = m_marginTop + ctx.plotH;
    int x_axis_h = ctx.h - m_marginBottom;
    if (on_opening_) {
        // 开度模式：x轴移到绘图区中间
        x_tick_h = ctx.plotH / 2 + m_marginTop;
        x_axis_h = x_tick_h;
        painter.drawLine(QPointF(m_marginLeft, x_tick_h), QPointF(m_marginLeft + ctx.plotW, x_tick_h));
    }
    bool first_indx = true;
    for (double tick : xTicks)
    {
        double x = timeToX(tick, ctx.viewMin, ctx.viewMax, ctx.plotW);
        painter.drawLine(QPointF(x, x_tick_h), QPointF(x, x_tick_h + 6));
        double x_text_x = first_indx ? x - 10 : x - 25;
        if (m_axisCfg.bottom_x_unit == AxisUnit::kms) {
            painter.drawText(QRectF(x_text_x, x_tick_h + 8, 50, 20), Qt::AlignHCenter,
                             QString::number(static_cast<int>(tick * 1000)));  // ms
        } else {
            painter.drawText(QRectF(x_text_x, x_tick_h + 8, 50, 20), Qt::AlignHCenter,
                             QString::number(tick, 'f', 1));
        }
        first_indx = false;
    }
    painter.drawText(QRect(m_marginLeft, x_axis_h + 5, ctx.plotW, 30), Qt::AlignHCenter,
                     m_axisCfg.bottom_x + GetAxisUnit(m_axisCfg.bottom_x_unit));
}

void QWavePlotWidget::drawYAxisSide(PaintCtx& ctx, double yMin, double yMax, int tickCnt, bool right)
{
    const bool visible = right ? right_y_visible_ : left_y_visible_;
    if (!visible)
        return;

    QPainter& painter = *ctx.painter;
    const double axisX = right ? (m_marginLeft + ctx.plotW) : m_marginLeft;

    auto ticks = genTicks(yMin, yMax, tickCnt);
    for (double tick : ticks)
    {
        double y = valueToY(tick, yMin, yMax, ctx.plotH);
        if (right)
            painter.drawLine(QPointF(axisX, y), QPointF(axisX + 6, y));
        else
            painter.drawLine(QPointF(axisX - 6, y), QPointF(axisX, y));
        // on_opening_ 模式下 0 刻度压在中间 x 轴上，不显示标签
        if (on_opening_ && std::abs(tick) < 1e-9)
            continue;
        QString tick_text = on_opening_ ? QString::number(std::abs(tick))
                                        : QString::number(tick, 'f', 2);
        if (right)
            painter.drawText(QRectF(ctx.w - m_marginRight + 8, y - 10, m_marginRight - 10, 20),
                             Qt::AlignLeft|Qt::AlignVCenter, tick_text);
        else
            painter.drawText(QRectF(0, y - 10, m_marginLeft - 8, 20),
                             Qt::AlignRight|Qt::AlignVCenter, tick_text);
    }

    // 旋转的轴标题
    const QString title = right ? (m_axisCfg.right_y + GetAxisUnit(m_axisCfg.right_y_unit))
                                : (m_axisCfg.left_y + GetAxisUnit(m_axisCfg.left_y_unit));
    const QRect rect = right ? QRect(ctx.w - m_marginRight - 60, m_marginTop, m_marginRight + 90, ctx.plotH + 10)
                             : QRect(-25, m_marginTop, m_marginLeft + 90, ctx.plotH + 10);
    drawRotatedAxisLabel(painter, rect, title, right ? 90 : -90);
}

void QWavePlotWidget::drawRotatedAxisLabel(QPainter& painter, const QRect& rect, const QString& text, int angle)
{
    painter.save();
    // 1. 将坐标系原点移动到矩形中心
    painter.translate(rect.center());
    // 2. 旋转（-90 从上往下读；90 从下往上读）
    painter.rotate(angle);
    // 3. 移回原点
    painter.translate(-rect.center());
    // 在旋转后的坐标系绘制文本
    painter.drawText(rect, Qt::AlignCenter, text);
    painter.restore();
}

void QWavePlotWidget::drawOpeningMarkers(PaintCtx& ctx)
{
    if (!on_opening_)
        return;

    QPainter& painter = *ctx.painter;
    painter.save();
    painter.setPen(QPen(Qt::red, 2));
    // 限幅到绘图区内，避免标记画出外框
    auto clampY = [&](double y) { return qBound<double>(m_marginTop, y, m_marginTop + ctx.plotH); };

    if (left_y_visible_) {
        // 阀1目标开度：中线上方；阀2目标开度：中线下方（显示层取负）
        const double ly1 = clampY(valueToY(m_targetOpening1, m_leftYMin, m_leftYMax, ctx.plotH));
        drawOpeningMarker(painter, m_marginLeft, ly1, "阀1目标开度", false);
        const double ly2 = clampY(valueToY(-m_targetOpening2, m_leftYMin, m_leftYMax, ctx.plotH));
        drawOpeningMarker(painter, m_marginLeft, ly2, "阀2目标开度", false);
    }
    if (right_y_visible_) {
        const double axisX = m_marginLeft + ctx.plotW;
        const double ry1 = clampY(valueToY(m_targetOpening1, m_rightYMin, m_rightYMax, ctx.plotH));
        drawOpeningMarker(painter, axisX, ry1, "阀1目标开度", true);
        const double ry2 = clampY(valueToY(-m_targetOpening2, m_rightYMin, m_rightYMax, ctx.plotH));
        drawOpeningMarker(painter, axisX, ry2, "阀2目标开度", true);
    }
    painter.restore();
}

void QWavePlotWidget::drawOpeningMarker(QPainter& painter, double axisX, double y, const QString& text, bool right)
{
    // 刻度线从轴线向绘图区内部伸入
    const double dir = right ? -1.0 : 1.0;
    painter.drawLine(QPointF(axisX, y), QPointF(axisX + dir * 14, y));
    // 文字画在轴线内侧
    const QRectF text_rect = right ? QRectF(axisX - 16 - 90, y - 9, 90, 18)
                                   : QRectF(axisX + 16, y - 9, 90, 18);
    painter.drawText(text_rect, right ? Qt::AlignRight|Qt::AlignVCenter : Qt::AlignLeft|Qt::AlignVCenter, text);
}

void QWavePlotWidget::drawGrid(PaintCtx& ctx, const QVector<double>& xTicks, const QVector<double>& leftYTicks)
{
    QPainter& painter = *ctx.painter;
    painter.setPen(QPen(QColor(210, 210, 210), 1, Qt::DotLine));
    for (double tick : xTicks)
    {
        double x = timeToX(tick, ctx.viewMin, ctx.viewMax, ctx.plotW);
        painter.drawLine(QPointF(x, m_marginTop), QPointF(x, m_marginTop + ctx.plotH));
    }
    for (double tick : leftYTicks)
    {
        double y = valueToY(tick, m_leftYMin, m_leftYMax, ctx.plotH);
        painter.drawLine(QPointF(m_marginLeft, y), QPointF(m_marginLeft + ctx.plotW, y));
    }
}

void QWavePlotWidget::drawCurves(PaintCtx& ctx)
{
    for (const auto& crv : m_curves) {
        // on_opening_ 模式下只显示 kOpening 曲线
        if (!crv.visible || crv.points.size() < 2)
            continue;
        if (on_opening_ && crv.type != WaveCurveType::kOpening)
            continue;
        drawSingleCurve(ctx, crv);
    }
}

void QWavePlotWidget::drawSingleCurve(PaintCtx& ctx, const WaveCurve& crv)
{
    QPainter& painter = *ctx.painter;
    painter.setPen(crv.pen);

    auto getPixel = [&](const WaveDataPoint& dp)->QPointF{
        double x = timeToX(dp.t, ctx.viewMin, ctx.viewMax, ctx.plotW);
        double val = displayValue(crv, dp.val);
        double y = crv.useRightY ? valueToY(val, m_rightYMin, m_rightYMax, ctx.plotH)
                                 : valueToY(val, m_leftYMin, m_leftYMax, ctx.plotH);
        return QPointF(x, y);
    };

    // 线性插值求t=clipT处的val
    auto interpVal = [](double t0, double v0, double t1, double v1, double clipT)->double{
        double alpha = (clipT - t0) / (t1 - t0);
        return v0 + alpha * (v1 - v0);
    };

    const auto& pts = crv.points;
    for (int i = 0; i < pts.size() - 1; ++i)
    {
        const WaveDataPoint& p0 = pts[i];
        const WaveDataPoint& p1 = pts[i+1];
        if (p0.isGap) continue;
        const bool p0In = (p0.t >= ctx.viewMin - 1e-12) && (p0.t <= ctx.viewMax + 1e-12);
        const bool p1In = (p1.t >= ctx.viewMin - 1e-12) && (p1.t <= ctx.viewMax + 1e-12);

        QPointF sPt, ePt;
        bool doDraw = false;

        if (p0In && p1In)
        {
            // 两点都在窗口内，直接绘制
            sPt = getPixel(p0);
            ePt = getPixel(p1);
            doDraw = true;
        }
        else if (p0In && !p1In)
        {
            // p0在窗口内，p1超出右边界viewMax，求与viewMax交点
            double clipVal = interpVal(p0.t, p0.val, p1.t, p1.val, ctx.viewMax);
            sPt = getPixel(p0);
            ePt = getPixel(WaveDataPoint(ctx.viewMax, clipVal));
            doDraw = true;
        }
        else if (!p0In && p1In)
        {
            // p0在窗口左边外，p1进入窗口，求viewMin交点
            double clipVal = interpVal(p0.t, p0.val, p1.t, p1.val, ctx.viewMin);
            sPt = getPixel(WaveDataPoint(ctx.viewMin, clipVal));
            ePt = getPixel(p1);
            doDraw = true;
        }
        // 两个都在外，doDraw=false，跳过

        if (doDraw) {
            painter.drawLine(sPt, ePt);
        }
    }

    drawCurveLastValue(ctx, crv);
}

void QWavePlotWidget::drawCurveLastValue(PaintCtx& ctx, const WaveCurve& crv)
{
    QPainter& painter = *ctx.painter;
    const double dp_v = crv.points.back().val;
    // on_opening_ 模式下 2 侧开度显示在中线下方，但数值标注仍显示原始正值
    const double disp_v = displayValue(crv, dp_v);
    const QString val_text = on_opening_ ? QString::number(std::abs(dp_v), 'f', 2)
                                         : QString::number(dp_v, 'f', 2);
    double y;
    if (crv.useRightY) {
        y = valueToY(disp_v, m_rightYMin, m_rightYMax, ctx.plotH);
        painter.drawLine(QPointF(m_marginLeft + ctx.plotW, y), QPointF(m_marginLeft + ctx.plotW + 5, y));
        painter.drawText(QRectF(ctx.w - m_marginRight + 8, y, m_marginRight - 10, 20),
                         Qt::AlignLeft|Qt::AlignVCenter, val_text);
    } else {
        y = valueToY(disp_v, m_leftYMin, m_leftYMax, ctx.plotH);
        painter.drawLine(QPointF(m_marginLeft - 5, y), QPointF(m_marginLeft, y));
        painter.drawText(QRectF(0, y - 10, m_marginLeft - 8, 20),
                         Qt::AlignRight|Qt::AlignVCenter, val_text);
    }
}

double QWavePlotWidget::displayValue(const WaveCurve& crv, double val) const
{
    // on_opening_ 模式：1侧开度画在中线上方，2侧开度取负画在中线下方（仅显示层，不改原始数据）
    if (on_opening_ && crv.type == WaveCurveType::kOpening && crv.side == 2)
        return -val;
    return val;
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