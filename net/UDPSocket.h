#ifndef __INCLUDED_UDPSOCKET_H_
#define __INCLUDED_UDPSOCKET_H_

#include "NetStructDefs.h"
;
#if USE_BOOST == 1
#include <boost/function.hpp>
#include <boost/bind.hpp>

typedef boost::function<bool(SRecvStruct*)> OnRecvCallBack;
#endif
////////////////////////////////////////////

#define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR,12)



class CConnectionManager;
class CUDPSocket
{
public:
	CUDPSocket() {;}
	virtual ~CUDPSocket() {;}
	int Bind(CConnectionManager *pEH, const WORD wPort, const bool bBlocking, const bool bIPHdr);
	void RecvFromSocket();
	void SendToSocket();
#ifdef _WIN32
	SOCKET GetSocket() {return m_sock;}
#else
	int GetSocket() {return m_sock;}
#endif

private:
	void SetSockOpt(const bool bBlocking, const bool bIPHdr);

	void HandleRecv(SRecvStruct *pRS);
#ifdef _WIN32
	SOCKET m_sock;
#else
	int m_sock;
#endif
	sockaddr_in m_sAddr;
	CConnectionManager *m_pEH;
#if USE_BOOST == 1
	OnRecvCallBack m_funcOnRecv;
#else

#endif
};

#endif