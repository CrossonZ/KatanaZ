#ifndef __INCLUDED_PLAYER_H_
#define __INCLUDED_PLAYER_H_
class CReliabilityLayer;
class CPlayer
{
public:
	CPlayer(CReliabilityLayer *pRL);

private:
	CReliabilityLayer *m_pRL;
};

#endif