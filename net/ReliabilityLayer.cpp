#include "ReliabilityLayer.h"
#include "ConnectionManager.h"
#include <time.h>
#include <stdio.h>
#include <math.h>

void CReliabilityLayer::SetAddr(sockaddr_in &sAddr)
{
	m_sAddr = sAddr;
	m_tLastCommunicationTime = time(NULL);
}

void CReliabilityLayer::Reset(sockaddr_in  &sAddr, UINT64 qwToken, CUDPSocket *pSocket)
{
	SetAddr(sAddr);
	m_qwToken = qwToken;
	m_pCurrentNP = NULL;

	m_pSocket = pSocket;
	m_eCS = CS_ESTABLISHED;
	m_iRemainSize = 0;

	m_iLastDeleteOffset = m_iLastACKSeq = 0;
	m_iSendPkgSeq = 0;  

	m_iNextRecvSeq = 1;
	m_iSelfTrafficCtrl = -1;
	m_iLastRtt = m_iEstimateRtt = m_iDeviationRtt = CHECK_RESEND_TIME;
	m_iResendTimes = 0;
	m_tLastCommunicationTime = time(NULL);
	//char szFileName[32];
	//_snprintf(szFileName, sizeof(szFileName), "%llu", qwToken);
	//pFile = CreateFile(szFileName, GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_FLAG_WRITE_THROUGH, NULL);
}

bool CReliabilityLayer::DeallocRL()
{
	m_RecvWindowMutex.Lock();
	for (map<int, SRecvStruct*>::iterator itMap = m_conRecvWindow.begin();itMap!=m_conRecvWindow.end();++itMap)
	{
		gpConnectionManager->DeallocRecvStruct(itMap->second);
	}
	m_conRecvWindow.clear();
	m_RecvWindowMutex.Unlock();

	SCommonPkg *pCP = NULL;
	while (!m_conSplitPkgQueue.empty())
	{
		pCP = m_conSplitPkgQueue.front().second;
		m_conSplitPkgQueue.pop();
		gpConnectionManager->DeallocRecvStruct(pCP->pRS);
	}

	for (int i=0;i<m_conSendWindow.size();++i)
	{
		gpConnectionManager->DeallocSendStruct(m_conSendWindow[i].pSS);
	}
	m_conSendWindow.clear();

	while (!m_conSendPacketQueue.empty())
	{
		gpConnectionManager->DeallocSinglePacket(m_conSendPacketQueue.front());
		m_conSendPacketQueue.pop();
	}

	while (!m_conCompletePacketQueue.empty())
	{
		gpConnectionManager->DeallocSinglePacket(m_conCompletePacketQueue.front());
		m_conCompletePacketQueue.pop();
	}

	while (!m_conPRHSendPacketQueue.empty())
	{
		gpConnectionManager->DeallocSinglePacket(m_conPRHSendPacketQueue.front());
		m_conPRHSendPacketQueue.pop();
	}
	return true;
}

void CReliabilityLayer::HandleRecvCommonPkg(SRecvStruct *pRS, int iTotalLen, const int iSeq)
{
	SSinglePacket *pSP = NULL;
	SCommonPkg *pCommon = NULL;
	
	int iLen = 0;
	while ((pCommon = pRS->GetCommonPkg())!= NULL)
	{
		/*if (iOffset + sizeof(SCommonPkgHead) >= iTotalLen)
		{
			//Fucking occur;
			//gpConnectionManager->DeallocCommonPkg(pCommon);
			return;
		}
		iLen = *(int *)(pCommonBuf+iOffset+1);
		if (iOffset + sizeof(SCommonPkgHead) + iLen > iTotalLen || (pCommonBuf[iOffset] == CPT_SPLIT && iLen < 4))
		{
			//Fucking occur;
			return;
		}
		pCommon = gpConnectionManager->AllocCommonPkg();
		memcpy(&pCommon->sCPH, pCommonBuf + iOffset, sizeof(SCommonPkgHead));
		iOffset += sizeof(SCommonPkgHead);
		pCommon->pBuf = gpConnectionManager->AllocBuffer(pCommon->sCPH.iLen);
		memcpy(pCommon->pBuf, pCommonBuf+iOffset, pCommon->sCPH.iLen);
		iOffset += pCommon->sCPH.iLen;*/
		
		switch(pCommon->sCPH.byCommonPkgType)
		{
		case CPT_SPLIT:
			{
				if (pCommon->sCPH.iLen <= 4 || !HandleSplitPkg(pCommon, iSeq))
				{
					//gpConnectionManager->DeallocCommonPkg(pCommon);
					//pCommonBuf->DecRef();
					gpConnectionManager->DeallocRecvStruct(pRS);   //出现异常情况
					return;
				}				
			}break;
		case CPT_WHOLE:
			{
				pSP = gpConnectionManager->AllocSinglePacket();
				pSP->bDeleteData = false;
				pSP->iLen = pCommon->sCPH.iLen;
				pSP->pBuf = gpConnectionManager->AllocBuffer(pSP->iLen);
				memcpy(pSP->pBuf, pCommon->szBuf, pSP->iLen);
				pSP->pBuf[pSP->iLen] = '\0';
				//PushCompletePacket(pSP);
				TmpHandleSP(pSP);
				//pCommonBuf->DecRef();
				gpConnectionManager->DeallocRecvStruct(pRS);
				//gpConnectionManager->DeallocCommonPkg(pCommon);
			}break;
		default:
			{	
				//gpConnectionManager->DeallocCommonPkg(pCommon);
				//pCommonBuf->DecRef();
				gpConnectionManager->DeallocRecvStruct(pRS);
			}
			break;
		}
	}
}

bool CReliabilityLayer::PushNetworkPkg(SNetworkPkg *pPkg)
{
	pPkg->sNPH.iSeq = ++m_iSendPkgSeq;

	//这段代码太搓了，必须要改
	SSendStruct *pSS = gpConnectionManager->AllocSendStruct();
	//pSS->dwSenderObjectID = m_dwObjectID;
	pSS->pSender = this;
	pSS->sAddr = m_sAddr;
	pSS->iLen = pPkg->sNPH.iTotalLen + NETWORK_PKG_HEAD_LEN;
	pSS->pBuf = gpConnectionManager->AllocBuffer(pSS->iLen);
	memcpy(pSS->pBuf, (char *)&pPkg->sNPH, NETWORK_PKG_HEAD_LEN);
	memcpy(pSS->pBuf+NETWORK_PKG_HEAD_LEN, pPkg->pBuf, pPkg->sNPH.iTotalLen);
	gpConnectionManager->DeallocNetworkPkg(pPkg);
	SSendWindow sSW;
	sSW.pSS = pSS;
	sSW.dwSendTime = GetTickCount();
	sSW.iResendTimes = 0;
	m_SendWindowMutex.Lock();
	m_conSendWindow.push_back(sSW);
	m_SendWindowMutex.Unlock();
	return gpConnectionManager->OnSend(pSS);	
}

bool CReliabilityLayer::HandleSplitPkg(SCommonPkg *pCP, int iPkgSeq)
{
	unsigned int iTotalLen = *(unsigned int *)pCP->szBuf;
	if (m_conSplitPkgQueue.empty())
	{
		m_conSplitPkgQueue.push(make_pair(iPkgSeq, pCP));
		m_iSplitTotalLen = iTotalLen;
		m_iSplitCurrentRecv = pCP->sCPH.iLen-4; //肯定大于零的
		return true;
	}
	int iLastSeq = m_conSplitPkgQueue.back().first;
	if (iLastSeq + 1!= iPkgSeq || iTotalLen != m_iSplitTotalLen) //出现序列号跳包的分包，理论上是不可能的
	{
		return false;
	}
	if (m_iSplitCurrentRecv + pCP->sCPH.iLen-4 > m_iSplitTotalLen) //预计拼包后超长，正常连接发过来时不可能的
	{
		return false;
	}
	if (m_iSplitCurrentRecv + pCP->sCPH.iLen-4 == m_iSplitTotalLen)
	{
		SSinglePacket *pSP = gpConnectionManager->AllocSinglePacket();
		if (pSP == NULL)
		{
			return false;
		}
		pSP->sAddr = m_sAddr;
		pSP->bDeleteData = false;
		pSP->iLen = m_iSplitTotalLen;
		pSP->pBuf = gpConnectionManager->AllocBuffer(m_iSplitTotalLen);
		int iOffset = 0;
		SCommonPkg *pCommon = NULL;
		while (!m_conSplitPkgQueue.empty())
		{
			pCommon = m_conSplitPkgQueue.front().second;
			m_conSplitPkgQueue.pop();
			memcpy(pSP->pBuf+iOffset, pCommon->szBuf+4, pCommon->sCPH.iLen-4);
			iOffset += pCommon->sCPH.iLen-4;
			if (pCommon->pRS == NULL)
			{
				printf("FUCKING occurs!\n");
			}
			else
			{
				gpConnectionManager->DeallocRecvStruct(pCommon->pRS);
			}
		}
		memcpy(pSP->pBuf+iOffset, pCP->szBuf+4, pCP->sCPH.iLen-4);
		if (pCP->pRS == NULL)
		{
			//printf("FUCKING occurs!\n");
		}
		else
		{
			gpConnectionManager->DeallocRecvStruct(pCP->pRS);
		}
		//gpConnectionManager->PushCompletePacket(pSP);
		TmpHandleSP(pSP);
	}
	else
	{
		m_conSplitPkgQueue.push(make_pair(iPkgSeq, pCP));
		m_iSplitCurrentRecv += pCP->sCPH.iLen-4; //肯定大于零的
	}
	return true;
}

bool CReliabilityLayer::PushSP(SSinglePacket *pSP)
{
	bool bRet = false;
	m_SendPacketQueueMutex.Lock();
	if (m_conSendPacketQueue.size() < CONGESTION_THRESHOLD)
	{
		m_conSendPacketQueue.push(pSP);
		bRet = true;
	}
	m_SendPacketQueueMutex.Unlock();
	return bRet;
}

void CReliabilityLayer::PushCompletePacket(SSinglePacket *pSP)
{
	m_CompletePacketQueueMutex.Lock();
	m_conCompletePacketQueue.push(pSP);
	m_CompletePacketQueueMutex.Unlock();
}

SSinglePacket* CReliabilityLayer::FetchPacket()
{
	SSinglePacket *pSP = NULL;
	m_CompletePacketQueueMutex.Lock();
	if (!m_conCompletePacketQueue.empty())
	{
		pSP = m_conCompletePacketQueue.front();
		m_conCompletePacketQueue.pop();
	}
	m_CompletePacketQueueMutex.Unlock();
	return pSP;
}

void CReliabilityLayer::TmpHandleSP(SSinglePacket *pSP)
{
	/*if (strcmp(pSP->pBuf, "#FILE_END#") != 0 )
	{
		DWORD dwActualWrite = 0;
		WriteFile(pFile, pSP->pBuf,pSP->iLen, &dwActualWrite, NULL);
		gpConnectionManager->DeallocSinglePacket(pSP);
	}
	else
	{
		FlushFileBuffers(pFile);
		gpConnectionManager->DeallocSinglePacket(pSP);
	}*/
	PushSP(pSP);
}

void CReliabilityLayer::PushPRHSendPacket(SSinglePacket *pSP)
{
	m_conPRHSendPacketQueue.push(pSP);
}

SSinglePacket *CReliabilityLayer::PopSP()
{
	SSinglePacket *pSP = NULL;
	if (!m_conPRHSendPacketQueue.empty())
	{
		pSP = m_conPRHSendPacketQueue.front();
		m_conPRHSendPacketQueue.pop();
	}
	else
	{
		m_SendPacketQueueMutex.Lock();
		if (!m_conSendPacketQueue.empty())
		{
			pSP = m_conSendPacketQueue.front();
			m_conSendPacketQueue.pop();
		}
		m_SendPacketQueueMutex.Unlock();
	}
	return pSP;
}

int CReliabilityLayer::HandleSendPkg()
{
	SSinglePacket *pSP = NULL;
	SCommonPkg *pCP = NULL;
	SCommonPkgHead sCPH;
	int iStatus = NS_IDLE;


	if (!gpConnectionManager->CheckOnSend())
	{
		return NS_CONGEST;
	}
	if ((pSP = PopSP()) != NULL)
	{
		iStatus = NS_NORMAL;
		if (pSP->iLen+sizeof(SCommonPkgHead) <= m_iRemainSize) //m_iRemainSize 永远小于MAX_MTU_SIZE
		{
			if (m_pCurrentNP == NULL)
			{
				m_pCurrentNP = gpConnectionManager->AllocNetworkPkg();
				m_pCurrentNP->sNPH.byPkgType = NPT_COMMON_MSG;
				m_pCurrentNP->sNPH.qwToken = m_qwToken;
				m_pCurrentNP->pBuf = gpConnectionManager->AllocBuffer(MAX_MTU_SIZE);
				m_iRemainSize = MAX_MTU_SIZE;
				m_pCurrentNP->sNPH.iTotalLen = 0;
				//m_pCurrentNP->sCommon.iSeq = m_iSendPkgSeq;
			}

			sCPH.byCommonPkgType = CPT_WHOLE;
			sCPH.iLen = pSP->iLen;
			memcpy(m_pCurrentNP->pBuf+m_pCurrentNP->sNPH.iTotalLen, &sCPH, sizeof(SCommonPkgHead));
			m_pCurrentNP->sNPH.iTotalLen += sizeof(SCommonPkgHead);
			memcpy(m_pCurrentNP->pBuf+m_pCurrentNP->sNPH.iTotalLen, pSP->pBuf, pSP->iLen);
			m_pCurrentNP->sNPH.iTotalLen += pSP->iLen;
			m_iRemainSize -= pSP->iLen+sizeof(SCommonPkgHead);
		}
		else if (pSP->iLen+sizeof(SCommonPkgHead) <= MAX_MTU_SIZE)
		{
			if (m_pCurrentNP != NULL)
			{
				PushNetworkPkg(m_pCurrentNP);
			}
			m_pCurrentNP = gpConnectionManager->AllocNetworkPkg();
			m_pCurrentNP->sNPH.byPkgType = NPT_COMMON_MSG;
			m_pCurrentNP->sNPH.qwToken = m_qwToken;
			m_pCurrentNP->pBuf = gpConnectionManager->AllocBuffer(MAX_MTU_SIZE);
			m_iRemainSize = MAX_MTU_SIZE;
			m_pCurrentNP->sNPH.iTotalLen = 0;
			//m_pCurrentNP->sCommon.iSeq = m_iSendPkgSeq;
			sCPH.byCommonPkgType = CPT_WHOLE;
			sCPH.iLen = pSP->iLen;
			memcpy(m_pCurrentNP->pBuf+m_pCurrentNP->sNPH.iTotalLen, &sCPH, sizeof(SCommonPkgHead));
			m_pCurrentNP->sNPH.iTotalLen += sizeof(SCommonPkgHead);
			memcpy(m_pCurrentNP->pBuf+m_pCurrentNP->sNPH.iTotalLen, pSP->pBuf, pSP->iLen);
			m_pCurrentNP->sNPH.iTotalLen += pSP->iLen;
			m_iRemainSize -= m_pCurrentNP->sNPH.iTotalLen;
		}
		else
		{	
			unsigned int iOffset = 0;
			int iCurrentLen = 0;
			while (iOffset < pSP->iLen)
			{
				if (m_pCurrentNP != NULL)
				{
					PushNetworkPkg(m_pCurrentNP);
				}
				int iSmall1 = pSP->iLen-iOffset;
				int iSmall2 = MAX_MTU_SIZE-sizeof(SCommonPkgHead)-sizeof(int);
				iCurrentLen = min(iSmall1, iSmall2);
				m_pCurrentNP = gpConnectionManager->AllocNetworkPkg();
				m_pCurrentNP->sNPH.byPkgType = NPT_COMMON_MSG;
				m_pCurrentNP->sNPH.qwToken = m_qwToken;
				m_pCurrentNP->pBuf = gpConnectionManager->AllocBuffer(MAX_MTU_SIZE);
				m_iRemainSize = MAX_MTU_SIZE;
				m_pCurrentNP->sNPH.iTotalLen = 0;
				//m_pCurrentNP->sCommon.iSeq = m_iSendPkgSeq;
				sCPH.byCommonPkgType = CPT_SPLIT;
				sCPH.iLen = iCurrentLen+4;
				memcpy(m_pCurrentNP->pBuf+m_pCurrentNP->sNPH.iTotalLen, &sCPH, sizeof(SCommonPkgHead));
				m_pCurrentNP->sNPH.iTotalLen += sizeof(SCommonPkgHead);
				memcpy(m_pCurrentNP->pBuf+m_pCurrentNP->sNPH.iTotalLen, &pSP->iLen, sizeof(int));
				m_pCurrentNP->sNPH.iTotalLen += sizeof(int);
				memcpy(m_pCurrentNP->pBuf+m_pCurrentNP->sNPH.iTotalLen, pSP->pBuf+iOffset, iCurrentLen);
				m_pCurrentNP->sNPH.iTotalLen += iCurrentLen;
				m_iRemainSize -= m_pCurrentNP->sNPH.iTotalLen;
				iOffset += iCurrentLen;
			}
		}
		gpConnectionManager->DeallocSinglePacket(pSP);
	}
	if (m_pCurrentNP != NULL)
	{
		PushNetworkPkg(m_pCurrentNP);
		m_pCurrentNP = NULL;
		m_iRemainSize = 0;
	}
	return iStatus;
}

int CReliabilityLayer::CheckResendPkg()
{
	DWORD dwTickCount = GetTickCount();
	int  iStatus = NS_IDLE;
	int i=0;
	m_SendWindowMutex.Lock();
	for (i=0;i<m_conSendWindow.size();++i)
	{
		if (m_conSendWindow[i].pSS->eSPF != SPF_RECVED_ACK || m_conSendWindow[i].dwSendTime + m_iEstimateRtt > dwTickCount )
		{
			break;
		}
		gpConnectionManager->DeallocSendStruct(m_conSendWindow[i].pSS);
	}
	if (i != 0)
	{
		m_conSendWindow.erase(m_conSendWindow.begin(), m_conSendWindow.begin()+i);
		m_iLastDeleteOffset = m_iLastDeleteOffset > i ? m_iLastDeleteOffset - i : 0;
	}
	for (i=0;i<m_conSendWindow.size() && i<RESEND_PACKET_NUM;++i)
	{
		if (m_conSendWindow[i].dwSendTime + (m_iEstimateRtt <<(1+m_iResendTimes)) < dwTickCount)
		{
			++m_conSendWindow[i].iResendTimes;
			m_conSendWindow[i].dwSendTime = dwTickCount;
			gpConnectionManager->OnSend(m_conSendWindow[i].pSS);
			iStatus = NS_NORMAL;
			++m_iResendTimes;
			printf("m_iResendTimes:%d,seq:%d\n",m_iResendTimes,m_iLastACKSeq+i);
			if (m_conSendWindow[i].iResendTimes > 5)
			{
				m_conSendWindow[i].pSS->eSPF = SPF_RECVED_ACK;
				++m_iLastDeleteOffset;
				++m_iLastACKSeq;
			}
		}
		else
		{
			break;
		}
	}
	m_SendWindowMutex.Unlock();
	return iStatus;
}

bool CReliabilityLayer::HandleRecombinationPacket()
{
	bool bIdle = true;
	m_RecvWindowMutex.Lock();
	if (m_conRecvWindow.empty())
	{
		m_RecvWindowMutex.Unlock();
		return bIdle;
	}
	map<int, SRecvStruct*>::iterator itMap = m_conRecvWindow.begin();
	if (itMap->first != m_iNextRecvSeq)
	{
		if (m_conRecvWindow.size() > TRAFFIC_CONTROL_THRESHOLD)
		{
			BuildTrafficControl((*m_conRecvWindow.begin()).first);
		}
		m_RecvWindowMutex.Unlock();
		return bIdle;
	}

	SSinglePacket *pPacket = NULL;
	SCommonPkg *pCommon = NULL;
	SRecvStruct *pRS = NULL;
	SNetworkPkgHeader *pNPH = NULL;
	for (;itMap!=m_conRecvWindow.end();++itMap)
	{
		if (itMap->first > m_iNextRecvSeq)
		{
			break;
		}
		pRS = itMap->second;
		//pNPH = (SNetworkPkgHeader *)pRS->szBuf;
		//int iOffset = NETWORK_PKG_HEAD_LEN;
		pNPH = pRS->GetNetworkPkg();
		if (pNPH != NULL)
		{
			HandleRecvCommonPkg(pRS, pNPH->iTotalLen, pNPH->iSeq);
		}
		gpConnectionManager->DeallocRecvStruct(pRS);
		++m_iNextRecvSeq;
	}
	m_conRecvWindow.erase(m_conRecvWindow.begin(),itMap);
	if (m_conRecvWindow.size() > TRAFFIC_CONTROL_THRESHOLD)
	{
		BuildTrafficControl((*m_conRecvWindow.begin()).first);
	}
	m_RecvWindowMutex.Unlock();
	return bIdle;
}

int CReliabilityLayer::CheckPacketLost(const int iRecvSeq)
{
	if (iRecvSeq == m_iNextRecvSeq)
	{
		BuildACK(m_iNextRecvSeq);
		++m_iNextRecvSeq;
		return 0;
	}
	if (iRecvSeq > m_iNextRecvSeq)
	{
		return 1;  //后面包，先缓存
	}
	if (iRecvSeq < m_iNextRecvSeq)
	{
		return -1; //旧包，丢弃
	}
	return -2;
}

void CReliabilityLayer::HandleSendWindow(int iAckSeq)
{
	if (iAckSeq > m_iLastACKSeq)
	{
		DWORD dwTick = GetTickCount();
		int i, iLast = m_iLastDeleteOffset+iAckSeq-m_iLastACKSeq - 1;
		if (iLast < 0 || iLast >= m_conSendWindow.size())
		{
			printf("Fucking Occurs, iLast:%d , m_conSendWindowSize:%d\n",iLast, m_conSendWindow.size());
			return;
		}

		m_SendWindowMutex.Lock();
		m_iLastRtt = dwTick - m_conSendWindow[iLast].dwSendTime;
		m_iDeviationRtt = m_iLastRtt - m_iEstimateRtt;
		m_iEstimateRtt += m_iDeviationRtt>>3;
		//printf("m_iLastRtt:%d, m_iDeviationRtt:%d, m_iEstimateRtt:%d\n",m_iLastRtt,m_iDeviationRtt,m_iEstimateRtt);
		for (i=0;i<iAckSeq-m_iLastACKSeq;++i)
		{
			//gpConnectionManager->DeallocSendStruct(m_conSendWindow[i].first);
			m_conSendWindow[m_iLastDeleteOffset+i].pSS->eSPF = SPF_RECVED_ACK;
			m_conSendWindow[m_iLastDeleteOffset+i].dwSendTime = dwTick;
		}
		m_iLastDeleteOffset += i;
		//m_conSendWindow.erase(m_conSendWindow.begin(), m_conSendWindow.begin()+i);
		m_SendWindowMutex.Unlock();
		m_iLastACKSeq = iAckSeq;
	}
}

void CReliabilityLayer::HandleSelfTrafficCtrl(int iSelfTrafficCtrl)
{
	m_iSelfTrafficCtrl = iSelfTrafficCtrl;
}

bool CReliabilityLayer::ClearRL()
{
	map<int, SRecvStruct*>::iterator itMap = m_conRecvWindow.begin();
	for (;itMap!=m_conRecvWindow.end();++itMap)
	{
		gpConnectionManager->DeallocRecvStruct(itMap->second);
	}
	m_conRecvWindow.clear();

	while (!m_conSplitPkgQueue.empty())
	{
		gpConnectionManager->DeallocRecvStruct(m_conSplitPkgQueue.front().second->pRS);
		m_conSplitPkgQueue.pop();
	}
	m_SendWindowMutex.Lock();
	for (int i=0;i<m_conSendWindow.size();++i)
	{
		gpConnectionManager->DeallocSendStruct(m_conSendWindow[i].pSS);
	}
	m_conSendWindow.clear();
	m_SendWindowMutex.Unlock();

	while (!m_conSendPacketQueue.empty())
	{
		gpConnectionManager->DeallocSinglePacket(m_conSendPacketQueue.front());
		m_conSendPacketQueue.pop();
	}

	while (!m_conPRHSendPacketQueue.empty())
	{
		gpConnectionManager->DeallocSinglePacket(m_conPRHSendPacketQueue.front());
		m_conPRHSendPacketQueue.pop();
	}
	return true;
}

void CReliabilityLayer::BuildACK(const int iACKSeq)
{
	SSendStruct *pSS = gpConnectionManager->AllocSendStruct();
	//pSS->dwSenderObjectID = m_dwObjectID;
	pSS->pSender = this;
	pSS->sAddr = m_sAddr;
	pSS->iLen = NETWORK_PKG_HEAD_LEN;
	pSS->pBuf = gpConnectionManager->AllocBuffer(pSS->iLen);
	pSS->eSPF = SPF_NO_ACK;
	SNetworkPkgHeader sNPH;
	sNPH.byPkgType = NPT_ACK;
	sNPH.iSeq = iACKSeq;
	sNPH.qwToken = m_qwToken;
	sNPH.iTotalLen = 0;
	memcpy(pSS->pBuf, (char *)&sNPH, NETWORK_PKG_HEAD_LEN);
	gpConnectionManager->OnSend(pSS);
}

void CReliabilityLayer::BuildTrafficControl(const int iRecvWindowBegin)
{
	SSendStruct *pSS = gpConnectionManager->AllocSendStruct();
	pSS->pSender = this;
	//pSS->dwSenderObjectID = m_dwObjectID;
	pSS->sAddr = m_sAddr;
	pSS->iLen = 8 + NETWORK_PKG_HEAD_LEN;
	pSS->pBuf = gpConnectionManager->AllocBuffer(pSS->iLen);
	pSS->eSPF = SPF_NO_ACK;
	SNetworkPkgHeader sNPH;
	sNPH.byPkgType = NPT_TRAFFIC_CONTROL;
	sNPH.iSeq = iRecvWindowBegin;
	sNPH.qwToken = m_qwToken;
	sNPH.iTotalLen = 0;
	memcpy(pSS->pBuf, (char *)&sNPH, NETWORK_PKG_HEAD_LEN);
	//memcpy(pSS->pBuf+NETWORK_PKG_HEAD_LEN, (char *)&iRecvWindowBegin, 4);
	//memcpy(pSS->pBuf+NETWORK_PKG_HEAD_LEN+4, (char *)&iRecvWindowSize, 4);
	gpConnectionManager->OnSend(pSS);
}
