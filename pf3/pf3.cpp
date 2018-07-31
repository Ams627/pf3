#include "stdafx.h"
#include <Psapi.h>
#pragma comment(lib, "Psapi.lib") 
#include <ams/fileutils.h>
#include <ams/athread.h>
#include <ams/codetiming.h>
#include "msgthread.h"
#include "RJISTypes.h"
#include "RJISMaps.h"
#include "RJISTTMaps.h"
#include "RJISAnalyser.h"
#include "globals.h"
#include "PrintProgress.h"
#include "ReaderThreads.h"
#include "ProcessFareList.h"
#include "ServerManagement.h"
#include "ActiveStations.h"
#include "ReadIDMS.h"
#include "config.h"
#include "globals.h"
#include "ProcessFareList.h"
#include "ProcessTimetableRequest.h"
#include "LineParsers.h"
#include "JourneyPlanner.h"

namespace LP = LineParsers; // namespace alias

int main(int argc, char**argv)
{
    auto ft1 = ams::GetCurrentFiletime();
    try
    {
        // Check that we are not already running:
        HANDLE hMutex = CreateMutex(NULL, TRUE, "Global\\F14F1B76-6A31-44AD-A5B5-20B5886746F8");
        if (hMutex && GetLastError() == ERROR_ALREADY_EXISTS)
        {
            throw QException("Powerfares is already running.");
        }

        //std::cout << "About to start logging\n";
        // start the logging thread:
//        MsgThread::GetInstance();
        // std::cout << "logging started\n";

        // Get the configuration for the app (location of data directories etc.)
        Config::Config& config = Config::Config::GetInstance();
        config.StoreArgv(argc, argv);
        config.Read("pfconfig.xml");

        // set the folder for RJIS fares files:
        std::string rjisDir = Config::directories.GetDirectory("rjis");
        // AMS - can't do this - SCD is not multithreaded
        SetCurrentDirectory(rjisDir.c_str());

        // analyse the RJIS file set to get the set numbers of the files:
        RJISAnalyser& rja = RJISAnalyser::GetInstance();
        int ndfSet = rja.GetSetNumber("NDF");
        if (rja.GetNumberOfSets() == 0)
        {
            throw QException("No RJIS files to process in the directory " + g_workingDirectory);
        }
        else if ((rja.GetNumberOfSets() == 2 && rja.GetNumberOfFilesInSet(ndfSet) != 1) || rja.GetNumberOfSets() > 2)
        {
            throw QException("Too many different RJIS sets in folder " +
                g_workingDirectory + ": " + rja.GetFileSetsAsString() +
                "\n(one NDF file from a different set is permitted.)"
                );
        }

        std::string ndfFilename = rja.GetFilename("NDF");       // non-derivable fares
        std::string nfoFilename = rja.GetFilename("NFO");       // non-derivable fares override
        std::string fflFilename = rja.GetFilename("FFL");       // flows
        std::string ttyFilename = rja.GetFilename("TTY");       // ticket types
        std::string clustersFilename = rja.GetFilename("FSC");  // clusters
        std::string railcardsFilename = rja.GetFilename("RLC"); // railcards
        std::string nsdFilename = rja.GetFilename("FNS");       // non-standard discounts
        std::string disFilename = rja.GetFilename("DIS");       // standard discounts
        std::string locFilename = rja.GetFilename("LOC");       // locations file
        std::string agsFilename = rja.GetFilename("AGS");       // aux groups file - not part of RJIS
        std::string rcardMinFilename = rja.GetFilename("RCM");  // railcard minimum fares filename (RSPS5045 p40)
        std::string restrictFilename = rja.GetFilename("RST");  // restrictions filename (RSPS5045 p45)

        // AMS need to arrange to get the name of this properly:
        std::string timetableFile = "s:\\ttisf968.mca";         // timetable file

        std::string auxDir = Config::directories.GetDirectory("aux");
        std::string plusbusNLCFilename = auxDir + "/" + "PFAUX.PLUSBUSNLC";
        std::string plusbusRestrictions = auxDir + "/" + "PFAUX.PLUSBUSRESTRICT";

        std::vector<HANDLE> events;

        // Each Lineparser function (LP::f) below adds a filename and a method to parse a line from that
        // file to a queue. We then start a number of threads to process the queue (equivalent to the number
        // of cores in the CPU).
        events.push_back(LP::AddPlusBusNLCFile(plusbusNLCFilename));
        events.push_back(LP::AddPlusBusRestrictionsFile(plusbusRestrictions));
        // optional auxiliary groups file:
        if (!agsFilename.empty())
        {
            //events.push_back(AddAuxGroupsFile(agsFilename));
        }
        events.push_back(LP::AddNDFFile(ndfFilename));
        //events.push_back(LP::AddNFOFile(nfoFilename));
        //events.push_back(LP::AddTimetableFile(timetableFile));
        //events.push_back(LP::AddFFLFile(fflFilename));
        //events.push_back(LP::AddTicketTypeFile(ttyFilename));
        //events.push_back(LP::AddClustersFile(clustersFilename));
        //events.push_back(LP::AddRailcardFile(railcardsFilename));
        //events.push_back(LP::AddNSDiscountsFile(nsdFilename));
        //events.push_back(LP::AddStandardDiscountsFile(disFilename));
        events.push_back(LP::AddLocationsFile(locFilename));
        //events.push_back(LP::AddRestrictionsFile(restrictFilename));

        // start threads and wait for them to terminate. When threads go out of scope they will terminate themselves:
        {
            ReaderThreads rt;
            WaitForMultipleObjects(static_cast<DWORD>(events.size()), events.data(), TRUE, INFINITE);
        }

        //std::cout << "number of restrictions RR records is " << RJISMaps::rrMap.size() << std::endl;
        //auto range = RJISMaps::rrMap.equal_range(RJISTypes::RestrictionsRRKey('F', "YNG"));
        //for (auto p = range.first; p != range.second; ++p)
        //{
        //    std::cout << p->second.ticketCode_ << " " << p->second.restrictionCode_ << "\n";
        //}

        //std::cout << "number of restrictions TR records is " << RJISMaps::trMap.size() << std::endl;
        //auto range2 = RJISMaps::trMap.equal_range(RJISTypes::RestrictionsKey('F', "R1"));
        //for (auto p = range2.first; p != range2.second; ++p)
        //{
        //    std::cout << TTTypes::GetTimeFromMinutes(p->second.minutesFrom_) << " " << TTTypes::GetTimeFromMinutes(p->second.minutesTo_) << "\n";
        //}

        //RJISMaps::AdjustHDRecords();

        //std::cout << "number of restrictions HD records is " << RJISMaps::hdMap.size() << std::endl;
        //auto range3 = RJISMaps::hdMap.equal_range(RJISTypes::RestrictionsKey('F', "R1"));
        //for (auto p = range3.first; p != range3.second; ++p)
        //{
        //    p->second.dateRange_.DumpDates(std::cout);
        //    p->second.days_.DumpDays(std::cout);
        //}


        // create additional indices for the non-standard discounts table. We need these since the table can
        // include wildcards:
        for (auto p = RJISMaps::nonStandardDiscounts.begin(); p != RJISMaps::nonStandardDiscounts.end(); ++p)
        {
            RJISMaps::nsdOriginIndex .insert(std::make_pair(p->originCode_, p));
            RJISMaps::nsdDestinationIndex.insert(std::make_pair(p->destinationCode_, p));
        }

        // remove flow records for which there are no fares:
        for (auto p = std::begin(RJISMaps::flowMainFlows); p != std::end(RJISMaps::flowMainFlows);)
        {
            if (RJISMaps::flowMainFares.find(p->second.flowid_) == RJISMaps::flowMainFares.end())
            {
                RJISMaps::flowMainFlows.erase(p++);
            }
            else
            {
                p++;
            }
        }

        // remove plusbus nlcs from m-records:

        //std::set<UNLC> pbNLCs;
        //for (auto p : RJISMaps::plusbusNLCMap)
        //{
        //    pbNLCs.insert(p.second);
        //}

        //for (auto p : RJISMaps::g)


        long linenumber = PrintProgress::GetInstance().GetLinenumber();
        std::cout << linenumber + 1 << " lines                      \n";
        //std::cout << "main flow map size is: " << RJISMaps::flowMainFlows.size() << "\n";
        //std::cout << "main fare map size is: " << RJISMaps::flowMainFares.size() << "\n";
        //std::cout << "main ndf map size is: " << RJISMaps::ndfMain.size() << "\n";
        //std::cout << "main nfo map size is: " << RJISMaps::nfoMain.size() << "\n";
        //std::cout << "main tty map size is: " << RJISMaps::ticketTypes.size() << "\n";
        //std::cout << "main cluster map size is: " << RJISMaps::clusters.size() << "\n";
        //std::cout << "main railcard map size is: " << RJISMaps::railcards.size() << "\n";
        //std::cout << "main non-standard discounts map size is: " << RJISMaps::nonStandardDiscounts.size() << "\n";
        //std::cout << "main standard discounts D-record map size is: " << RJISMaps::standardDiscounts.size() << "\n";
        //std::cout << "main standard discounts S-record map size is: " << RJISMaps::statusStandardDiscounts.size() << "\n";
        //std::cout << "main locations map size is: " << RJISMaps::locations.size() << "\n";
        //std::cout << "main group map size is: " << RJISMaps::groups.size() << "\n";
        //std::cout << "main aux map size is: " << RJISMaps::auxGroups.size() << "\n";

        //std::cout << "plusbus NLCs: " << RJISMaps::plusbusNLCMap.size() << "\n";
        //std::cout << "plusbus restrictions: " << RJISMaps::plusbusRestrictionSet.size() << "\n";

        std::cout << "crsFlowMap size is: " << RJISTTMaps::crsFlowMap.size() << "\n";
        std::cout << "crsFlowMinutesMap size is: " << RJISTTMaps::crsFlowMinutesMap.size() << "\n";



        std::cout << "sorting crsFlowMinutesMap!\n";
        for (auto& p : RJISTTMaps::crsFlowMinutesMap)
        {
            std::sort(p.second.begin(), p.second.end(), [](const auto& x, const auto& y) {return x.minutes < y.minutes;});
        }
        std::cout << "sorted!\n";
        std::cout << "finished!\n";

        std::set<UNLC> activeStationSet;
        ActiveStations as;
        as.GetList(activeStationSet);
        //std::cout << "found " << activeStationSet.size() << "\n";
        std::string idmsDir = Config::directories.GetDirectory("idms");
        std::string webDir = Config::directories.GetDirectory("docroot");
        ReadIDMS ridms(webDir, idmsDir);
        ridms.WriteJSON("locations.js", activeStationSet);

        HANDLE h = GetCurrentProcess();
        PROCESS_MEMORY_COUNTERS_EX counters;
        DWORD cb = sizeof(counters);
        GetProcessMemoryInfo(h, (PROCESS_MEMORY_COUNTERS*)&counters, cb);
        std::cout << "Non-paged pool usage in megabytes is: " << counters.PrivateUsage / 1024 / 1024 << std::endl;

        auto ft2 = ams::GetCurrentFiletime();
        std::cerr << "seconds: " << (ft2 - ft1) / 10'000'000.0 << "\n";

        //ProcessTimetableRequest ttreq;
        //RJISDate::Date today{ RJISDate::Date::Today() };
        //auto tomorrow = today + 1;
        //TTTypes::CRSFlow flow("POO"s, "PKS"s);
        //auto minutesList = RJISTTMaps::crsFlowMinutesMap[flow];
        //std::cout << "flow is " << flow.GetString() << "\n";

        //std::sort(minutesList.begin(), minutesList.end(), [](const auto &x1, const auto&x2) {return x1.minutes < x2.minutes;});
        //for (auto minuteIndex : minutesList)
        //{
        //    auto run = RJISTTMaps::fullTimetable[minuteIndex.index];
        //    if (run.runningDates.IsDateInRange(tomorrow) && run.runningDays.IsDateInDayset(tomorrow))
        //    {
        //        std::cout << TTTypes::GetTimeFromMinutes(minuteIndex.minutes) << "\n";
        //    }
        //}

        // JourneyPlanner jp;
        // jp.GetPlan("PNZ", "ABD", 9 * 60 + 55);

        // if we specify the -filter option, then this program does RJISPrefilter instead of starting a server:
        if (config.CheckArg("-filter"))
        {
            as.MakeFilteredSets();
        }
        else
        {
            StartServer(80);
            // this thread (the main one) should wait forever for connections:
            Sleep(INFINITE);
        }
	}
    catch (std::exception &ex)
    {
        std::cerr << ex.what() << std::endl;
    }
}

