#ifndef MuLTI_BUILDER_H
#define MuLTI_BUILDER_H

#include "ClassData.h"

#define MaxNEvent 5000 // circular 

class EventMember{
public:
  unsigned short bd;
  unsigned short ch;
  unsigned short energy;
  unsigned short energy2;
  unsigned long long timestamp;
  unsigned short fineTime;

  std::vector<short> trace;

  EventMember(){
    Clear();
  }

  EventMember operator = (EventMember e){
    bd = e.bd;
    ch = e.ch;
    energy = e.energy;
    energy2 = e.energy2;
    timestamp = e.timestamp;
    fineTime = e.fineTime;
    trace = e.trace;
    return *this;
  }

  void Clear(){
    bd = 0;
    ch = 0;
    energy = 0;
    energy2 = 0;
    timestamp = 0;
    fineTime = 0;
    trace.clear();
  }
};



class MultiBuilder {

public:
  MultiBuilder(Data ** inData, std::vector<int> type);
  ~MultiBuilder();

  void SetTimeWindow(int ticks) {timeWindow = ticks;}
  int GetTimeWindow() const{return timeWindow;}

  void BuildEvents(bool isFinal = false, bool skipTrace = false, bool verbose = false);

  void ClearEvents();
  // void PrintStat();

  long eventIndex;
  long eventBuilt; // reset once call BuildEvents()
  long totalEventBuilt;
  std::vector<EventMember> events[MaxNEvent];

private:
  std::vector<int> typeList;
  const unsigned short nData;
  Data ** data; // assume all data has MaxNChannel (16) 

  unsigned short timeWindow;
  int loopIndex[MaxNDigitizer][MaxNChannels];
  int nextIndex[MaxNDigitizer][MaxNChannels];

  int nExhaushedCh;
  bool chExhaused[MaxNDigitizer][MaxNChannels];
  unsigned long long earlistTime;
  unsigned long long latestTime;
  int earlistDigi;
  int earlistCh;
  void FindEarlistTimeAndCh(bool verbose = false);
  void FindLatestTime(bool verbose = false);

};





#endif