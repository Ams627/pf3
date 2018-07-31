#pragma once
#include "globals.h"
namespace LineParsers
{
    HANDLE AddPlusBusNLCFile(std::string filename);
    HANDLE AddPlusBusRestrictionsFile(std::string filename);
    HANDLE AddTicketTypeFile(std::string filename);
    HANDLE AddNDFFile(std::string filename);
    HANDLE AddNFOFile(std::string filename);
    HANDLE AddRailcardFile(std::string filename);
    HANDLE AddFFLFile(std::string filename);
    HANDLE AddClustersFile(std::string filename);
    HANDLE AddNSDiscountsFile(std::string filename);
    HANDLE AddStandardDiscountsFile(std::string filename);
    HANDLE AddLocationsFile(std::string filename);
    HANDLE AddAuxGroupsFile(std::string filename);
    HANDLE AddTimetableFile(std::string filename);
    HANDLE AddRestrictionsFile(std::string filename);
};
