/*************

This can be loaded to root and run the DataReader()

***********/

#include "ClassData.h"
#include "MultiBuilder.h"

void DataReader(std::string fileName, int DPPType){

  Data * data = new Data();  
  data->DPPType = DPPType;

  FILE * haha = fopen(fileName.c_str(), "r");
  fseek(haha, 0L, SEEK_END);
  const long inFileSize = ftell(haha);
  printf("%s | file size : %ld Byte = %.2f MB\n", fileName.c_str() , inFileSize, inFileSize/1024./1024.);


  fseek(haha, 0, SEEK_SET);

  MultiBuilder * mb = new MultiBuilder(data, DPPType);
  mb->SetTimeWindow(0);

  char * buffer = nullptr;
  int countBdAgg = 0;

  do{

    //long fPos1 = ftell(haha);

    unsigned int word[1]; /// 4 bytes
    size_t dummy = fread(word, 4, 1, haha);
    if( dummy != 1) {
      printf("fread error, should read 4 bytes, but read %ld x 4 byte, file pos: %ld byte\n", dummy, ftell(haha));
      break;
    }

    fseek(haha, -4, SEEK_CUR);
    short header = ((word[0] >> 28 ) & 0xF);
    if( header != 0xA ) break;
    
    unsigned int aggSize = (word[0] & 0x0FFFFFFF) * 4; ///byte    
    buffer = new char[aggSize];
    dummy = fread(buffer, aggSize, 1, haha);
    if( dummy != 1) {
      printf("fread error, should read %d bytes, but read %ld x %d byte, file pos: %ld byte \n", aggSize, dummy, aggSize, ftell(haha));
      break;
    }

    //long fPos2 = ftell(haha);

    countBdAgg ++;
    // printf("Board Agg. has %d word  = %d bytes | %ld - %ld\n", aggSize/4, aggSize, fPos1, fPos2);    
    // printf("==================== %d Agg\n", countBdAgg);    

    data->DecodeBuffer(buffer, aggSize, false, 0); // data own the buffer
    data->ClearBuffer(); // this will clear the buffer.

    //if( countBdAgg % 100 == 0) data->PrintStat();
    //data->ClearData();

    //if( countBdAgg > 10 ){
      //data->PrintAllData();
      
      //mb->BuildEvents(false, true, false);
      //mb->BuildEventsBackWard(false);
    //}

    
  }while(!feof(haha) && ftell(haha) < inFileSize);
  fclose(haha);

  printf("============================ done | Total Agg. %d\n", countBdAgg);
  data->PrintStat();
  //data->PrintAllData();

  mb->BuildEvents(true, true, false);

  mb->PrintStat();

  delete mb;
  delete data;

}

int main(int argc, char **argv){

  printf("=========================================\n");
  printf("===         *.fsu data reader         ===\n");
  printf("=========================================\n");  
  if (argc <= 1)    {
    printf("Incorrect number of arguments:\n");
    printf("%s [inFile]  [DPPType] \n", argv[0]);
    printf("                +-- PHA = %d\n", V1730_DPP_PHA_CODE);   
    printf("                +-- PSD = %d\n", V1730_DPP_PSD_CODE);   
    return 1;
  }


  DataReader(argv[1], atoi(argv[2]));

  return 0;

}