#include "OnlineAnalyser.h"

#include <QRandomGenerator>
#include <random>

OnlineAnalyzer::OnlineAnalyzer(Digitizer ** digi, unsigned int nDigi, QMainWindow * parent ): QMainWindow(parent){

  this->digi = digi;
  this->nDigi = nDigi;

  oeb = new OnlineEventBuilder * [nDigi];
  for( unsigned int i = 0; i < nDigi; i++ ) oeb[i] = new OnlineEventBuilder(digi[i]);

  buildTimerThread = new TimingThread(this);
  buildTimerThread->SetWaitTimeinSec(1.0); //^Set event build interval
  connect( buildTimerThread, &TimingThread::timeUp, this, &OnlineAnalyzer::UpdateHistograms);

  setWindowTitle("Online Analyzer");
  setGeometry(0, 0, 1000, 800);  

  QWidget * layoutWidget = new QWidget(this);
  setCentralWidget(layoutWidget);
  layout = new QGridLayout(layoutWidget);
  layoutWidget->setLayout(layout);

  // QPushButton * bnSetting = new QPushButton("Settings", this);
  // layout->addWidget(bnSetting);

  SetUpCanvas();

}

OnlineAnalyzer::~OnlineAnalyzer(){

  for( unsigned int i = 0; i < nDigi; i++ ) delete oeb[i];
  delete [] oeb;

}

void OnlineAnalyzer::StartThread(){

}

void OnlineAnalyzer::StopThread(){

}

void OnlineAnalyzer::SetUpCanvas(){

  //TODO a simple way to set it at once
  Histogram2D * h2 = new Histogram2D("testing", -10, 10, -10, 10);
  RChartView * h2View = new RChartView(h2->GetChart());
  h2View->SetVRange(-10, 10); // this only set the reset key 'r' 
  h2View->SetHRange(-10, 10);

  layout->addWidget(h2View);

  std::random_device rd;
  std::mt19937 gen(rd());
  std::normal_distribution<double> distribution(0.0, 2.0);

  for( int i = 0; i < 1000 ; i++ ){
    h2->Fill(distribution(gen), distribution(gen));
  }

}

void OnlineAnalyzer::UpdateHistograms(){

  //Set with digitizer to be event build
  digiMTX[0].lock();
  oeb[0]->BuildEvents(100);
  digiMTX[0].unlock();

  //============ Get events, and do analysis


}