# Introduction

This is a DAQ for 1st gen CAEN digitizer for V1725, V17255S, V1230 with PHA and PSD firmware.

It has scope (updated every half-sec), allow full control of the digitizer (except LVDS), and allow saving waveform.

It can be connected to InfluxDB v1.8 and Elog.

Each channel has it own 1D histogram. It will not be filled by default, but can enable it in the "Online 1D histgram" panel. The binning of each histogram will be saved under the raw data path as singleSpectaSetting.txt

# Operation

When programSettings.txt is presented in the same folder as the FSUDAQ_Qt, the program will load it can config the following 

- (line 1) raw data path, where the data will be stored.
- (line 2) the influxDB v1.8 IP
- (line 3) the database name
- (line 4) the elog IP
- (line 5) the elog logbook name
- (line 6) elog user name
- (line 7) elog user password

If no programSettings.txt is found. The program can still search for all digitizers that connected using optical cable. 
Missing the raw data path will disable save data run, but still can start the ACQ. Missing InfluxDB (elog) variables will disable influxDB (elog). 

# Undergoing

the following additional functions are planned and I am working on them

- 1-D and 2-D histogram
- Online Analyzer
- support V1740 DPP-QDC
- synchronization helper

# Required / Development enviroment

Ubuntu 22.04

- CAENVMELib v3.3
- CAENCOmm v1.5.3
- CAENDigitizer v2.17.1
- CAEN A3818 Driver v1.6.8

- qt6-base-dev
- libqt6charts6-dec
- libcurl4-openssl-dev
- elog

The CAEN Libraries need to download and install manually. The other libraries can be installed using the following command:

`sudo apt install qt6-base-dev libqt6charts6-dev libcurl4-openssl-dev elog`

The elog installed using apt is 3.1.3. If a higher version is needed. Please go to https://elog.psi.ch/elog/

The libcurl4 is need for pushing data to InfluxDB v1.8

The QCustomPlot (https://www.qcustomplot.com/index.php/introduction) source files are already included in the repository.

# Compile

use `qmake6 -project ` to generate the *.pro

in the *.pro, add 

` QT += core widgets charts printsupport`

` LIBS += -lCAENDigitizer -lcurl`

then run ` qmake6 *.pro` it will generate Makefile

then  ` make`

if you want to use GDB debugger, in the *.pro file add

` QMAKE_CXXFLAGS += -g`

## exclude some files from the auto-gen *.pro

The following files must be excluded from the *.pro, as they are not related to the GUI

- DataGenerator.cpp
- DataReaderScript.cpp
- EventBuilder.cpp
- test.cpp
- test_indep.cpp

Those file can be compiled using 

`make -f Makefile_test`