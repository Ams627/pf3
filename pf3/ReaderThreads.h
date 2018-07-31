#pragma once
#include "globals.h"
#include <ams/AThread.h>

class ReaderThreads
{
    std::deque<AThread> threads_;
public:
    ReaderThreads();
    static unsigned WINAPI ReaderThreads::ReaderThread(void *p);
    virtual ~ReaderThreads();
};
