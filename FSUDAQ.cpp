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
#include <QProcess>
#include <QMessageBox>

#include "SplitPoleAnalyzer.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent){

  setWindowTitle("FSU DAQ");
  setGeometry(500, 100, 1100, 600);

  digi = nullptr;
  nDigi = 0;

  scalar = nullptr;
  scope = nullptr;
  digiSettings = nullptr;
  canvas = nullptr;
  onlineAnalyzer = nullptr;
  runTimer = new QTimer();
  breakAutoRepeat = true;
  needManualComment = true;
  runRecord = nullptr;
  model = nullptr;
  influx = nullptr;

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
    layout->addWidget(bnCloseDigitizers, 1, 0);
    connect(bnCloseDigitizers, &QPushButton::clicked, this, &MainWindow::CloseDigitizers);

    bnDigiSettings = new QPushButton("Digitizers Settings", this);
    layout->addWidget(bnDigiSettings, 0, 1);
    connect(bnDigiSettings, &QPushButton::clicked, this, &MainWindow::OpenDigiSettings);

    bnOpenScope = new QPushButton("Open Scope", this);
    layout->addWidget(bnOpenScope, 1, 1);
    connect(bnOpenScope, &QPushButton::clicked, this, &MainWindow::OpenScope);

    bnAnalyzer = new QPushButton("Online Analyzer", this);
    layout->addWidget(bnAnalyzer, 0, 2);
    connect(bnAnalyzer, &QPushButton::clicked, this, &MainWindow::OpenAnalyzer);

    bnCanvas = new QPushButton("Online 1D Histograms", this);
    layout->addWidget(bnCanvas, 1, 2);
    connect(bnCanvas, &QPushButton::clicked, this, &MainWindow::OpenCanvas);

    bnSync = new QPushButton("Sync Boards", this);
    layout->addWidget(bnSync);
    connect(bnSync, &QPushButton::clicked, this, &MainWindow::SetSyncMode);

  }

  {//^====================== influx and Elog
    QGroupBox * otherBox = new QGroupBox("Database and Elog", mainLayoutWidget);
    layoutMain->addWidget(otherBox);
    QGridLayout * layout = new QGridLayout(otherBox);
    layout->setVerticalSpacing(1);

    int rowID = 0;
    bnLock = new QPushButton("Unlock", this);
    bnLock->setChecked(true);
    layout->addWidget(bnLock, rowID, 0);

    QLabel * lbInfluxIP = new QLabel("Influx IP : ", this);
    lbInfluxIP->setAlignment(Qt::AlignRight | Qt::AlignCenter);
    layout->addWidget(lbInfluxIP, rowID, 1);

    leInfluxIP = new QLineEdit(this);
    leInfluxIP->setReadOnly(true);
    layout->addWidget(leInfluxIP, rowID, 2);

    QLabel * lbDatabaseName = new QLabel("Database Name : ", this);
    lbDatabaseName->setAlignment(Qt::AlignRight | Qt::AlignCenter);
    layout->addWidget(lbDatabaseName, rowID, 3);

    leDatabaseName = new QLineEdit(this);
    leDatabaseName->setReadOnly(true);
    layout->addWidget(leDatabaseName, rowID, 4);

    rowID ++;
    QLabel * lbElogIP = new QLabel("Elog IP : ", this);
    lbElogIP->setAlignment(Qt::AlignRight | Qt::AlignCenter);
    layout->addWidget(lbElogIP, rowID, 1);

    leElogIP = new QLineEdit(this);
    leElogIP->setReadOnly(true);
    layout->addWidget(leElogIP, rowID, 2);

    QLabel * lbElogName = new QLabel("Elog Name : ", this);
    lbElogName->setAlignment(Qt::AlignRight | Qt::AlignCenter);
    layout->addWidget(lbElogName, rowID, 3);

    leElogName = new QLineEdit(this);
    leElogName->setReadOnly(true);
    layout->addWidget(leElogName, rowID, 4);

    connect(bnLock, &QPushButton::clicked, this, [=](){
      if( leInfluxIP->isReadOnly() ){
        bnLock->setText("Lock and Set");

        leInfluxIP->setReadOnly(false);
        leDatabaseName->setReadOnly(false);
        leElogIP->setReadOnly(false);
        leElogName->setReadOnly(false);

        leInfluxIP->setEnabled(true);
        leDatabaseName->setEnabled(true);
        leElogIP->setEnabled(true);
        leElogName->setEnabled(true);

        leInfluxIP->setStyleSheet("color : blue;");
        leDatabaseName->setStyleSheet("color : blue;");
        leElogIP->setStyleSheet("color : blue;");
        leElogName->setStyleSheet("color : blue;");

      }else{
        bnLock->setText("Unlock");

        leInfluxIP->setReadOnly(true);
        leDatabaseName->setReadOnly(true);
        leElogIP->setReadOnly(true);
        leElogName->setReadOnly(true);

        leInfluxIP->setStyleSheet("");
        leDatabaseName->setStyleSheet("");
        leElogIP->setStyleSheet("");
        leElogName->setStyleSheet("");

        influxIP = leInfluxIP->text();
        dataBaseName = leDatabaseName->text();
        elogIP = leElogIP->text();
        elogName = leElogName->text();

        SaveProgramSettings();

        SetUpInflux();

        if( elogName != "" ) CheckElog();
      }

    });

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

    QPushButton * bnOpenRecord = new QPushButton("Open Record", this);
    connect(bnOpenRecord, &QPushButton::clicked, this, &MainWindow::OpenRecord);

    layout->addWidget(lbDataPath, rowID, 0);
    layout->addWidget(leDataPath, rowID, 1, 1, 5);
    layout->addWidget(bnSetDataPath, rowID, 6);
    layout->addWidget(bnOpenRecord, rowID, 7);

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
    connect( chkSaveData, &QCheckBox::stateChanged, this, [=](int state){
      cbAutoRun->setEnabled(state);
      if( state == 0 ) cbAutoRun->setCurrentIndex(0);
    });

    cbAutoRun = new RComboBox(this);
    cbAutoRun->addItem("Single Infinite", 0);
    cbAutoRun->addItem("Single 1 min", 1);
    cbAutoRun->addItem("Single 30 mins", 30);
    cbAutoRun->addItem("Single 60 mins", 60);
    cbAutoRun->addItem("Repeat 1 mins", -1);
    cbAutoRun->addItem("Repeat 60 mins", -60);
    cbAutoRun->addItem("Repeat 120 mins", -120);
    cbAutoRun->setEnabled(false);

    bnStartACQ = new QPushButton("Start ACQ", this);
    connect( bnStartACQ, &QPushButton::clicked, this, &MainWindow::AutoRun);
    bnStopACQ = new QPushButton("Stop ACQ", this);
    connect( bnStopACQ, &QPushButton::clicked, this, [=](){
      if( chkSaveData->isChecked() ){
        if( runTimer->isActive() ){
          runTimer->stop();
          runTimer->disconnect(runTimerConnection);
          StopACQ();
        }else{
          breakAutoRepeat = true;
          runTimer->disconnect(runTimerConnection);
        }
        needManualComment = true;
      }else{
        StopACQ();
      }
    });

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
    logInfo->setReadOnly(true);
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
  elogName = "";
  elogUser = "";
  elogPWD = "";
  influxIP = "";
  dataBaseName = "";
  programSettingsFilePath = QDir::current().absolutePath() + "/programSettings.txt";
  LoadProgramSettings();

  //=========== disable widget
  WaitForDigitizersOpen(true);

  SetUpInflux();

  CheckElog();

}

MainWindow::~MainWindow(){

  if( digi ) CloseDigitizers();
  SaveProgramSettings();

  if( scope ) delete scope;

  if( canvas ) delete canvas;

  if( onlineAnalyzer ) delete onlineAnalyzer;

  if( digiSettings ) delete digiSettings;

  if( scalar ) {
    CleanUpScalar();
    scalarThread->Stop();
    scalarThread->quit();
    scalarThread->exit();
    delete scalarThread;
  }

  delete influx;

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
    rawDataPath = leDataPath->text();
    bnStartACQ->setEnabled(true);
    bnStartACQ->setStyleSheet("background-color: green;");
  }else{
    leDataPath->clear();
    rawDataPath = "";
    bnStartACQ->setEnabled(false);
    bnStartACQ->setStyleSheet("");
  }

  SaveProgramSettings();

  //TODO Check us the dataPath empty, check is the lastRun.sh exist and load the last run.

}

void MainWindow::OpenRecord(){

  QString filePath = leDataPath->text() + "/RunTimeStamp.dat";

  if( runRecord == nullptr ){
    runRecord = new QMainWindow(this);
    runRecord->setGeometry(0,0, 500, 500);
    runRecord->setWindowTitle("Run Record");

    QWidget * widget = new QWidget(runRecord);
    runRecord->setCentralWidget(widget);

    QVBoxLayout * layout = new QVBoxLayout(widget);
    widget->setLayout(layout);
    
    QLabel * lbFilePath = new QLabel(widget);
    lbFilePath->setText(filePath);
    layout->addWidget(lbFilePath);

    tableView = new QTableView(widget);
    layout->addWidget(tableView);

    model = new QStandardItemModel(runRecord);
    tableView->setModel(model);

  }

  UpdateRecord();
  runRecord->show();

}

void MainWindow::UpdateRecord(){

  if( !runRecord ) return;

  QString filePath = leDataPath->text() + "/RunTimeStamp.dat";
  model->clear();
  
  if (!filePath.isEmpty()) {
    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {

      QTextStream stream(&file);
      while (!stream.atEnd()) {
        QString line = stream.readLine();
        QStringList fields = line.split('|');

        QList<QStandardItem*> items;
        for (const QString& field : fields) {
          items.append(new QStandardItem(field));
        }
        model->appendRow(items);
      }

      file.close();
      tableView->resizeColumnsToContents();
    }
  }

  tableView->scrollToBottom();
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
      if( count == 1 ) influxIP = line;
      if( count == 2 ) dataBaseName = line;
      if( count == 3 ) elogIP = line;
      if( count == 4 ) elogName = line;
      if( count == 5 ) elogUser = line;
      if( count == 6 ) elogPWD = line;

      count ++;
      line = in.readLine();
    }

    //looking for the lastRun.sh for 
    leDataPath->setText(rawDataPath);
    leInfluxIP->setText(influxIP);
    leDatabaseName->setText(dataBaseName);
    leElogIP->setText(elogIP);
    leElogName->setText(elogName);

    logMsgHTMLMode = false;
    LogMsg("Raw Data Path : " + rawDataPath);
    LogMsg("    Influx IP : " + influxIP);
    LogMsg("Database Name : " + dataBaseName);
    LogMsg("      Elog IP : " + elogIP);
    LogMsg("    Elog Name : " + elogName);
    LogMsg("    Elog User : " + elogUser);
    LogMsg("     Elog PWD : " + elogPWD);
    logMsgHTMLMode = true;

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
  file.write((influxIP+"\n").toStdString().c_str());
  file.write((dataBaseName+"\n").toStdString().c_str());
  file.write((elogIP+"\n").toStdString().c_str());
  file.write((elogName+"\n").toStdString().c_str());
  file.write((elogUser+"\n").toStdString().c_str());
  file.write((elogPWD+"\n").toStdString().c_str());
  file.write("//------------end of file.\n");
  
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
  LogMsg("Searching digitizers via optical link or USB .....Please wait");
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

  canvas = new SingleSpectra(digi, nDigi, rawDataPath);
  histThread = new TimingThread(this);
  histThread->SetWaitTimeinSec(0.5);
  connect(histThread, &TimingThread::timeUp, this, [=](){
    if( canvas == nullptr &&   !canvas->IsFillHistograms()) return; 
    canvas->FillHistograms();
  });

  LogMsg(QString("Done. Opened %1 digitizer(s).").arg(nDigi));

  WaitForDigitizersOpen(false);
  bnStartACQ->setStyleSheet("background-color: green;");
  bnStopACQ->setEnabled(false);
  bnStopACQ->setStyleSheet("");

  bnSync->setEnabled( nDigi >= 1  );

  if( rawDataPath == "" ) {
    chkSaveData->setChecked(false);
    chkSaveData->setEnabled(false);
  }

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

  if( canvas ){
    canvas->close();
    delete canvas;
    canvas = nullptr;
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

    if(readDataThread[i]->isRunning()){
      readDataThread[i]->Stop();
      readDataThread[i]->quit();
      readDataThread[i]->wait();
    }

    delete readDataThread[i];
  }
  delete [] digi;
  digi = nullptr;

  delete [] readDataThread;
  readDataThread = nullptr;

  LogMsg("Done. Closed " + QString::number(nDigi) + " Digitizer(s).");
  nDigi = 0;

  WaitForDigitizersOpen(true);
  bnStartACQ->setStyleSheet("");
  bnStopACQ->setStyleSheet("");

}

void MainWindow::WaitForDigitizersOpen(bool onOff){

  bnOpenDigitizers->setEnabled(onOff);
  bnCloseDigitizers->setEnabled(!onOff);
  bnOpenScope->setEnabled(!onOff);
  bnDigiSettings->setEnabled(!onOff);
  bnOpenScaler->setEnabled(!onOff);
  bnStartACQ->setEnabled(!onOff);
  bnStopACQ->setEnabled(!onOff);
  bnStopACQ->setStyleSheet("");
  chkSaveData->setEnabled(!onOff);
  bnCanvas->setEnabled(!onOff);
  bnAnalyzer->setEnabled(!onOff);

  cbAutoRun->setEnabled(chkSaveData->isChecked());
  bnSync->setEnabled(false);

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

  scalar->setGeometry(0, 0, 10 + nDigi * 200, 110 + MaxNChannels * 20);

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
    uint32_t chMask =  digi[iDigi]->GetChannelMask();
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

      leTrigger[iDigi][ch]->setEnabled( (chMask >> ch) & 0x1 );
      leAccept[iDigi][ch]->setEnabled( (chMask >> ch) & 0x1 );
      
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
  //if( !scalar->isVisible() ) return;
  
  // digi[0]->GetData()->PrintAllData();

  lbLastUpdateTime->setText("Last update: " + QDateTime::currentDateTime().toString("MM.dd hh:mm:ss"));

  //printf("----------------------\n");
  uint64_t totalFileSize = 0;
  for( unsigned int iDigi = 0; iDigi < nDigi; iDigi++){
    digiMTX[iDigi].lock();

    // printf("### %d ", iDigi);
    // digi[iDigi]->GetData()->PrintAllData(true, 10);
    if( chkSaveData->isChecked() ) totalFileSize += digi[iDigi]->GetData()->GetTotalFileSize();
    for( int i = 0; i < digi[iDigi]->GetNChannels(); i++){
      if( digi[iDigi]->GetChannelOnOff(i) == true ) {
        //printf(" %3d %2d | %7.2f %7.2f \n", digi[iDigi]->GetSerialNumber(), i, digi[iDigi]->GetData()->TriggerRate[i], digi[iDigi]->GetData()->NonPileUpRate[i]);
        leTrigger[iDigi][i]->setText(QString::number(digi[iDigi]->GetData()->TriggerRate[i], 'f', 2));
        leAccept[iDigi][i]->setText(QString::number(digi[iDigi]->GetData()->NonPileUpRate[i], 'f', 2));

        if( influx ){
          influx->AddDataPoint("Rate,Bd="+std::to_string(digi[iDigi]->GetSerialNumber()) + ",Ch=" + QString::number(i).rightJustified(2, '0').toStdString() + " value=" +  QString::number(digi[iDigi]->GetData()->TriggerRate[i], 'f', 2).toStdString());
        }
      }
    }
    digiMTX[iDigi].unlock();
  }

  if( influx ){
    if( chkSaveData->isChecked() ) {
      influx->AddDataPoint("RunID value=" + std::to_string(runID));
      influx->AddDataPoint("FileSize value=" + std::to_string(totalFileSize));
    }
    influx->WriteData(dataBaseName.toStdString());
    influx->ClearDataPointsBuffer();
  }

}

//***************************************************************
//***************************************************************
void MainWindow::StartACQ(){
  if( digi == nullptr ) return;

  bool commentResult = true;
  if( chkSaveData->isChecked()) commentResult = CommentDialog(true);
  if( commentResult == false) return;

  if( chkSaveData->isChecked() ) {
    LogMsg("<font style=\"color: orange;\">===================== <b>Start a new Run-" + QString::number(runID) + "</b></font>");
    WriteRunTimestamp(true);
  }else{
    LogMsg("<font style=\"color: orange;\">===================== <b>Start a non-save Run</b></font>");
  }

  //assume master board is the 0-th board
  for( int i = (int) nDigi-1; i >= 0 ; i--){
    if( chkSaveData->isChecked() ) {
      if( digi[i]->GetData()->OpenSaveFile((rawDataPath + "/" + prefix + "_" + QString::number(runID).rightJustified(3, '0')).toStdString()) == false ) {
        LogMsg("Cannot open save file : " + QString::fromStdString(digi[i]->GetData()->GetOutFileName() ) + ". Probably read-only?");
       continue; 
      };
    }
    readDataThread[i]->SetSaveData(chkSaveData->isChecked());
    LogMsg("Digi-" + QString::number(digi[i]->GetSerialNumber()) + " is starting ACQ." );
    digi[i]->WriteRegister(DPP::SoftwareClear_W, 1);
    digi[i]->GetData()->ClearData();

    digi[i]->StartACQ();

    readDataThread[i]->start();
  }
  if( chkSaveData->isChecked() ) SaveLastRunFile();

  // printf("------------ wait for 2 sec \n");
  // usleep(1000*1000);
  // printf("------------ Go! \n");
  // for( unsigned int i = 0; i < nDigi; i++) readDataThread[i]->go();

  scalarThread->start();

  if( !scalar->isVisible() ) {
    scalar->show();
  }else{
    scalar->activateWindow();
  }
  lbScalarACQStatus->setText("<font style=\"color: green;\"><b>ACQ On</b></font>");

  if( canvas != nullptr ) histThread->start();

  bnStartACQ->setEnabled(false);
  bnStartACQ->setStyleSheet("");
  bnStopACQ->setEnabled(true);
  bnStopACQ->setStyleSheet("background-color: red;");
  bnOpenScope->setEnabled(false);
  cbAutoRun->setEnabled(false);

  if( onlineAnalyzer ) onlineAnalyzer->StartThread();

  {//^=== elog and database
    if( influx ){
      influx->AddDataPoint("RunID value=" + std::to_string(runID));
      influx->AddDataPoint("SavingData,ExpName=" +  elogName.toStdString() + " value=1");
      influx->WriteData(dataBaseName.toStdString());
      influx->ClearDataPointsBuffer();
    }

    if( elogID > 0 && chkSaveData->isChecked() ){
      QString msg = "================================= Run-" + QString::number(runID).rightJustified(3, '0') + "<p>" 
                    + QDateTime::currentDateTime().toString("MM.dd hh:mm:ss") + "<p>"
                    + startComment + "<p>"
                    "---------------------------------<p>";
      WriteElog(msg, "Run Log", "Run", runID);
    }
  }

}

void MainWindow::StopACQ(){
  if( digi == nullptr ) return;

  bool commentResult = true;
  if( chkSaveData->isChecked() ) commentResult = CommentDialog(false);
  if( commentResult == false) return;

  if( chkSaveData->isChecked() ) {
    LogMsg("===================== Stop Run-" + QString::number(runID));
    WriteRunTimestamp(false);
  }else{
    LogMsg("===================== Stop a non-save Run");
  }

  for( unsigned int i = 0; i < nDigi; i++){
    LogMsg("Digi-" + QString::number(digi[i]->GetSerialNumber()) + " is stoping ACQ." );

    if( readDataThread[i]->isRunning() ) {
      readDataThread[i]->Stop();
      readDataThread[i]->quit();
      readDataThread[i]->wait();
    }
    digiMTX[i].lock();
    digi[i]->StopACQ();
    digiMTX[i].unlock();
    if( chkSaveData->isChecked() ) digi[i]->GetData()->CloseSaveFile();
  }

  if( scalarThread->isRunning()){
    scalarThread->Stop();
    scalarThread->quit();
    scalarThread->wait();
  }

  if( onlineAnalyzer ) onlineAnalyzer->StopThread();

  if( histThread->isRunning()){
    histThread->Stop();
    histThread->quit();
    histThread->wait();
  }
  
  lbScalarACQStatus->setText("<font style=\"color: red;\"><b>ACQ Off</b></font>");

  bnStartACQ->setEnabled(true);
  bnStartACQ->setStyleSheet("background-color: green;");
  bnStopACQ->setEnabled(false);
  bnStopACQ->setStyleSheet("");
  bnOpenScope->setEnabled(true);
  cbAutoRun->setEnabled(true);

  {//^=== elog and database
    if( influx ){
      influx->AddDataPoint("SavingData,ExpName=" +  elogName.toStdString() + " value=0");
      influx->WriteData(dataBaseName.toStdString());
      influx->ClearDataPointsBuffer();
    }

    if( elogID > 0 && chkSaveData->isChecked()){
      QString msg = QDateTime::currentDateTime().toString("MM.dd hh:mm:ss") + "<p>" + stopComment + "<p>";
      uint64_t totalFileSize = 0;
      for(unsigned int i = 0 ; i < nDigi; i++){
        uint64_t fileSize = digi[i]->GetData()->GetTotalFileSize();
        totalFileSize += fileSize;
        msg += "Digi-" + QString::number(digi[i]->GetSerialNumber()) + " Size : " + QString::number(fileSize/1024./1024., 'f', 2) + " MB<p>";
      }

      msg += "..... Total File Size : " + QString::number(totalFileSize/1024./1024., 'f', 2) + "MB<p>" +              
             "=================================<p>";  
      AppendElog(msg);
    }
  }

}

void MainWindow::AutoRun(){ //TODO

  if( chkSaveData->isChecked() == false){
    StartACQ();
    return;
  }

  if( cbAutoRun->currentData().toInt() == 0 ){
    StartACQ();
    //disconnect(runTimer, runTimerConnection);
    //runTimer->disconnect(runTimerConnection);
    return;
  }else{ // auto run
    needManualComment = true;
    StartACQ();    

    runTimerConnection =  connect( runTimer, &QTimer::timeout, this, [=](){
      needManualComment = false;
      StopACQ();
      if( cbAutoRun->currentData().toInt() < 0 ){

        bnStartACQ->setEnabled(false);
        bnStartACQ->setStyleSheet("");
        bnStopACQ->setEnabled(true);
        bnStopACQ->setStyleSheet("background-color : red;");

        LogMsg("Wait for 10 sec for next Run ...." );
        QElapsedTimer elapsedTimer;
        elapsedTimer.invalidate();
        elapsedTimer.start();
        while( elapsedTimer.elapsed() < 10000) {
          
          if( breakAutoRepeat ) {
            LogMsg("Break Auto repeat.");
            bnStartACQ->setEnabled(true);
            bnStartACQ->setStyleSheet("background-color : green");
            bnStopACQ->setEnabled(false);
            bnStopACQ->setStyleSheet("");
            return;
          }
          QCoreApplication::processEvents();
        }
        
        needManualComment = false;
        StartACQ();
        runTimer->setSingleShot(true);
        runTimer->start(qAbs( cbAutoRun->currentData().toInt() * 60 * 1000));
      }
    });
  }

  int timeMiliSec = cbAutoRun->currentData().toInt() * 60 * 1000;
  runTimer->setSingleShot(true);
  runTimer->start(qAbs(timeMiliSec));

  if( timeMiliSec ) breakAutoRepeat = false;

  // ///=========== single run
  // if ( timeMiliSec > 0 ){
  //   runTimer->setSingleShot(true);
  //   runTimer->start(timeMiliSec);
  // }

  // ///=========== infinite repeat run
  // if ( timeMiliSec < 0 ){
  //   runTimer->setSingleShot(false);
  //   runTimer->start(qAbs(timeMiliSec));
  // }

}

void MainWindow::SetSyncMode(){

  QDialog dialog;
  dialog.setWindowTitle("Board Synchronization");

  QVBoxLayout * layout = new QVBoxLayout(&dialog);

  QLabel * lbInfo1 = new QLabel("This will reset 0x8100 and 0x811C \nMaster must be the 1st board.\n (could be 100 ticks offset)", &dialog);
  lbInfo1->setStyleSheet("color : red;");

  QPushButton * bnNoSync = new QPushButton("No Sync");
  QPushButton * bnMethod1 = new QPushButton("Software TRG-OUT --> TRG-IN ");
  QPushButton * bnMethod2 = new QPushButton("Software TRG-OUT --> S-IN ");
  QPushButton * bnMethod3 = new QPushButton("External TRG-OUT --> S-IN ");

  layout->addWidget(lbInfo1, 0);
  layout->addWidget( bnNoSync, 2);
  layout->addWidget(bnMethod1, 3);
  layout->addWidget(bnMethod2, 4);
  layout->addWidget(bnMethod3, 5);

  bnNoSync->setFixedHeight(40);
  bnMethod1->setFixedHeight(40);
  bnMethod2->setFixedHeight(40);
  bnMethod3->setFixedHeight(40);

  connect(bnNoSync, &QPushButton::clicked, [&](){
    for(unsigned int i = 0; i < nDigi; i++){
      digi[i]->WriteRegister(DPP::AcquisitionControl, 0);
      digi[i]->WriteRegister(DPP::FrontPanelIOControl, 0);
    }
    dialog.accept();
  });
  
  connect(bnMethod1, &QPushButton::clicked, [&](){
    digi[0]->WriteRegister(DPP::AcquisitionControl, 0);
    digi[0]->WriteRegister(DPP::FrontPanelIOControl, 0x10000); //RUN
    for(unsigned int i = 1; i < nDigi; i++){
      digi[i]->WriteRegister(DPP::AcquisitionControl, 2);
      digi[i]->WriteRegister(DPP::FrontPanelIOControl, 0x10000); // S-IN
    }
    dialog.accept();
  });
  
  connect(bnMethod2, &QPushButton::clicked, [&](){
    digi[0]->WriteRegister(DPP::AcquisitionControl, 0);
    digi[0]->WriteRegister(DPP::FrontPanelIOControl, 0x10000); //RUN
    for(unsigned int i = 1; i < nDigi; i++){
      digi[i]->WriteRegister(DPP::AcquisitionControl, 1);
      digi[i]->WriteRegister(DPP::FrontPanelIOControl, 0x30000); // S-IN
    }
    dialog.accept();
  });  

  connect(bnMethod3, &QPushButton::clicked, [&](){
    for(unsigned int i = 0; i < nDigi; i++){
      digi[i]->WriteRegister(DPP::AcquisitionControl, 1);
      digi[i]->WriteRegister(DPP::FrontPanelIOControl, 0x30000); // S-IN
    }
    dialog.accept();
  });

  dialog.exec();

}

bool MainWindow::CommentDialog(bool isStartRun){

  if( isStartRun ) runID ++;
  QString runIDStr = QString::number(runID).rightJustified(3, '0');

  int result = QDialog::Rejected ;
  QLineEdit *lineEdit = new QLineEdit(this);

  if( needManualComment ) {
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
    QPushButton *button1 = new QPushButton("OK", dOpen);
    QPushButton *button2 = new QPushButton("Cancel", dOpen);

    vlayout->addWidget(label, 0, 0, 1, 2);
    vlayout->addWidget(lineEdit, 1, 0, 1, 2);
    vlayout->addWidget(button1, 2, 0);
    vlayout->addWidget(button2, 2, 1);

    connect(button1, &QPushButton::clicked, dOpen, &QDialog::accept);
    connect(button2, &QPushButton::clicked, dOpen, &QDialog::reject);
    result = dOpen->exec();
  }else{
    if( isStartRun ){
      lineEdit->setText("Auto Start, repeat every " + QString::number(qAbs(cbAutoRun->currentData().toInt())) + " mins.");
    }else{
      lineEdit->setText("Auto Stop, after " + QString::number(qAbs(cbAutoRun->currentData().toInt())) + " mins.");
    }
    result = QDialog::Accepted;
  }

  if(result == QDialog::Accepted ){
    if( isStartRun ){
      startComment = lineEdit->text();
      if( startComment == "") startComment = "No commet was typed.";
      
      if( needManualComment ){
        int minute = cbAutoRun->currentData().toInt();
        if(  minute > 0 ) {
          startComment += ", single run of " + QString::number(minute) + " mins.";
        }else{
          startComment += ", repeat run of " + QString::number(qAbs(minute)) + " mins.";
        }
      }
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

  UpdateRecord();

}

//***************************************************************
//***************************************************************
void MainWindow::OpenScope(){

  if( scope == nullptr ) {
    scope = new Scope(digi, nDigi, readDataThread);
    connect(scope, &Scope::SendLogMsg, this, &MainWindow::LogMsg);
    connect(scope, &Scope::CloseWindow, this, [=](){
      bnStartACQ->setEnabled(true);
      bnStartACQ->setStyleSheet("background-color: green;");
      bnStopACQ->setEnabled(false);  
      bnStopACQ->setStyleSheet("");
    });
    connect(scope, &Scope::TellACQOnOff, this, [=](bool onOff){
      if( scope  ) {
        if( onOff ) {
          lbScalarACQStatus->setText("<font style=\"color: green;\"><b>ACQ On</b></font>");
        }else{
          lbScalarACQStatus->setText("<font style=\"color: red;\"><b>ACQ Off</b></font>");
        }
      }

      if( canvas ){
        if( onOff) {
          histThread->start();
        }else{
          if( histThread->isRunning()){
            histThread->Stop();
            histThread->quit();
            histThread->wait();
          }
        }
      }
    });

    connect(scope, &Scope::UpdateScaler, this, &MainWindow::UpdateScalar);

    connect(scope, &Scope::UpdateOtherPanels, this, [=](){ UpdateAllPanels(1); });

    scope->show();
  }else{
    scope->show();
    scope->activateWindow();
  }

  bnStartACQ->setEnabled(false);
  bnStartACQ->setStyleSheet("");
  bnStopACQ->setEnabled(false);  
  bnStopACQ->setStyleSheet("");
  chkSaveData->setChecked(false);

}

//***************************************************************
//***************************************************************
void MainWindow::OpenDigiSettings(){

  if( digiSettings == nullptr ) {
    digiSettings = new DigiSettingsPanel(digi, nDigi, rawDataPath);
    //connect(scope, &Scope::SendLogMsg, this, &MainWindow::LogMsg);
    connect(digiSettings, &DigiSettingsPanel::UpdateOtherPanels, this, [=](){ UpdateAllPanels(2); });

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
    canvas = new SingleSpectra(digi, nDigi, rawDataPath);
    canvas->show();
  }else{
    canvas->show();
    canvas->activateWindow();
  }

}
//***************************************************************
//***************************************************************
void MainWindow::OpenAnalyzer(){

  if( onlineAnalyzer == nullptr ) {
    //onlineAnalyzer = new Analyzer(digi, nDigi);
    onlineAnalyzer = new SplitPole(digi, nDigi);
    onlineAnalyzer->show();
  }else{
    onlineAnalyzer->show();
    onlineAnalyzer->activateWindow();
  }

}

//***************************************************************
//***************************************************************
void MainWindow::UpdateAllPanels(int panelID){

  //panelID is the source panel that call
  // scope = 1;
  // digiSetting = 2;

  if( panelID == 1 ){ // from scope
    if( digiSettings && digiSettings->isVisible() ) digiSettings->UpdatePanelFromMemory();
  }

  if( panelID == 2 ){
    if(scope && scope->isVisible() ) scope->UpdatePanelFromMomeory();

    if(scalar) {
      for( unsigned int iDigi = 0; iDigi < nDigi; iDigi++){
        uint32_t chMask =  digi[iDigi]->GetChannelMask();
        for( int i = 0; i < digi[iDigi]->GetNChannels(); i++){
          leTrigger[iDigi][i]->setEnabled( (chMask >> i) & 0x1 );
          leAccept[iDigi][i]->setEnabled( (chMask >> i) & 0x1 );
        }
      } 
    }
  }

}

//***************************************************************
//***************************************************************
void MainWindow::SetUpInflux(){

  if( influxIP == "" ) return;

  if( influx ) {
    delete influx;
    influx = nullptr;
  }

  influx = new InfluxDB(influxIP.toStdString(), false);

  if( influx->TestingConnection() ){
    LogMsg("<font style=\"color : green;\"> InfluxDB URL (<b>"+ influxIP + "</b>) is Valid </font>");
    //==== chck database exist
    //LogMsg("List of database:");
    std::vector<std::string> databaseList = influx->GetDatabaseList();
    bool foundDatabase = false;
    for( int i = 0; i < (int) databaseList.size(); i++){
      if( databaseList[i] == dataBaseName.toStdString() ) foundDatabase = true;
      //LogMsg(QString::number(i) + "|" + QString::fromStdString(databaseList[i]));
    }
    if( foundDatabase ){
      LogMsg("<font style=\"color : green;\"> Database <b>" + dataBaseName + "</b> found.");
      influx->AddDataPoint("ProgramStart value=1");
      influx->WriteData(dataBaseName.toStdString());
      influx->ClearDataPointsBuffer();
      if( influx->IsWriteOK() ){
        LogMsg("<font style=\"color : green;\">test write database OK.</font>");
      }else{
        LogMsg("<font style=\"color : red;\">test write database FAIL.</font>");
      }
    }else{
      LogMsg("<font style=\"color : red;\"> Database <b>" + dataBaseName + "</b> NOT found.");
      delete influx;
      influx = nullptr;
    }
  }else{
    LogMsg("<font style=\"color : red;\"> InfluxDB URL (<b>"+ influxIP + "</b>) is NOT Valid </font>");
    delete influx;
    influx = nullptr;
  }

  if( influx == nullptr ){
    leInfluxIP->setEnabled(false);
    leDatabaseName->setEnabled(false);
  }

}

void MainWindow::CheckElog(){

  if( elogIP != "" && elogName != "" &&  elogUser != "" && elogPWD != "" ){
    WriteElog("Testing communication.", "Testing communication.", "Other", 0);
    AppendElog("test append elog.");
  }

  if( elogID >= 0 ) {
    LogMsg("Elog testing OK.");
    return;
  }

  //QMessageBox::information(nullptr, "Information", "Elog write Fail.\nPlease set the elog User and PWD in the programSettings.txt.\nline 6 = user.\nline 7 = pwd.");
  LogMsg("Elog testing Fail");
  if( elogIP == "" ) LogMsg("no elog IP");
  if( elogName == "" ) LogMsg("no elog Name");
  if( elogUser == "" ) LogMsg("no elog User name. Please set it in the programSettings.txt, line 6.");
  if( elogPWD == "" ) LogMsg("no elog User pwd. Please set it in the programSettings.txt, line 7.");
  if( elogID < 0 ) LogMsg("Possible elog IP, Name, User name, or pwd incorrect");
  leElogIP->setEnabled(false);
  leElogName->setEnabled(false);
  
}

void MainWindow::WriteElog(QString htmlText, QString subject, QString category, int runNumber){

  //if( elogID < 0 ) return;
  if( elogName == "" ) return;
  if( elogUser == "" ) return;
  if( elogPWD == "" ) return;
  QStringList arg;
  arg << "-h" << elogIP << "-p" << "8080" << "-l" << elogName << "-u" << elogUser << elogPWD << "-a" << "Author=FSUDAQ";
  if( runNumber > 0 ) arg << "-a" << "RunNo=" + QString::number(runNumber);
  if( category != "" ) arg << "-a" << "Category=" + category;
  arg << "-a" << "Subject=" + subject 
      << "-n " << "2" <<  htmlText  ;
  QProcess elogBash(this);
  elogBash.start("elog", arg); 
  elogBash.waitForFinished();
  QString output = QString::fromUtf8(elogBash.readAllStandardOutput());

  QRegularExpression regex("ID=(\\d+)");

  QRegularExpressionMatch match = regex.match(output);
  if (match.hasMatch()) {
    QString id = match.captured(1);
    elogID = id.toInt();
  } else {
    elogID = -1;
  }

}

void MainWindow::AppendElog(QString appendHtmlText){
  if( elogID < 1 ) return;
  if( elogName == "" ) return;
  
  QProcess elogBash(this);
  QStringList arg;
  arg << "-h" << elogIP << "-p" << "8080" << "-l" << elogName << "-u" << elogUser << elogPWD << "-w" << QString::number(elogID);
  //retrevie the elog
  elogBash.start("elog", arg); 
  elogBash.waitForFinished();
  QString output = QString::fromUtf8(elogBash.readAllStandardOutput());
  //qDebug() << output;
  QString separator = "========================================";
  int index = output.indexOf(separator);
  if( index != -1){
    QString originalHtml = output.mid(index + separator.length());
    arg.clear();
    arg << "-h" << elogIP << "-p" << "8080" << "-l" << elogName << "-u" << elogUser << elogPWD << "-e" << QString::number(elogID)
        << "-n" << "2" << originalHtml + "<br>" + appendHtmlText;
    
    elogBash.start("elog", arg); 
    elogBash.waitForFinished();
    output = QString::fromUtf8(elogBash.readAllStandardOutput());
    index = output.indexOf("ID=");
    if( index != -1 ){
      elogID = output.mid(index+3).toInt();
    }else{
      elogID = -1;
    }
  }else{
    elogID = -1;
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
  //qDebug() << outputStr;
  logInfo->repaint();
}
