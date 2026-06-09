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

    // ----- 供 PlotCanvas 使用的数据访问 -----
    QVector<double> getTimeData() const { return m_timeData; }
    QMap<QString, QVector<double>> getCurvesData() const { return m_curvesData; }
    QMap<QString, bool> getCurvesVisible() const { return m_curvesVisible; }
    QMap<QString, QColor> getCurvesColor() const { return m_curvesColor; }
    QMap<QString, AxisType> getCurvesAxis() const { return m_curvesAxis; }

private slots:
    void onCurveVisibilityChanged(const QString &name, bool visible);
    void onCurveAxisSwitched(const QString &name);
    void onCurveContextMenu(const QPoint &pos, const QString &name);

private:
    void rebuildControlPanel();      // 根据曲线列表重建下方的控制面板
    void refreshPlot();              // 请求重绘画布

    // 数据
    QVector<double> m_timeData;
    QMap<QString, QVector<double>> m_curvesData;
    QMap<QString, bool> m_curvesVisible;
    QMap<QString, QColor> m_curvesColor;
    QMap<QString, AxisType> m_curvesAxis;

    // UI 组件
    QVBoxLayout *m_mainLayout;
    PlotCanvas *m_plotCanvas;
    QWidget *m_controlPanel;
    QHBoxLayout *m_controlLayout;    // 水平布局，可换行（使用 QHBoxLayout 并允许换行需要配合 QWidget 的 sizePolicy）

    // 曲线控件映射：曲线名 -> 控件集合（复选框 + 轴按钮）
    struct CurveControl {
        QWidget *container;      // 整个控件组的外框容器
        QCheckBox *checkBox;
        QPushButton *axisButton;
        QLabel *colorLabel;
    };
    QMap<QString, CurveControl> m_curveControls;

    // 自动颜色分配
    QList<QColor> m_colorPalette;
    int m_nextColorIndex;
};

// // =========================== 使用示例 ===========================
// int main(int argc, char *argv[])
// {
//     QApplication app(argc, argv);

//     QPerfCurve curveWidget;
//     curveWidget.setWindowTitle("QPerfCurve - 曲线绘制与选择");

//     // 准备数据（时间从0到10秒）
//     QVector<double> time = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
//     QVector<double> displacement = {0, 10, 30, 60, 100, 150, 210, 280, 360, 450, 550};
//     QVector<double> flow = {0, 20, 45, 70, 100, 130, 160, 180, 190, 195, 200};

//     curveWidget.setTimeData(time);
//     curveWidget.addCurve("Displacement", displacement);  // 自动分配颜色
//     curveWidget.addCurve("Flow", flow);                  // 自动分配第二种颜色

//     // 可以手动指定颜色
//     // curveWidget.addCurve("Flow", flow, QColor(0, 150, 0));

//     curveWidget.resize(800, 500);
//     curveWidget.show();

//     return app.exec();
// }

#endif // _QPERF_CRUVE_H_