#pragma once
#include <ams/fileutils.h>
#include <ams/semqueue.h>

extern std::string g_workingDirectory;
extern std::map<SOCKET, std::vector<std::string>> sockmap;

// reader is either ams::FileReader or ams::FastFileReader
class FileReaderQueue
{
public:
    class EndThreadException {};
private:
    SemQueue<ams::FastLineReader, EndThreadException> queue_;

    FileReaderQueue(FileReaderQueue&) = delete;
    FileReaderQueue() : queue_(100) {}

public:
    virtual ~FileReaderQueue()
    {
        queue_.Stop();
    }
    static FileReaderQueue& GetInstance()
    {
        static FileReaderQueue instance;
        return instance;
    }

    void Stop()
    {
        queue_.Stop();
    }

    ams::FastLineReader Remove()
    {
        return queue_.Remove();
    }

    void Add(ams::FastLineReader&& reader)
    {
        queue_.Add(std::move(reader));
    }
};

#pragma pack(push, 1)
union UU64 {
    uint64_t u64;
    struct
    {
        DWORD a;
        DWORD b;
    };
    UU64() = default;
    UU64(FILETIME ft) : a(ft.dwLowDateTime), b(ft.dwHighDateTime) {}
    operator uint64_t() { return u64; }
};
#pragma pack(pop)

extern uint64_t g_requests;

inline GUID GetComputerID()
{
    // {F4C708E3-FF01-4A97-902F-1778C61B75B6}
    static const GUID computerID =
    { 0xf4c708e3, 0xff01, 0x4a97,{ 0x90, 0x2f, 0x17, 0x78, 0xc6, 0x1b, 0x75, 0xb6 } };
    return computerID;
}

inline void ReportError(std::string msg)
{
    std::cerr << msg << "\n";
}
