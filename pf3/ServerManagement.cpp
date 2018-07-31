#include "stdafx.h"
#include <ams/async/AIOCP.h>
#include <ams/AThread.h>
#include "msgthread.h"
#include "ServerManagement.h"
#include "ServerSocket.h"

// thread function - handles http requests:
unsigned WINAPI RequestHandler(void *p);

namespace
{
    const int maxclients = 1024;
	const int listenkey = 16383;
	const int acceptkeybase = 16384;
}


void StartServer(int listeningPort)
{
	try
	{
		AIOCP completionPort;

		// Create the client sockets:
		ServerSocket server(maxclients, completionPort);
		server.Init();
		// Associate the listening socket with the IO completion port:
		HANDLE listensocket = server.GetListenHandle();
		completionPort.Associate(listensocket, listenkey);
        SetFileCompletionNotificationModes(listensocket, FILE_SKIP_COMPLETION_PORT_ON_SUCCESS);
        // Associate the accept sockets with the IO completion port:
		std::vector<HANDLE> clientHandles;
		server.GetAcceptHandles(clientHandles);
		completionPort.Associate(acceptkeybase, clientHandles.begin(), clientHandles.end());
        for (auto handle : clientHandles)
        {   
            // Prevent immediate completion from triggering a completion packet. In other words if a function like
            // WSARecv return zero to indicate success, then we process the data immediately and a completion packet
            // is not sent to the completion port by the system. 
            SetFileCompletionNotificationModes(handle, FILE_SKIP_COMPLETION_PORT_ON_SUCCESS);
        }

		// we need the number of CPU cores:
		SYSTEM_INFO sysinfo;
		GetSystemInfo(&sysinfo);

		std::vector<AThread> threads;
		// reserve to avoid reallocations:
        // AMS DEBUG
        		threads.reserve(1);
        //		threads.reserve(sysinfo.dwNumberOfProcessors * 4);

		// we will create twice the number of threads as processors, though only half of the threads will be used
		// most of the time. The system decides which threads to run when an Async IO operation completes:
        for (DWORD i = 0; i < sysinfo.dwNumberOfProcessors * 2; ++i)
//        for (DWORD i = 0; i < 1; ++i)
		{
			threads.emplace_back(RequestHandler, 0, completionPort, false);
		}

		// Start listening for connections:

		server.Start("localhost", listeningPort);

		// wait for threads to complete:
        // AMS - this is crap! Need to do WaitForMultipleObjects!
		for (auto& t : threads)
		{
			WaitForSingleObject(t, INFINITE);
		}

	}
	catch (ServerSocketException& sse)
	{
		std::cerr << "Fatal Socket Error " << sse.what() << std::endl;
	}
	catch (...)
	{
		std::cerr << "exception occurred\n";
	}
}

// Thread function - there are many instances of this thread:
unsigned WINAPI RequestHandler(void *p)
{
	bool finished = false;
	HANDLE hPort = reinterpret_cast<HANDLE>(p);
	DWORD bytesReceived;
	ULONG_PTR key;
	LPOVERLAPPED pol;
	while (!finished)
	{
        // wait for an overlapped IO operation to complete - this may be AcceptEx (which returns initial incoming data), 
        // WSARecv, WSASend, DisconnectEx or TransmitFile
		BOOL result = GetQueuedCompletionStatus(hPort, &bytesReceived, &key, &pol, INFINITE);

        DWORD error = 0;
        ClientContext *pClientContext = reinterpret_cast<ClientContext*>(pol);
        ServerSocket& serverSocket = *pClientContext->pServerSocket;
        std::ostringstream oss;
		oss << "GetQueuedCompletionStatus returned - LPOVERLAPPED: " << std::hex << std::setfill('0') << std::setw(8) << pol;
		SENDDLMESSAGE(oss.str());
		if (!result)
		{
			std::ostringstream oss;
            // now get a more accurate error code - the extra call to WSAGetOverlappedResult causes GetLastError to return
            // a more representative value - this is undocumented and counter-intuitive, but this is what the .Net Framework does:
            // http://referencesource.microsoft.com/#System/net/System/Net/Sockets/Socket.cs,22f73d005323d27a
            DWORD transferred;
            DWORD flags;
            BOOL success = WSAGetOverlappedResult(pClientContext->socket, pol, &transferred, FALSE, &flags);
            error = WSAGetLastError();
            oss << "GetQueuedCompletionStatus failed - error was " << ams::GetWinErrorAsString(error) << "\n"
                << "key is " << key << "\nbytes received is " << bytesReceived << "\nlpOverlapped is " << pol << "\n"
                << "lpOverlapped->Internal is " << std::hex << pol->Internal << "\n"
                << std::dec << "lpOverlapped->InternalHigh is " << pol->InternalHigh << "\n";
            OutputDebugString(oss.str().c_str());

            if (error == WSAECONNRESET || error == WSAECONNABORTED || error == WSAECONNRESET || error == WSAEDISCON ||
                error == WSAENETDOWN || error == WSAENETRESET || error == WSAETIMEDOUT || error == WSA_OPERATION_ABORTED)
            {
                std::string errString = "Socket closed unexpectedly - error was " + ams::GetSockErrorAsString(error);
                OutputDebugString(errString.c_str());
                serverSocket.ReinitSocket(*pClientContext);
            }
            else
            {
                std::string errString = "very strange error - error was " + ams::GetSockErrorAsString(error);
                OutputDebugString(errString.c_str());
            }
            pClientContext->operation = Optypes::operrorClosed;
		}
        else
        {
            // REPORTMESSAGE("completion of type: ", 0);
            // AMS DEBUG
            assert(pClientContext->index < 5000);
            if (pClientContext->operation == Optypes::opaccept)
            {
                std::ostringstream oss;
                if (bytesReceived == 0)
                {
                    oss << "ZERO ";
                }
                oss << "AcceptEx complete on socket " << pClientContext->socket << "... ";
                OutputDebugString(oss.str().c_str());
            }
            else if (pClientContext->operation == Optypes::opdisconnect)
            {
                std::ostringstream oss;
                oss << "disconnect complete on socket " << pClientContext->socket << "...\n";
                if (bytesReceived != 0)
                {
                    bytesReceived = bytesReceived;
                }
                //            REPORTMESSAGE("disconnect", 0);
            }
            else if (pClientContext->operation == Optypes::opread)
            {
                REPORTMESSAGE("read complete", pClientContext->socket);
            }
            else if (pClientContext->operation == Optypes::opwrite)
            {
                //            REPORTMESSAGE("write", 0);
            }
            //        REPORTMESSAGE("Bytes received: " + std::to_string(bytesReceived), 0);
            else if (pClientContext->operation == Optypes::optransmitfile)
            {
                OutputDebugString("transmit complete\n");
            }
        }
        serverSocket.ProcessCompletion(key, pol, bytesReceived, error);
    }
	return 0;
}
