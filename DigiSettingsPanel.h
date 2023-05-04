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
  DigiSettingsPanel(Digitizer ** digi, unsigned int nDigi, QString rawDataPath, QMainWindow * parent = nullptr);
  ~DigiSettingsPanel();

private slots:
  void UpdatePanelFromMemory();
  void ReadSettingsFromBoard();

  void SaveSetting(int opt);
  void LoadSetting();

signals:
  void SendLogMsg(const QString &msg);

private:

  void SetUpInfo(QString label, std::string value, QGridLayout *gLayout, int row, int col);
  void SetUpCheckBox(QCheckBox * &chkBox, QString label, QGridLayout *gLayout, int row, int col, Reg para, std::pair<unsigned short, unsigned short> bit, int ch = -1);
  void SetUpComboBoxBit(RComboBox * &cb, QString label, QGridLayout *gLayout, int row, int col, std::vector<std::pair<std::string, unsigned int>> items, Reg para, std::pair<unsigned short, unsigned short> bit, int colspan = 1, int ch = -1);
  void SetUpComboBox(RComboBox * &cb, QString label, QGridLayout *gLayout, int row, int col, Reg para, int ch = -1);
  void SetUpSpinBox(RSpinBox * &sb, QString label, QGridLayout *gLayout, int row, int col, Reg para, int ch = -1);

  void CleanUpGroupBox(QGroupBox * & gBox);
  void SetUpGlobalTriggerMaskAndFrontPanelMask(QGridLayout * & gLayout);

  void SetUpPHABoard();
  void SetUpPHAChannel();
  
  void SetUpPSDBoard();

  void UpdateSpinBox(RSpinBox * &sb, Reg para, int ch);
  void UpdateComboBox(RComboBox * &cb, Reg para, int ch);
  void UpdateComboBoxBit(RComboBox * &cb, uint32_t fullBit, std::pair<unsigned short, unsigned short> bit);

  void SyncSpinBox(RSpinBox *(&spb)[][MaxNChannels+1], int ch);

  void UpdatePHASetting();


  Digitizer ** digi;
  unsigned int nDigi;
  unsigned short ID;

  QString rawDataPath;
  bool enableSignalSlot;

  QTabWidget * tabWidget;

  QGroupBox * infoBox[MaxNDigitizer];
  QGridLayout * infoLayout[MaxNDigitizer];

  QLineEdit * leSaveFilePath[MaxNDigitizer];

  QPushButton * bnRefreshSetting; // read setting from board
  QPushButton * bnProgramPreDefined;
  QPushButton * bnClearBuffer;
  
  QPushButton * bnSendSoftwareTriggerSignal;
  QPushButton * bnSendSoftwareClockSyncSignal;
  QPushButton * bnSaveSettings;
  QPushButton * bnLoadSettings;
  QPushButton * bnSaveSettingsToText;

  /// ============================= Board Configure
  QGridLayout * bdCfgLayout[MaxNDigitizer];
  QGridLayout * bdACQLayout[MaxNDigitizer];
  QGridLayout * bdGlbTRGOUTLayout[MaxNDigitizer];
  QGridLayout * bdTriggerLayout[MaxNDigitizer];
  QGridLayout * bdLVDSLayout[MaxNDigitizer];

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
  RSpinBox * sbVoltageLevel[MaxNDigitizer];  

  RComboBox * cbStartStopMode[MaxNDigitizer];
  RComboBox * cbAcqStartArm[MaxNDigitizer];
  RComboBox * cbPLLRefClock[MaxNDigitizer];

  RComboBox * cbLEMOMode[MaxNDigitizer];
  RComboBox * cbTRGOUTMode[MaxNDigitizer];

  RComboBox * cbTRGINMode[MaxNDigitizer];
  RComboBox * cbTRINMezzanines[MaxNDigitizer];

  RSpinBox * sbVMEInterruptLevel[MaxNDigitizer];
  QCheckBox * chkEnableOpticalInterrupt[MaxNDigitizer];
  QCheckBox * chkEnableVMEReadoutStatus[MaxNDigitizer];
  QCheckBox * chkEnableVME64Aligment[MaxNDigitizer];
  QCheckBox * chkEnableAddRelocation[MaxNDigitizer];
  RComboBox * cbInterruptMode[MaxNDigitizer];
  QCheckBox * chkEnableExtendedBlockTransfer[MaxNDigitizer];

  /// ============================= trigger validation mask
  RComboBox * cbMaskLogic[MaxNDigitizer][MaxNChannels/2];
  RSpinBox * sbMaskMajorLevel[MaxNDigitizer][MaxNChannels/2];
  QCheckBox * chkMaskExtTrigger[MaxNDigitizer][MaxNChannels/2];
  QCheckBox * chkMaskSWTrigger[MaxNDigitizer][MaxNChannels/2];
  QPushButton * bnTriggerMask[MaxNDigitizer][MaxNChannels/2][MaxNChannels/2];

  /// ============================= board Status
  QPushButton * bnACQStatus[MaxNDigitizer][9];
  QPushButton * bnBdFailStatus[MaxNDigitizer][3];
  QPushButton * bnReadOutStatus[MaxNDigitizer][3];

  /// ============================= Mask Configure
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
  QWidget * chStatus;
  QWidget * chInput;
  QWidget * chTrap;
  QWidget * chOthers;

  RComboBox * chSelection[MaxNDigitizer];

  //---------- PHA
  RSpinBox * sbRecordLength[MaxNDigitizer][MaxNChannels + 1];
  RComboBox * cbDynamicRange[MaxNDigitizer][MaxNChannels + 1];
  RSpinBox * sbPreTrigger[MaxNDigitizer][MaxNChannels + 1];
  RComboBox * cbRCCR2Smoothing[MaxNDigitizer][MaxNChannels + 1];
  RSpinBox * sbInputRiseTime[MaxNDigitizer][MaxNChannels + 1];
  RSpinBox * sbThreshold[MaxNDigitizer][MaxNChannels + 1];
  RSpinBox * sbRiseTimeValidWin[MaxNDigitizer][MaxNChannels + 1];
  RSpinBox * sbTriggerHoldOff[MaxNDigitizer][MaxNChannels + 1];
  RSpinBox * sbShapedTrigWidth[MaxNDigitizer][MaxNChannels + 1];
  RSpinBox * sbDCOffset[MaxNDigitizer][MaxNChannels + 1];
  RComboBox * cbPolarity[MaxNDigitizer][MaxNChannels + 1];

  RSpinBox * sbTrapRiseTime[MaxNDigitizer][MaxNChannels + 1];
  RSpinBox * sbTrapFlatTop[MaxNDigitizer][MaxNChannels + 1];
  RSpinBox * sbDecay[MaxNDigitizer][MaxNChannels + 1];
  RSpinBox * sbTrapScaling[MaxNDigitizer][MaxNChannels + 1];
  RSpinBox * sbPeaking[MaxNDigitizer][MaxNChannels + 1];
  RSpinBox * sbPeakingHoldOff[MaxNDigitizer][MaxNChannels + 1];
  RComboBox * cbPeakAvg[MaxNDigitizer][MaxNChannels + 1];
  RComboBox * cbBaseLineAvg[MaxNDigitizer][MaxNChannels + 1];
  QCheckBox * chkActiveBaseline[MaxNDigitizer][MaxNChannels + 1];
  QCheckBox * chkBaselineRestore[MaxNDigitizer][MaxNChannels + 1];
  RSpinBox * sbFineGain[MaxNDigitizer][MaxNChannels + 1];

  QCheckBox * chkDisableSelfTrigger[MaxNDigitizer][MaxNChannels + 1];
  QCheckBox * chkEnableRollOver[MaxNDigitizer][MaxNChannels + 1];
  QCheckBox * chkEnablePileUp[MaxNDigitizer][MaxNChannels + 1];
  QCheckBox * chkTagCorrelation[MaxNDigitizer][MaxNChannels + 1];
  RComboBox * cbDecimateTrace[MaxNDigitizer][MaxNChannels + 1];
  RComboBox * cbDecimateGain[MaxNDigitizer][MaxNChannels + 1];
  RSpinBox * sbNumEventAgg[MaxNDigitizer][MaxNChannels + 1];
  RComboBox * cbTriggerValid[MaxNDigitizer][MaxNChannels + 1];
  RComboBox * cbTrigCount[MaxNDigitizer][MaxNChannels + 1];
  RComboBox * cbTrigMode[MaxNDigitizer][MaxNChannels + 1];
  RComboBox * cbShapedTrigger[MaxNDigitizer][MaxNChannels + 1];
  RComboBox * cbExtra2Option[MaxNDigitizer][MaxNChannels + 1];
  RComboBox * cbVetoSource[MaxNDigitizer][MaxNChannels + 1];
  RSpinBox * sbVetoWidth[MaxNDigitizer][MaxNChannels + 1];
  RComboBox * cbVetoStep[MaxNDigitizer][MaxNChannels + 1];

  QPushButton * bnChStatus[MaxNDigitizer][MaxNChannels][3];
  QLineEdit * leADCTemp[MaxNDigitizer][MaxNChannels];

};


#endif