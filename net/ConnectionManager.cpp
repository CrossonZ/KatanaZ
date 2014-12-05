#include "ConnectionManager.h"
#include "../tools/UDPThread.h"
#include "../tools/Utility.h"
#include "../game/LogicThread.h"
#include <time.h>

#define BUFSIZE 1024
;

#ifdef DEBUG_NEW
//#include "../tools/DebugNew.h"
//#define new new(__FILE__, __LINE__)
#endif

#ifdef _WIN32
unsigned __stdcall StartRecvThread(void *arguments)
{
	CUDPSocket *pSock = (CUDPSocket *)arguments;
	pSock->RecvFromSocket();
	return 1;
}
//
unsigned __stdcall StartSendThread(void *arguments)
{
	CUDPSocket *pSock = (CUDPSocket *)arguments;//pArg->pSocket;
	pSock->SendToSocket();
	return 1;
}

unsigned __stdcall StartLogicThread(void *arguments)
{
	CLogicThread *pLT = (CLogicThread *)arguments;
	pLT->ThreadLoop();
	return 1;
}

#else
void *StartRecvThread(void *arguments)
{
	CUDPSocket *pSock = (CUDPSocket *)arguments;
	pSock->RecvFromSocket();
	return 0;
}
//
void *StartSendThread(void *arguments)
{
	CUDPSocket *pSock = (CUDPSocket *)arguments;//pArg->pSocket;
	pSock->SendToSocket();
	return 0;
}

void *StartLogicThread(void *arguments)
{
	CLogicThread *pLT = (CLogicThread *)arguments;
	pLT->ThreadLoop();
	return 0;
}
#endif

/*VOID NTAPI StartSendCallBack(PTP_CALLBACK_INSTANCE pInstance, PVOID pvContext, PTP_WORK Work)
{
	//SSendThreadArg * pArg = (SSendThreadArg *)pvContext;
	//CUDPSocket *pSock = pArg->pSocket;
	//pSock->SendToSocket(pArg->pPacket);
	//if (pArg->pPacket->eSPF == SPF_NO_ACK)
	//{
	//	gpConnectionManager->DeallocSendStruct(pArg->pPacket);
	//}
	//gpConnectionManager->DeallocSendThreadArg(pArg);
}

VOID NTAPI StartRecvCallBack(PTP_CALLBACK_INSTANCE pInstance, PVOID pvContext, PTP_WORK Work)
{
	CUDPSocket *pSock = (CUDPSocket *)pvContext;
	pSock->RecvFromSocket();
}*/

IMPLEMENT_SINGLETON_INSTANCE(CConnectionManager)

bool CConnectionManager::Init()
{	
	m_iSTAUsed =0;

	m_iRLUsed = 0;

	m_iRSUsed = 0;

	m_iSSUsed = 0;

	//m_iCPUsed = 0;

	m_iSPUsed = 0;

	m_iNPUsed = 0;

	m_iBufferUsed = 0;

	g_iPopTimes = 0;
	g_iPushTimes = 0;
	g_iAllocTimes = 0;
	m_byLastLT = 0;
	return true;
}

void CConnectionManager::UnInit()
{
	list<CUDPSocket *>::iterator itList = m_conSocketList.begin();
	for (;itList!=m_conSocketList.end();++itList)
	{
		delete *itList;
		*itList = NULL;
	}
	m_conSocketList.clear();
#ifdef _WIND32
	WSACleanup();
#endif
}

bool CConnectionManager::Start(vector<WORD> &conPortVector)
{
#ifdef _WIN32
	if (WSAStartup(MAKEWORD(2, 2), &m_wsaData) != 0)  
	{  
		perror("failed to load winsock!\n");  
		return false;  
	}  
#endif
	m_tvTimer.tv_sec = 6;   
	m_tvTimer.tv_usec = 0; 

	CUDPSocket *pSocket = NULL;
	for (unsigned int i=0;i<conPortVector.size();++i)
	{
		pSocket = new CUDPSocket;
		if (pSocket == NULL)
		{
			//perror("Create socket error!\n");
			return false;
		}
		int iRet = -1;
#if TYPE_SERVER == 1
		iRet = pSocket->Bind(this, conPortVector[i], true, false);
#else
		iRet = pSocket->Bind(this, conPortVector[i], false, false);
#endif
		if (iRet < 0)
		{
			//perror("Bind error!\n");
			return false;
		}
		m_conSocketList.push_back(pSocket);

		CUDPThread::Create(&StartRecvThread, pSocket, 0);
		CUDPThread::Create(&StartSendThread, pSocket, 0);


	}

	CLogicThread *pLT = NULL;
	for (unsigned int i=0;i<4;++i)
	{
		pLT = new CLogicThread;

		CUDPThread::Create(&StartLogicThread, pLT, 0);

		/*m_funcAddRLCallback = boost::bind(&CLogicThread::AddReliabilityLayer, pLT, _1);
		m_funcAddRSCallback = boost::bind(&CLogicThread::PushRS, pLT, _1);*/
		m_conLTList.push_back(pLT);
	}

	ThreadLoop();

	return false;
}

void CConnectionManager::ThreadLoop()
{
	SRecvStruct *pRS = NULL;
	SNetworkPkgHeader *pNPH = NULL;
	CReliabilityLayer *pRL = NULL;
	CLogicThread *pLT = NULL;
	int iOffset = 0;
	UINT64 qwToken = 0;
	while (true)  
	{
		pRS = PopRS();
		if (pRS != NULL)
		{
			iOffset = 0;
			pNPH = pRS->GetNetworkPkg();
			if (pNPH == NULL)
			{
				//printf("Fucking error occurs, Fatal NPH\n");
				DeallocRecvStruct(pRS);
				continue;
			}
			switch (pNPH->byPkgType)
			{
			case NPT_CONNECTION_REQ:
				{
					qwToken = GenerateGUID();
					//pRL = AddReliableLayer(qwToken, pRS->sAddr, pRS->pSock);
					CLogicThread *pLT = GetLTFromList();
					RecordRL(qwToken, pRS->sAddr, pLT);

					SSendStruct *pSS = gpConnectionManager->CreateConnectRsp(qwToken, pRS->sAddr, NULL/*pRL*/);
					if (pSS != NULL)
					{
						gpConnectionManager->OnSend(pSS);
					}
					DeallocRecvStruct(pRS);
				}break;
			case NPT_CONNECTION_RSP:
				{
					//gpConnectionManager->AddReliableLayer(pNPH->qwToken, pRS->sAddr, pRS->pSock);
					CLogicThread *pLT = GetLTFromList();
					RecordRL(pNPH->qwToken, pRS->sAddr, pLT);
					gpConnectionManager->SetMyToken(pNPH->qwToken);
					DeallocRecvStruct(pRS);
				}break;
			case NPT_ACK:
			case NPT_TRAFFIC_CONTROL:
			case NPT_COMMON_MSG:
			case NPT_FIN:
				{
					pLT = GetLT(pNPH->qwToken);
					if (pLT != NULL)
					{
						//m_funcAddRSCallback(pRS);
						pLT->PushRS(pRS);
					}	
				}break;
			default:
				DeallocRecvStruct(pRS);
				break;
			}
		}
		else
		{
			Sleep(50);
		}
	}
}

CLogicThread* CConnectionManager::GetLTFromList()
{
	CLogicThread *pLT = NULL;
	list<CLogicThread*>::iterator itList = m_conLTList.begin();
	int i=0;
	while (itList != m_conLTList.end())
	{
		if (i < m_byLastLT)
		{
			++itList;
			++i;
		}
		else
		{
			pLT = *itList;
			m_byLastLT = m_byLastLT+1 < m_conLTList.size() ? m_byLastLT+1 : 0;
			break;
		}
	}
	return pLT;
}

CLogicThread* CConnectionManager::GetLT(const UINT64 qwToken)
{
	CLogicThread *pLT = NULL;
	map<UINT64, CLogicThread*>::iterator itMap = m_conRL2LTMap.find(qwToken);

	if (itMap != m_conRL2LTMap.end())
	{
		pLT = itMap->second;
	}
	return pLT;
}

void CConnectionManager::RecordRL(UINT64 qwToken, sockaddr_in sAddr, CLogicThread *pLT)
{
	CReliabilityLayer* pRL = NULL;
#if USE_POOL == 1
	pRL = m_RLPool.New();
	m_iRLUsed++;
#else
	++m_iRLUsed;
	pRL = new CReliabilityLayer;
#endif
	if (pRL != NULL)
	{
		pRL->m_eCS = CS_INITIAL;
		pRL->Reset(sAddr, qwToken, NULL);

		m_conRL2LTMap.insert(make_pair(qwToken, pLT));
		////////////////////
		//m_funcAddRLCallback(pRL);
		pLT->AddReliabilityLayer(pRL);
	}
}


bool CConnectionManager::OnRecv(SRecvStruct *pRecvStruct)
{
	bool bRet = false;
	m_RSQueueMutex.Lock();
	if (m_conRSQueue.size() < RS_QUEUE_MAX)
	{
		bRet = true;
	}
	m_conRSQueue.push(pRecvStruct);
	++g_iPushTimes;
	m_RSQueueMutex.Unlock();
	return bRet;
}

void CConnectionManager::DeallocRecvStruct(SRecvStruct *pRS)
{
	pRS->DecRef();


	if (pRS->iRefCount <= 0)
	{
#if USE_POOL == 1
		m_RSPoolMutex.Lock();
		m_RSPool.DeleteWithoutDestroying(pRS);
		//pRS = NULL;
		m_iRSUsed --;
		m_RSPoolMutex.Unlock();
#else
		--m_iRSUsed;
		delete pRS;
	}


#endif

}

SRecvStruct *CConnectionManager::AllocRecvStruct()
{
#if USE_POOL == 1
	SRecvStruct * pRS = NULL;
	m_RSPoolMutex.Lock();
	pRS = m_RSPool.New();
	pRS->m_iOffset = 0;
	m_iRSUsed ++;
	m_RSPoolMutex.Unlock();
	pRS->iRefCount = 1;
	return pRS;
#else
	++m_iRSUsed;
	++g_iAllocTimes;
	return new SRecvStruct;
#endif
}

SSendThreadArg *CConnectionManager::AllocSendThreadArg()
{
#if USE_POOL == 1
	SSendThreadArg *pSTA = NULL;
	m_STAPoolMutex.Lock();
	pSTA = m_STAPool.New();
	m_iSTAUsed ++;
	m_STAPoolMutex.Unlock();
	return pSTA;
#else
	++m_iSTAUsed;
	return new SSendThreadArg;
#endif
}

void CConnectionManager::DeallocSendThreadArg(SSendThreadArg *pSTA)
{
#if USE_POOL == 1
	m_STAPoolMutex.Lock();
	m_STAPool.DeleteWithoutDestroying(pSTA);
	pSTA = NULL;
	m_iSTAUsed --;
	m_STAPoolMutex.Unlock();
#else
	--m_iSTAUsed;
	delete pSTA;
#endif
}

SSendStruct *CConnectionManager::AllocSendStruct()
{
#if USE_POOL == 1
	SSendStruct *pSS = NULL;
	m_SSPoolMutex.Lock();
	pSS = m_SSPool.New();
	m_iSSUsed++;
	m_SSPoolMutex.Unlock();
	pSS->eSPF = SPF_NEED_ACK;
	return pSS;
#else
	++m_iSSUsed;
	return new SSendStruct;//
#endif
}

void CConnectionManager::DeallocSendStruct(SSendStruct *pSS)
{
	DeallocBuffer(pSS->pBuf, pSS->iLen);
#if USE_POOL == 1
	m_SSPoolMutex.Lock();
	m_SSPool.DeleteWithoutDestroying(pSS);
	m_iSSUsed--;
	m_SSPoolMutex.Unlock();
#else
	--m_iSSUsed;
	delete pSS;
#endif
}

SSinglePacket *CConnectionManager::AllocSinglePacket()
{
#if USE_POOL == 1
	SSinglePacket *pSP = NULL;
    m_SPPoolMutex.Lock();
	pSP = new SSinglePacket;//m_SPPool.New();
	m_iSPUsed++;
	m_SPPoolMutex.Unlock();
	return pSP;
#else
	++m_iSPUsed;
	return new SSinglePacket;
#endif
}

void CConnectionManager::DeallocSinglePacket(SSinglePacket *pSP)
{
	DeallocBuffer(pSP->pBuf, pSP->iLen);
#if USE_POOL == 1
	m_SPPoolMutex.Lock();
	m_SPPool.DeleteWithoutDestroying(pSP);
	pSP = NULL;
	m_iSPUsed --;
	m_SPPoolMutex.Unlock();
#else
	--m_iSPUsed;
	delete pSP;
#endif
}

SNetworkPkg *CConnectionManager::AllocNetworkPkg()
{
#if USE_POOL == 1
	SNetworkPkg *pNP = NULL;
	m_NPPoolMutex.Lock();
	pNP = m_NPPool.New();
	++m_iNPUsed;
	m_NPPoolMutex.Unlock();
	return pNP;
#else
	++m_iNPUsed;
	return new SNetworkPkg;
#endif
}

void CConnectionManager::DeallocNetworkPkg(SNetworkPkg *pNP)
{
	DeallocBuffer(pNP->pBuf, pNP->sNPH.iTotalLen);
#if USE_POOL == 1
	m_NPPoolMutex.Lock();
	m_NPPool.DeleteWithoutDestroying(pNP);
	pNP = NULL;
	--m_iNPUsed;
	m_NPPoolMutex.Unlock();
#else
	--m_iNPUsed;
	delete pNP;
#endif
}

char *CConnectionManager::AllocBuffer(const int iSize)
{
#if USE_POOL == 1
	char *pRet = NULL;
	if (iSize < 32)
	{
		pRet = (char *)m_B5Pool.New();
	}
	else if (iSize < 64)
	{
		pRet = (char *)m_B6Pool.New();
	}
	else if (iSize < 128)
	{
		pRet = (char *)m_B7Pool.New();
	}
	else if (iSize < 256)
	{
		pRet = (char *)m_B8Pool.New();
	}
	else if (iSize < 512)
	{
		pRet = (char *)m_B9Pool.New();
	}
	else if (iSize < 1024)
	{
		pRet = (char *)m_B10Pool.New();
	}
	else if (iSize < 2048)
	{
		pRet = (char *)m_B11Pool.New();
	}
	else if (iSize < 4096)
	{
		pRet = (char *)m_B12Pool.New();
	}
	else if (iSize < MAX_VALID_SIZE)
	{
		pRet = (char *)m_BMaxPool.New();
	}
	return pRet;
#else
	++m_iBufferUsed;
	return new char[iSize+1];
#endif
}

void CConnectionManager::DeallocBuffer(char *pBuf, const int iSize)
{
#if USE_POOL == 1
	if (iSize < 32)
	{
		m_B5Pool.DeleteWithoutDestroying((SBuf32*)pBuf);
	}
	else if (iSize < 64)
	{
		m_B6Pool.DeleteWithoutDestroying((SBuf64*)pBuf);
	}
	else if (iSize < 128)
	{
		m_B7Pool.DeleteWithoutDestroying((SBuf128*)pBuf);
	}
	else if (iSize < 256)
	{
		m_B8Pool.DeleteWithoutDestroying((SBuf256*)pBuf);
	}
	else if (iSize < 512)
	{
		m_B9Pool.DeleteWithoutDestroying((SBuf512*)pBuf);
	}
	else if (iSize < 1024)
	{
		m_B10Pool.DeleteWithoutDestroying((SBuf1K*)pBuf);
	}
	else if (iSize < 2048)
	{
		m_B11Pool.DeleteWithoutDestroying((SBuf2K*)pBuf);
	}
	else if (iSize < 4096)
	{
		m_B12Pool.DeleteWithoutDestroying((SBuf4K*)pBuf);
	}
	else if (iSize < MAX_VALID_SIZE)
	{
		m_BMaxPool.DeleteWithoutDestroying((SValidPacketMax*)pBuf);
	}
#else
	--m_iBufferUsed;
	delete []pBuf;
#endif
}

/*int CConnectionManager::TraverseConnections()
{
	map<UINT64, CReliabilityLayer*>::iterator itTmp ,itMap = m_conRLMap.begin();
	int iStatus = NS_IDLE, iRet = NS_IDLE;
	time_t tNow = time(NULL);
	while (itMap != m_conRLMap.end())
	{
		itTmp = itMap ++;
		switch (itTmp->second->m_eCS)
		{
		case CS_INITIAL:
		case CS_ESTABLISHED:
			{
				if (itTmp->second->m_tLastCommunicationTime + 30 < tNow)
				{
					itTmp->second->m_eCS = CS_TIMEOUT;
				}
				else
				{
					if (itTmp->second->m_iSelfTrafficCtrl < itTmp->second->m_iLastACKSeq)
					{
						iRet = itTmp->second->HandleSendPkg();
						iStatus = min(iStatus, iRet);
					}

					iRet = itTmp->second->CheckResendPkg();
					iStatus = min(iStatus, iRet);
				}
			}break;
		case CS_TIMEOUT:
			{
				if (itTmp->second->m_tLastCommunicationTime + 40 < tNow)
				{
					itTmp->second->m_eCS = CS_RECYCLED;
					gpConnectionManager->RecycleReliableLayer(itTmp->second);
				}
			}break;
		default:
			break;
		}
	}
	return iStatus;
}*/

SRecvStruct *CConnectionManager::PopRS()
{
	SRecvStruct *pRS = NULL;
	m_RSQueueMutex.Lock();
	if (!m_conRSQueue.empty())
	{
		pRS = m_conRSQueue.front();
		m_conRSQueue.pop();
		++g_iPopTimes;
	}
	m_RSQueueMutex.Unlock();
	return pRS;
}

bool CConnectionManager::OnSend(SSendStruct *pSendStruct)
{
	bool bRet = false;
	m_SSQueueMutex.Lock();
	m_conSSQueue.push(pSendStruct);
	if (m_conSSQueue.size() < SS_QUEUE_MAX)
	{
		bRet = true;
	}
	m_SSQueueMutex.Unlock();
	return bRet;
}

SSendStruct *CConnectionManager::PopSS()
{
	SSendStruct *pSS = NULL;
	m_SSQueueMutex.Lock();
	if (!m_conSSQueue.empty())
	{
		pSS = m_conSSQueue.front();
		m_conSSQueue.pop();
	}
	m_SSQueueMutex.Unlock();
	return pSS;
}

void CConnectionManager::PushRecvPacket(SSinglePacket *pSP)
{
	m_SPRecvQueueMutex.Lock();
	m_conSPRecvQueue.push(pSP);
	m_SPRecvQueueMutex.Unlock();
}

SSinglePacket * CConnectionManager::GetRecvPacket()
{
	SSinglePacket *pSP = NULL;
	m_SPRecvQueueMutex.Lock();
	if (!m_conSPRecvQueue.empty())
	{
		pSP = m_conSPRecvQueue.front();
		m_conSPRecvQueue.pop();
	}
	m_SPRecvQueueMutex.Unlock();
	return pSP;
}

//CReliabilityLayer* CConnectionManager::GetReliableLayer(UINT64 qwToken)
//{
//	CReliabilityLayer *pRL = NULL;
//	m_RLMapMutex.Lock();
//	map<UINT64, CReliabilityLayer*>::iterator itMap = m_conRLMap.find(qwToken);
//	if (itMap != m_conRLMap.end())
//	{
//		pRL = itMap->second;
//	}
//	m_RLMapMutex.Unlock();
//	return pRL;
//}

/*CReliabilityLayer* CConnectionManager::AddReliableLayer(UINT64 qwToken, sockaddr_in sAddr, CUDPSocket *pSocket)
{
	CReliabilityLayer* pRL = NULL;
#if USE_POOL == 1
	pRL = m_RLPool.New();
	m_iRLUsed++;
#else
	++m_iRLUsed;
	pRL = new CReliabilityLayer;
#endif
	if (pRL != NULL)
	{
	pRL->m_eCS = CS_INITIAL;
	pRL->Reset(sAddr, qwToken, pSocket);
	m_RLMapMutex.Lock();
	pRL = m_conRLMap.insert(make_pair(qwToken, pRL)).first->second;
	m_RLMapMutex.Unlock();
	}

	return pRL;
}*/

void CConnectionManager::RecycleReliableLayer(CReliabilityLayer *pRL)
{

	pRL->DeallocRL();

	//m_RLMapMutex.Lock();
	//m_conRLMap.erase(pRL->GetToken());
	//m_RLMapMutex.Unlock();
	m_RLMapMutex.Lock();
	map<UINT64, CLogicThread*>::iterator itMap = m_conRL2LTMap.find(pRL->GetToken());
	if (itMap!=m_conRL2LTMap.end())
	{
		itMap->second->DeleteReliabilityLayer(pRL);
		m_conRL2LTMap.erase(itMap);
	}
	m_RLMapMutex.Unlock();
#if USE_POOL == 1
	m_RLPool.DeleteWithoutDestroying(pRL);
	m_iRLUsed --;
#else
	--m_iRLUsed;
	delete pRL;
	//printf ("DELTE RL\n");
	//printf("m_iRLUsed=%d,m_iBufferUsed=%d,m_iNPUsed=%d\n",m_iRLUsed, m_iBufferUsed,m_iNPUsed);
	//printf("m_iRSUsed=%d,m_iSSUsed=%d,m_iSPUsed=%d,m_iSTAUsed=%d\n",m_iRSUsed,m_iSSUsed,m_iSPUsed,m_iSTAUsed);
	if (m_iRLUsed == 0)
	{
		//printf("RSbuffer:%d, iPush:%d, iPop:%d, iAlloc:%d\n",gpConnectionManager->m_conRSQueue.size(), gpConnectionManager->g_iPushTimes, gpConnectionManager->g_iPopTimes, gpConnectionManager->g_iAllocTimes);
	}
#endif
}

bool CConnectionManager::Connect(const char *czServerIP, const WORD wPort)
{
	sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));   

	addr.sin_family = AF_INET;   
	addr.sin_port = htons(wPort) ;      
	addr.sin_addr.s_addr = inet_addr(czServerIP); 
	//CReliabilityLayer *pRL = gpConnectionManager->AddReliableLayer(0, addr, NULL);
	SSendStruct *pSS = CreateConnectReq(addr, NULL);

	if (pSS == NULL)
	{
		return false;
	}
	OnSend(pSS);

	return true;
}

SSendStruct *CConnectionManager::CreateConnectReq(sockaddr_in sAddr, CReliabilityLayer *pRL)
{
	SSendStruct *pSS = AllocSendStruct();
	//pSS->dwSenderObjectID = pRL->m_dwObjectID;
	pSS->pSender = pRL;
	pSS->sAddr = sAddr;
	pSS->iLen = NETWORK_PKG_HEAD_LEN;
	pSS->pBuf = gpConnectionManager->AllocBuffer(pSS->iLen);
	SNetworkPkgHeader sNPH;
	sNPH.byPkgType = NPT_CONNECTION_REQ;
	sNPH.iTotalLen = 0;
	memcpy(pSS->pBuf, &sNPH, NETWORK_PKG_HEAD_LEN);
	pSS->eSPF = SPF_NO_ACK;
	return pSS;
}

SSendStruct *CConnectionManager::CreateConnectRsp(UINT64 qwToken, sockaddr_in sAddr, CReliabilityLayer *pRL)
{
	SSendStruct *pSS = AllocSendStruct();
	//pSS->dwSenderObjectID = pRL->m_dwObjectID;
	pSS->pSender = pRL;
	pSS->sAddr = sAddr;
	pSS->iLen = NETWORK_PKG_HEAD_LEN;
	pSS->pBuf = gpConnectionManager->AllocBuffer(pSS->iLen);
	SNetworkPkgHeader sNPH;
	sNPH.byPkgType = NPT_CONNECTION_RSP;
	sNPH.qwToken = qwToken;
	sNPH.iTotalLen = 0;
	memcpy(pSS->pBuf, &sNPH, NETWORK_PKG_HEAD_LEN);
	pSS->eSPF = SPF_NO_ACK;
	return pSS;
}

/*bool CConnectionManager::HandleConnection()
{
	// 开始使用select  

	bool bRet = false;
	
	int nRet; // select返回值  
	SSendThreadArg *pSTA = NULL;
	SSendStruct *pSS = NULL;

	FD_ZERO(&m_rfd); // 在使用之前要清空  
	FD_ZERO(&m_wfd);
	list<CUDPSocket *>::iterator itList;
	for (itList = m_conSocketList.begin();itList!=m_conSocketList.end();++itList)
	{
		FD_SET((*itList)->GetSocket(), &m_rfd);//添加至select可读事件监听
		FD_SET((*itList)->GetSocket(), &m_wfd);//添加至select可写事件监听
	}
	nRet = select(0, &m_rfd, &m_wfd, NULL, &m_tvTimer);// 检测是否有套接字是否可读  
	if (nRet == SOCKET_ERROR)     
	{  
		printf("select()\n");  
		return false;  
	}  
	else if (nRet == 0) // 超时  
	{  
		printf("timeout\n");  
		return false;
	}  
	else    // 检测到有套接字可读  
	{  
		for (itList = m_conSocketList.begin();itList!=m_conSocketList.end();++itList)
		{
			//if (FD_ISSET((*itList)->GetSocket(), &m_rfd)) //socket 可读事件
			//{
			//	PTP_WORK pWork = CreateThreadpoolWork(&StartRecvCallBack, *itList, NULL);
			//	SubmitThreadpoolWork(pWork);
			//	//CloseThreadpoolWork(pWork);
			//	//CUDPThread::Create(&StartRecvThread, *itList, 0);
			//	bRet = true;
			//}
			if (FD_ISSET((*itList)->GetSocket(), &m_wfd)) //socket 可写事件
			{
				while ((pSS = PopSS())!=NULL)
				{
					if (pSS->pSender && pSS->pSender->m_dwObjectID == pSS->dwSenderObjectID )
					//if (pSS->eSPF != SPF_RECVED_ACK)
					{
						pSTA = AllocSendThreadArg();
						pSTA->pPacket = pSS;
						pSTA->pSocket = *itList;
						//CUDPThread::Create(&StartSendThread, pSTA, 0);
						PTP_WORK pWork = CreateThreadpoolWork(&StartSendCallBack,pSTA, NULL);
						SubmitThreadpoolWork(pWork);
						//CloseThreadpoolWork(pWork);
						bRet = true;
						break;
					}
					else
					{
						gpConnectionManager->DeallocSendStruct(pSS);
					}
				}				
			}
		}
	}
	return bRet;
}*/
