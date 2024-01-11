#include "MultiBuilder.h"

#include <algorithm>

MultiBuilder::MultiBuilder(Data ** multiData, std::vector<int> type, std::vector<int> sn) : nData(type.size()){
  data = multiData;
  typeList = type;
  snList = sn;
  for( int i = 0; i < (int) type.size(); i++) idList.push_back(i);
  timeWindow = 100;
  leftOverTime = 100;
  breakTime = -1;
  timeJump = 1e8;
  lastEventTime = 0;
  ClearEvents();

  // for( int i = 0; i < nData; i++){
  //   printf("sn: %d, numCh : %d \n", snList[i], data[i]->GetNChannel());
  // }

}

MultiBuilder::MultiBuilder(Data * singleData, int type, int sn): nData(1){
  data = new Data *[1];
  data[0] = singleData;
  typeList.push_back(type);
  snList.push_back(sn);
  idList.push_back(0);  
  timeWindow = 100;
  leftOverTime = 100;
  breakTime = -1;
  timeJump = 1e8;
  lastEventTime = 0;

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

void MultiBuilder::PrintStat(){

  printf("Total number of evet built : %ld\n", totalEventBuilt);
  for( int i = 0; i < nData ; i++){
    for( int ch = 0; ch < data[i]->GetNChannel() ; ch++){
      printf("%d %3d %2d | %7d (%d)\n", i, snList[i], ch, nextIndex[i][ch], loopIndex[i][ch]);
    }
  }

}

void MultiBuilder::PrintAllEvent(){

  printf("Total number of evet built : %ld\n", totalEventBuilt);
  for( int i = 0; i < totalEventBuilt; i++){
    printf("%5d ------- size: %ld\n", i, events[i].size());
    for( int j = 0; j < (int)events[i].size(); j++){
      events[i][j].Print();
    }
  }

}

void MultiBuilder::FindEarlistTimeAndCh(bool verbose){

  earlistTime = -1;
  earlistDigi = -1;
  earlistCh = -1;

  nExhaushedCh = 0;
  
  for( int i = 0; i < nData; i++){

    for( int j = 0; j < data[i]->GetNChannel(); j++ ){
      chExhaused[i][j] = false;
    }

    for(unsigned int ch = 0; ch < MaxNChannels; ch ++){
      if( ch >= data[i]->GetNChannel() ) {
        nExhaushedCh ++;
        chExhaused[i][ch] = true;
        continue;
      }

      if( data[i]->Timestamp[ch][data[i]->DataIndex[ch]] == 0 || 
          data[i]->DataIndex[ch] == -1 || 
          loopIndex[i][ch] * MaxNData + nextIndex[i][ch] > data[i]->LoopIndex[ch] * MaxNData +  data[i]->DataIndex[ch]) {
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

void MultiBuilder::FindLatestTimeAndCh(bool verbose){

  latestTime = 0;
  latestDigi = -1;
  latestCh = -1;

  nExhaushedCh = 0;
  
  for( int i = 0; i < nData; i++){

    for( int j = 0; j < data[i]->GetNChannel(); j++ ){
      chExhaused[i][j] = false;
    }

    for(unsigned int ch = 0; ch < MaxNChannels; ch ++){
      // printf(" %d, %d | %d", i, ch, nextIndex[i][ch]);
      if( nextIndex[i][ch] < 0  || ch >= data[i]->GetNChannel()) {
        nExhaushedCh ++;
        chExhaused[i][ch] = true;

        // printf(", exhanshed. %d \n", nExhaushedCh);
        continue;
      }

      unsigned long long time = data[i]->Timestamp[ch][nextIndex[i][ch]];
      // printf(", time : %llu\n", time );
      if( time > latestTime ) {
        latestTime = time;
        latestDigi = i;
        latestCh = ch;
      }
    }
  }

  if( verbose ) printf("%s | bd : %d, ch : %d, %llu\n", __func__, latestDigi, latestCh, latestTime);

}

void MultiBuilder::FindEarlistTimeAmongLastData(bool verbose){
  latestTime = -1;
  latestCh = -1;
  latestDigi = -1;
  for( int i = 0; i < nData; i++){
    for( unsigned ch = 0; ch < data[i]->GetNChannel(); ch++ ){
      int index = data[i]->DataIndex[ch];
      if( index == -1 ) continue;
      if( data[i]->Timestamp[ch][index] < latestTime ) {
        latestTime = data[i]->Timestamp[ch][index];
        latestCh = ch;
        latestDigi = i;
      }
    }
  }
  if( verbose )  printf("%s | bd : %d, ch : %d, %lld \n", __func__, latestDigi, latestCh, latestTime);
}
  
void MultiBuilder::FindLatestTimeOfData(bool verbose){
  latestTime = 0;
  latestCh = -1;
  latestDigi = -1;
  for( int i = 0; i < nData; i++){
    for( unsigned ch = 0; ch < data[i]->GetNChannel(); ch++ ){
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

  FindEarlistTimeAmongLastData(verbose); // give lastest Time, Ch, and Digi

  FindEarlistTimeAndCh(verbose); //Give the earliest time, ch, digi
  if( earlistCh == -1 || nExhaushedCh == nData * MaxNChannels) return; /// no data

  eventBuilt = 0;
  //======= Start building event
  Hit em;
  do{

    eventIndex ++;
    if( eventIndex >= MaxNEvent ) eventIndex = 0;
    events[eventIndex].clear();

    eventBuilt ++;
    totalEventBuilt ++;

    em.Clear();
    
    for( int k = 0; k < nData; k++){
      int bd = (k + earlistDigi) % nData;

      // printf("##### %d/%d | ", k, nData);
      // data[k]->PrintAllData(true, 10);

      const int numCh = data[bd]->GetNChannel();

      for( int i = 0; i < numCh; i++){
        int ch = (i + earlistCh ) % numCh;
        // printf("ch : %d | exhaused ? %s \n", ch, chExhaused[bd][ch] ? "Yes" : "No");
        if( chExhaused[bd][ch] ) continue;
        if( loopIndex[bd][ch] * MaxNData + nextIndex[bd][ch] > data[bd]->LoopIndex[ch] * MaxNData +  data[bd]->DataIndex[ch]) {
          nExhaushedCh ++;
          chExhaused[bd][ch] = true;
          continue;
        }

        do {

          unsigned long long time = data[bd]->Timestamp[ch][nextIndex[bd][ch]];
          //printf("%6ld, sn: %5d, ch: %2d, timestamp : %16llu | earlistTime : %16llu | timeWindow : %u \n", eventIndex, data[bd]->boardSN, ch, time, earlistTime, timeWindow);

          if( time >= earlistTime && (time - earlistTime <=  timeWindow) ){
            em.sn = snList[bd];
            em.bd = bd;
            em.ch = ch;
            em.energy = data[bd]->Energy[ch][nextIndex[bd][ch]];
            em.timestamp = time;
            em.fineTime = data[bd]->fineTime[ch][nextIndex[bd][ch]];

            if( !skipTrace ) em.trace = data[bd]->Waveform1[ch][nextIndex[bd][ch]];
            if( typeList[bd] == DPPType::DPP_PSD_CODE ) em.energy2 = data[bd]->Energy2[ch][nextIndex[bd][ch]];

            events[eventIndex].push_back(em);
            nextIndex[bd][ch]++;
            if( nextIndex[bd][ch] >= MaxNData) {
              loopIndex[bd][ch] ++;
              nextIndex[bd][ch] = 0;
            }
          }else{
            break;
          }
          if( timeWindow <= 0 ) break;
        }while( true );
        if( timeWindow <= 0 ) break;
      }
      if( timeWindow <= 0 ) break;
    }

    if( events[eventIndex].size() > 1) {
      std::sort(events[eventIndex].begin(), events[eventIndex].end(), [](const Hit& a, const Hit& b) {
        return a.timestamp < b.timestamp;
      });
    }

    lastEventTime = events[eventIndex].back().timestamp;
    
    ///Find the next earlist 
    FindEarlistTimeAndCh(false);

    // //if there is a time jump, say, bigger than TimeJump. break
    if( earlistTime - lastEventTime > timeJump ) {
      printf("%6lu, %16llu\n", eventIndex, earlistTime);
      printf("%5s - %16llu \n", "", lastEventTime);
      printf("%5s > %16llu \n", "", timeJump);
      printf("!!!!!!!! Time Jump detected stop event building. stop event buinding and get more data.\n"); 
      return;
    }

    if( verbose ){
      printf(">>>>>>>>>>>>>>>>> Event ID : %ld, total built: %ld, multiplicity : %ld\n", eventIndex, totalEventBuilt, events[eventIndex].size());
      for( int i = 0; i <(int) events[eventIndex].size(); i++){
        int chxxx = events[eventIndex][i].ch;
        int bd = events[eventIndex][i].bd;
        printf("%02d, %02d | %5d |  %5d %llu \n", bd, chxxx, nextIndex[bd][chxxx], events[eventIndex][i].energy, events[eventIndex][i].timestamp); 
      }

      if( nExhaushedCh == nData * MaxNChannels ) {
        printf("######################### no more event to be built\n"); 
        break;
      } 
      printf("----- next bd : %d, ch : %d, next earlist Time : %llu.\n", earlistDigi, earlistCh, earlistTime);
      printf("leftOver %llu, breakTime %llu \n", leftOverTime, breakTime);
    }

    if( !isFinal ){
      if( latestTime - earlistTime <= leftOverTime){
        if( verbose ) printf("######################### left over data for next build, latesTime : %llu. | leftOverTime : %llu\n", latestTime, leftOverTime);   
        break;
      } 
      
      if( earlistTime > breakTime  )  {
        if( verbose ) printf("######################### left over data for next build, earlistTime : %llu. | breakTime : %llu\n", earlistTime, breakTime);   
        break;
      }
    }
  }while(nExhaushedCh < nData * MaxNChannels);

}

void MultiBuilder::BuildEventsBackWard(int maxNumEvent, bool verbose){

  //skip trace, and only build for maxNumEvent events max

  // remember the end of DataIndex, prevent over build
  for( int k = 0; k < nData; k++){
    for( int i = 0; i < data[k]->GetNChannel(); i++){
      nextIndex[k][i] = data[k]->DataIndex[i];
      loopIndex[k][i] = data[k]->LoopIndex[i];
    }
  }

  FindLatestTimeAndCh(verbose);

  //========== build event
  eventBuilt = 0;
  Hit em;
  do{
    eventIndex ++;
    if( eventIndex >= MaxNEvent ) eventIndex = 0;
    events[eventIndex].clear();

    eventBuilt ++;
    totalEventBuilt ++;
    em.Clear();

    for( int k = 0; k < nData; k++){
      int bd = (k + latestDigi) % nData;

      const int numCh = data[k]->GetNChannel();

      for( int i = 0; i < numCh; i++){
        int ch = (i + latestCh) % numCh;
        if( chExhaused[bd][ch] ) continue;
        //if( nextIndex[bd][ch] <= lastBackWardIndex[bd][ch] || nextIndex[bd][ch] < 0){
        if( nextIndex[bd][ch] < 0){
          chExhaused[bd][ch] = true;
          nExhaushedCh ++;
          continue;
        }

        do{

          unsigned long long time = data[bd]->Timestamp[ch][nextIndex[bd][ch]];
          if( time <= latestTime && (latestTime - time <= timeWindow)){
            em.sn = snList[bd];
            em.bd = bd;
            em.ch = ch;
            em.energy = data[bd]->Energy[ch][nextIndex[bd][ch]];
            em.timestamp = time;
            if( typeList[bd] == DPPType::DPP_PSD_CODE ) em.energy2 = data[bd]->Energy2[ch][nextIndex[bd][ch]];

            events[eventIndex].push_back(em);
            nextIndex[bd][ch]--;
            if( nextIndex[bd][ch] < 0 && data[bd]->LoopIndex[ch] > 0 ) nextIndex[bd][ch] = MaxNData - 1;
            
          }else{
            break;
          }
          if( timeWindow == 0 ) break;
        }while(true);
        if( timeWindow == 0 ) break;
      }
      if( timeWindow == 0 ) break;
    }

    std::sort(events[eventIndex].begin(), events[eventIndex].end(), [](const Hit& a, const Hit& b) {
      return a.timestamp < b.timestamp;
    });

    FindLatestTimeAndCh(verbose);

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
      printf("----- next bd: %d, ch : %d, next latest Time : %llu.\n", latestDigi, latestCh, latestTime);

    }

  }while(nExhaushedCh < nData * MaxNChannels && eventBuilt < maxNumEvent);

  // // remember the end of DataIndex, prevent over build
  // for( int k = 0; k < nData; k++){
  //   for( int i = 0; i < MaxRegChannel; i++){
  //     lastBackWardIndex[k][i] = data[k]->DataIndex[i];
  //   }
  // }

}
