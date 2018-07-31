#pragma once
class ReadIDMS
{
    const std::string locFilename = "FareLocationsRefData.xml";
    const std::string stationsFilename = "StationsRefData.xml.xml";
    std::string idmsDirectory_;
    std::string webDirectory_;

    std::map<UNLC, std::string> locations_;     // map NLC to station name
    std::map<UNLC, CRSCode> nlcToCRS_;          // map NLC to CRS code

    // map NLC to tiploc code - there are up to 5 tiplocs per station
    std::map<UNLC, std::vector<std::string>> nlcToTiploc_;   

public:
    ReadIDMS(std::string webdir, std::string idmsdir);
    void WriteJSON(std::string filename, const std::set<UNLC>& nlcset);
    virtual ~ReadIDMS(){}
};
