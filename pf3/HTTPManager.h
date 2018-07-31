#pragma once
#include "globals.h"
#include "TTTypes.h"
#include "ProcessFareList.h"

struct HTTPException : public std::exception
{
    HTTPException(int responseCode, std::string msg) : std::exception(msg.c_str()) {}
};

class HTTPManager
{
    std::vector<std::string> request_;
    BYTE *response;         // passed in - the destination to write any response to the pushed data
    int responseMaxLength;  // passed in - the length of the buffer into which to write data
    int responseLength;     // will be returned by the GetResponse function - indicates the length of the response
    uint64_t filesize;
    bool file;
    std::string filename;
	ProcessFareList fl;
    static const int64_t GetPerfFrequency()
    {
        static LARGE_INTEGER freq;
        QueryPerformanceFrequency(&freq);
        return freq.QuadPart;
    }
    static int64_t perfFreq;
public:
    HTTPManager(BYTE* response, int length) : response(response), responseMaxLength(length), file(false)  {
    }

    virtual ~HTTPManager() {}
    bool PushData(std::vector<BYTE>& input);
    void ProcessCompleteRequest();
    void GenerateJSON(std::string & json, uint64_t elapsed, const FareSearchParams & originalSearchParams, const FoundPlusBus & plusbusResultsMap, const FareResultsMap & fareResultsMap, const std::vector<TTTypes::Journey> journeys) const;
    void ProcessGet(std::string uri);
    bool IsFile() { return file; }
    std::string GetFilename() { return filename; }
    uint64_t GetFileSize() { return filesize; }
    int GetResponseLength() { return responseLength; }
};
