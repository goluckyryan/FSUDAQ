#include "ClassDigitizer.h"

Digitizer::Digitizer(){
  Initalization();
}

Digitizer::Digitizer(int boardID, int portID, bool program,  bool verbose){
  Initalization();
  OpenDigitizer(boardID, portID, program, verbose);
}

Digitizer::~Digitizer(){
  CloseDigitizer();
  delete data;
}

void Digitizer::Initalization(){

  data = new Data();
  
  portID = -1;
  boardID = -1;
  handle = -1;
  NChannel = 16;
  ADCbits  = 1;
  DPPType = 0;
  ADCFullSize = 0;
  ch2ns = 0; 
  BoardInfo = {};

  channelMask = 0xFFFF;
  VMEBaseAddress = 0;
  LinkType = CAEN_DGTZ_USB;      /// default USB
  IOlev = CAEN_DGTZ_IOLevel_NIM; ///default NIM  
  
  isSettingFilledinMemeory = false;
  settingFileName = "";
  settingFileExist = false;
  settingFile = NULL;
  
  ret = -1;
  isConnected = false; 
  AcqRun = false;
  isDummy = true;
}

void Digitizer::Reset(){
  ret = CAEN_DGTZ_Reset(handle);
  if( ret != 0 ) ErrorMsg(__func__);
  
  ret |= CAEN_DGTZ_WriteRegister(handle, Register::DPP::SoftwareClear_W, 1);
  if( ret != 0 ) ErrorMsg("Reset-SoftwareClear_W");

}

void Digitizer::PrintBoard (){
  printf("Connected to Model %s with handle %d using %s\n", BoardInfo.ModelName, handle, LinkType == CAEN_DGTZ_USB ? "USB" : "Optical Link");
  printf("Sampling rate : %.0f MHz = %.1f ns \n", 1000/ch2ns, ch2ns);
  printf("Number of Channels : %d = 0x%X\n", NChannel, channelMask);
  printf("SerialNumber :\e[1m\e[33m %d\e[0m\n", BoardInfo.SerialNumber);
  printf("DPPType : %d (%s)\n", DPPType, GetDPPString().c_str());
  printf("ADC bit is \e[33m%d\e[0m, %d = 0x%X\n", ADCbits, ADCFullSize, ADCFullSize);
  printf("ROC FPGA Release is %s\n", BoardInfo.ROC_FirmwareRel);
  printf("AMC FPGA Release is %s\n", BoardInfo.AMC_FirmwareRel);
}

int Digitizer::OpenDigitizer(int boardID, int portID, bool program, bool verbose){
  
  this->boardID = boardID;
  this->portID = portID;
  
  if( boardID < 0 || portID < 0 ) return 0; /// for using the Digitizer Class without open digitizer
    
  /***************************************************/
  /** Open the digitizer and read board information  */
  /***************************************************/

  if( verbose) printf("============= Opening Digitizer at Board %d, Port %d \n", boardID, portID);
  
  ///-------- try USB first
  LinkType = CAEN_DGTZ_USB;     /// Link Type
  ret = (int) CAEN_DGTZ_OpenDigitizer(LinkType, boardID, 0, VMEBaseAddress, &handle);
  if (ret != 0){ ///---------- try Optical link
    LinkType = CAEN_DGTZ_OpticalLink ; 
    ret = (int) CAEN_DGTZ_OpenDigitizer(LinkType, portID, boardID, VMEBaseAddress, &handle);
  }
  ErrorMsg("=== Open Digitizer port " +std::to_string(portID) + " board " + std::to_string(boardID));
  
  if (ret != 0) {
    if( verbose) printf("Can't open digitizer\n");
    return -1;
  }else{
    ///----- Getting Board Info
    ret = (int) CAEN_DGTZ_GetInfo(handle, &BoardInfo);
    if (ret != 0) {
      if( verbose) printf("Can't read board info\n");
    }else{
      isConnected = true;
      NChannel = BoardInfo.Channels;
      channelMask = pow(2, NChannel)-1;
      switch(BoardInfo.Model){
            case CAEN_DGTZ_V1730: ch2ns = 2.0; break; ///ns -> 500 MSamples/s
            case CAEN_DGTZ_V1725: ch2ns = 4.0; break; ///ns -> 250 MSamples/s
            default : ch2ns = 4.0; break;
      }
      data->ch2ns = ch2ns;
      data->boardSN = BoardInfo.SerialNumber;
      ADCbits = BoardInfo.ADC_NBits;
      ADCFullSize = (unsigned int)( pow(2, ADCbits) -1 );
    }
  }

  ///====================== Check DPP firmware revision 
  sscanf(BoardInfo.AMC_FirmwareRel, "%d", &DPPType);
  data->DPPType = DPPType;
  switch( DPPType ) {
    case 0x80 : data->DPPTypeStr = "PHA"; break;  // x724
    case 0x82 : data->DPPTypeStr = "xCI"; break;  // x720
    case 0x83 : data->DPPTypeStr = "PSD"; break;  // x720
    case 0x84 : data->DPPTypeStr = "PSD"; break;  // x751
    case 0x85 : data->DPPTypeStr = "ZLE"; break;  // x751
    case 0x86 : data->DPPTypeStr = "PSD"; break;  // x743
    case 0x87 : data->DPPTypeStr = "QDC"; break;  // x740
    case 0x88 : data->DPPTypeStr = "PSD"; break;  // x730
    case 0x89 : data->DPPTypeStr = "DAW"; break;  // x724
    case 0x8B : data->DPPTypeStr = "PHA"; break;  // x730
    case 0x8C : data->DPPTypeStr = "ZLE"; break;  // x730
    case 0x8D : data->DPPTypeStr = "DAW"; break;  // x730
    default : data->DPPTypeStr = "STD"; break; // stardard
  }
  /// change address 0xEF08 (5 bits), this will reflected in the 2nd word of the Board Agg. header.
  ret = CAEN_DGTZ_WriteRegister(handle, Register::DPP::BoardID, (DPPType & 0xF));
  if ( verbose ){
    PrintBoard();    
    if (DPPType < 0x80 ) {
      printf("This digitizer does not have DPP-PHA firmware\n");
    }else {
      printf("\t==== This digitizer has a DPP firmware!\n");    
      printf("\e[32m\t %s \e[0m", GetDPPString().c_str());
    }
  }
  ErrorMsg("========== Set BoardID");
  
  ///======================= Check virtual probe
  int probes[MAX_SUPPORTED_PROBES];
  int numProbes;
  ret = CAEN_DGTZ_GetDPP_SupportedVirtualProbes(handle, 1, probes, &numProbes);
  ErrorMsg("=== Get Supported Virtual Probes");
  if( verbose ){
    printf("\t==== supported virtual probe (number of Probe : %d)\n", numProbes);
    for( int i = 0 ; i < numProbes; i++){
      switch (probes[i]){
         case  0: printf("\t\t CAEN_DGTZ_DPP_VIRTUALPROBE_Input\n"); break;
         case  1: printf("\t\t CAEN_DGTZ_DPP_VIRTUALPROBE_Delta\n"); break;
         case  2: printf("\t\t CAEN_DGTZ_DPP_VIRTUALPROBE_Delta2\n"); break;
         case  3: printf("\t\t CAEN_DGTZ_DPP_VIRTUALPROBE_Trapezoid\n"); break;
         case  4: printf("\t\t CAEN_DGTZ_DPP_VIRTUALPROBE_TrapezoidReduced\n"); break;
         case  5: printf("\t\t CAEN_DGTZ_DPP_VIRTUALPROBE_Baseline\n"); break;
         case  6: printf("\t\t CAEN_DGTZ_DPP_VIRTUALPROBE_Threshold\n"); break;
         case  7: printf("\t\t CAEN_DGTZ_DPP_VIRTUALPROBE_CFD\n"); break;
         case  8: printf("\t\t CAEN_DGTZ_DPP_VIRTUALPROBE_SmoothedInput\n"); break;
         case  9: printf("\t\t CAEN_DGTZ_DPP_VIRTUALPROBE_None\n"); break;
         case 10: printf("\t\t CAEN_DGTZ_DPP_DIGITALPROBE_TRGWin\n"); break;
         case 11: printf("\t\t CAEN_DGTZ_DPP_DIGITALPROBE_Armed\n"); break;
         case 12: printf("\t\t CAEN_DGTZ_DPP_DIGITALPROBE_PkRun\n"); break;
         case 13: printf("\t\t CAEN_DGTZ_DPP_DIGITALPROBE_Peaking\n"); break;
         case 14: printf("\t\t CAEN_DGTZ_DPP_DIGITALPROBE_CoincWin\n"); break;
         case 15: printf("\t\t CAEN_DGTZ_DPP_DIGITALPROBE_BLHoldoff\n"); break;
         case 16: printf("\t\t CAEN_DGTZ_DPP_DIGITALPROBE_TRGHoldoff\n"); break;
         case 17: printf("\t\t CAEN_DGTZ_DPP_DIGITALPROBE_TRGVal\n"); break;
         case 18: printf("\t\t CAEN_DGTZ_DPP_DIGITALPROBE_ACQVeto\n"); break;
         case 19: printf("\t\t CAEN_DGTZ_DPP_DIGITALPROBE_BFMVeto\n"); break;
         case 20: printf("\t\t CAEN_DGTZ_DPP_DIGITALPROBE_ExtTRG\n"); break;
         case 21: printf("\t\t CAEN_DGTZ_DPP_DIGITALPROBE_OverThr\n"); break;
         case 22: printf("\t\t CAEN_DGTZ_DPP_DIGITALPROBE_TRGOut\n"); break;
         case 23: printf("\t\t CAEN_DGTZ_DPP_DIGITALPROBE_Coincidence \n"); break;
         case 24: printf("\t\t CAEN_DGTZ_DPP_DIGITALPROBE_PileUp \n"); break;
         case 25: printf("\t\t CAEN_DGTZ_DPP_DIGITALPROBE_Gate \n");  break;
         case 26: printf("\t\t CAEN_DGTZ_DPP_DIGITALPROBE_GateShort \n"); break;
         case 27: printf("\t\t CAEN_DGTZ_DPP_DIGITALPROBE_Trigger \n"); break;
         case 28: printf("\t\t CAEN_DGTZ_DPP_DIGITALPROBE_None  \n"); break;
         case 29: printf("\t\t CAEN_DGTZ_DPP_DIGITALPROBE_BLFreeze  \n"); break;
         case 30: printf("\t\t CAEN_DGTZ_DPP_DIGITALPROBE_Busy  \n"); break;
         case 31: printf("\t\t CAEN_DGTZ_DPP_DIGITALPROBE_PrgVeto \n"); break;
      }  
    }
  }
  
  ErrorMsg("end of OpenDigitizer");
  
  if( isConnected ) isDummy = false;
  
  if( isConnected  && program) {
    ProgramBoard();
  }
  
  if( isConnected ) ReadAllSettingsFromBoard(); 

  return ret;
}

int Digitizer::CloseDigitizer(){

  if( !isConnected ) return 0;
  isConnected = false;
  printf("-------- Closing Digtizer Board : %d Port : %d \n", boardID, portID);
  printf("  Model %s with handle %d using %s\n", BoardInfo.ModelName, handle, LinkType == CAEN_DGTZ_USB ? "USB" : "Optical Link");
  ret  = CAEN_DGTZ_SWStopAcquisition(handle);
  ret |= CAEN_DGTZ_CloseDigitizer(handle);
  
  return ret;
}


void Digitizer::SetChannelMask(uint32_t mask){
  if( !isConnected ) return;
  channelMask = mask;
  ret |= CAEN_DGTZ_SetChannelEnableMask(handle, channelMask);
  SaveSettingToFile(Register::DPP::ChannelEnableMask, mask);
  SetSettingToMemory(Register::DPP::ChannelEnableMask, mask);
  ErrorMsg(__func__);
}

void Digitizer::SetChannelOnOff(unsigned short ch, bool onOff){
  if( !isConnected ) return;
  channelMask = ((channelMask & ~( 1 << ch) ) | ( onOff << ch)) ;
  SetChannelMask(channelMask);
}

int Digitizer::ProgramBoard(){
  
  printf("----- program Board\n");
  ret = CAEN_DGTZ_Reset(handle);
  if (ret) {
    printf("ERROR: can't reset the digitizer.\n");
    return -1;
  }
  
  /// Board Configuration without PHA or PSD fireware
  ///bx0000 0000 0000 0000 0000 0000 0001 0000  = 
  ///                                 |   | +- (1) trigger overlap not allowed
  ///                                 |   +- (3) test pattern disable  
  ///                                 + (6) Self-trigger polarity, 1 = negative, 0 = Positive
  ret = CAEN_DGTZ_WriteRegister(handle,  (uint32_t) Register::BoardConfiguration , 0x000E0114);  /// Channel Control Reg (indiv trg, seq readout) ??
  
  /// Set the I/O level (CAEN_DGTZ_IOLevel_NIM or CAEN_DGTZ_IOLevel_TTL)
  ret |= CAEN_DGTZ_SetIOLevel(handle, IOlev);

  /// Set the enabled channels
  ret |= CAEN_DGTZ_SetChannelEnableMask(handle, channelMask);

  /// Set the number of samples for each waveform
  ret |= CAEN_DGTZ_SetRecordLength(handle, 2000);
  
  /// Set Extras 2 to enable, this override Accusition mode, focring list mode
  ret |= CAEN_DGTZ_WriteRegister(handle, Register::BoardConfiguration , 0x00E8114 );
  
  /// Set the digitizer acquisition mode (CAEN_DGTZ_SW_CONTROLLED or CAEN_DGTZ_S_IN_CONTROLLED)
  ret |= CAEN_DGTZ_SetAcquisitionMode(handle, CAEN_DGTZ_SW_CONTROLLED); /// software command
  
  CAEN_DGTZ_DPP_AcqMode_t  AcqMode = CAEN_DGTZ_DPP_ACQ_MODE_List; 
  ret |= CAEN_DGTZ_SetDPPAcquisitionMode(handle, AcqMode, CAEN_DGTZ_DPP_SAVE_PARAM_EnergyAndTime);

  /** Set the digitizer's behaviour when an external trigger arrives:
  CAEN_DGTZ_TRGMODE_DISABLED: do nothing
  CAEN_DGTZ_TRGMODE_EXTOUT_ONLY: generate the Trigger Output signal
  CAEN_DGTZ_TRGMODE_ACQ_ONLY = generate acquisition trigger
  CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT = generate both Trigger Output and acquisition trigger
  see CAENDigitizer user manual, chapter "Trigger configuration" for details */
  //TODO set bit
  ret |= CAEN_DGTZ_SetExtTriggerInputMode(handle, CAEN_DGTZ_TRGMODE_ACQ_ONLY);

  ret |= CAEN_DGTZ_SetRunSynchronizationMode(handle, CAEN_DGTZ_RUN_SYNC_Disabled);  

  /// Set how many events to accumulate in the board memory before being available for readout
  ret |= CAEN_DGTZ_WriteRegister(handle, Register::DPP::NumberEventsPerAggregate_G + 0x7000, 100);
  ret |= CAEN_DGTZ_WriteRegister(handle, Register::DPP::AggregateOrganization, 0);
  ret |= CAEN_DGTZ_WriteRegister(handle, Register::DPP::MaxAggregatePerBlockTransfer, 50);

  ErrorMsg(__func__);
  return ret;

}

int Digitizer::ProgramPHABoard(){
  
  ret = CAEN_DGTZ_Reset(handle);
  printf("======== program board PHA\n");

  ret = CAEN_DGTZ_WriteRegister(handle, Register::DPP::RecordLength_G + 0x7000, 62);  
  ret = CAEN_DGTZ_WriteRegister(handle, Register::DPP::BoardConfiguration, 0x0F8915);  /// has Extra2, dual trace, input and trap-baseline
  ///ret = CAEN_DGTZ_WriteRegister(handle, Register::DPP::BoardConfiguration, 0x0D8115);  /// diable Extra2
  
  //TODO change to write register
  ret = CAEN_DGTZ_SetAcquisitionMode(handle, CAEN_DGTZ_SW_CONTROLLED); /// software command
  ret |= CAEN_DGTZ_SetIOLevel(handle, CAEN_DGTZ_IOLevel_NIM);
  ret |= CAEN_DGTZ_SetExtTriggerInputMode(handle, CAEN_DGTZ_TRGMODE_ACQ_ONLY);

  ret = CAEN_DGTZ_SetChannelEnableMask(handle, 0xFFFF);
  
  //ret = CAEN_DGTZ_SetNumEventsPerAggregate(handle, 0);
  
  ret = CAEN_DGTZ_SetRunSynchronizationMode(handle, CAEN_DGTZ_RUN_SYNC_Disabled);
  if( ret != 0 ) { printf("==== set board error.\n"); return 0;}

  printf("======== program Channels PHA\n");
  
  uint32_t address;
  
  address = Register::DPP::PHA::DecayTime;               ret |= CAEN_DGTZ_WriteRegister(handle, address + 0x7000 , 5000 ); 
  address = Register::DPP::PHA::TrapezoidFlatTop;        ret |= CAEN_DGTZ_WriteRegister(handle, address + 0x7000 , 0x1A ); 
  address = Register::DPP::PHA::TrapezoidRiseTime;       ret |= CAEN_DGTZ_WriteRegister(handle, address + 0x7000 , 6 ); 
  address = Register::DPP::PHA::PeakingTime;             ret |= CAEN_DGTZ_WriteRegister(handle, address + 0x7000 , 6 ); 
  address = Register::DPP::PHA::RCCR2SmoothingFactor;    ret |= CAEN_DGTZ_WriteRegister(handle, address + 0x7000 , 4 ); 
  address = Register::DPP::PHA::InputRiseTime;           ret |= CAEN_DGTZ_WriteRegister(handle, address + 0x7000 , 6 ); 
  address = Register::DPP::PHA::TriggerThreshold;        ret |= CAEN_DGTZ_WriteRegister(handle, address + 0x7000 , 1000 );
  address = Register::DPP::PHA::PeakHoldOff;             ret |= CAEN_DGTZ_WriteRegister(handle, address + 0x7000 , 0x3E );
  address = Register::DPP::PHA::TriggerHoldOffWidth;     ret |= CAEN_DGTZ_WriteRegister(handle, address + 0x7000 , 0x3E );
  address = Register::DPP::PHA::RiseTimeValidationWindow;ret |= CAEN_DGTZ_WriteRegister(handle, address + 0x7000 , 0x0 );
  
    
  ret |= CAEN_DGTZ_WriteRegister(handle, (uint32_t)(Register::DPP::ChannelDCOffset) + 0x7000 , 0xEEEE );
  ret |= CAEN_DGTZ_WriteRegister(handle, (uint32_t)(Register::DPP::PreTrigger) + 0x7000 , 32 );
  ret |= CAEN_DGTZ_WriteRegister(handle, (uint32_t)(Register::DPP::InputDynamicRange) + 0x7000 , 0x0 );
  
  ret |= CAEN_DGTZ_WriteRegister(handle, (int32_t)(Register::DPP::NumberEventsPerAggregate_G) + 0x7000, 511);
  ret |= CAEN_DGTZ_WriteRegister(handle, (int32_t)(Register::DPP::AggregateOrganization), 2);
  ret |= CAEN_DGTZ_WriteRegister(handle, (int32_t)(Register::DPP::MaxAggregatePerBlockTransfer), 4);
  ret |= CAEN_DGTZ_WriteRegister(handle, (int32_t)(Register::DPP::DPPAlgorithmControl) + 0x7000, 0xC30200f);
  
  if( ret != 0 ) { printf("==== set channels error.\n"); return 0;}
  
  printf("End of program board and channels\n");
  
  isSettingFilledinMemeory = false; /// unlock the ReadAllSettingsFromBoard();
  ReadAllSettingsFromBoard();
  
  return ret;
}

//========================================================= ACQ control
void Digitizer::StartACQ(){
  if ( AcqRun ) return;
  
  unsigned int bufferSize = CalByteForBuffer();
  if( bufferSize  > 160 * 1024 * 1024 ){
    printf("============= buffer size bigger than 160 MB");
    return;
  }
  
  data->AllocateMemory(bufferSize);
  ret = CAEN_DGTZ_SWStartAcquisition(handle);
  
  if( ret != 0 ) {
    ErrorMsg("Start ACQ");
    return;
  }
  
  printf("\e[1m\e[33m======= Acquisition Started for Board %d\e[0m\n", boardID);
  AcqRun = true;
  data->ClearTriggerRate();
}

void Digitizer::StopACQ(){
  if( !AcqRun ) return;
  int ret = CAEN_DGTZ_SWStopAcquisition(handle);
  ret |= CAEN_DGTZ_ClearData(handle);
  if( ret != 0 ) ErrorMsg("something wrong when try to stop ACQ and clear buffer");
  printf("\n\e[1m\e[33m====== Acquisition STOPPED for Board %d\e[0m\n", boardID);
  AcqRun = false;
  data->ClearTriggerRate();
  data->ClearBuffer();
}

unsigned int Digitizer::CalByteForBuffer(){
  unsigned int numAggBLT;
  unsigned int chMask   ;    
  unsigned int boardCfg ;
  unsigned int eventAgg[NChannel/2];
  unsigned int recordLength[NChannel/2];
  unsigned int aggOrgan;
  
  if( isConnected ){
    numAggBLT = ReadRegister(Register::DPP::MaxAggregatePerBlockTransfer, 0, false);
    chMask    = ReadRegister(Register::DPP::ChannelEnableMask, 0, false);
    boardCfg  = ReadRegister(Register::DPP::BoardConfiguration, 0, false);
    aggOrgan  = ReadRegister(Register::DPP::AggregateOrganization, 0, false);

    for( int pCh = 0; pCh < NChannel/2; pCh++){
      eventAgg[pCh]     = ReadRegister(Register::DPP::NumberEventsPerAggregate_G, pCh * 2 , false);
      recordLength[pCh] = ReadRegister(Register::DPP::RecordLength_G, pCh * 2 , false);
    }
  }else{
    numAggBLT = GetSettingFromMemory(Register::DPP::MaxAggregatePerBlockTransfer);
    chMask    = GetSettingFromMemory(Register::DPP::ChannelEnableMask);
    boardCfg  = GetSettingFromMemory(Register::DPP::BoardConfiguration);
    aggOrgan  = GetSettingFromMemory(Register::DPP::AggregateOrganization);
    for( int pCh = 0; pCh < NChannel/2; pCh++){
      eventAgg[pCh]     = GetSettingFromMemory(Register::DPP::NumberEventsPerAggregate_G, pCh * 2 );
      recordLength[pCh] = GetSettingFromMemory(Register::DPP::RecordLength_G, pCh * 2);
    }
  }
  
  ///printf("      agg. orgainzation (bit) : 0x%X \n", aggOrgan);
  ///printf("                 Channel Mask : %04X \n", chMask);
  ///printf("Max number of Agg per Readout : %u \n", numAggBLT);
  ///printf("             is Extra2 enabed : %u \n", ((boardCfg >> 17) & 0x1) );
  ///printf("               is Record wave : %u \n", ((boardCfg >> 16) & 0x1) );
  ///for( int pCh = 0; pCh < NChannel/2; pCh++){
  ///  printf("Paired Ch : %d, RecordLength (bit value): %u, Event per Agg. : %u \n", pCh, recordLength[pCh], eventAgg[pCh]);
  ///}

  unsigned int bufferSize = aggOrgan; // just for get rip of the warning in complier
  bufferSize = 0;
  for( int pCh = 0; pCh < NChannel/2 ; pCh++){
    if( (chMask & ( 3 << (2 * pCh) )) == 0 ) continue; 
    bufferSize +=  2 + ( 2  + ((boardCfg >> 17) & 0x1) + ((boardCfg >> 16) & 0x1)*recordLength[pCh]*4 ) * eventAgg[pCh] ;
  }
  bufferSize += 4; /// Bd. Agg Header
  bufferSize = bufferSize * numAggBLT * 4; /// 1 words = 4 byte

  ///printf("=============== Buffer Size : %8d Byte \n", bufferSize );
  return  bufferSize ;
}

int Digitizer::ReadData(){
  if( !isConnected ) return CAEN_DGTZ_DigitizerNotFound;
  if( !AcqRun) return CAEN_DGTZ_WrongAcqMode;
  if( data->buffer == NULL ) {
    printf("need allocate memory for readout buffer\n");
    return CAEN_DGTZ_InvalidBuffer;
  }
  
  ret = CAEN_DGTZ_ReadData(handle, CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT, data->buffer, &(data->nByte));
  //uint32_t EventSize = ReadRegister(Register::DPP::EventSize); // Is it as same as data->nByte?
  //printf("Read Buffer size %d byte, Event Size : %d byte \n", data->nByte, EventSize);
  
  if (ret || data->nByte == 0) {
    ErrorMsg(__func__);
  }

  return ret;
}

void Digitizer::PrintACQStatue(){
  if( !isConnected ) return;
  unsigned int status = ReadRegister(Register::DPP::AcquisitionStatus_R);
  
  printf("=================== Print ACQ status \n");
  printf("  32  28  24  20  16  12   8   4   0\n");
  printf("   |   |   |   |   |   |   |   |   |\n");
  std::cout <<"  0b" << std::bitset<32>(status) << std::endl;
  printf(" Acq state    (0x%1X): %s \n", (status >>  2) & 0x1, ((status >>  2) & 0x1) == 0? "stopped" : "running");
  printf(" Event Ready  (0x%1X): %s \n", (status >>  3) & 0x1, ((status >>  3) & 0x1) == 0? "no event in buffer" : "event in buffer");
  printf(" Event Full   (0x%1X): %s \n", (status >>  4) & 0x1, ((status >>  4) & 0x1) == 0? "not full" : "full");
  printf(" Clock source (0x%1X): %s \n", (status >>  5) & 0x1, ((status >>  5) & 0x1) == 0? "internal" : "external");
  printf(" Board ready  (0x%1X): %s \n", (status >>  8) & 0x1, ((status >>  8) & 0x1) == 0? "not ready" : "ready");
  printf(" Ch shutDown  (0x%1X): %s \n", (status >> 19) & 0x1, ((status >> 19) & 0x1) == 0? "Channels are on" : "channels are shutdown");
  printf(" TRG-IN           0x%1X \n", (status >> 16) & 0x1);
  printf(" Ch temp state    0x%04X \n", (status >> 20) & 0xF);
}

//===========================================================
//===========================================================
//===========================================================
void Digitizer::WriteRegister (Register::Reg registerAddress, uint32_t value, int ch, bool isSave2MemAndFile){
  
  printf("%30s[0x%04X](ch-%02d) [0x%04X]: 0x%08X \n", registerAddress.GetNameChar(), registerAddress.GetAddress(),ch, registerAddress.ActualAddress(ch), value);

  if( !isConnected ) {
    SetSettingToMemory(registerAddress, value, ch);
    SaveSettingToFile(registerAddress, value, ch);
    return;
  }
  
  if( registerAddress.GetType() == Register::RW::ReadONLY ) return;
  
  ret = CAEN_DGTZ_WriteRegister(handle, registerAddress.ActualAddress(ch), value);
  if( ret == 0 && isSave2MemAndFile && registerAddress.GetType() == Register::RW::ReadWrite) {
    SetSettingToMemory(registerAddress, value, ch);
    SaveSettingToFile(registerAddress, value, ch);
  }
  
  ErrorMsg("WriteRegister:" + std::to_string(registerAddress));
}

uint32_t Digitizer::ReadRegister(Register::Reg registerAddress, unsigned short ch, bool isSave2MemAndFile, std::string str ){
  if( !isConnected )  return 0;
  if( registerAddress.GetType() == Register::RW::WriteONLY ) return 0;

  ret = CAEN_DGTZ_ReadRegister(handle, registerAddress.ActualAddress(ch), &returnData);
  
  if( ret == 0 && isSave2MemAndFile) {
    SetSettingToMemory(registerAddress, returnData, ch);
    SaveSettingToFile(registerAddress, returnData, ch);
  }
  ErrorMsg("ReadRegister:" + std::to_string(registerAddress));
  if( str != "" ) printf("%s : 0x%04X(0x%04X) is 0x%08X \n", str.c_str(), 
                             registerAddress.ActualAddress(ch), registerAddress.GetAddress(), returnData);
  return returnData;
}

uint32_t Digitizer::PrintRegister(uint32_t address, std::string msg){
  if( !isConnected ) return 0 ;
  printf("\e[33m----------------------------------------------------\n");
  printf("------------ %s = 0x%X \n", msg.c_str(), address);
  printf("----------------------------------------------------\e[0m\n");

  uint32_t * value = new uint32_t[1];
  CAEN_DGTZ_ReadRegister(handle, address, value);
  printf(" %*s    32  28  24  20  16  12   8   4   0\n", (int) msg.length(), "");
  printf(" %*s     |   |   |   |   |   |   |   |   |\n", (int) msg.length(), "");
  printf(" %*s", (int) msg.length(), "");
  std::cout <<    "  : 0b" << std::bitset<32>(value[0]) << std::endl;
  printf(" %*s  : 0x%X\n", (int) msg.length(), msg.c_str(), value[0]);
  
  return value[0];
}

//========================================== setting file IO
Register::Reg Digitizer::FindRegister(uint32_t address){
  
  Register::Reg tempReg;  
  ///========= Find Match Register
  for( int p = 0; p < (int) RegisterDPPList[p]; p++){
    if( address == RegisterDPPList[p].GetAddress() ) {
      tempReg = RegisterDPPList[p];
      break;
    }
  }
  if( tempReg.GetName() == ""){
    if( DPPType == V1730_DPP_PHA_CODE ){
      for( int p = 0; p < (int) RegisterPHAList[p]; p++){
        if( address == RegisterPHAList[p].GetAddress() ) {
          tempReg = RegisterPHAList[p];
          break;
        }
      }
    }
    if( DPPType == V1730_DPP_PSD_CODE ){
      for( int p = 0; p < (int) RegisterPSDList[p]; p++){
        if( address == RegisterPSDList[p].GetAddress() ) {
          tempReg = RegisterPSDList[p];
          break;
        }
      }
    }
  }
  
  return tempReg;
}

void Digitizer::ReadAllSettingsFromBoard(bool force){
  if( !isConnected ) return;
  if( AcqRun ) return;
  if( isSettingFilledinMemeory && !force) return;

  printf("===== %s \n", __func__);

  /// board setting
  for( int p = 0; p < (int) RegisterDPPList.size(); p++){
    if( RegisterDPPList[p].GetType() == Register::RW::WriteONLY) continue;
    ReadRegister(RegisterDPPList[p]); 
  }
  
  channelMask = GetSettingFromMemory(Register::DPP::ChannelEnableMask);
  
  /// Channels Setting
  for( int ch = 0; ch < NChannel; ch ++){
    if( DPPType == V1730_DPP_PHA_CODE ){
      for( int p = 0; p < (int) RegisterPHAList.size(); p++){
        if( RegisterPHAList[p].GetType() == Register::RW::WriteONLY) continue;
        ReadRegister(RegisterPHAList[p], ch); 
      }
    }
    if( DPPType == V1730_DPP_PSD_CODE ){
      for( int p = 0; p < (int) RegisterPSDList.size(); p++){
        if( RegisterPSDList[p].GetType() == Register::RW::WriteONLY) continue;
        ReadRegister(RegisterPSDList[p], ch); 
      }
    }
  }
  isSettingFilledinMemeory = true;
}

void Digitizer::ProgramSettingsToBoard(){
  if( !isConnected ) return;
  if( isDummy ) return;
  
  Register::Reg haha;
  
  /// board setting
  for( int p = 0; p < (int) RegisterDPPList[p]; p++){
    if( RegisterDPPList[p].GetType() == Register::RW::ReadONLY) continue;
    haha = RegisterDPPList[p];
    WriteRegister(haha, GetSettingFromMemory(haha), -1, false); 
    usleep(100 * 1000);
  }
  /// Channels Setting
  for( int ch = 0; ch < NChannel; ch ++){
    if( DPPType == V1730_DPP_PHA_CODE ){
      for( int p = 0; p < (int) RegisterPHAList[p]; p++){
        if( RegisterPHAList[p].GetType() == Register::RW::ReadONLY) continue;
        haha = RegisterPHAList[p];
        WriteRegister(haha, GetSettingFromMemory(haha, ch), ch, false); 
        usleep(100 * 1000);
      }
    }
    if( DPPType == V1730_DPP_PSD_CODE ){
      for( int p = 0; p < (int) RegisterPSDList[p]; p++){
        if( RegisterPSDList[p].GetType() == Register::RW::ReadONLY) continue;
        haha = RegisterPHAList[p];
        WriteRegister(haha, GetSettingFromMemory(haha, ch), ch, false); 
        usleep(100 * 1000);
      }
    }
  } 
}

void Digitizer::SetSettingToMemory(Register::Reg registerAddress,  unsigned int value, unsigned short ch ){
  unsigned short index = registerAddress.Index(ch);
  if( index > SETTINGSIZE ) return;
  setting[index] = value;
}

unsigned int Digitizer::GetSettingFromMemory(Register::Reg registerAddress, unsigned short ch ){
  unsigned short index = registerAddress.Index(ch);
  if( index > SETTINGSIZE ) return 0xFFFF;
  return setting[index] ;
}

void Digitizer::PrintSettingFromMemory(){
  for( int i = 0; i < SETTINGSIZE; i++) printf("%4d | 0x%04X |0x%08X = %u \n", i, i*4, setting[i], setting[i]);
}

void Digitizer::SetSettingBinaryPath(std::string fileName){
  
  settingFile = fopen(fileName.c_str(), "r+");
  if( settingFile == NULL ){
    printf("cannot open file %s. Create one.\n", fileName.c_str());
    
    ReadAllSettingsFromBoard();  
    SaveAllSettingsAsBin(fileName);
    
    this->settingFileName = fileName;
    settingFileExist = true;
  
  }else{
    this->settingFileName = fileName;
    settingFileExist = true;
    fclose(settingFile);
    printf("setting file already exist. do nothing. Should program the digitizer\n");
  }
  
}

int Digitizer::LoadSettingBinaryToMemory(std::string fileName){
  
  settingFile = fopen(fileName.c_str(), "r");
  
  if( settingFile == NULL ) {
    printf(" %s does not exist or cannot load.\n", fileName.c_str());
    settingFileExist = false;
    return -1;
    
  }else{
    settingFileExist = true;
    settingFileName = fileName;
    fclose (settingFile);
    
    uint32_t fileDPP = ((ReadSettingFromFile(Register::DPP::AMCFirmwareRevision_R, 0) >> 8) & 0xFF);
    
    /// compare seeting DPP version;
    if( isConnected && DPPType != (int) fileDPP ){
      printf("DPPType in the file is %s(0x%X), but the dgitizer DPPType is %s(0x%X). \n", GetDPPString(fileDPP).c_str(), fileDPP, GetDPPString().c_str(),  DPPType);
      return -1;
    }else{
      /// load binary to memoery
      DPPType = fileDPP;
      printf("DPPType in the file is %s(0x%X). \n", GetDPPString(fileDPP).c_str(), fileDPP);

      settingFile = fopen(fileName.c_str(), "r");
      size_t dummy = fread( setting, SETTINGSIZE * sizeof(unsigned int), 1, settingFile);
      fclose (settingFile);

      if( dummy == 0 ) printf("reach the end of file\n");
      
      uint32_t boardInfo = GetSettingFromMemory(Register::DPP::BoardInfo_R);
      if( (boardInfo & 0xFF) == 0x0E ) ch2ns = 4.0;
      if( (boardInfo & 0xFF) == 0x0B ) ch2ns = 2.0;

      ///Should seperate file<->memory, memory<->board
      ///ProgramSettingsToBoard(); /// do nothing if not connected.
    
      return 0;
    }
  }
}
    
unsigned int Digitizer::ReadSettingFromFile(Register::Reg registerAddress, unsigned short ch){
  if ( !settingFileExist ) return -1;
  
  unsigned short index = registerAddress.Index(ch);
  
  settingFile = fopen (settingFileName.c_str(),"r");
  ///fseek( settingFile, address, SEEK_SET); 
  fseek( settingFile, index * 4, SEEK_SET); 
  ///printf(" at pos %lu Byte = index(%lu)\n", ftell(settingFile), ftell(settingFile)/4);
  unsigned int lala[1];
  size_t dummy = fread( lala, sizeof(unsigned int), 1, settingFile);
  ///printf(" data at pos %lu(%lu) : %X = %d\n", ftell(settingFile) - sizeof(unsigned int), (ftell(settingFile) - sizeof(unsigned int))/4, lala[0], lala[0]);
  fclose (settingFile);

  if( dummy == 0 ) printf("reach the end of file\n");

  return lala[0];
   
}

void Digitizer::SaveSettingToFile(Register::Reg registerAddress, unsigned int value, unsigned short ch){
  if ( !settingFileExist ) return ;
  
  unsigned short index = registerAddress.Index(ch);
  setting[index] = value;
  
  settingFile = fopen (settingFileName.c_str(),"r+");
  ///fseek( settingFile, address, SEEK_SET); 
  fseek( settingFile, index * 4, SEEK_SET); 
  unsigned int jaja[1] = {value};
  fwrite( jaja, sizeof(unsigned int), 1, settingFile);
  ///printf("fwrite ret : %d, 0x%0X, 0x%0X, %d, 0x%X = %d\n", (int)dummy, registerAddress, index*4, index, jaja[0], jaja[0]);
  fclose (settingFile);
}

void Digitizer::SaveAllSettingsAsBin(std::string fileName){
  if( !isSettingFilledinMemeory ) return;
  
  FILE * binFile = fopen(fileName.c_str(), "w+");
  if( binFile == NULL ) {
    printf("Cannot open %s.\n", fileName.c_str());
    return;
  }
  
  fwrite(setting, SETTINGSIZE * sizeof(unsigned int), 1, binFile);
  fseek(binFile, 0L, SEEK_END);
  unsigned int inFileSize = ftell(binFile);
  printf("Created file : %s. file size : %d Byte\n", fileName.c_str(), inFileSize);
  fclose (binFile);
}

void Digitizer::SaveAllSettingsAsText(std::string fileName){
  if( !isSettingFilledinMemeory ) return;
  
  FILE * txtFile = fopen(fileName.c_str(), "w+");  
  if( txtFile == NULL ) {
    printf("Cannot open %s.\n", fileName.c_str());
    return;
  }

  Register::Reg haha;
  
  for( unsigned int i = 0; i < SETTINGSIZE ; i++){
    haha.SetName("");
    uint32_t actualAddress = haha.CalAddress(i);
    
    ///printf("%7d--- 0x%04X,  0x%04X\n", i, haha->GetAddress(), haha->ActualAddress());
    for( int p = 0; p < (int) RegisterDPPList.size(); p++){
      if( haha.GetAddress() == (uint32_t) RegisterDPPList[p] ) haha = RegisterDPPList[p];
    }
    if( DPPType == V1730_DPP_PHA_CODE) {
      for( int p = 0; p < (int) RegisterPHAList.size(); p++){
        if( haha.GetAddress() == (uint32_t) RegisterPHAList[p] ) haha = RegisterPHAList[p];
      }
    }
    if( DPPType == V1730_DPP_PSD_CODE) {
      for( int p = 0; p < (int) RegisterPSDList.size(); p++){
        if( haha.GetAddress() == (uint32_t) RegisterPSDList[p] ) haha = RegisterPSDList[p];
      }
    }
    if( haha.GetName() != "" )  {
      std::string typeStr ;
      if( haha.GetType() == Register::RW::ReadWrite ) typeStr = "R/W";
      if( haha.GetType() == Register::RW::ReadONLY  ) typeStr = "R  ";
      if( haha.GetType() == Register::RW::WriteONLY ) typeStr = "  W";
      fprintf( txtFile, "0x%04X %30s   0x%08X  %s  %u\n", actualAddress, 
                                                          haha.GetNameChar(), 
                                                          setting[i], 
                                                          typeStr.c_str(),
                                                          setting[i]);
    }
  }
}

std::string Digitizer::GetDPPString(int DPPType){
  std::string DPPTypeStr = "";
  if( DPPType == 0 ) DPPType = this->DPPType;
  switch (DPPType){
    case V1724_DPP_PHA_CODE: DPPTypeStr = "DPP-PHA x724"; break; /// 0x80
    case V1720_DPP_CI_CODE : DPPTypeStr = "DPP-CI  x720"; break; /// 0x82
    case V1720_DPP_PSD_CODE: DPPTypeStr = "DPP-PSD x720"; break; /// 0x83
    case V1751_DPP_PSD_CODE: DPPTypeStr = "DPP-PSD x751"; break; /// 0x84
    case V1751_DPP_ZLE_CODE: DPPTypeStr = "DPP-ZLE x751"; break; /// 0x85
    case V1743_DPP_CI_CODE:  DPPTypeStr = "DPP-PSD x743"; break; /// 0x86
    case V1740_DPP_QDC_CODE: DPPTypeStr = "DPP-QDC x740"; break; /// 0x87
    case V1730_DPP_PSD_CODE: DPPTypeStr = "DPP-PSD x730"; break; /// 0x88
    case V1730_DPP_PHA_CODE: DPPTypeStr = "DPP-PHA x730"; break; /// 0x8B
    case V1730_DPP_ZLE_CODE: DPPTypeStr = "DPP-ZLE x730"; break; /// 0x8C
    case V1730_DPP_DAW_CODE: DPPTypeStr = "DPP-DAW x730"; break; /// 0x8D
  }
  return DPPTypeStr;
}

void Digitizer::ErrorMsg(std::string header){
  switch (ret){
    ///case CAEN_DGTZ_Success                 : /**   0 */ printf("%s | Operation completed successfully.\n", header.c_str()); break;
    case CAEN_DGTZ_CommError               : /**  -1 */ printf("%s | Communication Error.\n", header.c_str()); break;
    case CAEN_DGTZ_GenericError            : /**  -2 */ printf("%s | Unspecified error.\n", header.c_str()); break;
    case CAEN_DGTZ_InvalidParam            : /**  -3 */ printf("%s | Invalid parameter.\n", header.c_str()); break;
    case CAEN_DGTZ_InvalidLinkType         : /**  -4 */ printf("%s | Invalid Link Type.\n", header.c_str()); break;
    case CAEN_DGTZ_InvalidHandle           : /**  -5 */ printf("%s | Invalid device handler.\n", header.c_str()); break;
    case CAEN_DGTZ_MaxDevicesError         : /**  -6 */ printf("%s | Maximum number of devices exceeded.\n", header.c_str()); break;
    case CAEN_DGTZ_BadBoardType            : /**  -7 */ printf("%s | Operation not allowed on this type of board.\n", header.c_str()); break;
    case CAEN_DGTZ_BadInterruptLev         : /**  -8 */ printf("%s | The interrupt level is not allowed.\n", header.c_str()); break;
    case CAEN_DGTZ_BadEventNumber          : /**  -9 */ printf("%s | The event number is bad.\n", header.c_str()); break;
    case CAEN_DGTZ_ReadDeviceRegisterFail  : /** -10 */ printf("%s | Unable to read the registry.\n", header.c_str()); break;
    case CAEN_DGTZ_WriteDeviceRegisterFail : /** -11 */ printf("%s | Unable to write the registry.\n", header.c_str()); break;
    case CAEN_DGTZ_InvalidChannelNumber    : /** -13 */ printf("%s | The channel number is invalid.\n", header.c_str()); break;
    case CAEN_DGTZ_ChannelBusy             : /** -14 */ printf("%s | The channel is busy.\n", header.c_str()); break;
    case CAEN_DGTZ_FPIOModeInvalid         : /** -15 */ printf("%s | Invalid FPIO mode.\n", header.c_str()); break;
    case CAEN_DGTZ_WrongAcqMode            : /** -16 */ printf("%s | Wrong Acquistion mode.\n", header.c_str()); break;
    case CAEN_DGTZ_FunctionNotAllowed      : /** -17 */ printf("%s | This function is not allowed on this module.\n", header.c_str()); break;
    case CAEN_DGTZ_Timeout                 : /** -18 */ printf("%s | Communication Timeout.\n", header.c_str()); break;
    case CAEN_DGTZ_InvalidBuffer           : /** -19 */ printf("%s | The buffer is invalid.\n", header.c_str()); break;
    case CAEN_DGTZ_EventNotFound           : /** -20 */ printf("%s | The event is not found.\n", header.c_str()); break;
    case CAEN_DGTZ_InvalidEvent            : /** -21 */ printf("%s | The event is invalid.\n", header.c_str()); break;
    case CAEN_DGTZ_OutOfMemory             : /** -22 */ printf("%s | Out of memory.\n", header.c_str()); break;
    case CAEN_DGTZ_CalibrationError        : /** -23 */ printf("%s | Unable to calibrate the board.\n", header.c_str()); break;
    case CAEN_DGTZ_DigitizerNotFound       : /** -24 */ printf("%s | Unbale to open the digitizer.\n", header.c_str()); break;
    case CAEN_DGTZ_DigitizerAlreadyOpen    : /** -25 */ printf("%s | The digitizer is already open.\n", header.c_str()); break;
    case CAEN_DGTZ_DigitizerNotReady       : /** -26 */ printf("%s | The digitizer is not ready.\n", header.c_str()); break;
    case CAEN_DGTZ_InterruptNotConfigured  : /** -27 */ printf("%s | The digitizer has no IRQ configured.\n", header.c_str()); break;
    case CAEN_DGTZ_DigitizerMemoryCorrupted: /** -28 */ printf("%s | The digitizer flash memory is corrupted.\n", header.c_str()); break;
    case CAEN_DGTZ_DPPFirmwareNotSupported : /** -29 */ printf("%s | The digitier DPP firmware is not supported in this lib version.\n", header.c_str()); break;
    case CAEN_DGTZ_InvalidLicense          : /** -30 */ printf("%s | Invalid firmware licence.\n", header.c_str()); break;
    case CAEN_DGTZ_InvalidDigitizerStatus  : /** -31 */ printf("%s | The digitizer is found in a corrupted status.\n", header.c_str()); break;
    case CAEN_DGTZ_UnsupportedTrace        : /** -32 */ printf("%s | The given trace is not supported.\n", header.c_str()); break;
    case CAEN_DGTZ_InvalidProbe            : /** -33 */ printf("%s | The given probe is not supported.\n", header.c_str()); break;
    case CAEN_DGTZ_UnsupportedBaseAddress  : /** -34 */ printf("%s | The base address is not supported.\n", header.c_str()); break;
    case CAEN_DGTZ_NotYetImplemented       : /** -99 */ printf("%s | The function is not yet implemented.\n", header.c_str()); break;
  }
  
}

//============================== DPP-Alpgorthm Control 
void Digitizer::SetDPPAlgorithmControl(uint32_t bit, int ch){
  WriteRegister( Register::DPP::DPPAlgorithmControl, bit, ch);    
  if( ret != 0 ) ErrorMsg(__func__);
}

unsigned int Digitizer::ReadBits(Register::Reg address, unsigned int bitLength, unsigned int bitSmallestPos, int ch ){
  int tempCh = ch;
  if (ch < 0 && address < 0x8000 ) tempCh = 0; /// take ch-0 
  uint32_t bit = ReadRegister(address, tempCh);
  bit = (bit >> bitSmallestPos ) & uint(pow(2, bitLength)-1);
  return bit;
}

void Digitizer::SetBits(Register::Reg address, unsigned int bitValue, unsigned int bitLength, unsigned int bitSmallestPos, int ch){
  ///printf("address : 0x%X, value : 0x%X, len : %d, pos : %d, ch : %d \n", address, bitValue, bitLength, bitSmallestPos, ch);
  uint32_t bit ;
  uint32_t bitmask = (uint(pow(2, bitLength)-1) << bitSmallestPos);  
  int tempCh = ch;
  if (ch < 0 && address < 0x8000 ) tempCh = 0; /// take ch-0 
  bit = ReadRegister(address, tempCh);
  ///printf("bit : 0x%X, bitmask : 0x%X \n", bit, bitmask);
  bit = (bit & ~bitmask) | (bitValue << bitSmallestPos);
  ///printf("bit : 0x%X, ch : %d \n", bit, ch);
  WriteRegister(address, bit, ch);
  if( ret != 0 ) ErrorMsg(__func__);  
}

