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
    float                 ch2ns;         /// channel to ns
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
    int          ProgramPHABoard() ; /// program a default PHA board
    
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
    void WriteRegister (Register::Reg registerAddress, uint32_t value, int ch = -1, bool isSave2MemAndFile = true); 
    /// read value from digitizer and memory, and save to memory, and settingFile(if exist), 
    /// for board setting, ignore ch
    uint32_t ReadRegister (Register::Reg registerAddress, unsigned short ch = 0, bool isSave2MemAndFile = true, std::string str = "" ); 
    uint32_t PrintRegister(uint32_t address, std::string msg);
    
    //^================ Get Board info
    std::string GetModelName()               const {return BoardInfo.ModelName;}
    int         GetSerialNumber()            const {return BoardInfo.SerialNumber;}
    int         GetChannelMask()             const {return channelMask;}
    bool        GetChannelOnOff(unsigned ch) const {return (channelMask & ( 1 << ch) );} 
    float       GetCh2ns()                   const {return ch2ns;}
    int         GetNChannels()               const {return NChannel;}
    int         GetHandle()                  const {return handle;}
    int         GetDPPType()                 const {return DPPType;}
    std::string GetDPPString(int DPPType = 0);  /// if no input, use digitizer DPPType
    int         GetADCBits()                 const {return BoardInfo.ADC_NBits;}
    std::string GetROCVersion()              const {return BoardInfo.ROC_FirmwareRel;}
    std::string GetAMCVersion()              const {return BoardInfo.AMC_FirmwareRel;}
    CAEN_DGTZ_ConnectionType   GetLinkType() const {return LinkType;}
    
    //^================ Setting 
    Register::Reg FindRegister(uint32_t address);
    /// board <--> memory functions
    void           ReadAllSettingsFromBoard  (bool force = false);
    void           ProgramSettingsToBoard    ();

    /// simply read settings from memory
    void           SetSettingToMemory        (Register::Reg registerAddress, unsigned int value, unsigned short ch = 0);
    unsigned int   GetSettingFromMemory      (Register::Reg registerAddress, unsigned short ch = 0);
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
    void         SaveSettingToFile           (Register::Reg registerAddress, unsigned int value,  unsigned short ch = 0); /// also save to memory
    unsigned int ReadSettingFromFile         (Register::Reg registerAddress, unsigned short ch = 0); /// read from setting binary

};


#endif
