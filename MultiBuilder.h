#ifndef MuLTI_BUILDER_H
#define MuLTI_BUILDER_H

#include <queue>
#include "ClassData.h"
#include "Hit.h"

#define MaxNEvent 100000 // circular, this number should be at least nDigi * MaxNChannel * MaxNData

struct ChannelEntry {
  unsigned long long timestamp;
  int  digiIdx;  // index into data[]
  int  chIdx;    // channel index within that digitizer
  long absIndex; // absolute index = LoopIndex*dataSize + DataIndex

  // min-heap comparator (smallest timestamp on top)
  bool operator>(const ChannelEntry& o) const { return timestamp > o.timestamp; }
  // max-heap comparator (largest timestamp on top, for backward build)
  bool operator<(const ChannelEntry& o) const { return timestamp < o.timestamp; }
};

class MultiBuilder {

public:
  MultiBuilder(Data ** multiData, std::vector<int> type, std::vector<int> sn);
  MultiBuilder(Data * singleData, int type, int sn);
  ~MultiBuilder();

  void ForceStop(bool onOff) { forceStop = onOff;}

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
  Data ** data;
  int numTotCh;

  std::vector<uShort> dataSize;

  unsigned short timeWindow;
  unsigned long long leftOverTime;
  unsigned long long breakTime;
  unsigned long long timeJump;
  unsigned long long lastEventTime;

  // Forward build: persistent min-heap (smallest timestamp on top)
  std::priority_queue<ChannelEntry,
                      std::vector<ChannelEntry>,
                      std::greater<ChannelEntry>> pq;

  // Backward build: max-heap (largest timestamp on top), re-init each call
  std::priority_queue<ChannelEntry,
                      std::vector<ChannelEntry>,
                      std::less<ChannelEntry>> pqBack;

  // Per-channel tracking for persistent forward heap
  long nextForwardIndex[MaxNDigitizer][MaxNChannels]; // next abs index to push (-1 = uninit)
  bool inHeap[MaxNDigitizer][MaxNChannels];           // true if channel currently has entry in pq

  // Backward build watermark (prevent re-processing)
  long lastBackWardIndex[MaxNDigitizer][MaxNChannels];

  bool forceStop;

  void SeedForwardHeap();           // initial population of pq
  void RefreshExhaustedChannels();  // re-seed channels that were empty but now have data
  bool PushNextForward(int digiIdx, int chIdx, long nextAbs);
  bool PushNextBackward(int digiIdx, int chIdx, long nextAbs);
  Hit  MakeHit(const ChannelEntry& e, bool skipTrace) const;
  unsigned long long GetSafeBuildLimit() const;
};

#endif
