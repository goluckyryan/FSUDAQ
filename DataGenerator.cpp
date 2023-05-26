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
  std::uniform_int_distribution<int> RanCh(1, 2);
  std::uniform_int_distribution<unsigned short> RanEnergy(1, 1000);
  std::uniform_int_distribution<unsigned long long> RanTime(1, 50);

  int count = 0;
  unsigned long long time = 0;
  do{

    int ch = RanCh(gen);
    unsigned short energy = RanEnergy(gen);

    unsigned long long timestamp = time + RanTime(gen) + RanNext(gen)*100;
    time = timestamp;

    count ++;

    data->DataIndex[ch] ++;
    int index = data->DataIndex[ch];

    if( ch == 2 && index > 2) {
      data->DataIndex[ch] --;
      continue;
    }
    data->Energy[ch][index] = energy;
    data->Timestamp[ch][index] = timestamp;

    printf("%02d | %5d %10lld\n", ch, energy, time);

  }while( count < 50 );

  data->PrintAllData();

  printf("===================================\n");

  OnlineEventBuilder * eb = new OnlineEventBuilder(digi);

  eb->BuildEvents(100);


  delete digi;

}