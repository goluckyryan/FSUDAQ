#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QWidget>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QLabel>
#include <QGridLayout>

#include "ClassDigitizer.h"
#include "CustomThreads.h"

//^#===================================================== MainWindow
class MainWindow : public QMainWindow{
  Q_OBJECT
public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();


private slots:

  void OpenDataPath();
  void LoadProgramSettings();
  void SaveProgramSettings();
  void LoadLastRunFile();
  void SaveLastRunFile();

  void OpenDigitizers();
  void CloseDigitizers();

  void SetupScalar();
  void CleanUpScalar();
  void OpenScalar();
  void UpdateScalar();

  void StartACQ();
  void StopACQ();

private:

  Digitizer ** digi;
  unsigned int nDigi;

  QString programSettingsFilePath;
  QString rawDataPath;
  QString prefix;
  unsigned int runID;
  unsigned int elogID;

  QPushButton * bnOpenDigitizers;
  QPushButton * bnCloseDigitizers;
  QPushButton * bnOpenScope;
  QPushButton * bnDigiSettings;

  QPushButton * bnOpenScaler;
  QPushButton * bnStartACQ;
  QPushButton * bnStopACQ;

  //@----- log msg
  QPlainTextEdit * logInfo;
  void LogMsg(QString msg);
  bool logMsgHTMLMode = true;

  //@-----
  QLineEdit * leDataPath;
  QLineEdit * lePrefix;
  QLineEdit * leComment;
  QLineEdit * leRunID;

  //@----- Scalar
  QMainWindow  * scalar;
  QLineEdit  *** leTrigger; // need to delete manually
  QLineEdit  *** leAccept; // need to delete manually
  QLabel **lbDigi; 
  QLabel ** lbTrigger;
  QLabel ** lbAccept;
  QGridLayout * scalarLayout;
  ScalarThread * scalarThread;
  QLabel * lbLastUpdateTime;
  QLabel * lbScalarACQStatus;
  int nScalarBuilt;

  //@----- ACQ
  ReadDataThread ** readDataThread;

};


#endif // MAINWINDOW_H