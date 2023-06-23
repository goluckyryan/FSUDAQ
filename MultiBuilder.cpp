#include "MultiBuilder.h"

#include <algorithm>


MultiBuilder::MultiBuilder(Digitizer ** digi, unsigned int nDigi) : nData(nDigi){
  data = new Data *[nData];
  for( unsigned int i = 0; i < nData; i++){
    data[i] = digi[i]->GetData();
    typeList.push_back(digi[i]->GetDPPType());
    snList.push_back(digi[i]->GetSerialNumber());
    idList.push_back(i);
    timeWindow = 100;
    ClearEvents();
  } 
}

MultiBuilder::MultiBuilder(Digitizer ** digi, std::vector<int> id) : nData(id.size()){
  data = new Data *[nData];
  idList = id;
  for( unsigned int i = 0; i < nData; i++){
    int k = idList[i];
    data[i] = digi[k]->GetData();
    //TODO type and sn should be inside data[i]
    typeList.push_back(digi[k]->GetDPPType());
    snList.push_back(digi[k]->GetSerialNumber());
    timeWindow = 100;
    ClearEvents();
  } 
}

MultiBuilder::MultiBuilder(Data ** multiData, std::vector<int> type, std::vector<int> sn) : nData(type.size()){
  data = multiData;
  typeList = type;
  snList = sn;
  for( int i = 0; i < (int) type.size(); i++) idList.push_back(i);
  timeWindow = 100;
  ClearEvents();
}

MultiBuilder::MultiBuilder(Digitizer ** digi, int digiID) : nData(1){
  data = new Data *[nData];
  data[0] = digi[digiID]->GetData();
  typeList.push_back(digi[digiID]->GetDPPType());
  snList.push_back(digi[digiID]->GetSerialNumber());
  idList.push_back(digiID);
  timeWindow = 100;
  ClearEvents();
}

MultiBuilder::MultiBuilder(Digitizer * digi) : nData(1){
  data = new Data *[1];
  data[0] = digi->GetData();
  typeList.push_back(digi->GetDPPType());
  snList.push_back(digi->GetSerialNumber());
  idList.push_back(0);  
  timeWindow = 100;
  ClearEvents();
}

MultiBuilder::MultiBuilder(Data * singleData, int type): nData(1){
  data = new Data *[1];
  data[0] = singleData;
  typeList.push_back(type);
  snList.push_back(0);
  idList.push_back(0);  
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

void MultiBuilder::PrintStat(){

  printf("Total number of evet built : %ld\n", totalEventBuilt);
  for( int i = 0; i < nData ; i++){
    for( int ch = 0; ch < MaxNChannels ; ch++){
      printf("%d %d | %d \n", i, ch, nextIndex[i][ch]);
    }
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

void MultiBuilder::FindLatestTimeAndCh(bool verbose){

  latestTime = 0;
  latestDigi = -1;
  latestCh = -1;

  nExhaushedCh = 0;
  
  for( int i = 0; i < nData; i++){

    for( int j = 0; j < MaxNChannels; j++ ){
      chExhaused[i][j] = false;
    }

    for(unsigned int ch = 0; ch < MaxNChannels; ch ++){
      // printf(" %d, %d | %d", i, ch, nextIndex[i][ch]);
      if( nextIndex[i][ch] < 0 ) {
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

void MultiBuilder::FindLatestTimeOfData(bool verbose){
  latestTime = 0;
  latestCh = -1;
  latestDigi = -1;
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

  FindLatestTimeOfData(verbose);

  FindEarlistTimeAndCh(verbose);
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
            em.sn = snList[k];
            em.bd = k;
            em.ch = ch;
            em.energy = data[k]->Energy[ch][nextIndex[k][ch]];
            em.timestamp = time;
            em.fineTime = data[k]->fineTime[ch][nextIndex[k][ch]];

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

    std::sort(events[eventIndex].begin(), events[eventIndex].end(), [](const Hit& a, const Hit& b) {
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

void MultiBuilder::BuildEventsBackWard(int maxNumEvent, bool verbose){

  //skip trace, and only build for 100 events max

  // remember the end of DataIndex, prevent over build
  for( int k = 0; k < nData; k++){
    for( int i = 0; i < MaxNChannels; i++){
      nextIndex[k][i] = data[k]->DataIndex[i];
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
      for( unsigned int i = 0; i < MaxNChannels; i++){
        int ch = (i + latestCh) % MaxNChannels;
        if( chExhaused[k][ch] ) continue;
        //if( nextIndex[k][ch] <= lastBackWardIndex[k][ch] || nextIndex[k][ch] < 0){
        if( nextIndex[k][ch] < 0){
          chExhaused[k][ch] = true;
          nExhaushedCh ++;
          continue;
        }

        do{

          unsigned long long time = data[k]->Timestamp[ch][nextIndex[k][ch]];
          if( time <= latestTime && (latestTime - time <= timeWindow)){
            em.sn = snList[k];
            em.bd = k;
            em.ch = ch;
            em.energy = data[k]->Energy[ch][nextIndex[k][ch]];
            em.timestamp = time;
            if( typeList[k] == V1730_DPP_PSD_CODE ) em.energy2 = data[k]->Energy2[ch][nextIndex[k][ch]];

            events[eventIndex].push_back(em);
            nextIndex[k][ch]--;
            if( nextIndex[k][ch] < 0 && data[k]->LoopIndex[ch] > 0 ) nextIndex[k][ch] = MaxNData - 1;
            
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
      printf("----- next ch : %d, next latest Time : %llu.\n", latestCh, latestTime);

    }

  }while(nExhaushedCh < nData * MaxNChannels && eventBuilt <= maxNumEvent);

  // // remember the end of DataIndex, prevent over build
  // for( int k = 0; k < nData; k++){
  //   for( int i = 0; i < MaxNChannels; i++){
  //     lastBackWardIndex[k][i] = data[k]->DataIndex[i];
  //   }
  // }

}
