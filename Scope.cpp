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
    for(int j = 0; j < 100; j ++) dataTrace[i]->append(40*j, QRandomGenerator::global()->bounded(8000) + 8000);
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
  yaxis->setTickCount(6);
  yaxis->setTickInterval(16384);
  yaxis->setRange(-16384, 65536);
  yaxis->setLabelFormat("%.0f");

  xaxis->setRange(0, 5000);
  xaxis->setTickCount(11);
  xaxis->setLabelFormat("%.0f");
  xaxis->setTitleText("Time [ns]");

  updateTraceThread = new UpdateTraceThread();
  //connect(updateTraceThread, &UpdateTraceThread::updateTrace, this, &Scope::UpdateScope);

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
    if( digi[ID]->GetDPPType() == V1730_DPP_PHA_CODE ) {
      SetUpPHAPanel();
    }else{
      CleanUpSettingsGroupBox();
    }

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
  TraceView * plotView = new TraceView(plot);
  plotView->setRenderHints(QPainter::Antialiasing);
  layout->addWidget(plotView, rowID, 0, 1, 6);

  //================ Key binding
  rowID ++;
  QLabel * lbhints = new QLabel("Type 'r' to restore view, '+/-' Zoom in/out, arrow key to pan.", this);
  layout->addWidget(lbhints, rowID, 0, 1, 4);
  
  QLabel * lbinfo = new QLabel("Trace update every " + QString::number(updateTraceThread->GetWaitTimeSec()) + " sec.", this);
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
  bnScopeStart->setEnabled(false);
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


}

void Scope::StopScope(){

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
    //int iDigi = cbScopeDigi->currentIndex();
    if( sb->decimals() == 0 && sb->singleStep() != 1) {
      double step = sb->singleStep();
      double value = sb->value();
      sb->setValue( (std::round(value/step)*step));
    }

    //int ch = cbScopeCh->currentIndex();
    //if( chkSetAllChannel->isChecked() ) ch = -1;
    // QString msg;
    // msg = QString::fromStdString(digPara.GetPara()) + "|DIG:"+ QString::number(digi[iDigi]->GetSerialNumber()) + ",CH:" + (ch == -1 ? "All" : QString::number(ch));
    // msg += " = " + QString::number(sb->value());
    // if( digi[iDigi]->WriteValue(digPara, std::to_string(sb->value()), ch)){
    //   SendLogMsg(msg + "|OK.");
    //   sb->setStyleSheet("");
    //   UpdateSettingsFromMemeory();
    //   UpdateOtherPanels();
    // }else{
    //   SendLogMsg(msg + "|Fail.");
    //   sb->setStyleSheet("color:red;");
    // }
  });  
}

void Scope::CleanUpSettingsGroupBox(){

  printf("-- %s\n", __func__);

  QList<QLabel *> labelChildren1 = settingGroup->findChildren<QLabel *>();
  for( int i = 0; i < labelChildren1.size(); i++) delete labelChildren1[i];
  
  QList<RComboBox *> labelChildren2 = settingGroup->findChildren<RComboBox *>();
  for( int i = 0; i < labelChildren2.size(); i++) delete labelChildren2[i];

  QList<RSpinBox *> labelChildren3 = settingGroup->findChildren<RSpinBox *>();
  for( int i = 0; i < labelChildren3.size(); i++) delete labelChildren3[i];

}
void Scope::SetUpPHAPanel(){

  CleanUpSettingsGroupBox();

  printf("-- %s\n", __func__);

  SetUpSpinBox(sbReordLength, "Record Length [ns]",  0, 0, Register::DPP::RecordLength_G);
  SetUpSpinBox(sbPreTrigger,  "Pre Trigger [ns]", 0, 2, Register::DPP::PreTrigger);

  SetUpSpinBox(sbDCOffset, "DC offset", 0, 4, Register::DPP::ChannelDCOffset);

  SetUpSpinBox(sbInputRiseTime, "Input Rise Time [ns]", 1, 0, Register::DPP::PHA::InputRiseTime);
  SetUpSpinBox(sbThreshold,          "Threshold [LSB]", 1, 2, Register::DPP::PHA::TriggerThreshold);
  SetUpSpinBox(sbThreshold,     "Trigger HoldOff [ns]", 1, 4, Register::DPP::PHA::TriggerHoldOffWidth);
  
  SetUpSpinBox(sbTrapRiseTime, "Trap. Rise Time [ns]", 2, 0, Register::DPP::PHA::TrapezoidRiseTime);
  SetUpSpinBox(sbTrapFlatTop,    "Trap. FlatTop [ns]", 2, 2, Register::DPP::PHA::TrapezoidFlatTop);
  SetUpSpinBox(sbDecayTime,         "Decay Time [ns]", 2, 4, Register::DPP::PHA::DecayTime);
  SetUpSpinBox(sbPeakingTime,     "Peaking Time [ns]", 2, 6, Register::DPP::PHA::PeakingTime);


}

