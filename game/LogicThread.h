#ifndef __INCLUDED_LOGIC_THREAD_H_
#define __INCLUDED_LOGIC_THREAD_H_
#include "../tools/SimpleMutex.h"
#include "../net/NetStructDefs.h"
#include <queue>
class CLogicThread
{
public:
	CLogicThread() {};
	~CLogicThread() {};

	bool AddReliabilityLayer(CReliabilityLayer *pRL);
	bool DeleteReliabilityLayer(CReliabilityLayer *pRL);

	CReliabilityLayer *GetRL(const UINT64 qwToken);

	void PushRS(SRecvStruct *pRS);
	SRecvStruct *PopRS();

	void PushSS(SSendStruct *pSS);


	int GetWorkbanch();

	void ThreadLoop();

	void LoopPopRS();
	//void LoopPopSS();
	void LoopTraverseConnections();

private:
	map<UINT64, CReliabilityLayer*> m_conReliabilityLayerMap;
	CSimpleMutex m_ReliabilityLayerMutex;

	queue<SRecvStruct*> m_conRSQueue;
	CSimpleMutex m_RSQueueMutex;

	//queue<SSendStruct*> m_conSSQueue;
	//CSimpleMutex m_SSQueueMutex;

	//queue<SSinglePacket *> m_conSPQueue;

};

#endif