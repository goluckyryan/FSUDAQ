#include "CanvasClass.h"

#include <QValueAxis>
#include <QRandomGenerator>
#include <QGroupBox>
#include <QStandardItemModel>
#include <QLabel>
#include <QRandomGenerator>

Canvas::Canvas(Digitizer ** digi, unsigned int nDigi, QMainWindow * parent) : QMainWindow(parent){

  this->digi = digi;
  this->nDigi = nDigi;

  setWindowTitle("Canvas");
  setGeometry(0, 0, 1000, 800);  
  //setWindowFlags( this->windowFlags() & ~Qt::WindowCloseButtonHint );

  QWidget * layoutWidget = new QWidget(this);
  setCentralWidget(layoutWidget);
  QVBoxLayout * layout = new QVBoxLayout(layoutWidget);
  layoutWidget->setLayout(layout);

  //========================
  QGroupBox * controlBox = new QGroupBox("Control", this);
  layout->addWidget(controlBox);
  QGridLayout * ctrlLayout = new QGridLayout(controlBox);
  controlBox->setLayout(ctrlLayout);

  QPushButton * bnClearHist = new QPushButton("Clear Hist.", this);
  ctrlLayout->addWidget(bnClearHist, 0, 0);
  connect(bnClearHist, &QPushButton::clicked, this, [=](){
    for( int i = 0; i < MaxNDigitizer; i++){
      for( int j = 0; j < MaxNChannels; j++){
        if( hist[i][j] ) hist[i][j]->Clear();
      }
    }
  });

  cbDigi = new RComboBox(this);
  for( unsigned int i = 0; i < nDigi; i++) cbDigi->addItem("Digi-" + QString::number( digi[i]->GetSerialNumber() ), i);
  ctrlLayout->addWidget(cbDigi, 1, 0);
  connect( cbDigi, &RComboBox::currentIndexChanged, this, &Canvas::ChangeHistView);

  cbCh   = new RComboBox(this);
  for( int i = 0; i < MaxNChannels; i++) cbCh->addItem("ch-" + QString::number( i ), i);
  ctrlLayout->addWidget(cbCh, 1, 1);
  connect( cbCh, &RComboBox::currentIndexChanged, this, &Canvas::ChangeHistView);

  //========================
  histBox = new QGroupBox("Histgrams", this);
  layout->addWidget(histBox);
  histLayout = new QGridLayout(histBox);
  histBox->setLayout(histLayout);

  double xMax = 4000;
  double xMin = 0;
  double nBin = 100;

  for( unsigned int i = 0; i < MaxNDigitizer; i++){
    for( int j = 0; j < digi[i]->GetNChannels(); j++){
      if( i < nDigi ) {
        hist[i][j] = new Histogram1D("Digi-" + QString::number(digi[i]->GetSerialNumber()) +", Ch-" +  QString::number(j), "Raw Energy [ch]", nBin, xMin, xMax);
      }else{
        hist[i][j] = nullptr;
      }
    }
  }

  histLayout->addWidget(hist[0][0], 0, 0);
  oldBd = -1;
  oldCh = -1;

}

Canvas::~Canvas(){
  for( unsigned int i = 0; i < nDigi; i++ ){
    for( int ch = 0; ch < digi[i]->GetNChannels(); ch++){
      delete hist[i][ch];
    }
  }
}

void Canvas::ChangeHistView(){

  if( oldCh >= 0 ) {
    histLayout->removeWidget(hist[oldBd][oldCh]);
    hist[oldBd][oldCh]->setParent(nullptr);
  }
  int bd = cbDigi->currentIndex();
  int ch = cbCh->currentIndex();

  histLayout->addWidget(hist[bd][ch], 0, 0);

  oldBd = bd;
  oldCh = ch;
}

void Canvas::UpdateCanvas(){

  for( int i = 0; i < nDigi; i++){

    digiMTX[i].lock();
    for( int ch = 0; ch < digi[i]->GetNChannels(); ch ++ ){
      int lastIndex = digi[i]->GetData()->DataIndex[ch];
      int nDecoded = digi[i]->GetData()->NumEventsDecoded[ch];
    
      if( nDecoded == 0 ) continue;

      for( int j = lastIndex - nDecoded + 1; j <= lastIndex; j ++){
        hist[i][ch]->Fill( digi[i]->GetData()->Energy[ch][j]);
      }

      hist[i][ch]->UpdatePlot();

    }
    digiMTX[i].unlock();

  }
}