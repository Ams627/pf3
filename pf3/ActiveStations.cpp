#include "stdafx.h"
#include <ams/utility.h>
#include <ams/debugutils.h>
#include <ams/fileutils.h>
#include <ams/codetiming.h>

#include "RJISMaps.h"
#include "ActiveStations.h"
#include "config.h"
#include "RJISAnalyser.h"
#include "RelatedStations.h"
#include "globals.h"

//----------------------------------------------------------------------------
//
// Name: DeGroupIndividualStation
//
// Description: given a station which might or might be a group
//
//----------------------------------------------------------------------------
void DeGroupIndividualStation(std::set<UNLC>& activeStationSet, UNLC nlc, bool decluster = true)
{
    bool clusterFound = false;
    if (decluster)
    {
        // is the NLC a cluster? If it is then add all stations that are members of
        // that cluster:
        auto it = RJISMaps::decluster.find(nlc);
        if (it != RJISMaps::decluster.end())
        {
            // mark the fact that we found a cluster:
            clusterFound = true;
            // yes, it's a cluster, so iterate over the members:
            for (auto clusterMember : it->second)
            {
                // the cluster member might be a group station - search in the group map:
                auto groupit = RJISMaps::degroup.find(clusterMember);
                if (groupit != RJISMaps::degroup.end())
                {
                    // yes, it's a group station, so add all the group members to the active station set:
                    for (auto degroupedStation : groupit->second)
                    {
                        activeStationSet.insert(degroupedStation);
                    }
                }
                else
                {
                    // it's a "real" station (i.e. not a group station)
                    activeStationSet.insert(clusterMember);
                }
            }
        }
    }

    // if we DIDN'T find a cluster then we must check to see if the NLC is a group station:
    if (!clusterFound)
    {
        auto groupit = RJISMaps::degroup.find(nlc);
        if (groupit != RJISMaps::degroup.end())
        {
            // yes, it's a group station, so add all the group members to the active station set
            // (here we are iterating along a vector):
            for (auto degroupedStation : groupit->second)
            {
                activeStationSet.insert(degroupedStation);
            }
        }
        else
        {
            // it's a "real" station:
            activeStationSet.insert(nlc);
        }
    }

}

//----------------------------------------------------------------------------
//
// Name: ActiveStations (constructor)
//
// Description: Build the active stations set. This is a set of all stations
//              either to which or from which there are fares in the FFL, NDF
//              or NFO files. These
//
//----------------------------------------------------------------------------
ActiveStations::ActiveStations()
{
    // for all flows in the main flow map:
    for (auto p : RJISMaps::flowMainFlows)
    {
        // expand all flows with this origin:
        DeGroupIndividualStation(activeStationSet_, p.first.origin);
        // expand all flows with this destination:
        DeGroupIndividualStation(activeStationSet_, p.first.destination);
    }

    PP(activeStationSet_.size());
    // for all NDFs in the NDF map:
    for (auto p : RJISMaps::ndfMain)
    {
        DeGroupIndividualStation(activeStationSet_, p.first.origin);
        DeGroupIndividualStation(activeStationSet_, p.first.destination);
    }

    // for all NFOs in the NFO map:
    for (auto p : RJISMaps::nfoMain)
    {
        DeGroupIndividualStation(activeStationSet_, p.first.origin);
        DeGroupIndividualStation(activeStationSet_, p.first.destination);
    }
}

void ActiveStations::GetList(std::set<UNLC>& activeStationSet) const
{
    activeStationSet = activeStationSet_;
}

struct HandleFolder
{
    HANDLE handle_;
    std::string folder_;
};

typedef std::map<UNLC, HandleFolder> NLCHANDLEFOLDERMAP;

void HandleFileError(const NLCHANDLEFOLDERMAP& nlcToHandleMap, UNLC nlc, BOOL result)
{
    std::string errorString;
    if (!result)
    {
        DWORD error = GetLastError();
        errorString = ams::GetWinErrorAsString(error);
    }
    else
    {
        errorString = "cannot write all bytes";
    }

    auto mapEntry = nlcToHandleMap.at(nlc);
    std::ostringstream oss;
    oss << std::hex << mapEntry.handle_;
    throw QException("Cannot write line to FFL file in folder "s + nlcToHandleMap.at(nlc).folder_ +
        "\nWindows error is " + errorString + "\nHandle is " + oss.str() + " nlc is " + nlc.GetString());

}

//----------------------------------------------------------------------------
//
// Name: CreateFilteredFiles
//
// Description: Create all filtered fares files for low memory mode in a TVM.
//              We create around 3000 sets of files, one for each active station
//              in the UK. The file sets contain only the NDF, NFO and FFL files
//              each of which contains only fares for one origin. Each set is
//              placed in a folder named CCC-NNNN where CCC is the CRS code for
//              the station and NNNN is the NLC code for the station. The names
//              for the FFL, NDF and NFO files are the same in each folder - they 
//              are the original names for those files from the RJIS datafeed
//
//              Note that this requires a powerful server. The filtering process
//              is designed to be fast and as such it uses a lot of resources. In
//              particular it requires around 3000 files to be open at the same time.
//              For this reason we choose raw Windows file handling (CreateFile, 
//              WriteFile etc, since C++ quite often has low limits for the number
//              of std::ofstreams that can be open at the same time. (e.g. around 
//              512)
//
//----------------------------------------------------------------------------
void CreateFilteredFiles(NLCHANDLEFOLDERMAP& nlcToHandleMap, const std::string& filename)
{
    // open the files - we need to use raw windows files as we need to open 3000 or so at a time. The C++
    // library may use cstdio which in the MS C library limits the maximum number of handles to 2048 - see
    // setmaxstdio(...) for details:

    for (auto& p : nlcToHandleMap)
    {
        try
        {
            const auto& folderName = p.second.folder_;
            auto& handle = p.second.handle_;
            std::string fflFullPath(folderName + filename);
            HANDLE hFFL = CreateFile(fflFullPath.c_str(), GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
            if (hFFL == INVALID_HANDLE_VALUE)
            {
                DWORD error = GetLastError();
                throw QException("Cannot open file "s + fflFullPath + " - Windows error was " + ams::GetWinErrorAsString(error));
            }
            // store the handle of the newly opened file in the nlcToHandle map:
            handle = hFFL;
        }
        catch (QException& qex)
        {
            ReportError(qex.what());
        }
    }
}

//----------------------------------------------------------------------------
//
// Name: CloseFilteredFiles
//
// Description: Close all Windows file handles for every open filtered file.
//
//----------------------------------------------------------------------------
void CloseFilteredFiles(const NLCHANDLEFOLDERMAP& nlcToHandleMap)
{
    for (auto& p : nlcToHandleMap)
    {
        CloseHandle(p.second.handle_);
    }
}

//----------------------------------------------------------------------------
//
// Name: WriteFFLFiles
//
// Description: For each flow in the flow map, write that flow to every
//              single-origin flow file to which it applies. This may be
//              for example, 100 files if the flow is for a large cluster. In this
//              case we would write the flow to every single-origin flow file where
//              the destination is a member of the cluster.
//
//              As we iterate over the flow map, we also store a set of flowIDs
//              for each single-origin fare set. This enables us to write the 
//              T-records to the end of each flow file after we have written
//              the F-records.
//
//----------------------------------------------------------------------------
void WriteFFLFiles(const NLCHANDLEFOLDERMAP& nlcToHandleMap)
{
    // A set to store the flowIDs of used T records, per NLC
    std::map<UNLC, std::set<int>> usedFlowIDsPerNLC;

    int count = 0;
    for (auto ffl : RJISMaps::flowMainFlows)
    {
        if (count % 100000 == 99999)
        {
            std::cout << "flow " << count + 1 << "\n";
        }
        // we degroup the origin then decluster it:
        std::set<UNLC> usedNLCs;
        DeGroupIndividualStation(usedNLCs, ffl.first.origin);
        // write the flow record to all flow files:
        for (auto usedNLC : usedNLCs)
        {
            try
            {
                auto it = nlcToHandleMap.find(usedNLC);
                if (it != nlcToHandleMap.end())
                {
                    HANDLE h = it->second.handle_;
                    std::ostringstream oss;
                    oss << "RF" << ffl.first << ffl.second << "\n";
                    std::string s = oss.str();
                    std::vector<char> buffer(std::begin(s), std::end(s));
                    DWORD written;
                    BOOL result = WriteFile(h, buffer.data(), static_cast<DWORD>(buffer.size()), &written, 0);
                    if (!result || written != buffer.size())
                    {
                        HandleFileError(nlcToHandleMap, usedNLC, result);
                    }
                    // if we wrote an F-record, we add the flow ID to the set for the current NLC so that we
                    // can write any related T-records later:
                    usedFlowIDsPerNLC[usedNLC].insert(ffl.second.flowid_);
                }
            }
            catch (QException& qex)
            {
                ReportError(qex.what());
            }
        }
        count++;
    }

    count = 0;
    // for each origin for which there are T-records:
    for (auto originMapEntry : usedFlowIDsPerNLC)
    {
        if (count % 100000 == 99999)
        {
            std::cout << "T-record set entry " << count + 1 << "\n";
        }
        // for each flowID in the list of used flowIDs for the origin:
        for (auto flowid : originMapEntry.second)
        {
            // get all the matching T-records:
            auto matchingTrecords = RJISMaps::flowMainFares.equal_range(flowid);
            // for each T-record in the set of matching T-records:
            for (auto trecord = matchingTrecords.first; trecord != matchingTrecords.second; ++trecord)
            {
                // add the T-records found to the appropriate files:
                HANDLE h = nlcToHandleMap.at(originMapEntry.first).handle_;
                std::ostringstream oss;
                oss << "RT" << std::dec << std::setfill('0') << std::setw(7) << trecord->first << trecord->second << "\n";
                std::string s = oss.str();
                std::vector<char> buffer(std::begin(s), std::end(s));
                DWORD written;
                BOOL result = WriteFile(h, buffer.data(), static_cast<DWORD>(buffer.size()), &written, 0);
                if (!result || written != buffer.size())
                {
                    HandleFileError(nlcToHandleMap, originMapEntry.first, result);
                }
            }
        }
        count++;
    }
}


//----------------------------------------------------------------------------
//
// Name: WriteNDFFiles
//
// Description: Write each single-origin NDF file to its appropriate folder.
//
//----------------------------------------------------------------------------
void WriteNDFFiles(const NLCHANDLEFOLDERMAP& nlcToHandleMap)
{

    int count = 0;
    for (auto ndf : RJISMaps::ndfMain)
    {
        if (count % 100000 == 99999)
        {
            std::cout << "NDF " << count + 1 << "\n";
        }
        
        // we degroup the origin (it might be a group station but it cannot be a cluster)
        std::set<UNLC> usedNLCs;
        DeGroupIndividualStation(usedNLCs, ndf.first.origin, false);

        // write the flow record to all flow files:
        for (auto usedNLC : usedNLCs)
        {
            try
            {
                auto it = nlcToHandleMap.find(usedNLC);
                if (it != nlcToHandleMap.end())
                {
                    HANDLE h = it->second.handle_;
                    std::ostringstream oss;
                    oss << "R" << ndf.first << ndf.second << "\n";
                    std::string s = oss.str();
                    std::vector<char> buffer(std::begin(s), std::end(s));
                    DWORD written;
                    BOOL result = WriteFile(h, buffer.data(), static_cast<DWORD>(buffer.size()), &written, 0);
                    if (!result || written != buffer.size())
                    {
                        HandleFileError(nlcToHandleMap, usedNLC, result);
                    }
                }
            }
            catch (QException& qex)
            {
                ReportError(qex.what());
            }
        }
        count++;
    }
}

//----------------------------------------------------------------------------
//
// Name: WriteNDFFiles
//
// Description: Write each single-origin NFO file to its appropriate folder.
//
//----------------------------------------------------------------------------

void WriteNFOFiles(const NLCHANDLEFOLDERMAP& nlcToHandleMap)
{
    int count = 0;
    for (auto nfo : RJISMaps::nfoMain)
    {
        if (count % 100000 == 99999)
        {
            std::cout << "NFO " << count + 1 << "\n";
        }

        // we degroup the origin (it might be a group station but it cannot be a cluster)
        std::set<UNLC> usedNLCs;
        DeGroupIndividualStation(usedNLCs, nfo.first.origin, false);
        // write the flow record to all flow files:
        for (auto usedNLC : usedNLCs)
        {
            try
            {
                auto it = nlcToHandleMap.find(usedNLC);
                if (it != nlcToHandleMap.end())
                {
                    HANDLE h = it->second.handle_;
                    std::ostringstream oss;
                    oss << "R" << nfo.first << nfo.second << "\n";
                    std::string s = oss.str();
                    std::vector<char> buffer(std::begin(s), std::end(s));
                    DWORD written;
                    BOOL result = WriteFile(h, buffer.data(), static_cast<DWORD>(buffer.size()), &written, 0);
                    if (!result || written != buffer.size())
                    {
                        HandleFileError(nlcToHandleMap, usedNLC, result);
                    }
                }
            }
            catch (QException& qex)
            {
                ReportError(qex.what());
            }
        }
        count++;
    }
}



//----------------------------------------------------------------------------
//
// Name: MakeFilteredSets
//
// Description: 
//  Produces filtered RJIS fares sets, one per origin. Each origin has its own
//  folder under the configurable "filterset" folder. The folder is created  if
//  it does not already exist. A fare set is produced for a station if and only
//  if the station has a CRS code AND it is a member of the active station set.
//  A station is a member of the active station set if there are fares to it or 
//  from it in the NDF, NFO or FFL files. This means that a fare set is not
//  produced for stations from which there are no departures or at which there
//  are no arrivals.
//----------------------------------------------------------------------------

void ActiveStations::MakeFilteredSets()
{
    // get the configurable folder for the filtered RJIS sets
    auto filterSetDir = Config::directories.GetDirectory("filterset");
    if (filterSetDir.empty())
    {
        throw QException("no filteredSet folder specified - filtered fare set production skipped.");
    }

    ams::AddSlashIfMissing(filterSetDir);
    // create the filtered fare set folder (note that we never set the current folder since 
    // this is a multithreaded app and setting the current folder is not permitted)
    PP(filterSetDir);
    if (!ams::CreateDirectoryRec(filterSetDir))
    {
        throw QException("Cannot create filteredSet folder - filtered fare set production skipped.");
    }

    if (activeStationSet_.empty())
    {
        throw QException("Warning: filtered fare set is empty.");
    }

    // get the set numbers for the RJIS filenames - we will use the same set number for the
    // filenames in the subfolder for each station:
    RJISAnalyser rja = RJISAnalyser::GetInstance();
    auto fflSetnumber = rja.GetSetNumber("FFL");
    auto ndfSetnumber = rja.GetSetNumber("NDF");

    // construct the FFL, NFO and NDF filenames:
    std::ostringstream oss;
    oss << "RJFAF" << std::dec << std::setfill('0') << std::setw(3) << fflSetnumber;
    auto fflFilename = oss.str() + ".FFL";
    auto nfoFilename = oss.str() + ".NFO";
    oss.str(""s);
    oss.clear();
    oss << "RJFAF" << std::dec << std::setfill('0') << std::setw(3) << ndfSetnumber;
    auto ndfFilename = oss.str() + ".NDF";

    auto time0 = ams::GetCurrentFiletime();

    std::map<UNLC, HandleFolder> nlcToHandleMap;

    std::set<UNLC> testStationSet = {"5961", "5883", "5882", "5007" }; // 5961 is Dorchester south

    // first of all create a folder for all active stations - an active station is one for which there are any fares at all
    // in either FFL, NFO or NDF:
    for (auto thisNLC : activeStationSet_)
    {
        try
        {
            auto& locit = RJISMaps::locations.find(thisNLC);
            if (locit == RJISMaps::locations.end())
            {
                throw QException("Cannot find NLC "s + thisNLC.GetString() + " in locations file.");
            }

            // check the station has a CRS code - if it does, the folder name is CRS-NLC - e.g.
            // POO-5883 for Poole:
            CRSCode& crs = locit->second.crsCode_;
            if (isalpha(crs[0]) && isalpha(crs[1]) && isalpha(crs[2]))
            {
                // concatenate CRS and NLC to form folder name:
                auto folderName = filterSetDir + crs.GetString() + '-' + thisNLC.GetString();
                if (!CreateDirectory(folderName.c_str(), 0))
                {
                    auto error = GetLastError();
                    if (error != ERROR_ALREADY_EXISTS)
                    {
                        throw QException("Cannot create folder " + folderName + ". Windows error is " + ams::GetWinErrorAsString(error));
                    }
                }
                // report an error if the folder does not exist (this is a second check - and we
                // also check that the folder is actually a directory)
                auto attribs = GetFileAttributes(folderName.c_str());
                if (attribs == INVALID_FILE_ATTRIBUTES || (attribs & FILE_ATTRIBUTE_DIRECTORY) == 0)
                {
                    auto error = GetLastError();
                    throw QException("Cannot create folder " + folderName + ". Windows error is " + ams::GetWinErrorAsString(error));
                }
                // we will create a handle for each NLC code for which we have been able to create a folder
                // we place a zero entry into the map here for the NLC so that we can create files for that NLC later:
                if (folderName.back() != '\\' && folderName.back() != '/')
                {
                    folderName += '\\';
                }
                nlcToHandleMap[thisNLC] = { 0, folderName };
            }
        }
        catch (QException& qex)
        {
            ReportError(qex.what());
        }
    } // for each NLC

    std::cout << "Folders Created\n";

    
    // open the station output files - we open all 3000 of them at once so we can distribute fares
    // quickly to any station output file that requires that fare:
    CreateFilteredFiles(nlcToHandleMap, fflFilename);
    std::cout << "FFL files opened\n";
    WriteFFLFiles(nlcToHandleMap);
    CloseFilteredFiles(nlcToHandleMap);
    std::cout << "FFL files closed\n";

    CreateFilteredFiles(nlcToHandleMap, ndfFilename);
    std::cout << "NDF files opened\n";
    WriteNDFFiles(nlcToHandleMap);
    CloseFilteredFiles(nlcToHandleMap);
    std::cout << "NDF files closed\n";

    CreateFilteredFiles(nlcToHandleMap, nfoFilename);
    std::cout << "NFO files opened\n";
    WriteNFOFiles(nlcToHandleMap);
    CloseFilteredFiles(nlcToHandleMap);
    std::cout << "NFO files closed\n";


    // close all FFL files:

    auto time1 = ams::GetCurrentFiletime();

    auto seconds = (time1 - time0) / 10 / 1000 / 1000;
    PP(seconds);
}