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


#include <QScatterSeries>

//^==============================================
//^==============================================
class Histogram2D{
public:
  Histogram2D(QString title, double xMin, double xMax, double yMin, double yMax){

    this->xMin = xMin;
    this->xMax = xMax;
    this->yMin = yMin;
    this->yMax = yMax;

    plot = new RChart();
    scatterSeries = new QScatterSeries();
    scatterSeries->setName(title);
    scatterSeries->setMarkerShape(QScatterSeries::MarkerShapeCircle);
    scatterSeries->setBorderColor(QColor(0,0,0,0));
    scatterSeries->setMarkerSize(5.0);

    plot->addSeries(scatterSeries);
    plot->createDefaultAxes();

    QValueAxis * xaxis = qobject_cast<QValueAxis*> (plot->axes(Qt::Horizontal).first());
    xaxis->setRange(xMin, xMax);

    QValueAxis * yaxis = qobject_cast<QValueAxis*> (plot->axes(Qt::Vertical).first());
    yaxis->setRange(yMin, yMax);

  }

  ~Histogram2D(){
    delete scatterSeries;
    delete plot;
  }

  double GetXMin() const {return xMin;} 
  double GetXMax() const {return xMax;} 
  double GetyMin() const {return yMin;} 
  double GetyMax() const {return yMax;} 

  RChart * GetChart() {return plot;}
  void SetMarkerColor(QColor color) { scatterSeries->setColor(color); }
  void SetMarkerSize(qreal size) { scatterSeries->setMarkerSize(size);}
  void Clear(){  scatterSeries->clear();}
  void Fill(double x, double y) { scatterSeries->append(x, y); }

private:
  double xMin, xMax, yMin, yMax;

  RChart * plot;
  QScatterSeries * scatterSeries;

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

};
#endif