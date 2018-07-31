#pragma once


// GetRelatedStations - given a station NLC and a travel date, return a list of related stations.
//						most of the time this list will be empty or contain a single entry in the case of a
//						station like Dorchester South which is a member of a group.
//
//                      The list will also contain a county code and a travel card zone NLC if there is one
//                      This function DOES NOT RETURN STATION CLUSTERS.
//

template <class T> inline void AddStation(T& cont, UNLC nlc)
{
    cont.push_back(nlc);
}

template <> inline void AddStation(std::set<UNLC>& cont, UNLC nlc)
{
    cont.insert(nlc);
}

template <class Cont> void GetRelatedStations(Cont& container, const UNLC nlc, RJISDate::Date travelDate, bool checkdates = true)
{
    //    std::cout << "GetRelatedStations\n";
    // today's date:
    RJISDate::Date today(RJISDate::Date::Today());

    // get all location entries (L-records) for the nlc
    auto p1 = RJISMaps::locations.equal_range(nlc);
    for (auto nlcEntry = p1.first; nlcEntry != p1.second; ++nlcEntry)
    {
        auto& loc = nlcEntry->second;
        if (!checkdates || loc.seqDates_.GetQuoteDate() <= today && loc.seqDates_.GetStartDate() <= travelDate &&
            loc.seqDates_.GetEndDate() >= today)
        {
            // Add the county code from the location L-Record - these are rarely used but they are important in some cases:
            std::string countyCode = "CC" + loc.county_.GetString();
            UNLC countyNLC(countyCode);
            AddStation(container, countyNLC);

            if (loc.faregroup_ != nlc)
            {
                AddStation(container, loc.faregroup_);
            }

            // add the London Travelcard zones:
            if (loc.zoneNumber_ >= '1' && loc.zoneNumber_ <= '6')
            {
                //                std::cout << "adding zone " << loc.zoneNLC_ << std::endl;
                AddStation(container, loc.zoneNLC_);
            }
        }
    }
    // add the groups of which the station is a member - these are the M-records of RJIS5045
    auto p2 = RJISMaps::groups.find(nlc);
    if (p2 != RJISMaps::groups.end())
    {
        // iterate over the inner map, getting the structure contain the group code and list of end dates:
        for (auto groupEntry : p2->second)
        {
            // iterate along the vector of end dates:
            for (auto endDate : groupEntry.second)
            {
                if (endDate >= today)
                {
                    AddStation(container, groupEntry.first);
                }
            }
        }
    }
    auto p3 = RJISMaps::auxGroups.find(nlc);
    if (p3 != RJISMaps::auxGroups.end())
    {
        //        std::cout << "adding aux stations\n";
        // iterate over the inner map, getting the auxilliary member station NLC:
        for (auto groupstation : p3->second)
        {
            AddStation(container, groupstation);
        }
    }
}


// for each station in the list add the cluster IDs for that station to the output clusters array. Since the output
// list is usually an augmented form of the input list, we MUST COPY THE INPUT LIST and not pass it by reference.
// (the same variable is usually passed in for the first and second parameters).
template<class Cont1, class Cont2> void AddClusters(
    Cont1& clusters,            // OUT - the list of clusters for the given list of station/group/zone/county NLCs
    const Cont2 stations,       // IN - NOTE: MUST BE COPIED BY VALUE! DO NOT CHANGE TO A REFERENCE. 
    const RJISDate::Date date,  // the travel date (not the date of query)
    bool checkdate = true       // generally we should check the date
    )
{
    for (auto nlc : stations)
    {
        auto clusterIDEntry = RJISMaps::clusters.find(nlc); // K-V pair - K = station NLC, V = list of clusters
        if (clusterIDEntry != RJISMaps::clusters.end())
        {
            // iterate along the list of clusters we found for the given station NLC:
            for (auto clusterID : clusterIDEntry->second)
            {
                if (checkdate)
                {
                    // iterate along the date ranges for this cluster ID:
                    for (auto daterange : clusterID.second)
                    {
                        // add cluster to the list if the date specified as an input parameter is in the range:
                        if (daterange.IsDateInRange(date))
                        {
                            AddStation(clusters, clusterID.first);
                        }
                    }
                }
                else
                {
                    AddStation(clusters, clusterID.first);
                }
            }
        }
    }
}

