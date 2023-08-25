#ifndef MAINWINDOW_H
#define MAINWINDOW_H

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
#include "influxdb.h"
#include "analyzers/Analyser.h"

//^#===================================================== MainWindow
class MainWindow : public QMainWindow{
  Q_OBJECT
public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

  void closeEvent(QCloseEvent * event){
    if( scope ) scope->close();
    if( digiSettings ) digiSettings->close();
    if( canvas ) canvas->close();
    if( onlineAnalyzer ) onlineAnalyzer->close();
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

  void OpenCanvas();

  void OpenAnalyzer();

  void UpdateAllPanels(int panelID);

  void SetUpInflux();

  void CheckElog();
  void WriteElog(QString htmlText, QString subject, QString category, int runNumber);
  void AppendElog(QString appendHtmlText);

  void SetSyncMode();


private:

  Digitizer ** digi;
  unsigned int nDigi;

  QString programSettingsFilePath;
  QString rawDataPath;
  QString prefix;
  unsigned int runID;
  int elogID;

  QPushButton * bnOpenDigitizers;
  QPushButton * bnCloseDigitizers;
  QPushButton * bnOpenScope;
  QPushButton * bnDigiSettings;
  QPushButton * bnAnalyzer;

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
  
  //@----- Elog
  QString elogIP;
  QString elogName;
  QString elogUser;
  QString elogPWD;
  QLineEdit * leElogIP;
  QLineEdit * leElogName;

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
  QLabel * lbLastUpdateTime;
  QLabel * lbScalarACQStatus;

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
  SingleSpectra * canvas;
  TimingThread * histThread;

  //@----- Analyzer
  Analyzer * onlineAnalyzer;
  

};


#endif // MAINWINDOW_H