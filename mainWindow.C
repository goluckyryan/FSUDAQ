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

    QPushButton * bnOpenDigitizers = new QPushButton("Open Digitizers", this);
    layout->addWidget(bnOpenDigitizers, 0, 0);
    connect(bnOpenDigitizers, &QPushButton::clicked, this, &MainWindow::OpenDigitizers);
    
    QPushButton * bnCloseDigitizers = new QPushButton("Close Digitizers", this);
    layout->addWidget(bnCloseDigitizers, 0, 1);
    connect(bnCloseDigitizers, &QPushButton::clicked, this, &MainWindow::OpenDigitizers);

    QPushButton * bnOpenScope = new QPushButton("Open Scope", this);
    layout->addWidget(bnOpenScope, 1, 0);
    //connect(bnDigiSettings, &QPushButton::clicked, this, &MainWindow::OpenDigiSettings);

    QPushButton * bnDigiSettings = new QPushButton("Digitizers Settings", this);
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
    leDataPath->setEnabled(false);
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
    QLineEdit * leRunID = new QLineEdit(this);
    leRunID->setEnabled(false);

    QPushButton * bnOpenScaler = new QPushButton("Scalar", this);

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

    QPushButton * bnStartACQ = new QPushButton("Start ACQ", this);
    connect( bnStartACQ, &QPushButton::clicked, this, &MainWindow::StartACQ);
    QPushButton * bnStopACQ = new QPushButton("Stop ACQ", this);
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

}

MainWindow::~MainWindow(){

  if( digi ) CloseDigitizers();


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
  }

  LogMsg(QString("Done. Opened %1 digitizer(s).").arg(nDigi));

}

void MainWindow::CloseDigitizers(){

  LogMsg("Closing Digitizer(s)....");

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

void MainWindow::StartACQ(){
  if( digi == nullptr ) return;

  for( unsigned int i = 0; i < nDigi ; i++){
    digi[i]->GetData()->SetOutputFileName("haha");
    digi[i]->StartACQ();
    readDataThread[i]->SetSaveData(true);
    readDataThread[i]->start();
  }

}

void MainWindow::StopACQ(){
  if( digi == nullptr ) return;

  for( unsigned int i = 0; i < nDigi; i++){
    digi[i]->StopACQ();

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
