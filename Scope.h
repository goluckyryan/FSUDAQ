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
#include <QGroupBox>
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

  void SetUpComboBox(RComboBox * &cb, QString str, int row, int col, const Register::Reg para);
  void SetUpSpinBox(RSpinBox * &sb, QString str, int row, int col, const Register::Reg para);

  void CleanUpSettingsGroupBox();
  void SetUpPHAPanel();

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

  QGroupBox * settingGroup;
  QGridLayout * settingLayout;

  /// common to PSD and PHA
  RSpinBox * sbReordLength;
  RSpinBox * sbPreTrigger;

  RSpinBox * sbDCOffset;
  //RComboBox * cbDynamicRange;

  /// PHA
  RSpinBox * sbInputRiseTime;
  RSpinBox * sbTriggerHoldOff;
  RSpinBox * sbThreshold;
  //RComboBox * cbSmoothingFactor;

  RSpinBox * sbTrapRiseTime;
  RSpinBox * sbTrapFlatTop;
  RSpinBox * sbDecayTime;
  RSpinBox * sbPeakingTime;



};



#endif 