RJISDate::Triple seqDates_;
char holderType_; // 28
std::string description_; // 29 (length 20)
Indicator RESTRICTED_BY_ISSUE;	// 49
Indicator RESTRICTED_BY_AREA;	// 50
Indicator RESTRICTED_BY_TRAIN;	// 51
Indicator RESTRICTED_BY_DATE;	// 52
RailcardCode  masterCode_;      // 53
Indicator DISPLAY_FLAG; 	// 56
Passengers MAXPASSENGERS;	// 57
Passengers MINPASSENGERS	// 60
Passengers MAXHOLDERS		// 63
Passengers MINHOLDERS		// 66
Passengers MAXACC_ADULTS        // 69
Passengers MINACC_ADULTS        // 72
Passengers MAXADULTS		// 75
Passengers MINADULTS		// 78
Passengers MAXCHILDREN		// 81
Passengers MINCHILDREN		// 84
Fare price_;			// 87
Fare discountPrice_;		// 95
ValidityPeriod validityPeriod_;	// 103
RJISDate::Date lastValidDate_;	// 107
Indicator physicalCard_;	// 115
CapriCode capriCode_;		// 116
StatusCode adultStatus_;	// 119
StatusCode childStatus_;	// 122
StatusCode aaaStatus_;		// 125









