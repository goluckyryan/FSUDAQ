#ifndef DigiSettings_H
#define DigiSettings_H

#include <QMainWindow>
#include <QTabWidget>
#include <QLineEdit>
#include <QGridLayout>
#include <QCheckBox>
#include <QGroupBox>
#include <QPushButton>
#include <QRadioButton>

#include "macro.h"
#include "ClassDigitizer.h"
#include "CustomWidgets.h"

class DigiSettingsPanel : public QMainWindow{
  Q_OBJECT

public:
  DigiSettingsPanel(Digitizer ** digi, unsigned int nDigi, QString rawDataPath, QMainWindow * parent = nullptr);
  ~DigiSettingsPanel();

public slots:
  void UpdatePanelFromMemory();
  void ReadSettingsFromBoard();

  void SaveSetting(int opt);
  void LoadSetting();

signals:
  void SendLogMsg(const QString &msg);
  void UpdateOtherPanels();

private:

  void SetUpInfo(QString label, std::string value, QGridLayout *gLayout, int row, int col);
  void SetUpCheckBox(QCheckBox * &chkBox, QString label, QGridLayout *gLayout, int row, int col, Reg para, std::pair<unsigned short, unsigned short> bit, int ch = -1, int colSpan = 1);
  void SetUpComboBoxBit(RComboBox * &cb, QString label, QGridLayout *gLayout, int row, int col, std::vector<std::pair<std::string, unsigned int>> items, Reg para, std::pair<unsigned short, unsigned short> bit, int colspan = 1, int ch = -1);
  void SetUpComboBox(RComboBox * &cb, QString label, QGridLayout *gLayout, int row, int col, Reg para, int ch = -1);
  void SetUpSpinBox(RSpinBox * &sb, QString label, QGridLayout *gLayout, int row, int col, Reg para, int ch = -1);

  void CleanUpGroupBox(QGroupBox * & gBox);

  void SetUpChannelMask(unsigned int digiID);
  void SetUpACQReadOutTab();
  void SetUpGlobalTriggerMaskAndFrontPanelMask(QGridLayout * & gLayout);
  void SetUpInquiryCopyTab();

  void SetUpPHABoard();
  void SetUpPHAChannel();
  
  void SetUpPSDBoard();
  void SetUpPSDChannel();

  void UpdateSpinBox(RSpinBox * &sb, Reg para, int ch);
  void UpdateComboBox(RComboBox * &cb, Reg para, int ch);
  void UpdateComboBoxBit(RComboBox * &cb, uint32_t fullBit, std::pair<unsigned short, unsigned short> bit);

  void SyncSpinBox(RSpinBox *(&spb)[][MaxNChannels+1]);
  void SyncComboBox(RComboBox *(&cb)[][MaxNChannels+1]);
  void SyncCheckBox(QCheckBox *(&chk)[][MaxNChannels+1]);

  void UpdateBoardAndChannelsStatus(); // ReadRegister

  void SyncAllChannelsTab_PHA();
  void UpdateSettings_PHA();
  void SyncAllChannelsTab_PSD();
  void UpdateSettings_PSD();
  void SyncAllChannelsTab_QDC();
  void UpdateSettings_QDC();

  void CheckRadioAndCheckedButtons();

  Digitizer ** digi;
  unsigned int nDigi;
  unsigned short ID;

  std::vector<Reg> chRegList; 

  RComboBox * cbFromBoard;
  RComboBox * cbToBoard;
  QRadioButton * rbCh[MaxNChannels]; // Copy from ch
  QCheckBox * chkCh[MaxNChannels]; // Copy to Ch
  QPushButton * bnCopyBoard;
  QPushButton * bnCopyChannel;

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
  QLineEdit * leACQStatus[MaxNDigitizer];
  QLineEdit * leBdFailStatus[MaxNDigitizer];
  QLineEdit * leReadOutStatus[MaxNDigitizer];

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
  QTabWidget * chTab;

  RComboBox * chSelection[MaxNDigitizer];

  //----------- common for PHA and PSD
  RSpinBox * sbRecordLength[MaxNDigitizer][MaxNChannels + 1];
  RComboBox * cbDynamicRange[MaxNDigitizer][MaxNChannels + 1];
  RSpinBox * sbPreTrigger[MaxNDigitizer][MaxNChannels + 1];
  RSpinBox * sbThreshold[MaxNDigitizer][MaxNChannels + 1];
  RSpinBox * sbDCOffset[MaxNDigitizer][MaxNChannels + 1];
  RComboBox * cbPolarity[MaxNDigitizer][MaxNChannels + 1];
  RSpinBox * sbShapedTrigWidth[MaxNDigitizer][MaxNChannels + 1];
  RSpinBox * sbTriggerHoldOff[MaxNDigitizer][MaxNChannels + 1];
  RComboBox * cbTrigMode[MaxNDigitizer][MaxNChannels + 1];
  RComboBox * cbBaseLineAvg[MaxNDigitizer][MaxNChannels + 1];
  RSpinBox * sbNumEventAgg[MaxNDigitizer][MaxNChannels + 1];
  RSpinBox * sbVetoWidth[MaxNDigitizer][MaxNChannels + 1];
  RComboBox * cbVetoStep[MaxNDigitizer][MaxNChannels + 1];

  RComboBox * cbLocalShapedTrigger[MaxNDigitizer][MaxNChannels + 1];
  RComboBox * cbLocalTriggerValid[MaxNDigitizer][MaxNChannels + 1];
  RComboBox * cbExtra2Option[MaxNDigitizer][MaxNChannels + 1];
  RComboBox * cbVetoSource[MaxNDigitizer][MaxNChannels + 1];
  QCheckBox * chkDisableSelfTrigger[MaxNDigitizer][MaxNChannels + 1];
  RComboBox * cbTrigCount[MaxNDigitizer][MaxNChannels + 1];

  RComboBox * cbTRGOUTChannelProbe[MaxNDigitizer][MaxNChannels + 1];

  //---------- PHA
  RComboBox * cbRCCR2Smoothing[MaxNDigitizer][MaxNChannels + 1];
  RSpinBox * sbInputRiseTime[MaxNDigitizer][MaxNChannels + 1];
  RSpinBox * sbRiseTimeValidWin[MaxNDigitizer][MaxNChannels + 1];

  RSpinBox * sbTrapRiseTime[MaxNDigitizer][MaxNChannels + 1];
  RSpinBox * sbTrapFlatTop[MaxNDigitizer][MaxNChannels + 1];
  RSpinBox * sbDecay[MaxNDigitizer][MaxNChannels + 1];
  RSpinBox * sbTrapScaling[MaxNDigitizer][MaxNChannels + 1];
  RSpinBox * sbPeaking[MaxNDigitizer][MaxNChannels + 1];
  RSpinBox * sbPeakingHoldOff[MaxNDigitizer][MaxNChannels + 1];
  RComboBox * cbPeakAvg[MaxNDigitizer][MaxNChannels + 1];
  QCheckBox * chkActiveBaseline[MaxNDigitizer][MaxNChannels + 1];
  QCheckBox * chkBaselineRestore[MaxNDigitizer][MaxNChannels + 1];
  RSpinBox * sbFineGain[MaxNDigitizer][MaxNChannels + 1];

  QCheckBox * chkEnableRollOver[MaxNDigitizer][MaxNChannels + 1];
  QCheckBox * chkEnablePileUp[MaxNDigitizer][MaxNChannels + 1];
  QCheckBox * chkTagCorrelation[MaxNDigitizer][MaxNChannels + 1];
  RComboBox * cbDecimateTrace[MaxNDigitizer][MaxNChannels + 1];
  RComboBox * cbDecimateGain[MaxNDigitizer][MaxNChannels + 1];

  //---------------- PSD
  RComboBox * cbChargeSensitivity[MaxNDigitizer][MaxNChannels + 1];
  QCheckBox * chkChargePedestal[MaxNDigitizer][MaxNChannels + 1];
  RComboBox * cbTriggerOpt[MaxNDigitizer][MaxNChannels + 1];
  RComboBox * cbDiscriMode[MaxNDigitizer][MaxNChannels + 1];
  QCheckBox * chkPileUpInGate[MaxNDigitizer][MaxNChannels + 1];
  QCheckBox * chkTestPule[MaxNDigitizer][MaxNChannels + 1];
  RComboBox * cbTestPulseRate[MaxNDigitizer][MaxNChannels + 1];
  QCheckBox * chkBaseLineCal[MaxNDigitizer][MaxNChannels + 1];
  QCheckBox * chkDiscardQLong[MaxNDigitizer][MaxNChannels + 1];
  QCheckBox * chkRejPileUp[MaxNDigitizer][MaxNChannels + 1];
  QCheckBox * chkCutBelow[MaxNDigitizer][MaxNChannels + 1];
  QCheckBox * chkCutAbove[MaxNDigitizer][MaxNChannels + 1];
  QCheckBox * chkRejOverRange[MaxNDigitizer][MaxNChannels + 1];
  QCheckBox * chkDisableTriggerHysteresis[MaxNDigitizer][MaxNChannels + 1];
  QCheckBox * chkDisableOppositePulse[MaxNDigitizer][MaxNChannels + 1];

  RSpinBox * sbChargeZeroSupZero[MaxNDigitizer][MaxNChannels + 1];
  RSpinBox * sbShortGate[MaxNDigitizer][MaxNChannels + 1];
  RSpinBox * sbLongGate[MaxNDigitizer][MaxNChannels + 1];
  RSpinBox * sbGateOffset[MaxNDigitizer][MaxNChannels + 1];
  RSpinBox * sbFixedBaseline[MaxNDigitizer][MaxNChannels + 1];
  RSpinBox * sbTriggerLatency[MaxNDigitizer][MaxNChannels + 1];
  RSpinBox * sbPSDCutThreshold[MaxNDigitizer][MaxNChannels + 1];
  RSpinBox * sbPURGAPThreshold[MaxNDigitizer][MaxNChannels + 1];

  RSpinBox * sbCFDDely[MaxNDigitizer][MaxNChannels + 1];
  RComboBox * cbCFDFraction[MaxNDigitizer][MaxNChannels + 1];
  RComboBox * cbCFDInterpolation[MaxNDigitizer][MaxNChannels + 1];

  RComboBox * cbSmoothedChargeIntegration[MaxNDigitizer][MaxNChannels + 1];
  QCheckBox * chkMarkSaturation[MaxNDigitizer][MaxNChannels + 1];
  RComboBox * cbAdditionLocalTrigValid[MaxNDigitizer][MaxNChannels + 1];

  RComboBox * cbVetoMode[MaxNDigitizer][MaxNChannels + 1];
  QCheckBox * chkResetTimestampByTRGIN[MaxNDigitizer][MaxNChannels + 1];



  //---------------- channel status
  QPushButton * bnChStatus[MaxNDigitizer][MaxNChannels][3];
  QLineEdit * leADCTemp[MaxNDigitizer][MaxNChannels];

};


#endif