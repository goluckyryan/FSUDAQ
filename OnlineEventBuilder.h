#ifndef ONLINE_EVENT_BUILDER_H
#define ONLINE_EVENT_BUILDER_H

/**********************************************

This is an online event builder for single digitizer

In principle, it can be merged into the digitizer Class. 
But for clarity and separate the memory for an event. 
Use another class to hold the event data and methods.

************************************************/

#include "macro.h"
#include "ClassDigitizer.h"

#define MaxNEvent 10000

struct dataPoint{
  unsigned short ch;
  unsigned short energy;
  unsigned long long timeStamp;
};

class OnlineEventBuilder {

public:
  OnlineEventBuilder(Digitizer * digi);
  ~OnlineEventBuilder();

  void BuildEvents(unsigned short timeWindow, bool verbose = false);

  long eventIndex;
  std::vector<dataPoint> events[MaxNEvent]; // should be a cirular memory, store energy

  unsigned short GetTimeWindow() const { return timeWindow;}

private:

  unsigned short nCh;
  Data * data;

  unsigned short timeWindow;
  int nextIndex[MaxNChannels];

  int nExhaushedCh;
  bool chExhaused[MaxNChannels];
  unsigned long long earlistTime;
  int earlistCh;
  void FindEarlistTimeAndCh();
  unsigned long long latestTime;
  void FindLatestTime();

};

#endif