#include "stdafx.h"
#include "RJISTTMaps.h"


namespace RJISTTMaps
{
    std::vector<TTTypes::TrainRun> fullTimetable;
    std::map<CRSCode, std::map<short, std::vector<uint32_t>>> crsMinuteMap;
    
    // mapping between a flow and complete train runs in the full timetable:
    // e.g. given POO-SOU we get a vector of indexes into the timetable. The timetable is
    // itself a vector of TrainRuns.
    std::map<TTTypes::CRSFlow, std::vector<uint32_t>> crsFlowMap;

    // given a CRS flow, get a vector of minutes. There will be one entry in the vector for each departure:
    std::map<TTTypes::CRSFlow, std::vector<TTTypes::MinutesIndex>> crsFlowMinutesMap;

    // mapping between a flow and journey plans - this map is generated as required so it
    // can be used on a TVM. However on a server we could load the entire map
    std::map<TTTypes::CRSFlow, std::vector<TTTypes::TrainCall>> jpMap; 
}