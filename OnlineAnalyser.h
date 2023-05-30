#ifndef ONLINE_ANALYZER_H
#define ONLINE_ANALYZER_H

#include <QMainWindow>
#include <QChart>
#include <QChartView>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QLineEdit>
#include <QGridLayout>
#include <QGroupBox>

#include "macro.h"
#include "ClassDigitizer.h"
#include "CustomThreads.h"
#include "CustomWidgets.h"
#include "OnlineEventBuilder.h"

/**************************************

This class is for, obviously, Online analysis.

It provides essential event building, histograms, and filling.

This is the mother of all other derivative analysis class.

derivative class should define the SetUpCanvas() and UpdateHistogram();

***************************************/

#include "qcustomplot.h"

//^==============================================
//^==============================================
class Histogram1D : public QCustomPlot{
public:
  Histogram1D(QString title, QString xLabel, int xBin, double xMin, double xMax, QWidget * parent = nullptr) : QCustomPlot(parent){
    this->xMin = xMin;
    this->xMax = xMax;
    this->xBin = xBin;

    this->xAxis->setLabel(xLabel);

    bars = new QCPBars(this->xAxis, this->yAxis);
    bars->setWidth((xMax- xMin)/xBin);
  }

private:
  double xMin, xMax, xBin;

  QCPBars *bars;

};

//^==============================================
//^==============================================
class Histogram2D : public QCustomPlot{
public:
  Histogram2D(QString title, QString xLabel, QString yLabel, int xBin, double xMin, double xMax, int yBin, double yMin, double yMax, QWidget * parent = nullptr) : QCustomPlot(parent){
    this->xMin = xMin;
    this->xMax = xMax;
    this->yMin = yMin;
    this->yMax = yMax;
    this->xBin = xBin;
    this->yBin = yBin;

    axisRect()->setupFullAxesBox(true);
    this->xAxis->setLabel(xLabel);
    this->yAxis->setLabel(yLabel);

    colorMap = new QCPColorMap(this->xAxis, this->yAxis);
    colorMap->data()->setSize(xBin, yBin);
    colorMap->data()->setRange(QCPRange(xMin, xMax), QCPRange(yMin, yMax));
    colorMap->setInterpolate(false);

    colorScale = new QCPColorScale(this);
    this->plotLayout()->addElement(0, 1, colorScale); 
    colorScale->setType(QCPAxis::atRight); 
    colorMap->setColorScale(colorScale);

    QCPColorGradient color;
    color.clearColorStops();
    color.setColorStopAt( 0,  QColor("white" ));
    color.setColorStopAt( 1,  QColor("blue"));
    colorMap->setGradient(color);

    this->rescaleAxes();

    connect(this, &QCustomPlot::mouseMove, this, [=](QMouseEvent *event){
      double x = this->xAxis->pixelToCoord(event->pos().x());
      double y = this->yAxis->pixelToCoord(event->pos().y());
      int xI, yI;
      colorMap->data()->coordToCell(x, y, &xI, &yI);
      double z = colorMap->data()->cell(xI, yI);

      // Format the coordinates as desired
      QString coordinates = QString("X: %1, Y: %2, Z: %3").arg(x).arg(y).arg(z);

      // Show the coordinates as a tooltip
      QToolTip::showText(event->globalPosition().toPoint(), coordinates, this);
    });

  }

  void Fill(double x, double y){
    int xIndex, yIndex;
    colorMap->data()->coordToCell(x, y, &xIndex, &yIndex);
    //printf("%d %d\n", xIndex, yIndex);
    if( xIndex < 0 || xBin < xIndex || yIndex < 0 || yBin < yIndex ) return;
    double value = colorMap->data()->cell(xIndex, yIndex);
    colorMap->data()->setCell(xIndex, yIndex, value + 1);

    // rescale the data dimension (color) such that all data points lie in the span visualized by the color gradient:
    colorMap->rescaleDataRange();
  
    // make sure the axis rect and color scale synchronize their bottom and top margins (so they line up):
    // QCPMarginGroup *marginGroup = new QCPMarginGroup(this);
    // this->axisRect()->setMarginGroup(QCP::msBottom|QCP::msTop, marginGroup);
    // colorScale->setMarginGroup(QCP::msBottom|QCP::msTop, marginGroup);
    
    // rescale the key (x) and value (y) axes so the whole color map is visible:
    this->rescaleAxes();

  }

private:
  double xMin, xMax, yMin, yMax;
  int xBin, yBin;

  QCPColorMap * colorMap;
  QCPColorScale *colorScale;

};

//^==============================================
//^==============================================
class OnlineAnalyzer : public QMainWindow{
  Q_OBJECT

public:
  OnlineAnalyzer(Digitizer ** digi, unsigned int nDigi, QMainWindow * parent = nullptr);
  virtual ~OnlineAnalyzer();

  virtual void SetUpCanvas();

public slots:
  void StartThread();
  void StopThread();

private slots:

  virtual void UpdateHistograms(); // where event-building, analysis, and ploting

private:

  Digitizer ** digi;
  unsigned short nDigi;

  OnlineEventBuilder ** oeb;
  TimingThread * buildTimerThread;

  QGridLayout * layout;

  //======================== custom histograms
  Histogram2D * h2;

};
#endif