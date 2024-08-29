#ifndef COINCIDENTANLAYZER_H
#define COINCIDENTANLAYZER_H

#include "Analyser.h"
#include "FSUDAQ.h"

//^===========================================
class CoincidentAnalyzer : public Analyzer{
  Q_OBJECT
public:
  CoincidentAnalyzer(Digitizer ** digi, unsigned int nDigi, QString rawDataPath, QMainWindow * parent = nullptr): Analyzer(digi, nDigi, parent){

    this->digi = digi;
    this->nDigi = nDigi;
    this->rawDataPath = rawDataPath;

    SetUpdateTimeInSec(1.0);

    //RedefineEventBuilder({0}); // only build for the 0-th digitizer, otherwise, it will build event accross all digitizers
    SetBackwardBuild(false, 100); // using normal building (acceding in time) or backward building, int the case of backward building, default events to be build is 100. 

    evtbder = GetEventBuilder();
    evtbder->SetTimeWindow(500);

    allowSignalSlot = false;
    SetUpCanvas();

  }

  ~CoincidentAnalyzer(){
  }

  void SetUpCanvas();

public slots:
  void UpdateHistograms();

private:

  Digitizer ** digi;
  unsigned int nDigi;

  MultiBuilder *evtbder;

  bool allowSignalSlot;

  QLineEdit * leInfluxIP;
  QLineEdit * leDBName;

  // declaie histograms
  Histogram2D * h2D;
  Histogram1D * h1;
  Histogram1D * h1g;
  Histogram1D * hMulti;

  QCheckBox * chkRunAnalyzer;
  RSpinBox * sbUpdateTime;

  QCheckBox * chkBackWardBuilding;
  RSpinBox * sbBackwardCount;

  RSpinBox * sbBuildWindow;

  // data source for the h2D
  RComboBox * xDigi;
  RComboBox * xCh;
  RComboBox * yDigi;
  RComboBox * yCh;

  // data source for the h1
  RComboBox * aDigi;
  RComboBox * aCh;

  QString rawDataPath;
  void SaveSettings();
  void LoadSettings();

};


inline void CoincidentAnalyzer::SetUpCanvas(){

  setWindowTitle("Online Coincident Analyzer");
  setGeometry(0, 0, 1600, 1000);  

  {//^====== channel settings
    QGroupBox * box = new QGroupBox("Configuration", this);
    layout->addWidget(box, 0, 0);
    QGridLayout * boxLayout = new QGridLayout(box);
    boxLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    box->setLayout(boxLayout);

    int rowID = 0;

    {
      chkRunAnalyzer = new QCheckBox("Run Analyzer", this);
      boxLayout->addWidget(chkRunAnalyzer, rowID, 0);

      QLabel * lbUpdateTime = new QLabel("Update Period [s]", this);
      lbUpdateTime->setAlignment(Qt::AlignRight | Qt::AlignCenter);
      boxLayout->addWidget(lbUpdateTime, rowID, 1);
      sbUpdateTime = new RSpinBox(this, 1);
      sbUpdateTime->setMinimum(0.1);
      sbUpdateTime->setMaximum(5);
      sbUpdateTime->setValue(1);
      boxLayout->addWidget(sbUpdateTime, rowID, 2);

      connect(sbUpdateTime, &RSpinBox::valueChanged, this, [=](){ sbUpdateTime->setStyleSheet("color : blue"); });

      connect(sbUpdateTime, &RSpinBox::returnPressed, this, [=](){ 
        sbUpdateTime->setStyleSheet("");
        SetUpdateTimeInSec(sbUpdateTime->value()); 
      });

      QLabel * lbBuildWindow = new QLabel("Event Window [ns]", this);
      lbBuildWindow->setAlignment(Qt::AlignRight | Qt::AlignCenter);
      boxLayout->addWidget(lbBuildWindow, rowID, 3);
      sbBuildWindow = new RSpinBox(this, 0);
      sbBuildWindow->setMinimum(1);
      sbBuildWindow->setMaximum(9999999999);
      sbBuildWindow->setValue(1000);
      boxLayout->addWidget(sbBuildWindow, rowID, 4);

      connect(sbBuildWindow, &RSpinBox::valueChanged, this, [=](){
        sbBuildWindow->setStyleSheet("color : blue;");
      });

      connect(sbBuildWindow, &RSpinBox::returnPressed, this, [=](){
        sbBuildWindow->setStyleSheet("");
        evtbder->SetTimeWindow((int)sbBuildWindow->value());
      });

      rowID ++;
      chkBackWardBuilding = new QCheckBox("Use Backward builder", this);
      boxLayout->addWidget(chkBackWardBuilding, rowID, 0);

      QLabel * lbBKWindow = new QLabel("Max No. Backward Event", this);
      lbBKWindow->setAlignment(Qt::AlignRight | Qt::AlignCenter);
      boxLayout->addWidget(lbBKWindow, rowID, 1);
      sbBackwardCount = new RSpinBox(this, 0);
      sbBackwardCount->setMinimum(1);
      sbBackwardCount->setMaximum(9999);
      sbBackwardCount->setValue(100);
      boxLayout->addWidget(sbBackwardCount, rowID, 2);

      chkBackWardBuilding->setChecked(false);
      sbBackwardCount->setEnabled(false);

      connect(chkBackWardBuilding, &QCheckBox::stateChanged, this, [=](int status){
        SetBackwardBuild(status, sbBackwardCount->value());
        sbBackwardCount->setEnabled(status);
        SetBackwardBuild(true, sbBackwardCount->value());
      });

      connect(sbBackwardCount, &RSpinBox::valueChanged, this, [=](){
        sbBackwardCount->setStyleSheet("color : blue;");
      });

      connect(sbBackwardCount, &RSpinBox::returnPressed, this, [=](){ 
        sbBackwardCount->setStyleSheet("");
        SetBackwardBuild(true, sbBackwardCount->value());
      });

    }

    {
      rowID ++;
      QFrame *separator0 = new QFrame(box);
      separator0->setFrameShape(QFrame::HLine);
      separator0->setFrameShadow(QFrame::Sunken);
      boxLayout->addWidget(separator0, rowID, 0, 1, 4);

      rowID ++;
      QLabel * lbXDigi = new QLabel("X-Digi", this);
      lbXDigi->setAlignment(Qt::AlignRight | Qt::AlignCenter);
      boxLayout->addWidget(lbXDigi, rowID, 0);
      xDigi = new RComboBox(this);
      for(unsigned int i = 0; i < nDigi; i ++ ){
        xDigi->addItem("Digi-" +  QString::number(digi[i]->GetSerialNumber()), i);
      }
      boxLayout->addWidget(xDigi, rowID, 1);

      QLabel * lbXCh = new QLabel("X-Ch", this);
      lbXCh->setAlignment(Qt::AlignRight | Qt::AlignCenter);
      boxLayout->addWidget(lbXCh, rowID, 2);
      xCh = new RComboBox(this);
      for( int i = 0; i < digi[0]->GetNumInputCh(); i++) xCh->addItem("Ch-" + QString::number(i), i);
      boxLayout->addWidget(xCh, rowID, 3);

      rowID ++;
      QLabel * lbYDigi = new QLabel("Y-Digi", this);
      lbYDigi->setAlignment(Qt::AlignRight | Qt::AlignCenter);
      boxLayout->addWidget(lbYDigi, rowID, 0);
      yDigi = new RComboBox(this); 
      for(unsigned int i = 0; i < nDigi; i ++ ){
        yDigi->addItem("Digi-" +  QString::number(digi[i]->GetSerialNumber()), i);
      }
      boxLayout->addWidget(yDigi, rowID, 1);

      QLabel * lbYCh = new QLabel("Y-Ch", this);
      lbYCh->setAlignment(Qt::AlignRight | Qt::AlignCenter);
      boxLayout->addWidget(lbYCh, rowID, 2);
      yCh = new RComboBox(this);
      for( int i = 0; i < digi[0]->GetNumInputCh(); i++) yCh->addItem("Ch-" + QString::number(i), i);
      boxLayout->addWidget(yCh, rowID, 3);

      connect(xDigi, &RComboBox::currentIndexChanged, this, [=](){
        allowSignalSlot = false;
        xCh->clear();
        for( int i = 0; i < digi[0]->GetNumInputCh(); i++) xCh->addItem("Ch-" + QString::number(i), i);
        allowSignalSlot = true;

        int bd = xDigi->currentData().toInt();
        int ch = xCh->currentData().toInt();
        h2D->SetXTitle("Digi-" + QString::number(digi[bd]->GetSerialNumber()) + ", Ch-" + QString::number(ch));
        h2D->UpdatePlot();

      });

      connect(xCh, &RComboBox::currentIndexChanged, this, [=](){
        if( !allowSignalSlot) return;
        int bd = xDigi->currentData().toInt();
        int ch = xCh->currentData().toInt();
        h2D->SetXTitle("Digi-" + QString::number(digi[bd]->GetSerialNumber()) + ", Ch-" + QString::number(ch));
        h2D->UpdatePlot();
      });

      connect(yDigi, &RComboBox::currentIndexChanged, this, [=](){
        allowSignalSlot = false;
        yCh->clear();
        for( int i = 0; i < digi[0]->GetNumInputCh(); i++) yCh->addItem("Ch-" + QString::number(i), i);
        allowSignalSlot = true;

        int bd = yDigi->currentData().toInt();
        int ch = yCh->currentData().toInt();
        h2D->SetYTitle("Digi-" + QString::number(digi[bd]->GetSerialNumber()) + ", Ch-" + QString::number(ch));
        h2D->UpdatePlot();

      });

      connect(yCh, &RComboBox::currentIndexChanged, this, [=](){
        if( !allowSignalSlot) return;
        int bd = yDigi->currentData().toInt();
        int ch = yCh->currentData().toInt();
        h2D->SetYTitle("Digi-" + QString::number(digi[bd]->GetSerialNumber()) + ", Ch-" + QString::number(ch));
        h2D->UpdatePlot();
      });

    }

    {
      rowID ++;
      QFrame *separator1 = new QFrame(box);
      separator1->setFrameShape(QFrame::HLine);
      separator1->setFrameShadow(QFrame::Sunken);
      boxLayout->addWidget(separator1, rowID, 0, 1, 4);

      rowID ++;
      QLabel * lbaDigi = new QLabel("ID-Digi", this);
      lbaDigi->setAlignment(Qt::AlignRight | Qt::AlignCenter);
      boxLayout->addWidget(lbaDigi, rowID, 0);
      aDigi = new RComboBox(this);
      for(unsigned int i = 0; i < nDigi; i ++ ){
        aDigi->addItem("Digi-" +  QString::number(digi[i]->GetSerialNumber()), i);
      }
      boxLayout->addWidget(aDigi, rowID, 1);

      QLabel * lbaCh = new QLabel("1D-Ch", this);
      lbaCh->setAlignment(Qt::AlignRight | Qt::AlignCenter);
      boxLayout->addWidget(lbaCh, rowID, 2);
      aCh = new RComboBox(this);
      for( int i = 0; i < digi[0]->GetNumInputCh(); i++) aCh->addItem("Ch-" + QString::number(i), i);
      boxLayout->addWidget(aCh, rowID, 3);

      connect(aDigi, &RComboBox::currentIndexChanged, this, [=](){
        allowSignalSlot = false;
        aCh->clear();
        for( int i = 0; i < digi[0]->GetNumInputCh(); i++) aCh->addItem("Ch-" + QString::number(i), i);
        allowSignalSlot = true;

        int bd = aDigi->currentData().toInt();
        int ch = aCh->currentData().toInt();
        h1->SetXTitle("Digi-" + QString::number(digi[bd]->GetSerialNumber()) + ", Ch-" + QString::number(ch));
        h1->UpdatePlot();
        h1g->SetXTitle("Digi-" + QString::number(digi[bd]->GetSerialNumber()) + ", Ch-" + QString::number(ch));
        h1g->UpdatePlot();

      });

      connect(aCh, &RComboBox::currentIndexChanged, this, [=](){
        if( !allowSignalSlot) return;
        int bd = aDigi->currentData().toInt();
        int ch = aCh->currentData().toInt();
        h1->SetXTitle("Digi-" + QString::number(digi[bd]->GetSerialNumber()) + ", Ch-" + QString::number(ch));
        h1->UpdatePlot();
        h1g->SetXTitle("Digi-" + QString::number(digi[bd]->GetSerialNumber()) + ", Ch-" + QString::number(ch));
        h1g->UpdatePlot();
      });
      
    }

    {
      rowID ++;
      QFrame *separator2 = new QFrame(box);
      separator2->setFrameShape(QFrame::HLine);
      separator2->setFrameShadow(QFrame::Sunken);
      boxLayout->addWidget(separator2, rowID, 0, 1, 4);

      rowID ++;
      QLabel * lbIP = new QLabel("Database IP :", box);
      lbIP->setAlignment(Qt::AlignRight | Qt::AlignCenter);
      boxLayout->addWidget(lbIP, rowID, 0);
      leInfluxIP =  new QLineEdit(box);
      leInfluxIP->setReadOnly(true);
      boxLayout->addWidget(leInfluxIP, rowID, 1, 1, 3);

      QPushButton * bnInflux = new QPushButton("Set Influx", box);
      boxLayout->addWidget(bnInflux, rowID, 4);

      rowID ++;
      QLabel * lbDBName = new QLabel("Database name :", box);
      lbDBName->setAlignment(Qt::AlignRight | Qt::AlignCenter);
      boxLayout->addWidget(lbDBName, rowID, 0);
      leDBName =  new QLineEdit(box);
      leDBName->setReadOnly(true);
      boxLayout->addWidget(leDBName, rowID, 1);

      connect(bnInflux, &QPushButton::clicked, this, [=](){
        SetDatabaseButton();
        if( influx ) {
          leDBName->setText(dataBaseName);
          leInfluxIP->setText(dataBaseIP);
        }
      });

      // rowID ++;
      // QFrame *separator3 = new QFrame(box);
      // separator3->setFrameShape(QFrame::HLine);
      // separator3->setFrameShadow(QFrame::Sunken);
      // boxLayout->addWidget(separator3, rowID, 0, 1, 4);

      QPushButton * bnClearHist = new QPushButton("Clear All Hist.", this);
      boxLayout->addWidget(bnClearHist, rowID, 2);

      connect(bnClearHist, &QPushButton::clicked, this, [=](){
        h2D->Clear();
        h1->Clear();
        h1g->Clear();
        hMulti->Clear();
      });

      QPushButton * bnSaveSettings = new QPushButton("Save Settings", this);
      boxLayout->addWidget(bnSaveSettings, rowID, 3);

      connect(bnSaveSettings, &QPushButton::clicked, this, &CoincidentAnalyzer::SaveSettings);

      QPushButton * bnLoadSettings = new QPushButton("Load Settings", this);
      boxLayout->addWidget(bnLoadSettings, rowID, 4);

      connect(bnLoadSettings, &QPushButton::clicked, this, &CoincidentAnalyzer::LoadSettings);

    }

  }

  //============ histograms
  hMulti = new Histogram1D("Multiplicity", "", 16, 0, 16, this);
  layout->addWidget(hMulti, 0, 1);  

  // the "this" make the histogram a child of the SplitPole class. When SplitPole destory, all childs destory as well.
  h2D = new Histogram2D("Coincident Plot", "XXX", "YYY", 200, 0, 30000, 200, 0, 30000, this, rawDataPath);
  //layout is inheriatge from Analyzer
  layout->addWidget(h2D, 1, 0, 2, 1);

  int bd = xDigi->currentData().toInt();
  int ch = xCh->currentData().toInt();
  h2D->SetXTitle("Digi-" + QString::number(digi[bd]->GetSerialNumber()) + ", Ch-" + QString::number(ch));
  bd = yDigi->currentData().toInt();
  ch = yCh->currentData().toInt();
  h2D->SetYTitle("Digi-" + QString::number(digi[bd]->GetSerialNumber()) + ", Ch-" + QString::number(ch));
  h2D->UpdatePlot();

  h1 = new Histogram1D("1D Plot", "XXX", 300, 0, 30000, this);
  h1->SetColor(Qt::darkGreen);
  // h1->AddDataList("Test", Qt::red); // add another histogram in h1, Max Data List is 10
  bd = aDigi->currentData().toInt();
  ch = aCh->currentData().toInt();
  h1->SetXTitle("Digi-" + QString::number(digi[bd]->GetSerialNumber()) + ", Ch-" + QString::number(ch));
  h1->UpdatePlot();
  layout->addWidget(h1, 1, 1);
  
  h1g = new Histogram1D("1D Plot (PID gated)", "XXX", 300, 0, 30000, this);
  h1g->SetXTitle("Digi-" + QString::number(digi[bd]->GetSerialNumber()) + ", Ch-" + QString::number(ch));
  h1g->UpdatePlot();
  layout->addWidget(h1g, 2, 1);

  layout->setColumnStretch(0, 1);
  layout->setColumnStretch(1, 1);

  allowSignalSlot = true;

}

inline void CoincidentAnalyzer::UpdateHistograms(){

  // printf(">>>>>>>>>>>>> CoincidentAnalyzer::%s | %d %d %d \n", __func__, this->isVisible(), chkRunAnalyzer->isChecked(), isWorking);

  if( this->isVisible() == false ) return;
  if( chkRunAnalyzer->isChecked() == false ) return;

  if( isWorking ) return; 
  isWorking = true; // This is important. set the isWorking = true to prevent another call of UpdateHistograms()

  unsigned long long t0 = getTime_ns();
  BuildEvents(false); // call the event builder to build events
  // unsigned long long t1 = getTime_ns();
  // printf("Event Build time : %llu ns = %.f msec\n", t1 - t0, (t1-t0)/1e6);

  //============ Get events, and do analysis
  long eventBuilt = evtbder->eventBuilt;

  if( eventBuilt == 0 ) return;

  //============ Get the cut list, if any
  QList<QPolygonF> cutList = h2D->GetCutList();
  const int nCut = cutList.count();
  unsigned long long tMin[nCut] = {0xFFFFFFFFFFFFFFFF}, tMax[nCut] = {0};
  unsigned int count[nCut]={0};

  //============ Get the channel to plot
  int a_bd = aDigi->currentData().toInt();
  int a_ch = aCh->currentData().toInt();
  
  int x_bd = xDigi->currentData().toInt();
  int x_ch = xCh->currentData().toInt();

  int y_bd = yDigi->currentData().toInt();
  int y_ch = yCh->currentData().toInt();

  int a_sn = digi[a_bd]->GetSerialNumber();
  int x_sn = digi[x_bd]->GetSerialNumber();
  int y_sn = digi[y_bd]->GetSerialNumber();

  //============ Processing data and fill histograms
  long eventIndex = evtbder->eventIndex;
  long eventStart = eventIndex - eventBuilt + 1;
  if(eventStart < 0 ) eventStart += MaxNEvent;
  
  for( long i = eventStart ; i <= eventIndex; i ++ ){
    std::vector<Hit> event = evtbder->events[i];

    hMulti->Fill((int) event.size());
    if( event.size() == 0 ) return;

    int aE = -1;
    int xE = -1, yE = -1;
    unsigned long long xT = 0;
    for( int k = 0; k < (int) event.size(); k++ ){
      // event[k].Print();
      if( event[k].sn == a_sn && event[k].ch == a_ch) {
        h1->Fill(event[k].energy);
        aE = event[k].energy;
      }

      if( event[k].sn == x_sn && event[k].ch == x_ch) {
        xE = event[k].energy;
        xT = event[k].timestamp;
      }
      if( event[k].sn == y_sn && event[k].ch == y_ch) yE = event[k].energy; 
    }

    if( xE >= 0 && yE >= 0 ) h2D->Fill(xE, yE);

    //check events inside any Graphical cut and extract the rate
    for(int p = 0; p < cutList.count(); p++ ){ 
      if( cutList[p].isEmpty() ) continue;
      if( cutList[p].containsPoint(QPointF(xE, yE), Qt::OddEvenFill) &&  xE >= 0 && yE >= 0  ){
        if( xT < tMin[p] ) tMin[p] = xT;
        if( xT > tMax[p] ) tMax[p] = xT;
        count[p] ++;
        //printf(".... %d \n", count[p]);
        if( p == 0 && aE >= 0 ) h1g->Fill(aE); // only for the 1st gate
      }
    }

    unsigned long long ta = getTime_ns();
    if( ta - t0 > sbUpdateTime->value() * 0.9 * GetUpdateTimeInSec() * 1e9 ) break;

  }

  // printf("--------------- update histograms\n");
  h2D->UpdatePlot();
  h1->UpdatePlot();
  hMulti->UpdatePlot();
  h1g->UpdatePlot();

  if( influx ){
    QList<QString> cutNameList = h2D->GetCutNameList();
    for( int p = 0; p < cutList.count(); p ++){
      if( cutList[p].isEmpty() ) continue;
      double dT = (tMax[p]-tMin[p]) / 1e9; 
      double rate = count[p]*1.0/(dT);
      printf("%llu %llu, %f %d\n", tMin[p], tMax[p], dT, count[p]);
      printf("%10s | %d | %f Hz \n", cutNameList[p].toStdString().c_str(), count[p], rate);  
      
      influx->AddDataPoint("Cut,name=" + cutNameList[p].toStdString()+ " value=" + std::to_string(rate));
    }

    influx->WriteData(dataBaseName.toStdString());
    influx->ClearDataPointsBuffer();
  }

  // printf("<<<<<<<<<<<<< end of UpdateHistorgams\n");

  isWorking = false;
}

inline void CoincidentAnalyzer::SaveSettings(){
  QString filePath = QFileDialog::getSaveFileName(this, 
                                                  "Save Settings to File", 
                                                  QDir::toNativeSeparators(rawDataPath + "/CoinAnaSettings.txt" ), 
                                                  "Text file (*.txt)");

  if (!filePath.isEmpty()){
  
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
      QTextStream out(&file);

      // Define the text to write
      QStringList lines;

      lines << QString::number(digi[aDigi->currentData().toInt()]->GetSerialNumber());
      lines << QString::number(aCh->currentData().toInt());
      lines << QString::number(h1->GetNBin());
      lines << QString::number(h1->GetXMin());
      lines << QString::number(h1->GetXMax());

      lines << QString::number(digi[xDigi->currentData().toInt()]->GetSerialNumber());
      lines << QString::number(xCh->currentData().toInt());
      lines << QString::number(h2D->GetXNBin());
      lines << QString::number(h2D->GetXMin());
      lines << QString::number(h2D->GetXMax());

      lines << QString::number(digi[yDigi->currentData().toInt()]->GetSerialNumber());
      lines << QString::number(yCh->currentData().toInt());
      lines << QString::number(h2D->GetYNBin());
      lines << QString::number(h2D->GetYMin());
      lines << QString::number(h2D->GetYMax());
      
      
      lines << QString::number(sbUpdateTime->value());
      lines << QString::number(chkBackWardBuilding->isChecked());
      lines << QString::number(sbBackwardCount->value());

      lines<< dataBaseIP;
      lines<< dataBaseName;
      lines<< dataBaseToken;

      lines << "#===== End of File";

      // Write each line to the file
      for (const QString &line : lines) out << line << "\n";

      // Close the file
      file.close();
      qDebug() << "File written successfully to" << filePath;
    }else{
      qWarning() << "Unable to open file" << filePath;
    }

  }
}
inline void CoincidentAnalyzer::LoadSettings(){

  QString filePath = QFileDialog::getOpenFileName(this, 
                                                  "Load Settings to File", 
                                                  rawDataPath, 
                                                  "Text file (*.txt)");

  int a_sn, a_ch, a_bin;
  float a_min, a_max;
  int x_sn, x_ch, x_bin;
  float x_min, x_max;
  int y_sn, y_ch, y_bin;
  float y_min, y_max;

  float updateTime = 1.0;
  int bkCount = 100;
  bool isBkEvtBuild = false;

  if (!filePath.isEmpty()) {

    QFile file(filePath);

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
      QTextStream in(&file);

      short count = 0;
      while (!in.atEnd()) {
        QString line = in.readLine();

        if( count == 0 ) a_sn  = line.toInt();
        if( count == 1 ) a_ch  = line.toInt();
        if( count == 2 ) a_bin = line.toInt();
        if( count == 3 ) a_min = line.toFloat();
        if( count == 4 ) a_max = line.toFloat();

        if( count == 5 ) x_sn  = line.toFloat();
        if( count == 6 ) x_ch  = line.toFloat();
        if( count == 7 ) x_bin = line.toFloat();
        if( count == 8 ) x_min = line.toFloat();
        if( count == 9 ) x_max = line.toFloat();

        if( count == 10 ) y_sn  = line.toFloat();
        if( count == 11 ) y_ch  = line.toFloat();
        if( count == 12 ) y_bin = line.toFloat();
        if( count == 13 ) y_min = line.toFloat();
        if( count == 14 ) y_max = line.toFloat();

        if( count == 15 ) updateTime = line.toFloat();
        if( count == 16 ) isBkEvtBuild = line.toInt();
        if( count == 17 ) bkCount = line.toInt();

        if( count == 18 ) dataBaseIP = line;
        if( count == 19 ) dataBaseName = line;
        if( count == 20 ) dataBaseToken = line;

        count ++;
      }

      file.close();
      qDebug() << "File read successfully from" << filePath;

      if( count >= 21 ){
        
        sbUpdateTime->setValue(updateTime);
        chkBackWardBuilding->setChecked(isBkEvtBuild);
        sbBackwardCount->setValue(bkCount);

        int x_index  = xDigi->findText("Digi-" + QString::number(x_sn));
        int y_index  = yDigi->findText("Digi-" + QString::number(y_sn));
        int a_index  = aDigi->findText("Digi-" + QString::number(a_sn));
        if( x_index == -1 ) qWarning() << " Cannot find digitizer " << x_sn;
        if( y_index == -1 ) qWarning() << " Cannot find digitizer " << y_sn;
        if( a_index == -1 ) qWarning() << " Cannot find digitizer " << a_sn;

        xDigi->setCurrentIndex(x_index);
        yDigi->setCurrentIndex(y_index);
        aDigi->setCurrentIndex(a_index);

        xCh->setCurrentIndex(x_ch);
        yCh->setCurrentIndex(y_ch);
        aCh->setCurrentIndex(a_ch);

        h1->Rebin(a_bin, a_min, a_max);
        h1g->Rebin(a_bin, a_min, a_max);
        h2D->Rebin(x_bin, x_min, x_max, y_bin, y_min, y_max);

        SetDatabase(dataBaseIP, dataBaseName, dataBaseToken);
        if( influx ){
          leDBName->setText(dataBaseName);
          leInfluxIP->setText(dataBaseIP);
        }

      }

    }else {
      qWarning() << "Unable to open file" << filePath;
    }
  }

}


#endif