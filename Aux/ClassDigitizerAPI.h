#ifndef API_DIGITIZER_H
#define API_DIGITIZER_H

/**************************
 * 
 * This digiitizer class use only CAEN API instead of using register to control digitizer
 * 
 * This class is for V1743 or other digitizer that don't have open register table
 * 
***************************/

#include "../ClassDigitizer.h"

class DigitizerAPI : Digitizer {

public:
  DigitizerAPI(); // no digitizer open
  DigitizerAPI(int boardID, int portID = 0, bool program = false, bool verbose = false); 
  virtual ~DigitizerAPI();

  int ProgramBoard_V1743();

  int WriteRegisterBitmask(int32_t handle, uint32_t address, uint32_t data, uint32_t mask);

private: 

  int EventDecoding();


  ///==================================
  const float MIN_DAC_RAW_VALUE	= -1.25;
  const float MAX_DAC_RAW_VALUE	= +1.25;

  const unsigned short MAX_NUM_EVENTS_BLT = 1000;


};

#endif