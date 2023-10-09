#ifndef REGISTERADDRESS_H
#define REGISTERADDRESS_H

#include <vector>
#include <string>
#include <utility>

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

enum RW { ReadWrite, ReadONLY, WriteONLY};

class Reg{
  public:

    Reg(){
      name = "";
      address = 0;
      rwType = RW::ReadWrite;
      group = 0;
      maxBit = 0;
      partialStep = 0; //for time parameter, partial step * tick2ns = full step
      comboList.clear();
    }
    Reg(std::string name, uint32_t address, RW type = RW::ReadWrite, bool group = false, unsigned int max = 0, int pStep = 0){
      this->name = name;
      this->address = address;
      this->rwType = type;
      this->group = group;
      this->maxBit = max;
      this->partialStep = pStep;
      comboList.clear();
    };

    Reg(std::string name, uint32_t address, RW type = RW::ReadWrite,  bool group = false, std::vector<std::pair<std::string, unsigned int>> list = {}){
      this->name = name;
      this->address = address;
      this->rwType = type;
      this->group = group;
      this->maxBit = 0;
      this->partialStep = 0;
      this->comboList = list;
    }

    ~Reg(){};
    
    operator uint32_t () const {return this->address;}  /// this allows Reg kaka("kaka", 0x1234) uint32_t haha = kaka;
  
    std::string  GetName()         const {return name;}
    const char * GetNameChar()     const {return name.c_str();}
    uint32_t     GetAddress()      const {return address; }
    RW           GetRWType()       const {return rwType;}
    bool         IsCoupled()       const {return group;}
    unsigned int GetMaxBit()       const {return maxBit;} 
    int          GetPartialStep()  const {return partialStep;} /// step = partialStep * tick2ns, -1 : step = 1
    void         Print()           const ;

    std::vector<std::pair<std::string, unsigned int>> GetComboList() const {return comboList;}

    uint32_t  ActualAddress(int ch = -1){ //for QDC, ch is groupID
      if( address == 0x8180 ) return  (ch < 0 ? address : (address + 4*(ch/2))); // DPP::TriggerValidationMask_G
      if( address <  0x8000 ){
        if( group ) {
          if( ch < 0 ) return address + 0x7000;
          return address + ((ch % 2 == 0 ? ch : ch - 1) << 8) ;
        }
        return  (ch < 0 ? (address + 0x7000) : (address + (ch << 8)) );
      }
      if( address >= 0x8000 ) return  address;
      return 0;
    }

    unsigned short Index (unsigned short ch);
    uint32_t CalAddress(unsigned int index); /// output actual address, also write the registerAddress
    
    void SetName(std::string str) {this->name = str;}
  
  private:

    std::string name;
    uint32_t address; /// This is the table of register, the actual address should call ActualAddress();
    RW rwType; /// read/write = 0; read = 1; write = 2
    bool group;
    unsigned int maxBit ;
    int partialStep;
    std::vector<std::pair<std::string, unsigned int>> comboList;
};

inline void Reg::Print() const{
  printf("       Name: %s\n", name.c_str());
  printf(" Re.Address: 0x%04X\n", address);
  printf("       Type: %s\n", rwType == RW::ReadWrite ? "Read/Write" : (rwType == RW::ReadONLY ? "Read-Only" : "Write-Only") );
  printf("      Group: %s\n", group ? "True" : "False");
  printf(" Max Value : 0x%X = %d \n", maxBit, maxBit);
}

inline  unsigned short Reg::Index (unsigned short ch){ //for QDC, ch = group
  unsigned short index;
  if( address == 0x8180){ //DPP::TriggerValidationMask_G
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


const Reg EventReadOutBuffer("EventReadOutBuffer", 0x0000, RW::ReadONLY, false, {});  /// R

///========== Channel or Group
const Reg ChannelDummy32                ("ChannelDummy32"               , 0x1024, RW::ReadWrite, false, {});  /// R/W
const Reg InputDynamicRange             ("InputDynamicRange"            , 0x1028, RW::ReadWrite, false, {});  /// R/W
const Reg ChannelPulseWidth             ("ChannelPulseWidth"            , 0x1070, RW::ReadWrite, false, {});  /// R/W
const Reg ChannelTriggerThreshold       ("ChannelTriggerThreshold"      , 0x1080, RW::ReadWrite, false, {});  /// R/W
const Reg CoupleSelfTriggerLogic_G      ("CoupleSelfTriggerLogic_G"     , 0x1084, RW::ReadWrite,  true, {});  /// R/W 
const Reg ChannelStatus_R               ("ChannelStatus_R"              , 0x1088, RW::ReadONLY,  false, {});  /// R
const Reg AMCFirmwareRevision_R         ("AMCFirmwareRevision_R"        , 0x108C, RW::ReadONLY,  false, {});  /// R
const Reg ChannelDCOffset               ("ChannelDCOffset"              , 0x1098, RW::ReadWrite, false, {});  /// R/W
const Reg ChannelADCTemperature_R       ("ChannelADCTemperature_R"      , 0x10A8, RW::ReadONLY,  false, {});  /// R
const Reg ChannelSelfTriggerRateMeter_R ("ChannelSelfTriggerRateMeter_R", 0x10EC, RW::ReadONLY,  false, {});  /// R
  
///========== Board
const Reg BoardConfiguration           ("BoardConfiguration"           , 0x8000, RW::ReadWrite, false, {});  /// R/W
const Reg BufferOrganization           ("BufferOrganization"           , 0x800C, RW::ReadWrite, false, {});  /// R/W
const Reg CustomSize                   ("CustomSize"                   , 0x8020, RW::ReadWrite, false, {});  /// R/W
const Reg ADCCalibration_W             ("ADCCalibration_W"             , 0x809C, RW::WriteONLY, false, {});  ///   W
const Reg AcquisitionControl           ("AcquisitionControl"           , 0x8100, RW::ReadWrite, false, {});  /// R/W
const Reg AcquisitionStatus_R          ("AcquisitionStatus_R"          , 0x8104, RW::ReadONLY , false, {});  /// R
const Reg SoftwareTrigger_W            ("SoftwareTrigger_W"            , 0x8108, RW::WriteONLY, false, {});  ///   W
const Reg GlobalTriggerMask            ("GlobalTriggerMask"            , 0x810C, RW::ReadWrite, false, {});  /// R/W
const Reg FrontPanelTRGOUTEnableMask   ("FrontPanelTRGOUTEnableMask"   , 0x8110, RW::ReadWrite, false, {});  /// R/W
const Reg PostTrigger                  ("PostTrigger"                  , 0x8114, RW::ReadWrite, false, {});  /// R/W
const Reg LVDSIOData                   ("LVDSIOData"                   , 0x8118, RW::ReadWrite, false, {});  /// R/W
const Reg FrontPanelIOControl          ("FrontPanelIOControl"          , 0x811C, RW::ReadWrite, false, {});  /// R/W
const Reg RegChannelEnableMask            ("RegChannelEnableMask"            , 0x8120, RW::ReadWrite, false, {});  /// R/W
const Reg ROCFPGAFirmwareRevision_R    ("ROCFPGAFirmwareRevision_R"    , 0x8124, RW::ReadONLY , false, {});  /// R
const Reg EventStored_R                ("EventStored_R"                , 0x812C, RW::ReadONLY , false, {});  /// R
const Reg VoltageLevelModeConfig       ("VoltageLevelModeConfig"       , 0x8138, RW::ReadWrite, false, {});  /// R/W
const Reg SoftwareClockSync_W          ("SoftwareClockSync_W"          , 0x813C, RW::WriteONLY, false, {});  ///   W 
const Reg BoardInfo_R                  ("BoardInfo_R"                  , 0x8140, RW::ReadONLY , false, {});  /// R
const Reg AnalogMonitorMode            ("AnalogMonitorMode"            , 0x8144, RW::ReadWrite, false, {});  /// R/W
const Reg EventSize_R                  ("EventSize_R"                  , 0x814C, RW::ReadONLY , false, {});  /// R
const Reg FanSpeedControl              ("FanSpeedControl"              , 0x8168, RW::ReadWrite, false, {});  /// R/W
const Reg MemoryBufferAlmostFullLevel  ("MemoryBufferAlmostFullLevel"  , 0x816C, RW::ReadWrite, false, {});  /// R/W
const Reg RunStartStopDelay            ("RunStartStopDelay"            , 0x8170, RW::ReadWrite, false, {});  /// R/W
const Reg BoardFailureStatus_R         ("BoardFailureStatus_R"         , 0x8178, RW::ReadONLY , false, {});  /// R
const Reg FrontPanelLVDSIONewFeatures  ("FrontPanelLVDSIONewFeatures"  , 0x81A0, RW::ReadWrite, false, {});  /// R/W
const Reg BufferOccupancyGain          ("BufferOccupancyGain"          , 0x81B4, RW::ReadWrite, false, {});  /// R/W
const Reg ChannelsShutdown_W           ("ChannelsShutdown_W"           , 0x81C0, RW::WriteONLY, false, {});  ///   W 
const Reg ExtendedVetoDelay            ("ExtendedVetoDelay"            , 0x81C4, RW::ReadWrite, false, {});  /// R/W
const Reg ReadoutControl               ("ReadoutControl"               , 0xEF00, RW::ReadWrite, false, {});  /// R/W
const Reg ReadoutStatus_R              ("ReadoutStatus_R"              , 0xEF04, RW::ReadONLY , false, {});  /// R
const Reg BoardID                      ("BoardID"                      , 0xEF08, RW::ReadWrite, false, {});  /// R/W
const Reg MCSTBaseAddressAndControl    ("MCSTBaseAddressAndControl"    , 0xEF0C, RW::ReadWrite, false, {});  /// R/W
const Reg RelocationAddress            ("RelocationAddress"            , 0xEF10, RW::ReadWrite, false, {});  /// R/W
const Reg InterruptStatusID            ("InterruptStatusID"            , 0xEF14, RW::ReadWrite, false, {});  /// R/W
const Reg InterruptEventNumber         ("InterruptEventNumber"         , 0xEF18, RW::ReadWrite, false, {});  /// R/W
const Reg MaxAggregatePerBlockTransfer ("MaxAggregatePerBlockTransfer" , 0xEF1C, RW::ReadWrite, false, {});  /// R/W
const Reg Scratch                      ("Scratch"                      , 0xEF20, RW::ReadWrite, false, {});  /// R/W
const Reg SoftwareReset_W              ("SoftwareReset_W"              , 0xEF24, RW::WriteONLY, false, {});  ///   W
const Reg SoftwareClear_W              ("SoftwareClear_W"              , 0xEF28, RW::WriteONLY, false, {});  ///   W  


///====== Common for PHA and PSD
namespace DPP {

  namespace Bit_BoardConfig{

    /// -------------------- shared with PHA, PSD, and QDC
    const std::pair<unsigned short, unsigned short> AnalogProbe1 = {2, 12} ; 
    const std::pair<unsigned short, unsigned short> RecordTrace = {1, 16} ; 

    /// -------------------- shared with PHA and PSD
    const std::pair<unsigned short, unsigned short> EnableAutoDataFlush = {1, 0} ; /// length, smallest pos
    const std::pair<unsigned short, unsigned short> TrigPropagation = {1, 2} ; 
    const std::pair<unsigned short, unsigned short> DualTrace = {1, 11} ; 
    const std::pair<unsigned short, unsigned short> EnableExtra2 = {1, 17} ; 

    /// -------------------- PHA only
    const std::pair<unsigned short, unsigned short> DecimateTrace = {1, 1} ; 
    const std::pair<unsigned short, unsigned short> AnalogProbe2 = {2, 14} ; 
    const std::pair<unsigned short, unsigned short> DigiProbel1_PHA = {4, 20} ; 
    const std::pair<unsigned short, unsigned short> DigiProbel2_PHA = {3, 26} ; 

    const std::vector<std::pair<std::string, unsigned int>> ListAnaProbe1_PHA = {{"Input", 0},
                                                                              {"RC-CR", 1},
                                                                              {"RC-CR2", 2},
                                                                              {"Trapezoid", 3}};
    const std::vector<std::pair<std::string, unsigned int>> ListAnaProbe2_PHA = {{"Input", 0},
                                                                  {"Threshold", 1},
                                                                  {"Trap. - Baseline", 2},
                                                                  {"Trap. Baseline", 3}};

    const std::vector<std::pair<std::string, unsigned int>> ListDigiProbe1_PHA = {{"Peaking", 0},
                                                                    {"Armed", 1},
                                                                    {"Peak Run", 2},
                                                                    {"Pile Up", 3},
                                                                    {"peaking", 4},
                                                                    {"TRG Valid. Win", 5},
                                                                    {"Baseline Freeze", 6},
                                                                    {"TRG Holdoff", 7},
                                                                    {"TRG Valid.", 8},
                                                                    {"ACQ Busy", 9},
                                                                    {"Zero Cross", 10},
                                                                    {"Ext. TRG", 11},
                                                                    {"Budy", 12}};

    const std::vector<std::pair<std::string, unsigned int>> ListDigiProbe2_PHA = {{"Trigger", 0}};

    ///------------------------ PSD only
    const std::pair<unsigned short, unsigned short> DigiProbel1_PSD = {3, 23} ; 
    const std::pair<unsigned short, unsigned short> DigiProbel2_PSD = {3, 26} ; 
    const std::pair<unsigned short, unsigned short> DisableDigiTrace_PSD = {1, 31} ; 
    const std::vector<std::pair<std::string, unsigned int>> ListAnaProbe_PSD = {{"Input + N/A", 0},
                                                                                {"CFD + N/A", 2},
                                                                                {"Input + Baseline",1},
                                                                                {"CFD + Baseline", 3},
                                                                                {"Input + CFD", 5}};

    const std::vector<std::pair<std::string, unsigned int>> ListDigiProbe1_PSD = {{"Long gate", 0},
                                                                                  {"Over Threshold", 1},
                                                                                  {"Shaped TRG", 2},
                                                                                  {"TRG Val. Accp. Win", 3},
                                                                                  {"Pile Up", 4},
                                                                                  {"Coincidence", 5},
                                                                                  {"Trigger", 7}};

    const std::vector<std::pair<std::string, unsigned int>> ListDigiProbe2_PSD = {{"Short Gate", 0},
                                                                                  {"Over Threshold", 1},
                                                                                  {"TRG Valid.", 2},
                                                                                  {"TRG Holdoff", 3},
                                                                                  {"Pile Up Trigger", 4},
                                                                                  {"PSD cut high", 5},
                                                                                  {"Baseline Freeze", 6},
                                                                                  {"Trigger", 7}};

    /// -------------------- QDC only
    const std::pair<unsigned short, unsigned short> ExtTriggerMode_QDC = {2, 20} ; 

    const std::vector<std::pair<std::string, unsigned int>> ListExtTriggerMode_QDC = {{"Trigger", 0},
                                                                                      {"Veto", 1},
                                                                                      {"Anti-Veto", 2}};

    const std::vector<std::pair<std::string, unsigned int>> ListAnaProbe_QDC = {{"Input", 0},
                                                                                {"Smoothed Input", 1},
                                                                                {"Baseline", 2}};
  }
  
  namespace Bit_DPPAlgorithmControl_PHA {
    const std::pair<unsigned short, unsigned short> TrapRescaling = {6, 0} ; /// length, smallest pos
    const std::pair<unsigned short, unsigned short> TraceDecimation = {2, 8};
    const std::pair<unsigned short, unsigned short> TraceDeciGain = {2, 10,};
    const std::pair<unsigned short, unsigned short> PeakMean = {2, 12};
    const std::pair<unsigned short, unsigned short> Polarity = {1, 16};
    const std::pair<unsigned short, unsigned short> TriggerMode = {2, 18};
    const std::pair<unsigned short, unsigned short> BaselineAvg = {3, 20};
    const std::pair<unsigned short, unsigned short> DisableSelfTrigger = {1, 24};
    const std::pair<unsigned short, unsigned short> EnableRollOverFlag = {1, 26};
    const std::pair<unsigned short, unsigned short> EnablePileUpFlag = {1, 27};

    const std::vector<std::pair<std::string, unsigned int>> ListTraceDecimation = {{"Disabled", 0},
                                                                                    {"2 samples", 1},
                                                                                    {"4 samples", 2},
                                                                                    {"8 samples", 3}};

    const std::vector<std::pair<std::string, unsigned int>> ListDecimationGain = {{"x1", 0},
                                                                                  {"x2", 1},
                                                                                  {"x4", 2},
                                                                                  {"x8", 3}};

    const std::vector<std::pair<std::string, unsigned int>> ListPeakMean = {{"1 sample", 0},
                                                                            {"4 sample", 1},
                                                                            {"16 sample", 2},
                                                                            {"64 sample", 3}};

    const std::vector<std::pair<std::string, unsigned int>> ListPolarity = {{"Positive", 0},
                                                                            {"Negative", 1}};

    const std::vector<std::pair<std::string, unsigned int>> ListTrigMode = {{"Independent", 0},
                                                                            {"Coincident", 1},
                                                                            {"Anti-Coincident", 3}};

    const std::vector<std::pair<std::string, unsigned int>> ListBaselineAvg = {{"Not Used", 0},
                                                                               {"16 samples", 1},
                                                                               {"64 samples", 2},
                                                                               {"256 samples", 3},
                                                                               {"1024 samples", 4},
                                                                               {"4096 samples", 5},
                                                                               {"16384 samples", 6}};
  }

  namespace Bit_DPPAlgorithmControl_PSD {
    const std::pair<unsigned short, unsigned short> ChargeSensitivity = {3, 0} ; /// length, smallest pos
    const std::pair<unsigned short, unsigned short> ChargePedestal = {1, 4};
    const std::pair<unsigned short, unsigned short> TriggerCountOpt = {1, 5,};
    const std::pair<unsigned short, unsigned short> DiscriminationMode = {1, 6};
    const std::pair<unsigned short, unsigned short> PileupWithinGate = {1, 7};
    const std::pair<unsigned short, unsigned short> InternalTestPulse = {1, 8};
    const std::pair<unsigned short, unsigned short> TestPulseRate = {2, 9};
    const std::pair<unsigned short, unsigned short> BaselineCal = {1, 15};
    const std::pair<unsigned short, unsigned short> Polarity = {1, 16};
    const std::pair<unsigned short, unsigned short> TriggerMode = {2, 18};
    const std::pair<unsigned short, unsigned short> BaselineAvg = {3, 20};
    const std::pair<unsigned short, unsigned short> DisableSelfTrigger = {1, 24};
    const std::pair<unsigned short, unsigned short> DiscardQLongSmallerQThreshold = {1, 25};
    const std::pair<unsigned short, unsigned short> RejectPileup = {1, 26};
    const std::pair<unsigned short, unsigned short> EnablePSDCutBelow = {1, 27};
    const std::pair<unsigned short, unsigned short> EnablePSDCutAbove = {1, 28};
    const std::pair<unsigned short, unsigned short> RejectOverRange = {1, 29};
    const std::pair<unsigned short, unsigned short> DisableTriggerHysteresis = {1, 30};
    const std::pair<unsigned short, unsigned short> DisableOppositePolarityInhibitZeroCrossingOnCFD = {1, 31};

    const std::vector<std::pair<std::string, unsigned int>> ListChargeSensitivity_2Vpp = {{"5 fC", 0},
                                                                                     {"20 fC", 1},
                                                                                     {"80 fC", 2},
                                                                                     {"320 fC", 3},
                                                                                     {"1.28 pC", 4},
                                                                                     {"5.12 pC", 5}};

    const std::vector<std::pair<std::string, unsigned int>> ListChargeSensitivity_p5Vpp = {{"1.25 fC", 0},
                                                                                           {"5 fC", 1},
                                                                                           {"20 fC", 2},
                                                                                           {"80 fC", 3},
                                                                                           {"320 fC", 4},
                                                                                           {"1.28 pC", 5}};

    const std::vector<std::pair<std::string, unsigned int>> ListTestPulseRate_730 = {{"1 kHz", 0},
                                                                                     {"10 kHz", 1},
                                                                                     {"100 kHz", 2},
                                                                                     {"1 MHz", 3}};

    const std::vector<std::pair<std::string, unsigned int>> ListTestPulseRate_725 = {{"500 Hz", 0},
                                                                                     {"5 kHz", 1},
                                                                                     {"500 kHz", 2},
                                                                                     {"500 kHz", 3}};

    const std::vector<std::pair<std::string, unsigned int>> ListTriggerCountOpt = {{"Only accepted self-Trigger", 0},
                                                                                   {"All self-trigger", 1}};

    const std::vector<std::pair<std::string, unsigned int>> ListDiscriminationMode = {{"Leading Edge", 0},
                                                                                      {"Digital Const. Frac.", 1}};

    const std::vector<std::pair<std::string, unsigned int>> ListPolarity = {{"Positive", 0},
                                                                            {"Negative", 1}};

    const std::vector<std::pair<std::string, unsigned int>> ListTrigMode = {{"Independent", 0},
                                                                            {"Coincident ", 1},
                                                                            {"Anti-Coincident", 3}};

    const std::vector<std::pair<std::string, unsigned int>> ListBaselineAvg = {{"Fixed", 0},
                                                                               {"16 samples", 1},
                                                                               {"64 samples", 2},
                                                                               {"256 samples", 3},
                                                                               {"1024 samples", 4}};
  }

  namespace Bit_DPPAlgorithmControl_QDC {

  }

  namespace Bit_AcquistionControl {
    const std::pair<unsigned short, unsigned short> StartStopMode = {2, 0} ;
    const std::pair<unsigned short, unsigned short> ACQStartArm = {1, 2} ;
    const std::pair<unsigned short, unsigned short> TrigCountMode_QDC = {1, 3} ;
    const std::pair<unsigned short, unsigned short> PLLRef = {1, 6} ;
    const std::pair<unsigned short, unsigned short> LVDSBusyEnable = {1, 8} ;
    const std::pair<unsigned short, unsigned short> LVDSVetoEnable = {1, 9} ;
    const std::pair<unsigned short, unsigned short> LVDSRunInMode = {1, 11} ;
    const std::pair<unsigned short, unsigned short> VetoTRGOut = {1, 12} ;

    const std::vector<std::pair<std::string, unsigned int>> ListStartStopMode = {{"SW controlled", 0},
                                                                                 {"S-IN/GPI controlled", 1},
                                                                                 {"1st TRG-IN", 2},
                                                                                 {"LVDS controlled", 3}};

    const std::vector<std::pair<std::string, unsigned int>> ListACQStartArm = {{"ACQ STOP", 0},
                                                                               {"ACQ RUN", 1}};

    const std::vector<std::pair<std::string, unsigned int>> ListPLLRef = {{"Internal 50 MHz", 0},
                                                                          {"Ext. CLK-IN", 1}};

    const std::vector<std::pair<std::string, unsigned int>> ListTrigCountMode_QDC = {{"Comb. ch", 0},
                                                                                     {"Comb. ch + TRG-IN + SW", 1}};
  }

  namespace Bit_AcqStatus {
    const std::pair<unsigned short, unsigned short> AcqStatus = {1, 2} ;
    const std::pair<unsigned short, unsigned short> EventReady = {1, 3} ;
    const std::pair<unsigned short, unsigned short> EventFull = {1, 4} ;
    const std::pair<unsigned short, unsigned short> ClockSource = {1, 5} ;
    const std::pair<unsigned short, unsigned short> PLLLock = {1, 7} ;
    const std::pair<unsigned short, unsigned short> BoardReady = {1, 8} ;
    const std::pair<unsigned short, unsigned short> SINStatus = {1, 15} ;
    const std::pair<unsigned short, unsigned short> TRGINStatus = {1, 16} ;
    const std::pair<unsigned short, unsigned short> ChannelsDown = {1, 19} ;
  }

  namespace Bit_ReadoutControl {
    const std::pair<unsigned short, unsigned short> VMEInterruptLevel = {3, 0} ;
    const std::pair<unsigned short, unsigned short> EnableOpticalLinkInpt = {1, 3} ;
    const std::pair<unsigned short, unsigned short> EnableEventAligned = {1, 4} ;
    const std::pair<unsigned short, unsigned short> VMEAlign64Mode = {1, 5} ;
    const std::pair<unsigned short, unsigned short> VMEBaseAddressReclocated = {1, 6} ;
    const std::pair<unsigned short, unsigned short> InterrupReleaseMode = {1, 7} ;
    const std::pair<unsigned short, unsigned short> EnableExtendedBlockTransfer = {1, 8} ;
  }

  namespace Bit_GlobalTriggerMask {
    const std::pair<unsigned short, unsigned short> GroupedChMask = {8, 0} ;
    const std::pair<unsigned short, unsigned short> MajorCoinWin = {4, 20} ;
    const std::pair<unsigned short, unsigned short> MajorLevel = {2, 24} ;
    const std::pair<unsigned short, unsigned short> LVDSTrigger = {1, 29} ;
    const std::pair<unsigned short, unsigned short> ExtTrigger = {1, 30} ;
    const std::pair<unsigned short, unsigned short> SWTrigger = {1, 31} ;
  }

  namespace Bit_TRGOUTMask{
    const std::pair<unsigned short, unsigned short> GroupedChMask = {8, 0} ;
    const std::pair<unsigned short, unsigned short> TRGOUTLogic = {2, 8} ;
    const std::pair<unsigned short, unsigned short> MajorLevel = {3, 10} ;
    const std::pair<unsigned short, unsigned short> LVDSTrigger = {1, 29} ;
    const std::pair<unsigned short, unsigned short> ExtTrigger = {1, 30} ;
    const std::pair<unsigned short, unsigned short> SWTrigger = {1, 31} ;
  }

  namespace Bit_FrontPanelIOControl {
    const std::pair<unsigned short, unsigned short> LEMOLevel = {1, 0} ;
    const std::pair<unsigned short, unsigned short> DisableTrgOut = {1, 1} ;
    const std::pair<unsigned short, unsigned short> LVDSDirection1 = {1, 2} ; // [3:0]
    const std::pair<unsigned short, unsigned short> LVDSDirection2 = {1, 3} ; // [7:4]
    const std::pair<unsigned short, unsigned short> LVDSDirection3 = {1, 4} ; // [11:8]
    const std::pair<unsigned short, unsigned short> LVDSDirection4 = {1, 5} ; // [15:12]
    const std::pair<unsigned short, unsigned short> LVDSConfiguration = {2, 6};
    const std::pair<unsigned short, unsigned short> LVDSNewFeature = {1, 8};
    const std::pair<unsigned short, unsigned short> LVDSLatchMode = {1, 9};
    const std::pair<unsigned short, unsigned short> TRGINMode = {1, 10};
    const std::pair<unsigned short, unsigned short> TRGINMezzanine = {1, 11};
    const std::pair<unsigned short, unsigned short> TRGOUTConfig = {6, 14};
    const std::pair<unsigned short, unsigned short> PatternConfig = {2, 21};

    const std::vector<std::pair<std::string, unsigned int>> ListLEMOLevel = {{"NIM I/O", 0},
                                                                              {"TTL I/O", 1}};
    const std::vector<std::pair<std::string, unsigned int>> ListTRGIMode = {{"Edge of TRG-IN", 0},
                                                                            {"Whole duration of TR-IN", 1}};
    const std::vector<std::pair<std::string, unsigned int>> ListTRGIMezzanine = {{"Pocessed by Motherboard", 0},
                                                                                  {"Skip Motherboard", 1}};

    const std::vector<std::pair<std::string, unsigned int>> ListTRGOUTConfig = {{"Disable",            0x00002}, /// this is TRG_OUT high  imped. 0x811C bit[1]
                                                                                {"force TRG-OUT is 0", 0x08000},
                                                                                {"force TRG-OUT is 1", 0x0C000},
                                                                                {"Trigger (Mask)",     0x00000},
                                                                                {"Channel Probe",      0x20000},
                                                                                {"S-IN",               0x30000},
                                                                                {"RUN",                0x10000},
                                                                                {"Sync Clock",         0x50000},
                                                                                {"Clock Phase",        0x90000},
                                                                                {"BUSY/UNLOCK",        0xD0000}};
  }

  namespace Bit_VetoWidth {
    const std::pair<unsigned short, unsigned short> VetoWidth = {16, 0} ;
    const std::pair<unsigned short, unsigned short> VetoStep = {2, 16} ;

    const std::vector<std::pair<std::string, unsigned int>> ListVetoStep = {{"  16 ns (725),   8 ns (730)", 0},
                                                                            {"   4 ns (725),   2 ns (730)", 1},
                                                                            {"1048 ns (725), 524 ns (730)", 2},
                                                                            {" 264 ns (725), 134 ns (730)", 3}};

  }

  const Reg RecordLength_G              ("RecordLength_G"              , 0x1020, RW::ReadWrite,  true, 0x3FFF,  8); /// R/W
  const Reg InputDynamicRange           ("InputDynamicRange"           , 0x1028, RW::ReadWrite, false, {{"2 Vpp", 0},{"0.5 Vpp", 1}}); /// R/W
  const Reg NumberEventsPerAggregate_G  ("NumberEventsPerAggregate_G"  , 0x1034, RW::ReadWrite,  true,  0x3FF, -1); /// R/W
  const Reg PreTrigger                  ("PreTrigger"                  , 0x1038, RW::ReadWrite, false,   0xFF,  4); /// R/W
  const Reg TriggerThreshold            ("TriggerThreshold"            , 0x106C, RW::ReadWrite, false, 0x3FFF, -1); /// R/W
  const Reg TriggerHoldOffWidth         ("TriggerHoldOffWidth"         , 0x1074, RW::ReadWrite, false,  0x3FF,  4); /// R/W
  const Reg DPPAlgorithmControl         ("DPPAlgorithmControl"         , 0x1080, RW::ReadWrite, false, {}); /// R/W 
  const Reg ChannelStatus_R             ("ChannelStatus_R"             , 0x1088, RW::ReadONLY , false, {}); /// R
  const Reg AMCFirmwareRevision_R       ("AMCFirmwareRevision_R"       , 0x108C, RW::ReadONLY , false, {}); /// R
  const Reg ChannelDCOffset             ("ChannelDCOffset"             , 0x1098, RW::ReadWrite, false, 0xFFFF, -1); /// R/W
  const Reg ChannelADCTemperature_R     ("ChannelADCTemperature_R"     , 0x10A8, RW::ReadONLY , false, {}); /// R
  const Reg IndividualSoftwareTrigger_W ("IndividualSoftwareTrigger_W" , 0x10C0, RW::WriteONLY, false, {}); ///   W
  const Reg VetoWidth                   ("VetoWidth"                   , 0x10D4, RW::ReadWrite, false, {}); /// R/W
  
  /// I know there are many duplication, it is the design.
  const Reg BoardConfiguration          ("BoardConfiguration"          , 0x8000, RW::ReadWrite, false, {});  /// R/W
  const Reg AggregateOrganization       ("AggregateOrganization"       , 0x800C, RW::ReadWrite, false, {{"Not use", 0x0},
                                                                                                        {"Not use", 0x1},
                                                                                                        {   "4", 0x2},
                                                                                                        {   "8", 0x3},
                                                                                                        {  "16", 0x4},
                                                                                                        {  "32", 0x5},
                                                                                                        {  "64", 0x6},
                                                                                                        { "128", 0x7},
                                                                                                        { "256", 0x8},
                                                                                                        { "512", 0x9},
                                                                                                        {"1024", 0xA}});  /// R/W
  const Reg ADCCalibration_W            ("ADCCalibration_W"            , 0x809C, RW::WriteONLY, false, {});  ///   W
  const Reg ChannelShutdown_W           ("ChannelShutdown_W"           , 0x80BC, RW::WriteONLY, false, {{"no shutdown", 0},{"shutdown", 1}});  ///   W
  const Reg AcquisitionControl          ("AcquisitionControl"          , 0x8100, RW::ReadWrite, false, {});  /// R/W
  const Reg AcquisitionStatus_R         ("AcquisitionStatus_R"         , 0x8104, RW::ReadONLY , false, {});  /// R
  const Reg SoftwareTrigger_W           ("SoftwareTrigger_W"           , 0x8108, RW::WriteONLY, false, {});  ///   W
  const Reg GlobalTriggerMask           ("GlobalTriggerMask"           , 0x810C, RW::ReadWrite, false, {});  /// R/W
  const Reg FrontPanelTRGOUTEnableMask  ("FrontPanelTRGOUTEnableMask"  , 0x8110, RW::ReadWrite, false, {});  /// R/W
  const Reg LVDSIOData                  ("LVDSIOData"                  , 0x8118, RW::ReadWrite, false, {});  /// R/W
  const Reg FrontPanelIOControl         ("FrontPanelIOControl"         , 0x811C, RW::ReadWrite, false, {});  /// R/W
  const Reg RegChannelEnableMask           ("RegChannelEnableMask"           , 0x8120, RW::ReadWrite, false, {});  /// R/W
  const Reg ROCFPGAFirmwareRevision_R   ("ROCFPGAFirmwareRevision_R"   , 0x8124, RW::ReadONLY , false, {});  /// R
  const Reg EventStored_R               ("EventStored_R"               , 0x812C, RW::ReadONLY , false, {});  /// R
  const Reg VoltageLevelModeConfig      ("VoltageLevelModeConfig"      , 0x8138, RW::ReadWrite, false, {});  /// R/W
  const Reg SoftwareClockSync_W         ("SoftwareClockSync_W"         , 0x813C, RW::WriteONLY, false, {});  ///   W
  const Reg BoardInfo_R                 ("BoardInfo_R"                 , 0x8140, RW::ReadONLY , false, {});  /// R    
  const Reg AnalogMonitorMode           ("AnalogMonitorMode"           , 0x8144, RW::ReadWrite, false, {{"Trig. Maj. Mode", 0},
                                                                                                        {"Test mode", 1},
                                                                                                        {"Buffer occp. Mode", 3},
                                                                                                        {"Voltage Lvl Mode", 4}});  /// R/W
  const Reg EventSize_R                 ("EventSize_R"                 , 0x814C, RW::ReadONLY , false, {});  /// R
  const Reg TimeBombDowncounter_R       ("TimeBombDowncounter_R"       , 0x8158, RW::ReadONLY , false, {});  /// R
  const Reg FanSpeedControl             ("FanSpeedControl"             , 0x8168, RW::ReadWrite, false, {});  /// R/W
  const Reg RunStartStopDelay           ("RunStartStopDelay"           , 0x8170, RW::ReadWrite, false, 0xFF, 8);  /// R/W
  const Reg BoardFailureStatus_R        ("BoardFailureStatus_R"        , 0x8178, RW::ReadONLY , false, {});  /// R
  const Reg DisableExternalTrigger      ("DisableExternalTrigger"      , 0x817C, RW::ReadWrite, false, {});  /// R/W
  const Reg FrontPanelLVDSIONewFeatures ("FrontPanelLVDSIONewFeatures" , 0x81A0, RW::ReadWrite, false, {});  /// R/W
  const Reg BufferOccupancyGain         ("BufferOccupancyGain"         , 0x81B4, RW::ReadWrite, false, 0xF, -1);  /// R/W
  const Reg ExtendedVetoDelay           ("ExtendedVetoDelay"           , 0x81C4, RW::ReadWrite, false, 0xFFFF, 4);  /// R/W
  const Reg ReadoutControl              ("ReadoutControl"              , 0xEF00, RW::ReadWrite, false, {});  /// R/W
  const Reg ReadoutStatus_R             ("ReadoutStatus_R"             , 0xEF04, RW::ReadONLY , false, {});  /// R
  const Reg BoardID                     ("BoardID"                     , 0xEF08, RW::ReadWrite, false, {});  /// R/W 
  const Reg MCSTBaseAddressAndControl   ("MCSTBaseAddressAndControl"   , 0xEF0C, RW::ReadWrite, false, {});  /// R/W
  const Reg RelocationAddress           ("RelocationAddress"           , 0xEF10, RW::ReadWrite, false, {});  /// R/W
  const Reg InterruptStatusID           ("InterruptStatusID"           , 0xEF14, RW::ReadWrite, false, {});  /// R/W
  const Reg InterruptEventNumber        ("InterruptEventNumber"        , 0xEF18, RW::ReadWrite, false, {});  /// R/W
  const Reg MaxAggregatePerBlockTransfer("MaxAggregatePerBlockTransfer", 0xEF1C, RW::ReadWrite, false, 0x3FF, -1);  /// R/W
  const Reg Scratch                     ("Scratch"                     , 0xEF20, RW::ReadWrite, false, {});  /// R/W
  const Reg SoftwareReset_W             ("SoftwareReset_W"             , 0xEF24, RW::WriteONLY, false, {});  ///   W
  const Reg SoftwareClear_W             ("SoftwareClear_W"             , 0xEF28, RW::WriteONLY, false, {});  ///   W  
  const Reg ConfigurationReload_W       ("ConfigurationReload_W"       , 0xEF34, RW::WriteONLY, false, {});  ///   W
  const Reg ROMChecksum_R               ("ROMChecksum_R"               , 0xF000, RW::ReadONLY , false, {});  /// R
  const Reg ROMChecksumByte2_R          ("ROMChecksumByte2_R"          , 0xF004, RW::ReadONLY , false, {});  /// R
  const Reg ROMChecksumByte1_R          ("ROMChecksumByte1_R"          , 0xF008, RW::ReadONLY , false, {});  /// R
  const Reg ROMChecksumByte0_R          ("ROMChecksumByte0_R"          , 0xF00C, RW::ReadONLY , false, {});  /// R
  const Reg ROMConstantByte2_R          ("ROMConstantByte2_R"          , 0xF010, RW::ReadONLY , false, {});  /// R
  const Reg ROMConstantByte1_R          ("ROMConstantByte1_R"          , 0xF014, RW::ReadONLY , false, {});  /// R
  const Reg ROMConstantByte0_R          ("ROMConstantByte0_R"          , 0xF018, RW::ReadONLY , false, {});  /// R
  const Reg ROM_C_Code_R                ("ROM_C_Code_R"                , 0xF01C, RW::ReadONLY , false, {});  /// R
  const Reg ROM_R_Code_R                ("ROM_R_Code_R"                , 0xF020, RW::ReadONLY , false, {});  /// R
  const Reg ROM_IEEE_OUI_Byte2_R        ("ROM_IEEE_OUI_Byte2_R"        , 0xF024, RW::ReadONLY , false, {});  /// R
  const Reg ROM_IEEE_OUI_Byte1_R        ("ROM_IEEE_OUI_Byte1_R"        , 0xF028, RW::ReadONLY , false, {});  /// R
  const Reg ROM_IEEE_OUI_Byte0_R        ("ROM_IEEE_OUI_Byte0_R"        , 0xF02C, RW::ReadONLY , false, {});  /// R
  const Reg ROM_BoardVersion_R          ("ROM_BoardVersion_R"          , 0xF030, RW::ReadONLY , false, {});  /// R
  const Reg ROM_BoardFromFactor_R       ("ROM_BoardFromFactor_R"       , 0xF034, RW::ReadONLY , false, {});  /// R
  const Reg ROM_BoardIDByte1_R          ("ROM_BoardIDByte1_R"          , 0xF038, RW::ReadONLY , false, {});  /// R
  const Reg ROM_BoardIDByte0_R          ("ROM_BoardIDByte0_R"          , 0xF03C, RW::ReadONLY , false, {});  /// R
  const Reg ROM_PCB_rev_Byte3_R         ("ROM_PCB_rev_Byte3_R"         , 0xF040, RW::ReadONLY , false, {});  /// R
  const Reg ROM_PCB_rev_Byte2_R         ("ROM_PCB_rev_Byte2_R"         , 0xF044, RW::ReadONLY , false, {});  /// R
  const Reg ROM_PCB_rev_Byte1_R         ("ROM_PCB_rev_Byte1_R"         , 0xF048, RW::ReadONLY , false, {});  /// R
  const Reg ROM_PCB_rev_Byte0_R         ("ROM_PCB_rev_Byte0_R"         , 0xF04C, RW::ReadONLY , false, {});  /// R
  const Reg ROM_FlashType_R             ("ROM_FlashType_R"             , 0xF050, RW::ReadONLY , false, {});  /// R
  const Reg ROM_BoardSerialNumByte1_R   ("ROM_BoardSerialNumByte1_R"   , 0xF080, RW::ReadONLY , false, {});  /// R
  const Reg ROM_BoardSerialNumByte0_R   ("ROM_BoardSerialNumByte0_R"   , 0xF084, RW::ReadONLY , false, {});  /// R
  const Reg ROM_VCXO_Type_R             ("ROM_VCXO_Type_R"             , 0xF088, RW::ReadONLY , false, {});  /// R

  const Reg TriggerValidationMask_G     ("TriggerValidationMask_G"     , 0x8180, RW::ReadWrite, true, {});  /// R/W,  
  
  namespace PHA {
    const Reg DataFlush_W               ("DataFlush_W"              , 0x103C, RW::WriteONLY, false, {}); ///   W   not sure
    const Reg ChannelStopAcquisition    ("ChannelStopAcquisition"   , 0x1040, RW::ReadWrite, false, {{"Run", 0}, {"Stop", 1}}); /// R/W   not sure
    const Reg RCCR2SmoothingFactor      ("RCCR2SmoothingFactor"     , 0x1054, RW::ReadWrite, false, {{  "Disabled", 0x0},
                                                                                                      {  "2 sample", 0x1},
                                                                                                      {  "4 sample", 0x2},
                                                                                                      {  "8 sample", 0x4},
                                                                                                      { "16 sample", 0x8},
                                                                                                      { "32 sample", 0x10},
                                                                                                      { "64 sample", 0x20},
                                                                                                      {"128 sample", 0x3F}
                                                                                                    }); /// R/W   Trigger Filter smoothing, triggerSmoothingFactor
    const Reg InputRiseTime             ("InputRiseTime"            , 0x1058, RW::ReadWrite, false,   0xFF,  4); /// R/W   OK
    const Reg TrapezoidRiseTime         ("TrapezoidRiseTime"        , 0x105C, RW::ReadWrite, false,  0xFFF,  4); /// R/W   OK
    const Reg TrapezoidFlatTop          ("TrapezoidFlatTop"         , 0x1060, RW::ReadWrite, false,  0xFFF,  4); /// R/W   OK
    const Reg PeakingTime               ("PeakingTime"              , 0x1064, RW::ReadWrite, false,  0xFFF,  4); /// R/W   OK
    const Reg DecayTime                 ("DecayTime"                , 0x1068, RW::ReadWrite, false, 0xFFFF,  4); /// R/W   OK
    const Reg TriggerThreshold          ("TriggerThreshold"         , 0x106C, RW::ReadWrite, false, 0x3FFF, -1); /// R/W   OK
    const Reg RiseTimeValidationWindow  ("RiseTimeValidationWindow" , 0x1070, RW::ReadWrite, false,  0x3FF,  1); /// R/W   OK
    const Reg TriggerHoldOffWidth       ("TriggerHoldOffWidth"      , 0x1074, RW::ReadWrite, false,  0x3FF,  4); /// R/W   OK
    const Reg PeakHoldOff               ("PeakHoldOff"              , 0x1078, RW::ReadWrite, false,  0x3FF,  4); /// R/W   OK
    const Reg ShapedTriggerWidth        ("ShapedTriggerWidth"       , 0x1084, RW::ReadWrite, false,  0x3FF,  4); /// R/W   not sure
    const Reg DPPAlgorithmControl2_G    ("DPPAlgorithmControl2_G"   , 0x10A0, RW::ReadWrite,  true, {}); /// R/W   OK
    const Reg FineGain                  ("FineGain"                 , 0x10C4, RW::ReadWrite, false, {}); /// R/W   OK

    namespace Bit_DPPAlgorithmControl2 {
      const std::pair<unsigned short, unsigned short> LocalShapeTriggerMode = {3, 0} ;
      const std::pair<unsigned short, unsigned short> LocalTrigValidMode = {3, 4} ;
      const std::pair<unsigned short, unsigned short> Extra2Option = {3, 8} ;
      const std::pair<unsigned short, unsigned short> VetoSource = {2, 14} ;
      const std::pair<unsigned short, unsigned short> TriggerCounterFlag = {2, 16} ;
      const std::pair<unsigned short, unsigned short> ActivebaselineCalulation = {1, 18} ;
      const std::pair<unsigned short, unsigned short> TagCorrelatedEvents = {1, 19} ;
      const std::pair<unsigned short, unsigned short> ChannelProbe = {4, 20} ; 
      const std::pair<unsigned short, unsigned short> EnableActiveBaselineRestoration = {1, 29} ;

      const std::vector<std::pair<std::string, unsigned int>> ListLocalShapeTrigMode = {{"Disabled", 0},
                                                                                        {"AND", 4},
                                                                                        {"The even Channel", 5},
                                                                                        {"The odd Channel", 6},
                                                                                        {"OR", 7}};

      const std::vector<std::pair<std::string, unsigned int>> ListLocalTrigValidMode = {{"Disabled", 0},
                                                                                        {"Crossed Trigger", 4},
                                                                                        {"Both from Mother board", 5},
                                                                                        {"AND", 6},
                                                                                        {"OR", 7}};

      const std::vector<std::pair<std::string, unsigned int>> ListExtra2 = {{"Extended timeStamp +  baseline * 4", 0},
                                                                            {"Extended timeStamp + Fine timestamp", 2},
                                                                            {"Lost Trig. Count  + Total Trig. Count", 4},
                                                                            {"Event Before 0-xing + After 0-xing", 5}};

      const std::vector<std::pair<std::string, unsigned int>> ListVetoSource = {{"Disabled", 0},
                                                                                {"Common (Global Trig. Mask)", 1},
                                                                                {"Difference (Trig. Mask)", 2},
                                                                                {"Negative Saturation", 3}};

      const std::vector<std::pair<std::string, unsigned int>> ListTrigCounter = {{"1024", 0},
                                                                                {"128", 1},
                                                                                {"8192", 2}};

      const std::vector<std::pair<std::string, unsigned int>> ListChannelProbe = {{"Acq Armed", 1},
                                                                                  {"Self-Trig", 2},
                                                                                  {"Pile-Up", 3},
                                                                                  {"Pile-Up / Self-Trig", 4},
                                                                                  {"Veto", 5},
                                                                                  {"Coincident", 6},
                                                                                  {"Trig Valid.", 7},
                                                                                  {"Trig Valid. Acq Windown", 8},
                                                                                  {"Anti-coin. Event", 9},
                                                                                  {"Discard no coin. Event", 10},
                                                                                  {"Valid Event", 11},
                                                                                  {"Not Valid Event", 12}};

    }

  }

  namespace PSD  {
    const Reg CFDSetting                     ("CFDSetting"                     , 0x103C, RW::ReadWrite, false, 0xFF, 1); /// R/W
    const Reg ForcedDataFlush_W              ("ForcedDataFlush_W"              , 0x1040, RW::WriteONLY, false, {}); ///   W
    const Reg ChargeZeroSuppressionThreshold ("ChargeZeroSuppressionThreshold" , 0x1044, RW::ReadWrite, false, 0xFFFF, -1); /// R/W
    const Reg ShortGateWidth                 ("ShortGateWidth"                 , 0x1054, RW::ReadWrite, false,  0xFFF,  1); /// R/W
    const Reg LongGateWidth                  ("LongGateWidth"                  , 0x1058, RW::ReadWrite, false,  0xFFF,  1); /// R/W
    const Reg GateOffset                     ("GateOffset"                     , 0x105C, RW::ReadWrite, false,   0xFF,  1); /// R/W
    const Reg TriggerThreshold               ("TriggerThreshold"               , 0x1060, RW::ReadWrite, false, 0x3FFF, -1); /// R/W
    const Reg FixedBaseline                  ("FixedBaseline"                  , 0x1064, RW::ReadWrite, false, 0x3FFF, -1); /// R/W
    const Reg TriggerLatency                 ("TriggerLatency"                 , 0x106C, RW::ReadWrite, false,  0x3FF,  4); /// R/W
    const Reg ShapedTriggerWidth             ("ShapedTriggerWidth"             , 0x1070, RW::ReadWrite, false,  0x3FF,  4); /// R/W
    const Reg TriggerHoldOffWidth            ("TriggerHoldOffWidth"            , 0x1074, RW::ReadWrite, false, 0xFFFF,  4); /// R/W
    const Reg ThresholdForPSDCut             ("ThresholdForPSDCut"             , 0x1078, RW::ReadWrite, false,  0x3FF, -1); /// R/W
    const Reg PurGapThreshold                ("PurGapThreshold"                , 0x107C, RW::ReadWrite, false,  0xFFF, -1); /// R/W
    const Reg DPPAlgorithmControl2_G         ("DPPAlgorithmControl2_G"         , 0x1084, RW::ReadWrite,  true, {}); /// R/W 
    const Reg EarlyBaselineFreeze            ("EarlyBaselineFreeze"            , 0x10D8, RW::ReadWrite, false,  0x3FF,  4); /// R/W

    namespace Bit_CFDSetting {
      const std::pair<unsigned short, unsigned short> CFDDealy = {8, 0} ;
      const std::pair<unsigned short, unsigned short> CFDFraction = {2, 8} ;
      const std::pair<unsigned short, unsigned short> Interpolation = {2, 10} ;

      const std::vector<std::pair<std::string, unsigned int>> ListCFDFraction = {{"25%", 0},
                                                                                 {"50%", 1},
                                                                                 {"75%", 2},
                                                                                 {"100%", 3}};

      const std::vector<std::pair<std::string, unsigned int>> ListItepolation = {{"sample before and after 0-Xing", 0},
                                                                                 {"2nd sample before and after 0-Xing", 1},
                                                                                 {"3rd sample before and after 0-Xing", 2},
                                                                                 {"4th sample before and after 0-Xing", 3}};
    }

    namespace Bit_DPPAlgorithmControl2 {
      const std::pair<unsigned short, unsigned short> LocalShapeTriggerMode = {3, 0} ;
      const std::pair<unsigned short, unsigned short> LocalTrigValidMode = {3, 4} ;
      const std::pair<unsigned short, unsigned short> ExtraWordOption = {3, 8} ;
      const std::pair<unsigned short, unsigned short> SmoothedChargeIntegration = {5, 11} ;
      const std::pair<unsigned short, unsigned short> TriggerCounterFlag = {2, 16} ;
      const std::pair<unsigned short, unsigned short> VetoSource = {2, 18} ;
      const std::pair<unsigned short, unsigned short> ChannelProbe = {4, 20} ;
      const std::pair<unsigned short, unsigned short> MarkSaturation = {1, 24} ;
      const std::pair<unsigned short, unsigned short> AdditionLocalTrigValid = {2, 25} ;
      const std::pair<unsigned short, unsigned short> VetoMode = {1, 27} ;
      const std::pair<unsigned short, unsigned short> ResetTimestampByTRGIN = {1, 28} ;

      const std::vector<std::pair<std::string, unsigned int>> ListLocalShapeTrigMode = {{"Disabled", 0},
                                                                                        {"AND", 4},
                                                                                        {"The even Channel", 5},
                                                                                        {"The odd Channel", 6},
                                                                                        {"OR", 7}};

      const std::vector<std::pair<std::string, unsigned int>> ListLocalTrigValidMode = {{"Disabled", 0},
                                                                                        {"Crossed Trigger", 4},
                                                                                        {"Both from Mother board", 5},
                                                                                        {"AND", 6},
                                                                                        {"OR", 7}};

      const std::vector<std::pair<std::string, unsigned int>> ListExtraWordOpt = {{"Extended timeStamp +  baseline * 4", 0},
                                                                                  {"Extended timeStamp + Flag", 1},
                                                                                  {"Extended timeStamp + Flag + Fine Time Stamp", 2},
                                                                                  {"Lost Trig. Count  + Total Trig. Count", 4},
                                                                                  {"Positive 0-Xing + Negative 0-Xing", 5},
                                                                                  {"Fixed value 0x12345678", 7}};

      const std::vector<std::pair<std::string, unsigned int>> ListSmoothedChargeIntegration = {{"Disabled", 0},
                                                                                              {"2 samples", 2},
                                                                                              {"4 sample",  4},
                                                                                              {"8 sample",  6},
                                                                                              {"16 sample", 8}};

      const std::vector<std::pair<std::string, unsigned int>> ListVetoSource = {{"Disabled", 0},
                                                                                {"Common (Global Trig. Mask)", 1},
                                                                                {"Difference (Trig. Mask)", 2},
                                                                                {"Negative Saturation", 3}};

      const std::vector<std::pair<std::string, unsigned int>> ListChannelProbe = {{"OverThreshold", 1},
                                                                                  {"Self-Trig", 2},
                                                                                  {"Pile-Up", 3},
                                                                                  {"Pile-Up / Self-Trig", 4},
                                                                                  {"Veto", 5},
                                                                                  {"Coincident", 6},
                                                                                  {"Trig Valid.", 7},
                                                                                  {"Trig Valid. Acq Windown", 8},
                                                                                  {"Neutron Pulse", 9},
                                                                                  {"Gamma Pulse", 10},
                                                                                  {"Neutron Pulse (gate end)", 11},
                                                                                  {"Gamma Pulse (gate end)", 12}};
                                                                                
      const std::vector<std::pair<std::string, unsigned int>> ListTrigCounter = {{"1024", 0},
                                                                                {"128", 1},
                                                                                {"8192", 2}};

      const std::vector<std::pair<std::string, unsigned int>> ListAdditionLocalTrigValid = {{"No Addtional Opt.", 0},
                                                                                            {"AND motherboard", 1}, // must be crossed
                                                                                            {"OR motherboard", 2}}; // must be crossed

      const std::vector<std::pair<std::string, unsigned int>> ListVetoMode = {{"after charge integration", 0},
                                                                              {"inhibit self-trigger", 1}}; // must be crossed


    }

  }

  namespace QDC { // Register already grouped in channel. and there no control for indiviual channel except the Fine DC offset and threshold, so it is like no group
    const Reg GateWidth                   ("GateWidth"                     , 0x1030, RW::ReadWrite, false, 0xFFF, 1); /// R/W
    const Reg GateOffset                  ("GateOfset"                     , 0x1034, RW::ReadWrite, false,  0xFF, 1); /// R/W
    const Reg FixedBaseline               ("FixedBaseline"                 , 0x1038, RW::ReadWrite, false, 0xFFF, -1); /// R/W
    const Reg PreTrigger                  ("PreTrigger"                    , 0x103C, RW::ReadWrite, false,  0xFF, 1); /// R/W
    const Reg DPPAlgorithmControl         ("DPPAlgorithmControl"           , 0x1040, RW::ReadWrite, false, {}); /// R/W
    const Reg TriggerHoldOffWidth         ("Trigger Hold-off width"        , 0x1074, RW::ReadWrite, false, 0xFFFF, 1); /// R/W
    const Reg TRGOUTWidth                 ("Trigger out width"             , 0x1078, RW::ReadWrite, false, 0xFFFF, 1); /// R/W
    const Reg OverThresholdWidth          ("Over Threshold width"          , 0x107C, RW::ReadWrite, false, 0xFFFF, 1); /// R/W
    const Reg GroupStatus_R               ("Group Status"                  , 0x1088, RW::ReadONLY,  false, {});  /// R/
    const Reg AMCFirmwareRevision_R       ("AMC firmware version"          , 0x108C, RW::ReadONLY,  false, {});  /// R/
    const Reg DCOffset                    ("DC offset"                     , 0x1098, RW::ReadWrite, false, 0xFFFF, -1); /// R/W
    const Reg ChannelMask                 ("Channel Group Mask"            , 0x10A8, RW::ReadWrite, false,   0xFF, 1); /// R/W
    const Reg DCOffset_LowCh              ("DC offset for low ch."         , 0x10C0, RW::ReadWrite, false, 0xFFFFFFFF, -1); /// R/W
    const Reg DCOffset_HighCh             ("DC offset for high ch."        , 0x10C4, RW::ReadWrite, false, 0xFFFFFFFF, -1); /// R/W
    const Reg TriggerThreshold_sub0       ("Trigger Threshold sub0"             , 0x10D0, RW::ReadWrite, false, 0xFFF, -1); /// R/W
    const Reg TriggerThreshold_sub1       ("Trigger Threshold sub1"             , 0x10D4, RW::ReadWrite, false, 0xFFF, -1); /// R/W
    const Reg TriggerThreshold_sub2       ("Trigger Threshold sub2"             , 0x10D8, RW::ReadWrite, false, 0xFFF, -1); /// R/W
    const Reg TriggerThreshold_sub3       ("Trigger Threshold sub3"             , 0x10DC, RW::ReadWrite, false, 0xFFF, -1); /// R/W
    const Reg TriggerThreshold_sub4       ("Trigger Threshold sub4"             , 0x10E0, RW::ReadWrite, false, 0xFFF, -1); /// R/W
    const Reg TriggerThreshold_sub5       ("Trigger Threshold sub5"             , 0x10E4, RW::ReadWrite, false, 0xFFF, -1); /// R/W
    const Reg TriggerThreshold_sub6       ("Trigger Threshold sub6"             , 0x10E8, RW::ReadWrite, false, 0xFFF, -1); /// R/W
    const Reg TriggerThreshold_sub7       ("Trigger Threshold sub7"             , 0x10EC, RW::ReadWrite, false, 0xFFF, -1); /// R/W


    const Reg NumberEventsPerAggregate      ("Number of Events per Aggregate", 0x8020, RW::ReadWrite, false, 0x3FF, 1); /// R/W
    const Reg RecordLength                  ("Record Length"                 , 0x8024, RW::ReadWrite, false, 0x1FFF, 1); /// R/W
    const Reg GroupEnableMask               ("Group Enable Mask"             , 0x8120, RW::ReadWrite, false, 0xFF, 1); /// R/W

    namespace Bit_DPPAlgorithmControl {
      const std::pair<unsigned short, unsigned short> ChargeSensitivity = {3, 0} ; /// length, smallest pos
      const std::pair<unsigned short, unsigned short> InternalTestPulse = {1, 4};
      const std::pair<unsigned short, unsigned short> TestPulseRate = {2, 5};
      const std::pair<unsigned short, unsigned short> OverThresholdWitdhEnable = {1, 7};
      const std::pair<unsigned short, unsigned short> ChargePedestal = {1, 8};
      const std::pair<unsigned short, unsigned short> InputSmoothingFactor = {3, 12};
      const std::pair<unsigned short, unsigned short> Polarity = {1, 16};
      const std::pair<unsigned short, unsigned short> TriggerMode = {2, 18};
      const std::pair<unsigned short, unsigned short> BaselineAvg = {3, 20};
      const std::pair<unsigned short, unsigned short> DisableSelfTrigger = {1, 24};
      const std::pair<unsigned short, unsigned short> DisableTriggerHysteresis = {1, 30};

      const std::vector<std::pair<std::string, unsigned int>> ListChargeSensitivity = {{"0.16 pC", 0},
                                                                                       {"0.32 pC", 1},
                                                                                       {"0.64 pC", 2},
                                                                                       {"1.28 pC", 3},
                                                                                       {"2.56 pC", 4},
                                                                                       {"5.12 pC", 5},
                                                                                       {"10.24 pC", 6},
                                                                                       {"20.48 pC", 7}};

      const std::vector<std::pair<std::string, unsigned int>> ListTestPulseRate = {{"1 kHz", 0},
                                                                                   {"10 kHz", 1},
                                                                                   {"100 kHz", 2},
                                                                                   {"1 MHz", 3}};

      const std::vector<std::pair<std::string, unsigned int>> ListInputSmoothingFactor = {{"Disabled", 0},
                                                                                          {"2 samples", 1},
                                                                                          {"4 samples", 2},
                                                                                          {"8 samples", 3},
                                                                                          {"16 samples", 4},
                                                                                          {"32 samples", 5},
                                                                                          {"64 samples", 6}};
      
      const std::vector<std::pair<std::string, unsigned int>> ListPolarity = {{"Positive", 0},
                                                                              {"Negative", 1}};
    
      const std::vector<std::pair<std::string, unsigned int>> ListTrigMode = {{"Self-Trigger", 0},
                                                                              {"Coupled OR", 1}};
    
      const std::vector<std::pair<std::string, unsigned int>> ListBaselineAvg = {{"Fixed", 0},
                                                                                 {"4 samples", 1},
                                                                                 {"16 samples", 2},
                                                                                 {"64 samples", 3}};
    }

  }

} // end of DPP namepace Register

const std::vector<Reg> RegisterChannelList_PHA = {
  DPP::RecordLength_G             ,
  DPP::InputDynamicRange          ,
  DPP::NumberEventsPerAggregate_G ,
  DPP::PreTrigger                 ,
  DPP::PHA::DataFlush_W               ,
  DPP::PHA::ChannelStopAcquisition    ,
  DPP::PHA::RCCR2SmoothingFactor      ,
  DPP::PHA::InputRiseTime             ,
  DPP::PHA::TrapezoidRiseTime         ,
  DPP::PHA::TrapezoidFlatTop          ,
  DPP::PHA::PeakingTime               ,
  DPP::PHA::DecayTime                 ,
  DPP::PHA::TriggerThreshold          ,
  DPP::PHA::RiseTimeValidationWindow  ,
  DPP::PHA::TriggerHoldOffWidth       ,
  DPP::PHA::PeakHoldOff               ,
  DPP::DPPAlgorithmControl        ,
  DPP::PHA::ShapedTriggerWidth        ,
  DPP::ChannelStatus_R            ,
  DPP::AMCFirmwareRevision_R      ,
  DPP::ChannelDCOffset            ,
  DPP::PHA::DPPAlgorithmControl2_G    ,
  DPP::ChannelADCTemperature_R    ,
  DPP::IndividualSoftwareTrigger_W,
  DPP::PHA::FineGain                  ,
  DPP::VetoWidth                  ,
  DPP::TriggerValidationMask_G   
};

const std::vector<Reg> RegisterChannelList_PSD = {
  DPP::RecordLength_G             ,
  DPP::InputDynamicRange          ,
  DPP::NumberEventsPerAggregate_G ,
  DPP::PreTrigger                 ,
  DPP::PSD::CFDSetting                     ,
  DPP::PSD::ForcedDataFlush_W              ,
  DPP::PSD::ChargeZeroSuppressionThreshold ,
  DPP::PSD::ShortGateWidth                 ,
  DPP::PSD::LongGateWidth                  ,
  DPP::PSD::GateOffset                     ,
  DPP::PSD::TriggerThreshold               ,
  DPP::PSD::FixedBaseline                  ,
  DPP::PSD::TriggerLatency                 ,
  DPP::PSD::ShapedTriggerWidth             ,
  DPP::PSD::TriggerHoldOffWidth            ,
  DPP::PSD::ThresholdForPSDCut             ,
  DPP::PSD::PurGapThreshold                ,
  DPP::DPPAlgorithmControl        ,
  DPP::PSD::DPPAlgorithmControl2_G         ,
  DPP::ChannelStatus_R            ,
  DPP::AMCFirmwareRevision_R      ,
  DPP::ChannelDCOffset            ,
  DPP::ChannelADCTemperature_R    ,
  DPP::IndividualSoftwareTrigger_W,
  DPP::VetoWidth                  ,
  DPP::PSD::EarlyBaselineFreeze            ,  
  DPP::TriggerValidationMask_G   
};

const std::vector<Reg> RegisterChannelList_QDC = {
  DPP::QDC::GateWidth,
  DPP::QDC::GateOffset,
  DPP::QDC::FixedBaseline,
  DPP::QDC::PreTrigger,
  DPP::QDC::DPPAlgorithmControl,
  DPP::QDC::TriggerHoldOffWidth,
  DPP::QDC::TRGOUTWidth,
  DPP::QDC::OverThresholdWidth,
  DPP::QDC::GroupStatus_R,
  DPP::QDC::AMCFirmwareRevision_R,
  DPP::QDC::DCOffset,
  DPP::QDC::ChannelMask,
  DPP::QDC::DCOffset_LowCh,
  DPP::QDC::DCOffset_HighCh,
  DPP::QDC::TriggerThreshold_sub0,
  DPP::QDC::TriggerThreshold_sub1,
  DPP::QDC::TriggerThreshold_sub2,
  DPP::QDC::TriggerThreshold_sub3,
  DPP::QDC::TriggerThreshold_sub4,
  DPP::QDC::TriggerThreshold_sub5,
  DPP::QDC::TriggerThreshold_sub6,
  DPP::QDC::TriggerThreshold_sub7
};

/// Only Board Setting 
const std::vector<Reg> RegisterBoardList_PHAPSD = {
  
  DPP::BoardConfiguration          ,
  DPP::AggregateOrganization       ,
  DPP::ADCCalibration_W            ,
  DPP::ChannelShutdown_W           ,
  DPP::AcquisitionControl          ,
  DPP::AcquisitionStatus_R         ,
  DPP::SoftwareTrigger_W           ,
  DPP::GlobalTriggerMask           ,
  DPP::FrontPanelTRGOUTEnableMask  ,
  DPP::LVDSIOData                  ,
  DPP::FrontPanelIOControl         ,
  DPP::RegChannelEnableMask           ,
  DPP::ROCFPGAFirmwareRevision_R   ,
  DPP::EventStored_R               ,
  DPP::VoltageLevelModeConfig      ,
  DPP::SoftwareClockSync_W         ,
  DPP::BoardInfo_R                 ,
  DPP::AnalogMonitorMode           ,
  DPP::EventSize_R                 ,
  DPP::TimeBombDowncounter_R       ,
  DPP::FanSpeedControl             ,
  DPP::RunStartStopDelay           ,
  DPP::BoardFailureStatus_R        ,
  DPP::DisableExternalTrigger      ,
  DPP::FrontPanelLVDSIONewFeatures ,
  DPP::BufferOccupancyGain         ,
  DPP::ExtendedVetoDelay           ,
  DPP::ReadoutControl              ,
  DPP::ReadoutStatus_R             ,
  DPP::BoardID                     ,
  DPP::MCSTBaseAddressAndControl   ,
  DPP::RelocationAddress           ,
  DPP::InterruptStatusID           ,
  DPP::InterruptEventNumber        ,
  DPP::MaxAggregatePerBlockTransfer,
  DPP::Scratch                     ,
  DPP::SoftwareReset_W             ,
  DPP::SoftwareClear_W             ,
  DPP::ConfigurationReload_W       ,
  DPP::ROMChecksum_R               ,
  DPP::ROMChecksumByte2_R          ,
  DPP::ROMChecksumByte1_R          ,
  DPP::ROMChecksumByte0_R          ,
  DPP::ROMConstantByte2_R          ,
  DPP::ROMConstantByte1_R          ,
  DPP::ROMConstantByte0_R          ,
  DPP::ROM_C_Code_R                ,
  DPP::ROM_R_Code_R                ,
  DPP::ROM_IEEE_OUI_Byte2_R        ,
  DPP::ROM_IEEE_OUI_Byte1_R        ,
  DPP::ROM_IEEE_OUI_Byte0_R        ,
  DPP::ROM_BoardVersion_R          ,
  DPP::ROM_BoardFromFactor_R       ,
  DPP::ROM_BoardIDByte1_R          ,
  DPP::ROM_BoardIDByte0_R          ,
  DPP::ROM_PCB_rev_Byte3_R         ,
  DPP::ROM_PCB_rev_Byte2_R         ,
  DPP::ROM_PCB_rev_Byte1_R         ,
  DPP::ROM_PCB_rev_Byte0_R         ,
  DPP::ROM_FlashType_R             ,
  DPP::ROM_BoardSerialNumByte1_R   ,
  DPP::ROM_BoardSerialNumByte0_R   ,
  DPP::ROM_VCXO_Type_R             
  
};

const std::vector<Reg> RegisterBoardList_QDC = {

  DPP::BoardConfiguration  ,
  DPP::AggregateOrganization,
  DPP::QDC::NumberEventsPerAggregate,
  DPP::QDC::RecordLength ,
  DPP::AcquisitionControl,
  DPP::AcquisitionStatus_R,
  DPP::SoftwareTrigger_W,
  DPP::GlobalTriggerMask,
  DPP::FrontPanelTRGOUTEnableMask,
  DPP::LVDSIOData,
  DPP::FrontPanelIOControl,
  DPP::QDC::GroupEnableMask,
  DPP::ROCFPGAFirmwareRevision_R,
  DPP::VoltageLevelModeConfig,
  DPP::SoftwareClockSync_W,
  DPP::BoardInfo_R,
  DPP::AnalogMonitorMode,
  DPP::EventSize_R,
  DPP::TimeBombDowncounter_R,
  DPP::FanSpeedControl,
  DPP::RunStartStopDelay,
  DPP::BoardFailureStatus_R,
  DPP::DisableExternalTrigger,
  DPP::FrontPanelLVDSIONewFeatures,
  DPP::BufferOccupancyGain,
  DPP::ExtendedVetoDelay,
  DPP::ReadoutControl,
  DPP::ReadoutStatus_R,
  DPP::BoardID,
  DPP::MCSTBaseAddressAndControl,
  DPP::RelocationAddress,
  DPP::InterruptStatusID,
  DPP::InterruptEventNumber,
  DPP::MaxAggregatePerBlockTransfer,
  DPP::Scratch                     ,
  DPP::SoftwareReset_W             ,
  DPP::SoftwareClear_W             ,
  DPP::ConfigurationReload_W       ,
  DPP::ROMChecksum_R               ,
  DPP::ROMChecksumByte2_R          ,
  DPP::ROMChecksumByte1_R          ,
  DPP::ROMChecksumByte0_R          ,
  DPP::ROMConstantByte2_R          ,
  DPP::ROMConstantByte1_R          ,
  DPP::ROMConstantByte0_R          ,
  DPP::ROM_C_Code_R                ,
  DPP::ROM_R_Code_R                ,
  DPP::ROM_IEEE_OUI_Byte2_R        ,
  DPP::ROM_IEEE_OUI_Byte1_R        ,
  DPP::ROM_IEEE_OUI_Byte0_R        ,
  DPP::ROM_BoardVersion_R          ,
  DPP::ROM_BoardFromFactor_R       ,
  DPP::ROM_BoardIDByte1_R          ,
  DPP::ROM_BoardIDByte0_R          ,
  DPP::ROM_PCB_rev_Byte3_R         ,
  DPP::ROM_PCB_rev_Byte2_R         ,
  DPP::ROM_PCB_rev_Byte1_R         ,
  DPP::ROM_PCB_rev_Byte0_R         ,
  DPP::ROM_FlashType_R             ,
  DPP::ROM_BoardSerialNumByte1_R   ,
  DPP::ROM_BoardSerialNumByte0_R   ,
  DPP::ROM_VCXO_Type_R             

};

#endif
