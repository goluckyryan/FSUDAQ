# About this directory

This stores auxillary programs, mainly focus on reading the *.fsu file, For example, the fsuReader.h is a *.fsu reader. 

# fsuReader.h

This declare the FSUReader class. it can be included as a header in a custom cpp file, or it can be loaded on cern root CLI (command line interface). 

```sh
>.L fsuReader.h
>FSUReader * reader = new FSUReader(<fileName>, <dataBufferSize>, verbose)
>reader->ScanNumBlock(verbose)
>reader->ReadNextBlock()
```
The FSUReader can also read a fileList as a chain.

Inside the FSUReader, there is a Data Class (from ClassData.h). This is why we need to set the data Buffer size for the reader. The data buffer is a circular buffer, typically 10 to 600 should be enough.

There is a std::vector<Hit> in the FSUReader, in the ReadNextBlock(bool traceON = false, int verbose = 0, uShort saveData = 0), when saveData = 0, no data will be saved to the Hit vector, saveData = 1, no trace, and saveData = 2, with trace. Note that the Hit vector can be very memory demanding, especially with trace.

The ScanNumBlock() method scans number of block (or aggeragetion), and also extract the 
