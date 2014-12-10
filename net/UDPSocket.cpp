#include "UDPSocket.h"
#include "ConnectionManager.h"

int CUDPSocket::Bind(CConnectionManager *pEH, const WORD wPort, const bool bBlocking, const bool bIPHdr)
{
	m_pEH = pEH;
	memset(&m_sAddr, 0, sizeof(sockaddr_in));
	m_sAddr.sin_family = AF_INET;
	m_sAddr.sin_port = htons(wPort);
	m_sAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	m_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (m_sock == -1)
		return -1;

	SetSockOpt(bBlocking, bIPHdr);
#if USE_BOOST == 1
	m_funcOnRecv = boost::bind(&CConnectionManager::OnRecv, m_pEH, _1);
#endif
#if TYPE_SERVER == 1
	return ::bind(m_sock, (sockaddr*)&m_sAddr, sizeof(m_sAddr));
#else
	return true;
#endif
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////Private Function///////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CUDPSocket::SetSockOpt(const bool bBlocking, const bool bIPHdr)
{
#ifdef _WIN32
	int sock_opt=1024*256;
	setsockopt(m_sock, SOL_SOCKET, SO_RCVBUF, (char *)&sock_opt, sizeof(sock_opt));
	
	// Immediate hard close. Don't linger the socket, or recreating the socket quickly on Vista fails.
	// Fail with voice and xbox

	sock_opt=0;
	setsockopt(m_sock, SOL_SOCKET, SO_LINGER, (char *)&sock_opt, sizeof (sock_opt));
	// Do not assert, ignore failure

	// This doesn't make much difference: 10% maybe
	// Not supported on console 2
	sock_opt=1024*16;
	setsockopt(m_sock, SOL_SOCKET, SO_SNDBUF, (char *)&sock_opt, sizeof(sock_opt));

	//Set nonblocking
	DWORD dwBlocking = bBlocking ? 1 : 0;
	ioctlsocket(m_sock, FIONBIO, &dwBlocking);

	//IPHeader include
	sock_opt = bIPHdr ? 1 : 0;
	setsockopt(m_sock, IPPROTO_IP, IP_HDRINCL, (char *)&sock_opt, sizeof(sock_opt));

	//下面这段是防止windows本身的bug，当sendto对方主机不能到达时，本机的读事件一直产生recvfrom返回SOCKET_ERROR，错误代码为10054,此解决方案，源于网络
	BOOL BNewBehavior = FALSE;
	DWORD dwBytesReturned = 0;
	WSAIoctl(m_sock, SIO_UDP_CONNRESET, &BNewBehavior, sizeof(BNewBehavior), NULL, 0, &dwBytesReturned, NULL, NULL);
#else
	//if (bBlocking
	int flags, s;  
	//得到文件状态标志  
	flags = fcntl (m_sock, F_GETFL, 0);  
	if (flags == -1)  
	{  
		perror ("fcntl");  
		return;  
	}  

	//设置文件状态标志  
	flags |= O_NONBLOCK;  
	s = fcntl (m_sock, F_SETFL, flags);  
	if (s == -1)  
	{  
		perror ("fcntl");  
		return;  
	}  
#endif
}

void CUDPSocket::HandleRecv(SRecvStruct *pRS)
{
#ifdef _WIN32
	int iLen = sizeof(pRS->sAddr);
#else
	socklen_t iLen = sizeof(pRS->sAddr);
#endif
	const int flag = 0;
	while (1)
	{
#ifdef FOR_TEST
		printf("recv one package\n");
#endif
#ifdef _WIN32
		pRS->iLen = recvfrom(m_sock, pRS->szBuf, sizeof(pRS->szBuf), flag, (sockaddr *)&pRS->sAddr, &iLen);

		if (pRS->iLen < 0 && WSAGetLastError() == WSAEWOULDBLOCK)
		{
			Sleep(1);
			continue;
		}
#else
		int iRet = recvfrom(m_sock, pRS->szBuf, sizeof(pRS->szBuf), flag, (sockaddr *)&pRS->sAddr, &iLen);
		if (iRet == -1)
		{
			printf("return -1, errno = %d\n", errno);
			if (errno == EAGAIN  || errno == EWOULDBLOCK)
			{
				Sleep(1);
				continue;
			}
		}
		else
		{
			printf("recvnum:%d\n",iRet);
		}
#endif
		break;
	}
}


void CUDPSocket::RecvFromSocket()
{
	while (1)
	{
		SRecvStruct *pRS = m_pEH->AllocRecvStruct();
		if (pRS != NULL)
		{
			pRS->iRefCount = 1;
			pRS->pSock=this;
			HandleRecv(pRS);

			if (pRS->iLen >= NETWORK_PKG_HEAD_LEN)
			{
#if USE_BOOST == 1
				if (!m_funcOnRecv(pRS))
#else
				if (!m_pEH->OnRecv(pRS))
#endif
				{
					printf("-------------FUCK------------\n");
					m_pEH->DeallocRecvStruct(pRS);
				}
			}
			else
			{
				Sleep(1);

				m_pEH->DeallocRecvStruct(pRS);
				//break;
			}
		}
		else
		{
			printf("--------------------FUCKING-----------------------------------\n");
			exit(-1);
		}
	}
}

void CUDPSocket::SendToSocket()
{
	int iSent = 0;
	SSendStruct *pSS = NULL;
	while (1)
	{
		if ((pSS=gpConnectionManager->PopSS()) == NULL)
		{
			Sleep(1);
			continue;
		}
		int iOffset = 0;
		printf("Prepare to send\n");
		while (1)
		{
			iSent = sendto(this->m_sock, pSS->pBuf+iOffset, pSS->iLen-iOffset, 0, (sockaddr*)&pSS->sAddr, sizeof(pSS->sAddr));
#ifdef _WIN32
			if (iSent == -1 && WSAGetLastError() == WSAEWOULDBLOCK)
			{
				Sleep(1);
				continue;
			}
#else 
			if (iSent == -1)
			{
				printf("send return -1, errno = %d\n", errno);
				if (errno == EAGAIN  || errno == EWOULDBLOCK)
				{
					Sleep(1);
					continue;
				}
			}
#endif
			else if ((iOffset += iSent) == pSS->iLen)
			{
				printf("send succuss len:%d\n", iOffset);
				break;
			}

		}
		if(pSS->eSPF == SPF_NO_ACK)
		{
			gpConnectionManager->DeallocSendStruct(pSS);
		}
	}
}