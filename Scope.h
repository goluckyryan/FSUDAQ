#ifndef SCOPE_H
#define SCOPE_H

#include <QMainWindow>
#include <QChart>
#include <QChartView>
#include <QSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QLineEdit>
#include <QComboBox>
#include <QGridLayout>
#include <QLineSeries>
#include <QRubberBand>
#include <QMouseEvent>
#include <QGestureEvent>

#include "macro.h"
#include "ClassDigitizer.h"
#include "CustomThreads.h"
#include "CustomWidgets.h"

//^====================================================
//^====================================================
class Scope : public QMainWindow{
  Q_OBJECT

public:
  Scope(Digitizer ** digi, unsigned int nDigi, ReadDataThread ** readDataThread, QMainWindow * parent = nullptr);
  ~Scope();

public slots:

private slots:
  void StartScope();
  void StopScope();

signals:

private:

  Digitizer ** digi;
  unsigned short nDigi;

  ReadDataThread ** readDataThread;   
  UpdateTraceThread * updateTraceThread;

  bool enableSignalSlot;

  Trace * plot;
  QLineSeries * dataTrace[MaxNumberOfTrace]; // 2 analog, 2 digi

  RComboBox * cbScopeDigi;
  RComboBox * cbScopeCh;


  QPushButton * bnScopeStart;
  QPushButton * bnScopeStop;

  QLineEdit * leTriggerRate;

};



#endif 