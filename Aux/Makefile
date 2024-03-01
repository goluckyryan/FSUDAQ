########################################################################
#
#
#########################################################################

CC	=	g++

#COPTS	=	-fPIC -DLINUX -O2 -std=c++17 -lpthread
COPTS	=	-fPIC -DLINUX -g -O0 -Wall -std=c++17 -lpthread

CAENLIBS	=	-lCAENDigitizer

ROOTLIBS = `root-config --cflags --glibs`

OBJS = ClassDigitizer.o MultiBuilder.o ClassInfluxDB.o

ALL = test test_indep EventBuilder EventBuilderNoTrace DataReader DumpFSU2ROOT SettingsExplorer 

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
		$(CC) $(COPTS) -o test test.cpp ../ClassDigitizer.o ../MultiBuilder.o ../ClassInfluxDB.o $(CAENLIBS)  $(ROOTLIBS) -lcurl

test_indep : test_indep.cpp ../RegisterAddress.h ../macro.h
		@echo "--------- making test_indep"
		$(CC) $(COPTS) -o test_indep test_indep.cpp $(CAENLIBS) 

DataReader : DataReaderScript.cpp ../ClassData.h MultiBuilder.o
		@echo "--------- making DataReader"
		$(CC) $(COPTS) -o DataReader DataReaderScript.cpp ../ClassData.h MultiBuilder.o

# EventBuilder_old : EventBuilder_old.cpp ../ClassData.h MultiBuilder.o fsuReader.h
# 		@echo "--------- making EventBuilder"
# 		$(CC) $(COPTS) -o EventBuilder_old EventBuilder_old.cpp MultiBuilder.o $(ROOTLIBS) 

EventBuilder : EventBuilder.cpp ../ClassData.h fsuReader.h fsutsReader.h ../Hit.h CustomStruct.h
		@echo "--------- making EventBuilder"
		$(CC) $(COPTS) -o EventBuilder EventBuilder.cpp $(ROOTLIBS) 

EventBuilderNoTrace : EventBuilderNoTrace.cpp ../ClassData.h fsuReader.h ../Hit.h CustomStruct.h
		@echo "--------- making EventBuilderNoTrace"
		$(CC) $(COPTS) -o EventBuilderNoTrace EventBuilderNoTrace.cpp  $(ROOTLIBS) 

DumpFSU2ROOT : DumpFSU2ROOT.cpp ../ClassData.h MultiBuilder.o
		@echo "--------- making DumpFSU2ROOT"
		$(CC) $(COPTS) -o DumpFSU2ROOT DumpFSU2ROOT.cpp ../ClassData.h MultiBuilder.o $(ROOTLIBS)

SettingsExplorer : SettingsExplorer.cpp ../ClassDigitizer.o ../RegisterAddress.h
		@echo "--------- making SettingsExplorer"
		$(CC) $(COPTS) -o SettingsExplorer SettingsExplorer.cpp ../ClassDigitizer.o $(CAENLIBS)
