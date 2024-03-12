# About this directory

This stores auxillary programs, mainly focus on reading the *.fsu file, For example, the fsuReader.h is a *.fsu reader. 

# Need to Make

```sh
make
```

# fsuReader.h

This declare the FSUReader class. it can be included as a header in a custom cpp file, or it can be loaded on cern root CLI (command line interface). 

```sh
>.L fsuReader.h
>FSUReader * reader = new FSUReader(<fileName>, <dataBufferSize>, verbose)
>reader->ScanNumBlock(verbose)
>reader->ReadNextBlock()
```
The FSUReader can also read a fileList as a chain.

Within the FSUReader, there exists a Data Class (defined in ClassData.h). This necessitates the specification of the data buffer size for the reader. The data buffer operates as a circular buffer, typically ranging from 10 to 600, which should be sufficient.

The ScanNumBlock() method not only scans the number of blocks (or aggregations) but also tallies the number of hits.

The FSUReader includes a std::vector<Hit>, which is utilized in the ReadNextBlock(bool traceON = false, int verbose = 0, uShort saveData = 0) function. When saveData = 0, no data is stored in the Hit vector; saveData = 1 excludes trace data, while saveData = 2 includes it. It's worth noting that the Hit vector can be quite memory-intensive, particularly when trace data is included.

The pivotal method for the event builder is the ReadBatch(unsigned int batchSize, bool verbose) function. This method reads a large number of Hits (with a size of batchSize). It effectively reads twice the specified batchSize: the first batch is stored as hitList_A, and the second as hitList_B. By comparing the timestamps of hitList_A and hitList_B, sorting if necessary, the method ensures that both hitLists are time-sorted. It then outputs hitList_A and internally stores hitList_B. Subsequent calls to this method replace hitList_A with hitList_B, retrieve a new batch to serve as the new hitList_B, repeat the sorting process, and output hitList_A.

Occasionally, the earliest timestamp of hitList_B precedes that of hitList_A. In such cases, increasing the batchSize resolves the issue, albeit at the cost of higher memory usage. Typically, a batchSize of 1 million should suffice.

With this approach, it is guaranteed that the output hitList_A is always time-sorted (given the batchSize is big enough), thereby simplifying the event building process.

# EvenBuilder.cpp

This defines the EventBuilder. The arguments are

```sh
./EventBuilder [timeWindow] [withTrace] [verbose] [batchSize] [inFile1]  [inFile2] .... 
    timeWindow : in ns 
     withTrace : 0 for no trace, 1 for trace 
       verbose : > 0 for debug  
     batchSize : the size of hit in a batch 
    Output file name is contructed from inFile1 
```

as an example, 

```sh
/EventBuilder 0 0 0 1000000 '\ls -1 test_001*.fsu'

```

# SettingsExplorer.cpp

This defines the Setting explorer, the explorer takes the setting *bin file as argument.