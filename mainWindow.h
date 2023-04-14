#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QWidget>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QLineEdit>

#include "ClassDigitizer.h"
#include "CustomThreads.h"

//^#===================================================== MainWindow
class MainWindow : public QMainWindow{
  Q_OBJECT
public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();


private slots:

  void OpenDigitizers();
  void CloseDigitizers();

  void OpenDataPath();

  void StartACQ();
  void StopACQ();

private:

  Digitizer ** digi;
  unsigned int nDigi;

  //@----- log msg
  QPlainTextEdit * logInfo;
  void LogMsg(QString msg);
  bool logMsgHTMLMode = true;

  //@-----
  QLineEdit * leDataPath;
  QLineEdit * lePrefix;
  QLineEdit * leComment;

  //@----- Scalar
  QMainWindow  * scalar;
  QLineEdit  *** leTrigger; // need to delete manually
  QLineEdit  *** leAccept; // need to delete manually

  //@----- ACQ
  ReadDataThread ** readDataThread;

};


#endif // MAINWINDOW_H