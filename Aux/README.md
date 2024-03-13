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
    timeWindow : in ns, -1 = no event building 
     withTrace : 0 for no trace, 1 for trace 
       verbose : > 0 for debug  
     batchSize : the size of hit in a batch 
    Output file name is contructed from inFile1 
```

as an example, 

```sh
/EventBuilder 0 0 0 1000000 '\ls -1 test_001*.fsu'
```

setting the timeWindow to be -1, will split out a timesorted Hit.

## Important output message

Sometimes, you may encounter following output in red color
```sh
!!!!!!!!!!!!!!!!! ReadBatch | Need to increase the batch size.
```
That means the fsuReder need larger batchSize.

```sh
event 786 has size = 2350 > MAX_MULTI = 2000
```

This indicate the event 786 has event size 2350, which is larger than MAX_MULTI of 2000. depends on your experimental setup. If you think multiplicity more than 2000 makes sense, you can edit the MAX_MULTI in the EventBuilder.cpp.

## output 

Evenbuilder output is standard information, an example structure is

```sh
******************************************************************************
*Tree    :tree      : test_001_379_-1.root                                   *
*Entries :  2017231 : Total =       121385718 bytes  File  Size =   47528456 *
*        :          : Tree compression factor =   2.55                       *
******************************************************************************
*Br    0 :evID      : event_ID/l                                             *
*Entries :  2017231 : Total  Size=   16167926 bytes  File Size  =    4222686 *
*Baskets :      327 : Basket Size=    3835392 bytes  Compression=   3.83     *
*............................................................................*
*Br    1 :multi     : multi/i                                                *
*Entries :  2017231 : Total  Size=    8084409 bytes  File Size  =      56959 *
*Baskets :      165 : Basket Size=    1917952 bytes  Compression= 141.87     *
*............................................................................*
*Br    2 :sn        : sn[multi]/s                                            *
*Entries :  2017231 : Total  Size=   12143148 bytes  File Size  =    4648638 *
*Baskets :      406 : Basket Size=   25600000 bytes  Compression=   2.61     *
*............................................................................*
*Br    3 :ch        : ch[multi]/s                                            *
*Entries :  2017231 : Total  Size=   12143148 bytes  File Size  =    4719909 *
*Baskets :      406 : Basket Size=   25600000 bytes  Compression=   2.57     *
*............................................................................*
*Br    4 :e         : e[multi]/s                                             *
*Entries :  2017231 : Total  Size=   12142738 bytes  File Size  =    7040714 *
*Baskets :      406 : Basket Size=   25600000 bytes  Compression=   1.72     *
*............................................................................*
*Br    5 :e2        : e2[multi]/s                                            *
*Entries :  2017231 : Total  Size=   12143148 bytes  File Size  =    4649857 *
*Baskets :      406 : Basket Size=   25600000 bytes  Compression=   2.61     *
*............................................................................*
*Br    6 :e_t       : e_timestamp[multi]/l                                   *
*Entries :  2017231 : Total  Size=   24270794 bytes  File Size  =   12883867 *
*Baskets :      649 : Basket Size=   25600000 bytes  Compression=   1.88     *
*............................................................................*
*Br    7 :e_f       : e_fineTime[multi]/s                                    *
*Entries :  2017231 : Total  Size=   12143579 bytes  File Size  =    4636856 *
*Baskets :      406 : Basket Size=   25600000 bytes  Compression=   2.62     *
*............................................................................*
*Br    8 :traceLength : traceLength[multi]/s                                 *
*Entries :  2017231 : Total  Size=   12146944 bytes  File Size  =    4640404 *
*Baskets :      407 : Basket Size=   25600000 bytes  Compression=   2.62     *
*............................................................................*
```


# SettingsExplorer.cpp

This defines the Setting explorer, the explorer takes the setting *bin file as argument.