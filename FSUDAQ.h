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

class ScalarWorker; //Forward declaration

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
  // void UpdateScalar();

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

// private:
public:

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
  QThread * scalarThread;
  ScalarWorker * scalarWorker;
  QTimer * scalarTimer;

  // TimingThread * scalarThread;
  QLineEdit  *** leTrigger; // need to delete manually
  QLineEdit  *** leAccept; // need to delete manually
  QPushButton * runStatus[MaxNDigitizer];
  QLabel * lbLastUpdateTime;
  QLabel * lbScalarACQStatus;
  QLabel * lbAggCount[MaxNDigitizer];
  QLabel * lbFileSize[MaxNDigitizer];
  QLabel * lbTotalFileSize;

  unsigned short scalarUpdateTimeMilliSec;

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

  //@----- Analyzer
  Analyzer * onlineAnalyzer;
  

};

//^======================== Scalar Worker

class ScalarWorker : public QObject{
  Q_OBJECT
public:
  ScalarWorker(FSUDAQ * parent): SS(parent){}

public slots:
  void UpdateScalar(){

    DebugPrint("%s", "FSUDAQ");

    // printf("================== FSUDAQ::%s\n", __func__);

    if( SS->digi == nullptr ) return;
    if( SS->scalar == nullptr ) return;
    //if( !scalar->isVisible() ) return;
    
    // digi[0]->GetData()->PrintAllData();

    // lbLastUpdateTime->setText("Last update: " + QDateTime::currentDateTime().toString("MM.dd hh:mm:ss"));
    SS->lbLastUpdateTime->setText(QDateTime::currentDateTime().toString("MM/dd hh:mm:ss"));
    SS->scalarCount ++;

    uint64_t totalFileSize = 0;
    for( unsigned int iDigi = 0; iDigi < SS->nDigi; iDigi++){
      // printf("======== digi-%d\n", iDigi);
      if( SS->digi[iDigi]->IsBoardDisabled() ) continue;

      uint32_t acqStatus = SS->digi[iDigi]->GetACQStatusFromMemory();
      //printf("Digi-%d : acq on/off ? : %d \n", digi[iDigi]->GetSerialNumber(), (acqStatus >> 2) & 0x1 );
      if( ( acqStatus >> 2 ) & 0x1 ){
        if( SS->runStatus[iDigi]->styleSheet() == "") SS->runStatus[iDigi]->setStyleSheet("background-color : green;");
      }else{
        if( SS->runStatus[iDigi]->styleSheet() != "") SS->runStatus[iDigi]->setStyleSheet("");
      }

      if(SS->digiSettings && SS->digiSettings->isVisible() && SS->digiSettings->GetTabID() == iDigi) SS->digiSettings->UpdateACQStatus(acqStatus);

      // digiMTX[iDigi].lock();

      QString blockCountStr = QString::number(SS->digi[iDigi]->GetData()->AggCount);
      blockCountStr += "/" + QString::number(SS->readDataThread[iDigi]->GetReadCount());
      SS->readDataThread[iDigi]->SetReadCountZero();
      SS->lbAggCount[iDigi]->setText(blockCountStr);
      SS->lbFileSize[iDigi]->setText(QString::number(SS->digi[iDigi]->GetData()->GetTotalFileSize()/1024./1024., 'f', 3) + " MB");

      SS->digi[iDigi]->GetData()->CalTriggerRate(); //this will reset NumEventDecode & AggCount
      if( SS->chkSaveData->isChecked() ) totalFileSize += SS->digi[iDigi]->GetData()->GetTotalFileSize();
      for( int i = 0; i < SS->digi[iDigi]->GetNumInputCh(); i++){
        QString a = "";
        QString b = "";
        
        if( SS->digi[iDigi]->GetInputChannelOnOff(i) == true ) {
          // printf(" %3d %2d | %7.2f %7.2f \n", digi[iDigi]->GetSerialNumber(), i, digi[iDigi]->GetData()->TriggerRate[i], digi[iDigi]->GetData()->NonPileUpRate[i]);
          QString a = QString::number(SS->digi[iDigi]->GetData()->TriggerRate[i], 'f', 2);
          QString b = QString::number(SS->digi[iDigi]->GetData()->NonPileUpRate[i], 'f', 2);
          SS->leTrigger[iDigi][i]->setText(a);
          SS->leAccept[iDigi][i]->setText(b);

          if( SS->influx && SS->chkInflux->isChecked() && a != "inf" ){
            SS->influx->AddDataPoint("TrigRate,Bd="+std::to_string(SS->digi[iDigi]->GetSerialNumber()) + ",Ch=" + QString::number(i).rightJustified(2, '0').toStdString() + " value=" +  a.toStdString());
          }

        }
      }

      // digiMTX[iDigi].unlock();
      // printf("============= end of  FSUDAQ::%s\n", __func__);

    }

    SS->lbTotalFileSize->setText("Total Data Size : " + QString::number(totalFileSize/1024./1024., 'f', 3) + " MB");

    SS->repaint();
    SS->scalar->repaint();

    if( SS->influx && SS->chkInflux->isChecked() && SS->scalarCount >= 3){
      if( SS->chkSaveData->isChecked() ) {
        SS->influx->AddDataPoint("RunID value=" + std::to_string(SS->runID));
        SS->influx->AddDataPoint("FileSize value=" + std::to_string(totalFileSize));
      }
      //nflux->PrintDataPoints();
      SS->influx->WriteData(SS->dataBaseName.toStdString());
      SS->influx->ClearDataPointsBuffer();
      SS->scalarCount = 0;
    }

    emit workDone();
  }

signals:
  void workDone();

private:
  FSUDAQ * SS;
};


#endif // MAINWINDOW_H