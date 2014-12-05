#include "SimpleMutex.h"

CSimpleMutex::CSimpleMutex() //: isInitialized(false)
{
	// Prior implementation of Initializing in Lock() was not threadsafe
	Init();
}

CSimpleMutex::~CSimpleMutex()
{
	// 	if (isInitialized==false)
	// 		return;
#ifdef _WIN32
	//	CloseHandle(hMutex);
	DeleteCriticalSection(&criticalSection);

#else
	pthread_mutex_destroy(&hMutex);
#endif


}

#ifdef _DEBUG 
#include <stdio.h>
#endif

void CSimpleMutex::Lock(void)
{
#ifdef _WIN32
	EnterCriticalSection(&criticalSection);
#else
	pthread_mutex_lock(&hMutex);
#endif

}

void CSimpleMutex::Unlock(void)
{
	// 	if (isInitialized==false)
	// 		return;
#ifdef _WIN32
	//	ReleaseMutex(hMutex);
	LeaveCriticalSection(&criticalSection);
#else
	int error = pthread_mutex_unlock(&hMutex);
#endif
}

void CSimpleMutex::Init(void)
{
#ifdef _WIN32
	//	hMutex = CreateMutex(NULL, FALSE, 0);
	//	RakAssert(hMutex);
	InitializeCriticalSection(&criticalSection);

#else
	int error = pthread_mutex_init(&hMutex, 0);
#endif

}
