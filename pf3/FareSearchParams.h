#pragma once

struct FareSearchParams
{
    UFlow flow_;                    // the origin, destination pair we search for - this is not identical to the flow we actually use which may be a cluster flow
    RailcardCode railcard_;
    RouteCode route_;               // if this is empty match any route
    TicketCode ticketCode_;         // if this is empty match any ticketcode
    RJISDate::Date queryDate_;
    RJISDate::Date travelDate_;
    RJISDate::Date returnDate_;
    bool recalculate = false;       // used to indicate a non-standard discount recalculation

    FareSearchParams() = default;

    FareSearchParams(std::string origin,                        // the original origin (not a derived one such as a flow or cluster)
        std::string destination,                                // the original destination (not a derived one such as a flow or cluster)
        std::string railcard,
        RJISDate::Date queryDate = RJISDate::Date::Today(),     // default query date is today
        RJISDate::Date travelDate = RJISDate::Date::Today(),    // default travel date is today
        RJISDate::Date returnDate = RJISDate::Date::Today()     // default return date is today
        )
    {
        flow_.origin = origin;
        flow_.destination = destination;
        railcard_ = railcard;

        // the default search is for any route, any ticket code. If these fields are clear (all zeroes) it means
        // we will not filter on any route or ticket code but extract all fares for the given origin and destination:
        ticketCode_.Clear();
        route_.Clear();
        queryDate_ = queryDate;
        travelDate_ = travelDate;
        returnDate_ = returnDate;
    }

    // derived fields - set from the railcard:
    mutable StatusCode adultstatus_;
    mutable StatusCode childstatus_;
    mutable CRSCode crsOrigin_;
    mutable CRSCode crsDestination_;
};
