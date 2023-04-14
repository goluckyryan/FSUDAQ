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
  }
  void SetSaveData(bool onOff) {this->isSaveData = onOff;}
  void run(){
    clock_gettime(CLOCK_REALTIME, &ta);
    while(true){
      digiMTX[ID].lock();
      int ret = digi->ReadData();
      digiMTX[ID].unlock();

      if( ret == CAEN_DGTZ_Success ){
        digiMTX[ID].lock();
        if( isSaveData ) {
          digi->GetData()->SaveBuffer();
        }else{
          digi->GetData()->DecodeBuffer(false);
        }
        digiMTX[ID].unlock();

      }else{
        printf("%s------------ ret : %d \n", __func__, ret);
        if( ret == CAEN_DGTZ_WrongAcqMode) break;
      }

    //   // if( ret == CAEN_FELib_Success){
    //   //   if( isSaveData) digi->SaveDataToFile();
    //   // }else if(ret == CAEN_FELib_Stop){
    //   //   digi->ErrorMsg("No more data");
    //   //   //emit endOfLastData();
    //   //   break;
    //   // }else{
    //   //   //digi->ErrorMsg("ReadDataLoop()");
    //   //   digi->evt->ClearTrace();
    //   // }

    //   // if( isSaveData ){
    //   //   clock_gettime(CLOCK_REALTIME, &tb);
    //   //   if( tb.tv_sec - ta.tv_sec > 2 ) {
    //   //     emit sendMsg("FileSize ("+ QString::number(digi->GetSerialNumber()) +"): " +  QString::number(digi->GetTotalFilesSize()/1024./1024.) + " MB");
    //   //     //emit checkFileSize();
    //   //     //double duration = tb.tv_nsec-ta.tv_nsec + tb.tv_sec*1e+9 - ta.tv_sec*1e+9;
    //   //     //printf("%4d, duration : %10.0f, %6.1f\n", readCount, duration, 1e9/duration);
    //   //     ta = tb;
    //   //   }
    //   // }
    }
  }
signals:
  void sendMsg(const QString &msg);
  //void endOfLastData();
  //void checkFileSize();
private:
  Digitizer * digi; 
  int ID;
  timespec ta, tb;
  bool isSaveData;
};

//^#===================================================== UpdateTrace Thread
class UpdateTraceThread : public QThread {
  Q_OBJECT
public:
  UpdateTraceThread(QObject * parent = 0) : QThread(parent){
    waitTime = 2;
    stop = false;
  }
  unsigned int GetWaitTimeSec() const {return waitTime;}
  void SetWaitTimeSec(unsigned sec) {waitTime = sec;}
  void Stop() {this->stop = true;}
  void run(){
    unsigned int count = 0;
    stop = false;
    do{
      usleep(100000);
      count ++;
      if( count % waitTime == 0){
        emit updateTrace();
      }
    }while(!stop);
  }
signals:
  void updateTrace();

private:
  bool stop;
  unsigned int waitTime; //100 of milisec
};

//^#======================================================= Scalar Thread
class ScalarThread : public QThread {
  Q_OBJECT
public:
  ScalarThread(QObject * parent = 0 ) : QThread(parent){
    waitTime = 20; // 10 x 100 milisec
    stop = false;
  }
  void Stop() { this->stop = true;}
  unsigned int GetWaitTimeinSec() const {return waitTime/10;}
  void run(){
    unsigned int count  = 0;
    stop = false;
    do{
      usleep(100000);
      count ++;
      if( count % waitTime == 0){
        emit updataScalar();
      }
    }while(!stop);
  }
signals:
  void updataScalar();
private:
  bool stop;
  unsigned int waitTime;
};

#endif
