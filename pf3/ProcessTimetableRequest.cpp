#include "stdafx.h"
#include "ProcessTimetableRequest.h"
#include "RJISTTMaps.h"
#include "FareSearchParams.h"

void ProcessTimetableRequest::GetTimes(std::vector<TTTypes::Journey>& timetableResults, const FareSearchParams& searchParams)
{
    std::cout << "timetable request\n";
    // Get the following sets:
    //    1. all timetable entries from the origin to the destination
    //    2. all the timetable entries for the next two hours
    // Then form the intersection of these two sets.
    std::set<int> crsFlowSet, crsOriginSet;
    RJISDate::Date today{ RJISDate::Date::Today() };
    std::cout << "origin: " << searchParams.crsOrigin_.GetString() << "\n";
    TTTypes::CRSFlow crsFlow(searchParams.crsOrigin_.GetString(), searchParams.crsDestination_.GetString());

    std::cout << "crs flow is " << crsFlow.GetString() << "\n";
    // are there any timetable entries for this flow? (result from count() is zero or one):
    if (RJISTTMaps::crsFlowMap.count(crsFlow) > 0)
    {
        std::cout << "found flows\n";
        // for each timetable entry for this flow:
        for (auto p : RJISTTMaps::crsFlowMap[crsFlow])
        {
            auto run = RJISTTMaps::fullTimetable[p];
            // check that the train runs today and on this day of the week and if it does, insert it:
            if (run.runningDates.IsDateInRange(today) && run.runningDays.IsDateInDayset(today))
            {
                crsFlowSet.insert(p);
            }
        }

        // get all trains departing in the next two hours from the origin:
        SYSTEMTIME st;
        GetLocalTime(&st);
        int minutes = (60 * st.wHour) + st.wMinute;
//        int minutes = 600; // 10 am
        for (auto i = 0U; i < 120; ++i)
        {
            auto sz = RJISTTMaps::crsMinuteMap[searchParams.crsOrigin_.GetString()][(i + minutes) % 1440].size();
            for (auto p : RJISTTMaps::crsMinuteMap[searchParams.crsOrigin_.GetString()][(i + minutes) % 1440])
            {
                crsOriginSet.insert(p);
                //auto tr = RJISTTMaps::fullTimetable[p];
                //std::cout << tr.linenumber << std::endl;
                //int jj = 10;
            }
        }

        std::set<int> resultSet;
        auto result = std::set_intersection(crsFlowSet.begin(), crsFlowSet.end(),
            crsOriginSet.begin(), crsOriginSet.end(),
            std::inserter(resultSet, resultSet.begin()));

        TTTypes::Journey oneJourney;
        for (auto p : resultSet)
        {
            auto ttEntry = RJISTTMaps::fullTimetable[p];
            bool started = false;
            for (auto q : ttEntry.callingAt)
            {
                if (started)
                {
                    oneJourney.Add(q.crs, q.GetArrival());
                    if (q.crs == searchParams.crsDestination_.GetString())
                    {
                        timetableResults.push_back(oneJourney);
                        oneJourney.Clear();
                        break;
                    }
                }
                else if ( q.crs == searchParams.crsOrigin_.GetString())
                {
                    started = true;
                    oneJourney.Add(q.crs, q.GetDeparture());
                }
            }
        }
        // AMS - need to change this sort to improve performance since it is actually swapping structs and not pointers to structs:
        std::sort(timetableResults.begin(), timetableResults.end(), [](const TTTypes::Journey& j1, const TTTypes::Journey&j2) {return j1.GetFirst().time < j2.GetFirst().time;});
    }
}
