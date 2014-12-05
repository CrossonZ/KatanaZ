#ifndef __INCLUDED_CONNECTION_MANAGER_H_
#define __INCLUDED_CONNECTION_MANAGER_H_

#define MAX_CLIENT_NUM 1000

#define SYS_SEND_BUFFER_SIZE 131702 //128*1024
#define SYS_RECV_BUFFER_SIZE 262404 //128*1024

#define RECV_BUF_SIZE 4096
#define SEND_BUF_SIZE 4096

#include "../tools/Singleton.h"
#include "../tools/SimpleMutex.h"
#if USE_POOL == 1
#include "../tools/ObjectPool.h"
#endif
#include "ReliabilityLayer.h"
#include "UDPSocket.h"
#include <queue>
;

#if USE_BOOST == 1
#include <boost/function.hpp>
#endif

struct SBuf32: public SObject
{
	char szBuf[32];
};

struct SBuf64: public SObject
{
	char szBuf[64];
};

struct SBuf128: public SObject
{
	char szBuf[128];
};

struct SBuf256: public SObject
{
	char szBuf[256];
};

struct SBuf512: public SObject
{
	char szBuf[512];
};

struct SBuf1K: public SObject
{
	char szBuf[1024];
};

struct SBuf2K: public SObject
{
	char szBuf[2048];
};

struct SBuf4K: public SObject
{
	char szBuf[4096];
};

struct SValidPacketMax: public SObject
{
	char szBuf[MAX_VALID_SIZE];
};

#ifdef _WIN32

unsigned __stdcall StartRecvThread(void *arguments);
unsigned __stdcall StartSendThread(void *arguments);

unsigned __stdcall StartLogicThread(void *arguments);
#else 
void *StartRecvThread(void *arguments);
void *StartSendThread(void *arguments);

void *StartLogicThread(void *arguments);
#endif 

//VOID NTAPI StartSendCallBack(PTP_CALLBACK_INSTANCE pInstance, PVOID pvContext, PTP_WORK Work);
//
//VOID NTAPI StartRecvCallBack(PTP_CALLBACK_INSTANCE pInstance, PVOID pvContext, PTP_WORK Work);

class CLogicThread;
class CReliabilityLayer;

#if USE_BOOST == 1
typedef boost::function<bool(CReliabilityLayer*)> AddRLCallback;
typedef boost::function<void(SRecvStruct *)> AddRSCallback;
#endif

class CConnectionManager /*: public CEventHandler*/
{
	DECLARE_SINGLETON_CLASS(CConnectionManager)

public:

	bool Init();
	void UnInit();

	bool Start(vector<WORD> &conPortVector);

	void ThreadLoop(); 

	CLogicThread* GetLTFromList();
	BYTE m_byLastLT;
	CLogicThread* GetLT(const UINT64 qwToken);
	void RecordRL(UINT64 qwToken, sockaddr_in sAddr, CLogicThread *pLT);

	map<UINT64, CLogicThread*>  m_conRL2LTMap;
	list<CLogicThread*> m_conLTList;
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool OnRecv(SRecvStruct *pRS);
	SRecvStruct *AllocRecvStruct();
	void DeallocRecvStruct(SRecvStruct *pRS);

	SSendStruct *AllocSendStruct();
	void DeallocSendStruct(SSendStruct *pSS);

	SSinglePacket *AllocSinglePacket();
	void DeallocSinglePacket(SSinglePacket *pSP);

    SSendThreadArg *AllocSendThreadArg();
	void DeallocSendThreadArg(SSendThreadArg *pSTA);

	SNetworkPkg *AllocNetworkPkg();
	void DeallocNetworkPkg(SNetworkPkg *pNP);

	char *AllocBuffer(const int iSize);
	void DeallocBuffer(char *pBuf, const int iSize);

	SRecvStruct *PopRS();

	bool CheckOnSend() {return m_conSSQueue.size() < SS_QUEUE_MAX - 10;}
	bool OnSend(SSendStruct *pSendStruct);

	SSendStruct *PopSS();

	void PushRecvPacket(SSinglePacket *pSP);
	SSinglePacket *GetRecvPacket();

	//CReliabilityLayer* GetReliableLayer(UINT64 qwToken);

	//CReliabilityLayer* AddReliableLayer(UINT64 qwToken, sockaddr_in sAddr, CUDPSocket *pSocket);
	void RecycleReliableLayer(CReliabilityLayer *pRL);

	bool Connect(const char *czServerIP, const WORD wPort);//For client;

	SSendStruct *CreateConnectReq(sockaddr_in sAddr, CReliabilityLayer *pRL);
	SSendStruct *CreateConnectRsp(UINT64 qwToken, sockaddr_in sAddr, CReliabilityLayer *pRL);

	void SetMyToken(const UINT64 qwToken) {m_qwMyToken = qwToken;}
	UINT64 GetMyToken() const {return m_qwMyToken;}

	int GetRSQueueSize() {m_RSQueueMutex.Lock(); return m_conRSQueue.size(); m_RSQueueMutex.Unlock();}
#if USE_POOL == 1
	ObjectPool<SSendThreadArg> m_STAPool;
	//ObjectPool<CUDPSocket> m_SocketPool;
	ObjectPool<CReliabilityLayer> m_RLPool;
	ObjectPool<SRecvStruct> m_RSPool;
	ObjectPool<SSendStruct> m_SSPool;
	//ObjectPool<SCommonPkg> m_CPPool;
	//int m_iCPUsed;
	ObjectPool<SSinglePacket> m_SPPool;
	ObjectPool<SNetworkPkg> m_NPPool;

	ObjectPool<SBuf32> m_B5Pool;
	ObjectPool<SBuf64> m_B6Pool;
	ObjectPool<SBuf128> m_B7Pool;
	ObjectPool<SBuf256> m_B8Pool;
	ObjectPool<SBuf512> m_B9Pool;
	ObjectPool<SBuf1K> m_B10Pool;
	ObjectPool<SBuf2K> m_B11Pool;
	ObjectPool<SBuf4K> m_B12Pool;
	ObjectPool<SValidPacketMax> m_BMaxPool;
#endif
	int m_iBufferUsed;
	int m_iNPUsed;
	int m_iRSUsed;
    int m_iSSUsed;
	int m_iSPUsed;
	int m_iRLUsed;
	int m_iSTAUsed;


	int g_iPopTimes;
	int g_iPushTimes;
	int g_iAllocTimes;

private:
#ifdef _WIN32
	WSADATA m_wsaData; // 套接字信息数据
#endif

	//用于select
	timeval m_tvTimer;    // 定时变量  
	fd_set  m_rfd;        // 读描述符集 
	fd_set  m_wfd;       // 写描述符集

#if USE_POOL == 1
	CSimpleMutex m_STAPoolMutex;
	//CSimpleMutex m_RLPoolMutex;
	CSimpleMutex m_RSPoolMutex;
	CSimpleMutex m_SSPoolMutex;
	CSimpleMutex m_CPPoolMutex;
	CSimpleMutex m_SPPoolMutex;
	CSimpleMutex m_NPPoolMutex;
#endif
	queue<SRecvStruct *> m_conRSQueue;
	CSimpleMutex m_RSQueueMutex;

	queue<SSendStruct *> m_conSSQueue;
	CSimpleMutex m_SSQueueMutex;

	queue<SSinglePacket *> m_conSPRecvQueue; //Recv from network, send to player layer
	CSimpleMutex m_SPRecvQueueMutex;

	map<UINT64, CReliabilityLayer*> m_conRLMap;
	CSimpleMutex m_RLMapMutex;

	list<CUDPSocket *> m_conSocketList;

	UINT64 m_qwMyToken;
};
#define gpConnectionManager (CConnectionManager::GetInstance())

#endif