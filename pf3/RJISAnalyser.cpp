#include "stdafx.h"
#include "RJISAnalyser.h"

namespace {
	const std::set<std::string> s_validExtensions = {
        "DAT", "DIS", "FFL", "FNS", "FRR", "FSC", "LOC", "NDF",
        "NFO", "RCM", "RLC", "RST", "RTE", "SUP", "TAP", "TCL",
        "TJS", "TOC", "TPB", "TPK", "TPN", "TRR", "TSP", "TTY", "TVL", "AGS" };
}
int GetSetNumberFromFilename(std::string filename)
{
	int result = -1;
	if (filename.length() >= 8)
	{
		if (isdigit(filename[5]) && isdigit(filename[6]) && isdigit(filename[7]))
		{
			result = 100 * (filename[5] - '0') + 10 * (filename[6] - '0') + (filename[7] - '0');
		}
	}
	return result;
}

const std::set<std::string>& RJISAnalyser::GetValidExtensionSet()
{
	return s_validExtensions;
}


std::string GetExtension(std::string filename)
{
	auto p = filename.find_last_of('.');
	std::string result;
	if (p != std::string::npos)
	{
		result = filename.substr(p + 1);
	}
	return result;
}

RJISAnalyser::RJISAnalyser()
{
	WIN32_FIND_DATA fdata;
	HANDLE h = FindFirstFile("RJFAF*", &fdata);

	if (h != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (!(fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				std::string filename(fdata.cFileName);
				int setnumber = GetSetNumberFromFilename(filename);
				if (setnumber != -1)
				{
					std::string ext = GetExtension(filename);
                    ams::MakeUpper(ext);
					// store the set number for each extension if the extension is in the set of 
					// valid extensions:
					if (s_validExtensions.find(ext) != s_validExtensions.end())
					{
						rjisfilemap_[setnumber]++;
						ams::MakeUpper(ext);
						rjistypemap_[ext] = setnumber; 
					}
				}
			}
		} while (FindNextFile(h, &fdata));
		FindClose(h);
	}
}