#include "UDPThread.h"

#if defined(_WIN32)
int CUDPThread::Create( unsigned __stdcall start_address( void* ), void *arglist, int priority)
#else
int CUDPThread::Create( void* start_address( void* ), void *arglist, int priority)
#endif
{
#if defined(_WIN32)
	HANDLE threadHandle;
	unsigned threadID = 0;

	threadHandle = (HANDLE) _beginthreadex( NULL, MAX_ALLOCA_STACK_ALLOCATION*2, start_address, arglist, 0, &threadID );

	SetThreadPriority(threadHandle, priority);

	if (threadHandle==0)
	{
		return 1;
	}
	else
	{
		CloseHandle( threadHandle );
		return 0;
	}
#else
	pthread_t threadHandle;
	// Create thread linux
	pthread_attr_t attr;
	sched_param param;
	param.sched_priority=priority;
	pthread_attr_init( &attr );
	pthread_attr_setschedparam(&attr, &param);
	pthread_attr_setstacksize(&attr, MAX_ALLOCA_STACK_ALLOCATION*2);

	pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED );
	int res = pthread_create( &threadHandle, &attr, start_address, arglist );
	return res;
#endif
}