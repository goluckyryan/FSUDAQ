#include "Analyser.h"

#include <QRandomGenerator>
#include <random>

Analyzer::Analyzer(Digitizer ** digi, unsigned int nDigi, QMainWindow * parent ): QMainWindow(parent){

  this->digi = digi;
  this->nDigi = nDigi;

  setWindowTitle("Online Analyzer");
  setGeometry(0, 0, 1000, 800);  

  mb = new MultiBuilder(digi, nDigi);

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

  delete mb;
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

void Analyzer::BuildEvents(){

  for( unsigned int i = 0; i < nDigi; i++ ) digiMTX[digiID].lock();
  mb->BuildEvents(0, 0, 0);
  for( unsigned int i = 0; i < nDigi; i++ ) digiMTX[digiID].unlock();

}

//^####################################### below are open to customization

void Analyzer::SetUpCanvas(){

}

void Analyzer::UpdateHistograms(){


}