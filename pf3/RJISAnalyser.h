#pragma once
#include <ams/stringutils.h>

class RJISAnalyser
{
	std::map<int, int> rjisfilemap_;			// count of number of files for each RJIS file set number in the specified folder
	std::map<std::string, int> rjistypemap_;	// for each RJIS file extension, store the set number in the map
    RJISAnalyser();
public:

    static RJISAnalyser& GetInstance()
    {
        static RJISAnalyser instance;
        return instance;
    }

	virtual ~RJISAnalyser() {}

	int GetNumberOfFilesInSet(int set)
	{
		auto result = 0;
		auto p = rjisfilemap_.find(set);
		if (p != rjisfilemap_.end())
		{
			result = p->second;
		}
		return result;
	}

	std::string GetFileSetsAsString()
	{
		std::string result;
		std::string separator = ", ";
		for (auto p : rjisfilemap_)
		{
			result = result + std::to_string(p.second) + separator;
		}
		// get rid of the final comma before returning:
		return result.substr(0, result.length() - separator.length());
	}

	size_t GetNumberOfSets()
	{
		return rjisfilemap_.size();
	}

	const std::set<std::string>& GetValidExtensionSet();

	// given an extension (e.g.) FFL, get the three digit RJIS set number:
	int GetSetNumber(std::string s)
	{
		int result;
		ams::MakeUpper(s);
		auto p = rjistypemap_.find(s);
		if (p == rjistypemap_.end())
		{
			result = -1;
		}
		else
		{
			result = p->second;
		}
		return result;
	}

    std::string GetFilename(std::string ext)
    {
        std::string result;
        ams::MakeUpper(ext);

        // get the set number:
        auto p = rjistypemap_.find(ext);
        if (p == rjistypemap_.end())
        {
            result = "";
        }
        else
        {
            result = MakeRJISFilename(p->second, ext);
        }
        return result;
    }

    static std::string MakeRJISFilename(int rjisSetNumber, std::string ext)
    {
        if (rjisSetNumber < 0 || rjisSetNumber > 999)
        {
            throw QException("Cannot make RJIS filename from integer " +
                std::to_string(rjisSetNumber) + " (Must be from 0 to 999)");
        }
        std::vector<char> currentDirectory(_MAX_PATH + 1);
        DWORD dirLength = GetCurrentDirectory(static_cast<DWORD>(currentDirectory.size()), currentDirectory.data());
        std::string sDir(currentDirectory.begin(), currentDirectory.begin() + dirLength);
        if (sDir.back() != '\\')
        {
            sDir += '\\';
        }

        std::ostringstream oss;
        std::string sep = ext.empty() ? "" : ".";
        oss << sDir << "RJFAF" << std::dec << std::setfill('0') << std::setw(3) << rjisSetNumber << sep << ext;
        return oss.str();
    }

};
