#include "SingleSpectra.h"

#include <QValueAxis>
#include <QGroupBox>
#include <QStandardItemModel>
#include <QLabel>
#include <queue>
#include <algorithm>
// #include <QScreen>

SingleSpectra::SingleSpectra(Digitizer ** digi, unsigned int nDigi, QString rawDataPath, QMainWindow * parent) : QMainWindow(parent){
  DebugPrint("%s", "SingleSpectra");
  this->digi = digi;
  this->nDigi = nDigi;
  this->settingPath = rawDataPath + "/HistogramSettings.txt";

  maxFillTimeinMilliSec = SingleHistogramFillingTime;  

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
      // if( oldCh >=  digi[index]->GetNumInputCh()) {
      //   cbCh->setCurrentIndex(0);
      // }else{
      //   if( oldCh >= 0 ){
      //     cbCh->setCurrentIndex(oldCh);
      //   }else{
      //     cbCh->setCurrentIndex(0);
      //   }
      // }

      cbCh->setCurrentIndex(oldChComboBoxindex[index]);
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
  }

  //set default oldChComboBoxindex
  for( unsigned int i = 0; i < nDigi; i++ ) oldChComboBoxindex[i] = 0;
  oldBd = 0;

  layout->setStretch(0, 1);
  layout->setStretch(1, 6);

  ClearInternalDataCount();


  workerThread = new QThread(this);
  histWorker = new HistWorker(this);
  timer = new QTimer(this);

  histWorker->moveToThread(workerThread);

  // this is another way
  // timer = new QTimer();
  // timer->moveToThread(workerThread);
  // connect(this, &SingleSpectra::startWorkerTimer, timer, static_cast<void(QTimer::*)(int)>(&QTimer::start));
  // connect(this, &SingleSpectra::stopWorkerTimer, timer, &QTimer::stop);

  isFillingHistograms = false;
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
  int oldCh = oldChComboBoxindex[oldBd] == 0 ? digi[oldBd]->GetNumInputCh() : oldChComboBoxindex[oldBd] - 1;

  if( oldChComboBoxindex[oldBd] > 0 ){
    histLayout->removeWidget(hist[oldBd][oldCh]);
    histVisibility[oldBd][oldCh] = false;
    hist[oldBd][oldCh]->setParent(nullptr);
  }else{
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
  oldChComboBoxindex[bd] = cbCh->currentIndex();

}

void SingleSpectra::FillHistograms(){

  if( !this->isVisible() ) return;
  if( chkIsFillHistogram->checkState() == Qt::Unchecked ) return;
  if( isFillingHistograms ) return;

  isFillingHistograms = true;

  printf("####################### SingleSpectra::%s\n", __func__);

  timespec ta, tb;
  clock_gettime(CLOCK_REALTIME, &ta);

  // ---- max-heap: most-backlogged channel first
  struct ChannelBacklog {
    int  backlog;   // events queued for this cycle (capped at MaxHistFillPerChannel)
    int  digiIdx;
    int  chIdx;
    long trueBacklog; // actual pending events, for the fill report
    bool operator<(const ChannelBacklog& o) const { return backlog < o.backlog; }
  };
  std::priority_queue<ChannelBacklog> pq;

  for( int ID = 0; ID < (int)nDigi; ID++){
    for( int ch = 0; ch < digi[ID]->GetNumInputCh(); ch++){
      long tail = digi[ID]->GetData()->GetAbsDataIndex(ch);
      long& last = lastFilledIndex[ID][ch];

      if( tail <= last ) continue;

      // clamp if ring buffer lapped the fill cursor
      long dataSize = digi[ID]->GetData()->GetDataSize();
      if( tail - last > dataSize ) last = tail - dataSize;

      long trueBacklog = tail - last;
      int  capped      = (int)std::min((long)MaxHistFillPerChannel, trueBacklog);
      pq.push({capped, ID, ch, trueBacklog});
    }
  }

  if( pq.empty() ){
    isFillingHistograms = false;
    return;
  }

  // ---- drain heap within time budget
  while( isFillingHistograms && !pq.empty() ){
    ChannelBacklog top = pq.top(); pq.pop();
    int  ID   = top.digiIdx;
    int  ch   = top.chIdx;
    long tail = digi[ID]->GetData()->GetAbsDataIndex(ch);

    lastFilledIndex[ID][ch]++;
    if( lastFilledIndex[ID][ch] > tail ){
      // cursor overtook tail (data stopped); done with this channel this cycle
      continue;
    }

    uShort energy = digi[ID]->GetData()->GetEnergy(ch, lastFilledIndex[ID][ch]);
    hist[ID][ch]->Fill(energy);
    if( digi[ID]->GetDPPType() == DPPTypeCode::DPP_PSD_CODE ){
      uShort e2 = digi[ID]->GetData()->GetEnergy2(ch, lastFilledIndex[ID][ch]);
      hist[ID][ch]->Fill(e2, 1);
    }
    hist2D[ID]->Fill(ch, energy);

    // re-insert with decremented backlog if this channel still has quota
    if( top.backlog - 1 > 0 ) pq.push({top.backlog - 1, ID, ch, top.trueBacklog});

    clock_gettime(CLOCK_REALTIME, &tb);
    if( (tb.tv_nsec - ta.tv_nsec)/1e6 + (tb.tv_sec - ta.tv_sec)*1e3 >= maxFillTimeinMilliSec ) break;
  }

  clock_gettime(CLOCK_REALTIME, &tb);
  printf("total time : %8.3f ms\n", (tb.tv_nsec - ta.tv_nsec)/1e6 + (tb.tv_sec - ta.tv_sec)*1e3);

  isFillingHistograms = false;
}

void SingleSpectra::ReplotHistograms(){

  // qDebug() << __func__ << "| thread:" << QThread::currentThreadId();

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

