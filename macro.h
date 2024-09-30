#ifndef MACRO_H
#define MACRO_H

#define MaxNPorts 4   //for optical link
#define MaxNBoards 4  //for both optical link and usb

#define MaxNDigitizer MaxNPorts * MaxNBoards

#define MaxRegChannel 16 
#define MaxNChannels 64
#define MaxRecordLength 0x3fff * 8 
#define MaxSaveFileSize  1024 * 1024 * 1024 * 2

#define ScalarUpdateinMiliSec  1000 // msec

#define MaxDisplayTraceTimeLength 20000 //ns
#define ScopeUpdateMiliSec  200 // msec
#define MaxNumberOfTrace  5   // in an event

#define SETTINGSIZE 2048

#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define WHITE   "\033[37m"

#define DAQLockFile "DAQLock.dat"
#define PIDFile  "pid.dat"

#include <sys/time.h> /** struct timeval, select() */

inline unsigned int getTime_us(){
  unsigned int time_us;
  struct timeval t1;
  struct timezone tz;
  gettimeofday(&t1, &tz);
  time_us = (t1.tv_sec) * 1000 * 1000 + t1.tv_usec;
  return time_us;
}

#include <chrono>
inline unsigned long long getTime_ns(){
  std::chrono::high_resolution_clock::time_point currentTime = std::chrono::high_resolution_clock::now();
  std::chrono::nanoseconds nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(currentTime.time_since_epoch());
  return nanoseconds.count();
}

typedef unsigned short uShort;
typedef unsigned int uInt;
typedef unsigned long  uLong;
typedef unsigned long long ullong;

#define DebugMode 0 //process check, when 1, print out all function call

// if DebugMode is 1, define DebugPrint() to be printf(), else, DebugPrint() define nothing
#if DebugMode
#define DebugPrint(fmt, ...) printf(fmt "::%s\n",##__VA_ARGS__, __func__);
#else
#define DebugPrint(fmt, ...)
#endif


#endif
