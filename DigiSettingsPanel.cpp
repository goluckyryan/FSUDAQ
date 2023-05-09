#include "DigiSettingsPanel.h"

#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QDir>
#include <QFileDialog>

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

DigiSettingsPanel::DigiSettingsPanel(Digitizer ** digi, unsigned int nDigi, QString rawDataPath, QMainWindow *parent): QMainWindow(parent){

  this->digi = digi;
  this->nDigi = nDigi;

  this->rawDataPath = rawDataPath;

  enableSignalSlot = false;

  setWindowTitle("Digitizer Settings");
  setGeometry(0, 0, 1600, 850);  

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
    tabLayout_V1->setSpacing(0);

    {//^====================== Group of Digitizer Info
      infoBox[iDigi] = new QGroupBox("Board Info", tab);
      tabLayout_V1->addWidget(infoBox[iDigi]);
      
      infoLayout[iDigi] = new QGridLayout(infoBox[iDigi]);
      infoLayout[iDigi]->setSpacing(2);

      SetUpInfo(    "Model ", digi[ID]->GetModelName(), infoLayout[ID], 0, 0);
      SetUpInfo( "DPP Type ", digi[ID]->GetDPPString(), infoLayout[ID], 0, 2);
      SetUpInfo("Link Type ", digi[ID]->GetLinkType() == CAEN_DGTZ_USB ? "USB" : "Optical Link" , infoLayout[ID], 0, 4);

      SetUpInfo(  "S/N No. ", std::to_string(digi[ID]->GetSerialNumber()), infoLayout[ID], 1, 0);
      SetUpInfo(  "No. Ch. ", std::to_string(digi[ID]->GetNChannels()), infoLayout[ID], 1, 2);
      SetUpInfo("Sampling Rate ", std::to_string((int) digi[ID]->GetCh2ns()) + " ns = " + std::to_string( (int) (1000/digi[ID]->GetCh2ns())) + " MHz" , infoLayout[ID], 1, 4);

      SetUpInfo("ADC bit ", std::to_string(digi[ID]->GetADCBits()), infoLayout[ID], 2, 0);
      SetUpInfo("ROC version ", digi[ID]->GetROCVersion(), infoLayout[ID], 2, 2);
      SetUpInfo("AMC version ", digi[ID]->GetAMCVersion(), infoLayout[ID], 2, 4);

      uint32_t boardInfo = digi[ID]->GetSettingFromMemory(DPP::BoardInfo_R);
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
      buttonLayout->setSpacing(2);

      int rowID = 0 ;
      QLabel * lbSavePath = new QLabel("Save File Path : ", this);
      lbSavePath->setAlignment(Qt::AlignRight | Qt::AlignCenter);
      buttonLayout->addWidget(lbSavePath, rowID, 0);
      
      leSaveFilePath[iDigi] = new QLineEdit(this);
      leSaveFilePath[iDigi]->setReadOnly(true);
      buttonLayout->addWidget(leSaveFilePath[iDigi], rowID, 1, 1, 3);

      rowID ++; //---------------------------
      bnRefreshSetting = new QPushButton("Refresh Settings", this);
      buttonLayout->addWidget(bnRefreshSetting, rowID, 0);
      connect(bnRefreshSetting, &QPushButton::clicked, this, &DigiSettingsPanel::ReadSettingsFromBoard);

      bnProgramPreDefined = new QPushButton("Program Default", this);
      buttonLayout->addWidget(bnProgramPreDefined, rowID, 1);
      connect(bnProgramPreDefined, &QPushButton::clicked, this, [=](){ digi[ID]->ProgramPHABoard();}); //TODO for PSD

      bnClearBuffer = new QPushButton("Clear Buffer/FIFO", this);
      buttonLayout->addWidget(bnClearBuffer, rowID, 2);
      connect(bnClearBuffer, &QPushButton::clicked, this, [=](){ digi[ID]->WriteRegister(DPP::SoftwareClear_W, 1);});

      bnLoadSettings = new QPushButton("Load Settings", this);
      buttonLayout->addWidget(bnLoadSettings, rowID, 3);
      connect(bnLoadSettings, &QPushButton::clicked, this, &DigiSettingsPanel::LoadSetting);

      rowID ++; //---------------------------
      bnSendSoftwareTriggerSignal = new QPushButton("Send SW Trigger Signal", this);
      buttonLayout->addWidget(bnSendSoftwareTriggerSignal, rowID, 0);
      connect(bnSendSoftwareTriggerSignal, &QPushButton::clicked, this, [=](){ digi[ID]->WriteRegister(DPP::SoftwareTrigger_W, 1);});

      bnSendSoftwareClockSyncSignal = new QPushButton("Send SW Clock-Sync Signal", this);
      buttonLayout->addWidget(bnSendSoftwareClockSyncSignal, rowID, 1);
      connect(bnSendSoftwareClockSyncSignal, &QPushButton::clicked, this, [=](){ digi[ID]->WriteRegister(DPP::SoftwareClockSync_W, 1);});

      bnSaveSettings = new QPushButton("Save Settings (bin)", this);
      buttonLayout->addWidget(bnSaveSettings, rowID, 2);
      connect(bnSaveSettings, &QPushButton::clicked, this, [=](){ SaveSetting(0);});

      bnSaveSettingsToText = new QPushButton("Save Settings (txt)", this);
      buttonLayout->addWidget(bnSaveSettingsToText, rowID, 3);
      connect(bnSaveSettingsToText, &QPushButton::clicked, this, [=](){ SaveSetting(1);});
    }

    {//^======================= Board Settings

      QTabWidget * bdTab = new QTabWidget(tab);
      tabLayout_V1->addWidget(bdTab);

      QWidget * bdCfg = new QWidget(this);
      bdTab->addTab(bdCfg, "Board");
      bdCfgLayout[iDigi] = new QGridLayout(bdCfg);
      bdCfgLayout[iDigi]->setAlignment(Qt::AlignTop);
      bdCfgLayout[iDigi]->setSpacing(2);

      QWidget * bdACQ = new QWidget(this);
      bdTab->addTab(bdACQ, "ACQ / Readout");
      bdACQLayout[iDigi] = new QGridLayout(bdACQ);
      bdACQLayout[iDigi]->setAlignment(Qt::AlignTop);
      bdACQLayout[iDigi]->setSpacing(2);

      QWidget * bdGlbTrgOUTMask = new QWidget(this);
      bdTab->addTab(bdGlbTrgOUTMask, "Global / TRG-OUT / TRG-IN");
      bdGlbTRGOUTLayout[iDigi] = new QGridLayout(bdGlbTrgOUTMask);
      bdGlbTRGOUTLayout[iDigi]->setAlignment(Qt::AlignTop);
      bdGlbTRGOUTLayout[iDigi]->setSpacing(2);

      QWidget * bdTriggerMask = new QWidget(this);
      bdTab->addTab(bdTriggerMask, "Trigger Mask");
      bdTriggerLayout[iDigi] = new QGridLayout(bdTriggerMask);
      bdTriggerLayout[iDigi]->setAlignment(Qt::AlignTop);
      bdTriggerLayout[iDigi]->setSpacing(2);

      QWidget * bdLVDS = new QWidget(this);
      bdTab->addTab(bdLVDS, "LVDS");
      bdLVDSLayout[iDigi] = new QGridLayout(bdLVDS);
      bdLVDSLayout[iDigi]->setAlignment(Qt::AlignTop);
      bdLVDSLayout[iDigi]->setSpacing(2);

      QLabel * lbLVDSInfo = new QLabel(" LDVS settings will be implement later. ", bdLVDS);
      bdLVDSLayout[iDigi]->addWidget(lbLVDSInfo);

      SetUpChannelMask();

      if( digi[ID]->GetDPPType() == V1730_DPP_PHA_CODE ) SetUpPHABoard();
      if( digi[ID]->GetDPPType() == V1730_DPP_PSD_CODE ) SetUpPSDBoard();

      SetUpGlobalTriggerMaskAndFrontPanelMask(bdGlbTRGOUTLayout[iDigi]);
      SetUpACQReadOutTab();

    }

    {//^======================= Channel Settings
      QWidget * temp_V2 = new QWidget(this);
      tabLayout_H->addWidget(temp_V2);
      QVBoxLayout * tabLayout_V2 = new QVBoxLayout(temp_V2);
      tabLayout_V2->setSpacing(0);
      
      chTab = new QTabWidget(tab);
      tabLayout_V2->addWidget(chTab);

      if( digi[iDigi]->GetDPPType() == V1730_DPP_PHA_CODE) SetUpPHAChannel();
      if( digi[iDigi]->GetDPPType() == V1730_DPP_PSD_CODE) SetUpPSDChannel();

    }

  }

  //TODO ----- Copy settings tab


  connect(tabWidget, &QTabWidget::currentChanged, this, [=](int index){ 
    if( index < (int) nDigi) {
      ID = index;
      //if( digi[ID]->GetDPPType() == V1730_DPP_PHA_CODE ) UpdatePanelFromMemory(); 
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

void DigiSettingsPanel::SetUpCheckBox(QCheckBox * &chkBox, QString label, QGridLayout *gLayout, int row, int col, Reg para, std::pair<unsigned short, unsigned short> bit, int ch, int colSpan){

  chkBox = new QCheckBox(label, this);
  gLayout->addWidget(chkBox, row, col, 1, colSpan);

  connect(chkBox, &QCheckBox::stateChanged, this, [=](int state){
    if( !enableSignalSlot ) return;

    int chID = ch < 0 ? chSelection[ID]->currentData().toInt() : ch;
    digi[ID]->SetBits(para, bit, state ? 1 : 0, chID);
    if( para.IsCoupled() == true && chID >= 0 ) digi[ID]->SetBits(para, bit, state ? 1 : 0, chID%2 == 0 ? chID + 1 : chID - 1);
    UpdatePanelFromMemory();
  });

}

void DigiSettingsPanel::SetUpComboBoxBit(RComboBox * &cb, QString label, QGridLayout *gLayout, int row, int col, std::vector<std::pair<std::string, unsigned int>> items, Reg para, std::pair<unsigned short, unsigned short> bit, int colspan, int ch){

  QLabel * lab = new QLabel(label, this);
  lab->setAlignment(Qt::AlignRight | Qt::AlignCenter);
  gLayout->addWidget(lab, row, col);
  cb = new RComboBox(this);
  gLayout->addWidget(cb, row, col + 1, 1, colspan);

  if( ch < 0 ) cb->addItem("", -999);

  for(int i = 0; i < (int) items.size(); i++){
    cb->addItem(QString::fromStdString(items[i].first), items[i].second);
  }

  connect(cb, &RComboBox::currentIndexChanged, this, [=](){
    if( !enableSignalSlot ) return;

    if( ch == -1 && cb->currentText() == "" ) return;

    int chID = ch < 0 ? chSelection[ID]->currentData().toInt() : ch;

    digi[ID]->SetBits(para, bit, cb->currentData().toUInt(), chID);
    if( para.IsCoupled() == true && chID >= 0  ) digi[ID]->SetBits(para, bit, cb->currentData().toUInt(), chID%2 == 0 ? chID + 1 : chID - 1);
    UpdatePanelFromMemory();
  });

}

void DigiSettingsPanel::SetUpComboBox(RComboBox * &cb, QString label, QGridLayout *gLayout, int row, int col, Reg para, int ch){

  QLabel * lab = new QLabel(label, this);
  lab->setAlignment(Qt::AlignRight | Qt::AlignCenter);
  gLayout->addWidget(lab, row, col);
  cb = new RComboBox(this);
  gLayout->addWidget(cb, row, col + 1);

  if( ch < 0 ) cb->addItem("", -999);

  std::vector<std::pair<std::string, unsigned int>> items = para.GetComboList();
  for(int i = 0; i < (int) items.size(); i++){
    cb->addItem(QString::fromStdString(items[i].first), items[i].second);
  }

  connect(cb, &RComboBox::currentIndexChanged, this, [=](){
    if( !enableSignalSlot ) return;

    if( ch == -1 && cb->currentText() == "" ) return;

    int chID = ch < 0 ? chSelection[ID]->currentData().toInt() : ch;
    digi[ID]->WriteRegister(para, cb->currentData().toUInt(), chID);
    if( para.IsCoupled() == true && chID >= 0  ) digi[ID]->WriteRegister(para, cb->currentData().toUInt(), chID%2 == 0 ? chID + 1 : chID - 1);
    UpdatePanelFromMemory();
  });

}

void DigiSettingsPanel::SetUpSpinBox(RSpinBox * &sb, QString label, QGridLayout *gLayout, int row, int col, Reg para, int ch){
  QLabel * lab = new QLabel(label, this);
  lab->setAlignment(Qt::AlignRight | Qt::AlignCenter);
  gLayout->addWidget(lab, row, col);

  sb = new RSpinBox(this);
  gLayout->addWidget(sb, row, col + 1);

  sb->setMinimum(0);
  if( ch == -1 ) sb->setMaximum(-1);

  if( para.GetPartialStep() == -1 ) {
    sb->setSingleStep(1);
    sb->setMaximum(para.GetMaxBit());
  }else{
    sb->setMaximum(para.GetMaxBit() * para.GetPartialStep() * digi[ID]->GetCh2ns());
    sb->setSingleStep(para.GetPartialStep() * digi[ID]->GetCh2ns());
  }

  connect(sb, &RSpinBox::valueChanged, this, [=](){
    if( !enableSignalSlot ) return;
    sb->setStyleSheet("color : blue;");
  });

  connect(sb, &RSpinBox::returnPressed, this, [=](){
    if( !enableSignalSlot ) return;

    if( ch == -1 && sb->value() == -1 ) return;

    if( sb->decimals() == 0 && sb->singleStep() != 1) {
      double step = sb->singleStep();
      double value = sb->value();
      sb->setValue( (std::round(value/step)*step));
    }

    sb->setStyleSheet("");

    int chID = ch < 0 ? chSelection[ID]->currentData().toInt() : ch;

    if( para == DPP::ChannelDCOffset ){
      digi[ID]->WriteRegister(para, 0xFFFF * (1.0 - sb->value() / 100. ), chID);
      UpdatePanelFromMemory();
      return;
    }

    uint32_t bit = para.GetPartialStep() == -1 ? sb->value() : sb->value() / para.GetPartialStep() / digi[ID]->GetCh2ns();

    digi[ID]->WriteRegister(para, bit, chID);
    if( para.IsCoupled() == true  && chID >= 0 ) digi[ID]->WriteRegister(para, bit, chID%2 == 0 ? chID + 1 : chID - 1);

    UpdatePanelFromMemory();
  });

}

//&###########################################################
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

void DigiSettingsPanel::SetUpGlobalTriggerMaskAndFrontPanelMask(QGridLayout * & gLayout){

  //^=============================== Others

  SetUpComboBoxBit(cbLEMOMode[ID], "LEMO Mode ", gLayout, 0, 0, DPP::Bit_FrontPanelIOControl::ListLEMOLevel, DPP::FrontPanelIOControl, DPP::Bit_FrontPanelIOControl::LEMOLevel, 1, 0);

  ///============================ Trig out mode
  QLabel * trgOutMode = new QLabel("TRI-OUT Mode ", this);
  trgOutMode->setAlignment(Qt::AlignRight | Qt::AlignCenter);
  gLayout->addWidget(trgOutMode, 0, 2);
  cbTRGOUTMode[ID] = new RComboBox(this);
  gLayout->addWidget(cbTRGOUTMode[ID], 0, 3);

  std::vector<std::pair<std::string, unsigned int>>  items = DPP::Bit_FrontPanelIOControl::ListTRGOUTConfig;
  for(int i = 0; i < (int) items.size(); i++){
    cbTRGOUTMode[ID]->addItem(QString::fromStdString(items[i].first), items[i].second);
  }

  connect( cbTRGOUTMode[ID], &RComboBox::currentIndexChanged, this, [=](int index){
    if( !enableSignalSlot ) return;

    if( index == 0 ) {
      digi[ID]->SetBits(DPP::FrontPanelIOControl, DPP::Bit_FrontPanelIOControl::DisableTrgOut, 1, -1);
    }else{
      digi[ID]->SetBits(DPP::FrontPanelIOControl, DPP::Bit_FrontPanelIOControl::DisableTrgOut, 0, -1);
      unsigned short bit = (cbTRGOUTMode[ID]->currentData().toUInt() >> 14) & 0x3F ;
      digi[ID]->SetBits(DPP::FrontPanelIOControl, {6, 14}, bit, -1);
    }
  });

  SetUpCheckBox(chkEnableExternalTrigger[ID], "Enable TRG-IN ", gLayout, 1, 1, DPP::DisableExternalTrigger, {1, 0});

  ///============================ Trig In mode
  QLabel * trgInMode = new QLabel("TRI-In Mode ", this);
  trgInMode->setAlignment(Qt::AlignRight | Qt::AlignCenter);
  gLayout->addWidget(trgInMode, 2, 0);

  cbTRGINMode[ID] = new RComboBox(this);
  gLayout->addWidget(cbTRGINMode[ID], 2, 1, 1, 2);

  items = DPP::Bit_FrontPanelIOControl::ListTRGIMode;
  for(int i = 0; i < (int) items.size(); i++){
    cbTRGINMode[ID]->addItem(QString::fromStdString(items[i].first), items[i].second);
  }
  connect( cbTRGINMode[ID], &RComboBox::currentIndexChanged, this, [=](int index){
    if( !enableSignalSlot ) return;
    digi[ID]->SetBits(DPP::FrontPanelIOControl, DPP::Bit_FrontPanelIOControl::TRGINMode, index, -1);
  });
  
  ///============================ Trig In Mezzanines
  QLabel * trgInMezzaninesMode = new QLabel("TRI-In Mezzanines ", this);
  trgInMezzaninesMode->setAlignment(Qt::AlignRight | Qt::AlignCenter);
  gLayout->addWidget(trgInMezzaninesMode, 3, 0);

  cbTRINMezzanines[ID] = new RComboBox(this);
  gLayout->addWidget(cbTRINMezzanines[ID], 3, 1, 1, 2);

  items = DPP::Bit_FrontPanelIOControl::ListTRGIMezzanine;
  for(int i = 0; i < (int) items.size(); i++){
    cbTRINMezzanines[ID]->addItem(QString::fromStdString(items[i].first), items[i].second);
  }
  connect( cbTRINMezzanines[ID], &RComboBox::currentIndexChanged, this, [=](int index){
    if( !enableSignalSlot ) return;
    digi[ID]->SetBits(DPP::FrontPanelIOControl, DPP::Bit_FrontPanelIOControl::TRGINMode, index, -1);
  });

  connect(chkEnableExternalTrigger[ID], &QCheckBox::stateChanged, this, [=](int state){
    cbTRGINMode[ID]->setEnabled(state);
    cbTRINMezzanines[ID]->setEnabled(state);
  });

  SetUpComboBox(cbAnalogMonitorMode[ID], "Analog Monitor Mode ", gLayout, 4, 0, DPP::AnalogMonitorMode, 0);

  connect(cbAnalogMonitorMode[ID], &RComboBox::currentIndexChanged, this, [=](int index){
    if( index < 2) {
      sbBufferGain[ID]->setEnabled(false);
      sbVoltageLevel[ID]->setEnabled(false);
    }

    sbBufferGain[ID]->setEnabled(( index == 2 ));
    sbVoltageLevel[ID]->setEnabled(( index == 3 ));

  });

  SetUpSpinBox(sbBufferGain[ID], "Buffer Occup. Gain ", gLayout, 5, 0, DPP::BufferOccupancyGain);
  SetUpSpinBox(sbVoltageLevel[ID], "Voltage Level ", gLayout, 5, 2, DPP::VoltageLevelModeConfig);
  sbVoltageLevel[ID]->setToolTip("1 LSD = 0.244 mV");
  sbVoltageLevel[ID]->setToolTipDuration(-1);

  //^=============================== Global Mask, TRG-OUT Mask
  QWidget * kaka = new QWidget(this);
  gLayout->addWidget(kaka, 7, 0, 1,     4);

  QGridLayout * maskLayout = new QGridLayout(kaka);
  maskLayout->setAlignment(Qt::AlignLeft);
  maskLayout->setSpacing(2);

  for( int i = 0; i < MaxNChannels/2; i++){

    if( i % 2 == 0 ){
      QLabel * chIDLabel = new QLabel(QString::number(2*i) + "-" + QString::number(2*i + 1), this);
      chIDLabel->setAlignment(Qt::AlignLeft | Qt::AlignBottom);
      maskLayout->addWidget(chIDLabel, 0, 1 + i, 1, 2);
    }

    bnGlobalTriggerMask[ID][i] = new QPushButton(this);
    bnGlobalTriggerMask[ID][i]->setFixedSize(QSize(20,20));
    bnGlobalTriggerMask[ID][i]->setToolTipDuration(-1);
    maskLayout->addWidget(bnGlobalTriggerMask[ID][i], 1, 1 + i );
    connect(bnGlobalTriggerMask[ID][i], &QPushButton::clicked, this, [=](){
      if( !enableSignalSlot) return;

      if( bnGlobalTriggerMask[ID][i]->styleSheet() == "" ){
        bnGlobalTriggerMask[ID][i]->setStyleSheet("background-color : green;");
        digi[ID]->SetBits(DPP::GlobalTriggerMask, {1, i}, 1, i);
      }else{
        bnGlobalTriggerMask[ID][i]->setStyleSheet("");
        digi[ID]->SetBits(DPP::GlobalTriggerMask, {1, i}, 0, i);
      }
    });

    bnTRGOUTMask[ID][i] = new QPushButton(this);
    bnTRGOUTMask[ID][i]->setFixedSize(QSize(20,20));
    bnTRGOUTMask[ID][i]->setToolTipDuration(-1);
    maskLayout->addWidget(bnTRGOUTMask[ID][i], 2, 1 + i );
    connect(bnTRGOUTMask[ID][i], &QPushButton::clicked, this, [=](){
      if( !enableSignalSlot) return;

      if( bnTRGOUTMask[ID][i]->styleSheet() == "" ){
        bnTRGOUTMask[ID][i]->setStyleSheet("background-color : green;");
        digi[ID]->SetBits(DPP::FrontPanelTRGOUTEnableMask, {1, i}, 1, i);
      }else{
        bnTRGOUTMask[ID][i]->setStyleSheet("");
        digi[ID]->SetBits(DPP::FrontPanelTRGOUTEnableMask, {1, i}, 0, i);
      }
    });
  }

  QLabel * lbGlobalTrg = new QLabel("Global Trigger Mask : ", this);
  lbGlobalTrg->setAlignment(Qt::AlignRight | Qt::AlignCenter);
  maskLayout->addWidget(lbGlobalTrg, 1, 0);

  //*============================================
  QLabel * lbMajorCoinWin = new QLabel("Coin. Win [ns] : ", this);
  maskLayout->addWidget(lbMajorCoinWin, 1, 9);

  sbGlbMajCoinWin[ID] = new RSpinBox(this);
  sbGlbMajCoinWin[ID]->setMinimum(0);
  sbGlbMajCoinWin[ID]->setMaximum(0xF * 4 * digi[ID]->GetCh2ns() );
  sbGlbMajCoinWin[ID]->setSingleStep(4 * digi[ID]->GetCh2ns());
  maskLayout->addWidget(sbGlbMajCoinWin[ID], 1, 10);
  connect(sbGlbMajCoinWin[ID], &RSpinBox::valueChanged, this, [=](){
    if( !enableSignalSlot ) return;
    sbGlbMajCoinWin[ID]->setStyleSheet("color : blue;");
  });

  connect(sbGlbMajCoinWin[ID], &RSpinBox::returnPressed, this, [=](){
    if( !enableSignalSlot ) return;

    if( sbGlbMajCoinWin[ID]->decimals() == 0 && sbGlbMajCoinWin[ID]->singleStep() != 1) {
      double step = sbGlbMajCoinWin[ID]->singleStep();
      double value = sbGlbMajCoinWin[ID]->value();
      sbGlbMajCoinWin[ID]->setValue( (std::round(value/step)*step));
    }

    sbGlbMajCoinWin[ID]->setStyleSheet("");
    digi[ID]->SetBits(DPP::GlobalTriggerMask, DPP::Bit_GlobalTriggerMask::MajorCoinWin, sbGlbMajCoinWin[ID]->value() / 4 / digi[ID]->GetCh2ns(), -1);
  });

  //*============================================
  QLabel * lbMajorLvl = new QLabel("Maj. Level", this);
  lbMajorLvl->setAlignment(Qt::AlignBottom | Qt::AlignHCenter);
  maskLayout->addWidget(lbMajorLvl, 0, 11);

  sbGlbMajLvl[ID] = new RSpinBox(this);
  sbGlbMajLvl[ID]->setMinimum(0);
  sbGlbMajLvl[ID]->setMaximum(7);
  sbGlbMajLvl[ID]->setSingleStep(1);
  connect(sbGlbMajLvl[ID], &RSpinBox::valueChanged, this, [=](){
    if( !enableSignalSlot ) return;
    sbGlbMajLvl[ID]->setStyleSheet("color : blue;");
  });

  connect(sbGlbMajLvl[ID], &RSpinBox::returnPressed, this, [=](){
    if( !enableSignalSlot ) return;
    sbGlbMajLvl[ID]->setStyleSheet("");
    digi[ID]->SetBits(DPP::GlobalTriggerMask, DPP::Bit_GlobalTriggerMask::MajorLevel, sbGlbMajLvl[ID]->value(), -1);

    if( sbGlbMajLvl[ID]->value() > 0 ) {
      sbGlbMajCoinWin[ID]->setEnabled(true);
    }else{
      sbGlbMajCoinWin[ID]->setEnabled(false);
    }

  });


  QLabel * lbOtherTrigger = new QLabel("OR trigger", this);
  lbOtherTrigger->setAlignment(Qt::AlignBottom | Qt::AlignHCenter);
  maskLayout->addWidget(lbOtherTrigger, 0, 12);

  //*============================================
  cbGlbUseOtherTriggers[ID] = new RComboBox(this);
  cbGlbUseOtherTriggers[ID]->addItem("None", 0);
  cbGlbUseOtherTriggers[ID]->addItem("TRG-IN", 1);
  cbGlbUseOtherTriggers[ID]->addItem("SW", 2);
  cbGlbUseOtherTriggers[ID]->addItem("TRG-IN OR SW", 3);
  maskLayout->addWidget(cbGlbUseOtherTriggers[ID], 1, 12);
  connect(cbGlbUseOtherTriggers[ID], &RComboBox::currentIndexChanged, this, [=](int index){
    if( !enableSignalSlot ) return;
    digi[ID]->SetBits(DPP::GlobalTriggerMask, {2, 30}, index, -1);
  });


  QLabel * lbTrgOut = new QLabel("TRG-OUT Mask : ", this);
  lbTrgOut->setAlignment(Qt::AlignRight | Qt::AlignCenter);
  maskLayout->addWidget(lbTrgOut, 2, 0);

  QLabel * lbTrgOutLogic = new QLabel("Logic : ", this);
  lbTrgOutLogic->setAlignment(Qt::AlignRight | Qt::AlignCenter);
  maskLayout->addWidget(lbTrgOutLogic, 2, 9);

  //*============================================
  cbTRGOUTLogic[ID] = new RComboBox(this);
  cbTRGOUTLogic[ID]->addItem("OR", 0);
  cbTRGOUTLogic[ID]->addItem("AND", 1);
  cbTRGOUTLogic[ID]->addItem("Maj.", 2);
  maskLayout->addWidget(cbTRGOUTLogic[ID], 2, 10);
  maskLayout->addWidget(sbGlbMajLvl[ID], 1, 11);

  connect(cbTRGOUTLogic[ID], &RComboBox::currentIndexChanged, this, [=](int index){
    if( !enableSignalSlot ) return;
    digi[ID]->SetBits(DPP::FrontPanelTRGOUTEnableMask, DPP::Bit_TRGOUTMask::TRGOUTLogic, index, -1);
    sbTRGOUTMajLvl[ID]->setEnabled( (index == 2) );
  });

  //*============================================
  sbTRGOUTMajLvl[ID] = new RSpinBox(this);
  sbTRGOUTMajLvl[ID]->setMinimum(0);
  sbTRGOUTMajLvl[ID]->setMaximum(7);
  sbTRGOUTMajLvl[ID]->setSingleStep(1);
  sbTRGOUTMajLvl[ID]->setEnabled( false );
  maskLayout->addWidget(sbTRGOUTMajLvl[ID], 2, 11);

  connect(sbTRGOUTMajLvl[ID], &RSpinBox::valueChanged, this, [=](){
    if( !enableSignalSlot ) return;
    sbTRGOUTMajLvl[ID]->setStyleSheet("color : blue;");
  });

  connect(sbTRGOUTMajLvl[ID], &RSpinBox::returnPressed, this, [=](){
    if( !enableSignalSlot ) return;
    sbTRGOUTMajLvl[ID]->setStyleSheet("");
    digi[ID]->SetBits(DPP::FrontPanelTRGOUTEnableMask, DPP::Bit_TRGOUTMask::MajorLevel, sbTRGOUTMajLvl[ID]->value(), -1);
  });

  //*============================================
  cbTRGOUTUseOtherTriggers[ID] = new RComboBox(this);
  cbTRGOUTUseOtherTriggers[ID]->addItem("None", 0);
  cbTRGOUTUseOtherTriggers[ID]->addItem("TRG-IN", 1);
  cbTRGOUTUseOtherTriggers[ID]->addItem("SW", 2);
  cbTRGOUTUseOtherTriggers[ID]->addItem("TRG-IN OR SW", 3);
  maskLayout->addWidget(cbTRGOUTUseOtherTriggers[ID], 2, 12);

  connect(cbTRGOUTUseOtherTriggers[ID], &RComboBox::currentIndexChanged, this, [=](int index){
    if( !enableSignalSlot ) return;
    digi[ID]->SetBits(DPP::FrontPanelTRGOUTEnableMask, {2, 30}, index, -1);
  });

  //^============================================ Trigger Validation Mask

  QLabel * info = new QLabel ("Each Row define the trigger requested by other coupled channels.", this);
  bdTriggerLayout[ID]->addWidget(info, 0, 0, 1, 13 );

  for( int i = 0; i < MaxNChannels/2 ; i++){

    if( i == 0 ) {
      for( int j = 0; j < MaxNChannels/2; j++) {
        QLabel * lb0 = new QLabel(QString::number(2*j) + "-" + QString::number(2*j+1), this);
        lb0->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
        bdTriggerLayout[ID]->addWidget(lb0, 1, j + 1 );
      }

      QLabel * lb1 = new QLabel("Logic", this);
      lb1->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
      bdTriggerLayout[ID]->addWidget(lb1, 1, MaxNChannels/2 + 1 );
      
      QLabel * lb2 = new QLabel("Maj. Lvl.", this);
      lb2->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
      bdTriggerLayout[ID]->addWidget(lb2, 1, MaxNChannels/2 + 2 );

      QLabel * lb3 = new QLabel("Ext.", this);
      lb3->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
      bdTriggerLayout[ID]->addWidget(lb3, 1, MaxNChannels/2 + 3 );

      QLabel * lb4 = new QLabel("SW", this);
      lb4->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
      bdTriggerLayout[ID]->addWidget(lb4, 1, MaxNChannels/2 + 4 );
    }

    QLabel * lbCh = new QLabel(QString::number(2*i) + "-" + QString::number(2*i+1), this);
    lbCh->setAlignment(Qt::AlignCenter | Qt::AlignRight);
    bdTriggerLayout[ID]->addWidget(lbCh, i + 2, 0 );

    for( int j = 0; j < MaxNChannels/2; j++){
      bnTriggerMask[ID][i][j] = new QPushButton(this);
      bdTriggerLayout[ID]->addWidget(bnTriggerMask[ID][i][j] , i + 2, j + 1 );

      connect(bnTriggerMask[ID][i][j], &QPushButton::clicked, this, [=](){
        if( !enableSignalSlot) return;

        if( bnTriggerMask[ID][i][j]->styleSheet() == "" ){
          bnTriggerMask[ID][i][j]->setStyleSheet("background-color : green;");
          digi[ID]->SetBits(DPP::TriggerValidationMask_G, {1, j}, 1, 2*i);
        }else{
          bnTriggerMask[ID][i][j]->setStyleSheet("");
          digi[ID]->SetBits(DPP::TriggerValidationMask_G, {1, j}, 0, 2*i);
        }
      });
    }

    cbMaskLogic[ID][i] = new RComboBox(this);
    cbMaskLogic[ID][i]->addItem("OR", 0);
    cbMaskLogic[ID][i]->addItem("AND", 1);
    cbMaskLogic[ID][i]->addItem("Maj.", 2);
    bdTriggerLayout[ID]->addWidget(cbMaskLogic[ID][i], i + 2, MaxNChannels/2 + 1);
    connect(cbMaskLogic[ID][i], &RComboBox::currentIndexChanged, this, [=](int index){
      if( !enableSignalSlot) return;
      digi[ID]->SetBits(DPP::TriggerValidationMask_G, {2, 8}, index, 2*i);
      sbMaskMajorLevel[ID][i]->setEnabled((index == 2));

    });

    sbMaskMajorLevel[ID][i] = new RSpinBox(this);
    sbMaskMajorLevel[ID][i]->setMinimum(0);
    sbMaskMajorLevel[ID][i]->setMaximum(7);
    sbMaskMajorLevel[ID][i]->setSingleStep(1);
    sbMaskMajorLevel[ID][i]->setEnabled(false);
    bdTriggerLayout[ID]->addWidget(sbMaskMajorLevel[ID][i], i + 2, MaxNChannels/2 + 2);
    connect(sbMaskMajorLevel[ID][i], &RSpinBox::valueChanged, this, [=](){
      if( !enableSignalSlot ) return;
      sbMaskMajorLevel[ID][i]->setStyleSheet("color : blue;");
    });
    connect(sbMaskMajorLevel[ID][i], &RSpinBox::returnPressed, this, [=](){
      if( !enableSignalSlot ) return;
      sbMaskMajorLevel[ID][i]->setStyleSheet("");
      digi[ID]->SetBits(DPP::TriggerValidationMask_G, {3, 10} ,sbMaskMajorLevel[ID][i]->value(), 2*i);
    });

    chkMaskExtTrigger[ID][i] = new QCheckBox("",this);
    chkMaskExtTrigger[ID][i]->setLayoutDirection(Qt::RightToLeft);
    bdTriggerLayout[ID]->addWidget(chkMaskExtTrigger[ID][i], i + 2, MaxNChannels/2 + 3);
    connect(chkMaskExtTrigger[ID][i], &QCheckBox::stateChanged, this, [=](int state){
      if( !enableSignalSlot ) return;
      digi[ID]->SetBits(DPP::TriggerValidationMask_G, {1, 30} , state ? 1 : 0 , 2*i);
    });

    chkMaskSWTrigger[ID][i] = new QCheckBox("",this);
    chkMaskSWTrigger[ID][i]->setLayoutDirection(Qt::RightToLeft);
    bdTriggerLayout[ID]->addWidget(chkMaskSWTrigger[ID][i], i + 2, MaxNChannels/2 + 4);
    connect(chkMaskSWTrigger[ID][i], &QCheckBox::stateChanged, this, [=](int state){
      if( !enableSignalSlot ) return;
      digi[ID]->SetBits(DPP::TriggerValidationMask_G, {1, 31} , state ? 1 : 0 , 2*i);
    });

  }

}

//&###########################################################
void DigiSettingsPanel::SetUpChannelMask(){

  QLabel * chMaskLabel = new QLabel("Channel Mask : ", this);
  chMaskLabel->setAlignment(Qt::AlignRight | Qt::AlignCenter);
  bdCfgLayout[ID]->addWidget(chMaskLabel, 0, 0);

  QWidget * chWiget = new QWidget(this);
  bdCfgLayout[ID]->addWidget(chWiget, 0, 1, 1, 3);

  QGridLayout * chLayout = new QGridLayout(chWiget);
  chLayout->setAlignment(Qt::AlignLeft);
  chLayout->setSpacing(0);

  for( int i = 0; i < MaxNChannels; i++){
    bnChEnableMask[ID][i] = new QPushButton(this);
    bnChEnableMask[ID][i]->setFixedSize(QSize(20,20));
    bnChEnableMask[ID][i]->setToolTip("Ch-" + QString::number(i));
    bnChEnableMask[ID][i]->setToolTipDuration(-1);
    QLabel * chIDLabel = new QLabel(QString::number(i), this);
    chIDLabel->setAlignment(Qt::AlignBottom | Qt::AlignHCenter);
    chLayout->addWidget(chIDLabel, 0, i);
    chLayout->addWidget(bnChEnableMask[ID][i], 1, i );

    connect(bnChEnableMask[ID][i], &QPushButton::clicked, this, [=](){
      if( !enableSignalSlot) return;

      if( bnChEnableMask[ID][i]->styleSheet() == "" ){
         bnChEnableMask[ID][i]->setStyleSheet("background-color : green;");
         digi[ID]->SetBits(DPP::ChannelEnableMask, {1, i}, 1, i);
      }else{
         bnChEnableMask[ID][i]->setStyleSheet("");
         digi[ID]->SetBits(DPP::ChannelEnableMask, {1, i}, 0, i);
      }
    });
  }
  
}

void DigiSettingsPanel::SetUpACQReadOutTab(){
  SetUpSpinBox(sbAggNum[ID],        "Agg. Num. / read ", bdACQLayout[ID], 0, 0, DPP::MaxAggregatePerBlockTransfer);
  SetUpComboBox(cbAggOrg[ID], "Aggregate Organization ", bdACQLayout[ID], 1, 0, DPP::AggregateOrganization, 0);

  SetUpComboBoxBit(cbStartStopMode[ID], "Start/Stop Mode ", bdACQLayout[ID], 2, 0, DPP::Bit_AcquistionControl::ListStartStopMode,
                                                                                   DPP::AcquisitionControl, DPP::Bit_AcquistionControl::StartStopMode, 1, 0);

  SetUpComboBoxBit(cbAcqStartArm[ID], "Acq Start/Arm ", bdACQLayout[ID], 3, 0, DPP::Bit_AcquistionControl::ListACQStartArm,
                                                                               DPP::AcquisitionControl, DPP::Bit_AcquistionControl::ACQStartArm, 1, 0);

  SetUpComboBoxBit(cbPLLRefClock[ID], "PLL Ref. Clock ", bdACQLayout[ID], 4, 0, DPP::Bit_AcquistionControl::ListPLLRef, 
                                                                                DPP::AcquisitionControl, DPP::Bit_AcquistionControl::ACQStartArm, 1, 0);

  SetUpSpinBox(sbRunDelay[ID], "Run Delay [ns] ", bdACQLayout[ID], 5, 0, DPP::RunStartStopDelay);

  {//=======================
    QGroupBox * readOutGroup = new QGroupBox("Readout Control (VME only)",this);
    bdACQLayout[ID]->addWidget(readOutGroup, 6, 0, 1, 2);

    QGridLayout *readOutLayout = new QGridLayout(readOutGroup);
    readOutLayout->setSpacing(2);

    SetUpCheckBox(chkEnableOpticalInterrupt[ID],         "Enable Optical Link Interrupt", readOutLayout, 0, 1, DPP::ReadoutControl, DPP::Bit_ReadoutControl::EnableOpticalLinkInpt);
    SetUpCheckBox(chkEnableVME64Aligment[ID],                  "Enable VME Align64 Mode", readOutLayout, 1, 1, DPP::ReadoutControl, DPP::Bit_ReadoutControl::VMEAlign64Mode);
    SetUpCheckBox(chkEnableExtendedBlockTransfer[ID],   "Enable Extended Block Transfer", readOutLayout, 2, 1, DPP::ReadoutControl, DPP::Bit_ReadoutControl::EnableExtendedBlockTransfer);
    SetUpCheckBox(chkEnableAddRelocation[ID],       "Enable VME Base Address Relocation", readOutLayout, 3, 1, DPP::ReadoutControl, DPP::Bit_ReadoutControl::VMEBaseAddressReclocated);
    SetUpCheckBox(chkEnableVMEReadoutStatus[ID], "VME Bus Error / Event Aligned Readout", readOutLayout, 4, 1, DPP::ReadoutControl, DPP::Bit_ReadoutControl::EnableEventAligned);

    SetUpComboBoxBit(cbInterruptMode[ID], "Interrupt Release Mode : ", readOutLayout, 5, 0, {{"On Register Access", 0},{"On Acknowledge", 1}}, DPP::ReadoutControl, DPP::Bit_ReadoutControl::InterrupReleaseMode, 1, 0);

    QLabel * lbInterruptLevel = new QLabel ("VME Interrupt Level :", this);
    lbInterruptLevel->setAlignment(Qt::AlignRight | Qt::AlignCenter);
    readOutLayout->addWidget(lbInterruptLevel, 6, 0);

    sbVMEInterruptLevel[ID] = new RSpinBox(this);
    sbVMEInterruptLevel[ID]->setMinimum(0);
    sbVMEInterruptLevel[ID]->setMaximum(7);
    sbVMEInterruptLevel[ID]->setSingleStep(1);
    readOutLayout->addWidget(sbVMEInterruptLevel[ID], 6, 1);

    connect(sbVMEInterruptLevel[ID], &RSpinBox::valueChanged, this, [=](){
      if( !enableSignalSlot ) return;
      sbVMEInterruptLevel[ID]->setStyleSheet("color : blue;");
    });

    connect(sbVMEInterruptLevel[ID], &RSpinBox::returnPressed, this, [=](){
      if( !enableSignalSlot ) return;
      sbVMEInterruptLevel[ID]->setStyleSheet("");
      digi[ID]->SetBits(DPP::ReadoutControl, DPP::Bit_ReadoutControl::VMEInterruptLevel, sbVMEInterruptLevel[ID]->value(), -1);
    });
  }
}

void DigiSettingsPanel::SetUpPHABoard(){
  printf("============== %s \n", __func__);

  SetUpCheckBox(chkAutoDataFlush[ID],        "Auto Data Flush", bdCfgLayout[ID], 1, 0, DPP::BoardConfiguration, DPP::Bit_BoardConfig::EnableAutoDataFlush);
  SetUpCheckBox(chkTrigPropagation[ID],      "Trig. Propagate", bdCfgLayout[ID], 2, 0, DPP::BoardConfiguration, DPP::Bit_BoardConfig::TrigPropagation);  
  SetUpCheckBox(chkDecimateTrace[ID],         "Decimate Trace", bdCfgLayout[ID], 3, 0, DPP::BoardConfiguration, DPP::Bit_BoardConfig::DecimateTrace);
  SetUpCheckBox(chkDualTrace[ID],                 "Dual Trace", bdCfgLayout[ID], 1, 1, DPP::BoardConfiguration, DPP::Bit_BoardConfig::DualTrace);
  
  connect(chkDualTrace[ID], &QCheckBox::stateChanged, this, [=](int state){
    if( !enableSignalSlot) return;
    cbAnaProbe2[ID]->setEnabled(state);
    cbDigiProbe2[ID]->setEnabled(state);
  });

  SetUpCheckBox(chkTraceRecording[ID],     "Record Trace", bdCfgLayout[ID], 2, 1, DPP::BoardConfiguration, DPP::Bit_BoardConfig::RecordTrace);
  SetUpCheckBox(chkEnableExtra2[ID],      "Enable Extra2", bdCfgLayout[ID], 3, 1, DPP::BoardConfiguration, DPP::Bit_BoardConfig::EnableExtra2);

  SetUpComboBoxBit(cbAnaProbe1[ID],   "Ana. Probe 1 ", bdCfgLayout[ID], 1, 2, DPP::Bit_BoardConfig::ListAnaProbe1_PHA, DPP::BoardConfiguration, DPP::Bit_BoardConfig::AnalogProbe1, 1, 0);
  SetUpComboBoxBit(cbAnaProbe2[ID],   "Ana. Probe 2 ", bdCfgLayout[ID], 2, 2, DPP::Bit_BoardConfig::ListAnaProbe2_PHA, DPP::BoardConfiguration, DPP::Bit_BoardConfig::AnalogProbe2, 1, 0);
  SetUpComboBoxBit(cbDigiProbe1[ID], "Digi. Probe 1 ", bdCfgLayout[ID], 3, 2, DPP::Bit_BoardConfig::ListDigiProbe1_PHA, DPP::BoardConfiguration, DPP::Bit_BoardConfig::DigiProbel1, 1, 0);
  SetUpComboBoxBit(cbDigiProbe2[ID], "Digi. Probe 2 ", bdCfgLayout[ID], 4, 2, DPP::Bit_BoardConfig::ListDigiProbe2_PHA, DPP::BoardConfiguration, DPP::Bit_BoardConfig::DigiProbel2, 1, 0);

}

//&###########################################################
void DigiSettingsPanel::SetUpPHAChannel(){

  QWidget * chAllSetting = new QWidget(this);
  //chAllSetting->setStyleSheet("background-color: #ECECEC;");
  chTab->addTab(chAllSetting, "Channel Settings");

  QWidget * chStatus = new QWidget(this);
  //chStatus->setStyleSheet("background-color: #ECECEC;");
  chTab->addTab(chStatus, "Status");

  QWidget * chInput = new QWidget(this);
  chTab->addTab(chInput, "Input");

  QWidget * chTrig = new QWidget(this);
  chTab->addTab(chTrig, "Trigger");

  QWidget * chTrap = new QWidget(this);
  chTab->addTab(chTrap, "Trapezoid");

  QWidget * chOthers = new QWidget(this);
  chTab->addTab(chOthers, "Others");

  //^======================== All Channels
  QVBoxLayout * allSettingLayout = new QVBoxLayout(chAllSetting);
  allSettingLayout->setAlignment(Qt::AlignTop);
  allSettingLayout->setSpacing(2);

  QWidget * jaja = new QWidget(this);
  allSettingLayout->addWidget(jaja);

  QHBoxLayout * papa = new QHBoxLayout(jaja);
  papa->setAlignment(Qt::AlignLeft);

  {//^============================== Channel selection
    QLabel * lbChSel = new QLabel ("Ch : ", this);
    lbChSel->setAlignment(Qt::AlignCenter | Qt::AlignRight);
    papa->addWidget(lbChSel);
   
    chSelection[ID] = new RComboBox(this);
    chSelection[ID]->addItem("All Ch.", -1);
    for( int i = 0; i < digi[ID]->GetNChannels(); i++) chSelection[ID]->addItem(QString::number(i), i);
    papa->addWidget(chSelection[ID]);

    connect(chSelection[ID], &RComboBox::currentIndexChanged, this, [=](){
      SyncAllChannelsTab_PHA();
    });
  }

  {//*========================= input
    QGroupBox * inputBox = new QGroupBox("input Settings", this);
    allSettingLayout->addWidget(inputBox);

    QGridLayout  * inputLayout = new QGridLayout(inputBox);
    inputLayout->setSpacing(2);

    SetUpSpinBox(sbRecordLength[ID][MaxNChannels], "Record Length [G][ns] : ", inputLayout, 0, 0, DPP::RecordLength_G);
    SetUpComboBox(cbDynamicRange[ID][MaxNChannels],        "Dynamic Range : ", inputLayout, 0, 2, DPP::InputDynamicRange);
    SetUpSpinBox(sbPreTrigger[ID][MaxNChannels],        "Pre-Trigger [ns] : ", inputLayout, 1, 0, DPP::PreTrigger);
    SetUpComboBox(cbRCCR2Smoothing[ID][MaxNChannels],   "Smoothing factor : ", inputLayout, 1, 2, DPP::PHA::RCCR2SmoothingFactor);
    SetUpSpinBox(sbInputRiseTime[ID][MaxNChannels],       "Rise Time [ns] : ", inputLayout, 2, 0, DPP::PHA::InputRiseTime);
    SetUpSpinBox(sbThreshold[ID][MaxNChannels],          "Threshold [LSB] : ", inputLayout, 2, 2, DPP::PHA::TriggerThreshold);
    SetUpSpinBox(sbRiseTimeValidWin[ID][MaxNChannels], "Rise Time Valid. Win. [ns] : ", inputLayout, 3, 0, DPP::PHA::RiseTimeValidationWindow);
    SetUpSpinBox(sbDCOffset[ID][MaxNChannels],             "DC Offset [%] : ", inputLayout, 3, 2, DPP::ChannelDCOffset);
    SetUpComboBoxBit(cbPolarity[ID][MaxNChannels],              "Polarity : ", inputLayout, 5, 0, DPP::Bit_DPPAlgorithmControl_PHA::ListPolarity, DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl_PHA::Polarity);

  }

  {//*=================== Trigger
    QGroupBox * trigBox = new QGroupBox("Trigger Settings", this);
    allSettingLayout->addWidget(trigBox);

    QGridLayout  * trigLayout = new QGridLayout(trigBox);
    trigLayout->setSpacing(2);

    SetUpCheckBox(chkDisableSelfTrigger[ID][MaxNChannels],       "Disable Self Trigger ", trigLayout, 0, 1, DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl_PHA::DisableSelfTrigger);
    SetUpComboBoxBit(cbTrigMode[ID][MaxNChannels],                        "Trig Mode : ", trigLayout, 1, 0, DPP::Bit_DPPAlgorithmControl_PHA::ListTrigMode, DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl_PHA::TriggerMode, 1);
    SetUpSpinBox(sbTriggerHoldOff[ID][MaxNChannels],           "Tigger Hold-off [ns] : ", trigLayout, 1, 2, DPP::PHA::TriggerHoldOffWidth);
    SetUpComboBoxBit(cbLocalTriggerValid[ID][MaxNChannels],  "Local Trig. Valid. [G] : ", trigLayout, 2, 0, DPP::PHA::Bit_DPPAlgorithmControl2::ListLocalTrigValidMode, DPP::PHA::DPPAlgorithmControl2_G, DPP::PHA::Bit_DPPAlgorithmControl2::LocalTrigValidMode);
    SetUpComboBoxBit(cbTrigCount[ID][MaxNChannels],          "Trig. Counter Flag [G] : ", trigLayout, 2, 2, DPP::PHA::Bit_DPPAlgorithmControl2::ListTrigCounter, DPP::PHA::DPPAlgorithmControl2_G, DPP::PHA::Bit_DPPAlgorithmControl2::TriggerCounterFlag);
    SetUpComboBoxBit(cbLocalShapedTrigger[ID][MaxNChannels], "Local Shaped Trig. [G] : ", trigLayout, 3, 0, DPP::PHA::Bit_DPPAlgorithmControl2::ListLocalShapeTrigMode, DPP::PHA::DPPAlgorithmControl2_G, DPP::PHA::Bit_DPPAlgorithmControl2::LocalShapeTriggerMode, 1);
    SetUpSpinBox(sbShapedTrigWidth[ID][MaxNChannels],       "Shaped Trig. Width [ns] : ", trigLayout, 3, 2, DPP::PHA::ShapedTriggerWidth);
  }

  {//*===================== Trapezoid
    QGroupBox * trapBox = new QGroupBox("Trapezoid Settings", this);
    allSettingLayout->addWidget(trapBox);

    QGridLayout  * trapLayout = new QGridLayout(trapBox);
    trapLayout->setSpacing(2);

    SetUpSpinBox(sbTrapRiseTime[ID][MaxNChannels],          "Rise Time [ns] : ", trapLayout, 0, 0, DPP::PHA::TrapezoidRiseTime);
    SetUpSpinBox(sbTrapFlatTop[ID][MaxNChannels],            "Flat Top [ns] : ", trapLayout, 0, 2, DPP::PHA::TrapezoidFlatTop);
    SetUpSpinBox(sbDecay[ID][MaxNChannels],                     "Decay [ns] : ", trapLayout, 1, 0, DPP::PHA::DecayTime);
    SetUpSpinBox(sbTrapScaling[ID][MaxNChannels],                "Rescaling : ", trapLayout, 1, 2, DPP::PHA::DPPAlgorithmControl2_G);
    SetUpSpinBox(sbPeaking[ID][MaxNChannels],                 "Peaking [ns] : ", trapLayout, 2, 0, DPP::PHA::PeakingTime);
    SetUpSpinBox(sbPeakingHoldOff[ID][MaxNChannels],    "Peak Hold-off [ns] : ", trapLayout, 2, 2, DPP::PHA::PeakHoldOff);
    SetUpComboBoxBit(cbPeakAvg[ID][MaxNChannels],                "Peak Avg. : ", trapLayout, 3, 0, DPP::Bit_DPPAlgorithmControl_PHA::ListPeakMean, DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl_PHA::PeakMean);
    SetUpComboBoxBit(cbBaseLineAvg[ID][MaxNChannels],        "Baseline Avg. : ", trapLayout, 3, 2, DPP::Bit_DPPAlgorithmControl_PHA::ListBaselineAvg, DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl_PHA::BaselineAvg);
    SetUpCheckBox(chkActiveBaseline[ID][MaxNChannels],     "Active basline [G]", trapLayout, 4, 0, DPP::PHA::DPPAlgorithmControl2_G, DPP::PHA::Bit_DPPAlgorithmControl2::ActivebaselineCalulation);
    SetUpCheckBox(chkBaselineRestore[ID][MaxNChannels], "Baseline Restorer [G]", trapLayout, 4, 1, DPP::PHA::DPPAlgorithmControl2_G, DPP::PHA::Bit_DPPAlgorithmControl2::EnableActiveBaselineRestoration);
    SetUpSpinBox(sbFineGain[ID][MaxNChannels],                   "Fine Gain : ", trapLayout, 4, 2, DPP::PHA::FineGain);

  }

  {//*====================== Others
    QGroupBox * otherBox = new QGroupBox("Others Settings", this);
    allSettingLayout->addWidget(otherBox);
    
    QGridLayout  * otherLayout = new QGridLayout(otherBox);
    otherLayout->setSpacing(2);

    SetUpCheckBox(chkEnableRollOver[ID][MaxNChannels],     "Enable Roll-Over Event", otherLayout, 0, 1, DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl_PHA::EnableRollOverFlag);
    SetUpCheckBox(chkEnablePileUp[ID][MaxNChannels],          "Allow Pile-up Event", otherLayout, 0, 3, DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl_PHA::EnablePileUpFlag);
    SetUpCheckBox(chkTagCorrelation[ID][MaxNChannels],  "Tag Correlated events [G]", otherLayout, 1, 1, DPP::PHA::DPPAlgorithmControl2_G, DPP::PHA::Bit_DPPAlgorithmControl2::TagCorrelatedEvents);
    SetUpSpinBox(sbNumEventAgg[ID][MaxNChannels],          "Events per Agg. [G] : ", otherLayout, 1, 2, DPP::NumberEventsPerAggregate_G);
    SetUpComboBoxBit(cbDecimateTrace[ID][MaxNChannels],         "Decimate Trace : ", otherLayout, 2, 0, DPP::Bit_DPPAlgorithmControl_PHA::ListTraceDecimation, DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl_PHA::TraceDecimation);
    SetUpComboBoxBit(cbDecimateGain[ID][MaxNChannels],           "Decimate Gain : ", otherLayout, 2, 2, DPP::Bit_DPPAlgorithmControl_PHA::ListDecimationGain, DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl_PHA::TraceDeciGain);
    SetUpComboBoxBit(cbVetoSource[ID][MaxNChannels],           "Veto Source [G] : ", otherLayout, 3, 0, DPP::PHA::Bit_DPPAlgorithmControl2::ListVetoSource, DPP::PHA::DPPAlgorithmControl2_G, DPP::PHA::Bit_DPPAlgorithmControl2::VetoSource);
    SetUpSpinBox(sbVetoWidth[ID][MaxNChannels],                     "Veto Width : ", otherLayout, 3, 2, DPP::VetoWidth);
    SetUpComboBoxBit(cbVetoStep[ID][MaxNChannels],                   "Veto Step : ", otherLayout, 4, 0, DPP::Bit_VetoWidth::ListVetoStep, DPP::VetoWidth, DPP::Bit_VetoWidth::VetoStep, 1);
    SetUpComboBoxBit(cbExtra2Option[ID][MaxNChannels],       "Extra2 Option [G] : ", otherLayout, 5, 0, DPP::PHA::Bit_DPPAlgorithmControl2::ListExtra2, DPP::PHA::DPPAlgorithmControl2_G, DPP::PHA::Bit_DPPAlgorithmControl2::Extra2Option, 3);

  }

  {//^================== status
    QGridLayout * statusLayout = new QGridLayout(chStatus);
    statusLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    statusLayout->setSpacing(2);

    QLabel * lbCh   = new QLabel ("Ch.", this);      lbCh->setAlignment(Qt::AlignHCenter);   statusLayout->addWidget(lbCh, 0, 0);
    QLabel * lbLED  = new QLabel ("Status", this);   lbLED->setAlignment(Qt::AlignHCenter);  statusLayout->addWidget(lbLED, 0, 1, 1, 3);
    QLabel * lbTemp = new QLabel ("Temp [C]", this); lbTemp->setAlignment(Qt::AlignHCenter); statusLayout->addWidget(lbTemp, 0, 4);

    QStringList chStatusInfo = {"SPI bus is busy.", "ADC Calibration is done.", "ADC shutdown, over-heat"};

    for( int i = 0; i < MaxNChannels; i++){

      QLabel * lbChID = new QLabel (QString::number(i), this);      
      lbChID->setAlignment(Qt::AlignRight | Qt::AlignCenter);
      lbChID->setFixedWidth(20);
      statusLayout->addWidget(lbChID, i + 1, 0); 

      for( int j = 0; j < 3; j++ ){
        bnChStatus[ID][i][j] = new QPushButton(this);
        bnChStatus[ID][i][j]->setToolTip(chStatusInfo[j]);
        bnChStatus[ID][i][j]->setFixedSize(20, 20);
        statusLayout->addWidget(bnChStatus[ID][i][j], i + 1, j + 1);
      }
      leADCTemp[ID][i] = new QLineEdit(this);
      leADCTemp[ID][i]->setReadOnly(true);
      leADCTemp[ID][i]->setFixedWidth(100);
      statusLayout->addWidget(leADCTemp[ID][i], i +1, 3 + 1);
    }
    
    QPushButton * bnADCCali = new QPushButton("ADC Calibration", this);
    statusLayout->addWidget(bnADCCali, MaxNChannels + 1, 0, 1, 5);

    connect(bnADCCali, &QPushButton::clicked, this, [=](){
      digi[ID]->WriteRegister(DPP::ADCCalibration_W, 1);
      for( int i = 0 ; i < digi[ID]->GetNChannels(); i++ ) digi[ID]->ReadRegister(DPP::ChannelStatus_R, i);
      UpdatePanelFromMemory();
    });
  }

  {//^============================= input

    QVBoxLayout *inputLayout = new QVBoxLayout(chInput);

    QTabWidget * inputTab = new QTabWidget(this);
    inputLayout->addWidget(inputTab);

    QStringList tabName = {"Common Settings", "Probably OK Setings"};

    const int nTab = tabName.count();

    QWidget ** tabID = new QWidget * [nTab];

    for( int i = 0; i < nTab; i++){   
      tabID [i]  = new QWidget(this);
      inputTab->addTab(tabID[i], tabName[i]);

      QGridLayout * tabLayout = new QGridLayout(tabID[i]);
      tabLayout->setSpacing(2);
      tabLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);

      QLabel * lb0 = new QLabel("Ch.", this); lb0->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb0, 0, 0);
      
      for( int ch = 0; ch < digi[ID]->GetNChannels(); ch++){
      
        QLabel * chid = new QLabel(QString::number(ch), this);
        chid->setAlignment(Qt::AlignRight | Qt::AlignCenter);
        chid->setFixedWidth(20);
        tabLayout->addWidget(chid, ch + 1, 0);

        if( i == 0 ) {
          if( ch == 0 ){
            QLabel * lb1 = new QLabel("Threshold", this); lb1->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb1, 0, 2);
            QLabel * lb2 = new QLabel("DC offset [%]", this); lb2->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb2, 0, 4);
            QLabel * lb3 = new QLabel("Record Length [G][ns]", this); lb3->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb3, 0, 6);
            QLabel * lb4 = new QLabel("Pre-Trigger [ns]", this); lb4->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb4, 0, 8);
            QLabel * lb5 = new QLabel("Dynamic Range", this); lb5->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb5, 0, 10);
            QLabel * lb6 = new QLabel("Polarity", this); lb6->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb6, 0, 12);
          }
          SetUpSpinBox(sbThreshold[ID][ch],       "", tabLayout, ch + 1, 1, DPP::PHA::TriggerThreshold, ch);
          SetUpSpinBox(sbDCOffset[ID][ch],        "", tabLayout, ch + 1, 3, DPP::ChannelDCOffset, ch);
          SetUpSpinBox(sbRecordLength[ID][ch],    "", tabLayout, ch + 1, 5, DPP::RecordLength_G, ch);
          SetUpSpinBox(sbPreTrigger[ID][ch],      "", tabLayout, ch + 1, 7, DPP::PreTrigger, ch);
          SetUpComboBox(cbDynamicRange[ID][ch],   "", tabLayout, ch + 1, 9, DPP::InputDynamicRange, ch);
          SetUpComboBoxBit(cbPolarity[ID][ch], "", tabLayout, ch + 1, 11, DPP::Bit_DPPAlgorithmControl_PHA::ListPolarity, DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl_PHA::Polarity, 1, ch);
        }

        if ( i == 1 ){
          if( ch == 0 ){
            QLabel * lb1 = new QLabel("Rise Time [ns]", this); lb1->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb1, 0, 2);
            QLabel * lb2 = new QLabel("Rise Time Valid. Win. [ns]", this); lb2->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb2, 0, 4);
            QLabel * lb4 = new QLabel("Shaped Trig. Width [ns]", this); lb4->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb4, 0, 8);
            QLabel * lb5= new QLabel("Smoothing factor", this); lb5->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb5, 0, 10);
          }
          SetUpSpinBox(sbInputRiseTime[ID][ch],   "", tabLayout, ch + 1, 1, DPP::PHA::InputRiseTime, ch);
          SetUpSpinBox(sbRiseTimeValidWin[ID][ch],"", tabLayout, ch + 1, 3, DPP::PHA::RiseTimeValidationWindow, ch);
          SetUpSpinBox(sbShapedTrigWidth[ID][ch], "", tabLayout, ch + 1, 7, DPP::PHA::ShapedTriggerWidth, ch);
          SetUpComboBox(cbRCCR2Smoothing[ID][ch], "", tabLayout, ch + 1, 9, DPP::PHA::RCCR2SmoothingFactor, ch);
        }
      }

    }
  
  }

  {//^================================== Trigger 

    QVBoxLayout *trigLayout = new QVBoxLayout(chTrig);

    QTabWidget * trigTab = new QTabWidget(this);
    trigLayout->addWidget(trigTab);

    QStringList tabName = {"Common Settings", "Probably OK Setings"};

    const int nTab = tabName.count();

    QWidget ** tabID = new QWidget * [nTab];

    for( int i = 0; i < nTab; i++){   
      tabID [i]  = new QWidget(this);
      trigTab->addTab(tabID[i], tabName[i]);

      QGridLayout * tabLayout = new QGridLayout(tabID[i]);
      tabLayout->setSpacing(2);
      tabLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);

      QLabel * lb0 = new QLabel("Ch.", this); lb0->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb0, 0, 0);
      
      for( int ch = 0; ch < digi[ID]->GetNChannels(); ch++){
        QLabel * chid = new QLabel(QString::number(ch), this);
        chid->setAlignment(Qt::AlignRight | Qt::AlignCenter);
        chid->setFixedWidth(20);
        tabLayout->addWidget(chid, ch + 1, 0);

        if( i == 0 ){
          if( ch == 0 ){
            QLabel * lb0 = new QLabel("Trig Mode", this); lb0->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb0, 0, 4);
            QLabel * lb3 = new QLabel("Tigger Hold-off [ns]", this); lb3->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb3, 0, 6);
            QLabel * lb4 = new QLabel("Local Trig. Valid. [G]", this); lb4->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb4, 0, 8);
          }
          SetUpCheckBox(chkDisableSelfTrigger[ID][ch],  "Disable Self Trigger", tabLayout, ch + 1, 1, DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl_PHA::DisableSelfTrigger, ch);
          SetUpComboBoxBit(cbTrigMode[ID][ch],                              "", tabLayout, ch + 1, 3, DPP::Bit_DPPAlgorithmControl_PHA::ListTrigMode, DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl_PHA::TriggerMode, 1, ch);
          SetUpSpinBox(sbTriggerHoldOff[ID][ch],                            "", tabLayout, ch + 1, 5, DPP::PHA::TriggerHoldOffWidth, ch);
          SetUpComboBoxBit(cbLocalTriggerValid[ID][ch],                     "", tabLayout, ch + 1, 7, DPP::PHA::Bit_DPPAlgorithmControl2::ListLocalTrigValidMode, DPP::PHA::DPPAlgorithmControl2_G, DPP::PHA::Bit_DPPAlgorithmControl2::LocalTrigValidMode, 1, ch);
        }

        if( i == 1 ){
          if( ch == 0 ){
            QLabel * lb3 = new QLabel("Shaped Trig. Width [ns]", this); lb3->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb3, 0, 2);
            QLabel * lb2 = new QLabel("Local Shaped Trig. [G]", this); lb2->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb2, 0, 4);
            QLabel * lb1 = new QLabel("Trig. Counter Flag [G]", this); lb1->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb1, 0, 6);
          }
          SetUpSpinBox(sbShapedTrigWidth[ID][ch],        "", tabLayout, ch + 1, 1, DPP::PSD::ShapedTriggerWidth, ch);
          SetUpComboBoxBit(cbLocalShapedTrigger[ID][ch], "", tabLayout, ch + 1, 3, DPP::PHA::Bit_DPPAlgorithmControl2::ListLocalShapeTrigMode, DPP::PHA::DPPAlgorithmControl2_G, DPP::PHA::Bit_DPPAlgorithmControl2::LocalShapeTriggerMode, 1, ch);
          SetUpComboBoxBit(cbTrigCount[ID][ch],          "", tabLayout, ch + 1, 5, DPP::PHA::Bit_DPPAlgorithmControl2::ListTrigCounter, DPP::PHA::DPPAlgorithmControl2_G, DPP::PHA::Bit_DPPAlgorithmControl2::TriggerCounterFlag, 1, ch);
          SetUpCheckBox(chkTagCorrelation[ID][ch], "Tag Correlated events [G]", tabLayout, ch + 1, 7, DPP::PHA::DPPAlgorithmControl2_G, DPP::PHA::Bit_DPPAlgorithmControl2::TagCorrelatedEvents, ch);

        }
      }
    }

  }

  {//^================================== Trapezoid 

    QVBoxLayout *trapLayout = new QVBoxLayout(chTrap);

    QTabWidget * trapTab = new QTabWidget(this);
    trapLayout->addWidget(trapTab);

    QStringList tabName = {"Common Settings", "Probably OK Setings"};

    const int nTab = tabName.count();

    QWidget ** tabID = new QWidget * [nTab];

    for( int i = 0; i < nTab; i++){   
      tabID [i]  = new QWidget(this);
      trapTab->addTab(tabID[i], tabName[i]);

      QGridLayout * tabLayout = new QGridLayout(tabID[i]);
      tabLayout->setSpacing(2);
      tabLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);

      QLabel * lb0 = new QLabel("Ch.", this); lb0->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb0, 0, 0);
      
      for( int ch = 0; ch < digi[ID]->GetNChannels(); ch++){
      
        QLabel * chid = new QLabel(QString::number(ch), this);
        chid->setAlignment(Qt::AlignRight | Qt::AlignCenter);
        chid->setFixedWidth(20);
        tabLayout->addWidget(chid, ch + 1, 0);


        if( i == 0 ) {
          if( ch == 0 ){
            QLabel * lb1 = new QLabel("Rise Time [ns]", this); lb1->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb1, 0, 2);
            QLabel * lb2 = new QLabel("Flat Top [ns]", this); lb2->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb2, 0, 4);
            QLabel * lb3 = new QLabel("Decay [ns]", this); lb3->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb3, 0, 6);
            QLabel * lb4 = new QLabel("Peaking [ns]", this); lb4->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb4, 0, 8);
            QLabel * lb5 = new QLabel("Peak Avg", this); lb5->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb5, 0, 10);
            QLabel * lb6 = new QLabel("baseline Avg", this); lb6->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb6, 0, 12);
          }
          SetUpSpinBox(sbTrapRiseTime[ID][ch], "", tabLayout, ch + 1, 1, DPP::PHA::TrapezoidRiseTime, ch);
          SetUpSpinBox(sbTrapFlatTop[ID][ch],  "", tabLayout, ch + 1, 3, DPP::PHA::TrapezoidFlatTop, ch);
          SetUpSpinBox(sbDecay[ID][ch],        "", tabLayout, ch + 1, 5, DPP::PHA::DecayTime, ch);
          SetUpSpinBox(sbPeaking[ID][ch],      "", tabLayout, ch + 1, 7, DPP::PHA::PeakingTime, ch);
          SetUpComboBoxBit(cbPeakAvg[ID][ch],  "", tabLayout, ch + 1, 9, DPP::Bit_DPPAlgorithmControl_PHA::ListPeakMean, DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl_PHA::PeakMean, 1, ch);
          SetUpComboBoxBit(cbBaseLineAvg[ID][ch], "", tabLayout, ch + 1, 11, DPP::Bit_DPPAlgorithmControl_PHA::ListBaselineAvg, DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl_PHA::BaselineAvg, 1, ch);
        }

        if ( i == 1 ){
          if( ch == 0 ){
            QLabel * lb1 = new QLabel("Peak holdoff [ns]", this); lb1->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb1, 0, 2);
            QLabel * lb2 = new QLabel("Rescaling", this); lb2->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb2, 0, 4);
            QLabel * lb3 = new QLabel("Fine Gain", this); lb3->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb3, 0, 6);
          }
          SetUpSpinBox(sbPeakingHoldOff[ID][ch],   "", tabLayout, ch + 1, 1, DPP::PHA::PeakHoldOff, ch);
          SetUpSpinBox(sbTrapScaling[ID][ch],      "", tabLayout, ch + 1, 3, DPP::PHA::DPPAlgorithmControl2_G, ch);
          SetUpSpinBox(sbFineGain[ID][ch],         "", tabLayout, ch + 1, 5, DPP::PHA::FineGain, ch);

          SetUpCheckBox(chkActiveBaseline[ID][ch], "Active basline [G]", tabLayout, ch + 1, 7, DPP::PHA::DPPAlgorithmControl2_G, DPP::PHA::Bit_DPPAlgorithmControl2::ActivebaselineCalulation, ch);
          SetUpCheckBox(chkBaselineRestore[ID][ch], "Baseline Restorer [G]", tabLayout, ch + 1, 9, DPP::PHA::DPPAlgorithmControl2_G, DPP::PHA::Bit_DPPAlgorithmControl2::EnableActiveBaselineRestoration, ch);
        }
      }
    }
  }

  {//^======================================== Others
    QVBoxLayout *otherLayout = new QVBoxLayout(chOthers);

    QTabWidget * othersTab = new QTabWidget(this);
    otherLayout->addWidget(othersTab);

    QStringList tabName = {"Tab-1", "Tab-2", "Veto", "Extra2"};

    const int nTab = tabName.count();

    QWidget ** tabID = new QWidget * [nTab];

    for( int i = 0; i < nTab; i++){
      tabID [i]  = new QWidget(this);
      othersTab->addTab(tabID[i], tabName[i]);

      QGridLayout * tabLayout = new QGridLayout(tabID[i]);
      tabLayout->setSpacing(2);
      tabLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);

      QLabel * lb0 = new QLabel("Ch.", this); lb0->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb0, 0, 0);

      for( int ch = 0; ch < digi[ID]->GetNChannels(); ch++){
        QLabel * chid = new QLabel(QString::number(ch), this);
        chid->setAlignment(Qt::AlignRight | Qt::AlignCenter);
        chid->setFixedWidth(20);
        tabLayout->addWidget(chid, ch + 1, 0);

        if( i == 0 ){
          SetUpCheckBox(chkEnableRollOver[ID][ch],    "Enable Roll-Over Event", tabLayout, ch + 1, 2, DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl_PHA::EnableRollOverFlag, ch);
          SetUpCheckBox(chkEnablePileUp[ID][ch],         "Allow Pile-up Event", tabLayout, ch + 1, 3, DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl_PHA::EnablePileUpFlag, ch);
        }

        if( i == 1 ){
          if( ch == 0 ){
            QLabel * lb1 = new QLabel("Decimate Trace", this); lb1->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb1, 0, 2);
            QLabel * lb2 = new QLabel("Decimate Gain", this); lb2->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb2, 0, 4);
            QLabel * lb3 = new QLabel("Events per Agg. [G]", this); lb3->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb3, 0, 6);
          }
          SetUpComboBoxBit(cbDecimateTrace[ID][ch], "", tabLayout, ch + 1, 1, DPP::Bit_DPPAlgorithmControl_PHA::ListTraceDecimation, DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl_PHA::TraceDecimation, 1, ch);
          SetUpComboBoxBit(cbDecimateGain[ID][ch],  "", tabLayout, ch + 1, 3, DPP::Bit_DPPAlgorithmControl_PHA::ListDecimationGain, DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl_PHA::TraceDeciGain, 1, ch);
          SetUpSpinBox(sbNumEventAgg[ID][ch],       "", tabLayout, ch + 1, 5, DPP::NumberEventsPerAggregate_G, ch);
        }

        if( i == 2 ){
          if( ch == 0 ){
            QLabel * lb3 = new QLabel("Veto Source [G]", this); lb3->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb3, 0, 2);
            QLabel * lb4 = new QLabel("Veto Width", this); lb4->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb4, 0, 4);
            QLabel * lb5 = new QLabel("Veto Step", this); lb5->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb5, 0, 6);
          }
          SetUpComboBoxBit(cbVetoSource[ID][ch],    "", tabLayout, ch + 1, 1, DPP::PHA::Bit_DPPAlgorithmControl2::ListVetoSource, DPP::PHA::DPPAlgorithmControl2_G, DPP::PHA::Bit_DPPAlgorithmControl2::VetoSource, 1, ch);
          SetUpSpinBox(sbVetoWidth[ID][ch],         "", tabLayout, ch + 1, 3, DPP::VetoWidth, ch);
          SetUpComboBoxBit(cbVetoStep[ID][ch],      "", tabLayout, ch + 1, 5, DPP::Bit_VetoWidth::ListVetoStep, DPP::VetoWidth, DPP::Bit_VetoWidth::VetoStep, 1, ch);
        }

        if( i == 3 ){
          if( ch == 0 ){
            QLabel * lb2 = new QLabel("Extra2 Option [G]", this); lb2->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb2, 0, 2);
          }
          SetUpComboBoxBit(cbExtra2Option[ID][ch],  "", tabLayout, ch + 1, 1, DPP::PHA::Bit_DPPAlgorithmControl2::ListExtra2, DPP::PHA::DPPAlgorithmControl2_G, DPP::PHA::Bit_DPPAlgorithmControl2::Extra2Option, 2, ch);
        }
      }
    }
  }

  enableSignalSlot = true;
}

//&###########################################################
void DigiSettingsPanel::SetUpPSDBoard(){
  printf("============== %s \n", __func__);

  SetUpCheckBox(chkAutoDataFlush[ID],        "Auto Data Flush", bdCfgLayout[ID], 1, 0, DPP::BoardConfiguration, DPP::Bit_BoardConfig::EnableAutoDataFlush);
  SetUpCheckBox(chkTrigPropagation[ID],      "Trig. Propagate", bdCfgLayout[ID], 2, 0, DPP::BoardConfiguration, DPP::Bit_BoardConfig::TrigPropagation);  
  SetUpCheckBox(chkDecimateTrace[ID],    "Disable Digi. Trace", bdCfgLayout[ID], 3, 0, DPP::BoardConfiguration, DPP::Bit_BoardConfig::DisableDigiTrace);

  SetUpCheckBox(chkTraceRecording[ID],     "Record Trace", bdCfgLayout[ID], 2, 1, DPP::BoardConfiguration, DPP::Bit_BoardConfig::RecordTrace);
  SetUpCheckBox(chkEnableExtra2[ID],       "Enable Extra", bdCfgLayout[ID], 3, 1, DPP::BoardConfiguration, DPP::Bit_BoardConfig::EnableExtra2);

  SetUpComboBoxBit(cbAnaProbe1[ID],     "Ana. Probe ", bdCfgLayout[ID], 1, 2, DPP::Bit_BoardConfig::ListAnaProbe_PSD, DPP::BoardConfiguration, DPP::Bit_BoardConfig::AnaProbe_PSD, 1, 0);
  SetUpComboBoxBit(cbDigiProbe1[ID], "Digi. Probe 1 ", bdCfgLayout[ID], 3, 2, DPP::Bit_BoardConfig::ListDigiProbe1_PSD, DPP::BoardConfiguration, DPP::Bit_BoardConfig::DigiProbel1, 1, 0);
  SetUpComboBoxBit(cbDigiProbe2[ID], "Digi. Probe 2 ", bdCfgLayout[ID], 4, 2, DPP::Bit_BoardConfig::ListDigiProbe2_PSD, DPP::BoardConfiguration, DPP::Bit_BoardConfig::DigiProbel2, 1, 0);

}

void DigiSettingsPanel::SetUpPSDChannel(){

  QWidget * chAllSetting = new QWidget(this);
  //chAllSetting->setStyleSheet("background-color: #ECECEC;");
  chTab->addTab(chAllSetting, "Channel Settings");

  QWidget * chStatus = new QWidget(this);
  //chStatus->setStyleSheet("background-color: #ECECEC;");
  chTab->addTab(chStatus, "Status");

  QWidget * chInput = new QWidget(this);
  chTab->addTab(chInput, "Input");

  QWidget * chTrig = new QWidget(this);
  chTab->addTab(chTrig, "Trigger");

  QWidget * chTrap = new QWidget(this);
  chTab->addTab(chTrap, "Pulse Shape");

  QWidget * chOthers = new QWidget(this);
  chTab->addTab(chOthers, "Others");

  //^========================= All Channels
  QVBoxLayout * allSettingLayout = new QVBoxLayout(chAllSetting);
  allSettingLayout->setAlignment(Qt::AlignTop);
  allSettingLayout->setSpacing(2);

  QWidget * jaja = new QWidget(this);
  allSettingLayout->addWidget(jaja);

  QHBoxLayout * papa = new QHBoxLayout(jaja);
  papa->setAlignment(Qt::AlignLeft);

  {//^============================== Channel selection
    QLabel * lbChSel = new QLabel ("Ch : ", this);
    lbChSel->setAlignment(Qt::AlignCenter | Qt::AlignRight);
    papa->addWidget(lbChSel);
   
    chSelection[ID] = new RComboBox(this);
    chSelection[ID]->addItem("All Ch.", -1);
    for( int i = 0; i < digi[ID]->GetNChannels(); i++) chSelection[ID]->addItem(QString::number(i), i);
    papa->addWidget(chSelection[ID]);

    connect(chSelection[ID], &RComboBox::currentIndexChanged, this, [=](){
      SyncAllChannelsTab_PSD();
    });
  }

  {//*=============== input
    QGroupBox * inputBox = new QGroupBox("input Settings", this);
    allSettingLayout->addWidget(inputBox);

    QGridLayout  * inputLayout = new QGridLayout(inputBox);
    inputLayout->setSpacing(2);

    SetUpSpinBox(sbRecordLength[ID][MaxNChannels], "Record Length [G][ns] : ", inputLayout, 0, 0, DPP::RecordLength_G);
    SetUpComboBox(cbDynamicRange[ID][MaxNChannels],        "Dynamic Range : ", inputLayout, 0, 2, DPP::InputDynamicRange);
    SetUpSpinBox(sbPreTrigger[ID][MaxNChannels],        "Pre-Trigger [ns] : ", inputLayout, 1, 0, DPP::PreTrigger);
    SetUpSpinBox(sbThreshold[ID][MaxNChannels],          "Threshold [LSB] : ", inputLayout, 1, 2, DPP::PSD::TriggerThreshold);

    SetUpSpinBox(sbDCOffset[ID][MaxNChannels],             "DC Offset [%] : ", inputLayout, 2, 0, DPP::ChannelDCOffset);
    SetUpComboBoxBit(cbPolarity[ID][MaxNChannels],              "Polarity : ", inputLayout, 2, 2, DPP::Bit_DPPAlgorithmControl_PSD::ListPolarity, DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl_PSD::Polarity);

    SetUpCheckBox(chkChargePedestal[ID][MaxNChannels],     "Add Charge Pedestal", inputLayout, 3, 1, DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl_PSD::ChargePedestal);
    SetUpComboBoxBit(cbChargeSensitivity[ID][MaxNChannels], "Charge Sensitivity : ", inputLayout, 3, 2, DPP::Bit_DPPAlgorithmControl_PSD::ListChargeSensitivity_2Vpp, DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl_PSD::ChargeSensitivity);
    connect( cbDynamicRange[ID][MaxNChannels], &RComboBox::currentTextChanged, this, [=](QString text){

      cbChargeSensitivity[ID][MaxNChannels]->clear();
      cbChargeSensitivity[ID][MaxNChannels]->addItem("", -999);

      const std::vector<std::pair<std::string, unsigned int>> list = text.contains("0.5") ? DPP::Bit_DPPAlgorithmControl_PSD::ListChargeSensitivity_p5Vpp : DPP::Bit_DPPAlgorithmControl_PSD::ListChargeSensitivity_2Vpp;  
        
      for( int i = 0; i < (int) list.size(); i++) cbChargeSensitivity[ID][MaxNChannels]->addItem(QString::fromStdString(list[i].first), list[i].second);
      
    });

    SetUpCheckBox(chkBaseLineCal[ID][MaxNChannels],     "Baseline ReCal.", inputLayout, 4, 1, DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl_PSD::BaselineCal);
    SetUpComboBoxBit(cbBaseLineAvg[ID][MaxNChannels],  "Baseline Avg. : ", inputLayout, 4, 2, DPP::Bit_DPPAlgorithmControl_PSD::ListBaselineAvg, DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl_PSD::BaselineAvg);

    SetUpCheckBox(chkDiscardQLong[ID][MaxNChannels], "Discard QLong < QThr.", inputLayout, 5, 1, DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl_PSD::DiscardQLongSmallerQThreshold);
    SetUpSpinBox(sbChargeZeroSupZero[ID][MaxNChannels],     "Q-Threshold : ", inputLayout, 5, 2, DPP::PSD::ChargeZeroSuppressionThreshold);

    SetUpSpinBox(sbPSDCutThreshold[ID][MaxNChannels], "PSD Cut Threshold : ", inputLayout, 6, 0, DPP::PSD::ThresholdForPSDCut);
    SetUpCheckBox(chkCutBelow[ID][MaxNChannels],            "Cut Below Thr.", inputLayout, 7, 1, DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl_PSD::EnablePSDCutBelow);
    SetUpCheckBox(chkCutAbove[ID][MaxNChannels],            "Cut Above Thr.", inputLayout, 7, 2, DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl_PSD::EnablePSDCutAbove);
    SetUpCheckBox(chkRejOverRange[ID][MaxNChannels],      "Rej. Over-Range ", inputLayout, 7, 3, DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl_PSD::RejectOverRange);

  }

  {//*=================== Trigger
    QGroupBox * trigBox = new QGroupBox("Trigger Settings", this);
    allSettingLayout->addWidget(trigBox);

    QGridLayout  * trigLayout = new QGridLayout(trigBox);
    trigLayout->setSpacing(2);

    SetUpCheckBox(chkDisableOppositePulse[ID][MaxNChannels],  "Disable 0-Xing inhibit from opp. pulse", trigLayout, 0, 1, DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl_PSD::DisableOppositePolarityInhibitZeroCrossingOnCFD, -1, 2);
    SetUpCheckBox(chkDisableSelfTrigger[ID][MaxNChannels],                     "Disable Self Trigger ", trigLayout, 0, 3, DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl_PSD::DisableSelfTrigger);
    SetUpCheckBox(chkRejPileUp[ID][MaxNChannels],                                      "Rej. Pile-Up ", trigLayout, 1, 1, DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl_PSD::RejectPileup);
    SetUpCheckBox(chkPileUpInGate[ID][MaxNChannels],                                 "Pile-Up in Gate", trigLayout, 1, 3, DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl_PSD::PileupWithinGate);
    SetUpCheckBox(chkDisableTriggerHysteresis[ID][MaxNChannels],           "Disbale Trig. Hysteresis ", trigLayout, 2, 1, DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl_PSD::DisableTriggerHysteresis, -1, 2);
    SetUpComboBoxBit(cbTrigMode[ID][MaxNChannels],                                      "Trig Mode : ", trigLayout, 2, 2, DPP::Bit_DPPAlgorithmControl_PSD::ListTrigMode, DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl_PSD::TriggerMode);

    SetUpComboBoxBit(cbLocalTriggerValid[ID][MaxNChannels],                  "Local Trig. Valid. [G] : ", trigLayout, 3, 0, DPP::PSD::Bit_DPPAlgorithmControl2::ListLocalTrigValidMode, DPP::PSD::DPPAlgorithmControl2_G, DPP::PSD::Bit_DPPAlgorithmControl2::LocalTrigValidMode);
    SetUpSpinBox(sbTriggerHoldOff[ID][MaxNChannels],                           "Tigger Hold-off [ns] : ", trigLayout, 3, 2, DPP::PSD::TriggerHoldOffWidth);
    SetUpComboBoxBit(cbAdditionLocalTrigValid[ID][MaxNChannels],         "Local Trig. Valid. Opt [G] : ", trigLayout, 4, 0, DPP::PSD::Bit_DPPAlgorithmControl2::ListAdditionLocalTrigValid, DPP::PSD::DPPAlgorithmControl2_G, DPP::PSD::Bit_DPPAlgorithmControl2::AdditionLocalTrigValid);
    SetUpComboBoxBit(cbDiscriMode[ID][MaxNChannels],                                   "Discri. Mode : ", trigLayout, 4, 2, DPP::Bit_DPPAlgorithmControl_PSD::ListDiscriminationMode, DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl_PSD::DiscriminationMode);
    SetUpComboBoxBit(cbLocalShapedTrigger[ID][MaxNChannels],                 "Local Shaped Trig. [G] : ", trigLayout, 5, 0, DPP::PSD::Bit_DPPAlgorithmControl2::ListLocalShapeTrigMode, DPP::PSD::DPPAlgorithmControl2_G, DPP::PSD::Bit_DPPAlgorithmControl2::LocalShapeTriggerMode, 1);
    SetUpSpinBox(sbShapedTrigWidth[ID][MaxNChannels],                       "Shaped Trig. Width [ns] : ", trigLayout, 5, 2, DPP::PSD::ShapedTriggerWidth);

    SetUpComboBoxBit(cbTrigCount[ID][MaxNChannels],    "Trig. Counter Flag [G] : ", trigLayout, 6, 0, DPP::PSD::Bit_DPPAlgorithmControl2::ListTrigCounter, DPP::PSD::DPPAlgorithmControl2_G, DPP::PSD::Bit_DPPAlgorithmControl2::TriggerCounterFlag);
    SetUpSpinBox(sbTriggerLatency[ID][MaxNChannels],     "Trigger Latency [ns] : ", trigLayout, 6, 2, DPP::PSD::TriggerLatency);
    SetUpComboBoxBit(cbTriggerOpt[ID][MaxNChannels],        "Trigger Count opt : ", trigLayout, 7, 0, DPP::Bit_DPPAlgorithmControl_PSD::ListTriggerCountOpt, DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl_PSD::TriggerCountOpt, 2);
  }

  {//*===================== PSD
    QGroupBox * trapBox = new QGroupBox("PSD Settings", this);
    allSettingLayout->addWidget(trapBox);

    QGridLayout  * trapLayout = new QGridLayout(trapBox);
    trapLayout->setSpacing(2);

    SetUpSpinBox(sbShortGate[ID][MaxNChannels],             "Short Gate [ns] : ", trapLayout, 1, 0, DPP::PSD::ShortGateWidth);
    SetUpSpinBox(sbLongGate[ID][MaxNChannels],               "Long Gate [ns] : ", trapLayout, 1, 2, DPP::PSD::LongGateWidth);
    SetUpSpinBox(sbGateOffset[ID][MaxNChannels],           "Gate Offset [ns] : ", trapLayout, 2, 0, DPP::PSD::GateOffset);
    SetUpSpinBox(sbFixedBaseline[ID][MaxNChannels],          "Fixed Baseline : ", trapLayout, 2, 2, DPP::PSD::FixedBaseline);

    SetUpSpinBox(sbPURGAPThreshold[ID][MaxNChannels],     "PUR-GAP Threshold : ", trapLayout, 3, 0, DPP::PSD::PurGapThreshold);
    SetUpComboBoxBit(cbSmoothedChargeIntegration[ID][MaxNChannels], "Smooth Q-integr. [G] : ", trapLayout, 3, 2, DPP::PSD::Bit_DPPAlgorithmControl2::ListSmoothedChargeIntegration, DPP::PSD::DPPAlgorithmControl2_G, DPP::PSD::Bit_DPPAlgorithmControl2::SmoothedChargeIntegration, 1, 1);

    SetUpSpinBox(sbCFDDely[ID][MaxNChannels],                "CFD Delay [ns] : ", trapLayout, 4, 0, DPP::PSD::CFDSetting);
    SetUpComboBoxBit(cbCFDFraction[ID][MaxNChannels],          "CFD Fraction : ", trapLayout, 4, 2, DPP::PSD::Bit_CFDSetting::ListCFDFraction, DPP::PSD::CFDSetting, DPP::PSD::Bit_CFDSetting::CFDFraction, 1);
    SetUpComboBoxBit(cbCFDInterpolation[ID][MaxNChannels], "CFD interpolaton : ", trapLayout, 5, 0, DPP::PSD::Bit_CFDSetting::ListItepolation, DPP::PSD::CFDSetting, DPP::PSD::Bit_CFDSetting::Interpolation, 3);

  }

  {//*====================== Others
    QGroupBox * otherBox = new QGroupBox("Others Settings", this);
    allSettingLayout->addWidget(otherBox);
    
    QGridLayout  * otherLayout = new QGridLayout(otherBox);
    otherLayout->setSpacing(2);
   
    SetUpCheckBox(chkMarkSaturation[ID][MaxNChannels],          "Mark Saturation Pulse [G]", otherLayout,  0, 0, DPP::PSD::DPPAlgorithmControl2_G, DPP::PSD::Bit_DPPAlgorithmControl2::MarkSaturation, -1, 2);
    SetUpCheckBox(chkResetTimestampByTRGIN[ID][MaxNChannels],  "TRI-IN Reset Timestamp [G]", otherLayout,  0, 2, DPP::PSD::DPPAlgorithmControl2_G, DPP::PSD::Bit_DPPAlgorithmControl2::ResetTimestampByTRGIN, -1, 2);
    
    SetUpCheckBox(chkTestPule[ID][MaxNChannels],                       "Int. Test Pulse", otherLayout, 1, 0, DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl_PSD::InternalTestPulse);
    if( digi[ID]->GetBoardInfo().Model == CAEN_DGTZ_V1730 ){
      SetUpComboBoxBit(cbTestPulseRate[ID][MaxNChannels],  "Test Pulse Rate : ", otherLayout, 1, 2, DPP::Bit_DPPAlgorithmControl_PSD::ListTestPulseRate_730, DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl_PSD::TestPulseRate);
    }
    if( digi[ID]->GetBoardInfo().Model == CAEN_DGTZ_V1725 ){
      SetUpComboBoxBit(cbTestPulseRate[ID][MaxNChannels],  "Test Pulse Rate : ", otherLayout, 1, 2, DPP::Bit_DPPAlgorithmControl_PSD::ListTestPulseRate_725, DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl_PSD::TestPulseRate);
    }

    SetUpComboBoxBit(cbExtra2Option[ID][MaxNChannels],       "Extra word Option [G] : ", otherLayout, 2, 0, DPP::PSD::Bit_DPPAlgorithmControl2::ListExtraWordOpt, DPP::PSD::DPPAlgorithmControl2_G, DPP::PSD::Bit_DPPAlgorithmControl2::ExtraWordOption, 3);
    SetUpSpinBox(sbNumEventAgg[ID][MaxNChannels],          "Events per Agg. [G] : ", otherLayout, 3, 0, DPP::NumberEventsPerAggregate_G);

    SetUpComboBoxBit(cbVetoSource[ID][MaxNChannels],   "Veto Source [G] : ", otherLayout, 5, 0, DPP::PSD::Bit_DPPAlgorithmControl2::ListVetoSource, DPP::PSD::DPPAlgorithmControl2_G, DPP::PSD::Bit_DPPAlgorithmControl2::VetoSource);
    SetUpComboBoxBit(cbVetoMode[ID][MaxNChannels],       "Veto Mode [G] : ", otherLayout, 5, 2, DPP::PSD::Bit_DPPAlgorithmControl2::ListVetoMode, DPP::PSD::DPPAlgorithmControl2_G, DPP::PSD::Bit_DPPAlgorithmControl2::VetoMode);
    SetUpSpinBox(sbVetoWidth[ID][MaxNChannels],             "Veto Width : ", otherLayout, 6, 0, DPP::VetoWidth);
    SetUpComboBoxBit(cbVetoStep[ID][MaxNChannels],           "Veto Step : ", otherLayout, 6, 2, DPP::Bit_VetoWidth::ListVetoStep, DPP::VetoWidth, DPP::Bit_VetoWidth::VetoStep, 1);

  }

  {//^================== status
    QGridLayout * statusLayout = new QGridLayout(chStatus);
    statusLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    statusLayout->setSpacing(2);

    QLabel * lbCh   = new QLabel ("Ch.", this);      lbCh->setAlignment(Qt::AlignHCenter);   statusLayout->addWidget(lbCh, 0, 0);
    QLabel * lbLED  = new QLabel ("Status", this);   lbLED->setAlignment(Qt::AlignHCenter);  statusLayout->addWidget(lbLED, 0, 1, 1, 3);
    QLabel * lbTemp = new QLabel ("Temp [C]", this); lbTemp->setAlignment(Qt::AlignHCenter); statusLayout->addWidget(lbTemp, 0, 4);

    QStringList chStatusInfo = {"SPI bus is busy.", "ADC Calibration is done.", "ADC shutdown, over-heat"};

    for( int i = 0; i < MaxNChannels; i++){

      QLabel * lbChID = new QLabel (QString::number(i), this);      
      lbChID->setAlignment(Qt::AlignRight | Qt::AlignCenter);
      lbChID->setFixedWidth(20);
      statusLayout->addWidget(lbChID, i + 1, 0); 

      for( int j = 0; j < 3; j++ ){
        bnChStatus[ID][i][j] = new QPushButton(this);
        bnChStatus[ID][i][j]->setToolTip(chStatusInfo[j]);
        bnChStatus[ID][i][j]->setFixedSize(20, 20);
        statusLayout->addWidget(bnChStatus[ID][i][j], i + 1, j + 1);
      }
      leADCTemp[ID][i] = new QLineEdit(this);
      leADCTemp[ID][i]->setReadOnly(true);
      leADCTemp[ID][i]->setFixedWidth(100);
      statusLayout->addWidget(leADCTemp[ID][i], i +1, 3 + 1);
    }
    
    QPushButton * bnADCCali = new QPushButton("ADC Calibration", this);
    statusLayout->addWidget(bnADCCali, MaxNChannels + 1, 0, 1, 5);

    connect(bnADCCali, &QPushButton::clicked, this, [=](){
      digi[ID]->WriteRegister(DPP::ADCCalibration_W, 1);
      for( int i = 0 ; i < digi[ID]->GetNChannels(); i++ ) digi[ID]->ReadRegister(DPP::ChannelStatus_R, i);
      UpdatePanelFromMemory();
    });
  }

  {//^============================= input

    QVBoxLayout *inputLayout = new QVBoxLayout(chInput);

    QTabWidget * inputTab = new QTabWidget(this);
    inputLayout->addWidget(inputTab);

    QStringList tabName = {"Common Settings", "QLong", "PSD Cut", "Others"};

    const int nTab = tabName.count();

    QWidget ** tabID = new QWidget * [nTab];

    for( int i = 0; i < nTab; i++){   
      tabID [i]  = new QWidget(this);
      inputTab->addTab(tabID[i], tabName[i]);

      QGridLayout * tabLayout = new QGridLayout(tabID[i]);
      tabLayout->setSpacing(2);
      tabLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);

      QLabel * lb0 = new QLabel("Ch.", this); lb0->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb0, 0, 0);
      
      for( int ch = 0; ch < digi[ID]->GetNChannels(); ch++){
      
        QLabel * chid = new QLabel(QString::number(ch), this);
        chid->setAlignment(Qt::AlignRight | Qt::AlignCenter);
        chid->setFixedWidth(20);
        tabLayout->addWidget(chid, ch + 1, 0);

        if( i == 0 ) {
          if( ch == 0 ){
            QLabel * lb1 = new QLabel("Threshold", this); lb1->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb1, 0, 2);
            QLabel * lb2 = new QLabel("DC offset [%]", this); lb2->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb2, 0, 4);
            QLabel * lb3 = new QLabel("Record Length [G][ns]", this); lb3->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb3, 0, 6);
            QLabel * lb4 = new QLabel("Pre-Trigger [ns]", this); lb4->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb4, 0, 8);
            QLabel * lb5 = new QLabel("Dynamic Range", this); lb5->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb5, 0, 10);
            QLabel * lb6 = new QLabel("Polarity", this); lb6->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb6, 0, 12);
          }
          SetUpSpinBox(sbThreshold[ID][ch],       "", tabLayout, ch + 1, 1, DPP::PHA::TriggerThreshold, ch);
          SetUpSpinBox(sbDCOffset[ID][ch],        "", tabLayout, ch + 1, 3, DPP::ChannelDCOffset, ch);
          SetUpSpinBox(sbRecordLength[ID][ch],    "", tabLayout, ch + 1, 5, DPP::RecordLength_G, ch);
          SetUpSpinBox(sbPreTrigger[ID][ch],      "", tabLayout, ch + 1, 7, DPP::PreTrigger, ch);
          SetUpComboBox(cbDynamicRange[ID][ch],   "", tabLayout, ch + 1, 9, DPP::InputDynamicRange, ch);
          SetUpComboBoxBit(cbPolarity[ID][ch], "", tabLayout, ch + 1, 11, DPP::Bit_DPPAlgorithmControl_PHA::ListPolarity, DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl_PHA::Polarity, 1, ch);
        }

        if ( i == 1 ){
          if( ch == 0 ){
            QLabel * lb1 = new QLabel("Q-Threshold", this); lb1->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb1, 0, 2);
          }

          SetUpSpinBox(sbChargeZeroSupZero[ID][ch],                   "", tabLayout, ch + 1, 1, DPP::PSD::ChargeZeroSuppressionThreshold, ch);
          SetUpCheckBox(chkDiscardQLong[ID][ch], "Discard QLong < QThr.", tabLayout, ch + 1, 3, DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl_PSD::DiscardQLongSmallerQThreshold, ch);
        }

        if ( i == 2 ){
          if( ch == 0 ){
            QLabel * lb1 = new QLabel("PSD Cut Threshold", this); lb1->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb1, 0, 2);
          }

         SetUpSpinBox(sbPSDCutThreshold[ID][ch],                      "", tabLayout, ch + 1, 1, DPP::PSD::ThresholdForPSDCut, ch);
          SetUpCheckBox(chkCutBelow[ID][ch],           "Cut Below Thr. ", tabLayout, ch + 1, 3, DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl_PSD::EnablePSDCutBelow, ch);
          SetUpCheckBox(chkCutAbove[ID][ch],           "Cut Above Thr. ", tabLayout, ch + 1, 4, DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl_PSD::EnablePSDCutAbove, ch);
          SetUpCheckBox(chkRejOverRange[ID][ch],      "Rej. Over-Range ", tabLayout, ch + 1, 5, DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl_PSD::RejectOverRange, ch);
        }

        if( i == 3 ){

          if( ch == 0 ){
            QLabel * lb1 = new QLabel("Charge Sensitivity", this); lb1->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb1, 0, 3);
            QLabel * lb2 = new QLabel("Baseline Avg.", this);      lb2->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb2, 0, 6);
          }

          SetUpCheckBox(chkChargePedestal[ID][ch],     "Add Charge Pedestal", tabLayout, ch + 1, 1, DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl_PSD::ChargePedestal);
          SetUpComboBoxBit(cbChargeSensitivity[ID][ch],                   "", tabLayout, ch + 1, 2, DPP::Bit_DPPAlgorithmControl_PSD::ListChargeSensitivity_2Vpp, DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl_PSD::ChargeSensitivity);
          connect( cbDynamicRange[ID][ch], &RComboBox::currentTextChanged, this, [=](QString text){

            cbChargeSensitivity[ID][ch]->clear();
            cbChargeSensitivity[ID][ch]->addItem("", -999);

            const std::vector<std::pair<std::string, unsigned int>> list = text.contains("0.5") ? DPP::Bit_DPPAlgorithmControl_PSD::ListChargeSensitivity_p5Vpp : DPP::Bit_DPPAlgorithmControl_PSD::ListChargeSensitivity_2Vpp;  
            
            for( int i = 0; i < (int) list.size(); i++) cbChargeSensitivity[ID][ch]->addItem(QString::fromStdString(list[i].first), list[i].second);

          });

          SetUpCheckBox(chkBaseLineCal[ID][ch],     "Baseline ReCal.", tabLayout, ch + 1, 4, DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl_PSD::BaselineCal);
          SetUpComboBoxBit(cbBaseLineAvg[ID][ch],                  "", tabLayout, ch + 1, 5, DPP::Bit_DPPAlgorithmControl_PSD::ListBaselineAvg, DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl_PSD::BaselineAvg);
        }

      }

    }
  
  }

  {//^================================== Trigger 

    QVBoxLayout *trigLayout = new QVBoxLayout(chTrig);

    QTabWidget * trigTab = new QTabWidget(this);
    trigLayout->addWidget(trigTab);

    QStringList tabName = {"Common Settings", "Probably OK Setings", "Others", "Others-2", "Others-3"};

    const int nTab = tabName.count();

    QWidget ** tabID = new QWidget * [nTab];

    for( int i = 0; i < nTab; i++){   
      tabID [i]  = new QWidget(this);
      trigTab->addTab(tabID[i], tabName[i]);

      QGridLayout * tabLayout = new QGridLayout(tabID[i]);
      tabLayout->setSpacing(2);
      tabLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);

      QLabel * lb0 = new QLabel("Ch.", this); lb0->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb0, 0, 0);
      
      for( int ch = 0; ch < digi[ID]->GetNChannels(); ch++){
        QLabel * chid = new QLabel(QString::number(ch), this);
        chid->setAlignment(Qt::AlignRight | Qt::AlignCenter);
        chid->setFixedWidth(20);
        tabLayout->addWidget(chid, ch + 1, 0);

        if( i == 0 ){
          if( ch == 0 ){
            QLabel * lb0 = new QLabel("Trig Mode", this); lb0->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb0, 0, 4);
            QLabel * lb3 = new QLabel("Tigger Hold-off [ns]", this); lb3->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb3, 0, 6);
            QLabel * lb4 = new QLabel("Local Trig. Valid. [G]", this); lb4->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb4, 0, 8);
          }
          SetUpCheckBox(chkDisableSelfTrigger[ID][ch],  "Disable Self Trigger", tabLayout, ch + 1, 1, DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl_PHA::DisableSelfTrigger, ch);
          SetUpComboBoxBit(cbTrigMode[ID][ch],                              "", tabLayout, ch + 1, 3, DPP::Bit_DPPAlgorithmControl_PHA::ListTrigMode, DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl_PHA::TriggerMode, 1, ch);
          SetUpSpinBox(sbTriggerHoldOff[ID][ch],                            "", tabLayout, ch + 1, 5, DPP::PHA::TriggerHoldOffWidth, ch);
          SetUpComboBoxBit(cbLocalTriggerValid[ID][ch],                     "", tabLayout, ch + 1, 7, DPP::PHA::Bit_DPPAlgorithmControl2::ListLocalTrigValidMode, DPP::PHA::DPPAlgorithmControl2_G, DPP::PHA::Bit_DPPAlgorithmControl2::LocalTrigValidMode, 1, ch);
        }

        if( i == 1 ){
          if( ch == 0 ){
            QLabel * lb3 = new QLabel("Shaped Trig. Width [ns]", this); lb3->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb3, 0, 2);
            QLabel * lb2 = new QLabel("Local Shaped Trig. [G]", this); lb2->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb2, 0, 4);
            QLabel * lb1 = new QLabel("Trig. Counter Flag [G]", this); lb1->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb1, 0, 6);
          }
          SetUpSpinBox(sbShapedTrigWidth[ID][ch],        "", tabLayout, ch + 1, 1, DPP::PSD::ShapedTriggerWidth, ch);
          SetUpComboBoxBit(cbLocalShapedTrigger[ID][ch], "", tabLayout, ch + 1, 3, DPP::PHA::Bit_DPPAlgorithmControl2::ListLocalShapeTrigMode, DPP::PHA::DPPAlgorithmControl2_G, DPP::PHA::Bit_DPPAlgorithmControl2::LocalShapeTriggerMode, 1, ch);
          SetUpComboBoxBit(cbTrigCount[ID][ch],          "", tabLayout, ch + 1, 5, DPP::PHA::Bit_DPPAlgorithmControl2::ListTrigCounter, DPP::PHA::DPPAlgorithmControl2_G, DPP::PHA::Bit_DPPAlgorithmControl2::TriggerCounterFlag, 1, ch);

        }

        if( i == 2 ){
          SetUpCheckBox(chkDisableOppositePulse[ID][ch],  "Disable 0-Xing inhibit from opp. pulse", tabLayout, ch + 1, 1, DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl_PSD::DisableOppositePolarityInhibitZeroCrossingOnCFD, ch, 2);
          SetUpCheckBox(chkRejPileUp[ID][ch],                                      "Rej. Pile-Up ", tabLayout, ch + 1, 3, DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl_PSD::RejectPileup, ch);
        }

        if( i == 3){
          SetUpCheckBox(chkDisableTriggerHysteresis[ID][ch],           "Disbale Trig. Hysteresis ", tabLayout, ch + 1, 1, DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl_PSD::DisableTriggerHysteresis, ch, 2);
          SetUpCheckBox(chkPileUpInGate[ID][ch],                                 "Pile-Up in Gate", tabLayout, ch + 1, 3, DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl_PSD::PileupWithinGate, ch);

        }
        if( i == 4){
          if( ch == 0 ){
            QLabel * lb3 = new QLabel("Local Trig. Valid. Opt [G]", this); lb3->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb3, 0, 2);
            QLabel * lb2 = new QLabel("Discri. Mode", this);               lb2->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb2, 0, 4);
            QLabel * lb1 = new QLabel("Trigger Count opt", this);          lb1->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb1, 0, 6);
            QLabel * lb0 = new QLabel("Trigger Latency [ns]", this);       lb0->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb0, 0, 8);
          }
          SetUpComboBoxBit(cbAdditionLocalTrigValid[ID][ch], "", tabLayout, ch + 1, 1, DPP::PSD::Bit_DPPAlgorithmControl2::ListAdditionLocalTrigValid, DPP::PSD::DPPAlgorithmControl2_G, DPP::PSD::Bit_DPPAlgorithmControl2::AdditionLocalTrigValid, 1, ch);
          SetUpComboBoxBit(cbDiscriMode[ID][ch],             "", tabLayout, ch + 1, 3, DPP::Bit_DPPAlgorithmControl_PSD::ListDiscriminationMode, DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl_PSD::DiscriminationMode, 1, ch);
          SetUpComboBoxBit(cbTriggerOpt[ID][ch],             "", tabLayout, ch + 1, 5, DPP::Bit_DPPAlgorithmControl_PSD::ListTriggerCountOpt, DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl_PSD::TriggerCountOpt, 2, ch);
          SetUpSpinBox(sbTriggerLatency[ID][ch],             "", tabLayout, ch + 1, 7, DPP::PSD::TriggerLatency, ch);
        }

      }
    }

  }


  {//^================================== PSD 

    QVBoxLayout *trapLayout = new QVBoxLayout(chTrap);

    QTabWidget * trapTab = new QTabWidget(this);
    trapLayout->addWidget(trapTab);

    QStringList tabName = {"Common Settings", "CFD", "Others"};

    const int nTab = tabName.count();

    QWidget ** tabID = new QWidget * [nTab];

    for( int i = 0; i < nTab; i++){   
      tabID [i]  = new QWidget(this);
      trapTab->addTab(tabID[i], tabName[i]);

      QGridLayout * tabLayout = new QGridLayout(tabID[i]);
      tabLayout->setSpacing(2);
      tabLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);

      QLabel * lb0 = new QLabel("Ch.", this); lb0->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb0, 0, 0);
      
      for( int ch = 0; ch < digi[ID]->GetNChannels(); ch++){
      
        QLabel * chid = new QLabel(QString::number(ch), this);
        chid->setAlignment(Qt::AlignRight | Qt::AlignCenter);
        chid->setFixedWidth(20);
        tabLayout->addWidget(chid, ch + 1, 0);


        if( i == 0 ) {
          if( ch == 0 ){
            QLabel * lb1 = new QLabel("Short Gate [ns]", this); lb1->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb1, 0, 2);
            QLabel * lb2 = new QLabel("Long Gate [ns]", this); lb2->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb2, 0, 4);
            QLabel * lb3 = new QLabel("Gate Offset [ns]", this); lb3->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb3, 0, 6);
            QLabel * lb4 = new QLabel("Fixed Baseline", this); lb4->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb4, 0, 8);
          }

          SetUpSpinBox(sbShortGate[ID][ch],            "", tabLayout, ch + 1, 1, DPP::PSD::ShortGateWidth, ch);
          SetUpSpinBox(sbLongGate[ID][ch],             "", tabLayout, ch + 1, 3, DPP::PSD::LongGateWidth, ch);
          SetUpSpinBox(sbGateOffset[ID][ch],           "", tabLayout, ch + 1, 5, DPP::PSD::GateOffset, ch);
          SetUpSpinBox(sbFixedBaseline[ID][ch],        "", tabLayout, ch + 1, 7, DPP::PSD::FixedBaseline, ch);
          
        }

        if ( i == 1 ){
          if( ch == 0 ){
            QLabel * lb1 = new QLabel("CFD Delay [ns]", this); lb1->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb1, 0, 2);
            QLabel * lb2 = new QLabel("CFD Fraction", this); lb2->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb2, 0, 4);
            QLabel * lb3 = new QLabel("CFD interpolaton", this); lb3->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb3, 0, 6);
          }
 
          SetUpSpinBox(sbCFDDely[ID][ch],              "", tabLayout, ch + 1, 1, DPP::PSD::CFDSetting, ch);
          SetUpComboBoxBit(cbCFDFraction[ID][ch],      "", tabLayout, ch + 1, 3, DPP::PSD::Bit_CFDSetting::ListCFDFraction, DPP::PSD::CFDSetting, DPP::PSD::Bit_CFDSetting::CFDFraction, 1, ch);
          SetUpComboBoxBit(cbCFDInterpolation[ID][ch], "", tabLayout, ch + 1, 5, DPP::PSD::Bit_CFDSetting::ListItepolation, DPP::PSD::CFDSetting, DPP::PSD::Bit_CFDSetting::Interpolation, 2, ch);
        }

        if( i == 2){
          if( ch == 0){
            QLabel * lb5 = new QLabel("PUR-GAP Threshold", this); lb5->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb5, 0, 2);
            QLabel * lb6 = new QLabel("Smooth Q-integr. [G]", this); lb6->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb6, 0, 4);
          }
          SetUpSpinBox(sbPURGAPThreshold[ID][ch],               "", tabLayout, ch + 1, 1, DPP::PSD::PurGapThreshold, ch);
          SetUpComboBoxBit(cbSmoothedChargeIntegration[ID][ch], "", tabLayout, ch + 1, 3, DPP::PSD::Bit_DPPAlgorithmControl2::ListSmoothedChargeIntegration, DPP::PSD::DPPAlgorithmControl2_G, DPP::PSD::Bit_DPPAlgorithmControl2::SmoothedChargeIntegration, 1, ch);
        }

      }
    }
  }

  {//^======================================== Others
    QVBoxLayout *otherLayout = new QVBoxLayout(chOthers);

    QTabWidget * othersTab = new QTabWidget(this);
    otherLayout->addWidget(othersTab);

    QStringList tabName = {"Tab-1", "Test Pulse", "Veto", "Extra2"};

    const int nTab = tabName.count();

    QWidget ** tabID = new QWidget * [nTab];

    for( int i = 0; i < nTab; i++){
      tabID [i]  = new QWidget(this);
      othersTab->addTab(tabID[i], tabName[i]);

      QGridLayout * tabLayout = new QGridLayout(tabID[i]);
      tabLayout->setSpacing(2);
      tabLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);

      QLabel * lb0 = new QLabel("Ch.", this); lb0->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb0, 0, 0);

      for( int ch = 0; ch < digi[ID]->GetNChannels(); ch++){
        QLabel * chid = new QLabel(QString::number(ch), this);
        chid->setAlignment(Qt::AlignRight | Qt::AlignCenter);
        chid->setFixedWidth(20);
        tabLayout->addWidget(chid, ch + 1, 0);

        if( i == 0 ){
          if( ch == 0 ){
            QLabel * lb3 = new QLabel("Events per Agg. [G]", this); lb3->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb3, 0, 6);
          }
          SetUpCheckBox(chkMarkSaturation[ID][ch],          "Mark Saturation Pulse [G]", tabLayout,  ch + 1, 1, DPP::PSD::DPPAlgorithmControl2_G, DPP::PSD::Bit_DPPAlgorithmControl2::MarkSaturation, ch, 2);
          SetUpCheckBox(chkResetTimestampByTRGIN[ID][ch],  "TRI-IN Reset Timestamp [G]", tabLayout,  ch + 1, 3, DPP::PSD::DPPAlgorithmControl2_G, DPP::PSD::Bit_DPPAlgorithmControl2::ResetTimestampByTRGIN, ch, 2);
          SetUpSpinBox(sbNumEventAgg[ID][ch],       "", tabLayout, ch + 1, 5, DPP::NumberEventsPerAggregate_G, ch);
        }

        if( i == 1 ){
          if( ch == 0 ){
            QLabel * lb3 = new QLabel("Test Pulse Rate", this); lb3->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb3, 0, 3);
          }
          SetUpCheckBox(chkTestPule[ID][ch],   "Int. Test Pulse", tabLayout, ch + 1, 1, DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl_PSD::InternalTestPulse);
          if( digi[ID]->GetBoardInfo().Model == CAEN_DGTZ_V1730 ){
            SetUpComboBoxBit(cbTestPulseRate[ID][ch],  "", tabLayout, ch + 1, 2, DPP::Bit_DPPAlgorithmControl_PSD::ListTestPulseRate_730, DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl_PSD::TestPulseRate, 1, ch);
          }
          if( digi[ID]->GetBoardInfo().Model == CAEN_DGTZ_V1725 ){
            SetUpComboBoxBit(cbTestPulseRate[ID][ch],  "", tabLayout, ch + 1, 2, DPP::Bit_DPPAlgorithmControl_PSD::ListTestPulseRate_725, DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl_PSD::TestPulseRate, 1, ch);
          }
        }

        if( i == 2 ){
          if( ch == 0 ){
            QLabel * lb3 = new QLabel("Veto Source [G]", this); lb3->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb3, 0, 2);
            QLabel * lb2 = new QLabel("Veto Mode [G]", this); lb2->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb2, 0, 4);
            QLabel * lb4 = new QLabel("Veto Width", this); lb4->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb4, 0, 6);
            QLabel * lb5 = new QLabel("Veto Step", this); lb5->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb5, 0, 8);
          }
          SetUpComboBoxBit(cbVetoSource[ID][ch],    "", tabLayout, ch + 1, 1, DPP::PHA::Bit_DPPAlgorithmControl2::ListVetoSource, DPP::PHA::DPPAlgorithmControl2_G, DPP::PHA::Bit_DPPAlgorithmControl2::VetoSource, 1, ch);
          SetUpComboBoxBit(cbVetoMode[ID][ch],      "", tabLayout, ch + 1, 3, DPP::PSD::Bit_DPPAlgorithmControl2::ListVetoMode, DPP::PSD::DPPAlgorithmControl2_G, DPP::PSD::Bit_DPPAlgorithmControl2::VetoMode, 1, ch);
          SetUpSpinBox(sbVetoWidth[ID][ch],         "", tabLayout, ch + 1, 5, DPP::VetoWidth, ch);
          SetUpComboBoxBit(cbVetoStep[ID][ch],      "", tabLayout, ch + 1, 7, DPP::Bit_VetoWidth::ListVetoStep, DPP::VetoWidth, DPP::Bit_VetoWidth::VetoStep, 1, ch);
        }

        if( i == 3 ){
          if( ch == 0 ){
            QLabel * lb2 = new QLabel("Extra2 Option [G]", this); lb2->setAlignment(Qt::AlignHCenter); tabLayout->addWidget(lb2, 0, 2);
          }
          SetUpComboBoxBit(cbExtra2Option[ID][ch],  "", tabLayout, ch + 1, 1, DPP::PHA::Bit_DPPAlgorithmControl2::ListExtra2, DPP::PHA::DPPAlgorithmControl2_G, DPP::PHA::Bit_DPPAlgorithmControl2::Extra2Option, 2, ch);
        }
      }
    }
  }


}

//&###########################################################
void DigiSettingsPanel::UpdatePanelFromMemory(){

  printf("============== %s \n", __func__);

  enableSignalSlot = false;

  //*========================================
  uint32_t AcqStatus = digi[ID]->GetSettingFromMemory(DPP::AcquisitionStatus_R);
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

  //*========================================
  uint32_t BdFailStatus = digi[ID]->GetSettingFromMemory(DPP::BoardFailureStatus_R);
  for( int i = 0; i < 3; i++){
    if( Digitizer::ExtractBits(BdFailStatus, {1, BdFailToolTip[i].second}) == 0 ){
      bnBdFailStatus[ID][i]->setStyleSheet("background-color: green;");
      bnBdFailStatus[ID][i]->setToolTip(BdFailToolTip[i].first.first);
    }else{
      bnBdFailStatus[ID][i]->setStyleSheet("background-color: red;");
      bnBdFailStatus[ID][i]->setToolTip(BdFailToolTip[i].first.second);
    }
  }

  //*========================================
  uint32_t ReadoutStatus = digi[ID]->GetSettingFromMemory(DPP::ReadoutStatus_R);
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

  //*========================================
  uint32_t BdCfg = digi[ID]->GetSettingFromMemory(DPP::BoardConfiguration);

  chkAutoDataFlush[ID]->setChecked(   Digitizer::ExtractBits(BdCfg, DPP::Bit_BoardConfig::EnableAutoDataFlush) );
  chkTrigPropagation[ID]->setChecked( Digitizer::ExtractBits(BdCfg, DPP::Bit_BoardConfig::TrigPropagation) );
  chkDecimateTrace[ID]->setChecked(   Digitizer::ExtractBits(BdCfg, DPP::Bit_BoardConfig::DecimateTrace) );
  chkTraceRecording[ID]->setChecked(  Digitizer::ExtractBits(BdCfg, DPP::Bit_BoardConfig::TrigPropagation) );
  chkEnableExtra2[ID]->setChecked(    Digitizer::ExtractBits(BdCfg, DPP::Bit_BoardConfig::EnableExtra2) );

  int temp = Digitizer::ExtractBits(BdCfg, DPP::Bit_BoardConfig::DigiProbel1);
  for(int i = 0; i < cbDigiProbe1[ID]->count(); i++){
    if( cbDigiProbe1[ID]->itemData(i).toInt() == temp) {
      cbDigiProbe1[ID]->setCurrentIndex(i);
      break;
    }
  }
  temp = Digitizer::ExtractBits(BdCfg, DPP::Bit_BoardConfig::DigiProbel2);
  for(int i = 0; i < cbDigiProbe2[ID]->count(); i++){
    if( cbDigiProbe2[ID]->itemData(i).toInt() == temp) {
      cbDigiProbe2[ID]->setCurrentIndex(i);
      break;
    }
  }
  
  if( digi[ID]->GetDPPType() == V1730_DPP_PHA_CODE ) {
    chkDualTrace[ID]->setChecked(       Digitizer::ExtractBits(BdCfg, DPP::Bit_BoardConfig::DualTrace) );
    
    temp = Digitizer::ExtractBits(BdCfg, DPP::Bit_BoardConfig::AnalogProbe1);
    for( int i = 0; i < cbAnaProbe1[ID]->count(); i++){
      if( cbAnaProbe1[ID]->itemData(i).toInt() == temp) {
        cbAnaProbe1[ID]->setCurrentIndex(i);
        break;
      }
    }

    temp = Digitizer::ExtractBits(BdCfg, DPP::Bit_BoardConfig::AnalogProbe2);
    for(int i = 0; i < cbAnaProbe2[ID]->count(); i++){
      if( cbAnaProbe2[ID]->itemData(i).toInt() == temp) {
        cbAnaProbe2[ID]->setCurrentIndex(i);
        break;
      }
    }
  }

  if( digi[ID]->GetDPPType() == V1730_DPP_PSD_CODE ) {
    temp = Digitizer::ExtractBits(BdCfg, DPP::Bit_BoardConfig::AnaProbe_PSD);
    for( int i = 0; i < cbAnaProbe1[ID]->count(); i++){
      if( cbAnaProbe1[ID]->itemData(i).toInt() == temp) {
        cbAnaProbe1[ID]->setCurrentIndex(i);
        break;
      }
    }
  }
  //*========================================
  uint32_t chMask = digi[ID]->GetSettingFromMemory(DPP::ChannelEnableMask);
  for( int i = 0; i < MaxNChannels; i++){
    if( (chMask >> i ) & 0x1 ) {
      bnChEnableMask[ID][i]->setStyleSheet("background-color: green;");
    }else{
      bnChEnableMask[ID][i]->setStyleSheet("");
    }
  }

  //*========================================
  uint32_t aggOrg = digi[ID]->GetSettingFromMemory(DPP::AggregateOrganization);
  for( int i = 0; i < cbAggOrg[ID]->count(); i++){
    if( cbAggOrg[ID]->itemData(i).toUInt() == aggOrg ){
      cbAggOrg[ID]->setCurrentIndex(i);
      break;
    }
  }

  sbAggNum[ID]->setValue(digi[ID]->GetSettingFromMemory(DPP::MaxAggregatePerBlockTransfer));

  chkEnableExternalTrigger[ID]->setChecked( ! ( digi[ID]->GetSettingFromMemory(DPP::DisableExternalTrigger) & 0x1) );

  sbRunDelay[ID]->setValue(digi[ID]->GetSettingFromMemory(DPP::RunStartStopDelay));

  //*========================================
  uint32_t anaMonitor = digi[ID]->GetSettingFromMemory(DPP::AnalogMonitorMode);
  for( int i = 0 ; i < cbAnalogMonitorMode[ID]->count(); i++){
    if( cbAnalogMonitorMode[ID]->itemData(i).toUInt() == anaMonitor ){ 
      cbAnalogMonitorMode[ID]->setCurrentIndex(i);
      break;
    }
  }

  {
    int index = cbAnalogMonitorMode[ID]->currentIndex();
    sbBufferGain[ID]->setEnabled(( index == 2 ));
    sbVoltageLevel[ID]->setEnabled(( index == 3 ));
  }

  sbBufferGain[ID]->setValue(digi[ID]->GetSettingFromMemory(DPP::BufferOccupancyGain));
  sbVoltageLevel[ID]->setValue(digi[ID]->GetSettingFromMemory(DPP::VoltageLevelModeConfig));

  //*========================================
  uint32_t frontPanel = digi[ID]->GetSettingFromMemory(DPP::FrontPanelIOControl);
  cbLEMOMode[ID]->setCurrentIndex( ( frontPanel & 0x1 ));

  if( (frontPanel >> 1 ) & 0x1 ) { // bit-1, TRIG-OUT high impedance, i.e. disable
    cbTRGOUTMode[ID]->setCurrentIndex(0);
  }else{
    unsigned short trgOutBit = ((frontPanel >> 14 ) & 0x3F ) << 14 ;

    for( int i = 0; i < cbTRGOUTMode[ID]->count() ; i++ ){
      if( cbTRGOUTMode[ID]->itemData(i).toUInt() == trgOutBit ){
        cbTRGOUTMode[ID]->setCurrentIndex(i);
        break;
      }
    }
  }

  //*========================================
  uint32_t glbTrgMask = digi[ID]->GetSettingFromMemory(DPP::GlobalTriggerMask);

  for( int i = 0; i < MaxNChannels/2; i++){
    if( (glbTrgMask >> i ) & 0x1 ){
      bnGlobalTriggerMask[ID][i]->setStyleSheet("background-color: green;");
    }else{
      bnGlobalTriggerMask[ID][i]->setStyleSheet("");
    }
  }

  sbGlbMajCoinWin[ID]->setValue( Digitizer::ExtractBits(glbTrgMask, DPP::Bit_GlobalTriggerMask::MajorCoinWin) );
  sbGlbMajLvl[ID]->setValue( Digitizer::ExtractBits(glbTrgMask, DPP::Bit_GlobalTriggerMask::MajorLevel) );
  cbGlbUseOtherTriggers[ID]->setCurrentIndex(Digitizer::ExtractBits(glbTrgMask, {2, 30}));

  //*========================================
  uint32_t TRGOUTMask = digi[ID]->GetSettingFromMemory(DPP::FrontPanelTRGOUTEnableMask);
  for( int i = 0; i < MaxNChannels/2; i++){
    if( (TRGOUTMask >> i ) & 0x1 ){
      bnTRGOUTMask[ID][i]->setStyleSheet("background-color: green;");
    }else{
      bnTRGOUTMask[ID][i]->setStyleSheet("");
    }
  }

  cbTRGOUTLogic[ID]->setCurrentIndex(Digitizer::ExtractBits(TRGOUTMask, DPP::Bit_TRGOUTMask::TRGOUTLogic));
  sbTRGOUTMajLvl[ID]->setEnabled( Digitizer::ExtractBits(TRGOUTMask, DPP::Bit_TRGOUTMask::TRGOUTLogic) == 2 );
  sbTRGOUTMajLvl[ID]->setValue( Digitizer::ExtractBits(TRGOUTMask, DPP::Bit_TRGOUTMask::MajorLevel));
  cbTRGOUTUseOtherTriggers[ID]->setCurrentIndex(Digitizer::ExtractBits(TRGOUTMask, {2, 30}));

  //*========================================
  uint32_t readoutCtl = digi[ID]->GetSettingFromMemory(DPP::ReadoutControl);

  sbVMEInterruptLevel[ID]->setValue(readoutCtl & 0x7);
  chkEnableOpticalInterrupt[ID]->setChecked( Digitizer::ExtractBits(readoutCtl, DPP::Bit_ReadoutControl::EnableOpticalLinkInpt));
  chkEnableVMEReadoutStatus[ID]->setChecked( Digitizer::ExtractBits(readoutCtl, DPP::Bit_ReadoutControl::EnableEventAligned));
  chkEnableVME64Aligment[ID]->setChecked( Digitizer::ExtractBits(readoutCtl, DPP::Bit_ReadoutControl::VMEAlign64Mode));
  chkEnableAddRelocation[ID]->setChecked( Digitizer::ExtractBits(readoutCtl, DPP::Bit_ReadoutControl::VMEBaseAddressReclocated));
  chkEnableExtendedBlockTransfer[ID]->setChecked( Digitizer::ExtractBits(readoutCtl, DPP::Bit_ReadoutControl::EnableExtendedBlockTransfer));
  cbInterruptMode[ID]->setCurrentIndex(Digitizer::ExtractBits(readoutCtl, DPP::Bit_ReadoutControl::InterrupReleaseMode));

  //*========================================
  for( int i = 0; i < MaxNChannels/2; i++){
    uint32_t trigger = digi[ID]->GetSettingFromMemory(DPP::TriggerValidationMask_G,  2*i );

    cbMaskLogic[ID][i]->setCurrentIndex( (trigger >> 8 ) & 0x3 );
    chkMaskExtTrigger[ID][i]->setEnabled((((trigger >> 8 ) & 0x3) != 2));

    sbMaskMajorLevel[ID][i]->setValue( ( trigger >> 10 ) & 0x3 );
    chkMaskExtTrigger[ID][i]->setChecked( ( trigger >> 30 ) & 0x1 );
    chkMaskSWTrigger[ID][i]->setChecked( ( trigger >> 31 ) & 0x1 );

    for( int j = 0; j < MaxNChannels/2; j++){
      if( ( trigger >> j ) & 0x1 ) {
        bnTriggerMask[ID][i][j]->setStyleSheet("background-color: green;");
      }else{
        bnTriggerMask[ID][i][j]->setStyleSheet("");
      }
    }
  }

  //*========================================== Channel Status
  for( int i = 0; i < MaxNChannels; i++){
    uint32_t chStatus = digi[ID]->GetSettingFromMemory(DPP::ChannelStatus_R, i);

    bnChStatus[ID][i][0]->setStyleSheet( ( (chStatus >> 2 ) & 0x1 ) ? "background-color: red;" : "");
    bnChStatus[ID][i][1]->setStyleSheet( ( (chStatus >> 3 ) & 0x1 ) ? "background-color: green;" : "");
    bnChStatus[ID][i][2]->setStyleSheet( ( (chStatus >> 8 ) & 0x1 ) ? "background-color: red;" : "");

    leADCTemp[ID][i]->setText( QString::number( digi[ID]->GetSettingFromMemory(DPP::ChannelADCTemperature_R, i) ) );
  }

  if( digi[ID]->GetDPPType() == V1730_DPP_PHA_CODE ) UpdatePHASetting();
  if( digi[ID]->GetDPPType() == V1730_DPP_PSD_CODE ) UpdatePSDSetting();

  enableSignalSlot = true;
}

//&###########################################################
void DigiSettingsPanel::UpdateSpinBox(RSpinBox * &sb, Reg para, int ch){

  int ch2ns = digi[ID]->GetCh2ns();
  int pStep = para.GetPartialStep();

  uint32_t value = digi[ID]->GetSettingFromMemory(para, ch);

  //para == DCoffset
  if( para == DPP::ChannelDCOffset ){
    sb->setValue( 100. - value * 100. / 0xFFFF);
    return;
  }

  sb->setValue( pStep > 0 ? value * pStep * ch2ns : value);

  //printf("%d, %s | %d %d %u, %f\n", para.GetNameChar(), ch, ch2ns, pStep, value, sb->value());

}

void DigiSettingsPanel::UpdateComboBox(RComboBox * & cb, Reg para, int ch){

  uint32_t value = digi[ID]->GetSettingFromMemory(para, ch);

  for( int i = 0; i < cb->count(); i ++){
    if( cb->itemData(i).toUInt() == value ) {
      cb->setCurrentIndex(i);
      break;
    }
  }
}

void DigiSettingsPanel::UpdateComboBoxBit(RComboBox * & cb, uint32_t fullBit, std::pair<unsigned short, unsigned short> bit){

  int temp = Digitizer::ExtractBits(fullBit, bit);
  for( int i = 0; i < cb->count(); i++){
    if( cb->itemData(i).toInt() == temp) {
      cb->setCurrentIndex(i);
      break;
    }
  }

}

void DigiSettingsPanel::SyncSpinBox(RSpinBox *(&spb)[][MaxNChannels+1]){
  if( !enableSignalSlot ) return;

  const int nCh = digi[ID]->GetNChannels();

  int ch = chSelection[ID]->currentData().toInt();

  if( ch >= 0 ){
    enableSignalSlot = false;    
    spb[ID][nCh]->setValue( spb[ID][ch]->value());
    enableSignalSlot = true;
  }else{
    //check is all SpinBox has same value;
    int count = 1;
    const int value = spb[ID][0]->value();
    for( int i = 1; i < nCh; i ++){
      if( spb[ID][i]->value() == value ) count++;
    }

    //printf("%d =? %d , %d, %f\n", count, nCh, value, spb[ID][0]->value());
    enableSignalSlot = false;
    if( count != nCh ){
       spb[ID][nCh]->setValue(-1);
    }else{
       spb[ID][nCh]->setValue(value);
    }
    enableSignalSlot = true;
  }

}

void DigiSettingsPanel::SyncComboBox(RComboBox *(&cb)[][MaxNChannels+1]){
  if( !enableSignalSlot ) return;

  const int nCh = digi[ID]->GetNChannels();

  int ch = chSelection[ID]->currentData().toInt();

  if( ch >= 0 ){
    enableSignalSlot = false;
    cb[ID][nCh]->setCurrentText(cb[ID][ch]->currentText());
    enableSignalSlot = true;
  }else{
    //check is all SpinBox has same value;
    int count = 1;
    const QString text = cb[ID][0]->currentText();
    for( int i = 1; i < nCh; i ++){
      if( cb[ID][i]->currentText() == text ) count++;
    }

    //printf("%d =? %d , %s\n", count, nCh, text.toStdString().c_str());
    enableSignalSlot = false;
    if( count != nCh ){
       cb[ID][nCh]->setCurrentText("");
    }else{
       cb[ID][nCh]->setCurrentText(text);
    }
    enableSignalSlot = true;
  }
}

void DigiSettingsPanel::SyncCheckBox(QCheckBox *(&chk)[][MaxNChannels+1]){
  if( !enableSignalSlot ) return;

  const int nCh = digi[ID]->GetNChannels();
  int ch = chSelection[ID]->currentData().toInt();
  if( ch >= 0 ){
    enableSignalSlot = false;
    chk[ID][nCh]->setCheckState(chk[ID][ch]->checkState());
    enableSignalSlot = true;
  }else{
    //check is all SpinBox has same value;
    int count = 1;
    const Qt::CheckState state = chk[ID][0]->checkState();
    for( int i = 1; i < nCh; i ++){
      if( chk[ID][i]->checkState() == state ) count++;
    }

    //printf("%d =? %d , %s\n", count, nCh, text.toStdString().c_str());
    enableSignalSlot = false;
    if( count != nCh ){
       chk[ID][nCh]->setCheckState(Qt::PartiallyChecked);
    }else{
       chk[ID][nCh]->setCheckState(state);
    }
    enableSignalSlot = true;
  }
}

void DigiSettingsPanel::SyncAllChannelsTab_PHA(){

  SyncSpinBox(sbRecordLength);
  SyncSpinBox(sbPreTrigger);
  SyncSpinBox(sbInputRiseTime);
  SyncSpinBox(sbThreshold);
  SyncSpinBox(sbRiseTimeValidWin);
  SyncSpinBox(sbTriggerHoldOff);
  SyncSpinBox(sbShapedTrigWidth);
  SyncSpinBox(sbDCOffset);
  SyncSpinBox(sbTrapRiseTime);
  SyncSpinBox(sbTrapFlatTop);
  SyncSpinBox(sbDecay);
  SyncSpinBox(sbTrapScaling);
  SyncSpinBox(sbPeaking);
  SyncSpinBox(sbPeakingHoldOff);
  SyncSpinBox(sbFineGain);
  SyncSpinBox(sbNumEventAgg);
  SyncSpinBox(sbVetoWidth);

  SyncComboBox(cbDynamicRange);
  SyncComboBox(cbRCCR2Smoothing);
  SyncComboBox(cbDecimateTrace);
  SyncComboBox(cbPolarity);
  SyncComboBox(cbPeakAvg);
  SyncComboBox(cbBaseLineAvg);
  SyncComboBox(cbDecimateGain);
  SyncComboBox(cbTrigMode);
  SyncComboBox(cbLocalTriggerValid);
  SyncComboBox(cbLocalShapedTrigger);
  SyncComboBox(cbVetoSource);
  SyncComboBox(cbTrigCount);
  SyncComboBox(cbExtra2Option);
  SyncComboBox(cbVetoStep);

  SyncCheckBox(chkDisableSelfTrigger);
  SyncCheckBox(chkEnableRollOver);
  SyncCheckBox(chkEnablePileUp);
  SyncCheckBox(chkActiveBaseline);
  SyncCheckBox(chkBaselineRestore);
  SyncCheckBox(chkTagCorrelation);

}


void DigiSettingsPanel::UpdatePHASetting(){

  enableSignalSlot = false;

  printf("------ %s \n", __func__);

  for( int ch = 0; ch < digi[ID]->GetNChannels(); ch ++){
    UpdateSpinBox(sbRecordLength[ID][ch],     DPP::RecordLength_G, ch);
    UpdateSpinBox(sbPreTrigger[ID][ch],       DPP::PreTrigger, ch);
    UpdateSpinBox(sbInputRiseTime[ID][ch],    DPP::PHA::InputRiseTime, ch);
    UpdateSpinBox(sbThreshold[ID][ch],        DPP::PHA::TriggerThreshold, ch);
    UpdateSpinBox(sbRiseTimeValidWin[ID][ch], DPP::PHA::RiseTimeValidationWindow, ch);
    UpdateSpinBox(sbTriggerHoldOff[ID][ch],   DPP::PHA::TriggerHoldOffWidth, ch);
    UpdateSpinBox(sbShapedTrigWidth[ID][ch],  DPP::PHA::ShapedTriggerWidth, ch);
    UpdateSpinBox(sbDCOffset[ID][ch],         DPP::ChannelDCOffset, ch);
    UpdateSpinBox(sbTrapRiseTime[ID][ch],     DPP::PHA::TrapezoidRiseTime, ch);
    UpdateSpinBox(sbTrapFlatTop[ID][ch],      DPP::PHA::TrapezoidFlatTop, ch);
    UpdateSpinBox(sbDecay[ID][ch],            DPP::PHA::DecayTime, ch);
    UpdateSpinBox(sbTrapScaling[ID][ch],      DPP::PHA::DPPAlgorithmControl2_G, ch);
    UpdateSpinBox(sbPeaking[ID][ch],          DPP::PHA::PeakingTime, ch);
    UpdateSpinBox(sbPeakingHoldOff[ID][ch],   DPP::PHA::PeakHoldOff, ch);
    UpdateSpinBox(sbFineGain[ID][ch],         DPP::PHA::FineGain, ch);
    UpdateSpinBox(sbNumEventAgg[ID][ch],      DPP::NumberEventsPerAggregate_G, ch);
    UpdateSpinBox(sbVetoWidth[ID][ch],        DPP::VetoWidth, ch);

    UpdateComboBox(cbDynamicRange[ID][ch],    DPP::InputDynamicRange, ch);
    UpdateComboBox(cbRCCR2Smoothing[ID][ch],  DPP::PHA::RCCR2SmoothingFactor, ch);

    uint32_t dpp = digi[ID]->GetSettingFromMemory(DPP::DPPAlgorithmControl, ch);
  
    chkDisableSelfTrigger[ID][ch]->setChecked( Digitizer::ExtractBits(dpp, DPP::Bit_DPPAlgorithmControl_PHA::DisableSelfTrigger) );
    chkEnableRollOver[ID][ch]->setChecked( Digitizer::ExtractBits(dpp, DPP::Bit_DPPAlgorithmControl_PHA::EnableRollOverFlag) );
    chkEnablePileUp[ID][ch]->setChecked( Digitizer::ExtractBits(dpp, DPP::Bit_DPPAlgorithmControl_PHA::EnablePileUpFlag) );

    UpdateComboBoxBit(cbDecimateTrace[ID][ch], dpp, DPP::Bit_DPPAlgorithmControl_PHA::TraceDecimation);
    UpdateComboBoxBit(cbPolarity[ID][ch],      dpp, DPP::Bit_DPPAlgorithmControl_PHA::Polarity);
    UpdateComboBoxBit(cbPeakAvg[ID][ch],       dpp, DPP::Bit_DPPAlgorithmControl_PHA::PeakMean);
    UpdateComboBoxBit(cbBaseLineAvg[ID][ch],   dpp, DPP::Bit_DPPAlgorithmControl_PHA::BaselineAvg);
    UpdateComboBoxBit(cbDecimateGain[ID][ch],  dpp, DPP::Bit_DPPAlgorithmControl_PHA::TraceDeciGain);
    UpdateComboBoxBit(cbTrigMode[ID][ch],      dpp, DPP::Bit_DPPAlgorithmControl_PHA::TriggerMode);

    uint32_t dpp2 = digi[ID]->GetSettingFromMemory(DPP::PHA::DPPAlgorithmControl2_G, ch);
 
    chkActiveBaseline[ID][ch]->setChecked( Digitizer::ExtractBits(dpp2, DPP::PHA::Bit_DPPAlgorithmControl2::ActivebaselineCalulation));
    chkBaselineRestore[ID][ch]->setChecked( Digitizer::ExtractBits(dpp2, DPP::PHA::Bit_DPPAlgorithmControl2::EnableActiveBaselineRestoration));
    chkTagCorrelation[ID][ch]->setChecked( Digitizer::ExtractBits(dpp2, DPP::PHA::Bit_DPPAlgorithmControl2::TagCorrelatedEvents));

    UpdateComboBoxBit(cbLocalTriggerValid[ID][ch],  dpp2, DPP::PHA::Bit_DPPAlgorithmControl2::LocalTrigValidMode);
    UpdateComboBoxBit(cbLocalShapedTrigger[ID][ch], dpp2, DPP::PHA::Bit_DPPAlgorithmControl2::LocalShapeTriggerMode);
    UpdateComboBoxBit(cbVetoSource[ID][ch],    dpp2, DPP::PHA::Bit_DPPAlgorithmControl2::VetoSource);
    UpdateComboBoxBit(cbTrigCount[ID][ch],     dpp2, DPP::PHA::Bit_DPPAlgorithmControl2::TriggerCounterFlag);
    UpdateComboBoxBit(cbExtra2Option[ID][ch],  dpp2, DPP::PHA::Bit_DPPAlgorithmControl2::Extra2Option);
    
    uint32_t vetoBit = digi[ID]->GetSettingFromMemory(DPP::VetoWidth, ch);

    UpdateComboBoxBit(cbVetoStep[ID][ch], vetoBit, DPP::Bit_VetoWidth::VetoStep);
  }


  enableSignalSlot = true;

  SyncAllChannelsTab_PHA();

}


void DigiSettingsPanel::SyncAllChannelsTab_PSD(){

    SyncSpinBox(sbRecordLength);
    SyncSpinBox(sbPreTrigger);
    SyncSpinBox(sbThreshold);
    SyncSpinBox(sbDCOffset);
    SyncSpinBox(sbPSDCutThreshold);
    SyncSpinBox(sbChargeZeroSupZero);
    SyncSpinBox(sbShapedTrigWidth);
    SyncSpinBox(sbTriggerHoldOff);
    SyncSpinBox(sbTriggerLatency);
    SyncSpinBox(sbShortGate);
    SyncSpinBox(sbLongGate);
    SyncSpinBox(sbGateOffset);
    SyncSpinBox(sbFixedBaseline);
    SyncSpinBox(sbCFDDely);
    SyncSpinBox(sbNumEventAgg);
    SyncSpinBox(sbVetoWidth);
    SyncSpinBox(sbPURGAPThreshold);

    SyncCheckBox(chkBaseLineCal);
    SyncCheckBox(chkChargePedestal);
    SyncCheckBox(chkDiscardQLong);
    SyncCheckBox(chkCutBelow);
    SyncCheckBox(chkCutAbove);
    SyncCheckBox(chkRejOverRange);
    SyncCheckBox(chkDisableOppositePulse);
    SyncCheckBox(chkDisableSelfTrigger);
    SyncCheckBox(chkRejPileUp);
    SyncCheckBox(chkPileUpInGate);
    SyncCheckBox(chkDisableTriggerHysteresis);
    SyncCheckBox(chkMarkSaturation);
    SyncCheckBox(chkResetTimestampByTRGIN);
    SyncCheckBox(chkTestPule);

    SyncComboBox(cbDynamicRange);
    SyncComboBox(cbPolarity);
    SyncComboBox(cbChargeSensitivity);
    SyncComboBox(cbBaseLineAvg);
    SyncComboBox(cbTrigMode);
    SyncComboBox(cbLocalTriggerValid);
    SyncComboBox(cbAdditionLocalTrigValid);
    SyncComboBox(cbDiscriMode);
    SyncComboBox(cbLocalShapedTrigger);
    SyncComboBox(cbTrigCount);
    SyncComboBox(cbTriggerOpt);
    SyncComboBox(cbSmoothedChargeIntegration);
    SyncComboBox(cbCFDFraction);
    SyncComboBox(cbCFDInterpolation);
    SyncComboBox(cbTestPulseRate);
    SyncComboBox(cbExtra2Option);
    SyncComboBox(cbVetoSource);
    SyncComboBox(cbVetoMode);
    SyncComboBox(cbVetoStep);


}
void DigiSettingsPanel::UpdatePSDSetting(){

  enableSignalSlot = false;

  printf("------ %s \n", __func__);

  for(int ch = 0; ch < digi[ID]->GetNChannels(); ch ++){

    UpdateSpinBox(sbRecordLength[ID][ch],      DPP::RecordLength_G, ch);
    UpdateSpinBox(sbPreTrigger[ID][ch],        DPP::PreTrigger, ch);
    UpdateSpinBox(sbThreshold[ID][ch],         DPP::PSD::TriggerThreshold, ch);
    UpdateSpinBox(sbDCOffset[ID][ch],          DPP::ChannelDCOffset, ch);
    UpdateSpinBox(sbChargeZeroSupZero[ID][ch], DPP::PSD::ChargeZeroSuppressionThreshold, ch);
    UpdateSpinBox(sbPSDCutThreshold[ID][ch],   DPP::PSD::ThresholdForPSDCut, ch);
    UpdateSpinBox(sbTriggerHoldOff[ID][ch],    DPP::PSD::TriggerHoldOffWidth, ch);
    UpdateSpinBox(sbShapedTrigWidth[ID][ch],   DPP::PSD::ShapedTriggerWidth, ch);
    UpdateSpinBox(sbTriggerLatency[ID][ch],    DPP::PSD::TriggerLatency, ch);
    UpdateSpinBox(sbShortGate[ID][ch],         DPP::PSD::ShortGateWidth, ch);
    UpdateSpinBox(sbLongGate[ID][ch],          DPP::PSD::LongGateWidth, ch);
    UpdateSpinBox(sbGateOffset[ID][ch],        DPP::PSD::GateOffset, ch);
    UpdateSpinBox(sbFixedBaseline[ID][ch],     DPP::PSD::FixedBaseline, ch);
    UpdateSpinBox(sbPURGAPThreshold[ID][ch],   DPP::PSD::PurGapThreshold, ch);
    UpdateSpinBox(sbNumEventAgg[ID][ch],       DPP::NumberEventsPerAggregate_G, ch);
    UpdateComboBox(cbDynamicRange[ID][ch],     DPP::InputDynamicRange, ch);


    uint32_t dpp = digi[ID]->GetSettingFromMemory(DPP::DPPAlgorithmControl, ch);

    UpdateComboBoxBit(cbPolarity[ID][ch],           dpp, DPP::Bit_DPPAlgorithmControl_PSD::Polarity);
    UpdateComboBoxBit(cbChargeSensitivity[ID][ch],  dpp, DPP::Bit_DPPAlgorithmControl_PSD::ChargeSensitivity);
    UpdateComboBoxBit(cbBaseLineAvg[ID][ch],        dpp, DPP::Bit_DPPAlgorithmControl_PSD::BaselineAvg);
    UpdateComboBoxBit(cbTrigMode[ID][ch],           dpp, DPP::Bit_DPPAlgorithmControl_PSD::TriggerMode);
    UpdateComboBoxBit(cbDiscriMode[ID][ch],         dpp, DPP::Bit_DPPAlgorithmControl_PSD::DiscriminationMode);
    UpdateComboBoxBit(cbTriggerOpt[ID][ch],         dpp, DPP::Bit_DPPAlgorithmControl_PSD::TriggerCountOpt);
    UpdateComboBoxBit(cbTestPulseRate[ID][ch],      dpp, DPP::Bit_DPPAlgorithmControl_PSD::TestPulseRate);

    chkChargePedestal[ID][ch]->setChecked(  Digitizer::ExtractBits(dpp, DPP::Bit_DPPAlgorithmControl_PSD::ChargePedestal));
    chkBaseLineCal[ID][ch]->setChecked(     Digitizer::ExtractBits(dpp, DPP::Bit_DPPAlgorithmControl_PSD::BaselineCal));
    chkDiscardQLong[ID][ch]->setChecked(    Digitizer::ExtractBits(dpp, DPP::Bit_DPPAlgorithmControl_PSD::DiscardQLongSmallerQThreshold));
    chkCutBelow[ID][ch]->setChecked(        Digitizer::ExtractBits(dpp, DPP::Bit_DPPAlgorithmControl_PSD::EnablePSDCutBelow));
    chkCutAbove[ID][ch]->setChecked(        Digitizer::ExtractBits(dpp, DPP::Bit_DPPAlgorithmControl_PSD::EnablePSDCutAbove));
    chkRejOverRange[ID][ch]->setChecked(    Digitizer::ExtractBits(dpp, DPP::Bit_DPPAlgorithmControl_PSD::RejectOverRange));
    chkTestPule[ID][ch]->setChecked(        Digitizer::ExtractBits(dpp, DPP::Bit_DPPAlgorithmControl_PSD::InternalTestPulse));

    uint32_t dpp2 = digi[ID]->GetSettingFromMemory(DPP::PSD::DPPAlgorithmControl2_G, ch);

    chkDisableOppositePulse[ID][ch]->setChecked(     Digitizer::ExtractBits(dpp2, DPP::Bit_DPPAlgorithmControl_PSD::DisableOppositePolarityInhibitZeroCrossingOnCFD));
    chkDisableSelfTrigger[ID][ch]->setChecked(       Digitizer::ExtractBits(dpp2, DPP::Bit_DPPAlgorithmControl_PSD::DisableSelfTrigger));
    chkRejPileUp[ID][ch]->setChecked(                Digitizer::ExtractBits(dpp2, DPP::Bit_DPPAlgorithmControl_PSD::RejectPileup));
    chkPileUpInGate[ID][ch]->setChecked(             Digitizer::ExtractBits(dpp2, DPP::Bit_DPPAlgorithmControl_PSD::PileupWithinGate));
    chkDisableTriggerHysteresis[ID][ch]->setChecked( Digitizer::ExtractBits(dpp2, DPP::Bit_DPPAlgorithmControl_PSD::DisableTriggerHysteresis));
    chkMarkSaturation[ID][ch]->setChecked(           Digitizer::ExtractBits(dpp2, DPP::PSD::Bit_DPPAlgorithmControl2::MarkSaturation));
    chkResetTimestampByTRGIN[ID][ch]->setChecked(    Digitizer::ExtractBits(dpp2, DPP::PSD::Bit_DPPAlgorithmControl2::ResetTimestampByTRGIN));


    UpdateComboBoxBit(cbLocalTriggerValid[ID][ch],          dpp2, DPP::PSD::Bit_DPPAlgorithmControl2::LocalTrigValidMode);
    UpdateComboBoxBit(cbAdditionLocalTrigValid[ID][ch],     dpp2, DPP::PSD::Bit_DPPAlgorithmControl2::AdditionLocalTrigValid);
    UpdateComboBoxBit(cbLocalShapedTrigger[ID][ch],         dpp2, DPP::PSD::Bit_DPPAlgorithmControl2::LocalShapeTriggerMode);
    UpdateComboBoxBit(cbTrigCount[ID][ch],                  dpp2, DPP::PSD::Bit_DPPAlgorithmControl2::TriggerCounterFlag);
    UpdateComboBoxBit(cbSmoothedChargeIntegration[ID][ch],  dpp2, DPP::PSD::Bit_DPPAlgorithmControl2::SmoothedChargeIntegration);
    UpdateComboBoxBit(cbVetoSource[ID][ch],                 dpp2, DPP::PSD::Bit_DPPAlgorithmControl2::VetoSource);
    UpdateComboBoxBit(cbVetoMode[ID][ch],                   dpp2, DPP::PSD::Bit_DPPAlgorithmControl2::VetoMode);
    UpdateComboBoxBit(cbExtra2Option[ID][ch],               dpp2, DPP::PSD::Bit_DPPAlgorithmControl2::ExtraWordOption);
   
    uint32_t CFDBit = digi[ID]->GetSettingFromMemory(DPP::PSD::CFDSetting, ch);

    UpdateSpinBox(sbCFDDely[ID][ch],           DPP::PSD::CFDSetting, ch);
    UpdateComboBoxBit(cbCFDFraction[ID][ch],      CFDBit, DPP::PSD::Bit_CFDSetting::CFDFraction);
    UpdateComboBoxBit(cbCFDInterpolation[ID][ch], CFDBit, DPP::PSD::Bit_CFDSetting::Interpolation);

    uint32_t vetoBit = digi[ID]->GetSettingFromMemory(DPP::VetoWidth, ch);

    UpdateSpinBox(sbVetoWidth[ID][ch],         DPP::VetoWidth, ch);
    UpdateComboBoxBit(cbVetoStep[ID][ch], vetoBit, DPP::Bit_VetoWidth::VetoStep);
  }
  enableSignalSlot = true;

  SyncAllChannelsTab_PSD();

}

//*================================================================
//*================================================================

void DigiSettingsPanel::ReadSettingsFromBoard(){

  digi[ID]->ReadAllSettingsFromBoard(true);

  UpdatePanelFromMemory();

}

void DigiSettingsPanel::SaveSetting(int opt){
  
  QDir dir(rawDataPath);
  if( !dir.exists() ) dir.mkpath(".");

  QString filePath = QFileDialog::getSaveFileName(this, "Save Settings File", rawDataPath, opt == 0 ? "Binary (*.bin)" : "Text file (*.txt)");

  if (!filePath.isEmpty()) {

    QFileInfo  fileInfo(filePath);
    QString ext = fileInfo.suffix();
    if( opt == 0 ){
      if( ext == "") filePath += ".bin";
      digi[ID]->SaveAllSettingsAsBin(filePath.toStdString().c_str());
      leSaveFilePath[ID]->setText(filePath + " | dynamic update");
    }
    if( opt == 1 ){
      if( ext == "") filePath += ".txt";
      digi[ID]->SaveAllSettingsAsText(filePath.toStdString().c_str());
      leSaveFilePath[ID]->setText(filePath + " | not loadable!!");
    }

    SendLogMsg("Saved setting file <b>" +  filePath + "</b>.");

  }

}

void DigiSettingsPanel::LoadSetting(){
  QFileDialog fileDialog(this);
  fileDialog.setDirectory(rawDataPath);
  fileDialog.setFileMode(QFileDialog::ExistingFile);
  fileDialog.setNameFilter("Binary (*.bin);;");
  int result = fileDialog.exec();

  if( ! (result == QDialog::Accepted) ) return;

  if( fileDialog.selectedFiles().size() == 0 ) return; // when no file selected.

  QString fileName = fileDialog.selectedFiles().at(0);

  if( digi[ID]->LoadSettingBinaryToMemory(fileName.toStdString().c_str()) == 0 ){
    SendLogMsg("Loaded settings file " + fileName + " for Digi-" + QString::number(digi[ID]->GetSerialNumber()));
    leSaveFilePath[ID]->setText(fileName + " | dynamic update");
    digi[ID]->ProgramSettingsToBoard();
    UpdatePanelFromMemory();

  }else{
    SendLogMsg("Fail to Loaded settings file " + fileName + " for Digi-" + QString::number(digi[ID]->GetSerialNumber()));
  }
}

