#ifndef __INCLUDED_UTILITY_H_
#define __INCLUDED_UTILITY_H_
#ifdef _WIN32
#include <objbase.h>
#endif
UINT64 g_GUID = 1;
UINT64 GenerateGUID()
{
	return ++g_GUID;
	/*
	GUID * pguid = new GUID;
	CoCreateGuid(pguid);
	return *(UINT64*)pguid->Data4;
	*/
}

#endif