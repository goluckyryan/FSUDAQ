#include "Analyser.h"
#include "CustomWidgets.h"

#include <QRandomGenerator>
#include <random>

Analyzer::Analyzer(Digitizer ** digi, unsigned int nDigi, QMainWindow * parent ): QMainWindow(parent), dataList(NULL){

  this->digi = digi;
  this->nDigi = nDigi;

  setWindowTitle("Online Analyzer");
  setGeometry(0, 0, 1000, 800);  

  influx = nullptr;
  dataBaseIP = "";
  dataBaseName = "";
  dataBaseToken = "";

  dataList = new Data*[nDigi];
  typeList.clear();
  snList.clear();

  for( unsigned int k = 0; k < nDigi; k ++) {
    dataList[k] = digi[k]->GetData();
    typeList.push_back(digi[k]->GetDPPType());
    snList.push_back(digi[k]->GetSerialNumber());
  }

  isBuildBackward = false;
  mb = new MultiBuilder(dataList, typeList, snList);

  buildTimerThread = new TimingThread(this);
  buildTimerThread->SetWaitTimeinSec(1.0); //^Set event build interval
  connect( buildTimerThread, &TimingThread::timeUp, this, &Analyzer::UpdateHistograms);

  QWidget * layoutWidget = new QWidget(this);
  setCentralWidget(layoutWidget);
  layout = new QGridLayout(layoutWidget);
  layoutWidget->setLayout(layout);

  // QPushButton * bnSetting = new QPushButton("Settings", this);
  // layout->addWidget(bnSetting);

}

Analyzer::~Analyzer(){

  if( buildTimerThread ){
    if( !buildTimerThread->isStopped() ){
      buildTimerThread->Stop();
      buildTimerThread->quit();
      buildTimerThread->wait();
    }
    delete buildTimerThread;
  }

  delete influx;
  delete mb;
  delete [] dataList;
}

double Analyzer::RandomGauss(double mean, double sigma){

  // Box-Muller transform to generate normally distributed random numbers
  double u1 = QRandomGenerator::global()->generateDouble();
  double u2 = QRandomGenerator::global()->generateDouble();
  double z0 = sqrt(-2.0 * log(u1)) * cos(2 * M_PI * u2);

  // Apply mean and standard deviation
  return mean + z0 * sigma;

}

void Analyzer::SetDatabase(QString IP, QString Name, QString Token){
  dataBaseIP = IP;
  dataBaseName = Name;
  dataBaseToken = Token;

  if( influx ) {
    delete influx;
    influx = nullptr;
  }

  influx = new InfluxDB(dataBaseIP.toStdString());

  if( influx->TestingConnection() ){
    printf("InfluxDB URL (%s) is Valid. Version : %s\n", dataBaseIP.toStdString().c_str(), influx->GetVersionString().c_str());

    if( influx->GetVersionNo() > 1 && dataBaseToken.isEmpty() ) {
      printf("A Token is required for accessing the database.\n");
      delete influx;
      influx = nullptr;
      return;
    }

    influx->SetToken(dataBaseToken.toStdString());

    //==== chck database exist
    influx->CheckDatabases();
    std::vector<std::string> databaseList = influx->GetDatabaseList();
    bool foundDatabase = false;
    for( int i = 0; i < (int) databaseList.size(); i++){
      if( databaseList[i] == dataBaseName.toStdString() ) foundDatabase = true;
    }
    if( foundDatabase ){
      influx->AddDataPoint("test value=1");
      influx->WriteData(dataBaseName.toStdString());
      influx->ClearDataPointsBuffer();
      if( influx->IsWriteOK() ){
        printf("test write database OK.\n");
      }else{
        printf("################# test write database FAIL.\n");
        delete influx;
        influx = nullptr;
      }
    }else{
      printf("Database name : %s NOT found.\n", dataBaseName.toStdString().c_str());
      delete influx;
      influx = nullptr;
    }
  }else{
    printf("InfluxDB URL (%s) is NOT Valid. \n", dataBaseIP.toStdString().c_str());
    delete influx;
    influx = nullptr;
  }

}

void Analyzer::RedefineEventBuilder(std::vector<int> idList){
  delete mb;
  delete [] dataList;
  typeList.clear();
  snList.clear();
  dataList = new Data*[idList.size()];

  for( size_t k = 0; k <  idList.size(); k ++) {
    dataList[k] = digi[idList[k]]->GetData();
    typeList.push_back(digi[idList[k]]->GetDPPType());
    snList.push_back(digi[idList[k]]->GetSerialNumber());
  }

  mb = new MultiBuilder(dataList, typeList, snList);
}

void Analyzer::StartThread(){
  mb->ClearEvents();
  buildTimerThread->start();
}

void Analyzer::StopThread(){
  // printf("%s\n", __func__);
  buildTimerThread->Stop();
  buildTimerThread->quit();
  buildTimerThread->wait();
}

void Analyzer::BuildEvents(bool verbose){
  
  unsigned int nData = mb->GetNumOfDigitizer();
  std::vector<int> idList = mb->GetDigiIDList();
  // for( unsigned int i = 0; i < nData; i++ ) digiMTX[idList[i]].lock();
  if( isBuildBackward ){
    mb->BuildEventsBackWard(maxNumEventBuilt, verbose);
  }else{
    mb->BuildEvents(0, true, verbose);
  }
  // for( unsigned int i = 0; i < nData; i++ ) digiMTX[idList[i]].unlock();

}

void Analyzer::SetDatabaseButton(){

  QDialog dialog;
  dialog.setWindowTitle("Influx Database");

  QGridLayout layout(&dialog);

  //------------------------------
  QLabel ipLabel("Database IP : "); 
  layout.addWidget(&ipLabel, 0, 0);

  QLineEdit ipLineEdit;
  ipLineEdit.setFixedSize(1000, 20); 
  ipLineEdit.setText(dataBaseIP);
  layout.addWidget(&ipLineEdit, 0, 1);

  //------------------------------
  QLabel nameLabel("Database Name : "); 
  layout.addWidget(&nameLabel, 1, 0);
  
  QLineEdit nameLineEdit;
  nameLineEdit.setFixedSize(1000, 20); 
  nameLineEdit.setText(dataBaseName);
  layout.addWidget(&nameLineEdit, 1, 1);

  //------------------------------
  QLabel tokenLabel("Database Token : "); 
  layout.addWidget(&tokenLabel, 2, 0);
  
  QLineEdit tokenLineEdit;
  tokenLineEdit.setFixedSize(1000, 20); 
  tokenLineEdit.setText(dataBaseToken);
  layout.addWidget(&tokenLineEdit, 2, 1);

  layout.addWidget(new QLabel("Only for version 2+, version 1+ can be skipped."), 3, 0, 1, 2);

  // Buttons for OK and Cancel
  QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  layout.addWidget(&buttonBox);

  QObject::connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
  QObject::connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

  dialog.resize(400, dialog.sizeHint().height()); // Set the width to 400 pixels

  // Show the dialog and get the result
  if (dialog.exec() == QDialog::Accepted) {
    SetDatabase(ipLineEdit.text(), nameLineEdit.text(),tokenLineEdit.text());
  }

}

//^####################################### below are open to customization

void Analyzer::SetUpCanvas(){

}

void Analyzer::UpdateHistograms(){


}