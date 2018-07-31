#include "stdafx.h"
#include <ams/stringutils.h>
#include <ams/codetiming.h>
#include <ams/jsonutils.h>
#include <ams/jsutils.h>
#include "HTTPManager.h"
#include "msgthread.h"
#include "ProcessFareList.h"
#include "ProcessTimetableRequest.h"
#include "config.h"
#include "globals.h"
#include "mimetypesmap.h"

namespace {

void GetODR(bool& success, std::string& origin, std::string& destination, std::string& railcard, const std::string& url)
{
	success = false;
	if (url[0] == '?')
	{
		auto pos1 = url.find('&', 1);
		if (pos1 != std::string::npos)
		{
			auto pos2 = url.find('&', pos1 + 1);
			if (pos2 != std::string::npos)
			{
				std::vector<std::string> nvpairs;
				nvpairs.push_back(url.substr(1, pos1 - 1));
				nvpairs.push_back(url.substr(pos1 + 1, pos2 - pos1 - 1));
				nvpairs.push_back(url.substr(pos2 + 1));

				std::string o, d, r;
				for (auto s : nvpairs)
				{
					auto epos = s.find('=');
					if (epos != std::string::npos)
					{
						std::string name = s.substr(0, epos);
						if (name == "o")
						{
							o = s.substr(epos + 1);
						}
						else if (name == "d")
						{
							d = s.substr(epos + 1);
						}
						else if (name == "r")
						{
							r = s.substr(epos + 1);
						}
					}
				}
				if (o.length() == 4 && d.length() == 4)
				{
					origin = o;
					destination = d;
					railcard = r;
					success = true;
				}
			}
		}
	}
}
}

int64_t HTTPManager::perfFreq = HTTPManager::GetPerfFrequency();

// call this function with a chunk of data from the connected socket. It stores the http headers in the request_ list
// and returns false if the header is incomplete.
// When the header is eventually complete - signalled by CRLF on a line by itself (i.e. two CRLFs in a row) then
// this function will call ProcessCompleteRequest then return true.
bool HTTPManager::PushData(std::vector<BYTE>& input)
{
    bool result = false;
    bool cr = false;
    std::string line;
    for (BYTE b : input)
    {
        if (b == '\r')
        {
            cr = true;
        }
        else if (b == '\n')
        {
            // a \r\n on a line by itself indicates the end of the header:
            if (line.length() == 0)
            {
                result = true;
                OutputDebugString(request_[0].c_str());OutputDebugString("\n");
                ProcessCompleteRequest();
                request_.clear();
            }
            else
            {
                // otherwise, trim and store the line
                ams::Trim(line);
                request_.push_back(line);
                line.clear();
            }
        }
        else
        {
            line += static_cast<char>(b);
        }
    }
    return result;
}

// this function will process complete http requests (i.e. once a complete http header is received)
void HTTPManager::ProcessCompleteRequest()
{
    std::ostringstream oss;
    oss << "Request is " << request_[0];
    REPORTMESSAGE(oss.str(), 0);

    size_t pos = request_[0].find_first_of("\t ");
    if (pos == std::string::npos)
    {
        throw HTTPException(400, "Invalid Request Method"); // will send 400 bad request
    }
    // here we must be able to find a non-white space:
    std::string method = request_[0].substr(0, pos);
    if (method == "GET")
    {
        // find the first char of the URI - we know it is there since we found a white
        // space at the beginning and the string is trimmed (therefore there can be no trailing white
        // space)
        size_t pos2 = request_[0].find_first_not_of("\t ", pos);

        // find the white space after the URI - the one between the URI and the protocol:
        size_t pos3 = request_[0].find_first_of("\t ", pos2 + 1);
        if (pos3 == std::string::npos)
        {
            throw HTTPException(400, "Invalid protocol");
        }
        size_t pos4 = request_[0].find_first_not_of("\t ", pos3 + 1);
        std::string uri = request_[0].substr(pos2, pos3 - pos2);
        std::string protocol = request_[0].substr(pos4);

        ProcessGet(uri);
    }
    else
    {
        throw HTTPException(400, "Invalid Request Method"); // will send 400 bad request
    }
}

void HTTPManager::GenerateJSON(std::string& json,
    uint64_t elapsed,
    const FareSearchParams& originalSearchParams,
    const FoundPlusBus& plusbusResultsMap,
    const FareResultsMap& fareResultsMap,
    const std::vector<TTTypes::Journey> journeys
    ) const
{
    uint64_t utcJS = ams::GetJSDateMilliseconds();
    json = "{" + ams::JSON::Key("tech") + "{" + ams::JSON::NVPair("serverutc", utcJS, true) +
        ams::JSON::NVPair("serverCPU", elapsed, true) + ams::JSON::NVPair("computerID", GetComputerID()) + "},";
    json += ams::JSON::Key("fares") + "{" + ams::JSON::Key("ftec") + "{" + ams::JSON::NVPair("version", 989, true)
        + ams::JSON::NVPair("ndfVersion", 822) + "},";
    json += ams::JSON::Key("result") + "[{" +
        ams::JSON::NVPair("rlc", originalSearchParams.railcard_.GetString(), true);

    // only add a plusbus element if there are any fares:
    if (!plusbusResultsMap.pbFares_.empty())
    {
        json += ams::JSON::Key("plusbus") + "{" +
            ams::JSON::NVPair("o", plusbusResultsMap.origin_.GetString(), true) +
            ams::JSON::NVPair("d", plusbusResultsMap.destination_.GetString(), true) +
            ams::JSON::NVPair("po", plusbusResultsMap.pbOrigin_.GetString(), true) +
            ams::JSON::NVPair("pd", plusbusResultsMap.pbDestination_.GetString(), true) +
            ams::JSON::Key("fares") + "{";

        for (auto p : plusbusResultsMap.pbFares_)
        {
            json += ams::JSON::Key(p.first) +
                "{" + ams::JSON::NVPair("a", p.second.first, true) + ams::JSON::NVPair("c", p.second.second) + "},";
        }
        if (json.back() == ',')
        {
            json = json.substr(0, json.length() - 1);
        }
        json += "}},";
    }

    json += ams::JSON::Key("flows") + "[";
    // now add array of flows:
    bool first = true;
    for (auto p : fareResultsMap)
    {
        if (first)
        {
            first = false;
        }
        else
        {
            json += ',';
        }
        json += "{";
        json += ams::JSON::NVPair("o", p.first.flow_.origin.GetString(), true) +
            ams::JSON::NVPair("d", p.first.flow_.destination.GetString(), true) +
            ams::JSON::NVPair("route", p.first.route_.GetString(), true) +
            ams::JSON::NVPair("flowid", p.first.flowid_, true) +
            ams::JSON::NVPair("discount", p.first.discInd_, true) +
            ams::JSON::Key("fares") + "[";
        bool first2 = true;
        for (auto q : p.second)
        {
            if (first2)
            {
                first2 = false;
            }
            else
            {
                json += ',';
            }
            json += "{";
            json += ams::JSON::NVPair("a", q.adultPrice_, true) + ams::JSON::NVPair("c", q.childPrice_, true) +
                ams::JSON::NVPair("t", q.ticketcode_.GetString(), true) +
                ams::JSON::NVPair("cl", q.ticketClass_, true) +
                ams::JSON::NVPair("tt", q.ticketType_, true) +
                ams::JSON::NVPair("r", q.restrictionCode_.GetString());
            json += "}"; // close single fare element
        }
        json += "]"; // close fare array 
        json += "}"; // close single flow element
    }
    json += "]"; // close flow array
    json += "}"; // close single result element
    json += "]"; // close result array (only one element at the moment)
    json += "},"; // close fares element
    json += ams::JSON::Key("times") + "{" +
        ams::JSON::NVPair("ocrs", originalSearchParams.crsOrigin_.GetString(), true) +
        ams::JSON::NVPair("dcrs", originalSearchParams.crsDestination_.GetString(), true) +
        ams::JSON::Key("journeys") + "[";
    for (auto p : journeys)
    {

        json += "{" + ams::JSON::NVPair("dep", p.GetFirst().time, true) +
            ams::JSON::NVPair("arr", p.GetLast().time) + "},";
    }
    if (json.back() == ',')
    {
        json = json.substr(0, json.length() - 1);
    }

    json += "]"; // close journeys array
    json += "}"; // close times element
    json += "}"; // close json
}


void HTTPManager::ProcessGet(std::string uri)
{
    static const std::string RJISURI = "/PFRJIS";
    bool found = false;
    std::string responseBody;
    std::string responseString;
	size_t compareLength = RJISURI.length();
    if (uri.substr(0, compareLength) == RJISURI)
    {
        found = true;
        file = false;
		std::string origin;
		std::string destination;
		std::string railcard;
		bool success;
		GetODR(success, origin, destination, railcard, uri.substr(compareLength));
		
		ams::MakeUpper(origin);
		ams::MakeUpper(destination);
		ams::MakeUpper(railcard);

        if (railcard.empty())
        {
            railcard = "   ";
        }
		//std::cout << "o is " << origin << std::endl;
		//std::cout << "d is " << destination << std::endl;
		//std::cout << "r is " << railcard << std::endl;

		if (success)
		{
			responseString = "HTTP/1.0 200 OK\r\nAccess-Control-Allow-Origin: *\r\n";
			RJISDate::Date travelDate(RJISDate::Date::Today());
			ProcessFareList farelist;

            LARGE_INTEGER prefareTime, postfareTime;
            QueryPerformanceCounter(&prefareTime);

            // get all rail fares:
            FareResultsMap fareResults;
            FareSearchParams searchParams(origin, destination, railcard);
			farelist.GetAllFares(fareResults, searchParams);
            // Get all plusbusFares:
            FoundPlusBus plusbusFares;
            farelist.GetPlusbusFares(plusbusFares, searchParams);

            std::vector<TTTypes::Journey> journeys;
            ProcessTimetableRequest timetableReq;
            timetableReq.GetTimes(journeys, searchParams);

            QueryPerformanceCounter(&postfareTime);
            std::cerr << "Got fares... in time " << ams::LiDiff(postfareTime, prefareTime) * 1'000'000 / perfFreq<< " microseconds\n";

            std::string json;
            GenerateJSON(json, ams::LiDiff(postfareTime, prefareTime) * 1'000'000 / perfFreq, searchParams, plusbusFares, fareResults, journeys);
            
            // AMS debug
            //std::ofstream ofs("c:/temp/json.txt");
            //ofs << json << std::endl;
            //ofs.close();

            responseBody = json;
		}
		else
		{
			responseString = "HTTP/1.0 200 OK\r\nAccess-Control-Allow-Origin: *\r\n";
			RJISDate::Date travelDate(RJISDate::Date::Today());
//			std::cout << "uri: " << uri << "\n";
			responseBody = 
				"<?xml version = \"1.0\" encoding = \"UTF-8\" ?>\n"
				"<powerfares>Bad request</powerfares>\n";
		}
    }
    else if (uri[0] == '/' && uri.length() < 255)
    {
        std::string dir = Config::directories.GetDirectory("docroot");
        std::string notFoundURL = dir + "/404.html";
        if (uri == "/fares")
        {
            uri = "/index.html";
        }
        dir += uri;
        REPORTMESSAGE(dir, 0);
        LARGE_INTEGER size;
        WIN32_FILE_ATTRIBUTE_DATA fad;
        if (GetFileAttributesEx(dir.c_str(), GetFileExInfoStandard, &fad))
        {
            size.HighPart = fad.nFileSizeHigh;
            size.LowPart = fad.nFileSizeLow;
            filename = dir;
            file = true;
            found = true;
            filesize = size.QuadPart;
            responseString = "HTTP/1.1 200 OK\r\n";

            // get the content-type 
            std::string contentType = "";
            auto pos = uri.rfind('.');
            if (pos != std::string::npos)
            {
                std::string ext = uri.substr(pos + 1);
                contentType = GetMimeType(ext);
            }
            if (!contentType.empty())
            {
                responseString += "Content-Type: " + contentType + "\r\n";
//                std::cout << "*** " << responseString + "\n";
            }
        }
        else if (GetFileAttributesEx(notFoundURL.c_str(), GetFileExInfoStandard, &fad))
        {
            size.HighPart = fad.nFileSizeHigh;
            size.LowPart = fad.nFileSizeLow;
            filename = notFoundURL;
            file = true;
            found = true;
            filesize = size.QuadPart;
            responseString = "HTTP/1.1 404 Not Found\r\n";

            // get the content-type 
            std::string contentType = "";
            auto pos = notFoundURL.rfind('.');
            if (pos != std::string::npos)
            {
                std::string ext = notFoundURL.substr(pos + 1);
                contentType = GetMimeType(ext);
            }
            if (!contentType.empty())
            {
                responseString += "Content-Type: " + contentType + "\r\n";
                //                std::cout << "*** " << responseString + "\n";
            }
        }
        else
        {
            std::cout << "plan zeta!\n";
            DWORD error = GetLastError();
            // the file does not exist:
            responseString = "HTTP/1.0 404 Not Found\r\n";
            responseBody = "<!doctype html>\n<html lang=\"en\">\n<head>\n<title>Not found</title>\n<style type='text/css'>\nbody{\nfont-size:2em;\n}\n</style>\n<script>\n</script>\n</head>\n<body>\n404 - resource not found</body>\n</html>\n";
        }
    }
    uint64_t contentLength;
    if (file)
    {
        contentLength = filesize;
    }
    else
    {
        contentLength = responseBody.length();
    }
    if (found)
    {
        responseString += "Content-Length: " + std::to_string(contentLength) + "\r\n";
    }
    responseString += "\r\n";
    if (!file)
    {
//        std::cout << "sent JSON of length " << responseBody.length() << std::endl;
        responseString += responseBody;
    }

//    std::cout << "LENGTH: " << responseString.length() << "\n";
    // this DANGEROUS LOOP NEEDS CHANGING AMS
    int i = 0;
    for (auto c : responseString)
    {
        response[i++] = c;
    }
    responseLength = i;
}

