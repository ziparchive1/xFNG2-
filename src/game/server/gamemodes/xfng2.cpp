/* (c) KeksTW */
/* Modified by XX11org for DDNet 20+ */
#include "xfng2.h"

#include "../entities/character.h"
#include "../xfng2define.h"
#include "../player.h"
#include "../repeekethami.h"

#include <engine/shared/config.h>

#include <cmath>
#include <cstdio>
#include <cstring>

CGameControllerXFNG2::CGameControllerXFNG2(class CGameContext *pGameServer) :
	IGameController(pGameServer)
{
	m_pGameType = "xfng2";
	m_GameFlags = GAMEFLAG_TEAMS;
	m_Warmup = g_Config.m_SvWarmup;
}

void CGameControllerXFNG2::Tick()
{
	IGameController::Tick();
	DoWincheck();
}

void CGameControllerXFNG2::DoWincheck()
{
	if(m_GameOverTick != -1 || m_Warmup || GameServer()->m_World.m_ResetRequested)
		return;

	if(IsTeamplay())
	{
		bool ScoreLimitReached = g_Config.m_SvScorelimit > 0 &&
			(m_aTeamscore[TEAM_RED] >= g_Config.m_SvScorelimit || m_aTeamscore[TEAM_BLUE] >= g_Config.m_SvScorelimit);
		bool TimeLimitReached = g_Config.m_SvTimelimit > 0 &&
			(Server()->Tick() - m_RoundStartTick) >= g_Config.m_SvTimelimit * Server()->TickSpeed() * 60;

		if(ScoreLimitReached || TimeLimitReached)
		{
			if(m_aTeamscore[TEAM_RED] != m_aTeamscore[TEAM_BLUE])
				EndRound();
			else
				m_SuddenDeath = 1;
		}
	}
	else
	{
		int Topscore = 0;
		int TopscoreCount = 0;
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(GameServer()->m_apPlayers[i])
			{
				int Score = GameServer()->m_apPlayers[i]->m_Score.value_or(0);
				if(Score > Topscore)
				{
					Topscore = Score;
					TopscoreCount = 1;
				}
				else if(Score == Topscore)
				{
					TopscoreCount++;
				}
			}
		}

		bool ScoreLimitReached = g_Config.m_SvScorelimit > 0 && Topscore >= g_Config.m_SvScorelimit;
		bool TimeLimitReached = g_Config.m_SvTimelimit > 0 &&
			(Server()->Tick() - m_RoundStartTick) >= g_Config.m_SvTimelimit * Server()->TickSpeed() * 60;

		if(ScoreLimitReached || TimeLimitReached)
		{
			if(TopscoreCount == 1)
				EndRound();
			else
				m_SuddenDeath = 1;
		}
	}
}

void CGameControllerXFNG2::Snap(int SnappingClient)
{
	IGameController::Snap(SnappingClient);

	CNetObj_GameData *pGameDataObj = Server()->SnapNewItem<CNetObj_GameData>(0);
	if(!pGameDataObj)
		return;

	pGameDataObj->m_TeamscoreRed = m_aTeamscore[TEAM_RED];
	pGameDataObj->m_TeamscoreBlue = m_aTeamscore[TEAM_BLUE];
	pGameDataObj->m_FlagCarrierRed = FLAG_ATSTAND;
	pGameDataObj->m_FlagCarrierBlue = FLAG_ATSTAND;
}

void CGameControllerXFNG2::OnCharacterSpawn(class CCharacter *pChr)
{
	pChr->IncreaseHealth(10);

	pChr->GiveWeapon(WEAPON_HAMMER, false, -1);
	pChr->GiveWeapon(WEAPON_LASER, false, -1);
}

int CGameControllerXFNG2::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon)
{
	if(!pKiller || Weapon == WEAPON_GAME)
		return 0;

	CPlayer *pVictimPlayer = pVictim->GetPlayer();
	if(!pVictimPlayer)
		return 0;

	if(pKiller == pVictimPlayer)
	{
		pVictimPlayer->m_Stats.m_Selfkills++;
	}
	else
	{
		if(Weapon == WEAPON_LASER || Weapon == WEAPON_GRENADE)
		{
			if(IsTeamplay() && pVictimPlayer->GetTeam() == pKiller->GetTeam())
			{
				pKiller->m_Stats.m_Teamkills++;
			}
			else
			{
				pKiller->m_Stats.m_Kills++;
				pVictimPlayer->m_Stats.m_Hits++;
				m_aTeamscore[pKiller->GetTeam()]++;
			}
		}
		else if(Weapon == WEAPON_SPIKE_NORMAL)
		{
			if(pKiller->GetCharacter())
				GameServer()->MakeLaserTextPoints(pKiller->GetCharacter()->m_Pos, pKiller->GetCID(), g_Config.m_SvPlayerScoreSpikeNormal);

			pKiller->m_Stats.m_GrabsNormal++;
			pVictimPlayer->m_Stats.m_Deaths++;
			m_aTeamscore[pKiller->GetTeam()] += g_Config.m_SvTeamScoreSpikeNormal;
			pVictimPlayer->m_RespawnTick = Server()->Tick() + Server()->TickSpeed() * 0.5f;
		}
		else if(Weapon == WEAPON_SPIKE_RED)
		{
			if(pKiller->GetTeam() == TEAM_RED || !IsTeamplay())
			{
				pKiller->m_Stats.m_GrabsTeam++;
				pVictimPlayer->m_Stats.m_Deaths++;
				m_aTeamscore[TEAM_RED] += g_Config.m_SvTeamScoreSpikeTeam;
				if(pKiller->GetCharacter())
					GameServer()->MakeLaserTextPoints(pKiller->GetCharacter()->m_Pos, pKiller->GetCID(), g_Config.m_SvPlayerScoreSpikeTeam);
			}
			else
			{
				pKiller->m_Stats.m_GrabsFalse++;
				m_aTeamscore[TEAM_BLUE] += g_Config.m_SvTeamScoreSpikeFalse;
				if(pKiller->GetCharacter())
					GameServer()->MakeLaserTextPoints(pKiller->GetCharacter()->m_Pos, pKiller->GetCID(), g_Config.m_SvPlayerScoreSpikeFalse);
			}
			pVictimPlayer->m_RespawnTick = Server()->Tick() + Server()->TickSpeed() * 0.5f;
		}
		else if(Weapon == WEAPON_SPIKE_BLUE)
		{
			if(pKiller->GetTeam() == TEAM_BLUE || !IsTeamplay())
			{
				pKiller->m_Stats.m_GrabsTeam++;
				pVictimPlayer->m_Stats.m_Deaths++;
				m_aTeamscore[TEAM_BLUE] += g_Config.m_SvTeamScoreSpikeTeam;
				if(pKiller->GetCharacter())
					GameServer()->MakeLaserTextPoints(pKiller->GetCharacter()->m_Pos, pKiller->GetCID(), g_Config.m_SvPlayerScoreSpikeTeam);
			}
			else
			{
				pKiller->m_Stats.m_GrabsFalse++;
				m_aTeamscore[TEAM_RED] += g_Config.m_SvTeamScoreSpikeFalse;
				if(pKiller->GetCharacter())
					GameServer()->MakeLaserTextPoints(pKiller->GetCharacter()->m_Pos, pKiller->GetCID(), g_Config.m_SvPlayerScoreSpikeFalse);
			}
			pVictimPlayer->m_RespawnTick = Server()->Tick() + Server()->TickSpeed() * 0.5f;
		}
		else if(Weapon == WEAPON_SPIKE_GREEN)
		{
			pKiller->m_Stats.m_GrabsGreen++;
			pVictimPlayer->m_Stats.m_Deaths++;
			m_aTeamscore[pKiller->GetTeam()] += g_Config.m_SvTeamScoreSpikeGreen;
			pVictimPlayer->m_RespawnTick = Server()->Tick() + Server()->TickSpeed() * 0.5f;
			if(pKiller->GetCharacter())
				GameServer()->MakeLaserTextPoints(pKiller->GetCharacter()->m_Pos, pKiller->GetCID(), g_Config.m_SvPlayerScoreSpikeGreen);
		}
		else if(Weapon == WEAPON_SPIKE_PURPLE)
		{
			pKiller->m_Stats.m_GrabsPurple++;
			pVictimPlayer->m_Stats.m_Deaths++;
			m_aTeamscore[pKiller->GetTeam()] += g_Config.m_SvTeamScoreSpikePurple;
			pVictimPlayer->m_RespawnTick = Server()->Tick() + Server()->TickSpeed() * 0.5f;
			if(pKiller->GetCharacter())
				GameServer()->MakeLaserTextPoints(pKiller->GetCharacter()->m_Pos, pKiller->GetCID(), g_Config.m_SvPlayerScoreSpikePurple);
		}
		else if(Weapon == WEAPON_SPIKE_GOLD)
		{
			pKiller->m_Stats.m_GrabsGold++;
			pVictimPlayer->m_Stats.m_Deaths++;
			m_aTeamscore[pKiller->GetTeam()] += g_Config.m_SvTeamScoreSpikeGold;
			pVictimPlayer->m_RespawnTick = Server()->Tick() + Server()->TickSpeed() * 0.5f;
			if(pKiller->GetCharacter())
				GameServer()->MakeLaserTextPoints(pKiller->GetCharacter()->m_Pos, pKiller->GetCID(), g_Config.m_SvPlayerScoreSpikeGold);
		}
		else if(Weapon == WEAPON_HAMMER)
		{
			pKiller->m_Stats.m_Unfreezes++;
		}
	}

	if(Weapon == WEAPON_SELF || Weapon == WEAPON_WORLD)
	{
		pVictimPlayer->m_RespawnTick = Server()->Tick() + Server()->TickSpeed() * 0.75f;
	}

	return 0;
}

void CGameControllerXFNG2::PostReset()
{
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(GameServer()->m_apPlayers[i])
		{
			GameServer()->m_apPlayers[i]->Respawn();
			GameServer()->m_apPlayers[i]->m_Score.reset();
			GameServer()->m_apPlayers[i]->ResetStats();
			GameServer()->m_apPlayers[i]->m_ScoreStartTick = Server()->Tick();
			GameServer()->m_apPlayers[i]->m_RespawnTick = Server()->Tick() + Server()->TickSpeed() / 2;
		}
	}
}

void CGameControllerXFNG2::EndRound()
{
	IGameController::EndRound();
	GameServer()->SendRoundStats();
}
