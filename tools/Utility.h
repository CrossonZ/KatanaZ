#ifndef __INCLUDED_UTILITY_H_
#define __INCLUDED_UTILITY_H_
#include <objbase.h>

UINT64 GenerateGUID()
{
	GUID * pguid = new GUID;
	CoCreateGuid(pguid);
	return *(UINT64*)pguid->Data4;
}

#endif