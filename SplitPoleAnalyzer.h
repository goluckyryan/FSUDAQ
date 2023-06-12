#ifndef SPLITPOLEANLAYZER_H
#define SPLITPOLEANLAYZER_H

/*********************************************
 * This is online analyzer for Split-Pole at FSU
 * 
 * It is a template for other analyzer.
 * 
 * Any new analyzer add to added to FSUDAQ.cpp
 * in FSUDAQ.cpp, in OpenAnalyzer()
 * 
 * add the source files in FSUDAQ_Qt6.pro
 * then compile
 * >qmake6 FSUDAQ_Qt6.pro
 * >make
 * 
 * ******************************************/


#include "Analyser.h"

class SplitPole : public Analyzer{
  Q_OBJECT
public:
  SplitPole(Digitizer ** digi, unsigned int nDigi, QMainWindow * parent = nullptr);
  ~SplitPole();

  void SetUpCanvas();

public slots:
  void UpdateHistograms();

private:

  OnlineEventBuilder * oeb;

  Histogram2D * h2;
  Histogram1D * h1;

};


#endif