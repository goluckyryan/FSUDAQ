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

    ///---- fixed parameter
    int                   portID;        /// port ID for optical link for using PCIe card, from 0, 1, 2, 3
    int                   boardID;       /// board identity
    int                   handle;        /// i don't know why, but better separete the handle from boardID
    int                   NChannel;      /// number of channel
    int                   ADCbits;       /// ADC bit
    int                   DPPType;       /// DPP verion
    unsigned int          ADCFullSize;   /// pow(2, ADCbits) - 1
    float                 ch2ns;         /// channel to ns
    CAEN_DGTZ_BoardInfo_t BoardInfo;
    
    ///----- adjustable parameters
    uint32_t                 channelMask ;   /// the channel mask from NChannel
    uint32_t                 VMEBaseAddress; /// For direct USB or Optical-link connection, VMEBaseAddress must be 0    
    CAEN_DGTZ_ConnectionType LinkType;       /// USB or Optic
    CAEN_DGTZ_IOLevel_t      IOlev;          /// TTL signal (1 = 1.5 to 5V, 0 = 0 to 0.7V ) or NIM signal (1 = -1 to -0.8V, 0 = 0V)

    ///------- other parameters
    int  ret;            /// return value, refer to CAEN_DGTZ_ErrorCode
    bool isConnected;    /// true for digitizer communication estabished.
    bool AcqRun;         /// true when digitizer is taking data
    bool isDummy;        /// true for a dummy digitizer.

    /// ------- setting 
    std::string  settingFileName;          /// 
    FILE *       settingFile;              /// 
    bool         settingFileExist;         /// 
    bool         isSettingFilledinMemeory; /// false for disabled ReadAllSettingFromBoard()   
    unsigned int setting[SETTINGSIZE];     /// Setting, 4bytes x 2048 = 8192 bytes
    
    ///---------- protected functions
    
    void         ErrorMsg(std::string header = "");

  public:
    Digitizer(); /// no digitizer open
    Digitizer(int boardID, int portID = 0, bool program = false, bool verbose = false); 
    ~Digitizer();

    /// portID is for optical link for using PCIe card, from 0, 1, 2, 3
    int  OpenDigitizer(int boardID, int portID = 0, bool program = false, bool verbose = false);
    void SetDPPType        (int type) { this->DPPType = type;} /// for manual override, or, digitizer does not open
    void SetChannelMask    (uint32_t mask);
    void SetChannelOnOff   (unsigned short ch, bool onOff);
    int  CloseDigitizer();
    void Initalization();
    void Reset();
    bool IsDummy()         {return isDummy;};
    
    void         PrintBoard()      ;    
    virtual int  ProgramBoard()    ;    /// program a generic board, no program channel
    int          ProgramPHABoard() ; /// program a default PHA board
    
    ///================ ACQ control
    void   StopACQ();
    void   StartACQ();
    void   ReadData();
    bool   IsRunning() const {return AcqRun;}
    Data * GetData()   const {return data;}
    void   PrintACQStatue();

    unsigned int CalByteForBuffer();

    ///=================Settings
    /// write value to digitizer, memory, and settingFile (if exist)
    /// ONLY WriteRegister can have ch = -1, for writting all channels
    /// for board setting, ignore ch
    void WriteRegister (Reg registerAddress, uint32_t value, int ch = -1, bool isSave2MemAndFile = true); 
    /// read value from digitizer and memory, and save to memory, and settingFile(if exist), 
    /// for board setting, ignore ch
    uint32_t ReadRegister (Reg registerAddress, unsigned short ch = 0, bool isSave2MemAndFile = true, std::string str = "" ); 
    uint32_t PrintRegister(uint32_t address, std::string msg);
    
    ///================ Get Board info
    std::string GetModelName()               const {return BoardInfo.ModelName;}
    int         GetSerialNumber()            const {return BoardInfo.SerialNumber;}
    int         GetChannelMask()             const {return channelMask;}
    bool        GetChannelOnOff(unsigned ch) const {return (channelMask & ( 1 << ch) );} 
    float       GetCh2ns()                   const {return ch2ns;}
    int         GetNChannel()                const {return NChannel;}
    int         GetHandle()                  const {return handle;}
    bool        GetConnectionStatus()        const {return isConnected;}
    int         GetDPPType()                 const {return DPPType;}
    std::string GetDPPString(int DPPType = 0);  /// if no input, use digitizer DPPType
    int         GetADCBits()                 const {return BoardInfo.ADC_NBits;}
    std::string GetROCVersion()              const {return BoardInfo.ROC_FirmwareRel;}
    std::string GetAMCVersion()              const {return BoardInfo.AMC_FirmwareRel;}
    CAEN_DGTZ_ConnectionType   GetLinkType() const {return LinkType;}
    
    ///================ Setting 
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

    ///=================== Relic methods
    ///void SetRecordLength       (unsigned int ns, int ch = -1);  /// when ch == -1, mean set all channels
    ///void SetInputDynamicRange  (unsigned int TwoVol_0_or_halfVol_1, int ch = -1);
    ///void SetPreTriggerSample   (unsigned int nSample, int ch = -1 );
    ///void SetPreTriggerDuration (unsigned int ns, int ch = -1 );
    ///void SetDCOffset           (float offsetPrecentage, int ch = -1);
    ///void SetVetoWidth          (uint32_t bit, int ch = -1); /// See manual
    ///void SetTriggerPolarity    (bool RiseingIsZero, int ch = -1); ///not used for DPP firmware
    ///
    ///void SetEventAggregation              (unsigned int numEvent, int ch = -1);
    ///void SetAggregateOrganization         (unsigned int bit);
    ///void SetMaxAggregatePerBlockTransfer  (unsigned int numEvent);
    ///
    ///void SetBits(Reg address, unsigned int bitValue, unsigned int bitLength, unsigned int bitSmallestPos, int ch = -1);
    ///void SetDPPAlgorithmControl(uint32_t bit, int ch = -1);
    ///
    ///void SetACQControl(uint32_t bit);
    ///void SetGlobalTriggerMask(uint32_t bit);
    ///void SetFrontPanelTRGOUTMask(uint32_t bit);
    ///void SetFrontPanelIOControl(uint32_t bit);
    ///void SetTriggerValidationMask(uint32_t bit);
    ///void SetBoardID(unsigned int ID) {WriteRegister(Register::DPP::BoardID, ID));
    
    ///unsigned int GetRecordLengthSample(int ch)             {return ReadRegister(Register::DPP::RecordLength_G, ch) * 8;}
    ///unsigned int GetInputDynamicRange(int ch)              {return ReadRegister(Register::DPP::InputDynamicRange, ch);}
    ///unsigned int GetPreTriggerSample(int ch)               {return ReadRegister(Register::DPP::PreTrigger, ch) * 4;}
    ///float        GetDCOffset(int ch)                       {return 100.0 - ReadRegister(Register::DPP::ChannelDCOffset, ch) * 100. / 0xFFFFF; }
    ///unsigned int GetVetoWidth(int ch)                      {return ReadRegister(Register::DPP::VetoWidth, ch);}
    ///unsigned int GetEventAggregation(int ch = -1)          {return ReadRegister(Register::DPP::NumberEventsPerAggregate_G, ch);}
    ///unsigned int GetAggregateOrganization()                {return ReadRegister(Register::DPP::AggregateOrganization);}
    ///unsigned int GetMaxNumberOfAggregatePerBlockTransfer() {return ReadRegister(Register::DPP::MaxAggregatePerBlockTransfer);}
    ///
    ///unsigned int ReadBits(Reg address, unsigned int bitLength, unsigned int bitSmallestPos, int ch = -1 );
    ///unsigned int GetDPPAlgorithmControl(int ch = -1) {return ReadRegister(Register::DPP::DPPAlgorithmControl, ch);}
    ///
    ///int   GetChTemperature(int ch) ;
};


#endif
