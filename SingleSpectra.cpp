#include "SingleSpectra.h"

#include <QValueAxis>
#include <QGroupBox>
#include <QStandardItemModel>
#include <QLabel>
// #include <QScreen>

SingleSpectra::SingleSpectra(Digitizer ** digi, unsigned int nDigi, QString rawDataPath, QMainWindow * parent) : QMainWindow(parent){
  DebugPrint("%s", "SingleSpectra");
  this->digi = digi;
  this->nDigi = nDigi;
  this->settingPath = rawDataPath + "/HistogramSettings.txt";

  maxFillTimeinMilliSec = 1000;  
  maxFillTimePerDigi = maxFillTimeinMilliSec/nDigi;

  isSignalSlotActive = true;

  setWindowTitle("Single Histograms");

  //====== resize window if screen too small
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
        }
        if( hist2D[i] ) hist2D[i]->Clear();
      }
    });

    chkIsFillHistogram = new QCheckBox("Fill Histograms", this);
    ctrlLayout->addWidget(chkIsFillHistogram, 0, 6, 1, 2);
    chkIsFillHistogram->setChecked(false);
    isFillingHistograms = false;

    QLabel * lbSettingPath = new QLabel( settingPath , this);
    ctrlLayout->addWidget(lbSettingPath, 1, 0, 1, 6);

    QPushButton * bnSaveButton = new QPushButton("Save Hist. Settings", this);
    ctrlLayout->addWidget(bnSaveButton, 1, 6, 1, 2);
    connect(bnSaveButton, &QPushButton::clicked, this, &SingleSpectra::SaveSetting);

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
          if( digi[i]->GetDPPType() == DPPTypeCode::DPP_PSD_CODE ){
            hist[i][j]->AddDataList("Short Energy", Qt::green);
          }
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


  workerThread = new QThread(this);
  histWorker = new HistWorker(this);
  timer = new QTimer(this);

  histWorker->moveToThread(workerThread);

  // Setup the timer to trigger every second
  connect(timer, &QTimer::timeout, histWorker, &HistWorker::FillHistograms);

  connect( histWorker, &HistWorker::workDone, this, &SingleSpectra::ReplotHistograms);

  workerThread->start();

}

SingleSpectra::~SingleSpectra(){
  DebugPrint("%s", "SingleSpectra");

  timer->stop();

  if( workerThread->isRunning() ){
    workerThread->quit();
    workerThread->wait();
  }

  SaveSetting();

  for( unsigned int i = 0; i < nDigi; i++ ){
    for( int ch = 0; ch < digi[i]->GetNumInputCh(); ch++){
      delete hist[i][ch];
    }
    delete hist2D[i];
  }
}

void SingleSpectra::ClearInternalDataCount(){
  DebugPrint("%s", "SingleSpectra");
  for( unsigned int i = 0; i < nDigi; i++){
    for( int ch = 0; ch < MaxRegChannel ; ch++) {
      lastFilledIndex[i][ch] = -1;
      loopFilledIndex[i][ch] = 0;
    }
  }
}

void SingleSpectra::ChangeHistView(){
  DebugPrint("%s", "SingleSpectra");
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

  // printf("%s | %d %d \n", __func__, chkIsFillHistogram->checkState(), isFillingHistograms);
  if( this->isVisible() == false ) return;
  if( chkIsFillHistogram->checkState() == Qt::Unchecked ) return;
  if( isFillingHistograms) return;

  isFillingHistograms = true;
  timespec t0, t1;

  QVector<int> randomDigiList = generateNonRepeatedCombination(nDigi);

  // qDebug() << randomDigiList;

  for( int i = 0; i < nDigi; i++){
    int ID = randomDigiList[i];

    QVector<int> randomChList = generateNonRepeatedCombination(digi[ID]->GetNumInputCh());

    // qDebug() << randomChList;
    // digiMTX[ID].lock();
    // digi[ID]->GetData()->PrintAllData();

    clock_gettime(CLOCK_REALTIME, &t0);
    for( int k = 0; k < digi[ID]->GetNumInputCh(); k ++ ){
      int ch = randomChList[k];
      int lastIndex = digi[ID]->GetData()->GetDataIndex(ch);
      if( lastIndex < 0 ) continue;
      // printf("--- ch %2d | last index %d \n", ch, lastIndex);

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

        uShort data = digi[ID]->GetData()->GetEnergy(ch, lastFilledIndex[ID][ch]);

        // printf(" ch: %d, last fill idx : %d | %d \n", ch, lastFilledIndex[ID][ch], data);

        hist[ID][ch]->Fill( data );
        if( digi[i]->GetDPPType() == DPPTypeCode::DPP_PSD_CODE ){
          uShort e2 = digi[ID]->GetData()->GetEnergy2(ch, lastFilledIndex[ID][ch]);
          // printf("%u \n", e2);
          hist[ID][ch]->Fill( e2, 1);
        }
        hist2D[ID]->Fill(ch, data);
      }
      // if( histVisibility[ID][ch]  ) hist[ID][ch]->UpdatePlot();

      clock_gettime(CLOCK_REALTIME, &t1);
      if( t1.tv_nsec - t0.tv_nsec + (t1.tv_sec - t0.tv_sec)*1e9 > maxFillTimePerDigi * 1e6 ) break;  
    }

    // if( hist2DVisibility[ID] ) hist2D[ID]->UpdatePlot();
    // digiMTX[ID].unlock();

  }

  isFillingHistograms = false;

}

void SingleSpectra::ReplotHistograms(){

  int ID = cbDigi->currentData().toInt();
  int ch = cbCh->currentData().toInt();

  if( ch == digi[ID]->GetNumInputCh()) {
    if( hist2DVisibility[ID] ) hist2D[ID]->UpdatePlot();
    return;
  }

  if( histVisibility[ID][ch]  ) hist[ID][ch]->UpdatePlot();

}

void SingleSpectra::SaveSetting(){
  DebugPrint("%s", "SingleSpectra");

  QFile file(settingPath );

  if (!file.exists()) {
    // If the file does not exist, create it
    if (!file.open(QIODevice::WriteOnly)) {
      qWarning() << "Could not create file" << settingPath;
    } else {
      qDebug() << "File" << settingPath  << "created successfully";
      file.close();
    }
  }

  if( file.open(QIODevice::Text | QIODevice::WriteOnly) ){

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
      QString b = QString::number(hist2D[i]->GetXNBin()-2).rightJustified(6, ' ');
      QString c = QString::number(hist2D[i]->GetXMin()).rightJustified(6, ' ');
      QString d = QString::number(hist2D[i]->GetXMax()).rightJustified(6, ' ');
      QString e = QString::number(hist2D[i]->GetYNBin()-2).rightJustified(6, ' ');
      QString f = QString::number(hist2D[i]->GetYMin()).rightJustified(6, ' ');
      QString g = QString::number(hist2D[i]->GetYMax()).rightJustified(6, ' ');
      file.write( QString("%1 %2 %3 %4 %5 %6 %7\n").arg(a).arg(b).arg(c).arg(d).arg(e).arg(f).arg(g).toStdString().c_str() );
    }

    file.write("##========== End of file\n");
    file.close();

    printf("Saved Histogram Settings to %s\n", settingPath.toStdString().c_str());
  }else{
    printf("%s|cannot open HistogramSettings.txt\n", __func__);
  }

}

void SingleSpectra::LoadSetting(){
  DebugPrint("%s", "SingleSpectra");

  QFile file(settingPath);

  if( file.open(QIODevice::Text | QIODevice::ReadOnly) ){

    QTextStream in(&file);
    QString line = in.readLine();

    int digiSN = 0;
    int digiID = -1;

    while ( !line.isNull() ){
      if( line.contains("##========== ") ) break;
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
        // if( list.count() != 4 ) {
        //   line = in.readLine();
        //   continue;
        // }
        QVector<float> data;
        for( int i = 0; i < list.count(); i++){   
          data.push_back(list[i].toFloat());
        }
        
        if( 0 <= data[0] && data[0] < digi[digiID]->GetNumInputCh() ){
          hist[digiID][int(data[0])]->Rebin(data[1], data[2], data[3]);
        }

        if( int(data[0]) == digi[digiID]->GetNumInputCh() && data.size() == 7 ){
          hist2D[digiID]->Rebin(int(data[1]), data[2], data[3], int(data[4]), data[5], data[6]);
        }

      }

      line = in.readLine();
    }

  }else{

    printf("%s|cannot open HistogramSettings.txt\n", __func__);

  }

}

QVector<int> SingleSpectra::generateNonRepeatedCombination(int size) {
  QVector<int> combination;
  for (int i = 0; i < size; ++i) combination.append(i);

  for (int i = 0; i < size - 1; ++i) {
    int j = QRandomGenerator::global()->bounded(i, size);
    combination.swapItemsAt(i, j);
  }
  return combination;
}