#include "DigiSettingsPanel.h"

#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>

                                                                                  // bit = 0, bit = 1
std::vector<std::pair<std::pair<QString, QString>, unsigned short>> ACQToolTip = {{{"ACQ STOP", "ACQ RUN"}, 2}, 
                                                                                  {{"No Event", "Has Events"}, 3},
                                                                                  {{"No ch is full", "At least 1 ch is full"}, 4},
                                                                                  {{"Internal Clock", "Ext. Clock"}, 5}, 
                                                                                  {{"PLL Unlocked", "PLL locked"}, 7},
                                                                                  {{"Board NOT readly", "Board Ready"}, 8},
                                                                                  {{"S-IN/GPI 0 Level", "S-IN/GPI 1 Level"}, 15},
                                                                                  {{"TRG-IN 0 Level", "TRG-IN 1 Level"}, 16},
                                                                                  {{"Channels are ON", "Channles are in shutdown"}, 19}};

std::vector<std::pair<std::pair<QString, QString>, unsigned short>> BdFailToolTip = {{{"PLL Lock OK", "PLL Lock loss"}, 4}, 
                                                                                     {{"Temperature OK", "Temperature Failure"}, 5},
                                                                                     {{"ADC Power OK", "ADC Power Down"}, 6}};

std::vector<std::pair<std::pair<QString, QString>, unsigned short>> ReadoutToolTip = {{{"No Data Ready", "Event Ready"}, 0}, 
                                                                                      {{"No Bus Error", "Bus Error"}, 2},
                                                                                      {{"FIFO not empty", "FIFO is empty"}, 3}};

DigiSettingsPanel::DigiSettingsPanel(Digitizer ** digi, unsigned int nDigi, QMainWindow *parent): QMainWindow(parent){

  this->digi = digi;
  this->nDigi = nDigi;

  enableSignalSlot = false;

  setWindowTitle("Digitizer Settings");
  setGeometry(0, 0, 1000, 800);  

  tabWidget = new QTabWidget(this);
  setCentralWidget(tabWidget);

  //@===================================== tab
  for( unsigned int iDigi = 0 ; iDigi < this->nDigi; iDigi ++ ){
  
    ID = iDigi;

    QScrollArea * scrollArea = new QScrollArea(this); 
    scrollArea->setWidgetResizable(true);
    scrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    tabWidget->addTab(scrollArea, "Digi-" + QString::number(digi[iDigi]->GetSerialNumber()));

    QWidget * tab = new QWidget(tabWidget);
    scrollArea->setWidget(tab);

    QHBoxLayout * tabLayout_H  = new QHBoxLayout(tab);

    QWidget * temp_V1 = new QWidget(this);
    tabLayout_H->addWidget(temp_V1);
    QVBoxLayout * tabLayout_V1 = new QVBoxLayout(temp_V1);

    {//^====================== Group of Digitizer Info
      infoBox[iDigi] = new QGroupBox("Board Info", tab);
      //infoBox->setSizePolicy(sizePolicy);

      infoLayout[iDigi] = new QGridLayout(infoBox[iDigi]);
      tabLayout_V1->addWidget(infoBox[iDigi]);

      SetUpInfo(    "Model ", digi[ID]->GetModelName(), infoLayout[ID], 0, 0);
      SetUpInfo( "DPP Type ", digi[ID]->GetDPPString(), infoLayout[ID], 0, 2);
      SetUpInfo("Link Type ", digi[ID]->GetLinkType() == CAEN_DGTZ_USB ? "USB" : "Optical Link" , infoLayout[ID], 0, 4);

      SetUpInfo(  "S/N No. ", std::to_string(digi[ID]->GetSerialNumber()), infoLayout[ID], 1, 0);
      SetUpInfo(  "No. Ch. ", std::to_string(digi[ID]->GetNChannels()), infoLayout[ID], 1, 2);
      SetUpInfo("Sampling Rate ", std::to_string(digi[ID]->GetCh2ns()), infoLayout[ID], 1, 4);

      SetUpInfo("ADC bit ", std::to_string(digi[ID]->GetADCBits()), infoLayout[ID], 2, 0);
      SetUpInfo("ROC version ", digi[ID]->GetROCVersion(), infoLayout[ID], 2, 2);
      SetUpInfo("AMC version ", digi[ID]->GetAMCVersion(), infoLayout[ID], 2, 4);

      uint32_t boardInfo = digi[ID]->GetSettingFromMemory(Register::DPP::BoardInfo_R);
      SetUpInfo("Family Code ", (boardInfo & 0xFF) == 0x0E ? "725 Family" : "730 Family", infoLayout[ID], 3, 0);
      SetUpInfo("Ch. Mem. Size ", ((boardInfo >> 8 ) & 0xFF) == 0x01 ? "640 kSample" : "5.12 MSample", infoLayout[ID], 3, 2);
      SetUpInfo("Board Type ", ((boardInfo >> 16) & 0xFF) == 0x10 ? "16-ch VME" : "DT, NIM, or 8-ch VME", infoLayout[ID], 3, 4);

    }

    {//^======================= Board status
      QGroupBox * boardStatusBox = new QGroupBox("Board Status", tab);
      tabLayout_V1->addWidget(boardStatusBox);
      
      QGridLayout * statusLayout = new QGridLayout(boardStatusBox);
      statusLayout->setAlignment(Qt::AlignLeft);
      statusLayout->setHorizontalSpacing(0);

      const int boxSize = 20;

      int rowID = 0; //==============================
      QLabel * acqLabel = new QLabel("ACQ status : ", boardStatusBox);
      acqLabel->setAlignment(Qt::AlignRight | Qt::AlignCenter);
      statusLayout->addWidget(acqLabel, rowID, 0);
      for( int i = 0; i < 9; i++){
        bnACQStatus[ID][i] = new QPushButton(boardStatusBox);
        bnACQStatus[ID][i]->setEnabled(false);
        bnACQStatus[ID][i]->setFixedSize(QSize(boxSize,boxSize));
        bnACQStatus[ID][i]->setToolTip(ACQToolTip[i].first.first);
        bnACQStatus[ID][i]->setToolTipDuration(-1);
        statusLayout->addWidget(bnACQStatus[ID][i], rowID, i + 1);
      }

      rowID ++; //==============================
      QLabel * bdFailLabel = new QLabel("Board Failure status : ", boardStatusBox);
      bdFailLabel->setAlignment(Qt::AlignRight | Qt::AlignCenter);
      statusLayout->addWidget(bdFailLabel, rowID, 0);
      for( int i = 0; i < 3; i++){
        bnBdFailStatus[ID][i] = new QPushButton(boardStatusBox);
        bnBdFailStatus[ID][i]->setEnabled(false);
        bnBdFailStatus[ID][i]->setFixedSize(QSize(boxSize,boxSize));
        bnBdFailStatus[ID][i]->setToolTip(BdFailToolTip[i].first.first);
        bnBdFailStatus[ID][i]->setToolTipDuration(-1);
        statusLayout->addWidget(bnBdFailStatus[ID][i], rowID, i + 1);
      }

      QLabel * ReadoutLabel = new QLabel("Readout status : ", boardStatusBox);
      ReadoutLabel->setAlignment(Qt::AlignRight | Qt::AlignCenter);
      statusLayout->addWidget(ReadoutLabel, rowID, 10);
      for( int i = 0; i < 3; i++){
        bnReadOutStatus[ID][i] = new QPushButton(boardStatusBox);
        bnReadOutStatus[ID][i]->setEnabled(false);
        bnReadOutStatus[ID][i]->setFixedSize(QSize(boxSize,boxSize));
        bnReadOutStatus[ID][i]->setToolTip(ReadoutToolTip[i].first.first);
        bnReadOutStatus[ID][i]->setToolTipDuration(-1);
        statusLayout->addWidget(bnReadOutStatus[ID][i], rowID, i + 11);
      }

    }

    {//^======================= Buttons

      QWidget * buttonsWidget = new QWidget(tab);
      tabLayout_V1->addWidget(buttonsWidget);
      QGridLayout * buttonLayout = new QGridLayout(buttonsWidget);

      bnRefreshSetting = new QPushButton("Refresh Settings", this);
      buttonLayout->addWidget(bnRefreshSetting, 0, 0);
      connect(bnRefreshSetting, &QPushButton::clicked, this, &DigiSettingsPanel::ReadSettingsFromBoard);

      bnProgramPreDefined = new QPushButton("Clear Buffer/FIFO", this);
      buttonLayout->addWidget(bnProgramPreDefined, 0, 1);
      connect(bnProgramPreDefined, &QPushButton::clicked, this, [=](){ digi[ID]->ProgramPHABoard();}); //TODO for PSD

      bnClearBuffer = new QPushButton("Clear Buffer/FIFO", this);
      buttonLayout->addWidget(bnClearBuffer, 0, 2);
      connect(bnClearBuffer, &QPushButton::clicked, this, [=](){ digi[ID]->WriteRegister(Register::DPP::SoftwareClear_W, 1);});

      bnSendSoftwareTriggerSignal = new QPushButton("Send SW Trigger Signal", this);
      buttonLayout->addWidget(bnSendSoftwareTriggerSignal, 1, 0);
      connect(bnSendSoftwareTriggerSignal, &QPushButton::clicked, this, [=](){ digi[ID]->WriteRegister(Register::DPP::SoftwareTrigger_W, 1);});

      bnSendSoftwareClockSyncSignal = new QPushButton("Send SW Clock-Sync Signal", this);
      buttonLayout->addWidget(bnSendSoftwareClockSyncSignal, 1, 1);
      connect(bnSendSoftwareClockSyncSignal, &QPushButton::clicked, this, [=](){ digi[ID]->WriteRegister(Register::DPP::SoftwareClockSync_W, 1);});

      bnSaveSettings = new QPushButton("Save Settings", this);
      buttonLayout->addWidget(bnSaveSettings, 1, 2);
      bnLoadSettings = new QPushButton("Load Settings", this);
      buttonLayout->addWidget(bnLoadSettings, 1, 3);

    }

    {//^======================= Board Settings
      boardSettingBox[iDigi] = new QGroupBox("Board Settings", tab);
      tabLayout_V1->addWidget(boardSettingBox[iDigi]);

      settingLayout[iDigi] = new QGridLayout(boardSettingBox[iDigi]);

      if( digi[ID]->GetDPPType() == V1730_DPP_PHA_CODE ) SetUpPHABoard();

    }

    {//^======================= Channel Settings
      
      QTabWidget * chTab = new QTabWidget(tab);
      tabLayout_H->addWidget(chTab);

      QScrollArea * scrollArea = new QScrollArea(this); 
      scrollArea->setWidgetResizable(true);
      scrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
      chTab->addTab(scrollArea, "Channel Settings");

    }

  }

  connect(tabWidget, &QTabWidget::currentChanged, this, [=](int index){ 
    if( index < (int) nDigi) {
      ID = index;
      // CleanUpGroupBox(boardSettingBox[ID]);
      // if( digi[ID]->GetDPPType() == V1730_DPP_PHA_CODE ) SetUpPHABoard();
      // if( digi[ID]->GetDPPType() == V1730_DPP_PSD_CODE ) SetUpPSDBoard();
      UpdatePanelFromMemory(); 
    }
  });


  ID = 0;
  UpdatePanelFromMemory();

  enableSignalSlot = true;

}

DigiSettingsPanel::~DigiSettingsPanel(){
  printf("%s \n", __func__);

}

//*================================================================
//*================================================================
void DigiSettingsPanel::SetUpInfo(QString label, std::string value, QGridLayout *gLayout, int row, int col){
  QLabel * lab = new QLabel(label, this);
  lab->setAlignment(Qt::AlignRight | Qt::AlignCenter);
  QLineEdit * leInfo = new QLineEdit(this);
  leInfo->setAlignment(Qt::AlignHCenter);
  leInfo->setReadOnly(true);
  leInfo->setText(QString::fromStdString(value));
  gLayout->addWidget(lab, row, col);
  gLayout->addWidget(leInfo, row, col + 1);
}

void DigiSettingsPanel::SetUpCheckBox(QCheckBox * &chkBox, QString label, QGridLayout *gLayout, int row, int col, Register::Reg para, std::pair<unsigned short, unsigned short> bit){

  chkBox = new QCheckBox(label, this);
  gLayout->addWidget(chkBox, row, col);

  connect(chkBox, &QCheckBox::stateChanged, this, [=](int state){
    if( !enableSignalSlot ) return;
    digi[ID]->SetBits(para, bit, state ? 1 : 0, -1);
  });

}

void DigiSettingsPanel::SetUpComboBoxBit(RComboBox * &cb, QString label, QGridLayout *gLayout, int row, int col, std::vector<std::pair<std::string, unsigned int>> items, Register::Reg para, std::pair<unsigned short, unsigned short> bit){

  QLabel * lab = new QLabel(label, this);
  lab->setAlignment(Qt::AlignRight | Qt::AlignCenter);
  gLayout->addWidget(lab, row, col);
  cb = new RComboBox(this);
  gLayout->addWidget(cb, row, col + 1);


  for(int i = 0; i < (int) items.size(); i++){
    cb->addItem(QString::fromStdString(items[i].first), items[i].second);
  }

  connect(cb, &RComboBox::currentIndexChanged, this, [=](){
    if( !enableSignalSlot ) return;
    digi[ID]->SetBits(para, bit, cb->currentData().toUInt(), -1);
  });

}
void DigiSettingsPanel::SetUpComboBox(RComboBox * &cb, QString label, QGridLayout *gLayout, int row, int col, Register::Reg para){

  QLabel * lab = new QLabel(label, this);
  lab->setAlignment(Qt::AlignRight | Qt::AlignCenter);
  gLayout->addWidget(lab, row, col);
  cb = new RComboBox(this);
  gLayout->addWidget(cb, row, col + 1);

  std::vector<std::pair<std::string, unsigned int>> items = para.GetComboList();

  for(int i = 0; i < (int) items.size(); i++){
    cb->addItem(QString::fromStdString(items[i].first), items[i].second);
  }

  connect(cb, &RComboBox::currentIndexChanged, this, [=](){
    if( !enableSignalSlot ) return;
    digi[ID]->WriteRegister(para, cb->currentData().toUInt());
  });

}

void DigiSettingsPanel::SetUpSpinBox(RSpinBox * &sb, QString label, QGridLayout *gLayout, int row, int col, Register::Reg para){
  QLabel * lab = new QLabel(label, this);
  lab->setAlignment(Qt::AlignRight | Qt::AlignCenter);
  gLayout->addWidget(lab, row, col);

  sb = new RSpinBox(this);
  gLayout->addWidget(sb, row, col + 1);

  sb->setMinimum(0);
  sb->setMaximum(para.GetMax());
  if( para.GetPartialStep() == -1 ) {
    sb->setSingleStep(1);
  }else{
    sb->setSingleStep(para.GetPartialStep() * digi[ID]->GetCh2ns());
  }

  connect(sb, &RSpinBox::valueChanged, this, [=](){
    if( !enableSignalSlot ) return;
    sb->setStyleSheet("color : blue;");
  });

  connect(sb, &RSpinBox::returnPressed, this, [=](){
    if( !enableSignalSlot ) return;

    if( sb->decimals() == 0 && sb->singleStep() != 1) {
      double step = sb->singleStep();
      double value = sb->value();
      sb->setValue( (std::round(value/step)*step));
    }

    digi[ID]->WriteRegister(para, sb->value(), -1); //TODO for each channel

  });

}

void DigiSettingsPanel::CleanUpGroupBox(QGroupBox * & gBox){

  printf("============== %s \n", __func__);

  QList<QLabel *> labelChildren1 = gBox->findChildren<QLabel *>();
  for( int i = 0; i < labelChildren1.size(); i++) delete labelChildren1[i];

  QList<QLineEdit *> labelChildren1a = gBox->findChildren<QLineEdit *>();
  for( int i = 0; i < labelChildren1a.size(); i++) delete labelChildren1a[i];
  
  QList<RComboBox *> labelChildren2 = gBox->findChildren<RComboBox *>();
  for( int i = 0; i < labelChildren2.size(); i++) delete labelChildren2[i];

  QList<RSpinBox *> labelChildren3 = gBox->findChildren<RSpinBox *>();
  for( int i = 0; i < labelChildren3.size(); i++) delete labelChildren3[i];

  QList<QCheckBox *> labelChildren4 = gBox->findChildren<QCheckBox *>();
  for( int i = 0; i < labelChildren4.size(); i++) delete labelChildren4[i];

}

void DigiSettingsPanel::SetUpPHABoard(){
  printf("============== %s \n", __func__);

  QLabel * chMaskLabel = new QLabel("Channel Mask : ", this);
  chMaskLabel->setAlignment(Qt::AlignRight | Qt::AlignCenter);
  settingLayout[ID]->addWidget(chMaskLabel, 0, 0);

  QWidget * chWiget = new QWidget(this);
  settingLayout[ID]->addWidget(chWiget, 0, 1, 1, 4);

  QGridLayout * chLayout = new QGridLayout(chWiget);
  chLayout->setAlignment(Qt::AlignLeft);
  chLayout->setSpacing(0);

  for( int i = 0; i < MaxNChannels; i++){
    bnChEnableMask[ID][i] = new QPushButton(this);
    bnChEnableMask[ID][i]->setFixedSize(QSize(20,20));
    bnChEnableMask[ID][i]->setToolTip("Ch-" + QString::number(i));
    bnChEnableMask[ID][i]->setToolTipDuration(-1);
    QLabel * chIDLabel = new QLabel(QString::number(i), this);
    chIDLabel->setAlignment(Qt::AlignHCenter);
    chLayout->addWidget(chIDLabel, 0, i);
    chLayout->addWidget(bnChEnableMask[ID][i], 1, i );

    connect(bnChEnableMask[ID][i], &QPushButton::clicked, this, [=](){
      if( !enableSignalSlot) return;

      if( bnChEnableMask[ID][i]->styleSheet() == "" ){
         bnChEnableMask[ID][i]->setStyleSheet("background-color : green;");
         digi[ID]->SetBits(Register::DPP::ChannelEnableMask, {1, i}, 1, i);
      }else{
         bnChEnableMask[ID][i]->setStyleSheet("");
         digi[ID]->SetBits(Register::DPP::ChannelEnableMask, {1, i}, 0, i);
      }
    });
  }

  SetUpCheckBox(chkAutoDataFlush[ID],        "Auto Data Flush", settingLayout[ID], 1, 0, Register::DPP::BoardConfiguration, Register::DPP::Bit_BoardConfig::EnableAutoDataFlush);
  SetUpCheckBox(chkDecimateTrace[ID],         "Decimate Trace", settingLayout[ID], 2, 0, Register::DPP::BoardConfiguration, Register::DPP::Bit_BoardConfig::DecimateTrace);
  SetUpCheckBox(chkTrigPropagation[ID],      "Trig. Propagate", settingLayout[ID], 3, 0, Register::DPP::BoardConfiguration, Register::DPP::Bit_BoardConfig::TrigPropagation);
  SetUpCheckBox(chkEnableExternalTrigger[ID], "Enable TRG-IN ", settingLayout[ID], 4, 0, Register::DPP::DisableExternalTrigger, {1, 0});
  
  SetUpCheckBox(chkDualTrace[ID],            "Dual Trace", settingLayout[ID], 1, 1, Register::DPP::BoardConfiguration, Register::DPP::Bit_BoardConfig::DualTrace);
  SetUpCheckBox(chkTraceRecording[ID],     "Record Trace", settingLayout[ID], 2, 1, Register::DPP::BoardConfiguration, Register::DPP::Bit_BoardConfig::RecordTrace);
  SetUpCheckBox(chkEnableExtra2[ID],      "Enable Extra2", settingLayout[ID], 3, 1, Register::DPP::BoardConfiguration, Register::DPP::Bit_BoardConfig::EnableExtra2);


  SetUpComboBoxBit(cbAnaProbe1[ID],   "Ana. Probe 1 ", settingLayout[ID], 1, 2, Register::DPP::Bit_BoardConfig::ListAnaProbe1, Register::DPP::BoardConfiguration, Register::DPP::Bit_BoardConfig::AnalogProbe1);
  SetUpComboBoxBit(cbAnaProbe2[ID],   "Ana. Probe 2 ", settingLayout[ID], 2, 2, Register::DPP::Bit_BoardConfig::ListAnaProbe2, Register::DPP::BoardConfiguration, Register::DPP::Bit_BoardConfig::AnalogProbe2);
  SetUpComboBoxBit(cbDigiProbe1[ID], "Digi. Probe 1 ", settingLayout[ID], 3, 2, Register::DPP::Bit_BoardConfig::ListDigiProbe1, Register::DPP::BoardConfiguration, Register::DPP::Bit_BoardConfig::DigiProbel1);
  SetUpComboBoxBit(cbDigiProbe2[ID], "Digi. Probe 2 ", settingLayout[ID], 4, 2, {{"trigger", 0}}, Register::DPP::BoardConfiguration, Register::DPP::Bit_BoardConfig::DigiProbel2);
  cbDigiProbe2[ID]->setEnabled(false);

  SetUpSpinBox(sbAggNum[ID],         "Agg. Num. / read", settingLayout[ID], 5, 0, Register::DPP::MaxAggregatePerBlockTransfer);
  SetUpComboBox(cbAggOrg[ID], "Aggregate Organization ", settingLayout[ID], 6, 0, Register::DPP::AggregateOrganization);

  SetUpComboBoxBit(cbStartStopMode[ID], "Start/Stop Mode ", settingLayout[ID], 7, 0, {{"SW controlled", 0},
                                                                                      {"S-IN/GPI controlled", 1},
                                                                                      {"1st Trigger", 2},
                                                                                      {"LVDS controlled", 3}},
                                                                                   Register::DPP::AcquisitionControl, Register::DPP::Bit_AcquistionControl::StartStopMode);

  SetUpComboBoxBit(cbAcqStartArm[ID], "Acq Start/Arm ", settingLayout[ID], 8, 0, {{"ACQ STOP", 0},
                                                                                   {"ACQ RUN", 1}},Register::DPP::AcquisitionControl, Register::DPP::Bit_AcquistionControl::ACQStartArm);

  SetUpComboBoxBit(cbPLLRefClock[ID], "PLL Ref. Clock ", settingLayout[ID], 5, 2, {{"Internal 50 MHz", 0},{"Ext. CLK-IN", 1}}, Register::DPP::AcquisitionControl, Register::DPP::Bit_AcquistionControl::ACQStartArm);


  SetUpSpinBox(sbRunDelay[ID], "Run Delay [ns] ", settingLayout[ID], 6, 2, Register::DPP::RunStartStopDelay);

  SetUpComboBox(cbAnalogMonitorMode[ID], "Analog Monitor Mode ", settingLayout[ID], 7, 2, Register::DPP::AnalogMonitorMode);

  SetUpSpinBox(sbBufferGain[ID], "Buffer Occup. Gain ", settingLayout[ID], 8, 2, Register::DPP::BufferOccupancyGain);

}


void DigiSettingsPanel::SetUpPSDBoard(){

}

void DigiSettingsPanel::UpdatePanelFromMemory(){

  printf("============== %s \n", __func__);

  enableSignalSlot = false;

  uint32_t AcqStatus = digi[ID]->GetSettingFromMemory(Register::DPP::AcquisitionStatus_R);
  for( int i = 0; i < 9; i++){
    if( Digitizer::ExtractBits(AcqStatus, {1, ACQToolTip[i].second}) == 0 ){
      bnACQStatus[ID][i]->setStyleSheet("");
      bnACQStatus[ID][i]->setToolTip(ACQToolTip[i].first.first);
      if(ACQToolTip[i].second == 19 )  bnACQStatus[ID][i]->setStyleSheet("background-color: green;");
    }else{
      bnACQStatus[ID][i]->setStyleSheet("background-color: green;");
      bnACQStatus[ID][i]->setToolTip(ACQToolTip[i].first.second);
      if(ACQToolTip[i].second == 19 )  bnACQStatus[ID][i]->setStyleSheet("");
    }
  }

  uint32_t BdFailStatus = digi[ID]->GetSettingFromMemory(Register::DPP::BoardFailureStatus_R);
  for( int i = 0; i < 3; i++){
    if( Digitizer::ExtractBits(BdFailStatus, {1, BdFailToolTip[i].second}) == 0 ){
      bnBdFailStatus[ID][i]->setStyleSheet("background-color: green;");
      bnBdFailStatus[ID][i]->setToolTip(BdFailToolTip[i].first.first);
    }else{
      bnBdFailStatus[ID][i]->setStyleSheet("background-color: red;");
      bnBdFailStatus[ID][i]->setToolTip(BdFailToolTip[i].first.second);
    }
  }

  uint32_t ReadoutStatus = digi[ID]->GetSettingFromMemory(Register::DPP::ReadoutStatus_R);
  for( int i = 0; i < 3; i++){
    if( Digitizer::ExtractBits(ReadoutStatus, {1, ReadoutToolTip[i].second}) == 0 ){
      if( ReadoutToolTip[i].second != 2 ) {
        bnReadOutStatus[ID][i]->setStyleSheet("");
      }else{
        bnReadOutStatus[ID][i]->setStyleSheet("background-color: green;");
      }
      bnReadOutStatus[ID][i]->setToolTip(ReadoutToolTip[i].first.first);
    }else{
      if( ReadoutToolTip[i].second != 2 ) {
        bnReadOutStatus[ID][i]->setStyleSheet("background-color: green;");
      }else{
        bnReadOutStatus[ID][i]->setStyleSheet("background-color: red;");
      }
      bnReadOutStatus[ID][i]->setToolTip(ReadoutToolTip[i].first.second);
    }
  }

  uint32_t BdCfg = digi[ID]->GetSettingFromMemory(Register::DPP::BoardConfiguration);

  chkAutoDataFlush[ID]->setChecked(   Digitizer::ExtractBits(BdCfg, Register::DPP::Bit_BoardConfig::EnableAutoDataFlush) );
  chkDecimateTrace[ID]->setChecked(   Digitizer::ExtractBits(BdCfg, Register::DPP::Bit_BoardConfig::DecimateTrace) );
  chkTrigPropagation[ID]->setChecked( Digitizer::ExtractBits(BdCfg, Register::DPP::Bit_BoardConfig::TrigPropagation) );
  chkDualTrace[ID]->setChecked(       Digitizer::ExtractBits(BdCfg, Register::DPP::Bit_BoardConfig::DualTrace) );
  chkTraceRecording[ID]->setChecked(  Digitizer::ExtractBits(BdCfg, Register::DPP::Bit_BoardConfig::TrigPropagation) );
  chkEnableExtra2[ID]->setChecked(    Digitizer::ExtractBits(BdCfg, Register::DPP::Bit_BoardConfig::EnableExtra2) );

  int temp = Digitizer::ExtractBits(BdCfg, Register::DPP::Bit_BoardConfig::AnalogProbe1);
  for( int i = 0; i < cbAnaProbe1[ID]->count(); i++){
    if( cbAnaProbe1[ID]->itemData(i).toInt() == temp) {
      cbAnaProbe1[ID]->setCurrentIndex(i);
      break;
    }
  }

  temp = Digitizer::ExtractBits(BdCfg, Register::DPP::Bit_BoardConfig::AnalogProbe2);
  for(int i = 0; i < cbAnaProbe2[ID]->count(); i++){
    if( cbAnaProbe2[ID]->itemData(i).toInt() == temp) {
      cbAnaProbe2[ID]->setCurrentIndex(i);
      break;
    }
  }
  temp = Digitizer::ExtractBits(BdCfg, Register::DPP::Bit_BoardConfig::DigiProbel1);
  for(int i = 0; i < cbDigiProbe1[ID]->count(); i++){
    if( cbDigiProbe1[ID]->itemData(i).toInt() == temp) {
      cbDigiProbe1[ID]->setCurrentIndex(i);
      break;
    }
  }

  uint32_t chMask = digi[ID]->GetSettingFromMemory(Register::DPP::ChannelEnableMask);
  for( int i = 0; i < MaxNChannels; i++){
    if( (chMask >> i ) & 0x1 ) {
      bnChEnableMask[ID][i]->setStyleSheet("background-color: green;");
    }else{
      bnChEnableMask[ID][i]->setStyleSheet("");
    }
  }

  uint32_t aggOrg = digi[ID]->GetSettingFromMemory(Register::DPP::AggregateOrganization);
  for( int i = 0; i < cbAggOrg[ID]->count(); i++){
    if( cbAggOrg[ID]->itemData(i).toUInt() == aggOrg ){
      cbAggOrg[ID]->setCurrentIndex(i);
      break;
    }
  }

  sbAggNum[ID]->setValue(digi[ID]->GetSettingFromMemory(Register::DPP::MaxAggregatePerBlockTransfer));

  chkEnableExternalTrigger[ID]->setChecked( ! ( digi[ID]->GetSettingFromMemory(Register::DPP::DisableExternalTrigger) & 0x1) );

  sbRunDelay[ID]->setValue(digi[ID]->GetSettingFromMemory(Register::DPP::RunStartStopDelay));

  uint32_t anaMonitor = digi[ID]->GetSettingFromMemory(Register::DPP::AnalogMonitorMode);
  for( int i = 0 ; i < cbAnalogMonitorMode[ID]->count(); i++){
    if( cbAnalogMonitorMode[ID]->itemData(i).toUInt() == anaMonitor ){ 
      cbAnalogMonitorMode[ID]->setCurrentIndex(i);
      break;
    }
  }

  sbBufferGain[ID]->setValue(digi[ID]->GetSettingFromMemory(Register::DPP::BufferOccupancyGain));

  enableSignalSlot = true;

  printf("============== end of %s \n", __func__);

}

void DigiSettingsPanel::ReadSettingsFromBoard(){

  digi[ID]->ReadAllSettingsFromBoard();

  UpdatePanelFromMemory();

}


//*================================================================
//*================================================================


