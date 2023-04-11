#ifndef REGISTERADDRESS_H
#define REGISTERADDRESS_H

#include <vector>

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

enum RW { ReadWrite = 0, ReadONLY = 1, WriteONLY = 2};

class Reg{
  public:
    Reg(){
      this->name = "";
      this->address = 0;
      this->type = 0;
      this->group = 0;
    }
    Reg(std::string name, uint32_t address, char type = 0, bool group = 0){
      this->name = name;
      this->address = address;
      this->type = type;
      this->group = group;
    };

    ~Reg(){};
    
    operator uint32_t () const {return this->address;}  /// this allows Reg kaka("kaka", 0x1234) uint32_t haha = kaka;
  
    std::string  GetName()         const {return this->name;}
    const char * GetNameChar()     const {return this->name.c_str();}
    uint32_t     GetAddress()      const {return this->address; }
    char         GetType()         const {return this->type;}
    bool         GetGroup()        const {return this->group;}
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
    uint32_t address; /// This is the table of register, the actual address should call ActualAddress();
    std::string name;
    char type; /// read/write = 0; read = 1; write = 2
    bool group;
};

inline void Reg::Print() const{
  printf("   Name: %s\n", name.c_str());
  printf("Address: 0x%04X\n", address);
  printf("   Type: %s\n", type == RW::ReadWrite ? "Read/Write" : (type == RW::ReadONLY ? "Read-Only" : "Write-Only") );
  printf("  Group: %s\n", group ? "True" : "False");
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

namespace Register {  
  
  const Reg EventReadOutBuffer("EventReadOutBuffer", 0x0000, 1);  /// R
  
  ///========== Channel or Group
  const Reg ChannelDummy32                ("ChannelDummy32"               , 0x1024);  /// R/W
  const Reg InputDynamicRange             ("InputDynamicRange"            , 0x1028);  /// R/W
  const Reg ChannelPulseWidth             ("ChannelPulseWidth"            , 0x1070);  /// R/W
  const Reg ChannelTriggerThreshold       ("ChannelTriggerThreshold"      , 0x1080);  /// R/W
  const Reg CoupleSelfTriggerLogic_G      ("CoupleSelfTriggerLogic_G"     , 0x1084, 0 , 1);  /// R/W 
  const Reg ChannelStatus_R               ("ChannelStatus_R"              , 0x1088, 1);  /// R
  const Reg AMCFirmwareRevision_R         ("AMCFirmwareRevision_R"        , 0x108C, 1);  /// R
  const Reg ChannelDCOffset               ("ChannelDCOffset"              , 0x1098);  /// R/W
  const Reg ChannelADCTemperature_R       ("ChannelADCTemperature_R"      , 0x10A8, 1);  /// R
  const Reg ChannelSelfTriggerRateMeter_R ("ChannelSelfTriggerRateMeter_R", 0x10EC, 1);  /// R
    
  ///========== Board
  const Reg BoardConfiguration           ("BoardConfiguration"           , 0x8000, 0);  /// R/W
  const Reg BufferOrganization           ("BufferOrganization"           , 0x800C, 0);  /// R/W
  const Reg CustomSize                   ("CustomSize"                   , 0x8020, 0);  /// R/W
  const Reg ADCCalibration_W             ("ADCCalibration_W"             , 0x809C, 2);  ///   W
  const Reg AcquisitionControl           ("AcquisitionControl"           , 0x8100, 0);  /// R/W
  const Reg AcquisitionStatus_R          ("AcquisitionStatus_R"          , 0x8104, 1);  /// R
  const Reg SoftwareTrigger_W            ("SoftwareTrigger_W"            , 0x8108, 2);  ///   W
  const Reg GlobalTriggerMask            ("GlobalTriggerMask"            , 0x810C, 0);  /// R/W
  const Reg FrontPanelTRGOUTEnableMask   ("FrontPanelTRGOUTEnableMask"   , 0x8110, 0);  /// R/W
  const Reg PostTrigger                  ("PostTrigger"                  , 0x8114, 0);  /// R/W
  const Reg LVDSIOData                   ("LVDSIOData"                   , 0x8118, 0);  /// R/W
  const Reg FrontPanelIOControl          ("FrontPanelIOControl"          , 0x811C, 0);  /// R/W
  const Reg ChannelEnableMask            ("ChannelEnableMask"            , 0x8120, 0);  /// R/W
  const Reg ROCFPGAFirmwareRevision_R    ("ROCFPGAFirmwareRevision_R"    , 0x8124, 1);  /// R
  const Reg EventStored_R                ("EventStored_R"                , 0x812C, 1);  /// R
  const Reg VoltageLevelModeConfig       ("VoltageLevelModeConfig"       , 0x8138, 0);  /// R/W
  const Reg SoftwareClockSync_W          ("SoftwareClockSync_W"          , 0x813C, 2);  ///   W 
  const Reg BoardInfo_R                  ("BoardInfo_R"                  , 0x8140, 1);  /// R
  const Reg AnalogMonitorMode            ("AnalogMonitorMode"            , 0x8144, 0);  /// R/W
  const Reg EventSize_R                  ("EventSize_R"                  , 0x814C, 1);  /// R
  const Reg FanSpeedControl              ("FanSpeedControl"              , 0x8168, 0);  /// R/W
  const Reg MemoryBufferAlmostFullLevel  ("MemoryBufferAlmostFullLevel"  , 0x816C, 0);  /// R/W
  const Reg RunStartStopDelay            ("RunStartStopDelay"            , 0x8170, 0);  /// R/W
  const Reg BoardFailureStatus_R         ("BoardFailureStatus_R"         , 0x8178, 1);  /// R
  const Reg FrontPanelLVDSIONewFeatures  ("FrontPanelLVDSIONewFeatures"  , 0x81A0, 0);  /// R/W
  const Reg BufferOccupancyGain          ("BufferOccupancyGain"          , 0x81B4, 0);  /// R/W
  const Reg ChannelsShutdown_W           ("ChannelsShutdown_W"           , 0x81C0, 2);  ///   W 
  const Reg ExtendedVetoDelay            ("ExtendedVetoDelay"            , 0x81C4, 0);  /// R/W
  const Reg ReadoutControl               ("ReadoutControl"               , 0xEF00, 0);  /// R/W
  const Reg ReadoutStatus_R              ("ReadoutStatus_R"              , 0xEF04, 1);  /// R
  const Reg BoardID                      ("BoardID"                      , 0xEF08, 0);  /// R/W
  const Reg MCSTBaseAddressAndControl    ("MCSTBaseAddressAndControl"    , 0xEF0C, 0);  /// R/W
  const Reg RelocationAddress            ("RelocationAddress"            , 0xEF10, 0);  /// R/W
  const Reg InterruptStatusID            ("InterruptStatusID"            , 0xEF14, 0);  /// R/W
  const Reg InterruptEventNumber         ("InterruptEventNumber"         , 0xEF18, 0);  /// R/W
  const Reg MaxAggregatePerBlockTransfer ("MaxAggregatePerBlockTransfer" , 0xEF1C, 0);  /// R/W
  const Reg Scratch                      ("Scratch"                      , 0xEF20, 0);  /// R/W
  const Reg SoftwareReset_W              ("SoftwareReset_W"              , 0xEF24, 2);  ///   W
  const Reg SoftwareClear_W              ("SoftwareClear_W"              , 0xEF28, 2);  ///   W  

  
  ///====== Common for PHA and PSD
  namespace DPP {
    
    const Reg RecordLength_G              ("RecordLength_G"              , 0x1020, 0, 1); /// R/W
    const Reg InputDynamicRange           ("InputDynamicRange"           , 0x1028, 0); /// R/W
    const Reg NumberEventsPerAggregate_G  ("NumberEventsPerAggregate_G"  , 0x1034, 0, 1); /// R/W
    const Reg PreTrigger                  ("PreTrigger"                  , 0x1038, 0); /// R/W
    const Reg TriggerThreshold            ("TriggerThreshold"            , 0x106C, 0); /// R/W
    const Reg TriggerHoldOffWidth         ("TriggerHoldOffWidth"         , 0x1074, 0); /// R/W
    const Reg DPPAlgorithmControl         ("DPPAlgorithmControl"         , 0x1080, 0); /// R/W 
    const Reg ChannelStatus_R             ("ChannelStatus_R"             , 0x1088, 1); /// R
    const Reg AMCFirmwareRevision_R       ("AMCFirmwareRevision_R"       , 0x108C, 1); /// R
    const Reg ChannelDCOffset             ("ChannelDCOffset"             , 0x1098, 0); /// R/W
    const Reg ChannelADCTemperature_R     ("ChannelADCTemperature_R"     , 0x10A8, 1); /// R
    const Reg IndividualSoftwareTrigger_W ("IndividualSoftwareTrigger_W" , 0x10C0, 2); ///   W
    const Reg VetoWidth                   ("VetoWidth"                   , 0x10D4, 0); /// R/W
    
    /// I know there are many duplication, it is the design.
    const Reg BoardConfiguration          ("BoardConfiguration"          , 0x8000, 0 );  /// R/W
    const Reg AggregateOrganization       ("AggregateOrganization"       , 0x800C, 0 );  /// R/W
    const Reg ADCCalibration_W            ("ADCCalibration_W"            , 0x809C, 2 );  ///   W
    const Reg ChannelShutdown_W           ("ChannelShutdown_W"           , 0x80BC, 2 );  ///   W
    const Reg AcquisitionControl          ("AcquisitionControl"          , 0x8100, 0 );  /// R/W
    const Reg AcquisitionStatus_R         ("AcquisitionStatus_R"         , 0x8104, 1 );  /// R
    const Reg SoftwareTrigger_W           ("SoftwareTrigger_W"           , 0x8108, 2 );  ///   W
    const Reg GlobalTriggerMask           ("GlobalTriggerMask"           , 0x810C, 0 );  /// R/W
    const Reg FrontPanelTRGOUTEnableMask  ("FrontPanelTRGOUTEnableMask"  , 0x8110, 0 );  /// R/W
    const Reg LVDSIOData                  ("LVDSIOData"                  , 0x8118, 0 );  /// R/W
    const Reg FrontPanelIOControl         ("FrontPanelIOControl"         , 0x811C, 0 );  /// R/W
    const Reg ChannelEnableMask           ("ChannelEnableMask"           , 0x8120, 0 );  /// R/W
    const Reg ROCFPGAFirmwareRevision_R   ("ROCFPGAFirmwareRevision_R"   , 0x8124, 1 );  /// R
    const Reg EventStored_R               ("EventStored_R"               , 0x812C, 1 );  /// R
    const Reg VoltageLevelModeConfig      ("VoltageLevelModeConfig"      , 0x8138, 0 );  /// R/W
    const Reg SoftwareClockSync_W         ("SoftwareClockSync_W"         , 0x813C, 2 );  ///   W
    const Reg BoardInfo_R                 ("BoardInfo_R"                 , 0x8140, 1 );  /// R    
    const Reg AnalogMonitorMode           ("AnalogMonitorMode"           , 0x8144, 0 );  /// R/W
    const Reg EventSize_R                 ("EventSize_R"                 , 0x814C, 1 );  /// R
    const Reg TimeBombDowncounter_R       ("TimeBombDowncounter_R"       , 0x8158, 1 );  /// R
    const Reg FanSpeedControl             ("FanSpeedControl"             , 0x8168, 0 );  /// R/W
    const Reg RunStartStopDelay           ("RunStartStopDelay"           , 0x8170, 0 );  /// R/W
    const Reg BoardFailureStatus_R        ("BoardFailureStatus_R"        , 0x8178, 1 );  /// R
    const Reg DisableExternalTrigger      ("DisableExternalTrigger"      , 0x817C, 0 );  /// R/W
    const Reg TriggerValidationMask_G     ("TriggerValidationMask_G"     , 0x8180, 0 , 1);  /// R/W,  
    const Reg FrontPanelLVDSIONewFeatures ("FrontPanelLVDSIONewFeatures" , 0x81A0, 0 );  /// R/W
    const Reg BufferOccupancyGain         ("BufferOccupancyGain"         , 0x81B4, 0 );  /// R/W
    const Reg ExtendedVetoDelay           ("ExtendedVetoDelay"           , 0x81C4, 0 );  /// R/W
    const Reg ReadoutControl              ("ReadoutControl"              , 0xEF00, 0 );  /// R/W
    const Reg ReadoutStatus_R             ("ReadoutStatus_R"             , 0xEF04, 1 );  /// R
    const Reg BoardID                     ("BoardID"                     , 0xEF08, 0 );  /// R/W 
    const Reg MCSTBaseAddressAndControl   ("MCSTBaseAddressAndControl"   , 0xEF0C, 0 );  /// R/W
    const Reg RelocationAddress           ("RelocationAddress"           , 0xEF10, 0 );  /// R/W
    const Reg InterruptStatusID           ("InterruptStatusID"           , 0xEF14, 0 );  /// R/W
    const Reg InterruptEventNumber        ("InterruptEventNumber"        , 0xEF18, 0 );  /// R/W
    const Reg MaxAggregatePerBlockTransfer("MaxAggregatePerBlockTransfer", 0xEF1C, 0 );  /// R/W
    const Reg Scratch                     ("Scratch"                     , 0xEF20, 0 );  /// R/W
    const Reg SoftwareReset_W             ("SoftwareReset_W"             , 0xEF24, 2 );  ///   W
    const Reg SoftwareClear_W             ("SoftwareClear_W"             , 0xEF28, 2 );  ///   W  
    const Reg ConfigurationReload_W       ("ConfigurationReload_W"       , 0xEF34, 2 );  ///   W
    const Reg ROMChecksum_R               ("ROMChecksum_R"               , 0xF000, 1 );  /// R
    const Reg ROMChecksumByte2_R          ("ROMChecksumByte2_R"          , 0xF004, 1 );  /// R
    const Reg ROMChecksumByte1_R          ("ROMChecksumByte1_R"          , 0xF008, 1 );  /// R
    const Reg ROMChecksumByte0_R          ("ROMChecksumByte0_R"          , 0xF00C, 1 );  /// R
    const Reg ROMConstantByte2_R          ("ROMConstantByte2_R"          , 0xF010, 1 );  /// R
    const Reg ROMConstantByte1_R          ("ROMConstantByte1_R"          , 0xF014, 1 );  /// R
    const Reg ROMConstantByte0_R          ("ROMConstantByte0_R"          , 0xF018, 1 );  /// R
    const Reg ROM_C_Code_R                ("ROM_C_Code_R"                , 0xF01C, 1 );  /// R
    const Reg ROM_R_Code_R                ("ROM_R_Code_R"                , 0xF020, 1 );  /// R
    const Reg ROM_IEEE_OUI_Byte2_R        ("ROM_IEEE_OUI_Byte2_R"        , 0xF024, 1 );  /// R
    const Reg ROM_IEEE_OUI_Byte1_R        ("ROM_IEEE_OUI_Byte1_R"        , 0xF028, 1 );  /// R
    const Reg ROM_IEEE_OUI_Byte0_R        ("ROM_IEEE_OUI_Byte0_R"        , 0xF02C, 1 );  /// R
    const Reg ROM_BoardVersion_R          ("ROM_BoardVersion_R"          , 0xF030, 1 );  /// R
    const Reg ROM_BoardFromFactor_R       ("ROM_BoardFromFactor_R"       , 0xF034, 1 );  /// R
    const Reg ROM_BoardIDByte1_R          ("ROM_BoardIDByte1_R"          , 0xF038, 1 );  /// R
    const Reg ROM_BoardIDByte0_R          ("ROM_BoardIDByte0_R"          , 0xF03C, 1 );  /// R
    const Reg ROM_PCB_rev_Byte3_R         ("ROM_PCB_rev_Byte3_R"         , 0xF040, 1 );  /// R
    const Reg ROM_PCB_rev_Byte2_R         ("ROM_PCB_rev_Byte2_R"         , 0xF044, 1 );  /// R
    const Reg ROM_PCB_rev_Byte1_R         ("ROM_PCB_rev_Byte1_R"         , 0xF048, 1 );  /// R
    const Reg ROM_PCB_rev_Byte0_R         ("ROM_PCB_rev_Byte0_R"         , 0xF04C, 1 );  /// R
    const Reg ROM_FlashType_R             ("ROM_FlashType_R"             , 0xF050, 1 );  /// R
    const Reg ROM_BoardSerialNumByte1_R   ("ROM_BoardSerialNumByte1_R"   , 0xF080, 1 );  /// R
    const Reg ROM_BoardSerialNumByte0_R   ("ROM_BoardSerialNumByte0_R"   , 0xF084, 1 );  /// R
    const Reg ROM_VCXO_Type_R             ("ROM_VCXO_Type_R"             , 0xF088, 1 );  /// R
    
    namespace PHA {
      const Reg DataFlush_W               ("DataFlush_W"              , 0x103C, 2); ///   W   not sure
      const Reg ChannelStopAcquisition    ("ChannelStopAcquisition"   , 0x1040); /// R/W   not sure
      const Reg RCCR2SmoothingFactor      ("RCCR2SmoothingFactor"     , 0x1054); /// R/W   Trigger Filter smoothing, triggerSmoothingFactor
      const Reg InputRiseTime             ("InputRiseTime"            , 0x1058); /// R/W   OK
      const Reg TrapezoidRiseTime         ("TrapezoidRiseTime"        , 0x105C); /// R/W   OK
      const Reg TrapezoidFlatTop          ("TrapezoidFlatTop"         , 0x1060); /// R/W   OK
      const Reg PeakingTime               ("PeakingTime"              , 0x1064); /// R/W   OK
      const Reg DecayTime                 ("DecayTime"                , 0x1068); /// R/W   OK
      const Reg TriggerThreshold          ("TriggerThreshold"         , 0x106C); /// R/W   OK
      const Reg RiseTimeValidationWindow  ("RiseTimeValidationWindow" , 0x1070); /// R/W   OK
      const Reg TriggerHoldOffWidth       ("TriggerHoldOffWidth"      , 0x1074); /// R/W   OK
      const Reg PeakHoldOff               ("PeakHoldOff"              , 0x1078); /// R/W   OK
      const Reg ShapedTriggerWidth        ("ShapedTriggerWidth"       , 0x1084); /// R/W   not sure
      const Reg DPPAlgorithmControl2_G    ("DPPAlgorithmControl2_G"   , 0x10A0, 0, 1); /// R/W   OK
      const Reg FineGain                  ("FineGain"                 , 0x10C4); /// R/W   OK
    }

    namespace PSD  {
      const Reg CFDSetting                     ("CFDSetting"                     , 0x103C); /// R/W
      const Reg ForcedDataFlush_W              ("ForcedDataFlush_W"              , 0x1040, 2); ///   W
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
      const Reg DPPAlgorithmControl2_G         ("DPPAlgorithmControl2_G"         , 0x1084, 0, 1); /// R/W 
      const Reg EarlyBaselineFreeze            ("EarlyBaselineFreeze"            , 0x10D8); /// R/W
    }
  }
};

const std::vector<Reg> RegisterPHAList = {
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

const std::vector<Reg> RegisterPSDList = {
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
const std::vector<Reg> RegisterDPPList = {
  
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


/***************************
namespace Register {  
  const uint32_t EventReadOutBuffer      = 0x0000;  /// R
  
  ///========== Channel or Group
  const uint32_t ChannelDummy32                = 0x1024;  /// R/W
  const uint32_t InputDynamicRange             = 0x1028;  /// R/W
  const uint32_t ChannelPulseWidth             = 0x1070;  /// R/W
  const uint32_t ChannelTriggerThreshold       = 0x1080;  /// R/W
  const uint32_t CoupleSelfTriggerLogic_G      = 0x1084;  /// R/W 
  const uint32_t ChannelStatus_R               = 0x1088;  /// R
  const uint32_t AMCFirmwareRevision_          = 0x108C;  /// R
  const uint32_t ChannelDCOffset               = 0x1098;  /// R/W
  const uint32_t ChannelADCTemperature_R       = 0x10A8;  /// R
  const uint32_t ChannelSelfTriggerRateMeter_R = 0x10EC;  /// R
    
  ///========== Board
  const uint32_t BoardConfiguration           = 0x8000;  /// R/W
  const uint32_t BufferOrganization           = 0x800C;  /// R/W
  const uint32_t CustomSize                   = 0x8020;  /// R/W
  const uint32_t ADCCalibration_W             = 0x809C;  ///   W
  const uint32_t AcquisitionControl           = 0x8100;  /// R/W
  const uint32_t AcquisitionStatus_R          = 0x8104;  /// R
  const uint32_t SoftwareTrigger_W            = 0x8108;  ///   W
  const uint32_t GlobalTriggerMask            = 0x810C;  /// R/W
  const uint32_t FrontPanelTRGOUTEnableMask   = 0x8110;  /// R/W
  const uint32_t PostTrigger                  = 0x8114;  /// R/W
  const uint32_t LVDSIOData                   = 0x8118;  /// R/W
  const uint32_t FrontPanelIOControl          = 0x811C;  /// R/W
  const uint32_t ChannelEnableMask            = 0x8120;  /// R/W
  const uint32_t ROCFPGAFirmwareRevision_R    = 0x8124;  /// R
  const uint32_t EventStored_R                = 0x812C;  /// R
  const uint32_t VoltageLevelModeConfig       = 0x8138;  /// R/W
  const uint32_t SoftwareClockSync_W          = 0x813C;  ///   W 
  const uint32_t BoardInfo_R                  = 0x8140;  /// R
  const uint32_t AnalogMonitorMode            = 0x8144;  /// R/W
  const uint32_t EventSize_R                  = 0x814C;  /// R
  const uint32_t FanSpeedControl              = 0x8168;  /// R/W
  const uint32_t MemoryBufferAlmostFullLevel  = 0x816C;  /// R/W
  const uint32_t RunStartStopDelay            = 0x8170;  /// R/W
  const uint32_t BoardFailureStatus_R         = 0x8178;  /// R
  const uint32_t FrontPanelLVDSIONewFeatures  = 0x81A0;  /// R/W
  const uint32_t BufferOccupancyGain          = 0x81B4;  /// R/W
  const uint32_t ChannelsShutdown_W           = 0x81C0;  ///   W 
  const uint32_t ExtendedVetoDelay            = 0x81C4;  /// R/W
  const uint32_t ReadoutControl               = 0xEF00;  /// R/W
  const uint32_t ReadoutStatus_R              = 0xEF04;  /// R
  const uint32_t BoardID                      = 0xEF08;  /// R/W
  const uint32_t MCSTBaseAddressAndControl    = 0xEF0C;  /// R/W
  const uint32_t RelocationAddress            = 0xEF10;  /// R/W
  const uint32_t InterruptStatusID            = 0xEF14;  /// R/W
  const uint32_t InterruptEventNumber         = 0xEF18;  /// R/W
  const uint32_t MaxAggregatePerBlockTransfer = 0xEF1C;  /// R/W
  const uint32_t Scratch                      = 0xEF20;  /// R/W
  const uint32_t SoftwareReset_W              = 0xEF24;  ///   W
  const uint32_t SoftwareClear_W              = 0xEF28;  ///   W  

  ///====== Common for PHA and PSD
  namespace DPP {
    const uint32_t RecordLength_G              = 0x1020; /// R/W
    const uint32_t InputDynamicRange           = 0x1028; /// R/W
    const uint32_t NumberEventsPerAggregate_G  = 0x1034; /// R/W
    const uint32_t PreTrigger                  = 0x1038; /// R/W
    const uint32_t TriggerThreshold            = 0x106C; /// R/W
    const uint32_t TriggerHoldOffWidth         = 0x1074; /// R/W
    const uint32_t DPPAlgorithmControl         = 0x1080; /// R/W 
    const uint32_t ChannelStatus_R             = 0x1088; /// R
    const uint32_t AMCFirmwareRevision_R       = 0x108C; /// R
    const uint32_t ChannelDCOffset             = 0x1098; /// R/W
    const uint32_t ChannelADCTemperature_R     = 0x10A8; /// R
    const uint32_t IndividualSoftwareTrigger_W = 0x10C0; ///   W
    const uint32_t VetoWidth                   = 0x10D4; /// R/W
    
    /// I know there are many duplication, it is the design.
    const uint32_t BoardConfiguration          = 0x8000;  /// R/W
    const uint32_t AggregateOrganization       = 0x800C;  /// R/W
    const uint32_t ADCCalibration_W            = 0x809C;  ///   W
    const uint32_t ChannelShutdown_W           = 0x80BC;  ///   W
    const uint32_t AcquisitionControl          = 0x8100;  /// R/W
    const uint32_t AcquisitionStatus_R         = 0x8104;  /// R
    const uint32_t SoftwareTrigger_W           = 0x8108;  ///   W
    const uint32_t GlobalTriggerMask           = 0x810C;  /// R/W
    const uint32_t FrontPanelTRGOUTEnableMask  = 0x8110;  /// R/W
    const uint32_t LVDSIOData                  = 0x8118;  /// R/W
    const uint32_t FrontPanelIOControl         = 0x811C;  /// R/W
    const uint32_t ChannelEnableMask           = 0x8120;  /// R/W
    const uint32_t ROCFPGAFirmwareRevision_R   = 0x8124;  /// R
    const uint32_t EventStored_R               = 0x812C;  /// R
    const uint32_t VoltageLevelModeConfig      = 0x8138;  /// R/W
    const uint32_t SoftwareClockSync_W         = 0x813C;  ///   W
    const uint32_t BoardInfo_R                 = 0x8140;  /// R    /// [0:7] 0x0E = 725, 0x0B = 730, [8:15] 0x01 = 640 kSample, 0x08 = 5.12 MSample, [16:23] channel number
    const uint32_t AnalogMonitorMode           = 0x8144;  /// R/W
    const uint32_t EventSize_R                 = 0x814C;  /// R
    const uint32_t TimeBombDowncounter_R       = 0x8158;  /// R
    const uint32_t FanSpeedControl             = 0x8168;  /// R/W
    const uint32_t RunStartStopDelay           = 0x8170;  /// R/W
    const uint32_t BoardFailureStatus_R        = 0x8178;  /// R
    const uint32_t DisableExternalTrigger      = 0x817C;  /// R/W
    const uint32_t TriggerValidationMask_G     = 0x8180;  /// R/W,  0x8180 + 4n
    const uint32_t FrontPanelLVDSIONewFeatures = 0x81A0;  /// R/W
    const uint32_t BufferOccupancyGain         = 0x81B4;  /// R/W
    const uint32_t ExtendedVetoDelay           = 0x81C4;  /// R/W
    const uint32_t ReadoutControl              = 0xEF00;  /// R/W
    const uint32_t ReadoutStatus_R             = 0xEF04;  /// R
    const uint32_t BoardID                     = 0xEF08;  /// R/W  /// Geo address on VME crate
    const uint32_t MCSTBaseAddressAndControl   = 0xEF0C;  /// R/W
    const uint32_t RelocationAddress           = 0xEF10;  /// R/W
    const uint32_t InterruptStatusID           = 0xEF14;  /// R/W
    const uint32_t InterruptEventNumber        = 0xEF18;  /// R/W
    const uint32_t MaxAggregatePerBlockTransfer= 0xEF1C;  /// R/W
    const uint32_t Scratch                     = 0xEF20;  /// R/W
    const uint32_t SoftwareReset_W             = 0xEF24;  ///   W
    const uint32_t SoftwareClear_W             = 0xEF28;  ///   W  
    const uint32_t ConfigurationReload_W       = 0xEF34;  ///   W
    const uint32_t ROMChecksum_R               = 0xF000;  /// R
    const uint32_t ROMChecksumByte2_R          = 0xF004;  /// R
    const uint32_t ROMChecksumByte1_R          = 0xF008;  /// R
    const uint32_t ROMChecksumByte0_R          = 0xF00C;  /// R
    const uint32_t ROMConstantByte2_R          = 0xF010;  /// R
    const uint32_t ROMConstantByte1_R          = 0xF014;  /// R
    const uint32_t ROMConstantByte0_R          = 0xF018;  /// R
    const uint32_t ROM_C_Code_R                = 0xF01C;  /// R
    const uint32_t ROM_R_Code_R                = 0xF020;  /// R
    const uint32_t ROM_IEEE_OUI_Byte2_R        = 0xF024;  /// R
    const uint32_t ROM_IEEE_OUI_Byte1_R        = 0xF028;  /// R
    const uint32_t ROM_IEEE_OUI_Byte0_R        = 0xF02C;  /// R
    const uint32_t ROM_BoardVersion_R          = 0xF030;  /// R
    const uint32_t ROM_BoardFromFactor_R       = 0xF034;  /// R
    const uint32_t ROM_BoardIDByte1_R          = 0xF038;  /// R
    const uint32_t ROM_BoardIDByte0_R          = 0xF03C;  /// R
    const uint32_t ROM_PCB_rev_Byte3_R         = 0xF040;  /// R
    const uint32_t ROM_PCB_rev_Byte2_R         = 0xF044;  /// R
    const uint32_t ROM_PCB_rev_Byte1_R         = 0xF048;  /// R
    const uint32_t ROM_PCB_rev_Byte0_R         = 0xF04C;  /// R
    const uint32_t ROM_FlashType_R             = 0xF050;  /// R
    const uint32_t ROM_BoardSerialNumByte1_R   = 0xF080;  /// R
    const uint32_t ROM_BoardSerialNumByte0_R   = 0xF084;  /// R
    const uint32_t ROM_VCXO_Type_R             = 0xF088;  /// R

    namespace PHA {
      const uint32_t DataFlush_W               = 0x103C; ///   W   not sure
      const uint32_t ChannelStopAcquisition    = 0x1040; /// R/W   not sure
      const uint32_t RCCR2SmoothingFactor      = 0x1054; /// R/W   Trigger Filter smoothing, triggerSmoothingFactor
      const uint32_t InputRiseTime             = 0x1058; /// R/W   OK
      const uint32_t TrapezoidRiseTime         = 0x105C; /// R/W   OK
      const uint32_t TrapezoidFlatTop          = 0x1060; /// R/W   OK
      const uint32_t PeakingTime               = 0x1064; /// R/W   OK
      const uint32_t DecayTime                 = 0x1068; /// R/W   OK
      const uint32_t TriggerThreshold          = 0x106C; /// R/W   OK
      const uint32_t RiseTimeValidationWindow  = 0x1070; /// R/W   OK
      const uint32_t TriggerHoldOffWidth       = 0x1074; /// R/W   OK
      const uint32_t PeakHoldOff               = 0x1078; /// R/W   OK
      const uint32_t ShapedTriggerWidth        = 0x1084; /// R/W   not sure
      const uint32_t DPPAlgorithmControl2_G    = 0x10A0; /// R/W   OK
      const uint32_t FineGain                  = 0x10C4; /// R/W   OK
    }

    namespace PSD  {
      const uint32_t CFDSetting                      = 0x103C; /// R/W
      const uint32_t ForcedDataFlush_W               = 0x1040; ///   W
      const uint32_t ChargeZeroSuppressionThreshold  = 0x1044; /// R/W
      const uint32_t ShortGateWidth                  = 0x1054; /// R/W
      const uint32_t LongGateWidth                   = 0x1058; /// R/W
      const uint32_t GateOffset                      = 0x105C; /// R/W
      const uint32_t TriggerThreshold                = 0x1060; /// R/W
      const uint32_t FixedBaseline                   = 0x1064; /// R/W
      const uint32_t TriggerLatency                  = 0x106C; /// R/W
      const uint32_t ShapedTriggerWidth              = 0x1070; /// R/W
      const uint32_t TriggerHoldOffWidth             = 0x1074; /// R/W
      const uint32_t ThresholdForPSDCut              = 0x1078; /// R/W
      const uint32_t PurGapThreshold                 = 0x107C; /// R/W
      const uint32_t DPPAlgorithmControl2_G          = 0x1084; /// R/W 
      const uint32_t EarlyBaselineFreeze             = 0x10D8; /// R/W
    }
  }
  
}
*********************************/

#endif
