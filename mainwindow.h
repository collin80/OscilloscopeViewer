#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "qcustomplot.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void loadCSV();

private slots:
    void titleDoubleClick(QMouseEvent *event, QCPTextElement *title);
    void axisLabelDoubleClick(QCPAxis* axis, QCPAxis::SelectablePart part);
    void plottableDoubleClick(QCPAbstractPlottable* plottable,int dataIdx, QMouseEvent* event);
    void plottableClick(QCPAbstractPlottable* plottable, int dataIdx, QMouseEvent* event);
    void selectionChanged();
    void mousePress();
    void mouseWheel();
    void contextMenuRequest(QPoint pos);
    void resetView();
    void zoomIn();
    void zoomOut();


private:
    Ui::MainWindow *ui;
    QVector<double> x, y;
};

#endif // MAINWINDOW_H
