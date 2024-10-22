#include "MultiBuilder.h"

#include <algorithm>

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
  breakTime = -1;
  timeJump = 1e8;
  lastEventTime = 0;
  forceStop = false;
  ClearEvents();


  // for( int i = 0; i < nData; i++){
  //   printf("sn: %d, numCh : %d \n", snList[i], data[i]->GetNChannel());
  // }

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
  breakTime = -1;
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

  for( int i = 0; i < MaxNDigitizer; i++){
    for( int j = 0; j < MaxNChannels; j++){
      nextIndex[i][j] = -1;
      chExhaused[i][j] = false;
      lastBackWardIndex[i][j] = 0;
    }

    earlistDigi = -1;
    earlistCh = -1;
    earlistTime = -1;
    latestTime = 0;

    nExhaushedCh = 0;
  }

}

void MultiBuilder::PrintStat(){
  DebugPrint("%s", "MultiBuilder");
  printf("Total number of evet built : %ld\n", totalEventBuilt);
  for( int i = 0; i < nData ; i++){
    for( int ch = 0; ch < data[i]->GetNChannel() ; ch++){
      if( nextIndex[i][ch] >= 0 ) printf("%d %3d %2d | %7ld\n", i, snList[i], ch, nextIndex[i][ch]);
    }
  }
}

void MultiBuilder::PrintAllEvent(){
  DebugPrint("%s", "MultiBuilder");
  printf("Total number of evet built : %ld\n", totalEventBuilt);
  for( int i = 0; i < totalEventBuilt; i++){
    printf("%5d ------- size: %ld\n", i, events[i].size());
    for( int j = 0; j < (int)events[i].size(); j++){
      events[i][j].Print();
    }
  }
}

//^############################################### forward event builder

void MultiBuilder::FindEarlistTimeAndCh(bool verbose){
  DebugPrint("%s", "MultiBuilder");
  earlistTime = -1;
  earlistDigi = -1;
  earlistCh = -1;
  nExhaushedCh = 0;
  for( int i = 0; i < nData; i++){

    for( int j = 0; j < data[i]->GetNChannel(); j++ ) chExhaused[i][j] = false;

    for(unsigned int ch = 0; ch < data[i]->GetNChannel(); ch ++){

      {// check is dataIndex is valid
        int index = data[i]->GetDataIndex(ch);
        if( index < 0 ) {
          nExhaushedCh ++;
          chExhaused[i][ch] = true;
          continue;
        }

        if( data[i]->GetTimestamp(ch, index) == 0 || nextIndex[i][ch] > data[i]->GetAbsDataIndex(ch)) {
          nExhaushedCh ++;
          chExhaused[i][ch] = true;
          continue;
        }
      }

      if( nextIndex[i][ch] == -1 ) nextIndex[i][ch] = 0;

      unsigned long long time = data[i]->GetTimestamp(ch, nextIndex[i][ch]);
      if( time < earlistTime ) {
        earlistTime = time;
        earlistDigi = i;
        earlistCh = ch;
      }
      // printf(" ch : %d | time %llu | %llu\n", ch, time, earlistTime);
    }
  }

  if( verbose ) printf("%s | bd : %d, ch : %d, %llu\n", __func__, earlistDigi, earlistCh, earlistTime);
}

void MultiBuilder::FindEarlistTimeAmongLastData(bool verbose){
  DebugPrint("%s", "MultiBuilder");
  latestTime = -1;
  latestCh = -1;
  latestDigi = -1;
  for( int i = 0; i < nData; i++){
    for( unsigned ch = 0; ch < data[i]->GetNChannel(); ch++ ){
      if( chExhaused[i][ch] ) continue;
      int index = data[i]->GetDataIndex(ch);
      if( index == -1 ) continue;
      if( data[i]->GetTimestamp(ch, index) < latestTime ) {
        latestTime = data[i]->GetTimestamp(ch, index);
        latestCh = ch;
        latestDigi = i;
      }
    }
  }
  if( verbose )  printf("%s | bd : %d, ch : %d, %lld \n", __func__, latestDigi, latestCh, latestTime);
}


void MultiBuilder::BuildEvents(bool isFinal, bool skipTrace, bool verbose){
  DebugPrint("%s", "MultiBuilder");

  FindEarlistTimeAndCh(verbose); //Give the earliest time, ch, digi

  FindEarlistTimeAmongLastData(verbose); // give lastest Time, Ch, and Digi for event building

  if( earlistCh == -1 || nExhaushedCh == numTotCh) return; /// no data

  eventBuilt = 0;
  //======= Start building event
  Hit em;
  do{

    if( forceStop ) break;

    eventIndex ++;
    if( eventIndex >= MaxNEvent ) eventIndex = 0;
    events[eventIndex].clear();
    em.Clear();
    
    for( int k = 0; k < nData; k++){
      int bd = (k + earlistDigi) % nData;

      // printf("##### %d/%d | ", bd, nData);
      // data[bd]->PrintAllData(true);

      const int numCh = data[bd]->GetNChannel();

      for( int i = 0; i < numCh; i++){
        int ch = (i + earlistCh ) % numCh;
        // printf("ch : %d | exhaused ? %s \n", ch, chExhaused[bd][ch] ? "Yes" : "No");
        if( chExhaused[bd][ch] ) continue;

        // printf(" ch : %2d | %d(%d) | %d(%d)\n", ch, loopIndex[bd][ch], nextIndex[bd][ch], data[bd]->GetLoopIndex(ch), data[bd]->GetDataIndex(ch) );

        if( nextIndex[bd][ch] == -1 || nextIndex[bd][ch] > data[bd]->GetAbsDataIndex(ch)) {
          nExhaushedCh ++;
          chExhaused[bd][ch] = true;

          // printf(" ch : %d exhaused\n", ch);
          continue;
        }

        do {

          unsigned long long time = data[bd]->GetTimestamp(ch, nextIndex[bd][ch]);
          // printf("%6d, sn: %5d, ch: %2d, timestamp : %16llu | earlistTime : %16llu | timeWindow : %u \n", nextIndex[bd][ch], data[bd]->boardSN, ch, time, earlistTime, timeWindow);

          if( time >= earlistTime && (time - earlistTime <=  timeWindow) ){
            em.sn = snList[bd];
            em.ch = ch;
            em.energy = data[bd]->GetEnergy(ch, nextIndex[bd][ch]);
            em.timestamp = time;
            em.fineTime = data[bd]->GetFineTime(ch, nextIndex[bd][ch]);

            if( !skipTrace ) em.trace = data[bd]->Waveform1[ch][nextIndex[bd][ch]];
            if( typeList[bd] == DPPTypeCode::DPP_PSD_CODE ) em.energy2 = data[bd]->GetEnergy2(ch, nextIndex[bd][ch]);

            events[eventIndex].push_back(em);
            nextIndex[bd][ch]++;
            
          }else{
            break;
          }
          if( timeWindow == 0 ) break;
        }while( true );
        if( timeWindow == 0 ) break;
      }
      if( timeWindow == 0 ) break;
    }

    if( events[eventIndex].size() == 0 ) {
      if( eventIndex > 1) {
        eventIndex --;
      }else{
        eventIndex = MaxNEvent - 1;
      }
      continue;
    }

    if( events[eventIndex].size() > 1) {
      std::sort(events[eventIndex].begin(), events[eventIndex].end(), [](const Hit& a, const Hit& b) {
        return a.timestamp < b.timestamp;
      });
    }

    // lastEventTime = events[eventIndex].back().timestamp;
    
    ///Find the next earlist 
    FindEarlistTimeAndCh(false);

    // //if there is a time jump, say, bigger than TimeJump. break
    // if( earlistTime - lastEventTime > timeJump ) {
    //   if( verbose ){
    //     printf("!!!!!!!! Time Jump detected stop event building and get more data.\n"); 
    //     printf("event index : %6lu,  last event time : %16llu\n", eventIndex, lastEventTime);
    //     printf("              %6s    earilest time : %16llu \n", "", earlistTime);
    //     printf("              %6s        time jump > %16llu \n", "", timeJump);
    //   }
    //   return;
    // }

    eventBuilt ++;
    totalEventBuilt ++;

    if( verbose ){
      printf(">>>>>>>>>>>>>>>>> Event ID : %ld, total built: %ld, multiplicity : %ld\n", eventIndex, totalEventBuilt, events[eventIndex].size());
      for( int i = 0; i <(int) events[eventIndex].size(); i++){
        int chxxx = events[eventIndex][i].ch;
        int sn = events[eventIndex][i].sn;
        int bd = 0;
        for( int pp = 0; pp < nData; pp++){
          if( sn == data[pp]->boardSN ) {
            bd = pp;
            break;
          }
        }
        printf("%05d, %02d | %7ld |  %5d %llu \n", sn, chxxx, nextIndex[bd][chxxx], events[eventIndex][i].energy, events[eventIndex][i].timestamp); 
      }

      if( nExhaushedCh == numTotCh ) {
        printf("######################### no more event to be built\n"); 
        break;
      } 
      printf("----- next bd : %d, ch : %d, next earlist Time : %llu.\n", earlistDigi, earlistCh, earlistTime);
      //printf("leftOver %llu, breakTime %llu \n", leftOverTime, breakTime);
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
  }while(nExhaushedCh < numTotCh);

  forceStop = false;

}

//^############################################### backward event builder

void MultiBuilder::FindLatestTimeAndCh(bool verbose){
  DebugPrint("%s", "MultiBuilder");
  latestTime = 0;
  latestDigi = -1;
  latestCh = -1;

  nExhaushedCh = 0;
  
  for( int i = 0; i < nData; i++){
    for( int j = 0; j < data[i]->GetNChannel(); j++ ) chExhaused[i][j] = false;

    for(unsigned int ch = 0; ch < data[i]->GetNChannel(); ch ++){
      if( nextIndex[i][ch] < 0  || data[i]->GetDataIndex(ch) < 0 || nextIndex[i][ch] <= lastBackWardIndex[i][ch] ) {
        nExhaushedCh ++;
        chExhaused[i][ch] = true;
        // printf(", exhanshed. %d \n", nExhaushedCh);
        continue;
      }

      unsigned long long time = data[i]->GetTimestamp(ch, nextIndex[i][ch]);
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
  
void MultiBuilder::FindLatestTimeOfData(bool verbose){
  DebugPrint("%s", "MultiBuilder");
  latestTime = 0;
  latestCh = -1;
  latestDigi = -1;
  for( int i = 0; i < nData; i++){
    // printf("%s | digi-%d-th | %d\n", __func__, i, data[i]->GetNChannel());
    for( unsigned ch = 0; ch < data[i]->GetNChannel(); ch++ ){
      int index = data[i]->GetDataIndex(ch);
      // printf("ch-%2d | index : %d \n", ch, index);
      if( index == -1 ) continue;
      if( data[i]->GetTimestamp(ch, index) > latestTime ) {
        latestTime = data[i]->GetTimestamp(ch, index);
        latestCh = ch;
        latestDigi = i;
      }
    }
  }
  if( verbose )  printf("%s | bd : %d, ch : %d, %lld \n", __func__, latestDigi, latestCh, latestTime);
}

void MultiBuilder::BuildEventsBackWard(int maxNumEvent, bool verbose){
  DebugPrint("%s", "MultiBuilder");
  //skip trace, and only build for maxNumEvent events max

  // Get the last data index and loop index

  for( int k = 0; k < nData; k++){
    for( int i = 0; i < data[k]->GetNChannel(); i++){
      nextIndex[k][i] = data[k]->GetAbsDataIndex(i);
    }
  }

  FindLatestTimeAndCh(verbose);

  //========== build event
  eventBuilt = 0;
  Hit em;
  do{
    if( forceStop ) break;
    eventIndex ++;
    if( eventIndex >= MaxNEvent ) eventIndex = 0;
    events[eventIndex].clear();

    em.Clear();

    for( int k = 0; k < nData; k++){
      int bd = (k + latestDigi) % nData;

      const int numCh = data[k]->GetNChannel();

      for( int i = 0; i < numCh; i++){
        int ch = (i + latestCh) % numCh;
        if( chExhaused[bd][ch] ) continue;
        if( nextIndex[bd][ch] <= lastBackWardIndex[bd][ch] || nextIndex[bd][ch] <= 0){
          nExhaushedCh ++;
          chExhaused[bd][ch] = true;
          continue;
        }

        do{

          unsigned long long time = data[bd]->GetTimestamp(ch, nextIndex[bd][ch]);
          if( time <= latestTime && (latestTime - time <= timeWindow)){
            em.sn = snList[bd];
            em.ch = ch;
            em.energy = data[bd]->GetEnergy(ch, nextIndex[bd][ch]);
            em.timestamp = time;
            if( typeList[bd] == DPPTypeCode::DPP_PSD_CODE ) em.energy2 = data[bd]->GetEnergy2(ch, nextIndex[bd][ch]);

            events[eventIndex].push_back(em);
            nextIndex[bd][ch]--;
            // if( nextIndex[bd][ch] < 0 && data[bd]->GetLoopIndex(ch) > 0 ) nextIndex[bd][ch] = dataSize[bd] - 1;
            
          }else{
            break;
          }
          if( timeWindow == 0 ) break;
        }while(true);
        if( timeWindow == 0 ) break;
      }
      if( timeWindow == 0 ) break;
    }


    FindLatestTimeAndCh(verbose);

    if( verbose ) printf(" nExhaushedCh %d | numToCh %d \n", nExhaushedCh, numTotCh);
    if( nExhaushedCh == numTotCh  ) {
      if( verbose ) printf("######################### no more event to be built\n"); 
      break;
    } 
    if( verbose ) printf("----- next bd: %d, ch : %d, next latest Time : %llu.\n", latestDigi, latestCh, latestTime);

    if( events[eventIndex].size() > 0 ) {
      eventBuilt ++;
      totalEventBuilt ++;

      std::sort(events[eventIndex].begin(), events[eventIndex].end(), [](const Hit& a, const Hit& b) {
        return a.timestamp < b.timestamp;
      });
    }else{
      continue;
    }

    if( verbose ){
      printf(">>>>>>>>>>>>>>>>> Event ID : %ld, total built: %ld, multiplicity : %ld\n", eventIndex, totalEventBuilt, events[eventIndex].size());
      for( int i = 0; i <(int) events[eventIndex].size(); i++){
        int chxxx = events[eventIndex][i].ch;
        int sn = events[eventIndex][i].sn;
        int bd = 0;
        for( int pp = 0; pp < nData; pp++){
          if( sn == data[pp]->boardSN ) {
            bd = pp;
            break;
          }
        }
        printf("%5d, %02d | %7ld |  %5d %llu \n", sn, chxxx, nextIndex[bd][chxxx], events[eventIndex][i].energy, events[eventIndex][i].timestamp); 
      }
    }

  }while(nExhaushedCh < numTotCh && eventBuilt < maxNumEvent);

  forceStop = false;

  // remember the end of DataIndex, prevent over build
  for( int k = 0; k < nData; k++){
    for( int i = 0; i < data[k]->GetNChannel(); i++){
      lastBackWardIndex[k][i] = data[k]->GetAbsDataIndex(i);
    }
  }

}
