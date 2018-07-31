#pragma once
#include "globals.h"
#include "HTTPManager.h"
#include <ams/async/AIOCP.h>
class ServerSocketException : public std::exception
{
public:
    ServerSocketException(std::string s) : std::exception(s.c_str()) {}
};

namespace {
    // AMS DEBUG
const int bufsize = 2 * 1024 * 1024;
const int addrsize = 64;
}

enum class Optypes
{
    opaccept, 
    opread,
    opwrite,
    optransmitfile,
    opdisconnect,
    operrorClosed
};

class ServerSocket;

// this structure has one instance per-client connection and needs to be updated by users of the ServerSocket
// class before starting an async operation (read, write or accept). The *system* updates this structure
// *during* the async operation - it MUST BE VALID for the duration of the async operation - this means
// a stack instance must not go out of scope and any allocated instance must not be freed.
// The OVERLAPPED member must come first. This class must not inherit from another class without specifying 
// novtable since the OVERLAPPED member must be first - inheriting from another class will put the vtable
// first.

struct ClientContext
{
public:
    OVERLAPPED ol;                          // must be first and event must be zeroed before an async op is started
private:
    std::vector<BYTE> buffer;               // the buffer to read or write (accept potentially fills this buffer too)
public:
    int compid;                             // AMS completion ID for debugging purposes
    int bytesSent;                          // total bytesSent on this socket
    static int s_compid;                    // AMS current completion ID for debugging purposes
    ServerSocket *pServerSocket;            // instance of the server socket class to which this context belongs
    SOCKET socket;                          // socket passed to AcceptEx
    HANDLE hfile;                           // HANDLE for TransmitFile - must be closed on completion
    Optypes operation;                      // indicates accept, read or write
    DWORD bytesReceived;                    // passed to AcceptEx - returns the number of bytes read if AE completes synchronously
    HTTPManager httpmanager;                // http manager for this socket
    int index;                              // index
    operator LPOVERLAPPED() {return &ol;}   // returns the address of this structure so it can be used in async calls
    size_t GetBufSize() {return bufsize;}   // Getter for buffer size
    size_t GetAddrSize() {return addrsize;} // Getter for address size
    BYTE* Data() { return buffer.data(); }
    bool reuse;
    bool disconnect;
public:
    // construct - initialise the buffer and DEFAULT-INITIALISE overlapped
    ClientContext() : buffer(bufsize + 2 * addrsize),
        httpmanager(buffer.data(), bufsize), ol(), disconnect(false), reuse(false), compid(0), bytesSent(0) {}
private:
    ClientContext(const ClientContext&) : httpmanager(0, 0) {}
};

__declspec(selectany) int ClientContext::s_compid;

class ServerSocket
{
    static bool startup_;
    static LPFN_ACCEPTEX AcceptEx;
    static LPFN_DISCONNECTEX DisconnectEx;
    static LPFN_TRANSMITFILE TransmitFile;
    static bool init_;
    AIOCP& iocp_;
    SOCKET listenSocket_;
    std::vector<ClientContext> clientcontexts_;

public:
    // constructor - allocate a number of acceptsocks - one per expected client:
    ServerSocket(int maxclients, AIOCP& iocp) : clientcontexts_(maxclients), iocp_(iocp)
    {
    }

    AIOCP& GetIOCP() { return iocp_; }

    HANDLE GetListenHandle() {return (HANDLE)listenSocket_;}

    // return the accept handles as an array of HANDLEs. T must support push_back
    template <class T> void GetAcceptHandles(T& output)
    {
        for (auto& context : clientcontexts_)
        {
            output.push_back(reinterpret_cast<HANDLE>(context.socket));
        }
    }

    void Init();
    void ReinitSocket(ClientContext&);
    void Start(std::string address, int port);
    void ProcessCompletion(ULONG_PTR key, LPOVERLAPPED pol, DWORD bytesTransferred, DWORD error);
    void Accept(ClientContext& context);
    void Read(ClientContext & context);
    void Write(ClientContext & context, int bytes);
    void SendFile(ClientContext & context, int headerLength, std::string filename);
    void Disconnect(ClientContext & context);
};



inline LPFN_ACCEPTEX GetAcceptEx()
{
    SOCKET s1 = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    LPFN_ACCEPTEX result;
    GUID GuidAcceptEx = WSAID_ACCEPTEX;
    DWORD dwbytes;
    int res = WSAIoctl(s1, SIO_GET_EXTENSION_FUNCTION_POINTER,
        &GuidAcceptEx, sizeof (GuidAcceptEx),
        &result, sizeof (result),
        &dwbytes, NULL, NULL);
    if (res == SOCKET_ERROR)
    {
        int error = WSAGetLastError();
        throw ServerSocketException("WSAIoctl failed to get address for AcceptEx");
    }
    return result;
}

inline LPFN_DISCONNECTEX GetDisconnectEx()
{
    SOCKET s1 = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    LPFN_DISCONNECTEX result;
    GUID GuidDisconnectEx = WSAID_DISCONNECTEX;
    DWORD dwbytes;
    int res = WSAIoctl(s1, SIO_GET_EXTENSION_FUNCTION_POINTER,
        &GuidDisconnectEx, sizeof (GuidDisconnectEx),
        &result, sizeof (result),
        &dwbytes, NULL, NULL);
    if (res == SOCKET_ERROR)
    {
        throw ServerSocketException("WSAIoctl failed to get address for GuidDisconnectEx");
    }
    return result;
}

inline LPFN_TRANSMITFILE GetTransmitFile()
{
    SOCKET s1 = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    LPFN_TRANSMITFILE result;
    GUID GuidTrasmitFile = WSAID_TRANSMITFILE;
    DWORD dwbytes;
    int res = WSAIoctl(s1, SIO_GET_EXTENSION_FUNCTION_POINTER,
        &GuidTrasmitFile, sizeof (GuidTrasmitFile),
        &result, sizeof (result),
        &dwbytes, NULL, NULL);
    if (res == SOCKET_ERROR)
    {
        throw ServerSocketException("WSAIoctl failed to get address for GuidTrasmitFile");
    }
    return result;
}


inline bool Startup()
{
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != NO_ERROR)
    {
        throw ServerSocketException("WSAStartup failed - cannot start winsock");
    }
    return false;
}