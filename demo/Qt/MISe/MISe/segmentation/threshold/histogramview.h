#ifndef HISTOGRAMVIEW_H
#define HISTOGRAMVIEW_H

#include <ift.h>
#include <QWidget>
#include <QtCharts>

namespace Ui {
class HistogramView;
}

class HistogramView : public QWidget
{
    Q_OBJECT

public:
    explicit HistogramView( iftImage *img);
    ~HistogramView();


private slots:
    void updateInterval();

private:
    void generateHistogram( iftImage *img);
    iftIntArray *histogram;
    void plotHistogram();

    int otsuValue;
    int maxValue;
    QAreaSeries *series;
    QValueAxis *axisX, *axisY;
    QGraphicsLineItem * otsuLine;

    Ui::HistogramView *ui;


};

#endif // HISTOGRAMVIEW_H
