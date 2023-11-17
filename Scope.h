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

  void closeEvent(QCloseEvent * event){
    if(isACQStarted) StopScope();
    emit CloseWindow();
    event->accept();
  }

  QVector<QPointF> TrapezoidFilter(QVector<QPointF> data, int baseLineEndS, int riseTimeS, int flatTopS, float decayTime_ns );

public slots:
  void StartScope();
  void StopScope();
  void UpdateScope();
  void ReadSettingsFromBoard();
  void UpdatePanelFromMomeory();

signals:

  void CloseWindow();
  void SendLogMsg(const QString &msg);
  void TellACQOnOff(const bool onOff);
  void UpdateScaler();
  void UpdateOtherPanels();

private:

  void SetUpComboBoxSimple(RComboBox * &cb, QString str, int row, int col);
  void SetUpComboBox(RComboBox * &cb, QString str, int row, int col, const Reg para);
  void SetUpSpinBox(RSpinBox * &sb, QString str, int row, int col, const Reg para);

  void CleanUpSettingsGroupBox();
  void SetUpPanel_PHA();
  void SetUpPanel_PSD();
  void SetUpPanel_QDC();
  void EnableControl(bool enable);

  void UpdateComobox(RComboBox * &cb, const Reg para);
  void UpdateSpinBox(RSpinBox * &sb, const Reg para);
  void UpdatePanel_PHA();
  void UpdatePanel_PSD();
  void UpdatePanel_QDC();

  void NullThePointers();

  Digitizer ** digi;
  unsigned short nDigi;
  unsigned short ID; // the id of digi, index of cbScopeDigi
  bool isACQStarted;
  int tick2ns;
  int factor; // whether dual trace or not
  bool traceOn[MaxNDigitizer];

  ReadDataThread ** readDataThread;   
  TimingThread * updateTraceThread;
  TimingThread * updateScalarThread;

  bool enableSignalSlot;

  RChart * plot;
  RChartView * plotView;
  QLineSeries * dataTrace[MaxNumberOfTrace]; // 2 analog, 2 digi for PHA, PSD, 1 analog, 4 digi for QDC

  RComboBox * cbScopeDigi;
  RComboBox * cbScopeCh;

  QPushButton * bnScopeStart;
  QPushButton * bnScopeStop;
  QPushButton * bnReadSettingsFromBoard;

  QLineEdit * leTriggerRate;

  QGroupBox * settingGroup;
  QGridLayout * settingLayout;

  QPushButton * runStatus;

  /// common to PSD and PHA
  RSpinBox * sbReordLength;
  RSpinBox * sbPreTrigger;
  RSpinBox * sbDCOffset;
  RComboBox * cbDynamicRange;
  RComboBox * cbPolarity;

  /// PHA
  RSpinBox * sbInputRiseTime;
  RSpinBox * sbTriggerHoldOff;
  RSpinBox * sbThreshold;
  RComboBox * cbSmoothingFactor;

  RSpinBox * sbTrapRiseTime;
  RSpinBox * sbTrapFlatTop;
  RSpinBox * sbDecayTime;
  RSpinBox * sbPeakingTime;
  RSpinBox * sbPeakHoldOff;
  RComboBox * cbPeakAvg;
  RComboBox * cbBaselineAvg;

  RComboBox * cbAnaProbe1;
  RComboBox * cbAnaProbe2;
  RComboBox * cbDigiProbe1;
  RComboBox * cbDigiProbe2;

  /// PSD
  RSpinBox * sbShortGate;
  RSpinBox * sbLongGate;
  RSpinBox * sbGateOffset;

  /// QDC
  //sbShortGate -> GateWidth
  //sbGateOffset -> GateOffset
  //sbTriggerHoldOff ->Trigger Hold Off

};



#endif 