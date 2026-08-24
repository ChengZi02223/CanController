#include "QWavePlotWithLegendWidget.h"
#include <QPixmap>
#include <QPainter>
#include <QHBoxLayout>
#include <QDebug>

QWavePlotWithLegendWidget::QWavePlotWithLegendWidget(QWidget *parent)
    : QWidget(parent)
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(6);

    // 绘图控件
    m_plot = new QWavePlotWidget(this);
    m_mainLayout->addWidget(m_plot, 1); // 绘图区域占满剩余高度

    // 图例容器
    m_legendContainer = new QWidget();
    m_legendFlowLayout = new FlowLayout(m_legendContainer, 2, 10, 4);
    m_legendContainer->setMaximumHeight(90);
    m_mainLayout->addWidget(m_legendContainer);
    connect(m_plot, &QWavePlotWidget::sigCurveFirstData,
            this, &QWavePlotWithLegendWidget::refreshLegend);
}

void QWavePlotWithLegendWidget::clearAllLegendItems()
{
    for (auto& item : m_legendItems)
    {
        m_legendFlowLayout->removeWidget(item.groupWidget);
        delete item.groupWidget; //删除父容器，内部checkbox/label全部自动析构
    }
    m_legendItems.clear();
}

LegendItem QWavePlotWithLegendWidget::createLegendItem(int curveIdx)
{
    LegendItem item;
    item.curveIndex = curveIdx;

    QString curveName = m_plot->curveName(curveIdx);
    QPen curvePen = m_plot->curvePen(curveIdx);
    bool curveVis = m_plot->isCurveVisible(curveIdx);

    // ===== 关键：每组先包一个容器widget，内部用QHBoxLayout，内部统一垂直居中 =====
    item.groupWidget = new QWidget(m_legendContainer);
    QHBoxLayout* groupLayout = new QHBoxLayout(item.groupWidget);
    groupLayout->setContentsMargins(0,0,0,0);
    groupLayout->setSpacing(6);
    groupLayout->setAlignment(Qt::AlignVCenter); // !!!组内所有控件强制垂直居中

    // 1. CheckBox
    item.checkBox = new QCheckBox(curveName, m_legendContainer);
    item.checkBox->setChecked(curveVis);
    connect(item.checkBox, &QCheckBox::toggled, this, [=](bool checked){
        m_plot->setCurveVisible(curveIdx, checked);
    });

    // 2. 彩色线段Label：生成固定长度彩色横线Pixmap
    item.lineLabel = new QLabel(item.groupWidget);
    const int pixW = 26;
    const int pixH = 8; // 把pixmap整体加高，给垂直居中留空间
    QPixmap linePix(pixW, pixH);
    linePix.fill(Qt::transparent);
    QPainter p(&linePix);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(curvePen);
    int midY = pixH / 2;
    p.drawLine(0, midY, pixW, midY);
    item.lineLabel->setPixmap(linePix);
    item.lineLabel->setFixedSize(pixW, pixH);
    item.lineLabel->setAlignment(Qt::AlignVCenter);

    groupLayout->addWidget(item.checkBox);
    groupLayout->addWidget(item.lineLabel);
    groupLayout->addStretch(1); 
    m_legendFlowLayout->addWidget(item.groupWidget);

    return item;
}

void QWavePlotWithLegendWidget::refreshLegend()
{
    clearAllLegendItems();

    int curveCount = m_plot->curveCount();
    for (int i = 0; i < curveCount; ++i)
    {
        // 关键：仅曲线存在数据点时才生成图例
        bool hasData = !m_plot->getCurvePoints(i).isEmpty();
        if (!hasData)
            continue;

        if (!isCurveVisible(i))
            continue;
        LegendItem newItem = createLegendItem(i);
        m_legendItems.append(newItem);
    }
}

// ====================== 透传绘图控件接口 ======================
void QWavePlotWithLegendWidget::setupAxis(const AxisConfig &cfg)
{
    m_plot->setupAxis(cfg);
}

void QWavePlotWithLegendWidget::updateAxis(AxisName name, QString label, AxisUnit unit) {
    m_plot->updateAxis(name, label, unit);
}

int QWavePlotWithLegendWidget::addCurve(const QString &name, const QPen &pen, bool useRightY, WaveCurveType type, bool visible)
{
    int idx = m_plot->addCurve(name, pen, useRightY, type, visible);
    refreshLegend(); // 新增曲线刷新图例
    return idx;
}

int QWavePlotWithLegendWidget::addCurve(WaveCurve curve){
    int idx = m_plot->addCurve(curve);
    refreshLegend(); // 新增曲线刷新图例
    return idx;
}

void QWavePlotWithLegendWidget::appendData(int curveIndex, double time, double value)
{
    m_plot->appendData(curveIndex, time, value);
    // refreshLegend(); // 写入数据后刷新，自动生成图例
}

void QWavePlotWithLegendWidget::appendData(int side, WaveCurveType type, double time, double value) {
    m_plot->appendData(side, type, time, value);
}

void QWavePlotWithLegendWidget::setMaxPointCount(int cnt)
{
    m_plot->setMaxPointCount(cnt);
}

void QWavePlotWithLegendWidget::setTimeWindow(double sec)
{
    m_plot->setTimeWindow(sec);
}

void QWavePlotWithLegendWidget::clearAll()
{
    m_plot->clearAll();
    refreshLegend(); // 清空所有数据，销毁全部图例
}

void QWavePlotWithLegendWidget::setLeftYRange(double min, double max)
{
    m_plot->setLeftYRange(min, max);
}

void QWavePlotWithLegendWidget::setRightYRange(double min, double max)
{
    m_plot->setRightYRange(min, max);
}

void QWavePlotWithLegendWidget::setAutoY(bool enable)
{
    m_plot->setAutoY(enable);
}

void QWavePlotWithLegendWidget::setCurveVisible(int curveIndex, bool visible)
{
    m_plot->setCurveVisible(curveIndex, visible);
    refreshLegend();
}

bool QWavePlotWithLegendWidget::isCurveVisible(int curveIndex) const
{
    return m_plot->isCurveVisible(curveIndex);
}

void QWavePlotWithLegendWidget::setAllCurveVisible(bool visible)
{
    m_plot->setAllCurveVisible(visible);
    refreshLegend();
}

void QWavePlotWithLegendWidget::showCurveType(bool use_right, WaveCurveType type) {
    m_plot->showCurveType(use_right, type);
    refreshLegend();
}

void QWavePlotWithLegendWidget::tryRefreshLegend()
{
    refreshLegend();
}