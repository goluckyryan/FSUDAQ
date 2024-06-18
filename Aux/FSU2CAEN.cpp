#include "fsuReader.h"

struct FileInfo{
  std::string fileName;
  int fileevID;
  unsigned long hitCount;
};

unsigned short header = 0xCAE1;
unsigned int flags = 0;

#define minNARG  4

//^#############################################################
//^#############################################################
int main(int argc, char **argv) {
  
  printf("=========================================\n");
  printf("===      *.fsu to CoMPASS bin         ===\n");
  printf("===      no trace, no flags           ===\n");
  printf("=========================================\n");  
  if (argc < minNARG)    {
    printf("Incorrect number of arguments:\n");
    printf("%s [tar] [batchSize] [inFile1] [inFile2] .... \n", argv[0]);
    printf("           tar : output tar, 0 = no, 1 = yes \n");   
    printf("     batchSize : the size of hit in a batch \n");   
    printf("\n");
    printf(" Example: %s 100000 '\\ls -1 *001*.fsu'\n", argv[0]);
    printf("\n\n");

    return 1;
  }
  unsigned int debug = false;

  uInt runStartTime = getTime_us();

  ///============= read input
  // long timeWindow = atoi(argv[1]);
  // bool traceOn = atoi(argv[2]);
  bool tarFlag = atoi(argv[1]);
  unsigned int batchSize = atoi(argv[2]);
  int nFile = argc - minNARG + 1;
  std::string inFileName[nFile];
  for( int i = 0 ; i < nFile ; i++){ inFileName[i] = argv[i+ minNARG - 1];}


  std::string temp = inFileName[0];
  size_t pos = temp.find('_');
  pos = temp.find('_', pos + 1);
  std::string outFile_prefix = temp.substr(0, pos);
  std::string outFileName = outFile_prefix + ".BIN";

  
  printf("========================================= Number of Files : %d \n", nFile);
  for( int i = 0; i < nFile; i++) printf("%2d | %s \n", i, inFileName[i].c_str());
  printf("=========================================\n"); 
  printf("     Batch size = %d events/file\n", batchSize);
  printf("  Out file name = %s \n", outFileName.c_str());
  printf("  Is tar output = %s \n", tarFlag ? "Yes" : "No");
  printf("========================================= Grouping files\n");  

  std::vector<std::vector<FileInfo>> fileGroupList; // fileName and evID = SN * 1000 + index
  std::vector<FileInfo> fileList;

  unsigned long long int totalHitCount = 0;

  FSUReader * readerA = new FSUReader(inFileName[0], 1, 1);
  readerA->ScanNumBlock(0,0);
  FileInfo fileInfo = {inFileName[0], readerA->GetSN() * 1000 +  readerA->GetFileOrder(), readerA->GetHitCount()};
  fileList.push_back(fileInfo);
  totalHitCount += readerA->GetHitCount();

  for( int i = 1; i < nFile; i++){
    FSUReader * readerB = new FSUReader(inFileName[i], 1, 1);
    readerB->ScanNumBlock(0,0);
    totalHitCount += readerB->GetHitCount();
    fileInfo = {inFileName[i], readerB->GetSN() * 1000 +  readerB->GetFileOrder(), readerB->GetHitCount()};
    
    if( readerA->GetSN() == readerB->GetSN() ){
      fileList.push_back(fileInfo);
    }else{
      fileGroupList.push_back(fileList);
      fileList.clear();
      fileList.push_back(fileInfo);
    }
  
    delete readerA;
    readerA = readerB;
  }
  fileGroupList.push_back(fileList);
  delete readerA;

  printf("======================= total Hit Count : %llu\n", totalHitCount);

  for( size_t i = 0; i < fileGroupList.size(); i++){
    printf("group ----- %ld \n", i);

    //sort by evID
    std::sort(fileGroupList[i].begin(), fileGroupList[i].end(), [](const FileInfo & a, const FileInfo & b) {
      return a.fileevID < b.fileevID;
    });

    for( size_t j = 0; j < fileGroupList[i].size(); j++){
      printf("%3ld | %8d | %9lu| %s \n", j, fileGroupList[i][j].fileevID, fileGroupList[i][j].hitCount, fileGroupList[i][j].fileName.c_str() );
    }

  }

  //*======================================= Open files
  printf("========================================= Open files & Build Events.\n"); 

  const short nGroup = fileGroupList.size();
  std::vector<Hit> hitList[nGroup];
  
  FSUReader ** reader = new FSUReader * [nGroup];
  ulong evID[nGroup];
  for( short i = 0; i < nGroup; i++){
    std::vector<std::string> fList;
    for( size_t j = 0; j < fileGroupList[i].size(); j++){
      fList.push_back( fileGroupList[i][j].fileName );
    }
    reader[i] = new FSUReader(fList, 600, debug);
    hitList[i] = reader[i]->ReadBatch(batchSize, debug );
    reader[i]->PrintHitListInfo(&hitList[i], "hitList-" + std::to_string(reader[i]->GetSN()));
    evID[i] = 0;
    if( debug ) {

      for( size_t p = 0; p < 10; p ++ ){
        if( hitList[i].size() <= p ) break;
        hitList[i][p].Print();
      }

    }
  }

  unsigned long long tStart = 0;
  unsigned long long tEnd = 0;

  unsigned long long t0 = -1;
  short g0 = 0 ;
  int nFileFinished = 0;
  unsigned long long hitProcessed = 0;

  FILE * outFile = fopen(outFileName.c_str(), "wb");

  do{

    // find the earlist time
    t0 = -1;
    for( short i = 0; i < nGroup; i++){

      if( hitList[i].size() == 0 ) continue;

      //chekc if reached the end of hitList
      if( evID[i] >= hitList[i].size() ) {
        hitList[i] = reader[i]->ReadBatch(batchSize, debug + 1);
        if( debug ) reader[i]->PrintHitListInfo( &hitList[i], "hitList-" + std::to_string(i));
        evID[i] = 0;
        if( hitList[i].size() == 0 ) continue;
      }

      if( hitList[i][evID[i]].timestamp < t0 ) {
        t0 = hitList[i][evID[i]].timestamp;
        g0 = i;
      }
      
    }

    if( hitList[g0][evID[g0]].energy2 > 0 ) header += 4;
    if( hitList[g0][evID[g0]].traceLength > 0 ) header += 8;
    if( hitList[g0][evID[g0]].pileUp ) flags += 0x8000;
    if( hitList[g0][evID[g0]].fineTime > 0 ) flags += 0x4000;

    fwrite(&(header), 2, 1, outFile);
    fwrite(&(hitList[g0][evID[g0]].sn), 2, 1, outFile);
    fwrite(&(hitList[g0][evID[g0]].ch), 2, 1, outFile);
    unsigned psTimestamp = hitList[g0][evID[g0]].timestamp * 1000 + hitList[g0][evID[g0]].fineTime;
    fwrite(&(psTimestamp), 8, 1, outFile);
    fwrite(&(hitList[g0][evID[g0]].energy), 2, 1, outFile);
    if( hitList[g0][evID[g0]].energy2 > 0 ) fwrite(&(hitList[g0][evID[g0]].energy2), 2, 1, outFile);
    fwrite(&(flags), 4, 1, outFile);
    if( hitList[g0][evID[g0]].traceLength > 0 ){
      char waveCode = 1;
      fwrite(&(waveCode), 1, 1, outFile);
      fwrite(&(hitList[g0][evID[g0]].traceLength), 4, 1, outFile);

      for( int i = 0; i < hitList[g0][evID[g0]].traceLength; i++ ){
        fwrite(&(hitList[g0][evID[g0]].trace[i]), 2, 1, outFile);
      }
    }

    evID[g0]++;
    if( hitProcessed == 0) tStart = hitList[g0][evID[g0]].timestamp;
    hitProcessed ++;
    if( hitProcessed % 10000 == 0 ) printf("hit Porcessed %llu/%llu hit....%.2f%%\n\033[A\r", hitProcessed, totalHitCount, hitProcessed*100./totalHitCount);

    if( hitProcessed == totalHitCount -1 ) tEnd = hitList[g0][evID[g0]].timestamp;

    //*=============
    nFileFinished = 0;
    for( int i = 0 ; i < nGroup; i++) {
      if( hitList[i].size() == 0 ) {
        nFileFinished ++;
        continue;
      }else{
        if( evID[i] >= hitList[i].size( )) {
          hitList[i] = reader[i]->ReadBatch(batchSize, debug);
          evID[i] = 0;
          if( hitList[i].size() == 0 ) nFileFinished ++;
        }
      }
    }
    if( debug > 1 ) printf("========== nFileFinished : %d\n", nFileFinished);

  }while( nFileFinished < nGroup);

  uInt runEndTime = getTime_us();
  double runTime = (runEndTime - runStartTime) * 1e-6;
  printf("========================================= finished.\n");
  printf(" event building time = %.2f sec = %.2f min\n", runTime, runTime/60.);
  printf("           total hit = %llu \n", hitProcessed);
  double tDuration_sec = (tEnd - tStart) * 1e-9;
  printf("     first timestamp = %20llu ns\n", tStart);
  printf("      last timestamp = %20llu ns\n", tEnd);
  printf(" total data duration = %.2f sec = %.2f min\n", tDuration_sec, tDuration_sec/60.);
  printf("==============> saved to %s \n", outFileName.c_str());
  
  for( int i = 0; i < nGroup; i++) delete reader[i];
  delete [] reader;

  if( tarFlag ){
    std::string tarFileName = outFile_prefix + ".tar";

    std::string command = "tar -cvf " + tarFileName + " " + outFileName;
    int result = std::system(command.c_str());

    if (result == 0) {
      printf("Archive created successfully: %s\n", tarFileName.c_str());
      std::system(("rm -f " + outFileName).c_str());
      printf("Remove %s.\n", outFileName.c_str());
    } else {
      printf("Error creating archive\n");
    }
  }

  printf("============================================== end of program\n");

  return 0;

}

