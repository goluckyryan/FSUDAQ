#ifndef MuLTI_BUILDER_H
#define MuLTI_BUILDER_H

#include "ClassData.h"

#define MaxNEvent 100000 // circular, this number should be at least nDigi * MaxNChannel * MaxNData

class Hit{
public:
  int sn;
  unsigned short bd;
  unsigned short ch;
  unsigned short energy;
  unsigned short energy2;
  unsigned long long timestamp;
  unsigned short fineTime;

  std::vector<short> trace;

  Hit(){
    Clear();
  }

  void Clear(){
    sn = 0;
    bd = 0;
    ch = 0;
    energy = 0;
    energy2 = 0;
    timestamp = 0;
    fineTime = 0;
    trace.clear();
  }

  void Print(){
    printf("(%2d, %2d)[%3d] %6d %10llu, %6d, %5ld\n", bd, ch, sn, energy, timestamp, fineTime, trace.size());
  }

};

class MultiBuilder {

public:
  MultiBuilder(Data ** multiData, std::vector<int> type, std::vector<int> sn);
  MultiBuilder(Data * singleData, int type, int sn);
  ~MultiBuilder();

  void SetTimeWindow(int ticks) {timeWindow = ticks;}
  int GetTimeWindow() const{return timeWindow;}

  unsigned int GetNumOfDigitizer() const {return nData;}
  std::vector<int> GetDigiIDList() const {return idList;}

  void BuildEvents(bool isFinal = false, bool skipTrace = false, bool verbose = false);
  void BuildEventsBackWard(int maxNumEvent = 100, bool verbose = false); // always skip trace, for faster online building 

  void ClearEvents();
  void PrintStat();

  long eventIndex;
  long eventBuilt; // reset once call BuildEvents()
  long totalEventBuilt;
  std::vector<Hit> events[MaxNEvent];

private:
  std::vector<int> typeList;
  std::vector<int> snList;
  std::vector<int> idList;
  const unsigned short nData;
  Data ** data; // assume all data has MaxNChannel (16) 

  unsigned short timeWindow;
  int loopIndex[MaxNDigitizer][MaxRegChannel];
  int nextIndex[MaxNDigitizer][MaxRegChannel];

  int nExhaushedCh;
  bool chExhaused[MaxNDigitizer][MaxRegChannel];

  void FindEarlistTimeAndCh(bool verbose = false); // search thtough the nextIndex
  unsigned long long earlistTime;
  int earlistDigi;
  int earlistCh;
  void FindLatestTimeAndCh(bool verbose = false); // search thtough the nextIndex
  unsigned long long latestTime;
  int latestDigi;
  int latestCh;

  void FindLatestTimeOfData(bool verbose = false);

  int lastBackWardIndex[MaxNDigitizer][MaxRegChannel];

};





#endif