#include "SingleSpectra.h"

#include <QValueAxis>
#include <QRandomGenerator>
#include <QGroupBox>
#include <QStandardItemModel>
#include <QLabel>
#include <QRandomGenerator>

SingleSpectra::SingleSpectra(Digitizer ** digi, unsigned int nDigi, QString rawDataPath, QMainWindow * parent) : QMainWindow(parent){

  this->digi = digi;
  this->nDigi = nDigi;
  this->rawDataPath = rawDataPath;

  setWindowTitle("1-D Histograms");
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
    connect( cbDigi, &RComboBox::currentIndexChanged, this, &SingleSpectra::ChangeHistView);

    cbCh   = new RComboBox(this);
    for( int i = 0; i < MaxNChannels; i++) cbCh->addItem("ch-" + QString::number( i ), i);
    ctrlLayout->addWidget(cbCh, 0, 2, 1, 2);
    connect( cbCh, &RComboBox::currentIndexChanged, this, &SingleSpectra::ChangeHistView);

    QPushButton * bnClearHist = new QPushButton("Clear All Hist.", this);
    ctrlLayout->addWidget(bnClearHist, 0, 4, 1, 2);
    connect(bnClearHist, &QPushButton::clicked, this, [=](){
      for( unsigned int i = 0; i < nDigi; i++){
        for( int j = 0; j < MaxNChannels; j++){
          if( hist[i][j] ) hist[i][j]->Clear();
        }
      }
    });

    QCheckBox * chkIsFillHistogram = new QCheckBox("Fill Histograms", this);
    ctrlLayout->addWidget(chkIsFillHistogram, 0, 6);
    connect(chkIsFillHistogram, &QCheckBox::stateChanged, this, [=](int state){ fillHistograms = state;});
    chkIsFillHistogram->setChecked(false);
    fillHistograms = false;

  }

  {//^========================
    histBox = new QGroupBox("Histgrams", this);
    layout->addWidget(histBox);
    histLayout = new QGridLayout(histBox);
    histBox->setLayout(histLayout);

    double xMax = 5000;
    double xMin = 0;
    double nBin = 200;

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

    LoadSetting();

    histLayout->addWidget(hist[0][0], 0, 0);
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

}

SingleSpectra::~SingleSpectra(){

  SaveSetting();

  for( unsigned int i = 0; i < nDigi; i++ ){
    for( int ch = 0; ch < digi[i]->GetNChannels(); ch++){
      delete hist[i][ch];
    }
  }
}

void SingleSpectra::ChangeHistView(){

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

void SingleSpectra::FillHistograms(){
  if( !fillHistograms ) return;

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

void SingleSpectra::SaveSetting(){

  QFile file(rawDataPath + "/singleSpectraSetting.txt");

  file.open(QIODevice::Text | QIODevice::WriteOnly);

  for( unsigned int i = 0; i < nDigi; i++){
    file.write(("======= " + QString::number(digi[i]->GetSerialNumber()) + "\n").toStdString().c_str());
    for( int ch = 0; ch < digi[i]->GetNChannels() ; ch++){
      QString a = QString::number(ch).rightJustified(2, ' ');
      QString b = QString::number(hist[i][ch]->GetNBin()).rightJustified(6, ' ');
      QString c = QString::number(hist[i][ch]->GetXMin()).rightJustified(6, ' ');
      QString d = QString::number(hist[i][ch]->GetXMax()).rightJustified(6, ' ');
      file.write( QString("%1 %2 %3 %4\n").arg(a).arg(b).arg(c).arg(d).toStdString().c_str() );
    }
  }

  file.write("//========== End of file\n");
  file.close();
}

void SingleSpectra::LoadSetting(){

  QFile file(rawDataPath + "/singleSpectraSetting.txt");

  if( file.open(QIODevice::Text | QIODevice::ReadOnly) ){

    QTextStream in(&file);
    QString line = in.readLine();

    int digiSN = 0;
    int digiID = -1;

    while ( !line.isNull() ){
      if( line.contains("//========== ") ) break;
      if( line.contains("//") ) continue;
      if( line.contains("======= ") ){
        digiSN = line.mid(7).toInt();

        digiID = -1;
        for( unsigned int i = 0; i < nDigi; i++){
          if( digiSN == digi[i]->GetSerialNumber() ) {
            digiID = i;
            break;
          }
        }
        line = in.readLine();
        continue;
      }

      if( digiID >= 0 ){

        QStringList list = line.split(QRegularExpression("\\s+"));
        list.removeAll("");
        if( list.count() != 4 ) {
          line = in.readLine();
          continue;
        }
        QVector<int> data;
        for( int i = 0; i < list.count(); i++){   
          data.push_back(list[i].toInt());
        }

        hist[digiID][data[0]]->Rebin(data[1], data[2], data[3]);
      }

      line = in.readLine();
    }

  }else{

  }

}