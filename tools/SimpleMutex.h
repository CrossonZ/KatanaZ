#ifndef __INCLUDED_SIMPLE_MUTEX_H_
#define __INCLUDED_SIMPLE_MUTEX_H_
#include "../main/GlobalDefines.h"
class CSimpleMutex
{
public:
	// Constructor
	CSimpleMutex();

	// Destructor
	~CSimpleMutex();

	// Locks the mutex.  Slow!
	void Lock(void);

	// Unlocks the mutex.
	void Unlock(void);

private:
	void Init(void);
	CRITICAL_SECTION criticalSection; /// Docs say this is faster than a mutex for single process access
};

#endif