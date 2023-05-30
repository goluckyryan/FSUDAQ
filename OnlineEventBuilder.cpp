#include "OnlineEventBuilder.h"

#include <algorithm>

OnlineEventBuilder::OnlineEventBuilder(Digitizer * digi){

  data = digi->GetData();
  nCh = digi->GetNChannels();

  eventIndex = -1;
  for( int i = 0; i < MaxNEvent; i++ ) events[i].clear();
  
  for( int i = 0; i < MaxNChannels; i++ ){
    nextIndex[i] = -1;
    chExhaused[i] = false;
  }

  earlistTime = -1;
  earlistCh = -1;

  nExhaushedCh = 0;

}

OnlineEventBuilder::~OnlineEventBuilder(){


}

void OnlineEventBuilder::FindEarlistTimeAndCh(){

  earlistTime = -1;
  earlistCh = -1;

  nExhaushedCh = 0;
  for( int i = 0; i < MaxNChannels; i++ ){
    chExhaused[i] = false;
  }

  for(unsigned int ch = 0; ch < nCh; ch ++){
    if( data->DataIndex[ch] == -1 || nextIndex[ch] > data->DataIndex[ch]) {
      nExhaushedCh ++;
      chExhaused[ch] = true;
      continue;
    }

    if( nextIndex[ch] == -1 ) nextIndex[ch] = 0;

    unsigned long long time = data->Timestamp[ch][nextIndex[ch]];
    if( time < earlistTime ) {
      earlistTime = time;
      earlistCh = ch;
      //printf("ch: %d , nextIndex : %d, %llu\n", ch, nextIndex[ch] , earlistTime);
    }
  }

}

void OnlineEventBuilder::FindLatestTime(){

  latestTime = 0;
  for( unsigned ch = 0; ch < nCh; ch++ ){
    int index = data->DataIndex[ch];
    if( index == -1 ) continue;
    if( data->Timestamp[ch][index] > latestTime ) {
      latestTime = data->Timestamp[ch][index];
    }
  }

  printf("--- latest time %lld \n", latestTime);
}

void OnlineEventBuilder::BuildEvents(unsigned short timeWindow, bool verbose){

  this->timeWindow = timeWindow;

  FindLatestTime();
  FindEarlistTimeAndCh();

  if( earlistCh == -1 || nExhaushedCh == nCh) return; /// no data

  eventbuilt = 0;

  //======= Start building event
  do{

    eventIndex ++;
    if( eventIndex >= MaxNEvent ) eventIndex = 0;

    eventbuilt ++;
    
    unsigned long long dT =0;
    dataPoint dp = {0, 0, 0};
    for( unsigned int i = 0; i < nCh; i++){
      int ch = (i + earlistCh ) % nCh;

      if( chExhaused[ch] ) continue;
      if( nextIndex[ch] > data->DataIndex[ch]) {
        nExhaushedCh ++;
        chExhaused[ch] = true;
        continue;
      }

      do {

        unsigned long long time = data->Timestamp[ch][nextIndex[ch]];

        if(  time - earlistTime <  timeWindow ){
          dp.ch = ch;
          dp.energy = data->Energy[ch][nextIndex[ch]];
          dp.timeStamp = time;

          events[eventIndex].push_back(dp);
          nextIndex[ch]++;
          if( nextIndex[ch] >= MaxNData) nextIndex[ch] = 0;
        }else{
          break;
        }

      }while( dT < timeWindow);

    }

    std::sort(events[eventIndex].begin(), events[eventIndex].end(), [](const dataPoint& a, const dataPoint& b) {
      return a.timeStamp < b.timeStamp;
    });
    
    ///Find the next earlist 
    FindEarlistTimeAndCh();

    if( verbose ){
      printf(">>>>>>>>>>>>>>>>>>>>>>>>> Event ID : %ld\n", eventIndex);
      for( int i = 0; i <(int) events[eventIndex].size(); i++){
        printf("%02d | %5d %llu \n", events[eventIndex][i].ch, events[eventIndex][i].energy, events[eventIndex][i].timeStamp); 
      }

      if( nExhaushedCh == nCh ) {
        printf("######################### no more event to be built\n"); 
        break;
      }  
      printf("----- next ch : %d, next earlist Time : %llu \n", earlistCh, earlistTime);
    }

    if( latestTime - earlistTime <= timeWindow ) {
      if( verbose ) {
        printf("######################### left over data for next build\n"); 
      } 
      break;
    }

  }while(nExhaushedCh < nCh);

}