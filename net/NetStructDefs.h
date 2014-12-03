#ifndef __INCLUDED_NET_STRUCT_DEFINES_H_
#define __INCLUDED_NET_STRUCT_DEFINES_H_
#include "../main/GlobalDefines.h"

enum CONNECTION_STATE
{
	CS_INITIAL = 1, //CS_CLOSED
	CS_ESTABLISHED = 2,
	CS_FIN_RECVD = 3,


	CS_TIMEOUT = 4,
	CS_RECYCLED = 5,

};


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////Socket Manager////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum E_NETWORK_PKG_TYPE
{
	NPT_START = 0,

	NPT_CONNECTION_REQ = 1,
	NPT_CONNECTION_RSP = 2,
	NPT_ACK = 3,
	NPT_NAK = 4,
	NPT_TRAFFIC_CONTROL = 5,
	NPT_COMMON_MSG = 6,
	NPT_FIN = 7,

	NPT_END
};

#pragma pack(1)
struct SObject
{
	void SetObjectID(DWORD dwID) {m_dwObjectID = dwID;}
	DWORD m_dwObjectID;
};
struct SNetworkPkgHeader
{
	SNetworkPkgHeader():byPkgType(NPT_START),qwToken(0),iSeq(0),iTotalLen(0)
	{
	}
	BYTE byPkgType; //
	UINT64 qwToken;
	int iSeq;
	int iTotalLen;
};


struct SNetworkPkg: public SObject
{
	SNetworkPkgHeader sNPH;
	char *pBuf;
};
#pragma pack()

#define NETWORK_PKG_HEAD_LEN 17


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////Reliable Layer////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma pack(1)

enum E_COMMON_PKG_TYPE
{

	CPT_SPLIT = 1,
	CPT_WHOLE = 2,
	/////////////
	CPT_END
};

struct SCommonPkgHead
{
	BYTE byCommonPkgType;
	int iLen;
};
struct SRecvStruct;
struct SCommonPkg//: public SObject
{
	//BYTE byCommonPkgType;
	//int iLen;
	SCommonPkgHead sCPH;
	char szBuf[MAX_CP_SIZE];
	SRecvStruct *pRS;
};

#pragma pack()

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////Socket Layer//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum SEND_PACKET_FLAG
{
	SPF_NO_ACK = 0,
	SPF_NEED_ACK = 1,
	SPF_RECVED_ACK = 2,
};

class CUDPSocket;
class CReliabilityLayer;

struct SSendStruct: public SObject
{
	SSendStruct():iTTL(0),eSPF(SPF_NEED_ACK)
	{
	}
	char *pBuf;
	int iLen;
	sockaddr_in sAddr;
	int iTTL;
	SEND_PACKET_FLAG eSPF;
	CReliabilityLayer *pSender;
	DWORD dwSenderObjectID;
};

struct SRecvStruct: public SObject
{
	SRecvStruct(): iLen(0),iRefCount(1),m_iOffset(0)
	{
	}
	char *ReadBuf(const int iReadLen)
	{
		if (m_iOffset + iReadLen < iLen)
		{
			return NULL;
		}
		++iRefCount;
		m_iOffset += iReadLen;
		return szBuf + m_iOffset;
	}

	SNetworkPkgHeader *GetNetworkPkg()
	{
		return NETWORK_PKG_HEAD_LEN > iLen ? NULL : (SNetworkPkgHeader *)szBuf;
	}

	SCommonPkg *GetCommonPkg()
	{
		if (m_iOffset < NETWORK_PKG_HEAD_LEN)
		{
			m_iOffset = NETWORK_PKG_HEAD_LEN;
		}
		if (m_iOffset + sizeof(SCommonPkgHead) > iLen)
		{
			return NULL;
		}
		SCommonPkg *pCP = (SCommonPkg*)(szBuf+m_iOffset);
		m_iOffset += sizeof(SCommonPkgHead);
		//*(pCP->szBuf = szBuf+ m_iOffset;
		m_iOffset += pCP->sCPH.iLen;
		if (m_iOffset > iLen)
		{
			return NULL;
		}
		++iRefCount;
		pCP->pRS = this;
		return pCP;
	}

	void DecRef()
	{
		--iRefCount;
	}
	char szBuf[MAX_MTU_SIZE];

	int iLen;
	sockaddr_in sAddr;
	CUDPSocket *pSock;
	int iRefCount;
	int m_iOffset;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////Logical Layer/////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct SSinglePacket: public SObject
{
	/// The system that send this packet.
	sockaddr_in sAddr;

	/// A unique identifier for the system that sent this packet, regardless of IP address (internal / external / remote system)
	/// Only valid once a connection has been established (ID_CONNECTION_REQUEST_ACCEPTED, or ID_NEW_INCOMING_CONNECTION)
	/// Until that time, will be UNASSIGNED_RAKNET_GUID
	UINT64 qwGUID;

	/// The length of the data in bytes
	unsigned int iLen;

	/// The length of the data in bits
	//BitSize_t bitSize;

	/// The data from the sender
	char* pBuf;

	/// @internal
	/// Indicates whether to delete the data, or to simply delete the packet.
	bool bDeleteData;

	/// @internal
	/// If true, this message is meant for the user, not for the plugins, so do not process it through plugins
	//bool wasGeneratedLocally;
};
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define RS_QUEUE_MAX 10000
#define SS_QUEUE_MAX 1000


enum NET_STATUS
{
	NS_CONGEST = -1,
	NS_NORMAL = 0,
	NS_IDLE = 1,
};

struct SSendThreadArg: public SObject
{
	SSendStruct *pPacket;
	CUDPSocket *pSocket;
};

#endif