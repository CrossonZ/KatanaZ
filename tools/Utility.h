#ifndef __INCLUDED_UTILITY_H_
#define __INCLUDED_UTILITY_H_

#ifdef _WIN32
#include <objbase.h>
#else
#include <sys/time.h>
static struct  timeval  g_startTime;
static struct  timeval  g_currentTime;
unsigned int GetTickCount();
#endif

static unsigned long long g_GUID = 1;
unsigned long long GenerateGUID();

#endif
