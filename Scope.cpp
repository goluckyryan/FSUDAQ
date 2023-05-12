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

  plot = new Trace();
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
  ch2ns = digi[ID]->GetCh2ns();

  connect(cbScopeDigi, &RComboBox::currentIndexChanged, this, [=](int index){
    if( !enableSignalSlot ) return;
    ID = index;
    ch2ns = digi[ID]->GetCh2ns();
    //---setup cbScopeCh
    cbScopeCh->clear();
    for( int i = 0; i < digi[ID]->GetNChannels(); i++) cbScopeCh->addItem("Ch-" + QString::number(i));

    //---Setup SettingGroup
    CleanUpSettingsGroupBox();
    if( digi[ID]->GetDPPType() == V1730_DPP_PHA_CODE ) SetUpPHAPanel();
    if( digi[ID]->GetDPPType() == V1730_DPP_PSD_CODE ) SetUpPSDPanel();

    ReadSettingsFromBoard();

  });

  bnReadSettingsFromBoard = new QPushButton("Refresh Settings", this);
  layout->addWidget(bnReadSettingsFromBoard, rowID, 2);
  connect(bnReadSettingsFromBoard, &QPushButton::clicked, this, &Scope::ReadSettingsFromBoard);

  //================ Trace settings
  rowID ++;
  {
    settingGroup = new QGroupBox("Trace Settings",this);
    layout->addWidget(settingGroup, rowID, 0, 1, 6);

    settingLayout = new QGridLayout(settingGroup);
    settingLayout->setSpacing(0);

    if( digi[ID]->GetDPPType() == V1730_DPP_PHA_CODE ) SetUpPHAPanel();

  }
  //================ Plot view
  rowID ++;
  plotView = new TraceView(plot, this);
  plotView->setRenderHints(QPainter::Antialiasing);
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

  UpdatePanelFromMomeory();

  bnScopeStart->setEnabled(true);
  bnScopeStop->setEnabled(false);

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

    digi[iDigi]->GetData()->SetSaveWaveToMemory(true);

    digi[iDigi]->StartACQ();

    readDataThread[iDigi]->SetScopeMode(true);
    readDataThread[iDigi]->SetSaveData(false);

    readDataThread[iDigi]->start();

  }

  updateTraceThread->start();

  bnScopeStart->setEnabled(false);
  bnScopeStop->setEnabled(true);

  EnableControl(false);

}

void Scope::StopScope(){
  if( !digi ) return;

  updateTraceThread->Stop();
  updateTraceThread->quit();
  updateTraceThread->exit();


  for( unsigned int iDigi = 0; iDigi < nDigi; iDigi ++){

    digi[iDigi]->StopACQ();

    readDataThread[iDigi]->quit();
    readDataThread[iDigi]->wait();
  }

  bnScopeStart->setEnabled(true);
  bnScopeStop->setEnabled(false);

  EnableControl(true);

}

void Scope::UpdateScope(){

  //printf("---- %s \n", __func__);

  if( !digi ) return;

  int ch = cbScopeCh->currentIndex();
  int ch2ns = digi[ID]->GetCh2ns();

  Data * data = digi[ID]->GetData();

  digiMTX[ID].lock();
  //leTriggerRate->setText(QString::number(data->TriggerRate[ch]) + " [" + QString::number(data->NumEventsDecoded[ch]) + "]");
  leTriggerRate->setText(QString::number(data->TriggerRate[ch]));

  unsigned short index = data->NumEvents[ch] - 1;
  unsigned short traceLength = data->Waveform1[ch][index].size();

  if( data->TriggerRate[ch] > 0 ){

    //printf("--- %d | %d \n", index, traceLength );

    QVector<QPointF> points[4];
    for( int i = 0; i < (int) (data->Waveform1[ch][index]).size() ; i++ ) {
      points[0].append(QPointF(ch2ns * i, (data->Waveform1[ch][index])[i])); 
      if( i < (int) data->Waveform2[ch][index].size() )  points[1].append(QPointF(ch2ns * i, (data->Waveform2[ch][index])[i]));
      if( i < (int) data->DigiWaveform1[ch][index].size() )  points[2].append(QPointF(ch2ns * i, (data->DigiWaveform1[ch][index])[i] * 1000));
      if( i < (int) data->DigiWaveform2[ch][index].size() )  points[3].append(QPointF(ch2ns * i, (data->DigiWaveform2[ch][index])[i] * 1000 + 500));
    }
    dataTrace[0]->replace(points[0]);
    dataTrace[1]->replace(points[1]);
    dataTrace[2]->replace(points[2]);
    dataTrace[3]->replace(points[3]);
  }
  digiMTX[ID].unlock();

  plot->axes(Qt::Horizontal).first()->setRange(0, ch2ns * traceLength);
  //plotView->SetHRange(0, ch2ns * traceLength);

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
    sb->setMaximum(para.GetMaxBit() * para.GetPartialStep() * ch2ns);
    if( para.GetPartialStep() > 0 ) sb->setSingleStep(para.GetPartialStep() * ch2ns);
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

    uint32_t value = sb->value() / ch2ns / abs(para.GetPartialStep());

    if( para == DPP::RecordLength_G || para == DPP::PreTrigger){
      int factor = digi[ID]->IsDualTrace() ? 2 : 1;
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
  cbAnaProbe1->addItem("Input", 0);
  cbAnaProbe1->addItem("RC-CR", 1);
  cbAnaProbe1->addItem("RC-CR2", 2);
  cbAnaProbe1->addItem("Trap.", 3);
  connect(cbAnaProbe1, &RComboBox::currentIndexChanged, this, [=](){
    if( !enableSignalSlot ) return;
    digi[ID]->SetBits(DPP::BoardConfiguration, DPP::Bit_BoardConfig::AnalogProbe1, cbAnaProbe1->currentData().toInt(), cbScopeCh->currentIndex());
    dataTrace[0]->setName(cbAnaProbe1->currentText());
  });

  SetUpComboBoxSimple(cbAnaProbe2, "Ana. Probe 2 ", rowID, 2);
  cbAnaProbe2->addItem("Input", 0);
  cbAnaProbe2->addItem("Threshold", 1);
  cbAnaProbe2->addItem("Trap.-Baseline", 2);
  cbAnaProbe2->addItem("Baseline", 3);
  connect(cbAnaProbe2, &RComboBox::currentIndexChanged, this, [=](){
    if( !enableSignalSlot ) return;
    digi[ID]->SetBits(DPP::BoardConfiguration, DPP::Bit_BoardConfig::AnalogProbe2, cbAnaProbe2->currentData().toInt(), cbScopeCh->currentIndex());
    dataTrace[1]->setName(cbAnaProbe2->currentText());
  });

  SetUpComboBoxSimple(cbDigiProbe1, "Digi. Probe 1 ", rowID, 4);
  cbDigiProbe1->addItem("Peaking", 0);
  cbDigiProbe1->addItem("Armed", 1);
  cbDigiProbe1->addItem("Peak Run", 2);
  cbDigiProbe1->addItem("Pile Up", 3);
  cbDigiProbe1->addItem("peaking", 4);
  cbDigiProbe1->addItem("TRG Valid. Win", 5);
  cbDigiProbe1->addItem("Baseline Freeze", 6);
  cbDigiProbe1->addItem("TRG Holdoff", 7);
  cbDigiProbe1->addItem("TRG Valid.", 8);
  cbDigiProbe1->addItem("ACQ Busy", 9);
  cbDigiProbe1->addItem("Zero Cross", 10);
  cbDigiProbe1->addItem("Ext. TRG", 11);
  cbDigiProbe1->addItem("Budy", 12);
  connect(cbDigiProbe1, &RComboBox::currentIndexChanged, this, [=](){
    if( !enableSignalSlot ) return;
    digi[ID]->SetBits(DPP::BoardConfiguration, DPP::Bit_BoardConfig::DigiProbel1, cbDigiProbe1->currentData().toInt(), cbScopeCh->currentIndex());
    dataTrace[2]->setName(cbDigiProbe2->currentText());
  });

  SetUpComboBoxSimple(cbDigiProbe2, "Digi. Probe 2 ", rowID, 6);
  cbDigiProbe2->addItem("Trigger", 0);
  dataTrace[3]->setName(cbDigiProbe2->currentText());
  cbDigiProbe2->setEnabled(false);

  enableSignalSlot = true;

}

void Scope::SetUpPSDPanel(){

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
  if( para.GetPartialStep() >   0 ) sb->setValue(haha * para.GetPartialStep() * ch2ns);
  if( para.GetPartialStep() == -1 ) sb->setValue(haha);
  //enableSignalSlot = true;
}


void Scope::UpdatePanelFromMomeory(){

  enableSignalSlot = false;
  int ch = cbScopeCh->currentIndex();

  int factor = digi[ID]->IsDualTrace() ? 2 : 1; // if dual trace, 

  unsigned int haha = digi[ID]->GetSettingFromMemory(DPP::RecordLength_G, ch);
  sbReordLength->setValue(haha * DPP::RecordLength_G.GetPartialStep() * ch2ns / factor);

  haha = digi[ID]->GetSettingFromMemory(DPP::PreTrigger, ch);
  sbPreTrigger->setValue(haha * DPP::PreTrigger.GetPartialStep() * ch2ns / factor);

  haha = digi[ID]->GetSettingFromMemory(DPP::ChannelDCOffset, ch);
  sbDCOffset->setValue((1.0 - haha * 1.0 / 0xFFFF) * 100 );

  UpdateComobox(cbDynamicRange, DPP::InputDynamicRange);

  uint32_t DPPAlg = digi[ID]->GetSettingFromMemory(DPP::DPPAlgorithmControl, ch);
  if( Digitizer::ExtractBits(DPPAlg, DPP::Bit_DPPAlgorithmControl_PHA::Polarity) ){
    cbPolarity->setCurrentIndex(1);
  }else{
    cbPolarity->setCurrentIndex(0);
  }

  if( digi[ID]->GetDPPType() == V1730_DPP_PHA_CODE ){
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
    temp = Digitizer::ExtractBits(BdCfg, DPP::Bit_BoardConfig::DigiProbel1);
    for(int i = 0; i < cbDigiProbe1->count(); i++){
      if( cbDigiProbe1->itemData(i).toInt() == temp) {
        cbDigiProbe1->setCurrentIndex(i);
        dataTrace[2]->setName(cbDigiProbe1->currentText());
        break;
      }
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

  UpdatePanelFromMomeory();

}


