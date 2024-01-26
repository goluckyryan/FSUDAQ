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

  isSignalSlotActive = true;

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
    connect( cbDigi, &RComboBox::currentIndexChanged, this, [=](int index){
      isSignalSlotActive = false;
      cbCh->clear();
      cbCh->addItem("All Ch", digi[index]->GetNumInputCh() );
      for( int i = 0; i < digi[index]->GetNumInputCh(); i++) cbCh->addItem("ch-" + QString::number( i ), i);

      isSignalSlotActive = true;

      //printf("oldCh = %d \n", oldCh);
      if( oldCh >=  digi[index]->GetNumInputCh()) {
        cbCh->setCurrentIndex(0);
      }else{
        if( oldCh >= 0 ){
          cbCh->setCurrentIndex(oldCh);
        }else{
          cbCh->setCurrentIndex(0);
        }
      }
      ChangeHistView();
      
    });

    cbCh   = new RComboBox(this);
    cbCh->addItem("All Ch", digi[0]->GetNumInputCh());
    for( int i = 0; i < digi[0]->GetNumInputCh(); i++) cbCh->addItem("ch-" + QString::number( i ), i);
    ctrlLayout->addWidget(cbCh, 0, 2, 1, 2);
    connect( cbCh, &RComboBox::currentIndexChanged, this, &SingleSpectra::ChangeHistView);

    QPushButton * bnClearHist = new QPushButton("Clear All Hist.", this);
    ctrlLayout->addWidget(bnClearHist, 0, 4, 1, 2);
    connect(bnClearHist, &QPushButton::clicked, this, [=](){
      for( unsigned int i = 0; i < nDigi; i++){
        for( int j = 0; j < digi[i]->GetNumInputCh(); j++){
          if( hist[i][j] ) hist[i][j]->Clear();
          lastFilledIndex[i][j] = -1;
          loopFilledIndex[i][j] = 0;
        }
        if( hist2D[i] ) hist2D[i]->Clear();
      }
    });

    QCheckBox * chkIsFillHistogram = new QCheckBox("Fill Histograms", this);
    ctrlLayout->addWidget(chkIsFillHistogram, 0, 6);
    connect(chkIsFillHistogram, &QCheckBox::stateChanged, this, [=](int state){ fillHistograms = state;});
    chkIsFillHistogram->setChecked(false);
    fillHistograms = false;

  }

  {//^========================
    for( unsigned int i = 0; i < nDigi; i++ ) {
      hist2DVisibility[i] = false;
      for( int j = 0; j < digi[i]->GetNumInputCh() ; j++ ) {
        histVisibility[i][j] = false;
      }
    }

    histBox = new QGroupBox("Histgrams", this);
    layout->addWidget(histBox);
    histLayout = new QGridLayout(histBox);
    histBox->setLayout(histLayout);

    double eMax = 5000;
    double eMin = 0;
    double nBin = 200;

    for( unsigned int i = 0; i < MaxNDigitizer; i++){
      if( i >= nDigi ) continue;
      for( int j = 0; j < digi[i]->GetNumInputCh(); j++){
        if( i < nDigi ) {
          hist[i][j] = new Histogram1D("Digi-" + QString::number(digi[i]->GetSerialNumber()) +", Ch-" +  QString::number(j), "Raw Energy [ch]", nBin, eMin, eMax);
        }else{
          hist[i][j] = nullptr;
        }
      }
      hist2D[i] = new Histogram2D("Digi-" + QString::number(digi[i]->GetSerialNumber()), "Channel", "Raw Energy [ch]", digi[i]->GetNumInputCh(), 0, digi[i]->GetNumInputCh(), nBin, eMin, eMax);
      hist2D[i]->SetChannelMap(true, digi[i]->GetNumInputCh() < 20  ? 1 : 4);
      hist2D[i]->Rebin(digi[i]->GetNumInputCh(), -0.5, digi[i]->GetNumInputCh()+0.5, nBin, eMin, eMax);
    }

    LoadSetting();

    histLayout->addWidget(hist2D[0], 0, 0);
    hist2DVisibility[0] = true;
    oldBd = 0;
    oldCh = digi[0]->GetNumInputCh();
  }

  layout->setStretch(0, 1);
  layout->setStretch(1, 6);

  ClearInternalDataCount();


}

SingleSpectra::~SingleSpectra(){

  SaveSetting();

  for( unsigned int i = 0; i < nDigi; i++ ){
    for( int ch = 0; ch < digi[i]->GetNumInputCh(); ch++){
      delete hist[i][ch];
    }
    delete hist2D[i];
  }
}

void SingleSpectra::ClearInternalDataCount(){
  for( unsigned int i = 0; i < nDigi; i++){
    for( int ch = 0; ch < MaxRegChannel ; ch++) {
      lastFilledIndex[i][ch] = -1;
      loopFilledIndex[i][ch] = 0;
    }
  }
}

void SingleSpectra::ChangeHistView(){

  if( !isSignalSlotActive ) return;

  int bd = cbDigi->currentIndex();
  int ch = cbCh->currentData().toInt();

  //printf("bd : %d, ch : %d \n", bd, ch);

  // Remove oldCh
  if( oldCh >= 0 && oldCh < digi[oldBd]->GetNumInputCh()){
    histLayout->removeWidget(hist[oldBd][oldCh]);
    hist[oldBd][oldCh]->setParent(nullptr);
    histVisibility[oldBd][oldCh] = false;
  }

  if( oldCh == digi[oldBd]->GetNumInputCh() ){
    histLayout->removeWidget(hist2D[oldBd]);
    hist2D[oldBd]->setParent(nullptr);
    hist2DVisibility[oldBd] = false;
  }

  // Add ch
  if( ch >=0 && ch < digi[bd]->GetNumInputCh()) {
    histLayout->addWidget(hist[bd][ch], 0, 0);
    histVisibility[bd][ch] = true;

    hist[bd][ch]->UpdatePlot();
  }

  if( ch == digi[bd]->GetNumInputCh() ){
    histLayout->addWidget(hist2D[bd], 0, 0);
    hist2DVisibility[bd] = true;
    hist2D[bd]->UpdatePlot();
  }

  oldBd = bd;
  oldCh = ch;

  // for( unsigned int i = 0; i < nDigi; i++ ){
  //   if( hist2DVisibility[i] ) printf(" hist2D-%d is visible\n", i); 
  //   for( int j = 0; j < digi[i]->GetNumInputCh(); j++){
  //     if( histVisibility[i][j] ) printf(" hist-%d-%d is visible\n", i, j); 
  //   }
  // }

}

void SingleSpectra::FillHistograms(){
  if( !fillHistograms ) return;

  for( int i = 0; i < nDigi; i++){

    digiMTX[i].lock();
    for( int ch = 0; ch < digi[i]->GetNumInputCh(); ch ++ ){
      int lastIndex = digi[i]->GetData()->GetDataIndex(ch);
      int loopIndex = digi[i]->GetData()->GetLoopIndex(ch);

      int temp1 = lastIndex + loopIndex * digi[i]->GetData()->GetDataSize();
      int temp2 = lastFilledIndex[i][ch] + loopFilledIndex[i][ch] * digi[i]->GetData()->GetDataSize();

      // printf("%d |%d   %d \n", ch, temp2, temp1);
      if( lastIndex < 0 ) continue;

      if( temp1 <= temp2 ) continue;
      
      for( int j = 0 ; j <= temp1 - temp2; j ++){
        lastFilledIndex[i][ch] ++;
        if( lastFilledIndex[i][ch] > digi[i]->GetData()->GetDataSize() ) {
          lastFilledIndex[i][ch] = 0;
          loopFilledIndex[i][ch] ++;
        }
        hist[i][ch]->Fill( digi[i]->GetData()->GetEnergy(ch, lastFilledIndex[i][ch]));

        hist2D[i]->Fill(ch, digi[i]->GetData()->GetEnergy(ch, lastFilledIndex[i][ch]));

      }
      if( histVisibility[i][ch]  ) hist[i][ch]->UpdatePlot();
      if( hist2DVisibility[i] ) hist2D[i]->UpdatePlot();
    }
    digiMTX[i].unlock();

  }
}

void SingleSpectra::SaveSetting(){

  QFile file(rawDataPath + "/singleSpectraSetting.txt");

  file.open(QIODevice::Text | QIODevice::WriteOnly);

  for( unsigned int i = 0; i < nDigi; i++){
    file.write(("======= " + QString::number(digi[i]->GetSerialNumber()) + "\n").toStdString().c_str());
    for( int ch = 0; ch < digi[i]->GetNumInputCh() ; ch++){
      QString a = QString::number(ch).rightJustified(2, ' ');
      QString b = QString::number(hist[i][ch]->GetNBin()).rightJustified(6, ' ');
      QString c = QString::number(hist[i][ch]->GetXMin()).rightJustified(6, ' ');
      QString d = QString::number(hist[i][ch]->GetXMax()).rightJustified(6, ' ');
      file.write( QString("%1 %2 %3 %4\n").arg(a).arg(b).arg(c).arg(d).toStdString().c_str() );
    }

    QString a = QString::number(digi[i]->GetNumInputCh()).rightJustified(2, ' ');
    QString b = QString::number(hist2D[i]->GetXNBin()).rightJustified(6, ' ');
    QString c = QString::number(hist2D[i]->GetXMin()).rightJustified(6, ' ');
    QString d = QString::number(hist2D[i]->GetXMax()).rightJustified(6, ' ');
    QString e = QString::number(hist2D[i]->GetYNBin()).rightJustified(6, ' ');
    QString f = QString::number(hist2D[i]->GetYMin()).rightJustified(6, ' ');
    QString g = QString::number(hist2D[i]->GetYMax()).rightJustified(6, ' ');
    file.write( QString("%1 %2 %3 %4 %5 %6 %7\n").arg(a).arg(b).arg(c).arg(d).arg(e).arg(f).arg(g).toStdString().c_str() );
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
        
        if( 0 <= data[0] && data[0] < digi[digiID]->GetNumInputCh() ){
          hist[digiID][data[0]]->Rebin(data[1], data[2], data[3]);
        }

        if( data[0] == digi[digiID]->GetNumInputCh() && data.size() == 7 ){
          hist2D[digiID]->Rebin(data[1], data[2], data[3], data[4], data[5], data[6]);
        }

      }

      line = in.readLine();
    }

  }else{

  }

}