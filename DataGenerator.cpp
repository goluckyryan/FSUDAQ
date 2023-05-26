#include <cstdio>
#include <random>

#include "macro.h"
#include "ClassDigitizer.h"
#include "OnlineEventBuilder.h"

int main(){

  Digitizer * digi = new Digitizer();

  Data * data = digi->GetData();

  std::random_device rd;
  std::mt19937 gen(rd());

  std::uniform_int_distribution<int> RanNext(0, 1);
  std::uniform_int_distribution<int> RanCh(0, 4);
  std::uniform_int_distribution<unsigned short> RanEnergy(1, 1000);
  std::uniform_int_distribution<unsigned long long> RanTime(1, 50);

  OnlineEventBuilder * eb = new OnlineEventBuilder(digi);
  unsigned long long time = 0;

  for( int q = 0; q < 3; q ++ ){
    int count = 0;
    do{

      int ch = RanCh(gen);
      unsigned short energy = RanEnergy(gen);

      unsigned long long timestamp = time + RanTime(gen) + RanNext(gen)*100;
      time = timestamp;

      count ++;

      data->DataIndex[ch] ++;
      if( data->DataIndex[ch] > MaxNData ) {
        data->LoopIndex[ch] ++;
        data->DataIndex[ch] = 0;
      }
      int index = data->DataIndex[ch];

      // if( ch == 2 && index > 2) {
      //   data->DataIndex[ch] --;
      //   continue;
      // }
      data->Energy[ch][index] = energy;
      data->Timestamp[ch][index] = timestamp;

      printf("%02d | %5d %10lld\n", ch, energy, time);

    }while( count < 40 );

    data->PrintAllData();

    printf("===================================\n");

    eb->BuildEvents(100, true);

  }

  delete eb;
  delete digi;

}