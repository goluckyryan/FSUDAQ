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
  void SetUpSpinBox(RSpinBox * &sb, QString label, QGridLayout *gLayout, int row, int col, Reg para, int ch = -1, bool isBoard = false);

  void CleanUpGroupBox(QGroupBox * & gBox);

  void SetUpChannelMask(unsigned int digiID);
  void SetUpACQReadOutTab();
  void SetUpGlobalTriggerMaskAndFrontPanelMask(QGridLayout * & gLayout);
  void SetUpInquiryCopyTab();

  void SetUpBoard_PHA();
  void SetUpChannel_PHA();
  
  void SetUpBoard_PSD();
  void SetUpChannel_PSD();
  
  void SetUpBoard_QDC();
  void SetUpChannel_QDC();

  void UpdateSpinBox(RSpinBox * &sb, Reg para, int ch);
  void UpdateComboBox(RComboBox * &cb, Reg para, int ch);
  void UpdateComboBoxBit(RComboBox * &cb, uint32_t fullBit, std::pair<unsigned short, unsigned short> bit);

  void SyncSpinBox(RSpinBox *(&spb)[][MaxRegChannel+1]);
  void SyncComboBox(RComboBox *(&cb)[][MaxRegChannel+1]);
  void SyncCheckBox(QCheckBox *(&chk)[][MaxRegChannel+1]);

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
  QRadioButton * rbCh[MaxRegChannel]; // Copy from ch
  QCheckBox * chkCh[MaxRegChannel]; // Copy to Ch
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

  QPushButton * bnChEnableMask[MaxNDigitizer][MaxRegChannel];
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
  RComboBox * cbMaskLogic[MaxNDigitizer][MaxRegChannel/2];
  RSpinBox * sbMaskMajorLevel[MaxNDigitizer][MaxRegChannel/2];
  QCheckBox * chkMaskExtTrigger[MaxNDigitizer][MaxRegChannel/2];
  QCheckBox * chkMaskSWTrigger[MaxNDigitizer][MaxRegChannel/2];
  QPushButton * bnTriggerMask[MaxNDigitizer][MaxRegChannel/2][MaxRegChannel/2];

  /// ============================= board Status
  QPushButton * bnACQStatus[MaxNDigitizer][9];
  QPushButton * bnBdFailStatus[MaxNDigitizer][3];
  QPushButton * bnReadOutStatus[MaxNDigitizer][3];
  QLineEdit * leACQStatus[MaxNDigitizer];
  QLineEdit * leBdFailStatus[MaxNDigitizer];
  QLineEdit * leReadOutStatus[MaxNDigitizer];

  /// ============================= Mask Configure
  QPushButton * bnGlobalTriggerMask[MaxNDigitizer][MaxRegChannel/2];
  RSpinBox * sbGlbMajCoinWin[MaxNDigitizer];
  RSpinBox * sbGlbMajLvl[MaxNDigitizer];
  RComboBox * cbGlbUseOtherTriggers[MaxNDigitizer]; // combine bit 30, 31

  QPushButton * bnTRGOUTMask[MaxNDigitizer][MaxRegChannel/2];
  RSpinBox * sbTRGOUTMajLvl[MaxNDigitizer];
  RComboBox * cbTRGOUTLogic[MaxNDigitizer];
  RComboBox * cbTRGOUTUseOtherTriggers[MaxNDigitizer]; // combine bit 30, 31

  /// ============================ Channel
  QTabWidget * chTab;

  RComboBox * chSelection[MaxNDigitizer];

  //----------- common for PHA and PSD
  RSpinBox * sbRecordLength[MaxNDigitizer][MaxRegChannel + 1];
  RComboBox * cbDynamicRange[MaxNDigitizer][MaxRegChannel + 1];
  RSpinBox * sbPreTrigger[MaxNDigitizer][MaxRegChannel + 1];
  RSpinBox * sbThreshold[MaxNDigitizer][MaxRegChannel + 1];
  RSpinBox * sbDCOffset[MaxNDigitizer][MaxRegChannel + 1];
  RComboBox * cbPolarity[MaxNDigitizer][MaxRegChannel + 1];
  RSpinBox * sbShapedTrigWidth[MaxNDigitizer][MaxRegChannel + 1];
  RSpinBox * sbTriggerHoldOff[MaxNDigitizer][MaxRegChannel + 1];
  RComboBox * cbTrigMode[MaxNDigitizer][MaxRegChannel + 1];
  RComboBox * cbBaseLineAvg[MaxNDigitizer][MaxRegChannel + 1];
  RSpinBox * sbNumEventAgg[MaxNDigitizer][MaxRegChannel + 1];
  RSpinBox * sbVetoWidth[MaxNDigitizer][MaxRegChannel + 1];
  RComboBox * cbVetoStep[MaxNDigitizer][MaxRegChannel + 1];

  RComboBox * cbLocalShapedTrigger[MaxNDigitizer][MaxRegChannel + 1];
  RComboBox * cbLocalTriggerValid[MaxNDigitizer][MaxRegChannel + 1];
  RComboBox * cbExtra2Option[MaxNDigitizer][MaxRegChannel + 1];
  RComboBox * cbVetoSource[MaxNDigitizer][MaxRegChannel + 1];
  QCheckBox * chkDisableSelfTrigger[MaxNDigitizer][MaxRegChannel + 1];
  RComboBox * cbTrigCount[MaxNDigitizer][MaxRegChannel + 1];

  RComboBox * cbTRGOUTChannelProbe[MaxNDigitizer][MaxRegChannel + 1];

  //---------- PHA
  RComboBox * cbRCCR2Smoothing[MaxNDigitizer][MaxRegChannel + 1];
  RSpinBox * sbInputRiseTime[MaxNDigitizer][MaxRegChannel + 1];
  RSpinBox * sbRiseTimeValidWin[MaxNDigitizer][MaxRegChannel + 1];

  RSpinBox * sbTrapRiseTime[MaxNDigitizer][MaxRegChannel + 1];
  RSpinBox * sbTrapFlatTop[MaxNDigitizer][MaxRegChannel + 1];
  RSpinBox * sbDecay[MaxNDigitizer][MaxRegChannel + 1];
  RSpinBox * sbTrapScaling[MaxNDigitizer][MaxRegChannel + 1];
  RSpinBox * sbPeaking[MaxNDigitizer][MaxRegChannel + 1];
  RSpinBox * sbPeakingHoldOff[MaxNDigitizer][MaxRegChannel + 1];
  RComboBox * cbPeakAvg[MaxNDigitizer][MaxRegChannel + 1];
  QCheckBox * chkActiveBaseline[MaxNDigitizer][MaxRegChannel + 1];
  QCheckBox * chkBaselineRestore[MaxNDigitizer][MaxRegChannel + 1];
  RSpinBox * sbFineGain[MaxNDigitizer][MaxRegChannel + 1];

  QCheckBox * chkEnableRollOver[MaxNDigitizer][MaxRegChannel + 1];
  QCheckBox * chkEnablePileUp[MaxNDigitizer][MaxRegChannel + 1];
  QCheckBox * chkTagCorrelation[MaxNDigitizer][MaxRegChannel + 1];
  RComboBox * cbDecimateTrace[MaxNDigitizer][MaxRegChannel + 1];
  RComboBox * cbDecimateGain[MaxNDigitizer][MaxRegChannel + 1];

  //---------------- PSD
  RComboBox * cbChargeSensitivity[MaxNDigitizer][MaxRegChannel + 1];
  QCheckBox * chkChargePedestal[MaxNDigitizer][MaxRegChannel + 1];
  RComboBox * cbTriggerOpt[MaxNDigitizer][MaxRegChannel + 1];
  RComboBox * cbDiscriMode[MaxNDigitizer][MaxRegChannel + 1];
  QCheckBox * chkPileUpInGate[MaxNDigitizer][MaxRegChannel + 1];
  QCheckBox * chkTestPule[MaxNDigitizer][MaxRegChannel + 1];
  RComboBox * cbTestPulseRate[MaxNDigitizer][MaxRegChannel + 1];
  QCheckBox * chkBaseLineCal[MaxNDigitizer][MaxRegChannel + 1];
  QCheckBox * chkDiscardQLong[MaxNDigitizer][MaxRegChannel + 1];
  QCheckBox * chkRejPileUp[MaxNDigitizer][MaxRegChannel + 1];
  QCheckBox * chkCutBelow[MaxNDigitizer][MaxRegChannel + 1];
  QCheckBox * chkCutAbove[MaxNDigitizer][MaxRegChannel + 1];
  QCheckBox * chkRejOverRange[MaxNDigitizer][MaxRegChannel + 1];
  QCheckBox * chkDisableTriggerHysteresis[MaxNDigitizer][MaxRegChannel + 1];
  QCheckBox * chkDisableOppositePulse[MaxNDigitizer][MaxRegChannel + 1];

  RSpinBox * sbChargeZeroSupZero[MaxNDigitizer][MaxRegChannel + 1];
  RSpinBox * sbShortGate[MaxNDigitizer][MaxRegChannel + 1];
  RSpinBox * sbLongGate[MaxNDigitizer][MaxRegChannel + 1];
  RSpinBox * sbGateOffset[MaxNDigitizer][MaxRegChannel + 1];
  RSpinBox * sbFixedBaseline[MaxNDigitizer][MaxRegChannel + 1];
  RSpinBox * sbTriggerLatency[MaxNDigitizer][MaxRegChannel + 1];
  RSpinBox * sbPSDCutThreshold[MaxNDigitizer][MaxRegChannel + 1];
  RSpinBox * sbPURGAPThreshold[MaxNDigitizer][MaxRegChannel + 1];

  RSpinBox * sbCFDDely[MaxNDigitizer][MaxRegChannel + 1];
  RComboBox * cbCFDFraction[MaxNDigitizer][MaxRegChannel + 1];
  RComboBox * cbCFDInterpolation[MaxNDigitizer][MaxRegChannel + 1];

  RComboBox * cbSmoothedChargeIntegration[MaxNDigitizer][MaxRegChannel + 1];
  QCheckBox * chkMarkSaturation[MaxNDigitizer][MaxRegChannel + 1];
  RComboBox * cbAdditionLocalTrigValid[MaxNDigitizer][MaxRegChannel + 1];

  RComboBox * cbVetoMode[MaxNDigitizer][MaxRegChannel + 1];
  QCheckBox * chkResetTimestampByTRGIN[MaxNDigitizer][MaxRegChannel + 1];

  //------------------- QDC
  RComboBox * cbExtTriggerMode[MaxNDigitizer];
  RSpinBox * sbEventPreAgg_QDC[MaxNDigitizer];

  //...... reuse varaible
  //Gate Width          -> sbShortGate
  //Gate offset         -> sbGateOffset
  //PreTrigger          -> sbPreTrigger
  //Trig Hold off with  -> sbTriggerHoldOff
  //Trig out width      -> sbShapedTrigWidth

  //QCheckBox * chkOverthreshold[MaxNDigitizer][MaxRegChannel+1]; //TODO need firmware version 4.25 & 135.17
  //RSpinBox * sbOverThresholdWidth[MaxNDigitizer][MaxRegChannel + 1];
  QPushButton * pbSubChMask[MaxNDigitizer][MaxRegChannel+1][8];
  RSpinBox * sbSubChOffset[MaxNDigitizer][MaxRegChannel + 1][8];
  RSpinBox * sbSubChThreshold[MaxNDigitizer][MaxRegChannel + 1][8];
  QLabel   * lbSubCh[MaxNDigitizer][8];
  QLabel   * lbSubCh2[MaxNDigitizer][8];

  //---------------- channel status
  QPushButton * bnChStatus[MaxNDigitizer][MaxRegChannel][3];
  QLineEdit * leADCTemp[MaxNDigitizer][MaxRegChannel];

};


#endif