#include "Arena.h"


//bool CArena::CompareActionTime(const CFighter &pFighterA, const CFighter &pFighterB)
//{
//	
//}

bool CArena::Apply(list<CFighter *> &conTeamAList, list<CFighter*> &conTeamBList)
{
	fp = fopen("log.txt", "a+");
	if (conTeamAList.empty() || conTeamBList.empty())
	{
		return false;
	}
	m_conTeamAList = conTeamAList;
	m_conTeamBList = conTeamBList;
	m_conGameQueueList.insert(m_conGameQueueList.end(), conTeamAList.begin(), conTeamAList.end());
	m_conGameQueueList.insert(m_conGameQueueList.end(), conTeamBList.begin(), conTeamBList.end());
	m_conGameQueueList.sort(CompareActionTimeFunc());
	return true;
}

int CArena::AutoGame()
{
	int iRound = 0;
	if (m_conTeamAList.empty() || m_conTeamBList.empty())
	{
		return 0;
	}

	list<CFighter *>::iterator itListA = m_conTeamAList.begin();
	list<CFighter *>::iterator itListB = m_conTeamBList.begin();

	/*for (;itListA != m_conTeamAList.end();++itListA)
	{
		if ((*itListA)->m_fActionTime <= (*itListB)->m_fActionTime)
		{
			m_conGameQueueList.push_back(*itListA);
			printf("Team1:DC=%d,AC=%d,Speed=%d,HP=%d\n", (*itListA)->m_dwDC, (*itListA)->m_dwAC, (*itListA)->m_wSpeed, (*itListA)->m_dwHP);
		}
		else
		{
			for (;itListB!=m_conTeamBList.end();++itListB)
			{
				if ((*itListA)->m_fActionTime > (*itListB)->m_fActionTime)
				{
					m_conGameQueueList.push_back(*itListB);
					printf("Team2:DC=%d,AC=%d,Speed=%d,HP=%d\n", (*itListB)->m_dwDC, (*itListB)->m_dwAC, (*itListB)->m_wSpeed, (*itListB)->m_dwHP);
				}
				else
				{
					m_conGameQueueList.push_back(*itListA);
					printf("Team1:DC=%d,AC=%d,Speed=%d,HP=%d\n", (*itListA)->m_dwDC, (*itListA)->m_dwAC, (*itListA)->m_wSpeed, (*itListA)->m_dwHP);
					break;
				}
			}
			if (itListB == m_conTeamBList.end())
			{
				break;
			}
		}
	}
	for (;itListA != m_conTeamAList.end();++itListA)
	{
		m_conGameQueueList.push_back(*itListA);
		printf("Team1:DC=%d,AC=%d,Speed=%d,HP=%d\n", (*itListA)->m_dwDC, (*itListA)->m_dwAC, (*itListA)->m_wSpeed, (*itListA)->m_dwHP);
	}
	for (;itListB!=m_conTeamBList.end();++itListB)
	{
		m_conGameQueueList.push_back(*itListB);
		printf("Team2:DC=%d,AC=%d,Speed=%d,HP=%d\n", (*itListB)->m_dwDC, (*itListB)->m_dwAC, (*itListB)->m_wSpeed, (*itListB)->m_dwHP);
	}*/

	while(!m_conTeamAList.empty() && !m_conTeamBList.empty())
	{
		if (!Action())
		{
			break;
		}
		iRound++;
	}
	for (itListA=m_conTeamAList.begin();itListA != m_conTeamAList.end();++itListA)
	{
		printf("Team1:DC=%d,AC=%d,Speed=%d,HP=%d\n", (*itListA)->m_dwDC, (*itListA)->m_dwAC, (*itListA)->m_wSpeed, (*itListA)->m_dwHP);
	}
	for (itListB =m_conTeamBList.begin();itListB != m_conTeamBList.end();++itListB)
	{
		printf("Team2:DC=%d,AC=%d,Speed=%d,HP=%d\n", (*itListB)->m_dwDC, (*itListB)->m_dwAC, (*itListB)->m_wSpeed, (*itListB)->m_dwHP);
	}
	fclose(fp);
	return m_conTeamAList.empty() ? 2 : 1;

}

bool CArena::Action()
{
	list<CFighter *>::iterator itListQueue = m_conGameQueueList.begin();
	float fActionTime = (*itListQueue)->m_fActionTime;
	CFighter *pFighter = *itListQueue;
	if (pFighter->m_byTeam == 1)
	{
		pFighter->DoFight(m_conTeamAList, m_conTeamBList);
	}
	else
	{
		pFighter->DoFight(m_conTeamBList, m_conTeamAList);
	}
	bool bInsert = false, bNeedInsert = pFighter->m_dwHP != 0;

	itListQueue = m_conGameQueueList.erase(itListQueue);
	//itListQueue = m_conGameQueueList.begin();
	pFighter->m_fActionTime = ACTION_TIME_MAX / ((float)pFighter->m_wSpeed);
	while (itListQueue != m_conGameQueueList.end())
	{
		if ((*itListQueue)->m_dwHP == 0)
		{
			itListQueue = m_conGameQueueList.erase(itListQueue);
		}
		else
		{
			(*itListQueue)->m_fActionTime = (*itListQueue)->m_fActionTime > fActionTime ? (*itListQueue)->m_fActionTime - fActionTime : 0;
			if (!bInsert&&bNeedInsert && pFighter->m_fActionTime < (*itListQueue)->m_fActionTime)
			{
				m_conGameQueueList.insert(itListQueue, pFighter);
				bInsert = true;
			}
			++itListQueue;
		}
	}
	if (!bInsert && bNeedInsert)
	{
		m_conGameQueueList.push_back(pFighter);
	} 


	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	char szBuf[4096];
	int iOffset = 0;
	for (itListQueue = m_conGameQueueList.begin();itListQueue != m_conGameQueueList.end();++itListQueue)
	{
		iOffset += _snprintf(szBuf+iOffset, sizeof(szBuf)-iOffset, "ActionTime:%f", (*itListQueue)->m_fActionTime);
		//printf(" ActionTime :%f", (*itListQueue)->m_fActionTime);
	}
	szBuf[iOffset] = '\n';
	iOffset++;
	fwrite(szBuf, 1, iOffset, fp);
	//printf("\n");
	return true;
}
