#include "mainWindow.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QDateTime>
#include <QLabel>

#include <QScrollBar>
#include <QCoreApplication>
#include <QDialog>
#include <QFileDialog>
#include <QScrollArea>

#include <TH1.h>

#include "CustomWidgets.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent){

  setWindowTitle("FSU DAQ");
  setGeometry(500, 100, 1000, 500);

  digi = nullptr;
  nDigi = 0;

  QWidget * mainLayoutWidget = new QWidget(this);
  setCentralWidget(mainLayoutWidget);
  QVBoxLayout * layoutMain = new QVBoxLayout(mainLayoutWidget);
  mainLayoutWidget->setLayout(layoutMain);

  {//^=======================
    QGroupBox * box = new QGroupBox("Digitizer(s)", mainLayoutWidget);
    layoutMain->addWidget(box);
    QGridLayout * layout = new QGridLayout(box);

    bnOpenDigitizers = new QPushButton("Open Digitizers", this);
    layout->addWidget(bnOpenDigitizers, 0, 0);
    connect(bnOpenDigitizers, &QPushButton::clicked, this, &MainWindow::OpenDigitizers);
    
    bnCloseDigitizers = new QPushButton("Close Digitizers", this);
    layout->addWidget(bnCloseDigitizers, 0, 1);
    connect(bnCloseDigitizers, &QPushButton::clicked, this, &MainWindow::CloseDigitizers);

    bnOpenScope = new QPushButton("Open Scope", this);
    layout->addWidget(bnOpenScope, 1, 0);
    //connect(bnDigiSettings, &QPushButton::clicked, this, &MainWindow::OpenDigiSettings);

    bnDigiSettings = new QPushButton("Digitizers Settings", this);
    layout->addWidget(bnDigiSettings, 1, 1);
    //connect(bnDigiSettings, &QPushButton::clicked, this, &MainWindow::OpenDigiSettings);

  }

  {//^====================== ACQ control
    QGroupBox * box = new QGroupBox("ACQ Control", mainLayoutWidget);
    layoutMain->addWidget(box);
    QGridLayout * layout = new QGridLayout(box);

    int rowID = 0;
    QLabel * lbDataPath = new QLabel("Data Path : ", this);
    lbDataPath->setAlignment(Qt::AlignRight | Qt::AlignCenter);
    leDataPath = new QLineEdit(this);
    leDataPath->setReadOnly(true);
    QPushButton * bnSetDataPath = new QPushButton("Set Path", this);
    connect(bnSetDataPath, &QPushButton::clicked, this, &MainWindow::OpenDataPath);

    layout->addWidget(lbDataPath, rowID, 0);
    layout->addWidget(leDataPath, rowID, 1, 1, 3);
    layout->addWidget(bnSetDataPath, rowID, 4);

    rowID ++;
    QLabel * lbPrefix = new QLabel("Prefix : ", this);
    lbPrefix->setAlignment(Qt::AlignRight | Qt::AlignCenter);
    lePrefix = new QLineEdit(this);

    QLabel * lbRunID = new QLabel("Run No. :", this);
    lbRunID->setAlignment(Qt::AlignRight | Qt::AlignCenter);
    leRunID = new QLineEdit(this);
    leRunID->setReadOnly(true);

    bnOpenScaler = new QPushButton("Scalar", this);
    connect(bnOpenScaler, &QPushButton::clicked, this, &MainWindow::OpenScalar);

    layout->addWidget(lbPrefix, rowID, 0);
    layout->addWidget(lePrefix, rowID, 1);
    layout->addWidget(lbRunID, rowID, 2);
    layout->addWidget(leRunID, rowID, 3);
    layout->addWidget(bnOpenScaler, rowID, 4);

    rowID ++;
    QLabel * lbComment = new QLabel("Run Comment : ", this);
    lbComment->setAlignment(Qt::AlignRight | Qt::AlignCenter);

    leComment = new QLineEdit(this);
    leComment->setEnabled(false);

    bnStartACQ = new QPushButton("Start ACQ", this);
    connect( bnStartACQ, &QPushButton::clicked, this, &MainWindow::StartACQ);
    bnStopACQ = new QPushButton("Stop ACQ", this);
    connect( bnStopACQ, &QPushButton::clicked, this, &MainWindow::StopACQ);

    layout->addWidget(lbComment, rowID, 0);
    layout->addWidget(leComment, rowID, 1, 1, 2);
    layout->addWidget(bnStartACQ, rowID, 3);
    layout->addWidget(bnStopACQ, rowID, 4);

    layout->setColumnStretch(0, 1);
    layout->setColumnStretch(1, 2);
    layout->setColumnStretch(2, 1);
    layout->setColumnStretch(3, 3);
    layout->setColumnStretch(4, 3);

  }


  {//^===================== Log Msg

    logMsgHTMLMode = true;
    QGroupBox * box3 = new QGroupBox("Log Message", mainLayoutWidget);
    layoutMain->addWidget(box3);
    layoutMain->setStretchFactor(box3, 1);
    QVBoxLayout * layout3 = new QVBoxLayout(box3);
    logInfo = new QPlainTextEdit(this);
    logInfo->isReadOnly();
    QFont font; 
    font.setFamily("Courier New");
    logInfo->setFont(font);
    layout3->addWidget(logInfo);

  }

  LogMsg("<font style=\"color: blue;\"><b>Welcome to FSU DAQ.</b></font>");

  rawDataPath = "";
  prefix = "temp";
  runID = 0;
  elogID = 0;
  programSettingsFilePath = QDir::current().absolutePath() + "/programSettings.txt";
  LoadProgramSettings();

  {
    scalar = new QMainWindow(this);
    scalar->setWindowTitle("Scalar");

    QScrollArea * scopeScroll = new QScrollArea(scalar);
    scalar->setCentralWidget(scopeScroll);
    scopeScroll->setWidgetResizable(true);
    scopeScroll->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QWidget * layoutWidget = new QWidget(scalar);
    scopeScroll->setWidget(layoutWidget);

    scalarLayout = new QGridLayout(layoutWidget);
    scalarLayout->setSpacing(0);
    scalarLayout->setAlignment(Qt::AlignTop);

    leTrigger = nullptr;
    leAccept = nullptr;

    lbLastUpdateTime = nullptr;
    lbScalarACQStatus = nullptr;

    nScalarBuilt = 0;

    scalarThread = new ScalarThread();
    //connect(scalarThread, &ScalarThread::updataScalar, this, &MainWindow::UpdateScalar);

  }
}

MainWindow::~MainWindow(){

  if( digi ) CloseDigitizers();
  SaveProgramSettings();

}

//***************************************************************
//***************************************************************
void MainWindow::OpenDataPath(){

  QFileDialog fileDialog(this);
  fileDialog.setFileMode(QFileDialog::Directory);
  int result = fileDialog.exec();

  //qDebug() << fileDialog.selectedFiles();
  if( result > 0 ) {
    leDataPath->setText(fileDialog.selectedFiles().at(0));
  }else{
    leDataPath->clear();
  }

}

void MainWindow::LoadProgramSettings(){

  LogMsg("Loading <b>" + programSettingsFilePath + "</b> for Program Settings.");
  QFile file(programSettingsFilePath);

  if( !file.open(QIODevice::Text | QIODevice::ReadOnly) ) {
    LogMsg("<b>" + programSettingsFilePath + "</b> not found.");
  }else{

    QTextStream in(&file);
    QString line = in.readLine();

    int count = 0;
    while( !line.isNull()){
      if( line.left(6) == "//----") break;

      if( count == 0 ) rawDataPath = line;
      count ++;
      line = in.readLine();
    }

    //looking for the lastRun.sh for 
    leDataPath->setText(rawDataPath);
    LoadLastRunFile();
  }

}

void MainWindow::SaveProgramSettings(){

  rawDataPath = leDataPath->text();

  QFile file(programSettingsFilePath);
  
  file.open(QIODevice::Text | QIODevice::WriteOnly);

  file.write((rawDataPath+"\n").toStdString().c_str());
  file.write("//------------end of file.");
  
  file.close();
  LogMsg("Saved program settings to <b>"+ programSettingsFilePath + "<b>.");

}

void MainWindow::LoadLastRunFile(){

  QFile file(rawDataPath + "/lastRun.sh");

  if( !file.open(QIODevice::Text | QIODevice::ReadOnly) ) {
    LogMsg("<b>" + rawDataPath + "/lastRun.sh</b> not found.");
    runID = 0;
    prefix = "temp";
    leRunID->setText(QString::number(runID));
    lePrefix->setText(prefix);
  }else{

    QTextStream in(&file);
    QString line = in.readLine();

    int count = 0;
    while( !line.isNull()){

      int index = line.indexOf("=");
      QString haha = line.mid(index+1).remove(" ");

      //qDebug() << haha;

      switch (count){
        case 0 : prefix = haha; break;
        case 1 : runID = haha.toInt(); break;
        case 2 : elogID = haha.toInt(); break;
      }

      count ++;
      line = in.readLine();
    }

    lePrefix->setText(prefix);
    leRunID->setText(QString::number(runID));

  }

}

void MainWindow::SaveLastRunFile(){

  QFile file(rawDataPath + "/lastRun.sh");

  file.open(QIODevice::Text | QIODevice::WriteOnly);

  file.write(("prefix=" + prefix + "\n").toStdString().c_str());
  file.write(("runID=" + QString::number(runID) + "\n").toStdString().c_str());
  file.write(("elogID=" + QString::number(elogID) + "\n").toStdString().c_str());
  file.write("//------------end of file.");
  
  file.close();
  LogMsg("Saved program settings to <b>"+ rawDataPath + "/lastRun.sh<b>.");

}

//***************************************************************
//***************************************************************
void MainWindow::OpenDigitizers(){

  //sereach for digitizers
  LogMsg("Searching digitizers via optical link.....Please wait");
  logMsgHTMLMode = false;
  nDigi = 0;
  std::vector<std::pair<int, int>> portList; //boardID, portID
  Digitizer dig;
  for(int port = 0; port < MaxNPorts; port++){
    for( int board = 0; board < MaxNBoards; board ++){ /// max number of iasy chain
      dig.OpenDigitizer(board, port);
      if( dig.IsConnected() ){
        nDigi++;
        portList.push_back(std::pair(board, port));
        LogMsg(QString("... Found at port: %1, board: %2. SN: %3 %4").arg(port).arg(board).arg(dig.GetSerialNumber(), 3, 10, QChar(' ')).arg(dig.GetDPPString().c_str()));
        dig.CloseDigitizer();
        QCoreApplication::processEvents(); //to prevent Qt said application not responding.
      }//else{
        //LogMsg(QString("... Nothing at port: %1, board: %2.").arg(port).arg(board));
      //}
    }
  }
  LogMsg(QString("Done seraching. Found %1 digitizer(s). Opening digitizer(s)....").arg(nDigi));
  logMsgHTMLMode = true;

  digi = new Digitizer * [nDigi];
  readDataThread = new ReadDataThread * [nDigi];
  for( unsigned int i = 0; i < nDigi; i++){
    digi[i] = new Digitizer(portList[i].first, portList[i].second);
    readDataThread[i] = new ReadDataThread(digi[i], i);
    connect(readDataThread[i], &ReadDataThread::sendMsg, this, &MainWindow::LogMsg);
  }

  LogMsg(QString("Done. Opened %1 digitizer(s).").arg(nDigi));

  SetupScalar();

}

void MainWindow::CloseDigitizers(){

  LogMsg("Closing Digitizer(s)....");

  CleanUpScalar();

  for(unsigned int i = 0; i < nDigi; i ++){
    digi[i]->CloseDigitizer();
    delete digi[i];
    digi[i] = nullptr;

    delete readDataThread[i];
  }
  delete [] digi;
  digi = nullptr;

  delete [] readDataThread;
  readDataThread = nullptr;

  LogMsg("Done. Closed " + QString::number(nDigi) + " Digitizer(s).");
  nDigi = 0;


}

//***************************************************************
//***************************************************************
void MainWindow::SetupScalar(){

  scalar->setGeometry(0, 0, 10 + nDigi * 180, 110 + MaxNChannels * 20);

  if( lbLastUpdateTime == nullptr ){
    lbLastUpdateTime = new QLabel("Last update : NA", scalar);
    lbScalarACQStatus = new QLabel("ACQ status", scalar);
  }
  
  lbLastUpdateTime->setAlignment(Qt::AlignCenter);
  scalarLayout->removeWidget(lbLastUpdateTime);
  scalarLayout->addWidget(lbLastUpdateTime, 0, 1, 1, 1 + nDigi);

  lbScalarACQStatus->setAlignment(Qt::AlignCenter);
  scalarLayout->removeWidget(lbScalarACQStatus);
  scalarLayout->addWidget(lbScalarACQStatus, 1, 1, 1, 1 + nDigi);

  int rowID = 3;
  if( nScalarBuilt == 0 ){
    ///==== create the header row
    for( int ch = 0; ch < MaxNChannels; ch++){

      if( ch == 0 ){
        QLabel * lbCH_H = new QLabel("Ch", scalar); 
        scalarLayout->addWidget(lbCH_H, rowID, 0);
      }  

      rowID ++;
      QLabel * lbCH = new QLabel(QString::number(ch), scalar);
      lbCH->setAlignment(Qt::AlignCenter);
      scalarLayout->addWidget(lbCH, rowID, 0);
    }
  }

  ///===== create the trigger and accept
  leTrigger = new QLineEdit**[nDigi];
  leAccept = new QLineEdit**[nDigi];
  lbDigi    = new QLabel * [nDigi];
  lbTrigger = new QLabel * [nDigi];
  lbAccept  = new QLabel * [nDigi];
  for( unsigned int iDigi = 0; iDigi < nDigi; iDigi++){
    rowID = 2;
    leTrigger[iDigi] = new QLineEdit *[digi[iDigi]->GetNChannels()];
    leAccept[iDigi] = new QLineEdit *[digi[iDigi]->GetNChannels()];

    lbDigi[iDigi] = new QLabel("Digi-" + QString::number(digi[iDigi]->GetSerialNumber()), scalar); 
    lbDigi[iDigi]->setAlignment(Qt::AlignCenter);
    scalarLayout->addWidget(lbDigi[iDigi], rowID, 2*iDigi+1, 1, 2);

    rowID ++;

    lbTrigger[iDigi] = new QLabel("Trig. [Hz]", scalar);
    lbTrigger[iDigi]->setAlignment(Qt::AlignCenter);
    scalarLayout->addWidget(lbTrigger[iDigi], rowID, 2*iDigi+1);
    lbAccept[iDigi] = new QLabel("Accp. [Hz]", scalar);
    lbAccept[iDigi]->setAlignment(Qt::AlignCenter);
    scalarLayout->addWidget(lbAccept[iDigi], rowID, 2*iDigi+2);

    for( int ch = 0; ch < MaxNChannels; ch++){
   
      rowID ++;

      leTrigger[iDigi][ch] = new QLineEdit(scalar);
      leTrigger[iDigi][ch]->setReadOnly(true);
      leTrigger[iDigi][ch]->setAlignment(Qt::AlignRight);
      scalarLayout->addWidget(leTrigger[iDigi][ch], rowID, 2*iDigi+1);

      leAccept[iDigi][ch] = new QLineEdit(scalar);
      leAccept[iDigi][ch]->setReadOnly(true);
      leAccept[iDigi][ch]->setAlignment(Qt::AlignRight);
      leAccept[iDigi][ch]->setStyleSheet("background-color: #F0F0F0;");
      scalarLayout->addWidget(leAccept[iDigi][ch], rowID, 2*iDigi+2);
    }
  }

}

void MainWindow::CleanUpScalar(){

  scalar->close();

  if( leTrigger == nullptr ) return;

  for( unsigned int i = 0; i < nDigi; i++){
    for( int ch = 0; ch < digi[i]->GetNChannels(); ch ++){
      delete leTrigger[i][ch];
      delete leAccept[i][ch];
    }
    delete [] leTrigger[i];
    delete [] leAccept[i];

    delete lbDigi[i];
    delete lbTrigger[i];
    delete lbAccept[i];
  }
  delete [] leTrigger;
  delete [] lbDigi;
  delete [] lbTrigger;
  delete [] lbAccept;
  leTrigger = nullptr;
  leAccept = nullptr;
  lbDigi = nullptr;
  lbTrigger = nullptr;
  lbAccept = nullptr;

  delete lbLastUpdateTime;
  delete lbScalarACQStatus;

  lbLastUpdateTime = nullptr;
  lbScalarACQStatus = nullptr;

  printf("---- end of %s \n", __func__);

}

void MainWindow::OpenScalar(){
  scalar->show();
}

void MainWindow::UpdateScalar(){


}

//***************************************************************
//***************************************************************

void MainWindow::StartACQ(){
  if( digi == nullptr ) return;

  for( unsigned int i = 0; i < nDigi ; i++){
    digi[i]->GetData()->OpenSaveFile((rawDataPath + "/" + prefix).toStdString());
    digi[i]->StartACQ();
    readDataThread[i]->SetSaveData(true);
    readDataThread[i]->start();
  }

  SaveLastRunFile();

}

void MainWindow::StopACQ(){
  if( digi == nullptr ) return;

  for( unsigned int i = 0; i < nDigi; i++){
    digi[i]->StopACQ();
    digi[i]->GetData()->CloseSaveFile();

    if( readDataThread[i]->isRunning() ) {
      readDataThread[i]->quit();
      readDataThread[i]->wait();
    }
  }
  
}

//***************************************************************
//***************************************************************
void MainWindow::LogMsg(QString msg){
  QString outputStr = QStringLiteral("[%1] %2").arg(QDateTime::currentDateTime().toString("MM.dd hh:mm:ss"), msg);
  if( logMsgHTMLMode ){ 
    logInfo->appendHtml(outputStr);
  }else{
    logInfo->appendPlainText(outputStr);
  }
  QScrollBar *v = logInfo->verticalScrollBar();
  v->setValue(v->maximum());
  //qDebug() << msg;
  logInfo->repaint();
}
