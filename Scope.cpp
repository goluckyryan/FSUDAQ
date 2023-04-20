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

  });

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
void Scope::SetUpComboBox(RComboBox * &cb, QString str, int row, int col, const Register::Reg para){
  QLabel * lb = new QLabel(str, settingGroup);
  lb->setAlignment(Qt::AlignRight | Qt::AlignCenter);
  settingLayout->addWidget(lb, row, col);

  cb = new RComboBox(settingGroup);
  settingLayout->addWidget(cb, row, col + 1);
}

void Scope::SetUpSpinBox(RSpinBox * &sb, QString str, int row, int col, const Register::Reg para){
  QLabel * lb = new QLabel(str, settingGroup);
  lb->setAlignment(Qt::AlignRight | Qt::AlignCenter);
  settingLayout->addWidget(lb, row, col);

  sb = new RSpinBox(settingGroup);
  if( para.GetPartialStep() != 0 ){
    sb->setMinimum(0);
    sb->setMaximum(para.GetMax() * para.GetPartialStep() * ch2ns);
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

    uint32_t value = sb->value() / ch2ns / para.GetPartialStep();

    if( para.GetName() == "RecordLength_G" || para.GetName() == "PreTrigger"){
      int factor = digi[ID]->IsDualTrace() ? 2 : 1;
      value = value * factor;
    }

    digiMTX[ID].lock();
    digi[ID]->WriteRegister(para, value, ch);
    digiMTX[ID].unlock();
    if( digi[ID]->GetErrorCode() == CAEN_DGTZ_Success ){
      SendLogMsg(msg + "|OK.");
      sb->setStyleSheet("");
    }else{
      SendLogMsg(msg + "|Fail.");
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

  SetUpSpinBox(sbReordLength, "Record Length [ns]",  0, 0, Register::DPP::RecordLength_G);

  SetUpSpinBox(sbPreTrigger,  "Pre Trigger [ns]", 0, 2, Register::DPP::PreTrigger);

  SetUpSpinBox(sbDCOffset, "DC offset [%]", 0, 4, Register::DPP::ChannelDCOffset);
  sbDCOffset->setDecimals(2);

  SetUpSpinBox(sbInputRiseTime, "Input Rise Time [ns]", 1, 0, Register::DPP::PHA::InputRiseTime);
  SetUpSpinBox(sbThreshold,          "Threshold [LSB]", 1, 2, Register::DPP::PHA::TriggerThreshold);
  SetUpSpinBox(sbTriggerHoldOff,"Trigger HoldOff [ns]", 1, 4, Register::DPP::PHA::TriggerHoldOffWidth);
  
  SetUpSpinBox(sbTrapRiseTime, "Trap. Rise Time [ns]", 2, 0, Register::DPP::PHA::TrapezoidRiseTime);
  SetUpSpinBox(sbTrapFlatTop,    "Trap. FlatTop [ns]", 2, 2, Register::DPP::PHA::TrapezoidFlatTop);
  SetUpSpinBox(sbDecayTime,         "Decay Time [ns]", 2, 4, Register::DPP::PHA::DecayTime);
  SetUpSpinBox(sbPeakingTime,     "Peaking Time [ns]", 2, 6, Register::DPP::PHA::PeakingTime);

}

void Scope::SetUpPSDPanel(){

  SetUpSpinBox(sbReordLength, "Record Length [ns]",  0, 0, Register::DPP::RecordLength_G);
  SetUpSpinBox(sbPreTrigger,  "Pre Trigger [ns]", 0, 2, Register::DPP::PreTrigger);
}

//*=======================================================
//*=======================================================

void Scope::UpdateSpinBox(RSpinBox * &sb, const Register::Reg para){
  int ch = cbScopeCh->currentIndex();

  unsigned int haha = digi[ID]->GetSettingFromMemory(para, ch);
  if( para.GetPartialStep() >   0 ) sb->setValue(haha * para.GetPartialStep() * ch2ns);
  if( para.GetPartialStep() == -1 ) sb->setValue(haha);

}


void Scope::UpdatePanelFromMomeory(){

  int ch = cbScopeCh->currentIndex();

  int factor = digi[ID]->IsDualTrace() ? 2 : 1; // if dual trace, 

  unsigned int haha = digi[ID]->GetSettingFromMemory(Register::DPP::RecordLength_G, ch);
  sbReordLength->setValue(haha * Register::DPP::RecordLength_G.GetPartialStep() * ch2ns / factor);

  haha = digi[ID]->GetSettingFromMemory(Register::DPP::PreTrigger, ch);
  sbPreTrigger->setValue(haha * Register::DPP::PreTrigger.GetPartialStep() * ch2ns / factor);

  haha = digi[ID]->GetSettingFromMemory(Register::DPP::ChannelDCOffset, ch);
  sbDCOffset->setValue((1.0 - haha * 1.0 / 0xFFFF) * 100 );

  UpdateSpinBox(sbInputRiseTime,  Register::DPP::PHA::InputRiseTime);
  UpdateSpinBox(sbThreshold,      Register::DPP::PHA::TriggerThreshold);
  UpdateSpinBox(sbTriggerHoldOff, Register::DPP::PHA::TriggerHoldOffWidth);

  UpdateSpinBox(sbTrapRiseTime, Register::DPP::PHA::TrapezoidRiseTime);
  UpdateSpinBox(sbTrapFlatTop,  Register::DPP::PHA::TrapezoidFlatTop);
  UpdateSpinBox(sbDecayTime,    Register::DPP::PHA::DecayTime);
  UpdateSpinBox(sbPeakingTime,  Register::DPP::PHA::PeakingTime);


}
