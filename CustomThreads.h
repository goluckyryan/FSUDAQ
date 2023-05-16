#ifndef CUSTOMTHREADS_H
#define CUSTOMTHREADS_H

#include <QThread>
#include <QMutex>

#include "macro.h"
#include "ClassDigitizer.h"

static QMutex digiMTX[MaxNBoards * MaxNPorts];

//^#===================================================== ReadData Thread
class ReadDataThread : public QThread {
  Q_OBJECT
public:
  ReadDataThread(Digitizer * dig, int digiID, QObject * parent = 0) : QThread(parent){ 
    this->digi = dig;
    this->ID = digiID;
    isSaveData = false;
    isScope = false;
  }
  void SetSaveData(bool onOff)  {this->isSaveData = onOff;}
  void SetScopeMode(bool onOff) {this->isScope = onOff;}
  void run(){
    clock_gettime(CLOCK_REALTIME, &ta);
    while(true){
      digiMTX[ID].lock();
      int ret = digi->ReadData();
      digiMTX[ID].unlock();

      if( ret == CAEN_DGTZ_Success ){
        digiMTX[ID].lock();
        digi->GetData()->DecodeBuffer(!isScope);
        if( isSaveData ) digi->GetData()->SaveData();
        digiMTX[ID].unlock();

      }else{
        printf("ReadDataThread::%s------------ ret : %d \n", __func__, ret);
        digi->StopACQ();
        break;
      }
    }
    printf("ReadDataThread stopped.\n");
  }
signals:
  void sendMsg(const QString &msg);
private:
  Digitizer * digi; 
  int ID;
  timespec ta, tb;
  bool isSaveData;
  bool isScope;
};

//^#======================================================= Timing Thread
class TimingThread : public QThread {
  Q_OBJECT
public:
  TimingThread(QObject * parent = 0 ) : QThread(parent){
    waitTime = 20; // 10 x 100 milisec
    stop = false;
  }
  void Stop() { this->stop = true;}
  void SetWaitTimeinSec(float sec) {waitTime = sec * 10 ;}
  float GetWaitTimeinSec() const {return waitTime/10.;}
  void run(){
    unsigned int count  = 0;
    stop = false;
    do{
      usleep(100000);
      count ++;
      if( count % waitTime == 0){
        emit timeUp();
      }
    }while(!stop);
  }
signals:
  void timeUp();
private:
  bool stop;
  unsigned int waitTime;
};

#endif
