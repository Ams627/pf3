#pragma once
#include "FareSearchParams.h"
#include "TTTypes.h"

class ProcessTimetableRequest
{
public:
    ProcessTimetableRequest() {}
    virtual ~ProcessTimetableRequest(){}
    //    void GetTimes(std::vector<TTTypes::Journey> & timetableResults, const FareSearchParams & searchParams);
    void GetTimes(std::vector<TTTypes::Journey>& timetableResults, const FareSearchParams& searchParams);
};
