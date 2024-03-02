/*************

This can be loaded to root and run the DataReader()

***********/

#include "../ClassData.h"
#include "../MultiBuilder.h"

#include "TROOT.h"
#include "TFile.h"
#include "TTree.h"

int main(int argc, char **argv){

  printf("=========================================\n");
  printf("===         *.fsu to root             ===\n");
  printf("=========================================\n");  
  if (argc <= 3)    {
    printf("Incorrect number of arguments:\n");
    printf("%s [list of inFile\n", argv[0]);
    return 1;
  }

///============= read input
  int nFile = argc - 1;
  TString inFileName[nFile];
  for( int i = 0 ; i < nFile ; i++){
    inFileName[i] = argv[i+1];
  }
  
  /// Form outFileName;
  TString outFileName = inFileName[0];
  int pos = outFileName.Last('/');
  outFileName.Remove(0,pos+1);
  pos = outFileName.Index("_");
  pos = outFileName.Index("_", pos+1);
  outFileName.Remove(pos);  
  outFileName += "_single.root";
  printf("-------> Out file name : %s \n", outFileName.Data());
  
  
  printf(" Number of Files : %d \n", nFile);

  printf("===================================== input files:\n");  

  ///============= sorting file by the serial number & order
  int ID[nFile]; /// serial+ order*1000;
  int type[nFile];
  int sn[nFile];
  unsigned int fileSize[nFile];
  for( int i = 0; i < nFile; i++){
    int pos = inFileName[i].Last('/');
    int snPos = inFileName[i].Index("_", pos); // first "_"
    //snPos = inFileName[i].Index("_", snPos + 1);
    sn[i] = atoi(&inFileName[i][snPos+5]);
    TString typeStr = &inFileName[i][snPos+9];
    typeStr.Resize(3);

    if( typeStr == "PHA" ) type[i] = DPPTypeCode::DPP_PHA_CODE;
    if( typeStr == "PSD" ) type[i] = DPPTypeCode::DPP_PSD_CODE;
    if( typeStr == "QDC" ) type[i] = DPPTypeCode::DPP_QDC_CODE;

    int order = atoi(&inFileName[i][snPos+13]);
    ID[i] = sn[i] + order * 1000;

    FILE * temp = fopen(inFileName[i].Data(), "rb");
    if( temp == NULL ) {
      fileSize[i] = 0;
    }else{
      fseek(temp, 0, SEEK_END);
      fileSize[i] = ftell(temp);
    }
    fclose(temp);
  }

  for( int i = 0; i < nFile; i++) printf("%2d | %s %d %d %u\n", i, inFileName[i].Data(), ID[i], type[i], fileSize[i]);

  //*======================================= open raw files 
  printf("##############################################\n"); 

  FILE ** inFile = new FILE *[nFile];
  Data ** data = new Data *[nFile];
  for( int i = 0; i < nFile; i++){
    inFile[i] = fopen(inFileName[i].Data(), "r");
    if( inFile[i] ){
      if( type[i] == DPPTypeCode::DPP_PHA_CODE || type[i] == DPPTypeCode::DPP_PSD_CODE ) data[i] = new Data(16);
      if( type[i] == DPPTypeCode::DPP_QDC_CODE ) data[i] = new Data(64);
      data[i]->DPPType = type[i];
      data[i]->boardSN = ID[i]%1000;
    }else{
      data[i] = nullptr;
    }
  }


  char * buffer = nullptr;

  unsigned int word[1]; /// 4 bytes

  //============ tree
  TFile * rootFile = new TFile(outFileName, "recreate");
  TTree * tree = new TTree("tree", outFileName);

  unsigned short bd;
  unsigned short ch;
  unsigned short e;
  unsigned long long e_t;

  tree->Branch("bd", &bd, "bd/s");
  tree->Branch("ch", &ch, "ch/s");
  tree->Branch("e",   &e, "e/s");
  tree->Branch("e_t", &e_t, "e_t/l");


  //============ 
  for( int i = 0; i < nFile; i++){
    if( inFile[i] == nullptr ) continue;

    MultiBuilder * mb = new MultiBuilder(data[i], type[i], sn[i]);
    mb->SetTimeWindow(0);
    int countBdAgg = 0;

    do{

      //long fPos1 = ftell(inFile[i]);
      size_t dummy = fread(word, 4, 1, inFile[i]);
      if( dummy != 1) {
        printf("fread error, should read 4 bytes, but read %ld x 4 byte, file pos: %ld byte\n", dummy, ftell(inFile[i]));
        break;
      }

      fseek(inFile[i], -4, SEEK_CUR);
      short header = ((word[0] >> 28 ) & 0xF);
      if( header != 0xA ) break;

      unsigned int aggSize = (word[0] & 0x0FFFFFFF) * 4; ///byte
      buffer = new char[aggSize];
      dummy = fread(buffer, aggSize, 1, inFile[i]);
      if( dummy != 1) {
        printf("fread error, should read %d bytes, but read %ld x %d byte, file pos: %ld byte \n", aggSize, dummy, aggSize, ftell(inFile[i]));
        break;
      }

      //long fPos2 = ftell(inFile[i]);

      countBdAgg ++;
      // printf("Board Agg. has %d word  = %d bytes | %ld - %ld\n", aggSize/4, aggSize, fPos1, fPos2);    
      // printf("==================== %d Agg\n", countBdAgg);    

      data[i]->DecodeBuffer(buffer, aggSize, false, 0); // data own the buffer
      data[i]->ClearBuffer(); // this will clear the buffer.

      //============ save data into tree
      mb->BuildEvents(true);

      for(int k = 0; k < mb->eventBuilt; k++){
        bd  = ID[i]%1000;
        ch  = mb->events[k][0].ch;
        e   = mb->events[k][0].energy;
        e_t = mb->events[k][0].timestamp;

        tree->Fill();
      }

      mb->ClearEvents();
      data[i]->ClearData();


    }while(!feof(inFile[i]) && ftell(inFile[i]) < fileSize[i]);
    fclose(inFile[i]);

    printf("================ %s done | Total Agg. %d\n", inFileName[i].Data(), countBdAgg);
    //data[i]->PrintStat();
    //data->PrintAllData();

    //mb->BuildEvents(true, true, false);
    //mb->PrintStat();


    delete mb;

  }

  tree->Write();
  tree->Print();
  rootFile->Close();

  return 0;

}
