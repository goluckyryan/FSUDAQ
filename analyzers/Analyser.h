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
#include "MultiBuilder.h"
#include "ClassInfluxDB.h"
#include "math.h"

/**************************************

This class is for, obviously, Online analysis.

It provides essential event building, histograms, and filling.

This is the mother of all other derivative analysis class.

derivative class should define the SetUpCanvas() and UpdateHistogram();

After creating a new class based on the Analyzer class, 
users need to add the class files to the FSUDAQ_Qt6.pro project file, 
include the header file in FSUDAQ.cpp, 
modify the MainWindow::OpenAnalyzer() method, 
and recompile FSUDAQ to incorporate the changes and activate the custom analyzer.

***************************************/
#include "Histogram1D.h"
#include "Histogram2D.h"

class AnalyzerWorker; //Forward decalration

//^==============================================
//^==============================================
class Analyzer : public QMainWindow{
  Q_OBJECT

public:
  Analyzer(Digitizer ** digi, unsigned int nDigi, QMainWindow * parent = nullptr);
  virtual ~Analyzer();

  MultiBuilder * GetEventBuilder() { return mb;}

  void RedefineEventBuilder(std::vector<int> idList);
  void SetBackwardBuild(bool TF, int maxNumEvent = 100) { isBuildBackward = TF; maxNumEventBuilt = maxNumEvent;}
  void SetDatabase(QString IP, QString Name, QString Token);

  double RandomGauss(double mean, double sigma);
  void SetDatabaseButton();

  double GetUpdateTimeInSec() const {return waitTimeinSec;}

  virtual void SetUpCanvas();
  virtual void UpdateHistograms(); // where event-building, analysis, and ploting
  virtual void ReplotHistograms();

public slots:
  void startTimer(){ 
    // printf("start timer\n");
    mb->ForceStop(false);
    anaTimer->start(waitTimeinSec*1000); 
  } 
  void stopTimer(){ 
    // printf("stop worker\n");
    anaTimer->stop(); 
    mb->ForceStop(true);
    mb->ClearEvents();
  }  

private slots:

protected:
  QGridLayout * layout;
  void BuildEvents(bool verbose = false);
  void SetUpdateTimeInSec(double sec = 1.0) { waitTimeinSec = sec; }

  InfluxDB * influx;
  QString dataBaseIP;
  QString dataBaseName;
  QString dataBaseToken;

  bool isWorking; // a flag to indicate the worker is working

  Digitizer ** digi;
  unsigned short nDigi;

  Data ** dataList;
  std::vector<int> typeList;
  std::vector<int> snList;

  double waitTimeinSec; 

  MultiBuilder * mb;
  bool isBuildBackward;
  int maxNumEventBuilt;
  // TimingThread * buildTimerThread;

  QThread * anaThread;
  AnalyzerWorker * anaWorker;
  QTimer * anaTimer;

};

//^================================================ AnalyzerWorker

class AnalyzerWorker : public QObject{
  Q_OBJECT
public:
  AnalyzerWorker(Analyzer * parent): SS(parent){}

public slots:
  void UpdateHistograms(){
    SS->UpdateHistograms();
    emit workDone();
  }

signals:
  void workDone();

private:
  Analyzer * SS;
};

#endif
