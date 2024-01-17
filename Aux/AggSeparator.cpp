#ifndef AGGSEPARATOR_H
#define AGGSEPARATOR_H

#include <stdio.h>
#include <string>
#include <sstream>
#include <cmath>
#include <cstring>  ///memset
#include <iostream> ///cout
#include <sstream>
#include <iomanip> // for setw
#include <algorithm>
#include <bitset>
#include <vector>
#include <sys/stat.h>


int AggSeperator(std::string inFileName, short verbose = false){

  FILE * file = fopen(inFileName.c_str(), "r");

  if( file == NULL ) {
    printf("file : %s cannot be open. exit program.\n", inFileName.c_str());
    return -1;
  }

  std::string folder = "";
  size_t found = inFileName.find_last_of('/');
  std::string fileName = inFileName;
  if( found != std::string::npos ){
    folder = inFileName.substr(0, found + 1);
    fileName = inFileName.substr(found +1 );
  }

  // printf("  folder : %s \n", folder.c_str());
  // printf("fileName : %s \n", fileName.c_str());

  char * buffer = nullptr;
  unsigned int word; // 4 bytes = 32 bits

  bool newFileFlag[8] = {true};

  do{

    size_t dummy = fread(&word, 4, 1, file);
    if( dummy != 1 ){
      printf("End of File.\n");
      break;
    }

    short header = ((word >> 28 ) & 0xF);
    if( header != 0xA ) {
      printf("header error. abort.\n");
      break;
    }
    unsigned int aggSize = (word & 0x0FFFFFFF) * 4; ///byte

    dummy = fread(&word, 4, 1, file);
    unsigned int BoardID = ((word >> 27) & 0x1F);
    unsigned short pattern = ((word >> 8 ) & 0x7FFF );
    bool BoardFailFlag =  ((word >> 26) & 0x1 );
    unsigned int ChannelMask = ( word & 0xFF ) ;

    dummy = fread(&word, 4, 1, file);
    unsigned int bdAggCounter = word;

    if( verbose ) printf("Agg counter : %u\n", bdAggCounter);

    dummy = fread(&word, 4, 1, file);
    unsigned int aggTimeTag = word;

    for( int chMask = 0; chMask < 8 ; chMask ++ ){ // the max numnber of Coupled/RegChannel is 8 for PHA, PSD, QDC
      if( ((ChannelMask >> chMask) & 0x1 ) == 0 ) continue;
      if( verbose ) printf("==================== Dual/Group Channel Block, ch Mask : 0x%X\n", chMask *2);

      dummy = fread(&word, 4, 1, file);
      unsigned int dualChannelBlockSize = ( word & 0x7FFFFFFF ) * 4 ;

      if( verbose ) printf("dual channel size : %d words\n", dualChannelBlockSize / 4);

      buffer = new char[dualChannelBlockSize];
      fseek(file, -4, SEEK_CUR);
      dummy = fread(buffer, dualChannelBlockSize, 1, file);

      FILE * haha = nullptr;
      
      if( newFileFlag[chMask] ) {
        haha = fopen( (fileName + "." + std::to_string(chMask )).c_str(), "wb");
        newFileFlag[chMask] = false;
      }else{
        haha = fopen( (fileName + "." + std::to_string(chMask )).c_str(), "a+");
      }

      fwrite(buffer, dualChannelBlockSize, 1, haha);

      fclose(haha); 
      
    }

  }while( !feof(file));

  fclose(file);

  printf("======================= Duel channel seperated \n");

  return 1;

}

#endif