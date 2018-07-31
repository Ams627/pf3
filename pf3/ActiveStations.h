#pragma once

class ActiveStations
{
    std::set<UNLC> activeStationSet_;
public:
    ActiveStations();
    void GetList(std::set<UNLC>& activeStationSet) const;
    void MakeFilteredSets();
    virtual ~ActiveStations(){}
};
