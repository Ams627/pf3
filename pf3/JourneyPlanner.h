#pragma once


// a single journey plan. This is simply a list of crs codes - intervening stations at which a change 
// must take place. This list does not contain the origin and final destination CRS codes, just
// the intervening ones:
typedef std::vector<CRSCode> Oneplan;

// A vector of journey plans for a single origing and destination. Each member of the vector is
// an inner vector of CRS codes (Oneplan defined above)
struct OneOrigDestPlanlist
{
    std::vector<Oneplan> plans;
};


class JourneyPlanner
{
public:
    JourneyPlanner() {}
    virtual ~JourneyPlanner(){}
    void LoadPlanFile(CRSCode crs);
    short GetSingleLegTimes(CRSCode origin, CRSCode destination, short hour);
    void GetPlan(CRSCode origin, CRSCode destination, short hour);
};
