#include "stdafx.h"
#include "JourneyPlanner.h"
#include "config.h"
#include <ams/fileutils.h>
#include "RJISTTMaps.h"

#pragma optimize("", off)
// all journey plans available - this would be the result of reading many jXXX.plan files. We can
// access this map using allPlans[crsOrigin][crsDestination]. We include this as it may be beneficial
// to load all plans when running on a powerful server. When running on a TVM we will only load the
// plans for the current origin or other origins when we are instructed
std::map<CRSCode, std::map<CRSCode, OneOrigDestPlanlist>> allplans;



// process a single line from a jXXX.plan file. Each line contains an origin, a destination and a number of
// plans. Each plan on the line is separated by a vertical bar character. The origin at the start of the line
// is redundant in the current implementation but may be used on a powerful server in the future if all the
// journey planning information is contained within a single file. Comment lines start with
// two slashes as in C++. Comments are not allowed within plan lines. The first four characters
// of a line MUST be a crs code followed by the colon character:
void LineFunc(std::string line)
{
    try
    {
        ams::Trim(line);
        // add an end of line marker - this is used in the line processing state machine below:
        line += '\x1';

        static int linenumber = 0;
        // allow double-forward-slash comments:
        if (line.length() > 2 && line[0] != '/' && line[1] != '/')
        {
            // two CRS codes followed by a colon must be at the start of a line:
            if (line[6] != ':')
            {
                throw QException("error at line "s + std::to_string(linenumber + 1) + " colon expected at column 7 - line ignored.");
            }
            CRSCode origin(line);
            CRSCode destination(line, 3);
            OneOrigDestPlanlist onedestPlanlist;

            Oneplan oneplan;
            enum class States { Init, InCRS, GotCRS } state(States::Init);

            std::string currentCRS;
            for (auto i = 7u; i < line.length(); ++i)
            {
                auto c = line[i];
                if (state == States::Init)
                {
                    if (isupper(c))
                    {
                        state = States::InCRS;
                        currentCRS += c;
                    }
                    else if (!isspace(c))
                    {
                        if (c == 1)
                        {
                            throw QException("unexpected end of line at line "s + std::to_string(linenumber + 1) + " - line ignored.");
                        }
                        else
                        {
                            throw QException("unexpected character: "s + c + " at line " + std::to_string(linenumber + 1) + " - line ignored.");
                        }
                    }
                }
                else if (state == States::InCRS)
                {
                    if (isupper(c))
                    {
                        currentCRS += c;
                        if (currentCRS.length() > 3)
                        {
                            throw QException("unexpected character: "s + c + " at line " + std::to_string(linenumber + 1) + " - line ignored.");
                        }
                    }
                    else if (currentCRS.length() != 3)
                    {
                        if (c == 1)
                        {
                            throw QException("unexpected end of line."s + " at line " + std::to_string(linenumber + 1) + " - line ignored.");
                        }
                        else
                        {
                            std::string errorString;
                            if (isspace(c))
                            {
                                errorString = "unexpected space or tab";
                            }
                            else
                            {
                                errorString = "unexpected character: "s + c;
                            }
                            throw QException(errorString + " at line " + std::to_string(linenumber + 1) + " line ignored.");
                        }
                    }
                    else if (isspace(c))
                    {
                        oneplan.push_back(currentCRS);
                        currentCRS.clear();
                        state = States::GotCRS;
                    }
                    else if (c == '|')
                    {
                        oneplan.push_back(currentCRS);
                        currentCRS.clear();
                        onedestPlanlist.plans.push_back(oneplan);
                        oneplan.clear();
                        state = States::Init;
                    }
                    else if (c == 1)
                    {
                        // end of line here and we must have a 3-character CRS
                        oneplan.push_back(currentCRS);
                        onedestPlanlist.plans.push_back(oneplan);
                    }
                    else
                    {
                        throw QException("unexpected character: "s + c + " at line " + std::to_string(linenumber + 1) + " - line ignored.");
                    }
                }
                else if (state == States::GotCRS) // we have at least one CRS already and we have just had a separator
                {
                    if (isupper(c))
                    {
                        currentCRS.push_back(c);
                        state = States::InCRS;
                    }
                    else if (c == '|')
                    {
                        onedestPlanlist.plans.push_back(oneplan);
                        oneplan.clear();
                        state = States::Init;
                    }
                    else if (c == 1)
                    {
                        // end of line - do nothing here
                    }
                    else if (c != ' ')
                    {
                        throw QException("unexpected character: "s + c + " at line " + std::to_string(linenumber + 1) + " - line ignored.");
                    }
                }
            }
            allplans[origin][destination] = onedestPlanlist;
        }
        linenumber++;
    }
    catch (std::exception ex)
    {
        std::cerr << "ERROR: " << ex.what() << "\n";
    }
}



// load a jXXX.plan file into the journey planning map (where XXX is a CRS code)
void JourneyPlanner::LoadPlanFile(CRSCode crs)
{
    auto filename = Config::directories.GetDirectory("aux") + "/jp/j" + crs.GetString() + ".plan";
    std::cout << "loading plan file " << filename << "\n";
    ams::FastLineReader(filename, LineFunc).Read();
}

// get the first train running from the specified origin to destination after the specified time. The time is specified as a number of minutes from 
// midnight. It is known that the origin-destination pair given are on a direct line. A call to this function thus gets one
// leg of a journey plan:
short JourneyPlanner::GetSingleLegTimes(CRSCode origin, CRSCode destination, short minutes)
{
    std::cout << __FUNCTION__ << "o: " << origin << " d: " << destination << " time: " << TTTypes::GetTimeFromMinutes(minutes) << "\n";
    short result = -1;
    RJISDate::Date testMonday(2015, 11, 23);

    // get all the timetable entries for this flow
    auto minutesIndex = RJISTTMaps::crsFlowMinutesMap[TTTypes::CRSFlow(origin, destination)];

    TTTypes::MinutesIndex firstPossibleMinute(minutes);
    auto firstTrain = std::upper_bound(minutesIndex.begin(),
                                       minutesIndex.end(), 
                                       firstPossibleMinute,
        [](const auto &x, const auto&y) {return x.minutes < y.minutes;});

    std::cout << "first train is at:" << TTTypes::GetTimeFromMinutes(firstTrain->minutes) << "\n";

    if (firstTrain->minutes == 803)
    {
        firstTrain = firstTrain;
    }

    // scan along the index entries return for this origin and destination. When we find a train that departs later
    // than the time we specified, then we 
    for (auto index = firstTrain; index != minutesIndex.end(); index++)
    {
        // get the details of the train run from the timetable:
        auto run = RJISTTMaps::fullTimetable[index->index];

        // check that the train is running on the requested travel date and that the day of the week of the travel date is in the set of running
        // days (Monday-Sunday) for this train.
        if (run.runningDates.IsDateInRange(testMonday) && run.runningDays.IsDateInDayset(testMonday))
        {
            std::cout << "Train: " << run.trainID <<
                " line: " << run.linenumber + 1 <<
                " from: " << run.callingAt.front().crs <<
                " to: " << run.callingAt.back().crs <<
                "\n";
            std::cout << origin << " depart: " << TTTypes::GetTimeFromMinutes(index->minutes) << "\n";
            auto arrival = run.callingAt[index->subIndex2].GetPublicArrival();
            std::cout << destination << " arrive: " << TTTypes::GetTimeFromMinutes(arrival) << "\n";
            result = arrival;
            // we found a train so exit the loop
            break;
        }
    }
    return result;
}

// Print a journey plan - origin destination are CRS codes. earliest minutes if the earliest time of the first leg of the
// journey:
void JourneyPlanner::GetPlan(const CRSCode origin, const CRSCode destination, const short earliestMinutes)
{
    LoadPlanFile(origin);
    // for each separate journey plan in the plan list for this o-d pair:
    for (auto plan : allplans[origin][destination].plans)
    {
        auto currentOrigin = origin;
        auto currentMinutes = earliestMinutes;

        // for each station in a single journey plan:
        for (auto crs : plan)
        {
            // add ten to returned minutes to allow for change time:
            currentMinutes = 10 + GetSingleLegTimes(currentOrigin, crs, currentMinutes);
            currentOrigin = crs;
        }

        GetSingleLegTimes(currentOrigin, destination, currentMinutes);
        std::cout << "--- End of plan ---\n";
    }
}
#pragma optimize("", on)
