#ifndef __INCLUDED_ARENA_H_
#define __INCLUDED_ARENA_H_
#include "Fighter.h"

class CArena
{
public:

	bool Apply(list<CFighter *> &conTeamAList, list<CFighter*> &conTeamBList);

	int AutoGame();

	bool Action();

	class CompareActionTimeFunc
	{
	public:
		bool operator()(const CFighter *pFighterA, const CFighter *pFighterB)
		{
			return pFighterA->m_fActionTime < pFighterB->m_fActionTime;
		}
	};

	list<CFighter *> m_conTeamAList;
	list<CFighter *> m_conTeamBList;

	list<CFighter *> m_conGameQueueList;
	FILE *fp;
};

#endif