#ifndef CUSTOMTHREADS_H
#define CUSTOMTHREADS_H

#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QMessageBox>
#include <QCoreApplication>

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
    readCount = 0;
    stop = false;
  }
  void Stop() { this->stop = true;}
  void SetSaveData(bool onOff)  {this->isSaveData = onOff;}
  void SetScopeMode(bool onOff) {this->isScope = onOff;}

  void SetReadCountZero() {readCount = 0;}
  unsigned long GetReadCount() const {return readCount;}

  void run(){

    stop = false;
    readCount = 0;
    clock_gettime(CLOCK_REALTIME, &t0);
    // ta = t0;
    t1 = t0;

    digiMTX[ID].lock();
    digi->ReadACQStatus();
    digiMTX[ID].unlock();

    printf("ReadDataThread for digi-%d running.\n", digi->GetSerialNumber());
    do{
      
      if( stop) break;

      digiMTX[ID].lock();
      int ret = digi->ReadData();
      digiMTX[ID].unlock();
      readCount ++;

      if( stop) break;

      if( ret == CAEN_DGTZ_Success && !stop){
        digiMTX[ID].lock();
        digi->GetData()->DecodeBuffer(!isScope, 0);
        if( isSaveData ) digi->GetData()->SaveData();
        digiMTX[ID].unlock();

      }else{
        printf("ReadDataThread::%s------------ ret : %d \n", __func__, ret);
        digiMTX[ID].lock();
        digi->StopACQ();
        if( ret == CAEN_DGTZ_OutOfMemory ){
          digi->WriteRegister(DPP::SoftwareClear_W, 1);
          digi->GetData()->ClearData();
        }
        digiMTX[ID].unlock();
        emit sendMsg("Digi-" + QString::number(digi->GetSerialNumber()) + " ACQ off.");
        stop = true;
        break;
      }

      clock_gettime(CLOCK_REALTIME, &t2);
      if( t2.tv_sec - t1.tv_sec > 1 ){
        digiMTX[ID].lock();
        digi->ReadACQStatus();
        digiMTX[ID].unlock();
        t2 = t1;
        // QCoreApplication::processEvents();
      }

      // if( isSaveData && !stop ) {
      //   clock_gettime(CLOCK_REALTIME, &tb);
      //   if( tb.tv_sec - ta.tv_sec > 2 ) {
      //     digiMTX[ID].lock();
      //     emit sendMsg("FileSize ("+ QString::number(digi->GetSerialNumber()) +"): " +  QString::number(digi->GetData()->GetTotalFileSize()/1024./1024., 'f', 4) + " MB [" + QString::number(tb.tv_sec-t0.tv_sec) + " sec]");
      //     //emit sendMsg("FileSize ("+ QString::number(digi->GetSerialNumber()) +"): " +  QString::number(digi->GetData()->GetTotalFileSize()/1024./1024., 'f', 4) + " MB [" + QString::number(tb.tv_sec-t0.tv_sec) + " sec] (" + QString::number(readCount) + ")");
      //     digiMTX[ID].unlock();
      //     // readCount = 0;
      //     ta = tb;
      //   }
      // }
    }while(!stop);
    printf("ReadDataThread for digi-%d stopped.\n", digi->GetSerialNumber());
  }
signals:
  void sendMsg(const QString &msg);
private:
  Digitizer * digi; 
  bool stop;
  int ID;
  timespec ta, tb, t1, t2, t0;
  bool isSaveData;
  bool isScope;
  unsigned long readCount; 
};

//^#======================================================= Timing Thread
// class TimingThread : public QThread {
//   Q_OBJECT
// public:
//   TimingThread(QObject * parent = 0 ) : QThread(parent){
//     waitTime = 20; // multiple of 100 mili sec
//     stop = false;
//   }
//   bool isStopped() const {return stop;}
//   void Stop() { this->stop = true;}
//   void SetWaitTimeinSec(float sec) {waitTime = sec * 10 ;}
//   float GetWaitTimeinSec() const {return waitTime/10.;}
//   void DoOnce() {emit timeUp();};
//   void run(){
//     unsigned int count  = 0;
//     stop = false;
//     do{
//       usleep(100000);
//       count ++;
//       if( count % waitTime == 0){
//         emit timeUp();
//       }
//     }while(!stop);
//   }
// signals:
//   void timeUp();
// private:
//   bool stop;
//   unsigned int waitTime;
// };

#endif
