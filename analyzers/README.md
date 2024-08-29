# Introduction

This folder stored all online analyzers. The Analyser.cpp/h is the base class for all analyzer. 

The Analyzer.cpp/h has the MultiBuilder (to handle event building) and InfluxDB (to handle pushing data to influxDB database) classes. In Addision, it has a QThread, a AnalyzerWorker, and a QTimer, these three object handle the threading of UpdateHistograms().

The AnalyzerWorker moves to the QThread. QTimer::timeout will trigger AnalyzerWorker::UpdateHistograms().

There is an important bool 'isWorking'. This boolean variable is true when AnalyzerWorker::UpdateHistograms() is running, and it is false when finsihed. This prevent UpdateHistograms() runs twice at the same time.

There are two virual methods
- SetupCanvas()
- UpdateHistograms()

Users must implement these two methods in theie custom analyzer.


# Intruction to make new Analyzer

The CoindientAnalyzer.h is a good example.

1. inheirate the Analyzer class
```cpp
class CustomAnalyzer : public Analyzer{
  Q_OBJECT
public:
  CustomAnalyzer(Digitizer ** digi, unsigned int nDigi, QMainWindow * parent = nullptr): Analyzer(digi, nDigi, parent){
    SetUpdateTimeInSec(1.0); // set histogram update period in sec
    mb->SetTimeWindow(500); // set the event time windows
    // ... other custom stuffs
  }

  void SetUpCanvas();
public slots:
  void UpdateHistograms();

private:

  Histogram2D * h2D;
  Histogram1D * h1D;
  // some priavte variables

}
```

2. implement the SetUpCanvas() method
```cpp
inline void CustomAnalyzer::SetUpCanvas(){
  setWindowTitle("Title");
  setGeometry(0, 0, 1600, 1000);   

  h2D = new Histogram2D("Coincident Plot", "XXX", "YYY", 200, 0, 30000, 200, 0, 30000, this, rawDataPath);

  //layout is inheriatge from Analyzer
  layout->addWidget(h2D, 0, 0); // row-0, col-0

  h1 = new Histogram1D("1D Plot", "XXX", 300, 0, 30000, this);
  h1->SetColor(Qt::darkGreen);
  layout->addWidget(h1, 0, 1); // row-0, col-1

  //other GUI elements
}
```

3. implement the UpdateHistograms() method
```cpp
inline void CustomAnalyzer::UpdateHistograms(){
  // don't update histogram when the windows not visible
  if( this->isVisible() == false ) return; 

  BuildEvents(false); // call the event builder to build events, no verbose

  //check number of event built
  long eventBuilt = mb->eventBuilt;
  if( eventBuilt == 0 ) return;

  //============ Processing data and fill histograms
  long eventIndex = mb->eventIndex;
  long eventStart = eventIndex - eventBuilt + 1;
  if(eventStart < 0 ) eventStart += MaxNEvent;
  
  for( long i = eventStart ; i <= eventIndex; i ++ ){
    std::vector<Hit> event = mb->events[i];

    //analysis and fill historgam 
  }  

  //Render histograms
  h2D->UpdatePlot();
  h1D->UpdatePlot();

}
```