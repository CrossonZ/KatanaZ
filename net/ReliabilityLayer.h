#ifndef __INCLUDED_RELIABILITY_LAYER_H_
#define __INCLUDED_RELIABILITY_LAYER_H_

#include "NetStructDefs.h"
#include "../tools/SimpleMutex.h"
#include <queue>

#define TRAFFIC_CONTROL_THRESHOLD 20
#define RESEND_PACKET_NUM 5
#define CHECK_RESEND_TIME 2000
#define CONGESTION_THRESHOLD 100

//Reliable Layer 与Player应该是一一对应关系，相当于client session
struct SSendWindow
{
	SSendStruct * pSS;
	DWORD dwSendTime;
	int iResendTimes;
};

class CReliabilityLayer: public SObject
{
public:
	CReliabilityLayer() 
	{

	}
	virtual ~CReliabilityLayer() {}

	bool DeallocRL();
	void HandleRecvCommonPkg(SRecvStruct *pRS, int iTotalLen, const int iSeq);

	int HandleSendPkg(); //将上层的包推到网络层

	int CheckResendPkg();

	bool HandleRecombinationPacket();

	void Reset(sockaddr_in  &sAddr, UINT64 qwToken, CUDPSocket *pSocket);

	void SetAddr(sockaddr_in &sAddr);

	sockaddr_in GetAddr() const {return m_sAddr;}
	UINT64 GetToken() const {return m_qwToken;}

	bool ClearSendBufMap();

	bool PushSP(SSinglePacket *pSP); //接收来自player的直接数据
	int CheckPacketLost(const int iRecvSeq);

	void HandleSendWindow(int iACKSeq);

	void HandleSelfTrafficCtrl(int iSelfTrafficCtrl);

	bool ClearRL();

	map<int, SRecvStruct*> m_conRecvWindow;
	CSimpleMutex m_RecvWindowMutex;
	int m_iSelfTrafficCtrl;
	int m_iLastACKSeq;
	int m_iLastDeleteOffset;
	time_t m_tLastCommunicationTime;
	CUDPSocket *m_pSocket;

	CONNECTION_STATE m_eCS;
private:

	void TmpHandleSP(SSinglePacket *pSP);

	void PushCompletePacket(SSinglePacket *pSP);
	SSinglePacket* FetchPacket();
	
	void PushPRHSendPacket(SSinglePacket *pSP);
	bool SendToSocket(/*SNetworkPkg *pNP*/);

	void BuildACK(const int iACKSeq);
	void BuildTrafficControl(const int iRecvWindowBegin);

	SSinglePacket *PopSP();

	bool PushNetworkPkg(SNetworkPkg *pPkg);

	bool HandleSplitPkg(SCommonPkg *pCP, int iPkgSeq);

	queue<pair<int, SCommonPkg *> > m_conSplitPkgQueue;

	vector<SSendWindow> m_conSendWindow;
	CSimpleMutex m_SendWindowMutex;

	queue<SSinglePacket *> m_conSendPacketQueue;   //用于缓存来自player的直接数据
	CSimpleMutex m_SendPacketQueueMutex;

	queue<SSinglePacket *> m_conCompletePacketQueue;  //供用户层获取网络数据队列
	CSimpleMutex m_CompletePacketQueueMutex;

	queue<SSinglePacket *> m_conPRHSendPacketQueue; //

	SNetworkPkg *m_pCurrentNP;

	sockaddr_in m_sAddr; 
	unsigned int m_iRemainSize;

	unsigned int m_iSplitTotalLen;
	unsigned int m_iSplitCurrentRecv;


	int m_iSendPkgSeq;

	int m_iNextRecvSeq;

	int m_iLastNAKSeq;
	WORD m_wLastNAKSplitSeq;
	WORD m_wDumplicateNAKNum;

	UINT64 m_qwToken;

	int m_iLastRtt;
	int m_iDeviationRtt;
	int m_iEstimateRtt;

	int m_iResendTimes;

	//////////////////

	//HANDLE pFile;
};

#endif