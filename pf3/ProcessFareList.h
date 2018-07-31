#pragma once
#include "RJISMaps.h"
#include "RJISTTMaps.h"
#include "TTTypes.h"
#include "FareSearchParams.h"

struct FareException : public QException
{
    FareException(std::string msg) : QException(msg.c_str()) {}
    FareException(char *msg) : QException(msg) {}
    virtual ~FareException() {}
};

// NDFs are keyed by origin, destination, route, railcard and ticket code:
struct FoundNDFKey
{
    UFlow flow_;                // the flow actually found - not the original origin/destination
    RouteCode route_;           // the route found from the FFL/NDF or NFO file
    RailcardCode railcard_;     // railcard used
    TicketCode ticketcode_;     // ticket code found
    FoundNDFKey(UFlow flow, const RouteCode& route, const RailcardCode& railcard, const TicketCode &ticketCode) :
        flow_(flow), route_(route), railcard_(railcard), ticketcode_(ticketCode)
    {}

    bool operator<(const FoundNDFKey& other) const
    {
        return std::forward_as_tuple(flow_, route_, railcard_, ticketcode_) <
            std::forward_as_tuple(other.flow_, other.route_, other.railcard_, other.ticketcode_);
    }
};

struct FoundNDFValue
{
    int adultPrice_;
    int childPrice_;
    RestrictionCode restrictionCode_;
    FoundNDFValue(int adultPrice, int childPrice, RestrictionCode restrictionCode) :
        adultPrice_(adultPrice),
        childPrice_(childPrice),
        restrictionCode_(restrictionCode)
    {}
};


struct FoundFareKey
{
    UFlow flow_;                // the flow actually found - not the original origin/destination
    RouteCode route_;           // the route found from the FFL/NDF or NFO file
    int flowid_;                // flowID from the FFL F record or -1 in the case of an NDF
    int discInd_;               // non-standard discount indicator from the FFL F record or -1 in the case of an NDF
    FoundFareKey(UFlow flow, const RouteCode& route, int discInd, int flowid) :
        flow_(flow), route_(route), discInd_(discInd), flowid_(flowid)
    {}

    bool operator<(const FoundFareKey& other) const
    {
        return std::forward_as_tuple(flow_, route_, flowid_, discInd_) <
            std::forward_as_tuple(other.flow_, other.route_, other.flowid_, other.discInd_);
    }
};

struct FoundFareValue
{
    int adultPrice_;
    int childPrice_;
    TicketCode ticketcode_;             // three letter ticket code from the FFL F records like 7DF or SOR
    RestrictionCode restrictionCode_;   // two letter restriction code from the FFL T records like BE
    int ticketClass_;                   // 1, 2 or 9. This is the ticket class from the ticket types file
    char ticketType_;                    // S, R, or N (single, return or season)
    FoundFareValue(int adultPrice, int childPrice, const TicketCode &ticketCode,
        const RestrictionCode restrictionCode, const int ticketClass, char ticketType) :
        adultPrice_(adultPrice),
        childPrice_(childPrice),
        ticketcode_(ticketCode),
        restrictionCode_(restrictionCode),
        ticketClass_(ticketClass),
        ticketType_(ticketType)
    {}
};

struct FoundPlusBus
{
    bool valid_ = false;        // set to true when ANY plusbus fares are found
    UNLC origin_;               // main station origin: e.g. Poole would be 5883
    UNLC destination_;          // main station destination: e.g. Birmingham New Street would be 1127
    UNLC pbOrigin_;             // plusbus NLC for Poole - this will be H132
    UNLC pbDestination_;        // plusbus NLC for Birmingham J809
    std::map<std::string, std::pair<int, int>> pbFares_; // plusbus fares - key is the fare type and is one of B0, B1, B2, B3, SW0, SW1, SM0, SM1, SQ0, SQ1, SA0, SA1
};

typedef std::map<FoundFareKey, std::vector<FoundFareValue>> FareResultsMap;
typedef std::map<FoundNDFKey, FoundNDFValue> NDFResultsMap;

// map railcard to a plusbus structure - normally there will be only one railcard
typedef std::map<std::string, FoundPlusBus> PlusbusMap;

class ProcessFareList
{
public:
    ProcessFareList() {}
    void GetJourneyPlan(std::vector<TTTypes::TrainCall>, FareSearchParams& searchParams);
    void GetPlusbusFares(FoundPlusBus& plusbusResult, FareSearchParams & searchParams);
    void ProcessFareList::GetAllFares(
        FareResultsMap& result,
        const FareSearchParams& searchParams);

    virtual ~ProcessFareList(){}
};
