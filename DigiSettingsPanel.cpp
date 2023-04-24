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

    QScrollArea * scrollArea = new QScrollArea(this); 
    scrollArea->setWidgetResizable(true);
    scrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    tabWidget->addTab(scrollArea, "Digi-" + QString::number(digi[iDigi]->GetSerialNumber()));

    QWidget * tab = new QWidget(tabWidget);
    scrollArea->setWidget(tab);

    QHBoxLayout * tabLayout_H  = new QHBoxLayout(tab); //tab->setLayout(tabLayout_H);

    QVBoxLayout * tabLayout_V1 = new QVBoxLayout(tab); tabLayout_H->addLayout(tabLayout_V1);
    QVBoxLayout * tabLayout_V2 = new QVBoxLayout(tab); tabLayout_H->addLayout(tabLayout_V2);

    {//^====================== Group of Digitizer Info
      QGroupBox * infoBox = new QGroupBox("Board Info", tab);
      //infoBox->setSizePolicy(sizePolicy);

      QGridLayout * infoLayout = new QGridLayout(infoBox);
      tabLayout_V1->addWidget(infoBox);

      SetUpInfo(    "Model ", digi[iDigi]->GetModelName(), infoLayout, 0, 0);
      SetUpInfo( "DPP Type ", digi[iDigi]->GetDPPString(), infoLayout, 0, 2);
      SetUpInfo("Link Type ", digi[iDigi]->GetLinkType() == CAEN_DGTZ_USB ? "USB" : "Optical Link" , infoLayout, 0, 4);

      SetUpInfo(  "S/N No. ", std::to_string(digi[iDigi]->GetSerialNumber()), infoLayout, 1, 0);
      SetUpInfo(  "No. Ch. ", std::to_string(digi[iDigi]->GetNChannels()), infoLayout, 1, 2);
      SetUpInfo("Sampling Rate ", std::to_string(digi[iDigi]->GetCh2ns()), infoLayout, 1, 4);

      SetUpInfo("ADC bit ", std::to_string(digi[iDigi]->GetADCBits()), infoLayout, 2, 0);
      SetUpInfo("ROC version ", digi[iDigi]->GetROCVersion(), infoLayout, 2, 2);
      SetUpInfo("AMC version ", digi[iDigi]->GetAMCVersion(), infoLayout, 2, 4);
    }

    {//^======================= Board status
      QGroupBox * bardStatusBox = new QGroupBox("Board Status", tab);
      QGridLayout * statusLayout = new QGridLayout(bardStatusBox);
      tabLayout_V1->addWidget(bardStatusBox);
    }

    {//^======================= Board Settings
      QGroupBox * boardSettingBox = new QGroupBox("Board Settings", tab);
      QGridLayout * settingLayout = new QGridLayout(boardSettingBox);
      tabLayout_V1->addWidget(boardSettingBox);

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

}

DigiSettingsPanel::~DigiSettingsPanel(){
  printf("%s \n", __func__);

}

//*================================================================
//*================================================================
void DigiSettingsPanel::SetUpInfo(QString name, std::string value, QGridLayout *gLayout, int row, int col){
  QLabel * lab = new QLabel(name, this);
  lab->setAlignment(Qt::AlignRight | Qt::AlignCenter);
  QLineEdit * leInfo = new QLineEdit(this);
  leInfo->setReadOnly(true);
  leInfo->setText(QString::fromStdString(value));
  gLayout->addWidget(lab, row, col);
  gLayout->addWidget(leInfo, row, col + 1);
}
