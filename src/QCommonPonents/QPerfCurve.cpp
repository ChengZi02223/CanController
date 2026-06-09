#include "QPerfCurve.h"

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

    QVector<double> time = m_curve->getTimeData();
    QMap<QString, QVector<double>> curves = m_curve->getCurvesData();
    QMap<QString, bool> visible = m_curve->getCurvesVisible();
    QMap<QString, QColor> colors = m_curve->getCurvesColor();
    QMap<QString, AxisType> axes = m_curve->getCurvesAxis();

    if (time.isEmpty() || curves.isEmpty()) {
        painter.drawText(rect(), Qt::AlignCenter, "No data available");
        return;
    }

    // 分别计算左轴和右轴可见曲线的数据范围
    double xMin = time.first(), xMax = time.last();
    double yLeftMin = std::numeric_limits<double>::max();
    double yLeftMax = -std::numeric_limits<double>::max();
    double yRightMin = std::numeric_limits<double>::max();
    double yRightMax = -std::numeric_limits<double>::max();
    bool hasLeftCurve = false, hasRightCurve = false;

    for (auto it = curves.begin(); it != curves.end(); ++it) {
        const QString &name = it.key();
        if (!visible.value(name, false)) continue;
        const QVector<double> &data = it.value();
        if (data.size() != time.size()) continue;

        AxisType axis = axes.value(name, AxisType::Left);
        double *minPtr = (axis == AxisType::Left) ? &yLeftMin : &yRightMin;
        double *maxPtr = (axis == AxisType::Left) ? &yLeftMax : &yRightMax;
        bool *hasPtr = (axis == AxisType::Left) ? &hasLeftCurve : &hasRightCurve;
        *hasPtr = true;

        for (double val : data) {
            if (val < *minPtr) *minPtr = val;
            if (val > *maxPtr) *maxPtr = val;
        }
    }

    if (!hasLeftCurve && !hasRightCurve) {
        painter.drawText(rect(), Qt::AlignCenter, "No curve selected");
        return;
    }

    // 为每个轴添加边距 (10%)
    auto addMargin = [](double &minVal, double &maxVal) {
        double margin = (maxVal - minVal) * 0.1;
        if (qFuzzyIsNull(margin)) margin = 1.0;
        minVal -= margin;
        maxVal += margin;
    };
    if (hasLeftCurve) addMargin(yLeftMin, yLeftMax);
    if (hasRightCurve) addMargin(yRightMin, yRightMax);

    // 绘图区域（预留轴和标签空间）
    const int leftMargin = 60, rightMargin = 70, topMargin = 20, bottomMargin = 40;
    QRect plotRect = rect().adjusted(leftMargin, topMargin, -rightMargin, -bottomMargin);
    if (plotRect.width() <= 0 || plotRect.height() <= 0) return;

    // 坐标转换
    auto dataToWidget = [&](double x, double y, AxisType axis) -> QPointF {
        double yMin = (axis == AxisType::Left) ? yLeftMin : yRightMin;
        double yMax = (axis == AxisType::Left) ? yLeftMax : yRightMax;
        double px = plotRect.left() + (x - xMin) / (xMax - xMin) * plotRect.width();
        double py = plotRect.bottom() - (y - yMin) / (yMax - yMin) * plotRect.height();
        return QPointF(px, py);
    };

    // 绘制网格
    painter.save();
    painter.setPen(QPen(Qt::lightGray, 1, Qt::DotLine));
    // 垂直网格
    for (int i = 0; i <= 5; ++i) {
        double x = xMin + i * (xMax - xMin) / 5.0;
        QPointF p1 = dataToWidget(x, yLeftMin, AxisType::Left);
        QPointF p2 = dataToWidget(x, yLeftMax, AxisType::Left);
        painter.drawLine(p1, p2);
    }
    // 水平网格（基于左轴）
    if (hasLeftCurve) {
        for (int i = 0; i <= 5; ++i) {
            double y = yLeftMin + i * (yLeftMax - yLeftMin) / 5.0;
            QPointF p1 = dataToWidget(xMin, y, AxisType::Left);
            QPointF p2 = dataToWidget(xMax, y, AxisType::Left);
            painter.drawLine(p1, p2);
        }
    }
    painter.restore();

    // 绘制坐标轴
    painter.setPen(Qt::black);
    // X 轴
    QPointF axisXStart = dataToWidget(xMin, yLeftMin, AxisType::Left);
    QPointF axisXEnd = dataToWidget(xMax, yLeftMin, AxisType::Left);
    painter.drawLine(axisXStart, axisXEnd);
    // 左 Y 轴
    QPointF axisYLeftStart = dataToWidget(xMin, yLeftMin, AxisType::Left);
    QPointF axisYLeftEnd = dataToWidget(xMin, yLeftMax, AxisType::Left);
    painter.drawLine(axisYLeftStart, axisYLeftEnd);
    // 右 Y 轴
    if (hasRightCurve) {
        QPointF axisYRightStart = dataToWidget(xMax, yRightMin, AxisType::Right);
        QPointF axisYRightEnd = dataToWidget(xMax, yRightMax, AxisType::Right);
        painter.drawLine(axisYRightStart, axisYRightEnd);
    }

    // 刻度标签
    QFontMetrics fm(painter.font());

    // X 轴
    for (int i = 0; i <= 5; ++i) {
        double x = xMin + i * (xMax - xMin) / 5.0;
        QPointF p = dataToWidget(x, yLeftMin, AxisType::Left);
        QString label = QString::number(x, 'f', 1);
        QRect rect(p.x() - 20, p.y() + 2, 40, fm.height());
        painter.drawText(rect, Qt::AlignHCenter | Qt::AlignTop, label);
    }
    painter.drawText(rect().left() + leftMargin, rect().bottom() - 5, "Time (s)");

    // 左 Y 轴
    if (hasLeftCurve) {
        for (int i = 0; i <= 5; ++i) {
            double y = yLeftMin + i * (yLeftMax - yLeftMin) / 5.0;
            QPointF p = dataToWidget(xMin, y, AxisType::Left);
            QString label = QString::number(y, 'f', 1);
            QRect rect(p.x() - 45, p.y() - 8, 40, fm.height());
            painter.drawText(rect, Qt::AlignRight | Qt::AlignVCenter, label);
        }
        painter.save();
        painter.rotate(-90);
        painter.drawText(-(plotRect.top() + plotRect.bottom()) / 2, 25, "Left Axis");
        painter.restore();
    }

    // 右 Y 轴
    if (hasRightCurve) {
        for (int i = 0; i <= 5; ++i) {
            double y = yRightMin + i * (yRightMax - yRightMin) / 5.0;
            QPointF p = dataToWidget(xMax, y, AxisType::Right);
            QString label = QString::number(y, 'f', 1);
            QRect rect(p.x() + 5, p.y() - 8, 50, fm.height());
            painter.drawText(rect, Qt::AlignLeft | Qt::AlignVCenter, label);
        }
        painter.save();
        painter.rotate(90);
        painter.drawText((plotRect.top() + plotRect.bottom()) / 2, -rect().right() + 15, "Right Axis");
        painter.restore();
    }

    // 绘制每条可见曲线
    for (auto it = curves.begin(); it != curves.end(); ++it) {
        const QString &name = it.key();
        if (!visible.value(name, false)) continue;
        const QVector<double> &data = it.value();
        if (data.size() != time.size()) continue;

        AxisType axis = axes.value(name, AxisType::Left);
        QColor color = colors.value(name, Qt::black);
        painter.setPen(QPen(color, 2));
        QPainterPath path;
        bool firstPoint = true;
        for (int i = 0; i < time.size(); ++i) {
            QPointF pt = dataToWidget(time[i], data[i], axis);
            if (firstPoint) {
                path.moveTo(pt);
                firstPoint = false;
            } else {
                path.lineTo(pt);
            }
        }
        painter.drawPath(path);

        painter.setBrush(color);
        for (int i = 0; i < time.size(); ++i) {
            QPointF pt = dataToWidget(time[i], data[i], axis);
            painter.drawEllipse(pt, 3, 3);
        }
    }
}

// =========================== QPerfCurve 实现 ===========================
QPerfCurve::QPerfCurve(QWidget *parent)
    : QWidget(parent), m_nextColorIndex(0)
{
    m_colorPalette = { Qt::red, Qt::green, Qt::blue, Qt::cyan, Qt::magenta,
                       Qt::darkYellow, Qt::darkCyan, Qt::darkMagenta };

    m_mainLayout = new QVBoxLayout(this);
    m_plotCanvas = new PlotCanvas(this, this);
    m_controlPanel = new QWidget(this);
    m_controlLayout = new QHBoxLayout(m_controlPanel);
    m_controlLayout->setContentsMargins(5, 5, 5, 5);
    m_controlLayout->setSpacing(10);
    // 允许换行：使用 QHBoxLayout 并设置控件的 sizePolicy 即可（自动换行需配合 FlowLayout，简单起见保持水平滚动）
    m_controlPanel->setLayout(m_controlLayout);
    m_controlPanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_controlPanel->setMaximumHeight(60);

    m_mainLayout->addWidget(m_plotCanvas, 1);
    m_mainLayout->addWidget(m_controlPanel, 0);

    setLayout(m_mainLayout);
}

QPerfCurve::~QPerfCurve() {}

void QPerfCurve::setTimeData(const QVector<double> &time)
{
    m_timeData = time;
    refreshPlot();
}

void QPerfCurve::addCurve(const QString &name, const QVector<double> &data,
                      AxisType axis, const QColor &color)
{
    if (data.size() != m_timeData.size() && !m_timeData.isEmpty()) {
        qWarning() << "Data size mismatch for curve" << name;
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
    m_curvesData.remove(name);
    m_curvesVisible.remove(name);
    m_curvesColor.remove(name);
    m_curvesAxis.remove(name);
    rebuildControlPanel();
    refreshPlot();
}

void QPerfCurve::setCurveVisible(const QString &name, bool visible)
{
    if (m_curvesVisible.contains(name)) {
        m_curvesVisible[name] = visible;
        // 同步复选框状态
        if (m_curveControls.contains(name)) {
            m_curveControls[name].checkBox->setChecked(visible);
        }
        refreshPlot();
    }
}

void QPerfCurve::setCurveAxis(const QString &name, AxisType axis)
{
    if (m_curvesAxis.contains(name)) {
        m_curvesAxis[name] = axis;
        // 更新按钮文本
        if (m_curveControls.contains(name)) {
            QPushButton *btn = m_curveControls[name].axisButton;
            btn->setText(axis == AxisType::Left ? "L" : "R");
        }
        refreshPlot();
    }
}

void QPerfCurve::clearCurves()
{
    m_curvesData.clear();
    m_curvesVisible.clear();
    m_curvesColor.clear();
    m_curvesAxis.clear();
    rebuildControlPanel();
    refreshPlot();
}

void QPerfCurve::onCurveVisibilityChanged(const QString &name, bool visible)
{
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
    // 1. 删除所有动态创建的曲线控件容器（及其子控件）
    for (auto &control : m_curveControls) {
        if (control.container) {
            delete control.container;   // 会递归删除子控件
        }
    }
    m_curveControls.clear();

    // 2. 清空布局中的所有项目
    QLayoutItem *child;
    while ((child = m_controlLayout->takeAt(0)) != nullptr) {
        delete child->widget();   // 删除可能残留的 widget
        delete child;
    }

    // 为每条曲线创建一组控件
    for (auto it = m_curvesData.begin(); it != m_curvesData.end(); ++it) {
        const QString &name = it.key();
        QColor color = m_curvesColor[name];
        AxisType axis = m_curvesAxis[name];

        // 颜色指示器
        QLabel *colorLabel = new QLabel;
        QPixmap pix(16, 16);
        pix.fill(color);
        colorLabel->setPixmap(pix);
        colorLabel->setFixedSize(16, 16);
        colorLabel->setToolTip(name);

        // 复选框（曲线名称）
        QCheckBox *checkBox = new QCheckBox(name);
        checkBox->setChecked(m_curvesVisible[name]);
        // 轴切换按钮
        QPushButton *axisBtn = new QPushButton(axis == AxisType::Left ? "L" : "R");
        axisBtn->setFixedSize(24, 24);
        axisBtn->setToolTip("Switch axis (Left/Right)");

        // 布局：将这三个控件放入一个 QHBoxLayout 内，便于整体管理
        QWidget *container = new QWidget;
        QHBoxLayout *layout = new QHBoxLayout(container);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(4);
        layout->addWidget(colorLabel);
        layout->addWidget(checkBox);
        layout->addWidget(axisBtn);
        container->setLayout(layout);
        container->setContextMenuPolicy(Qt::CustomContextMenu);
        container->setProperty("curveName", name); // 存储曲线名，便于右键菜单识别

        // 信号连接
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

    // 添加一个弹性空间，使控件左对齐
    m_controlLayout->addStretch();
    m_controlPanel->update();
}

void QPerfCurve::refreshPlot()
{
    m_plotCanvas->update();
}