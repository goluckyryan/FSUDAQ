#ifndef INFLUXDB_H
#define INFLUXDB_H

#include <stdio.h>
#include <iostream>
#include <vector>
#include <string>
#include <curl/curl.h>

class InfluxDB{
  private:

    CURL * curl;
    CURLcode respond;
    long respondCode;

    std::string databaseIP;
    std::string dataPoints;
    std::string token;

    struct curl_slist * headers;

    std::vector<std::string> databaseList;

    unsigned short influxVersion;
    std::string influxVersionStr;

    bool connectionOK;
    
    static size_t WriteCallBack(char *contents, size_t size, size_t nmemb, void *userp);
  
    void Execute();
  
  public:
  
    InfluxDB(std::string url, bool verbose = false);
    InfluxDB();
    ~InfluxDB();
    
    void SetURL(std::string url);
    void SetToken(std::string token);
    bool TestingConnection(bool debug = false);
    bool IsConnectionOK() const {return connectionOK;}

    unsigned short GetVersionNo() const {return influxVersion;}
    std::string GetVersionString() const {return influxVersionStr;}

    /// Query, query will be in CSV format
    std::string CheckInfluxVersion(bool debug = false);
    std::string CheckDatabases(); /// this save the list of database into databaseList
    void PrintDataBaseList();
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
