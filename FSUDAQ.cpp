#include "FSUDAQ.h"

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

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent){

  setWindowTitle("FSU DAQ");
  setGeometry(500, 100, 1100, 500);

  digi = nullptr;
  nDigi = 0;

  scalar = nullptr;
  scope = nullptr;
  digiSettings = nullptr;
  canvas = nullptr;

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
    connect(bnOpenScope, &QPushButton::clicked, this, &MainWindow::OpenScope);

    bnDigiSettings = new QPushButton("Digitizers Settings", this);
    layout->addWidget(bnDigiSettings, 1, 1);
    connect(bnDigiSettings, &QPushButton::clicked, this, &MainWindow::OpenDigiSettings);

    bnCanvas = new QPushButton("Canvas", this);
    layout->addWidget(bnCanvas, 2, 0);
    connect(bnCanvas, &QPushButton::clicked, this, &MainWindow::OpenCanvas);

  }

  {//^====================== ACQ control
    QGroupBox * box = new QGroupBox("ACQ Control", mainLayoutWidget);
    layoutMain->addWidget(box);
    QGridLayout * layout = new QGridLayout(box);

    int rowID = 0;
    //------------------------------------------
    QLabel * lbDataPath = new QLabel("Data Path : ", this);
    lbDataPath->setAlignment(Qt::AlignRight | Qt::AlignCenter);
    leDataPath = new QLineEdit(this);
    leDataPath->setReadOnly(true);
    QPushButton * bnSetDataPath = new QPushButton("Set Path", this);
    connect(bnSetDataPath, &QPushButton::clicked, this, &MainWindow::OpenDataPath);

    layout->addWidget(lbDataPath, rowID, 0);
    layout->addWidget(leDataPath, rowID, 1, 1, 6);
    layout->addWidget(bnSetDataPath, rowID, 7);

    //------------------------------------------
    rowID ++;
    QLabel * lbPrefix = new QLabel("Prefix : ", this);
    lbPrefix->setAlignment(Qt::AlignRight | Qt::AlignCenter);
    lePrefix = new QLineEdit(this);
    lePrefix->setAlignment(Qt::AlignHCenter);

    QLabel * lbRunID = new QLabel("Run No. :", this);
    lbRunID->setAlignment(Qt::AlignRight | Qt::AlignCenter);
    leRunID = new QLineEdit(this);
    leRunID->setReadOnly(true);
    leRunID->setAlignment(Qt::AlignHCenter);

    chkSaveData = new QCheckBox("Save Data", this);
    cbAutoRun = new RComboBox(this);
    cbAutoRun->addItem("Single Infinite", 0);
    cbAutoRun->addItem("Single 30 mins", 30);
    cbAutoRun->addItem("Single 60 mins", 60);
    cbAutoRun->addItem("Repeat 60 mins", -60);
    cbAutoRun->setEnabled(false);

    bnStartACQ = new QPushButton("Start ACQ", this);
    connect( bnStartACQ, &QPushButton::clicked, this, &MainWindow::StartACQ);
    bnStopACQ = new QPushButton("Stop ACQ", this);
    connect( bnStopACQ, &QPushButton::clicked, this, &MainWindow::StopACQ);

    layout->addWidget(lbPrefix, rowID, 0);
    layout->addWidget(lePrefix, rowID, 1);
    layout->addWidget(lbRunID, rowID, 2);
    layout->addWidget(leRunID, rowID, 3);
    layout->addWidget(chkSaveData, rowID, 4);
    layout->addWidget(cbAutoRun, rowID, 5);
    layout->addWidget(bnStartACQ, rowID, 6);
    layout->addWidget(bnStopACQ, rowID, 7);

    //------------------------------------------
    rowID ++;
    QLabel * lbComment = new QLabel("Run Comment : ", this);
    lbComment->setAlignment(Qt::AlignRight | Qt::AlignCenter);

    leComment = new QLineEdit(this);
    leComment->setReadOnly(true);

    bnOpenScaler = new QPushButton("Scalar", this);
    connect(bnOpenScaler, &QPushButton::clicked, this, &MainWindow::OpenScalar);

    layout->addWidget(lbComment, rowID, 0);
    layout->addWidget(leComment, rowID, 1, 1, 6);

    layout->addWidget(bnOpenScaler, rowID, 7);

    layout->setColumnStretch(0, 1);
    layout->setColumnStretch(1, 2);
    layout->setColumnStretch(2, 1);
    layout->setColumnStretch(3, 1);
    layout->setColumnStretch(4, 1);
    layout->setColumnStretch(5, 1);
    layout->setColumnStretch(6, 3);
    layout->setColumnStretch(7, 3);

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

  //=========== disable widget
  WaitForDigitizersOpen(true);

}

MainWindow::~MainWindow(){

  if( digi ) CloseDigitizers();
  SaveProgramSettings();

  if( scope ) delete scope;

  if( digiSettings ) delete digiSettings;

  if( canvas ) delete canvas;

  if( scalar ) {
    CleanUpScalar();
    scalarThread->Stop();
    scalarThread->quit();
    scalarThread->exit();
    delete scalarThread;
  }

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

    //check is rawDataPath exist, if not, create one
    QDir rawDataDir;
    if( !rawDataDir.exists(rawDataPath ) ) {
      if( rawDataDir.mkdir(rawDataPath) ){
          LogMsg("Created folder <b>" + rawDataPath + "</b> for storing root files.");
      }else{
          LogMsg("<font style=\"color:red;\"><b>" + rawDataPath + "</b> cannot be created. Access right problem? </font>" );
      }
      }else{
        LogMsg("<b>" + rawDataPath + "</b> already exist." );
    }
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
  for(int port = 0; port < MaxNPorts; port++){
    for( int board = 0; board < MaxNBoards; board ++){ /// max number of iasy chain
      Digitizer dig;
      dig.OpenDigitizer(board, port);
      if( dig.IsConnected() ){
        nDigi++;
        portList.push_back(std::pair(board, port));
        LogMsg(QString("... Found at port: %1, board: %2. SN: %3 %4").arg(port).arg(board).arg(dig.GetSerialNumber(), 3, 10, QChar(' ')).arg(dig.GetDPPString().c_str()));
      }//else{
        //LogMsg(QString("... Nothing at port: %1, board: %2.").arg(port).arg(board));
      //}
      dig.CloseDigitizer();
      QCoreApplication::processEvents(); //to prevent Qt said application not responding.
    }
  }
  logMsgHTMLMode = true;

  if( nDigi == 0 ) {
    LogMsg(QString("Done seraching. No digitizer found."));
    return;
  }else{
    LogMsg(QString("Done seraching. Found %1 digitizer(s). Opening digitizer(s)....").arg(nDigi));
  }
  
  digi = new Digitizer * [nDigi];
  readDataThread = new ReadDataThread * [nDigi];
  for( unsigned int i = 0; i < nDigi; i++){
    digi[i] = new Digitizer(portList[i].first, portList[i].second);
    //digi[i]->Reset();

    ///============== load settings 
    QString fileName = rawDataPath + "/Digi-" + QString::number(digi[i]->GetSerialNumber()) + "_" + QString::fromStdString(digi[i]->GetData()->DPPTypeStr) + ".bin";
    QFile file(fileName);
    if( !file.open(QIODevice::Text | QIODevice::ReadOnly) ) {

      if( digi[i]->GetDPPType() == V1730_DPP_PHA_CODE ) {
        //digi[i]->ProgramPHABoard();
        //LogMsg("<b>" + fileName + "</b> not found. Program predefined PHA settings.");
        LogMsg("<b>" + fileName + "</b> not found.");
      }

      if( digi[i]->GetDPPType() == V1730_DPP_PSD_CODE ){
        //digi[i]->ProgramPSDBoard();
        //LogMsg("<b>" + fileName + "</b> not found. Program predefined PSD settings.");
        LogMsg("<b>" + fileName + "</b> not found.");
      }

    }else{
      LogMsg("Found <b>" + fileName + "</b> for digitizer settings.");
      
      if( digi[i]->LoadSettingBinaryToMemory(fileName.toStdString().c_str()) == 0 ){
        LogMsg("Loaded settings file <b>" + fileName + "</b> for Digi-" + QString::number(digi[i]->GetSerialNumber()));
        digi[i]->ProgramSettingsToBoard();
        
      }else{
        LogMsg("Fail to Loaded settings file " + fileName + " for Digi-" + QString::number(digi[i]->GetSerialNumber()));
      }
    }    
    digi[i]->ReadAllSettingsFromBoard(true);

    readDataThread[i] = new ReadDataThread(digi[i], i);
    connect(readDataThread[i], &ReadDataThread::sendMsg, this, &MainWindow::LogMsg);
    
    QCoreApplication::processEvents(); //to prevent Qt said application not responding.
  }

  LogMsg(QString("Done. Opened %1 digitizer(s).").arg(nDigi));

  WaitForDigitizersOpen(false);
  bnStopACQ->setEnabled(false);

  SetupScalar();

}

void MainWindow::CloseDigitizers(){

  LogMsg("Closing Digitizer(s)....");

  CleanUpScalar();

  if( scope ) {
    scope->close();
    delete scope;
    scope = nullptr;
  }

  if( digiSettings ){
    digiSettings->close();
    delete digiSettings;
    digiSettings = nullptr;
  }

  if( digi == nullptr ) return;

  for(unsigned int i = 0; i < nDigi; i ++){
    digi[i]->CloseDigitizer();
    delete digi[i];
    delete readDataThread[i];
  }
  delete [] digi;
  digi = nullptr;

  delete [] readDataThread;
  readDataThread = nullptr;

  LogMsg("Done. Closed " + QString::number(nDigi) + " Digitizer(s).");
  nDigi = 0;

  WaitForDigitizersOpen(true);

}

void MainWindow::WaitForDigitizersOpen(bool onOff){

  bnOpenDigitizers->setEnabled(onOff);
  bnCloseDigitizers->setEnabled(!onOff);
  bnOpenScope->setEnabled(!onOff);
  bnDigiSettings->setEnabled(!onOff);
  bnOpenScaler->setEnabled(!onOff);
  bnStartACQ->setEnabled(!onOff);
  bnStopACQ->setEnabled(!onOff);
  chkSaveData->setEnabled(!onOff);

}

//***************************************************************
//***************************************************************
void MainWindow::SetupScalar(){

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

  scalarThread = new TimingThread();
  connect(scalarThread, &TimingThread::timeUp, this, &MainWindow::UpdateScalar);

  scalar->setGeometry(0, 0, 10 + nDigi * 200, 110 + MaxNChannels * 25);

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

  ///===== create the trigger and accept
  leTrigger = new QLineEdit**[nDigi];
  leAccept = new QLineEdit**[nDigi];
  for( unsigned int iDigi = 0; iDigi < nDigi; iDigi++){
    rowID = 2;
    leTrigger[iDigi] = new QLineEdit *[digi[iDigi]->GetNChannels()];
    leAccept[iDigi] = new QLineEdit *[digi[iDigi]->GetNChannels()];
    for( int ch = 0; ch < MaxNChannels; ch++){

      if( ch == 0 ){
          QLabel * lbDigi = new QLabel("Digi-" + QString::number(digi[iDigi]->GetSerialNumber()), scalar); 
          lbDigi->setAlignment(Qt::AlignCenter);
          scalarLayout->addWidget(lbDigi, rowID, 2*iDigi+1, 1, 2);

          rowID ++;

          QLabel * lbA = new QLabel("Trig. [Hz]", scalar);
          lbA->setAlignment(Qt::AlignCenter);
          scalarLayout->addWidget(lbA, rowID, 2*iDigi+1);
          QLabel * lbB = new QLabel("Accp. [Hz]", scalar);
          lbB->setAlignment(Qt::AlignCenter);
          scalarLayout->addWidget(lbB, rowID, 2*iDigi+2);
      }
    
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

  if( scalar == nullptr) return;

  scalar->close();

  if( leTrigger == nullptr ) return;

  for( unsigned int i = 0; i < nDigi; i++){
    for( int ch = 0; ch < digi[i]->GetNChannels(); ch ++){
      delete leTrigger[i][ch];
      delete leAccept[i][ch];
    }
    delete [] leTrigger[i];
    delete [] leAccept[i];

  }
  delete [] leTrigger;
  leTrigger = nullptr;
  leAccept = nullptr;

  //Clean up QLabel
  QList<QLabel *> labelChildren = scalar->findChildren<QLabel *>();
  for( int i = 0; i < labelChildren.size(); i++) delete labelChildren[i];

  printf("---- end of %s \n", __func__);

}

void MainWindow::OpenScalar(){
  scalar->show();
}

void MainWindow::UpdateScalar(){
  if( digi == nullptr ) return;
  if( scalar == nullptr ) return;
  if( !scalar->isVisible() ) return;
  
  lbLastUpdateTime->setText("Last update: " + QDateTime::currentDateTime().toString("MM.dd hh:mm:ss"));

  for( unsigned int iDigi = 0; iDigi < nDigi; iDigi++){
    Data * data = digi[iDigi]->GetData();

    for( int i = 0; i < digi[iDigi]->GetNChannels(); i++){
      leTrigger[iDigi][i]->setText(QString::number(data->TriggerRate[i]));
      leAccept[iDigi][i]->setText(QString::number(data->NonPileUpRate[i]));
    }
  }

}

//***************************************************************
//***************************************************************

void MainWindow::StartACQ(){
  if( digi == nullptr ) return;

  bool commentResult = true;
  if( chkSaveData->isChecked() ) commentResult = CommentDialog(true);
  if( commentResult == false) return;

  LogMsg("===================== Start a new Run-" + QString::number(runID));

  for( unsigned int i = 0; i < nDigi ; i++){
    if( chkSaveData->isChecked() ) {
      if( digi[i]->GetData()->OpenSaveFile((rawDataPath + "/" + prefix + "_" + QString::number(runID).rightJustified(3, '0')).toStdString()) == false ) {
        LogMsg("Cannot open save file : " + QString::fromStdString(digi[i]->GetData()->GetOutFileName() ) + ". Probably read-only?");
       continue; 
      };
    }
    readDataThread[i]->SetSaveData(chkSaveData->isChecked());
    LogMsg("Digi-" + QString::number(digi[i]->GetSerialNumber()) + " is starting ACQ." );
    digi[i]->StartACQ();
    readDataThread[i]->start();
  }
  if( chkSaveData->isChecked() ) SaveLastRunFile();
  scalarThread->start();

  if( !scalar->isVisible() ) {
    scalar->show();
  }else{
    scalar->activateWindow();
  }
  lbScalarACQStatus->setText("<font style=\"color: green;\"><b>ACQ On</b></font>");

  bnStartACQ->setEnabled(false);
  bnStopACQ->setEnabled(true);
  bnOpenScope->setEnabled(false);
}

void MainWindow::StopACQ(){
  if( digi == nullptr ) return;

  LogMsg("===================== Stop Run-" + QString::number(runID));

  bool commentResult = true;
  if( chkSaveData->isChecked() ) commentResult = CommentDialog(true);
  if( commentResult == false) return;

  for( unsigned int i = 0; i < nDigi; i++){
    LogMsg("Digi-" + QString::number(digi[i]->GetSerialNumber()) + " is stoping ACQ." );
    digi[i]->StopACQ();
    if( chkSaveData->isChecked() ) digi[i]->GetData()->CloseSaveFile();

    if( readDataThread[i]->isRunning() ) {
      readDataThread[i]->quit();
      readDataThread[i]->wait();
    }
  }

  if( scalarThread->isRunning()){
    scalarThread->Stop();
    scalarThread->quit();
    scalarThread->wait();
  }
  
  lbScalarACQStatus->setText("<font style=\"color: red;\"><b>ACQ Off</b></font>");

  bnStartACQ->setEnabled(true);
  bnStopACQ->setEnabled(false);
  bnOpenScope->setEnabled(true);
}

void MainWindow::AutoRun(){

}

bool MainWindow::CommentDialog(bool isStartRun){

  if( isStartRun ) runID ++;
  QString runIDStr = QString::number(runID).rightJustified(3, '0');

  QDialog * dOpen = new QDialog(this);
  if( isStartRun ) {
    dOpen->setWindowTitle("Start Run Comment");
  }else{
    dOpen->setWindowTitle("Stop Run Comment");
  }
  dOpen->setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
  dOpen->setMinimumWidth(600);
  connect(dOpen, &QDialog::finished, dOpen, &QDialog::deleteLater);

  QGridLayout * vlayout = new QGridLayout(dOpen);
  QLabel *label = new QLabel("Enter Run comment for <font style=\"color : red;\">Run-" +  runIDStr + "</font> : ", dOpen);
  QLineEdit *lineEdit = new QLineEdit(dOpen);
  QPushButton *button1 = new QPushButton("OK", dOpen);
  QPushButton *button2 = new QPushButton("Cancel", dOpen);

  vlayout->addWidget(label, 0, 0, 1, 2);
  vlayout->addWidget(lineEdit, 1, 0, 1, 2);
  vlayout->addWidget(button1, 2, 0);
  vlayout->addWidget(button2, 2, 1);

  connect(button1, &QPushButton::clicked, dOpen, &QDialog::accept);
  connect(button2, &QPushButton::clicked, dOpen, &QDialog::reject);
  int result = dOpen->exec();

  if(result == QDialog::Accepted ){

    if( isStartRun ){
      startComment = lineEdit->text();
      if( startComment == "") startComment = "No commet was typed.";
      startComment = "Start Comment: " + startComment;
      leComment->setText(startComment);
      leRunID->setText(QString::number(runID));
    }else{
      stopComment = lineEdit->text();
      if( stopComment == "") stopComment = "No commet was typed.";
      stopComment = "Stop Comment: " + stopComment;
      leComment->setText(stopComment);
    }
    
  }else{

    if( isStartRun ){
      LogMsg("Start Run aborted. ");
      runID --;
      leRunID->setText(QString::number(runID));
    }else{
      LogMsg("Stop Run cancelled. ");
    }
    return false;

  }

  return true;

}

void MainWindow::WriteRunTimestamp(bool isStartRun){

  QFile file(rawDataPath + "/RunTimeStamp.dat");
  
  QString dateTime = QDateTime::currentDateTime().toString("yyyy.MM.dd hh:mm:ss");
  if( file.open(QIODevice::Text | QIODevice::WriteOnly | QIODevice::Append) ){

    if( isStartRun ){
      file.write(("Start Run | " + QString::number(runID) + " | " + dateTime + " | " + startComment + "\n").toStdString().c_str());
    }else{
      file.write((" Stop Run | " + QString::number(runID) + " | " + dateTime + " | " + stopComment + "\n").toStdString().c_str());
    }
    
    file.close();
  }


  QFile fileCSV(rawDataPath +  "/RunTimeStamp.csv");

  if( fileCSV.open(QIODevice::Text | QIODevice::WriteOnly | QIODevice::Append) ){

    QTextStream out(&fileCSV);

    if( isStartRun){
      out << QString::number(runID) + "," + dateTime + "," + startComment;
    }else{
      out << "," + dateTime + "," + stopComment + "\n";
    }

    fileCSV.close();
  }

}

//***************************************************************
//***************************************************************
void MainWindow::OpenScope(){

  if( scope == nullptr ) {
    scope = new Scope(digi, nDigi, readDataThread);
    connect(scope, &Scope::SendLogMsg, this, &MainWindow::LogMsg);
    connect(scope, &Scope::CloseWindow, this, [=](){
      bnStartACQ->setEnabled(true);
      bnStopACQ->setEnabled(false);  
    });
    connect(scope, &Scope::TellACQOnOff, this, [=](bool onOff){
      if( scope == nullptr ) return;
      if( onOff ) {
        lbScalarACQStatus->setText("<font style=\"color: green;\"><b>ACQ On</b></font>");
      }else{
        lbScalarACQStatus->setText("<font style=\"color: red;\"><b>ACQ Off</b></font>");
      }
    });

    connect(scope, &Scope::UpdateScaler, this, &MainWindow::UpdateScalar);
    scope->show();
  }else{
    scope->show();
    scope->activateWindow();
  }

  bnStartACQ->setEnabled(false);
  bnStopACQ->setEnabled(false);  

}

//***************************************************************
//***************************************************************
void MainWindow::OpenDigiSettings(){

  if( digiSettings == nullptr ) {
    digiSettings = new DigiSettingsPanel(digi, nDigi, rawDataPath);
    //connect(scope, &Scope::SendLogMsg, this, &MainWindow::LogMsg);
    digiSettings->show();
  }else{
    digiSettings->show();
    digiSettings->activateWindow();
  }

}

//***************************************************************
//***************************************************************
void MainWindow::OpenCanvas(){

  if( canvas == nullptr ) {
    canvas = new Canvas(digi, nDigi);
    canvas->show();
  }else{
    canvas->show();
    canvas->activateWindow();
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
