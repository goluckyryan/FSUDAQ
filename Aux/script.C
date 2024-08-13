#include "fsuReader.h"
// #include "../MultiBuilder.cpp"

#include "SplitPolePlotter.C"

void script(){

 TChain * chain = new TChain("tree");

 chain->Add("data/12C_dp_009_3000.root");
//  chain->Add("run13._3000.root");

 SplitPolePlotter(chain);


  //^=====================================================

  // FSUReader * reader = new FSUReader("data/12C_dp_002_19555_PSD_4_000.fsu", 10000, 2);

  // reader->ScanNumBlock(1, 0);

  // reader->ReadNextBlock(0, 9);

  // for( int i = 0; i < 10 ; i++ ) reader->ReadNextBlock(0, 9);

  // std::vector<Hit> hitList = reader->ReadBatch(10, true);

  // for ( int i = 0; i < 10 ; i ++) hitList[i].Print();

  // // int ch = 5;
  // // std::vector<unsigned long long > tList;
  // // int nEvent = 0;
  // // for( int i = 0; i < data->TotNumNonPileUpEvents[ch]; i++){
  // //     tList.push_back(data->Timestamp[ch][i]);
  // //     printf("%3d | %d %llu \n", i, data->Energy[ch][i], data->Timestamp[ch][i]);
  // //     nEvent ++;
  // // }

  // // std::sort(tList.begin(), tList.end());

  // // unsigned long long dTime = tList.back() - tList.front();
  // // double sec = dTime * data->tick2ns / 1e9;

  // // printf("=========== %llu, %llu = %llu | %f sec | %f Hz\n", tList.back(), tList.front(), dTime, sec, nEvent/sec );

  // //data->PrintStat(0);


  // data->ClearData();
  // data->ClearTriggerRate();


  // MultiBuilder * mb = new MultiBuilder(data, reader->GetDPPType(), 334);
  // mb->SetTimeWindow(10000);
  
  // unsigned long totNumBlock = reader->GetTotNumBlock();
  
  // int lastDataIndex = 0;
  // int lastLoopIndex = 0;

  // for( unsigned long i = 0; i < 2; i++){

  //     reader->ReadNextBlock();
      
  //     // int maxDataIndex = 0;
  //     // int maxLoopIndex = 0;

  //     // for( int ch = 0; ch < 16 ; ch++){
  //     //     if( data->DataIndex[ch] > maxDataIndex ) maxDataIndex = data->DataIndex[ch];
  //     //     if( data->LoopIndex[ch] > maxLoopIndex ) maxLoopIndex = data->LoopIndex[ch];
  //     // }

  //     // if( (maxLoopIndex * MaxNData + maxDataIndex) - (lastLoopIndex * MaxNData + lastDataIndex) > MaxNData * 0.05){
          
  //     //     printf("Agg ID : %lu \n", i );

  //     //     data->PrintStat();
  //     //     data->PrintAllData();

  //     //     mb->BuildEvents();
  //     //     mb->PrintAllEvent();
  //     //     mb->PrintStat();

  //     //     lastDataIndex = maxDataIndex;
  //     //     lastLoopIndex = maxLoopIndex;
  //     // }

  // }


  // data->PrintStat();
  // data->PrintAllData();
  
  
  // //mb->BuildEvents(true);

  // mb->BuildEventsBackWard(300);

  // mb->PrintAllEvent();
  // mb->PrintStat();


  // delete mb;
  // delete reader;

}
