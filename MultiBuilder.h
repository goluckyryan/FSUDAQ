#ifndef MuLTI_BUILDER_H
#define MuLTI_BUILDER_H

#include "ClassData.h"
#include "Hit.h"

#define MaxNEvent 100000 // circular, this number should be at least nDigi * MaxNChannel * MaxNData


class MultiBuilder {

public:
  MultiBuilder(Data ** multiData, std::vector<int> type, std::vector<int> sn);
  MultiBuilder(Data * singleData, int type, int sn);
  ~MultiBuilder();

  void SetTimeWindow(unsigned short nanosec) {timeWindow = nanosec; leftOverTime = nanosec;}
  unsigned short GetTimeWindow() const{return timeWindow;}

  void SetTimeJump(unsigned long long TimeJumpInNanoSec) {timeJump = TimeJumpInNanoSec;}
  unsigned long long GetTimeJump() const {return timeJump;}

  void SetLeftOverTime(unsigned long long nanosec) {leftOverTime = nanosec;}
  unsigned long long GetLeftOverTime() const{return leftOverTime;}

  void SetBreakTime(unsigned long long nanosec) {breakTime = nanosec;}
  unsigned long long GetBreakTime() const{return breakTime;}

  unsigned int GetNumOfDigitizer() const {return nData;}
  std::vector<int> GetDigiIDList() const {return idList;}

  void BuildEvents(bool isFinal = false, bool skipTrace = false, bool verbose = false);
  void BuildEventsBackWard(int maxNumEvent = 100, bool verbose = false); // always skip trace, for faster online building 

  void ClearEvents();
  void PrintStat();
  void PrintAllEvent();

  long eventIndex;
  long eventBuilt; // reset once call BuildEvents()
  long totalEventBuilt;
  std::vector<Hit> events[MaxNEvent];

private:
  std::vector<int> typeList;
  std::vector<int> snList;
  std::vector<int> idList;
  std::vector<int> tick2ns;
  const unsigned short nData;
  Data ** data; // assume all data has MaxNChannel (16) 

  std::vector<uShort> dataSize;

  unsigned short timeWindow;
  unsigned long long leftOverTime;
  unsigned long long breakTime; // timestamp for breaking the event builder

  unsigned long long timeJump; //time diff for a time jump, default is 1e8 ns
  unsigned long long lastEventTime; // timestamp for detect time jump 

  int loopIndex[MaxNDigitizer][MaxNChannels];
  int nextIndex[MaxNDigitizer][MaxNChannels];

  int nExhaushedCh;
  bool chExhaused[MaxNDigitizer][MaxNChannels];

  void FindEarlistTimeAndCh(bool verbose = false); // search through the nextIndex
  unsigned long long earlistTime;
  int earlistDigi;
  int earlistCh;
  void FindLatestTimeAndCh(bool verbose = false); // search through the nextIndex
  unsigned long long latestTime;
  int latestDigi;
  int latestCh;

  void FindEarlistTimeAmongLastData(bool verbose = false);
  void FindLatestTimeOfData(bool verbose = false);

  int lastBackWardIndex[MaxNDigitizer][MaxNChannels];

};





#endif