#include "QPerfCurve.h"
#include <QScrollArea>
#include <QApplication>
#include <limits>

// =========================== PlotCanvas 实现 ===========================
PlotCanvas::PlotCanvas(QPerfCurve *parentCurve, QWidget *parent)
    : QWidget(parent), m_curve(parentCurve)
{
    setMinimumSize(500, 300);
    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);
}

void PlotCanvas::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 获取数据副本（避免在绘制过程中数据被修改）
    QVector<double> time = m_curve->getTimeData();
    QMap<QString, QVector<double>> curves = m_curve->getCurvesData();
    QMap<QString, bool> visible = m_curve->getCurvesVisible();
    QMap<QString, QColor> colors = m_curve->getCurvesColor();
    QMap<QString, AxisType> axes = m_curve->getCurvesAxis();

    if (time.isEmpty() || curves.isEmpty()) {
        painter.drawText(rect(), Qt::AlignCenter, "No data available");
        return;
    }

    // 检查时间数据是否有效
    bool timeValid = false;
    for (int i = 1; i < time.size(); ++i) {
        if (!qFuzzyCompare(time[i], time[i-1])) {
            timeValid = true;
            break;
        }
    }
    if (!timeValid && time.size() > 1) {
        painter.drawText(rect(), Qt::AlignCenter, "Invalid time data");
        return;
    }

    // 分别计算左轴和右轴可见曲线的数据范围
    double xMin = time.first(), xMax = time.last();
    double yLeftMin = std::numeric_limits<double>::max();
    double yLeftMax = -std::numeric_limits<double>::max();
    double yRightMin = std::numeric_limits<double>::max();
    double yRightMax = -std::numeric_limits<double>::max();
    bool hasLeftCurve = false, hasRightCurve = false;

    // 使用迭代器遍历，注意不要修改容器
    for (auto it = curves.constBegin(); it != curves.constEnd(); ++it) {
        const QString &name = it.key();
        if (!visible.value(name, false)) continue;
        const QVector<double> &data = it.value();
        if (data.size() != time.size()) continue;

        bool dataValid = false;
        for (double val : data) {
            if (std::isfinite(val)) {
                dataValid = true;
                break;
            }
        }
        if (!dataValid) continue;

        AxisType axis = axes.value(name, AxisType::Left);
        double *minPtr = (axis == AxisType::Left) ? &yLeftMin : &yRightMin;
        double *maxPtr = (axis == AxisType::Left) ? &yLeftMax : &yRightMax;
        bool *hasPtr = (axis == AxisType::Left) ? &hasLeftCurve : &hasRightCurve;
        *hasPtr = true;

        for (double val : data) {
            if (std::isfinite(val)) {
                if (val < *minPtr) *minPtr = val;
                if (val > *maxPtr) *maxPtr = val;
            }
        }
    }

    if (!hasLeftCurve && !hasRightCurve) {
        painter.drawText(rect(), Qt::AlignCenter, "No curve selected or invalid data");
        return;
    }

    // 为每个轴添加边距
    auto addMargin = [](double &minVal, double &maxVal) {
        double range = maxVal - minVal;
        if (range < 1e-10) {
            if (qFuzzyIsNull(range)) {
                minVal = -1.0;
                maxVal = 1.0;
            } else {
                double margin = range * 0.1;
                minVal -= margin;
                maxVal += margin;
            }
        } else {
            double margin = range * 0.1;
            minVal -= margin;
            maxVal += margin;
        }
    };
    
    if (hasLeftCurve) addMargin(yLeftMin, yLeftMax);
    if (hasRightCurve) addMargin(yRightMin, yRightMax);

    // 绘图区域
    const int leftMargin = 60, rightMargin = 70, topMargin = 20, bottomMargin = 40;
    QRect plotRect = rect().adjusted(leftMargin, topMargin, -rightMargin, -bottomMargin);
    if (plotRect.width() <= 0 || plotRect.height() <= 0) return;

    // 坐标转换函数
    auto dataToWidget = [&](double x, double y, AxisType axis) -> QPointF {
        double yMin = (axis == AxisType::Left) ? yLeftMin : yRightMin;
        double yMax = (axis == AxisType::Left) ? yLeftMax : yRightMax;
        
        double xRange = xMax - xMin;
        double yRange = yMax - yMin;
        
        if (qFuzzyIsNull(xRange) || qFuzzyIsNull(yRange)) {
            return QPointF(plotRect.left() + plotRect.width() / 2.0, 
                          plotRect.top() + plotRect.height() / 2.0);
        }
        
        double clampedX = qBound(xMin, x, xMax);
        double clampedY = qBound(yMin, y, yMax);
        
        double px = plotRect.left() + (clampedX - xMin) / xRange * plotRect.width();
        double py = plotRect.bottom() - (clampedY - yMin) / yRange * plotRect.height();
        
        px = qBound(plotRect.left() - 10.0, px, plotRect.right() + 10.0);
        py = qBound(plotRect.top() - 10.0, py, plotRect.bottom() + 10.0);
        
        return QPointF(px, py);
    };

    // 绘制网格
    painter.save();
    painter.setPen(QPen(Qt::lightGray, 1, Qt::DotLine));
    
    if (xMax > xMin) {
        for (int i = 0; i <= 5; ++i) {
            double x = xMin + i * (xMax - xMin) / 5.0;
            QPointF p1 = dataToWidget(x, yLeftMin, AxisType::Left);
            QPointF p2 = dataToWidget(x, yLeftMax, AxisType::Left);
            if (std::isfinite(p1.x()) && std::isfinite(p1.y()) && 
                std::isfinite(p2.x()) && std::isfinite(p2.y())) {
                painter.drawLine(p1, p2);
            }
        }
    }
    
    if (hasLeftCurve && yLeftMax > yLeftMin) {
        for (int i = 0; i <= 5; ++i) {
            double y = yLeftMin + i * (yLeftMax - yLeftMin) / 5.0;
            QPointF p1 = dataToWidget(xMin, y, AxisType::Left);
            QPointF p2 = dataToWidget(xMax, y, AxisType::Left);
            if (std::isfinite(p1.x()) && std::isfinite(p1.y()) && 
                std::isfinite(p2.x()) && std::isfinite(p2.y())) {
                painter.drawLine(p1, p2);
            }
        }
    }
    painter.restore();

    // 绘制坐标轴
    painter.setPen(Qt::black);
    
    QPointF axisXStart = dataToWidget(xMin, yLeftMin, AxisType::Left);
    QPointF axisXEnd = dataToWidget(xMax, yLeftMin, AxisType::Left);
    if (std::isfinite(axisXStart.x()) && std::isfinite(axisXStart.y()) && 
        std::isfinite(axisXEnd.x()) && std::isfinite(axisXEnd.y())) {
        painter.drawLine(axisXStart, axisXEnd);
    }
    
    QPointF axisYLeftStart = dataToWidget(xMin, yLeftMin, AxisType::Left);
    QPointF axisYLeftEnd = dataToWidget(xMin, yLeftMax, AxisType::Left);
    if (std::isfinite(axisYLeftStart.x()) && std::isfinite(axisYLeftStart.y()) && 
        std::isfinite(axisYLeftEnd.x()) && std::isfinite(axisYLeftEnd.y())) {
        painter.drawLine(axisYLeftStart, axisYLeftEnd);
    }
    
    if (hasRightCurve) {
        QPointF axisYRightStart = dataToWidget(xMax, yRightMin, AxisType::Right);
        QPointF axisYRightEnd = dataToWidget(xMax, yRightMax, AxisType::Right);
        if (std::isfinite(axisYRightStart.x()) && std::isfinite(axisYRightStart.y()) && 
            std::isfinite(axisYRightEnd.x()) && std::isfinite(axisYRightEnd.y())) {
            painter.drawLine(axisYRightStart, axisYRightEnd);
        }
    }

    // 刻度标签
    QFontMetrics fm(painter.font());

    if (xMax > xMin) {
        for (int i = 0; i <= 5; ++i) {
            double x = xMin + i * (xMax - xMin) / 5.0;
            QPointF p = dataToWidget(x, yLeftMin, AxisType::Left);
            if (std::isfinite(p.x()) && std::isfinite(p.y())) {
                QString label = QString::number(x, 'f', 1);
                QRect rect(p.x() - 20, p.y() + 2, 40, fm.height());
                painter.drawText(rect, Qt::AlignHCenter | Qt::AlignTop, label);
            }
        }
    }
    painter.drawText(rect().left() + leftMargin, rect().bottom() - 5, "Time (s)");

    if (hasLeftCurve && yLeftMax > yLeftMin) {
        for (int i = 0; i <= 5; ++i) {
            double y = yLeftMin + i * (yLeftMax - yLeftMin) / 5.0;
            QPointF p = dataToWidget(xMin, y, AxisType::Left);
            if (std::isfinite(p.x()) && std::isfinite(p.y())) {
                QString label = QString::number(y, 'f', 1);
                QRect rect(p.x() - 45, p.y() - 8, 40, fm.height());
                painter.drawText(rect, Qt::AlignRight | Qt::AlignVCenter, label);
            }
        }
        painter.save();
        painter.rotate(-90);
        painter.drawText(-(plotRect.top() + plotRect.bottom()) / 2, 25, "Left Axis");
        painter.restore();
    }

    if (hasRightCurve && yRightMax > yRightMin) {
        for (int i = 0; i <= 5; ++i) {
            double y = yRightMin + i * (yRightMax - yRightMin) / 5.0;
            QPointF p = dataToWidget(xMax, y, AxisType::Right);
            if (std::isfinite(p.x()) && std::isfinite(p.y())) {
                QString label = QString::number(y, 'f', 1);
                QRect rect(p.x() + 5, p.y() - 8, 50, fm.height());
                painter.drawText(rect, Qt::AlignLeft | Qt::AlignVCenter, label);
            }
        }
        painter.save();
        painter.rotate(90);
        painter.drawText((plotRect.top() + plotRect.bottom()) / 2, -rect().right() + 15, "Right Axis");
        painter.restore();
    }

    // 绘制每条可见曲线
    for (auto it = curves.constBegin(); it != curves.constEnd(); ++it) {
        const QString &name = it.key();
        if (!visible.value(name, false)) continue;
        const QVector<double> &data = it.value();
        if (data.size() != time.size()) continue;

        bool hasValidData = false;
        for (int i = 0; i < time.size(); ++i) {
            if (std::isfinite(time[i]) && std::isfinite(data[i])) {
                hasValidData = true;
                break;
            }
        }
        if (!hasValidData) continue;

        AxisType axis = axes.value(name, AxisType::Left);
        QColor color = colors.value(name, Qt::black);
        painter.setPen(QPen(color, 2));
        QPainterPath path;
        bool firstPoint = true;
        
        for (int i = 0; i < time.size(); ++i) {
            if (!std::isfinite(time[i]) || !std::isfinite(data[i])) {
                firstPoint = true;
                continue;
            }
            
            QPointF pt = dataToWidget(time[i], data[i], axis);
            if (!std::isfinite(pt.x()) || !std::isfinite(pt.y())) {
                firstPoint = true;
                continue;
            }
            
            if (firstPoint) {
                path.moveTo(pt);
                firstPoint = false;
            } else {
                path.lineTo(pt);
            }
        }
        
        if (!path.isEmpty()) {
            painter.drawPath(path);
        }

        if (time.size() <= 200) {
            painter.setBrush(color);
            for (int i = 0; i < time.size(); ++i) {
                if (!std::isfinite(time[i]) || !std::isfinite(data[i])) continue;
                QPointF pt = dataToWidget(time[i], data[i], axis);
                if (std::isfinite(pt.x()) && std::isfinite(pt.y())) {
                    painter.drawEllipse(pt, 3, 3);
                }
            }
        }
    }
}

// =========================== QPerfCurve 实现 ===========================
QPerfCurve::QPerfCurve(QWidget *parent)
    : QWidget(parent), 
      m_nextColorIndex(0),
      m_maxDataPoints(0),
      m_autoRefreshEnabled(false)
{
    m_colorPalette = { Qt::red, Qt::green, Qt::blue, Qt::cyan, Qt::magenta,
                       Qt::darkYellow, Qt::darkCyan, Qt::darkMagenta };

    m_mainLayout = new QVBoxLayout(this);
    m_plotCanvas = new PlotCanvas(this, this);
    m_controlPanel = new QWidget(this);
    m_controlLayout = new QHBoxLayout(m_controlPanel);
    m_controlLayout->setContentsMargins(5, 5, 5, 5);
    m_controlLayout->setSpacing(30);
    m_controlPanel->setLayout(m_controlLayout);
    m_controlPanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_controlPanel->setMaximumHeight(60);

    m_mainLayout->addWidget(m_plotCanvas, 1);
    m_mainLayout->addWidget(m_controlPanel, 0);

    setLayout(m_mainLayout);

    m_autoRefreshTimer = new QTimer(this);
    connect(m_autoRefreshTimer, &QTimer::timeout, this, &QPerfCurve::onAutoRefresh);
    m_autoRefreshTimer->setInterval(100);
}

QPerfCurve::~QPerfCurve() 
{
    // 停止定时器
    if (m_autoRefreshTimer) {
        m_autoRefreshTimer->stop();
    }
}

void QPerfCurve::setTimeData(const QVector<double> &time)
{
    QMutexLocker locker(&m_dataMutex);
    m_timeData = time;
    refreshPlot();
}

void QPerfCurve::addCurve(const QString &name, const QVector<double> &data,
                      AxisType axis, const QColor &color)
{
    QMutexLocker locker(&m_dataMutex);
    
    if (!m_timeData.isEmpty() && data.size() != m_timeData.size()) {
        qWarning() << "Data size mismatch for curve" << name 
                   << "data size:" << data.size() 
                   << "time size:" << m_timeData.size();
        return;
    }
    
    m_curvesData[name] = data;
    m_curvesVisible[name] = true;
    m_curvesAxis[name] = axis;
    
    if (color.isValid()) {
        m_curvesColor[name] = color;
    } else {
        QColor autoColor = m_colorPalette[m_nextColorIndex % m_colorPalette.size()];
        m_curvesColor[name] = autoColor;
        m_nextColorIndex++;
    }
    
    rebuildControlPanel();
    refreshPlot();
}

void QPerfCurve::removeCurve(const QString &name)
{
    QMutexLocker locker(&m_dataMutex);
    m_curvesData.remove(name);
    m_curvesVisible.remove(name);
    m_curvesColor.remove(name);
    m_curvesAxis.remove(name);
    rebuildControlPanel();
    refreshPlot();
}

void QPerfCurve::setCurveVisible(const QString &name, bool visible)
{
    QMutexLocker locker(&m_dataMutex);
    if (m_curvesVisible.contains(name)) {
        m_curvesVisible[name] = visible;
        if (m_curveControls.contains(name)) {
            m_curveControls[name].checkBox->setChecked(visible);
        }
        refreshPlot();
    }
}

void QPerfCurve::setCurveAxis(const QString &name, AxisType axis)
{
    QMutexLocker locker(&m_dataMutex);
    if (m_curvesAxis.contains(name)) {
        m_curvesAxis[name] = axis;
        if (m_curveControls.contains(name)) {
            QPushButton *btn = m_curveControls[name].axisButton;
            btn->setText(axis == AxisType::Left ? "L" : "R");
        }
        refreshPlot();
    }
}

void QPerfCurve::clearCurves()
{
    QMutexLocker locker(&m_dataMutex);
    m_curvesData.clear();
    m_curvesVisible.clear();
    m_curvesColor.clear();
    m_curvesAxis.clear();
    rebuildControlPanel();
    refreshPlot();
}

// ===== 核心修复：动态数据追加 =====

void QPerfCurve::appendDataPoint(double time, const QMap<QString, double> &dataPoints)
{
    // 验证输入
    if (!std::isfinite(time)) {
        qWarning() << "Invalid time value:" << time;
        return;
    }
    
    for (auto it = dataPoints.constBegin(); it != dataPoints.constEnd(); ++it) {
        if (!std::isfinite(it.value())) {
            qWarning() << "Invalid data for curve" << it.key() << ":" << it.value();
            return;
        }
    }

    QMutexLocker locker(&m_dataMutex);

    if (m_curvesData.isEmpty()) {
        qWarning() << "No curves defined. Cannot append data.";
        return;
    }

    // 检查是否所有曲线都有数据
    for (auto it = m_curvesData.constBegin(); it != m_curvesData.constEnd(); ++it) {
        if (!dataPoints.contains(it.key())) {
            qWarning() << "Missing data for curve:" << it.key();
            return;
        }
    }

    // 使用 QMap 的迭代器进行追加，避免迭代器失效
    // 先获取所有曲线名称
    QStringList curveNames = m_curvesData.keys();
    
    // 追加时间
    m_timeData.append(time);
    
    // 逐条追加数据
    for (const QString &name : curveNames) {
        m_curvesData[name].append(dataPoints.value(name));
    }

    // 裁剪数据
    trimDataIfNeeded();

    // 刷新绘图（在锁外执行）
    locker.unlock();
    refreshPlot();
    
    emit dataAppended(m_timeData.size());
}

void QPerfCurve::appendDataPoints(const QVector<double> &time, const QMap<QString, QVector<double>> &dataPoints)
{
    // 验证输入
    for (double t : time) {
        if (!std::isfinite(t)) {
            qWarning() << "Invalid time value:" << t;
            return;
        }
    }

    QMutexLocker locker(&m_dataMutex);

    if (m_curvesData.isEmpty()) {
        qWarning() << "No curves defined. Cannot append data.";
        return;
    }

    if (time.isEmpty()) {
        return;
    }

    // 检查数据有效性
    QStringList curveNames = m_curvesData.keys();
    for (const QString &name : curveNames) {
        if (!dataPoints.contains(name)) {
            qWarning() << "Missing data for curve:" << name;
            return;
        }
        if (dataPoints[name].size() != time.size()) {
            qWarning() << "Data size mismatch for curve:" << name;
            return;
        }
        for (double val : dataPoints[name]) {
            if (!std::isfinite(val)) {
                qWarning() << "Invalid data for curve" << name << ":" << val;
                return;
            }
        }
    }

    // 追加时间
    m_timeData.append(time);
    
    // 逐条追加数据
    for (const QString &name : curveNames) {
        m_curvesData[name].append(dataPoints.value(name));
    }

    // 裁剪数据
    trimDataIfNeeded();

    // 刷新绘图（在锁外执行）
    locker.unlock();
    refreshPlot();
    
    emit dataAppended(m_timeData.size());
}

void QPerfCurve::setMaxDataPoints(int maxPoints)
{
    QMutexLocker locker(&m_dataMutex);
    m_maxDataPoints = maxPoints;
    if (m_maxDataPoints > 0) {
        trimDataIfNeeded();
        refreshPlot();
    }
}

void QPerfCurve::clearAllData()
{
    QMutexLocker locker(&m_dataMutex);
    m_timeData.clear();
    for (auto it = m_curvesData.begin(); it != m_curvesData.end(); ++it) {
        it.value().clear();
    }
    refreshPlot();
}

void QPerfCurve::setAutoRefreshInterval(int intervalMs)
{
    if (m_autoRefreshTimer) {
        m_autoRefreshTimer->setInterval(intervalMs);
    }
}

void QPerfCurve::setAutoRefreshEnabled(bool enabled)
{
    m_autoRefreshEnabled = enabled;
    if (m_autoRefreshTimer) {
        if (enabled) {
            m_autoRefreshTimer->start();
        } else {
            m_autoRefreshTimer->stop();
        }
    }
}

void QPerfCurve::onAutoRefresh()
{
    if (m_autoRefreshEnabled) {
        refreshPlot();
    }
}

void QPerfCurve::trimDataIfNeeded()
{
    if (m_maxDataPoints <= 0 || m_timeData.size() <= m_maxDataPoints) {
        return;
    }

    int excess = m_timeData.size() - m_maxDataPoints;
    
    // 移除最早的数据点
    m_timeData.remove(0, excess);
    
    // 对所有曲线移除最早的数据点
    for (auto it = m_curvesData.begin(); it != m_curvesData.end(); ++it) {
        it.value().remove(0, excess);
    }

    emit maxDataPointsReached();
}

bool QPerfCurve::validateData() const
{
    QMutexLocker locker(&m_dataMutex);
    
    for (double t : m_timeData) {
        if (!std::isfinite(t)) {
            return false;
        }
    }
    
    for (auto it = m_curvesData.constBegin(); it != m_curvesData.constEnd(); ++it) {
        for (double val : it.value()) {
            if (!std::isfinite(val)) {
                return false;
            }
        }
    }
    
    return true;
}

// ===== 原有函数 =====

void QPerfCurve::onCurveVisibilityChanged(const QString &name, bool visible)
{
    QMutexLocker locker(&m_dataMutex);
    if (m_curvesVisible.contains(name)) {
        m_curvesVisible[name] = visible;
        refreshPlot();
    }
}

void QPerfCurve::onCurveAxisSwitched(const QString &name)
{
    if (!m_curvesAxis.contains(name)) return;
    AxisType newAxis = (m_curvesAxis[name] == AxisType::Left) ? AxisType::Right : AxisType::Left;
    setCurveAxis(name, newAxis);
}

void QPerfCurve::onCurveContextMenu(const QPoint &pos, const QString &name)
{
    QMenu menu;
    QAction *actLeft = menu.addAction("Bind to Left Axis");
    QAction *actRight = menu.addAction("Bind to Right Axis");
    QAction *selected = menu.exec(pos);
    if (selected == actLeft) {
        setCurveAxis(name, AxisType::Left);
    } else if (selected == actRight) {
        setCurveAxis(name, AxisType::Right);
    }
}

void QPerfCurve::rebuildControlPanel()
{
    // 清除现有控件
    for (auto &control : m_curveControls) {
        if (control.container) {
            delete control.container;
        }
    }
    m_curveControls.clear();

    QLayoutItem *child;
    while ((child = m_controlLayout->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }

    for (auto it = m_curvesData.constBegin(); it != m_curvesData.constEnd(); ++it) {
        const QString &name = it.key();
        QColor color = m_curvesColor[name];
        AxisType axis = m_curvesAxis[name];

        QLabel *colorLabel = new QLabel;
        QPixmap pix(16, 16);
        pix.fill(color);
        colorLabel->setPixmap(pix);
        colorLabel->setFixedSize(16, 16);
        colorLabel->setToolTip(name);

        QCheckBox *checkBox = new QCheckBox(name);
        checkBox->setChecked(m_curvesVisible[name]);
        QPushButton *axisBtn = new QPushButton(axis == AxisType::Left ? "L" : "R");
        axisBtn->setFixedSize(24, 24);
        axisBtn->setToolTip("Switch axis (Left/Right)");

        QWidget *container = new QWidget;
        QHBoxLayout *layout = new QHBoxLayout(container);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(4);
        layout->addWidget(checkBox);
        layout->addWidget(colorLabel);
        layout->addWidget(axisBtn);
        container->setLayout(layout);
        container->setContextMenuPolicy(Qt::CustomContextMenu);
        container->setProperty("curveName", name);

        connect(checkBox, &QCheckBox::toggled, this, [this, name](bool checked) {
            onCurveVisibilityChanged(name, checked);
        });
        connect(axisBtn, &QPushButton::clicked, this, [this, name]() {
            onCurveAxisSwitched(name);
        });
        connect(container, &QWidget::customContextMenuRequested, this, [this, name](const QPoint &pos) {
            QPoint globalPos = dynamic_cast<QWidget*>(sender())->mapToGlobal(pos);
            onCurveContextMenu(globalPos, name);
        });

        m_controlLayout->addWidget(container);
        m_curveControls[name] = {container, checkBox, axisBtn, colorLabel};
    }

    m_controlLayout->addStretch();
    m_controlPanel->update();
}

void QPerfCurve::refreshPlot()
{
    if (m_plotCanvas) {
        m_plotCanvas->update();
    }
}