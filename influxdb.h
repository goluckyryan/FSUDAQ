#ifndef INFLUXDB_H
#define INFLUXDB_H

#include <stdio.h>
#include <iostream>
#include <vector>
#include <string>
#include <curl/curl.h>

class InfluxDB{
  private:
    
    bool isURLValid;

    CURL * curl;
    CURLcode respond;
    long respondCode;

    std::string databaseIP;
    std::string dataPoints;
    
    std::vector<std::string> databaseList;
    
    static size_t WriteCallBack(char *contents, size_t size, size_t nmemb, void *userp);
  
    void Execute();
  
  public:
  
    InfluxDB(std::string url, bool verbose = false);
    ~InfluxDB();
    
    void SetURL(std::string url);
    bool TestingConnection();
    bool IsURLValid() const {return isURLValid;}

    /// Query
    std::string CheckDatabases(); /// this save the list of database into databaseList
    std::string Query(std::string databaseName, std::string query);

    /// the CheckDatabases() function must be called before  
    std::vector<std::string> GetDatabaseList() {return databaseList;}

    void CreateDatabase(std::string databaseName);
    
    /// for single or batch write, 
    /// 1, addDataPoint first, you can add as many as you like
    /// 2, writeData.
    void AddDataPoint(std::string fullString);
    unsigned int GetDataLength() const {return dataPoints.length();}
    void ClearDataPointsBuffer();
    void PrintDataPoints();
    void WriteData(std::string databaseName);
    bool IsWriteOK() const {return (respond == CURLE_OK) ? true: false;}

};

#endif
