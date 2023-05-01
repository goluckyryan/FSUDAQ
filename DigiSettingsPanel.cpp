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
  setGeometry(0, 0, 1400, 800);  

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
      tabLayout_V1->addWidget(infoBox[iDigi]);
      
      infoLayout[iDigi] = new QGridLayout(infoBox[iDigi]);
      infoLayout[iDigi]->setSpacing(2);

      SetUpInfo(    "Model ", digi[ID]->GetModelName(), infoLayout[ID], 0, 0);
      SetUpInfo( "DPP Type ", digi[ID]->GetDPPString(), infoLayout[ID], 0, 2);
      SetUpInfo("Link Type ", digi[ID]->GetLinkType() == CAEN_DGTZ_USB ? "USB" : "Optical Link" , infoLayout[ID], 0, 4);

      SetUpInfo(  "S/N No. ", std::to_string(digi[ID]->GetSerialNumber()), infoLayout[ID], 1, 0);
      SetUpInfo(  "No. Ch. ", std::to_string(digi[ID]->GetNChannels()), infoLayout[ID], 1, 2);
      SetUpInfo("Sampling Rate ", std::to_string(digi[ID]->GetCh2ns()), infoLayout[ID], 1, 4);

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
      
      leSaveFilePath = new QLineEdit(this);
      leSaveFilePath->setReadOnly(true);
      buttonLayout->addWidget(leSaveFilePath, rowID, 1, 1, 3);

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


      rowID ++; //---------------------------
      bnSendSoftwareTriggerSignal = new QPushButton("Send SW Trigger Signal", this);
      buttonLayout->addWidget(bnSendSoftwareTriggerSignal, rowID, 0);
      connect(bnSendSoftwareTriggerSignal, &QPushButton::clicked, this, [=](){ digi[ID]->WriteRegister(DPP::SoftwareTrigger_W, 1);});

      bnSendSoftwareClockSyncSignal = new QPushButton("Send SW Clock-Sync Signal", this);
      buttonLayout->addWidget(bnSendSoftwareClockSyncSignal, rowID, 1);
      connect(bnSendSoftwareClockSyncSignal, &QPushButton::clicked, this, [=](){ digi[ID]->WriteRegister(DPP::SoftwareClockSync_W, 1);});

      bnSaveSettings = new QPushButton("Save Settings", this);
      buttonLayout->addWidget(bnSaveSettings, rowID, 2);
      bnLoadSettings = new QPushButton("Load Settings", this);
      buttonLayout->addWidget(bnLoadSettings, rowID, 3);

    }

    {//^======================= Board Settings
      boardSettingBox[iDigi] = new QGroupBox("Board Settings", tab);
      tabLayout_V1->addWidget(boardSettingBox[iDigi]);

      settingLayout[iDigi] = new QGridLayout(boardSettingBox[iDigi]);
      settingLayout[iDigi]->setSpacing(2);

      if( digi[ID]->GetDPPType() == V1730_DPP_PHA_CODE ) SetUpPHABoard();

      SetUpGlobalTriggerMaskAndFrontPanelMask();


    }

    {//^======================= Channel Settings
      
      QTabWidget * chTab = new QTabWidget(tab);
      tabLayout_H->addWidget(chTab);

      chAllSetting = new QWidget(this);
      //chAllSetting->setStyleSheet("background-color: #ECECEC;");
      chTab->addTab(chAllSetting, "Channel Settings");

      QWidget * chStatus = new QWidget(this);
      //chStatus->setStyleSheet("background-color: #ECECEC;");
      chTab->addTab(chStatus, "Status");

      QWidget * chInput = new QWidget(this);
      chTab->addTab(chInput, "Input");

      QWidget * chTrap = new QWidget(this);
      chTab->addTab(chTrap, "Trapezoid");

      QWidget * chOthers = new QWidget(this);
      chTab->addTab(chOthers, "Others");

      QWidget * chTrigger = new QWidget(this);
      chTab->addTab(chTrigger, "Trigger");

      SetUpPHAChannel();

    }

  }

  //TODO ----- Copy settings tab


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

void DigiSettingsPanel::SetUpCheckBox(QCheckBox * &chkBox, QString label, QGridLayout *gLayout, int row, int col, Reg para, std::pair<unsigned short, unsigned short> bit){

  chkBox = new QCheckBox(label, this);
  gLayout->addWidget(chkBox, row, col);

  connect(chkBox, &QCheckBox::stateChanged, this, [=](int state){
    if( !enableSignalSlot ) return;
    digi[ID]->SetBits(para, bit, state ? 1 : 0, -1);
  });

}

void DigiSettingsPanel::SetUpComboBoxBit(RComboBox * &cb, QString label, QGridLayout *gLayout, int row, int col, std::vector<std::pair<std::string, unsigned int>> items, Reg para, std::pair<unsigned short, unsigned short> bit, int colspan){

  QLabel * lab = new QLabel(label, this);
  lab->setAlignment(Qt::AlignRight | Qt::AlignCenter);
  gLayout->addWidget(lab, row, col);
  cb = new RComboBox(this);
  gLayout->addWidget(cb, row, col + 1, 1, colspan);

  for(int i = 0; i < (int) items.size(); i++){
    cb->addItem(QString::fromStdString(items[i].first), items[i].second);
  }

  connect(cb, &RComboBox::currentIndexChanged, this, [=](){
    if( !enableSignalSlot ) return;
    digi[ID]->SetBits(para, bit, cb->currentData().toUInt(), -1);
  });

}
void DigiSettingsPanel::SetUpComboBox(RComboBox * &cb, QString label, QGridLayout *gLayout, int row, int col, Reg para){

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

void DigiSettingsPanel::SetUpSpinBox(RSpinBox * &sb, QString label, QGridLayout *gLayout, int row, int col, Reg para){
  QLabel * lab = new QLabel(label, this);
  lab->setAlignment(Qt::AlignRight | Qt::AlignCenter);
  gLayout->addWidget(lab, row, col);

  sb = new RSpinBox(this);
  gLayout->addWidget(sb, row, col + 1);

  sb->setMinimum(0);
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

    if( sb->decimals() == 0 && sb->singleStep() != 1) {
      double step = sb->singleStep();
      double value = sb->value();
      sb->setValue( (std::round(value/step)*step));
    }

    sb->setStyleSheet("");
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

void DigiSettingsPanel::SetUpGlobalTriggerMaskAndFrontPanelMask(){

  QWidget * triggerBox = new QWidget(this);
  int row = settingLayout[ID]->rowCount();
  settingLayout[ID]->addWidget(triggerBox, row +1, 0, 3, 4);
  triggerLayout[ID] = new QGridLayout(triggerBox);
  triggerLayout[ID]->setAlignment(Qt::AlignLeft);
  triggerLayout[ID]->setSpacing(2);

  for( int i = 0; i < MaxNChannels/2; i++){

    if( i % 2 == 0 ){
      QLabel * chIDLabel = new QLabel(QString::number(2*i) + "-" + QString::number(2*i + 1), this);
      chIDLabel->setAlignment(Qt::AlignLeft | Qt::AlignBottom);
      triggerLayout[ID]->addWidget(chIDLabel, 0, 1 + i, 1, 2);
    }

    bnGlobalTriggerMask[ID][i] = new QPushButton(this);
    bnGlobalTriggerMask[ID][i]->setFixedSize(QSize(20,20));
    bnGlobalTriggerMask[ID][i]->setToolTipDuration(-1);
    triggerLayout[ID]->addWidget(bnGlobalTriggerMask[ID][i], 1, 1 + i );
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
    triggerLayout[ID]->addWidget(bnTRGOUTMask[ID][i], 2, 1 + i );
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
  triggerLayout[ID]->addWidget(lbGlobalTrg, 1, 0);

  //*============================================
  QLabel * lbMajorCoinWin = new QLabel("Coin. Win [ns] : ", this);
  triggerLayout[ID]->addWidget(lbMajorCoinWin, 1, 9);

  sbGlbMajCoinWin[ID] = new RSpinBox(this);
  sbGlbMajCoinWin[ID]->setMinimum(0);
  sbGlbMajCoinWin[ID]->setMaximum(0xF * 4 * digi[ID]->GetCh2ns() );
  sbGlbMajCoinWin[ID]->setSingleStep(4 * digi[ID]->GetCh2ns());
  triggerLayout[ID]->addWidget(sbGlbMajCoinWin[ID], 1, 10);
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
  triggerLayout[ID]->addWidget(lbMajorLvl, 0, 11);

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
  triggerLayout[ID]->addWidget(lbOtherTrigger, 0, 12);

  //*============================================
  cbGlbUseOtherTriggers[ID] = new RComboBox(this);
  cbGlbUseOtherTriggers[ID]->addItem("None", 0);
  cbGlbUseOtherTriggers[ID]->addItem("TRG-IN", 1);
  cbGlbUseOtherTriggers[ID]->addItem("SW", 2);
  cbGlbUseOtherTriggers[ID]->addItem("TRG-IN OR SW", 3);
  triggerLayout[ID]->addWidget(cbGlbUseOtherTriggers[ID], 1, 12);
  connect(cbGlbUseOtherTriggers[ID], &RComboBox::currentIndexChanged, this, [=](int index){
    if( !enableSignalSlot ) return;
    digi[ID]->SetBits(DPP::GlobalTriggerMask, {2, 30}, index, -1);
  });


  QLabel * lbTrgOut = new QLabel("TRG-OUT Mask : ", this);
  lbTrgOut->setAlignment(Qt::AlignRight | Qt::AlignCenter);
  triggerLayout[ID]->addWidget(lbTrgOut, 2, 0);

  QLabel * lbTrgOutLogic = new QLabel("Logic : ", this);
  lbTrgOutLogic->setAlignment(Qt::AlignRight | Qt::AlignCenter);
  triggerLayout[ID]->addWidget(lbTrgOutLogic, 2, 9);

  //*============================================
  cbTRGOUTLogic[ID] = new RComboBox(this);
  cbTRGOUTLogic[ID]->addItem("OR", 0);
  cbTRGOUTLogic[ID]->addItem("AND", 1);
  cbTRGOUTLogic[ID]->addItem("Maj.", 2);
  triggerLayout[ID]->addWidget(cbTRGOUTLogic[ID], 2, 10);
  triggerLayout[ID]->addWidget(sbGlbMajLvl[ID], 1, 11);

  connect(cbTRGOUTLogic[ID], &RComboBox::currentIndexChanged, this, [=](int index){
    if( !enableSignalSlot ) return;
    digi[ID]->SetBits(DPP::FrontPanelTRGOUTEnableMask, DPP::Bit_TRGOUTMask::TRGOUTLogic, index, -1);
  });

  //*============================================
  sbTRGOUTMajLvl[ID] = new RSpinBox(this);
  sbTRGOUTMajLvl[ID]->setMinimum(0);
  sbTRGOUTMajLvl[ID]->setMaximum(7);
  sbTRGOUTMajLvl[ID]->setSingleStep(1);
  triggerLayout[ID]->addWidget(sbTRGOUTMajLvl[ID], 2, 11);

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
  triggerLayout[ID]->addWidget(cbTRGOUTUseOtherTriggers[ID], 2, 12);

  connect(cbTRGOUTUseOtherTriggers[ID], &RComboBox::currentIndexChanged, this, [=](int index){
    if( !enableSignalSlot ) return;
    digi[ID]->SetBits(DPP::FrontPanelTRGOUTEnableMask, {2, 30}, index, -1);
  });

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
         digi[ID]->SetBits(DPP::ChannelEnableMask, {1, i}, 1, i);
      }else{
         bnChEnableMask[ID][i]->setStyleSheet("");
         digi[ID]->SetBits(DPP::ChannelEnableMask, {1, i}, 0, i);
      }
    });
  }

  SetUpCheckBox(chkAutoDataFlush[ID],        "Auto Data Flush", settingLayout[ID], 1, 0, DPP::BoardConfiguration, DPP::Bit_BoardConfig::EnableAutoDataFlush);
  SetUpCheckBox(chkDecimateTrace[ID],         "Decimate Trace", settingLayout[ID], 2, 0, DPP::BoardConfiguration, DPP::Bit_BoardConfig::DecimateTrace);
  SetUpCheckBox(chkTrigPropagation[ID],      "Trig. Propagate", settingLayout[ID], 3, 0, DPP::BoardConfiguration, DPP::Bit_BoardConfig::TrigPropagation);
  SetUpCheckBox(chkEnableExternalTrigger[ID], "Enable TRG-IN ", settingLayout[ID], 4, 0, DPP::DisableExternalTrigger, {1, 0});
  
  SetUpCheckBox(chkDualTrace[ID],            "Dual Trace", settingLayout[ID], 1, 1, DPP::BoardConfiguration, DPP::Bit_BoardConfig::DualTrace);
  
  connect(chkDualTrace[ID], &QCheckBox::stateChanged, this, [=](int state){
    if( !enableSignalSlot) return;
    cbAnaProbe2[ID]->setEnabled(state);
    cbDigiProbe2[ID]->setEnabled(state);
  });


  SetUpCheckBox(chkTraceRecording[ID],     "Record Trace", settingLayout[ID], 2, 1, DPP::BoardConfiguration, DPP::Bit_BoardConfig::RecordTrace);
  SetUpCheckBox(chkEnableExtra2[ID],      "Enable Extra2", settingLayout[ID], 3, 1, DPP::BoardConfiguration, DPP::Bit_BoardConfig::EnableExtra2);

  SetUpComboBoxBit(cbAnaProbe1[ID],   "Ana. Probe 1 ", settingLayout[ID], 1, 2, DPP::Bit_BoardConfig::ListAnaProbe1_PHA, DPP::BoardConfiguration, DPP::Bit_BoardConfig::AnalogProbe1);
  SetUpComboBoxBit(cbAnaProbe2[ID],   "Ana. Probe 2 ", settingLayout[ID], 2, 2, DPP::Bit_BoardConfig::ListAnaProbe2_PHA, DPP::BoardConfiguration, DPP::Bit_BoardConfig::AnalogProbe2);
  SetUpComboBoxBit(cbDigiProbe1[ID], "Digi. Probe 1 ", settingLayout[ID], 3, 2, DPP::Bit_BoardConfig::ListDigiProbe1_PHA, DPP::BoardConfiguration, DPP::Bit_BoardConfig::DigiProbel1);
  SetUpComboBoxBit(cbDigiProbe2[ID], "Digi. Probe 2 ", settingLayout[ID], 4, 2, {{"trigger", 0}}, DPP::BoardConfiguration, DPP::Bit_BoardConfig::DigiProbel2);

  SetUpSpinBox(sbAggNum[ID],         "Agg. Num. / read", settingLayout[ID], 5, 0, DPP::MaxAggregatePerBlockTransfer);
  SetUpComboBox(cbAggOrg[ID], "Aggregate Organization ", settingLayout[ID], 6, 0, DPP::AggregateOrganization);

  SetUpComboBoxBit(cbStartStopMode[ID], "Start/Stop Mode ", settingLayout[ID], 7, 0, {{"SW controlled", 0},
                                                                                      {"S-IN/GPI controlled", 1},
                                                                                      {"1st Trigger", 2},
                                                                                      {"LVDS controlled", 3}},
                                                                                   DPP::AcquisitionControl, DPP::Bit_AcquistionControl::StartStopMode);

  SetUpComboBoxBit(cbAcqStartArm[ID], "Acq Start/Arm ", settingLayout[ID], 8, 0, {{"ACQ STOP", 0},
                                                                                   {"ACQ RUN", 1}},DPP::AcquisitionControl, DPP::Bit_AcquistionControl::ACQStartArm);

  SetUpComboBoxBit(cbPLLRefClock[ID], "PLL Ref. Clock ", settingLayout[ID], 5, 2, {{"Internal 50 MHz", 0},{"Ext. CLK-IN", 1}}, DPP::AcquisitionControl, DPP::Bit_AcquistionControl::ACQStartArm);


  SetUpSpinBox(sbRunDelay[ID], "Run Delay [ns] ", settingLayout[ID], 6, 2, DPP::RunStartStopDelay);

  SetUpComboBox(cbAnalogMonitorMode[ID], "Analog Monitor Mode ", settingLayout[ID], 7, 2, DPP::AnalogMonitorMode);

  SetUpSpinBox(sbBufferGain[ID], "Buffer Occup. Gain ", settingLayout[ID], 8, 2, DPP::BufferOccupancyGain);


  SetUpComboBoxBit(cbLEMOMode[ID], "LEMO Mode ", settingLayout[ID], 9, 0, DPP::Bit_FrontPanelIOControl::ListLEMOLevel, DPP::FrontPanelIOControl, DPP::Bit_FrontPanelIOControl::LEMOLevel);
  

  ///============================ Trig out mode
  QLabel * trgOutMode = new QLabel("TRI-OUT Mode ", this);
  trgOutMode->setAlignment(Qt::AlignRight | Qt::AlignCenter);
  settingLayout[ID]->addWidget(trgOutMode, 9, 2);
  cbTRGOUTMode[ID] = new RComboBox(this);
  settingLayout[ID]->addWidget(cbTRGOUTMode[ID], 9, 3);

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

}

void DigiSettingsPanel::SetUpPHAChannel(){

  QVBoxLayout * allSettingLayout = new QVBoxLayout(chAllSetting);
  allSettingLayout->setAlignment(Qt::AlignTop);

  {//^========================= input
    QGroupBox * inputBox = new QGroupBox("input Settings", this);
    allSettingLayout->addWidget(inputBox);

    QGridLayout  * inputLayout = new QGridLayout(inputBox);

    SetUpSpinBox(sbRecordLength[MaxNChannels], "Record Length [G][ns] : ", inputLayout, 0, 0, DPP::RecordLength_G);
    SetUpComboBox(cbDynamicRange[MaxNChannels],        "Dynamic Range : ", inputLayout, 0, 2, DPP::InputDynamicRange);
    SetUpSpinBox(sbPreTrigger[MaxNChannels],        "Pre-Trigger [ns] : ", inputLayout, 1, 0, DPP::PreTrigger);
    SetUpComboBox(cbRCCR2Smoothing[MaxNChannels],   "Smoothing factor : ", inputLayout, 1, 2, DPP::PHA::RCCR2SmoothingFactor);
    SetUpSpinBox(sbInputRiseTime[MaxNChannels],       "Rise Time [ns] : ", inputLayout, 2, 0, DPP::PHA::InputRiseTime);
    SetUpSpinBox(sbThreshold[MaxNChannels],          "Threshold [LSB] : ", inputLayout, 2, 2, DPP::PHA::TriggerThreshold);
    SetUpSpinBox(sbRiseTimeValidWin[MaxNChannels], "Rise Time Valid. Win. [ns] : ", inputLayout, 3, 0, DPP::PHA::RiseTimeValidationWindow);
    SetUpSpinBox(sbTriggerHoldOff[MaxNChannels],         "Tigger Hold-off [ns] : ", inputLayout, 3, 2, DPP::PHA::TriggerHoldOffWidth);
    SetUpSpinBox(sbShapedTrigWidth[MaxNChannels],     "Shaped Trig. Width [ns] : ", inputLayout, 4, 0, DPP::PHA::ShapedTriggerWidth);
    SetUpSpinBox(sbDCOffset[MaxNChannels],             "DC Offset [%] : ", inputLayout, 4, 2, DPP::ChannelDCOffset);
    SetUpComboBoxBit(cbPolarity[MaxNChannels],              "Polarity : ", inputLayout, 5, 0, DPP::Bit_DPPAlgorithmControl::ListPolarity, DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl::Polarity);

  }

  {//^===================== Trapezoid
    QGroupBox * trapBox = new QGroupBox("Trapezoid Settings", this);
    allSettingLayout->addWidget(trapBox);

    QGridLayout  * trapLayout = new QGridLayout(trapBox);

    SetUpSpinBox(sbTrapRiseTime[MaxNChannels],   "Rise Time [ns] : ", trapLayout, 0, 0, DPP::PHA::TrapezoidRiseTime);
    SetUpSpinBox(sbTrapFlatTop[MaxNChannels],     "Flat Top [ns] : ", trapLayout, 0, 2, DPP::PHA::TrapezoidFlatTop);
    SetUpSpinBox(sbDecay[MaxNChannels],              "Decay [ns] : ", trapLayout, 1, 0, DPP::PHA::DecayTime);
    SetUpSpinBox(sbTrapScaling[MaxNChannels],         "Rescaling : ", trapLayout, 1, 2, DPP::PHA::DPPAlgorithmControl2_G);
    SetUpSpinBox(sbPeaking[MaxNChannels],          "Peaking [ns] : ", trapLayout, 2, 0, DPP::PHA::PeakingTime);
    SetUpSpinBox(sbPeakingHoldOff[MaxNChannels], "Peak Hold-off [ns] : ", trapLayout, 2, 2, DPP::PHA::PeakHoldOff);
    SetUpComboBoxBit(cbPeakAvg[MaxNChannels],         "Peak Avg. : ", trapLayout, 3, 0, DPP::Bit_DPPAlgorithmControl::ListPeakMean, DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl::PeakMean);
    SetUpComboBoxBit(cBaseLineAvg[MaxNChannels],  "Baseline Avg. : ", trapLayout, 3, 2, DPP::Bit_DPPAlgorithmControl::ListBaselineAvg, DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl::BaselineAvg);
    SetUpCheckBox(chkActiveBaseline[MaxNChannels], "Active basline [G]", trapLayout, 4, 0, DPP::PHA::DPPAlgorithmControl2_G, DPP::PHA::Bit_DPPAlgorithmControl2::ActivebaselineCalulation);
    SetUpCheckBox(chkBaselineRestore[MaxNChannels], "Baseline Restorer [G]", trapLayout, 4, 1, DPP::PHA::DPPAlgorithmControl2_G, DPP::PHA::Bit_DPPAlgorithmControl2::EnableActiveBaselineRestoration);
    SetUpSpinBox(sbFineGain[MaxNChannels],            "Fine Gain : ", trapLayout, 4, 2, DPP::PHA::FineGain);

  }

  {//^====================== Others
    QGroupBox * otherBox = new QGroupBox("Others Settings", this);
    allSettingLayout->addWidget(otherBox);
    
    QGridLayout  * otherLayout = new QGridLayout(otherBox);
    
    SetUpCheckBox(chkDisableSelfTrigger[MaxNChannels],  "Disable Self Trigger ", otherLayout, 0, 0, DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl::DisableSelfTrigger);
    SetUpCheckBox(chkEnableRollOver[MaxNChannels],     "Enable Roll-Over Event", otherLayout, 0, 1, DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl::EnableRollOverFlag);
    SetUpCheckBox(chkEnablePileUp[MaxNChannels],          "Allow Pile-up Event", otherLayout, 1, 0, DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl::EnablePileUpFlag);
    SetUpCheckBox(chkTagCorrelation[MaxNChannels],  "Tag Correlated events [G]", otherLayout, 1, 1, DPP::PHA::DPPAlgorithmControl2_G, DPP::PHA::Bit_DPPAlgorithmControl2::TagCorrelatedEvents);
    SetUpComboBoxBit(cbDecimateTrace[MaxNChannels],         "Decimate Trace : ", otherLayout, 0, 2, DPP::Bit_DPPAlgorithmControl::ListTraceDecimation, DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl::TraceDecimation);
    SetUpComboBoxBit(cbDecimateGain[MaxNChannels],           "Decimate Gain : ", otherLayout, 1, 2, DPP::Bit_DPPAlgorithmControl::ListDecimationGain, DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl::TraceDeciGain);
    SetUpSpinBox(sbNumEventAgg[MaxNChannels],          "Events per Agg. [G] : ", otherLayout, 2, 0, DPP::NumberEventsPerAggregate_G);
    SetUpComboBoxBit(cbTriggerValid[MaxNChannels],  "Local Trig. Valid. [G] : ", otherLayout, 2, 2, DPP::PHA::Bit_DPPAlgorithmControl2::ListLocalTrigValidMode, DPP::PHA::DPPAlgorithmControl2_G, DPP::PHA::Bit_DPPAlgorithmControl2::LocalTrigValidMode);
    SetUpComboBoxBit(cbTrigCount[MaxNChannels],     "Trig. Counter Flag [G] : ", otherLayout, 3, 0, DPP::PHA::Bit_DPPAlgorithmControl2::ListTrigCounter, DPP::PHA::DPPAlgorithmControl2_G, DPP::PHA::Bit_DPPAlgorithmControl2::TriggerCounterFlag);
    SetUpComboBoxBit(cbTrigMode[MaxNChannels],                   "Trig Mode : ", otherLayout, 4, 0, DPP::Bit_DPPAlgorithmControl::ListTrigMode, DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl::TriggerMode, 3);
    SetUpComboBoxBit(cbShapedTrigger[MaxNChannels], "Local Shaped Trig. [G] : ", otherLayout, 5, 0, DPP::PHA::Bit_DPPAlgorithmControl2::ListLocalShapeTrigMode, DPP::PHA::DPPAlgorithmControl2_G, DPP::PHA::Bit_DPPAlgorithmControl2::LocalShapeTriggerMode, 3);
    SetUpComboBoxBit(cbExtra2Option[MaxNChannels],       "Extra2 Option [G] : ", otherLayout, 6, 0, DPP::PHA::Bit_DPPAlgorithmControl2::ListExtra2, DPP::PHA::DPPAlgorithmControl2_G, DPP::PHA::Bit_DPPAlgorithmControl2::Extra2Option, 3);
    SetUpComboBoxBit(cbVetoSource[MaxNChannels],           "Veto Source [G] : ", otherLayout, 7, 0, DPP::PHA::Bit_DPPAlgorithmControl2::ListVetoSource, DPP::PHA::DPPAlgorithmControl2_G, DPP::PHA::Bit_DPPAlgorithmControl2::VetoSource);
    SetUpSpinBox(sbVetoWidth[MaxNChannels],                     "Veto Width : ", otherLayout, 7, 2, DPP::VetoWidth);
    SetUpComboBoxBit(cbVetoStep[MaxNChannels],                   "Veto Step : ", otherLayout, 9, 0, DPP::Bit_VetoWidth::ListVetoStep, DPP::VetoWidth, DPP::Bit_VetoWidth::VetoStep, 3);

  }

}

void DigiSettingsPanel::SetUpPSDBoard(){

}

void DigiSettingsPanel::UpdatePanelFromMemory(){

  printf("============== %s \n", __func__);

  enableSignalSlot = false;

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

  uint32_t BdCfg = digi[ID]->GetSettingFromMemory(DPP::BoardConfiguration);

  chkAutoDataFlush[ID]->setChecked(   Digitizer::ExtractBits(BdCfg, DPP::Bit_BoardConfig::EnableAutoDataFlush) );
  chkDecimateTrace[ID]->setChecked(   Digitizer::ExtractBits(BdCfg, DPP::Bit_BoardConfig::DecimateTrace) );
  chkTrigPropagation[ID]->setChecked( Digitizer::ExtractBits(BdCfg, DPP::Bit_BoardConfig::TrigPropagation) );
  chkDualTrace[ID]->setChecked(       Digitizer::ExtractBits(BdCfg, DPP::Bit_BoardConfig::DualTrace) );
  chkTraceRecording[ID]->setChecked(  Digitizer::ExtractBits(BdCfg, DPP::Bit_BoardConfig::TrigPropagation) );
  chkEnableExtra2[ID]->setChecked(    Digitizer::ExtractBits(BdCfg, DPP::Bit_BoardConfig::EnableExtra2) );

  int temp = Digitizer::ExtractBits(BdCfg, DPP::Bit_BoardConfig::AnalogProbe1);
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
  temp = Digitizer::ExtractBits(BdCfg, DPP::Bit_BoardConfig::DigiProbel1);
  for(int i = 0; i < cbDigiProbe1[ID]->count(); i++){
    if( cbDigiProbe1[ID]->itemData(i).toInt() == temp) {
      cbDigiProbe1[ID]->setCurrentIndex(i);
      break;
    }
  }

  uint32_t chMask = digi[ID]->GetSettingFromMemory(DPP::ChannelEnableMask);
  for( int i = 0; i < MaxNChannels; i++){
    if( (chMask >> i ) & 0x1 ) {
      bnChEnableMask[ID][i]->setStyleSheet("background-color: green;");
    }else{
      bnChEnableMask[ID][i]->setStyleSheet("");
    }
  }

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

  uint32_t anaMonitor = digi[ID]->GetSettingFromMemory(DPP::AnalogMonitorMode);
  for( int i = 0 ; i < cbAnalogMonitorMode[ID]->count(); i++){
    if( cbAnalogMonitorMode[ID]->itemData(i).toUInt() == anaMonitor ){ 
      cbAnalogMonitorMode[ID]->setCurrentIndex(i);
      break;
    }
  }

  sbBufferGain[ID]->setValue(digi[ID]->GetSettingFromMemory(DPP::BufferOccupancyGain));


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

  uint32_t TRGOUTMask = digi[ID]->GetSettingFromMemory(DPP::FrontPanelTRGOUTEnableMask);
  for( int i = 0; i < MaxNChannels/2; i++){
    if( (TRGOUTMask >> i ) & 0x1 ){
      bnTRGOUTMask[ID][i]->setStyleSheet("background-color: green;");
    }else{
      bnTRGOUTMask[ID][i]->setStyleSheet("");
    }
  }

  cbTRGOUTLogic[ID]->setCurrentIndex(Digitizer::ExtractBits(TRGOUTMask, DPP::Bit_TRGOUTMask::TRGOUTLogic));
  sbTRGOUTMajLvl[ID]->setValue( Digitizer::ExtractBits(TRGOUTMask, DPP::Bit_TRGOUTMask::MajorLevel));
  cbTRGOUTUseOtherTriggers[ID]->setCurrentIndex(Digitizer::ExtractBits(TRGOUTMask, {2, 30}));

  enableSignalSlot = true;

}

void DigiSettingsPanel::UpdatePHASetting(){


}

//*================================================================
//*================================================================

void DigiSettingsPanel::ReadSettingsFromBoard(){

  digi[ID]->ReadAllSettingsFromBoard();

  UpdatePanelFromMemory();

}


