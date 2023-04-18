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
#include "Scope.h"

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
  void AutoRun();
  bool CommentDialog(bool isStartRun);
  void WriteRunTimestamp(bool isStartRun);

  void OpenScope();

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

  QCheckBox * chkSaveData;
  QComboBox * cbAutoRun;

  QString startComment;
  QString stopComment;

  //@----- Scalar
  QMainWindow  * scalar;
  QGridLayout * scalarLayout;
  ScalarThread * scalarThread;
  QLineEdit  *** leTrigger; // need to delete manually
  QLineEdit  *** leAccept; // need to delete manually
  QLabel * lbLastUpdateTime;
  QLabel * lbScalarACQStatus;

  //@----- ACQ
  ReadDataThread ** readDataThread;

  //@----- Scope
  Scope * scope;

};


#endif // MAINWINDOW_H