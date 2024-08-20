#ifndef NEUTRONGAMMA_H
#define NEUTRONGAMMA_H


/***********************************\
 * 
 *  This Analyzer does not use event builder, 
 *  this simply use the energy_short and energy_long for each data.
 * 
 *************************************/

#include <QMainWindow>
#include <QChart>
#include <QChartView>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QLineEdit>
#include <QGridLayout>
#include <QGroupBox>
#include <QVector>
#include <QRandomGenerator>

#include "Analyser.h"

//^====================================================
//^====================================================
class NeutronGamma : public Analyzer{
  Q_OBJECT

public:
  NeutronGamma(Digitizer ** digi, unsigned int nDigi, QString rawDataPath, QMainWindow * parent = nullptr): Analyzer(digi, nDigi, parent){
    this->digi = digi;
    this->nDigi = nDigi;
    this->settingPath = rawDataPath + "/NG_HistogramSettings.txt";

    SetUpdateTimeInSec(1.0);

    isSignalSlotActive = false;
    SetUpCanvas();

    ClearInternalDataCount();
  };

  ~NeutronGamma(){
    // for( unsigned int i = 0; i < nDigi; i++ ){
    //   for( int ch = 0; ch < digi[i]->GetNumInputCh(); ch++){
    //     delete hist2D[i][ch];
    //   }
    // } 
    delete hist2D; 
  }

  void ClearInternalDataCount();

  // void LoadSetting();
  // void SaveSetting();

public slots:
  void UpdateHistograms();

private:
  QVector<int> generateNonRepeatedCombination(int size);
  void SetUpCanvas();

  Digitizer ** digi;
  unsigned short nDigi;

  Histogram2D * hist2D;

  RComboBox * cbDigi;
  RComboBox * cbCh;

  QGroupBox * histBox;
  QGridLayout * histLayout;

  int lastFilledIndex[MaxNDigitizer][MaxNChannels];
  int loopFilledIndex[MaxNDigitizer][MaxNChannels];

  bool fillHistograms;

  QString settingPath;

  unsigned short maxFillTimeinMilliSec;
  unsigned short maxFillTimePerDigi;

  bool isSignalSlotActive;

};

inline void NeutronGamma::SetUpCanvas(){

  setWindowTitle("Neutron-Gamma Separation");

  QScreen * screen = QGuiApplication::primaryScreen();
  QRect screenGeo = screen->geometry();
  if( screenGeo.width() < 1000 || screenGeo.height() < 800) {
    setGeometry(0, 0, screenGeo.width() - 100, screenGeo.height() - 100);
  }else{
    setGeometry(0, 0, 1000, 800);
  }

  QWidget * layoutWidget = new QWidget(this);
  setCentralWidget(layoutWidget);
  QVBoxLayout * layout = new QVBoxLayout(layoutWidget);
  layoutWidget->setLayout(layout);

  {//^==================================
    QGroupBox * controlBox = new QGroupBox("Control", this);
    layout->addWidget(controlBox);
    QGridLayout * ctrlLayout = new QGridLayout(controlBox);
    controlBox->setLayout(ctrlLayout);


    cbDigi = new RComboBox(this);
    for( unsigned int i = 0; i < nDigi; i++) cbDigi->addItem("Digi-" + QString::number( digi[i]->GetSerialNumber() ), i);
    ctrlLayout->addWidget(cbDigi, 0, 0, 1, 2);
    connect( cbDigi, &RComboBox::currentIndexChanged, this, [=](int index){
      isSignalSlotActive = false;
      cbCh->clear();
      cbCh->addItem("All Ch", digi[index]->GetNumInputCh() );
      for( int i = 0; i < digi[index]->GetNumInputCh(); i++) cbCh->addItem("ch-" + QString::number( i ), i);

      hist2D->Clear();
      isSignalSlotActive = true;      
    });

    cbCh   = new RComboBox(this);
    for( int i = 0; i < digi[0]->GetNumInputCh(); i++) cbCh->addItem("ch-" + QString::number( i ), i);
    ctrlLayout->addWidget(cbCh, 0, 2, 1, 2);
    // connect( cbCh, &RComboBox::currentIndexChanged, this, &SingleSpectra::ChangeHistView);
    connect( cbCh, &RComboBox::currentIndexChanged, this, [=](){
      hist2D->Clear();  
    });

    QCheckBox * chkIsFillHistogram = new QCheckBox("Fill Histograms", this);
    ctrlLayout->addWidget(chkIsFillHistogram, 0, 8);
    connect(chkIsFillHistogram, &QCheckBox::stateChanged, this, [=](int state){ fillHistograms = state;});
    chkIsFillHistogram->setChecked(false);
    fillHistograms = false;

    QLabel * lbSettingPath = new QLabel( settingPath , this);
    ctrlLayout->addWidget(lbSettingPath, 1, 0, 1, 8);

  }

  {//^==================================== 

    histBox = new QGroupBox("Histgrams", this);
    layout->addWidget(histBox, 10);
    histLayout = new QGridLayout(histBox);
    histBox->setLayout(histLayout);

    double eMax = 50000;
    double eMin = 0;
    double nBin = 1000;

    // for( unsigned int i = 0; i < MaxNDigitizer; i++){
    //   if( i >= nDigi ) continue;
    //   for( int j = 0; j < digi[i]->GetNumInputCh(); j++){
    //     if( i < nDigi ) {
    //       hist2D[i][j] = new Histogram2D("Digi-" + QString::number(digi[i]->GetSerialNumber()), "Long Energy [ch]", "Short Energy [ch]", nBin, eMin, eMax, nBin, eMin, eMax);
    //     }else{
    //       hist2D[i][j] = nullptr;
    //     }
    //   }
    // }
    // histLayout->addWidget(hist2D[0][0], 0, 0);
    
    hist2D = new Histogram2D("Neutron-Gamma", "PSD = (l-s)/l", "Short Energy [ch]", nBin, eMin, eMax, nBin, 0, 1);

    histLayout->addWidget(hist2D, 0, 0);

  }

}

inline void NeutronGamma::ClearInternalDataCount(){
  for( unsigned int i = 0; i < nDigi; i++){
    for( int ch = 0; ch < MaxRegChannel ; ch++) {
      lastFilledIndex[i][ch] = -1;
      loopFilledIndex[i][ch] = 0;
    }
  }
}

inline void NeutronGamma::UpdateHistograms(){
  if( !fillHistograms ) return;
  if( this->isVisible() == false ) return;

  int ID = cbDigi->currentData().toInt();
  int ch = cbCh->currentData().toInt();

  int lastIndex = digi[ID]->GetData()->GetDataIndex(ch);
  if( lastIndex < 0 ) return;

  int loopIndex = digi[ID]->GetData()->GetLoopIndex(ch);

  int temp1 = lastIndex + loopIndex * digi[ID]->GetData()->GetDataSize();
  int temp2 = lastFilledIndex[ID][ch] + loopFilledIndex[ID][ch] * digi[ID]->GetData()->GetDataSize() + 1;

  if( temp1 - temp2 > digi[ID]->GetData()->GetDataSize() ) { //DefaultDataSize = 10k
    temp2 = temp1 - digi[ID]->GetData()->GetDataSize();
    lastFilledIndex[ID][ch] = lastIndex;
    lastFilledIndex[ID][ch] = loopIndex - 1;
  }

  for( int j = 0 ; j <= temp1 - temp2; j ++){
    lastFilledIndex[ID][ch] ++;
    if( lastFilledIndex[ID][ch] > digi[ID]->GetData()->GetDataSize() ) {
      lastFilledIndex[ID][ch] = 0;
      loopFilledIndex[ID][ch] ++;
    }

    uShort data_long = digi[ID]->GetData()->GetEnergy(ch, lastFilledIndex[ID][ch]);
    uShort data_short = digi[ID]->GetData()->GetEnergy2(ch, lastFilledIndex[ID][ch]);

    // printf(" ch: %d, last fill idx : %d | %d \n", ch, lastFilledIndex[ID][ch], data);

    double psd = (data_long - data_short) *1.0 / data_long;

    hist2D->Fill(data_long, psd);
  }


  hist2D->UpdatePlot();

}

/*
void NeutronGamma::UpdateHistograms(){
  if( !fillHistograms ) return;

  timespec t0, t1;

  QVector<int> randomDigiList = generateNonRepeatedCombination(nDigi);

  for( int i = 0; i < nDigi; i++){
    int ID = randomDigiList[i];

    QVector<int> randomChList = generateNonRepeatedCombination(digi[ID]->GetNumInputCh());

    digiMTX[ID].lock();


    clock_gettime(CLOCK_REALTIME, &t0);
    for( int k = 0; k < digi[ID]->GetNumInputCh(); k ++ ){
      int ch = randomChList[k];
      int lastIndex = digi[ID]->GetData()->GetDataIndex(ch);
      // printf("--- ch %2d | last index %d \n", ch, lastIndex);
      if( lastIndex < 0 ) continue;

      int loopIndex = digi[ID]->GetData()->GetLoopIndex(ch);

      int temp1 = lastIndex + loopIndex * digi[ID]->GetData()->GetDataSize();
      int temp2 = lastFilledIndex[ID][ch] + loopFilledIndex[ID][ch] * digi[ID]->GetData()->GetDataSize() + 1;

      // printf("loopIndx : %d | ID now : %d, ID old : %d \n", loopIndex, temp1, temp2);

      if( temp1 <= temp2 ) continue;

      if( temp1 - temp2 > digi[ID]->GetData()->GetDataSize() ) { //DefaultDataSize = 10k
        temp2 = temp1 - digi[ID]->GetData()->GetDataSize();
        lastFilledIndex[ID][ch] = lastIndex;
        lastFilledIndex[ID][ch] = loopIndex - 1;
      }

      // printf("ch %d | regulated  ID now %d  new %d | last fill idx %d\n", ch, temp2, temp1, lastFilledIndex[ID][ch]);
      
      for( int j = 0 ; j <= temp1 - temp2; j ++){
        lastFilledIndex[ID][ch] ++;
        if( lastFilledIndex[ID][ch] > digi[ID]->GetData()->GetDataSize() ) {
          lastFilledIndex[ID][ch] = 0;
          loopFilledIndex[ID][ch] ++;
        }

        uShort data_long = digi[ID]->GetData()->GetEnergy(ch, lastFilledIndex[ID][ch]);
        uShort data_short = digi[ID]->GetData()->GetEnergy2(ch, lastFilledIndex[ID][ch]);

        // printf(" ch: %d, last fill idx : %d | %d \n", ch, lastFilledIndex[ID][ch], data);

        hist2D[ID][ch]->Fill(data_long, data_short);
      }
      if( histVisibility[ID][ch]  ) hist2D[ID][ch]->UpdatePlot();

      clock_gettime(CLOCK_REALTIME, &t1);
      if( t1.tv_nsec - t0.tv_nsec + (t1.tv_sec - t0.tv_sec)*1e9 > maxFillTimePerDigi * 1e6 ) break;  
    }

    digiMTX[ID].unlock();

  }

}
*/

inline QVector<int> NeutronGamma::generateNonRepeatedCombination(int size) {
  QVector<int> combination;
  for (int i = 0; i < size; ++i) combination.append(i);

  for (int i = 0; i < size - 1; ++i) {
    int j = QRandomGenerator::global()->bounded(i, size);
    combination.swapItemsAt(i, j);
  }
  return combination;
}

#endif