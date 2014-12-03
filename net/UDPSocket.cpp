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
	m_funcOnRecv = boost::bind(&CConnectionManager::OnRecv, m_pEH, _1);
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
}

void CUDPSocket::HandleRecv(SRecvStruct *pRS)
{
	int iLen = sizeof(pRS->sAddr);
	const int flag = 0;
	while (1)
	{
		pRS->iLen = recvfrom(m_sock, pRS->szBuf, sizeof(pRS->szBuf), flag, (sockaddr *)&pRS->sAddr, &iLen);
		if (pRS->iLen < 0 && WSAGetLastError() == WSAEWOULDBLOCK)
		{
			Sleep(50);
			continue;
		}
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
				if (!m_funcOnRecv(pRS))
				{
					printf("-------------FUCK------------\n");
					m_pEH->DeallocRecvStruct(pRS);
				}
			}
			else
			{
				Sleep(50);

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
			Sleep(100);
			continue;
		}
		int iOffset = 0;
		while (1)
		{
			iSent = sendto(this->m_sock, pSS->pBuf+iOffset, pSS->iLen-iOffset, 0, (sockaddr*)&pSS->sAddr, sizeof(pSS->sAddr));
			if (iSent == -1 && WSAGetLastError() == WSAEWOULDBLOCK)
			{
				Sleep(10);
				continue;
			}
			else if ((iOffset += iSent) == pSS->iLen)
			{
				break;
			}
		}
		if(pSS->eSPF == SPF_NO_ACK)
		{
			gpConnectionManager->DeallocSendStruct(pSS);
		}
	}
}