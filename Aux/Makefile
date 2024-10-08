########################################################################
#
#
#########################################################################

CC	=	g++

COPTS	=	-fPIC -DLINUX -O2 -std=c++17 -lpthread
# COPTS	=	-fPIC -DLINUX -g -O0 -Wall -std=c++17 -lpthread

CAENLIBS	=	-lCAENDigitizer -lCAENVME

ROOTLIBS = `root-config --cflags --glibs`

OBJS = ClassDigitizer.o MultiBuilder.o ClassInfluxDB.o

ALL = test EventBuilder DataReader DumpFSU2ROOT SettingsExplorer FSU2CAEN

#########################################################################

all	:	$(ALL)

clean	:
		/bin/rm -f $(OBJS) $(ALL)

MultiBuilder.o : ../MultiBuilder.cpp ../MultiBuilder.h ../Hit.h
		$(CC) $(COPTS) -c ../MultiBuilder.cpp

ClassDigitizer.o : ../ClassDigitizer.cpp ../ClassDigitizer.h ../RegisterAddress.h ../macro.h ../ClassData.h
		$(CC) $(COPTS) -c ../ClassDigitizer.cpp

ClassInfluxDB.o : ../ClassInfluxDB.cpp ../ClassInfluxDB.h 
		$(CC) $(COPTS) -c ../ClassInfluxDB.cpp -lcurl

test	:	test.cpp ../ClassDigitizer.o ../MultiBuilder.o ../ClassInfluxDB.o
		@echo "--------- making test"
		$(CC) -fPIC -DLINUX -O0 -std=c++17 -lpthread -g -o test test.cpp ../ClassDigitizer.o ../MultiBuilder.o ../ClassInfluxDB.o $(CAENLIBS)  $(ROOTLIBS) -lcurl

# test_indep : test_indep.cpp ../RegisterAddress.h ../macro.h
# 		@echo "--------- making test_indep"
# 		$(CC) $(COPTS) -o test_indep test_indep.cpp $(CAENLIBS) 

DataReader : DataReaderScript.cpp ../ClassData.h MultiBuilder.o
		@echo "--------- making DataReader"
		$(CC) $(COPTS) -o DataReader DataReaderScript.cpp ../ClassData.h MultiBuilder.o

EventBuilder : EventBuilder.cpp ../ClassData.h fsuReader.h ../Hit.h
		@echo "--------- making EventBuilder"
		$(CC) $(COPTS) -o EventBuilder EventBuilder.cpp $(ROOTLIBS) 

FSU2CAEN : FSU2CAEN.cpp ../ClassData.h fsuReader.h ../Hit.h
		@echo "--------- making FSU2CAEN"
		$(CC) $(COPTS) -o FSU2CAEN FSU2CAEN.cpp 

DumpFSU2ROOT : DumpFSU2ROOT.cpp ../ClassData.h MultiBuilder.o
		@echo "--------- making DumpFSU2ROOT"
		$(CC) $(COPTS) -o DumpFSU2ROOT DumpFSU2ROOT.cpp ../ClassData.h MultiBuilder.o $(ROOTLIBS)

SettingsExplorer : SettingsExplorer.cpp ../ClassDigitizer.o ../RegisterAddress.h
		@echo "--------- making SettingsExplorer"
		$(CC) $(COPTS) -o SettingsExplorer SettingsExplorer.cpp ../ClassDigitizer.o $(CAENLIBS)
