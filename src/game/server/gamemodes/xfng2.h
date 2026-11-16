/* (c) KeksTW. */
/* Modified by XX11org for DDNet 20+ */
#ifndef GAME_SERVER_GAMEMODES_XFNG2_H
#define GAME_SERVER_GAMEMODES_XFNG2_H

#include <game/server/gamecontroller.h>

class CGameControllerXFNG2 : public IGameController
{
public:
	CGameControllerXFNG2(class CGameContext *pGameServer);

	void Tick() override;
	void Snap(int SnappingClient) override;
	void OnCharacterSpawn(class CCharacter *pChr) override;
	int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon) override;

	void DoWincheck() override;
	void PostReset() override;

protected:
	void EndRound() override;

private:
	int m_aTeamscore[2] = {0, 0};
};

#endif
