#pragma once
#include "TTTypes.h"

namespace RJISTTMaps
{
    extern std::vector<TTTypes::TrainRun> fullTimetable;
    extern std::map<CRSCode, std::map<short, std::vector<uint32_t>>> crsMinuteMap;
    extern std::map<TTTypes::CRSFlow, std::vector<uint32_t>> crsFlowMap;
    extern std::map<TTTypes::CRSFlow, std::vector<TTTypes::MinutesIndex>> crsFlowMinutesMap;
}