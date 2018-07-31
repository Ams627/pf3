#include "stdafx.h"
#include "ReadIDMS.h"

ReadIDMS::ReadIDMS(std::string webDir, std::string idmsDir) : webDirectory_(webDir), idmsDirectory_(idmsDir)
{
    std::string locPath = idmsDir + "/" + locFilename;
    TiXmlDocument ldoc(locPath.c_str());
    bool loadOK = ldoc.LoadFile();
    if (loadOK)
    {
        auto stationsReferenceData = ldoc.FirstChildElement("FareLocationsReferenceData");

        int count = 0;
        for (const TiXmlElement* pStation = stationsReferenceData->FirstChildElement("FareLocation");
        pStation; pStation = pStation->NextSiblingElement("FareLocation"))
        {
            const TiXmlElement* pNLCElement = pStation->FirstChildElement("Nlc");
            const TiXmlElement* pNameElement = pStation->FirstChildElement("OJPDisplayName");
            const TiXmlElement* pCRSElement = pStation->FirstChildElement("CRS");

            if (pNLCElement != nullptr)
            {
                auto nlc = pNLCElement->GetText();
                auto name = pNameElement->GetText();
                if (nlc != nullptr && name != nullptr)
                {
                    std::string sname(name);
                    std::string snlc(nlc);
                    ams::Trim(snlc);
                    ams::Trim(sname);
                    if (snlc.length() == 4 && sname.length() != 0)
                    {
                        count++;
                        UNLC unlc(snlc);
                        
                        auto insertResult = locations_.insert(std::make_pair(unlc, sname));
                        if (!insertResult.second)
                        {
                            std::cout << "already inserted for nlc: " << snlc
                                << " and name " << sname << " existing name is " << locations_[unlc] << std::endl;
                        }
                    }
                }
            }
        }
//        std::cout << "loaded file " << filename << " - there are " << locations_.size() << " stations\n" << std::endl;
    }
    else
    {
        throw QException("Cannot read from file: " + locFilename);
    }

    std::string statPath = idmsDir + "/" + stationsFilename;
    TiXmlDocument sdoc(statPath.c_str());
    if (sdoc.LoadFile())
    {
        auto stationsReferenceData = sdoc.FirstChildElement("FareLocationsReferenceData");

        int count = 0;
        for (const TiXmlElement* pStation = stationsReferenceData->FirstChildElement("Station");
        pStation; pStation = pStation->NextSiblingElement("Station"))
        {
            const TiXmlElement* pNLCElement = pStation->FirstChildElement("Nlc");
            const TiXmlElement* pCRSElement = pStation->FirstChildElement("CRS");
            const TiXmlElement* pTiplocElement = pStation->FirstChildElement("Tiploc");

            auto nlc = pNLCElement ? pNLCElement->GetText() : nullptr;
            auto crs = pCRSElement ? pCRSElement->GetText() : nullptr;
            auto tiploc = pTiplocElement ? pTiplocElement->GetText() : nullptr;

            // there must be an NLC otherwise we don't insert anything into the crs and tiploc maps:
            if (nlc)
            {
                UNLC unlc(nlc);
                if (crs)
                {
                    CRSCode crsc(crs);
                    nlcToCRS_.insert(std::make_pair(unlc, crsc));
                }
                if (tiploc)
                {
                    nlcToTiploc_[unlc].push_back(tiploc);
                }
            }
        }
    }
}

//----------------------------------------------------------------------------
//
// Name: WriteJSON
//
// Description: Writes various JSON files are required by a prototypical html5
//              tvm
//
//----------------------------------------------------------------------------
void ReadIDMS::WriteJSON(std::string filename, const std::set<UNLC>& nlcset)
{
    if (locations_.size() > 0)
    {
        std::string outFilename = webDirectory_ + "/" + filename;

        std::ofstream ofs(outFilename);
        if (!ofs)
        {
            throw QException("Cannot open output file " + outFilename);
        }
        ofs << "railLocations = [\n";
        bool first = true;
        for (auto p : locations_)
        {
            auto &nlc = p.first;
            if (nlcset.find(nlc) != nlcset.end())
            {
                if (!first)
                {
                    ofs << ",\n";
                }
                else
                {
                    first = false;
                }
                ofs << "{" << "\"nlc\":\"" << nlc << "\", " << "\"name\":\"" << p.second << "\"" << "}";
            }
        }
        ofs << "\n];\n";
    }

}
