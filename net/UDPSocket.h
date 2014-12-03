#ifndef __INCLUDED_UDPSOCKET_H_
#define __INCLUDED_UDPSOCKET_H_

#include "NetStructDefs.h"
;
#include <boost/function.hpp>
#include <boost/bind.hpp>
////////////////////////////////////////////

#define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR,12)

//class CEventHandler
//{
//public:
//	CEventHandler() {}
//	virtual ~CEventHandler() {}
//
//	virtual bool OnRecv(SRecvStruct *pRS);
//	virtual void DeallocRecvStruct(SRecvStruct *pRS);
//
//	virtual SRecvStruct *AllocRecvStruct();
//};

typedef boost::function<bool(SRecvStruct*)> OnRecvCallBack;

class CConnectionManager;
class CUDPSocket
{
public:
	CUDPSocket() {;}
	virtual ~CUDPSocket() {;}
	int Bind(CConnectionManager *pEH, const WORD wPort, const bool bBlocking, const bool bIPHdr);
	void RecvFromSocket();
	void SendToSocket(/*SSendStruct *pSS*/);
	SOCKET GetSocket() {return m_sock;}

private:
	void SetSockOpt(const bool bBlocking, const bool bIPHdr);

	void HandleRecv(SRecvStruct *pRS);

	SOCKET m_sock;
	sockaddr_in m_sAddr;
	CConnectionManager *m_pEH;
	OnRecvCallBack m_funcOnRecv;
};

#endif