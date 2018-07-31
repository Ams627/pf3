#pragma once
#include "TiplocToNLC.h"

// types and utility functions for storing RSPS5046 timetable information:

namespace TTTypes
{
    inline std::string GetTimeFromMinutes(int minutes)
    {
        std::ostringstream oss;
        auto hours = minutes / 60;
        oss << std::dec << std::setfill('0') << std::setw(2) << hours << ":" << std::setw(2) << minutes - hours * 60;
        return oss.str();
    }

    inline RJISDate::Range GetDateRangeFromBS(std::string line)
    {
        #pragma loop(no_vector)
        for (int i = 0; i < 12; ++i)
        {
            if (!isdigit(line[i + 9]))
            {
                throw QException(std::string("") + "invalid character '" + line[i] + "' in date\n");
            }
        }
        int y1 = 10 * (line[9] - '0') + line[10] - '0';
        int m1 = 10 * (line[11] - '0') + line[12] - '0';
        int d1 = 10 * (line[13] - '0') + line[14] - '0';
        int y2 = 10 * (line[15] - '0') + line[16] - '0';
        int m2 = 10 * (line[17] - '0') + line[18] - '0';
        int d2 = 10 * (line[19] - '0') + line[20] - '0';
        return RJISDate::Range(2000 + y1, m1, d1, 2000 + y2, m2, d2);
    }

#pragma pack(push, 1)
    class TrainCall
    {
        // all times are minutes from midnight. -1 means no time could be read (either spaces in the time field or no time field
        // present for that particular record type:
        short arrivalMinutes = -1;
        short departMinutes = -1;
        short publicArrivalMinutes = -1;
        short publicDepartMinutes = -1;
        char recordType;
    public:
        CRSCode crs;
    public:
        TrainCall(std::string line)
        {
            std::string tiploc = line.substr(2, 8);
            if (isdigit(line[7]))
            {
                tiploc = tiploc.substr(0, 7);
            }
            ams::Trim(tiploc);
            crs = GetCRS(tiploc);

            // check this time - whether it is a departure or arrival time it is always present so this is a sensible sanity check:
            if (!isdigit(line[10]) || !isdigit(line[11]) || !isdigit(line[12]) || !isdigit(line[13]))
            {
                throw QException("invalid character in time");
            }

            std::string recordType = line.substr(0, 2);

            // origin record:
            if (recordType == "LO")
            {
                recordType = 'O';
                departMinutes = 600 * (line[10] - '0') + 60 * (line[11] - '0') + 10 * (line[12] - '0') + line[13] - '0';
                if (line[15] != ' ')
                {
                    publicDepartMinutes = 600 * (line[15] - '0') + 60 * (line[16] - '0') + 10 * (line[17] - '0') + line[18] - '0';
                }
            }
            // intermediate station record:
            else if (recordType == "LI")
            {
                recordType = 'I';
                arrivalMinutes = 600 * (line[10] - '0') + 60 * (line[11] - '0') + 10 * (line[12] - '0') + line[13] - '0';
                departMinutes = 600 * (line[15] - '0') + 60 * (line[16] - '0') + 10 * (line[17] - '0') + line[18] - '0';
                if (line[25] != ' ')
                {
                    publicArrivalMinutes = 600 * (line[25] - '0') + 60 * (line[26] - '0') + 10 * (line[27] - '0') + line[28] - '0';
                }
                if (line[29] != ' ')
                {
                    publicDepartMinutes = 600 * (line[29] - '0') + 60 * (line[30] - '0') + 10 * (line[31] - '0') + line[32] - '0';
                }
            }
            // terminating station record
            else if (recordType == "LT")
            {
                recordType = 'T';
                arrivalMinutes = 600 * (line[10] - '0') + 60 * (line[11] - '0') + 10 * (line[12] - '0') + line[13] - '0';
                if (line[15] != ' ')
                {
                    publicArrivalMinutes = 600 * (line[15] - '0') + 60 * (line[16] - '0') + 10 * (line[17] - '0') + line[18] - '0';
                }
            }
            //            assert(minutes < 1440);
        }

        uint16_t GetArrival() const
        {
            short result;
            if (abs(publicArrivalMinutes - arrivalMinutes) < 10 && publicArrivalMinutes != -1)
            {
                result = publicArrivalMinutes;
            }
            else
            {
                result = arrivalMinutes;
            }
            return result;
        }

        uint16_t GetDeparture() const
        {
            short result;
            if (abs(publicDepartMinutes - departMinutes) < 10 && publicDepartMinutes != -1)
            {
                result = publicDepartMinutes;
            }
            else
            {
                result = departMinutes;
            }
            return result;
        }
        uint16_t  GetPublicArrival() const { return publicArrivalMinutes; }
        uint16_t  GetPublicDeparture()  const { return publicDepartMinutes; }
    };
#pragma pack(pop, 1)

    struct TrainRun
    {
        RJISDate::Dayset runningDays;
        RJISDate::Range runningDates;
        std::string trainUID;
        std::string trainID;
        char stpIndicator;
        int linenumber;                 // source line number in the mca file
        std::vector<TrainCall> callingAt;
        void AddCall(const TrainCall& call)
        {
            callingAt.push_back(call);
        }
        void ClearCalls()
        {
            callingAt.clear();
        }
    };

    // a flow from origin to destination - used in the CRS flow map
    struct CRSFlow
    {
        CRSCode crsOrigin;
        CRSCode crsDestination;
        CRSFlow(CRSCode origin, CRSCode destination) : crsOrigin(origin), crsDestination(destination) {}
        CRSFlow(std::string crsOrigin, std::string crsDestination) : crsOrigin(crsOrigin), crsDestination(crsDestination) {}
        bool operator<(const CRSFlow& other) const
        {
            return std::forward_as_tuple(crsOrigin, crsDestination) < std::forward_as_tuple(other.crsOrigin, other.crsDestination);
        }
        std::string GetString() { return crsOrigin.GetString() + crsDestination.GetString(); }
    };

    struct CallingPoint
    {
        CRSCode crs;
        uint16_t time;
    };

#pragma pack(push, 1)
    struct MinutesIndex
    {
        uint16_t minutes;
        uint32_t index;
        uint16_t subIndex1;
        uint16_t subIndex2;
        MinutesIndex() = default;
        MinutesIndex(uint16_t minutes) : minutes(minutes) {}
        MinutesIndex(uint16_t minutes, uint32_t index, uint16_t subIndex1, uint16_t subIndex2) : 
            minutes(minutes),
            index(index),
            subIndex1(subIndex1),
            subIndex2(subIndex2)
        {}
    };

    struct Journey
    {
        std::vector<CallingPoint> calling;
        void Add(CRSCode crs, uint16_t time)
        {
            calling.push_back(CallingPoint{ crs, time });
        }
        void Clear()
        {
            calling.clear();
        }
        const CallingPoint& GetFirst() const
        {
            return calling.front();
        }
        const CallingPoint& GetLast() const
        {
            return calling.back();
        }
    };
#pragma pack(pop)
}
