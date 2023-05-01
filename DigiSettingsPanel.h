#ifndef DigiSettings_H
#define DigiSettings_H

#include <QMainWindow>
#include <QTabWidget>
#include <QLineEdit>
#include <QGridLayout>
#include <QCheckBox>
#include <QGroupBox>
#include <QPushButton>

#include "macro.h"
#include "ClassDigitizer.h"
#include "CustomWidgets.h"

class DigiSettingsPanel : public QMainWindow{
  Q_OBJECT

public:
  DigiSettingsPanel(Digitizer ** digi, unsigned int nDigi, QMainWindow * parent = nullptr);
  ~DigiSettingsPanel();

private slots:
  void UpdatePanelFromMemory();
  void ReadSettingsFromBoard();

signals:
  void SendLogMsg(const QString &msg);


private:

  void SetUpInfo(QString label, std::string value, QGridLayout *gLayout, int row, int col);
  void SetUpCheckBox(QCheckBox * &chkBox, QString label, QGridLayout *gLayout, int row, int col, Reg para, std::pair<unsigned short, unsigned short> bit);
  void SetUpComboBoxBit(RComboBox * &cb, QString label, QGridLayout *gLayout, int row, int col, std::vector<std::pair<std::string, unsigned int>> items, Reg para, std::pair<unsigned short, unsigned short> bit, int colspan = 1);
  void SetUpComboBox(RComboBox * &cb, QString label, QGridLayout *gLayout, int row, int col, Reg para);
  void SetUpSpinBox(RSpinBox * &sb, QString label, QGridLayout *gLayout, int row, int col, Reg para);

  void CleanUpGroupBox(QGroupBox * & gBox);
  void SetUpGlobalTriggerMaskAndFrontPanelMask();
  
  void SetUpPHABoard();
  void SetUpPHAChannel();
  
  void SetUpPSDBoard();



  void UpdatePHASetting();


  Digitizer ** digi;
  unsigned int nDigi;
  unsigned short ID;

  bool enableSignalSlot;

  QTabWidget * tabWidget;

  QGroupBox * infoBox[MaxNDigitizer];
  QGridLayout * infoLayout[MaxNDigitizer];

  QLineEdit * leSaveFilePath;

  QPushButton * bnRefreshSetting; // read setting from board
  QPushButton * bnProgramPreDefined;
  QPushButton * bnClearBuffer;
  
  QPushButton * bnSendSoftwareTriggerSignal;
  QPushButton * bnSendSoftwareClockSyncSignal;
  QPushButton * bnSaveSettings;
  QPushButton * bnLoadSettings;

  /// ============================= Board Configure
  QGroupBox * boardSettingBox[MaxNDigitizer];
  QGridLayout * settingLayout[MaxNDigitizer];

  QCheckBox * chkAutoDataFlush[MaxNDigitizer];
  QCheckBox * chkDecimateTrace[MaxNDigitizer];
  QCheckBox * chkTrigPropagation[MaxNDigitizer];
  QCheckBox * chkDualTrace[MaxNDigitizer];
  QCheckBox * chkTraceRecording[MaxNDigitizer];
  QCheckBox * chkEnableExtra2[MaxNDigitizer];

  RComboBox * cbAnaProbe1[MaxNDigitizer];
  RComboBox * cbAnaProbe2[MaxNDigitizer];
  RComboBox * cbDigiProbe1[MaxNDigitizer];
  RComboBox * cbDigiProbe2[MaxNDigitizer];

  QPushButton * bnChEnableMask[MaxNDigitizer][MaxNChannels];
  RComboBox * cbAggOrg[MaxNDigitizer];
  RSpinBox * sbAggNum[MaxNDigitizer];
  QCheckBox * chkEnableExternalTrigger[MaxNDigitizer];
  RSpinBox * sbRunDelay[MaxNDigitizer];
  RComboBox * cbAnalogMonitorMode[MaxNDigitizer];
  RSpinBox * sbBufferGain[MaxNDigitizer];

  RComboBox * cbStartStopMode[MaxNDigitizer];
  RComboBox * cbAcqStartArm[MaxNDigitizer];
  RComboBox * cbPLLRefClock[MaxNDigitizer];

  RComboBox * cbLEMOMode[MaxNDigitizer];
  RComboBox * cbTRGOUTMode[MaxNDigitizer];

  /// ============================= board Status
  QPushButton * bnACQStatus[MaxNDigitizer][9];
  QPushButton * bnBdFailStatus[MaxNDigitizer][3];
  QPushButton * bnReadOutStatus[MaxNDigitizer][3];

  /// ============================= Trigger Configure
  QGridLayout * triggerLayout[MaxNDigitizer];

  QPushButton * bnGlobalTriggerMask[MaxNDigitizer][MaxNChannels/2];
  RSpinBox * sbGlbMajCoinWin[MaxNDigitizer];
  RSpinBox * sbGlbMajLvl[MaxNDigitizer];
  RComboBox * cbGlbUseOtherTriggers[MaxNDigitizer]; // combine bit 30, 31

  QPushButton * bnTRGOUTMask[MaxNDigitizer][MaxNChannels/2];
  RSpinBox * sbTRGOUTMajLvl[MaxNDigitizer];
  RComboBox * cbTRGOUTLogic[MaxNDigitizer];
  RComboBox * cbTRGOUTUseOtherTriggers[MaxNDigitizer]; // combine bit 30, 31

  /// ============================ Channel
  QWidget * chAllSetting;

  //---------- PHA
  RSpinBox * sbRecordLength[MaxNChannels + 1];
  RComboBox * cbDynamicRange[MaxNChannels + 1];
  RSpinBox * sbPreTrigger[MaxNChannels + 1];
  RComboBox * cbRCCR2Smoothing[MaxNChannels + 1];
  RSpinBox * sbInputRiseTime[MaxNChannels + 1];
  RSpinBox * sbThreshold[MaxNChannels + 1];
  RSpinBox * sbRiseTimeValidWin[MaxNChannels + 1];
  RSpinBox * sbTriggerHoldOff[MaxNChannels + 1];
  RSpinBox * sbShapedTrigWidth[MaxNChannels + 1];
  RSpinBox * sbDCOffset[MaxNChannels + 1];
  RComboBox * cbPolarity[MaxNChannels + 1];

  RSpinBox * sbTrapRiseTime[MaxNChannels + 1];
  RSpinBox * sbTrapFlatTop[MaxNChannels + 1];
  RSpinBox * sbDecay[MaxNChannels + 1];
  RSpinBox * sbTrapScaling[MaxNChannels + 1];
  RSpinBox * sbPeaking[MaxNChannels + 1];
  RSpinBox * sbPeakingHoldOff[MaxNChannels + 1];
  RComboBox * cbPeakAvg[MaxNChannels + 1];
  RComboBox * cBaseLineAvg[MaxNChannels + 1];
  QCheckBox * chkActiveBaseline[MaxNChannels + 1];
  QCheckBox * chkBaselineRestore[MaxNChannels + 1];
  RSpinBox * sbFineGain[MaxNChannels + 1];

  QCheckBox * chkDisableSelfTrigger[MaxNChannels + 1];
  QCheckBox * chkEnableRollOver[MaxNChannels + 1];
  QCheckBox * chkEnablePileUp[MaxNChannels + 1];
  QCheckBox * chkTagCorrelation[MaxNChannels + 1];
  RComboBox * cbDecimateTrace[MaxNChannels + 1];
  RComboBox * cbDecimateGain[MaxNChannels + 1];
  RSpinBox * sbNumEventAgg[MaxNChannels + 1];
  RComboBox * cbTriggerValid[MaxNChannels + 1];
  RComboBox * cbTrigCount[MaxNChannels + 1];
  RComboBox * cbTrigMode[MaxNChannels + 1];
  RComboBox * cbShapedTrigger[MaxNChannels + 1];
  RComboBox * cbExtra2Option[MaxNChannels + 1];
  RComboBox * cbVetoSource[MaxNChannels + 1];
  RSpinBox * sbVetoWidth[MaxNChannels + 1];
  RComboBox * cbVetoStep[MaxNChannels + 1];


  //QPushButton * bnTriggerValidMask[MaxNDigitizer][MaxNChannels/2];
};


#endif