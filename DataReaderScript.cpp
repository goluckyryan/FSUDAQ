#include "ClassData.h"
#include "MultiBuilder.h"


void DataReaderScript(){

  Data * data = new Data();  
  data->DPPType = V1730_DPP_PSD_CODE;

  std::string fileName = "data/temp_043_089_PSD_000.fsu";

  FILE * haha = fopen(fileName.c_str(), "r");
  fseek(haha, 0L, SEEK_END);
  const long inFileSize = ftell(haha);
  printf("%s | file size : %ld Byte = %.2f MB\n", fileName.c_str() , inFileSize, inFileSize/1024./1024.);


  fseek(haha, 0, SEEK_SET);

  MultiBuilder * mb = new MultiBuilder(data, V1730_DPP_PSD_CODE);

  char * buffer = nullptr;
  int countBdAgg = 0;

  do{

    long fPos1 = ftell(haha);

    unsigned int word[1]; /// 4 bytes
    size_t dummy = fread(word, 4, 1, haha);
    fseek(haha, -4, SEEK_CUR);
    short header = ((word[0] >> 28 ) & 0xF);
    if( header != 0xA ) break;
    
    unsigned int aggSize = (word[0] & 0x0FFFFFFF) * 4; ///byte    
    buffer = new char[aggSize];
    dummy = fread(buffer, aggSize, 1, haha);
    if( dummy != 1) printf("fread error, should read 4 bytes, but read %ld x 4 byte\n", dummy);

    long fPos2 = ftell(haha);

    countBdAgg ++;
    printf("Board Agg. has %d word  = %d bytes | %ld - %ld\n", aggSize/4, aggSize, fPos1, fPos2);    
    printf("==================== %d Agg\n", countBdAgg);    

    data->DecodeBuffer(buffer, aggSize, false, 0); // data own the buffer
    data->ClearBuffer(); // this will clear the buffer.

    //if( countBdAgg % 100 == 0) data->PrintStat();
    //data->ClearData();

    if( countBdAgg > 500 ){
      data->PrintAllData();
      mb->BuildEventsBackWard(false);
      break;
    }

    
  }while(!feof(haha) && ftell(haha) < inFileSize);

  data->PrintStat();

  fclose(haha);

  mb->PrintStat();

  delete mb;
  delete data;

}

int main(){

  DataReaderScript();

  return 0;

}