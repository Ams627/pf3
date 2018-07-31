#include "stdafx.h"
#include "LineParsers.h"
#include <ams/fileutils.h>
#include "PrintProgress.h"
#include "RJISMaps.h"
#include "RJISTTMaps.h"

namespace LineParsers
{

HANDLE AddPlusBusNLCFile(std::string filename)
{
    HANDLE event = CreateEvent(0, FALSE, FALSE, 0);
    FileReaderQueue& queue = FileReaderQueue::GetInstance();
    queue.Add(ams::FastLineReader(filename, [](std::string line) {
        static PrintProgress& pp = PrintProgress::GetInstance();
        static int linenumber = 0;
        pp.Print();
        if (line.length() > 0 && line[0] != '/')
        {
            if (line.length() != 8)
            {
                throw QException("plusbus NLC file - line length not equal to 8");
            }
            UNLC mainStationNLC(line, 0);
            UNLC plusbusNLC(line, 4);
            if (RJISMaps::plusbusNLCMap.find(mainStationNLC) != RJISMaps::plusbusNLCMap.end())
            {
                std::cout << mainStationNLC << ":" << plusbusNLC << "\n";
            }
            RJISMaps::plusbusNLCMap[mainStationNLC] = plusbusNLC;
        }
    }, event));

    return event;
}


HANDLE AddPlusBusRestrictionsFile(std::string filename)
{
    HANDLE event = CreateEvent(0, FALSE, FALSE, 0);
    FileReaderQueue& queue = FileReaderQueue::GetInstance();
    queue.Add(ams::FastLineReader(filename, [](std::string line) {
        static PrintProgress& pp = PrintProgress::GetInstance();
        static int linenumber = 0;
        pp.Print();
        if (line.length() > 0 && line[0] != '/')
        {
            if (line.length() != 8)
            {
                throw QException("plusbus restrictions file - line length not equal to 8");
            }
            UFlow flow(line, 0);
            RJISMaps::plusbusRestrictionSet.insert(flow);
        }
    }, event));

    return event;
}


HANDLE AddTicketTypeFile(std::string filename)
{
    HANDLE event = CreateEvent(0, FALSE, FALSE, 0);
    FileReaderQueue& queue = FileReaderQueue::GetInstance();
    queue.Add(ams::FastLineReader(filename, [](std::string line) {
        static PrintProgress& pp = PrintProgress::GetInstance();
        static int linenumber = 0;
        pp.Print();
        if (line.length() == 113 && line[0] == 'R')
        {
            TicketCode key(line, 1);
            RJISTypes::TicketTypeValue value(line, 4);
            RJISMaps::ticketTypes.insert(std::make_pair(key, value));
        }
    }, event));

    return event;
}

HANDLE AddNDFFile(std::string filename)
{
    HANDLE event = CreateEvent(0, FALSE, FALSE, 0);
    FileReaderQueue& queue = FileReaderQueue::GetInstance();
    queue.Add(ams::FastLineReader(filename, [](std::string line) {
        static PrintProgress& pp = PrintProgress::GetInstance();
        static int linenumber = 0;
        pp.Print();
        if (line.length() == 67 && line[0] == 'R')
        {
            UFlow flow;
            flow.Set(line, 1);
            RJISTypes::NDFMainValue value(line, 9);
            value.linenumber = linenumber; // AMS debug
            RJISMaps::ndfMain.insert(std::make_pair(flow, value));
        }
        linenumber++;
    }, event));
    return event;
}


HANDLE AddNFOFile(std::string filename)
{
    HANDLE event = CreateEvent(0, FALSE, FALSE, 0);
    FileReaderQueue& queue = FileReaderQueue::GetInstance();
    static int linenumber = 0;
    queue.Add(ams::FastLineReader(filename, [](std::string line) {
        static PrintProgress& pp = PrintProgress::GetInstance();
        static int linenumber = 0;
        pp.Print();
        if (line.length() == 67 && line[0] == 'R')
        {
            UFlow flow;
            flow.Set(line, 1);
            RJISTypes::NDFMainValue value(line, 9);
            value.linenumber = linenumber; // AMS debug
            RJISMaps::nfoMain.insert(std::make_pair(flow, value));
        }
        linenumber++;
    }, event));
    return event;
}

HANDLE AddRailcardFile(std::string filename)
{
    HANDLE event = CreateEvent(0, FALSE, FALSE, 0);
    FileReaderQueue& queue = FileReaderQueue::GetInstance();
    queue.Add(ams::FastLineReader(filename, [](std::string line) {
        static PrintProgress& pp = PrintProgress::GetInstance();
        static int linenumber = 0;
        pp.Print();
        if (line.length() == 127 && line[0] != '\\')
        {
            RailcardCode code(line, 0);
            RJISTypes::RailcardValue value(line, 3);
            RJISMaps::railcards.insert(std::make_pair(code, value));
        }
        linenumber++;
    }, event));
    return event;
}

// railcard minimum fares file - normally ".RCM". Railcard minimum fares apply to adult
// fares only and the use of the minimum fare is on certain trains only (marked by
// the train restriction):
HANDLE AddRailcardMinFaresFile(std::string filename)
{
    HANDLE event = CreateEvent(0, FALSE, FALSE, 0);
    FileReaderQueue& queue = FileReaderQueue::GetInstance();
    queue.Add(ams::FastLineReader(filename, [](std::string line) {
        static PrintProgress& pp = PrintProgress::GetInstance();
        static int linenumber = 0;
        pp.Print();
        if (line.length() == 30 && line[0] != '\\')
        {
            RJISTypes::RailcardMinKey key(line, 0);
            RJISTypes::RailcardMinValue value(line, 6);
        }
        linenumber++;
    }, event));
    return event;
}


HANDLE AddFFLFile(std::string filename)
{
    HANDLE event = CreateEvent(0, FALSE, FALSE, 0);
    FileReaderQueue& queue = FileReaderQueue::GetInstance();
    queue.Add(ams::FastLineReader(filename, [](std::string line) {
        static PrintProgress& pp = PrintProgress::GetInstance();
        static int linenumber = 0;
        pp.Print();
        // RF indicates a Flow record - do not process usage_code = 'C'
        if (line.length() == 49 && line[0] == 'R' && line[1] == 'F' && line[18] != 'C')
        {
            UFlow flow;
            flow.Set(line, 2);
            RJISTypes::FFLFlowMainValue value(line, 10);
            RJISMaps::flowMainFlows.insert(std::make_pair(flow, value));
            // check if flow valid in both directions:
            if (line[19] == 'R')
            {
                flow.Reverse();
                RJISMaps::flowMainFlows.insert(std::make_pair(flow, value));
            }
        }
        // else we probably have a Fare record:
        else if (line.length() == 22 && line[0] == 'R' && line[1] == 'T')
        {
            int flowid = 0;
            for (auto i = 0u; i < 7; ++i)
            {
                flowid = flowid * 10 + line[2 + i] - '0';
            }
            RJISTypes::FFLFareMainValue value(line, 9);
            RJISMaps::flowMainFares.insert(std::make_pair(flowid, value));
        }
    }, event));
    return event;
}

HANDLE AddClustersFile(std::string filename)
{
    HANDLE event = CreateEvent(0, FALSE, FALSE, 0);
    FileReaderQueue& queue = FileReaderQueue::GetInstance();
    queue.Add(ams::FastLineReader(filename, [](std::string line) {
        static PrintProgress& pp = PrintProgress::GetInstance();
        static int linenumber = 0;
        pp.Print();
        if (line.length() == 25 && line[0] == 'R')
        {
            UNLC clusterID(line, 1);
            UNLC stationNLC(line, 5);
            RJISDate::Range daterange(line, 9);
            RJISMaps::clusters[stationNLC][clusterID].push_back(daterange);
            RJISMaps::decluster[clusterID].push_back(stationNLC);
        }
    }, event));
    return event;
}

HANDLE AddNSDiscountsFile(std::string filename)
{
    HANDLE event = CreateEvent(0, FALSE, FALSE, 0);
    FileReaderQueue& queue = FileReaderQueue::GetInstance();
    queue.Add(ams::FastLineReader(filename, [](std::string line) {
        static PrintProgress& pp = PrintProgress::GetInstance();
        static int linenumber = 0;
        pp.Print();
        if (line.length() == 68 && line[0] == 'R')
        {
            RJISTypes::NSDiscEntry nsd(line, 1);
            RJISMaps::nonStandardDiscounts.push_back(nsd);
        }
    }, event));
    return event;
};

HANDLE AddStandardDiscountsFile(std::string filename)
{
    HANDLE event = CreateEvent(0, FALSE, FALSE, 0);
    FileReaderQueue& queue = FileReaderQueue::GetInstance();
    queue.Add(ams::FastLineReader(filename, [](std::string line) {
        static PrintProgress& pp = PrintProgress::GetInstance();
        static int linenumber = 0;
        pp.Print();
        if (line.length() == 18 && line[0] == 'D')
        {
            RJISTypes::SDiscountKey key(line, 1);
            RJISTypes::SDiscountValue value(line, 4);
            RJISMaps::standardDiscounts.insert(std::make_pair(key, value));
        }
        else if (line.length() == 99 && line[0] == 'S')
        {
            RJISTypes::StatusKey key(line, 1);
            RJISTypes::StatusValue value(line, 12);
            RJISMaps::statusStandardDiscounts.insert(std::make_pair(key, value));
        }
    }, event));
    return event;
};

HANDLE AddLocationsFile(std::string filename)
{
    HANDLE event = CreateEvent(0, FALSE, FALSE, 0);
    FileReaderQueue& queue = FileReaderQueue::GetInstance();
    queue.Add(ams::FastLineReader(filename, [](std::string line) {
        static PrintProgress& pp = PrintProgress::GetInstance();
        pp.Print();
        if (line.length() == 289 && line[0] == 'R' && line[1] == 'L' && line[2] == '7' && line[3] == '0')
        {
            UNLC key(line, 36);
            UNLC group(line, 69);
            if (isalnum(line[75]) && isalnum(line[76]))
            {
                UNLC countyNLC;
                countyNLC.SetCountyCode(line, 75);
                RJISDate::Date enddate(2999, 12, 31);
                RJISMaps::groups[key][countyNLC].push_back(enddate);
                RJISMaps::degroup[countyNLC].push_back(key);
            }
            RJISTypes::LocationLValue value(line, 9);
            RJISMaps::locations.insert(std::make_pair(key, value));
            if (key != group)
            {
                RJISDate::Date enddate(2999, 12, 31);
                RJISMaps::groups[key][group].push_back(enddate);
                RJISMaps::degroup[group].push_back(key);
            }
        }
        else if (line.length() == 27 && line[0] == 'R' && line[1] == 'M')
        {
            // AMS - don't read M-records for the time being - we don't know what they are
            // for. The include funny things like "blandford bus" for which there are no
            // fares
            //             UNLC groupCode(line, 4);
            //             RJISDate::Date endDate(line, 9);
            //             UNLC memberStation(line, 19);
            //             RJISMaps::groups[memberStation][groupCode].push_back(endDate);
            //             RJISMaps::degroup[groupCode].push_back(memberStation);
        }
    }, event));
    return event;
};

HANDLE AddAuxGroupsFile(std::string filename)
{
    HANDLE event = CreateEvent(0, FALSE, FALSE, 0);
    FileReaderQueue& queue = FileReaderQueue::GetInstance();
    queue.Add(ams::FastLineReader(filename, [](std::string line) {
        static PrintProgress& pp = PrintProgress::GetInstance();
        pp.Print();
        if (line.length() == 9 && line[0] == 'R')
        {
            UNLC station(line, 1);
            UNLC groupNLC(line, 5);
            RJISMaps::auxGroups[station].insert(groupNLC);
        }
    }, event));
    return event;
};

HANDLE AddTimetableFile(std::string filename)
{
    HANDLE event = CreateEvent(0, FALSE, FALSE, 0);
    FileReaderQueue& queue = FileReaderQueue::GetInstance();
    queue.Add(ams::FastLineReader(filename, [](std::string line) {
        static int movementNumber = 0;
        static uint32_t currentIndex = 0;
        static TTTypes::TrainRun oneRun;

        static PrintProgress& pp = PrintProgress::GetInstance();
        static int linenumber = 0;
        pp.Print();

        try
        {
            if (line.length() > 15)
            {
                std::string recordType = line.substr(0, 2);
                if (recordType == "BS")
                {
                    std::string trainUID = line.substr(3, 6);
                    RJISDate::Range daterange = TTTypes::GetDateRangeFromBS(line);
                    RJISDate::Dayset dayset(line.substr(21, 7));
                    oneRun.runningDates = daterange;
                    oneRun.runningDays = dayset;
                    oneRun.trainUID = trainUID;
                    oneRun.trainID = line.substr(32, 4);
                    oneRun.stpIndicator = line[79];
                    oneRun.linenumber = linenumber;
                }
                if ((recordType == "LI" || recordType == "LO") && line.substr(10, 4) != "    ")
                {
                    TTTypes::TrainCall tc(line);
                    RJISTTMaps::crsMinuteMap[tc.crs][tc.GetDeparture()].push_back(currentIndex);
                    oneRun.AddCall(tc);
                }
                else if (recordType == "LT")
                {
                    TTTypes::TrainCall tc(line);
                    oneRun.AddCall(tc);
                    RJISTTMaps::crsMinuteMap[tc.crs][tc.GetArrival()].push_back(currentIndex);
                    RJISTTMaps::fullTimetable.push_back(oneRun);

                    for (auto i = 0; i < oneRun.callingAt.size() - 1; i++)
                    {
                        for (auto j = i + 1; j < oneRun.callingAt.size(); j++)
                        {
                            TTTypes::CRSFlow crsflow(oneRun.callingAt[i].crs, oneRun.callingAt[j].crs);
                            RJISTTMaps::crsFlowMap[crsflow].push_back(currentIndex);
                        }
                    }

                    for (auto i = 0; i < oneRun.callingAt.size() - 1; i++)
                    {
                        for (auto j = i + 1; j < oneRun.callingAt.size(); j++)
                        {
                            auto mins = oneRun.callingAt[i].GetDeparture();
                            auto crs = oneRun.callingAt[i].crs;
                            auto destCRS = oneRun.callingAt[j].crs;
                            if (mins == 640 && crs == "POO"s && destCRS == "PKS"s)
                            {
                               // std::cout << "POO->" << destCRS <<" at 10:40 " << linenumber + 1 << "\n";
                            }
                            TTTypes::CRSFlow crsflow(oneRun.callingAt[i].crs, oneRun.callingAt[j].crs);
                            //if (crs == "POO"s && destCRS == "PKS"s)
                            //{
                            //    auto sz = RJISTTMaps::crsFlowMinutesMap[crsflow].size();
                            //    std::cout << "sizeof vector for this flow (" << crsflow.GetString() << ") is " << sz << "\n";
                            //    if (sz < 10)
                            //    {
                            //        std::cout << "line number for sz = " << sz << " is " << linenumber << "\n";
                            //    }
                            //}
                            TTTypes::MinutesIndex mi{ oneRun.callingAt[i].GetDeparture(), currentIndex, static_cast<uint16_t>(i), static_cast<uint16_t>(j) };
                            RJISTTMaps::crsFlowMinutesMap[crsflow].push_back(mi);
                        }
                    }

                    currentIndex++;
                    oneRun.ClearCalls();
                }
            }
            linenumber++;
        }
        catch (std::exception ex)
        {
            std::ostringstream oss;
            oss << "Problem at line " << linenumber + 1 << " " << ex.what();
            throw QException(oss.str());
        }
    }, event));
    return event;
}

void ProcessRailcardRecord(const std::string& line)
{
    RJISMaps::rrMap.emplace(RJISTypes::RestrictionsRRKey(line, 3), RJISTypes::RestrictionsRR(line, 7));
}

void ProcessTimeResRecord(const std::string& line)
{
    RJISMaps::trMap.emplace(RJISTypes::RestrictionsKey(line, 3), RJISTypes::RestrictionsTR(line, 6));
}

// HD record page 47
void ProcessHeaderDateBandRecord(const std::string& line)
{
    RJISTypes::RestrictionsKey rkey(line, 3);
    RJISMaps::hdMap.emplace(rkey, RJISTypes::RestrictionsHD(line, 6, rkey.cf_.IsFuture()));
}



HANDLE AddRestrictionsFile(std::string filename)
{
    HANDLE event = CreateEvent(0, FALSE, FALSE, 0);
    FileReaderQueue& queue = FileReaderQueue::GetInstance();
    queue.Add(ams::FastLineReader(filename, [](std::string line) {
        static PrintProgress& pp = PrintProgress::GetInstance();
        static int linenumber = 0;
        static int dateRecordCount = 0;
        pp.Print();

        // check for the line isn't zero length and not a comment. If it is not a comment, check it's length is at least 3:
        if (line.length() > 0 && line[0] != '/' && line.length() > 3)
        {
            std::string recordType = line.substr(1, 2);
            if (recordType == "RD") // main current/future record
            {
                if (++dateRecordCount > 2)
                {
                    throw QException("more than two RD records in the restrictions file.");
                }
                if (line[3] == 'C')
                {
                    RJISMaps::currentDateRange.Set(line, 4, true);
                }
                else if (line[3] == 'F')
                {
                    RJISMaps::futureDateRange.Set(line, 4, true);
                }
            }
            else if (recordType == "RR") // railcard restriction record
            {
                ProcessRailcardRecord(line);
            }
            else if (recordType == "TR") // time restriction record by two-character restriction code
            {
                ProcessTimeResRecord(line);
            }
            else if (recordType == "HD")
            {
                ProcessHeaderDateBandRecord(line);
            }
        }
    }, event));
    return event;
}

} // namespace