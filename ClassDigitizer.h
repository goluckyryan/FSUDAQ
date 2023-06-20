#ifndef DIGITIZER_H
#define DIGITIZER_H

#include <stdio.h>
#include <string>
#include <sstream>
#include <cmath>
#include <cstring>  ///memset
#include <iostream> ///cout
#include <bitset>

#include "CAENDigitizer.h"
#include "CAENDigitizerType.h"

#include "macro.h"
#include "ClassData.h"
#include "RegisterAddress.h"

//################################################################
class Digitizer{
  
  protected:
    
    Data * data;

    //^---- fixed parameter
    int                   portID;        /// port ID for optical link for using PCIe card, from 0, 1, 2, 3
    int                   boardID;       /// board identity
    int                   handle;        /// i don't know why, but better separete the handle from boardID
    int                   NChannel;      /// number of channel
    int                   ADCbits;       /// ADC bit
    int                   DPPType;       /// DPP verion
    unsigned int          ADCFullSize;   /// pow(2, ADCbits) - 1
    float                 tick2ns;         /// channel to ns
    CAEN_DGTZ_BoardInfo_t BoardInfo;
    
    //^----- adjustable parameters
    uint32_t                 channelMask ;   /// the channel mask from NChannel
    uint32_t                 VMEBaseAddress; /// For direct USB or Optical-link connection, VMEBaseAddress must be 0    
    CAEN_DGTZ_ConnectionType LinkType;       /// USB or Optic
    CAEN_DGTZ_IOLevel_t      IOlev;          /// TTL signal (1 = 1.5 to 5V, 0 = 0 to 0.7V ) or NIM signal (1 = -1 to -0.8V, 0 = 0V)

    //^------- other parameters
    int  ret;            /// return value, refer to CAEN_DGTZ_ErrorCode
    bool isConnected;    /// true for digitizer communication estabished.
    bool AcqRun;         /// true when digitizer is taking data
    bool isDummy;        /// true for a dummy digitizer.

    //^-------- setting 
    std::string  settingFileName;          /// 
    FILE *       settingFile;              /// 
    bool         settingFileExist;         /// 
    bool         isSettingFilledinMemeory; /// false for disabled ReadAllSettingFromBoard()   
    unsigned int setting[SETTINGSIZE];     /// Setting, 4bytes x 2048 = 8192 bytes
    
    //^-------- other protected functions
    void         ErrorMsg(std::string header = "");

    uint32_t     returnData;

  public:
    Digitizer(); /// no digitizer open
    Digitizer(int boardID, int portID = 0, bool program = false, bool verbose = false); 
    virtual ~Digitizer();

    //^------ portID is for optical link for using PCIe card, from 0, 1, 2, 3
    int  OpenDigitizer(int boardID, int portID = 0, bool program = false, bool verbose = false);
    void SetDPPType        (int type) { this->DPPType = type;} /// for manual override, or, digitizer does not open
    void SetChannelMask    (uint32_t mask);
    void SetChannelOnOff   (unsigned short ch, bool onOff);
    int  CloseDigitizer();
    void Initalization();
    void Reset();
    bool IsDummy()         {return isDummy;}
    bool IsConnected()     {return isConnected;}
    
    void         PrintBoard()      ;    
    virtual int  ProgramBoard()    ; /// program a generic board, no program channel
    int          ProgramPHABoard() ; /// program a default PHA board with dual trace
    int          ProgramPSDBoard() ; 
    int          ProgramQDCBoard() ;
    
    //^================ ACQ control
    void   StopACQ();
    void   StartACQ();
    int    ReadData();
    bool   IsRunning() const {return AcqRun;}
    Data * GetData()   const {return data;}
    void   PrintACQStatue();

    unsigned int CalByteForBuffer();

    //^================= Settings
    /// write value to digitizer, memory, and settingFile (if exist)
    /// ONLY WriteRegister can have ch = -1, for writting all channels
    /// for board setting, ignore ch
    void WriteRegister (Reg registerAddress, uint32_t value, int ch = -1, bool isSave2MemAndFile = true); 
    /// read value from digitizer and memory, and save to memory, and settingFile(if exist), 
    /// for board setting, ignore ch
    uint32_t ReadRegister (Reg registerAddress, unsigned short ch = 0, bool isSave2MemAndFile = true, std::string str = "" ); 
    uint32_t PrintRegister(uint32_t address, std::string msg);
    
    //^================ Get Board info
    CAEN_DGTZ_BoardInfo_t GetBoardInfo()     const {return BoardInfo;}
    std::string GetModelName()               const {return BoardInfo.ModelName;}
    int         GetSerialNumber()            const {return BoardInfo.SerialNumber;}
    int         GetChannelMask()             { channelMask = GetSettingFromMemory(DPP::ChannelEnableMask); return channelMask;}
    bool        GetChannelOnOff(unsigned ch) { channelMask = GetSettingFromMemory(DPP::ChannelEnableMask); return (channelMask & ( 1 << ch) );} 
    float       GetTick2ns()                   const {return tick2ns;}
    int         GetNChannels()               const {return NChannel;}
    int         GetHandle()                  const {return handle;}
    int         GetDPPType()                 const {return DPPType;}
    std::string GetDPPString(int DPPType = 0);  /// if no input, use digitizer DPPType
    int         GetADCBits()                 const {return BoardInfo.ADC_NBits;}
    std::string GetROCVersion()              const {return BoardInfo.ROC_FirmwareRel;}
    std::string GetAMCVersion()              const {return BoardInfo.AMC_FirmwareRel;}
    CAEN_DGTZ_ConnectionType   GetLinkType() const {return LinkType;}
    int         GetErrorCode()               const {return ret;}
    
    //^================ Setting 
    Reg FindRegister(uint32_t address);
    /// board <--> memory functions
    void           ReadAllSettingsFromBoard  (bool force = false);
    void           ProgramSettingsToBoard    ();

    /// simply read settings from memory
    void           SetSettingToMemory        (Reg registerAddress, unsigned int value, unsigned short ch = 0);
    unsigned int   GetSettingFromMemory      (Reg registerAddress, unsigned short ch = 0);
    void           PrintSettingFromMemory    ();
    unsigned int * GetSettings()              {return setting;};    

    /// memory <--> file
    void         SaveAllSettingsAsText       (std::string fileName);
    void         SaveAllSettingsAsBin        (std::string fileName); 
    std::string  GetSettingFileName()        {return settingFileName;}
    /// tell the digitizer where to look at the setting file.
    /// if not exist, call SaveAllSettinsAsBin();
    void         SetSettingBinaryPath        (std::string fileName);
    /// load setting file to memory
    /// if problem, return -1; load without problem, return 0;
    int          LoadSettingBinaryToMemory   (std::string fileName); 
    void         SaveSettingToFile           (Reg registerAddress, unsigned int value,  unsigned short ch = 0); /// also save to memory
    unsigned int ReadSettingFromFile         (Reg registerAddress, unsigned short ch = 0); /// read from setting binary

    //============ old methods, that only manipulate digitizer register, not setting in memory
    void SetDPPAlgorithmControl(uint32_t bit, int ch);
    unsigned int ReadBits(Reg address, unsigned int bitLength, unsigned int bitSmallestPos, int ch );
    void SetBits(Reg address, unsigned int bitValue, unsigned int bitLength, unsigned int bitSmallestPos, int ch);
    void SetBits(Reg address, std::pair<unsigned short, unsigned short> bit, unsigned int bitValue, int ch){ SetBits(address, bitValue, bit.first, bit.second, ch);}
    static unsigned int ExtractBits(uint32_t value, std::pair<unsigned short, unsigned short> bit){ return ((value >> bit.second) & uint(pow(2, bit.first)-1) ); }

    //====== Board Config breakDown
    // bool IsEnabledAutoDataFlush()   {return (  GetSettingFromMemory(DPP::BoardConfiguration) & 0x1 );}
    // bool IsDecimateTrace()          {return ( (GetSettingFromMemory(DPP::BoardConfiguration) >>  1) & 0x1 );}
    // bool IsTriggerPropagate()       {return ( (GetSettingFromMemory(DPP::BoardConfiguration) >>  2) & 0x1 );}
    bool IsDualTrace_PHA()           {return ( (GetSettingFromMemory(DPP::BoardConfiguration) >> 11) & 0x1 );}
    // unsigned short AnaProbe1Type()  {return ( (GetSettingFromMemory(DPP::BoardConfiguration) >> 12) & 0x3 );}
    // unsigned short AnaProbe2Type()  {return ( (GetSettingFromMemory(DPP::BoardConfiguration) >> 14) & 0x3 );}
    bool IsRecordTrace()             {return ( (GetSettingFromMemory(DPP::BoardConfiguration) >> 16) & 0x1 );}
    // bool IsEnabledExtra2()          {return ( (GetSettingFromMemory(DPP::BoardConfiguration) >> 17) & 0x1 );}
    // bool IsRecordTimeStamp()        {return ( (GetSettingFromMemory(DPP::BoardConfiguration) >> 18) & 0x1 );}
    // bool IsRecordEnergy()           {return ( (GetSettingFromMemory(DPP::BoardConfiguration) >> 19) & 0x1 );}
    // unsigned short DigiProbe1Type() {return ( (GetSettingFromMemory(DPP::BoardConfiguration) >> 20) & 0xF );}
    // unsigned short DigiProbe2Type() {return ( (GetSettingFromMemory(DPP::BoardConfiguration) >> 26) & 0x7 );}

    // //====== DPP Algorithm Contol breakdown
    // unsigned short TrapReScaling(int ch)       {return ( (GetSettingFromMemory(DPP::DPPAlgorithmControl, ch) >>  0) & 0x1F );}
    // unsigned short TraceDecimation(int ch)     {return ( (GetSettingFromMemory(DPP::DPPAlgorithmControl, ch) >>  8) & 0x3 );}
    // unsigned short TraceDecimationGain(int ch) {return ( (GetSettingFromMemory(DPP::DPPAlgorithmControl, ch) >> 10) & 0x3 );}
    // unsigned short PeakMean(int ch)            {return ( (GetSettingFromMemory(DPP::DPPAlgorithmControl, ch) >> 12) & 0x3 );}
    // unsigned short Polarity(int ch)            {return ( (GetSettingFromMemory(DPP::DPPAlgorithmControl, ch) >> 16) & 0x1 );}
    // unsigned short TriggerMode(int ch)         {return ( (GetSettingFromMemory(DPP::DPPAlgorithmControl, ch) >> 18) & 0x3 );}
    // unsigned short BaseLineAvg(int ch)         {return ( (GetSettingFromMemory(DPP::DPPAlgorithmControl, ch) >> 18) & 0x7 );}
    // unsigned short DisableSelfTrigger(int ch)  {return ( (GetSettingFromMemory(DPP::DPPAlgorithmControl, ch) >> 24) & 0x1 );}
    // unsigned short EnableRollOverFlag(int ch)  {return ( (GetSettingFromMemory(DPP::DPPAlgorithmControl, ch) >> 26) & 0x1 );}
    // unsigned short EnablePileUpFlag(int ch)    {return ( (GetSettingFromMemory(DPP::DPPAlgorithmControl, ch) >> 27) & 0x1 );}

    // //====== DPP Algorithm Contol 2 breakdown
    // unsigned short LocalShapeMode(int ch)       {return ( (GetSettingFromMemory(DPP::PHA::DPPAlgorithmControl2_G, ch) >>  0) & 0x7 );}
    // unsigned short LocalTrigValidMode(int ch)   {return ( (GetSettingFromMemory(DPP::PHA::DPPAlgorithmControl2_G, ch) >>  4) & 0x7 );}
    // unsigned short Extra2Option(int ch)         {return ( (GetSettingFromMemory(DPP::PHA::DPPAlgorithmControl2_G, ch) >>  8) & 0x3 );}
    // unsigned short VetoSource(int ch)           {return ( (GetSettingFromMemory(DPP::PHA::DPPAlgorithmControl2_G, ch) >>  14) & 0x3 );}
    // unsigned short TrigCounter(int ch)          {return ( (GetSettingFromMemory(DPP::PHA::DPPAlgorithmControl2_G, ch) >>  16) & 0x3 );}
    // unsigned short ActiveBaseLineCal(int ch)    {return ( (GetSettingFromMemory(DPP::PHA::DPPAlgorithmControl2_G, ch) >>  18) & 0x1 );}
    // unsigned short TagCorrelatedEvents(int ch)  {return ( (GetSettingFromMemory(DPP::PHA::DPPAlgorithmControl2_G, ch) >>  19) & 0x1 );}
    // unsigned short OptimizeBaseLineRestorer(int ch)  {return ( (GetSettingFromMemory(DPP::PHA::DPPAlgorithmControl2_G, ch) >>  29) & 0x1 );}

    //====== Acquistion Control vreakdown


};


#endif
