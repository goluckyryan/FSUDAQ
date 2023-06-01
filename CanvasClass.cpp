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

  enableSignalSlot = false;

  setWindowTitle("Canvas");
  setGeometry(0, 0, 1000, 800);  
  //setWindowFlags( this->windowFlags() & ~Qt::WindowCloseButtonHint );

  QWidget * layoutWidget = new QWidget(this);
  setCentralWidget(layoutWidget);
  QVBoxLayout * layout = new QVBoxLayout(layoutWidget);
  layoutWidget->setLayout(layout);

  {//^========================
    QGroupBox * controlBox = new QGroupBox("Control", this);
    layout->addWidget(controlBox);
    QGridLayout * ctrlLayout = new QGridLayout(controlBox);
    controlBox->setLayout(ctrlLayout);

    cbDigi = new RComboBox(this);
    for( unsigned int i = 0; i < nDigi; i++) cbDigi->addItem("Digi-" + QString::number( digi[i]->GetSerialNumber() ), i);
    ctrlLayout->addWidget(cbDigi, 0, 0, 1, 2);
    connect( cbDigi, &RComboBox::currentIndexChanged, this, &Canvas::ChangeHistView);

    cbCh   = new RComboBox(this);
    for( int i = 0; i < MaxNChannels; i++) cbCh->addItem("ch-" + QString::number( i ), i);
    ctrlLayout->addWidget(cbCh, 0, 2, 1, 2);
    connect( cbCh, &RComboBox::currentIndexChanged, this, &Canvas::ChangeHistView);

    QPushButton * bnClearHist = new QPushButton("Clear Hist.", this);
    ctrlLayout->addWidget(bnClearHist, 0, 4, 1, 2);
    connect(bnClearHist, &QPushButton::clicked, this, [=](){
      for( unsigned int i = 0; i < nDigi; i++){
        for( int j = 0; j < MaxNChannels; j++){
          if( hist[i][j] ) hist[i][j]->Clear();
        }
      }
    });

    QLabel * lbNBin = new QLabel("#Bin:", this);
    lbNBin->setAlignment(Qt::AlignRight | Qt::AlignCenter );
    ctrlLayout->addWidget(lbNBin, 1, 0);
    sbNBin = new RSpinBox(this);
    sbNBin->setMinimum(0);
    sbNBin->setMaximum(500);
    ctrlLayout->addWidget(sbNBin, 1, 1);
    connect(sbNBin, &RSpinBox::valueChanged, this, [=](){
      if( !enableSignalSlot ) return;
      sbNBin->setStyleSheet("color : blue;");
      bnReBin->setEnabled(true);
    });

    QLabel * lbXMin = new QLabel("X-Min:", this);
    lbXMin->setAlignment(Qt::AlignRight | Qt::AlignCenter );
    ctrlLayout->addWidget(lbXMin, 1, 2);
    sbXMin = new RSpinBox(this);
    sbXMin->setMinimum(-0x3FFF);
    sbXMin->setMaximum(0x3FFF);
    ctrlLayout->addWidget(sbXMin, 1, 3);
    connect(sbXMin, &RSpinBox::valueChanged, this, [=](){
      if( !enableSignalSlot ) return;
      sbXMin->setStyleSheet("color : blue;");
      bnReBin->setEnabled(true);
    });

    QLabel * lbXMax = new QLabel("X-Max:", this);
    lbXMax->setAlignment(Qt::AlignRight | Qt::AlignCenter );
    ctrlLayout->addWidget(lbXMax, 1, 4);
    sbXMax = new RSpinBox(this);
    sbXMax->setMinimum(-0x3FFF);
    sbXMax->setMaximum(0x3FFF);
    ctrlLayout->addWidget(sbXMax, 1, 5);
    connect(sbXMin, &RSpinBox::valueChanged, this, [=](){
      if( !enableSignalSlot ) return;
      sbXMin->setStyleSheet("color : blue;");
      bnReBin->setEnabled(true);
    });

    bnReBin = new QPushButton("Rebin and Clear Histogram", this);
    ctrlLayout->addWidget(bnReBin, 1, 6);
    bnReBin->setEnabled(false);
    connect(bnReBin, &QPushButton::clicked, this, [=](){
      if( !enableSignalSlot ) return;
      int bd = cbDigi->currentIndex();
      int ch = cbCh->currentIndex();
      hist[bd][ch]->Rebin( sbNBin->value(), sbXMin->value(), sbXMax->value());

      sbNBin->setStyleSheet("");
      sbXMin->setStyleSheet("");
      sbXMax->setStyleSheet("");
      bnReBin->setEnabled(false);

      hist[bd][ch]->UpdatePlot();

    });

  }

  {//^========================
    histBox = new QGroupBox("Histgrams", this);
    layout->addWidget(histBox);
    histLayout = new QGridLayout(histBox);
    histBox->setLayout(histLayout);

    double xMax = 4000;
    double xMin = 0;
    double nBin = 100;

    for( unsigned int i = 0; i < MaxNDigitizer; i++){
      if( i >= nDigi ) continue;
      for( int j = 0; j < digi[i]->GetNChannels(); j++){
        if( i < nDigi ) {
          hist[i][j] = new Histogram1D("Digi-" + QString::number(digi[i]->GetSerialNumber()) +", Ch-" +  QString::number(j), "Raw Energy [ch]", nBin, xMin, xMax);
        }else{
          hist[i][j] = nullptr;
        }
      }
    }

    histLayout->addWidget(hist[0][0], 0, 0);

    sbNBin->setValue(nBin);
    sbXMin->setValue(xMin);
    sbXMax->setValue(xMax);
  }

  layout->setStretch(0, 1);
  layout->setStretch(1, 6);


  for( unsigned int i = 0; i < nDigi; i++){
    for( int ch = 0; ch < MaxNChannels ; ch++) {
      lastFilledIndex[i][ch] = -1;
      loopFilledIndex[i][ch] = 0;
    }
  }
  oldBd = -1;
  oldCh = -1;

  enableSignalSlot = true;

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

  if( enableSignalSlot ){
    sbNBin->setValue(hist[bd][ch]->GetNBin());
    sbXMin->setValue(hist[bd][ch]->GetXMin());
    sbXMax->setValue(hist[bd][ch]->GetXMax());

    sbNBin->setStyleSheet("");
    sbXMin->setStyleSheet("");
    sbXMax->setStyleSheet("");
  }

  bnReBin->setEnabled(false);

  oldBd = bd;
  oldCh = ch;
}

void Canvas::UpdateCanvas(){

  for( int i = 0; i < nDigi; i++){

    digiMTX[i].lock();
    for( int ch = 0; ch < digi[i]->GetNChannels(); ch ++ ){
      int lastIndex = digi[i]->GetData()->DataIndex[ch];
      int loopIndex = digi[i]->GetData()->LoopIndex[ch];

      int temp1 = lastIndex + loopIndex*MaxNData;
      int temp2 = lastFilledIndex[i][ch] + loopFilledIndex[i][ch]*MaxNData;

      //printf("%d |%d   %d \n", ch, temp2, temp1);
      if( lastIndex < 0 ) continue;

      if( temp1 <= temp2 ) continue;
      
      for( int j = 0 ; j <= temp1 - temp2; j ++){
        lastFilledIndex[i][ch] ++;
        if( lastFilledIndex[i][ch] > MaxNData ) {
          lastFilledIndex[i][ch] = 0;
          loopFilledIndex[i][ch] ++;
        }
        hist[i][ch]->Fill( digi[i]->GetData()->Energy[ch][lastFilledIndex[i][ch]]);
      }
      hist[i][ch]->UpdatePlot();
    }
    digiMTX[i].unlock();

  }
}