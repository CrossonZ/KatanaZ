#include "LogicThread.h"
#include "../net/ReliabilityLayer.h"
#include "../net/ConnectionManager.h"
#include <time.h>

bool CLogicThread::AddReliabilityLayer(CReliabilityLayer *pRL)
{
	m_ReliabilityLayerMutex.Lock();
	m_conReliabilityLayerMap.insert(make_pair(pRL->GetToken(), pRL));
	m_ReliabilityLayerMutex.Unlock();
	return true;
}

bool CLogicThread::DeleteReliabilityLayer(CReliabilityLayer *pRL)
{
	m_ReliabilityLayerMutex.Lock();
	m_conReliabilityLayerMap.erase(pRL->GetToken());
	m_ReliabilityLayerMutex.Unlock();
	return true;
}

CReliabilityLayer *CLogicThread::GetRL(const UINT64 qwToken)
{
	CReliabilityLayer *pRL = NULL;
	m_ReliabilityLayerMutex.Lock();
	map<UINT64, CReliabilityLayer*>::iterator itMap = m_conReliabilityLayerMap.find(qwToken);
	if (itMap != m_conReliabilityLayerMap.end())
	{
		pRL = itMap->second;
	}
	m_ReliabilityLayerMutex.Unlock();
	return pRL;
}

void CLogicThread::PushRS(SRecvStruct *pRS)
{
	m_RSQueueMutex.Lock();
	m_conRSQueue.push(pRS);
	m_RSQueueMutex.Unlock();
}

SRecvStruct *CLogicThread::PopRS()
{
	SRecvStruct *pRS = NULL;
	m_RSQueueMutex.Lock();
	if (!m_conRSQueue.empty())
	{
		pRS = m_conRSQueue.front();
		m_conRSQueue.pop();
	}
	m_RSQueueMutex.Unlock();
	return pRS;
}

int CLogicThread::GetWorkbanch()
{
	return 0;
}

void CLogicThread::ThreadLoop()
{
	while (1)
	{
		LoopPopRS();
		//LoopPopSS();
		LoopTraverseConnections();
		Sleep(1);
	}
}

void CLogicThread::LoopPopRS()
{
	SRecvStruct *pRS = NULL;
	SNetworkPkgHeader *pNPH = NULL;
	CReliabilityLayer *pRL = NULL;
	while ((pRS = PopRS()) != NULL)
	{
		pNPH = pRS->GetNetworkPkg();
		pRL = GetRL(pNPH->qwToken);
		switch (pNPH->byPkgType)
		{
		case NPT_ACK:
			{
				if (pRL != NULL && pRL->m_eCS == CS_ESTABLISHED)
				{
					pRL->m_tLastCommunicationTime = time(NULL);
					pRL->HandleSendWindow(pNPH->iSeq);
				}	
				gpConnectionManager->DeallocRecvStruct(pRS);
			}break;
		case NPT_TRAFFIC_CONTROL:
			{
				if (pRL != NULL && pRL->m_eCS == CS_ESTABLISHED)
				{
					pRL->HandleSelfTrafficCtrl(pNPH->iSeq);
				}	
				gpConnectionManager->DeallocRecvStruct(pRS);
			}break;
		case NPT_COMMON_MSG:
			{
				if (pRL!=NULL && pRL->m_eCS == CS_ESTABLISHED)
				{
					pRL->SetAddr(pRS->sAddr);

					int iLost = pRL->CheckPacketLost(pNPH->iSeq);
					if (iLost == 1)
					{
						pRL->m_RecvWindowMutex.Lock();
						if (!pRL->m_conRecvWindow.insert(make_pair(pNPH->iSeq, pRS)).second)
						{
							gpConnectionManager->DeallocRecvStruct(pRS); //插入失败，该序列号的包已经在缓存中
						}
						pRL->m_RecvWindowMutex.Unlock();
					}
					else if (iLost == 0)
					{
						pRL->HandleRecvCommonPkg(pRS, pNPH->iTotalLen,pNPH->iSeq);
						gpConnectionManager->DeallocRecvStruct(pRS);
						pRL->HandleRecombinationPacket();	
					}
					else
					{
						gpConnectionManager->DeallocRecvStruct(pRS);
					}		
				}
				else
				{
					gpConnectionManager->DeallocRecvStruct(pRS);
				}
			}break;
		case NPT_FIN:
			{
				if (pRL != NULL && pRL->m_eCS == CS_ESTABLISHED)
				{
					//pRL->m_eCS = CS_FIN_RECVD; //此时该链接不接收处理任何来自客户端的数据，需等待发送缓冲区中将该链接的所有数据清空后，将链接状态置为CS_RECYCLED并清空所有数据并回收链接
					gpConnectionManager->RecycleReliableLayer(pRL);
				}	
				gpConnectionManager->DeallocRecvStruct(pRS);
			}break;
		default:
			{
				gpConnectionManager->DeallocRecvStruct(pRS);
			}
			break;
		}
	}
}

//void CLogicThread::LoopPopSS()
//{
//	SSendStruct *pSS = NULL;
//	m_SSQueueMutex.Lock();
//	while (!m_conSSQueue.empty())
//	{
//		pSS = m_conSSQueue.front();
//		m_conSSQueue.pop();
//		gpConnectionManager->OnSend(pSS);
//	}
//	m_SSQueueMutex.Unlock();
//}

void CLogicThread::LoopTraverseConnections()
{
	map<UINT64, CReliabilityLayer*>::iterator itTmp ,itMap = m_conReliabilityLayerMap.begin();
	int iStatus = NS_IDLE, iRet = NS_IDLE;
	time_t tNow = time(NULL);
	m_ReliabilityLayerMutex.Lock();
	while (itMap != m_conReliabilityLayerMap.end())
	{
		itTmp = itMap ++;
		switch (itTmp->second->m_eCS)
		{
		case CS_INITIAL:
		case CS_ESTABLISHED:
			{
				if (itTmp->second->m_tLastCommunicationTime + 10 < tNow)
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
				if (itTmp->second->m_tLastCommunicationTime + 20 < tNow)
				{
					itTmp->second->m_eCS = CS_RECYCLED;
					gpConnectionManager->RecycleReliableLayer(itTmp->second);
				}
			}break;
		default:
			break;
		}
	}
	m_ReliabilityLayerMutex.Unlock();
}