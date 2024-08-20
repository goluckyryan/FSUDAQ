#ifndef FSUDAQ_H
#define FSUDAQ_H

#include <QMainWindow>
#include <QWidget>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QLabel>
#include <QComboBox>
#include <QCheckBox>
#include <QGridLayout>

#include "ClassDigitizer.h"
#include "CustomThreads.h"
#include "CustomWidgets.h"
#include "Scope.h"
#include "DigiSettingsPanel.h"
#include "SingleSpectra.h"
#include "ClassInfluxDB.h"
#include "analyzers/Analyser.h"

//^#===================================================== FSUDAQ
class FSUDAQ : public QMainWindow{
  Q_OBJECT
public:
  FSUDAQ(QWidget *parent = nullptr);
  ~FSUDAQ();

  void closeEvent(QCloseEvent * event){
    if( scope ) {
      delete scope;
      scope = nullptr;
    }
    if( digiSettings ) {
      delete digiSettings;
      digiSettings = nullptr;
    }
    if( singleHistograms ) {
      delete singleHistograms;
      singleHistograms = nullptr;
    }
    if( onlineAnalyzer ) {
      delete onlineAnalyzer;
      onlineAnalyzer = nullptr;
    }
    event->accept();
  }

private slots:

  void OpenDataPath();
  void OpenRecord();
  void UpdateRecord();
  void LoadProgramSettings();
  void SaveProgramSettings();
  void LoadLastRunFile();
  void SaveLastRunFile();

  void OpenDigitizers();
  void CloseDigitizers();
  void WaitForDigitizersOpen(bool onOff);

  void SetupScalar();
  void CleanUpScalar();
  void OpenScalar();
  void UpdateScalar();

  void StartACQ();
  void StopACQ();
  void AutoRun();
  bool CommentDialog(bool isStartRun);
  void WriteRunTimestamp(bool isStartRun);

  void OpenScope();

  void OpenDigiSettings();

  void OpenSingleHistograms();

  void OpenAnalyzer();

  void UpdateAllPanels(int panelID);

  void SetUpInflux();

  void CheckElog();
  void WriteElog(QString htmlText, QString subject, QString category, int runNumber);
  void AppendElog(QString appendHtmlText);

  void SetSyncMode();

  void SetAndLockInfluxElog();

private:

  Digitizer ** digi;
  unsigned int nDigi;

  QString programSettingsFilePath;
  QString rawDataPath;
  QString prefix;
  unsigned int runID;
  int elogID;

  RComboBox * cbOpenDigitizers;
  RComboBox * cbOpenMethod;

  //QPushButton * bnOpenDigitizers;
  QPushButton * bnCloseDigitizers;
  QPushButton * bnOpenScope;
  QPushButton * bnDigiSettings;
  //QPushButton * bnAnalyzer;

  RComboBox * cbAnalyzer;

  QPushButton * bnOpenScaler;
  QPushButton * bnStartACQ;
  QPushButton * bnStopACQ;
  QPushButton * bnSync;

  QPushButton * bnCanvas;

  QTimer * runTimer;
  bool breakAutoRepeat;
  bool needManualComment;
  QMetaObject::Connection runTimerConnection;

  //@----- influx
  InfluxDB * influx;

  QString influxIP;
  QString dataBaseName;
  QLineEdit * leInfluxIP;
  QLineEdit * leDatabaseName;
  QPushButton * bnLock;
  QString influxToken;
  short scalarCount;
  
  //@----- Elog
  QString elogIP;
  QString elogName;
  QString elogUser;
  QString elogPWD;
  QLineEdit * leElogIP;
  QLineEdit * leElogName;

  QCheckBox * chkInflux;
  QCheckBox * chkElog;

  //@----- log msg
  QPlainTextEdit * logInfo;
  void LogMsg(QString msg);
  bool logMsgHTMLMode = true;

  //@-----
  QLineEdit * leDataPath;
  QLineEdit * lePrefix;
  QLineEdit * leComment;
  QLineEdit * leRunID;

  QCheckBox * chkSaveData;
  RComboBox * cbAutoRun;

  QString startComment;
  QString stopComment;

  //@----- Scalar
  QMainWindow  * scalar;
  QGridLayout * scalarLayout;
  TimingThread * scalarThread;
  QLineEdit  *** leTrigger; // need to delete manually
  QLineEdit  *** leAccept; // need to delete manually
  QPushButton * runStatus[MaxNDigitizer];
  QLabel * lbLastUpdateTime;
  QLabel * lbScalarACQStatus;
  QLabel * lbAggCount[MaxNDigitizer];
  QLabel * lbFileSize[MaxNDigitizer];
  QLabel * lbTotalFileSize;

  //@----- Run Record
  QMainWindow * runRecord;
  QStandardItemModel *model;
  QTableView * tableView;

  //@----- ACQ
  ReadDataThread ** readDataThread;
  
  //@----- Scope
  Scope * scope;

  //@----- DigiSettings
  DigiSettingsPanel * digiSettings;

  //@----- SingleSpectra
  SingleSpectra * singleHistograms;
  TimingThread * histThread;

  //@----- Analyzer
  Analyzer * onlineAnalyzer;
  

};


#endif // MAINWINDOW_H