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
  dataBaseName = "";

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
  for( unsigned int i = 0; i < nData; i++ ) digiMTX[idList[i]].lock();
  if( isBuildBackward ){
    mb->BuildEventsBackWard(maxNumEventBuilt, verbose);
  }else{
    mb->BuildEvents(0, true, verbose);
  }
  for( unsigned int i = 0; i < nData; i++ ) digiMTX[idList[i]].unlock();

}

//^####################################### below are open to customization

void Analyzer::SetUpCanvas(){

}

void Analyzer::UpdateHistograms(){


}