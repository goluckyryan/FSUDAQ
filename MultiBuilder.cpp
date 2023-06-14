#include "MultiBuilder.h"

#include <algorithm>

MultiBuilder::MultiBuilder(Data ** inData, std::vector<int> type) : nData(type.size()){
  data = inData;
  typeList = type;
  timeWindow = 100;
  ClearEvents();
}

MultiBuilder::~MultiBuilder(){

}

void MultiBuilder::ClearEvents(){
  eventIndex = -1;
  eventBuilt = 0;
  totalEventBuilt = 0;
  for( int i = 0; i < MaxNEvent; i++) events[i].clear();

  for( int i = 0; i < MaxNDigitizer; i++){
    for( int j = 0; j < MaxNChannels; j++){
      loopIndex[i][j] = 0;
      nextIndex[i][j] = -1;
      chExhaused[i][j] = false;
    }

    earlistDigi = -1;
    earlistCh = -1;
    earlistTime = -1;
    latestTime = 0;

    nExhaushedCh = 0;
  }
}

void MultiBuilder::FindEarlistTimeAndCh(bool verbose){

  earlistTime = -1;
  earlistDigi = -1;
  earlistCh = -1;

  nExhaushedCh = 0;
  
  for( int i = 0; i < nData; i++){

    for( int j = 0; j < MaxNChannels; j++ ){
      chExhaused[i][j] = false;
    }

    for(unsigned int ch = 0; ch < MaxNChannels; ch ++){
      if( data[i]->Timestamp[ch][data[i]->DataIndex[ch]] == 0 || data[i]->DataIndex[ch] == -1 || loopIndex[i][ch] * MaxNData + nextIndex[i][ch] > data[i]->LoopIndex[ch] * MaxNData +  data[i]->DataIndex[ch]) {
        nExhaushedCh ++;
        chExhaused[i][ch] = true;
        continue;
      }

      if( nextIndex[i][ch] == -1 ) nextIndex[i][ch] = 0;

      unsigned long long time = data[i]->Timestamp[ch][nextIndex[i][ch]];
      if( time < earlistTime ) {
        earlistTime = time;
        earlistDigi = i;
        earlistCh = ch;
      }
    }
  }

  if( verbose ) printf("%s | bd : %d, ch : %d, %llu\n", __func__, earlistDigi, earlistCh, earlistTime);

}

void MultiBuilder::FindLatestTime(bool verbose){
  latestTime = 0;
  int latestCh = -1;
  int latestDigi = -1;
  for( int i = 0; i < nData; i++){
    for( unsigned ch = 0; ch < MaxNChannels; ch++ ){
      int index = data[i]->DataIndex[ch];
      if( index == -1 ) continue;
      if( data[i]->Timestamp[ch][index] > latestTime ) {
        latestTime = data[i]->Timestamp[ch][index];
        latestCh = ch;
        latestDigi = i;
      }
    }
  }
  if( verbose )  printf("%s | bd : %d, ch : %d, %lld \n", __func__, latestDigi, latestCh, latestTime);
}

void MultiBuilder::BuildEvents(bool isFinal, bool skipTrace, bool verbose){

  FindLatestTime(verbose);

  FindEarlistTimeAndCh(verbose);
  if( earlistCh == -1 || nExhaushedCh == nData * MaxNChannels) return; /// no data

  eventBuilt = 0;
  //======= Start building event
  EventMember em;
  do{

    eventIndex ++;
    if( eventIndex >= MaxNEvent ) eventIndex = 0;
    events[eventIndex].clear();

    eventBuilt ++;
    totalEventBuilt ++;

    em.Clear();
    
    for( int k = 0; k < nData; k++){
      for( unsigned int i = 0; i < MaxNChannels; i++){
        int ch = (i + earlistCh ) % MaxNChannels;
        if( chExhaused[k][ch] ) continue;
        if( loopIndex[k][ch] * MaxNData + nextIndex[k][ch] > data[k]->LoopIndex[ch] * MaxNData +  data[k]->DataIndex[ch]) {
          nExhaushedCh ++;
          chExhaused[k][ch] = true;
          continue;
        }

        do {

          unsigned long long time = data[k]->Timestamp[ch][nextIndex[k][ch]];

          if( time >= earlistTime && (time - earlistTime <=  timeWindow) ){
            em.bd = k; // TODO serial number
            em.ch = ch;
            em.energy = data[k]->Energy[ch][nextIndex[k][ch]];
            em.timestamp = time;
            if( !skipTrace ) em.trace = data[k]->Waveform1[ch][nextIndex[k][ch]];
            if( typeList[k] == V1730_DPP_PSD_CODE ) em.energy2 = data[k]->Energy2[ch][nextIndex[k][ch]];

            events[eventIndex].push_back(em);
            nextIndex[k][ch]++;
            if( nextIndex[k][ch] >= MaxNData) {
              loopIndex[k][ch] ++;
              nextIndex[k][ch] = 0;
            }
          }else{
            break;
          }
          if( timeWindow == 0 ) break;
        }while( true );
        if( timeWindow == 0 ) break;
      }
      if( timeWindow == 0 ) break;

    }

    std::sort(events[eventIndex].begin(), events[eventIndex].end(), [](const EventMember& a, const EventMember& b) {
      return a.timestamp < b.timestamp;
    });
    
    ///Find the next earlist 
    FindEarlistTimeAndCh(verbose);

    if( verbose ){
      printf(">>>>>>>>>>>>>>>>> Event ID : %ld, total built: %ld, multiplicity : %ld\n", eventIndex, totalEventBuilt, events[eventIndex].size());
      for( int i = 0; i <(int) events[eventIndex].size(); i++){
        int chxxx = events[eventIndex][i].ch;
        int bd = events[eventIndex][i].bd;
        printf("%02d, %02d | %d |  %5d %llu \n", bd, chxxx, nextIndex[bd][chxxx], events[eventIndex][i].energy, events[eventIndex][i].timestamp); 
      }

      if( nExhaushedCh == nData * MaxNChannels ) {
        printf("######################### no more event to be built\n"); 
        break;
      } 
      printf("----- next ch : %d, next earlist Time : %llu.\n", earlistCh, earlistTime);

    }

    if( !isFinal && latestTime - earlistTime <= timeWindow ) {
      if( verbose ) {
        printf("######################### left over data for next build, latesTime : %llu.\n", latestTime); 
      } 
      break;
    }

  }while(nExhaushedCh < nData * MaxNChannels);

}
