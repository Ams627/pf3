#pragma once
class PrintProgress
{
    static const int interval = 1'000'000;
    static long globalLineNumber_;
    CRITICAL_SECTION cs_;
    PrintProgress() {
        InitializeCriticalSection(&cs_);
    }
public:
    PrintProgress(PrintProgress&) = delete;

    static PrintProgress& GetInstance()
    {
        static PrintProgress instance;
        return instance;
    }

    void Print()
    {
        int i = InterlockedIncrement(&globalLineNumber_);
        if (i % interval == interval - 1)
        {
            DWORD procnum = GetCurrentProcessorNumber();
            DWORD threadid = GetCurrentThreadId();
            std::ostringstream oss;
            oss << std::dec << std::setfill(' ') << std::setw(10) << i + 1 << " lines\r";
            std::string s = oss.str();
            EnterCriticalSection(&cs_);
            std::cout << s;
            LeaveCriticalSection(&cs_);
        }
    }
    long GetLinenumber()
    {
        return globalLineNumber_;
    }
    virtual ~PrintProgress(){}
};
