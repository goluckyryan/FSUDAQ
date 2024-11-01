#include "ClassDigitizer.h"

Digitizer::Digitizer(){
  DebugPrint("%s", "Digitizer");
  Initalization();
}

Digitizer::Digitizer(int boardID, int portID, bool program,  bool verbose){
  DebugPrint("%s", "Digitizer");
  Initalization();
  OpenDigitizer(boardID, portID, program, verbose);
}

Digitizer::~Digitizer(){
  DebugPrint("%s", "Digitizer");
  CloseDigitizer();
  delete data;
}

void Digitizer::Initalization(){
  DebugPrint("%s", "Digitizer");

  data = nullptr; 

  portID = -1;
  boardID = -1;
  handle = -1;
  NumInputCh = 16;
  NumRegChannel = 16;
  isInputChEqRegCh = true;
  NCoupledCh = 8;
  ADCbits  = 1;
  DPPType = DPPTypeCode::DPP_PHA_CODE;
  ModelType = ModelTypeCode::VME;
  ADCFullSize = 0;
  tick2ns = 0; 
  BoardInfo = {};
  MemorySizekSample = 0;

  regChannelMask = 0xFFFF;
  VMEBaseAddress = 0;
  LinkType = CAEN_DGTZ_USB;      /// default USB
  IOlev = CAEN_DGTZ_IOLevel_NIM; ///default NIM  
  
  isSettingFilledinMemeory = false;
  settingFileName = "";
  isSettingFileExist = false;
  isSettingFileUpdate = false;
  settingFile = NULL;
  
  ret = -1;
  isConnected = false; 
  AcqRun = false;
  isDummy = true;

}

void Digitizer::Reset(){
  DebugPrint("%s", "Digitizer");
  // ret = CAEN_DGTZ_Reset(handle);
  // if( ret != 0 ) ErrorMsg(__func__);
  
  // ret |= CAEN_DGTZ_WriteRegister(handle, DPP::SoftwareClear_W, 1);
  // if( ret != 0 ) ErrorMsg("Reset-SoftwareClear_W");

  // Clear data off the Output Buffer, the event counter, perform a FPGA global reset to default configuration
  ret = CAEN_DGTZ_WriteRegister(handle, DPP::SoftwareReset_W, 1);
  if( ret != 0 ) ErrorMsg("Reset-SoftwareReset_W");

}

void Digitizer::PrintBoard (){
  DebugPrint("%s", "Digitizer");
  printf("Connected to Model %s with handle %d using %s\n", BoardInfo.ModelName, handle, LinkType == CAEN_DGTZ_USB ? "USB" : "Optical Link");
  printf("          Family Name : %s \n", familyName.c_str());
  printf("        Sampling rate : %.1f MHz = %.1f ns \n", 1000./tick2ns, tick2ns);
  printf("No. of Input Channels : %d \n", NumInputCh);
  printf("  No. of Reg Channels : %d, mask : 0x%X\n", NumRegChannel, regChannelMask);
  printf("         SerialNumber :\e[1m\e[33m %d\e[0m\n", BoardInfo.SerialNumber);
  printf("              DPPType : %d (%s)\n", DPPType, GetDPPString().c_str());
  printf("              ADC bit : \e[33m%d\e[0m, %d = 0x%X\n", ADCbits, ADCFullSize, ADCFullSize);
  printf("     ROC FPGA Release : %s\n", BoardInfo.ROC_FirmwareRel);
  printf("     AMC FPGA Release : %s\n", BoardInfo.AMC_FirmwareRel);
  printf(" Channle Memeory Size : %u kSample\n", MemorySizekSample);
}

int Digitizer::OpenDigitizer(int boardID, int portID, bool program, bool verbose){
  DebugPrint("%s", "Digitizer");
  this->boardID = boardID;
  this->portID = portID;
  
  if( boardID < 0 || portID < 0 ) return 0; /// for using the Digitizer Class without open digitizer
    
  /***************************************************/
  /** Open the digitizer and read board information  */
  /***************************************************/

  if( verbose) printf("============= Opening Digitizer at Board %d, Port %d \n", boardID, portID);
  
  ///-------- try USB first
  if( portID < 4){
    LinkType = CAEN_DGTZ_USB;     /// Link Type
    ret = (int) CAEN_DGTZ_OpenDigitizer(LinkType, boardID, 0, VMEBaseAddress, &handle);
    if (ret != 0){ ///---------- try Optical link
      LinkType = CAEN_DGTZ_OpticalLink ; 
      ret = (int) CAEN_DGTZ_OpenDigitizer(LinkType, portID, boardID, VMEBaseAddress, &handle);
    }
    ErrorMsg("=== Open Digitizer port " +std::to_string(portID) + " board " + std::to_string(boardID));
  }else{
    LinkType = CAEN_DGTZ_USB_A4818 ; // portID = A4818 PID
    ret = (int) CAEN_DGTZ_OpenDigitizer(LinkType, portID, boardID, VMEBaseAddress, &handle);
    ErrorMsg("=== Open Digitizer using A4818 (PID:" +std::to_string(portID) + ") board " + std::to_string(boardID) + ".");
  }

  if( ret == 0 ){
    if( LinkType == CAEN_DGTZ_USB )         printf("### Open digitizer via USB, board : %d\n", boardID);
    if( LinkType == CAEN_DGTZ_OpticalLink ) printf("### Open digitizer via Optical Link, port : %d, board : %d\n", portID, boardID);
    if( LinkType == CAEN_DGTZ_USB_A4818 )   printf("### Open digitizer via A4818, port : %d, board : %d\n", portID, boardID);
  }
  
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
      NumRegChannel = BoardInfo.Channels;
      NumInputCh = NumRegChannel;
      isInputChEqRegCh = true;
      regChannelMask = pow(2, NumInputCh)-1;
      switch(BoardInfo.Model){
        case CAEN_DGTZ_DT5730: tick2ns =  2.0; NCoupledCh = NumInputCh/2; ModelType = ModelTypeCode::DT; break; ///ns -> 500 MSamples/s
        case CAEN_DGTZ_DT5725: tick2ns =  4.0; NCoupledCh = NumInputCh/2; ModelType = ModelTypeCode::DT; break; ///ns -> 250 MSamples/s
        case CAEN_DGTZ_V1730:  tick2ns =  2.0; NCoupledCh = NumInputCh/2; ModelType = ModelTypeCode::VME; break; ///ns -> 500 MSamples/s
        case CAEN_DGTZ_V1725:  tick2ns =  4.0; NCoupledCh = NumInputCh/2; ModelType = ModelTypeCode::VME; break; ///ns -> 250 MSamples/s
        case CAEN_DGTZ_V1740: {
          NumInputCh = 64;
          NCoupledCh = NumRegChannel;
          isInputChEqRegCh = false;
          ModelType = ModelTypeCode::VME;
          tick2ns = 16.0; break; ///ns -> 62.5 MSamples/s
        }
        default : tick2ns = 4.0; break;
      }

      switch(BoardInfo.FamilyCode){
        case CAEN_DGTZ_XX740_FAMILY_CODE: familyName = "740 family"; break;
        case CAEN_DGTZ_XX730_FAMILY_CODE: familyName = "730 family"; break;
        case CAEN_DGTZ_XX725_FAMILY_CODE: familyName = "725 family"; break;
        default: familyName = "not supported"; break;
      }

      data = new Data(NumInputCh);
      data->tick2ns = tick2ns;
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
    default :   data->DPPTypeStr = "STD"; break; // stardard
  }
  /// change address 0xEF08 (5 bits), this will reflected in the 2nd word of the Board Agg. header.
  ret = CAEN_DGTZ_WriteRegister(handle, DPP::BoardID, (DPPType & 0xF));

  //TODO somehow the bdInfo does not work, use DPPType to set it
  uint32_t bdInfo = GetSettingFromMemory(DPP::BoardInfo_R);
  uint32_t haha = ((bdInfo >> 8 ) & 0xFF);
  // printf("------- 0x%08X = %u \n", bdInfo, haha);
  switch(haha) {
    case 0x01 : MemorySizekSample =  640; break;
    case 0x02 : MemorySizekSample =  192; break;
    case 0x08 : MemorySizekSample = 5242; break;
    case 0x10 : MemorySizekSample = 1536; break;
    default: MemorySizekSample =  192; break;
  }

  if ( verbose ){
    PrintBoard();    
    if (DPPType < 0x80 ) {
      printf("This digitizer does not have DPP-PHA firmware\n");
    }else {
      printf("\t==== This digitizer has a DPP firmware!");    
      printf("\e[32m\t %s \e[0m\n", GetDPPString().c_str());
    }
  }
  ErrorMsg("========== Set BoardID");
  
  ///======================= Check virtual probe
  if( DPPType != DPPTypeCode::DPP_QDC_CODE ){
    int probes[MAX_SUPPORTED_PROBES];
    int numProbes;
    ret = CAEN_DGTZ_GetDPP_SupportedVirtualProbes(handle, 1, probes, &numProbes);
    ErrorMsg("=== Get Supported Virtual Probes");
    if( verbose ){
      printf("\t==== supported virtual probe (number of Probe : %d)\n", numProbes);
      for( int i = 0 ; i < numProbes; i++){
        printf("\t\t %8d ", probes[i]);
        switch (probes[i]){
          case  0: printf("CAEN_DGTZ_DPP_VIRTUALPROBE_Input\n"            ); break;
          case  1: printf("CAEN_DGTZ_DPP_VIRTUALPROBE_Delta\n"            ); break;
          case  2: printf("CAEN_DGTZ_DPP_VIRTUALPROBE_Delta2\n"           ); break;
          case  3: printf("CAEN_DGTZ_DPP_VIRTUALPROBE_Trapezoid\n"        ); break;
          case  4: printf("CAEN_DGTZ_DPP_VIRTUALPROBE_TrapezoidReduced\n" ); break;
          case  5: printf("CAEN_DGTZ_DPP_VIRTUALPROBE_Baseline\n"         ); break;
          case  6: printf("CAEN_DGTZ_DPP_VIRTUALPROBE_Threshold\n"        ); break;
          case  7: printf("CAEN_DGTZ_DPP_VIRTUALPROBE_CFD\n"              ); break;
          case  8: printf("CAEN_DGTZ_DPP_VIRTUALPROBE_SmoothedInput\n"    ); break;
          case  9: printf("CAEN_DGTZ_DPP_VIRTUALPROBE_None\n"             ); break;
          case 10: printf("CAEN_DGTZ_DPP_DIGITALPROBE_TRGWin\n"           ); break;
          case 11: printf("CAEN_DGTZ_DPP_DIGITALPROBE_Armed\n"            ); break;
          case 12: printf("CAEN_DGTZ_DPP_DIGITALPROBE_PkRun\n"            ); break;
          case 13: printf("CAEN_DGTZ_DPP_DIGITALPROBE_Peaking\n"          ); break;
          case 14: printf("CAEN_DGTZ_DPP_DIGITALPROBE_CoincWin\n"         ); break;
          case 15: printf("CAEN_DGTZ_DPP_DIGITALPROBE_BLHoldoff\n"        ); break;
          case 16: printf("CAEN_DGTZ_DPP_DIGITALPROBE_TRGHoldoff\n"       ); break;
          case 17: printf("CAEN_DGTZ_DPP_DIGITALPROBE_TRGVal\n"           ); break;
          case 18: printf("CAEN_DGTZ_DPP_DIGITALPROBE_ACQVeto\n"          ); break;
          case 19: printf("CAEN_DGTZ_DPP_DIGITALPROBE_BFMVeto\n"          ); break;
          case 20: printf("CAEN_DGTZ_DPP_DIGITALPROBE_ExtTRG\n"           ); break;
          case 21: printf("CAEN_DGTZ_DPP_DIGITALPROBE_OverThr\n"          ); break;
          case 22: printf("CAEN_DGTZ_DPP_DIGITALPROBE_TRGOut\n"           ); break;
          case 23: printf("CAEN_DGTZ_DPP_DIGITALPROBE_Coincidence \n"     ); break;
          case 24: printf("CAEN_DGTZ_DPP_DIGITALPROBE_PileUp \n"          ); break;
          case 25: printf("CAEN_DGTZ_DPP_DIGITALPROBE_Gate \n"            );  break;
          case 26: printf("CAEN_DGTZ_DPP_DIGITALPROBE_GateShort \n"       ); break;
          case 27: printf("CAEN_DGTZ_DPP_DIGITALPROBE_Trigger \n"         ); break;
          case 28: printf("CAEN_DGTZ_DPP_DIGITALPROBE_None  \n"           ); break;
          case 29: printf("CAEN_DGTZ_DPP_DIGITALPROBE_BLFreeze  \n"       ); break;
          case 30: printf("CAEN_DGTZ_DPP_DIGITALPROBE_Busy  \n"           ); break;
          case 31: printf("CAEN_DGTZ_DPP_DIGITALPROBE_PrgVeto \n"         ); break;
          default : printf("Unknown probe\n"); break; 
        }  
      }
    }
  }  
  ErrorMsg("end of OpenDigitizer");

  softwareDisable = false;
  AcqRun = false;
  
  if( isConnected ) isDummy = false;

  if( isConnected  && program) {
    if( DPPType == DPPTypeCode::DPP_PHA_CODE ) ProgramBoard_PHA();
    if( DPPType == DPPTypeCode::DPP_PSD_CODE ) ProgramBoard_PSD();
    if( DPPType == DPPTypeCode::DPP_QDC_CODE ) ProgramBoard_QDC();
  }
  
  //if( isConnected ) ReadAllSettingsFromBoard(); 

  return ret;
}

int Digitizer::CloseDigitizer(){
  DebugPrint("%s", "Digitizer");
  if( !isConnected ) return 0;
  isConnected = false;
  ret  = CAEN_DGTZ_SWStopAcquisition(handle);
  printf("-------- Closing Digtizer Board : %d Port : %d \n", boardID, portID);
  if( LinkType == CAEN_DGTZ_USB ) printf("  Model %s with handle %d using USB\n", BoardInfo.ModelName, handle);
  if( LinkType == CAEN_DGTZ_OpticalLink ) printf("  Model %s with handle %d using Optical Fiber\n", BoardInfo.ModelName, handle);
  if( LinkType == CAEN_DGTZ_USB_A4818 ) printf("  Model %s with handle %d using A4818\n", BoardInfo.ModelName, handle);
  ret |= CAEN_DGTZ_CloseDigitizer(handle);
  
  return ret;
}

void Digitizer::SetRegChannelMask(uint32_t mask){
  DebugPrint("%s", "Digitizer");
  if( softwareDisable ) return;
  if( AcqRun ) return;
  if( !isConnected ) return;
  regChannelMask = mask;
  ret |= CAEN_DGTZ_SetChannelEnableMask(handle, regChannelMask);
  SetSettingToMemory(DPP::RegChannelEnableMask, mask);
  SaveSettingToFile(DPP::RegChannelEnableMask, mask);
  ErrorMsg(__func__);
}

bool Digitizer::GetInputChannelOnOff(unsigned ch) {
  // DebugPrint("%s", "Digitizer");
  if( softwareDisable ) return false;
  // regChannelMask = GetSettingFromMemory(DPP::RegChannelEnableMask);
  if( isInputChEqRegCh ) return (regChannelMask & ( 1 << ch) );

  int grpID = ch/8; //may change for not grouped in 8;
  return (regChannelMask & ( 1 << grpID) );
} 

void Digitizer::SetRegChannelOnOff(unsigned short ch, bool onOff){
  DebugPrint("%s", "Digitizer");
  if( softwareDisable ) return;
  if( AcqRun ) return;
  if( !isConnected ) return;
  regChannelMask = ((regChannelMask & ~( 1 << ch) ) | ( onOff << ch)) ;
  SetRegChannelMask(regChannelMask);
}

void Digitizer::ProgramBoard(){
  DebugPrint("%s", "Digitizer");
  if( softwareDisable ) return;
  if( AcqRun ) return;
  if( DPPType == DPPTypeCode::DPP_PHA_CODE ) ProgramBoard_PHA();
  if( DPPType == DPPTypeCode::DPP_PSD_CODE ) ProgramBoard_PSD();
  if( DPPType == DPPTypeCode::DPP_QDC_CODE ) ProgramBoard_QDC();
}

void Digitizer::ProgramChannel(short chOrGroup){
  if( softwareDisable ) return;
  if( AcqRun ) return;
  if( DPPType == DPPTypeCode::DPP_PHA_CODE ) ProgramChannel_PHA(chOrGroup);
  if( DPPType == DPPTypeCode::DPP_PSD_CODE ) ProgramChannel_PSD(chOrGroup);
  if( DPPType == DPPTypeCode::DPP_QDC_CODE ) ProgramChannel_QDC(chOrGroup);
}

int Digitizer::ProgramBoard_PHA(){
  DebugPrint("%s", "Digitizer");
  printf("===== Digitizer::%s\n", __func__);

  Reset();

  //*========================== Board
  /// change address 0xEF08 (5 bits), this will reflected in the 2nd word of the Board Agg. header.
  ret = CAEN_DGTZ_WriteRegister(handle, DPP::BoardID, (DPPType & 0xF));
  //WriteRegister(DPP::BoardID, (DPPType & 0xF));

  //ret = CAEN_DGTZ_WriteRegister(handle, DPP::BoardConfiguration, 0x0F8915);  /// has Extra2, dual trace, input and trap-baseline
  ret |= CAEN_DGTZ_WriteRegister(handle, DPP::BoardConfiguration, 0x0E8915);  /// has Extra2, no trace
  //ret = CAEN_DGTZ_WriteRegister(handle, DPP::BoardConfiguration, 0x0D8115);  /// diable Extra2
  
  //TODO change to write register
  ret = CAEN_DGTZ_SetAcquisitionMode(handle, CAEN_DGTZ_SW_CONTROLLED); /// software command
  ret |= CAEN_DGTZ_SetChannelEnableMask(handle, ModelType == ModelTypeCode::VME ? 0xFFFF : 0x00FF);
  ret |= CAEN_DGTZ_SetRunSynchronizationMode(handle, CAEN_DGTZ_RUN_SYNC_Disabled);
  ret |= CAEN_DGTZ_SetIOLevel(handle, CAEN_DGTZ_IOLevel_NIM);
  ret |= CAEN_DGTZ_SetExtTriggerInputMode(handle, CAEN_DGTZ_TRGMODE_ACQ_ONLY);
  ret |= CAEN_DGTZ_WriteRegister(handle, (int32_t)(DPP::GlobalTriggerMask), 0x0);
  ret |= CAEN_DGTZ_WriteRegister(handle, (int32_t)(DPP::FrontPanelTRGOUTEnableMask), 0x0);

  //ret = CAEN_DGTZ_SetNumEventsPerAggregate(handle, 0);
  
  if( ret != 0 ) { printf("==== set board error.\n"); return 0;}
  
  //*========================== Group
  ProgramChannel_PHA(-1);
  
  isSettingFilledinMemeory = false; /// unlock the ReadAllSettingsFromBoard();
  usleep(1000*300);
  ReadAllSettingsFromBoard();
  
  return ret;
}

int Digitizer::ProgramChannel_PHA(short ch){
  DebugPrint("%s", "Digitizer");
  printf("===== Digitizer::%s|ch:%d\n", __func__,ch);

  uint32_t channel = (ch << 8);
  if( ch < 0 ) channel = 0x7000;

  uint32_t address = (ch << 8);
  
  address = channel + DPP::RecordLength_G;               ret  = CAEN_DGTZ_WriteRegister(handle, address, 62);  
  address = channel + DPP::PHA::DecayTime;               ret |= CAEN_DGTZ_WriteRegister(handle, address, 5000 ); 
  address = channel + DPP::PHA::TrapezoidFlatTop;        ret |= CAEN_DGTZ_WriteRegister(handle, address, 0x1A ); 
  address = channel + DPP::PHA::TrapezoidRiseTime;       ret |= CAEN_DGTZ_WriteRegister(handle, address, 6 ); 
  address = channel + DPP::PHA::PeakingTime;             ret |= CAEN_DGTZ_WriteRegister(handle, address, 6 ); 
  address = channel + DPP::PHA::RCCR2SmoothingFactor;    ret |= CAEN_DGTZ_WriteRegister(handle, address, 4 ); 
  address = channel + DPP::PHA::InputRiseTime;           ret |= CAEN_DGTZ_WriteRegister(handle, address, 6 ); 
  address = channel + DPP::PHA::TriggerThreshold;        ret |= CAEN_DGTZ_WriteRegister(handle, address, 1000 );
  address = channel + DPP::PHA::PeakHoldOff;             ret |= CAEN_DGTZ_WriteRegister(handle, address, 0x3E );
  address = channel + DPP::PHA::TriggerHoldOffWidth;     ret |= CAEN_DGTZ_WriteRegister(handle, address, 0x3E );
  address = channel + DPP::PHA::RiseTimeValidationWindow;ret |= CAEN_DGTZ_WriteRegister(handle, address, 0x0 );
  address = channel + DPP::PreTrigger;                   ret |= CAEN_DGTZ_WriteRegister(handle, address, 32 );
  address = channel + DPP::InputDynamicRange;            ret |= CAEN_DGTZ_WriteRegister(handle, address, 0x0 );
  address = channel + DPP::DPPAlgorithmControl;          ret |= CAEN_DGTZ_WriteRegister(handle, address, 0x030200f);
  address = channel + DPP::PHA::DPPAlgorithmControl2_G;  ret |= CAEN_DGTZ_WriteRegister(handle, address, 0x200); // use fine time
  
  if( ch >= 0 ) {
    ret |= CAEN_DGTZ_SetChannelDCOffset(handle, ch, 0xAAAA);
  }else{
    for( int i = 0; i < NumRegChannel; i ++ ){
      ret |= CAEN_DGTZ_SetChannelDCOffset(handle, i, 0xAAAA);
    }
  }

  if( ret != 0 ) { printf("!!!!!!!! set channels error.\n");}

  AutoSetDPPEventAggregation();

  if( ch >= 0 ){
    isSettingFilledinMemeory = false;
    usleep(1000*300);
    ReadAllSettingsFromBoard();
  }

  return ret;
}

int Digitizer::ProgramBoard_PSD(){
  DebugPrint("%s", "Digitizer");
  printf("===== Digitizer::%s\n", __func__);

  //ret = CAEN_DGTZ_Reset(handle);
  Reset();

  //*========================== Board
  /// change address 0xEF08 (5 bits), this will reflected in the 2nd word of the Board Agg. header.
  ret = CAEN_DGTZ_WriteRegister(handle, DPP::BoardID, (DPPType & 0xF));
  //WriteRegister(DPP::BoardID, (DPPType & 0xF));

  //ret = CAEN_DGTZ_WriteRegister(handle, DPP::BoardConfiguration, 0x0F0115);  /// has Extra2, dual trace, input and CFD
  ret |= CAEN_DGTZ_WriteRegister(handle, DPP::BoardConfiguration, 0x0E0115);  /// has Extra2, no trace
  ret |= CAEN_DGTZ_SetAcquisitionMode(handle, CAEN_DGTZ_SW_CONTROLLED); /// software command
  ret |= CAEN_DGTZ_SetIOLevel(handle, CAEN_DGTZ_IOLevel_NIM);
  ret |= CAEN_DGTZ_SetExtTriggerInputMode(handle, CAEN_DGTZ_TRGMODE_ACQ_ONLY);
  ret |= CAEN_DGTZ_WriteRegister(handle, (int32_t)(DPP::GlobalTriggerMask), 0x0);
  ret |= CAEN_DGTZ_WriteRegister(handle, (int32_t)(DPP::FrontPanelTRGOUTEnableMask), 0x0);
 
  ret |= CAEN_DGTZ_SetChannelEnableMask(handle, 0xFFFF);

  //*========================== Group
  ProgramChannel_PSD(-1);

  isSettingFilledinMemeory = false; /// unlock the ReadAllSettingsFromBoard();
  usleep(1000*300);
  ReadAllSettingsFromBoard();

  return ret;
}

int Digitizer::ProgramChannel_PSD(short ch){
  DebugPrint("%s", "Digitizer");
  printf("===== Digitizer::%s|ch:%d\n", __func__,ch);

  uint32_t channel = (ch << 8);
  if( ch <  0 ) channel = 0x7000;
  uint32_t address = (ch << 8);

  address = channel + DPP::PSD::DPPAlgorithmControl2_G; ret  = CAEN_DGTZ_WriteRegister(handle, address, 0x00000200 ); // use fine time
  address = channel + DPP::DPPAlgorithmControl;         ret |= CAEN_DGTZ_WriteRegister(handle, address, 0x00100003 ); // baseline 16 sample, 320fC
  address = channel + DPP::PSD::TriggerThreshold;       ret |= CAEN_DGTZ_WriteRegister(handle, address, 100 );
  address = channel + DPP::PreTrigger;                  ret |= CAEN_DGTZ_WriteRegister(handle, address, 20 );
  address = channel + DPP::RecordLength_G;              ret |= CAEN_DGTZ_WriteRegister(handle, address, 80 );
  address = channel + DPP::PSD::ShortGateWidth;         ret |= CAEN_DGTZ_WriteRegister(handle, address, 32 );
  address = channel + DPP::PSD::LongGateWidth;          ret |= CAEN_DGTZ_WriteRegister(handle, address, 64 );
  address = channel + DPP::PSD::GateOffset;             ret |= CAEN_DGTZ_WriteRegister(handle, address, 19 );
 
  if( ch >= 0 ) {
    ret |= CAEN_DGTZ_SetChannelDCOffset(handle, ch, 0xAAAA);
  }else{
    for( int i = 0; i < NumRegChannel; i ++ ){
      ret |= CAEN_DGTZ_SetChannelDCOffset(handle, i, 0xAAAA);
    }
  }

  if( ret != 0 ) { printf("!!!!!!!! set channels error.\n");}

  AutoSetDPPEventAggregation();

  if( ch >= 0 ){
    isSettingFilledinMemeory = false;
    usleep(1000*300);
    ReadAllSettingsFromBoard();
  }

  return ret;
}

int Digitizer::ProgramBoard_QDC(){
  DebugPrint("%s", "Digitizer");
  printf("===== Digitizer::%s\n", __func__);
  Reset();

  int ret = 0;

  //*========================== Board
  /// change address 0xEF08 (5 bits), this will reflected in the 2nd word of the Board Agg. header.
  ret = CAEN_DGTZ_WriteRegister(handle, DPP::BoardID, (DPPType & 0xF));
  //WriteRegister(DPP::BoardID, (DPPType & 0xF));

  //WriteRegister(DPP::QDC::NumberEventsPerAggregate, 0x10, -1);
  WriteRegister(DPP::QDC::RecordLength_W, 16, -1); // 128 sample = 2048 ns

  WriteRegister(DPP::BoardConfiguration, 0xE0110);
  //WriteRegister(DPP::AggregateOrganization, 0x0);
  //WriteRegister(DPP::MaxAggregatePerBlockTransfer, 100);
  WriteRegister(DPP::AcquisitionControl, 0x0);
  WriteRegister(DPP::GlobalTriggerMask, 0x0);
  WriteRegister(DPP::FrontPanelTRGOUTEnableMask, 0x0);
  WriteRegister(DPP::FrontPanelIOControl, 0x0);
  WriteRegister(DPP::QDC::GroupEnableMask, 0xFF);

  //*========================== Group
  ProgramChannel_QDC(-1);

  isSettingFilledinMemeory = false; /// unlock the ReadAllSettingsFromBoard();
  usleep(1000*300);
  ReadAllSettingsFromBoard();

  return ret;

}

int Digitizer::ProgramChannel_QDC(short group){

  printf("===== Digitizer::%s|ch:%d\n", __func__,group);

  WriteRegister(DPP::QDC::PreTrigger,  60, group); // at 60 sample = 960 ns
  WriteRegister(DPP::QDC::GateWidth, 100/16, group);
  WriteRegister(DPP::QDC::GateOffset, 0, group);
  WriteRegister(DPP::QDC::FixedBaseline, 0, group);
  
  //WriteRegister(DPP::QDC::DPPAlgorithmControl, 0x300112); // with test pulse, positive
  //WriteRegister(DPP::QDC::DPPAlgorithmControl, 0x300102); // No test pulse, positive 
  WriteRegister(DPP::QDC::DPPAlgorithmControl, 0x310102); // No test pulse, negative 
  
  WriteRegister(DPP::QDC::TriggerHoldOffWidth, 100/16, group);
  WriteRegister(DPP::QDC::TRGOUTWidth, 100/16, group);
  //WriteRegister(DPP::QDC::OverThresholdWidth, 100/16, group);
  WriteRegister(DPP::QDC::SubChannelMask, 0xFF, group);

  WriteRegister(DPP::QDC::DCOffset, 0xAAAA, group);

  WriteRegister(DPP::QDC::TriggerThreshold_sub0, 100, group);
  WriteRegister(DPP::QDC::TriggerThreshold_sub1, 100, group);
  WriteRegister(DPP::QDC::TriggerThreshold_sub2, 100, group);
  WriteRegister(DPP::QDC::TriggerThreshold_sub3, 100, group);
  WriteRegister(DPP::QDC::TriggerThreshold_sub4, 100, group);
  WriteRegister(DPP::QDC::TriggerThreshold_sub5, 100, group);
  WriteRegister(DPP::QDC::TriggerThreshold_sub6, 100, group);
  WriteRegister(DPP::QDC::TriggerThreshold_sub7, 100, group);

  AutoSetDPPEventAggregation();

  if( group >= 0 ){
    isSettingFilledinMemeory = false;
    usleep(1000*300);
    ReadAllSettingsFromBoard();
  }

  return ret;
}

//========================================================= ACQ control
void Digitizer::StartACQ(){
  DebugPrint("%s", "Digitizer");
  if( softwareDisable ) return;
  if ( AcqRun ) return;

  // ret |= CAEN_DGTZ_SetDPPEventAggregation(handle, 0, 0); // Auto set

  unsigned int bufferSize = CalByteForBufferCAEN();
  // unsigned int bufferSize = 200 * 1024 * 1024;
  // if( DPPType == DPPTypeCode::DPP_QDC_CODE ) bufferSize = 500 * 1024 * 1024;
  // if( bufferSize  > 160 * 1024 * 1024 ) printf("============= buffer size bigger than 160 MB (%u)\n", bufferSize );

  data->AllocateMemory(bufferSize);

  unsigned int acqID = ExtractBits(GetSettingFromMemory(DPP::AcquisitionControl), DPP::Bit_AcquistionControl::StartStopMode);
  unsigned int trgOutID = ExtractBits(GetSettingFromMemory(DPP::FrontPanelIOControl), DPP::Bit_FrontPanelIOControl::TRGOUTConfig);

  std::string acqStr = "";
  for( int i = 0; i < (int) DPP::Bit_AcquistionControl::ListStartStopMode.size(); i++){
    if( DPP::Bit_AcquistionControl::ListStartStopMode[i].second == acqID  ){
      acqStr = DPP::Bit_AcquistionControl::ListStartStopMode[i].first;
    }
  }
  std::string trgOutStr = "";
  for( int i = 0; i < (int) DPP::Bit_FrontPanelIOControl::ListTRGOUTConfig.size(); i++){
    if( DPP::Bit_FrontPanelIOControl::ListTRGOUTConfig[i].second == (trgOutID << 14)  ){
      trgOutStr = DPP::Bit_FrontPanelIOControl::ListTRGOUTConfig[i].first;
    }
  }

  if( DPPType == DPPTypeCode::DPP_PHA_CODE ) {

    printf(" Setting Trapzoid Scaling Factor and Fine Gain \n");
    for( int ch = 0; ch < NumRegChannel; ch++){
      unsigned int riseTime = GetSettingFromMemory(DPP::PHA::TrapezoidRiseTime, ch);
      unsigned int decayTime = GetSettingFromMemory(DPP::PHA::DecayTime, ch); 
      unsigned int SHF = std::floor(std::log2(riseTime * decayTime));
      SetBits(DPP::DPPAlgorithmControl, DPP::Bit_DPPAlgorithmControl_PHA::TrapRescaling, SHF, ch);

      //Always fix the fineGate = fg = 1
      unsigned int f = 0xFFFF * pow(2, SHF)/ riseTime / decayTime;
      WriteRegister(DPP::PHA::FineGain, f, ch);

    }

  }

  data->ClearTriggerRate();
  data->ClearData();
  if( DPPType == DPPTypeCode::DPP_QDC_CODE ) SetQDCOptimialAggOrg();

  printf("    ACQ mode : %s (%d), TRG-OUT mode : %s (%d) \n", acqStr.c_str(), acqID, trgOutStr.c_str(), trgOutID);

  AcqRun = true;
  usleep(1000); // wait for 1 msec to start/Arm ACQ;

  ret = CAEN_DGTZ_SWStartAcquisition(handle);
  if( ret != 0 ) {
    ErrorMsg("Start ACQ");
    return;
  }
  
  printf("\e[1m\e[33m======= Acquisition Started for %d | Board %d, Port %d\e[0m\n", BoardInfo.SerialNumber, boardID, portID);


}

void Digitizer::StopACQ(){
  DebugPrint("%s", "Digitizer");
  if( !AcqRun ) return;
  int ret = CAEN_DGTZ_SWStopAcquisition(handle);
  ret |= CAEN_DGTZ_ClearData(handle);
  if( ret != 0 ) ErrorMsg("something wrong when try to stop ACQ and clear buffer");
  printf("\n\e[1m\e[33m====== Acquisition STOPPED for %d | Board %d, Port %d\e[0m\n", BoardInfo.SerialNumber, boardID, portID);
  AcqRun = false;
  data->PrintStat();
  data->ClearTriggerRate();
  data->ClearNumEventsDecoded();
  data->ClearBuffer();
  data->ClearReferenceTime();
  data->ZeroTotalFileSize();

  ReadACQStatus();
}

unsigned int Digitizer::CalByteForBuffer(bool verbose){
  DebugPrint("%s", "Digitizer");
  unsigned int numAggBLT;
  unsigned int chMask   ;    
  unsigned int boardCfg ;
  unsigned int eventAgg[NumInputCh/2];
  unsigned int recordLength[NumInputCh/2];
  unsigned int aggOrgan;
  
  if( isConnected ){
    numAggBLT = ReadRegister(DPP::MaxAggregatePerBlockTransfer, 0, false);
    chMask    = ReadRegister(DPP::RegChannelEnableMask, 0, false);
    boardCfg  = ReadRegister(DPP::BoardConfiguration, 0, false);
    aggOrgan  = ReadRegister(DPP::AggregateOrganization, 0, false);

    for( int pCh = 0; pCh < NumInputCh/2; pCh++){
      eventAgg[pCh]     = ReadRegister(DPP::NumberEventsPerAggregate_G, pCh * 2 , false);
      recordLength[pCh] = ReadRegister(DPP::RecordLength_G, pCh * 2 , false);
    }
  }else{
    numAggBLT = GetSettingFromMemory(DPP::MaxAggregatePerBlockTransfer);
    chMask    = GetSettingFromMemory(DPP::RegChannelEnableMask);
    boardCfg  = GetSettingFromMemory(DPP::BoardConfiguration);
    aggOrgan  = GetSettingFromMemory(DPP::AggregateOrganization);
    for( int pCh = 0; pCh < NumInputCh/2; pCh++){
      eventAgg[pCh]     = GetSettingFromMemory(DPP::NumberEventsPerAggregate_G, pCh * 2 );
      recordLength[pCh] = GetSettingFromMemory(DPP::RecordLength_G, pCh * 2);
    }
  }
  
  if( verbose ){
    printf("=================================== Setting related to Buffer\n");
    printf("      agg. orgainzation (bit) : 0x%X \n", aggOrgan);
    printf("                 Channel Mask : %04X \n", chMask);
    printf("Max number of Agg per Readout : %u \n", numAggBLT);
    printf("             is Extra2 enabed : %u \n", ((boardCfg >> 17) & 0x1) );
    printf("               is Record wave : %u \n", ((boardCfg >> 16) & 0x1) );
    for( int pCh = 0; pCh < NumInputCh/2; pCh++){
      printf("Paired Ch : %d, RecordLength (bit value): %u, Event per Agg. : %u \n", pCh, recordLength[pCh], eventAgg[pCh]);
    }
    printf("==============================================================\n");
  }

  unsigned int bufferSize = aggOrgan; // just for get rip of the warning in complier
  bufferSize = 0;
  for( int pCh = 0; pCh < NumInputCh/2 ; pCh++){
    if( (chMask & ( 3 << (2 * pCh) )) == 0 ) continue; 
    bufferSize +=  2 + ( 2  + ((boardCfg >> 17) & 0x1) + ((boardCfg >> 16) & 0x1)*recordLength[pCh]*4 ) * eventAgg[pCh] ;
  }
  bufferSize += 4; /// Bd. Agg Header
  bufferSize = bufferSize * numAggBLT * 4; /// 1 words = 4 byte

  ///printf("=============== Buffer Size : %8d Byte \n", bufferSize );
  return  bufferSize ;
}

unsigned int Digitizer::CalByteForBufferCAEN(){
  DebugPrint("%s", "Digitizer");
  char * BufferCAEN;
  uint32_t AllocatedSize;
  ret = CAEN_DGTZ_MallocReadoutBuffer(handle, &BufferCAEN, &AllocatedSize);

  if( BufferCAEN) delete BufferCAEN;
  return AllocatedSize;

}

int Digitizer::ReadData(){
  // DebugPrint("%s", "Digitizer");
  if( softwareDisable ) return CAEN_DGTZ_DigitizerNotReady;
  if( !isConnected ) return CAEN_DGTZ_DigitizerNotFound;
  if( !AcqRun) return CAEN_DGTZ_WrongAcqMode;
  if( data->buffer == NULL ) {
    printf("need allocate memory for readout buffer\n");
    return CAEN_DGTZ_InvalidBuffer;
  }
  
  ret = CAEN_DGTZ_ReadData(handle, CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT, data->buffer, &(data->nByte));
  //uint32_t EventSize = ReadRegister(DPP::EventSize); // Is it as same as data->nByte?
  // if( data->nByte > 0 ) printf("Read Buffer size %d byte \n", data->nByte);
  
  if (ret != CAEN_DGTZ_Success || data->nByte == 0) {
    ErrorMsg(__func__);
    if( ret == CAEN_DGTZ_OutOfMemory) {
      printf("Abort ReadData.\n");
      return ret;
    }
  }

  // ReadACQStatus();

  return ret;
}

void Digitizer::ReadAndPrintACQStatue(){
  DebugPrint("%s", "Digitizer");
  if( !isConnected ) return;
  unsigned int status = ReadRegister(DPP::AcquisitionStatus_R);
  
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
void Digitizer::WriteRegister (Reg registerAddress, uint32_t value, int ch, bool isSave2MemAndFile){
  if( softwareDisable ) return;
  if( AcqRun ) return;
  printf("WRITE|%30s[0x%04X](digi-%d,ch-%02d) [0x%04X]: 0x%08X \n", registerAddress.GetNameChar(), registerAddress.GetAddress(),GetSerialNumber(), ch, registerAddress.ActualAddress(ch), value);

  if( !isConnected ) {
    //SetSettingToMemory(registerAddress, value, ch); //TODO should the binary setting be edited offline?
    //SaveSettingToFile(registerAddress, value, ch);
    return;
  }
  
  if( registerAddress.GetRWType() == RW::ReadONLY ) return;
  
  ret = CAEN_DGTZ_WriteRegister(handle, registerAddress.ActualAddress(ch), value);

  if( registerAddress == DPP::DecimationFactor ) data->SetDecimationFactor(value);

  if( ret == 0 && isSave2MemAndFile && !AcqRun && registerAddress.GetRWType() == RW::ReadWrite ) {
    if( ch < 0 ) {
      if( registerAddress.GetAddress() < 0x8000 ){
        for(int i = 0; i < NumInputCh; i++){
          SetSettingToMemory(registerAddress, value, i);
          SaveSettingToFile(registerAddress, value, i);
        }
      }else{
        SetSettingToMemory(registerAddress, value, 0);
        SaveSettingToFile(registerAddress, value, 0);
      }
    }else{
      SetSettingToMemory(registerAddress, value, ch);
      SaveSettingToFile(registerAddress, value, ch);
      if( registerAddress.IsCoupled() ) {
        SetSettingToMemory(registerAddress, value, ch%2 == 0 ? ch + 1 : ch -1);
        SaveSettingToFile(registerAddress, value, ch%2 == 0 ? ch + 1 : ch -1);
      }
    }
  }

  if( ret == 0 && isSave2MemAndFile && !AcqRun && registerAddress == DPP::QDC::RecordLength_W){
    SetSettingToMemory(registerAddress, value, 0);
    SaveSettingToFile(registerAddress, value, 0);
  }

  std::stringstream ss;
  ss << std::hex << registerAddress.ActualAddress(ch);
  ErrorMsg("WriteRegister:0x" + ss.str()+ "(" + registerAddress.GetName() + ")");
}

uint32_t Digitizer::ReadRegister(Reg registerAddress, unsigned short ch, bool isSave2MemAndFile, std::string str ){
  DebugPrint("%s", "Digitizer");
  if( softwareDisable ) return 0;
  if( AcqRun ) return 0;
  if( !isConnected )  return 0;
  if( registerAddress.GetRWType() == RW::WriteONLY ) return 0;
  // if( registerAddress == DPP::QDC::RecordLength_W ) return 0;

  ret = CAEN_DGTZ_ReadRegister(handle, registerAddress.ActualAddress(ch), &returnData);
  
  if( ret == 0 && isSave2MemAndFile && !AcqRun) {
  //if( isSave2MemAndFile) {
    SetSettingToMemory(registerAddress, returnData, ch);
    SaveSettingToFile(registerAddress, returnData, ch);
  }

  if( registerAddress == DPP::DecimationFactor ) data->SetDecimationFactor( returnData );

  std::stringstream ss;
  ss << std::hex << registerAddress.ActualAddress(ch);

  ErrorMsg("Register:0x" + ss.str() + "(" + registerAddress.GetName() + ")");
  if( !str.empty() ) printf("READ|%s : 0x%04X(0x%04X) is 0x%08X \n", str.c_str(), 
                             registerAddress.ActualAddress(ch), registerAddress.GetAddress(), returnData);
  return returnData;
}

uint32_t Digitizer::PrintRegister(uint32_t address, std::string msg){
  if( !isConnected ) return 0 ;
  printf("\e[33m----------------------------------------------------\n");
  printf("------------ %s = 0x%X \n", msg.c_str(), address);
  printf("----------------------------------------------------\e[0m\n");

  uint32_t value;
  CAEN_DGTZ_ReadRegister(handle, address, &value);
  printf(" %*s    32  28  24  20  16  12   8   4   0\n", (int) msg.length(), "");
  printf(" %*s     |   |   |   |   |   |   |   |   |\n", (int) msg.length(), "");
  printf(" %*s", (int) msg.length(), "");
  std::cout <<    "  : 0b" << std::bitset<32>(value) << std::endl;
  printf(" %*s  : 0x%08X = %u\n", (int) msg.length(), msg.c_str(), value, value);
  
  return value;
}

//========================================== setting file IO
Reg Digitizer::FindRegister(uint32_t address){
  
  Reg tempReg;  
  ///========= Find Match Register
  if( DPPType == DPPTypeCode::DPP_PHA_CODE || DPPType == DPPTypeCode::DPP_PSD_CODE ){
    for( int p = 0; p < (int) RegisterBoardList_PHAPSD[p]; p++){
      if( address == RegisterBoardList_PHAPSD[p].GetAddress() ) {
        tempReg = RegisterBoardList_PHAPSD[p];
        break;
      }
    }
    if( tempReg.GetName() == ""){
      if( DPPType == V1730_DPP_PHA_CODE ){
        for( int p = 0; p < (int) RegisterChannelList_PHA[p]; p++){
          if( address == RegisterChannelList_PHA[p].GetAddress() ) {
            tempReg = RegisterChannelList_PHA[p];
            break;
          }
        }
      }
      if( DPPType == V1730_DPP_PSD_CODE ){
        for( int p = 0; p < (int) RegisterChannelList_PSD[p]; p++){
          if( address == RegisterChannelList_PSD[p].GetAddress() ) {
            tempReg = RegisterChannelList_PSD[p];
            break;
          }
        }
      }
    }
  }else{
    for( int p = 0; p < (int) RegisterBoardList_QDC[p]; p++){
      if( address == RegisterBoardList_QDC[p].GetAddress() ) {
        tempReg = RegisterBoardList_QDC[p];
        break;
      }
    }
    for( int p = 0; p < (int) RegisterChannelList_QDC[p]; p++){
      if( address == RegisterChannelList_QDC[p].GetAddress() ) {
        tempReg = RegisterChannelList_QDC[p];
        break;
      }
    }
  }
  
  return tempReg;
}

void Digitizer::ReadAllSettingsFromBoard(bool force){
  if( softwareDisable ) return;
  if( AcqRun ) return;
  if( !isConnected ) return;
  if( isSettingFilledinMemeory && !force) return;

  printf("===== Digitizer(%d)::%s \n", GetSerialNumber(),  __func__);

  /// board setting
  if( DPPType == DPPTypeCode::DPP_PHA_CODE || DPPType == DPPTypeCode::DPP_PSD_CODE ){

    for( int p = 0; p < (int) RegisterBoardList_PHAPSD.size(); p++){
      if( RegisterBoardList_PHAPSD[p].GetRWType() == RW::WriteONLY) continue;
      if( ModelType == ModelTypeCode::DT && RegisterBoardList_PHAPSD[p].GetAddress() == 0x81C4 ) continue;
      ReadRegister(RegisterBoardList_PHAPSD[p]); 
    }
    regChannelMask = GetSettingFromMemory(DPP::RegChannelEnableMask);
    
    /// Channels Setting
    for( int ch = 0; ch < NumInputCh; ch ++){
      if( DPPType == V1730_DPP_PHA_CODE ){
        for( int p = 0; p < (int) RegisterChannelList_PHA.size(); p++){
          if( RegisterChannelList_PHA[p].GetRWType() == RW::WriteONLY) continue;
          ReadRegister(RegisterChannelList_PHA[p], ch); 
        }
      }
      if( DPPType == V1730_DPP_PSD_CODE ){
        for( int p = 0; p < (int) RegisterChannelList_PSD.size(); p++){
          if( RegisterChannelList_PSD[p].GetRWType() == RW::WriteONLY) continue;
          ReadRegister(RegisterChannelList_PSD[p], ch); 
        }
      }
    }

  }else{
    for( int p = 0; p < (int) RegisterBoardList_QDC.size(); p++){
      if( RegisterBoardList_QDC[p].GetRWType() == RW::WriteONLY) continue;
      ReadRegister(RegisterBoardList_QDC[p]); 
    }

    ReadQDCRecordLength();

    regChannelMask = GetSettingFromMemory(DPP::QDC::GroupEnableMask);

    for( int ch = 0; ch < GetNumRegChannels(); ch ++){
      for( int p = 0; p < (int) RegisterChannelList_QDC.size(); p++){
        if( RegisterChannelList_QDC[p].GetRWType() == RW::WriteONLY) continue;
        ReadRegister(RegisterChannelList_QDC[p], ch); 
      }
    }

  }

  //printf("BoardID : 0x%X = DataFormat \n", GetSettingFromMemory(DPP::BoardID));

  isSettingFilledinMemeory = true;

}

void Digitizer::ProgramSettingsToBoard(){
  DebugPrint("%s", "Digitizer");
  if( softwareDisable ) return;
  if( AcqRun ) return;
  if( !isConnected || isDummy ) return;
  
  printf("========== %s \n", __func__);

  const short pauseMilliSec = 20;

  Reg haha;
  
  if( DPPType == DPPTypeCode::DPP_PHA_CODE || DPPType == DPPTypeCode::DPP_PSD_CODE ){
  
    /// board setting
    //for( int p = 0; p < (int) RegisterBoardList_PHAPSD.size(); p++){
    //  if( RegisterBoardList_PHAPSD[p].GetRWType() == RW::ReadWrite) {
    //    haha = RegisterBoardList_PHAPSD[p];
    //    WriteRegister(haha, GetSettingFromMemory(haha), -1, false); 
    //    usleep(pauseMilliSec * 1000);
    //  }
    //}

    haha = DPP::BoardConfiguration; WriteRegister(haha, GetSettingFromMemory(haha), -1, false);
    haha = DPP::AcquisitionControl; WriteRegister(haha, GetSettingFromMemory(haha), -1, false);
    haha = DPP::GlobalTriggerMask; WriteRegister(haha, GetSettingFromMemory(haha), -1, false);
    haha = DPP::FrontPanelIOControl; WriteRegister(haha, GetSettingFromMemory(haha), -1, false);
    haha = DPP::FrontPanelTRGOUTEnableMask; WriteRegister(haha, GetSettingFromMemory(haha), -1, false);
    haha = DPP::RegChannelEnableMask; WriteRegister(haha, GetSettingFromMemory(haha), -1, false);

    /// Channels Setting
    for( int ch = 0; ch < NumInputCh; ch ++){
      if( DPPType == V1730_DPP_PHA_CODE ){
        for( int p = 0; p < (int) RegisterChannelList_PHA.size(); p++){
          if( RegisterChannelList_PHA[p].GetRWType() == RW::ReadWrite ){      
            haha = RegisterChannelList_PHA[p];
            WriteRegister(haha, GetSettingFromMemory(haha, ch), ch, false); 
            usleep(pauseMilliSec * 1000);
          }
        }
      }
      if( DPPType == V1730_DPP_PSD_CODE ){
        for( int p = 0; p < (int) RegisterChannelList_PSD.size(); p++){
          if( RegisterChannelList_PSD[p].GetRWType() == RW::ReadWrite){
            haha = RegisterChannelList_PSD[p];
            WriteRegister(haha, GetSettingFromMemory(haha, ch), ch, false); 
            usleep(pauseMilliSec * 1000);
          }
        }
      }
    }


  }else{
    /// board setting
    //for( int p = 0; p < (int) RegisterBoardList_QDC.size(); p++){
    //  if( RegisterBoardList_QDC[p].GetRWType() == RW::ReadWrite) {
    //    haha = RegisterBoardList_QDC[p];
    //    WriteRegister(haha, GetSettingFromMemory(haha), -1, false); 
    //    usleep(pauseMilliSec * 1000);
    //  }
    //}

    haha = DPP::BoardConfiguration; WriteRegister(haha, GetSettingFromMemory(haha), -1, false);
    haha = DPP::AcquisitionControl; WriteRegister(haha, GetSettingFromMemory(haha), -1, false);
    haha = DPP::GlobalTriggerMask; WriteRegister(haha, GetSettingFromMemory(haha), -1, false);
    haha = DPP::FrontPanelIOControl; WriteRegister(haha, GetSettingFromMemory(haha), -1, false);
    haha = DPP::FrontPanelTRGOUTEnableMask; WriteRegister(haha, GetSettingFromMemory(haha), -1, false);
    haha = DPP::QDC::GroupEnableMask; WriteRegister(haha, GetSettingFromMemory(haha), -1, false);
    haha = DPP::QDC::RecordLength_W; WriteRegister(haha, GetSettingFromMemory(haha), -1, false);
    // haha = DPP::QDC::NumberEventsPerAggregate; WriteRegister(haha, GetSettingFromMemory(haha), -1, false);

    haha = DPP::DecimationFactor; WriteRegister(haha, GetSettingFromMemory(haha), -1, false);

    /// Channels Setting
    for( int ch = 0; ch < GetNumRegChannels(); ch ++){
      for( int p = 0; p < (int) RegisterChannelList_QDC.size(); p++){
        if( RegisterChannelList_QDC[p].GetRWType() == RW::ReadWrite ){
          haha = RegisterChannelList_QDC[p];
          WriteRegister(haha, GetSettingFromMemory(haha, ch), ch, false); 
          usleep(pauseMilliSec * 1000);
        }
      }
    }

  }

  //set agg
  ret = CAEN_DGTZ_SetNumEventsPerAggregate(handle, 10);
  ret |= CAEN_DGTZ_SetDPPEventAggregation(handle, 0, 0); // Auto set

}

void Digitizer::SetSettingToMemory(Reg registerAddress,  unsigned int value, unsigned short ch ){
  if( AcqRun ) return;
  DebugPrint("%s", "Digitizer");
  unsigned short index = registerAddress.Index(ch);
  if( index > SETTINGSIZE ) return;
  setting[index] = value;
}

unsigned int Digitizer::GetSettingFromMemory(Reg registerAddress, unsigned short ch ){
  DebugPrint("%s", "Digitizer");
  unsigned short index = registerAddress.Index(ch);
  if( index > SETTINGSIZE ) return 0xFFFF;
  return setting[index] ;
}

void Digitizer::PrintSettingFromMemory(){
  DebugPrint("%s", "Digitizer");
  for( int i = 0; i < SETTINGSIZE; i++) printf("%4d | 0x%04X |0x%08X = %u \n", i, i*4, setting[i], setting[i]);
}

void Digitizer::SetSettingBinaryPath(std::string fileName){
  DebugPrint("%s", "Digitizer");
  settingFile = fopen(fileName.c_str(), "r+");
  if( settingFile == NULL ){
    printf("cannot open file %s. Create one.\n", fileName.c_str());
    
    ReadAllSettingsFromBoard();  
    SaveAllSettingsAsBin(fileName);
    
    this->settingFileName = fileName;
    isSettingFileExist = true;
  
  }else{
    this->settingFileName = fileName;
    isSettingFileExist = true;
    fclose(settingFile);
    printf("setting file already exist. do nothing. Should program the digitizer\n");
  }
  
}

int Digitizer::LoadSettingBinaryToMemory(std::string fileName){
  DebugPrint("%s", "Digitizer");
  settingFile = fopen(fileName.c_str(), "r");
  
  if( settingFile == NULL ) {
    printf(" %s does not exist or cannot load.\n", fileName.c_str());
    isSettingFileExist = false;
    return -1;
    
  }else{
    isSettingFileExist = true;
    settingFileName = fileName;
    fclose (settingFile);
    
    uint32_t fileDPP = ((ReadSettingFromFile(DPP::AMCFirmwareRevision_R, 0) >> 8) & 0xFF);
    
    /// compare seeting DPP version;
    if( isConnected && DPPType != (int) fileDPP ){
      printf("DPPType in the file is %s(0x%X), but the dgitizer DPPType is %s(0x%X). \n", GetDPPString(fileDPP).c_str(), fileDPP, GetDPPString().c_str(),  DPPType);
      return -1;
    }else{
      /// load binary to memoery
      DPPType = fileDPP;
      printf("DPPType in the file is %s(0x%X). Board Type is %s \n", GetDPPString(fileDPP).c_str(), fileDPP, GetDPPString().c_str());

      settingFile = fopen(fileName.c_str(), "r");
      size_t dummy = fread( setting, SETTINGSIZE * sizeof(unsigned int), 1, settingFile);
      fclose (settingFile);

      if( dummy != 0 ) printf("reach the end of file (read %ld).\n", dummy);
      
      uint32_t boardInfo = GetSettingFromMemory(DPP::BoardInfo_R);
      if( (boardInfo & 0xFF) == 0x0E ) {tick2ns = 4.0;  NumRegChannel = 16;}// 725
      if( (boardInfo & 0xFF) == 0x0B ) {tick2ns = 2.0;  NumRegChannel = 16;}// 730
      if( (boardInfo & 0xFF) == 0x04 ) {tick2ns = 16.0; NumRegChannel =  8;}// 740

      ///Should seperate file<->memory, memory<->board
      ///ProgramSettingsToBoard(); /// do nothing if not connected.
    
      return 0;
    }
  }
}
    
unsigned int Digitizer::ReadSettingFromFile(Reg registerAddress, unsigned short ch){
  DebugPrint("%s", "Digitizer");
  if ( !isSettingFileExist ) return -1;
  
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

void Digitizer::SaveSettingToFile(Reg registerAddress, unsigned int value, unsigned short ch){
  DebugPrint("%s", "Digitizer");
  if ( !isSettingFileExist ) return ;
  if ( !isSettingFileUpdate ) return;

  // printf("Write setting file : %s. %s, ch:%u, 0x%8X\n", settingFileName.c_str(), registerAddress.GetNameChar(), ch, value);

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
  DebugPrint("%s", "Digitizer");
  SaveAllSettingsAsTextForRun(fileName);
  
  if( !isSettingFilledinMemeory ) return;
  
  settingFileName = fileName;
  isSettingFileExist = true;

}

void Digitizer::SaveAllSettingsAsTextForRun (std::string fileName){
  DebugPrint("%s", "Digitizer");
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
  DebugPrint("%s", "Digitizer");
  if( !isSettingFilledinMemeory && isConnected) return;
  
  FILE * txtFile = fopen(fileName.c_str(), "w+");  
  if( txtFile == NULL ) {
    printf("Cannot open %s.\n", fileName.c_str());
    return;
  }

  Reg haha;
  
  for( unsigned int i = 0; i < SETTINGSIZE ; i++){
    haha.SetName("");
    uint32_t actualAddress = haha.CalAddress(i);

    if( ModelType == ModelTypeCode::DT && actualAddress == 0x81C4 ) continue;
    
    if ( DPPType == V1730_DPP_PHA_CODE || DPPType == V1730_DPP_PSD_CODE ){
      ///printf("%7d--- 0x%04X,  0x%04X\n", i, haha->GetAddress(), haha->ActualAddress());
      for( int p = 0; p < (int) RegisterBoardList_PHAPSD.size(); p++){
        if( haha.GetAddress() == (uint32_t) RegisterBoardList_PHAPSD[p] ) {
          haha = RegisterBoardList_PHAPSD[p];
          break;
        }
      }

      if( DPPType == V1730_DPP_PHA_CODE) {
        for( int p = 0; p < (int) RegisterChannelList_PHA.size(); p++){
          if( haha.GetAddress() == (uint32_t) RegisterChannelList_PHA[p] ) {
            haha = RegisterChannelList_PHA[p];
            break;
          }
        }
      }
      if( DPPType == V1730_DPP_PSD_CODE) {
        for( int p = 0; p < (int) RegisterChannelList_PSD.size(); p++){
          if( haha.GetAddress() == (uint32_t) RegisterChannelList_PSD[p] ) {
            haha = RegisterChannelList_PSD[p];
            break;
          }
        }
      }
    }else{

      for( int p = 0; p < (int) RegisterBoardList_QDC.size(); p++){
        if( haha.GetAddress() == (uint32_t) RegisterBoardList_QDC[p] ) {
          haha = RegisterBoardList_QDC[p];
          break;
        }
      }

      for( int p = 0; p < (int) RegisterChannelList_QDC.size(); p++){
        if( haha.GetAddress() == (uint32_t) RegisterChannelList_QDC[p] ) {
          haha = RegisterChannelList_QDC[p];
          break;
        }
      }

    }

    if( haha.GetName() != "" )  {
      std::string typeStr ;
      if( haha.GetRWType() == RW::ReadWrite ) typeStr = "R/W";
      if( haha.GetRWType() == RW::ReadONLY  ) typeStr = "R  ";
      if( haha.GetRWType() == RW::WriteONLY ) typeStr = "  W";
      fprintf( txtFile, "0x%04X %30s   0x%08X  %s  %u\n", actualAddress, 
                                                          haha.GetNameChar(), 
                                                          setting[i], 
                                                          typeStr.c_str(),
                                                          setting[i]);
    }
  }

  printf("Saved setting as text to %s.\n", fileName.c_str());
  fclose(txtFile);

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
    case CAEN_DGTZ_CommError               : /**  -1 */ printf("%s digi-%d | %d, Communication Error.\n",                         header.c_str(), BoardInfo.SerialNumber, ret); break;
    case CAEN_DGTZ_GenericError            : /**  -2 */ printf("%s digi-%d | %d, Unspecified error.\n",                           header.c_str(), BoardInfo.SerialNumber, ret); break;
    case CAEN_DGTZ_InvalidParam            : /**  -3 */ printf("%s digi-%d | %d, Invalid parameter.\n",                           header.c_str(), BoardInfo.SerialNumber, ret); break;
    case CAEN_DGTZ_InvalidLinkType         : /**  -4 */ printf("%s digi-%d | %d, Invalid Link Type.\n",                           header.c_str(), BoardInfo.SerialNumber, ret); break;
    case CAEN_DGTZ_InvalidHandle           : /**  -5 */ printf("%s digi-%d | %d, Invalid device handler.\n",                      header.c_str(), BoardInfo.SerialNumber, ret); break;
    case CAEN_DGTZ_MaxDevicesError         : /**  -6 */ printf("%s digi-%d | %d, Maximum number of devices exceeded.\n",          header.c_str(), BoardInfo.SerialNumber, ret); break;
    case CAEN_DGTZ_BadBoardType            : /**  -7 */ printf("%s digi-%d | %d, Operation not allowed on this type of board.\n", header.c_str(), BoardInfo.SerialNumber, ret); break;
    case CAEN_DGTZ_BadInterruptLev         : /**  -8 */ printf("%s digi-%d | %d, The interrupt level is not allowed.\n",          header.c_str(), BoardInfo.SerialNumber, ret); break;
    case CAEN_DGTZ_BadEventNumber          : /**  -9 */ printf("%s digi-%d | %d, The event number is bad.\n",                     header.c_str(), BoardInfo.SerialNumber, ret); break;
    case CAEN_DGTZ_ReadDeviceRegisterFail  : /** -10 */ printf("%s digi-%d | %d, Unable to read the registry.\n",                 header.c_str(), BoardInfo.SerialNumber, ret); break;
    case CAEN_DGTZ_WriteDeviceRegisterFail : /** -11 */ printf("%s digi-%d | %d, Unable to write the registry.\n",                header.c_str(), BoardInfo.SerialNumber, ret); break;
    case CAEN_DGTZ_InvalidChannelNumber    : /** -13 */ printf("%s digi-%d | %d, The channel number is invalid.\n",               header.c_str(), BoardInfo.SerialNumber, ret); break;
    case CAEN_DGTZ_ChannelBusy             : /** -14 */ printf("%s digi-%d | %d, The channel is busy.\n",                         header.c_str(), BoardInfo.SerialNumber, ret); break;
    case CAEN_DGTZ_FPIOModeInvalid         : /** -15 */ printf("%s digi-%d | %d, Invalid FPIO mode.\n",                           header.c_str(), BoardInfo.SerialNumber, ret); break;
    case CAEN_DGTZ_WrongAcqMode            : /** -16 */ printf("%s digi-%d | %d, Wrong Acquistion mode.\n",                       header.c_str(), BoardInfo.SerialNumber, ret); break;
    case CAEN_DGTZ_FunctionNotAllowed      : /** -17 */ printf("%s digi-%d | %d, This function is not allowed on this module.\n", header.c_str(), BoardInfo.SerialNumber, ret); break;
    case CAEN_DGTZ_Timeout                 : /** -18 */ printf("%s digi-%d | %d, Communication Timeout.\n",                       header.c_str(), BoardInfo.SerialNumber, ret); break;
    case CAEN_DGTZ_InvalidBuffer           : /** -19 */ printf("%s digi-%d | %d, The buffer is invalid.\n",                       header.c_str(), BoardInfo.SerialNumber, ret); break;
    case CAEN_DGTZ_EventNotFound           : /** -20 */ printf("%s digi-%d | %d, The event is not found.\n",                      header.c_str(), BoardInfo.SerialNumber, ret); break;
    case CAEN_DGTZ_InvalidEvent            : /** -21 */ printf("%s digi-%d | %d, The event is invalid.\n",                        header.c_str(), BoardInfo.SerialNumber, ret); break;
    case CAEN_DGTZ_OutOfMemory             : /** -22 */ printf("%s digi-%d | %d, Out of memory.\n",                               header.c_str(), BoardInfo.SerialNumber, ret); break;
    case CAEN_DGTZ_CalibrationError        : /** -23 */ printf("%s digi-%d | %d, Unable to calibrate the board.\n",               header.c_str(), BoardInfo.SerialNumber, ret); break;
    case CAEN_DGTZ_DigitizerNotFound       : /** -24 */ printf("%s digi-%d | %d, Unbale to open the digitizer.\n",                header.c_str(), BoardInfo.SerialNumber, ret); break;
    case CAEN_DGTZ_DigitizerAlreadyOpen    : /** -25 */ printf("%s digi-%d | %d, The digitizer is already open.\n",               header.c_str(), BoardInfo.SerialNumber, ret); break;
    case CAEN_DGTZ_DigitizerNotReady       : /** -26 */ printf("%s digi-%d | %d, The digitizer is not ready.\n",                  header.c_str(), BoardInfo.SerialNumber, ret); break;
    case CAEN_DGTZ_InterruptNotConfigured  : /** -27 */ printf("%s digi-%d | %d, The digitizer has no IRQ configured.\n",         header.c_str(), BoardInfo.SerialNumber, ret); break;
    case CAEN_DGTZ_DigitizerMemoryCorrupted: /** -28 */ printf("%s digi-%d | %d, The digitizer flash memory is corrupted.\n",     header.c_str(), BoardInfo.SerialNumber, ret); break;
    case CAEN_DGTZ_DPPFirmwareNotSupported : /** -29 */ printf("%s digi-%d | %d, The digitier DPP firmware is not supported in this lib version.\n", header.c_str(), BoardInfo.SerialNumber, ret); break;
    case CAEN_DGTZ_InvalidLicense          : /** -30 */ printf("%s digi-%d | %d, Invalid firmware licence.\n",                     header.c_str(), BoardInfo.SerialNumber, ret); break;
    case CAEN_DGTZ_InvalidDigitizerStatus  : /** -31 */ printf("%s digi-%d | %d, The digitizer is found in a corrupted status.\n", header.c_str(), BoardInfo.SerialNumber, ret); break;
    case CAEN_DGTZ_UnsupportedTrace        : /** -32 */ printf("%s digi-%d | %d, The given trace is not supported.\n",             header.c_str(), BoardInfo.SerialNumber, ret); break;
    case CAEN_DGTZ_InvalidProbe            : /** -33 */ printf("%s digi-%d | %d, The given probe is not supported.\n",             header.c_str(), BoardInfo.SerialNumber, ret); break;
    case CAEN_DGTZ_UnsupportedBaseAddress  : /** -34 */ printf("%s digi-%d | %d, The base address is not supported.\n",            header.c_str(), BoardInfo.SerialNumber, ret); break;
    case CAEN_DGTZ_NotYetImplemented       : /** -99 */ printf("%s digi-%d | %d, The function is not yet implemented.\n",          header.c_str(), BoardInfo.SerialNumber, ret); break;
  }
  
}

//============================== DPP-Alpgorthm Control 
void Digitizer::SetDPPAlgorithmControl(uint32_t bit, int ch){
  DebugPrint("%s", "Digitizer");
  if( softwareDisable ) return;
  if( AcqRun ) return;
  WriteRegister( DPP::DPPAlgorithmControl, bit, ch);    
  if( ret != 0 ) ErrorMsg(__func__);
}

unsigned int Digitizer::ReadBits(Reg address, unsigned int bitLength, unsigned int bitSmallestPos, int ch ){
  DebugPrint("%s", "Digitizer");
  if( softwareDisable ) return 0;
  if( AcqRun ) return 0;
  int tempCh = ch;
  if (ch < 0 && address < 0x8000 ) tempCh = 0; /// take ch-0 
  uint32_t bit = ReadRegister(address, tempCh);
  bit = (bit >> bitSmallestPos ) & uint(pow(2, bitLength)-1);
  return bit;
}

void Digitizer::SetBits(Reg address, unsigned int bitValue, unsigned int bitLength, unsigned int bitSmallestPos, int ch){
  DebugPrint("%s", "Digitizer");
  if( softwareDisable ) return;
  if( AcqRun ) return;
  ///printf("address : 0x%X, value : 0x%X, len : %d, pos : %d, ch : %d \n", address, bitValue, bitLength, bitSmallestPos, ch);
  uint32_t bit ;
  uint32_t bitmask = (uint(pow(2, bitLength)-1) << bitSmallestPos);  
  int tempCh = ch;
  if (ch < 0 && address < 0x8000 ) tempCh = 0; /// take ch-0 
  //bit = ReadRegister(address, tempCh);
  bit = GetSettingFromMemory(address, tempCh);
  ///printf("bit : 0x%X, bitmask : 0x%X \n", bit, bitmask);
  bit = (bit & ~bitmask) | (bitValue << bitSmallestPos);
  ///printf("bit : 0x%X, ch : %d \n", bit, ch);
  WriteRegister(address, bit, ch);
  if( ret != 0 ) ErrorMsg(__func__);  
}

void Digitizer::AutoSetDPPEventAggregation(){ 
  //ret  = CAEN_DGTZ_SetDPPAcquisitionMode(handle, CAEN_DGTZ_DPP_ACQ_MODE_List, CAEN_DGTZ_DPP_SAVE_PARAM_EnergyAndTime);

  // if( DPPType == DPPTypeCode::DPP_QDC_CODE ){

  // }else{

  //   for( int ch = 0; ch < GetNumInputCh(); ch += 2 ){
  //     uint32_t a1, a2;
  //     ret |= CAEN_DGTZ_GetRecordLength(handle, &a1, ch);
  //     ret |= CAEN_DGTZ_GetNumEventsPerAggregate(handle, &a2, ch);
  //     printf("Ch %2d | RecordLength : %d | Event Agg : %d \n", ch, a1, a2);
  //   }

  //   uint32_t chMask ;
  //   ret |= CAEN_DGTZ_GetChannelEnableMask(handle, &chMask);
  //   printf("Ch Mask %0X \n", chMask);

  // }

  ret = 0;
  ret |= CAEN_DGTZ_SetDPPEventAggregation(handle, 0, 0); // AutoSet
  if( ret != 0 ) { 
    printf("!!!!!!!! set %s error.\n", __func__);
  }else{
    Reg regAdd = DPP::AggregateOrganization;
    uint32_t haha = ReadRegister(regAdd);
    SetSettingToMemory(regAdd, haha, 0);
    SaveSettingToFile(regAdd, haha, 0);
  }
}

uint32_t Digitizer::ReadQDCRecordLength()  {
  returnData = ReadRegister(DPP::QDC::RecordLength_R);
  Reg temp = DPP::QDC::RecordLength_R; 
  int indexR = temp.Index(0);
  temp = DPP::QDC::RecordLength_W; 
  int indexW = temp.Index(0);
  setting[indexW] = setting[indexR];
  //printf("%d %d | %u %u \n", indexR, indexW, setting[indexR], setting[indexW]);
  return returnData;
}

void Digitizer::SetQDCOptimialAggOrg(){
  DebugPrint("%s", "Digitizer");
  if( DPPType != DPPTypeCode::DPP_QDC_CODE ) {
    printf("%s | this method only support QDC board.\n", __func__); 
    return;
  }

  uint32_t EventAgg = ReadRegister(DPP::QDC::NumberEventsPerAggregate, 0);
  if( EventAgg == 0 ) WriteRegister(DPP::QDC::NumberEventsPerAggregate, 30);
  uint32_t chMask = ReadRegister(DPP::QDC::GroupEnableMask);
  uint32_t RecordLen = ReadRegister(DPP::QDC::RecordLength_R, 0);  
  if( RecordLen == 0 ) SetBits(DPP::BoardConfiguration, DPP::Bit_BoardConfig::RecordTrace, 0, -1);
  
  uint32_t AggRead  = ReadRegister(DPP::MaxAggregatePerBlockTransfer);
  uint32_t boardCfg = ReadRegister(DPP::BoardConfiguration);
  uint32_t aggOrgan = ReadRegister(DPP::AggregateOrganization);

  bool Ex = ((boardCfg >> 17) & 0x1);
  bool traceOn = ((boardCfg >> 16) & 0x1);

  printf("=================================== Setting related to Buffer\n");
  printf("      agg. orgainzation (bit) : 0x%X \n", aggOrgan);
  printf("                 Channel Mask : %08X \n", chMask);
  printf("Max number of Agg per Readout : %u \n", AggRead);
  printf("              is Extra enabed : %u \n", Ex );
  printf("               is Record wave : %u \n", traceOn );
  printf("                  Event / Agg : %u \n", EventAgg );
  printf("          Record Length (bit) : %u = %u sample = %u ns\n", RecordLen, RecordLen*8, RecordLen*8*16);
  printf("==============================================================\n");

  int eventSize = 6 + 2 * Ex + traceOn * RecordLen * 8; // sample
  printf(" estimated event size : %d sample \n", eventSize);
  double maxAggOrg = log2( MemorySizekSample * 1024 / eventSize / EventAgg );
  printf(" max Agg. Org. should be less than %.2f\n", maxAggOrg);
  uint32_t aggOrg = std::floor(maxAggOrg) ;
  int bufferSize = pow(2, aggOrg) * EventAgg * eventSize;
  printf("================= BufferSize : %d kSample | system memeory : %d kSample \n", bufferSize / 1024, MemorySizekSample);
  
  WriteRegister(DPP::AggregateOrganization, aggOrg);

  //TODO when maxAggOrg < 1, need to reduce the Event/Agg


}