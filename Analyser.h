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
//#include "OnlineEventBuilder.h"
#include "MultiBuilder.h"

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

  MultiBuilder * GetEventBuilder() { return mb;}

  void RedefineEventBuilder(std::vector<int> idList);
  void SetBackwardBuild(bool TF) { isBuildBackward = TF;}

public slots:
  void StartThread();
  void StopThread();
  virtual void UpdateHistograms(); // where event-building, analysis, and ploting

private slots:


protected:
  QGridLayout * layout;
  void BuildEvents();
  void SetUpdateTimeInSec(double sec = 1.0) {waitTimeinSec = sec; buildTimerThread->SetWaitTimeinSec(waitTimeinSec);}

private:
  Digitizer ** digi;
  unsigned short nDigi;

  double waitTimeinSec; 

  MultiBuilder * mb;
  bool isBuildBackward;
  TimingThread * buildTimerThread;

};
#endif