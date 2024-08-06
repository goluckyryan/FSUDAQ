#include "Scope.h"

#include <QApplication>
#include <QValueAxis>
#include <QRandomGenerator>
#include <QGroupBox>
#include <QStandardItemModel>
#include <QLabel>
// #include <QScreen>

QVector<QPointF> Scope::TrapezoidFilter(QVector<QPointF> data, int baseLineEndS, int riseTimeS, int flatTopS, float decayTime_ns){
  DebugPrint("%s", "Scope");
  QVector<QPointF>  trapezoid;
  trapezoid.clear();
  
  ///find baseline;
  double baseline = 0;
  for( int i = 0; i < baseLineEndS; i++){
    baseline += data[i].y();
  }
  baseline = baseline*1./baseLineEndS;
  
  int length = data.size();
  
  double pn = 0.;
  double sn = 0.;
  for( int i = 0; i < length ; i++){
  
    double dlk = data[i].y() - baseline;
    if( i - riseTimeS >= 0            ) dlk -= data[i - riseTimeS].y()             - baseline;
    if( i - flatTopS - riseTimeS >= 0  ) dlk -= data[i - flatTopS - riseTimeS].y()   - baseline;
    if( i - flatTopS - 2*riseTimeS >= 0) dlk += data[i - flatTopS - 2*riseTimeS].y() - baseline;
    
    if( i == 0 ){
        pn = dlk;
        sn = pn + dlk*decayTime_ns;
    }else{
        pn = pn + dlk;
        sn = sn + pn + dlk*decayTime_ns;
    }    
    
    trapezoid.append(QPointF(data[i].x(), sn / decayTime_ns / riseTimeS));
  
  }
  
  return trapezoid;
}


Scope::Scope(Digitizer ** digi, unsigned int nDigi, ReadDataThread ** readDataThread, QMainWindow * parent) : QMainWindow(parent){
  DebugPrint("%s", "Scope");
  this->digi = digi;
  this->nDigi = nDigi;
  this->readDataThread = readDataThread;

  setWindowTitle("Scope");
  setWindowFlags( this->windowFlags() & ~Qt::WindowCloseButtonHint );

  //====== resize window if screen too small
  QScreen * screen = QGuiApplication::primaryScreen();
  QRect screenGeo = screen->geometry();
  if( screenGeo.width() < 1000 || screenGeo.height() < 800) {
    setGeometry(0, 0, screenGeo.width() - 100, screenGeo.height() - 100);
  }else{
    setGeometry(0, 0, 1000, 800);
  }
  // setGeometry(0, 0, 1000, 800);

  enableSignalSlot = false;

  isACQStarted = false;

  plot = new RChart();
  for( int i = 0; i < MaxNumberOfTrace; i++) {
    dataTrace[i] = new QLineSeries();
    dataTrace[i]->setName("Trace " + QString::number(i));
    for(int j = 0; j < 100; j ++) dataTrace[i]->append(40*j, QRandomGenerator::global()->bounded(8000) - 4000);
    plot->addSeries(dataTrace[i]);
  }

  dataTrace[0]->setPen(QPen(Qt::red, 2));
  dataTrace[1]->setPen(QPen(Qt::blue, 2));
  dataTrace[2]->setPen(QPen(Qt::darkYellow, 1));
  dataTrace[3]->setPen(QPen(Qt::darkGreen, 1));
  dataTrace[4]->setPen(QPen(Qt::darkMagenta, 1));

  plot->setAnimationDuration(1); // msec
  plot->setAnimationOptions(QChart::NoAnimation);
  plot->createDefaultAxes(); /// this must be after addSeries();
  /// this must be after createDefaultAxes();
  QValueAxis * yaxis = qobject_cast<QValueAxis*> (plot->axes(Qt::Vertical).first());
  QValueAxis * xaxis = qobject_cast<QValueAxis*> (plot->axes(Qt::Horizontal).first());
  yaxis->setTickCount(7);
  yaxis->setTickInterval((0x1FFF)/4);
  yaxis->setRange(-(0x3FFF), 0x3FFF);
  yaxis->setLabelFormat("%.0f");

  xaxis->setRange(0, 4000);
  xaxis->setTickCount(11);
  xaxis->setLabelFormat("%.0f");
  xaxis->setTitleText("Time [ns]");

  updateTraceThread = new TimingThread();
  updateTraceThread->SetWaitTimeinSec(ScopeUpdateMiliSec / 1000.);
  connect(updateTraceThread, &TimingThread::timeUp, this, &Scope::UpdateScope);

  updateScalarThread = new TimingThread();
  updateScalarThread->SetWaitTimeinSec(2);
  connect(updateScalarThread, &TimingThread::timeUp, this, [=](){
    emit UpdateScaler();
  });

  NullThePointers();

  //*================================== UI
  int rowID = -1;

  QWidget * layoutWidget = new QWidget(this);
  setCentralWidget(layoutWidget);
  QGridLayout * layout = new QGridLayout(layoutWidget);
  layoutWidget->setLayout(layout);

  //================ Board & Ch selection
  rowID ++;
  cbScopeDigi = new RComboBox(this);
  cbScopeCh = new RComboBox(this);
  layout->addWidget(cbScopeDigi, rowID, 0);
  layout->addWidget(cbScopeCh, rowID, 1);

  for(unsigned int i = 0; i < nDigi; i++){
    cbScopeDigi->addItem("Digi-" +  QString::number(digi[i]->GetSerialNumber()), i);
  }

  ID = 0;
  cbScopeDigi->setCurrentIndex(0);
  for( int i = 0; i < digi[0]->GetNumInputCh(); i++) cbScopeCh->addItem("Ch-" + QString::number(i));
  tick2ns = digi[ID]->GetTick2ns();
  factor = digi[ID]->IsDualTrace_PHA() ? 2 : 1;

  connect(cbScopeDigi, &RComboBox::currentIndexChanged, this, [=](int index){
    if( !enableSignalSlot ) return;

    bool saveACQStartStatus = isACQStarted;
    if( isACQStarted) StopScope();

    ID = index;
    tick2ns = digi[ID]->GetTick2ns();
    factor = digi[ID]->IsDualTrace_PHA() ? 2 : 1;

    enableSignalSlot = false;
    //---setup cbScopeCh
    cbScopeCh->clear();
    for( int i = 0; i < digi[ID]->GetNumInputCh(); i++) cbScopeCh->addItem("Ch-" + QString::number(i));

    //---Setup SettingGroup
    CleanUpSettingsGroupBox();
    if( digi[ID]->GetDPPType() == V1730_DPP_PHA_CODE ) SetUpPanel_PHA();
    if( digi[ID]->GetDPPType() == V1730_DPP_PSD_CODE ) SetUpPanel_PSD();
    if( digi[ID]->GetDPPType() == V1740_DPP_QDC_CODE ) SetUpPanel_QDC();

    ReadSettingsFromBoard();

    if( saveACQStartStatus )StartScope();

  });

  connect(cbScopeCh, &RComboBox::currentIndexChanged, this, [=](){
    if( !enableSignalSlot ) return;

    bool saveACQStartStatus = isACQStarted;
    if( isACQStarted) StopScope();

    ReadSettingsFromBoard();

    if( saveACQStartStatus )StartScope();

  });

  bnReadSettingsFromBoard = new QPushButton("Refresh Settings", this);
  layout->addWidget(bnReadSettingsFromBoard, rowID, 2);
  connect(bnReadSettingsFromBoard, &QPushButton::clicked, this, &Scope::ReadSettingsFromBoard);

  QPushButton * bnClearMemory = new QPushButton("Clear Memory", this);
  layout->addWidget(bnClearMemory, rowID, 3);
  connect(bnClearMemory, &QPushButton::clicked, this, [=](){
    digiMTX[ID].lock();
    digi[ID]->GetData()->ClearData();
    digiMTX[ID].unlock();
  });

  QPushButton * bnClearBuffer = new QPushButton("Clear Buffer/FIFO", this);
  layout->addWidget(bnClearBuffer, rowID, 4);
  connect(bnClearBuffer, &QPushButton::clicked, this, [=](){ digi[ID]->WriteRegister(DPP::SoftwareClear_W, 1);});

  QLabel * lbRun = new QLabel("Run Status : ", this);
  lbRun->setAlignment(Qt::AlignCenter | Qt::AlignRight);
  layout->addWidget(lbRun, rowID, 5);

  runStatus = new QPushButton("", this);
  runStatus->setEnabled(false);
  runStatus->setFixedSize(QSize(20,20));
  layout->addWidget(runStatus, rowID, 6);

  //================ Trace settings
  rowID ++;
  {
    settingGroup = new QGroupBox("Trace Settings",this);
    layout->addWidget(settingGroup, rowID, 0, 1, 7);

    settingLayout = new QGridLayout(settingGroup);
    settingLayout->setSpacing(0);

    if( digi[ID]->GetDPPType() == V1730_DPP_PHA_CODE ) SetUpPanel_PHA();
    if( digi[ID]->GetDPPType() == V1730_DPP_PSD_CODE ) SetUpPanel_PSD();
    if( digi[ID]->GetDPPType() == V1740_DPP_QDC_CODE ) SetUpPanel_QDC();

  }
  //================ Plot view
  rowID ++;
  plotView = new RChartView(plot, this);
  layout->addWidget(plotView, rowID, 0, 1, 7);

  
  //================ Key binding
  rowID ++;
  QLabel * lbhints = new QLabel("Type 'r' to restore view, '+/-' Zoom in/out, arrow key to pan.", this);
  layout->addWidget(lbhints, rowID, 0, 1, 4);
  
  QLabel * lbinfo = new QLabel("Trace updates every " + QString::number(updateTraceThread->GetWaitTimeinSec()) + " sec.", this);
  lbinfo->setAlignment(Qt::AlignRight);
  layout->addWidget(lbinfo, rowID, 6);

  rowID ++;
  //TODO =========== Trace step
  QLabel * lbinfo2 = new QLabel("Maximum time range is " + QString::number(MaxDisplayTraceTimeLength) + " ns due to processing speed.", this);
  layout->addWidget(lbinfo2, rowID, 0, 1, 5);

  //================ close button
  rowID ++;
  bnScopeStart = new QPushButton("Start", this);
  layout->addWidget(bnScopeStart, rowID, 0);
  connect(bnScopeStart, &QPushButton::clicked, this, [=](){this->StartScope();});

  chkSoleRun = new QCheckBox("Only this channel", this);
  layout->addWidget(chkSoleRun, rowID, 1);

  bnScopeStop = new QPushButton("Stop", this);
  layout->addWidget(bnScopeStop, rowID, 2);
  connect(bnScopeStop, &QPushButton::clicked, this, &Scope::StopScope);

  QLabel * lbTriggerRate = new QLabel("Trigger Rate [Hz] : ", this);
  lbTriggerRate->setAlignment(Qt::AlignCenter | Qt::AlignRight);
  layout->addWidget(lbTriggerRate, rowID, 3);

  leTriggerRate = new QLineEdit(this);
  leTriggerRate->setAlignment(Qt::AlignRight);
  leTriggerRate->setReadOnly(true);
  layout->addWidget(leTriggerRate, rowID, 4);

  QPushButton * bnClose = new QPushButton("Close", this);
  layout->addWidget(bnClose, rowID, 6);
  connect(bnClose, &QPushButton::clicked, this, &Scope::close);

  layout->setColumnStretch(0, 1);
  layout->setColumnStretch(1, 1);
  layout->setColumnStretch(2, 1);
  layout->setColumnStretch(3, 1);
  layout->setColumnStretch(4, 1);
  layout->setColumnStretch(5, 1);

  bnScopeStart->setEnabled(true);
  bnScopeStart->setStyleSheet("background-color: green;");
  bnScopeStop->setEnabled(false);
  bnScopeStop->setStyleSheet("");

  UpdatePanelFromMomeory();

  if( digi[ID]->GetDPPType() == V1730_DPP_PHA_CODE ) {
    QValueAxis * yaxis = qobject_cast<QValueAxis*> (plot->axes(Qt::Vertical).first());
    yaxis->setRange(-(0x1FFF), 0x1FFF);
  }
  if( digi[ID]->GetDPPType() == V1730_DPP_PSD_CODE ) {
    QValueAxis * yaxis = qobject_cast<QValueAxis*> (plot->axes(Qt::Vertical).first());
    yaxis->setRange(0, 0x3FFF);
  }
  if( digi[ID]->GetDPPType() == V1740_DPP_QDC_CODE ) {
    QValueAxis * yaxis = qobject_cast<QValueAxis*> (plot->axes(Qt::Vertical).first());
    yaxis->setRange(0, 0xFFF);
  }

  enableSignalSlot = true;

}


Scope::~Scope(){
  DebugPrint("%s", "Scope");
  updateTraceThread->Stop();
  updateTraceThread->quit();
  updateTraceThread->wait();
  delete updateTraceThread;

  updateScalarThread->Stop();
  updateScalarThread->quit();
  updateScalarThread->wait();
  delete updateScalarThread;

  for( int i = 0; i < MaxNumberOfTrace; i++) delete dataTrace[i];
  delete plot;
}

void Scope::NullThePointers(){
  DebugPrint("%s", "Scope");
  sbReordLength = nullptr;
  sbPreTrigger = nullptr;
  sbDCOffset = nullptr;
  cbDynamicRange = nullptr;
  cbPolarity = nullptr;

  /// PHA
  sbInputRiseTime = nullptr;
  sbTriggerHoldOff = nullptr;
  sbThreshold = nullptr;
  cbSmoothingFactor = nullptr;

  sbTrapRiseTime = nullptr;
  sbTrapFlatTop = nullptr;
  sbDecayTime = nullptr;
  sbPeakingTime = nullptr;
  sbPeakHoldOff = nullptr;
  cbPeakAvg = nullptr;
  cbBaselineAvg = nullptr;

  cbAnaProbe1 = nullptr;
  cbAnaProbe2 = nullptr;
  cbDigiProbe1 = nullptr;
  cbDigiProbe2 = nullptr;

  /// PSD
  sbShortGate = nullptr;
  sbLongGate = nullptr;
  sbGateOffset = nullptr;

}

//*=======================================================
//*=======================================================
void Scope::StartScope(){
  DebugPrint("%s", "Scope");
  if( !digi ) return;

  //TODO set other channel to be no trace;
  emit UpdateOtherPanels();

  if( chkSoleRun->isChecked() ){

    int ID = cbScopeDigi->currentIndex();
    int ch = cbScopeCh->currentIndex();
    oldDigi = ID;
    oldCh = ch;

    //save present settings, channleMap, trigger condition
    traceOn[ID] = digi[ID]->IsRecordTrace();
    digi[ID]->SetBits(DPP::BoardConfiguration, DPP::Bit_BoardConfig::RecordTrace, 1, -1);
    chMask = digi[ID]->GetSettingFromMemory(DPP::RegChannelEnableMask);

    if( digi[ID]->GetDPPType() == DPPTypeCode::DPP_PHA_CODE ){
      dppAlg  = digi[ID]->GetSettingFromMemory(DPP::DPPAlgorithmControl, ch);
      dppAlg2 = digi[ID]->GetSettingFromMemory(DPP::PHA::DPPAlgorithmControl2_G, ch);

      digi[ID]->SetBits(DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl_PHA::TriggerMode, 0, ch);
      digi[ID]->SetBits(DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl_PHA::DisableSelfTrigger, 0, ch);

      digi[ID]->SetBits(DPP::PHA::DPPAlgorithmControl2_G, DPP::PHA::Bit_DPPAlgorithmControl2::LocalShapeTriggerMode, 0, ch);
      digi[ID]->SetBits(DPP::PHA::DPPAlgorithmControl2_G, DPP::PHA::Bit_DPPAlgorithmControl2::LocalTrigValidMode, 0, ch);

    }

    if( digi[ID]->GetDPPType() == DPPTypeCode::DPP_PSD_CODE ){
      dppAlg  = digi[ID]->GetSettingFromMemory(DPP::DPPAlgorithmControl, ch);
      dppAlg2 = digi[ID]->GetSettingFromMemory(DPP::PSD::DPPAlgorithmControl2_G, ch);
      
      digi[ID]->SetBits(DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl_PHA::TriggerMode, 0, ch);
      digi[ID]->SetBits(DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl_PHA::DisableSelfTrigger, 0, ch);

      digi[ID]->SetBits(DPP::PSD::DPPAlgorithmControl2_G, DPP::PSD::Bit_DPPAlgorithmControl2::LocalShapeTriggerMode, 0, ch);
      digi[ID]->SetBits(DPP::PSD::DPPAlgorithmControl2_G, DPP::PSD::Bit_DPPAlgorithmControl2::LocalTrigValidMode, 0, ch);
    }

    if( digi[ID]->GetDPPType() == DPPTypeCode::DPP_QDC_CODE ){
      dppAlg  = digi[ID]->GetSettingFromMemory(DPP::QDC::DPPAlgorithmControl, ch);
      digi[ID]->SetBits(DPP::QDC::DPPAlgorithmControl, DPP::QDC::Bit_DPPAlgorithmControl::TriggerMode, 0, ch); //set self-triiger
    }

    digi[ID]->WriteRegister(DPP::RegChannelEnableMask, (1 << ch));

    //=========== start 
    digi[ID]->WriteRegister(DPP::SoftwareClear_W, 1);

    readDataThread[ID]->SetScopeMode(true);
    readDataThread[ID]->SetSaveData(false);

    digi[ID]->StartACQ();
    readDataThread[ID]->start();

  }else{

    for( int iDigi = (int)nDigi-1 ; iDigi >= 0; iDigi --){

      traceOn[iDigi] = digi[iDigi]->IsRecordTrace(); //remember setting
      SendLogMsg("Digi-" + QString::number(digi[iDigi]->GetSerialNumber()) + " is starting ACQ." );
      digi[iDigi]->WriteRegister(DPP::SoftwareClear_W, 1);
      digi[iDigi]->SetBits(DPP::BoardConfiguration, DPP::Bit_BoardConfig::RecordTrace, 1, -1);

      //AggPerRead[iDigi] = digi[iDigi]->GetSettingFromMemory(DPP::MaxAggregatePerBlockTransfer);
      //SendLogMsg("Set Agg/Read to 1 for scope, it was " + QString::number(AggPerRead[iDigi]) + ".");
      //digi[iDigi]->WriteRegister(DPP::MaxAggregatePerBlockTransfer, 1);

      readDataThread[iDigi]->SetScopeMode(true);
      readDataThread[iDigi]->SetSaveData(false);

      digi[iDigi]->StartACQ();

  //    printf("----- readDataThread running ? %d.\n", readDataThread[iDigi]->isRunning());
      // if( readDataThread[iDigi]->isRunning() ){
      //   readDataThread[iDigi]->quit();
      //   readDataThread[iDigi]->wait();
      // }
      readDataThread[iDigi]->start();
  //    printf("----- readDataThread running ? %d.\n", readDataThread[iDigi]->isRunning());
    }

  }

  updateTraceThread->start();
  updateScalarThread->start();

  bnScopeStart->setEnabled(false);
  bnScopeStart->setStyleSheet("");
  bnScopeStop->setEnabled(true);
  bnScopeStop->setStyleSheet("background-color: red;");

  chkSoleRun->setEnabled(false);

  EnableControl(false);

  TellACQOnOff(true);

  isACQStarted = true;

}

void Scope::StopScope(){
  DebugPrint("%s", "Scope");
  if( !digi ) return;

  // printf("------ Scope::%s \n", __func__);
  updateTraceThread->Stop();
  updateTraceThread->quit();
  updateTraceThread->exit();

  updateScalarThread->Stop();
  updateScalarThread->quit();
  updateScalarThread->exit();

  if( chkSoleRun->isChecked() ){

    //int ID = cbScopeDigi->currentIndex();
    int ID = oldDigi;

    if( readDataThread[ID]->isRunning() ){
      readDataThread[ID]->Stop();
      readDataThread[ID]->quit();
      readDataThread[ID]->wait();
      readDataThread[ID]->SetScopeMode(false);
    }

    digiMTX[ID].lock();
    digi[ID]->StopACQ();
    digi[ID]->ReadACQStatus();
    digiMTX[ID].unlock();

    //restore setting
    digi[ID]->SetBits(DPP::BoardConfiguration, DPP::Bit_BoardConfig::RecordTrace, traceOn[ID], -1);
    digi[ID]->WriteRegister(DPP::RegChannelEnableMask, chMask);

    if( digi[ID]->GetDPPType() == DPPTypeCode::DPP_PHA_CODE ){
      digi[ID]->WriteRegister(DPP::DPPAlgorithmControl, dppAlg, oldCh);
      digi[ID]->WriteRegister(DPP::PHA::DPPAlgorithmControl2_G, dppAlg2, oldCh);
    }

    if( digi[ID]->GetDPPType() == DPPTypeCode::DPP_PSD_CODE ){
      digi[ID]->WriteRegister(DPP::DPPAlgorithmControl, dppAlg, oldCh);
      digi[ID]->WriteRegister(DPP::PSD::DPPAlgorithmControl2_G, dppAlg2, oldCh);
    }

    if( digi[ID]->GetDPPType() == DPPTypeCode::DPP_QDC_CODE ){
      digi[ID]->WriteRegister(DPP::QDC::DPPAlgorithmControl, dppAlg, oldCh);
    }

  }else{

    for( unsigned int iDigi = 0; iDigi < nDigi; iDigi ++){

      if( readDataThread[iDigi]->isRunning() ){
        readDataThread[iDigi]->Stop();
        readDataThread[iDigi]->quit();
        readDataThread[iDigi]->wait();
        readDataThread[iDigi]->SetScopeMode(false);
      }
      digiMTX[iDigi].lock();
      digi[iDigi]->StopACQ();
      digi[iDigi]->ReadACQStatus();
      //digi[iDigi]->GetData()->PrintAllData();
      digiMTX[iDigi].unlock();

      digi[iDigi]->SetBits(DPP::BoardConfiguration, DPP::Bit_BoardConfig::RecordTrace, traceOn[iDigi], -1);
      //digi[iDigi]->WriteRegister(DPP::MaxAggregatePerBlockTransfer, AggPerRead[iDigi]);

    }

  }

  emit UpdateOtherPanels();

  bnScopeStart->setEnabled(true);
  bnScopeStart->setStyleSheet("background-color: green;");
  bnScopeStop->setEnabled(false);
  bnScopeStop->setStyleSheet("");

  chkSoleRun->setEnabled(true);
  runStatus->setStyleSheet("");

  EnableControl(true);

  TellACQOnOff(false);

  isACQStarted = false;

  // printf("----- end of %s\n", __func__);

}

void Scope::UpdateScope(){
  DebugPrint("%s", "Scope");
  if( !digi ) return;

  int ch = cbScopeCh->currentIndex();

  if( digi[ID]->GetInputChannelOnOff(ch) == false) return;

  //printf("### %d %d \n", ch, digi[ID]->GetData()->DataIndex[ch]);

  uint32_t acqStatus = digi[ID]->GetACQStatusFromMemory();
  if( ( acqStatus >> 2 ) & 0x1 ){
    runStatus->setStyleSheet("background-color : green;");
  }else{
    runStatus->setStyleSheet("");
  }

  factor = digi[ID]->IsDualTrace_PHA() ? 2 : 1;

  Data * data = digi[ID]->GetData();
  int index = data->GetDataIndex(ch);
  int traceLength = data->Waveform1[ch][index].size();
  if( digi[ID]->GetDPPType() == V1730_DPP_PSD_CODE ) traceLength =  data->DigiWaveform1[ch][index].size();

  if( index < 0 || data->TriggerRate[ch] == 0){
    leTriggerRate->setStyleSheet("font-weight : bold; color : red;");
    leTriggerRate->setText("No Trigger");
  }else{
    leTriggerRate->setStyleSheet("");
    leTriggerRate->setText(QString::number(data->TriggerRate[ch]));
  }

  if( traceLength * tick2ns * factor > MaxDisplayTraceTimeLength) traceLength = MaxDisplayTraceTimeLength / tick2ns/ factor;

  //printf("--- %s| %d, %d, %d | %ld(%d) | %d, %d | %d\n", __func__, ch, data->GetLoopIndex(ch), index, data->Waveform1[ch][index].size(), traceLength, factor, tick2ns, traceLength * tick2ns * factor );
  if( index > 0 ){

    QVector<QPointF> points[5];
    if( digi[ID]->GetDPPType() == V1730_DPP_PHA_CODE ) {
      if( dataTrace[4]->count() > 0 ) dataTrace[4]->clear();
      for( int i = 0; i < traceLength ; i++ ) {
        points[0].append(QPointF(tick2ns * i * factor, (data->Waveform1[ch][index])[i])); 
        if( i < (int) data->Waveform2[ch][index].size() )      points[1].append(QPointF(tick2ns * i * factor, (data->Waveform2[ch][index])[i]));
        if( i < (int) data->DigiWaveform1[ch][index].size() )  points[2].append(QPointF(tick2ns * i * factor, (data->DigiWaveform1[ch][index])[i] * 1000));
        if( i < (int) data->DigiWaveform2[ch][index].size() )  points[3].append(QPointF(tick2ns * i * factor, (data->DigiWaveform2[ch][index])[i] * 1000 + 500));
      }
      dataTrace[0]->replace(points[0]);
      dataTrace[1]->replace(points[1]);
      dataTrace[2]->replace(points[2]);
      dataTrace[3]->replace(points[3]);
    }

    if( digi[ID]->GetDPPType() == V1730_DPP_PSD_CODE ) {
      if( dataTrace[4]->count() > 0 ) dataTrace[4]->clear();
      for( int i = 0; i < traceLength ; i++ ) {
        points[0].append(QPointF(tick2ns * i * factor, (data->Waveform1[ch][index])[i])); 
        if( i < (int) data->Waveform2[ch][index].size() )      points[1].append(QPointF(tick2ns * i * factor, (data->Waveform2[ch][index])[i]));
        if( i < (int) data->DigiWaveform1[ch][index].size() )  points[2].append(QPointF(tick2ns * i * factor, (data->DigiWaveform1[ch][index])[i] * 1000));
        if( i < (int) data->DigiWaveform2[ch][index].size() )  points[3].append(QPointF(tick2ns * i * factor, (data->DigiWaveform2[ch][index])[i] * 1000 + 500));
      }
      dataTrace[0]->replace(points[0]);
      dataTrace[1]->replace(points[1]);
      dataTrace[2]->replace(points[2]);
      dataTrace[3]->replace(points[3]);
    }

    if( digi[ID]->GetDPPType() == V1740_DPP_QDC_CODE ) {

      for( int i = 0; i < traceLength ; i++ ) {
        points[0].append(QPointF(tick2ns * i, (data->Waveform1[ch][index])[i])); 
        if( i < (int) data->DigiWaveform1[ch][index].size() )  points[1].append(QPointF(tick2ns * i,  (data->DigiWaveform1[ch][index])[i] * 1000));
        if( i < (int) data->DigiWaveform2[ch][index].size() )  points[2].append(QPointF(tick2ns * i,  (data->DigiWaveform2[ch][index])[i] * 1000 + 500));
        if( i < (int) data->DigiWaveform3[ch][index].size() )  points[3].append(QPointF(tick2ns * i,  (data->DigiWaveform3[ch][index])[i] * 1000 + 1000));
        if( i < (int) data->DigiWaveform4[ch][index].size() )  points[4].append(QPointF(tick2ns * i,  (data->DigiWaveform4[ch][index])[i] * 1000 + 1500));
      }
      dataTrace[0]->replace(points[0]);
      dataTrace[1]->replace(points[1]);
      dataTrace[2]->replace(points[2]);
      dataTrace[3]->replace(points[3]);
      dataTrace[4]->replace(points[4]);
    }
  }
  //data->ClearTriggerRate();
  //digiMTX[ID].unlock();

  // if( data->TriggerRate[ch] == 0 ){
  //     dataTrace[0]->clear();
  //     dataTrace[1]->clear();
  //     dataTrace[2]->clear();
  //     dataTrace[3]->clear();
  //     dataTrace[4]->clear();
  // }

  plot->axes(Qt::Horizontal).first()->setRange(0, tick2ns * traceLength * factor);

  QCoreApplication::processEvents();

}

//*=======================================================
//*=======================================================
void Scope::SetUpComboBoxSimple(RComboBox * &cb, QString str, int row, int col){
  DebugPrint("%s", "Scope");
  QLabel * lb = new QLabel(str, settingGroup);
  lb->setAlignment(Qt::AlignRight | Qt::AlignCenter);
  settingLayout->addWidget(lb, row, col);

  cb = new RComboBox(settingGroup);
  settingLayout->addWidget(cb, row, col + 1);
  
}

void Scope::SetUpComboBox(RComboBox * &cb, QString str, int row, int col, const Reg para){
  DebugPrint("%s", "Scope");
  SetUpComboBoxSimple(cb, str, row, col);

  for( int i = 0; i < (int) para.GetComboList().size(); i++){
    cb->addItem(QString::fromStdString(para.GetComboList()[i].first), para.GetComboList()[i].second);
  }

  connect(cb, &RComboBox::currentIndexChanged, this , [=](){
    if( ! enableSignalSlot ) return;

    int ch = cbScopeCh->currentIndex();
    int value = cb->currentData().toInt();

    if( digi[ID]->GetDPPType() == DPPTypeCode::DPP_QDC_CODE ) {
      int grp = ch/8; // convert ch to grp 

      digiMTX[ID].lock();
      digi[ID]->WriteRegister(para, value, grp);
      digiMTX[ID].unlock();

    }else{

      digiMTX[ID].lock();
      digi[ID]->WriteRegister(para, value, ch);
      digiMTX[ID].unlock();

    }

    QString msg;
    msg = QString::fromStdString(para.GetName()) ;
    msg += "|DIG:"+ QString::number(digi[ID]->GetSerialNumber()) + ",CH:" + (ch == -1 ? "All" : QString::number(ch));
    msg += " = " + cb->currentText();
    if( digi[ID]->GetErrorCode() == CAEN_DGTZ_Success ){
      SendLogMsg(msg + " | OK.");
      cb->setStyleSheet("");
      emit UpdateOtherPanels();
    }else{
      SendLogMsg(msg + " | Fail.");
      cb->setStyleSheet("color:red;");
    }

  });

}

void Scope::SetUpSpinBox(RSpinBox * &sb, QString str, int row, int col, const Reg para){
  DebugPrint("%s", "Scope");
  QLabel * lb = new QLabel(str, settingGroup);
  lb->setAlignment(Qt::AlignRight | Qt::AlignCenter);
  settingLayout->addWidget(lb, row, col);

  sb = new RSpinBox(settingGroup);
  if( para.GetPartialStep() != 0 ){
    sb->setMinimum(0);
    sb->setMaximum(para.GetMaxBit() * para.GetPartialStep() * tick2ns);
    if( para.GetPartialStep() > 0 ) sb->setSingleStep(para.GetPartialStep() * tick2ns);
    if( para.GetPartialStep() == -1 ) sb->setSingleStep(1);
  }
  settingLayout->addWidget(sb, row, col + 1);

  connect(sb, &RSpinBox::valueChanged, this, [=](){
    if( !enableSignalSlot ) return;
    sb->setStyleSheet("color:blue");
  });


  connect(sb, &RSpinBox::returnPressed, this, [=](){
    if( !enableSignalSlot ) return;
    if( sb->decimals() == 0 && sb->singleStep() != 1) {
      double step = sb->singleStep();
      double value = sb->value();
      sb->setValue( (std::round(value/step)*step));
    }

    int ch = cbScopeCh->currentIndex();
    //if( chkSetAllChannel->isChecked() ) ch = -1;
    QString msg;
    msg = QString::fromStdString(para.GetName()) + "|DIG:"+ QString::number(digi[ID]->GetSerialNumber()) + ",CH:" + (ch == -1 ? "All" : QString::number(ch));
    msg += " = " + QString::number(sb->value());

    uint32_t value = sb->value() / tick2ns / abs(para.GetPartialStep());

    //todo NEED TO CHECK
    // if( para == DPP::RecordLength_G){
    //   int factor = digi[ID]->IsDualTrace_PHA() ? 2 : 1;
    //   value = value * factor;
    // }

    if( para == DPP::ChannelDCOffset || para == DPP::QDC::DCOffset){
      value = uint16_t((1.0 - sb->value()/100.) * 0xFFFF);
    }

    if( para == DPP::PHA::TriggerThreshold ){
      value = sb->value();
    }

    msg += " | 0x" + QString::number(value, 16); 

    if( digi[ID]->GetDPPType() == DPPTypeCode::DPP_QDC_CODE ){
      int grp = ch/8; // convert ch to grp 
      digiMTX[ID].lock();
        digi[ID]->WriteRegister(para, value, grp);
      digiMTX[ID].unlock();
    }else{
      digiMTX[ID].lock();
      digi[ID]->WriteRegister(para, value, ch);
      digiMTX[ID].unlock();
    }

    if( digi[ID]->GetErrorCode() == CAEN_DGTZ_Success ){
      SendLogMsg(msg + " | OK.");
      sb->setStyleSheet("");
      emit UpdateOtherPanels();
    }else{
      SendLogMsg(msg + " | Fail.");
      sb->setStyleSheet("color:red;");
    }
  });  
}

void Scope::CleanUpSettingsGroupBox(){
  DebugPrint("%s", "Scope");
  QList<QLabel *> labelChildren1 = settingGroup->findChildren<QLabel *>();
  for( int i = 0; i < labelChildren1.size(); i++) delete labelChildren1[i];
  
  QList<RComboBox *> labelChildren2 = settingGroup->findChildren<RComboBox *>();
  for( int i = 0; i < labelChildren2.size(); i++) delete labelChildren2[i];

  QList<RSpinBox *> labelChildren3 = settingGroup->findChildren<RSpinBox *>();
  for( int i = 0; i < labelChildren3.size(); i++) delete labelChildren3[i];

  NullThePointers();

}

void Scope::SetUpPanel_PHA(){
  printf("--- %s \n", __func__);

  enableSignalSlot = false;

  int rowID = 0;
  SetUpSpinBox(sbReordLength,     "Record Length [ns] ", rowID, 0, DPP::RecordLength_G);
  SetUpSpinBox(sbPreTrigger,        "Pre Trigger [ns] ", rowID, 2, DPP::PreTrigger);
  SetUpSpinBox(sbDCOffset,             "DC offset [%] ", rowID, 4, DPP::ChannelDCOffset);
  sbDCOffset->setDecimals(2);    
  SetUpComboBox(cbDynamicRange,        "Dynamic Range ", rowID, 6, DPP::InputDynamicRange);

  rowID ++; //=============================================================
  SetUpSpinBox(sbInputRiseTime, "Input Rise Time [ns] ", rowID, 0, DPP::PHA::InputRiseTime);
  SetUpSpinBox(sbThreshold,          "Threshold [LSB] ", rowID, 2, DPP::PHA::TriggerThreshold);
  SetUpSpinBox(sbTriggerHoldOff,"Trigger HoldOff [ns] ", rowID, 4, DPP::PHA::TriggerHoldOffWidth);
  SetUpComboBox(cbSmoothingFactor,     "Smooth Factor ", rowID, 6, DPP::PHA::RCCR2SmoothingFactor);
  
  rowID ++; //=============================================================
  SetUpSpinBox(sbTrapRiseTime,  "Trap. Rise Time [ns] ", rowID, 0, DPP::PHA::TrapezoidRiseTime);
  SetUpSpinBox(sbTrapFlatTop,     "Trap. FlatTop [ns] ", rowID, 2, DPP::PHA::TrapezoidFlatTop);
  SetUpSpinBox(sbDecayTime,          "Decay Time [ns] ", rowID, 4, DPP::PHA::DecayTime);
  SetUpSpinBox(sbPeakingTime,      "Peaking Time [ns] ", rowID, 6, DPP::PHA::PeakingTime);

  rowID ++; //=============================================================
  SetUpComboBoxSimple(cbPolarity, "Polarity ", rowID, 0);
  cbPolarity->addItem("Positive", 0);
  cbPolarity->addItem("Negative", 1);
  connect(cbPolarity, &RComboBox::currentIndexChanged, this, [=](){
    if( !enableSignalSlot ) return;
    digi[ID]->SetBits(DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl_PHA::Polarity, cbPolarity->currentData().toInt(), cbScopeCh->currentIndex());
  });

  SetUpComboBoxSimple(cbBaselineAvg, "Baseline Avg. ", rowID, 2);
  cbBaselineAvg->addItem("Not evaluated", 0);
  cbBaselineAvg->addItem("16 sample", 1);
  cbBaselineAvg->addItem("64 sample", 2);
  cbBaselineAvg->addItem("256 sample", 3);
  cbBaselineAvg->addItem("1024 sample", 4);
  cbBaselineAvg->addItem("4096 sample", 5);
  cbBaselineAvg->addItem("16384 sample", 6);
  connect(cbBaselineAvg, &RComboBox::currentIndexChanged, this, [=](){
    if( !enableSignalSlot ) return;
    digi[ID]->SetBits(DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl_PHA::BaselineAvg, cbBaselineAvg->currentData().toInt(), cbScopeCh->currentIndex());
  });

  SetUpComboBoxSimple(cbPeakAvg, "Peak Avg. ", rowID, 4);
  cbPeakAvg->addItem("1 sample", 0);
  cbPeakAvg->addItem("4 sample", 1);
  cbPeakAvg->addItem("16 sample", 2);
  cbPeakAvg->addItem("64 sample", 3);
  connect(cbPeakAvg, &RComboBox::currentIndexChanged, this, [=](){
    if( !enableSignalSlot ) return;
    digi[ID]->SetBits(DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl_PHA::PeakMean, cbPeakAvg->currentData().toInt(), cbScopeCh->currentIndex());
  });

  SetUpSpinBox(sbPeakHoldOff,      "Peak HoldOff [ns] ", rowID, 6, DPP::PHA::PeakHoldOff);


  rowID ++; //=============================================================
  SetUpComboBoxSimple(cbAnaProbe1, "Ana. Probe 1 ", rowID, 0);
  for( int i = 0; i < (int) DPP::Bit_BoardConfig::ListAnaProbe1_PHA.size(); i++){
    cbAnaProbe1->addItem(QString::fromStdString(DPP::Bit_BoardConfig::ListAnaProbe1_PHA[i].first), DPP::Bit_BoardConfig::ListAnaProbe1_PHA[i].second);
  }
  connect(cbAnaProbe1, &RComboBox::currentIndexChanged, this, [=](){
    if( !enableSignalSlot ) return;
    digi[ID]->SetBits(DPP::BoardConfiguration, DPP::Bit_BoardConfig::AnalogProbe1, cbAnaProbe1->currentData().toInt(), cbScopeCh->currentIndex());
    dataTrace[0]->setName(cbAnaProbe1->currentText());
  });

  SetUpComboBoxSimple(cbAnaProbe2, "Ana. Probe 2 ", rowID, 2);
  for( int i = 0; i < (int) DPP::Bit_BoardConfig::ListAnaProbe2_PHA.size(); i++){
    cbAnaProbe2->addItem(QString::fromStdString(DPP::Bit_BoardConfig::ListAnaProbe2_PHA[i].first), DPP::Bit_BoardConfig::ListAnaProbe2_PHA[i].second);
  }
  connect(cbAnaProbe2, &RComboBox::currentIndexChanged, this, [=](){
    if( !enableSignalSlot ) return;
    digi[ID]->SetBits(DPP::BoardConfiguration, DPP::Bit_BoardConfig::AnalogProbe2, cbAnaProbe2->currentData().toInt(), cbScopeCh->currentIndex());
    dataTrace[1]->setName(cbAnaProbe2->currentText());
  });

  SetUpComboBoxSimple(cbDigiProbe1, "Digi. Probe 1 ", rowID, 4);
  for( int i = 0; i < (int) DPP::Bit_BoardConfig::ListDigiProbe1_PHA.size(); i++){
    cbDigiProbe1->addItem(QString::fromStdString(DPP::Bit_BoardConfig::ListDigiProbe1_PHA[i].first), DPP::Bit_BoardConfig::ListDigiProbe1_PHA[i].second);
  }
  connect(cbDigiProbe1, &RComboBox::currentIndexChanged, this, [=](){
    if( !enableSignalSlot ) return;
    digi[ID]->SetBits(DPP::BoardConfiguration, DPP::Bit_BoardConfig::DigiProbel1_PHA, cbDigiProbe1->currentData().toInt(), cbScopeCh->currentIndex());
    dataTrace[2]->setName(cbDigiProbe1->currentText());
  });

  SetUpComboBoxSimple(cbDigiProbe2, "Digi. Probe 2 ", rowID, 6);
  for( int i = 0; i < (int) DPP::Bit_BoardConfig::ListDigiProbe2_PHA.size(); i++){
    cbDigiProbe2->addItem(QString::fromStdString(DPP::Bit_BoardConfig::ListDigiProbe2_PHA[i].first), DPP::Bit_BoardConfig::ListDigiProbe2_PHA[i].second);
  }
  dataTrace[3]->setName(cbDigiProbe2->currentText());
  cbDigiProbe2->setEnabled(false);

  dataTrace[4]->setVisible(false);

  enableSignalSlot = true;

}

void Scope::SetUpPanel_PSD(){

  enableSignalSlot = false;
  printf("==== %s \n", __func__);

  int rowID = 0;
  SetUpSpinBox(sbReordLength,     "Record Length [ns] ", rowID, 0, DPP::RecordLength_G);
  SetUpSpinBox(sbPreTrigger,        "Pre Trigger [ns] ", rowID, 2, DPP::PreTrigger);
  SetUpSpinBox(sbDCOffset,             "DC offset [%] ", rowID, 4, DPP::ChannelDCOffset);
  sbDCOffset->setDecimals(2);    
  SetUpComboBox(cbDynamicRange,        "Dynamic Range ", rowID, 6, DPP::InputDynamicRange);


  rowID ++; //=============================================================
  SetUpComboBoxSimple(cbPolarity, "Polarity ", rowID, 0);
  cbPolarity->addItem("Positive", 0);
  cbPolarity->addItem("Negative", 1);
  connect(cbPolarity, &RComboBox::currentIndexChanged, this, [=](){
    if( !enableSignalSlot ) return;
    digi[ID]->SetBits(DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl_PSD::Polarity, cbPolarity->currentData().toInt(), cbScopeCh->currentIndex());
  });

  SetUpSpinBox(sbShortGate, "Short Gate [ns] ", rowID, 2, DPP::PSD::ShortGateWidth);
  SetUpSpinBox(sbLongGate, "Long Gate [ns] ", rowID, 4, DPP::PSD::LongGateWidth);
  SetUpSpinBox(sbGateOffset, "Gate Offset [ns] ", rowID, 6, DPP::PSD::GateOffset);

  rowID ++; //=============================================================
  SetUpSpinBox(sbThreshold, "Threshold [LSB] ", rowID, 0, DPP::PSD::TriggerThreshold);
  SetUpComboBoxSimple(cbAnaProbe1, "Ana. Probe ", rowID, 2);
  for( int i = 0; i < (int) DPP::Bit_BoardConfig::ListAnaProbe_PSD.size(); i++){
    cbAnaProbe1->addItem(QString::fromStdString(DPP::Bit_BoardConfig::ListAnaProbe_PSD[i].first), DPP::Bit_BoardConfig::ListAnaProbe_PSD[i].second);
  }
  connect(cbAnaProbe1, &RComboBox::currentIndexChanged, this, [=](){
    if( !enableSignalSlot ) return;
    digi[ID]->SetBits(DPP::BoardConfiguration, DPP::Bit_BoardConfig::AnalogProbe1, cbAnaProbe1->currentData().toInt(), cbScopeCh->currentIndex());
    //dataTrace[0]->setName(cbAnaProbe1->currentText());

    switch( cbAnaProbe1->currentIndex() ){
      case 0 : dataTrace[0]->setName("input"); dataTrace[1]->setName("N/A"); break;
      case 1 : dataTrace[0]->setName("CFD"); dataTrace[1]->setName("N/A"); break;
      case 2 : dataTrace[0]->setName("baseline"); dataTrace[1]->setName("input"); break;
      case 3 : dataTrace[0]->setName("baseline"); dataTrace[1]->setName("CFD"); break;
      case 4 : dataTrace[0]->setName("CFD"); dataTrace[1]->setName("input"); break;
    }

  });

  SetUpComboBoxSimple(cbDigiProbe1, "Digi. Probe 1 ", rowID, 4);
  for( int i = 0; i < (int) DPP::Bit_BoardConfig::ListDigiProbe1_PSD.size(); i++){
    cbDigiProbe1->addItem(QString::fromStdString(DPP::Bit_BoardConfig::ListDigiProbe1_PSD[i].first), DPP::Bit_BoardConfig::ListDigiProbe1_PSD[i].second);
  }
  connect(cbDigiProbe1, &RComboBox::currentIndexChanged, this, [=](){
    if( !enableSignalSlot ) return;
    digi[ID]->SetBits(DPP::BoardConfiguration, DPP::Bit_BoardConfig::DigiProbel1_PSD, cbDigiProbe1->currentData().toInt(), cbScopeCh->currentIndex());
    dataTrace[2]->setName(cbDigiProbe1->currentText());
  });

  SetUpComboBoxSimple(cbDigiProbe2, "Digi. Probe 2 ", rowID, 6);
  for( int i = 0; i < (int) DPP::Bit_BoardConfig::ListDigiProbe2_PSD.size(); i++){
    cbDigiProbe2->addItem(QString::fromStdString(DPP::Bit_BoardConfig::ListDigiProbe2_PSD[i].first), DPP::Bit_BoardConfig::ListDigiProbe2_PSD[i].second);
  }
  connect(cbDigiProbe2, &RComboBox::currentIndexChanged, this, [=](){
    if( !enableSignalSlot ) return;
    digi[ID]->SetBits(DPP::BoardConfiguration, DPP::Bit_BoardConfig::DigiProbel1_PSD, cbDigiProbe1->currentData().toInt(), cbScopeCh->currentIndex());
    dataTrace[3]->setName(cbDigiProbe2->currentText());
  });

  dataTrace[4]->setVisible(false);

  enableSignalSlot = true;
}

void Scope::SetUpPanel_QDC() {

  enableSignalSlot = false;
  printf("==== %s \n", __func__);

  int rowID = 0;

  SetUpSpinBox(sbReordLength,     "Record Length [ns] ", rowID, 0, DPP::QDC::RecordLength_W);
  SetUpSpinBox(sbPreTrigger,        "Pre Trigger [ns] ", rowID, 2, DPP::QDC::PreTrigger);
  SetUpSpinBox(sbDCOffset,             "DC offset [%] ", rowID, 4, DPP::QDC::DCOffset);
  sbDCOffset->setDecimals(2);   

  SetUpComboBoxSimple(cbPolarity, "Polarity ", rowID, 6);
  cbPolarity->addItem("Positive", 0);
  cbPolarity->addItem("Negative", 1);
  connect(cbPolarity, &RComboBox::currentIndexChanged, this, [=](){
    if( !enableSignalSlot ) return;
    digi[ID]->SetBits(DPP::QDC::DPPAlgorithmControl, DPP::QDC::Bit_DPPAlgorithmControl::Polarity, cbPolarity->currentData().toInt(), cbScopeCh->currentIndex()/8);
  });

  rowID ++; //=============================================================
  SetUpSpinBox(sbShortGate,   "Gate Width [ns] ", rowID, 0, DPP::QDC::GateWidth);
  SetUpSpinBox(sbGateOffset, "Gate Offset [ns] ", rowID, 2, DPP::QDC::GateOffset);


  QLabel * lb = new QLabel("Threshold [LSB] ", settingGroup);
  lb->setAlignment(Qt::AlignRight | Qt::AlignCenter);
  settingLayout->addWidget(lb, rowID, 4);

  sbThreshold = new RSpinBox(settingGroup);
  settingLayout->addWidget(sbThreshold, rowID, 5);

  sbThreshold->setMinimum(0); 
  sbThreshold->setMaximum(0xFFF);
  sbThreshold->setSingleStep(1);

  connect(sbThreshold, &RSpinBox::valueChanged, this, [=](){
    if( !enableSignalSlot ) return;
    sbThreshold->setStyleSheet("color : blue;");
  });

  connect(sbThreshold, &RSpinBox::returnPressed, this, [=](){
    if( !enableSignalSlot ) return;  

    uint32_t value = sbThreshold->value();
    sbThreshold->setStyleSheet("");

    int ch = cbScopeCh->currentIndex();

    int grp = ch/8;
    int subCh = ch%8;

    Reg para = DPP::QDC::TriggerThreshold_sub0;

    digiMTX[ID].lock();
    switch(subCh){
      case 0: para = DPP::QDC::TriggerThreshold_sub0; break;
      case 1: para = DPP::QDC::TriggerThreshold_sub1; break;
      case 2: para = DPP::QDC::TriggerThreshold_sub2; break;
      case 3: para = DPP::QDC::TriggerThreshold_sub3; break;
      case 4: para = DPP::QDC::TriggerThreshold_sub4; break;
      case 5: para = DPP::QDC::TriggerThreshold_sub5; break;
      case 6: para = DPP::QDC::TriggerThreshold_sub6; break;
      case 7: para = DPP::QDC::TriggerThreshold_sub7; break;
    }
    digiMTX[ID].unlock();

    QString msg;
    msg = QString::fromStdString(para.GetName()) + "|DIG:"+ QString::number(digi[ID]->GetSerialNumber()) + ",CH:" + (ch == -1 ? "All" : QString::number(ch));
    msg += " = " + QString::number(sbThreshold->value());
    msg += " | 0x" + QString::number(value, 16); 

    digi[ID]->WriteRegister(para, value, grp);
    
    if( digi[ID]->GetErrorCode() == CAEN_DGTZ_Success ){
      SendLogMsg(msg + " | OK.");
      sbThreshold->setStyleSheet("");
      emit UpdateOtherPanels();
    }else{
      SendLogMsg(msg + " | Fail.");
      sbThreshold->setStyleSheet("color:red;");
    }

  });

  rowID ++; //=============================================================
  SetUpComboBoxSimple(cbAnaProbe1, "Ana. Probe ", rowID, 0);
  for( int i = 0; i < (int) DPP::Bit_BoardConfig::ListAnaProbe_QDC.size(); i++){
    cbAnaProbe1->addItem(QString::fromStdString(DPP::Bit_BoardConfig::ListAnaProbe_QDC[i].first), DPP::Bit_BoardConfig::ListAnaProbe_QDC[i].second);
  }
  connect(cbAnaProbe1, &RComboBox::currentIndexChanged, this, [=](){
    if( !enableSignalSlot ) return;
    digi[ID]->SetBits(DPP::BoardConfiguration, DPP::Bit_BoardConfig::AnalogProbe1, cbAnaProbe1->currentData().toInt(), cbScopeCh->currentIndex());
    dataTrace[0]->setName(cbAnaProbe1->currentText());

  });

  dataTrace[1]->setVisible(true); dataTrace[1]->setName("Gate");
  dataTrace[2]->setVisible(true); dataTrace[2]->setName("Trigger");
  dataTrace[3]->setVisible(true); dataTrace[3]->setName("Trigger HoldOff");
  dataTrace[4]->setVisible(true); dataTrace[4]->setName("OverThreshold");

  enableSignalSlot = true;

}

void Scope::EnableControl(bool enable){
  DebugPrint("%s", "Scope");
  if( digi[ID]->GetDPPType() == V1730_DPP_PHA_CODE ){

    sbReordLength->setEnabled(enable);
    sbPreTrigger->setEnabled(enable);
    sbDCOffset->setEnabled(enable);
    cbDynamicRange->setEnabled(enable);

    sbInputRiseTime->setEnabled(enable);
    //sbThreshold->setEnabled(enable);
    sbTriggerHoldOff->setEnabled(enable);
    cbSmoothingFactor->setEnabled(enable);

    sbTrapRiseTime->setEnabled(enable);
    sbTrapFlatTop->setEnabled(enable);
    sbDecayTime->setEnabled(enable);
    sbPeakingTime->setEnabled(enable);

    cbPolarity->setEnabled(enable);
    cbBaselineAvg->setEnabled(enable);
    cbPeakAvg->setEnabled(enable);
    sbPeakHoldOff->setEnabled(enable);

  }else{

    settingGroup->setEnabled(enable);

  }

}

//*=======================================================
//*=======================================================

void Scope::UpdateComobox(RComboBox * &cb, const Reg para){
  DebugPrint("%s", "Scope");
  int ch = cbScopeCh->currentIndex();

  enableSignalSlot = false;
  int haha = -99; 

  if( digi[ID]->GetDPPType() == DPPTypeCode::DPP_QDC_CODE ){
    haha = digi[ID]->GetSettingFromMemory(para, ch/8);
  }else{
    haha = digi[ID]->GetSettingFromMemory(para, ch);
  }

  for( int i = 0; i < cb->count(); i++){
    int kaka = cb->itemData(i).toInt();
    if( kaka == haha ){
      cb->setCurrentIndex(i);
      break;
    }
  }

  //enableSignalSlot = true;
}

void Scope::UpdateSpinBox(RSpinBox * &sb, const Reg para){
  DebugPrint("%s", "Scope");
  int ch = cbScopeCh->currentIndex();

  if( digi[ID]->GetDPPType() == DPPTypeCode::DPP_QDC_CODE ) ch = ch /8;

  enableSignalSlot = false;
  unsigned int haha = digi[ID]->GetSettingFromMemory(para, ch);

  if( para.GetPartialStep() >   0 ) sb->setValue(haha * para.GetPartialStep() * tick2ns);
  if( para.GetPartialStep() == -1 ) sb->setValue(haha);
  //enableSignalSlot = true;
}


void Scope::UpdatePanelFromMomeory(){

  enableSignalSlot = false;
  printf("==== Scope::%s \n", __func__);

  int ch = cbScopeCh->currentIndex();

  if( digi[ID]->GetDPPType() == V1730_DPP_PHA_CODE ) UpdatePanel_PHA();
  if( digi[ID]->GetDPPType() == V1730_DPP_PSD_CODE ) UpdatePanel_PSD();
  if( digi[ID]->GetDPPType() == V1740_DPP_QDC_CODE ) UpdatePanel_QDC();

  settingGroup->setEnabled(digi[ID]->GetInputChannelOnOff(ch));

}

void Scope::UpdatePanel_PHA(){
  enableSignalSlot = false;
  printf("==== %s \n", __func__);

  int ch = cbScopeCh->currentIndex();

  uint32_t DPPAlg = digi[ID]->GetSettingFromMemory(DPP::DPPAlgorithmControl, ch);
  if( Digitizer::ExtractBits(DPPAlg, DPP::Bit_DPPAlgorithmControl_PHA::Polarity) ){
    cbPolarity->setCurrentIndex(1);
  }else{
    cbPolarity->setCurrentIndex(0);
  }

  unsigned int haha = digi[ID]->GetSettingFromMemory(DPP::RecordLength_G, ch);
  sbReordLength->setValue(haha * DPP::RecordLength_G.GetPartialStep() * tick2ns);

  haha = digi[ID]->GetSettingFromMemory(DPP::ChannelDCOffset, ch);
  sbDCOffset->setValue((1.0 - haha * 1.0 / 0xFFFF) * 100 );

  UpdateComobox(cbDynamicRange, DPP::InputDynamicRange);
  UpdateSpinBox(sbPreTrigger, DPP::PreTrigger);

  UpdateSpinBox(sbInputRiseTime,  DPP::PHA::InputRiseTime);
  UpdateSpinBox(sbThreshold,      DPP::PHA::TriggerThreshold);
  UpdateSpinBox(sbTriggerHoldOff, DPP::PHA::TriggerHoldOffWidth);

  UpdateSpinBox(sbTrapRiseTime, DPP::PHA::TrapezoidRiseTime);
  UpdateSpinBox(sbTrapFlatTop,  DPP::PHA::TrapezoidFlatTop);
  UpdateSpinBox(sbDecayTime,    DPP::PHA::DecayTime);
  UpdateSpinBox(sbPeakingTime,  DPP::PHA::PeakingTime);

  UpdateComobox(cbSmoothingFactor, DPP::PHA::RCCR2SmoothingFactor);

  int temp = Digitizer::ExtractBits(DPPAlg, DPP::Bit_DPPAlgorithmControl_PHA::BaselineAvg);
  for(int i = 0; i < cbBaselineAvg->count(); i++){
    if( cbBaselineAvg->itemData(i).toInt() == temp) {
      cbBaselineAvg->setCurrentIndex(i);
      break;
    }
  }

  temp = Digitizer::ExtractBits(DPPAlg, DPP::Bit_DPPAlgorithmControl_PHA::PeakMean);
  for(int i = 0; i < cbPeakAvg->count(); i++){
    if( cbPeakAvg->itemData(i).toInt() == temp) {
      cbPeakAvg->setCurrentIndex(i);
      break;
    }
  }

  uint32_t BdCfg = digi[ID]->GetSettingFromMemory(DPP::BoardConfiguration);

  temp = Digitizer::ExtractBits(BdCfg, DPP::Bit_BoardConfig::AnalogProbe1);
  for(int i = 0; i < cbAnaProbe1->count(); i++){
    if( cbAnaProbe1->itemData(i).toInt() == temp) {
      cbAnaProbe1->setCurrentIndex(i);
      dataTrace[0]->setName(cbAnaProbe1->currentText());
      break;
    }
  }
  temp = Digitizer::ExtractBits(BdCfg, DPP::Bit_BoardConfig::AnalogProbe2);
  for(int i = 0; i < cbAnaProbe2->count(); i++){
    if( cbAnaProbe2->itemData(i).toInt() == temp) {
      cbAnaProbe2->setCurrentIndex(i);
      dataTrace[1]->setName(cbAnaProbe2->currentText());
      break;
    }
  }
  temp = Digitizer::ExtractBits(BdCfg, DPP::Bit_BoardConfig::DigiProbel1_PHA);
  for(int i = 0; i < cbDigiProbe1->count(); i++){
    if( cbDigiProbe1->itemData(i).toInt() == temp) {
      cbDigiProbe1->setCurrentIndex(i);
      dataTrace[2]->setName(cbDigiProbe1->currentText());
      break;
    }
  }

  enableSignalSlot = true;
}

void Scope::UpdatePanel_PSD(){
  enableSignalSlot = false;

  printf("==== %s \n", __func__);

  int ch = cbScopeCh->currentIndex();

  uint32_t DPPAlg = digi[ID]->GetSettingFromMemory(DPP::DPPAlgorithmControl, ch);
  if( Digitizer::ExtractBits(DPPAlg, DPP::Bit_DPPAlgorithmControl_PSD::Polarity) ){
    cbPolarity->setCurrentIndex(1);
  }else{
    cbPolarity->setCurrentIndex(0);
  }

  unsigned int haha = digi[ID]->GetSettingFromMemory(DPP::RecordLength_G, ch);
  sbReordLength->setValue(haha * DPP::RecordLength_G.GetPartialStep() * tick2ns);

  haha = digi[ID]->GetSettingFromMemory(DPP::ChannelDCOffset, ch);
  sbDCOffset->setValue((1.0 - haha * 1.0 / 0xFFFF) * 100 );

  UpdateComobox(cbDynamicRange, DPP::InputDynamicRange);
  UpdateSpinBox(sbPreTrigger, DPP::PreTrigger);

  UpdateSpinBox(sbShortGate, DPP::PSD::ShortGateWidth);
  UpdateSpinBox(sbLongGate,  DPP::PSD::LongGateWidth);
  UpdateSpinBox(sbGateOffset, DPP::PSD::GateOffset);

  uint32_t BdCfg = digi[ID]->GetSettingFromMemory(DPP::BoardConfiguration, ch);

  int temp = Digitizer::ExtractBits(BdCfg, DPP::Bit_BoardConfig::AnalogProbe1);
  for( int i = 0; i < cbAnaProbe1->count(); i++){
    if( cbAnaProbe1->itemData(i).toInt() == temp ) {
      cbAnaProbe1->setCurrentIndex(i);
      dataTrace[0]->setName(cbAnaProbe1->currentText());
      break;
    }
  }
  temp = Digitizer::ExtractBits(BdCfg, DPP::Bit_BoardConfig::DigiProbel1_PSD);
  for( int i = 0; i < cbDigiProbe1->count(); i++){
    if( cbDigiProbe1->itemData(i).toInt() == temp ) {
      cbDigiProbe1->setCurrentIndex(i);
      dataTrace[2]->setName(cbDigiProbe1->currentText());
      break;
    }
  }
  temp = Digitizer::ExtractBits(BdCfg, DPP::Bit_BoardConfig::DigiProbel2_PSD);
  for( int i = 0; i < cbDigiProbe2->count(); i++){
    if( cbDigiProbe2->itemData(i).toInt() == temp ) {
      cbDigiProbe2->setCurrentIndex(i);
      dataTrace[3]->setName(cbDigiProbe2->currentText());
      break;
    }
  }

  enableSignalSlot = true;
}

void Scope::UpdatePanel_QDC(){
  enableSignalSlot = false;

  printf("==== %s \n", __func__);

  int ch = cbScopeCh->currentIndex();

  int grp = ch/8;

  uint32_t DPPAlg = digi[ID]->GetSettingFromMemory(DPP::QDC::DPPAlgorithmControl, grp);
  if( Digitizer::ExtractBits(DPPAlg, DPP::QDC::Bit_DPPAlgorithmControl::Polarity) ){
    cbPolarity->setCurrentIndex(1);
  }else{
    cbPolarity->setCurrentIndex(0);
  }

  uint32_t haha = digi[ID]->GetSettingFromMemory(DPP::QDC::DCOffset, grp);
  sbDCOffset->setValue((1.0 - haha * 1.0 / 0xFFFF) * 100 );

  //UpdateSpinBox(sbReordLength, DPP::QDC::RecordLength);
  sbReordLength->setValue(digi[ID]->ReadQDCRecordLength() * 8 * 16);
  UpdateSpinBox(sbPreTrigger, DPP::QDC::PreTrigger);

  UpdateSpinBox(sbShortGate, DPP::QDC::GateWidth);
  UpdateSpinBox(sbGateOffset, DPP::QDC::GateOffset);

  int subCh = ch%8;
  uint32_t threshold = 0 ;
  switch( subCh ){
    case 0 : threshold = digi[ID]->GetSettingFromMemory(DPP::QDC::TriggerThreshold_sub0, grp); break;
    case 1 : threshold = digi[ID]->GetSettingFromMemory(DPP::QDC::TriggerThreshold_sub1, grp); break;
    case 2 : threshold = digi[ID]->GetSettingFromMemory(DPP::QDC::TriggerThreshold_sub2, grp); break;
    case 3 : threshold = digi[ID]->GetSettingFromMemory(DPP::QDC::TriggerThreshold_sub3, grp); break;
    case 4 : threshold = digi[ID]->GetSettingFromMemory(DPP::QDC::TriggerThreshold_sub4, grp); break;
    case 5 : threshold = digi[ID]->GetSettingFromMemory(DPP::QDC::TriggerThreshold_sub5, grp); break;
    case 6 : threshold = digi[ID]->GetSettingFromMemory(DPP::QDC::TriggerThreshold_sub6, grp); break;
    case 7 : threshold = digi[ID]->GetSettingFromMemory(DPP::QDC::TriggerThreshold_sub7, grp); break;
  }
  sbThreshold->setValue(threshold);


  uint32_t BdCfg = digi[ID]->GetSettingFromMemory(DPP::BoardConfiguration, 0);

  int temp = Digitizer::ExtractBits(BdCfg, DPP::Bit_BoardConfig::AnalogProbe1);
  for( int i = 0; i < cbAnaProbe1->count(); i++){
    if( cbAnaProbe1->itemData(i).toInt() == temp ) {
      cbAnaProbe1->setCurrentIndex(i);
      dataTrace[0]->setName(cbAnaProbe1->currentText());
      break;
    }
  }

  enableSignalSlot = true;
}

void Scope::ReadSettingsFromBoard(){
  DebugPrint("%s", "Scope");
  digi[ID]->ReadRegister(DPP::BoardConfiguration);

  if( digi[ID]->GetDPPType() == V1730_DPP_PHA_CODE ){
    int ch = cbScopeCh->currentIndex();
  
    digi[ID]->ReadRegister(DPP::RecordLength_G, ch);
    digi[ID]->ReadRegister(DPP::PreTrigger, ch);
    digi[ID]->ReadRegister(DPP::ChannelDCOffset, ch);
    digi[ID]->ReadRegister(DPP::InputDynamicRange, ch);
    digi[ID]->ReadRegister(DPP::BoardConfiguration, ch);
    digi[ID]->ReadRegister(DPP::DPPAlgorithmControl, ch);

    digi[ID]->ReadRegister(DPP::PHA::InputRiseTime, ch);
    digi[ID]->ReadRegister(DPP::PHA::TriggerThreshold, ch);
    digi[ID]->ReadRegister(DPP::PHA::TriggerHoldOffWidth, ch);
    digi[ID]->ReadRegister(DPP::PHA::TrapezoidRiseTime, ch);
    digi[ID]->ReadRegister(DPP::PHA::TrapezoidFlatTop, ch);
    digi[ID]->ReadRegister(DPP::PHA::DecayTime, ch);
    digi[ID]->ReadRegister(DPP::PHA::PeakingTime, ch);
  }

  if( digi[ID]->GetDPPType() == V1730_DPP_PSD_CODE){
    int ch = cbScopeCh->currentIndex();
  
    digi[ID]->ReadRegister(DPP::RecordLength_G, ch);
    digi[ID]->ReadRegister(DPP::PreTrigger, ch);
    digi[ID]->ReadRegister(DPP::ChannelDCOffset, ch);
    digi[ID]->ReadRegister(DPP::InputDynamicRange, ch);
    digi[ID]->ReadRegister(DPP::BoardConfiguration, ch);
    digi[ID]->ReadRegister(DPP::DPPAlgorithmControl, ch);

    digi[ID]->ReadRegister(DPP::PSD::ShortGateWidth);
    digi[ID]->ReadRegister(DPP::PSD::LongGateWidth);
    digi[ID]->ReadRegister(DPP::PSD::GateOffset);
  }

  if( digi[ID]->GetDPPType() == V1740_DPP_QDC_CODE){
    int ch = cbScopeCh->currentIndex();
    int grp = ch/8;

    //digi[ID]->ReadRegister(DPP::QDC::RecordLength, grp);

    digi[ID]->ReadQDCRecordLength();
    
    digi[ID]->ReadRegister(DPP::QDC::PreTrigger, grp);
    digi[ID]->ReadRegister(DPP::QDC::DCOffset, grp);
    digi[ID]->ReadRegister(DPP::QDC::DPPAlgorithmControl, grp);

    digi[ID]->ReadRegister(DPP::QDC::GateOffset, grp);
    digi[ID]->ReadRegister(DPP::QDC::GateWidth, grp);

    digi[ID]->ReadRegister(DPP::QDC::TriggerThreshold_sub0, grp);
    digi[ID]->ReadRegister(DPP::QDC::TriggerThreshold_sub1, grp);
    digi[ID]->ReadRegister(DPP::QDC::TriggerThreshold_sub2, grp);
    digi[ID]->ReadRegister(DPP::QDC::TriggerThreshold_sub3, grp);
    digi[ID]->ReadRegister(DPP::QDC::TriggerThreshold_sub4, grp);
    digi[ID]->ReadRegister(DPP::QDC::TriggerThreshold_sub5, grp);
    digi[ID]->ReadRegister(DPP::QDC::TriggerThreshold_sub6, grp);
    digi[ID]->ReadRegister(DPP::QDC::TriggerThreshold_sub7, grp);

  }

  UpdatePanelFromMomeory();

}


