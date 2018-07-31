#include "stdafx.h"
#include "ams/fileutils.h"
#include "ams/jsutils.h"
#include "ProcessFareList.h"
#include "RJISMaps.h"
#include "FareDebug.h"
#include "config.h"
#include "RelatedStations.h"

struct NDFFoundKey
{
    RouteCode route_;					// five digit route code
    TicketCode ticketCode_;				// three letter RJIS ticket code
    NDFFoundKey() = default;
    NDFFoundKey(RouteCode route, TicketCode ticketcode) : route_(route), ticketCode_(ticketcode) {}
    bool operator<(const NDFFoundKey& other) const
    {
        return std::forward_as_tuple(route_, ticketCode_) <
            std::forward_as_tuple(other.route_, other.ticketCode_);
    }
};


std::string JSONAddNV(std::string name, std::string value)
{
    std::string result = "\"" + name + "\": " + "\"" + value + "\"";
    return result;
}

std::string JSONAddNV(std::string name, int value)
{
    std::string result = "\"" + name + "\": " + std::to_string(value);
    return result;
}

std::string JSONAddNV(std::string name, unsigned int value)
{
    std::string result = "\"" + name + "\": " + "\"" + std::to_string(value) + "\"";
    return result;
}


// given an RJIS percentage 0-999 reprenting 0-99.9% calculate the final price. We round UP to the nearest penny THEN round to the nearest 5p:
unsigned Rounding(unsigned x, unsigned  percent)
{
	return ((x * (1000 - percent) + 999) / 1000 + 2) / 5 * 5;
}


// returns a match from a list of non-standard discounts - everything (route, railcard, ticketcode) has to match, but an
// exact match is always a "better quality" match than a wildcard match. A non-match returns -1
int GetMatchQuality(const RJISTypes::NSDiscEntry& fns,
                    const RouteCode& route,
                    const RailcardCode& railcard,
                    const TicketCode& ticketcode
    )
{
    uint32_t matchQuality = 0;
    if (route == fns.route_)
    {
        matchQuality |= 0b100;
    }
    else if (fns.route_ != "*****"s)
    {
        // mark a non-match:
        matchQuality = -1;
    }

    // if we can still match
    if (matchQuality >= 0)
    {
        if (railcard == fns.railcard_)
        {
            matchQuality |= 0b10;
        }
        else if (fns.railcard_ != "***"s)
        {
            // mark a non-match:
            matchQuality = -1;
        }
    }

    // if we can still match
    if (matchQuality >= 0)
    {
        if (ticketcode == fns.ticketCode_)
        {
            matchQuality |= 1;
        }
        else if (fns.ticketCode_ != "***"s)
        {
            // mark a non-match:
            matchQuality = -1;
        }
    }
    return matchQuality;
}

// return an iterator into the NSD deque representing the best match. If there is no possible match then
// return RJISMaps::nonStandardDiscounts.end():
decltype(RJISMaps::nonStandardDiscounts)::iterator GetNonStandardDiscount(
    RouteCode route,                            // 5 digit route code - the one that we found, not a route code we are searching for
    TicketCode ticketcode,                      // 3 character ticket code - the one that we found, not a ticket code we are searching for
    const FareSearchParams& searchParams        // used for the railcard code we are actually calculating with
    )
{
    // get the complete set of stations to search for in the non-standard discounts table.
    // We must search on the station itself and any station groups, counties and zones (but DEFINITELY NOT CLUSTERS)
    std::deque<UNLC> allFNSStations;
    // add the station itself to the list
    allFNSStations.push_back(searchParams.flow_.origin);
    GetRelatedStations(allFNSStations, searchParams.flow_.origin, searchParams.travelDate_);

    // set the maximum match quality found to zero - we then try to find some discount that matches in some way:
    int maxMatchQuality = 0;

    // this is the best match we found so far - we set it to the end of the non-standard discount map if we don't 
    // find anything
    decltype(RJISMaps::nonStandardDiscounts)::iterator bestMatch = std::end(RJISMaps::nonStandardDiscounts);

    // for each station in the origin list (i.e. individual nlc plus group plus travelcard zone plus county)
    for (auto nlc : allFNSStations)
    {
        auto ofound = RJISMaps::nsdOriginIndex.equal_range(nlc);

        for (auto p = ofound.first; p != ofound.second; ++p)
        {
            // get a reference to the non-standard discount stored in the deque
            auto& fns = *(p->second);
            if (fns.dates_.AreDatesValid(searchParams.queryDate_, searchParams.travelDate_))
            {
                auto matchQuality = GetMatchQuality(fns, route, searchParams.railcard_, ticketcode);
                // ensure that a match for the individual station nlc trumps a group nlc match:
                if (matchQuality >= 0 && nlc == searchParams.flow_.origin)
                {
                    matchQuality |= 0b1000;
                }
                if (matchQuality > maxMatchQuality)
                {
                    maxMatchQuality = matchQuality;
                    bestMatch = p->second;
                }
            }
        }
    }
    // if we didn't find a match, then look at the destinations
    if (maxMatchQuality < 1)
    {
        allFNSStations.clear();
        allFNSStations.push_back(searchParams.flow_.destination);
        GetRelatedStations(allFNSStations, searchParams.flow_.destination, searchParams.travelDate_);

        for (auto nlc : allFNSStations)
        {
            auto dfound = RJISMaps::nsdDestinationIndex.equal_range(nlc);
            for (auto p = dfound.first; p != dfound.second; ++p)
            {
                // get a reference to the non-standard discount stored in the deque
                auto& fns = *(p->second);
                if (fns.dates_.AreDatesValid(searchParams.queryDate_, searchParams.travelDate_))
                {
                    auto matchQuality = GetMatchQuality(fns, route, searchParams.railcard_, ticketcode);
                    if (matchQuality >= 0 && nlc == searchParams.flow_.destination)
                    {
                        matchQuality |= 0b1000;
                    }
                    if (matchQuality > maxMatchQuality)
                    {
                        maxMatchQuality = matchQuality;
                        bestMatch = p->second;
                    }
                }
            }
        }
    }
    if (maxMatchQuality < 1)
    {
//        std::cout << "nothing found\n";
    }
    else
    {
//        std::cout << bestMatch->originCode_ << bestMatch->destinationCode_ << bestMatch->route_ << bestMatch->railcard_ << bestMatch->ticketCode_ << "\n";
    }
    return bestMatch;
}


std::string GetNDFDetailsAsString(const RJISTypes::NDFMainValue& ndf)
{
	std::ostringstream oss;
	oss << ndf.route_ << " " << ndf.railcardCode_ << " " << ndf.ticketCode_ << " " << "end: " << ndf.seqDates_.GetEndDate() << " start: " <<
							ndf.seqDates_.GetStartDate() << " quote: " << ndf.seqDates_.GetQuoteDate() << " adult: " << ndf.adultFare_ << " child: " << ndf.childFare_;
	return oss.str();
}

// Suppression set is a non-multiset since once we have found all NDFs for a given flow, railcard and date, the combo of route and ticket
// code will be unique:
typedef std::set<NDFFoundKey> SuppressionSet;

// list of NDFs found - this is a multimap since for each flow there may be several ticket code, route combos:
typedef std::multimap<UFlow, RJISTypes::NDFMainValue> NDFFoundMap;

// a secondary index by route and ticket code into the found map - there should only be one entry for the
// combination of route and ticket code:
typedef std::map<NDFFoundKey, NDFFoundMap::iterator> FoundMapIndex;

void ProcessNDFs(NDFResultsMap& results, UFlow flow, const FareSearchParams& searchParams, bool useReturnDate = false)
{
	NDFFoundMap foundNDFs;
	FoundMapIndex foundMapIndex;
	SuppressionSet suppressionSet;

    // using return date is used almost exclusively for plusbus fares:
    auto searchDate = useReturnDate ? searchParams.returnDate_ : searchParams.travelDate_;

	// first build the list of matching NDFs for this flow and railcard combination for the travel date and query date given:
	auto ndfIters = RJISMaps::ndfMain.equal_range(flow);

	for (auto ndfEntry = ndfIters.first; ndfEntry != ndfIters.second; ++ndfEntry)
	{
		auto &ndf = ndfEntry->second;
		// if the NDF is valid for travel on the travel date and the quote date allows querying on this date,
		// insert it into the NDF found map:
		if (ndf.seqDates_.AreDatesValid(searchParams.queryDate_, searchDate) &&
            ndf.railcardCode_ == searchParams.railcard_ &&
            (searchParams.route_.IsEmpty() || searchParams.route_ == ndf.route_) &&
            (searchParams.ticketCode_.IsEmpty() || searchParams.ticketCode_ == ndf.ticketCode_)
            )
		{
			foundNDFs.insert(std::make_pair(flow, ndf));
//			std::cout << "stored NDF - line " << ndf.linenumber << " " << GetNDFDetailsAsString(ndf) << "\n";
		}
	}

	// index all found NDFs by (route, ticket code). We have already matched just a single railcard code so there
    // should be only one matching entry for each combination of (route, ticket type) for the NDFs found.
	for (auto ndfEntry = std::begin(foundNDFs); ndfEntry != std::end(foundNDFs); ++ndfEntry)
	{
		NDFFoundKey key(ndfEntry->second.route_, ndfEntry->second.ticketCode_);
		foundMapIndex.insert(std::make_pair(key, ndfEntry));
	}

	// next examine the NFOs - (1) an NFO can be an ordinary NDF or (2) it can replace an existing NDF or (3) it can
	// suppress an NDF between the NFOs start and end dates

//	std::cout << "searching for NFOs\n";
	auto nfoIters = RJISMaps::nfoMain.equal_range(flow);
	for (auto nfoEntry = nfoIters.first; nfoEntry != nfoIters.second; ++nfoEntry)
	{
		auto &nfo = nfoEntry->second;
		// is this a suppression?
		if (nfo.suppressionMarker_.IsIndicated())
		{
			// can the suppression be used today? Only store it if it can suppress based on the query date (usually today)
			// and the intended date of travel. We store it since we might use it to replace an NFO
			if (nfo.seqDates_.AreDatesValid(searchParams.queryDate_, searchDate) &&
                nfo.railcardCode_ == searchParams.railcard_ &&
                (searchParams.route_.IsEmpty() || searchParams.route_ == nfo.route_) &&
                (searchParams.ticketCode_.IsEmpty() || searchParams.ticketCode_ == nfo.ticketCode_)
                )
			{
//				std::cout << "NFO suppression stored - line " << nfo.linenumber + 1 << " " << GetNDFDetailsAsString(nfo) << "\n";
				// store this suppression for route and ticket code comparison later - this is case 3:
				NDFFoundKey suppression;
				suppression.route_ = nfo.route_;
				suppression.ticketCode_ = nfo.ticketCode_;
				suppressionSet.insert(suppression);
			}
		}
		else
		{
			// this NFO is like an NDF, so do the same date comparison and if it passes
			// check to see if there is an existing NDF with the same route and ticket code:
			if (nfo.seqDates_.AreDatesValid(searchParams.queryDate_, searchDate) && 
                nfo.railcardCode_ == searchParams.railcard_ &&
                (searchParams.route_.IsEmpty() || searchParams.route_ == nfo.route_) &&
                (searchParams.ticketCode_.IsEmpty() || searchParams.ticketCode_ == nfo.ticketCode_)
                )
			{
				auto existingNDF =  foundMapIndex.find(NDFFoundKey(nfo.route_, nfo.ticketCode_));
				if (existingNDF == foundMapIndex.end())
				{
//					std::cout << "NFO addition - line number " << nfo.linenumber + 1 << "\n";

					// didn't find an existing NDF for this NFO, so just insert the NFO:
					foundNDFs.insert(std::make_pair(flow, nfo));
				}
				else
				{
//					std::cout << "NFO replacement\n";
					// we found an existing NDF so this NFO replaces that NDF:
					existingNDF->second->second = nfo;
				}
			}
		}
	}

	// for each item in the suppression set, delete entries from the NDF found map:
	for (auto suppression : suppressionSet)
	{
		auto existingNDF = foundMapIndex.find(suppression);
		if (existingNDF != foundMapIndex.end())
		{
//			std::cout << "deleting NFO\n";
			foundNDFs.erase(existingNDF->second);
		}
	}

	for (auto ndf : foundNDFs)
	{
        FoundNDFKey ffkey(ndf.first, ndf.second.route_, ndf.second.railcardCode_, ndf.second.ticketCode_);
        FoundNDFValue ffvalue(ndf.second.adultFare_, ndf.second.childFare_, ndf.second.restrictionCode_);
		results.insert(std::make_pair(ffkey, ffvalue));
	}
}

bool GetRailcardEntry(RJISTypes::RailcardValue& railcardEntry, const FareSearchParams& searchParams)
{
    bool found = false;
    auto matchingRailcards = RJISMaps::railcards.equal_range(searchParams.railcard_);
    for (auto p = matchingRailcards.first; p != matchingRailcards.second && !found; ++p)
    {
        if (p->second.seqDates_.AreDatesValid(searchParams.queryDate_, searchParams.travelDate_))
        {
            found = true;
            railcardEntry = p->second;
        }
    }
    return found;
}

bool GetTicketTypeEntry(RJISTypes::TicketTypeValue& ticketTypeEntry, TicketCode tty, const FareSearchParams& searchParams)
{
    bool found = false;
    auto matchingTickets = RJISMaps::ticketTypes.equal_range(tty);
    for (auto ttyRangeIterator = matchingTickets.first; ttyRangeIterator != matchingTickets.second && !found; ++ttyRangeIterator)
    {
        if (ttyRangeIterator->second.seqDates_.AreDatesValid(searchParams.queryDate_, searchParams.travelDate_))
        {
            found = true;
            ticketTypeEntry = ttyRangeIterator->second;
        }
    }
    return found;
}

bool GetStandardDiscount(
    int& percentage,                            // OUTPUT. The discount percentage 000-999 to apply
    const StatusCode& statusCode,
    const DiscountCategory& discountCategory,
    const FareSearchParams& searchParams)
{
    bool found = false;
    RJISTypes::SDiscountKey key(statusCode, discountCategory);
    auto matchingDiscounts = RJISMaps::standardDiscounts.equal_range(key);
    for (auto p = matchingDiscounts.first; p != matchingDiscounts.second && !found; ++p)
    {
        if (p->second.endDate_ >= searchParams.travelDate_)
        {
            found = true;
            percentage = p->second.percentage_;
        }
    }
    return found;
}

// ProcessFareEntry - process a single T record from the FFL file - these records are stored in the map RJISMaps::flowMainFares
void ProcessFareEntry(
    FareResultsMap& allFareResults,                 // OUTPUT. Mapping (flow, route, railcard, ticketcode)->(adult fare, child fare)
    UFlow flow,                                     // INPUT. The flow found
    const RJISTypes::FFLFlowMainValue& flowValue,   // INPUT. The value (of the key-value pair) for the flow
    const RJISTypes::FFLFareMainValue& fareEntry,   // INPUT. The value (of the key-value pair) from the fare map
    const FareSearchParams& searchParams       // values to possibly match
 )
{
    // we need to look in the ticket type file to get the discount category:
    RJISTypes::TicketTypeValue ttypeEntry;
    bool ticketTypeFound = GetTicketTypeEntry(ttypeEntry, fareEntry.ticketCode_, searchParams);
    if (!ticketTypeFound)
    {
        throw FareException(std::string() + "Cannot find ticket type " + fareEntry.ticketCode_.GetString() + " in ticket type file.");
    }

    int adultfare, childfare;
    if (flowValue.IsStandardDiscount())
    {
        // discount category 01-20 - retrieve from .TTY file based on ticket type then look up in .DIS (status discount) file
        // to obtain the discount percentage:
        auto& discountCategory = ttypeEntry.discountCategory_; 

        adultfare = fareEntry.fare_;
        int percentage;
        bool childDiscountFound = GetStandardDiscount(percentage, searchParams.childstatus_, discountCategory, searchParams);
        if (childDiscountFound)
        {
            childfare = Rounding(adultfare, percentage);
        }

        // only discount and round adult fare if we are using a railcard:
        if (searchParams.railcard_ != "   ")
        {
            bool adultDiscountFound = GetStandardDiscount(percentage, searchParams.adultstatus_, discountCategory, searchParams);
            if (adultDiscountFound)
            {
                adultfare = Rounding(adultfare, percentage);
            }
        }
    }
    else // non-standard discount:
    {
        // get non-standard discount using original flows and not 
        auto nsd = GetNonStandardDiscount(flowValue.route_, fareEntry.ticketCode_, searchParams);
        adultfare = fareEntry.fare_;
        childfare = fareEntry.fare_;
        // only discount the adult fare if the railcard is not all spaces:
        if (searchParams.railcard_ != "   "s)
        {
            if (nsd->adultNoDis_ == "N"s)
            {
                // N means calculate a standard discount for adults if a railcard is used
                // (a non-discounted adult fare is always taken directly from the flow file)
                auto& discountCategory = ttypeEntry.discountCategory_;

                int percentage;
                bool adultDiscountFound = GetStandardDiscount(percentage, searchParams.adultstatus_, discountCategory, searchParams);
                if (adultDiscountFound)
                {
                    adultfare = Rounding(adultfare, percentage);
                }
            }
            else if (nsd->adultNoDis_ == "X"s)
            {
                // X means no adult fare can be calculated, so we use the "no fare" indicator of 99999999
                adultfare = 99999999;
            }
            else if (nsd->adultNoDis_ == "D"s)
            {
                // D means use the same fare - a discounted fare cannot be calculated:
                adultfare = adultfare;
            }
            else if (nsd->adultNoDis_ == " "s)
            {
                std::cout << "RECALCULATE CHILD--------------------------\n";
            }
        }
        // always attempt to get a child discount even when there is no railcard:
        if (nsd->childNoDis_ == "N"s)
        {
            // N here means calculate a standard discount for children:
            auto& discountCategory = ttypeEntry.discountCategory_;

            int percentage;
            bool childDiscountFound = GetStandardDiscount(percentage, searchParams.childstatus_, discountCategory, searchParams);
            if (childDiscountFound)
            {
                childfare = Rounding(childfare, percentage);
            }

        }
        else if (nsd->adultNoDis_ == "X"s)
        {
            // X means no child fare can be calculated, so we use the "no fare" indicator of 99999999
            childfare = 99999999;
        }
        else if (nsd->adultNoDis_ == "D"s)
        {
            // do nothing as a discounted fare cannot be calculated:
        }
        else if (nsd->adultNoDis_ == " "s)
        {
            std::cout << "RECALCULATE ADULT--------------------------\n";
        }
    }
    FoundFareKey fk(flow, flowValue.route_, flowValue.nsDiscInd_, flowValue.flowid_);
    FoundFareValue fv(adultfare, childfare, fareEntry.ticketCode_, fareEntry.rescode_, ttypeEntry.ticketClass_, ttypeEntry.ticketType_);
    allFareResults[fk].push_back(fv);
}

std::string GetCRSFromNLC(UNLC nlc)
{
    std::string crs;
    auto p = RJISMaps::locations.equal_range(nlc);
    if (p.first != p.second)
    {
        crs = p.first->second.crsCode_.GetString();
    }
    else
    {
        crs = "";
    }
    return crs;
}

// Populate the derived fields of the searchParams structure (which are declared mutable):
void GetParamsDerivedFields(const FareSearchParams& searchParams)
{
    // get the adult and child statuses for the railcard passed in:
    RJISTypes::RailcardValue railcardEntry;
    bool railcardFound = GetRailcardEntry(railcardEntry, searchParams);
    if (railcardFound)
    {
        searchParams.adultstatus_ = railcardEntry.adultStatus_;
        searchParams.childstatus_ = railcardEntry.childStatus_;
    }
    searchParams.crsOrigin_ = GetCRSFromNLC(searchParams.flow_.origin);
    searchParams.crsDestination_ = GetCRSFromNLC(searchParams.flow_.destination);
}

// We are a journey-plan consumer and as such we don't do primary journey planning. We consume pre-produced .plan 
// files.
// This function opens a journey plan file and reads all possible journey plans for this origin - we can cachehint this file
// if necessary by implementing some form of cache-hinting. A hint would be given after the user enters
// his origin. The output from this function is a vector of TrainCalls - there will ALWAY be an EVEN number
// of the vector will consist of arrival departure pairs of TrainCalls
void ProcessFareList::GetJourneyPlan(std::vector<TTTypes::TrainCall>, FareSearchParams & searchParams)
{
    // we look in the auxiliary directory for the .plan files.
    auto& configuration = Config::Config::GetInstance();
    auto dir = Config::directories.GetDirectory("aux");
    dir += "/jp";
    // filename cannot be the CRS as DOS device names are not permitted (e.g. PRN.plan, CON.plan, AUX.plan)
    // so we prefix the CRS code with j:
    std::string filename = dir + "/j" + searchParams.crsOrigin_.GetString() + ".plan";

    typedef std::vector<CRSCode> Oneplan;
    std::map<CRSCode, std::vector<Oneplan>> planlist;
    try
    {
        ams::FastLineReader(filename, [&planlist, &searchParams](std::string line) {
            ams::Trim(line);
            static int linenumber = 0;
            // allow double-forward-slash comments:
            if (line.length() > 2 && line[0] != '/' && line[1] != '/')
            {
                if (line[3] != 0)
                {
                    std::cerr << "error at line " << linenumber + 1 << " colon expected at column 4 - line ignored\n";
                }
                CRSCode crs(line, 0);
                Oneplan oneplan;
                enum class States { Init, InCRS, GotCRS } state(States::Init);

                std::string currentCRS;
                for (auto i = 4u; i < line.length(); ++i)
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
                            // ERROR
                        }
                    }
                    else if (state == States::InCRS)
                    {
                        if (isupper(c))
                        {
                            currentCRS += c;
                        }
                        else if (currentCRS.length() != 3)
                        {
                            // ERROR - crs too short
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
                            planlist[searchParams.crsOrigin_].push_back(oneplan);
                            currentCRS.clear();
                            state = States::Init;
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
                            oneplan.push_back(currentCRS);
                            planlist[searchParams.crsOrigin_].push_back(oneplan);
                            currentCRS.clear();
                            state = States::Init;
                        }
                        else if (c != ' ')
                        {
                            // ERROR - unexpected character
                        }
                    }
                }
            }
            linenumber++;
        });
    }
    catch (std::exception ex)
    {
        std::cerr << "ERROR: " << ex.what() << std::endl;
    }
}

void ProcessFareList::GetPlusbusFares(FoundPlusBus& plusbusResult, FareSearchParams& searchParams)
{
    // determine group stations, county codes and London zone codes - we use these to search in the
    // NDF and NFO files - BUT we do not use clusters to search in the NDF or NFO files

//    std::cout << "origin " << searchParams.flow_.origin << "\n";
//    std::cout << "destination " << searchParams.flow_.destination << "\n";
    std::deque<UNLC> allOrigins{ searchParams.flow_.origin }, allDestinations{ searchParams.flow_.destination };
    GetRelatedStations(allOrigins, searchParams.flow_.origin, searchParams.travelDate_);
    GetRelatedStations(allDestinations, searchParams.flow_.destination, searchParams.travelDate_);

    for (auto p : allOrigins)
    {
//        std::cout << "origin: " << p << std::endl;
    }

    //std::cout << "all origins size is " << allOrigins.size() << "\n";
    //std::cout << "all destinations size is " << allDestinations.size() << "\n";

    // Generated a list of flows from any station in the flow list to any other station in the flow list.
    // This is for NDFs and NFOs so it does not include clusters:
    std::deque<UFlow> allPlusbusFlows;
    PermuteNLCs(allPlusbusFlows, allOrigins, allDestinations);

    // is plusbus for this flow disallowed (i.e. in the plusbus restrictions file)?
    bool restrictionFound = false;
    for (auto& flow : allPlusbusFlows)
    {
        if (RJISMaps::plusbusRestrictionSet.find(flow) != RJISMaps::plusbusRestrictionSet.end())
        {
            restrictionFound = true;
        }
    }

    // if we didn't find a restriction, search for plusbus fares:
    if (!restrictionFound)
    {
//        std::cout << "No restriction found\n";
        // use origin 0000 as a sentinel
        UNLC plusbusOrigin("0000");

        // for all potential station NLCs (i.e. physical station and group station NLC) for which a plusbus NLC 
        // is available, try to obtain a plusbus NLC:
        for (auto p : allOrigins)
        {
//            std::cout << "searching for origin " << p << "\n";
            auto res = RJISMaps::plusbusNLCMap.find(p);
            if (res != RJISMaps::plusbusNLCMap.end())
            {
                plusbusOrigin = res->second;
//                std::cout << "found plusbus origin " << plusbusOrigin << std::endl;
                // break out of loop since we found a plusbus origin and there should be only one - a rare useful use of
                // break:
                break;
            }
        }

        NDFResultsMap pbFareResults0, pbFareResults1, pbFareResults2, pbFareResults3;

        // if we have a plusbus NLC at the rail origin (e.g. we found H132 for Poole 5883),
        // search for fares from that NLC to any of the rail origins (individual rail NLC or group station NLC):
        if (plusbusOrigin != "0000")
        {
            std::deque<UFlow> allPBOriginFlows;
            std::vector<UNLC> origins{ plusbusOrigin };
            PermuteNLCs(allPBOriginFlows, origins, allOrigins);
            // get a list of matching NDFs for every flow combination - each call to ProcessNDFs will
            // add to the pbFareResults0 container - this is b0
            for (auto& flow : allPBOriginFlows)
            {
                ProcessNDFs(pbFareResults0, flow, searchParams);
            }
            // do the same for the return date with the flows reversed - this is b3:
            for (auto flow : allPBOriginFlows) // (no ref on flow)
            {
                flow.Reverse();
                ProcessNDFs(pbFareResults3, flow, searchParams, true);
            }
        }

        UNLC plusbusDestination = "0000";
        // for all potential rail destination NLCs (i.e. physical station and group station NLC) for which a plusbus NLC 
        // is available, try to obtain a plusbus NLC:
        for (auto p : allDestinations)
        {
            auto res = RJISMaps::plusbusNLCMap.find(p);
            if (res != RJISMaps::plusbusNLCMap.end())
            {
                plusbusDestination = res->second;
                // break out of loop since we found a plusbus origin and there should be only one - a rare useful use of
                // break:
                break;
            }
        }

        if (plusbusDestination != "0000")
        {
            std::deque<UFlow> allPBDestinationFlows;
            std::vector<UNLC> destinations { plusbusDestination };
            PermuteNLCs(allPBDestinationFlows, destinations, allDestinations);
            // get a list of matching NDFs for every railflow->plusbus combination - each call to ProcessNDFs will
            // add to the pbFareResults0 container - this is b1
            for (auto& flow : allPBDestinationFlows)
            {
                ProcessNDFs(pbFareResults1, flow, searchParams);
            }
            // do the same for the return date - this is b2:
            for (auto flow : allPBDestinationFlows) // (no ref on flow)
            {
                flow.Reverse();
                ProcessNDFs(pbFareResults2, flow, searchParams, true);
            }

        }

        for (auto p : pbFareResults0)
        {
            if (p.first.ticketcode_ == "PBD"s && ((p.second.adultPrice_ < 99'999'99 || p.second.childPrice_ < 99'999'999)))
            {
                plusbusResult.pbFares_["b0"] = { p.second.adultPrice_, p.second.childPrice_ };
            }
            else if (p.first.ticketcode_ == "PB7"s)
            {
                plusbusResult.pbFares_["sw0"] = { p.second.adultPrice_, p.second.childPrice_ };
            }
            else if (p.first.ticketcode_ == "BMS"s)
            {
                plusbusResult.pbFares_["sm0"] = { p.second.adultPrice_, p.second.childPrice_ };
            }
            else if (p.first.ticketcode_ == "BQS"s)
            {
                plusbusResult.pbFares_["sq0"] = { p.second.adultPrice_, p.second.childPrice_ };
            }
            else if (p.first.ticketcode_ == "BAS"s)
            {
                plusbusResult.pbFares_["sa0"] = { p.second.adultPrice_, p.second.childPrice_ };
            }
        }

        for (auto p : pbFareResults1)
        {
            if (p.first.ticketcode_ == "PBD"s && ((p.second.adultPrice_ < 99'999'99 || p.second.childPrice_ < 99'999'999)))
            {
                plusbusResult.pbFares_["b1"] = { p.second.adultPrice_, p.second.childPrice_ };
            }
            else if (p.first.ticketcode_ == "PB7"s)
            {
                plusbusResult.pbFares_["sw1"] = { p.second.adultPrice_, p.second.childPrice_ };
            }
            else if (p.first.ticketcode_ == "BMS"s)
            {
                plusbusResult.pbFares_["sm1"] = { p.second.adultPrice_, p.second.childPrice_ };
            }
            else if (p.first.ticketcode_ == "BQS"s)
            {
                plusbusResult.pbFares_["sq1"] = { p.second.adultPrice_, p.second.childPrice_ };
            }
            else if (p.first.ticketcode_ == "BAS"s)
            {
                plusbusResult.pbFares_["sa1"] = { p.second.adultPrice_, p.second.childPrice_ };
            }
        }

        for (auto p : pbFareResults2)
        {
            if (p.first.ticketcode_ == "PBD"s && ((p.second.adultPrice_ < 99'999'99 || p.second.childPrice_ < 99'999'999)))
            {
                plusbusResult.pbFares_["b2"] = { p.second.adultPrice_, p.second.childPrice_ };
            }
        }

        for (auto p : pbFareResults3)
        {
            if (p.first.ticketcode_ == "PBD"s && ((p.second.adultPrice_ < 99'999'99 || p.second.childPrice_ < 99'999'999)))
            {
                plusbusResult.pbFares_["b3"] = { p.second.adultPrice_, p.second.childPrice_ };
            }
        }
        plusbusResult.origin_ = searchParams.flow_.origin;
        plusbusResult.destination_ = searchParams.flow_.destination;
        plusbusResult.pbOrigin_ = plusbusOrigin;
        plusbusResult.pbDestination_ = plusbusDestination;
        for (auto p : plusbusResult.pbFares_)
        {
  //          std::cout << "\"" << p.first << "\": {\"a\": " << p.second.first << " \"c\":" << p.second.second << "},\n";
        }
    }

}

// Get all fares matching the searchparams
void ProcessFareList::GetAllFares(
    FareResultsMap& allFareResults,
    const FareSearchParams& searchParams)
{
    // fill in the search params derived fields - e.g. conversion of railcards to status codes:
    GetParamsDerivedFields(searchParams);

    UNLC origin(searchParams.flow_.origin);
    UNLC destination(searchParams.flow_.destination);

    // determine group stations, county codes and London zone codes - we use these to search in the
    // NDF and NFO files - BUT we do not use clusters to search in the NDF or NFO files
    std::deque<UNLC> allOrigins, allDestinations;
    GetRelatedStations(allOrigins, origin, searchParams.travelDate_);
    GetRelatedStations(allDestinations, destination, searchParams.travelDate_);
    // include the origin and destination stations themselves:
    allOrigins.push_back(origin);
    allDestinations.push_back(destination);

    // Generated a list of flows from any station in the flow list to any other station in the flow list.
    // This is for NDFs and NFOs so it does not include clusters:
    std::deque<UFlow> allNDFFlows;
    PermuteNLCs(allNDFFlows, allOrigins, allDestinations);

    // get a list of matching NDFs for every flow combination - each call to ProcessNDFs will
    // APPEND to the container specified in the first parameter:

    // store NDFs in their own container for now - we will combine the NDF with the FFL container later:
    NDFResultsMap ndfResults;
    for (auto& flow : allNDFFlows)
    {
//        std::cout << "searching for NDF for " << flow << std::endl;
        ProcessNDFs(ndfResults, flow, searchParams);
    }

    // print found NDFs for debugging purposes:

 //   DEBUGDumpNDFs(allFareResults);
 //   
 //   for (auto p : allNDFs)
    //{
 //       std::cout << p.first.origin << " " << p.first.destination << " " << p.second.route_ << " " <<
    //		p.second.ticketCode_ << " " << p.second.adultFare_ << " " << p.second.childFare_ << " line " << p.second.linenumber + 1 << "\n";
    //}


    // add the clusters to the origin and destination lists for searching in the flow maps - these are
    // generate FROM the existing lists and added TO the existing lists.
    AddClusters(allOrigins, allOrigins, searchParams.travelDate_);
    AddClusters(allDestinations, allDestinations, searchParams.travelDate_);

    std::deque<UFlow> permutedFlowlist;
    PermuteNLCs(permutedFlowlist, allOrigins, allDestinations);

    bool first = true;
    for (auto flow : permutedFlowlist)
    {
        // std::cout << "flow : " << flow << std::endl;
        // there can be several flow entries for each flow permutation. These will either have different end dates or link via a 
        // different flow ID to a different set of fares:
        auto matchingFlowEntries = RJISMaps::flowMainFlows.equal_range(flow);
        for (auto p = matchingFlowEntries.first; p != matchingFlowEntries.second; ++p)
        {
            auto& matchingFlowKey = p->first;
            auto& matchingFlowValue = p->second;
            if (matchingFlowValue.daterange_.IsDateInRange(searchParams.travelDate_))
            {
                // we might be searching for a particular route - however, we include all routes if the route code is empty:
                if (searchParams.route_.IsEmpty() || searchParams.route_ == matchingFlowValue.route_)
                {
                    // get all the fare entries for this flow (they correspond to T records in the FFL file)
                    auto matchingFareEntries = RJISMaps::flowMainFares.equal_range(matchingFlowValue.flowid_);
                    for (auto fareEntry = matchingFareEntries.first; fareEntry != matchingFareEntries.second; ++fareEntry)
                    {
                        // we might be searching for a particular ticket - however, we include all tickets if the ticket code is empty:
                        if (searchParams.ticketCode_.IsEmpty() || searchParams.ticketCode_ == fareEntry->second.ticketCode_)
                        {
                            // if (from the NDFs we found earlier) we find a matching NDF for this flow, route, railcard and fareEntry then we must use it 
                            // instead of the flow found here - therefore do not process this fare entry if we have already found an NDF:
                            auto ndf = ndfResults.find(FoundNDFKey(matchingFlowKey, matchingFlowValue.route_,
                                searchParams.railcard_, fareEntry->second.ticketCode_));
                            if (ndf == ndfResults.end()) // (if we didn't find an NDF)
                            {
                                ProcessFareEntry(allFareResults, matchingFlowKey, matchingFlowValue, fareEntry->second, searchParams);
                            }
                        }
                    }
                }
            }
        }
    }

    // merge NDF results and normal fare results:
    for (auto p : ndfResults)
    {
        FoundFareKey farekey(p.first.flow_, p.first.route_, -1, -1);
        RJISTypes::TicketTypeValue ttvalue;
        bool found = GetTicketTypeEntry(ttvalue, p.first.ticketcode_, searchParams);
        int ticketClass = 0;
        char ticketType; // S, R or N (single, return or season)
        if (found)
        {
            ticketClass = ttvalue.ticketClass_;
            ticketType = ttvalue.ticketType_;
        }
        FoundFareValue farevalue(p.second.adultPrice_, p.second.childPrice_, p.first.ticketcode_, p.second.restrictionCode_, ticketClass, ticketType);
        allFareResults[farekey].push_back(farevalue);
    }
}
