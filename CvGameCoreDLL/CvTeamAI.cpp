// teamAI.cpp

#include "CvGameCoreDLL.h"
#include "CvTeamAI.h"
#include "CvPlayerAI.h"
#include "CvRandom.h"
#include "CvGlobals.h"
#include "CvGameCoreUtils.h"
#include "CvMap.h"
#include "CvPlot.h"
#include "CvDLLInterfaceIFaceBase.h"
#include "CvGameAI.h"
#include "CvInfos.h"
#include "FProfiler.h"
#include "CyArgsList.h"
#include "CvDLLPythonIFaceBase.h"

#include "BetterBTSAI.h" // bbai
#include "CvDLLFAStarIFaceBase.h" // K-Mod (currently used in AI_isLandTarget)
#include <numeric> // K-Mod. used in AI_warSpoilsValue

// statics

CvTeamAI* CvTeamAI::m_aTeams = NULL;

void CvTeamAI::initStatics()
{
	m_aTeams = new CvTeamAI[MAX_TEAMS];
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		m_aTeams[iI].m_eID = ((TeamTypes)iI);
	}
}

void CvTeamAI::freeStatics()
{
	SAFE_DELETE_ARRAY(m_aTeams);
}

// inlined for performance reasons
DllExport CvTeamAI& CvTeamAI::getTeamNonInl(TeamTypes eTeam)
{
	return getTeam(eTeam);
}


// Public Functions...

CvTeamAI::CvTeamAI()
{
	m_aiWarPlanStateCounter = new int[MAX_TEAMS];
	m_aiAtWarCounter = new int[MAX_TEAMS];
	m_aiAtPeaceCounter = new int[MAX_TEAMS];
	m_aiHasMetCounter = new int[MAX_TEAMS];
	m_aiOpenBordersCounter = new int[MAX_TEAMS];
	m_aiDefensivePactCounter = new int[MAX_TEAMS];
	m_aiShareWarCounter = new int[MAX_TEAMS];
	m_aiWarSuccess = new int[MAX_TEAMS];
	m_aiEnemyPeacetimeTradeValue = new int[MAX_TEAMS];
	m_aiEnemyPeacetimeGrantValue = new int[MAX_TEAMS];
	m_aeWarPlan = new WarPlanTypes[MAX_TEAMS];


	AI_reset(true);
}


CvTeamAI::~CvTeamAI()
{
	AI_uninit();

	SAFE_DELETE_ARRAY(m_aiWarPlanStateCounter);
	SAFE_DELETE_ARRAY(m_aiAtWarCounter);
	SAFE_DELETE_ARRAY(m_aiAtPeaceCounter);
	SAFE_DELETE_ARRAY(m_aiHasMetCounter);
	SAFE_DELETE_ARRAY(m_aiOpenBordersCounter);
	SAFE_DELETE_ARRAY(m_aiDefensivePactCounter);
	SAFE_DELETE_ARRAY(m_aiShareWarCounter);
	SAFE_DELETE_ARRAY(m_aiWarSuccess);
	SAFE_DELETE_ARRAY(m_aiEnemyPeacetimeTradeValue);
	SAFE_DELETE_ARRAY(m_aiEnemyPeacetimeGrantValue);
	SAFE_DELETE_ARRAY(m_aeWarPlan);
}


void CvTeamAI::AI_init()
{
	AI_reset(false);

	//--------------------------------
	// Init other game data

}

// K-Mod
void CvTeamAI::AI_initMemory()
{
	// Note. this needs to be done after the map is set. unfortunately, AI_init is called before that happens.
	FAssert(GC.getMapINLINE().numPlotsINLINE() > 0);
	m_aiStrengthMemory.clear();
	m_aiStrengthMemory.resize(GC.getMapINLINE().numPlotsINLINE(), 0);
}
// K-Mod end


void CvTeamAI::AI_uninit()
{
	//m_aiStrengthMemory.clear(); // K-Mod. Clearing the memory will cause problems if the game is still in progress.
}


void CvTeamAI::AI_reset(bool bConstructor)
{
	AI_uninit();

	m_eWorstEnemy = NO_TEAM;

	for (int iI = 0; iI < MAX_TEAMS; iI++)
	{
		m_aiWarPlanStateCounter[iI] = 0;
		m_aiAtWarCounter[iI] = 0;
		m_aiAtPeaceCounter[iI] = 0;
		m_aiHasMetCounter[iI] = 0;
		m_aiOpenBordersCounter[iI] = 0;
		m_aiDefensivePactCounter[iI] = 0;
		m_aiShareWarCounter[iI] = 0;
		m_aiWarSuccess[iI] = 0;
		m_aiEnemyPeacetimeTradeValue[iI] = 0;
		m_aiEnemyPeacetimeGrantValue[iI] = 0;
		m_aeWarPlan[iI] = NO_WARPLAN;

		if (!bConstructor && getID() != NO_TEAM)
		{
			TeamTypes eLoopTeam = (TeamTypes) iI;
			CvTeamAI& kLoopTeam = GET_TEAM(eLoopTeam);
			kLoopTeam.m_aiWarPlanStateCounter[getID()] = 0;
			kLoopTeam.m_aiAtWarCounter[getID()] = 0;
			kLoopTeam.m_aiAtPeaceCounter[getID()] = 0;
			kLoopTeam.m_aiHasMetCounter[getID()] = 0;
			kLoopTeam.m_aiOpenBordersCounter[getID()] = 0;
			kLoopTeam.m_aiDefensivePactCounter[getID()] = 0;
			kLoopTeam.m_aiShareWarCounter[getID()] = 0;
			kLoopTeam.m_aiWarSuccess[getID()] = 0;
			kLoopTeam.m_aiEnemyPeacetimeTradeValue[getID()] = 0;
			kLoopTeam.m_aiEnemyPeacetimeGrantValue[getID()] = 0;
			kLoopTeam.m_aeWarPlan[getID()] = NO_WARPLAN;
		}
	}
}


void CvTeamAI::AI_doTurnPre()
{
	AI_doCounter();

	if (isHuman())
	{
		return;
	}

	if (isBarbarian())
	{
		return;
	}

	if (isMinorCiv())
	{
		return;
	}
}


void CvTeamAI::AI_doTurnPost()
{
	// K-Mod
	AI_updateStrengthMemory();

	// update the attitude cache for all team members.
	// (Note: attitude use to be updated near the start of CvGame::doTurn. I've moved it here for various reasons.)
	for (PlayerTypes i = (PlayerTypes)0; i < MAX_PLAYERS; i=(PlayerTypes)(i+1))
	{
		CvPlayerAI& kLoopPlayer = GET_PLAYER(i);
		if (kLoopPlayer.getTeam() == getID() && kLoopPlayer.isAlive())
		{
			GET_PLAYER(i).AI_updateCloseBorderAttitudeCache();
			GET_PLAYER(i).AI_updateAttitudeCache();
		}
	}
	// K-Mod end

	AI_updateWorstEnemy();

	AI_updateAreaStragies(false);

	/* if (isHuman())
	{
		return;
	}

	if (isBarbarian())
	{
		return;
	}

	if (isMinorCiv())
	{
		return;
	} */ // disabled by K-Mod. There are some basic things inside AI_doWar which are important for all players.

	AI_doWar();
}


void CvTeamAI::AI_makeAssignWorkDirty()
{
	int iI;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				GET_PLAYER((PlayerTypes)iI).AI_makeAssignWorkDirty();
			}
		}
	}
}

void CvTeamAI::AI_updateAreaStragies(bool bTargets)
{
	PROFILE_FUNC();

	CvArea* pLoopArea;
	int iLoop;

	if (!(GC.getGameINLINE().isFinalInitialized()))
	{
		return;
	}

	for(pLoopArea = GC.getMapINLINE().firstArea(&iLoop); pLoopArea != NULL; pLoopArea = GC.getMapINLINE().nextArea(&iLoop))
	{
		pLoopArea->setAreaAIType(getID(), AI_calculateAreaAIType(pLoopArea));
	}

	if (bTargets)
	{
		AI_updateAreaTargets();
	}
}


void CvTeamAI::AI_updateAreaTargets()
{
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		CvPlayerAI& kLoopPlayer = GET_PLAYER((PlayerTypes)iI); // K-Mod
		if (kLoopPlayer.isAlive())
		{
			if (kLoopPlayer.getTeam() == getID() && kLoopPlayer.AI_getCityTargetTimer() == 0) // K-Mod added timer check.
			{
				kLoopPlayer.AI_updateAreaTargets();
			}
		}
	}
}


int CvTeamAI::AI_countFinancialTrouble() const
{
	int iCount;
	int iI;

	iCount = 0;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				if (GET_PLAYER((PlayerTypes)iI).AI_isFinancialTrouble())
				{
					iCount++;
				}
			}
		}
	}

	return iCount;
}


int CvTeamAI::AI_countMilitaryWeight(CvArea* pArea) const
{
	int iCount;
	int iI;

	iCount = 0;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				iCount += GET_PLAYER((PlayerTypes)iI).AI_militaryWeight(pArea);
			}
		}
	}

	return iCount;
}

// K-Mod. return the total yield of the team, estimated by averaging over the last few turns of the yield's history graph.
int CvTeamAI::AI_estimateTotalYieldRate(YieldTypes eYield) const
{
	PROFILE_FUNC();
	const int iSampleSize = 5;
	// number of turns to use in weighted average.
	// Ignore turns with 0 production, because they are probably a revolt. Bias towards most recent turns.
	const int iTurn = GC.getGameINLINE().getGameTurn();

	int iTotal = 0;
	for (PlayerTypes eLoopPlayer = (PlayerTypes)0; eLoopPlayer < MAX_PLAYERS; eLoopPlayer=(PlayerTypes)(eLoopPlayer+1))
	{
		const CvPlayerAI& kLoopPlayer = GET_PLAYER(eLoopPlayer);
		if (kLoopPlayer.getTeam() == getID() && kLoopPlayer.isAlive())
		{
			int iSubTotal = 0;
			int iBase = 0;
			for (int i = 0; i < iSampleSize; i++)
			{
				int p = 0;
				switch (eYield)
				{
				case YIELD_PRODUCTION:
					p = kLoopPlayer.getIndustryHistory(iTurn - (i+1));
					break;
				case YIELD_COMMERCE:
					p = kLoopPlayer.getEconomyHistory(iTurn - (i+1));
					break;
				case YIELD_FOOD:
					p = kLoopPlayer.getAgricultureHistory(iTurn - (i+1));
					break;
				default:
					FAssertMsg(false, "invalid yield type in AI_estimateTotalYieldRate");
					break;
				}
				if (p > 0)
				{
					iSubTotal += (iSampleSize - i) * p;
					iBase += iSampleSize - i;
				}
			}
			iTotal += iSubTotal / std::max(1, iBase);
		}
	}
	return iTotal;
}
// K-Mod end

// K-Mod. return true if is fair enough for the AI to know there is a city here
bool CvTeamAI::AI_deduceCitySite(const CvCity* pCity) const
{
	PROFILE_FUNC();

	if (pCity->isRevealed(getID(), false))
		return true;

	// The rule is this:
	// if we can see more than n plots of the nth culture ring, we can deduce where the city is.

	int iPoints = 0;
	int iLevel = pCity->getCultureLevel();

	for (int iDX = -iLevel; iDX <= iLevel; iDX++)
	{
		for (int iDY = -iLevel; iDY <= iLevel; iDY++)
		{
			int iDist = pCity->cultureDistance(iDX, iDY);
			if (iDist > iLevel)
				continue;

			CvPlot* pLoopPlot = plotXY(pCity->getX_INLINE(), pCity->getY_INLINE(), iDX, iDY);

			if (pLoopPlot && pLoopPlot->getRevealedOwner(getID(), false) == pCity->getOwnerINLINE())
			{
				// if multiple cities have their plot in their range, then that will make it harder to deduce the precise city location.
				iPoints += 1 + std::max(0, iLevel - iDist - pLoopPlot->getNumCultureRangeCities(pCity->getOwnerINLINE())+1);

				if (iPoints > iLevel)
					return true;
			}
		}
	}
	return false;
}
// K-Mod end

bool CvTeamAI::AI_isAnyCapitalAreaAlone() const
{
	int iI;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				if (GET_PLAYER((PlayerTypes)iI).AI_isCapitalAreaAlone())
				{
					return true;
				}
			}
		}
	}

	return false;
}


bool CvTeamAI::AI_isPrimaryArea(CvArea* pArea) const
{
	int iI;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				if (GET_PLAYER((PlayerTypes)iI).AI_isPrimaryArea(pArea))
				{
					return true;
				}
			}
		}
	}

	return false;
}


bool CvTeamAI::AI_hasCitiesInPrimaryArea(TeamTypes eTeam) const
{
	CvArea* pLoopArea;
	int iLoop;

	FAssertMsg(eTeam != getID(), "shouldn't call this function on ourselves");

	for(pLoopArea = GC.getMapINLINE().firstArea(&iLoop); pLoopArea != NULL; pLoopArea = GC.getMapINLINE().nextArea(&iLoop))
	{
		if (AI_isPrimaryArea(pLoopArea))
		{
			if (GET_TEAM(eTeam).countNumCitiesByArea(pLoopArea))
			{
				return true;
			}
		}
	}

	return false;
}

// K-Mod. Return true if this team and eTeam have at least one primary area in common.
bool CvTeamAI::AI_hasSharedPrimaryArea(TeamTypes eTeam) const
{
	FAssert(eTeam != getID());

	const CvTeamAI& kTeam = GET_TEAM(eTeam);

	int iLoop;
	for(CvArea* pLoopArea = GC.getMapINLINE().firstArea(&iLoop); pLoopArea != NULL; pLoopArea = GC.getMapINLINE().nextArea(&iLoop))
	{
		if (AI_isPrimaryArea(pLoopArea) && kTeam.AI_isPrimaryArea(pLoopArea))
			return true;
	}
	return false;
}
// K-Mod end

AreaAITypes CvTeamAI::AI_calculateAreaAIType(CvArea* pArea, bool bPreparingTotal) const
{
	PROFILE_FUNC();

	// K-Mod. This function originally had "!isWater()" wrapping all of the code.
	// I've changed it to be more readable.
	if (pArea->isWater())
	{
		return AREAAI_NEUTRAL;
	}

	if (isBarbarian())
	{
		if ((pArea->getNumCities() - pArea->getCitiesPerPlayer(BARBARIAN_PLAYER)) == 0)
		{
			return AREAAI_ASSAULT;
		}

		if ((countNumAIUnitsByArea(pArea, UNITAI_ATTACK) + countNumAIUnitsByArea(pArea, UNITAI_ATTACK_CITY) + countNumAIUnitsByArea(pArea, UNITAI_PILLAGE) + countNumAIUnitsByArea(pArea, UNITAI_ATTACK_AIR)) > (((AI_countMilitaryWeight(pArea) * 20) / 100) + 1))
		{
			return AREAAI_OFFENSIVE; // XXX does this ever happen?
		}

		return AREAAI_MASSING;
	}

	bool bRecentAttack = false;
	bool bTargets = false;
	bool bChosenTargets = false;
	bool bDeclaredTargets = false;

	bool bAssault = false;
	bool bPreparingAssault = false;

	// int iOffensiveThreshold = (bPreparingTotal ? 25 : 20); // K-Mod, I don't use this.
	int iAreaCities = countNumCitiesByArea(pArea);
	int iWarSuccessRating = AI_getWarSuccessRating(); // K-Mod

	for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
	{
		if (GET_TEAM((TeamTypes)iI).isAlive())
		{
			if (AI_getWarPlan((TeamTypes)iI) != NO_WARPLAN)
			{
				FAssert(((TeamTypes)iI) != getID());
				FAssert(isHasMet((TeamTypes)iI) || GC.getGameINLINE().isOption(GAMEOPTION_ALWAYS_WAR));

				if (AI_getWarPlan((TeamTypes)iI) == WARPLAN_ATTACKED_RECENT)
				{
					FAssert(isAtWar((TeamTypes)iI));
					bRecentAttack = true;
				}

				if ((GET_TEAM((TeamTypes)iI).countNumCitiesByArea(pArea) > 0) || (GET_TEAM((TeamTypes)iI).countNumUnitsByArea(pArea) > 4))
				{
					bTargets = true;

					if (AI_isChosenWar((TeamTypes)iI))
					{
						bChosenTargets = true;

						if ((isAtWar((TeamTypes)iI)) ? (AI_getAtWarCounter((TeamTypes)iI) < 10) : AI_isSneakAttackReady((TeamTypes)iI))
						{
							bDeclaredTargets = true;
						}
					}
				}
				else
				{
                    bAssault = true;
                    if (AI_isSneakAttackPreparing((TeamTypes)iI))
                    {
                        bPreparingAssault = true;
                    }
				}
			}
		}
	}
    
	// K-Mod - based on idea from BBAI
	if( bTargets )
	{
		if(iAreaCities > 0 && getAtWarCount(true) > 0) 
		{
			int iPower = countPowerByArea(pArea);
			int iEnemyPower = countEnemyPowerByArea(pArea);
			
			iPower *= 100 + iWarSuccessRating + (bChosenTargets ? 100 : 50);
			iEnemyPower *= 100;
			// it would be nice to put some personality modifiers into this. But this is a Team function. :(
			if (iPower < iEnemyPower)
			{
				return AREAAI_DEFENSIVE;
			}
		}
	}
	// K-Mod end

	if (bDeclaredTargets)
	{
		return AREAAI_OFFENSIVE;
	}

	if (bTargets)
	{
		/* BBAI code. -- This code has two major problems.
		* Firstly, it makes offense more likely when we are in more wars.
		* Secondly, it chooses offense based on how many offense units we have -- but offense units are built for offense areas!
		*
		// AI_countMilitaryWeight is based on this team's pop and cities ... if this team is the biggest, it will over estimate needed units
		int iMilitaryWeight = AI_countMilitaryWeight(pArea);
		int iCount = 1;

		for( int iJ = 0; iJ < MAX_CIV_TEAMS; iJ++ )
		{
			if( iJ != getID() && GET_TEAM((TeamTypes)iJ).isAlive() )
			{
				if( !(GET_TEAM((TeamTypes)iJ).isBarbarian() || GET_TEAM((TeamTypes)iJ).isMinorCiv()) )
				{
					if( AI_getWarPlan((TeamTypes)iJ) != NO_WARPLAN )
					{
						iMilitaryWeight += GET_TEAM((TeamTypes)iJ).AI_countMilitaryWeight(pArea);
						iCount++;

						if( GET_TEAM((TeamTypes)iJ).isAVassal() )
						{
							for( int iK = 0; iK < MAX_CIV_TEAMS; iK++ )
							{
								if( iK != getID() && GET_TEAM((TeamTypes)iK).isAlive() )
								{
									if( GET_TEAM((TeamTypes)iJ).isVassal((TeamTypes)iK) )
									{
										iMilitaryWeight += GET_TEAM((TeamTypes)iK).AI_countMilitaryWeight(pArea);
									}
								}
							}
						}
					}
				}
			}
		}

		iMilitaryWeight /= iCount;
		if ((countNumAIUnitsByArea(pArea, UNITAI_ATTACK) + countNumAIUnitsByArea(pArea, UNITAI_ATTACK_CITY) + countNumAIUnitsByArea(pArea, UNITAI_PILLAGE) + countNumAIUnitsByArea(pArea, UNITAI_ATTACK_AIR)) > (((iMilitaryWeight * iOffensiveThreshold) / 100) + 1))
		{
			return AREAAI_OFFENSIVE;
		}
		*/
		// K-Mod. I'm not sure how best to do this yet. Let me just try a rough idea for now.
		// I'm using AI_countMilitaryWeight; but what I really want is "border terriory which needs defending"
		int iOurRelativeStrength = 100 * countPowerByArea(pArea) / (AI_countMilitaryWeight(pArea) + 20);
		iOurRelativeStrength *= 100 + (bDeclaredTargets ? 30 : 0) + (bPreparingTotal ? -20 : 0) + iWarSuccessRating/2;
		iOurRelativeStrength /= 100;
		int iEnemyRelativeStrength = 0;
		bool bEnemyCities = false;

		for (int iJ = 0; iJ < MAX_CIV_TEAMS; iJ++)
		{
			const CvTeamAI& kLoopTeam = GET_TEAM((TeamTypes)iJ);
			if (iJ != getID() && kLoopTeam.isAlive() && AI_getWarPlan((TeamTypes)iJ) != NO_WARPLAN)
			{
				int iPower = 100 * kLoopTeam.countPowerByArea(pArea);
				int iCommitment = (bPreparingTotal ? 30 : 20) + kLoopTeam.AI_countMilitaryWeight(pArea) * ((isAtWar((TeamTypes)iJ) ? 1 : 2) + kLoopTeam.getAtWarCount(true, true)) / 2;
				iPower /= iCommitment;
				iEnemyRelativeStrength += iPower;
				if (kLoopTeam.countNumCitiesByArea(pArea) > 0)
					bEnemyCities = true;
			}
		}
		if (bEnemyCities && iOurRelativeStrength > iEnemyRelativeStrength)
			return AREAAI_OFFENSIVE;
		// K-Mod end
	}

	if (bTargets)
	{
		for (int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; iPlayer++)
		{
			CvPlayerAI& kPlayer = GET_PLAYER((PlayerTypes)iPlayer);
			
			if (kPlayer.isAlive())
			{
				if (kPlayer.getTeam() == getID())
				{
					if (kPlayer.AI_isDoStrategy(AI_STRATEGY_DAGGER) || kPlayer.AI_isDoStrategy(AI_STRATEGY_FINAL_WAR))
					{
						if (pArea->getCitiesPerPlayer((PlayerTypes)iPlayer) > 0)
						{
							return AREAAI_MASSING;
						}
					}
				}
			}
		}
		if (bRecentAttack)
		{
			int iPower = countPowerByArea(pArea);
			int iEnemyPower = countEnemyPowerByArea(pArea);
			if (iPower > iEnemyPower)
			{
				return AREAAI_MASSING;
			}
			return AREAAI_DEFENSIVE;
		}
	}

	if (iAreaCities > 0)
	{
		if (countEnemyDangerByArea(pArea) > iAreaCities)
		{
			return AREAAI_DEFENSIVE;
		}
	}

	if (bChosenTargets)
	{
		return AREAAI_MASSING;
	}

	if (bTargets)
	{
		if (iAreaCities > (getNumMembers() * 3))
		{
			if (GC.getGameINLINE().isOption(GAMEOPTION_AGGRESSIVE_AI) || GC.getGameINLINE().isOption(GAMEOPTION_ALWAYS_WAR) || (countPowerByArea(pArea) > ((countEnemyPowerByArea(pArea) * 3) / 2)))
			{
				return AREAAI_MASSING;
			}
		}
		return AREAAI_DEFENSIVE;
	}
	else
	{
		if (bAssault)
		{
			if (AI_isPrimaryArea(pArea))
			{
                if (bPreparingAssault)
				{
					return AREAAI_ASSAULT_MASSING;
				}
			}
			else if (countNumCitiesByArea(pArea) > 0)
			{
				return AREAAI_ASSAULT_ASSIST;
			}

			return AREAAI_ASSAULT;
		}
	}
	return AREAAI_NEUTRAL;
}

int CvTeamAI::AI_calculateAdjacentLandPlots(TeamTypes eTeam) const
{
	PROFILE_FUNC();

	CvPlot* pLoopPlot;
	int iCount;
	int iI;

	FAssertMsg(eTeam != getID(), "shouldn't call this function on ourselves");

	iCount = 0;

	for (iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
	{
		pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);

		if (!(pLoopPlot->isWater()))
		{
			if ((pLoopPlot->getTeam() == eTeam) && pLoopPlot->isAdjacentTeam(getID(), true))
			{
				iCount++;
			}
		}
	}

	return iCount;
}


int CvTeamAI::AI_calculatePlotWarValue(TeamTypes eTeam) const
{
	FAssert(eTeam != getID());

	int iValue = 0;

	for (int iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
	{
		CvPlot* pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);

		if (pLoopPlot->getTeam() == eTeam)
		{
			if (!pLoopPlot->isWater() && pLoopPlot->isAdjacentTeam(getID(), true))
			{
				iValue += 4;
			}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      06/17/08                                jdog5000      */
/*                                                                                              */
/* Notes                                                                                        */
/************************************************************************************************/
			// This section of code does nothing without XML modding as AIObjective is 0 for all bonuses
			// Left alone for mods to use
			// Resource driven war in BBAI is done with CvTeamAI::AI_calculateBonusWarValue
			// without using the XML variable AIObjective
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
			BonusTypes eBonus = pLoopPlot->getBonusType(getID());
			if (NO_BONUS != eBonus)
			{
				iValue += 40 * GC.getBonusInfo(eBonus).getAIObjective();
			}
		}
	}

	return iValue;
}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      07/21/08                                jdog5000      */
/*                                                                                              */
/* War Strategy AI                                                                              */
/************************************************************************************************/
int CvTeamAI::AI_calculateBonusWarValue(TeamTypes eTeam) const
{
	FAssert(eTeam != getID());

	int iValue = 0;

	for (int iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
	{
		CvPlot* pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);

		if (pLoopPlot->getTeam() == eTeam)
		{
			BonusTypes eNonObsoleteBonus = pLoopPlot->getNonObsoleteBonusType(getID());
			if (NO_BONUS != eNonObsoleteBonus)
			{
				int iThisValue = 0;
				for( int iJ = 0; iJ < MAX_CIV_PLAYERS; iJ++ )
				{
					if( getID() == GET_PLAYER((PlayerTypes)iJ).getTeam() && GET_PLAYER((PlayerTypes)iJ).isAlive() )
					{
						// 10 seems like a typical value for a health/happiness resource the AI doesn't have
						// Values for strategic resources can be 60 or higher
						iThisValue += GET_PLAYER((PlayerTypes)iJ).AI_bonusVal(eNonObsoleteBonus, 1, true);
					}
				}
				iThisValue /= getAliveCount();

				if (!pLoopPlot->isWater())
				{
					if( pLoopPlot->isAdjacentTeam(getID(), true))
					{
						iThisValue *= 3;
					}
					else
					{
						CvCity* pWorkingCity = pLoopPlot->getWorkingCity();
						if( pWorkingCity != NULL )
						{
							for( int iJ = 0; iJ < MAX_CIV_PLAYERS; iJ++ )
							{
								if( getID() == GET_PLAYER((PlayerTypes)iJ).getTeam() && GET_PLAYER((PlayerTypes)iJ).isAlive() )
								{
									if( pWorkingCity->AI_playerCloseness((PlayerTypes)iJ ) > 0 )
									{
										iThisValue *= 2;
										break;
									}
								}
							}
						}
					}
				}

				iThisValue = std::max(0, iThisValue - 4);
				iThisValue /= 5;

				iValue += iThisValue;
			}
		}
	}

	return iValue;
}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

int CvTeamAI::AI_calculateCapitalProximity(TeamTypes eTeam) const
{
	CvCity* pOurCapitalCity;
	CvCity* pTheirCapitalCity;
	int iTotalDistance;
	int iCount;
	int iI, iJ;

	FAssertMsg(eTeam != getID(), "shouldn't call this function on ourselves");

	iTotalDistance = 0;
	iCount = 0;
	
	int iMinDistance = MAX_INT;
	int iMaxDistance = 0;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				pOurCapitalCity = GET_PLAYER((PlayerTypes)iI).getCapitalCity();

				if (pOurCapitalCity != NULL)
				{
					for (iJ = 0; iJ < MAX_PLAYERS; iJ++)
					{
						if (GET_PLAYER((PlayerTypes)iJ).isAlive())
						{
							if (GET_PLAYER((PlayerTypes)iJ).getTeam() != getID())
							{
								pTheirCapitalCity = GET_PLAYER((PlayerTypes)iJ).getCapitalCity();

								if (pTheirCapitalCity != NULL)
								{
									int iDistance = (plotDistance(pOurCapitalCity->getX_INLINE(), pOurCapitalCity->getY_INLINE(), pTheirCapitalCity->getX_INLINE(), pTheirCapitalCity->getY_INLINE()) * (pOurCapitalCity->area() != pTheirCapitalCity->area() ? 3 : 2));
									if (GET_PLAYER((PlayerTypes)iJ).getTeam() == eTeam)
									{
										iTotalDistance += iDistance;
										iCount++;
									}
									iMinDistance = std::min(iDistance, iMinDistance);
									iMaxDistance = std::max(iDistance, iMaxDistance);
								}
							}
						}
					}
				}
			}
		}
	}
	
	if (iCount > 0)
	{
		FAssert(iMaxDistance > 0);
		return ((GC.getMapINLINE().maxPlotDistance() * (iMaxDistance - ((iTotalDistance / iCount) - iMinDistance))) / iMaxDistance);
	}

	return 0;
}

// K-Mod. Return true if we can deduce the location of at least 'iMiniumum' cities belonging to eTeam.
bool CvTeamAI::AI_haveSeenCities(TeamTypes eTeam, bool bPrimaryAreaOnly, int iMinimum) const
{
	int iCount = 0;
	for (PlayerTypes eLoopPlayer = (PlayerTypes)0; eLoopPlayer < MAX_PLAYERS; eLoopPlayer=(PlayerTypes)(eLoopPlayer+1))
	{
		const CvPlayerAI& kLoopPlayer = GET_PLAYER(eLoopPlayer);
		if (kLoopPlayer.getTeam() == eTeam)
		{
			int iLoop;
			for (CvCity* pLoopCity = kLoopPlayer.firstCity(&iLoop); pLoopCity; pLoopCity = kLoopPlayer.nextCity(&iLoop))
			{
				if (AI_deduceCitySite(pLoopCity))
				{
					if (!bPrimaryAreaOnly || AI_isPrimaryArea(pLoopCity->area()))
					{
						if (++iCount >= iMinimum)
							return true;
					}
				}
			}
		}
	}
	return false;
}
// K-Mod end

bool CvTeamAI::AI_isWarPossible() const
{
	if (getAtWarCount(false) > 0)
	{
		return true;
	}

	if (GC.getGameINLINE().isOption(GAMEOPTION_ALWAYS_WAR))
	{
		return true;
	}

	if (!(GC.getGameINLINE().isOption(GAMEOPTION_ALWAYS_PEACE)) && !(GC.getGameINLINE().isOption(GAMEOPTION_NO_CHANGING_WAR_PEACE)))
	{
		return true;
	}

	return false;
}

// This function has been completely rewritten for K-Mod. The original BtS code, and the BBAI code have been deleted.
bool CvTeamAI::AI_isLandTarget(TeamTypes eTeam) const
{
	PROFILE_FUNC();
	const CvTeamAI& kOtherTeam = GET_TEAM(eTeam);

	int iLoop;
	for(CvArea* pLoopArea = GC.getMapINLINE().firstArea(&iLoop); pLoopArea != NULL; pLoopArea = GC.getMapINLINE().nextArea(&iLoop))
	{
		if (AI_isPrimaryArea(pLoopArea) && kOtherTeam.AI_isPrimaryArea(pLoopArea))
			return true;
	}

	for (PlayerTypes i = (PlayerTypes)0; i < MAX_PLAYERS; i=(PlayerTypes)(i+1))
	{
		const CvPlayerAI& kOurPlayer = GET_PLAYER(i);
		if (kOurPlayer.getTeam() != getID() || !kOurPlayer.isAlive())
			continue;

		int iL1;
		for (CvCity* pOurCity = kOurPlayer.firstCity(&iL1); pOurCity; pOurCity = kOurPlayer.nextCity(&iL1))
		{
			if (!kOurPlayer.AI_isPrimaryArea(pOurCity->area()))
				continue;
			// city in a primary area.
			for (PlayerTypes j = (PlayerTypes)0; j < MAX_PLAYERS; j=(PlayerTypes)(j+1))
			{
				const CvPlayerAI& kTheirPlayer = GET_PLAYER(j);
				if (kTheirPlayer.getTeam() != eTeam || !kTheirPlayer.isAlive() || !kTheirPlayer.AI_isPrimaryArea(pOurCity->area()))
					continue;

				std::vector<TeamTypes> teamVec;
				teamVec.push_back(getID());
				teamVec.push_back(eTeam);
				FAStar* pTeamStepFinder = gDLL->getFAStarIFace()->create();
				gDLL->getFAStarIFace()->Initialize(pTeamStepFinder, GC.getMapINLINE().getGridWidthINLINE(), GC.getMapINLINE().getGridHeightINLINE(), GC.getMapINLINE().isWrapXINLINE(), GC.getMapINLINE().isWrapYINLINE(), stepDestValid, stepHeuristic, stepCost, teamStepValid, stepAdd, NULL, NULL);
				gDLL->getFAStarIFace()->SetData(pTeamStepFinder, &teamVec);

				int iL2;
				for (CvCity* pTheirCity = kTheirPlayer.firstCity(&iL2); pTheirCity; pTheirCity = kTheirPlayer.nextCity(&iL2))
				{
					if (pTheirCity->area() != pOurCity->area())
						continue;


					if (gDLL->getFAStarIFace()->GeneratePath(pTeamStepFinder, pOurCity->getX_INLINE(), pOurCity->getY_INLINE(), pTheirCity->getX_INLINE(), pTheirCity->getY_INLINE(), false, 0, true))
					{
						// good.
						gDLL->getFAStarIFace()->destroy(pTeamStepFinder);
						return true;
					}
				}
				gDLL->getFAStarIFace()->destroy(pTeamStepFinder);
			}
		}
	}

	return false;
}

// this determines if eTeam or any of its allies are land targets of us
bool CvTeamAI::AI_isAllyLandTarget(TeamTypes eTeam) const
{
	for (int iTeam = 0; iTeam < MAX_CIV_TEAMS; iTeam++)
	{
		CvTeam& kLoopTeam = GET_TEAM((TeamTypes)iTeam);
		if (iTeam != getID())
		{
			if (iTeam == eTeam || kLoopTeam.isVassal(eTeam) || GET_TEAM(eTeam).isVassal((TeamTypes)iTeam) || kLoopTeam.isDefensivePact(eTeam))
			{
				if (AI_isLandTarget((TeamTypes)iTeam))
				{
					return true;
				}
			}
		}
	}

	return false;
}


bool CvTeamAI::AI_shareWar(TeamTypes eTeam) const
{
	int iI;

	for (iI = 0; iI < MAX_CIV_TEAMS; iI++)
	{
		if (GET_TEAM((TeamTypes)iI).isAlive() && !GET_TEAM((TeamTypes)iI).isMinorCiv())
		{
			if ((iI != getID()) && (iI != eTeam))
			{
				if (isAtWar((TeamTypes)iI) && GET_TEAM(eTeam).isAtWar((TeamTypes)iI))
				{
					return true;
				}
			}
		}
	}

	return false;
}


AttitudeTypes CvTeamAI::AI_getAttitude(TeamTypes eTeam, bool bForced) const
{
	int iAttitude;
	int iCount;
	int iI, iJ;

	//FAssertMsg(eTeam != getID(), "shouldn't call this function on ourselves");
	// K-Mod
	if (eTeam == getID())
		return ATTITUDE_FRIENDLY;
	// K-Mod end

	iAttitude = 0;
	iCount = 0;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				for (iJ = 0; iJ < MAX_PLAYERS; iJ++)
				{
					if (GET_PLAYER((PlayerTypes)iJ).isAlive() && iI != iJ)
					{
						TeamTypes eTeamLoop = GET_PLAYER((PlayerTypes)iJ).getTeam();

						//if (eTeamLoop == eTeam || GET_TEAM(eTeamLoop).isVassal(eTeam) || GET_TEAM(eTeam).isVassal(eTeamLoop))
						if (eTeamLoop == eTeam) // K-Mod. Removed attitude averaging between vassals and masters
						{
							//iAttitude += GET_PLAYER((PlayerTypes)iI).AI_getAttitude((PlayerTypes)iJ, bForced);
							iAttitude += GET_PLAYER((PlayerTypes)iI).AI_getAttitudeVal((PlayerTypes)iJ, bForced); // bbai. Average values rather than attitudes directly.
							iCount++;
						}
					}
				}
			}
		}
	}

	if (iCount > 0)
	{
		// return ((AttitudeTypes)(iAttitude / iCount));
		return CvPlayerAI::AI_getAttitudeFromValue(iAttitude/iCount); // bbai / K-Mod
	}

	return ATTITUDE_CAUTIOUS;
}


int CvTeamAI::AI_getAttitudeVal(TeamTypes eTeam, bool bForced) const
{
	int iAttitudeVal;
	int iCount;
	int iI, iJ;

	FAssertMsg(eTeam != getID(), "shouldn't call this function on ourselves");

	iAttitudeVal = 0;
	iCount = 0;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				for (iJ = 0; iJ < MAX_PLAYERS; iJ++)
				{
					if (GET_PLAYER((PlayerTypes)iJ).isAlive())
					{
						if (GET_PLAYER((PlayerTypes)iJ).getTeam() == eTeam)
						{
							iAttitudeVal += GET_PLAYER((PlayerTypes)iI).AI_getAttitudeVal((PlayerTypes)iJ, bForced);
							iCount++;
						}
					}
				}
			}
		}
	}

	if (iCount > 0)
	{
		return (iAttitudeVal / iCount);
	}

	return 0;
}


int CvTeamAI::AI_getMemoryCount(TeamTypes eTeam, MemoryTypes eMemory) const
{
	int iMemoryCount;
	int iCount;
	int iI, iJ;

	FAssertMsg(eTeam != getID(), "shouldn't call this function on ourselves");

	iMemoryCount = 0;
	iCount = 0;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				for (iJ = 0; iJ < MAX_PLAYERS; iJ++)
				{
					if (GET_PLAYER((PlayerTypes)iJ).isAlive())
					{
						if (GET_PLAYER((PlayerTypes)iJ).getTeam() == eTeam)
						{
							iMemoryCount += GET_PLAYER((PlayerTypes)iI).AI_getMemoryCount(((PlayerTypes)iJ), eMemory);
							iCount++;
						}
					}
				}
			}
		}
	}

	if (iCount > 0)
	{
		return (iMemoryCount / iCount);
	}

	return 0;
}


int CvTeamAI::AI_chooseElection(const VoteSelectionData& kVoteSelectionData) const
{
	VoteSourceTypes eVoteSource = kVoteSelectionData.eVoteSource;

	FAssert(!isHuman());
	FAssert(GC.getGameINLINE().getSecretaryGeneral(eVoteSource) == getID());

	int iBestVote = -1;
	int iBestValue = 0;

	for (int iI = 0; iI < (int)kVoteSelectionData.aVoteOptions.size(); ++iI)
	{
		VoteTypes eVote = kVoteSelectionData.aVoteOptions[iI].eVote;
		CvVoteInfo& kVoteInfo = GC.getVoteInfo(eVote);

		FAssert(kVoteInfo.isVoteSourceType(eVoteSource));

		FAssert(GC.getGameINLINE().isChooseElection(eVote));
		bool bValid = true;

		if (!GC.getGameINLINE().isTeamVote(eVote))
		{
			for (int iJ = 0; iJ < MAX_PLAYERS; iJ++)
			{
				if (GET_PLAYER((PlayerTypes)iJ).isAlive())
				{
					if (GET_PLAYER((PlayerTypes)iJ).getTeam() == getID())
					{
						PlayerVoteTypes eVote = GET_PLAYER((PlayerTypes)iJ).AI_diploVote(kVoteSelectionData.aVoteOptions[iI], eVoteSource, true);

						if (eVote != PLAYER_VOTE_YES || eVote == GC.getGameINLINE().getVoteOutcome((VoteTypes)iI))
						{
							bValid = false;
							break;
						}
					}
				}
			}
		}

		if (bValid)
		{
			int iValue = (1 + GC.getGameINLINE().getSorenRandNum(10000, "AI Choose Vote"));

			if (iValue > iBestValue)
			{
				iBestValue = iValue;
				iBestVote = iI;
			}
		}
	}

	return iBestVote;
}

// K-Mod. New war evaluation functions. WIP
// Very rough estimate of what would be gained by conquering the target - in units of Gold/turn (kind of).
int CvTeamAI::AI_warSpoilsValue(TeamTypes eTarget, WarPlanTypes eWarPlan) const
{
	PROFILE_FUNC();

	FAssert(eTarget != getID());
	const CvTeamAI& kTargetTeam = GET_TEAM(eTarget);
	bool bAggresive = GC.getGameINLINE().isOption(GAMEOPTION_AGGRESSIVE_AI);

	// Deny factor: the percieved value of denying the enemy team of its resources (generally)
	int iDenyFactor = bAggresive
		? 40 - AI_getAttitudeWeight(eTarget)/3
		: 20 - AI_getAttitudeWeight(eTarget)/2;

	iDenyFactor += AI_getWorstEnemy() == eTarget ? 20 : 0; // (in addition to attitude pentalities)

	if (kTargetTeam.AI_isAnyMemberDoVictoryStrategyLevel3())
	{
		if (kTargetTeam.AI_isAnyMemberDoVictoryStrategyLevel4())
		{
			iDenyFactor += AI_isAnyMemberDoVictoryStrategyLevel4() ? 50 : 30;
		}

		if (bAggresive || AI_isAnyMemberDoVictoryStrategy(AI_VICTORY_CONQUEST3))
		{
			iDenyFactor += 20;
		}

		if (GC.getGameINLINE().getTeamRank(eTarget) < GC.getGameINLINE().getTeamRank(getID()))
		{
			iDenyFactor += 10;
		}
	}
	if (AI_isAnyMemberDoVictoryStrategy(AI_VICTORY_CONQUEST4 | AI_VICTORY_DOMINATION4))
	{
		iDenyFactor += 20;
	}

	int iRankDelta = GC.getGameINLINE().getTeamRank(getID()) - GC.getGameINLINE().getTeamRank(eTarget);
	if (iRankDelta > 0)
	{
		int iRankHate = 0;
		for (PlayerTypes eLoopPlayer = (PlayerTypes)0; eLoopPlayer < MAX_CIV_PLAYERS; eLoopPlayer=(PlayerTypes)(eLoopPlayer+1))
		{
			const CvPlayerAI& kLoopPlayer = GET_PLAYER(eLoopPlayer);
			if (kLoopPlayer.getTeam() == getID() && kLoopPlayer.isAlive())
				iRankHate += GC.getLeaderHeadInfo(kLoopPlayer.getLeaderType()).getWorseRankDifferenceAttitudeChange(); // generally around 0-3
		}

		if (iRankHate > 0)
		{
			int iTotalTeams = GC.getGameINLINE().countCivTeamsEverAlive();
			iDenyFactor += (100 - AI_getAttitudeWeight(eTarget)) * (iRankHate * iRankDelta + (iTotalTeams+1)/2) / std::max(1, 8*(iTotalTeams + 1)*getAliveCount());
			// that's a max of around 200 * 3 / 8. ~ 75
		}
	}

	bool bImminentVictory = kTargetTeam.AI_getLowestVictoryCountdown() >= 0;

	bool bTotalWar = eWarPlan == WARPLAN_TOTAL || eWarPlan == WARPLAN_PREPARING_TOTAL;
	bool bOverseasWar = !AI_hasSharedPrimaryArea(eTarget);

	int iGainedValue = 0;
	int iDeniedValue = 0;

	// Cities & Land
	int iPopCap = 2 + getTotalPopulation(false) / std::max(1, getNumCities()); // max number of plots to consider the value of.
	int iYieldMultiplier = 0; // multiplier for the value of plot yields.
	for (PlayerTypes eLoopPlayer = (PlayerTypes)0; eLoopPlayer < MAX_PLAYERS; eLoopPlayer=(PlayerTypes)(eLoopPlayer+1))
	{
		const CvPlayerAI& kLoopPlayer = GET_PLAYER(eLoopPlayer);
		if (kLoopPlayer.getTeam() != getID() || !kLoopPlayer.isAlive())
			continue;
		int iFoodMulti = kLoopPlayer.AI_averageYieldMultiplier(YIELD_FOOD);
		iYieldMultiplier += kLoopPlayer.AI_averageYieldMultiplier(YIELD_PRODUCTION) * iFoodMulti / 100;
		iYieldMultiplier += kLoopPlayer.AI_averageYieldMultiplier(YIELD_COMMERCE) * iFoodMulti / 100;
	}
	iYieldMultiplier /= std::max(1, 2 * getAliveCount());
	// now.. here's a bit of ad-hoccery.
	// the actual yield multiplayer is not the only thing that goes up as the game progresses.
	// the raw produce of land also tends to increase, as improvements become more powerful. Therefore...:
	iYieldMultiplier = iYieldMultiplier * (1 + GET_PLAYER(getLeaderID()).getCurrentEra() + GC.getNumEraInfos()) / std::max(1, GC.getNumEraInfos());
	//

	std::set<int> close_areas; // set of area IDs for which the enemy has cities close to ours.
	for (PlayerTypes eLoopPlayer = (PlayerTypes)0; eLoopPlayer < MAX_PLAYERS; eLoopPlayer=(PlayerTypes)(eLoopPlayer+1))
	{
		const CvPlayerAI& kLoopPlayer = GET_PLAYER(eLoopPlayer);
		if (kLoopPlayer.getTeam() != eTarget || !kLoopPlayer.isAlive())
			continue;

		int iLoop;
		for (CvCity* pLoopCity = kLoopPlayer.firstCity(&iLoop); pLoopCity; pLoopCity = kLoopPlayer.nextCity(&iLoop))
		{
			if (!AI_deduceCitySite(pLoopCity))
				continue;

			int iCityValue = pLoopCity->getPopulation();

			bool bCoastal = pLoopCity->isCoastal(GC.getMIN_WATER_SIZE_FOR_OCEAN());
			iCityValue += bCoastal ? 2 : 0;

			// plots
			std::vector<int> plot_values;
			for (int i = 1; i < NUM_CITY_PLOTS; i++) // don't count city plot
			{
				CvPlot* pLoopPlot = pLoopCity->getCityIndexPlot(i);
				if (!pLoopPlot || !pLoopPlot->isRevealed(getID(), false) || pLoopPlot->getWorkingCity() != pLoopCity)
					continue;

				if (pLoopPlot->isWater() && !(bCoastal && pLoopPlot->calculateNatureYield(YIELD_FOOD, getID()) >= GC.getFOOD_CONSUMPTION_PER_POPULATION()))
					continue;

				// This is a very rough estimate of the value of the plot. It's a bit ad-hoc. I'm sorry about that, but I want it to be fast.
				//BonusTypes eBonus = pLoopPlot->getBonusType(getID());
				int iPlotValue = 0;
				iPlotValue += 3 * pLoopPlot->calculateBestNatureYield(YIELD_FOOD, getID()); // don't ignore floodplains
				iPlotValue += 2 * pLoopPlot->calculateNatureYield(YIELD_PRODUCTION, getID(), true); // ignore forest
				iPlotValue += GC.getTerrainInfo(pLoopPlot->getTerrainType()).getYield(YIELD_FOOD) >= GC.getFOOD_CONSUMPTION_PER_POPULATION() ? 1 : 0; // bonus for grassland
				iPlotValue += pLoopPlot->isRiver() ? 1 : 0;
				if (pLoopPlot->getBonusType(getID()) != NO_BONUS)
					iPlotValue = iPlotValue * 3/2;
				iPlotValue += pLoopPlot->getYield(YIELD_COMMERCE) / 2; // include some value for existing towns.

				plot_values.push_back(iPlotValue);
			}
			std::partial_sort(plot_values.begin(), plot_values.begin() + std::min(iPopCap, (int)plot_values.size()), plot_values.end(), std::greater<int>());
			iCityValue = std::accumulate(plot_values.begin(), plot_values.begin() + std::min(iPopCap, (int)plot_values.size()), iCityValue);
			iCityValue = iCityValue * iYieldMultiplier / 100;

			// holy city value
			for (ReligionTypes i = (ReligionTypes)0; i < GC.getNumReligionInfos(); i=(ReligionTypes)(i+1))
			{
				if (pLoopCity->isHolyCity(i))
					iCityValue += std::max(0, GC.getGameINLINE().countReligionLevels(i) / (pLoopCity->hasShrine(i) ? 1 : 2) - 4);
				// note: the -4 at the end is mostly there to offset the 'wonder' value that will be added later.
				// I don't want to double-count the value of the shrine, and the religion without the shrine isn't worth much anyway.
			}

			// corp HQ value
			for (CorporationTypes i = (CorporationTypes)0; i < GC.getNumCorporationInfos(); i=(CorporationTypes)(i+1))
			{
				if (pLoopCity->isHeadquarters(i))
					iCityValue += std::max(0, 2 * GC.getGameINLINE().countCorporationLevels(i) - 4);
			}

			// wonders
			iCityValue += 4 * pLoopCity->getNumActiveWorldWonders();

			// denied
			iDeniedValue += iCityValue * iDenyFactor / 100;
			if (2*pLoopCity->getCulture(eLoopPlayer) > pLoopCity->getCultureThreshold(GC.getGameINLINE().culturalVictoryCultureLevel()))
			{
				iDeniedValue += (kLoopPlayer.AI_isDoVictoryStrategy(AI_VICTORY_CULTURE4) ? 100 : 30) * iDenyFactor / 100;
			}
			if (bImminentVictory && pLoopCity->isCapital())
			{
				iDeniedValue += 200 * iDenyFactor / 100;
			}

			// gained
			int iGainFactor = 0;
			if (bTotalWar)
			{
				if (AI_isPrimaryArea(pLoopCity->area()))
					iGainFactor = 70;
				else
				{
					if (bOverseasWar && GET_PLAYER(pLoopCity->getOwnerINLINE()).AI_isPrimaryArea(pLoopCity->area()))
						iGainFactor = 45;
					else
						iGainFactor = 30;

					iGainFactor += AI_isAnyMemberDoVictoryStrategy(AI_VICTORY_CONQUEST3 | AI_VICTORY_DOMINATION2) ? 10 : 0;
				}
			}
			else
			{
				if (AI_isPrimaryArea(pLoopCity->area()))
					iGainFactor = 40;
				else
					iGainFactor = 25;
			}
			if (pLoopCity->AI_highestTeamCloseness(getID()) > 0)
			{
				iGainFactor += 30;
				close_areas.insert(pLoopCity->getArea());
			}

			iGainedValue += iCityValue * iGainFactor / 100;
			//
		}
	}

	// Resources
	std::vector<int> bonuses(GC.getNumBonusInfos(), 0); // percentage points
	for (int i = 0; i < GC.getMapINLINE().numPlotsINLINE(); i++)
	{
		CvPlot* pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(i);

		if (pLoopPlot->getTeam() == eTarget)
		{
			// note: There are ways of knowning that the team has resources even if the plot cannot be seen; so my handling of isRevealed is a bit ad-hoc.
			BonusTypes eBonus = pLoopPlot->getNonObsoleteBonusType(getID());
			if (eBonus != NO_BONUS)
			{
				if (pLoopPlot->isRevealed(getID(), false) && AI_isPrimaryArea(pLoopPlot->area()))
					bonuses[eBonus] += bTotalWar ? 60 : 20;
				else
					bonuses[eBonus] += bTotalWar ? 20 : 0;
			}
		}
	}
	for (BonusTypes i = (BonusTypes)0; i < GC.getNumBonusInfos(); i=(BonusTypes)(i+1))
	{
		if (bonuses[i] > 0)
		{
			int iBonusValue = 0;
			int iMissing = 0;
			for (PlayerTypes eLoopPlayer = (PlayerTypes)0; eLoopPlayer < MAX_PLAYERS; eLoopPlayer=(PlayerTypes)(eLoopPlayer+1))
			{
				const CvPlayerAI& kLoopPlayer = GET_PLAYER(eLoopPlayer);
				if (kLoopPlayer.getTeam() == getID() && kLoopPlayer.isAlive())
				{
					iBonusValue += kLoopPlayer.AI_bonusVal(i, 0, true);
					if (kLoopPlayer.getNumAvailableBonuses(i) == 0)
						iMissing++;
				}
			}
			iBonusValue += GC.getBonusInfo(i).getAIObjective(); // (support for mods.)
			iBonusValue = iBonusValue * getNumCities() * (std::min(100*iMissing, bonuses[i]) + std::max(0, bonuses[i] - 100*iMissing)/8) / std::max(1, 400 * getAliveCount());
			//
			iGainedValue += iBonusValue;
			// ignore denied value.
		}
	}

	// magnify the gained value based on how many of the target's cities are in close areas
	int iCloseCities = 0;
	for (std::set<int>::iterator it = close_areas.begin(); it != close_areas.end(); ++it)
	{
		CvArea* pLoopArea = GC.getMapINLINE().getArea(*it);
		if (AI_isPrimaryArea(pLoopArea))
		{
			for (PlayerTypes eLoopPlayer = (PlayerTypes)0; eLoopPlayer < MAX_PLAYERS; eLoopPlayer=(PlayerTypes)(eLoopPlayer+1))
			{
				const CvPlayerAI& kLoopPlayer = GET_PLAYER(eLoopPlayer);
				if (kLoopPlayer.getTeam() == eTarget)
					iCloseCities += pLoopArea->getCitiesPerPlayer(eLoopPlayer);
			}
		}
	}
	iGainedValue *= 75 + 50 * iCloseCities / std::max(1, kTargetTeam.getNumCities());
	iGainedValue /= 100;

	// amplify the gained value if we are aiming for a conquest or domination victory
	if (AI_isAnyMemberDoVictoryStrategy(AI_VICTORY_CONQUEST2 | AI_VICTORY_DOMINATION2))
		iGainedValue = iGainedValue * 4/3;

	// reduce the gained value based on how many other teams are at war with the target
	// similar to the way the target's strength estimate is reduced.
	iGainedValue *= 2;
	iGainedValue /= (isAtWar(eTarget) ? 1 : 2) + kTargetTeam.getAtWarCount(true, true);

	return iGainedValue + iDeniedValue;
}

int CvTeamAI::AI_warCommitmentCost(TeamTypes eTarget, WarPlanTypes eWarPlan) const
{
	PROFILE_FUNC();
	// Things to consider:
	//
	// risk of losing cities
	// relative unit strength
	// relative current power
	// productivity
	// war weariness
	// diplomacy
	// opportunity cost (need for expansion, research, etc.)

	// For most parts of the calculation, it is actually more important to consider the master of the target rather than the target itself.
	// (this is important for defensive pacts; and it also affects relative power, relative productivity, and so on.)
	TeamTypes eTargetMaster = GET_TEAM(eTarget).getMasterTeam();
	FAssert(eTargetMaster == eTarget || GET_TEAM(eTarget).isAVassal());

	const CvTeamAI& kTargetMasterTeam = GET_TEAM(eTargetMaster);

	bool bTotalWar = eWarPlan == WARPLAN_TOTAL || eWarPlan == WARPLAN_PREPARING_TOTAL;
	bool bAttackWar = bTotalWar || (eWarPlan == WARPLAN_DOGPILE && kTargetMasterTeam.getAtWarCount(true) + (isAtWar(eTarget) ? 0 : 1) > 1);

	int iTotalCost = 0;

	// Estimate of military production costs
	{
		// Base commitment for a war of this type.
		int iCommitmentPerMil = bTotalWar ? 540 : 250;
		const int iBaseline = -25; // this value will be added to iCommitmentPerMil at the end of the calculation.

		// scale based on our current strength relative to our enemies.
		// cf. with code in AI_calculateAreaAIType
		{
			int iWarSuccessRating = isAtWar(eTarget) ? AI_getWarSuccessRating() : 0;
			int iOurRelativeStrength = 100 * getPower(true) / (AI_countMilitaryWeight(0) + 20); // whether to include vassals is a tricky issue...
			// Sum the relative strength for all enemies, including existing wars and wars with civs attached to the target team.
			int iEnemyRelativeStrength = 0;
			int iFreePowerBonus = GC.getUnitInfo(GC.getGameINLINE().getBestLandUnit()).getPowerValue() * 2;
			for (TeamTypes i = (TeamTypes)0; i < MAX_CIV_TEAMS; i=(TeamTypes)(i+1))
			{
				const CvTeamAI& kLoopTeam = GET_TEAM(i);
				if (!kLoopTeam.isAlive() || i == getID() || kLoopTeam.isVassal(getID()))
					continue;

				// note: this still doesn't count vassal siblings. (ie. if the target is a vassal, this will not count the master's other vassals.)
				// also, the vassals of defensive pact civs are currently not counted either.
				if (isAtWar(i) || i == eTargetMaster || kLoopTeam.isDefensivePact(eTargetMaster) || kLoopTeam.isVassal(eTargetMaster))
				{
					// the + power is meant to account for the fact that the target may get stronger while we are planning for war - especially early in the game.
					// use a slightly reduced value if we're actually not intending to attack this target. (full weight to all enemies in defensive wars)
					int iWeight = !bAttackWar || isAtWar(i) || i == eTarget ? 100 : 80;
					iEnemyRelativeStrength += iWeight * ((isAtWar(i) ? 0 : iFreePowerBonus) + kLoopTeam.getPower(false)) / (((isAtWar(i) ? 1 : 2) + kLoopTeam.getAtWarCount(true, true))*kLoopTeam.AI_countMilitaryWeight(0)/2 + 20);
				}
			}
			//

			//iCommitmentPerMil = iCommitmentPerMil * (100 * iEnemyRelativeStrength) / std::max(1, iOurRelativeStrength * (100+iWarSuccessRating/2));
			iCommitmentPerMil = iCommitmentPerMil * iEnemyRelativeStrength / std::max(1, iOurRelativeStrength);
		}

		// scale based on the relative size of our civilizations.
		int iOurProduction = AI_estimateTotalYieldRate(YIELD_PRODUCTION); // (note: this is separate from our total production, because I use it again a bit later.)
		{
			int iOurTotalProduction = iOurProduction * 100;
			int iEnemyTotalProduction = 0;
			const int iVassalFactor = 60; // only count some reduced percentage of vassal production.

			for (TeamTypes i = (TeamTypes)0; i < MAX_CIV_TEAMS; i=(TeamTypes)(i+1))
			{
				const CvTeamAI& kLoopTeam = GET_TEAM(i);
				if (!kLoopTeam.isAlive() || i == getID()) // our team is already counted.
					continue;

				if (kLoopTeam.isVassal(getID()))
					iOurTotalProduction += kLoopTeam.AI_estimateTotalYieldRate(YIELD_PRODUCTION) * iVassalFactor;
				else if (i == eTargetMaster)
					iEnemyTotalProduction += kLoopTeam.AI_estimateTotalYieldRate(YIELD_PRODUCTION) * 100;
				else if (kLoopTeam.isVassal(eTargetMaster))
					iEnemyTotalProduction += kLoopTeam.AI_estimateTotalYieldRate(YIELD_PRODUCTION) * iVassalFactor;
				else if (isAtWar(i) || kLoopTeam.isDefensivePact(eTargetMaster))
					iEnemyTotalProduction += kLoopTeam.AI_estimateTotalYieldRate(YIELD_PRODUCTION) * 100;
			}

			iCommitmentPerMil *= 6 * iEnemyTotalProduction + iOurTotalProduction;
			iCommitmentPerMil /= std::max(1, iEnemyTotalProduction + 6 * iOurTotalProduction);
		}

		// scale based on the relative strengths of our units
		{
			int iEnemyUnit = std::max(30, kTargetMasterTeam.getTypicalUnitValue(NO_UNITAI, DOMAIN_LAND));
			int iOurAttackUnit = std::max(30, getTypicalUnitValue(UNITAI_ATTACK, DOMAIN_LAND));
			int iOurDefenceUnit = std::max(30, getTypicalUnitValue(UNITAI_CITY_DEFENSE, DOMAIN_LAND));
			int iHighScale = 30 + 70 * std::max(iOurAttackUnit, iOurDefenceUnit) / iEnemyUnit;
			int iLowScale = 10 + 90 * std::min(iOurAttackUnit, iOurDefenceUnit) / iEnemyUnit;

			iCommitmentPerMil = std::min(iCommitmentPerMil, 300) * 100 / iHighScale + std::max(0, iCommitmentPerMil - 300) * 100 / iLowScale;

			// Adjust for overseas wars
			if (!AI_hasSharedPrimaryArea(eTarget))
			{
				int iOurNavy = getTypicalUnitValue(NO_UNITAI, DOMAIN_SEA);
				int iEnemyNavy = std::max(1, kTargetMasterTeam.getTypicalUnitValue(NO_UNITAI, DOMAIN_SEA)); // (using max here to avoid div-by-zero later on)

				// rescale (unused)
				/* {
					int x = std::max(2, iOurNavy + iEnemyNavy) / 2;
					iOurNavy = iOurNavy * 100 / x;
					iEnemyNavy = iEnemyNavy * 100 / x;
				} */

				// Note: Commitment cost is currently meant to take into account risk as well as resource requirements.
				//       But with overseas wars, the relative strength of navy units effects these things differently.
				//       If our navy is much stronger than theirs, then our risk is low but we still need to commit a
				//       just as much resources to win the land-war for an invasion.
				//       If their navy is stronger than ours, our risk is high and our resources will be higher too.
				//
				//       The current calculations are too simplistic to explicitly specify all that stuff.
				if (bTotalWar)
				{
					//iCommitmentPerMil = iCommitmentPerMil * (4*iOurNavy + 5*iEnemyNavy) / (8*iOurNavy + 1*iEnemyNavy);
					//iCommitmentPerMil = iCommitmentPerMil * 200 / std::min(200, iLowScale + iHighScale);
					//
					iCommitmentPerMil = iCommitmentPerMil * 200 / std::min(240, (iLowScale + iHighScale) * (9*iOurNavy + 1*iEnemyNavy) / (6*iOurNavy + 4*iEnemyNavy));
				}
				else
					iCommitmentPerMil = iCommitmentPerMil * (1*iOurNavy + 4*iEnemyNavy) / (4*iOurNavy + 1*iEnemyNavy);
			}
		}

		// increase the cost for distant targets...
		if (AI_teamCloseness(eTarget) == 0)
		{
			// ... in the early game.
			if (getNumCities() < GC.getWorldInfo(GC.getMapINLINE().getWorldSize()).getTargetNumCities() * getAliveCount())
				iCommitmentPerMil = iCommitmentPerMil * 3/2;
			/* else
				iCommitmentPerMil = iCommitmentPerMil * 5/4; */

			// ... and for total war
			if (bTotalWar)
				iCommitmentPerMil = iCommitmentPerMil * 5/4;
		}

		iCommitmentPerMil += iBaseline; // The baseline should be a negative value which represents some amount of "free" commitment.

		if (iCommitmentPerMil > 0)
		{
			// iCommitmentPerMil will be multiplied by a rough estimate of the total resources this team could devote to war.
			int iCommitmentPool = iOurProduction * 3 + AI_estimateTotalYieldRate(YIELD_COMMERCE); // cf. AI_yieldWeight
			// Note: it would probably be good to take into account the expected increase in unit spending - but that's a bit tricky.

			// sometimes are resources are more in demand than other times...
			int iPoolMultiplier = 0;
			for (PlayerTypes eLoopPlayer = (PlayerTypes)0; eLoopPlayer < MAX_PLAYERS; eLoopPlayer=(PlayerTypes)(eLoopPlayer+1))
			{
				const CvPlayerAI& kLoopPlayer = GET_PLAYER(eLoopPlayer);
				if (kLoopPlayer.getTeam() == getID() && kLoopPlayer.isAlive())
				{
					iPoolMultiplier += 100;
					// increase value if we are still trying to expand peacefully. Now, the minimum site value is pretty arbitrary...
					int iSites = kLoopPlayer.AI_getNumPrimaryAreaCitySites(kLoopPlayer.AI_getMinFoundValue()*2); // note, there's a small cap on the number of sites, around 3.
					if (iSites > 0)
					{
						iPoolMultiplier += (50 + 50 * range(GC.getWorldInfo(GC.getMapINLINE().getWorldSize()).getTargetNumCities() - kLoopPlayer.getNumCities(), 0, iSites))/(bTotalWar ? 2 : 1);
					}
				}
			}
			iPoolMultiplier /= std::max(1, getAliveCount());
			iCommitmentPool = iCommitmentPool * iPoolMultiplier / 100;

			// Don't pick a fight if we're expecting to beat them to a peaceful victory.
			if (!AI_isAnyMemberDoVictoryStrategy(AI_VICTORY_DOMINATION4 | AI_VICTORY_CONQUEST4))
			{
				if (AI_isAnyMemberDoVictoryStrategy(AI_VICTORY_CULTURE4) ||
					(AI_isAnyMemberDoVictoryStrategy(AI_VICTORY_SPACE4) && !GET_TEAM(eTarget).AI_isAnyMemberDoVictoryStrategy(AI_VICTORY_CULTURE4 | AI_VICTORY_SPACE4)) ||
					(AI_getLowestVictoryCountdown() > 0 && (GET_TEAM(eTarget).AI_getLowestVictoryCountdown() < 0 || AI_getLowestVictoryCountdown() < GET_TEAM(eTarget).AI_getLowestVictoryCountdown())))
				{
					iCommitmentPool *= 2;
				}
			}

			iTotalCost += iCommitmentPerMil * iCommitmentPool / 1000;
		}
	}

	// war weariness
	int iTotalWw = 0;
	for (TeamTypes i = (TeamTypes)0; i < MAX_CIV_TEAMS; i=(TeamTypes)(i+1))
	{
		const CvTeamAI& kLoopTeam = GET_TEAM(i);
		if (kLoopTeam.isAlive() && (i == eTargetMaster || kLoopTeam.isVassal(eTargetMaster)))
			iTotalWw += getWarWeariness(i, true)/100;
	}
	// note: getWarWeariness has units of anger per 100,000 population, and it is customary to divide it by 100 immediately
	if (iTotalWw > 50)
	{
		int iS = isAtWar(eTarget) ? -1 : 1;
		int iWwCost = 0;
		for (PlayerTypes eLoopPlayer = (PlayerTypes)0; eLoopPlayer < MAX_CIV_PLAYERS; eLoopPlayer=(PlayerTypes)(eLoopPlayer+1))
		{
			const CvPlayerAI& kLoopPlayer = GET_PLAYER(eLoopPlayer);
			if (kLoopPlayer.getTeam() == getID() && kLoopPlayer.isAlive())
			{
				int iEstimatedPercentAnger = kLoopPlayer.getModifiedWarWearinessPercentAnger(iTotalWw) / 10; // (ugly, I know. But that's just how it's done.)
				// note. Unfortunately, we haven't taken the effect of jails into account.
				iWwCost += iS * kLoopPlayer.getNumCities() * kLoopPlayer.AI_getHappinessWeight(iS * iEstimatedPercentAnger * (100 + kLoopPlayer.getWarWearinessModifier())/100, 0, true) / 20;
			}
		}
		iTotalCost += iWwCost;
	}

	// Note: diplomacy cost is handled elsewhere

	return iTotalCost;
}

// diplomatic costs for declaring war (somewhat arbitrary - to encourage the AI to attack its enemies, and the enemies of its friends.)
int CvTeamAI::AI_warDiplomacyCost(TeamTypes eTarget) const
{
	if (isAtWar(eTarget))
	{
		//FAssertMsg(false, "AI_warDiplomacyCost called when already at war."); // sometimes we call this function for debug purposes.
		return 0;
	}

	if (AI_isAnyMemberDoVictoryStrategy(AI_VICTORY_CONQUEST4))
		return 0;

	const CvTeamAI& kTargetTeam = GET_TEAM(eTarget);

	// first, the cost of upsetting the team we are declaring war on.
	int iDiploPopulation = kTargetTeam.getTotalPopulation(false);
	int iDiploCost = 3 * iDiploPopulation * (100 + AI_getAttitudeWeight(eTarget)) / 200;

	// cost of upsetting their friends
	for (TeamTypes i = (TeamTypes)0; i < MAX_CIV_TEAMS; i=(TeamTypes)(i+1))
	{
		const CvTeamAI& kLoopTeam = GET_TEAM(i);

		if (!kLoopTeam.isAlive() || i == getID() || i == eTarget)
			continue;

		if (isHasMet(i) && kTargetTeam.isHasMet(i) && !kLoopTeam.isCapitulated())
		{
			int iPop = kLoopTeam.getTotalPopulation(false);
			iDiploPopulation += iPop;
			if (kLoopTeam.AI_getAttitude(eTarget) >= ATTITUDE_PLEASED && AI_getAttitude(i) >= ATTITUDE_PLEASED)
			{
				iDiploCost += iPop * (100 + AI_getAttitudeWeight(i)) / 200;
			}
			else if (kLoopTeam.isAtWar(eTarget))
			{
				iDiploCost -= iPop * (100 + AI_getAttitudeWeight(i)) / 400;
			}
		}
	}

	// scale the diplo cost based the personality of the team.
	{
		int iPeaceWeight = 0;
		for (PlayerTypes eLoopPlayer = (PlayerTypes)0; eLoopPlayer < MAX_CIV_PLAYERS; eLoopPlayer=(PlayerTypes)(eLoopPlayer+1))
		{
			const CvPlayerAI& kLoopPlayer = GET_PLAYER(eLoopPlayer);
			if (kLoopPlayer.getTeam() == getID() && kLoopPlayer.isAlive())
			{
				iPeaceWeight += kLoopPlayer.AI_getPeaceWeight(); // usually between 0-10.
			}
		}

		int iDiploWeight = 40;
		iDiploWeight += 10 * iPeaceWeight / getAliveCount();
		// This puts iDiploWeight somewhere around 50 - 250.
		if (GC.getGameINLINE().isOption(GAMEOPTION_AGGRESSIVE_AI))
			iDiploWeight /= 2;
		if (AI_isAnyMemberDoVictoryStrategy(AI_VICTORY_DIPLOMACY3))
			iDiploWeight += 50;
		if (AI_isAnyMemberDoVictoryStrategy(AI_VICTORY_DIPLOMACY4))
			iDiploWeight += 50;
		if (AI_isAnyMemberDoVictoryStrategy(AI_VICTORY_CONQUEST3)) // note: conquest4 ignores diplo completely.
			iDiploWeight /= 2;

		iDiploCost *= iDiploWeight;
		iDiploCost /= 100;
	}

	// Finally, reduce the value for large maps;
	// so that this diplomacy stuff doesn't become huge relative to other parts of the war evaluation.
	iDiploCost *= 3;
	iDiploCost /= std::max(5, GC.getWorldInfo((WorldSizeTypes)GC.getMapINLINE().getWorldSize()).getDefaultPlayers());

	return iDiploCost;
}
// K-Mod end.

/// \brief Relative value of starting a war against eTeam.
///
/// This function computes the value of starting a war against eTeam.
/// The returned value should be compared against other possible targets
/// to pick the best target.

// K-Mod. Complete remake of the function.
int CvTeamAI::AI_startWarVal(TeamTypes eTarget, WarPlanTypes ePlan) const
{
	TeamTypes eTargetMaster = GET_TEAM(eTarget).getMasterTeam(); // we need this for checking defensive pacts.
	int iTotalValue = AI_warSpoilsValue(eTarget, ePlan) - AI_warCommitmentCost(eTarget, ePlan) - AI_warDiplomacyCost(eTarget);

	// Call AI_warSpoilsValue for each additional enemy team involved in this war.
	// NOTE: a single call to AI_warCommitmentCost should include the cost of fighting all of these enemies.
	for (TeamTypes i = (TeamTypes)0; i < MAX_CIV_TEAMS; i=(TeamTypes)(i+1))
	{
		if (i == getID() || i == eTarget)
			continue;

		const CvTeam& kLoopTeam = GET_TEAM(i);

		if (!kLoopTeam.isAlive() || kLoopTeam.isVassal(getID()))
			continue;

		if (kLoopTeam.isVassal(eTarget) || GET_TEAM(eTarget).isVassal(i))
		{
			iTotalValue += AI_warSpoilsValue(i, WARPLAN_DOGPILE) - AI_warDiplomacyCost(i);
		}
		else if (kLoopTeam.isDefensivePact(eTargetMaster))
		{
			FAssert(!isAtWar(eTarget));
			iTotalValue += AI_warSpoilsValue(i, WARPLAN_ATTACKED); // note: no diplo cost for this b/c it isn't us declaring war.
		}
	}
	return iTotalValue;
}
// K-Mod end
#if 0 // Original / BBAI code, disabled by K-Mod.
int CvTeamAI::AI_startWarVal(TeamTypes eTeam) const
{
	PROFILE_FUNC();

	int iValue;

	iValue = AI_calculatePlotWarValue(eTeam);

	iValue += (3 * AI_calculateCapitalProximity(eTeam)) / ((iValue > 0) ? 2 : 3);
	
	int iClosenessValue = AI_teamCloseness(eTeam);
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      05/16/10                                jdog5000      */
/*                                                                                              */
/* War Strategy AI, Victory Strategy AI                                                         */
/************************************************************************************************/
/* original code
	if (iClosenessValue == 0)
	{
		iValue /= 4;
	}
	iValue += iClosenessValue / 4;
*/
	// Dividing iValue by 4 is a drastic move, will result in more backstabbing between friendly neighbors
	// which is appropriate for Aggressive
	// Closeness values are much smaller after the fix to CvPlayerAI::AI_playerCloseness, no need to divide by 4
	if (iClosenessValue == 0)
	{
		iValue /= (GC.getGameINLINE().isOption(GAMEOPTION_AGGRESSIVE_AI) ? 4 : 2);
	}
	iValue += iClosenessValue;

	iValue += AI_calculateBonusWarValue(eTeam);
	
	// Target other teams close to victory
	if( GET_TEAM(eTeam).AI_isAnyMemberDoVictoryStrategyLevel3() )
	{
		iValue += 10;

		bool bAggressive = GC.getGameINLINE().isOption(GAMEOPTION_AGGRESSIVE_AI);
		bool bConq4 = AI_isAnyMemberDoVictoryStrategy(AI_VICTORY_CONQUEST4);

		// Prioritize targets closer to victory
		if( bConq4 || bAggressive )
		{
			iValue *= 3;
			iValue /= 2;
		}

		if( GET_TEAM(eTeam).AI_isAnyMemberDoVictoryStrategyLevel4() )
		{
			if( GET_TEAM(eTeam).AI_getLowestVictoryCountdown() >= 0 )
			{
				iValue += 50;
			}

			iValue *= 2;

			if( bConq4 || bAggressive )
			{
				iValue *= 4;
			}
			else if( AI_isAnyMemberDoVictoryStrategyLevel3() )
			{
				iValue *= 2;
			}
		}
	}

	// This adapted legacy code just makes us more willing to enter a war in a trade deal 
	// as boost applies to all rivals
	if( AI_isAnyMemberDoVictoryStrategy(AI_VICTORY_DOMINATION3) )
	{
		iValue *= (GC.getGameINLINE().isOption(GAMEOPTION_AGGRESSIVE_AI) ? 3 : 2);
	}

	// If occupied or conquest inclined and early/not strong, value weak opponents
	if( getAnyWarPlanCount(true) > 0 || 
		(AI_isAnyMemberDoVictoryStrategy(AI_VICTORY_CONQUEST2) && !(AI_isAnyMemberDoVictoryStrategy(AI_VICTORY_CONQUEST3))) )
	{
		int iMultiplier = (75 * getPower(false))/std::max(1, GET_TEAM(eTeam).getDefensivePower(getID()));

		iValue *= range(iMultiplier, 50, 400);
		iValue /= 100;
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	switch (AI_getAttitude(eTeam))
	{
	case ATTITUDE_FURIOUS:
		iValue *= 16;
		break;

	case ATTITUDE_ANNOYED:
		iValue *= 8;
		break;

	case ATTITUDE_CAUTIOUS:
		iValue *= 4;
		break;

	case ATTITUDE_PLEASED:
		iValue *= 2;
		break;

	case ATTITUDE_FRIENDLY:
		iValue *= 1;
		break;

	default:
		FAssert(false);
		break;
	}
	
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      03/21/10                                jdog5000      */
/*                                                                                              */
/* Victory Strategy AI                                                                          */
/************************************************************************************************/
	// Make it harder to bribe player to start a war
	if ( AI_isAnyMemberDoVictoryStrategy(AI_VICTORY_CULTURE4))
	{
		iValue /= 8;
	}
	else if ( AI_isAnyMemberDoVictoryStrategy(AI_VICTORY_SPACE4))
	{
		iValue /= 4;
	}
	else if ( AI_isAnyMemberDoVictoryStrategy(AI_VICTORY_CULTURE3))
	{
		iValue /= 3;
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	return iValue;
}
#endif // (end of disabled code)

// XXX this should consider area power...
int CvTeamAI::AI_endWarVal(TeamTypes eTeam) const
{
	FAssertMsg(eTeam != getID(), "shouldn't call this function on ourselves");
	FAssertMsg(isAtWar(eTeam), "Current AI Team instance is expected to be at war with eTeam");

	const CvTeam& kWarTeam = GET_TEAM(eTeam); // K-Mod

	int iValue = 100;

	iValue += (getNumCities() * 3);
	iValue += (kWarTeam.getNumCities() * 3);

	iValue += getTotalPopulation();
	iValue += kWarTeam.getTotalPopulation();

	iValue += (kWarTeam.AI_getWarSuccess(getID()) * 20);

	int iOurPower = std::max(1, getPower(true));
	int iTheirPower = std::max(1, kWarTeam.getDefensivePower(getID()));

	iValue *= iTheirPower + 10;
	iValue /= std::max(1, iOurPower + iTheirPower + 10);

	WarPlanTypes eWarPlan = AI_getWarPlan(eTeam);

	// if we are not human, do we want to continue war for strategic reasons?
	// only check if our power is at least 120% of theirs
	if (!isHuman() && iOurPower > 120 * iTheirPower / 100)
	{
		bool bDagger = false;

		bool bAnyFinancialTrouble = false;
		for (int iI = 0; iI < MAX_PLAYERS; iI++)
		{
			const CvPlayerAI& kLoopPlayer = GET_PLAYER((PlayerTypes)iI); // K-Mod
			if (kLoopPlayer.isAlive() && kLoopPlayer.getTeam() == getID())
			{
				if (kLoopPlayer.AI_isDoStrategy(AI_STRATEGY_DAGGER))
				{
					bDagger = true;
				}

				if (kLoopPlayer.AI_isFinancialTrouble())
				{
					bAnyFinancialTrouble = true;
				}
			}
		}

		// if dagger, value peace at 90% * power ratio
		if (bDagger)
		{
			iValue *= 9 * iTheirPower;
			iValue /= 10 * iOurPower;
		}
		
	    // for now, we will always do the land mass check for domination
		// if we have more than half the land, then value peace at 90% * land ratio 
		int iLandRatio = getTotalLand(true) * 100 / std::max(1, kWarTeam.getTotalLand(true));
	    if (iLandRatio > 120)
	    {
			iValue *= 9 * 100;
			iValue /= 10 * iLandRatio;
	    }

		// if in financial trouble, warmongers will continue the fight to make more money
		if (bAnyFinancialTrouble)
		{
			switch (eWarPlan)
			{
				case WARPLAN_TOTAL:
					// if we total warmonger, value peace at 70% * power ratio factor
					if (bDagger || AI_maxWarRand() < 100)
					{
						iValue *= 7 * (5 * iTheirPower);
						iValue /= 10 * (iOurPower + (4 * iTheirPower));
					}
					break;

				case WARPLAN_LIMITED:
					// if we limited warmonger, value peace at 70% * power ratio factor
					if (AI_limitedWarRand() < 100)
					{
						iValue *= 7 * (5 * iTheirPower);
						iValue /= 10 * (iOurPower + (4 * iTheirPower));
					}
					break;

				case WARPLAN_DOGPILE:
					// if we dogpile warmonger, value peace at 70% * power ratio factor
					if (AI_dogpileWarRand() < 100)
					{
						iValue *= 7 * (5 * iTheirPower);
						iValue /= 10 * (iOurPower + (4 * iTheirPower));
					}
					break;

			}
		}
	}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      05/19/10                                jdog5000      */
/*                                                                                              */
/* War strategy AI, Victory Strategy AI                                                         */
/************************************************************************************************/
	/* original BBAI code
	if( AI_isAnyMemberDoVictoryStrategy(AI_VICTORY_CULTURE4) )
	{
		iValue *= 4;
	}
	else if( AI_isAnyMemberDoVictoryStrategy(AI_VICTORY_CULTURE3) || AI_isAnyMemberDoVictoryStrategy(AI_VICTORY_SPACE4) )
	{
		iValue *= 2;
	} */
	// K-Mod
	if (AI_isAnyMemberDoVictoryStrategy(AI_VICTORY_CULTURE4))
	{
		iValue *= 3;
	}
	else if (AI_isAnyMemberDoVictoryStrategy(AI_VICTORY_SPACE4))
	{
		iValue *= 2;
	}
	else if (AI_isAnyMemberDoVictoryStrategy(AI_VICTORY_CULTURE3 | AI_VICTORY_SPACE3))
	{
		iValue *= 4;
		iValue /= 3;
	}
	// K-Mod end

	if ((!isHuman() && eWarPlan == WARPLAN_TOTAL) ||
		(!kWarTeam.isHuman() && kWarTeam.AI_getWarPlan(getID()) == WARPLAN_TOTAL))
	{
		iValue *= 2;
	}
	else if ((!isHuman() && eWarPlan == WARPLAN_DOGPILE && kWarTeam.getAtWarCount(true) > 1) ||
		     (!kWarTeam.isHuman() && kWarTeam.AI_getWarPlan(getID()) == WARPLAN_DOGPILE && getAtWarCount(true) > 1))
	{
		iValue *= 3;
		iValue /= 2;
	}

	// Do we have a big stack en route?
	int iOurAttackers = 0;
	for( int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; iPlayer++ )
	{
		if( GET_PLAYER((PlayerTypes)iPlayer).getTeam() == getID() )
		{
			iOurAttackers += GET_PLAYER((PlayerTypes)iPlayer).AI_enemyTargetMissions(eTeam);
		}
	}
	int iTheirAttackers = 0;
	CvArea* pLoopArea = NULL;
	int iLoop;
	for(pLoopArea = GC.getMapINLINE().firstArea(&iLoop); pLoopArea != NULL; pLoopArea = GC.getMapINLINE().nextArea(&iLoop))
	{
		iTheirAttackers += countEnemyDangerByArea(pLoopArea, eTeam);
	}

	int iAttackerRatio = (100 * iOurAttackers) / std::max(1 + GC.getGameINLINE().getCurrentEra(), iTheirAttackers);

	if( GC.getGameINLINE().isOption(GAMEOPTION_AGGRESSIVE_AI) )
	{
		iValue *= 150;
		iValue /= range(iAttackerRatio, 150, 900);
	}
	else
	{
		iValue *= 200;
		iValue /= range(iAttackerRatio, 200, 600);
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	iValue -= (iValue % GC.getDefineINT("DIPLOMACY_VALUE_REMAINDER"));

	if (isHuman())
	{
		return std::max(iValue, GC.getDefineINT("DIPLOMACY_VALUE_REMAINDER"));
	}
	else
	{
		return iValue;
	}
}

// K-Mod. This is the tech value modifier for devaluing techs that are known by other civs
// It's based on the original bts code from AI_techTradVal
int CvTeamAI::AI_knownTechValModifier(TechTypes eTech) const
{
	int iTechCivs = 0;
	int iCivsMet = 0;

	for (TeamTypes i = (TeamTypes)0; i < MAX_CIV_TEAMS; i=(TeamTypes)(i+1))
	{
		const CvTeam& kLoopTeam = GET_TEAM(i);
		if (i != getID() && kLoopTeam.isAlive() && isHasMet(i))
		{
			if (kLoopTeam.isHasTech(eTech))
				iTechCivs++;

			iCivsMet++;
		}
	}

	return 50 * (iCivsMet - iTechCivs) / std::max(1, iCivsMet);
}
// K-Mod end

int CvTeamAI::AI_techTradeVal(TechTypes eTech, TeamTypes eTeam) const
{
	FAssert(eTeam != getID());
	/* original bts code
	int iKnownCount;
	int iPossibleKnownCount;
	int iCost;
	int iValue;
	int iI;

	iCost = std::max(0, (getResearchCost(eTech) - getResearchProgress(eTech)));

	iValue = ((iCost * 3) / 2);

	iKnownCount = 0;
	iPossibleKnownCount = 0;

	for (iI = 0; iI < MAX_CIV_TEAMS; iI++)
	{
		if (GET_TEAM((TeamTypes)iI).isAlive())
		{
			if (iI != getID())
			{
				if (isHasMet((TeamTypes)iI))
				{
					if (GET_TEAM((TeamTypes)iI).isHasTech(eTech))
					{
						iKnownCount++;
					}

					iPossibleKnownCount++;
				}
			}
		}
	}

	iValue += (((iCost / 2) * (iPossibleKnownCount - iKnownCount)) / iPossibleKnownCount);
	*/
	// K-Mod. Standardized the modifier for # of teams with the tech; and removed the effect of team size.
	int iValue = (150 + AI_knownTechValModifier(eTech)) * std::max(0, (getResearchCost(eTech, true, false) - getResearchProgress(eTech))) / 100;
	// K-Mod end

	iValue *= std::max(0, (GC.getTechInfo(eTech).getAITradeModifier() + 100));
	iValue /= 100;

	iValue -= (iValue % GC.getDefineINT("DIPLOMACY_VALUE_REMAINDER"));

	if (isHuman())
	{
		return std::max(iValue, GC.getDefineINT("DIPLOMACY_VALUE_REMAINDER"));
	}
	else
	{
		return iValue;
	}
}


DenialTypes CvTeamAI::AI_techTrade(TechTypes eTech, TeamTypes eTeam) const
{
	PROFILE_FUNC();

	FAssertMsg(eTeam != getID(), "shouldn't call this function on ourselves");

	if (GC.getGameINLINE().isOption(GAMEOPTION_NO_TECH_BROKERING))
	{
		CvTeam& kTeam = GET_TEAM(eTeam);

		if (!kTeam.isHasTech(eTech))
		{
			if (!kTeam.isHuman())
			{
				if (2 * kTeam.getResearchProgress(eTech) > kTeam.getResearchCost(eTech))
				{
					return DENIAL_NO_GAIN;
				}
			}
		}
	}

	if (isHuman())
	{
		return NO_DENIAL;
	}

	if (isVassal(eTeam))
	{
		return NO_DENIAL;
	}

	if (isAtWar(eTeam))
	{
		return NO_DENIAL;
	}

	if (AI_getWorstEnemy() == eTeam)
	{
		return DENIAL_WORST_ENEMY;
	}

	AttitudeTypes eAttitude = AI_getAttitude(eTeam);

	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				if (eAttitude <= GC.getLeaderHeadInfo(GET_PLAYER((PlayerTypes)iI).getPersonalityType()).getTechRefuseAttitudeThreshold())
				{
					return DENIAL_ATTITUDE;
				}
			}
		}
	}

	if (eAttitude < ATTITUDE_FRIENDLY)
	{
		if ((GC.getGameINLINE().getTeamRank(getID()) < (GC.getGameINLINE().countCivTeamsEverAlive() / 2)) ||
			(GC.getGameINLINE().getTeamRank(eTeam) < (GC.getGameINLINE().countCivTeamsEverAlive() / 2)))
		{
			int iNoTechTradeThreshold = AI_noTechTradeThreshold();

			iNoTechTradeThreshold *= std::max(0, (GC.getHandicapInfo(GET_TEAM(eTeam).getHandicapType()).getNoTechTradeModifier() + 100));
			iNoTechTradeThreshold /= 100;

			if (AI_getMemoryCount(eTeam, MEMORY_RECEIVED_TECH_FROM_ANY) > iNoTechTradeThreshold)
			{
				return DENIAL_TECH_WHORE;
			}
		}
	// K-Mod. Generic tech trade test
	}
	if (eTech == NO_TECH)
		return NO_DENIAL;

	if (eAttitude < ATTITUDE_FRIENDLY)
	{
	// K-Mod end
		int iKnownCount = 0;
		int iPossibleKnownCount = 0;

		for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
		{
			if (GET_TEAM((TeamTypes)iI).isAlive())
			{
				if ((iI != getID()) && (iI != eTeam))
				{
					if (isHasMet((TeamTypes)iI))
					{
						if (GET_TEAM((TeamTypes)iI).isHasTech(eTech))
						{
							iKnownCount++;
						}

						iPossibleKnownCount++;
					}
				}
			}
		}

		int iTechTradeKnownPercent = AI_techTradeKnownPercent();

		iTechTradeKnownPercent *= std::max(0, (GC.getHandicapInfo(GET_TEAM(eTeam).getHandicapType()).getTechTradeKnownModifier() + 100));
		iTechTradeKnownPercent /= 100;

		iTechTradeKnownPercent *= AI_getTechMonopolyValue(eTech, eTeam);
		iTechTradeKnownPercent /= 100;

		if ((iPossibleKnownCount > 0) ? (((iKnownCount * 100) / iPossibleKnownCount) < iTechTradeKnownPercent) : (iTechTradeKnownPercent > 0))
		{
			return DENIAL_TECH_MONOPOLY;
		}
	}

	for (int iI = 0; iI < GC.getNumUnitInfos(); iI++)
	{
		if (isTechRequiredForUnit(eTech, ((UnitTypes)iI)))
		{
			if (isWorldUnitClass((UnitClassTypes)(GC.getUnitInfo((UnitTypes)iI).getUnitClassType())))
			{
				if (getUnitClassMaking((UnitClassTypes)(GC.getUnitInfo((UnitTypes)iI).getUnitClassType())) > 0)
				{
					return DENIAL_MYSTERY;
				}
			}
		}
	}

	for (int iI = 0; iI < GC.getNumBuildingInfos(); iI++)
	{
		if (isTechRequiredForBuilding(eTech, ((BuildingTypes)iI)))
		{
			if (isWorldWonderClass((BuildingClassTypes)(GC.getBuildingInfo((BuildingTypes)iI).getBuildingClassType())))
			{
				if (getBuildingClassMaking((BuildingClassTypes)(GC.getBuildingInfo((BuildingTypes)iI).getBuildingClassType())) > 0)
				{
					return DENIAL_MYSTERY;
				}
			}
		}
	}

	for (int iI = 0; iI < GC.getNumProjectInfos(); iI++)
	{
		if (GC.getProjectInfo((ProjectTypes)iI).getTechPrereq() == eTech)
		{
			if (isWorldProject((ProjectTypes)iI))
			{
				if (getProjectMaking((ProjectTypes)iI) > 0)
				{
					return DENIAL_MYSTERY;
				}
			}

			for (int iJ = 0; iJ < GC.getNumVictoryInfos(); iJ++)
			{
				if (GC.getGameINLINE().isVictoryValid((VictoryTypes)iJ))
				{
					if (GC.getProjectInfo((ProjectTypes)iI).getVictoryThreshold((VictoryTypes)iJ))
					{
						return DENIAL_VICTORY;
					}
				}
			}
		}
	}

	return NO_DENIAL;
}


int CvTeamAI::AI_mapTradeVal(TeamTypes eTeam) const
{
	CvPlot* pLoopPlot;
	int iValue;
	int iI;

	FAssertMsg(eTeam != getID(), "shouldn't call this function on ourselves");

	iValue = 0;

	for (iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
	{
		pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);

		if (!(pLoopPlot->isRevealed(getID(), false)) && pLoopPlot->isRevealed(eTeam, false))
		{
			if (pLoopPlot->isWater())
			{
				iValue++;
			}
			else
			{
				iValue += 5;
			}
		}
	}

	iValue /= 10;

	if (GET_TEAM(eTeam).isVassal(getID()))
	{
		iValue /= 2;
	}

	iValue -= (iValue % GC.getDefineINT("DIPLOMACY_VALUE_REMAINDER"));

	if (isHuman())
	{
		return std::max(iValue, GC.getDefineINT("DIPLOMACY_VALUE_REMAINDER"));
	}
	else
	{
		return iValue;
	}
}


DenialTypes CvTeamAI::AI_mapTrade(TeamTypes eTeam) const
{
	PROFILE_FUNC();

	AttitudeTypes eAttitude;
	int iI;

	FAssertMsg(eTeam != getID(), "shouldn't call this function on ourselves");

	if (isHuman())
	{
		return NO_DENIAL;
	}

	if (isVassal(eTeam))
	{
		return NO_DENIAL;
	}

	if (isAtWar(eTeam))
	{
		return NO_DENIAL;
	}

	if (AI_getWorstEnemy() == eTeam)
	{
		return DENIAL_WORST_ENEMY;
	}

	eAttitude = AI_getAttitude(eTeam);

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				if (eAttitude <= GC.getLeaderHeadInfo(GET_PLAYER((PlayerTypes)iI).getPersonalityType()).getMapRefuseAttitudeThreshold())
				{
					return DENIAL_ATTITUDE;
				}
			}
		}
	}

	return NO_DENIAL;
}


int CvTeamAI::AI_vassalTradeVal(TeamTypes eTeam) const
{
	FAssertMsg(eTeam != getID(), "shouldn't call this function on ourselves");

	return AI_surrenderTradeVal(eTeam);
}


DenialTypes CvTeamAI::AI_vassalTrade(TeamTypes eTeam) const
{
	FAssertMsg(eTeam != getID(), "shouldn't call this function on ourselves");

	CvTeamAI& kMasterTeam = GET_TEAM(eTeam);

	for (int iLoopTeam = 0; iLoopTeam < MAX_TEAMS; iLoopTeam++)
	{
		CvTeam& kLoopTeam = GET_TEAM((TeamTypes)iLoopTeam);
		if (kLoopTeam.isAlive() && iLoopTeam != getID() && iLoopTeam != kMasterTeam.getID())
		{
			if (!kLoopTeam.isAtWar(kMasterTeam.getID()) && kLoopTeam.isAtWar(getID()))
			{
				if (kMasterTeam.isForcePeace((TeamTypes)iLoopTeam) || !kMasterTeam.canChangeWarPeace((TeamTypes)iLoopTeam))
				{
					if (!kLoopTeam.isAVassal())
					{
						return DENIAL_WAR_NOT_POSSIBLE_YOU;
					}
				}

				if (!kMasterTeam.isHuman())
				{
					DenialTypes eWarDenial = kMasterTeam.AI_declareWarTrade((TeamTypes)iLoopTeam, getID(), true);
					if (NO_DENIAL != eWarDenial)
					{
						return DENIAL_WAR_NOT_POSSIBLE_YOU;
					}
				}
			}
			else if (kLoopTeam.isAtWar(kMasterTeam.getID()) && !kLoopTeam.isAtWar(getID()))
			{
				if (!kMasterTeam.canChangeWarPeace((TeamTypes)iLoopTeam))
				{
					if (!kLoopTeam.isAVassal())
					{
						return DENIAL_PEACE_NOT_POSSIBLE_YOU;
					}
				}

				if (!kMasterTeam.isHuman())
				{
					DenialTypes ePeaceDenial = kMasterTeam.AI_makePeaceTrade((TeamTypes)iLoopTeam, getID());
					if (NO_DENIAL != ePeaceDenial)
					{
						return DENIAL_PEACE_NOT_POSSIBLE_YOU;
					}
				}
			}
		}
	}

	// K-Mod. some conditions moved from AI_surrenderTrade. (see the comments there)
	for (TeamTypes i = (TeamTypes)0; i < MAX_CIV_TEAMS; i=(TeamTypes)(i+1))
	{
		const CvTeam& kLoopTeam = GET_TEAM(i);

		if (!kLoopTeam.isAlive() || i == getID() && i == kMasterTeam.getID())
			continue;

		if (kMasterTeam.isAtWar(i) && !isAtWar(i))
		{
			if (isForcePeace(i) || !canChangeWarPeace(i))
			{
				if (!kLoopTeam.isAVassal())
				{
					return DENIAL_WAR_NOT_POSSIBLE_US;
				}
			}
		}
		else if (!kMasterTeam.isAtWar(i) && isAtWar(i))
		{
			// K-Mod. (peace-vassal deals cause the new master to declare war)
			if (!kLoopTeam.isAVassal())
			{
				if (kMasterTeam.isForcePeace(i) || !kMasterTeam.canChangeWarPeace(i))
				{
					return DENIAL_WAR_NOT_POSSIBLE_YOU;
				}

				// K-Mod. New rule: AI civs won't accept a vassal if it would mean joining a war they would never otherwise join.
				// Note: the following denials actually come from kMasterTeam, not this team. This is just the only way to do it.
				if (!kMasterTeam.isHuman())
				{
					AttitudeTypes eAttitude = kMasterTeam.AI_getAttitude(i);

					if (kMasterTeam.AI_noWarAttitudeProb(eAttitude) >= 100)
					{
						// ok, so we wouldn't independantly choose this war, but could we be bought into it?
						// If any of our team would refuse, then the team refuses. (cf. AI_declareWarTrade)
						for (PlayerTypes j = (PlayerTypes)0; j < MAX_CIV_PLAYERS; j=(PlayerTypes)(j+1))
						{
							const CvPlayerAI& kLoopPlayer = GET_PLAYER(j);
							if (kLoopPlayer.isAlive() && kLoopPlayer.getTeam() == getID())
							{
								if (eAttitude > GC.getLeaderHeadInfo(kLoopPlayer.getPersonalityType()).getDeclareWarThemRefuseAttitudeThreshold())
								{
									return DENIAL_ATTITUDE_THEM;
								}
							}
						}
					}
					// (note: if we're concerned about AI_startWarVal, then that should be checked in the trade value part; not the trade denial part.)
				}
			}
			//
		}
	}
	// K-Mod end

	return AI_surrenderTrade(eTeam);
}


int CvTeamAI::AI_surrenderTradeVal(TeamTypes eTeam) const
{
	FAssertMsg(eTeam != getID(), "shouldn't call this function on ourselves");

	return 0;
}


DenialTypes CvTeamAI::AI_surrenderTrade(TeamTypes eTeam, int iPowerMultiplier) const
{
	PROFILE_FUNC();

	FAssertMsg(eTeam != getID(), "shouldn't call this function on ourselves");

	CvTeam& kMasterTeam = GET_TEAM(eTeam);

	// K-Mod. I've disabled the denial for "war not possible"
	// In K-Mod, surrendering overrules existing peace treaties - just like defensive pacts overrule peace treaties.
	// However, for voluntary vassals, the treaties still apply. So I've moved the code to there.
	/* original bts code
	for (int iLoopTeam = 0; iLoopTeam < MAX_TEAMS; iLoopTeam++)
	{
		CvTeam& kLoopTeam = GET_TEAM((TeamTypes)iLoopTeam);
		if (kLoopTeam.isAlive() && iLoopTeam != getID() && iLoopTeam != kMasterTeam.getID())
		{
			if (kLoopTeam.isAtWar(kMasterTeam.getID()) && !kLoopTeam.isAtWar(getID()))
			{
				if (isForcePeace((TeamTypes)iLoopTeam) || !canChangeWarPeace((TeamTypes)iLoopTeam))
				{
					if (!kLoopTeam.isAVassal())
					{
						return DENIAL_WAR_NOT_POSSIBLE_US;
					}
				}
			}
			else if (!kLoopTeam.isAtWar(kMasterTeam.getID()) && kLoopTeam.isAtWar(getID()))
			{
				if (!canChangeWarPeace((TeamTypes)iLoopTeam))
				{
					if (!kLoopTeam.isAVassal())
					{
						return DENIAL_PEACE_NOT_POSSIBLE_US;
					}
				}
			}
		}
	} */
	// K-Mod. However, "peace not possible" should still be checked here --  but only if this is a not a peace-vassal deal!
	if (isAtWar(eTeam))
	{
		for (TeamTypes i = (TeamTypes)0; i < MAX_CIV_TEAMS; i=(TeamTypes)(i+1))
		{
			const CvTeam& kLoopTeam = GET_TEAM(i);
			if (!kLoopTeam.isAlive() || i == getID() && i == kMasterTeam.getID())
				continue;

			if (isAtWar(i) && !kMasterTeam.isAtWar(i))
			{
				if (!canChangeWarPeace(i))
				{
					if (!kLoopTeam.isAVassal())
					{
						return DENIAL_PEACE_NOT_POSSIBLE_US;
					}
				}
			}
		}
	}
	// K-Mod end

	if (isHuman() && kMasterTeam.isHuman())
	{
		return NO_DENIAL;
	}

	int iAttitudeModifier = 0;

	if (!GET_TEAM(eTeam).isParent(getID()))
	{
		int iPersonalityModifier = 0;
		int iMembers = 0;
		for (int iI = 0; iI < MAX_PLAYERS; iI++)
		{
			if (GET_PLAYER((PlayerTypes)iI).isAlive())
			{
				if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
				{
					iPersonalityModifier += GC.getLeaderHeadInfo(GET_PLAYER((PlayerTypes)iI).getPersonalityType()).getVassalPowerModifier();
					++iMembers;
				}
			}
		}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      06/03/09                                jdog5000      */
/*                                                                                              */
/* War Strategy AI                                                                              */
/************************************************************************************************/
/* original BTS code
		int iTotalPower = GC.getGameINLINE().countTotalCivPower();
		int iAveragePower = iTotalPower / std::max(1, GC.getGameINLINE().countCivTeamsAlive());
*/

		int iTotalPower = 0;
		int iNumNonVassals = 0;
		for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
		{
			CvTeam& kTeam = GET_TEAM((TeamTypes) iI);
			if (kTeam.isAlive() && !(kTeam.isMinorCiv()))
			{
				if (kTeam.isCapitulated())
				{
					// Count capitulated vassals as a fractional add to their master's power
					iTotalPower += (2*kTeam.getPower(false))/5;
				}
				else
				{
					iTotalPower += kTeam.getPower(false);
					iNumNonVassals++;
				}
			}
		}
		int iAveragePower = iTotalPower / std::max(1, iNumNonVassals);
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
		int iMasterPower = kMasterTeam.getPower(false);
		int iOurPower = getPower(true); // K-Mod (this value is used a bunch of times separately)
		int iVassalPower = (iOurPower * (iPowerMultiplier + iPersonalityModifier / std::max(1, iMembers))) / 100;

		if (isAtWar(eTeam))
		{
			int iTheirSuccess = std::max(10, GET_TEAM(eTeam).AI_getWarSuccess(getID()));
			int iOurSuccess = std::max(10, AI_getWarSuccess(eTeam));
			int iOthersBestSuccess = 0;
			for (int iTeam = 0; iTeam < MAX_CIV_TEAMS; ++iTeam)
			{
				if (iTeam != eTeam && iTeam != getID())
				{
					CvTeam& kLoopTeam = GET_TEAM((TeamTypes)iTeam);

					if (kLoopTeam.isAlive() && kLoopTeam.isAtWar(getID()))
					{
						int iSuccess = kLoopTeam.AI_getWarSuccess(getID());
						if (iSuccess > iOthersBestSuccess)
						{
							iOthersBestSuccess = iSuccess;
						}
					}
				}
			}

			// Discourage capitulation to a team that has not done the most damage
			if (iTheirSuccess < iOthersBestSuccess)
			{
				iOurSuccess += iOthersBestSuccess - iTheirSuccess;
			}

			iMasterPower = (2 * iMasterPower * iTheirSuccess) / (iTheirSuccess + iOurSuccess);

			if (AI_getWorstEnemy() == eTeam)
			{
				iMasterPower *= 3;
				iMasterPower /= 4;
			}
		}
		else
		{
			if (!GET_TEAM(eTeam).AI_isLandTarget(getID()))
			{
				iMasterPower /= 2;
			}
		}

		// K-Mod. (condition moved here from lower down; for efficiency.)
		if (3 * iVassalPower > 2 * iMasterPower)
			return DENIAL_POWER_US;
		// K-Mod end

		for (int iLoopTeam = 0; iLoopTeam < MAX_CIV_TEAMS; iLoopTeam++)
		{
			if (iLoopTeam != getID())
			{
				CvTeamAI& kLoopTeam = GET_TEAM((TeamTypes)iLoopTeam);

				if (kLoopTeam.isAlive())
				{
					if (kLoopTeam.AI_isLandTarget(getID()))
					{
						if (iLoopTeam != eTeam)
						{
							int iLoopPower = kLoopTeam.getPower(true); // K-Mod
							if (iLoopPower > iOurPower)
							{
								//if (kLoopTeam.isAtWar(eTeam) && !kLoopTeam.isAtWar(getID()))
								if (kLoopTeam.isAtWar(eTeam) && !kLoopTeam.isAtWar(getID()) && (!isAtWar(eTeam) || iMasterPower < 2 * iLoopPower)) // K-Mod
								{
									return DENIAL_POWER_YOUR_ENEMIES;
								}

								iAveragePower = (2 * iAveragePower * iLoopPower) / std::max(1, iLoopPower + iOurPower);

								//iAttitudeModifier += (3 * kLoopTeam.getPower(true)) / std::max(1, getPower(true)) - 2;
								iAttitudeModifier += (6 * iLoopPower / std::max(1, iOurPower) - 5)/2; // K-Mod. (effectively -2.5 instead of 2)
							}

							if (!kLoopTeam.isAtWar(eTeam) && kLoopTeam.isAtWar(getID()))
							{
								//iAveragePower = (iAveragePower * (getPower(true) + GET_TEAM(eTeam).getPower(false))) / std::max(1, getPower(true));
								iAveragePower = iAveragePower * (iOurPower + iMasterPower) / std::max(1, iOurPower + std::max(iOurPower, iLoopPower)); // K-Mod
							}
						}
					}

					if (!atWar(getID(), eTeam))
					{
						if (kLoopTeam.isAtWar(eTeam) && !kLoopTeam.isAtWar(getID()))
						{
							DenialTypes eDenial = AI_declareWarTrade((TeamTypes)iLoopTeam, eTeam, false);
							if (eDenial != NO_DENIAL)
							{
								return eDenial;
							}
						}
					}
				}
			}
		}

		if (!isVassal(eTeam) && canVassalRevolt(eTeam))
		{
			return DENIAL_POWER_US;
		}

		// if (iVassalPower > iAveragePower || 3 * iVassalPower > 2 * iMasterPower)
		if (5*iVassalPower > 4*iAveragePower) // K-Mod. (second condition already checked)
		{
			return DENIAL_POWER_US;
		}

		for (int i = 0; i < GC.getNumVictoryInfos(); i++)
		{
			bool bPopulationThreat = true;
			if (GC.getGameINLINE().getAdjustedPopulationPercent((VictoryTypes)i) > 0)
			{
				bPopulationThreat = false;

				int iThreshold = GC.getGameINLINE().getTotalPopulation() * GC.getGameINLINE().getAdjustedPopulationPercent((VictoryTypes)i);
				if (400 * getTotalPopulation(!isAVassal()) > 3 * iThreshold)
				{
					return DENIAL_VICTORY;
				}

				if (!atWar(getID(), eTeam))
				{
					if (400 * (getTotalPopulation(isAVassal()) + GET_TEAM(eTeam).getTotalPopulation()) > 3 * iThreshold)
					{
						bPopulationThreat = true;
					}
				}
			}

			bool bLandThreat = true;
			if (GC.getGameINLINE().getAdjustedLandPercent((VictoryTypes)i) > 0)
			{
				bLandThreat = false;

				int iThreshold = GC.getMapINLINE().getLandPlots() * GC.getGameINLINE().getAdjustedLandPercent((VictoryTypes)i);
				if (400 * getTotalLand(!isAVassal()) > 3 * iThreshold)
				{
					return DENIAL_VICTORY;
				}

				if (!atWar(getID(), eTeam))
				{
					if (400 * (getTotalLand(isAVassal()) + GET_TEAM(eTeam).getTotalLand()) > 3 * iThreshold)
					{
						bLandThreat = true;
					}
				}
			}

			if (GC.getGameINLINE().getAdjustedPopulationPercent((VictoryTypes)i) > 0 || GC.getGameINLINE().getAdjustedLandPercent((VictoryTypes)i) > 0)
			{
				if (bLandThreat && bPopulationThreat)
				{
					return DENIAL_POWER_YOU;
				}
			}
		}
	}

	if (!isAtWar(eTeam))
	{
		if (!GET_TEAM(eTeam).isParent(getID()))
		{
			if (AI_getWorstEnemy() == eTeam)
			{
				return DENIAL_WORST_ENEMY;
			}

			if (!AI_hasCitiesInPrimaryArea(eTeam) && AI_calculateAdjacentLandPlots(eTeam) == 0)
			{
				return DENIAL_TOO_FAR;
			}
		}

		AttitudeTypes eAttitude = AI_getAttitude(eTeam, false);

		AttitudeTypes eModifiedAttitude = CvPlayerAI::AI_getAttitudeFromValue(AI_getAttitudeVal(eTeam, false) + iAttitudeModifier);

		for (int iI = 0; iI < MAX_PLAYERS; iI++)
		{
			if (GET_PLAYER((PlayerTypes)iI).isAlive())
			{
				if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
				{
					if (eAttitude <= ATTITUDE_FURIOUS)
					{
						return DENIAL_ATTITUDE;
					}

					if (eModifiedAttitude <= GC.getLeaderHeadInfo(GET_PLAYER((PlayerTypes)iI).getPersonalityType()).getVassalRefuseAttitudeThreshold())
					{
						return DENIAL_ATTITUDE;
					}
				}
			}
		}
	}
	else
	{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      12/07/09                                jdog5000      */
/*                                                                                              */
/* Diplomacy AI                                                                                 */
/************************************************************************************************/
/* original BTS code
		if (AI_getWarSuccess(eTeam) + 4 * GC.getDefineINT("WAR_SUCCESS_CITY_CAPTURING") > GET_TEAM(eTeam).AI_getWarSuccess(getID()))
		{
			return DENIAL_JOKING;
		}
*/
		// Scale better for small empires, particularly necessary if WAR_SUCCESS_CITY_CAPTURING > 10
		if (AI_getWarSuccess(eTeam) + std::min(getNumCities(), 4) * GC.getWAR_SUCCESS_CITY_CAPTURING() > GET_TEAM(eTeam).AI_getWarSuccess(getID()))
		{
			return DENIAL_JOKING;
		}

		if( !kMasterTeam.isHuman() )
		{
			if( !(GET_TEAM(kMasterTeam.getID()).AI_acceptSurrender(getID())) )
			{
				return DENIAL_JOKING;
			}
		}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
	}
	
	return NO_DENIAL;
}

// K-Mod
int CvTeamAI::AI_countMembersWithStrategy(int iStrategy) const
{
	int iCount = 0;
	for (int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; iPlayer++)
	{
		if (GET_PLAYER((PlayerTypes)iPlayer).getTeam() == getID())
		{
			if (GET_PLAYER((PlayerTypes)iPlayer).isAlive())
			{
				if (GET_PLAYER((PlayerTypes)iPlayer).AI_isDoStrategy(iStrategy))
				{
					iCount++;
				}
			}
		}
	}

	return iCount;
}
// K-Mod end

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      03/20/10                                jdog5000      */
/*                                                                                              */
/* Victory Strategy AI                                                                          */
/************************************************************************************************/
bool CvTeamAI::AI_isAnyMemberDoVictoryStrategy( int iVictoryStrategy ) const
{
	for( int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; iPlayer++ )
	{
		if( GET_PLAYER((PlayerTypes)iPlayer).getTeam() == getID() )
		{
			if( GET_PLAYER((PlayerTypes)iPlayer).isAlive() )
			{
				if( GET_PLAYER((PlayerTypes)iPlayer).AI_isDoVictoryStrategy(iVictoryStrategy) )
				{
					return true;
				}
			}
		}
	}

	return false;
}

bool CvTeamAI::AI_isAnyMemberDoVictoryStrategyLevel4() const
{
	for( int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; iPlayer++ )
	{
		if( GET_PLAYER((PlayerTypes)iPlayer).getTeam() == getID() )
		{
			if( GET_PLAYER((PlayerTypes)iPlayer).isAlive() )
			{
				if( GET_PLAYER((PlayerTypes)iPlayer).AI_isDoVictoryStrategyLevel4() )
				{
					return true;
				}
			}
		}
	}

	return false;
}

bool CvTeamAI::AI_isAnyMemberDoVictoryStrategyLevel3() const
{
	for( int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; iPlayer++ )
	{
		if( GET_PLAYER((PlayerTypes)iPlayer).getTeam() == getID() )
		{
			if( GET_PLAYER((PlayerTypes)iPlayer).isAlive() )
			{
				if( GET_PLAYER((PlayerTypes)iPlayer).AI_isDoVictoryStrategyLevel3() )
				{
					return true;
				}
			}
		}
	}

	return false;
}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

// K-Mod. return a rating of our war success between -99 and 99.
// -99 means we losing and have very little hope of surviving. 99 means we are soundly defeating our enemies. Zero is neutral (eg. no wars being fought).
int CvTeamAI::AI_getWarSuccessRating() const
{
	PROFILE_FUNC();
	// (Based on my code for Force Peace diplomacy voting.)

	int iMilitaryUnits = 0;
	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		const CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iI);
		if (kLoopPlayer.getTeam() == getID())
		{
			iMilitaryUnits += kLoopPlayer.getNumMilitaryUnits();
		}
	}
	int iSuccessScale = iMilitaryUnits * GC.getDefineINT("WAR_SUCCESS_ATTACKING") / 5;

	int iThisTeamPower = getPower(true);
	int iScore = 0;

	for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
	{
		const CvTeam& kLoopTeam = GET_TEAM((TeamTypes)iI);
		if (iI != getID() && isAtWar((TeamTypes)iI) && kLoopTeam.isAlive() && !kLoopTeam.isAVassal())
		{
			int iThisTeamSuccess = AI_getWarSuccess((TeamTypes)iI);
			int iOtherTeamSuccess = kLoopTeam.AI_getWarSuccess(getID());

			int iOtherTeamPower = kLoopTeam.getPower(true);

			iScore += (iThisTeamSuccess+iSuccessScale) * iThisTeamPower;
			iScore -= (iOtherTeamSuccess+iSuccessScale) * iOtherTeamPower;
		}
	}
	iScore = range((100*iScore)/std::max(1, iThisTeamPower*iSuccessScale*5), -99, 99);
	return iScore;
}
// K-Mod end

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      03/20/10                                jdog5000      */
/*                                                                                              */
/* War Strategy AI                                                                              */
/************************************************************************************************/
/// \brief Compute power of enemies as percentage of our power.
///
///
int CvTeamAI::AI_getEnemyPowerPercent( bool bConsiderOthers ) const
{
	int iEnemyPower = 0;

	for( int iI = 0; iI < MAX_CIV_TEAMS; iI++ )
	{
		if( iI != getID() )
		{
			if( GET_TEAM((TeamTypes)iI).isAlive() && isHasMet((TeamTypes)iI) )
			{
				if( isAtWar((TeamTypes)iI) )
				{
					int iTempPower = 220 * GET_TEAM((TeamTypes)iI).getPower(false);
					iTempPower /= (AI_hasCitiesInPrimaryArea((TeamTypes)iI) ? 2 : 3);
					iTempPower /= (GET_TEAM((TeamTypes)iI).isMinorCiv() ? 3 : 1);
					iTempPower /= std::max(1, (bConsiderOthers ? GET_TEAM((TeamTypes)iI).getAtWarCount(true,true) : 1));
					iEnemyPower += iTempPower;
				}
				else if( AI_isChosenWar((TeamTypes)iI) )
				{
					// Haven't declared war yet
					int iTempPower = 240 * GET_TEAM((TeamTypes)iI).getDefensivePower(getID());
					iTempPower /= (AI_hasCitiesInPrimaryArea((TeamTypes)iI) ? 2 : 3);
					iTempPower /= 1 + (bConsiderOthers ? GET_TEAM((TeamTypes)iI).getAtWarCount(true,true) : 0);
					iEnemyPower += iTempPower;
				}
			}
		}
	}
	//return (iEnemyPower/std::max(1, (isAVassal() ? getCurrentMasterPower(true) : getPower(true))));
	// K-Mod - Lets not rely too much on our vassals...
	int iOurPower = getPower(false);
	const CvTeam& kMasterTeam = GET_TEAM(getMasterTeam());
	iOurPower += kMasterTeam.getPower(true);
	iOurPower /= 2;
	return iEnemyPower/std::max(1, iOurPower);
	// K-Mod end
}

// K-Mod
int CvTeamAI::AI_getAirPower() const
{
	int iTotalPower = 0;

	for (int iI = 0; iI < GC.getNumUnitClassInfos(); iI++)
	{
		// Since units of each class are counted per team rather than units of each type, just assume the default unit type.
		UnitTypes eLoopUnit = (UnitTypes)GC.getUnitClassInfo((UnitClassTypes)iI).getDefaultUnitIndex();
		if (eLoopUnit != NO_UNIT)
		{
			const CvUnitInfo& kUnitInfo = GC.getUnitInfo(eLoopUnit);

			if (kUnitInfo.getDomainType() == DOMAIN_AIR && kUnitInfo.getAirCombat() > 0)
			{
				iTotalPower += getUnitClassCount((UnitClassTypes)iI) * kUnitInfo.getPowerValue();
			}
		}
	}

	return iTotalPower;
}

/// \brief Sum up air power of enemies plus average of other civs we've met.
///
// K-Mod: I've rewritten this function to loop over unit classes rather than unit types.
// This is because a loop over unit types will double-count if there are two units in the same class.
int CvTeamAI::AI_getRivalAirPower( ) const
{
	// Count enemy air units, not just those visible to us
	int iRivalAirPower = 0;
	int iEnemyAirPower = 0;
	int iTeamCount = 0;

	for (int iI = 0; iI < GC.getNumUnitClassInfos(); iI++)
	{
		// Since units of each class are counted per team rather than units of each type, just assume the default unit type.
		UnitTypes eLoopUnit = (UnitTypes)GC.getUnitClassInfo((UnitClassTypes)iI).getDefaultUnitIndex();
		if (eLoopUnit != NO_UNIT)
		{
			const CvUnitInfo& kUnitInfo = GC.getUnitInfo(eLoopUnit);

			if (kUnitInfo.getDomainType() == DOMAIN_AIR && kUnitInfo.getAirCombat() > 0)
			{
				for (int iTeam = 0; iTeam < MAX_CIV_TEAMS; iTeam++)
				{
					if (iTeam != getID() && GET_TEAM((TeamTypes)iTeam).isAlive()
						&& isHasMet((TeamTypes)iTeam) && !GET_TEAM((TeamTypes)iTeam).isMinorCiv())
					{
						iTeamCount++;

						int iUnitPower = kUnitInfo.getPowerValue() * GET_TEAM((TeamTypes)iTeam).getUnitClassCount((UnitClassTypes)iI);

						iRivalAirPower += iUnitPower;
						if (AI_getWarPlan((TeamTypes)iTeam) != NO_WARPLAN)
							iEnemyAirPower += iUnitPower;
					}
				}
			}
		}
	}
	return iEnemyAirPower + iRivalAirPower / std::max(1, iTeamCount);
}

// K-Mod
bool CvTeamAI::AI_refusePeace(TeamTypes ePeaceTeam) const
{
	// Refuse peace if we need the war for our conquest / domination victory.
	if (!isHuman() &&
		AI_isAnyMemberDoVictoryStrategy(AI_VICTORY_CONQUEST4 | AI_VICTORY_DOMINATION4) &&
		(AI_isChosenWar(ePeaceTeam) || getAtWarCount(true, true) == 1) &&
		AI_getWarSuccessRating() > 0)
	{
		return true;
	}
	return false;
}
// K-Mod end

// the following is a bbai function which has been edited for K-Mod (most of the K-Mod changes are unmarked)
bool CvTeamAI::AI_acceptSurrender( TeamTypes eSurrenderTeam ) const
{
	PROFILE_FUNC();

	const CvTeamAI& kSurrenderTeam = GET_TEAM(eSurrenderTeam);

	if( isHuman() )
	{
		return true;
	}

	if( !isAtWar(eSurrenderTeam) )
	{
		return true;
	}

	if (kSurrenderTeam.AI_isAnyMemberDoVictoryStrategy(AI_VICTORY_SPACE3 | AI_VICTORY_CULTURE3))
	{
		// Capturing capital or Apollo city will stop space
		// Capturing top culture cities will stop culture
		return false;
	}

	// Check for whether another team has won enough to cause capitulation
	bool bMightCapToOther = false; // K-Mod
	for( int iI = 0; iI < MAX_CIV_TEAMS; iI++ )
	{
		if (GET_TEAM((TeamTypes)iI).isAlive())
		{
			if (iI != getID() && !(GET_TEAM((TeamTypes)iI).isVassal(getID())) )
			{
				if (kSurrenderTeam.isAtWar((TeamTypes)iI))
				{
					if (kSurrenderTeam.AI_getAtWarCounter((TeamTypes)iI) >= 10)
					{
						if( (kSurrenderTeam.AI_getWarSuccess((TeamTypes)iI) + std::min(kSurrenderTeam.getNumCities(), 4) * GC.getWAR_SUCCESS_CITY_CAPTURING()) < GET_TEAM((TeamTypes)iI).AI_getWarSuccess(eSurrenderTeam))
						{
							//return true;
							// K-Mod: that's not the only capitulation condition. I might revise it later, but in the mean time I'll just relax the effect.
							bMightCapToOther = true;
							break;
							//
						}
					}
				}
			}
		}
	}

	int iValuableCities = 0;
	int iCitiesThreatenedByUs = 0;
	int iValuableCitiesThreatenedByUs = 0;
	int iCitiesThreatenedByOthers = 0;

	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		if( GET_PLAYER((PlayerTypes)iI).getTeam() == eSurrenderTeam && GET_PLAYER((PlayerTypes)iI).isAlive() )
		{
			int iLoop;
			for (CvCity* pLoopCity = GET_PLAYER((PlayerTypes)iI).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER((PlayerTypes)iI).nextCity(&iLoop))
			{
				bool bValuable = false;

				if( pLoopCity->isHolyCity() )
				{
					bValuable = true;
				}
				else if( pLoopCity->isHeadquarters() )
				{
					bValuable = true;
				}
				else if( pLoopCity->hasActiveWorldWonder() )
				{
					bValuable = true;
				}
				else if( AI_isPrimaryArea(pLoopCity->area()) && (kSurrenderTeam.countNumCitiesByArea(pLoopCity->area()) < 3) )
				{
					bValuable = true;
				}
				else if( pLoopCity->isCapital() && (kSurrenderTeam.getNumCities() > kSurrenderTeam.getNumMembers() || countNumCitiesByArea(pLoopCity->area()) > 0) )
				{
					bValuable = true;
				}
				else
				{
					// Valuable terrain bonuses
					CvPlot* pLoopPlot = NULL;
					for (int iJ = 0; iJ < NUM_CITY_PLOTS; iJ++)
					{
						pLoopPlot = plotCity(pLoopCity->getX_INLINE(), pLoopCity->getY_INLINE(), iJ);

						if (pLoopPlot != NULL)
						{
							BonusTypes eBonus = pLoopPlot->getNonObsoleteBonusType(getID());
							if ( eBonus != NO_BONUS)
							{
								if(GET_PLAYER(getLeaderID()).AI_bonusVal(eBonus, 1) > 15)
								{
									bValuable = true;
									break;
								}
							}
						}
					}
				}

				/*
				int iOwnerPower = GET_PLAYER((PlayerTypes)iI).AI_getOurPlotStrength(pLoopCity->plot(), 2, true, false);
				int iOurPower = AI_getOurPlotStrength(pLoopCity->plot(), 2, false, false, true);
				int iOtherPower = GET_PLAYER((PlayerTypes)iI).AI_getEnemyPlotStrength(pLoopCity->plot(), 2, false, false) - iOurPower; */
				// K-Mod. Note. my new functions are not quite the same as the old.
				// a) this will not count vassals in "our power". b) it will only count forces that can been seen by the player calling the function.
				const CvPlayerAI& kLoopPlayer = GET_PLAYER((PlayerTypes)iI);
				int iOwnerPower = kLoopPlayer.AI_localDefenceStrength(pLoopCity->plot(), kLoopPlayer.getTeam(), DOMAIN_LAND, 2);
				int iOurPower = GET_PLAYER(getLeaderID()).AI_localAttackStrength(pLoopCity->plot(), getID());
				int iOtherPower = kLoopPlayer.AI_localAttackStrength(pLoopCity->plot(), NO_TEAM) - iOurPower;
				// K-Mod end

				if( iOtherPower > iOwnerPower )
				{
					iCitiesThreatenedByOthers++;
				}

				if (iOurPower > iOwnerPower)
				{
					iCitiesThreatenedByUs++;
					if( bValuable )
					{
						iValuableCities++;
						iValuableCitiesThreatenedByUs++;
						continue;
					}
				}

				if( bValuable && pLoopCity->getHighestPopulation() < 5 )
				{
					bValuable = false;
				}

				if( bValuable )
				{
					if( AI_isPrimaryArea(pLoopCity->area()) )
					{
						iValuableCities++;
					}
					else
					{
						for (int iJ = 0; iJ < MAX_PLAYERS; iJ++)
						{
							if (GET_PLAYER((PlayerTypes)iJ).isAlive())
							{
								if (GET_PLAYER((PlayerTypes)iJ).getTeam() == getID())
								{
									if( pLoopCity->AI_playerCloseness((PlayerTypes)iJ) > 5 )
									{
										iValuableCities++;
										break;
									}
								}
							}
						}
					}
				}
			}
		}
	}

	int iOurWarSuccessRating = AI_getWarSuccessRating();
	if( iOurWarSuccessRating < -30 )
	{
		// We're doing badly overall, need to be done with this war and gain an ally
		return true;
	}

	if( iValuableCitiesThreatenedByUs > 0 )
	{
		// Press for capture of valuable city
		return false;
	}

	if (iCitiesThreatenedByOthers > (1 + iCitiesThreatenedByUs/2) && (bMightCapToOther || iCitiesThreatenedByOthers >= iValuableCities)) // K-Mod
	{
		// Keep others from capturing spoils, but let it go if surrender civ is too small to care about
		/* original BBAI code
		if( 6*(iValuableCities + kSurrenderTeam.getNumCities()) > getNumCities() )
		{
			return true;
		} */
		// K-Mod
		if (5*iValuableCities + 3*(kSurrenderTeam.getNumCities()-iCitiesThreatenedByUs) > getNumCities())
			return true;
		// K-Mod end
	}

	// If we're low on the totem poll, accept so enemies don't drag anyone else into war with us
	// Top rank is 0, second is 1, etc.
	if ((bMightCapToOther || iOurWarSuccessRating < 60) && GC.getGameINLINE().getTeamRank(getID()) > 1 + GC.getGameINLINE().countCivTeamsAlive()/3)
	{
		return true;
	}

	if (iOurWarSuccessRating < 50)
	{
		// Accept if we have other wars to fight
		for (TeamTypes i = (TeamTypes)0; i < MAX_CIV_TEAMS; i=(TeamTypes)(i+1))
		{
			const CvTeam& kLoopTeam = GET_TEAM(i);
			if (isAtWar(i) && kLoopTeam.isAlive() && !kLoopTeam.isMinorCiv() && i != eSurrenderTeam && !kLoopTeam.isVassal(eSurrenderTeam))
			{
				if (kLoopTeam.AI_getWarSuccess(getID()) > 5*GC.getDefineINT("WAR_SUCCESS_ATTACKING"))
				{
					return true;
				}
			}
		}
	}

	// War weariness
	int iWearinessThreshold = (GC.getGameINLINE().isOption(GAMEOPTION_AGGRESSIVE_AI) ? 300 : 240);
	if (!bMightCapToOther)
	{
		iWearinessThreshold += 20*iValuableCities + 30*iCitiesThreatenedByUs;
		iWearinessThreshold += 10*std::max(0, GC.getWorldInfo(GC.getMapINLINE().getWorldSize()).getTargetNumCities() - kSurrenderTeam.getNumCities()); // (to help finish off small civs)
	}

	for (PlayerTypes i = (PlayerTypes)0; i < MAX_CIV_PLAYERS; i=(PlayerTypes)(i+1))
	{
		const CvPlayer& kLoopPlayer = GET_PLAYER(i);

		if (kLoopPlayer.isAlive() && kLoopPlayer.getTeam() == getID())
		{
			if (kLoopPlayer.getWarWearinessPercentAnger() > iWearinessThreshold) // K-Mod note: it isn't really "percent". The API lies.
			{
				int iWwPerMil = kLoopPlayer.getModifiedWarWearinessPercentAnger(getWarWeariness(eSurrenderTeam, true) / 100);

				if (iWwPerMil > iWearinessThreshold/2)
				{
					return true;
				}
			}
		}
	}

	if( (iValuableCities + iCitiesThreatenedByUs) >= (AI_maxWarRand()/100) )
	{
		// Continue conquest
		return false;
	}

	if( kSurrenderTeam.getNumCities() < (getNumCities()/4 - (AI_maxWarRand()/100)) )
	{
		// Too small to bother leaving alive
		return false;
	}
	
	return true;
}

void CvTeamAI::AI_getWarRands( int &iMaxWarRand, int &iLimitedWarRand, int &iDogpileWarRand ) const
{
	iMaxWarRand = AI_maxWarRand();
	iLimitedWarRand = AI_limitedWarRand();
	iDogpileWarRand = AI_dogpileWarRand();

	bool bCult4 = false;
	bool bSpace4 = false;
	bool bCult3 = false;
	bool bFinalWar = false;

	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
		{
			if (GET_PLAYER((PlayerTypes)iI).isAlive())
			{
				if (GET_PLAYER((PlayerTypes)iI).AI_isDoStrategy(AI_STRATEGY_FINAL_WAR))
				{
					bFinalWar = true;
				}

				if( GET_PLAYER((PlayerTypes)iI).AI_isDoVictoryStrategy(AI_VICTORY_CULTURE4))
				{
					bCult4 = true;
				}
				if( GET_PLAYER((PlayerTypes)iI).AI_isDoVictoryStrategy(AI_VICTORY_CULTURE3))
				{
					bCult3 = true;
				}
				if(GET_PLAYER((PlayerTypes)iI).AI_isDoVictoryStrategy(AI_VICTORY_SPACE4))
				{
					bSpace4 = true;
				}
			}
		}
	}

	if( bCult4 )
	{
		iMaxWarRand *= 4;
		iLimitedWarRand *= 3;
		iDogpileWarRand *= 2;
	}
	else if( bSpace4 )
	{
		iMaxWarRand *= 3;

		iLimitedWarRand *= 2;

		iDogpileWarRand *= 3;
		iDogpileWarRand /= 2;
	}
	else if( bCult3 )
	{
		iMaxWarRand *= 2;

		iLimitedWarRand *= 3;
		iLimitedWarRand /= 2;

		iDogpileWarRand *= 3;
		iDogpileWarRand /= 2;
	}

	int iNumMembers = getNumMembers();
	int iNumVassals = getVassalCount();
	
	iMaxWarRand *= (2 + iNumMembers);
	iMaxWarRand /= (2 + iNumMembers + iNumVassals);
	
	if (bFinalWar)
	{
	    iMaxWarRand /= 4;
	}

	iLimitedWarRand *= (2 + iNumMembers);
	iLimitedWarRand /= (2 + iNumMembers + iNumVassals);
	
	iDogpileWarRand *= (2 + iNumMembers);
	iDogpileWarRand /= (2 + iNumMembers + iNumVassals);
}


void CvTeamAI::AI_getWarThresholds( int &iTotalWarThreshold, int &iLimitedWarThreshold, int &iDogpileWarThreshold ) const
{
	iTotalWarThreshold = 0;
	iLimitedWarThreshold = 0;
	iDogpileWarThreshold = 0;

	//int iHighUnitSpendingPercent = 0;
	int iHighUnitSpending = 0; // K-Mod
	bool bConq2 = false;
	bool bDom3 = false;
	bool bAggressive = GC.getGameINLINE().isOption(GAMEOPTION_AGGRESSIVE_AI);
	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
		{
			if (GET_PLAYER((PlayerTypes)iI).isAlive())
			{
				/* int iUnitSpendingPercent = (GET_PLAYER((PlayerTypes)iI).calculateUnitCost() * 100) / std::max(1, GET_PLAYER((PlayerTypes)iI).calculatePreInflatedCosts());
				iHighUnitSpendingPercent += (std::max(0, iUnitSpendingPercent - 7) / 2); */
				int iUnitSpendingPerMil = GET_PLAYER((PlayerTypes)iI).AI_unitCostPerMil(); // K-Mod
				iHighUnitSpending += (std::max(0, iUnitSpendingPerMil - 16) / 6); // K-Mod

				if( GET_PLAYER((PlayerTypes)iI).AI_isDoStrategy(AI_STRATEGY_DAGGER))
				{
					bAggressive = true;
				}
				if( GET_PLAYER((PlayerTypes)iI).AI_isDoVictoryStrategy(AI_VICTORY_CONQUEST4))
				{
					bAggressive = true;
				}
				if( GET_PLAYER((PlayerTypes)iI).AI_isDoVictoryStrategy(AI_VICTORY_DOMINATION4))
				{
					bAggressive = true;
				}
				if( GET_PLAYER((PlayerTypes)iI).AI_isDoVictoryStrategy(AI_VICTORY_CONQUEST2))
				{
					bConq2 = true;
				}
				if(GET_PLAYER((PlayerTypes)iI).AI_isDoVictoryStrategy(AI_VICTORY_DOMINATION3))
				{
					bDom3 = true;
				}
			}
		}
	}

	iHighUnitSpending /= std::max(1, getNumMembers());

	iTotalWarThreshold = iHighUnitSpending * (bAggressive ? 3 : 2);
	if( bDom3 )
	{
		iTotalWarThreshold *= 3;

		iDogpileWarThreshold += 5;
	}
	else if( bConq2 )
	{
		iTotalWarThreshold *= 2;

		iDogpileWarThreshold += 2;
	}
	iTotalWarThreshold /= 3;
	iTotalWarThreshold += bAggressive ? 1 : 0;

	if( bAggressive && GET_PLAYER(getLeaderID()).getCurrentEra() < 3 )
	{
		iLimitedWarThreshold += 2;
	}
}

// Returns odds of player declaring total war times 100
int CvTeamAI::AI_getTotalWarOddsTimes100( ) const
{
	int iTotalWarRand;
	int iLimitedWarRand;
	int iDogpileWarRand;
	AI_getWarRands( iTotalWarRand, iLimitedWarRand, iDogpileWarRand );

	int iTotalWarThreshold;
	int iLimitedWarThreshold;
	int iDogpileWarThreshold;
	AI_getWarThresholds( iTotalWarThreshold, iLimitedWarThreshold, iDogpileWarThreshold );

	return ((100 * 100 * iTotalWarThreshold) / std::max(1, iTotalWarRand));
}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

int CvTeamAI::AI_makePeaceTradeVal(TeamTypes ePeaceTeam, TeamTypes eTeam) const
{
	int iModifier;
	int iValue;

	FAssertMsg(eTeam != getID(), "shouldn't call this function on ourselves");
	FAssertMsg(ePeaceTeam != getID(), "shouldn't call this function on ourselves");
	FAssertMsg(GET_TEAM(ePeaceTeam).isAlive(), "GET_TEAM(ePeaceTeam).isAlive is expected to be true");
	FAssertMsg(atWar(ePeaceTeam, eTeam), "eTeam should be at war with ePeaceTeam");

	iValue = (50 + GC.getGameINLINE().getGameTurn());
	iValue += ((GET_TEAM(eTeam).getNumCities() + GET_TEAM(ePeaceTeam).getNumCities()) * 8);

	iModifier = 0;

	switch ((GET_TEAM(eTeam).AI_getAttitude(ePeaceTeam) + GET_TEAM(ePeaceTeam).AI_getAttitude(eTeam)) / 2)
	{
	case ATTITUDE_FURIOUS:
		iModifier += 400;
		break;

	case ATTITUDE_ANNOYED:
		iModifier += 200;
		break;

	case ATTITUDE_CAUTIOUS:
		iModifier += 100;
		break;

	case ATTITUDE_PLEASED:
		iModifier += 50;
		break;

	case ATTITUDE_FRIENDLY:
		break;

	default:
		FAssert(false);
		break;
	}

	iValue *= std::max(0, (iModifier + 100));
	iValue /= 100;

	iValue *= 40;
	iValue /= (GET_TEAM(eTeam).AI_getAtWarCounter(ePeaceTeam) + 10);

	iValue -= (iValue % GC.getDefineINT("DIPLOMACY_VALUE_REMAINDER"));

	if (isHuman())
	{
		return std::max(iValue, GC.getDefineINT("DIPLOMACY_VALUE_REMAINDER"));
	}
	else
	{
		return iValue;
	}
}


DenialTypes CvTeamAI::AI_makePeaceTrade(TeamTypes ePeaceTeam, TeamTypes eTeam) const
{
	FAssertMsg(eTeam != getID(), "shouldn't call this function on ourselves");
	FAssertMsg(ePeaceTeam != getID(), "shouldn't call this function on ourselves");
	FAssertMsg(GET_TEAM(ePeaceTeam).isAlive(), "GET_TEAM(ePeaceTeam).isAlive is expected to be true");
	FAssertMsg(isAtWar(ePeaceTeam), "should be at war with ePeaceTeam");

	if (GET_TEAM(ePeaceTeam).isHuman())
	{
		return DENIAL_CONTACT_THEM;
	}

	if (GET_TEAM(ePeaceTeam).isAVassal())
	{
		return DENIAL_VASSAL;
	}

	if (isHuman())
	{
		return NO_DENIAL;
	}

	if (!canChangeWarPeace(ePeaceTeam))
	{
		return DENIAL_VASSAL;
	}

	if (AI_endWarVal(ePeaceTeam) > (GET_TEAM(ePeaceTeam).AI_endWarVal(getID()) * 2))
	{
		return DENIAL_CONTACT_THEM;
	}

	/* original bts code
    int iLandRatio = ((getTotalLand(true) * 100) / std::max(20, GET_TEAM(eTeam).getTotalLand(true)));
    if (iLandRatio > 250)
    {
		return DENIAL_VICTORY;
	} */
	// K-Mod
	if (AI_refusePeace(ePeaceTeam))
		return DENIAL_VICTORY;

	if (!GET_PLAYER(getLeaderID()).canContactAndTalk(GET_TEAM(ePeaceTeam).getLeaderID()) || GET_TEAM(ePeaceTeam).AI_refusePeace(getID()))
		return DENIAL_CONTACT_THEM;
	// K-Mod end

	return NO_DENIAL;
}


int CvTeamAI::AI_declareWarTradeVal(TeamTypes eWarTeam, TeamTypes eTeam) const
{
	PROFILE_FUNC();

	int iModifier;
	int iValue;

	FAssertMsg(eTeam != getID(), "shouldn't call this function on ourselves");
	FAssertMsg(eWarTeam != getID(), "shouldn't call this function on ourselves");
	FAssertMsg(GET_TEAM(eWarTeam).isAlive(), "GET_TEAM(eWarTeam).isAlive is expected to be true");
	FAssertMsg(!atWar(eWarTeam, eTeam), "eTeam should be at peace with eWarTeam");

	iValue = 0;
	iValue += (GET_TEAM(eWarTeam).getNumCities() * 10);
	iValue += (GET_TEAM(eWarTeam).getTotalPopulation(true) * 2);

	iModifier = 0;

	switch (GET_TEAM(eTeam).AI_getAttitude(eWarTeam))
	{
	case ATTITUDE_FURIOUS:
		break;

	case ATTITUDE_ANNOYED:
		iModifier += 25;
		break;

	case ATTITUDE_CAUTIOUS:
		iModifier += 50;
		break;

	case ATTITUDE_PLEASED:
		iModifier += 150;
		break;

	case ATTITUDE_FRIENDLY:
		iModifier += 400;
		break;

	default:
		FAssert(false);
		break;
	}

	iValue *= std::max(0, (iModifier + 100));
	iValue /= 100;

	int iTheirPower = GET_TEAM(eTeam).getPower(true);
	int iWarTeamPower = GET_TEAM(eWarTeam).getPower(true);

	iValue *= 50 + ((100 * iWarTeamPower) / (iTheirPower + iWarTeamPower + 1));
	iValue /= 100;

	if (!(GET_TEAM(eTeam).AI_isAllyLandTarget(eWarTeam)))
	{
		iValue *= 2;
	}

	if (!isAtWar(eWarTeam))
	{
		iValue *= 3;
	}
	else
	{
		iValue *= 150;
		iValue /= 100 + ((50 * std::min(100, (100 * AI_getWarSuccess(eWarTeam)) / (8 + getTotalPopulation(false)))) / 100);
	}
	
	iValue += (GET_TEAM(eTeam).getNumCities() * 20);
	iValue += (GET_TEAM(eTeam).getTotalPopulation(true) * 15);
	
	if (isAtWar(eWarTeam))
	{
		switch (GET_TEAM(eTeam).AI_getAttitude(getID()))
		{
		case ATTITUDE_FURIOUS:
		case ATTITUDE_ANNOYED:
		case ATTITUDE_CAUTIOUS:
			iValue *= 100;
			break;

		case ATTITUDE_PLEASED:
			iValue *= std::max(75, 100 - getAtWarCount(true) * 10);
			break;

		case ATTITUDE_FRIENDLY:
			iValue *= std::max(50, 100 - getAtWarCount(true) * 20);
			break;

		default:
			FAssert(false);
			break;
		}
		iValue /= 100;
	}
	
	iValue += GET_TEAM(eWarTeam).getNumNukeUnits() * 250;//Don't want to get nuked
	iValue += GET_TEAM(eTeam).getNumNukeUnits() * 150;//Don't want to use nukes on another's behalf

	if (GET_TEAM(eWarTeam).getAtWarCount(false) == 0)
	{
		iValue *= 2;
	
		for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
		{
			if (GET_TEAM((TeamTypes)iI).isAlive())
			{
				if (iI != getID() && iI != eWarTeam && iI != eTeam)
				{
					if (GET_TEAM(eWarTeam).isDefensivePact((TeamTypes)iI))
					{
						iValue += (GET_TEAM((TeamTypes)iI).getNumCities() * 30);
						iValue += (GET_TEAM((TeamTypes)iI).getTotalPopulation(true) * 20);
					}
				}
			}
		}
	}

	iValue *= 60 + (140 * GC.getGameINLINE().getGameTurn()) / std::max(1, GC.getGameINLINE().getEstimateEndTurn());
	iValue /= 100;

	iValue -= (iValue % GC.getDefineINT("DIPLOMACY_VALUE_REMAINDER"));

	if (isHuman())
	{
		return std::max(iValue, GC.getDefineINT("DIPLOMACY_VALUE_REMAINDER"));
	}
	else
	{
		return iValue;
	}
}


DenialTypes CvTeamAI::AI_declareWarTrade(TeamTypes eWarTeam, TeamTypes eTeam, bool bConsiderPower) const
{
	PROFILE_FUNC();

	AttitudeTypes eAttitude;
	AttitudeTypes eAttitudeThem;
	bool bLandTarget;
	int iI;

	FAssertMsg(eTeam != getID(), "shouldn't call this function on ourselves");
	FAssertMsg(eWarTeam != getID(), "shouldn't call this function on ourselves");
	FAssertMsg(GET_TEAM(eWarTeam).isAlive(), "GET_TEAM(eWarTeam).isAlive is expected to be true");
	FAssertMsg(!isAtWar(eWarTeam), "should be at peace with eWarTeam");

	if (GET_TEAM(eWarTeam).isVassal(eTeam) || GET_TEAM(eWarTeam).isDefensivePact(eTeam))
	{
		return DENIAL_JOKING;
	}

	if (isHuman())
	{
		return NO_DENIAL;
	}

	if (!canDeclareWar(eWarTeam))
	{
		return DENIAL_VASSAL;
	}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      12/06/09                                jdog5000      */
/*                                                                                              */
/* Diplomacy                                                                                    */
/************************************************************************************************/
/* original BTS code
	if (getAnyWarPlanCount(true) > 0)
	{
		return DENIAL_TOO_MANY_WARS;
	}
*/
	// Hide WHEOOHRN revealing war plans
	if( getAtWarCount(true) > 0 )
	{
		return DENIAL_TOO_MANY_WARS;
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	if (bConsiderPower)
	{
		bLandTarget = AI_isAllyLandTarget(eWarTeam);

		if ((GET_TEAM(eWarTeam).getDefensivePower(getID()) / ((bLandTarget) ? 2 : 1)) >
			(getPower(true) + ((atWar(eWarTeam, eTeam)) ? GET_TEAM(eTeam).getPower(true) : 0)))
		{
			if (bLandTarget)
			{
				return DENIAL_POWER_THEM;
			}
			else
			{
				return DENIAL_NO_GAIN;
			}
		}
	}

	eAttitude = AI_getAttitude(eTeam);

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				if (eAttitude <= GC.getLeaderHeadInfo(GET_PLAYER((PlayerTypes)iI).getPersonalityType()).getDeclareWarRefuseAttitudeThreshold())
				{
					return DENIAL_ATTITUDE;
				}
			}
		}
	}

	eAttitudeThem = AI_getAttitude(eWarTeam);

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				if (eAttitudeThem > GC.getLeaderHeadInfo(GET_PLAYER((PlayerTypes)iI).getPersonalityType()).getDeclareWarThemRefuseAttitudeThreshold())
				{
					return DENIAL_ATTITUDE_THEM;
				}
			}
		}
	}
	
	if (!atWar(eWarTeam, eTeam))
	{
		if (GET_TEAM(eWarTeam).getNumNukeUnits() > 0)
		{
			return DENIAL_JOKING;
		}
	}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      12/06/09                                jdog5000      */
/*                                                                                              */
/* Diplomacy                                                                                    */
/************************************************************************************************/
	if (getAnyWarPlanCount(true) > 0)
	{
		return DENIAL_TOO_MANY_WARS;
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	return NO_DENIAL;
}


int CvTeamAI::AI_openBordersTradeVal(TeamTypes eTeam) const
{
	return (getNumCities() + GET_TEAM(eTeam).getNumCities());
}


DenialTypes CvTeamAI::AI_openBordersTrade(TeamTypes eTeam) const
{
	PROFILE_FUNC();

	AttitudeTypes eAttitude;
	int iI;

	FAssertMsg(eTeam != getID(), "shouldn't call this function on ourselves");

	if (isHuman())
	{
		return NO_DENIAL;
	}

	if (isVassal(eTeam))
	{
		return NO_DENIAL;
	}

	if (AI_shareWar(eTeam))
	{
		return NO_DENIAL;
	}
	
	if (AI_getMemoryCount(eTeam, MEMORY_CANCELLED_OPEN_BORDERS) > 0)
	{
		return DENIAL_RECENT_CANCEL;
	}

	if (AI_getWorstEnemy() == eTeam)
	{
		return DENIAL_WORST_ENEMY;
	}

	eAttitude = AI_getAttitude(eTeam);

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				if (eAttitude <= GC.getLeaderHeadInfo(GET_PLAYER((PlayerTypes)iI).getPersonalityType()).getOpenBordersRefuseAttitudeThreshold())
				{
					return DENIAL_ATTITUDE;
				}
			}
		}
	}

	return NO_DENIAL;
}


int CvTeamAI::AI_defensivePactTradeVal(TeamTypes eTeam) const
{
	return ((getNumCities() + GET_TEAM(eTeam).getNumCities()) * 3);
}


DenialTypes CvTeamAI::AI_defensivePactTrade(TeamTypes eTeam) const
{
	PROFILE_FUNC();

	AttitudeTypes eAttitude;
	int iI;

	FAssertMsg(eTeam != getID(), "shouldn't call this function on ourselves");

	if (isHuman())
	{
		return NO_DENIAL;
	}

	if (GC.getGameINLINE().countCivTeamsAlive() == 2)
	{
		return DENIAL_NO_GAIN;
	}

	if (AI_getWorstEnemy() == eTeam)
	{
		return DENIAL_WORST_ENEMY;
	}

	eAttitude = AI_getAttitude(eTeam);

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				if (eAttitude <= GC.getLeaderHeadInfo(GET_PLAYER((PlayerTypes)iI).getPersonalityType()).getDefensivePactRefuseAttitudeThreshold())
				{
					return DENIAL_ATTITUDE;
				}
			}
		}
	}

	return NO_DENIAL;
}


DenialTypes CvTeamAI::AI_permanentAllianceTrade(TeamTypes eTeam) const
{
	PROFILE_FUNC();

	AttitudeTypes eAttitude;
	int iI;

	FAssertMsg(eTeam != getID(), "shouldn't call this function on ourselves");

	if (isHuman())
	{
		return NO_DENIAL;
	}

	if (AI_getWorstEnemy() == eTeam)
	{
		return DENIAL_WORST_ENEMY;
	}

	if ((getPower(true) + GET_TEAM(eTeam).getPower(true)) > (GC.getGameINLINE().countTotalCivPower() / 2))
	{
		if (getPower(true) > GET_TEAM(eTeam).getPower(true))
		{
			return DENIAL_POWER_US;
		}
		else
		{
			return DENIAL_POWER_YOU;
		}
	}

	if ((AI_getDefensivePactCounter(eTeam) + AI_getShareWarCounter(eTeam)) < 40)
	{
		return DENIAL_NOT_ALLIED;
	}

	eAttitude = AI_getAttitude(eTeam);

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				if (eAttitude <= GC.getLeaderHeadInfo(GET_PLAYER((PlayerTypes)iI).getPersonalityType()).getPermanentAllianceRefuseAttitudeThreshold())
				{
					return DENIAL_ATTITUDE;
				}
			}
		}
	}

	return NO_DENIAL;
}


TeamTypes CvTeamAI::AI_getWorstEnemy() const
{
	return m_eWorstEnemy;
}


void CvTeamAI::AI_updateWorstEnemy()
{
	PROFILE_FUNC();

	TeamTypes eBestTeam = NO_TEAM;
	int iBestValue = MAX_INT;

	for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
	{
		TeamTypes eLoopTeam = (TeamTypes) iI;
		CvTeam& kLoopTeam = GET_TEAM(eLoopTeam);
		if (kLoopTeam.isAlive())
		{
			if (iI != getID() && !kLoopTeam.isVassal(getID()))
			{
				if (isHasMet(eLoopTeam))
				{
					if (AI_getAttitude(eLoopTeam) < ATTITUDE_CAUTIOUS)
					{
						int iValue = AI_getAttitudeVal(eLoopTeam);
						if (iValue < iBestValue)
						{
							iBestValue = iValue;
							eBestTeam = eLoopTeam;
						}
					}
				}
			}
		}
	}

	m_eWorstEnemy = eBestTeam;
}


int CvTeamAI::AI_getWarPlanStateCounter(TeamTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiWarPlanStateCounter[eIndex];
}


void CvTeamAI::AI_setWarPlanStateCounter(TeamTypes eIndex, int iNewValue)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	m_aiWarPlanStateCounter[eIndex] = iNewValue;
	FAssert(AI_getWarPlanStateCounter(eIndex) >= 0);
}


void CvTeamAI::AI_changeWarPlanStateCounter(TeamTypes eIndex, int iChange)
{
	AI_setWarPlanStateCounter(eIndex, (AI_getWarPlanStateCounter(eIndex) + iChange));
}


int CvTeamAI::AI_getAtWarCounter(TeamTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiAtWarCounter[eIndex];
}


void CvTeamAI::AI_setAtWarCounter(TeamTypes eIndex, int iNewValue)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	m_aiAtWarCounter[eIndex] = iNewValue;
	FAssert(AI_getAtWarCounter(eIndex) >= 0);
}


void CvTeamAI::AI_changeAtWarCounter(TeamTypes eIndex, int iChange)
{
	AI_setAtWarCounter(eIndex, (AI_getAtWarCounter(eIndex) + iChange));
}


int CvTeamAI::AI_getAtPeaceCounter(TeamTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiAtPeaceCounter[eIndex];
}


void CvTeamAI::AI_setAtPeaceCounter(TeamTypes eIndex, int iNewValue)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	m_aiAtPeaceCounter[eIndex] = iNewValue;
	FAssert(AI_getAtPeaceCounter(eIndex) >= 0);
}


void CvTeamAI::AI_changeAtPeaceCounter(TeamTypes eIndex, int iChange)
{
	AI_setAtPeaceCounter(eIndex, (AI_getAtPeaceCounter(eIndex) + iChange));
}


int CvTeamAI::AI_getHasMetCounter(TeamTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiHasMetCounter[eIndex];
}


void CvTeamAI::AI_setHasMetCounter(TeamTypes eIndex, int iNewValue)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	m_aiHasMetCounter[eIndex] = iNewValue;
	FAssert(AI_getHasMetCounter(eIndex) >= 0);
}


void CvTeamAI::AI_changeHasMetCounter(TeamTypes eIndex, int iChange)
{
	AI_setHasMetCounter(eIndex, (AI_getHasMetCounter(eIndex) + iChange));
}


int CvTeamAI::AI_getOpenBordersCounter(TeamTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiOpenBordersCounter[eIndex];
}


void CvTeamAI::AI_setOpenBordersCounter(TeamTypes eIndex, int iNewValue)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	m_aiOpenBordersCounter[eIndex] = iNewValue;
	FAssert(AI_getOpenBordersCounter(eIndex) >= 0);
}


void CvTeamAI::AI_changeOpenBordersCounter(TeamTypes eIndex, int iChange)
{
	AI_setOpenBordersCounter(eIndex, (AI_getOpenBordersCounter(eIndex) + iChange));
}


int CvTeamAI::AI_getDefensivePactCounter(TeamTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiDefensivePactCounter[eIndex];
}


void CvTeamAI::AI_setDefensivePactCounter(TeamTypes eIndex, int iNewValue)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	m_aiDefensivePactCounter[eIndex] = iNewValue;
	FAssert(AI_getDefensivePactCounter(eIndex) >= 0);
}


void CvTeamAI::AI_changeDefensivePactCounter(TeamTypes eIndex, int iChange)
{
	AI_setDefensivePactCounter(eIndex, (AI_getDefensivePactCounter(eIndex) + iChange));
}


int CvTeamAI::AI_getShareWarCounter(TeamTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiShareWarCounter[eIndex];
}


void CvTeamAI::AI_setShareWarCounter(TeamTypes eIndex, int iNewValue)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	m_aiShareWarCounter[eIndex] = iNewValue;
	FAssert(AI_getShareWarCounter(eIndex) >= 0);
}


void CvTeamAI::AI_changeShareWarCounter(TeamTypes eIndex, int iChange)
{
	AI_setShareWarCounter(eIndex, (AI_getShareWarCounter(eIndex) + iChange));
}


int CvTeamAI::AI_getWarSuccess(TeamTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiWarSuccess[eIndex];
}


void CvTeamAI::AI_setWarSuccess(TeamTypes eIndex, int iNewValue)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	m_aiWarSuccess[eIndex] = iNewValue;
	FAssert(AI_getWarSuccess(eIndex) >= 0);
}


void CvTeamAI::AI_changeWarSuccess(TeamTypes eIndex, int iChange)
{
	AI_setWarSuccess(eIndex, (AI_getWarSuccess(eIndex) + iChange));
}


int CvTeamAI::AI_getEnemyPeacetimeTradeValue(TeamTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiEnemyPeacetimeTradeValue[eIndex];
}


void CvTeamAI::AI_setEnemyPeacetimeTradeValue(TeamTypes eIndex, int iNewValue)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	m_aiEnemyPeacetimeTradeValue[eIndex] = iNewValue;
	FAssert(AI_getEnemyPeacetimeTradeValue(eIndex) >= 0);
	// K-Mod. update attitude
	for (PlayerTypes i = (PlayerTypes)0; i < MAX_CIV_PLAYERS; i=(PlayerTypes)(i+1))
	{
		CvPlayerAI& kPlayer_i = GET_PLAYER(i);
		if (kPlayer_i.getTeam() == getID())
		{
			for (PlayerTypes j = (PlayerTypes)0; j < MAX_CIV_PLAYERS; j=(PlayerTypes)(j+1))
			{
				if (GET_PLAYER(j).getTeam() == eIndex)
				{
					kPlayer_i.AI_updateAttitudeCache(j);
				}
			}
		}
	}
	// K-Mod end
}


void CvTeamAI::AI_changeEnemyPeacetimeTradeValue(TeamTypes eIndex, int iChange)
{
	AI_setEnemyPeacetimeTradeValue(eIndex, (AI_getEnemyPeacetimeTradeValue(eIndex) + iChange));
}


int CvTeamAI::AI_getEnemyPeacetimeGrantValue(TeamTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiEnemyPeacetimeGrantValue[eIndex];
}


void CvTeamAI::AI_setEnemyPeacetimeGrantValue(TeamTypes eIndex, int iNewValue)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");
	m_aiEnemyPeacetimeGrantValue[eIndex] = iNewValue;
	FAssert(AI_getEnemyPeacetimeGrantValue(eIndex) >= 0);
	// K-Mod. update attitude
	for (PlayerTypes i = (PlayerTypes)0; i < MAX_CIV_PLAYERS; i=(PlayerTypes)(i+1))
	{
		CvPlayerAI& kPlayer_i = GET_PLAYER(i);
		if (kPlayer_i.getTeam() == getID())
		{
			for (PlayerTypes j = (PlayerTypes)0; j < MAX_CIV_PLAYERS; j=(PlayerTypes)(j+1))
			{
				if (GET_PLAYER(j).getTeam() == eIndex)
				{
					kPlayer_i.AI_updateAttitudeCache(j);
				}
			}
		}
	}
	// K-Mod end
}


void CvTeamAI::AI_changeEnemyPeacetimeGrantValue(TeamTypes eIndex, int iChange)
{
	AI_setEnemyPeacetimeGrantValue(eIndex, (AI_getEnemyPeacetimeGrantValue(eIndex) + iChange));
}


WarPlanTypes CvTeamAI::AI_getWarPlan(TeamTypes eIndex) const
{
	FAssert(eIndex >= 0);
	FAssert(eIndex < MAX_TEAMS);
	FAssert(eIndex != getID() || m_aeWarPlan[eIndex] == NO_WARPLAN);
	return m_aeWarPlan[eIndex];
}


bool CvTeamAI::AI_isChosenWar(TeamTypes eIndex) const
{
	switch (AI_getWarPlan(eIndex))
	{
	case WARPLAN_ATTACKED_RECENT:
	case WARPLAN_ATTACKED:
		return false;
		break;
	case WARPLAN_PREPARING_LIMITED:
	case WARPLAN_PREPARING_TOTAL:
	case WARPLAN_LIMITED:
	case WARPLAN_TOTAL:
	case WARPLAN_DOGPILE:
		return true;
		break;
	}

	return false;
}


bool CvTeamAI::AI_isSneakAttackPreparing(TeamTypes eIndex) const
{
	return ((AI_getWarPlan(eIndex) == WARPLAN_PREPARING_LIMITED) || (AI_getWarPlan(eIndex) == WARPLAN_PREPARING_TOTAL));
}


bool CvTeamAI::AI_isSneakAttackReady(TeamTypes eIndex) const
{
	//return (AI_isChosenWar(eIndex) && !(AI_isSneakAttackPreparing(eIndex)));
	return !isAtWar(eIndex) && AI_isChosenWar(eIndex) && !AI_isSneakAttackPreparing(eIndex); // K-Mod
}

// K-Mod
bool CvTeamAI::AI_isSneakAttackReady() const
{
	for (int i = 0; i < MAX_CIV_TEAMS; i++)
	{
		if (AI_isSneakAttackReady((TeamTypes)i))
			return true;
	}
	return false;
}
// K-Mod end

void CvTeamAI::AI_setWarPlan(TeamTypes eIndex, WarPlanTypes eNewValue, bool bWar)
{
	int iI;

	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_TEAMS, "eIndex is expected to be within maximum bounds (invalid Index)");

	if (AI_getWarPlan(eIndex) != eNewValue)
	{
		if (bWar || !isAtWar(eIndex))
		{
			m_aeWarPlan[eIndex] = eNewValue;

			AI_setWarPlanStateCounter(eIndex, 0);

			AI_updateAreaStragies();

			for (iI = 0; iI < MAX_PLAYERS; iI++)
			{
				if (GET_PLAYER((PlayerTypes)iI).isAlive())
				{
					if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
					{
						if (!(GET_PLAYER((PlayerTypes)iI).isHuman()))
						{
							GET_PLAYER((PlayerTypes)iI).AI_makeProductionDirty();
						}
					}
				}
			}
		}
	}
}

//if this number is over 0 the teams are "close"
//this may be expensive to run, kinda O(N^2)...
int CvTeamAI::AI_teamCloseness(TeamTypes eIndex, int iMaxDistance) const
{
	PROFILE_FUNC();
	int iI, iJ;
	
	if (iMaxDistance == -1)
	{
		iMaxDistance = DEFAULT_PLAYER_CLOSENESS;
	}
	
	FAssert(eIndex != getID());
	int iValue = 0;
	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				for (iJ = 0; iJ < MAX_PLAYERS; iJ++)
				{
					if (GET_PLAYER((PlayerTypes)iJ).isAlive())
					{
						if (GET_PLAYER((PlayerTypes)iJ).getTeam() == eIndex)
						{
							iValue += GET_PLAYER((PlayerTypes)iI).AI_playerCloseness((PlayerTypes)iJ, iMaxDistance);
						}
					}
				}
			}
		}
	}
	
	return iValue;	
}


void CvTeamAI::read(FDataStreamBase* pStream)
{
	CvTeam::read(pStream);

	uint uiFlag=0;
	pStream->Read(&uiFlag);	// flags for expansion

	pStream->Read(MAX_TEAMS, m_aiWarPlanStateCounter);
	pStream->Read(MAX_TEAMS, m_aiAtWarCounter);
	pStream->Read(MAX_TEAMS, m_aiAtPeaceCounter);
	pStream->Read(MAX_TEAMS, m_aiHasMetCounter);
	pStream->Read(MAX_TEAMS, m_aiOpenBordersCounter);
	pStream->Read(MAX_TEAMS, m_aiDefensivePactCounter);
	pStream->Read(MAX_TEAMS, m_aiShareWarCounter);
	pStream->Read(MAX_TEAMS, m_aiWarSuccess);
	pStream->Read(MAX_TEAMS, m_aiEnemyPeacetimeTradeValue);
	pStream->Read(MAX_TEAMS, m_aiEnemyPeacetimeGrantValue);

	pStream->Read(MAX_TEAMS, (int*)m_aeWarPlan);
	pStream->Read((int*)&m_eWorstEnemy);

	// K-Mod
	m_aiStrengthMemory.resize(GC.getMapINLINE().numPlotsINLINE(), 0);
	FAssert(m_aiStrengthMemory.size() > 0);
	if (uiFlag >= 1)
	{
		pStream->Read(m_aiStrengthMemory.size(), &m_aiStrengthMemory[0]);
	}
	// K-Mod end
}


void CvTeamAI::write(FDataStreamBase* pStream)
{
	CvTeam::write(pStream);

	uint uiFlag=1; //
	pStream->Write(uiFlag);		// flag for expansion

	pStream->Write(MAX_TEAMS, m_aiWarPlanStateCounter);
	pStream->Write(MAX_TEAMS, m_aiAtWarCounter);
	pStream->Write(MAX_TEAMS, m_aiAtPeaceCounter);
	pStream->Write(MAX_TEAMS, m_aiHasMetCounter);
	pStream->Write(MAX_TEAMS, m_aiOpenBordersCounter);
	pStream->Write(MAX_TEAMS, m_aiDefensivePactCounter);
	pStream->Write(MAX_TEAMS, m_aiShareWarCounter);
	pStream->Write(MAX_TEAMS, m_aiWarSuccess);
	pStream->Write(MAX_TEAMS, m_aiEnemyPeacetimeTradeValue);
	pStream->Write(MAX_TEAMS, m_aiEnemyPeacetimeGrantValue);

	pStream->Write(MAX_TEAMS, (int*)m_aeWarPlan);
	pStream->Write(m_eWorstEnemy);

	// K-Mod.
	FAssert(m_aiStrengthMemory.size() == GC.getMapINLINE().numPlotsINLINE());
	m_aiStrengthMemory.resize(GC.getMapINLINE().numPlotsINLINE()); // the consequences of the assert failing are really bad.
	FAssert(m_aiStrengthMemory.size() > 0);
	pStream->Write(m_aiStrengthMemory.size(), &m_aiStrengthMemory[0]); // uiFlag >= 1
	// K-Mod end
}

// K-Mod - AI tactical memory.
// The AI uses this to remember how strong the enemy defence is at particular plots.
// NOTE: AI_setStrengthMemory should not be used by human players - because it may cause OOS errors.
int CvTeamAI::AI_getStrengthMemory(int x, int y) const
{
	FAssert(m_aiStrengthMemory.size() == GC.getMapINLINE().numPlotsINLINE());
	return m_aiStrengthMemory[GC.getMapINLINE().plotNumINLINE(x, y)];
}

void CvTeamAI::AI_setStrengthMemory(int x, int y, int value)
{
	FAssert(m_aiStrengthMemory.size() == GC.getMapINLINE().numPlotsINLINE());
	m_aiStrengthMemory[GC.getMapINLINE().plotNumINLINE(x, y)] = value;
}

void CvTeamAI::AI_updateStrengthMemory()
{
	PROFILE_FUNC();

	if (!isAlive() || isHuman() || isMinorCiv() || isBarbarian())
		return;

	FAssert(m_aiStrengthMemory.size() == GC.getMapINLINE().numPlotsINLINE());
	for (int i = 0; i < GC.getMapINLINE().numPlotsINLINE(); i++)
	{
		if (m_aiStrengthMemory[i] == 0)
			continue;

		CvPlot* kLoopPlot = GC.getMapINLINE().plotByIndexINLINE(i);
		if (kLoopPlot->isVisible(getID(), false) && !kLoopPlot->isVisibleEnemyUnit(getLeaderID()))
			m_aiStrengthMemory[i] = 0;
		else
			m_aiStrengthMemory[i] = 96 * m_aiStrengthMemory[i] / 100; // reduce by 4%, rounding down. (arbitrary number)
	}
}
// K-Mod end

// Protected Functions...

int CvTeamAI::AI_noTechTradeThreshold() const
{
	int iRand;
	int iCount;
	int iI;

	iRand = 0;
	iCount = 0;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				iRand += GC.getLeaderHeadInfo(GET_PLAYER((PlayerTypes)iI).getPersonalityType()).getNoTechTradeThreshold();
				iCount++;
			}
		}
	}

	if (iCount > 0)
	{
		iRand /= iCount;
	}

	return iRand;
}


int CvTeamAI::AI_techTradeKnownPercent() const
{
	int iRand;
	int iCount;
	int iI;

	iRand = 0;
	iCount = 0;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				iRand += GC.getLeaderHeadInfo(GET_PLAYER((PlayerTypes)iI).getPersonalityType()).getTechTradeKnownPercent();
				iCount++;
			}
		}
	}

	if (iCount > 0)
	{
		iRand /= iCount;
	}

	return iRand;
}


int CvTeamAI::AI_maxWarRand() const
{
	int iRand;
	int iCount;
	int iI;

	iRand = 0;
	iCount = 0;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				iRand += GC.getLeaderHeadInfo(GET_PLAYER((PlayerTypes)iI).getPersonalityType()).getMaxWarRand();
				iCount++;
			}
		}
	}

	if (iCount > 0)
	{
		iRand /= iCount;
	}

	return iRand;
}


int CvTeamAI::AI_maxWarNearbyPowerRatio() const
{
	int iRand;
	int iCount;
	int iI;

	iRand = 0;
	iCount = 0;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				iRand += GC.getLeaderHeadInfo(GET_PLAYER((PlayerTypes)iI).getPersonalityType()).getMaxWarNearbyPowerRatio();
				iCount++;
			}
		}
	}

	if (iCount > 1)
	{
		iRand /= iCount;
	}

	return iRand;
}


int CvTeamAI::AI_maxWarDistantPowerRatio() const
{
	int iRand;
	int iCount;
	int iI;

	iRand = 0;
	iCount = 0;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				iRand += GC.getLeaderHeadInfo(GET_PLAYER((PlayerTypes)iI).getPersonalityType()).getMaxWarDistantPowerRatio();
				iCount++;
			}
		}
	}

	if (iCount > 1)
	{
		iRand /= iCount;
	}

	return iRand;
}


int CvTeamAI::AI_maxWarMinAdjacentLandPercent() const
{
	int iRand;
	int iCount;
	int iI;

	iRand = 0;
	iCount = 0;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				iRand += GC.getLeaderHeadInfo(GET_PLAYER((PlayerTypes)iI).getPersonalityType()).getMaxWarMinAdjacentLandPercent();
				iCount++;
			}
		}
	}

	if (iCount > 0)
	{
		iRand /= iCount;
	}

	return iRand;
}


int CvTeamAI::AI_limitedWarRand() const
{
	int iRand;
	int iCount;
	int iI;

	iRand = 0;
	iCount = 0;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				iRand += GC.getLeaderHeadInfo(GET_PLAYER((PlayerTypes)iI).getPersonalityType()).getLimitedWarRand();
				iCount++;
			}
		}
	}

	if (iCount > 0)
	{
		iRand /= iCount;
	}

	return iRand;
}


int CvTeamAI::AI_limitedWarPowerRatio() const
{
	int iRand;
	int iCount;
	int iI;

	iRand = 0;
	iCount = 0;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				iRand += GC.getLeaderHeadInfo(GET_PLAYER((PlayerTypes)iI).getPersonalityType()).getLimitedWarPowerRatio();
				iCount++;
			}
		}
	}

	if (iCount > 0)
	{
		iRand /= iCount;
	}

	return iRand;
}


int CvTeamAI::AI_dogpileWarRand() const
{
	int iRand;
	int iCount;
	int iI;

	iRand = 0;
	iCount = 0;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				iRand += GC.getLeaderHeadInfo(GET_PLAYER((PlayerTypes)iI).getPersonalityType()).getDogpileWarRand();
				iCount++;
			}
		}
	}

	if (iCount > 0)
	{
		iRand /= iCount;
	}

	return iRand;
}


int CvTeamAI::AI_makePeaceRand() const
{
	int iRand;
	int iCount;
	int iI;

	iRand = 0;
	iCount = 0;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				iRand += GC.getLeaderHeadInfo(GET_PLAYER((PlayerTypes)iI).getPersonalityType()).getMakePeaceRand();
				iCount++;
			}
		}
	}

	if (iCount > 0)
	{
		iRand /= iCount;
	}

	return iRand;
}


int CvTeamAI::AI_noWarAttitudeProb(AttitudeTypes eAttitude) const
{
	int iProb;
	int iCount;
	int iI;

	iProb = 0;
	iCount = 0;

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      03/20/10                                jdog5000      */
/*                                                                                              */
/* War Strategy AI                                                                              */
/************************************************************************************************/
	int iVictoryStrategyAdjust = 0;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				iProb += GC.getLeaderHeadInfo(GET_PLAYER((PlayerTypes)iI).getPersonalityType()).getNoWarAttitudeProb(eAttitude);
				iCount++;

				// In final stages of miltaristic victory, AI may turn on its friends!
				if( GET_PLAYER((PlayerTypes)iI).AI_isDoVictoryStrategy(AI_VICTORY_CONQUEST4) )
				{
					iVictoryStrategyAdjust += 30;
				}
				else if( GET_PLAYER((PlayerTypes)iI).AI_isDoVictoryStrategy(AI_VICTORY_DOMINATION4) )
				{
					iVictoryStrategyAdjust += 20;
				}
			}
		}
	}

	if (iCount > 1)
	{
		iProb /= iCount;
		iVictoryStrategyAdjust /= iCount;
	}

	iProb = std::max( 0, iProb - iVictoryStrategyAdjust );
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	return iProb;
}


void CvTeamAI::AI_doCounter()
{
	int iI;

	for (iI = 0; iI < MAX_TEAMS; iI++)
	{
		if (GET_TEAM((TeamTypes)iI).isAlive())
		{
			if (iI != getID())
			{
				AI_changeWarPlanStateCounter(((TeamTypes)iI), 1);

				if (isAtWar((TeamTypes)iI))
				{
					AI_changeAtWarCounter(((TeamTypes)iI), 1);
				}
				else
				{
					AI_changeAtPeaceCounter(((TeamTypes)iI), 1);
				}

				if (isHasMet((TeamTypes)iI))
				{
					AI_changeHasMetCounter(((TeamTypes)iI), 1);
				}

				if (isOpenBorders((TeamTypes)iI))
				{
					AI_changeOpenBordersCounter(((TeamTypes)iI), 1);
				}

				if (isDefensivePact((TeamTypes)iI))
				{
					AI_changeDefensivePactCounter(((TeamTypes)iI), 1);
				}
				else
				{
					if (AI_getDefensivePactCounter((TeamTypes)iI) > 0)
					{
						AI_changeDefensivePactCounter(((TeamTypes)iI), -1);
					}
				}

				if (isHasMet((TeamTypes)iI))
				{
					if (AI_shareWar((TeamTypes)iI))
					{
						AI_changeShareWarCounter(((TeamTypes)iI), 1);
					}
				}
			}
		}
	}
}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      03/26/10                                jdog5000      */
/*                                                                                              */
/* War Strategy AI                                                                              */
/************************************************************************************************/
// Block AI from declaring war on a distant vassal if it shares an area with the master
bool CvTeamAI::AI_isOkayVassalTarget( TeamTypes eTeam ) const
{
	/* if( GET_TEAM(eTeam).isAVassal() )
	{
		if( !(AI_hasCitiesInPrimaryArea(eTeam)) || AI_calculateAdjacentLandPlots(eTeam) == 0 )
		{
			for( int iI = 0; iI < MAX_CIV_TEAMS; iI++ )
			{
				if( GET_TEAM(eTeam).isVassal((TeamTypes)iI) )
				{
					if( AI_hasCitiesInPrimaryArea((TeamTypes)iI) && AI_calculateAdjacentLandPlots((TeamTypes)iI) > 0)
					{
						return false;
					}
				}
			}
		}
	}

	return true; */
	// K-Mod version. Same functionality (but without support for multiple masters)
	TeamTypes eMasterTeam = GET_TEAM(eTeam).getMasterTeam();
	if (eMasterTeam == eTeam)
		return true; // not a vassal.

	if ((!AI_hasCitiesInPrimaryArea(eTeam) || AI_calculateAdjacentLandPlots(eTeam) == 0) &&
		(AI_hasCitiesInPrimaryArea(eMasterTeam) && AI_calculateAdjacentLandPlots(eMasterTeam) > 0))
		return false;

	return true;
	// K-Mod end
}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/


/// \brief Make war decisions, mainly for starting or switching war plans.
///
///
// This function has been tweaked throughout by BBAI and K-Mod, some changes marked others not.
// (K-Mod has made several structural changes.)
void CvTeamAI::AI_doWar()
{
	PROFILE_FUNC();

	/* FAssert(!isHuman());
	FAssert(!isBarbarian());
	FAssert(!isMinorCiv());

	if (isAVassal())
	{
		return;
	} */ // disabled by K-Mod. All civs still need to do some basic updates.

	// allow python to handle it
	if (GC.getUSE_AI_DO_WAR_CALLBACK()) // K-Mod. block unused python callbacks
	{
		CyArgsList argsList;
		argsList.add(getID());
		long lResult=0;
		gDLL->getPythonIFace()->callFunction(PYGameModule, "AI_doWar", argsList.makeFunctionArgs(), &lResult);
		if (lResult == 1)
		{
			return;
		}
	}

	int iEnemyPowerPercent = AI_getEnemyPowerPercent();

	// K-Mod note: This first section also used for vassals, and for human players.
	for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
	{
		if (GET_TEAM((TeamTypes)iI).isAlive() && isHasMet((TeamTypes)iI))
		{
			if (AI_getWarPlan((TeamTypes)iI) != NO_WARPLAN)
			{
				int iTimeModifier = 100;

				int iAbandonTimeModifier = 100;
				iAbandonTimeModifier *= 50 + GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).getTrainPercent();
				iAbandonTimeModifier /= 150;
				if (!isAtWar((TeamTypes)iI)) // K-Mod. time / abandon modifiers are only relevant for war preparations. We don't need them if we are already at war.
				{
					int iThreshold = (80*AI_maxWarNearbyPowerRatio())/100;

					if( iEnemyPowerPercent < iThreshold )
					{
						iTimeModifier *= iEnemyPowerPercent;
						iTimeModifier /= iThreshold;
					}
					// K-Mod
					// intercontinental wars need more prep time
					if (!AI_hasCitiesInPrimaryArea((TeamTypes)iI))
					{
						iTimeModifier *= 5;
						iTimeModifier /= 4;
						iAbandonTimeModifier *= 5;
						iAbandonTimeModifier /= 4;
						// maybe in the future I'll count the number of local cities and the number of overseas cities
						// and use it to make a more appropriate modifier... but not now.
					}
					else
					{
						//with crush strategy, use just 2/3 of the prep time.
						int iCrushMembers = AI_countMembersWithStrategy(AI_STRATEGY_CRUSH);
						iTimeModifier *= 3 * (getNumMembers()-iCrushMembers) + 2 * iCrushMembers;
						iTimeModifier /= 3;
					}
					// K-Mod end

					iTimeModifier *= 50 + GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).getTrainPercent();
					iTimeModifier /= 150;

					FAssert(iTimeModifier >= 0);
				}

				bool bEnemyVictoryLevel4 = GET_TEAM((TeamTypes)iI).AI_isAnyMemberDoVictoryStrategyLevel4();

				if (AI_getWarPlan((TeamTypes)iI) == WARPLAN_ATTACKED_RECENT)
				{
					FAssert(isAtWar((TeamTypes)iI));

					if (AI_getAtWarCounter((TeamTypes)iI) > ((GET_TEAM((TeamTypes)iI).AI_isLandTarget(getID())) ? 9 : 3))
					{
						if( gTeamLogLevel >= 1 )
						{
							logBBAI("      Team %d (%S) switching WARPLANS against team %d (%S) from ATTACKED_RECENT to ATTACKED with enemy power percent %d", getID(), GET_PLAYER(getLeaderID()).getCivilizationDescription(0), iI, GET_PLAYER(GET_TEAM((TeamTypes)iI).getLeaderID()).getCivilizationDescription(0), iEnemyPowerPercent );
						}
						AI_setWarPlan(((TeamTypes)iI), WARPLAN_ATTACKED);
					}
				}
				else if (AI_getWarPlan((TeamTypes)iI) == WARPLAN_PREPARING_LIMITED)
				{
					FAssert(canEventuallyDeclareWar((TeamTypes)iI));

					if (AI_getWarPlanStateCounter((TeamTypes)iI) > ((5 * iTimeModifier) / (bEnemyVictoryLevel4 ? 400 : 100)))
					{
						if (AI_startWarVal((TeamTypes)iI, WARPLAN_LIMITED) > 0) // K-Mod. Last chance to change our mind if circumstances have changed
						{
							if( gTeamLogLevel >= 1 )
							{
								logBBAI("      Team %d (%S) switching WARPLANS against team %d (%S) from PREPARING_LIMITED to LIMITED after %d turns with enemy power percent %d", getID(), GET_PLAYER(getLeaderID()).getCivilizationDescription(0), iI, GET_PLAYER(GET_TEAM((TeamTypes)iI).getLeaderID()).getCivilizationDescription(0), AI_getWarPlanStateCounter((TeamTypes)iI), iEnemyPowerPercent );
							}
							AI_setWarPlan(((TeamTypes)iI), WARPLAN_LIMITED);
						}
						// K-Mod
						else
						{
							if (gTeamLogLevel >= 1)
							{
								logBBAI("      Team %d (%S) abandoning WARPLAN_LIMITED against team %d (%S) after %d turns with enemy power percent %d", getID(), GET_PLAYER(getLeaderID()).getCivilizationDescription(0), iI, GET_PLAYER(GET_TEAM((TeamTypes)iI).getLeaderID()).getCivilizationDescription(0), AI_getWarPlanStateCounter((TeamTypes)iI), iEnemyPowerPercent );
							}
						}
						// K-Mod end
					}
				}
				else if (AI_getWarPlan((TeamTypes)iI) == WARPLAN_LIMITED || AI_getWarPlan((TeamTypes)iI) == WARPLAN_DOGPILE)
				{
					if( !isAtWar((TeamTypes)iI) )
					{
						FAssert(canEventuallyDeclareWar((TeamTypes)iI));

						bool bActive = false;
						for( int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; iPlayer++ )
						{
							if( GET_PLAYER((PlayerTypes)iPlayer).getTeam() == getID() )
							{
								if( GET_PLAYER((PlayerTypes)iPlayer).AI_enemyTargetMissions((TeamTypes)iI) > 0 )
								{
									bActive = true;
									break;
								}
							}
						}

						if( !bActive )
						{
							if (AI_getWarPlanStateCounter((TeamTypes)iI) > ((15 * iAbandonTimeModifier) / (100)))
							{
								if( gTeamLogLevel >= 1 )
								{
									logBBAI("      Team %d (%S) abandoning WARPLAN_LIMITED or WARPLAN_DOGPILE against team %d (%S) after %d turns with enemy power percent %d", getID(), GET_PLAYER(getLeaderID()).getCivilizationDescription(0), iI, GET_PLAYER(GET_TEAM((TeamTypes)iI).getLeaderID()).getCivilizationDescription(0), AI_getWarPlanStateCounter((TeamTypes)iI), iEnemyPowerPercent );
								}
								AI_setWarPlan(((TeamTypes)iI), NO_WARPLAN);
							}
						}

						if( AI_getWarPlan((TeamTypes)iI) == WARPLAN_DOGPILE )
						{
							if( GET_TEAM((TeamTypes)iI).getAtWarCount(true) == 0 )
							{
								if( gTeamLogLevel >= 1 )
								{
									logBBAI("      Team %d (%S) abandoning WARPLAN_DOGPILE against team %d (%S) after %d turns because enemy has no war with enemy power percent %d", getID(), GET_PLAYER(getLeaderID()).getCivilizationDescription(0), iI, GET_PLAYER(GET_TEAM((TeamTypes)iI).getLeaderID()).getCivilizationDescription(0), AI_getWarPlanStateCounter((TeamTypes)iI), iEnemyPowerPercent );
								}
								AI_setWarPlan(((TeamTypes)iI), NO_WARPLAN);
							}
						}
					}
				}
				else if (AI_getWarPlan((TeamTypes)iI) == WARPLAN_PREPARING_TOTAL)
				{
					FAssert(canEventuallyDeclareWar((TeamTypes)iI));

					if (AI_getWarPlanStateCounter((TeamTypes)iI) > ((10 * iTimeModifier) / (bEnemyVictoryLevel4 ? 400 : 100)))
					{
						bool bAreaValid = false;
						bool bShareValid = false;

						int iLoop;
						for(CvArea* pLoopArea = GC.getMapINLINE().firstArea(&iLoop); pLoopArea != NULL; pLoopArea = GC.getMapINLINE().nextArea(&iLoop))
						{
							if (AI_isPrimaryArea(pLoopArea))
							{
								if (GET_TEAM((TeamTypes)iI).countNumCitiesByArea(pLoopArea) > 0)
								{
									bShareValid = true;

									AreaAITypes eAreaAI = AI_calculateAreaAIType(pLoopArea, true);

									/* BBAI code
									if ( eAreaAI == AREAAI_DEFENSIVE)
									{
										bAreaValid = false;
									}
									else if( eAreaAI == AREAAI_OFFENSIVE )
									{
										bAreaValid = true;
									} */
									// K-Mod. Doing it that way means the order the areas are checked is somehow important...
									if (eAreaAI == AREAAI_OFFENSIVE)
									{
										bAreaValid = true; // require at least one offense area
									}
									else if (eAreaAI == AREAAI_DEFENSIVE)
									{
										bAreaValid = false;
										break; // false if there are _any_ defence areas
									}
									// K-Mod end
								}
							}
						}

						if (((bAreaValid && iEnemyPowerPercent < 140) || (!bShareValid && iEnemyPowerPercent < 110) || GET_TEAM((TeamTypes)iI).AI_getLowestVictoryCountdown() >= 0) &&
							AI_startWarVal((TeamTypes)iI, WARPLAN_TOTAL) > 0) // K-Mod. Last chance to change our mind if circumstances have changed
						{
							if( gTeamLogLevel >= 1 )
							{
								logBBAI("      Team %d (%S) switching WARPLANS against team %d (%S) from PREPARING_TOTAL to TOTAL after %d turns with enemy power percent %d", getID(), GET_PLAYER(getLeaderID()).getCivilizationDescription(0), iI, GET_PLAYER(GET_TEAM((TeamTypes)iI).getLeaderID()).getCivilizationDescription(0), AI_getWarPlanStateCounter((TeamTypes)iI), iEnemyPowerPercent );
							}
							AI_setWarPlan(((TeamTypes)iI), WARPLAN_TOTAL);
						}
						else if (AI_getWarPlanStateCounter((TeamTypes)iI) > ((20 * iAbandonTimeModifier) / 100))
						{
							if( gTeamLogLevel >= 1 )
							{
								logBBAI("      Team %d (%S) abandoning WARPLAN_TOTAL_PREPARING against team %d (%S) after %d turns with enemy power percent %d", getID(), GET_PLAYER(getLeaderID()).getCivilizationDescription(0), iI, GET_PLAYER(GET_TEAM((TeamTypes)iI).getLeaderID()).getCivilizationDescription(0), AI_getWarPlanStateCounter((TeamTypes)iI), iEnemyPowerPercent );
							}
							AI_setWarPlan(((TeamTypes)iI), NO_WARPLAN);
						}
					}
				}
				else if (AI_getWarPlan((TeamTypes)iI) == WARPLAN_TOTAL)
				{
					if( !isAtWar((TeamTypes)iI) )
					{
						FAssert(canEventuallyDeclareWar((TeamTypes)iI));

						bool bActive = false;
						for( int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; iPlayer++ )
						{
							if( GET_PLAYER((PlayerTypes)iPlayer).getTeam() == getID() )
							{
								if( GET_PLAYER((PlayerTypes)iPlayer).AI_enemyTargetMissions((TeamTypes)iI) > 0 )
								{
									bActive = true;
									break;
								}
							}
						}

						if( !bActive )
						{
							if (AI_getWarPlanStateCounter((TeamTypes)iI) > ((25 * iAbandonTimeModifier) / (100)))
							{
								if( gTeamLogLevel >= 1 )
								{
									logBBAI("      Team %d (%S) abandoning WARPLAN_TOTAL against team %d (%S) after %d turns with enemy power percent %d", getID(), GET_PLAYER(getLeaderID()).getCivilizationDescription(0), iI, GET_PLAYER(GET_TEAM((TeamTypes)iI).getLeaderID()).getCivilizationDescription(0), AI_getWarPlanStateCounter((TeamTypes)iI), iEnemyPowerPercent );
								}
								AI_setWarPlan(((TeamTypes)iI), NO_WARPLAN);
							}
						}
					}
				}
			}
		}
	}

	// K-Mod. This is the end of the basics updates.
	// The rest of the stuff is related to making peace deals, and planning future wars.
	if (isHuman() || isBarbarian() || isMinorCiv() || isAVassal())
		return;
	// K-Mod end

	for (iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				GET_PLAYER((PlayerTypes)iI).AI_doPeace();
			}
		}
	}
	
	int iNumMembers = getNumMembers();
	/* original bts code
	int iHighUnitSpendingPercent = 0;
	int iLowUnitSpendingPercent = 0;
	
	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
			{
				int iUnitSpendingPercent = (GET_PLAYER((PlayerTypes)iI).calculateUnitCost() * 100) / std::max(1, GET_PLAYER((PlayerTypes)iI).calculatePreInflatedCosts());
				iHighUnitSpendingPercent += (std::max(0, iUnitSpendingPercent - 7) / 2);
				iLowUnitSpendingPercent += iUnitSpendingPercent;
			}
		}
	}
	
	iHighUnitSpendingPercent /= iNumMembers;
	iLowUnitSpendingPercent /= iNumMembers; */ // K-Mod, this simply wasn't being used anywhere.

	// K-Mod. Gather some data...
	bool bAtWar = false;
	bool bTotalWarPlan = false;
	bool bAnyWarPlan = false;
	bool bLocalWarPlan = false;
	for (int i = 0; i < MAX_CIV_TEAMS; i++)
	{
		if (GET_TEAM((TeamTypes)i).isAlive() && !GET_TEAM((TeamTypes)i).isMinorCiv())
		{
			bAtWar = bAtWar || isAtWar((TeamTypes)i);

			switch (AI_getWarPlan((TeamTypes)i))
			{
			case NO_WARPLAN:
				break;
			case WARPLAN_PREPARING_TOTAL:
			case WARPLAN_TOTAL:
				bTotalWarPlan = true;
			default: // all other warplans
				bLocalWarPlan = bLocalWarPlan || AI_isLandTarget((TeamTypes)i);
				bAnyWarPlan = true;
				break;
			}
		}
	}
	// K-Mod end

	// if at war, check for making peace
	//if (getAtWarCount(true) > 0) // XXX
	if (bAtWar) // K-Mod
	{
		if (GC.getGameINLINE().getSorenRandNum(AI_makePeaceRand(), "AI Make Peace") == 0)
		{
			for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
			{
				if (GET_TEAM((TeamTypes)iI).isAlive())
				{
					if (iI != getID())
					{
						if (!(GET_TEAM((TeamTypes)iI).isHuman()))
						{
							if (canContact((TeamTypes)iI, true))
							{
								FAssert(!(GET_TEAM((TeamTypes)iI).isMinorCiv()));

								if (isAtWar((TeamTypes)iI))
								{
									if (AI_isChosenWar((TeamTypes)iI))
									{
										if( AI_getAtWarCounter((TeamTypes)iI) > std::max(10, (14 * GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).getVictoryDelayPercent())/100) )
										{
											// If nothing is happening in war
											if( AI_getWarSuccess((TeamTypes)iI) + GET_TEAM((TeamTypes)iI).AI_getWarSuccess(getID()) < 2*GC.getDefineINT("WAR_SUCCESS_ATTACKING") )
											{
												if( (GC.getGameINLINE().getSorenRandNum(8, "AI Make Peace 1") == 0) )
												{
													bool bValid = true;

													for( int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; iPlayer++ )
													{
														if( GET_PLAYER((PlayerTypes)iPlayer).getTeam() == getID() )
														{
															if( GET_PLAYER((PlayerTypes)iPlayer).AI_enemyTargetMissions((TeamTypes)iI) > 0 )
															{
																bValid = false;
																break;
															}
														}

														if( GET_PLAYER((PlayerTypes)iPlayer).getTeam() == iI )
														{
															//MissionAITypes eMissionAI = MISSIONAI_ASSAULT;
															if( GET_PLAYER((PlayerTypes)iPlayer).AI_enemyTargetMissions(getID()) > 0 )
															{
																bValid = false;
																break;
															}
														}
													}

													if( bValid )
													{
														makePeace((TeamTypes)iI);

														if( gTeamLogLevel >= 1 )
														{
															logBBAI("  Team %d (%S) making peace due to time and no fighting", getID(), GET_PLAYER(getLeaderID()).getCivilizationDescription(0) );
														}

														break;
													}
												}
											}

											// Fought to a long draw
											if (AI_getAtWarCounter((TeamTypes)iI) > ((((AI_getWarPlan((TeamTypes)iI) == WARPLAN_TOTAL) ? 40 : 30) * GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).getVictoryDelayPercent())/100) )
											{
												int iOurValue = AI_endWarVal((TeamTypes)iI);
												int iTheirValue = GET_TEAM((TeamTypes)iI).AI_endWarVal(getID());
												if ((iOurValue > (iTheirValue / 2)) && (iTheirValue > (iOurValue / 2)))
												{
													if( gTeamLogLevel >= 1 )
													{
														logBBAI("  Team %d (%S) making peace due to time and endWarVal %d vs their %d", getID(), GET_PLAYER(getLeaderID()).getCivilizationDescription(0) , iOurValue, iTheirValue );
													}
													makePeace((TeamTypes)iI);
													break;
												}
											}

											// All alone in what we thought was a dogpile
											if (AI_getWarPlan((TeamTypes)iI) == WARPLAN_DOGPILE)
											{
												if (GET_TEAM((TeamTypes)iI).getAtWarCount(true) == 1)
												{
													int iOurValue = AI_endWarVal((TeamTypes)iI);
													int iTheirValue = GET_TEAM((TeamTypes)iI).AI_endWarVal(getID());
													if ((iTheirValue > (iOurValue / 2)))
													{
														if( gTeamLogLevel >= 1 )
														{
															logBBAI("  Team %d (%S) making peace due to being only dog-piler left", getID(), GET_PLAYER(getLeaderID()).getCivilizationDescription(0) );
														}
														makePeace((TeamTypes)iI);
														break;
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	// if no war plans, consider starting one!
	//if (getAnyWarPlanCount(true) == 0 || iEnemyPowerPercent < 45)
	if (!bAnyWarPlan || (iEnemyPowerPercent < 45 && !(bLocalWarPlan && bTotalWarPlan) && AI_getWarSuccessRating() > (bTotalWarPlan ? 40 : 15))) // K-Mod
	{
		bool bAggressive = GC.getGameINLINE().isOption(GAMEOPTION_AGGRESSIVE_AI);

		int iFinancialTroubleCount = 0;
		int iDaggerCount = 0;
		int iGetBetterUnitsCount = 0;
		for (iI = 0; iI < MAX_PLAYERS; iI++)
		{
			if (GET_PLAYER((PlayerTypes)iI).isAlive())
			{
				if (GET_PLAYER((PlayerTypes)iI).getTeam() == getID())
				{
					if ( GET_PLAYER((PlayerTypes)iI).AI_isDoStrategy(AI_STRATEGY_DAGGER)
						|| GET_PLAYER((PlayerTypes)iI).AI_isDoVictoryStrategy(AI_VICTORY_CONQUEST3)
						|| GET_PLAYER((PlayerTypes)iI).AI_isDoVictoryStrategy(AI_VICTORY_DOMINATION4) )
					{
						iDaggerCount++;
						bAggressive = true;
					}

					if (GET_PLAYER((PlayerTypes)iI).AI_isDoStrategy(AI_STRATEGY_GET_BETTER_UNITS))
					{
						iGetBetterUnitsCount++;
					}
					
					if (GET_PLAYER((PlayerTypes)iI).AI_isFinancialTrouble())
					{
						iFinancialTroubleCount++;
					}
				}
			}
		}

	    // if random in this range is 0, we go to war of this type (so lower numbers are higher probablity)
		// average of everyone on our team
		int iTotalWarRand;
	    int iLimitedWarRand;
	    int iDogpileWarRand;
		AI_getWarRands( iTotalWarRand, iLimitedWarRand, iDogpileWarRand );

		int iTotalWarThreshold;
		int iLimitedWarThreshold;
		int iDogpileWarThreshold;
		AI_getWarThresholds( iTotalWarThreshold, iLimitedWarThreshold, iDogpileWarThreshold );
				
		// we oppose war if half the non-dagger teammates in financial trouble
		bool bFinancesOpposeWar = false;
		if ((iFinancialTroubleCount - iDaggerCount) >= std::max(1, getNumMembers() / 2 ))
		{
			// this can be overridden by by the pro-war booleans
			bFinancesOpposeWar = true;
		}

		// if agressive, we may start a war to get money
		bool bFinancesProTotalWar = false;
		bool bFinancesProLimitedWar = false;
		bool bFinancesProDogpileWar = false;
		if (iFinancialTroubleCount > 0)
		{
			// do we like all out wars?
			if (iDaggerCount > 0 || iTotalWarRand < 100)
			{
				bFinancesProTotalWar = true;
			}

			// do we like limited wars?
			if (iLimitedWarRand < 100)
			{
				bFinancesProLimitedWar = true;
			}
			
			// do we like dogpile wars?
			if (iDogpileWarRand < 100)
			{
				bFinancesProDogpileWar = true;
			}
		}
		bool bFinancialProWar = (bFinancesProTotalWar || bFinancesProLimitedWar || bFinancesProDogpileWar);
		
		// overall war check (quite frequently true)
		bool bMakeWarChecks = false;
		if ((iGetBetterUnitsCount - iDaggerCount) * 3 < iNumMembers * 2)
		{
			if (bFinancialProWar || !bFinancesOpposeWar)
			{
				// random overall war chance (at noble+ difficulties this is 100%)
				if (GC.getGameINLINE().getSorenRandNum(100, "AI Declare War 1") < GC.getHandicapInfo(GC.getGameINLINE().getHandicapType()).getAIDeclareWarProb())
				{
					bMakeWarChecks = true;
				}
			}
		}
		
		if (bMakeWarChecks)
		{
			int iOurPower = getPower(true);

			if (bAggressive && (getAnyWarPlanCount(true) == 0))
			{
				iOurPower *= 4;
				iOurPower /= 3;
			}

			iOurPower *= (100 - iEnemyPowerPercent);
			iOurPower /= 100;

			if ((bFinancesProTotalWar || !bFinancesOpposeWar) &&
				(GC.getGameINLINE().getSorenRandNum(iTotalWarRand, "AI Maximum War") <= iTotalWarThreshold))
			{
				int iNoWarRoll = GC.getGameINLINE().getSorenRandNum(100, "AI No War");
				iNoWarRoll = range(iNoWarRoll + (bAggressive ? 10 : 0) + (bFinancesProTotalWar ? 10 : 0) - (20*iGetBetterUnitsCount)/iNumMembers, 0, 99);

				int iBestValue = 10; // K-Mod. I've set the starting value above zero just as a buffer against close-calls which end up being negative value in the near future.
				TeamTypes eBestTeam = NO_TEAM;

				for (int iPass = 0; iPass < 3; iPass++)
				{
					for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
					{
						if (canEventuallyDeclareWar((TeamTypes)iI) && AI_haveSeenCities((TeamTypes)iI))
						{
							TeamTypes eLoopMasterTeam = GET_TEAM((TeamTypes)iI).getMasterTeam(); // K-Mod (plus all changes which refer to this variable).
							bool bVassal = eLoopMasterTeam != iI;

							if (bVassal && !AI_isOkayVassalTarget((TeamTypes)iI))
								continue;

							if (iNoWarRoll >= AI_noWarAttitudeProb(AI_getAttitude((TeamTypes)iI)) && (!bVassal || iNoWarRoll >= AI_noWarAttitudeProb(AI_getAttitude(eLoopMasterTeam))))
							{
								int iDefensivePower = (GET_TEAM((TeamTypes)iI).getDefensivePower(getID()) * 2) / 3;

								if (iDefensivePower < ((iOurPower * ((iPass > 1) ? AI_maxWarDistantPowerRatio() : AI_maxWarNearbyPowerRatio())) / 100))
								{
									// XXX make sure they share an area....

									FAssertMsg(!(GET_TEAM((TeamTypes)iI).isBarbarian()), "Expected to not be declaring war on the barb civ");
									FAssertMsg(iI != getID(), "Expected not to be declaring war on self (DOH!)");

									if ((iPass > 1 && !bLocalWarPlan) || AI_isLandTarget((TeamTypes)iI) || AI_isAnyCapitalAreaAlone() || GET_TEAM((TeamTypes)iI).AI_isAnyMemberDoVictoryStrategyLevel4())
									{
										if ((iPass > 0) || (AI_calculateAdjacentLandPlots((TeamTypes)iI) >= ((getTotalLand() * AI_maxWarMinAdjacentLandPercent()) / 100)) || GET_TEAM((TeamTypes)iI).AI_isAnyMemberDoVictoryStrategyLevel4())
										{
											int iValue = AI_startWarVal((TeamTypes)iI, WARPLAN_TOTAL);

											if( iValue > 0 && gTeamLogLevel >= 2 )
											{
												logBBAI("      Team %d (%S) considering starting TOTAL warplan with team %d with value %d on pass %d with %d adjacent plots", getID(), GET_PLAYER(getLeaderID()).getCivilizationDescription(0), iI, iValue, iPass, AI_calculateAdjacentLandPlots((TeamTypes)iI) );
											}

											if (iValue > iBestValue)
											{
												iBestValue = iValue;
												eBestTeam = ((TeamTypes)iI);
											}
										}
									}
								}
							}
						}
					}

					if (eBestTeam != NO_TEAM)
					{
						if( gTeamLogLevel >= 1 )
						{
							logBBAI("    Team %d (%S) starting TOTAL warplan preparations against team %d on pass %d", getID(), GET_PLAYER(getLeaderID()).getCivilizationDescription(0), eBestTeam, iPass );
						}

						AI_setWarPlan(eBestTeam, (iDaggerCount > 0) ? WARPLAN_TOTAL : WARPLAN_PREPARING_TOTAL);
						break;
					}
				}
			}
/************************************************************************************************/
/* UNOFFICIAL_PATCH                       01/02/09                                jdog5000      */
/*                                                                                              */
/* Bugfix                                                                                       */
/************************************************************************************************/
			else if ((bFinancesProLimitedWar || !bFinancesOpposeWar) &&
				(GC.getGameINLINE().getSorenRandNum(iLimitedWarRand, "AI Limited War") <= iLimitedWarThreshold))
/************************************************************************************************/
/* UNOFFICIAL_PATCH                        END                                                  */
/************************************************************************************************/
			{
				int iNoWarRoll = GC.getGameINLINE().getSorenRandNum(100, "AI No War") - 10;
				iNoWarRoll = range(iNoWarRoll + (bAggressive ? 10 : 0) + (bFinancesProLimitedWar ? 10 : 0), 0, 99);

				int iBestValue = 0;
				TeamTypes eBestTeam = NO_TEAM;

				for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
				{
					if (canEventuallyDeclareWar((TeamTypes)iI) && AI_haveSeenCities((TeamTypes)iI))
					{
						TeamTypes eLoopMasterTeam = GET_TEAM((TeamTypes)iI).getMasterTeam(); // K-Mod (plus all changes which refer to this variable).
						bool bVassal = eLoopMasterTeam != iI;

						if (bVassal && !AI_isOkayVassalTarget((TeamTypes)iI))
							continue;

						if (iNoWarRoll >= AI_noWarAttitudeProb(AI_getAttitude((TeamTypes)iI)) && (!bVassal || iNoWarRoll >= AI_noWarAttitudeProb(AI_getAttitude(eLoopMasterTeam))))
						{
							if (AI_isLandTarget((TeamTypes)iI) || (AI_isAnyCapitalAreaAlone() && GET_TEAM((TeamTypes)iI).AI_isAnyCapitalAreaAlone()))
							{
								if (GET_TEAM((TeamTypes)iI).getDefensivePower(getID()) < ((iOurPower * AI_limitedWarPowerRatio()) / 100))
								{
									int iValue = AI_startWarVal((TeamTypes)iI, WARPLAN_LIMITED);

									if( iValue > 0 && gTeamLogLevel >= 2 )
									{
										logBBAI("      Team %d (%S) considering starting LIMITED warplan with team %d with value %d", getID(), GET_PLAYER(getLeaderID()).getCivilizationDescription(0), iI, iValue );
									}

									if (iValue > iBestValue)
									{
										//FAssert(!AI_shareWar((TeamTypes)iI)); // disabled by K-Mod. (It isn't always true.)
										iBestValue = iValue;
										eBestTeam = ((TeamTypes)iI);
									}
								}
							}
						}
					}
				}

				if (eBestTeam != NO_TEAM)
				{
					if( gTeamLogLevel >= 1 )
					{
						logBBAI("    Team %d (%S) starting LIMITED warplan preparations against team %d", getID(), GET_PLAYER(getLeaderID()).getCivilizationDescription(0), eBestTeam );
					}

					AI_setWarPlan(eBestTeam, (iDaggerCount > 0) ? WARPLAN_LIMITED : WARPLAN_PREPARING_LIMITED);
				}
			}
			else if ((bFinancesProDogpileWar || !bFinancesOpposeWar) &&
				(GC.getGameINLINE().getSorenRandNum(iDogpileWarRand, "AI Dogpile War") <= iDogpileWarThreshold))
			{
				int iNoWarRoll = GC.getGameINLINE().getSorenRandNum(100, "AI No War") - 20;
				iNoWarRoll = range(iNoWarRoll + (bAggressive ? 10 : 0) + (bFinancesProDogpileWar ? 10 : 0), 0, 99);

				int iBestValue = 0;
				TeamTypes eBestTeam = NO_TEAM;

				for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
				{
					if (canDeclareWar((TeamTypes)iI) && AI_haveSeenCities((TeamTypes)iI))
					{
						TeamTypes eLoopMasterTeam = GET_TEAM((TeamTypes)iI).getMasterTeam(); // K-Mod (plus all changes which refer to this variable).
						bool bVassal = eLoopMasterTeam != iI;

						if (bVassal && !AI_isOkayVassalTarget((TeamTypes)iI))
							continue;

						if (iNoWarRoll >= AI_noWarAttitudeProb(AI_getAttitude((TeamTypes)iI)) && (!bVassal || iNoWarRoll >= AI_noWarAttitudeProb(AI_getAttitude(eLoopMasterTeam))))
						{
							if (GET_TEAM((TeamTypes)iI).getAtWarCount(true) > 0)
							{
								if (AI_isLandTarget((TeamTypes)iI) || GET_TEAM((TeamTypes)iI).AI_isAnyMemberDoVictoryStrategyLevel4())
								{
									int iDogpilePower = iOurPower;

									for (int iJ = 0; iJ < MAX_CIV_TEAMS; iJ++)
									{
										if (GET_TEAM((TeamTypes)iJ).isAlive())
										{
											if (iJ != iI)
											{
												if (atWar(((TeamTypes)iJ), ((TeamTypes)iI)))
												{
													iDogpilePower += GET_TEAM((TeamTypes)iJ).getPower(false);
												}
											}
										}
									}

									FAssert(GET_TEAM((TeamTypes)iI).getPower(true) == GET_TEAM((TeamTypes)iI).getDefensivePower(getID()) || GET_TEAM((TeamTypes)iI).isAVassal());

									if (((GET_TEAM((TeamTypes)iI).getDefensivePower(getID()) * 3) / 2) < iDogpilePower)
									{
										int iValue = AI_startWarVal((TeamTypes)iI, WARPLAN_DOGPILE);

										if( iValue > 0 && gTeamLogLevel >= 2 )
										{
											logBBAI("      Team %d (%S) considering starting DOGPILE warplan with team %d with value %d", getID(), GET_PLAYER(getLeaderID()).getCivilizationDescription(0), iI, iValue );
										}

										if (iValue > iBestValue)
										{
											//FAssert(!AI_shareWar((TeamTypes)iI)); // disabled by K-Mod. (why is this even here?)
											iBestValue = iValue;
											eBestTeam = ((TeamTypes)iI);
										}
									}
								}
							}
						}
					}
				}

				if (eBestTeam != NO_TEAM)
				{
					if( gTeamLogLevel >= 1 )
					{
						logBBAI("  Team %d (%S) starting DOGPILE warplan preparations with team %d", getID(), GET_PLAYER(getLeaderID()).getCivilizationDescription(0), eBestTeam );
					}
					AI_setWarPlan(eBestTeam, WARPLAN_DOGPILE);
				}
			}
		}
	}
}


//returns true if war is veto'd by rolls.
bool CvTeamAI::AI_performNoWarRolls(TeamTypes eTeam)
{
	
	if (GC.getGameINLINE().getSorenRandNum(100, "AI Declare War 1") > GC.getHandicapInfo(GC.getGameINLINE().getHandicapType()).getAIDeclareWarProb())
	{
		return true;
	}
	
	if (GC.getGameINLINE().getSorenRandNum(100, "AI No War") <= AI_noWarAttitudeProb(AI_getAttitude(eTeam)))
	{
		return true;		
	}
	
	
	
	return false;	
}

int CvTeamAI::AI_getAttitudeWeight(TeamTypes eTeam) const
{
	int iAttitudeWeight = 0;
	switch (AI_getAttitude(eTeam))
	{
	case ATTITUDE_FURIOUS:
		iAttitudeWeight = -100;
		break;
	case ATTITUDE_ANNOYED:
		iAttitudeWeight = -40;
		break;
	case ATTITUDE_CAUTIOUS:
		iAttitudeWeight = -5;
		break;
	case ATTITUDE_PLEASED:
		iAttitudeWeight = 50;
		break;
	case ATTITUDE_FRIENDLY:
		iAttitudeWeight = 100;			
		break;
	}
	
	return iAttitudeWeight;
}

int CvTeamAI::AI_getLowestVictoryCountdown() const
{
	int iBestVictoryCountdown = MAX_INT;
	for (int iVictory = 0; iVictory < GC.getNumVictoryInfos(); iVictory++)
	{
		 int iCountdown = getVictoryCountdown((VictoryTypes)iVictory);
		 if (iCountdown > 0)
		 {
			iBestVictoryCountdown = std::min(iBestVictoryCountdown, iCountdown);
		 }
	}
	if (MAX_INT == iBestVictoryCountdown)
	{
		iBestVictoryCountdown = -1;
	}
	return iBestVictoryCountdown;	
}

int CvTeamAI::AI_getTechMonopolyValue(TechTypes eTech, TeamTypes eTeam) const
{
	int iValue = 0;
	int iI;
	
	bool bWarPlan = (getAnyWarPlanCount(eTeam) > 0);
	
	for (iI = 0; iI < GC.getNumUnitClassInfos(); iI++)
	{
		UnitTypes eLoopUnit = ((UnitTypes)GC.getUnitClassInfo((UnitClassTypes)iI).getDefaultUnitIndex());

		if (eLoopUnit != NO_UNIT)
		{
			if (isTechRequiredForUnit((eTech), eLoopUnit))
			{
				if (isWorldUnitClass((UnitClassTypes)iI))
				{
					iValue += 50;
				}
				
				if (GC.getUnitInfo(eLoopUnit).getPrereqAndTech() == eTech)
				{
					int iNavalValue = 0;
					
					int iCombatRatio = (GC.getUnitInfo(eLoopUnit).getCombat() * 100) / std::max(1, GC.getGameINLINE().getBestLandUnitCombat());
					if (iCombatRatio > 50)
					{
						iValue += ((bWarPlan ? 100 : 50) * (iCombatRatio - 40)) / 50;
					}

					switch (GC.getUnitInfo(eLoopUnit).getDefaultUnitAIType())
					{
					case UNITAI_UNKNOWN:
					case UNITAI_ANIMAL:
					case UNITAI_SETTLE:
					case UNITAI_WORKER:
					break;

					case UNITAI_ATTACK:
					case UNITAI_ATTACK_CITY:
					case UNITAI_COLLATERAL:
						iValue += bWarPlan ? 50 : 20;
						break;

					case UNITAI_PILLAGE:
					case UNITAI_RESERVE:
					case UNITAI_COUNTER:
					case UNITAI_PARADROP:
					case UNITAI_CITY_DEFENSE:
					case UNITAI_CITY_COUNTER:
					case UNITAI_CITY_SPECIAL:
						iValue += bWarPlan ? 40 : 15;
						break;


					case UNITAI_EXPLORE:
					case UNITAI_MISSIONARY:
						break;

					case UNITAI_PROPHET:
					case UNITAI_ARTIST:
					case UNITAI_SCIENTIST:
					case UNITAI_GENERAL:
					case UNITAI_MERCHANT:
					case UNITAI_ENGINEER:
					case UNITAI_GREAT_SPY: // K-Mod
						break;

					case UNITAI_SPY:
						break;

					case UNITAI_ICBM:
						iValue += bWarPlan ? 80 : 40;
						break;

					case UNITAI_WORKER_SEA:
						break;

					case UNITAI_ATTACK_SEA:
						iNavalValue += 50;
						break;

					case UNITAI_RESERVE_SEA:
					case UNITAI_ESCORT_SEA:
						iNavalValue += 30;
						break;

					case UNITAI_EXPLORE_SEA:
						iValue += GC.getGame().circumnavigationAvailable() ? 100 : 0;
						break;

					case UNITAI_ASSAULT_SEA:
						iNavalValue += 60;
						break;

					case UNITAI_SETTLER_SEA:
					case UNITAI_MISSIONARY_SEA:
					case UNITAI_SPY_SEA:
						break;

					case UNITAI_CARRIER_SEA:
					case UNITAI_MISSILE_CARRIER_SEA:
						iNavalValue += 40;
						break;

					case UNITAI_PIRATE_SEA:
						iNavalValue += 20;
						break;

					case UNITAI_ATTACK_AIR:
					case UNITAI_DEFENSE_AIR:
						iValue += bWarPlan ? 60 : 30;
						break;

					case UNITAI_CARRIER_AIR:
						iNavalValue += 40;
						break;

					case UNITAI_MISSILE_AIR:
						iValue += bWarPlan ? 40 : 20;
						break;

					default:
						FAssert(false);
						break;
					}
					
					if (iNavalValue > 0)
					{
						if (AI_isAnyCapitalAreaAlone())
						{
							iValue += iNavalValue / 2;
						}
						if (bWarPlan && !AI_isLandTarget(eTeam))
						{
							iValue += iNavalValue / 2;
						}
					}
				}
			}
		}
	}

	for (iI = 0; iI < GC.getNumBuildingInfos(); iI++)
	{
		if (isTechRequiredForBuilding(eTech, ((BuildingTypes)iI)))
		{
			CvBuildingInfo& kLoopBuilding = GC.getBuildingInfo((BuildingTypes)iI);
			if (kLoopBuilding.getReligionType() == NO_RELIGION)
			{
				iValue += 30;
			}
			if (isWorldWonderClass((BuildingClassTypes)kLoopBuilding.getBuildingClassType()))
			{
				if (!(GC.getGameINLINE().isBuildingClassMaxedOut((BuildingClassTypes)kLoopBuilding.getBuildingClassType())))
				{
					iValue += 50;
				}
			}
		}
	}

	for (iI = 0; iI < GC.getNumProjectInfos(); iI++)
	{
		if (GC.getProjectInfo((ProjectTypes)iI).getTechPrereq() == eTech)
		{
			if (isWorldProject((ProjectTypes)iI))
			{
				if (!(GC.getGameINLINE().isProjectMaxedOut((ProjectTypes)iI)))
				{
					iValue += 100;
				}
			}
			else
			{
				iValue += 50;
			}
		}
	}
	
	return iValue;
	
	
}

bool CvTeamAI::AI_isWaterAreaRelevant(CvArea* pArea)
{
	int iTeamCities = 0;
	int iOtherTeamCities = 0;
	
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      01/15/09                                jdog5000      */
/*                                                                                              */
/* City AI                                                                                      */
/************************************************************************************************/
	CvArea* pBiggestArea = GC.getMap().findBiggestArea(true);
	if (pBiggestArea == pArea)
	{
		return true;
	}
	
	// An area is deemed relevant if it has at least 2 cities of our and different teams.
	// Also count lakes which are connected to ocean by a bridge city
	for (int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; iPlayer++)
	{
		CvPlayerAI& kPlayer = GET_PLAYER((PlayerTypes)iPlayer);
		
		if ((iTeamCities < 2 && kPlayer.getTeam() == getID()) || (iOtherTeamCities < 2 && kPlayer.getTeam() != getID()))
		{
			int iLoop;
			CvCity* pLoopCity;
			
			for (pLoopCity = kPlayer.firstCity(&iLoop); pLoopCity != NULL; pLoopCity = kPlayer.nextCity(&iLoop))
			{
				if (pLoopCity->plot()->isAdjacentToArea(pArea->getID()))
				{
					if (kPlayer.getTeam() == getID())
					{
						iTeamCities++;
						
						if( pLoopCity->waterArea() == pBiggestArea )
						{
							return true;
						}
					}
					else
					{
						iOtherTeamCities++;
					}
				}
			}
		}
		if (iTeamCities >= 2 && iOtherTeamCities >= 2)
		{
			return true;
		}
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/	

	return false;
}

// Private Functions...
