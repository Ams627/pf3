#include "stdafx.h"
#include "ReaderThreads.h"
#include "PrintProgress.h"

ReaderThreads::ReaderThreads()
{
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);

    // start as many threads as there are CPUs (i.e. cores)
    for (auto i = 0u; i < sysinfo.dwNumberOfProcessors; ++i)
    {
       threads_.push_back(AThread(ReaderThread, 0, 0, false));
    }

}

ReaderThreads::~ReaderThreads()
{
    FileReaderQueue& queue = FileReaderQueue::GetInstance();
    queue.Stop();
    for (auto& p : threads_)
    {
//        std::cout << "Sent Stop Request\n";
        WaitForSingleObject(p, INFINITE);
    }
}

/* static - the thread function: */ 
unsigned WINAPI ReaderThreads::ReaderThread(void *p)
{
    FileReaderQueue& queue = FileReaderQueue::GetInstance();
    bool stopped = false;
    PrintProgress& pp = PrintProgress::GetInstance();
    try
    {
        while (!stopped)
        {
            ams::FastLineReader reader = queue.Remove();
            reader.Read();

            // set an event so that the monitoring thread knows that we have finished - this event is passed in the reader on the queue:
            HANDLE h = reader.GetEvent();
            if (h != nullptr)
            {
                // std::cout << "TID " << GetCurrentThreadId() << " Setting event for handle " << h << "\n";
                SetEvent(h);
            }
        }
    }
    catch (FileReaderQueue::EndThreadException e)
    {
        // this is the way to leave the thread
        e;
    }
    catch (std::exception ex)
    {
        std::cerr << ex.what() << "\n";
    }
    return 0;
}