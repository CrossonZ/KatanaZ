#include "../net/ConnectionManager.h"
#include "../game/Arena.h"
#include <time.h>
int main()
{

	srand((unsigned int)time(NULL));
	CConnectionManager *pCM = gpConnectionManager->GetInstance();
	vector<WORD> conVec;
	WORD wPort = 60000;
	for (int i=0;i<4;++i, ++wPort)
	{
		conVec.push_back(wPort);
	}
	pCM->Init();
	
	pCM->Start(conVec);
	int iDone=0;
	//pCM->Connect("127.0.0.1", 60000);
	SSinglePacket *pSP = NULL;
	while (1)
	{
		Sleep(100);
	}/*
	//while (1)
	//{
		//while((pSP = gpConnectionManager->GetRecvPacket() )!= NULL)
		//{
		//	
		//	++iDone;
		//	//gpConnectionManager->DeallocSinglePacket(pSP);
		//	printf("iDone : %d\n",iDone);
		//}
	    CFighter *pFighter;
		bool bInsert = false;
		CArena *pArena = new CArena;
		list<CFighter *> conListA, conListB;
		list<CFighter *>::iterator itList;
		for (int i=0;i<5;++i)
		{
			bInsert = false;
			pFighter = new CFighter;
			pFighter->m_dwAC = 100 + rand() % 100;
			pFighter->m_dwDC = 3000 + rand() % 1000;
			pFighter->m_dwHP = 10000 + rand() % 10000;
			pFighter->m_wSpeed = (1+rand()%4) * 50;
			pFighter->m_fActionTime = ACTION_TIME_MAX / pFighter->m_wSpeed;
			pFighter->m_byTeam = 1;
			//for (itList = pArena->m_conTeamAList.begin();itList != pArena->m_conTeamAList.end();++itList)
			//{
			//	if (pFighter->m_fActionTime <= (*itList)->m_fActionTime)
			//	{
			//		pArena->m_conTeamAList.insert(itList,pFighter);
			//		break;
			//	}
			//}
			//if (itList == pArena->m_conTeamAList.end())
			//{
				conListA.push_back(pFighter);
			//}
			
		}
		for (int i=0;i<5;++i)
		{
			pFighter = new CFighter;
			pFighter->m_dwAC = 100 + rand() % 100;
			pFighter->m_dwDC = 3000 + rand() % 1000;
			pFighter->m_dwHP = 10000 + rand() % 10000;
			pFighter->m_wSpeed = (1+rand()%4) * 50;
			pFighter->m_fActionTime = ACTION_TIME_MAX / pFighter->m_wSpeed;
			pFighter->m_byTeam = 2;

			//for (itList = pArena->m_conTeamBList.begin();itList != pArena->m_conTeamBList.end();++itList)
			//{
			//	if (pFighter->m_fActionTime <= (*itList)->m_fActionTime)
			//	{
			//		pArena->m_conTeamBList.insert(itList,pFighter);
			//		break;
			//	}
			//}
			//if (itList == pArena->m_conTeamBList.end())
			//{
				conListB.push_back(pFighter);
			//}
		}

		pArena->Apply(conListA, conListB);
		int iRet = pArena->AutoGame();
		printf("result:%d\n",iRet);
		Sleep(2000);*/
		system("pause");
	return 0;
}
