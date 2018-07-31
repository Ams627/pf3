#include "stdafx.h"
#include "globals.h"

std::string g_workingDirectory;
std::map<SOCKET, std::vector<std::string>> sockmap;
uint64_t g_requests = 0;