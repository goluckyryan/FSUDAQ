#include "DigiSettingsPanel.h"

#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>

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

    }

    {//^======================= Board status
      QGroupBox * boardStatusBox = new QGroupBox("Board Status", tab);
      tabLayout_V1->addWidget(boardStatusBox);
      
      //QGridLayout * statusLayout = new QGridLayout(bardStatusBox);

    }

    {//^======================= Buttons

      QGroupBox * buttonsBox = new QGroupBox("Buttons", tab);
      tabLayout_V1->addWidget(buttonsBox);
      QGridLayout * buttonLayout = new QGridLayout(buttonsBox);

      bnRefreshSetting = new QPushButton("Refresh Settings", this);
      buttonLayout->addWidget(bnRefreshSetting, 0, 0);
      connect(bnRefreshSetting, &QPushButton::clicked, this, &DigiSettingsPanel::ReadSettingsFromBoard);

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
      CleanUpGroupBox(infoBox[ID]);
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

void DigiSettingsPanel::SetUpComboBox(RComboBox * &cb, QString label, QGridLayout *gLayout, int row, int col, std::vector<std::pair<QString, int>> items){

  QLabel * lab = new QLabel(label, this);
  lab->setAlignment(Qt::AlignRight | Qt::AlignCenter);
  gLayout->addWidget(lab, row, col);
  cb = new RComboBox(this);
  gLayout->addWidget(cb, row, col + 1);

  for(int i = 0; i < (int) items.size(); i++){
    cb->addItem(items[i].first, items[i].second);
  }

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

  SetUpCheckBox(chkAutoDataFlush[ID],   "Auto Data Flush", settingLayout[ID], 0, 0, Register::DPP::BoardConfiguration, Register::DPP::Bit_BoardConfig::EnableAutoDataFlush);
  SetUpCheckBox(chkDecimateTrace[ID],    "Decimate Trace", settingLayout[ID], 1, 0, Register::DPP::BoardConfiguration, Register::DPP::Bit_BoardConfig::DecimateTrace);
  SetUpCheckBox(chkTrigPropagation[ID], "Trig. Propagate", settingLayout[ID], 2, 0, Register::DPP::BoardConfiguration, Register::DPP::Bit_BoardConfig::TrigPropagation);
  SetUpCheckBox(chkDualTrace[ID],            "Dual Trace", settingLayout[ID], 3, 0, Register::DPP::BoardConfiguration, Register::DPP::Bit_BoardConfig::DualTrace);
  SetUpCheckBox(chkTraceRecording[ID],     "Record Trace", settingLayout[ID], 4, 0, Register::DPP::BoardConfiguration, Register::DPP::Bit_BoardConfig::RecordTrace);
  SetUpCheckBox(chkEnableExtra2[ID],      "Enable Extra2", settingLayout[ID], 5, 0, Register::DPP::BoardConfiguration, Register::DPP::Bit_BoardConfig::EnableExtra2);

  SetUpComboBox(cbAnaProbe1[ID], "Ana. Probe 1 ", settingLayout[ID], 0, 1, {{"Input", 0},
                                                                    {"RC-CR", 1},
                                                                    {"RC-CR2", 2},
                                                                    {"Trapezoid", 3}});
  SetUpComboBox(cbAnaProbe2[ID], "Ana. Probe 2 ", settingLayout[ID], 1, 1, {{"Input", 0},
                                                                    {"Threshold", 1},
                                                                    {"Trap. - Baseline", 2},
                                                                    {"Trap. Baseline", 3}});
  SetUpComboBox(cbDigiProbe1[ID], "Digi. Probe 1 ", settingLayout[ID], 2, 1, {{"Peaking", 0},
                                                                      {"Armed", 1},
                                                                      {"Peak Run", 2},
                                                                      {"Pile Up", 3},
                                                                      {"peaking", 4},
                                                                      {"TRG Valid. Win", 5},
                                                                      {"Baseline Freeze", 6},
                                                                      {"TRG Holdoff", 7},
                                                                      {"TRG Valid.", 8},
                                                                      {"ACQ Busy", 9},
                                                                      {"Zero Cross", 10},
                                                                      {"Ext. TRG", 11},
                                                                      {"Budy", 12}});

  SetUpComboBox(cbDigiProbe2[ID], "Digi. Probe 2 ", settingLayout[ID], 3, 1, {{"trigger", 0}});
  cbDigiProbe2[ID]->setEnabled(false);


  SetUpComboBox(cbAggOrg[ID], "Aggregate Organization ", settingLayout[ID], 6, 0, {{"Not used", 0},
                                                                            {"4", 2},
                                                                            {"8", 3},
                                                                            {"16", 4},
                                                                            {"32", 5},
                                                                            {"64", 6},
                                                                            {"128", 7},
                                                                            {"256", 8},
                                                                            {"512", 9},
                                                                            {"1024", 10}});
}


void DigiSettingsPanel::SetUpPSDBoard(){

}

void DigiSettingsPanel::UpdatePanelFromMemory(){

  printf("============== %s \n", __func__);

  SetUpInfo(    "Model ", digi[ID]->GetModelName(), infoLayout[ID], 0, 0);
  SetUpInfo( "DPP Type ", digi[ID]->GetDPPString(), infoLayout[ID], 0, 2);
  SetUpInfo("Link Type ", digi[ID]->GetLinkType() == CAEN_DGTZ_USB ? "USB" : "Optical Link" , infoLayout[ID], 0, 4);

  SetUpInfo(  "S/N No. ", std::to_string(digi[ID]->GetSerialNumber()), infoLayout[ID], 1, 0);
  SetUpInfo(  "No. Ch. ", std::to_string(digi[ID]->GetNChannels()), infoLayout[ID], 1, 2);
  SetUpInfo("Sampling Rate ", std::to_string(digi[ID]->GetCh2ns()), infoLayout[ID], 1, 4);

  SetUpInfo("ADC bit ", std::to_string(digi[ID]->GetADCBits()), infoLayout[ID], 2, 0);
  SetUpInfo("ROC version ", digi[ID]->GetROCVersion(), infoLayout[ID], 2, 2);
  SetUpInfo("AMC version ", digi[ID]->GetAMCVersion(), infoLayout[ID], 2, 4);

  enableSignalSlot = false;

  uint32_t BdCfg = digi[ID]->GetSettingFromMemory(Register::DPP::BoardConfiguration);

  chkAutoDataFlush[ID]->setChecked(   Digitizer::ExtractBits(BdCfg, Register::DPP::Bit_BoardConfig::EnableAutoDataFlush) );
  chkDecimateTrace[ID]->setChecked(   Digitizer::ExtractBits(BdCfg, Register::DPP::Bit_BoardConfig::DecimateTrace) );
  chkTrigPropagation[ID]->setChecked( Digitizer::ExtractBits(BdCfg, Register::DPP::Bit_BoardConfig::TrigPropagation) );
  chkDualTrace[ID]->setChecked(       Digitizer::ExtractBits(BdCfg, Register::DPP::Bit_BoardConfig::DualTrace) );
  chkTraceRecording[ID]->setChecked(  Digitizer::ExtractBits(BdCfg, Register::DPP::Bit_BoardConfig::TrigPropagation) );
  chkEnableExtra2[ID]->setChecked(    Digitizer::ExtractBits(BdCfg, Register::DPP::Bit_BoardConfig::EnableExtra2) );

  //connect(chkAutoDataFlush, &QCheckBox::stateChanged, this, &DigiSettingsPanel::SetAutoDataFlush);

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


  enableSignalSlot = false;

}

void DigiSettingsPanel::ReadSettingsFromBoard(){

  digi[ID]->ReadAllSettingsFromBoard();

  UpdatePanelFromMemory();

}


//*================================================================
//*================================================================
void DigiSettingsPanel::SetAutoDataFlush(){

  if( !enableSignalSlot ) return;
  digi[ID]->SetBits(Register::DPP::BoardConfiguration, Register::DPP::Bit_BoardConfig::EnableAutoDataFlush, chkAutoDataFlush[ID]->checkState() ? 1 : 0, -1);

}

