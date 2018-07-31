#include "stdafx.h"
#include "RJISMaps.h"

namespace RJISMaps
{
    std::multimap<UFlow, RJISTypes::NDFMainValue> ndfMain;          
    std::multimap<UFlow, RJISTypes::NDFMainValue> nfoMain;
    std::multimap<UFlow, RJISTypes::FFLFlowMainValue> flowMainFlows; // multi as several flows for the same date
    std::multimap<int, RJISTypes::FFLFareMainValue> flowMainFares;   // T-records from the FFL file
    std::multimap<TicketCode, RJISTypes::TicketTypeValue> ticketTypes; // .TTY file
    std::map<UNLC, std::map<UNLC, std::vector<RJISDate::Range>>> clusters; // .FSC file
    std::map<UNLC, std::vector<UNLC>> decluster;                           // get all stations given a cluster
    std::multimap<RailcardCode, RJISTypes::RailcardValue> railcards;
    std::multimap<RJISTypes::RailcardMinKey, RJISTypes::RailcardMinValue> railcardMinFares;
    std::multimap<RJISTypes::SDiscountKey, RJISTypes::SDiscountValue> standardDiscounts;
    std::multimap<RJISTypes::StatusKey, RJISTypes::StatusValue> statusStandardDiscounts;
    std::multimap<UNLC, RJISTypes::LocationLValue> locations;
    std::map<UNLC, std::map<UNLC, std::vector<RJISDate::Date>>> groups;   // given a station, get a list of groups with dateranges for each station
    std::map<UNLC, std::set<UNLC>> auxGroups;   // given a station, get a list of aux groups to which the station belongs
    std::map<UNLC, std::vector<UNLC>> degroup;                            // given a group station, e.g. a group like 1072, find all the stations in the group
    std::deque<RJISTypes::NSDiscEntry> nonStandardDiscounts;

    // non-standard discounts are double-keyed (by origin and destination), so we create two 
    // multimaps which are indexes into the above container. If we give an origin, we will get a
    // range of indices into the nonStandardDiscounts
    std::multimap<UNLC, decltype(nonStandardDiscounts)::iterator> nsdOriginIndex, nsdDestinationIndex;

    std::map<UNLC, UNLC> plusbusNLCMap;             // given an NLC get a corresponding plusbus NLC
    std::set<UFlow> plusbusRestrictionSet;          // flows for which PB is not allowed

    // restrictions - 19 different record types:
    RJISDate::Range currentDateRange, futureDateRange;
    std::multimap<RJISTypes::RestrictionsRRKey, RJISTypes::RestrictionsRR> rrMap;
    std::multimap<RJISTypes::RestrictionsKey, RJISTypes::RestrictionsTR> trMap;
    std::multimap<RJISTypes::RestrictionsKey, RJISTypes::RestrictionsHD> hdMap;

    // get the earliest possible date in one of the date ranges given a month and a year and a range in which the date
    // must be present. This means y is either the year of the earliest date in the range or the following year. If there is no
    // valid date within the range at all this is an error WHICH WE DO NOT DETECT
    inline RJISDate::Date GetEarliestDateInRange(int min, int din, const RJISDate::Range& range)
    {
        std::cout << "date range (end date first)... ";
        range.DumpDates(std::cout);
        std::cout.put('\n');
        std::cout << "getting earliest date for month " << min << " day " << din << "\n";
        RJISDate::Date result;
        int y, m, d;
        range.GetStartDate().GetYMD(y, m, d);
        if (range.IsDateInRange(y, min, din))
        {
            result = RJISDate::Date(y, min, din);
        }
        else
        {
            result = RJISDate::Date(y + 1, min, din);
            if (!range.IsDateInRange(result))
            {
                throw QException("Cannot find (M,D)=(" + std::to_string(min) + ", " + std::to_string(din) + ") in current or future date range");
            }
        }
        return result;
    }
    
    void AdjustHDRecords()
    {
        int y, m, d;
        for (auto& p : hdMap)
        {
            bool future = p.first.cf_.IsFuture();
            try
            {
                if (future)
                {
                    std::cout << "future\n    start date\n";
                    p.second.dateRange_.GetStartDate().GetYMD(y, m, d);
                    p.second.dateRange_.SetStartDate(GetEarliestDateInRange(m, d, futureDateRange));
                    std::cout << "    end date\n";
                    p.second.dateRange_.GetEndDate().GetYMD(y, m, d);
                    p.second.dateRange_.SetEndDate(GetEarliestDateInRange(m, d, futureDateRange));
                }
                else
                {
                    std::cout << "current\n    start date\n";
                    p.second.dateRange_.GetStartDate().GetYMD(y, m, d);
                    p.second.dateRange_.SetStartDate(GetEarliestDateInRange(m, d, currentDateRange));
                    std::cout << "    end date\n";
                    p.second.dateRange_.GetEndDate().GetYMD(y, m, d);
                    p.second.dateRange_.SetEndDate(GetEarliestDateInRange(m, d, currentDateRange));
                }
            }
            catch (QException qe)
            {
                std::cout << (future ? "future\n" : "current\n");
                p.second.dateRange_.DumpDates(std::cout);
            }
        }
    }


};