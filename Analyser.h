#ifndef ANALYZER_H
#define ANALYZER_H

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
#include "Histogram1D.h"
#include "Histogram2D.h"

//^==============================================
//^==============================================
class Analyzer : public QMainWindow{
  Q_OBJECT

public:
  Analyzer(Digitizer ** digi, unsigned int nDigi, QMainWindow * parent = nullptr);
  virtual ~Analyzer();

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
  Histogram1D * h1;

};
#endif