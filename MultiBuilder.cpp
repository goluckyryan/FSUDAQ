#include "MultiBuilder.h"

#include <algorithm>
#include <climits>

MultiBuilder::MultiBuilder(Data ** multiData, std::vector<int> type, std::vector<int> sn) : nData(type.size()){
  DebugPrint("%s", "MultiBuilder");
  data = multiData;
  typeList = type;
  snList = sn;
  numTotCh = 0;
  for( uShort i = 0; i < nData; i++) {
    idList.push_back(i);
    dataSize.push_back(data[i]->GetDataSize());
    numTotCh += data[i]->GetNChannel();
  }
  timeWindow = 100;
  leftOverTime = 100;
  breakTime = ULLONG_MAX;
  timeJump = 1e8;
  lastEventTime = 0;
  forceStop = false;
  ClearEvents();
}

MultiBuilder::MultiBuilder(Data * singleData, int type, int sn): nData(1){
  DebugPrint("%s", "MultiBuilder");
  data = new Data *[1];
  data[0] = singleData;
  numTotCh = data[0]->GetNChannel();
  typeList.push_back(type);
  snList.push_back(sn);
  idList.push_back(0);
  timeWindow = 100;
  leftOverTime = 100;
  breakTime = ULLONG_MAX;
  timeJump = 1e8;
  lastEventTime = 0;
  forceStop = false;
  ClearEvents();
}

MultiBuilder::~MultiBuilder(){
  DebugPrint("%s", "MultiBuilder");
}

void MultiBuilder::ClearEvents(){
  DebugPrint("%s", "MultiBuilder");
  eventIndex = -1;
  eventBuilt = 0;
  totalEventBuilt = 0;
  for( int i = 0; i < MaxNEvent; i++) events[i].clear();

  while( !pq.empty() )     pq.pop();
  while( !pqBack.empty() ) pqBack.pop();

  for( int i = 0; i < MaxNDigitizer; i++){
    for( int j = 0; j < MaxNChannels; j++){
      nextForwardIndex[i][j] = -1;
      inHeap[i][j] = false;
      lastBackWardIndex[i][j] = 0;
    }
  }
}

void MultiBuilder::PrintStat(){
  DebugPrint("%s", "MultiBuilder");
  printf("Total number of event built : %ld\n", totalEventBuilt);
  for( int i = 0; i < nData; i++){
    for( int ch = 0; ch < data[i]->GetNChannel(); ch++){
      if( nextForwardIndex[i][ch] >= 0 )
        printf("%d %3d %2d | next: %7ld\n", i, snList[i], ch, nextForwardIndex[i][ch]);
    }
  }
}

void MultiBuilder::PrintAllEvent(){
  DebugPrint("%s", "MultiBuilder");
  printf("Total number of event built : %ld\n", totalEventBuilt);
  for( int i = 0; i < totalEventBuilt; i++){
    printf("%5d ------- size: %ld\n", i, events[i].size());
    for( int j = 0; j < (int)events[i].size(); j++){
      events[i][j].Print();
    }
  }
}

//^############################################### helpers

Hit MultiBuilder::MakeHit(const ChannelEntry& e, bool skipTrace) const {
  Hit h;
  h.sn        = snList[e.digiIdx];
  h.ch        = e.chIdx;
  h.timestamp = e.timestamp;
  h.energy    = data[e.digiIdx]->GetEnergy(e.chIdx, e.absIndex);
  h.fineTime  = data[e.digiIdx]->GetFineTime(e.chIdx, e.absIndex);
  if( typeList[e.digiIdx] == DPPTypeCode::DPP_PSD_CODE )
    h.energy2 = data[e.digiIdx]->GetEnergy2(e.chIdx, e.absIndex);
  if( !skipTrace )
    h.trace = data[e.digiIdx]->Waveform1[e.chIdx][e.absIndex];
  return h;
}

bool MultiBuilder::PushNextForward(int digiIdx, int chIdx, long nextAbs){
  nextForwardIndex[digiIdx][chIdx] = nextAbs;
  long tail = data[digiIdx]->GetAbsDataIndex(chIdx);
  if( nextAbs <= tail ){
    unsigned long long ts = data[digiIdx]->GetTimestamp(chIdx, nextAbs);
    if( ts != 0 ){
      pq.push({ts, digiIdx, chIdx, nextAbs});
      inHeap[digiIdx][chIdx] = true;
      return true;
    }
  }
  inHeap[digiIdx][chIdx] = false;
  return false;
}

bool MultiBuilder::PushNextBackward(int digiIdx, int chIdx, long nextAbs){
  if( nextAbs >= 0 && nextAbs > lastBackWardIndex[digiIdx][chIdx] ){
    unsigned long long ts = data[digiIdx]->GetTimestamp(chIdx, nextAbs);
    if( ts != 0 ){
      pqBack.push({ts, digiIdx, chIdx, nextAbs});
      return true;
    }
  }
  return false;
}

void MultiBuilder::SeedForwardHeap(){
  for( int i = 0; i < nData; i++){
    for( int ch = 0; ch < data[i]->GetNChannel(); ch++){
      if( inHeap[i][ch] ) continue;
      long absStart = (nextForwardIndex[i][ch] == -1) ? 0 : nextForwardIndex[i][ch];
      long tail = data[i]->GetAbsDataIndex(ch);
      if( data[i]->GetDataIndex(ch) < 0 ) continue; // channel has no data
      if( absStart > tail ) continue;
      unsigned long long ts = data[i]->GetTimestamp(ch, absStart);
      if( ts == 0 ) continue;
      pq.push({ts, i, ch, absStart});
      nextForwardIndex[i][ch] = absStart;
      inHeap[i][ch] = true;
    }
  }
}

void MultiBuilder::RefreshExhaustedChannels(){
  for( int i = 0; i < nData; i++){
    for( int ch = 0; ch < data[i]->GetNChannel(); ch++){
      if( inHeap[i][ch] ) continue;
      if( data[i]->GetDataIndex(ch) < 0 ) continue;
      long next = (nextForwardIndex[i][ch] == -1) ? 0 : nextForwardIndex[i][ch];
      long tail = data[i]->GetAbsDataIndex(ch);
      if( next > tail ) continue;
      unsigned long long ts = data[i]->GetTimestamp(ch, next);
      if( ts == 0 ) continue;
      pq.push({ts, i, ch, next});
      nextForwardIndex[i][ch] = next;
      inHeap[i][ch] = true;
    }
  }
}

unsigned long long MultiBuilder::GetSafeBuildLimit() const {
  unsigned long long limit = ULLONG_MAX;
  for( int i = 0; i < nData; i++){
    for( int ch = 0; ch < data[i]->GetNChannel(); ch++){
      int idx = data[i]->GetDataIndex(ch);
      if( idx < 0 ) continue;
      unsigned long long ts = data[i]->GetTimestamp(ch, idx);
      if( ts == 0 ) continue;
      if( ts < limit ) limit = ts;
    }
  }
  return limit;
}

//^############################################### forward event builder

void MultiBuilder::BuildEvents(bool isFinal, bool skipTrace, bool verbose){
  DebugPrint("%s", "MultiBuilder");

  RefreshExhaustedChannels();

  if( pq.empty() ) SeedForwardHeap();
  if( pq.empty() ) return; // no data

  unsigned long long safeLimit = GetSafeBuildLimit();

  eventBuilt = 0;

  while( !pq.empty() && !forceStop ){

    unsigned long long eventSeed = pq.top().timestamp;

    if( !isFinal ){
      if( safeLimit == ULLONG_MAX || safeLimit - eventSeed <= leftOverTime ){
        if( verbose ) printf("######################### left over data for next build. safeLimit: %llu, seed: %llu\n", safeLimit, eventSeed);
        break;
      }
      if( eventSeed > breakTime ){
        if( verbose ) printf("######################### seed %llu exceeds breakTime %llu\n", eventSeed, breakTime);
        break;
      }
    }

    eventIndex++;
    if( eventIndex >= MaxNEvent ) eventIndex = 0;
    events[eventIndex].clear();

    unsigned long long eventStart = eventSeed;

    // Drain all hits within [eventStart, eventStart + timeWindow]
    while( !pq.empty() ){
      if( pq.top().timestamp - eventStart > timeWindow ) break;
      ChannelEntry e = pq.top(); pq.pop();
      inHeap[e.digiIdx][e.chIdx] = false;
      events[eventIndex].push_back( MakeHit(e, skipTrace) );
      PushNextForward(e.digiIdx, e.chIdx, e.absIndex + 1);
      if( timeWindow == 0 ) break;
    }

    if( events[eventIndex].empty() ){
      eventIndex = (eventIndex == 0) ? MaxNEvent - 1 : eventIndex - 1;
      continue;
    }

    // Hits are already in timestamp order — no sort needed
    eventBuilt++;
    totalEventBuilt++;

    if( verbose ){
      printf(">>>>>>>>>>>>>>>>> Event ID : %ld, total built: %ld, multiplicity : %ld\n",
             eventIndex, totalEventBuilt, events[eventIndex].size());
      for( int i = 0; i < (int)events[eventIndex].size(); i++){
        events[eventIndex][i].Print();
      }
    }
  }

  forceStop = false;
}

//^############################################### backward event builder

void MultiBuilder::BuildEventsBackWard(int maxNumEvent, bool verbose){
  DebugPrint("%s", "MultiBuilder");

  // Re-init backward heap from current tail of each channel
  while( !pqBack.empty() ) pqBack.pop();

  for( int i = 0; i < nData; i++){
    for( int ch = 0; ch < data[i]->GetNChannel(); ch++){
      long tail = data[i]->GetAbsDataIndex(ch);
      if( data[i]->GetDataIndex(ch) < 0 ) continue;
      if( tail <= lastBackWardIndex[i][ch] ) continue;
      unsigned long long ts = data[i]->GetTimestamp(ch, tail);
      if( ts == 0 ) continue;
      pqBack.push({ts, i, ch, tail});
    }
  }

  if( pqBack.empty() ) return;

  eventBuilt = 0;

  while( !pqBack.empty() && !forceStop && eventBuilt < maxNumEvent ){

    unsigned long long eventSeed = pqBack.top().timestamp;

    eventIndex++;
    if( eventIndex >= MaxNEvent ) eventIndex = 0;
    events[eventIndex].clear();

    unsigned long long eventStart = eventSeed;

    while( !pqBack.empty() ){
      if( eventStart - pqBack.top().timestamp > timeWindow ) break;
      ChannelEntry e = pqBack.top(); pqBack.pop();
      events[eventIndex].push_back( MakeHit(e, true) ); // always skip trace
      PushNextBackward(e.digiIdx, e.chIdx, e.absIndex - 1);
      if( timeWindow == 0 ) break;
    }

    if( events[eventIndex].empty() ) continue;

    // Hits arrived latest-first from max-heap; reverse for chronological order
    std::reverse(events[eventIndex].begin(), events[eventIndex].end());

    eventBuilt++;
    totalEventBuilt++;

    if( verbose ){
      printf(">>>>>>>>>>>>>>>>> Event ID : %ld, total built: %ld, multiplicity : %ld\n",
             eventIndex, totalEventBuilt, events[eventIndex].size());
      for( int i = 0; i < (int)events[eventIndex].size(); i++){
        events[eventIndex][i].Print();
      }
    }
  }

  forceStop = false;

  // Save watermark to prevent re-processing on next call
  for( int i = 0; i < nData; i++){
    for( int ch = 0; ch < data[i]->GetNChannel(); ch++){
      lastBackWardIndex[i][ch] = data[i]->GetAbsDataIndex(ch);
    }
  }
}
