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

  void SetUpComboBox(RComboBox * &cb, QString str, QGridLayout * layout, int row, int col, const Register::Reg para);
  void SetUpSpinBox(RSpinBox * &sb, QString str, QGridLayout * layout, int row, int col, const Register::Reg para);

signals:

private:

  Digitizer ** digi;
  unsigned short nDigi;
  unsigned short ID; // the id of digi, index of cbScopeDigi
  int ch2ns;

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

  RSpinBox * sbReordLength;
  //RSpinBox * sbPreTrigger;

  //RComboBox * cbDynamicRange;

};



#endif 