#include "stdafx.h"
#include "msgthread.h"

SemQueue<QMessage, Terminator> g_messageQueue(5000);


