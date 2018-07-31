#pragma once
#include <ams/AThread.h>
#include <ams/semqueue.h>

struct Terminator {};

struct QMessage
{
    DWORD threadid;
    DWORD tickcount;
    SIZE_T sockethandle;
    DWORD processornumber;
    std::string msg;
    bool newline;
    QMessage(){}
    QMessage(DWORD threadid, std::string msg, bool newline, SIZE_T sockethandle) :
        threadid(threadid),
        msg(msg),
        newline(newline),
        sockethandle(sockethandle),
        tickcount(GetTickCount()),
        processornumber(0)
    
    {}
};


//#define SENDDLMESSAGE(s) g_messageQueue.Add(QMessage(GetCurrentThreadId(), s, true, 0))
//#define SENDDMESSAGE(s) g_messageQueue.Add(QMessage(GetCurrentThreadId(), s, false, 0))
//
//#define SENDSOCKLMESSAGE(s, sock) g_messageQueue.Add(QMessage(GetCurrentThreadId(), s, true, sock))
//#define SENDSOCKMESSAGE(s, sock) g_messageQueue.Add(QMessage(GetCurrentThreadId(), s, false, sock))

#define SENDSOCKLMESSAGE(s, sock)
#define REPORTMESSAGE(s, socket)
//#define REPORTMESSAGE(s, socket) g_messageQueue.Add(QMessage(GetCurrentThreadId(), s, true, socket))

#define SENDDLMESSAGE(s)
#define SENDDMESSAGE(s) 

extern SemQueue<QMessage, Terminator> g_messageQueue;

class MsgThread
{
    AThread thread_;
    static unsigned WINAPI MessageThread(void *p)
    {
        std::ofstream logfile("c:\\temp\\1.txt", std::ios::app);
        DWORD startTickCount = GetTickCount();
        REPORTMESSAGE("Message thread started", 0);
        bool exit = false;
        while (!exit)
        {
            try
            {
                QMessage message = g_messageQueue.Remove();
                DWORD mseconds = (message.tickcount - startTickCount);
                logfile << std::dec << std::setfill(' ') << std::setw(6) << mseconds << " ";
                logfile << message.sockethandle << " ";
                logfile << std::hex << std::setfill('0') << std::uppercase << std::setw(8) << message.threadid << " " << message.msg;
                if (message.newline)
                {
                    logfile << "\n";
                }
                logfile.flush();
            }
            catch (Terminator&)
            {
                std::cout << "Message thread terminated\n";
                exit = true;
            }
        }
        return 0;
    }
    // private constructor:
    MsgThread() : thread_(MessageThread, 0, 0, false)
    {
    }
    virtual ~MsgThread()
    {
        std::cout << "logging dying\n";
        g_messageQueue.Stop();
        WaitForSingleObject(thread_, INFINITE);
    }
public:
    static MsgThread& GetInstance()
    {
        // Meyers singleton:
        static MsgThread instance;
        return instance;
    }

	void Resume()
    {
        thread_.Resume();
    }
};

