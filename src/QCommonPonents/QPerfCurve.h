#ifndef _QPERF_CRUVE_H_
#define _QPERF_CRUVE_H_

#include <QWidget>
#include <QMap>
#include <QVector>
#include <cmath>
#include <QSplitter>
#include <QListWidget>
#include <QPainter>
#include <QPen>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDebug>
#include <QMenu>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include <QMutex>
#include <QMutexLocker>

enum class AxisType { Left, Right };

// =========================== PlotCanvas 绘图区域 ===========================
class QPerfCurve; // 前向声明

class PlotCanvas : public QWidget
{
    Q_OBJECT
public:
    explicit PlotCanvas(QPerfCurve *parentCurve, QWidget *parent = nullptr);
    void paintEvent(QPaintEvent *event) override;

private:
    QPerfCurve *m_curve;
};

// =========================== QPerfCurve 主控件 ===========================
class QPerfCurve : public QWidget
{
    Q_OBJECT
public:
    explicit QPerfCurve(QWidget *parent = nullptr);
    ~QPerfCurve();

    // ----- 数据管理 API -----
    void setTimeData(const QVector<double> &time);
    void addCurve(const QString &name, const QVector<double> &data,
                  AxisType axis = AxisType::Left, const QColor &color = QColor());
    void removeCurve(const QString &name);
    void setCurveVisible(const QString &name, bool visible);
    void setCurveAxis(const QString &name, AxisType axis);
    void clearCurves();

    // ===== 动态数据追加 API =====
    void appendDataPoint(double time, const QMap<QString, double> &dataPoints);
    void appendDataPoints(const QVector<double> &time, const QMap<QString, QVector<double>> &dataPoints);
    void setMaxDataPoints(int maxPoints);
    int getDataPointCount() const { return m_timeData.size(); }
    void clearAllData();
    void setAutoRefreshInterval(int intervalMs);
    void setAutoRefreshEnabled(bool enabled);

    // ----- 供 PlotCanvas 使用的数据访问 -----
    QVector<double> getTimeData() const { return m_timeData; }
    QMap<QString, QVector<double>> getCurvesData() const { return m_curvesData; }
    QMap<QString, bool> getCurvesVisible() const { return m_curvesVisible; }
    QMap<QString, QColor> getCurvesColor() const { return m_curvesColor; }
    QMap<QString, AxisType> getCurvesAxis() const { return m_curvesAxis; }

signals:
    void dataAppended(int newPointCount);
    void maxDataPointsReached();

private slots:
    void onCurveVisibilityChanged(const QString &name, bool visible);
    void onCurveAxisSwitched(const QString &name);
    void onCurveContextMenu(const QPoint &pos, const QString &name);
    void onAutoRefresh();

private:
    void rebuildControlPanel();
    void refreshPlot();
    void trimDataIfNeeded();
    bool validateData() const;

    // 数据
    QVector<double> m_timeData;
    QMap<QString, QVector<double>> m_curvesData;
    QMap<QString, bool> m_curvesVisible;
    QMap<QString, QColor> m_curvesColor;
    QMap<QString, AxisType> m_curvesAxis;
    
    // 动态数据相关
    int m_maxDataPoints;
    QTimer *m_autoRefreshTimer;
    bool m_autoRefreshEnabled;
    mutable QMutex m_dataMutex;  // 改为 mutable

    // UI 组件
    QVBoxLayout *m_mainLayout;
    PlotCanvas *m_plotCanvas;
    QWidget *m_controlPanel;
    QHBoxLayout *m_controlLayout;

    struct CurveControl {
        QWidget *container;
        QCheckBox *checkBox;
        QPushButton *axisButton;
        QLabel *colorLabel;
    };
    QMap<QString, CurveControl> m_curveControls;

    QList<QColor> m_colorPalette;
    int m_nextColorIndex;
};

#endif // _QPERF_CRUVE_H_