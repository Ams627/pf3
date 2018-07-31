#include "stdafx.h"
#include "ServerSocket.h"
#include "ExTCPTable.h"
#include <ams/errorutils.h>
#include "msgthread.h"

int SocketTime(SOCKET hSocket);

bool ServerSocket::init_ = false;
bool ServerSocket::startup_ = Startup();

LPFN_ACCEPTEX ServerSocket::AcceptEx = GetAcceptEx();
LPFN_DISCONNECTEX ServerSocket::DisconnectEx = GetDisconnectEx();
LPFN_TRANSMITFILE ServerSocket::TransmitFile = GetTransmitFile();


void ServerSocket::Init()
{
    ClientContext::s_compid = 0;
    listenSocket_ = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket_ == INVALID_SOCKET)
    {
        throw ServerSocketException("Cannot create socket - call to socket() failed");
    }

    // ensure a dual stack (ipv4 and ipv6) socket - turn off the "ipv6 only" flag:
    int off = 0;
    setsockopt(listenSocket_, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&off, sizeof(off));

    // create a socket for each client - these will be used in a call to AcceptEx
    int index = 0;
    for (auto& context : clientcontexts_)
    {
        context.index = index++;
        context.socket = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
        if (context.socket == INVALID_SOCKET)
        {
            throw ServerSocketException("Cannot create socket - call to socket() failed");
        }
        // ensure a dual stack socket:
        int off = 0;
        setsockopt(context.socket, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&off, sizeof(off));
        context.pServerSocket = this;
        sockmap[context.socket].push_back("init");
    }
    init_ = true;
}    

void ServerSocket::Start(std::string address, int port)
{
    struct addrinfo hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // this means that we will find the IP address when we are the server. It means we should set the
    // first parameter of getaddrinfo to zero:
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo *resolvedAddress = NULL;
    std::string sPort = std::to_string(port);
    DWORD result = getaddrinfo(0, sPort.c_str(), &hints, &resolvedAddress);


    struct sockaddr_in  *pSockaddr4;
    
    char ipstringbuffer[46];
    DWORD ipbufferlength = 46;
    int iRetval;

    for (auto ptr = resolvedAddress; ptr != NULL; ptr = ptr->ai_next)
    {
        //printf("getaddrinfo response %d\n", i++);
        //printf("\tFlags: 0x%x\n", ptr->ai_flags);
        //printf("\tFamily: ");

        if (ptr->ai_family == AF_UNSPEC)
        {
            //printf("Unspecified\n");
        }
        else if (ptr->ai_family == AF_INET)
        {
            //printf("AF_INET (IPv4)\n");
            pSockaddr4 = (struct sockaddr_in *) ptr->ai_addr;
            //printf("\tIPv4 address %s\n", inet_ntoa(pSockaddr4->sin_addr));
            int sz = sizeof sockaddr_in;
            // AMS - ok - we don't call bind twice - I thought we would have to call it for the IPV4 socket too
            // but apparently not!! (it has already been called for the IPv6 socket)
#if 0
            if (bind(listenSocket_, (SOCKADDR *)pSockaddr4, sz) == SOCKET_ERROR)
            {
                int error = WSAGetLastError();
                std::string s = ams::GetSockErrorAsString();
                throw ServerSocketException("bind failed for listen socket");
            }
#endif
        }
        else if (ptr->ai_family == AF_INET6)
        {
            //printf("AF_INET6 (IPv6)\n");
            LPSOCKADDR sockaddr6 = (LPSOCKADDR)ptr->ai_addr;

            // AMS - need to check static_cast<int> here - we need this for 64 bit code:
            if (bind(listenSocket_, sockaddr6, static_cast<int>(ptr->ai_addrlen)) == SOCKET_ERROR)
            {
                std::string s = ams::GetSockErrorAsString();
                throw ServerSocketException("bind failed for listen socket");
            }

            // The buffer length is changed by each call to WSAAddresstoString
            // So we need to set it for each iteration through the loop for safety
            ipbufferlength = 46;
            iRetval = WSAAddressToString(sockaddr6, (DWORD)ptr->ai_addrlen, NULL,
                ipstringbuffer, &ipbufferlength);
            //if (iRetval)
                //printf("WSAAddressToString failed with %u\n", WSAGetLastError());
            //else
                //printf("\tIPv6 address %s\n", ipstringbuffer);
        }
        else if (ptr->ai_family == AF_NETBIOS)
        {
            printf("AF_NETBIOS (NetBIOS)\n");
        }
        else
        {
            printf("Other %ld\n", ptr->ai_family);
        }
    }


    int res = listen(listenSocket_, SOMAXCONN);
    if (res == SOCKET_ERROR)
    {
        throw ServerSocketException("listen failed for listen socket");
    }

    int index = 0;
    for (auto& context : clientcontexts_)
    {
        Accept(context);
    }
}

// reinitialise a socket closed by the remote client - i.e. after one of the following errors:
// WSAECONNABORTED, WSAECONNRESET, WSAEDISCON, WSAENETDOWN, WSAENETRESET, WSAETIMEDOUT, WSA_OPERATION_ABORTED

void ServerSocket::ReinitSocket(ClientContext& clientContext)
{
    auto& s = clientContext.socket;
    auto oldsocket = s;
    // close the socket connection immediately and do not linger - note that calling closesocket will
    // remove the socket handle from the IO completion port associated list
    linger li;
    li.l_linger = 0;
    li.l_onoff = 1;
    int sockResult;
    sockResult = setsockopt(s, SOL_SOCKET, SO_LINGER, (char*)&li, sizeof linger);
    if (sockResult != 0)
    {
        std::string errString = "setsockopt failed: error is: " + ams::GetSockErrorAsString();
        throw ServerSocketException(errString);
    }
    sockResult = closesocket(s);
    if (sockResult != 0)
    {
        std::string errString = "closesocket failed: error is: " + ams::GetSockErrorAsString();
        throw ServerSocketException(errString);
    }
    s = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET)
    {
        std::string errString = "call to socket() failed: error is: " + ams::GetSockErrorAsString();
        throw ServerSocketException(errString);
    }

    // set the socket to dual-stack node so that IPV4 is also supported:
    int off = 0;
    sockResult = setsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&off, sizeof(off));
    if (sockResult != 0)
    {
        std::string errString = "setsockopt failed: error is: " + ams::GetSockErrorAsString();
        throw ServerSocketException(errString);
    }
    std::ostringstream oss;
    oss << "successfully reopened socket: " << oldsocket << "->" << s << "\n";
    OutputDebugString(oss.str().c_str());

    // associate the new socket with the original IO completion port:
    clientContext.pServerSocket->GetIOCP().Associate((HANDLE)s, 16384);

    // Prevent immediate completion from triggering a completion packet. In other words if a function like
    // WSARecv return zero to indicate success, then we process the data immediately and a completion packet
    // is not sent to the completion port by the system. 
    SetFileCompletionNotificationModes((HANDLE)s, FILE_SKIP_COMPLETION_PORT_ON_SUCCESS);
}

void ServerSocket::ProcessCompletion(ULONG_PTR key, LPOVERLAPPED pol, DWORD bytesTransferred, DWORD error)
{
    ClientContext *pClientContext = (ClientContext *)pol;
    std::ostringstream oss; oss << "Processing completion id " << pClientContext->compid << " on item " << pClientContext->index;
    SENDDLMESSAGE(oss.str());
    if (pClientContext->operation == Optypes::operrorClosed)
    {
        // the socket was closed (probably by the remote end) - we have already reinitialised the socket, so we will 
        // call accept on the new socket
        Accept(*pClientContext);
    }
    else if (pClientContext->operation == Optypes::opaccept)
    {
        // AMS DEBUG
        //int res = setsockopt(
        //    pClientContext->socket,
        //    SOL_SOCKET,
        //    SO_UPDATE_ACCEPT_CONTEXT,
        //    (char *)&listenSocket_,
        //    sizeof(listenSocket_));
        //if (res == SOCKET_ERROR)
        //{
        //    int error = WSAGetLastError();
        //    ExtTcpTable t;
        //    size_t n = t.GetCount();
        //    std::string msg = "setsockopt failed - error is: " + ams::GetSockErrorAsString(error);
        //    REPORTMESSAGE(msg, pClientContext->socket);
        //    Sleep(5000);
        //    throw ServerSocketException("setsockopt failed");
        //}

        //std::ostringstream oss;
        //for (size_t i = 0; i < bytesTransferred; ++i)
        //{
        //    oss << (char)pClientContext->Data()[i];
        //}
//        REPORTMESSAGE(oss.str(), 0);
        if (bytesTransferred == 0)
        {
            OutputDebugString("Zero byte accept - reading\n");
            Read(*pClientContext);
        }
        else
        {
            g_requests++;
            std::vector<BYTE> vb(pClientContext->Data(), pClientContext->Data() + bytesTransferred);
            if (!pClientContext->httpmanager.PushData(vb))
            {
                Read(*pClientContext);
            }
            else
            {
                // An http response is available:
                int l = pClientContext->httpmanager.GetResponseLength();
                if (pClientContext->httpmanager.IsFile())
                {
                    SendFile(*pClientContext, l, pClientContext->httpmanager.GetFilename());
                }
                else
                {
                    Write(*pClientContext, l);
                }
                pClientContext->bytesSent += l;
            }
        }
    }
    else if (pClientContext->operation == Optypes::opwrite)
    {
        if (g_requests == 0)
        {
            std::ostringstream oss;
            //oss << "Number of client sockets is: " << pClientContext->pServerSocket->clientcontexts_.size();
            REPORTMESSAGE(oss.str(), 0);
        }
//        g_requests++;
        if (g_requests % 10000 == 0)
        {
            std::ostringstream oss;
            oss << "complete request number " << g_requests << "\n";
            REPORTMESSAGE(oss.str(), pClientContext->socket);
            for (auto& c : pClientContext->pServerSocket->clientcontexts_)
            {
                oss.str("");
                oss << "index: " << std::dec << std::setw(4) << std::setfill(' ') << c.index <<
                    " socket: " << std::setfill('0') << std::setw(8) << std::hex << c.socket <<
                    " bytes: " << std::dec << c.bytesSent;
                REPORTMESSAGE(oss.str(), 0);
            }
            REPORTMESSAGE("----------------", 0);
        }
        if (pClientContext->disconnect)
        {
            REPORTMESSAGE("Disconnecting", pClientContext->socket);
            Disconnect(*pClientContext);
        }
        else
        {
            REPORTMESSAGE("Reading", pClientContext->socket);
            Read(*pClientContext);
        }
    }
    else if (pClientContext->operation == Optypes::opread)
    {
        if (bytesTransferred == 0)
        {
            // A graceful disconnect completed:
            sockmap[pClientContext->socket].push_back("graceful comp");
            std::string msg = "Graceful completion compid is " + std::to_string(pClientContext->compid);
            OutputDebugString(msg.c_str());
            REPORTMESSAGE("Read completed with zero bytes - starting Disconnect", pClientContext->socket);
            Disconnect(*pClientContext);
        }
        else
        {
            std::vector<BYTE> vb(pClientContext->Data(), pClientContext->Data() + bytesTransferred);
            if (!pClientContext->httpmanager.PushData(vb))
            {
                std::string s(vb.begin(), vb.end());
                REPORTMESSAGE("pushed http: " + s, pClientContext->socket);
                Read(*pClientContext);
            }
            else
            {
                // An http response is available:
                int l = pClientContext->httpmanager.GetResponseLength();
                // if the response is file send the file (ultimately using TransmitFile)
                if (pClientContext->httpmanager.IsFile())
                {
                    SendFile(*pClientContext, l, pClientContext->httpmanager.GetFilename());
                }
                else
                {
                    Write(*pClientContext, l);
                }
                pClientContext->bytesSent += l;
            }
        }
    }
    else if (pClientContext->operation == Optypes::optransmitfile)
    {
        CloseHandle(pClientContext->hfile);
        REPORTMESSAGE("TransmitFile completed:", pClientContext->socket);
        // start reading again since we will expect further requests:
        Read(*pClientContext);
    }
    else if (pClientContext->operation == Optypes::opdisconnect)
    {
        sockmap[pClientContext->socket].push_back("disconnect complete");
        // reuse the socket:
        std::ostringstream oss;
        oss << "disconnect completed on socket " << pClientContext->socket << " bytes received: " << bytesTransferred << "\n";
        OutputDebugString(oss.str().c_str());
        REPORTMESSAGE("Disconnect completed:", pClientContext->socket);
        assert(pClientContext->index >= 0 && pClientContext->index < 5000);
        Accept(*pClientContext);
    }
    else
    {
        std::cerr << "unknown operation type\n";
    }
    
}

void ServerSocket::Accept(ClientContext& context)
{
    memset(context, 0, sizeof OVERLAPPED);
    context.operation = Optypes::opaccept;
    context.compid = context.s_compid++;
    std::ostringstream oss;
    oss << "about to start accept: compid is " << std::to_string(context.compid) <<
        " socket is " << context.socket;
    SENDDLMESSAGE(oss.str());

    DWORD bytesReceived = 0;
    assert(context.index >=0 && context.index < 5000);

    //auto s = sockmap.at(context.socket).back();
    //if (s != "init" && s != "disconnect")
    //{
    //    std::cerr << "ERROR SOCKET NOT INIT OR DISCONNECT\n";
    //}

    // AMS DEBUG
    auto sz = context.GetBufSize();
    //for (auto p = 0u; p < sz; ++p)
    //{
    //    context.Data()[p] = 0x88;
    //}

    int res = AcceptEx(listenSocket_,
                       context.socket,
                       context.Data(),
                       static_cast<DWORD>(context.GetBufSize()),
                       static_cast<DWORD>(context.GetAddrSize()),
                       static_cast<DWORD>(context.GetAddrSize()),
                       &bytesReceived,
                       context);
    if (res == 0)
    {
        DWORD error = WSAGetLastError();
        if (error != ERROR_IO_PENDING)
        {
            std::string msg = ams::GetSockErrorAsString(error);
            ExtTcpTable t;
            for (auto i = 0U; i < t.GetCount(); ++i)
            {
                MIB_TCP6ROW_OWNER_PID connection = t.GetEntry(i);
                if (connection.dwOwningPid == GetCurrentProcessId())
                {
                    std::ostringstream oss;
                    oss<< "connection port: " << connection.dwLocalPort << " state:" << connection.dwState;
                    SENDDLMESSAGE(oss.str());
                }
            }
            std::ostringstream oss;
            oss << "AcceptEx failed - listen socket " << listenSocket_ << " accept socket " << context.socket << " winsock error is " << error << " " << ams::GetSockErrorAsString(error);
            SENDSOCKLMESSAGE(oss.str(), listenSocket_);
            std::string mesg = oss.str();
            OutputDebugString(mesg.c_str());
            for (auto p : sockmap[context.socket])
            {
                std::cout << "socket: " << context.socket << ": " << p << std::endl;
            }
            Sleep(200);
            throw ServerSocketException(oss.str());
        }
        else
        {
            std::ostringstream oss;
            oss << "AcceptEx succeeded - listen socket " << listenSocket_ << " accept socket " << context.socket << " winsock error is " << error << " " << ams::GetSockErrorAsString(error);
            OutputDebugString(oss.str().c_str());
        }
    }
    else
    {
        // AcceptEx completed synchronously:
        ProcessCompletion(0, context, bytesReceived, 0);
    }
    sockmap[context.socket].push_back("accept");
}


// read from one of the accept sockets:
void ServerSocket::Read(ClientContext & context)
{
    memset(context, 0, sizeof OVERLAPPED);
    context.operation = Optypes::opread;
    sockmap[context.socket].push_back("read");

    WSABUF wsabuf;
    wsabuf.buf = reinterpret_cast<char*>(context.Data());

    // AMS check the static cast:
    wsabuf.len = static_cast<ULONG>(context.GetBufSize());
    DWORD flags = 0;
    context.compid = context.s_compid++;
    std::string msg = "Calling WSARecv with compid " + std::to_string(context.compid) + "\n";
    OutputDebugString(msg.c_str());
    int result = WSARecv(context.socket, &wsabuf, 1, &context.bytesReceived, &flags, context, 0);
    if (result == 0)
    {
        // here we completed synchronously - NOTE CAREFULLY: if we called SetFileCompletionNotificationModes with parameter
        // FILE_SKIP_COMPLETION_PORT_ON_SUCCESS for this socket, then a completion packet will not be queued to the 
        // IO completion port - this means we do all processing of the synchronous packet now. Without the FILE_SKIP_COMPLETION_PORT_ON_SUCCESS
        // mode set for this socket we will ALSO QUEUE A PACKET TO THE COMPLETION PORT!
        if (context.bytesReceived == 0)
        {
            sockmap[context.socket].push_back("gracefulR");
            REPORTMESSAGE("Graceful disconnect started", context.socket);
            std::ostringstream oss;
            oss << "Graceful synchronous disconnect started on socket " << context.socket << "compid " << context.compid << "\n";
            OutputDebugString(oss.str().c_str());
            ProcessCompletion(0, context, 0, 0);
        }
        else
        {
            std::ostringstream oss;
            oss << "WSARecv sync complete bytes received: " << context.bytesReceived;
            REPORTMESSAGE(oss.str(), context.socket);
            ProcessCompletion(0, context, context.bytesReceived, 0);
        }
    }
    else 
    {
        int error = WSAGetLastError();
        if (error == WSAECONNRESET || error == WSAECONNABORTED || error == WSAECONNRESET || error == WSAEDISCON ||
            error == WSAENETDOWN || error == WSAENETRESET || error == WSAETIMEDOUT || error == WSA_OPERATION_ABORTED)
        {
            std::string errString = "Socket closed unexpectedly during WSARecv - error was " + ams::GetSockErrorAsString(error);
            OutputDebugString(errString.c_str());
            ReinitSocket(context);
            sockmap[context.socket].push_back("reinit after error");
            context.operation = Optypes::operrorClosed;
            ProcessCompletion(0, context, 0, 0);
        }
        else if (error != WSA_IO_PENDING)
        {
            std::ostringstream oss;
            std::string errString = ams::GetSockErrorAsString(error);
            oss << "error on socket " << context.socket << ": " << errString;
            OutputDebugString(oss.str().c_str());

            throw ServerSocketException("Error in WSARecv " + errString);
        }
        else
        {
            std::ostringstream oss;
            oss << "Read: WSA_IO_PENDING - bytes received: " << context.bytesReceived;
            REPORTMESSAGE(oss.str(), context.socket);
        }
    }
}

void ServerSocket::Write(ClientContext & context, int bytes)
{
    memset(context, 0, sizeof OVERLAPPED);
    context.operation = Optypes::opwrite;
    context.compid = context.s_compid++;
    sockmap[context.socket].push_back("writing");

    std::string msg = "About to start write: compid is " + std::to_string(context.compid);
    SENDDLMESSAGE(msg);

    WSABUF wsabuf;
    wsabuf.buf = reinterpret_cast<char*>(context.Data());
    wsabuf.len = bytes;
    DWORD flags = 0;
    
    BOOL result = WSASend(context.socket, &wsabuf, 1, &context.bytesReceived, flags, context, 0);
    if (result == 0)
    {
        SENDDLMESSAGE("immediate send completion");
        // immediate completion:
        //int res = SocketTime(context.socket);
        //ProcessCompletion(0, context, context.bytesReceived);
    }
    else
    {
        int error = WSAGetLastError();
        error = error;
    }
}

void ServerSocket::SendFile(ClientContext& context, int headerLength, std::string filename)
{
    memset(context, 0, sizeof OVERLAPPED);
    context.operation = Optypes::optransmitfile;
    TRANSMIT_FILE_BUFFERS tfbuf = {0, 0, 0, 0};
    tfbuf.Head = context.Data();
    tfbuf.HeadLength = headerLength;
    HANDLE hFile = CreateFile(filename.c_str(), GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, 0);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        context.hfile = hFile;
        sockmap[context.socket].push_back("transmit");
        TransmitFile(context.socket, hFile, 0, 0, context, &tfbuf, 0);
    }
}


void ServerSocket::Disconnect(ClientContext& context)
{
    //ClientContext *pNewContext = new ClientContext;
    //memset(pNewContext, 0, sizeof OVERLAPPED);
    //pNewContext->operation = Optypes::opdisonnect;
    //pNewContext->socket = context.socket;
    //pNewContext->compid = 222;
    //pNewContext->index = 4000;

    memset(context, 0, sizeof OVERLAPPED);
    context.operation = Optypes::opdisconnect;

    context.compid = context.s_compid++;
    std::string msg = "About to start disconnect: compid is " + std::to_string(context.compid);
    SENDDLMESSAGE(msg);

    int res1 = SocketTime(context.socket);
//    shutdown(context.socket, SD_BOTH);
    SENDDLMESSAGE("calling disconnectex");
    sockmap[context.socket].push_back("calling DisconnectEx");
    BOOL result = DisconnectEx(context.socket, context, TF_REUSE_SOCKET, 0);
//    int res2 = SocketTime(context.socket);

//    SENDDLMESSAGE("calling transmitfile");
//    BOOL result = TransmitFile(context.socket, NULL, 0, 0, context, 0, 0);

    if (result)
    {
        SENDDLMESSAGE("immediate disconnect completion");
        // we completed synchronously:
        ProcessCompletion(0, context, 0, 0);
    }
    else
    {
        int error = WSAGetLastError();
        if (error != ERROR_IO_PENDING)
        {
            std::ostringstream oss;
            oss << "DisconnectEx error " << ams::GetSockErrorAsString();
            SENDDLMESSAGE(oss.str());
            ExtTcpTable t;
            Sleep(5000);
            throw ServerSocketException("DisconnectEx failed");
        }
    }
}

int SocketTime(SOCKET hSocket)
{
    int seconds;
    int bytes = sizeof(seconds);
    int err = getsockopt(hSocket, SOL_SOCKET, SO_CONNECT_TIME,
        (char *)&seconds, (PINT)&bytes);
    if (err != NO_ERROR)
    {
        throw ServerSocketException("getsockopt failed");
        return 0xFFFFFFFF;
    }
    return seconds;
}