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
    bool   isGap = false;

    WaveDataPoint() = default;
    WaveDataPoint(double t_, double v_) : t(t_), val(v_) {}
};
// {"阀芯实时位移", "位移闭环控制偏差", "最终需求值", "PWM 输出占空比", "电磁铁实际电流", "电磁铁目标电流"};
enum class WaveCurveType {kRealDisp = 0, kCloseLoopErr, kDemandVal, kPWMRatio, kRealCurrent, kTargetCurrent, kOpening};
struct WaveCurve
{
    QString name;
    int side;
    QPen pen;
    bool useRightY;
    bool visible;
    WaveCurveType type;
    QVector<WaveDataPoint> points;
};

enum class AxisName {kLeftY, kRightY, kBottomX};
enum class AxisUnit {knone, kms, ks, kmm, kmA, kp};
// 阀芯实时位移", "位移闭环控制偏差", "最终需求值", "PWM输出占空比
struct AxisConfig
{
    QString left_y;
    QString right_y;
    QString bottom_x;

    AxisUnit left_y_unit;
    AxisUnit right_y_unit;
    AxisUnit bottom_x_unit;
};

class QWavePlotWidget : public QWidget
{
    Q_OBJECT
public:

    explicit QWavePlotWidget(QWidget *parent = nullptr);
    QVector<WaveDataPoint> getCurvePoints(int idx) const;
    void setupAxis(const AxisConfig& cfg);
    void updateAxis(AxisName name, QString label, AxisUnit unit);
    int addCurve(const QString& name, const QPen& pen, bool useRightY, WaveCurveType type, bool visible);
    int addCurve(WaveCurve curve);
    void appendData(int curveIndex, double time, double value);
    void appendData(int side, WaveCurveType type, double time, double value);
    // 按曲线类型追加（用于全局唯一的曲线，如合并后的单条"开度"曲线）
    void appendDataByType(WaveCurveType type, double time, double value);
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
    void showCurveType(bool use_right, WaveCurveType type);
    int curveCount() const { return m_curves.size(); }
    QString curveName(int idx) const;
    QPen curvePen(int idx) const;
    // 新增：重置视图到跟随最新
    void resetViewFollowLatest();
    void markGapAllCurves();
    void setLeftYMaxFloor(double v);
    void setRightYMaxFloor(double v);
    double leftYMaxFloor()  const { return m_leftYMaxFloor; }
    double rightYMaxFloor() const { return m_rightYMaxFloor; }

    void setOnOpening(bool opening);
    void setYAxisVisible(int side, bool visible);

    // 目标开度标记（开度模式下显示在左侧Y轴上/下半区的红色刻度）
    void setTargetOpening1(double v);   // 阀1目标开度：上半区
    void setTargetOpening2(double v);   // 阀2目标开度：下半区
    double targetOpening1() const { return m_targetOpening1; }
    double targetOpening2() const { return m_targetOpening2; }
signals:
    // 当某条曲线第一次写入数据时触发
    void sigCurveFirstData(int curveIdx);
    void sigLeftYMaxChanged(double newMax);
    void sigRightYMaxChanged(double newMax);

protected:
    void paintEvent(QPaintEvent *event) override;
    // 重写鼠标滚轮事件
    void wheelEvent(QWheelEvent *event) override;
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
    double m_leftYMaxFloor  = 0.0;
    double m_rightYMaxFloor = 0.0;
    bool on_opening_ = false;
    bool left_y_visible_ = true;
    bool right_y_visible_ = true;
    // 目标开度（红色刻度标记用）
    double m_targetOpening1 = 80.0;  // 阀1目标开度，显示在中线上方
    double m_targetOpening2 = 63.0;  // 阀2目标开度，显示在中线下方
    // ============ 新增滚动交互变量 ============
    // 时间偏移：可视窗口整体向左偏移多少秒
    double m_viewTimeOffset = 0.0;
    // 是否自动跟随最新数据（未手动滚动时为true）
    bool m_followLatest = true;
    // 滚轮每次滚动偏移步长(秒)
    const double m_scrollStepSec = 1.0;
    // ==========================================

    double timeToX(double t, double tViewMin, double tViewMax, double plotWidth);
    double valueToY(double val, double yMin, double yMax, int plotHeight);
    void calcDataRange(double& tLatest,
                       double& leftMin, double& leftMax,
                       double& rightMin, double& rightMax);
    QVector<double> genTicks(double min, double max, int tickCnt, bool y_axis = true);
    // 计算当前可视窗口 [viewMin, viewMax]
    void getViewTimeRange(double tLatest, double& viewMin, double& viewMax);

    // ============ paintEvent 拆分出的绘制子函数 ============
    // 画布上下文：绘制子函数共享的几何/窗口参数
    struct PaintCtx {
        QPainter* painter = nullptr;
        int w = 0, h = 0;          // 控件尺寸
        int plotW = 0, plotH = 0;  // 绘图区尺寸
        double viewMin = 0.0;      // 可视时间窗口
        double viewMax = 0.0;
    };
    // 自动量程计算（m_autoY 时更新左右 Y 范围，必要时抬升 floor 并发信号）
    void updateAutoYRange(double leftMin, double leftMax, double rightMin, double rightMax);
    // X轴：中线（开度模式）、刻度、标签、轴标题
    void drawXAxis(PaintCtx& ctx, const QVector<double>& xTicks);
    // Y轴一侧：刻度、标签、旋转的轴标题；right=false 左轴 / true 右轴
    void drawYAxisSide(PaintCtx& ctx, double yMin, double yMax, int tickCnt, bool right);
    // 旋转绘制的轴标题
    void drawRotatedAxisLabel(QPainter& painter, const QRect& rect, const QString& text, int angle);
    // 开度模式：左右轴上/下半区的目标开度红色标记
    void drawOpeningMarkers(PaintCtx& ctx);
    // 单个目标开度标记（right=false 轴线向右伸入，true 向左伸入）
    void drawOpeningMarker(QPainter& painter, double axisX, double y, const QString& text, bool right);
    // 网格线
    void drawGrid(PaintCtx& ctx, const QVector<double>& xTicks, const QVector<double>& leftYTicks);
    // 曲线绘制（含 on_opening_ 曲线过滤）
    void drawCurves(PaintCtx& ctx);
    void drawSingleCurve(PaintCtx& ctx, const WaveCurve& crv);
    // 曲线末端实时数值标注
    void drawCurveLastValue(PaintCtx& ctx, const WaveCurve& crv);
    // 显示值：on_opening_ 模式下 2 侧开度取负（仅显示层，不改原始数据）
    double displayValue(const WaveCurve& crv, double val) const;
};
#endif // QWAVEPLOTWIDGET_H
