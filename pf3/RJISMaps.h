#pragma once
#include "RJISTypes.h"

// RJIS Maps and Indexes:
namespace RJISMaps
{
    extern std::multimap<UFlow, RJISTypes::NDFMainValue> ndfMain;
    extern std::multimap<UFlow, RJISTypes::NDFMainValue> nfoMain;
    extern std::multimap<UFlow, RJISTypes::FFLFlowMainValue> flowMainFlows;         // multi as several flows for the same date
    extern std::multimap<int, RJISTypes::FFLFareMainValue> flowMainFares;
    extern std::multimap<TicketCode, RJISTypes::TicketTypeValue> ticketTypes;
    extern std::map<UNLC, std::map<UNLC, std::vector<RJISDate::Range>>> clusters;   // given a station, get a list of clusters with dateranges for each membership
    extern std::map<UNLC, std::vector<UNLC>> decluster;                             // given an cluster, get a list of stations:
    extern std::multimap<RailcardCode, RJISTypes::RailcardValue> railcards;
    extern std::multimap<RJISTypes::RailcardMinKey, RJISTypes::RailcardMinValue> railcardMinFares;
    extern std::multimap<RJISTypes::SDiscountKey, RJISTypes::SDiscountValue> standardDiscounts;
    extern std::multimap<RJISTypes::StatusKey, RJISTypes::StatusValue> statusStandardDiscounts;
    extern std::multimap<UNLC, RJISTypes::LocationLValue> locations;
    extern std::map<UNLC, std::map<UNLC, std::vector<RJISDate::Date>>> groups;      // given a station, get a list of groups with dateranges for each station
    extern std::map<UNLC, std::set<UNLC>> auxGroups;                                // given a station, get a list of aux groups to which the station belongs
    extern std::map<UNLC, std::vector<UNLC>> degroup;                               // given a group station, e.g. a group like 1072, find all the stations in the group
    extern std::deque<RJISTypes::NSDiscEntry> nonStandardDiscounts;
    extern std::deque<RJISTypes::NSDiscEntry> nonStandardDiscounts;
    extern std::multimap<UNLC, decltype(nonStandardDiscounts)::iterator> nsdOriginIndex, nsdDestinationIndex;
    extern std::map<UNLC, UNLC> plusbusNLCMap; // given an NLC get a corresponding plusbus NLC
    extern std::set<UFlow> plusbusRestrictionSet;                                  // flows for which PB is not allowed

    extern void AdjustHDRecords();
    // restrictions - 19 different record types:
    extern RJISDate::Range currentDateRange, futureDateRange;
    extern std::multimap<RJISTypes::RestrictionsRRKey, RJISTypes::RestrictionsRR> rrMap;
    extern std::multimap<RJISTypes::RestrictionsKey, RJISTypes::RestrictionsTR> trMap;
    extern std::multimap<RJISTypes::RestrictionsKey, RJISTypes::RestrictionsHD> hdMap;

};
