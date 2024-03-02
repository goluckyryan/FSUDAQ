# Discord 

https://discord.gg/vxAM2ZHM

# Introduction

This is a DAQ for 1st gen CAEN digitizer for 

- V1725, V17255S, V1230 with PHA and PSD firmware.
- V1740 with QDC firmware

It has scope (updated every half-sec), allow full control of the digitizer (except LVDS), and allow saving waveform.

It can be connected to InfluxDB v1.8+ and Elog.

Each channel has it own 1D histogram. It will not be filled by default, but can enable it in the "Online 1D histgram" panel. The binning of each histogram will be saved under the raw data path as singleSpectaSetting.txt

## Wiki
https://fsunuc.physics.fsu.edu/wiki/index.php/CAEN_digitizer

## Online analyzer
A Multi-builder (event builder that can build event across multiple digitizer) is made. It has normal event building code and also a backward event building code that build events from the latest data up to certain amont of event.

A 1-D and 2-D histogram is avalible. In the 2-D histogram, graphical cuts can be created and rename.

An online analyzer class is created as a template for online analysis. An example is the SplitPoleAnalyzer.h. It demo a 2-D histogram and a 1-D histogram, and the way to output the rates of cuts to influxDB.

<span style="color:red;">Notice that, when the FSUDAQ is started, the online analyzer is not created, no event will be built. Once the online anlyzer is created and opened, event will be built, even the window is closed. </span>

# Operation

When programSettings.txt is presented in the same folder as the FSUDAQ_Qt, the program will load it can config the following 

- (line 1) raw data path, where the data will be stored.
- (line 2) the influxDB v1.8+ IP
- (line 3) the database name
- (line 4) the influxDB token (for v2.0+)
- (line 5) the elog IP
- (line 6) the elog logbook name
- (line 7) elog user name
- (line 8) elog user password

If no programSettings.txt is found. The program can still search for all digitizers that connected using optical cable. 
Missing the raw data path will disable save data run, but still can start the ACQ. Missing InfluxDB (elog) variables will disable influxDB (elog). 


# ToDo

- Gaussians fitting for 1D Histogram
- Improve the color scheme for 2D histogram
- Save Histogram?

# Required / Development enviroment

Ubuntu 22.04

- CAENVMELib v3.3 +
- CAENCOmm v1.5.3 +
- CAENDigitizer v2.17.1 +
- CAEN A3818 Driver v1.6.8 +

- qt6-base-dev
- libqt6charts6-dec
- libcurl4-openssl-dev
- elog

The CAEN Libraries need to download and install manually. The other libraries can be installed using the following command:

`sudo apt install qt6-base-dev libqt6charts6-dev libcurl4-openssl-dev elog`

The elog installed using apt is 3.1.3. If a higher version is needed. Please go to https://elog.psi.ch/elog/

The libcurl4 is need for pushing data to InfluxDB v1.8+

InfluxDB can be installed via `sudo apt install influxdb` for v1.8. the v2.0+ can be manually installed from the InfluxDB webpage https://docs.influxdata.com/influxdb/v2/.

The QCustomPlot (https://www.qcustomplot.com/index.php/introduction) source files are already included in the repository.

## For A4818 optical-USB 

need to install the A4818 driver.

Make sure connect the optical fiber before switch on the digitizer(s). If unplug the optical fiber and reconnect, need to restart the digitizer(s).

## For A5818 PCI gen 3

need to install CAENVMELib v4.0 +

## For Raspberry Pi installation

All required CAEN Libraries support ARM archetect, so installation of those would not be a problem.

THe libqt6charts6-dev should be replaced by qt6-chart-dev, and the elog need to be installed manually (or can be skipped)

`sudo apt install qt6-base-dev qt6-chart-dev libcurl4-openssl-dev`

I tested with a Raspberry Pi 5 with 8 GB + A4818 optical-USB adaptor. it works.

# Compile

## in case the *.pro not exist or modified
use `qmake6 -project ` to generate the *.pro

in the *.pro, add 

- ` QT += core widgets charts printsupport`
- ` LIBS += -lCAENDigitizer -lcurl`

## if *.pro exist

then run ` qmake6 *.pro` it will generate Makefile

then  ` make`

if you want to use GDB debugger, in the *.pro file add

` QMAKE_CXXFLAGS += -g`

# Auxillary programs (e.g. Event Builder)

There is a folder Aux, this folder contains many auxillary programs, such as EventBuilder. User can `make` under the folder to compile the programs.

# Tested Trigger Rate

* V1725, 1 MHz to 3 ch (decay = 500 ns), no trace. need to set Agg/Read = 100 and Evt/Agg = 511.

# Known Issues

* If accessing the database takes too long, recommend to disable the database.
* Pile up rate is not accurate for very high input rate ( > 60 kHz ).
* When using the scope, the Agg/Read must be smaller than 512.
* Although the Events/Agg used the CAEN API to recalculate before ACQ start, for PHA firmware, when the trigger rate changed, the Events per Agg need to be changed.
* The Agg Organization, Event per Agg, Record Length are strongly correlated. Some settings are invalid and will cause the digitizer goes crazy.
* load digitizer setting would not load everything, only load the channel settings and some board settings.
* Sometimes, the buffer is not in time order, and make the trigger/Accept rate to be nagative. This is nothing to do with the program but the digitizer settings. Recommand reporgram the digitizer.
* for 1740 QDC, RecordLenght is board setting, but readout is indivuial group.
* FOr PHA, the trapezoid scaling and fine-gain register are calculated before ACQ start.

# Known Bugs

* EventBuilder will crash when trigger rate of the data is very high.