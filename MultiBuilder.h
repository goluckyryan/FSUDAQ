#ifndef MuLTI_BUILDER_H
#define MuLTI_BUILDER_H

#include "ClassData.h"
#include "ClassDigitizer.h"

#define MaxNEvent 5000 // circular 

class EventMember{
public:
  int sn;
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
};



class MultiBuilder {

public:
  MultiBuilder(Digitizer ** digi, unsigned int nDigi);
  MultiBuilder(Digitizer ** digi, std::vector<int> id);
  MultiBuilder(Data ** multiData, std::vector<int> type, std::vector<int> sn);

  //for signal digitizer
  MultiBuilder(Digitizer ** digi, int digiID);
  MultiBuilder(Digitizer * digi);
  MultiBuilder(Data * singleData, int type);
  ~MultiBuilder();

  void SetTimeWindow(int ticks) {timeWindow = ticks;}
  int GetTimeWindow() const{return timeWindow;}

  unsigned int GetNumOfDigitizer() const {return nData;}
  std::vector<int> GetDigiIDList() const {return idList;}

  void BuildEvents(bool isFinal = false, bool skipTrace = false, bool verbose = false);
  void BuildEventsBackWard(bool verbose = false); // always skip trace, for faster online building 

  void ClearEvents();
  void PrintStat();

  long eventIndex;
  long eventBuilt; // reset once call BuildEvents()
  long totalEventBuilt;
  std::vector<EventMember> events[MaxNEvent];

private:
  std::vector<int> typeList;
  std::vector<int> snList;
  std::vector<int> idList;
  const unsigned short nData;
  Data ** data; // assume all data has MaxNChannel (16) 

  unsigned short timeWindow;
  int loopIndex[MaxNDigitizer][MaxNChannels];
  int nextIndex[MaxNDigitizer][MaxNChannels];

  int nExhaushedCh;
  bool chExhaused[MaxNDigitizer][MaxNChannels];

  void FindEarlistTimeAndCh(bool verbose = false); // search thtough the nextIndex
  unsigned long long earlistTime;
  int earlistDigi;
  int earlistCh;
  void FindLatestTimeAndCh(bool verbose = false); // search thtough the nextIndex
  unsigned long long latestTime;
  int latestDigi;
  int latestCh;

  void FindLatestTimeOfData(bool verbose = false);

  int lastBackWardIndex[MaxNDigitizer][MaxNChannels];

};





#endif