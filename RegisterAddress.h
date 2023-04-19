#ifndef REGISTERADDRESS_H
#define REGISTERADDRESS_H

#include <vector>
#include <string>

///======= 
/// All 0x1XXX registers are either indiviual or Group
/// Indiviual register are all independence
/// Group register, 2m and 2m+1 channels setting are shared. and the name will have _G as prefix 
/// Most 0x8XXX registers are common, which share for all channel

/// For adding Register, two things needed. 
/// 1) add to the namepace
/// 2) add to the RegisterXXXList

/// The Reg Class has conversion operator
/// Reg haha("haha", 0x1234);
/// uint32_t papa = haha; /// papa = 0x1234

namespace Register {  

enum RW { ReadWrite, ReadONLY, WriteONLY};

class Reg{
  public:

    Reg(){
      name = "";
      address = 0;
      type = RW::ReadWrite;
      group = 0;
      maxValue = 0;
      partialStep = 0;
    }
    Reg(std::string name, uint32_t address, RW type = RW::ReadWrite, bool group = false, unsigned int max = 0, int pStep = 0){
      this->name = name;
      this->address = address;
      this->type = type;
      this->group = group;
      this->maxValue = max;
      this->partialStep = pStep;
    };

    ~Reg(){};
    
    operator uint32_t () const {return this->address;}  /// this allows Reg kaka("kaka", 0x1234) uint32_t haha = kaka;
  
    std::string  GetName()         const {return name;}
    const char * GetNameChar()     const {return name.c_str();}
    uint32_t     GetAddress()      const {return address; }
    RW           GetType()         const {return type;}
    bool         GetGroup()        const {return group;}
    unsigned int GetMax()          const {return maxValue;} 
    int          GetPartialStep()  const {return partialStep;} /// step = partialStep * ch2ns, -1 : step = 1
    void         Print()           const ;

    uint32_t  ActualAddress(int ch = -1){
      if( address == 0x8180 ) return  (ch < 0 ? address : (address + 4*(ch/2)));
      if( address <  0x8000 ) return  (ch < 0 ? (address + 0x7000) : (address + (ch << 8)) );
      if( address >= 0x8000 ) return  address;
      return 0;
    }

    unsigned short Index (unsigned short ch);
    uint32_t CalAddress(unsigned int index); /// output actual address, also write the registerAddress
    
    void SetName(std::string str) {this->name = str;}
  
  private:

    std::string name;
    uint32_t address; /// This is the table of register, the actual address should call ActualAddress();
    RW type; /// read/write = 0; read = 1; write = 2
    bool group;
    unsigned int maxValue ;
    int partialStep;

};

inline void Reg::Print() const{
  printf("       Name: %s\n", name.c_str());
  printf(" Re.Address: 0x%04X\n", address);
  printf("       Type: %s\n", type == RW::ReadWrite ? "Read/Write" : (type == RW::ReadONLY ? "Read-Only" : "Write-Only") );
  printf("      Group: %s\n", group ? "True" : "False");
  printf(" Max Value : 0x%X = %d \n", maxValue, maxValue);
}

inline  unsigned short Reg::Index (unsigned short ch){
  unsigned short index;
  if( address == 0x8180){
    index = ((address + 4*(ch/2)) & 0x0FFF) / 4;
  }else if( address < 0x8000){
    index = (address + (ch << 8)) / 4;
  }else{
    if(address < 0xF000) {
      index = (address & 0x0FFF) / 4; 
    }else{ 
      index = ((address & 0x0FFF) + 0x0200 ) / 4; 
    }
  }
  return index;
}

inline uint32_t Reg::CalAddress(unsigned int index){
  
  uint32_t actualAddress = 0xFFFF;
  this->address = 0xFFFF;
  
  if(                        index < 0x0200 /4 )  {actualAddress = index * 4 + 0x8000; this->address =  index * 4 + 0x8000; }
  if( 0x0200 / 4 <= index && index < 0x0300 /4 )  {actualAddress = index * 4 + 0xEE00; this->address =  index * 4 + 0xEE00; }/// EE00 == F000 - 0200
  if( 0x0F00 / 4 <= index && index < 0x1000 /4 )  {actualAddress = index * 4 + 0xE000; this->address =  index * 4 + 0xE000; }
  if( 0x1000 / 4 <= index )                       {actualAddress = index * 4;          this->address = (index * 4) & 0xF0FF; }

  ///for TriggerValidationMask
  if( index == ((0x8180 +  4) & 0x0FFF) / 4 ) {actualAddress = 0x8180 +  4; address = 0x8180;} /// 1
  if( index == ((0x8180 +  8) & 0x0FFF) / 4 ) {actualAddress = 0x8180 +  8; address = 0x8180;} /// 2
  if( index == ((0x8180 + 12) & 0x0FFF) / 4 ) {actualAddress = 0x8180 + 12; address = 0x8180;} /// 3
  if( index == ((0x8180 + 16) & 0x0FFF) / 4 ) {actualAddress = 0x8180 + 16; address = 0x8180;} /// 4
  if( index == ((0x8180 + 20) & 0x0FFF) / 4 ) {actualAddress = 0x8180 + 20; address = 0x8180;} /// 5
  if( index == ((0x8180 + 24) & 0x0FFF) / 4 ) {actualAddress = 0x8180 + 24; address = 0x8180;} /// 6
  if( index == ((0x8180 + 28) & 0x0FFF) / 4 ) {actualAddress = 0x8180 + 28; address = 0x8180;} /// 7
  
  return actualAddress;
}


  
  const Reg EventReadOutBuffer("EventReadOutBuffer", 0x0000, RW::ReadONLY);  /// R
  
  ///========== Channel or Group
  const Reg ChannelDummy32                ("ChannelDummy32"               , 0x1024);  /// R/W
  const Reg InputDynamicRange             ("InputDynamicRange"            , 0x1028);  /// R/W
  const Reg ChannelPulseWidth             ("ChannelPulseWidth"            , 0x1070);  /// R/W
  const Reg ChannelTriggerThreshold       ("ChannelTriggerThreshold"      , 0x1080);  /// R/W
  const Reg CoupleSelfTriggerLogic_G      ("CoupleSelfTriggerLogic_G"     , 0x1084, RW::ReadWrite , 1);  /// R/W 
  const Reg ChannelStatus_R               ("ChannelStatus_R"              , 0x1088, RW::ReadONLY);  /// R
  const Reg AMCFirmwareRevision_R         ("AMCFirmwareRevision_R"        , 0x108C, RW::ReadONLY);  /// R
  const Reg ChannelDCOffset               ("ChannelDCOffset"              , 0x1098);  /// R/W
  const Reg ChannelADCTemperature_R       ("ChannelADCTemperature_R"      , 0x10A8, RW::ReadONLY);  /// R
  const Reg ChannelSelfTriggerRateMeter_R ("ChannelSelfTriggerRateMeter_R", 0x10EC, RW::ReadONLY);  /// R
    
  ///========== Board
  const Reg BoardConfiguration           ("BoardConfiguration"           , 0x8000, RW::ReadWrite);  /// R/W
  const Reg BufferOrganization           ("BufferOrganization"           , 0x800C, RW::ReadWrite);  /// R/W
  const Reg CustomSize                   ("CustomSize"                   , 0x8020, RW::ReadWrite);  /// R/W
  const Reg ADCCalibration_W             ("ADCCalibration_W"             , 0x809C, RW::WriteONLY);  ///   W
  const Reg AcquisitionControl           ("AcquisitionControl"           , 0x8100, RW::ReadWrite);  /// R/W
  const Reg AcquisitionStatus_R          ("AcquisitionStatus_R"          , 0x8104, RW::ReadONLY);  /// R
  const Reg SoftwareTrigger_W            ("SoftwareTrigger_W"            , 0x8108, RW::WriteONLY);  ///   W
  const Reg GlobalTriggerMask            ("GlobalTriggerMask"            , 0x810C, RW::ReadWrite);  /// R/W
  const Reg FrontPanelTRGOUTEnableMask   ("FrontPanelTRGOUTEnableMask"   , 0x8110, RW::ReadWrite);  /// R/W
  const Reg PostTrigger                  ("PostTrigger"                  , 0x8114, RW::ReadWrite);  /// R/W
  const Reg LVDSIOData                   ("LVDSIOData"                   , 0x8118, RW::ReadWrite);  /// R/W
  const Reg FrontPanelIOControl          ("FrontPanelIOControl"          , 0x811C, RW::ReadWrite);  /// R/W
  const Reg ChannelEnableMask            ("ChannelEnableMask"            , 0x8120, RW::ReadWrite);  /// R/W
  const Reg ROCFPGAFirmwareRevision_R    ("ROCFPGAFirmwareRevision_R"    , 0x8124, RW::ReadONLY);  /// R
  const Reg EventStored_R                ("EventStored_R"                , 0x812C, RW::ReadONLY);  /// R
  const Reg VoltageLevelModeConfig       ("VoltageLevelModeConfig"       , 0x8138, RW::ReadWrite);  /// R/W
  const Reg SoftwareClockSync_W          ("SoftwareClockSync_W"          , 0x813C, RW::WriteONLY);  ///   W 
  const Reg BoardInfo_R                  ("BoardInfo_R"                  , 0x8140, RW::ReadONLY);  /// R
  const Reg AnalogMonitorMode            ("AnalogMonitorMode"            , 0x8144, RW::ReadWrite);  /// R/W
  const Reg EventSize_R                  ("EventSize_R"                  , 0x814C, RW::ReadONLY);  /// R
  const Reg FanSpeedControl              ("FanSpeedControl"              , 0x8168, RW::ReadWrite);  /// R/W
  const Reg MemoryBufferAlmostFullLevel  ("MemoryBufferAlmostFullLevel"  , 0x816C, RW::ReadWrite);  /// R/W
  const Reg RunStartStopDelay            ("RunStartStopDelay"            , 0x8170, RW::ReadWrite);  /// R/W
  const Reg BoardFailureStatus_R         ("BoardFailureStatus_R"         , 0x8178, RW::ReadONLY);  /// R
  const Reg FrontPanelLVDSIONewFeatures  ("FrontPanelLVDSIONewFeatures"  , 0x81A0, RW::ReadWrite);  /// R/W
  const Reg BufferOccupancyGain          ("BufferOccupancyGain"          , 0x81B4, RW::ReadWrite);  /// R/W
  const Reg ChannelsShutdown_W           ("ChannelsShutdown_W"           , 0x81C0, RW::WriteONLY);  ///   W 
  const Reg ExtendedVetoDelay            ("ExtendedVetoDelay"            , 0x81C4, RW::ReadWrite);  /// R/W
  const Reg ReadoutControl               ("ReadoutControl"               , 0xEF00, RW::ReadWrite);  /// R/W
  const Reg ReadoutStatus_R              ("ReadoutStatus_R"              , 0xEF04, RW::ReadONLY);  /// R
  const Reg BoardID                      ("BoardID"                      , 0xEF08, RW::ReadWrite);  /// R/W
  const Reg MCSTBaseAddressAndControl    ("MCSTBaseAddressAndControl"    , 0xEF0C, RW::ReadWrite);  /// R/W
  const Reg RelocationAddress            ("RelocationAddress"            , 0xEF10, RW::ReadWrite);  /// R/W
  const Reg InterruptStatusID            ("InterruptStatusID"            , 0xEF14, RW::ReadWrite);  /// R/W
  const Reg InterruptEventNumber         ("InterruptEventNumber"         , 0xEF18, RW::ReadWrite);  /// R/W
  const Reg MaxAggregatePerBlockTransfer ("MaxAggregatePerBlockTransfer" , 0xEF1C, RW::ReadWrite);  /// R/W
  const Reg Scratch                      ("Scratch"                      , 0xEF20, RW::ReadWrite);  /// R/W
  const Reg SoftwareReset_W              ("SoftwareReset_W"              , 0xEF24, RW::WriteONLY);  ///   W
  const Reg SoftwareClear_W              ("SoftwareClear_W"              , 0xEF28, RW::WriteONLY);  ///   W  

  
  ///====== Common for PHA and PSD
  namespace DPP {
    
    const Reg RecordLength_G              ("RecordLength_G"              , 0x1020, RW::ReadWrite, 1, 0x3FFF,  8); /// R/W
    const Reg InputDynamicRange           ("InputDynamicRange"           , 0x1028, RW::ReadWrite, 0,      1, -1); /// R/W
    const Reg NumberEventsPerAggregate_G  ("NumberEventsPerAggregate_G"  , 0x1034, RW::ReadWrite, 1,  0x3FF, -1); /// R/W
    const Reg PreTrigger                  ("PreTrigger"                  , 0x1038, RW::ReadWrite, 0,   0xFF,  4); /// R/W
    const Reg TriggerThreshold            ("TriggerThreshold"            , 0x106C, RW::ReadWrite, 0, 0x3FFF, -1); /// R/W
    const Reg TriggerHoldOffWidth         ("TriggerHoldOffWidth"         , 0x1074, RW::ReadWrite, 0,  0x3FF,  4); /// R/W
    const Reg DPPAlgorithmControl         ("DPPAlgorithmControl"         , 0x1080, RW::ReadWrite); /// R/W 
    const Reg ChannelStatus_R             ("ChannelStatus_R"             , 0x1088, RW::ReadONLY); /// R
    const Reg AMCFirmwareRevision_R       ("AMCFirmwareRevision_R"       , 0x108C, RW::ReadONLY); /// R
    const Reg ChannelDCOffset             ("ChannelDCOffset"             , 0x1098, RW::ReadWrite, 0, 0xFFFF, -1); /// R/W
    const Reg ChannelADCTemperature_R     ("ChannelADCTemperature_R"     , 0x10A8, RW::ReadONLY); /// R
    const Reg IndividualSoftwareTrigger_W ("IndividualSoftwareTrigger_W" , 0x10C0, RW::WriteONLY); ///   W
    const Reg VetoWidth                   ("VetoWidth"                   , 0x10D4, RW::ReadWrite); /// R/W
    
    /// I know there are many duplication, it is the design.
    const Reg BoardConfiguration          ("BoardConfiguration"          , 0x8000, RW::ReadWrite );  /// R/W
    const Reg AggregateOrganization       ("AggregateOrganization"       , 0x800C, RW::ReadWrite );  /// R/W
    const Reg ADCCalibration_W            ("ADCCalibration_W"            , 0x809C, RW::WriteONLY );  ///   W
    const Reg ChannelShutdown_W           ("ChannelShutdown_W"           , 0x80BC, RW::WriteONLY );  ///   W
    const Reg AcquisitionControl          ("AcquisitionControl"          , 0x8100, RW::ReadWrite );  /// R/W
    const Reg AcquisitionStatus_R         ("AcquisitionStatus_R"         , 0x8104, RW::ReadONLY );  /// R
    const Reg SoftwareTrigger_W           ("SoftwareTrigger_W"           , 0x8108, RW::WriteONLY );  ///   W
    const Reg GlobalTriggerMask           ("GlobalTriggerMask"           , 0x810C, RW::ReadWrite );  /// R/W
    const Reg FrontPanelTRGOUTEnableMask  ("FrontPanelTRGOUTEnableMask"  , 0x8110, RW::ReadWrite );  /// R/W
    const Reg LVDSIOData                  ("LVDSIOData"                  , 0x8118, RW::ReadWrite );  /// R/W
    const Reg FrontPanelIOControl         ("FrontPanelIOControl"         , 0x811C, RW::ReadWrite );  /// R/W
    const Reg ChannelEnableMask           ("ChannelEnableMask"           , 0x8120, RW::ReadWrite );  /// R/W
    const Reg ROCFPGAFirmwareRevision_R   ("ROCFPGAFirmwareRevision_R"   , 0x8124, RW::ReadONLY );  /// R
    const Reg EventStored_R               ("EventStored_R"               , 0x812C, RW::ReadONLY );  /// R
    const Reg VoltageLevelModeConfig      ("VoltageLevelModeConfig"      , 0x8138, RW::ReadWrite );  /// R/W
    const Reg SoftwareClockSync_W         ("SoftwareClockSync_W"         , 0x813C, RW::WriteONLY );  ///   W
    const Reg BoardInfo_R                 ("BoardInfo_R"                 , 0x8140, RW::ReadONLY );  /// R    
    const Reg AnalogMonitorMode           ("AnalogMonitorMode"           , 0x8144, RW::ReadWrite );  /// R/W
    const Reg EventSize_R                 ("EventSize_R"                 , 0x814C, RW::ReadONLY );  /// R
    const Reg TimeBombDowncounter_R       ("TimeBombDowncounter_R"       , 0x8158, RW::ReadONLY );  /// R
    const Reg FanSpeedControl             ("FanSpeedControl"             , 0x8168, RW::ReadWrite );  /// R/W
    const Reg RunStartStopDelay           ("RunStartStopDelay"           , 0x8170, RW::ReadWrite );  /// R/W
    const Reg BoardFailureStatus_R        ("BoardFailureStatus_R"        , 0x8178, RW::ReadONLY );  /// R
    const Reg DisableExternalTrigger      ("DisableExternalTrigger"      , 0x817C, RW::ReadWrite );  /// R/W
    const Reg FrontPanelLVDSIONewFeatures ("FrontPanelLVDSIONewFeatures" , 0x81A0, RW::ReadWrite );  /// R/W
    const Reg BufferOccupancyGain         ("BufferOccupancyGain"         , 0x81B4, RW::ReadWrite );  /// R/W
    const Reg ExtendedVetoDelay           ("ExtendedVetoDelay"           , 0x81C4, RW::ReadWrite );  /// R/W
    const Reg ReadoutControl              ("ReadoutControl"              , 0xEF00, RW::ReadWrite );  /// R/W
    const Reg ReadoutStatus_R             ("ReadoutStatus_R"             , 0xEF04, RW::ReadONLY );  /// R
    const Reg BoardID                     ("BoardID"                     , 0xEF08, RW::ReadWrite );  /// R/W 
    const Reg MCSTBaseAddressAndControl   ("MCSTBaseAddressAndControl"   , 0xEF0C, RW::ReadWrite );  /// R/W
    const Reg RelocationAddress           ("RelocationAddress"           , 0xEF10, RW::ReadWrite );  /// R/W
    const Reg InterruptStatusID           ("InterruptStatusID"           , 0xEF14, RW::ReadWrite );  /// R/W
    const Reg InterruptEventNumber        ("InterruptEventNumber"        , 0xEF18, RW::ReadWrite );  /// R/W
    const Reg MaxAggregatePerBlockTransfer("MaxAggregatePerBlockTransfer", 0xEF1C, RW::ReadWrite, 0, 0x3FF, -1);  /// R/W
    const Reg Scratch                     ("Scratch"                     , 0xEF20, RW::ReadWrite );  /// R/W
    const Reg SoftwareReset_W             ("SoftwareReset_W"             , 0xEF24, RW::WriteONLY );  ///   W
    const Reg SoftwareClear_W             ("SoftwareClear_W"             , 0xEF28, RW::WriteONLY );  ///   W  
    const Reg ConfigurationReload_W       ("ConfigurationReload_W"       , 0xEF34, RW::WriteONLY );  ///   W
    const Reg ROMChecksum_R               ("ROMChecksum_R"               , 0xF000, RW::ReadONLY );  /// R
    const Reg ROMChecksumByte2_R          ("ROMChecksumByte2_R"          , 0xF004, RW::ReadONLY );  /// R
    const Reg ROMChecksumByte1_R          ("ROMChecksumByte1_R"          , 0xF008, RW::ReadONLY );  /// R
    const Reg ROMChecksumByte0_R          ("ROMChecksumByte0_R"          , 0xF00C, RW::ReadONLY );  /// R
    const Reg ROMConstantByte2_R          ("ROMConstantByte2_R"          , 0xF010, RW::ReadONLY );  /// R
    const Reg ROMConstantByte1_R          ("ROMConstantByte1_R"          , 0xF014, RW::ReadONLY );  /// R
    const Reg ROMConstantByte0_R          ("ROMConstantByte0_R"          , 0xF018, RW::ReadONLY );  /// R
    const Reg ROM_C_Code_R                ("ROM_C_Code_R"                , 0xF01C, RW::ReadONLY );  /// R
    const Reg ROM_R_Code_R                ("ROM_R_Code_R"                , 0xF020, RW::ReadONLY );  /// R
    const Reg ROM_IEEE_OUI_Byte2_R        ("ROM_IEEE_OUI_Byte2_R"        , 0xF024, RW::ReadONLY );  /// R
    const Reg ROM_IEEE_OUI_Byte1_R        ("ROM_IEEE_OUI_Byte1_R"        , 0xF028, RW::ReadONLY );  /// R
    const Reg ROM_IEEE_OUI_Byte0_R        ("ROM_IEEE_OUI_Byte0_R"        , 0xF02C, RW::ReadONLY );  /// R
    const Reg ROM_BoardVersion_R          ("ROM_BoardVersion_R"          , 0xF030, RW::ReadONLY );  /// R
    const Reg ROM_BoardFromFactor_R       ("ROM_BoardFromFactor_R"       , 0xF034, RW::ReadONLY );  /// R
    const Reg ROM_BoardIDByte1_R          ("ROM_BoardIDByte1_R"          , 0xF038, RW::ReadONLY );  /// R
    const Reg ROM_BoardIDByte0_R          ("ROM_BoardIDByte0_R"          , 0xF03C, RW::ReadONLY );  /// R
    const Reg ROM_PCB_rev_Byte3_R         ("ROM_PCB_rev_Byte3_R"         , 0xF040, RW::ReadONLY );  /// R
    const Reg ROM_PCB_rev_Byte2_R         ("ROM_PCB_rev_Byte2_R"         , 0xF044, RW::ReadONLY );  /// R
    const Reg ROM_PCB_rev_Byte1_R         ("ROM_PCB_rev_Byte1_R"         , 0xF048, RW::ReadONLY );  /// R
    const Reg ROM_PCB_rev_Byte0_R         ("ROM_PCB_rev_Byte0_R"         , 0xF04C, RW::ReadONLY );  /// R
    const Reg ROM_FlashType_R             ("ROM_FlashType_R"             , 0xF050, RW::ReadONLY );  /// R
    const Reg ROM_BoardSerialNumByte1_R   ("ROM_BoardSerialNumByte1_R"   , 0xF080, RW::ReadONLY );  /// R
    const Reg ROM_BoardSerialNumByte0_R   ("ROM_BoardSerialNumByte0_R"   , 0xF084, RW::ReadONLY );  /// R
    const Reg ROM_VCXO_Type_R             ("ROM_VCXO_Type_R"             , 0xF088, RW::ReadONLY );  /// R

    const Reg TriggerValidationMask_G     ("TriggerValidationMask_G"     , 0x8180, RW::ReadWrite , 1);  /// R/W,  
    
    namespace PHA {
      const Reg DataFlush_W               ("DataFlush_W"              , 0x103C, RW::WriteONLY); ///   W   not sure
      const Reg ChannelStopAcquisition    ("ChannelStopAcquisition"   , 0x1040, RW::ReadWrite); /// R/W   not sure
      const Reg RCCR2SmoothingFactor      ("RCCR2SmoothingFactor"     , 0x1054, RW::ReadWrite); /// R/W   Trigger Filter smoothing, triggerSmoothingFactor
      const Reg InputRiseTime             ("InputRiseTime"            , 0x1058, RW::ReadWrite, 0,   0xFF,  4); /// R/W   OK
      const Reg TrapezoidRiseTime         ("TrapezoidRiseTime"        , 0x105C, RW::ReadWrite, 0,  0xFFF,  4); /// R/W   OK
      const Reg TrapezoidFlatTop          ("TrapezoidFlatTop"         , 0x1060, RW::ReadWrite, 0,  0xFFF,  4); /// R/W   OK
      const Reg PeakingTime               ("PeakingTime"              , 0x1064, RW::ReadWrite, 0,  0xFFF,  4); /// R/W   OK
      const Reg DecayTime                 ("DecayTime"                , 0x1068, RW::ReadWrite, 0, 0xFFFF,  4); /// R/W   OK
      const Reg TriggerThreshold          ("TriggerThreshold"         , 0x106C, RW::ReadWrite, 0, 0x3FFF, -1); /// R/W   OK
      const Reg RiseTimeValidationWindow  ("RiseTimeValidationWindow" , 0x1070, RW::ReadWrite, 0,  0x3FF,  1); /// R/W   OK
      const Reg TriggerHoldOffWidth       ("TriggerHoldOffWidth"      , 0x1074, RW::ReadWrite, 0,  0x3FF,  4); /// R/W   OK
      const Reg PeakHoldOff               ("PeakHoldOff"              , 0x1078, RW::ReadWrite, 0,  0x3FF,  4); /// R/W   OK
      const Reg ShapedTriggerWidth        ("ShapedTriggerWidth"       , 0x1084, RW::ReadWrite, 0,  0x3FF,  4); /// R/W   not sure
      const Reg DPPAlgorithmControl2_G    ("DPPAlgorithmControl2_G"   , 0x10A0, RW::ReadWrite, 1); /// R/W   OK
      const Reg FineGain                  ("FineGain"                 , 0x10C4); /// R/W   OK
    }

    namespace PSD  {
      const Reg CFDSetting                     ("CFDSetting"                     , 0x103C); /// R/W
      const Reg ForcedDataFlush_W              ("ForcedDataFlush_W"              , 0x1040, RW::WriteONLY); ///   W
      const Reg ChargeZeroSuppressionThreshold ("ChargeZeroSuppressionThreshold" , 0x1044); /// R/W
      const Reg ShortGateWidth                 ("ShortGateWidth"                 , 0x1054); /// R/W
      const Reg LongGateWidth                  ("LongGateWidth"                  , 0x1058); /// R/W
      const Reg GateOffset                     ("GateOffset"                     , 0x105C); /// R/W
      const Reg TriggerThreshold               ("TriggerThreshold"               , 0x1060); /// R/W
      const Reg FixedBaseline                  ("FixedBaseline"                  , 0x1064); /// R/W
      const Reg TriggerLatency                 ("TriggerLatency"                 , 0x106C); /// R/W
      const Reg ShapedTriggerWidth             ("ShapedTriggerWidth"             , 0x1070); /// R/W
      const Reg TriggerHoldOffWidth            ("TriggerHoldOffWidth"            , 0x1074); /// R/W
      const Reg ThresholdForPSDCut             ("ThresholdForPSDCut"             , 0x1078); /// R/W
      const Reg PurGapThreshold                ("PurGapThreshold"                , 0x107C); /// R/W
      const Reg DPPAlgorithmControl2_G         ("DPPAlgorithmControl2_G"         , 0x1084, RW::ReadWrite, 1); /// R/W 
      const Reg EarlyBaselineFreeze            ("EarlyBaselineFreeze"            , 0x10D8); /// R/W
    }
  }
};  // end of namepace Register

const std::vector<Register::Reg> RegisterPHAList = {
  Register::DPP::PHA::DataFlush_W               ,
  Register::DPP::PHA::ChannelStopAcquisition    ,
  Register::DPP::PHA::RCCR2SmoothingFactor      ,
  Register::DPP::PHA::InputRiseTime             ,
  Register::DPP::PHA::TrapezoidRiseTime         ,
  Register::DPP::PHA::TrapezoidFlatTop          ,
  Register::DPP::PHA::PeakingTime               ,
  Register::DPP::PHA::DecayTime                 ,
  Register::DPP::PHA::TriggerThreshold          ,
  Register::DPP::PHA::RiseTimeValidationWindow  ,
  Register::DPP::PHA::TriggerHoldOffWidth       ,
  Register::DPP::PHA::PeakHoldOff               ,
  Register::DPP::PHA::ShapedTriggerWidth        ,
  Register::DPP::PHA::DPPAlgorithmControl2_G    ,
  Register::DPP::PHA::FineGain                  ,
  
  Register::DPP::RecordLength_G             ,
  Register::DPP::InputDynamicRange          ,
  Register::DPP::NumberEventsPerAggregate_G ,
  Register::DPP::PreTrigger                 ,
  Register::DPP::TriggerThreshold           ,
  Register::DPP::TriggerHoldOffWidth        ,
  Register::DPP::DPPAlgorithmControl        ,
  Register::DPP::ChannelStatus_R            ,
  Register::DPP::AMCFirmwareRevision_R      ,
  Register::DPP::ChannelDCOffset            ,
  Register::DPP::ChannelADCTemperature_R    ,
  Register::DPP::IndividualSoftwareTrigger_W,
  Register::DPP::VetoWidth                  ,
  
  Register::DPP::TriggerValidationMask_G   
};

const std::vector<Register::Reg> RegisterPSDList = {
  Register::DPP::PSD::CFDSetting                     ,
  Register::DPP::PSD::ForcedDataFlush_W              ,
  Register::DPP::PSD::ChargeZeroSuppressionThreshold ,
  Register::DPP::PSD::ShortGateWidth                 ,
  Register::DPP::PSD::LongGateWidth                  ,
  Register::DPP::PSD::GateOffset                     ,
  Register::DPP::PSD::TriggerThreshold               ,
  Register::DPP::PSD::FixedBaseline                  ,
  Register::DPP::PSD::TriggerLatency                 ,
  Register::DPP::PSD::ShapedTriggerWidth             ,
  Register::DPP::PSD::TriggerHoldOffWidth            ,
  Register::DPP::PSD::ThresholdForPSDCut             ,
  Register::DPP::PSD::PurGapThreshold                ,
  Register::DPP::PSD::DPPAlgorithmControl2_G         ,
  Register::DPP::PSD::EarlyBaselineFreeze            ,
  
  Register::DPP::RecordLength_G             ,
  Register::DPP::InputDynamicRange          ,
  Register::DPP::NumberEventsPerAggregate_G ,
  Register::DPP::PreTrigger                 ,
  Register::DPP::TriggerThreshold           ,
  Register::DPP::TriggerHoldOffWidth        ,
  Register::DPP::DPPAlgorithmControl        ,
  Register::DPP::ChannelStatus_R            ,
  Register::DPP::AMCFirmwareRevision_R      ,
  Register::DPP::ChannelDCOffset            ,
  Register::DPP::ChannelADCTemperature_R    ,
  Register::DPP::IndividualSoftwareTrigger_W,
  Register::DPP::VetoWidth                  ,
  
  Register::DPP::TriggerValidationMask_G   
};

/// Only Board Setting 
const std::vector<Register::Reg> RegisterDPPList = {
  
  Register::DPP::BoardConfiguration          ,
  Register::DPP::AggregateOrganization       ,
  Register::DPP::ADCCalibration_W            ,
  Register::DPP::ChannelShutdown_W           ,
  Register::DPP::AcquisitionControl          ,
  Register::DPP::AcquisitionStatus_R         ,
  Register::DPP::SoftwareTrigger_W           ,
  Register::DPP::GlobalTriggerMask           ,
  Register::DPP::FrontPanelTRGOUTEnableMask  ,
  Register::DPP::LVDSIOData                  ,
  Register::DPP::FrontPanelIOControl         ,
  Register::DPP::ChannelEnableMask           ,
  Register::DPP::ROCFPGAFirmwareRevision_R   ,
  Register::DPP::EventStored_R               ,
  Register::DPP::VoltageLevelModeConfig      ,
  Register::DPP::SoftwareClockSync_W         ,
  Register::DPP::BoardInfo_R                 ,
  Register::DPP::AnalogMonitorMode           ,
  Register::DPP::EventSize_R                 ,
  Register::DPP::TimeBombDowncounter_R       ,
  Register::DPP::FanSpeedControl             ,
  Register::DPP::RunStartStopDelay           ,
  Register::DPP::BoardFailureStatus_R        ,
  Register::DPP::DisableExternalTrigger      ,
  Register::DPP::FrontPanelLVDSIONewFeatures ,
  Register::DPP::BufferOccupancyGain         ,
  Register::DPP::ExtendedVetoDelay           ,
  Register::DPP::ReadoutControl              ,
  Register::DPP::ReadoutStatus_R             ,
  Register::DPP::BoardID                     ,
  Register::DPP::MCSTBaseAddressAndControl   ,
  Register::DPP::RelocationAddress           ,
  Register::DPP::InterruptStatusID           ,
  Register::DPP::InterruptEventNumber        ,
  Register::DPP::MaxAggregatePerBlockTransfer,
  Register::DPP::Scratch                     ,
  Register::DPP::SoftwareReset_W             ,
  Register::DPP::SoftwareClear_W             ,
  Register::DPP::ConfigurationReload_W       ,
  Register::DPP::ROMChecksum_R               ,
  Register::DPP::ROMChecksumByte2_R          ,
  Register::DPP::ROMChecksumByte1_R          ,
  Register::DPP::ROMChecksumByte0_R          ,
  Register::DPP::ROMConstantByte2_R          ,
  Register::DPP::ROMConstantByte1_R          ,
  Register::DPP::ROMConstantByte0_R          ,
  Register::DPP::ROM_C_Code_R                ,
  Register::DPP::ROM_R_Code_R                ,
  Register::DPP::ROM_IEEE_OUI_Byte2_R        ,
  Register::DPP::ROM_IEEE_OUI_Byte1_R        ,
  Register::DPP::ROM_IEEE_OUI_Byte0_R        ,
  Register::DPP::ROM_BoardVersion_R          ,
  Register::DPP::ROM_BoardFromFactor_R       ,
  Register::DPP::ROM_BoardIDByte1_R          ,
  Register::DPP::ROM_BoardIDByte0_R          ,
  Register::DPP::ROM_PCB_rev_Byte3_R         ,
  Register::DPP::ROM_PCB_rev_Byte2_R         ,
  Register::DPP::ROM_PCB_rev_Byte1_R         ,
  Register::DPP::ROM_PCB_rev_Byte0_R         ,
  Register::DPP::ROM_FlashType_R             ,
  Register::DPP::ROM_BoardSerialNumByte1_R   ,
  Register::DPP::ROM_BoardSerialNumByte0_R   ,
  Register::DPP::ROM_VCXO_Type_R             
  
};

#endif
