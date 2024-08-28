#ifndef SINGLE_SPECTR_H
#define SINGLE_SPECTR_H

#include <QMainWindow>
#include <QChart>
#include <QChartView>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QLineEdit>
#include <QGridLayout>
#include <QGroupBox>
#include <QVector>
#include <QRandomGenerator>

#include "macro.h"
#include "ClassDigitizer.h"
#include "CustomThreads.h"
#include "CustomWidgets.h"
#include "Histogram1D.h"
#include "Histogram2D.h"

class HistWorker; //Forward decalration

//^====================================================
//^====================================================
class SingleSpectra : public QMainWindow{
  Q_OBJECT

public:
  SingleSpectra(Digitizer ** digi, unsigned int nDigi, QString rawDataPath, QMainWindow * parent = nullptr);
  ~SingleSpectra();

  void ClearInternalDataCount();
  // void SetFillHistograms(bool onOff) { fillHistograms = onOff;}
  // bool IsFillHistograms() const {return fillHistograms;}

  void LoadSetting();
  void SaveSetting();

  void SetMaxFillTime(unsigned short milliSec) { maxFillTimeinMilliSec = milliSec;}
  unsigned short GetMaxFillTime() const {return maxFillTimeinMilliSec;};

  QVector<int> generateNonRepeatedCombination(int size);


  /// This should be private....
  int lastFilledIndex[MaxNDigitizer][MaxNChannels];
  int loopFilledIndex[MaxNDigitizer][MaxNChannels];
  bool histVisibility[MaxNDigitizer][MaxNChannels];
  bool hist2DVisibility[MaxNDigitizer];
  unsigned short maxFillTimePerDigi;

  bool isFillingHistograms;
  Histogram1D * hist[MaxNDigitizer][MaxNChannels];
  Histogram2D * hist2D[MaxNDigitizer];

  Digitizer ** digi;
  unsigned short nDigi;

  void startWork(){ 
    // printf("timer start\n");
    timer->start(maxFillTimeinMilliSec); 
  } 
  void stopWork(){ 
    // printf("timer stop\n");
    timer->stop();
    ClearInternalDataCount();
  }

  QCheckBox * chkIsFillHistogram;

public slots:
  // void FillHistograms();
  void ChangeHistView();

private:


  RComboBox * cbDivision;

  RComboBox * cbDigi;
  RComboBox * cbCh;

  QGroupBox * histBox;
  QGridLayout * histLayout;
  int oldBd, oldCh;

  QString settingPath;

  unsigned short maxFillTimeinMilliSec;

  bool isSignalSlotActive;

  QThread * workerThread;
  HistWorker * histWorker;
  QTimer * timer;

};

//^#======================================================== HistWorker
class HistWorker : public QObject{
  Q_OBJECT
public:
  HistWorker(SingleSpectra * parent): SS(parent){}

public slots:
  void FillHistograms(){

    // printf("%s | %d %d \n", __func__, SS->chkIsFillHistogram->checkState(), SS->isFillingHistograms);
    if( SS->chkIsFillHistogram->checkState() == Qt::Unchecked ) return;
    if( SS->isFillingHistograms) return;

    SS->isFillingHistograms = true;
    timespec t0, t1;

    QVector<int> randomDigiList = SS->generateNonRepeatedCombination(SS->nDigi);

    // qDebug() << randomDigiList;

    for( int i = 0; i < SS->nDigi; i++){
      int ID = randomDigiList[i];

      QVector<int> randomChList = SS->generateNonRepeatedCombination(SS->digi[ID]->GetNumInputCh());

      // qDebug() << randomChList;
      // digiMTX[ID].lock();
      // digi[ID]->GetData()->PrintAllData();

      clock_gettime(CLOCK_REALTIME, &t0);
      for( int k = 0; k < SS->digi[ID]->GetNumInputCh(); k ++ ){
        int ch = randomChList[k];
        int lastIndex = SS->digi[ID]->GetData()->GetDataIndex(ch);
        if( lastIndex < 0 ) continue;
        // printf("--- ch %2d | last index %d \n", ch, lastIndex);

        int loopIndex = SS->digi[ID]->GetData()->GetLoopIndex(ch);

        int temp1 = lastIndex + loopIndex * SS->digi[ID]->GetData()->GetDataSize();
        int temp2 = SS->lastFilledIndex[ID][ch] + SS->loopFilledIndex[ID][ch] * SS->digi[ID]->GetData()->GetDataSize() + 1;

        // printf("loopIndx : %d | ID now : %d, ID old : %d \n", loopIndex, temp1, temp2);

        if( temp1 <= temp2 ) continue;

        if( temp1 - temp2 > SS->digi[ID]->GetData()->GetDataSize() ) { //DefaultDataSize = 10k
          temp2 = temp1 - SS->digi[ID]->GetData()->GetDataSize();
          SS->lastFilledIndex[ID][ch] = lastIndex;
          SS->lastFilledIndex[ID][ch] = loopIndex - 1;
        }

        // printf("ch %d | regulated  ID now %d  new %d | last fill idx %d\n", ch, temp2, temp1, lastFilledIndex[ID][ch]);
        
        for( int j = 0 ; j <= temp1 - temp2; j ++){
          SS->lastFilledIndex[ID][ch] ++;
          if( SS->lastFilledIndex[ID][ch] > SS->digi[ID]->GetData()->GetDataSize() ) {
            SS->lastFilledIndex[ID][ch] = 0;
            SS->loopFilledIndex[ID][ch] ++;
          }

          uShort data = SS->digi[ID]->GetData()->GetEnergy(ch, SS->lastFilledIndex[ID][ch]);

          // printf(" ch: %d, last fill idx : %d | %d \n", ch, lastFilledIndex[ID][ch], data);

          SS->hist[ID][ch]->Fill( data );
          if( SS->digi[i]->GetDPPType() == DPPTypeCode::DPP_PSD_CODE ){
            uShort e2 = SS->digi[ID]->GetData()->GetEnergy2(ch, SS->lastFilledIndex[ID][ch]);
            // printf("%u \n", e2);
            SS->hist[ID][ch]->Fill( e2, 1);
          }
          SS->hist2D[ID]->Fill(ch, data);
        }
        if( SS->histVisibility[ID][ch]  ) SS->hist[ID][ch]->UpdatePlot();

        clock_gettime(CLOCK_REALTIME, &t1);
        if( t1.tv_nsec - t0.tv_nsec + (t1.tv_sec - t0.tv_sec)*1e9 > SS->maxFillTimePerDigi * 1e6 ) break;  
      }

      if( SS->hist2DVisibility[ID] ) SS->hist2D[ID]->UpdatePlot();
      // digiMTX[ID].unlock();

    }

    SS->isFillingHistograms = false;
    emit workDone();
  }

signals:
  void workDone();

private:
  SingleSpectra * SS;
};

#endif