#pragma once
namespace RJISTypes
{
#pragma pack(push, 1)
    // the main value type for the NDF multimap:
    struct NDFMainValue
    {
		int linenumber;						// debugging only - stored linenumber from the NDF or NFO file
        RouteCode route_;					// five digit route code
        RailcardCode railcardCode_;			// three letter RJIS railcard code
        TicketCode ticketCode_;				// three letter RJIS ticket code
        char ndRecordType;                  // nd record type - N=NDF, O=NFO
        RJISDate::Triple seqDates_;         // start end and quote dates
        Indicator suppressionMarker_;       // suppression marker for NFO only
        Fare adultFare_;                    // 8 chars in pence
        Fare childFare_;                    // 8 chars in pence
        RestrictionCode restrictionCode_;   // restriction code
        Indicator composite_;
        Indicator crossLondon_;
        Indicator privateSettlement_;
		bool deleted = false;				// used in fare calculation when a suppression is valid

        NDFMainValue() = default;

        NDFMainValue(std::string s, size_t offset)
        {
            Set(s, offset);
        }

        inline void Set(const std::string &str, size_t offset)
        {
            route_.Set(str, offset);
            railcardCode_.Set(str, offset + 5);
            ticketCode_.Set(str, offset + 8);
            ndRecordType = str[offset + 11];
            seqDates_.Set(str, offset + 12);
            suppressionMarker_.Set(str, offset + 36);
            adultFare_.Set(str, offset + 37);
            childFare_.Set(str, offset + 45);
            restrictionCode_.Set(str, offset + 53);
            composite_.Set(str, offset + 55);
            crossLondon_.Set(str, offset + 56);
            privateSettlement_.Set(str, offset + 57);
        }

    };

    inline std::ostream& operator<<(std::ostream& str, const NDFMainValue ndfValue)
    {
        str << ndfValue.route_ << ndfValue.railcardCode_ << ndfValue.ticketCode_ <<
            ndfValue.ndRecordType <<
            ndfValue.seqDates_ <<
            ndfValue.suppressionMarker_;
        if (ndfValue.suppressionMarker_.IsIndicated())
        {
            // output 21 spaces if suppression marker is set:
            str << "                     "; 
        }
        else
        {
            str << std::dec << std::setfill('0') << std::setw(8) << ndfValue.adultFare_ <<
                std::setw(8) << ndfValue.childFare_ <<
                ndfValue.restrictionCode_ <<
                'Y' <<  // this will always be Y for any records we haved actually stored in the ndf map (COMPOSITE_INDICATOR)
                ndfValue.crossLondon_ <<
                ndfValue.privateSettlement_;
        }
        return str;
    }

#pragma pack(pop)

    struct FFLFlowMainValue
    {
        RouteCode route_;
        RJISDate::Range daterange_;
        TOCCode toc_;
        char direction;         // S = single Direction, R = both directions
        short crossLondon_;
        short nsDiscInd_;
        int flowid_ = 0;

        friend std::ostream& operator<<(std::ostream& str, const FFLFlowMainValue fflValue);

        FFLFlowMainValue() = default;
        FFLFlowMainValue(const std::string & str, size_t offset)
        {
            Set(str, offset);
        }

        inline void Set(const std::string & str, size_t offset)
        {
            route_.Set(str, offset);
            daterange_.Set(str, offset + 10);
            toc_.Set(str, offset + 26);
            direction = str[offset + 9];
            crossLondon_ = str[offset + 29] - '0';
            nsDiscInd_ = str[offset + 30] - '0';
            for (auto i = 0u; i < 7; ++i)
            {
                flowid_ = flowid_ * 10 + str[i + offset + 32] - '0';
            }
        }

        bool IsStandardDiscount() const
        {
            return nsDiscInd_ == 0 || nsDiscInd_ == 2;
        }

        std::string GetJSON() const
        {
            std::string discount;
            if (nsDiscInd_ == 0)
            {
                discount = "standard rail";
            }
            else if (nsDiscInd_ == 1)
            {
                discount = "non-standard rail";
            }
            else if (nsDiscInd_ == 2)
            {
                discount = "standard private";
            }
            else if (nsDiscInd_ == 3)
            {
                discount = "non-standard private";
            }
            return std::string("\"route\": \"") + route_.GetString() + "\", \"discount\": \"" + discount + "\", \"flowid\": " + std::to_string(flowid_);
        }
    };

    inline std::ostream& operator<<(std::ostream& str, const FFLFlowMainValue fflValue)
    {
        str << fflValue.route_ << "000A" << fflValue.direction << fflValue.daterange_ << fflValue.toc_ <<
            fflValue.crossLondon_ << fflValue.nsDiscInd_
            << "Y" // publication indicator - it is always published
            << std::dec << std::setfill('0') << std::setw(7) << fflValue.flowid_;
        return str;
    }

    struct FFLFareMainValue
    {
        TicketCode ticketCode_;
        Fare fare_;
        RestrictionCode rescode_;
        FFLFareMainValue(const std::string & str, size_t offset)
        {
            Set(str, offset);
        }
        inline void Set(const std::string & str, size_t offset)
        {
            ticketCode_.Set(str, offset);
            fare_.Set(str, offset + 3);
            rescode_.Set(str, offset + 11);
        }
    };

    inline std::ostream& operator<<(std::ostream& str, const FFLFareMainValue fflFareValue)
    {
        str << fflFareValue.ticketCode_ << std::dec << std::setfill('0') << std::setw(8) << fflFareValue.fare_ << fflFareValue.rescode_;
        return str;
    }


    struct TicketTypeValue
    {
        RJISDate::Triple seqDates_;
        std::string description_;
        TicketClass ticketClass_;
        char ticketType_; // S R or N - N = season
        char ticketGroup_;
        RJISDate::Date lastValidDate_;
        NumPassengers maxPassengers_;
        NumPassengers minPassengers_;
        NumPassengers maxAdults_;
        NumPassengers minAdults_;
        NumPassengers maxChildren_;
        NumPassengers minChildren_;
        Indicator resByDate_;
        Indicator resByTrain_;
        Indicator resByArea_;
        ValidityCode valCode_;
        std::string atbDescription_;
        GatePasses lulxLondon_;
        char reservationReqd_;
        CapriCode capriCode_;
        Indicator lul93_;
        UTSCode2 utsCode_;
        GateTimeCode timeRestriction_;
        Indicator freePassLul_;
        char packageMarker_;
        Multiplier faresMultiplier_;
        DiscountCategory discountCategory_;

        TicketTypeValue() = default;
        TicketTypeValue(const std::string & str, size_t offset)
        {
            Set(str, offset);
        }

        inline void Set(const std::string & str, size_t offset)
        {
            seqDates_.Set(str, offset); 			     
            description_ = str.substr(offset + 24, 15);  
            ticketClass_.Set(str, offset + 39); 		 
            ticketType_ = str[offset + 40]; 			 
            ticketGroup_= str[offset + 41]; 			 
            lastValidDate_.Set(str, offset + 42); 		 
            maxPassengers_.Set(str, offset + 50); 		 
            minPassengers_.Set(str, offset + 53); 		 
            maxAdults_.Set(str, offset + 56); 			 
            minAdults_.Set(str, offset + 59); 			 
            maxChildren_.Set(str, offset + 62); 		 
            minChildren_.Set(str, offset + 65); 		 
            resByDate_.Set(str, offset + 68); 			 
            resByTrain_.Set(str, offset + 69); 			 
            resByArea_.Set(str, offset + 70); 			 
            valCode_.Set(str, offset + 71); 			 
            atbDescription_ = str.substr(offset + 73, 20);
            lulxLondon_.Set(str, offset + 93); 			
            reservationReqd_ = str[offset + 94]; 		
            capriCode_.Set(str, offset + 95); 			
            lul93_.Set(str, offset + 98); 			    
            utsCode_.Set(str, offset + 99); 			
            timeRestriction_.Set(str, offset + 101); 	
            freePassLul_.Set(str, offset + 102); 		
            packageMarker_ = str[offset + 103]; 		    
            faresMultiplier_.Set(str, offset + 104); 	
            discountCategory_.Set(str, offset + 107); 	
        }
    };

    struct RailcardValue
    {
        RJISDate::Triple seqDates_;     
        char holderType_;               
        std::string description_;       
        Indicator restrictedByIssue;	
        Indicator restrictedByArea;	    
        Indicator restrictedByTrain;	
        Indicator restrictedByDate;	    
        RailcardCode  masterCode_;      
        Indicator DISPLAY_FLAG; 	    
        NumPassengers maxpassengers;	
        NumPassengers minpassengers;	
        NumPassengers maxholders;		
        NumPassengers minholders;		
        NumPassengers maxaccadults;    
        NumPassengers minaccadults;    
        NumPassengers maxadults;		
        NumPassengers minadults;		
        NumPassengers maxchildren;		
        NumPassengers minchildren;		
        Fare price_;			        
        Fare discountPrice_;		    
        ValidityPeriod validityPeriod_;	
        RJISDate::Date lastValidDate_;	
        Indicator physicalCard_;	    
        CapriCode capriCode_;		    
        StatusCode adultStatus_;	    
        StatusCode childStatus_;	    
        StatusCode aaaStatus_;		    

        RailcardValue() = default;
        RailcardValue(const std::string& str, size_t offset)
        {
            Set(str, offset);
        }

        inline void Set(const std::string & str, size_t offset)
        {
            seqDates_.Set(str, offset);
            holderType_ = str[offset + 24];
            description_ = str.substr(offset + 25, 20);
            restrictedByIssue.Set(str, offset + 45);
            restrictedByArea.Set(str, offset + 46);
            restrictedByTrain.Set(str, offset + 47);
            restrictedByDate.Set(str, offset + 48);
            masterCode_.Set(str, offset + 49);
            DISPLAY_FLAG.Set(str, offset + 52);
            maxpassengers.Set(str, offset + 53);
            minpassengers.Set(str, offset + 56);
            maxholders.Set(str, offset + 59);
            minholders.Set(str, offset + 62);
            maxaccadults.Set(str, offset + 65);
            minaccadults.Set(str, offset + 68);
            maxadults.Set(str, offset + 71);
            minadults.Set(str, offset + 74);
            maxchildren.Set(str, offset + 77);
            minchildren.Set(str, offset + 80);
            price_.Set(str, offset + 83);
            discountPrice_.Set(str, offset + 91);
            validityPeriod_.Set(str, offset + 99);
            if (validityPeriod_[0] == ' ' && validityPeriod_[1] == ' ' && validityPeriod_[2] == ' ' && validityPeriod_[3] == ' ')
            {
                lastValidDate_.Set(str, offset + 103);
            }
            else
            {
                lastValidDate_ = RJISDate::Date(0);
            }
            physicalCard_.Set(str, offset + 111);
            capriCode_.Set(str, offset + 112);
            adultStatus_.Set(str, offset + 115);
            childStatus_.Set(str, offset + 118);
            aaaStatus_.Set(str, offset + 121);
        }
    };

    struct RailcardMinKey
    {
        RailcardCode railcard_;
        TicketCode ticketCode_;
        bool operator<(const RailcardMinKey& other) const
        {
            return std::forward_as_tuple(railcard_, ticketCode_) <
                std::forward_as_tuple(other.railcard_, other.ticketCode_);
        }

        inline void Set(const std::string & str, size_t offset)
        {
            railcard_.Set(str, offset);
            ticketCode_.Set(str, offset + 3);
        }

        RailcardMinKey(const std::string & str, size_t offset)
        {
            Set(str, offset);
        }
    };

    struct RailcardMinValue
    {
        RJISDate::Range dateRange_;
        Fare fare_;
        inline void Set(const std::string & str, size_t offset)
        {
            dateRange_.Set(str, offset);
            fare_.Set(str, offset + 8);
        }
        RailcardMinValue(const std::string & str, size_t offset)
        {
            Set(str, offset);
        }
    };

    // railcard restrictions record (RR) - RSPS5045 section 4.19.18. An important element here is the two character restriction
    // code in columns 23 and 24. It is used as an index into the other records:

    struct RestrictionsRRKey
    {
        CFMarker cf_;
        RailcardCode rlc_;
        RestrictionsRRKey(std::string line, size_t offset)
        {
            cf_.Set(line, offset);
            rlc_.Set(line, offset + 1);
        }
        RestrictionsRRKey(char cf, std::string rlc) : cf_(cf), rlc_(rlc) {}
        bool operator<(const RestrictionsRRKey& other) const
        {
            return std::forward_as_tuple(cf_, rlc_) <
                std::forward_as_tuple(other.cf_, other.rlc_);
        }
    };

    struct RestrictionsRR
    {
        RSequence sequenceNumber_;
        TicketCode ticketCode_;
        RouteCode routeCode_;
        CRSCode location_;
        RestrictionCode restrictionCode_;
        Indicator totalBan_;
        RestrictionsRR(std::string line, size_t offset)
        {
            sequenceNumber_.Set(line, offset);
            ticketCode_.Set(line, offset + 4);
            routeCode_.Set(line, offset + 7);
            location_.Set(line, offset + 12);
            restrictionCode_.Set(line, offset + 15);
            totalBan_.Set(line, offset + 17);
        }
    };

    // Time restriction record - RSPS5045 section 4.19.8 page 49

    struct RestrictionsKey
    {
        CFMarker cf_;
        RestrictionCode restrictionCode_;
        RestrictionsKey(std::string line, size_t offset)
        {
            cf_.Set(line, offset);
            restrictionCode_.Set(line, offset + 1);
        }
        RestrictionsKey(char cf, std::string restrictionCode) : cf_(cf), restrictionCode_(restrictionCode) {}
        bool operator<(const RestrictionsKey& other) const
        {
            return std::forward_as_tuple(cf_, restrictionCode_) <
                std::forward_as_tuple(other.cf_, other.restrictionCode_);
        }
    };

    struct RestrictionsTR
    {
        RSequence sequenceNumber_;
        char outRet_;
        int minutesFrom_;
        int minutesTo_;
        char adv_;
        CRSCode location_;
        char resType_;
        char trainType_;
        Indicator minFare_;

        RestrictionsTR(std::string line, size_t offset)
        {
            sequenceNumber_.Set(line, offset);
            outRet_ = line[offset + 4];
            for (auto i = 0u; i < 8; ++i)
            {
                auto c = line[offset + 5 + i];
                if (!isdigit(c))
                {
                    throw QException("bad character found in number in restriction TR record:'"s + c + "'");
                }
            }
            minutesFrom_ = 600 * (line[offset + 5] - '0') + 60 * (line[offset + 6] - '0') + 10 * (line[offset + 7] - '0') + line[offset + 8] - '0';
            minutesTo_ = 600 * (line[offset + 9] - '0') + 60 * (line[offset + 10] - '0') + 10 * (line[offset + 11] - '0') + line[offset + 12] - '0';

            adv_ = line[offset + 13];
            location_.Set(line, offset + 14);
            resType_ = line[offset + 17];
            trainType_ = line[offset + 18];
            minFare_.Set(line, offset + 19);
        }

    };

    struct RestrictionsHD
    {
        RJISDate::Range dateRange_;
        RJISDate::Dayset days_;

        RestrictionsHD(std::string line, size_t offset, bool isFuture)
        {
            for (auto i = 0u; i < 8; ++i)
            {
                auto c = line[offset + i];
                if (!isdigit(c))
                {
                    throw QException("bad character found in date in restriction TR record:'"s + c + "'");
                }
            }

            // get from and to dates - they are in the form MMDD:
            auto mFrom = 10 * (line[offset] - '0') + line[offset + 1] - '0';
            auto dFrom = 10 * (line[offset + 2] - '0') + line[offset + 3] - '0';
            auto mTo = 10 * (line[offset + 4] - '0') + line[offset + 5] - '0';
            auto dTo = 10 * (line[offset + 6] - '0') + line[offset + 7] - '0';

            // use the year 2013 for now for the start and end dates since they are only in the form MMDD.
            // We will calculate the year after we have read the entire file - this means we have to go through all the
            // HD records and adjust the year:
            int y = 2013;
            dateRange_.Set(y, mFrom, dFrom, y, mTo, dTo);
            days_.Set(line, offset + 8, true);
        }

    };




#if 0 // no longer used
    struct NSDiscKey
    {
        UNLC originCode_;
        UNLC destinationCode_;
        RouteCode route_;
        RailcardCode railcard_;
        TicketCode ticketCode_;
        NSDiscKey(const std::string& str, size_t offset)
        {
            Set(str, offset);
        }

        inline void Set(const std::string & str, size_t offset)
        {
            originCode_.Set(str, offset);
            destinationCode_.Set(str, offset + 4);
            route_.Set(str, offset + 8);
            railcard_.Set(str, offset + 13);
            ticketCode_.Set(str, offset + 16);
        }

        // necessary for std::map:
        bool operator<(const NSDiscKey& other) const
        {
            return std::forward_as_tuple(originCode_, destinationCode_, route_, railcard_, ticketCode_) <
                std::forward_as_tuple(other.originCode_, other.destinationCode_, other.route_, other.railcard_, other.ticketCode_);
        }
    };

    struct NSDiscValue
    {
        RJISDate::Triple dates_;
        UNLC useNLC_;
        NSDiscFlag adultNoDis_;
        AddonAmount adultAddon_;
        NSDiscFlag adultRebook_;
        NSDiscFlag childNoDis_;
        AddonAmount childAddon_;
        NSDiscFlag childRebook_;
        bool deleted_ = false;

        NSDiscValue(const std::string& str, size_t offset)
        {
            Set(str, offset);
        }

        inline void Set(std::string s, size_t offset)
        {
            dates_.Set(s, offset);
            useNLC_.Set(s, offset + 24);
            adultNoDis_.Set(s, offset + 28);
            adultAddon_.Set(s, offset + 29);
            adultRebook_.Set(s, offset + 37);
            childNoDis_.Set(s, offset + 38);
            childAddon_.Set(s, offset + 39);
            childRebook_.Set(s, offset + 47);
        }
    };
#endif // #if 0

    struct SDiscountKey
    {
        StatusCode statusCode_;
        DiscountCategory discountCategory_;

        // necessary for std::map:
        bool operator<(const SDiscountKey& other) const
        {
            return std::forward_as_tuple(statusCode_, discountCategory_) <
                std::forward_as_tuple(other.statusCode_, other.discountCategory_);
        }

        SDiscountKey(const std::string& str, size_t offset)
        {
            Set(str, offset);
        }

        inline void Set(std::string s, size_t offset)
        {
            statusCode_.Set(s, offset);
            discountCategory_.Set(s, offset + 11);
        }
        SDiscountKey(StatusCode statusCode, DiscountCategory discountCategory)
            : statusCode_(statusCode), discountCategory_(discountCategory)
        {
        }

    };

    struct SDiscountValue
    {
        RJISDate::Date endDate_;
        char discountIndicator_;
        Percentage percentage_;
        SDiscountValue(const std::string& str, size_t offset)
        {
            Set(str, offset);
        }

        inline void Set(std::string s, size_t offset)
        {
            endDate_.Set(s, offset);
            discountIndicator_ = s[offset + 10];
            percentage_.Set(s, offset + 11);
        }
    };


    struct StatusKey
    {
        StatusCode statusCode_;
        RJISDate::Date endDate_;

        // necessary for std::map:
        bool operator<(const StatusKey& other) const
        {
            return std::forward_as_tuple(statusCode_, endDate_) <
                std::forward_as_tuple(other.statusCode_, other.endDate_);
        }
        
        StatusKey(const std::string& str, size_t offset)
        {
            Set(str, offset);
        }

        inline void Set(std::string s, size_t offset)
        {
            statusCode_.Set(s, offset);
            endDate_.Set(s, offset + 3);
        }
    };

    struct StatusValue
    {
        RJISDate::Date startDate_;
        std::string atbDesc_;
        std::string ccDesc_; // for CCST tickets
        UTSCode1 utsCode_;
        Fare firstSingleMaxFlat_;
        Fare firstReturnMaxFlat_;
        Fare stdSingleMaxFlat_;
        Fare stdReturnMaxFlat_;
        Fare firstLowerMin_;
        Fare firstHigherMin_;
        Fare stdLowerMin_;
        Fare stdHigherMin_;
        Indicator fsMarker;
        Indicator frMarker;
        Indicator ssMarker;
        Indicator srMarker;

        StatusValue(const std::string& str, size_t offset)
        {
            Set(str, offset);
        }

        inline void Set(std::string s, size_t offset)
        {
            startDate_.Set(s, offset);
            atbDesc_ = s.substr(8, 5);
            ccDesc_ = s.substr(13, 5);
            utsCode_.Set(s, offset + 18);
            firstSingleMaxFlat_.Set(s, offset + 19);
            firstReturnMaxFlat_.Set(s, offset + 27);
            stdSingleMaxFlat_.Set(s, offset + 35);
            stdReturnMaxFlat_.Set(s, offset + 43);
            firstLowerMin_.Set(s, offset + 51);
            firstHigherMin_.Set(s, offset + 59);
            stdLowerMin_.Set(s, offset + 67);
            stdHigherMin_.Set(s, offset + 75);
            fsMarker.Set(s, offset + 83);
            frMarker.Set(s, offset + 84);
            ssMarker.Set(s, offset + 85);
            srMarker.Set(s, offset + 86);
        }
    };

    struct LocationLValue
    {
        RJISDate::Triple seqDates_;
        AdminAreaCode adminAreaCode_;
        std::string description_;
        CRSCode crsCode_;
        UNLC faregroup_;
        CountyCode county_;
        PTECode pteCode_;
        UNLC zoneNLC_;
        char zoneNumber_;
        char region_;
        char hierarchy_;
        std::string ccDescOut_;
        std::string ccDescRet_;
        std::string facilities_;
        char lulDirectionInd_;
        char lulUtsMode_;
        Indicator lulzone1_;
        Indicator lulzone2_;
        Indicator lulzone3_;
        Indicator lulzone4_;
        Indicator lulzone5_;
        Indicator lulzone6_;
        char lulUtsLondon_;
        UTSCode3 utsCode_;
        UTSCode3 utsAltCode_;
        char utsPtrBias_;
        char utsOffset_;
        UTSCode3 utsNorth_;
        UTSCode3 utsEast_;
        UTSCode3 utsSouth_;
        UTSCode3 utsWest_;

        LocationLValue(const std::string& str, size_t offset)
        {
            Set(str, offset);
        }

        inline void Set(std::string s, size_t offset)
        {
            seqDates_.Set(s, offset);
            adminAreaCode_.Set(s, offset + 24);
            description_ = s.substr(offset + 31, 16);
            crsCode_.Set(s, offset + 47);
            faregroup_.Set(s, offset + 60);
            county_.Set(s, offset + 66);
            pteCode_.Set(s, offset + 68);
            zoneNLC_.Set(s, offset + 70);
            zoneNumber_ = s[offset + 74];
            region_ = s[offset + 76];
            hierarchy_ = s[offset + 77];
            ccDescOut_ = s.substr(offset + 78, 41);
            ccDescRet_ = s.substr(offset + 119, 16);
            facilities_ = s.substr(offset + 225, 26);
            lulDirectionInd_ = s[offset + 251];
            lulUtsMode_ = s[offset + 252];
            lulzone1_.Set(s, offset + 253);
            lulzone2_.Set(s, offset + 254);
            lulzone3_.Set(s, offset + 255);
            lulzone4_.Set(s, offset + 256);
            lulzone5_.Set(s, offset + 257);
            lulzone6_.Set(s, offset + 258);
            lulUtsLondon_ = s[offset + 259];
            utsCode_.Set(s, offset + 260);
            utsAltCode_.Set(s, offset + 263);
            utsPtrBias_ = s[offset + 266];
            utsOffset_ = s[offset + 267];
            utsNorth_.Set(s, offset + 268);
            utsEast_.Set(s, offset + 271);
            utsSouth_.Set(s, offset + 274);
            utsWest_.Set(s, offset + 277);
        }
    };

    struct NSDiscEntry
    {
        UNLC originCode_;
        UNLC destinationCode_;
        RouteCode route_;
        RailcardCode railcard_;
        TicketCode ticketCode_;
        RJISDate::Triple dates_;
        UNLC useNLC_;
        NSDiscFlag adultNoDis_;
        AddonAmount adultAddon_;
        NSDiscFlag adultRebook_;
        NSDiscFlag childNoDis_;
        AddonAmount childAddon_;
        NSDiscFlag childRebook_;
        bool deleted_ = false;
        uint32_t matchQuality = 0;

        friend std::ostream& operator<<(std::ostream& str, const NSDiscEntry& nsd);


        NSDiscEntry(const std::string& s, size_t offset)
        {
            Set(s, offset);
        }

        void Set(const std::string& s, size_t offset)
        {
            originCode_.Set(s, offset);
            destinationCode_.Set(s, offset + 4);
            route_.Set(s, offset + 8);
            railcard_.Set(s, offset + 13);
            ticketCode_.Set(s, offset + 16);
            dates_.Set(s, offset + 19);
            useNLC_.Set(s, offset + 43);
            adultNoDis_.Set(s, offset + 47);
            adultAddon_.Set(s, offset + 48);
            adultRebook_.Set(s, offset + 56);
            childNoDis_.Set(s, offset + 57);
            childAddon_.Set(s, offset + 58);
            childRebook_.Set(s, offset + 66);
        }
    };

    inline std::ostream& operator<<(std::ostream& str, const NSDiscEntry& nsd)
    {
        str << nsd.originCode_ << nsd.destinationCode_ << nsd.route_ << nsd.railcard_ << nsd.ticketCode_
            << nsd.dates_ << nsd.useNLC_ << nsd.adultNoDis_ << nsd.adultAddon_ << nsd.adultRebook_ <<
            nsd.childNoDis_ << nsd.childAddon_ << nsd.childRebook_;
        return str;
    }
};
