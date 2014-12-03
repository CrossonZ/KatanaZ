#include "Fighter.h"

void CFighter::DoFight(list<CFighter *> &conMyTeamList, list<CFighter *> &conEnermyTeamList)
{
	BYTE byRandom = rand()%conEnermyTeamList.size();
	list<CFighter *>::iterator itList = conEnermyTeamList.begin();
	for (int i=0;i<byRandom;++i)
	{
		++itList;
	}
	DWORD dwHurt = (DWORD)DoHurt(*itList);
	if ((*itList)->m_dwHP > dwHurt)
	{
		(*itList)->m_dwHP -= dwHurt;
		printf("team:%d,random:%d,dwHurt:%d,Alive\n",m_byTeam, byRandom, dwHurt);
	}
	else
	{
		(*itList)->m_dwHP = 0;
		conEnermyTeamList.erase(itList);
		printf("team:%d,random:%d,dwHurt:%d,Dead\n",m_byTeam, byRandom, dwHurt);
	}

}

int CFighter::DoHurt(CFighter *pEnermy)
{
	return (int)(m_dwDC*(1-min(pEnermy->m_dwAC/10000,1)));
}