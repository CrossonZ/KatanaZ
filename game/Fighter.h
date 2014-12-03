#ifndef __INCLUDED_FIGHTER_H_
#define __INCLUDED_FIGHTER_H_
#include "../main/GlobalDefines.h"
class CCommonValue
{
public:
	union UValue
	{
		BYTE byValue;
		WORD wValue;
	};
};

#define ACTION_TIME_MAX 1000

class CFighter
{
public:
	CFighter() {}
	virtual ~CFighter() {}

	void DoFight(list<CFighter *> &conMyTeamList, list<CFighter *> &conEnermyTeamList);
	int  DoHurt(CFighter *pEnermy);
	WORD m_wSpeed;
	float m_fActionTime;
	DWORD m_dwHP;
	DWORD m_dwDC;
	DWORD m_dwAC;

	BYTE m_byTeam;
};

#endif