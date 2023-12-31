#include "Scope.h"

#include <QValueAxis>
#include <QRandomGenerator>
#include <QGroupBox>
#include <QStandardItemModel>
#include <QLabel>


Scope::Scope(Digitizer ** digi, unsigned int nDigi, ReadDataThread ** readDataThread, QMainWindow * parent) : QMainWindow(parent){

  this->digi = digi;
  this->nDigi = nDigi;
  this->readDataThread = readDataThread;

  setWindowTitle("Scope");
  setGeometry(0, 0, 1000, 800);  
  setWindowFlags( this->windowFlags() & ~Qt::WindowCloseButtonHint );

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

  plot->setAnimationDuration(1); // msec
  plot->setAnimationOptions(QChart::NoAnimation);
  plot->createDefaultAxes(); /// this must be after addSeries();
  /// this must be after createDefaultAxes();
  QValueAxis * yaxis = qobject_cast<QValueAxis*> (plot->axes(Qt::Vertical).first());
  QValueAxis * xaxis = qobject_cast<QValueAxis*> (plot->axes(Qt::Horizontal).first());
  yaxis->setTickCount(7);
  yaxis->setTickInterval((0x1FFF)/4);
  yaxis->setRange(-(0x1FFF), 0x1FFF);
  yaxis->setLabelFormat("%.0f");

  xaxis->setRange(0, 4000);
  xaxis->setTickCount(11);
  xaxis->setLabelFormat("%.0f");
  xaxis->setTitleText("Time [ns]");

  updateTraceThread = new TimingThread();
  updateTraceThread->SetWaitTimeinSec(0.5);
  connect(updateTraceThread, &TimingThread::timeUp, this, &Scope::UpdateScope);


  sbReordLength = nullptr;
  sbPreTrigger = nullptr;
  sbDCOffset = nullptr;
  cbDynamicRange = nullptr;
  cbPolarity = nullptr;

  ///---- PHA
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
  for( int i = 0; i < digi[0]->GetNChannels(); i++) cbScopeCh->addItem("Ch-" + QString::number(i));
  tick2ns = digi[ID]->GetTick2ns();

  connect(cbScopeDigi, &RComboBox::currentIndexChanged, this, [=](int index){
    if( !enableSignalSlot ) return;

    bool saveACQStartStatus = isACQStarted;
    if( isACQStarted) StopScope();

    ID = index;
    tick2ns = digi[ID]->GetTick2ns();
    //---setup cbScopeCh
    cbScopeCh->clear();
    for( int i = 0; i < digi[ID]->GetNChannels(); i++) cbScopeCh->addItem("Ch-" + QString::number(i));

    //---Setup SettingGroup
    CleanUpSettingsGroupBox();
    if( digi[ID]->GetDPPType() == V1730_DPP_PHA_CODE ) SetUpPHAPanel();
    if( digi[ID]->GetDPPType() == V1730_DPP_PSD_CODE ) SetUpPSDPanel();

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

  //================ Trace settings
  rowID ++;
  {
    settingGroup = new QGroupBox("Trace Settings",this);
    layout->addWidget(settingGroup, rowID, 0, 1, 6);

    settingLayout = new QGridLayout(settingGroup);
    settingLayout->setSpacing(0);

    if( digi[ID]->GetDPPType() == V1730_DPP_PHA_CODE ) SetUpPHAPanel();
    if( digi[ID]->GetDPPType() == V1730_DPP_PSD_CODE ) SetUpPSDPanel();

  }
  //================ Plot view
  rowID ++;
  plotView = new RChartView(plot, this);
  layout->addWidget(plotView, rowID, 0, 1, 6);

  
  //================ Key binding
  rowID ++;
  QLabel * lbhints = new QLabel("Type 'r' to restore view, '+/-' Zoom in/out, arrow key to pan.", this);
  layout->addWidget(lbhints, rowID, 0, 1, 4);
  
  QLabel * lbinfo = new QLabel("Trace updates every " + QString::number(updateTraceThread->GetWaitTimeinSec()) + " sec.", this);
  lbinfo->setAlignment(Qt::AlignRight);
  layout->addWidget(lbinfo, rowID, 5);

  rowID ++;
  //TODO =========== Trace step
  QLabel * lbinfo2 = new QLabel("Maximum time range is " + QString::number(MaxDisplayTraceDataLength * 8) + " ns due to processing speed.", this);
  layout->addWidget(lbinfo2, rowID, 0, 1, 5);

  //================ close button
  rowID ++;
  bnScopeStart = new QPushButton("Start", this);
  layout->addWidget(bnScopeStart, rowID, 0);
  connect(bnScopeStart, &QPushButton::clicked, this, [=](){this->StartScope();});

  bnScopeStop = new QPushButton("Stop", this);
  layout->addWidget(bnScopeStop, rowID, 1);
  connect(bnScopeStop, &QPushButton::clicked, this, &Scope::StopScope);

  QLabel * lbTriggerRate = new QLabel("Trigger Rate [Hz] : ", this);
  lbTriggerRate->setAlignment(Qt::AlignCenter | Qt::AlignRight);
  layout->addWidget(lbTriggerRate, rowID, 2);

  leTriggerRate = new QLineEdit(this);
  leTriggerRate->setAlignment(Qt::AlignRight);
  leTriggerRate->setReadOnly(true);
  layout->addWidget(leTriggerRate, rowID, 3);

  QPushButton * bnClose = new QPushButton("Close", this);
  layout->addWidget(bnClose, rowID, 5);
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

  enableSignalSlot = true;

}


Scope::~Scope(){

  updateTraceThread->Stop();
  updateTraceThread->quit();
  updateTraceThread->wait();
  delete updateTraceThread;
  for( int i = 0; i < MaxNumberOfTrace; i++) delete dataTrace[i];
  delete plot;
}

//*=======================================================
//*=======================================================
void Scope::StartScope(){
  if( !digi ) return;

  //TODO set other channel to be no trace;

  for( unsigned int iDigi = 0; iDigi < nDigi; iDigi ++){

    traceOn[iDigi] = digi[iDigi]->IsRecordTrace(); //remember setting
    digi[iDigi]->GetData()->ClearData();
    digi[iDigi]->SetBits(DPP::BoardConfiguration, DPP::Bit_BoardConfig::RecordTrace, 1, -1);
    digi[iDigi]->StartACQ();

    printf("----- readDataThread running ? %d.\n", readDataThread[iDigi]->isRunning());
    if( readDataThread[iDigi]->isRunning() ){
      readDataThread[iDigi]->quit();
      readDataThread[iDigi]->wait();
    }
    readDataThread[iDigi]->SetScopeMode(true);
    readDataThread[iDigi]->SetSaveData(false);
    readDataThread[iDigi]->start();
  }

  emit UpdateOtherPanels();

  updateTraceThread->start();

  bnScopeStart->setEnabled(false);
  bnScopeStart->setStyleSheet("");
  bnScopeStop->setEnabled(true);
  bnScopeStop->setStyleSheet("background-color: red;");

  EnableControl(false);

  TellACQOnOff(true);

  isACQStarted = true;

}

void Scope::StopScope(){
  if( !digi ) return;

  // printf("------ Scope::%s \n", __func__);
  updateTraceThread->Stop();
  updateTraceThread->quit();
  updateTraceThread->exit();

  for( unsigned int iDigi = 0; iDigi < nDigi; iDigi ++){

    if( readDataThread[iDigi]->isRunning() ){
      readDataThread[iDigi]->Stop();
      readDataThread[iDigi]->quit();
      readDataThread[iDigi]->wait();
    }
    digiMTX[iDigi].lock();
    digi[iDigi]->StopACQ();
    digiMTX[iDigi].unlock();

    digi[iDigi]->SetBits(DPP::BoardConfiguration, DPP::Bit_BoardConfig::RecordTrace, traceOn[iDigi], -1);
  }

  emit UpdateOtherPanels();

  bnScopeStart->setEnabled(true);
  bnScopeStart->setStyleSheet("background-color: green;");
  bnScopeStop->setEnabled(false);
  bnScopeStop->setStyleSheet("");

  EnableControl(true);

  TellACQOnOff(false);

  isACQStarted = false;

  // printf("----- end of %s\n", __func__);

}

void Scope::UpdateScope(){

  if( !digi ) return;

  int ch = cbScopeCh->currentIndex();

  if( digi[ID]->GetChannelOnOff(ch) == false) return;

  int tick2ns = digi[ID]->GetTick2ns();
  int factor = digi[ID]->IsDualTrace_PHA() ? 2 : 1;

  digiMTX[ID].lock();
  Data * data = digi[ID]->GetData();

  //leTriggerRate->setText(QString::number(data->TriggerRate[ch]) + " [" + QString::number(data->NumEventsDecoded[ch]) + "]");
  leTriggerRate->setText(QString::number(data->TriggerRate[ch]));

  unsigned short index = data->DataIndex[ch];
  unsigned short traceLength = data->Waveform1[ch][index].size();

  if( data->TriggerRate[ch] > 0 ){

    //printf("--- %d | %d \n", index, traceLength );

    QVector<QPointF> points[4];
    if( digi[ID]->GetDPPType() == V1730_DPP_PHA_CODE ) {
      for( int i = 0; i < (int) (data->Waveform1[ch][index]).size() ; i++ ) {
        points[0].append(QPointF(tick2ns * i * factor, (data->Waveform1[ch][index])[i])); 
        if( i < (int) data->Waveform2[ch][index].size() )  points[1].append(QPointF(tick2ns * i * factor, (data->Waveform2[ch][index])[i]));
        if( i < (int) data->DigiWaveform1[ch][index].size() )  points[2].append(QPointF(tick2ns * i * factor, (data->DigiWaveform1[ch][index])[i] * 1000));
        if( i < (int) data->DigiWaveform2[ch][index].size() )  points[3].append(QPointF(tick2ns * i * factor, (data->DigiWaveform2[ch][index])[i] * 1000 + 500));
      }
    }

    if( digi[ID]->GetDPPType() == V1730_DPP_PSD_CODE ) {
      for( int i = 0; i < (int) (data->DigiWaveform1[ch][index]).size() ; i++ ) {
        points[0].append(QPointF(tick2ns * i * factor, (data->Waveform1[ch][index])[i])); 
        if( i < (int) data->Waveform2[ch][index].size() )  points[1].append(QPointF(tick2ns * i * factor, (data->Waveform2[ch][index])[i]));
        if( i < (int) data->DigiWaveform1[ch][index].size() )  points[2].append(QPointF(tick2ns * i, (data->DigiWaveform1[ch][index])[i] * 1000));
        if( i < (int) data->DigiWaveform2[ch][index].size() )  points[3].append(QPointF(tick2ns * i, (data->DigiWaveform2[ch][index])[i] * 1000 + 500));
      }
    }
    dataTrace[0]->replace(points[0]);
    dataTrace[1]->replace(points[1]);
    dataTrace[2]->replace(points[2]);
    dataTrace[3]->replace(points[3]);
  }
  digiMTX[ID].unlock();

  if( data->TriggerRate[ch] == 0 ){
      dataTrace[0]->clear();
      dataTrace[1]->clear();
      dataTrace[2]->clear();
      dataTrace[3]->clear();
  }

  plot->axes(Qt::Horizontal).first()->setRange(0, tick2ns * traceLength * factor);

  emit UpdateScaler();

}

//*=======================================================
//*=======================================================
void Scope::SetUpComboBoxSimple(RComboBox * &cb, QString str, int row, int col){
  QLabel * lb = new QLabel(str, settingGroup);
  lb->setAlignment(Qt::AlignRight | Qt::AlignCenter);
  settingLayout->addWidget(lb, row, col);

  cb = new RComboBox(settingGroup);
  settingLayout->addWidget(cb, row, col + 1);
  
}

void Scope::SetUpComboBox(RComboBox * &cb, QString str, int row, int col, const Reg para){
  
  SetUpComboBoxSimple(cb, str, row, col);

  for( int i = 0; i < (int) para.GetComboList().size(); i++){
    cb->addItem(QString::fromStdString(para.GetComboList()[i].first), para.GetComboList()[i].second);
  }

  connect(cb, &RComboBox::currentIndexChanged, this , [=](){
    if( ! enableSignalSlot ) return;

    int ch = cbScopeCh->currentIndex();
    int value = cb->currentData().toInt();
    digiMTX[ID].lock();
    digi[ID]->WriteRegister(para, value, ch);
    digiMTX[ID].unlock();

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

    if( para == DPP::RecordLength_G){
      int factor = digi[ID]->IsDualTrace_PHA() ? 2 : 1;
      value = value * factor;
    }

    if( para == DPP::ChannelDCOffset ){
      value = uint16_t((1.0 - sb->value()/100.) * 0xFFFF);
    }

    if( para == DPP::PHA::TriggerThreshold ){
      value = sb->value();
    }

    msg += " | 0x" + QString::number(value, 16); 

    digiMTX[ID].lock();
    digi[ID]->WriteRegister(para, value, ch);

    digiMTX[ID].unlock();
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

  QList<QLabel *> labelChildren1 = settingGroup->findChildren<QLabel *>();
  for( int i = 0; i < labelChildren1.size(); i++) delete labelChildren1[i];
  
  QList<RComboBox *> labelChildren2 = settingGroup->findChildren<RComboBox *>();
  for( int i = 0; i < labelChildren2.size(); i++) delete labelChildren2[i];

  QList<RSpinBox *> labelChildren3 = settingGroup->findChildren<RSpinBox *>();
  for( int i = 0; i < labelChildren3.size(); i++) delete labelChildren3[i];

}

void Scope::SetUpPHAPanel(){
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

  enableSignalSlot = true;

}

void Scope::SetUpPSDPanel(){

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
    digi[ID]->SetBits(DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl_PHA::Polarity, cbPolarity->currentData().toInt(), cbScopeCh->currentIndex());
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
    digi[ID]->SetBits(DPP::BoardConfiguration, DPP::Bit_BoardConfig::AnaProbe_PSD, cbAnaProbe1->currentData().toInt(), cbScopeCh->currentIndex());
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

  enableSignalSlot = true;
}

void Scope::EnableControl(bool enable){

  if( digi[ID]->GetDPPType() == V1730_DPP_PHA_CODE ){

    sbPreTrigger->setEnabled(enable);
    sbTrapRiseTime->setEnabled(enable);
    sbTrapFlatTop->setEnabled(enable);
    sbDecayTime->setEnabled(enable);

    sbInputRiseTime->setEnabled(enable);
    cbSmoothingFactor->setEnabled(enable);

  }

}

//*=======================================================
//*=======================================================

void Scope::UpdateComobox(RComboBox * &cb, const Reg para){
  int ch = cbScopeCh->currentIndex();

  enableSignalSlot = false;
  int haha = digi[ID]->GetSettingFromMemory(para, ch);

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
  int ch = cbScopeCh->currentIndex();

  enableSignalSlot = false;
  unsigned int haha = digi[ID]->GetSettingFromMemory(para, ch);

  if( para.GetPartialStep() >   0 ) sb->setValue(haha * para.GetPartialStep() * tick2ns);
  if( para.GetPartialStep() == -1 ) sb->setValue(haha);
  //enableSignalSlot = true;
}


void Scope::UpdatePanelFromMomeory(){

  enableSignalSlot = false;
  printf("==== %s \n", __func__);

  int ch = cbScopeCh->currentIndex();

  unsigned int haha = digi[ID]->GetSettingFromMemory(DPP::RecordLength_G, ch);
  sbReordLength->setValue(haha * DPP::RecordLength_G.GetPartialStep() * tick2ns);

  // if( digi[ID]->GetDPPType() == V1730_DPP_PHA_CODE ){
  //   int factor = digi[ID]->IsDualTrace() ? 2 : 1; // if dual trace, 
  //   sbReordLength->setValue(haha * DPP::RecordLength_G.GetPartialStep() * tick2ns / factor);
  // }else{
  // }

  haha = digi[ID]->GetSettingFromMemory(DPP::ChannelDCOffset, ch);
  sbDCOffset->setValue((1.0 - haha * 1.0 / 0xFFFF) * 100 );

  UpdateComobox(cbDynamicRange, DPP::InputDynamicRange);
  UpdateSpinBox(sbPreTrigger, DPP::PreTrigger);

  if( digi[ID]->GetDPPType() == V1730_DPP_PHA_CODE ) UpdatePHAPanel();
  if( digi[ID]->GetDPPType() == V1730_DPP_PSD_CODE ) UpdatePSDPanel();

  settingGroup->setEnabled(digi[ID]->GetChannelOnOff(ch));

}

void Scope::UpdatePHAPanel(){
  enableSignalSlot = false;

  int ch = cbScopeCh->currentIndex();

  uint32_t DPPAlg = digi[ID]->GetSettingFromMemory(DPP::DPPAlgorithmControl, ch);
  if( Digitizer::ExtractBits(DPPAlg, DPP::Bit_DPPAlgorithmControl_PHA::Polarity) ){
    cbPolarity->setCurrentIndex(1);
  }else{
    cbPolarity->setCurrentIndex(0);
  }

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

void Scope::UpdatePSDPanel(){
  enableSignalSlot = false;

  printf("==== %s \n", __func__);

  int ch = cbScopeCh->currentIndex();

  uint32_t DPPAlg = digi[ID]->GetSettingFromMemory(DPP::DPPAlgorithmControl, ch);
  if( Digitizer::ExtractBits(DPPAlg, DPP::Bit_DPPAlgorithmControl_PSD::Polarity) ){
    cbPolarity->setCurrentIndex(1);
  }else{
    cbPolarity->setCurrentIndex(0);
  }

  UpdateSpinBox(sbShortGate, DPP::PSD::ShortGateWidth);
  UpdateSpinBox(sbLongGate,  DPP::PSD::LongGateWidth);
  UpdateSpinBox(sbGateOffset, DPP::PSD::GateOffset);

  uint32_t BdCfg = digi[ID]->GetSettingFromMemory(DPP::BoardConfiguration, ch);

  int temp = Digitizer::ExtractBits(BdCfg, DPP::Bit_BoardConfig::AnaProbe_PSD);
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

void Scope::ReadSettingsFromBoard(){

  int ch = cbScopeCh->currentIndex();

  digi[ID]->ReadRegister(DPP::RecordLength_G, ch);
  digi[ID]->ReadRegister(DPP::PreTrigger, ch);
  digi[ID]->ReadRegister(DPP::ChannelDCOffset, ch);
  digi[ID]->ReadRegister(DPP::InputDynamicRange, ch);
  digi[ID]->ReadRegister(DPP::BoardConfiguration, ch);
  digi[ID]->ReadRegister(DPP::DPPAlgorithmControl, ch);

  if( digi[ID]->GetDPPType() == V1730_DPP_PHA_CODE ){

    digi[ID]->ReadRegister(DPP::PHA::InputRiseTime, ch);
    digi[ID]->ReadRegister(DPP::PHA::TriggerThreshold, ch);
    digi[ID]->ReadRegister(DPP::PHA::TriggerHoldOffWidth, ch);
    digi[ID]->ReadRegister(DPP::PHA::TrapezoidRiseTime, ch);
    digi[ID]->ReadRegister(DPP::PHA::TrapezoidFlatTop, ch);
    digi[ID]->ReadRegister(DPP::PHA::DecayTime, ch);
    digi[ID]->ReadRegister(DPP::PHA::PeakingTime, ch);

  }

  if( digi[ID]->GetDPPType() == V1730_DPP_PSD_CODE){

    digi[ID]->ReadRegister(DPP::PSD::ShortGateWidth);
    digi[ID]->ReadRegister(DPP::PSD::LongGateWidth);
    digi[ID]->ReadRegister(DPP::PSD::GateOffset);

  }

  UpdatePanelFromMomeory();

}


