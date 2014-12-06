#include "Utility.h"
#ifdef _WIN32

#else
unsigned int GetTickCount()
{
	gettimeofday(&g_currentTime,0);
	return 1000000 * (g_currentTime.tv_sec-g_startTime.tv_sec)+ g_currentTime.tv_usec-g_startTime.tv_usec;
}
#endif

unsigned long long GenerateGUID()
{
	return ++g_GUID;
	/*
	GUID * pguid = new GUID;
	CoCreateGuid(pguid);
	return *(UINT64*)pguid->Data4;
	*/
}