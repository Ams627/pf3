#pragma once
class ServerManagement
{
public:
    ServerManagement() {}
    virtual ~ServerManagement(){}
};


void StartServer(int listeningPort);