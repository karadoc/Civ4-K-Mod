// playerAI.cpp

#include "CvGameCoreDLL.h"
#include "CvPlayerAI.h"
#include "CvRandom.h"
#include "CvGlobals.h"
#include "CvGameCoreUtils.h"
#include "CvMap.h"
#include "CvArea.h"
#include "CvPlot.h"
#include "CvGameAI.h"
#include "CvTeamAI.h"
#include "CvGameCoreUtils.h"
#include "CvDiploParameters.h"
#include "CvInitCore.h"
#include "CyArgsList.h"
#include "CvDLLInterfaceIFaceBase.h"
#include "CvDLLEntityIFaceBase.h"
#include "CvDLLPythonIFaceBase.h"
#include "CvDLLEngineIFaceBase.h"
#include "CvInfos.h"
#include "CvPopupInfo.h"
#include "FProfiler.h"
#include "CvDLLFAStarIFaceBase.h"
#include "FAStarNode.h"
#include "CvEventReporter.h"

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      10/02/09                                jdog5000      */
/*                                                                                              */
/* AI logging                                                                                   */
/************************************************************************************************/
#include "BetterBTSAI.h"
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      08/21/09                                jdog5000      */
/*                                                                                              */
/* Efficiency                                                                                   */
/************************************************************************************************/
// Plot danger cache
//#define DANGER_RANGE						(4)
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

#define GREATER_FOUND_RANGE			(5)
#define CIVIC_CHANGE_DELAY			(20) // was 25
#define RELIGION_CHANGE_DELAY		(15)

// statics

CvPlayerAI* CvPlayerAI::m_aPlayers = NULL;

void CvPlayerAI::initStatics()
{
	m_aPlayers = new CvPlayerAI[MAX_PLAYERS];
	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		m_aPlayers[iI].m_eID = ((PlayerTypes)iI);
	}
}

void CvPlayerAI::freeStatics()
{
	SAFE_DELETE_ARRAY(m_aPlayers);
}

bool CvPlayerAI::areStaticsInitialized()
{
	if(m_aPlayers == NULL)
	{
		return false;
	}

	return true;
}

DllExport CvPlayerAI& CvPlayerAI::getPlayerNonInl(PlayerTypes ePlayer)
{
	return getPlayer(ePlayer);
}

// Public Functions...

CvPlayerAI::CvPlayerAI()
{
	m_aiNumTrainAIUnits = new int[NUM_UNITAI_TYPES];
	m_aiNumAIUnits = new int[NUM_UNITAI_TYPES];
	m_aiSameReligionCounter = new int[MAX_PLAYERS];
	m_aiDifferentReligionCounter = new int[MAX_PLAYERS];
	m_aiFavoriteCivicCounter = new int[MAX_PLAYERS];
	m_aiBonusTradeCounter = new int[MAX_PLAYERS];
	m_aiPeacetimeTradeValue = new int[MAX_PLAYERS];
	m_aiPeacetimeGrantValue = new int[MAX_PLAYERS];
	m_aiGoldTradedTo = new int[MAX_PLAYERS];
	m_aiAttitudeExtra = new int[MAX_PLAYERS];

	m_abFirstContact = new bool[MAX_PLAYERS];

	m_aaiContactTimer = new int*[MAX_PLAYERS];
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		m_aaiContactTimer[i] = new int[NUM_CONTACT_TYPES];
	}

	m_aaiMemoryCount = new int*[MAX_PLAYERS];
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		m_aaiMemoryCount[i] = new int[NUM_MEMORY_TYPES];
	}

	m_aiAverageYieldMultiplier = new int[NUM_YIELD_TYPES];
	m_aiAverageCommerceMultiplier = new int[NUM_COMMERCE_TYPES];
	m_aiAverageCommerceExchange = new int[NUM_COMMERCE_TYPES];
	
	m_aiBonusValue = NULL;
	m_aiUnitClassWeights = NULL;
	m_aiUnitCombatWeights = NULL;
	m_aiCloseBordersAttitudeCache = new int[MAX_PLAYERS];

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      09/03/09                       poyuzhe & jdog5000     */
/*                                                                                              */
/* Efficiency                                                                                   */
/************************************************************************************************/
	// From Sanguo Mod Performance, ie the CAR Mod
	// Attitude cache
	m_aiAttitudeCache = new int[MAX_PLAYERS];
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	AI_reset(true);
}


CvPlayerAI::~CvPlayerAI()
{
	AI_uninit();

	SAFE_DELETE_ARRAY(m_aiNumTrainAIUnits);
	SAFE_DELETE_ARRAY(m_aiNumAIUnits);
	SAFE_DELETE_ARRAY(m_aiSameReligionCounter);
	SAFE_DELETE_ARRAY(m_aiDifferentReligionCounter);
	SAFE_DELETE_ARRAY(m_aiFavoriteCivicCounter);
	SAFE_DELETE_ARRAY(m_aiBonusTradeCounter);
	SAFE_DELETE_ARRAY(m_aiPeacetimeTradeValue);
	SAFE_DELETE_ARRAY(m_aiPeacetimeGrantValue);
	SAFE_DELETE_ARRAY(m_aiGoldTradedTo);
	SAFE_DELETE_ARRAY(m_aiAttitudeExtra);
	SAFE_DELETE_ARRAY(m_abFirstContact);
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		SAFE_DELETE_ARRAY(m_aaiContactTimer[i]);
	}
	SAFE_DELETE_ARRAY(m_aaiContactTimer);

	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		SAFE_DELETE_ARRAY(m_aaiMemoryCount[i]);
	}
	SAFE_DELETE_ARRAY(m_aaiMemoryCount);
	
	SAFE_DELETE_ARRAY(m_aiAverageYieldMultiplier);
	SAFE_DELETE_ARRAY(m_aiAverageCommerceMultiplier);
	SAFE_DELETE_ARRAY(m_aiAverageCommerceExchange);
	SAFE_DELETE_ARRAY(m_aiCloseBordersAttitudeCache);

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      09/03/09                       poyuzhe & jdog5000     */
/*                                                                                              */
/* Efficiency                                                                                   */
/************************************************************************************************/
	// From Sanguo Mod Performance, ie the CAR Mod
	// Attitude cache
	SAFE_DELETE_ARRAY(m_aiAttitudeCache);
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

}


void CvPlayerAI::AI_init()
{
	AI_reset(false);

	//--------------------------------
	// Init other game data
	if ((GC.getInitCore().getSlotStatus(getID()) == SS_TAKEN) || (GC.getInitCore().getSlotStatus(getID()) == SS_COMPUTER))
	{
		FAssert(getPersonalityType() != NO_LEADER);
		AI_setPeaceWeight(GC.getLeaderHeadInfo(getPersonalityType()).getBasePeaceWeight() + GC.getGameINLINE().getSorenRandNum(GC.getLeaderHeadInfo(getPersonalityType()).getPeaceWeightRand(), "AI Peace Weight"));
		AI_setEspionageWeight(GC.getLeaderHeadInfo(getPersonalityType()).getEspionageWeight());
		//AI_setCivicTimer(((getMaxAnarchyTurns() == 0) ? (GC.getDefineINT("MIN_REVOLUTION_TURNS") * 2) : CIVIC_CHANGE_DELAY) / 2);
		AI_setReligionTimer(1);
		AI_setCivicTimer((getMaxAnarchyTurns() == 0) ? 1 : 2);
		AI_initStrategyRand(); // K-Mod
		updateCacheData(); // K-Mod
	}
}


void CvPlayerAI::AI_uninit()
{
	SAFE_DELETE_ARRAY(m_aiBonusValue);
	SAFE_DELETE_ARRAY(m_aiUnitClassWeights);
	SAFE_DELETE_ARRAY(m_aiUnitCombatWeights);
}


void CvPlayerAI::AI_reset(bool bConstructor)
{
	int iI;

	AI_uninit();

	m_iPeaceWeight = 0;
	m_iEspionageWeight = 0;
	m_iAttackOddsChange = 0;
	m_iCivicTimer = 0;
	m_iReligionTimer = 0;
	m_iExtraGoldTarget = 0;

/************************************************************************************************/
/* CHANGE_PLAYER                         06/08/09                                 jdog5000      */
/*                                                                                              */
/*                                                                                              */
/************************************************************************************************/
	if( bConstructor || getNumUnits() == 0 )
	{
		for (iI = 0; iI < NUM_UNITAI_TYPES; iI++)
		{
			m_aiNumTrainAIUnits[iI] = 0;
			m_aiNumAIUnits[iI] = 0;
		}
	}
/************************************************************************************************/
/* CHANGE_PLAYER                           END                                                  */
/************************************************************************************************/

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		m_aiSameReligionCounter[iI] = 0;
		m_aiDifferentReligionCounter[iI] = 0;
		m_aiFavoriteCivicCounter[iI] = 0;
		m_aiBonusTradeCounter[iI] = 0;
		m_aiPeacetimeTradeValue[iI] = 0;
		m_aiPeacetimeGrantValue[iI] = 0;
		m_aiGoldTradedTo[iI] = 0;
		m_aiAttitudeExtra[iI] = 0;
		m_abFirstContact[iI] = false;
		for (int iJ = 0; iJ < NUM_CONTACT_TYPES; iJ++)
		{
			m_aaiContactTimer[iI][iJ] = 0;
		}
		for (int iJ = 0; iJ < NUM_MEMORY_TYPES; iJ++)
		{
			m_aaiMemoryCount[iI][iJ] = 0;
		}

		if (!bConstructor && getID() != NO_PLAYER)
		{
			PlayerTypes eLoopPlayer = (PlayerTypes) iI;
			CvPlayerAI& kLoopPlayer = GET_PLAYER(eLoopPlayer);
			kLoopPlayer.m_aiSameReligionCounter[getID()] = 0;
			kLoopPlayer.m_aiDifferentReligionCounter[getID()] = 0;
			kLoopPlayer.m_aiFavoriteCivicCounter[getID()] = 0;
			kLoopPlayer.m_aiBonusTradeCounter[getID()] = 0;
			kLoopPlayer.m_aiPeacetimeTradeValue[getID()] = 0;
			kLoopPlayer.m_aiPeacetimeGrantValue[getID()] = 0;
			kLoopPlayer.m_aiGoldTradedTo[getID()] = 0;
			kLoopPlayer.m_aiAttitudeExtra[getID()] = 0;
			kLoopPlayer.m_abFirstContact[getID()] = false;
			for (int iJ = 0; iJ < NUM_CONTACT_TYPES; iJ++)
			{
				kLoopPlayer.m_aaiContactTimer[getID()][iJ] = 0;
			}
			for (int iJ = 0; iJ < NUM_MEMORY_TYPES; iJ++)
			{
				kLoopPlayer.m_aaiMemoryCount[getID()][iJ] = 0;
			}
		}
	}
	
	for (iI = 0; iI < NUM_YIELD_TYPES; iI++)
	{
		m_aiAverageYieldMultiplier[iI] = 0;
	}
	for (iI = 0; iI< NUM_COMMERCE_TYPES; iI++)
	{
		m_aiAverageCommerceMultiplier[iI] = 0;
		m_aiAverageCommerceExchange[iI] = 0;
	}
	m_iAverageGreatPeopleMultiplier = 0;
	m_iAverageCulturePressure = 0;
	//m_iAveragesCacheTurn = -1;
	
	m_iStrategyHash = 0;
	//m_iStrategyHashCacheTurn = -1;

// BBAI
	m_iStrategyRand = UINT_MAX; // was 0 (K-Mod)
	m_iVictoryStrategyHash = 0;
	//m_iVictoryStrategyHashCacheTurn = -1;
// BBAI end

	m_bWasFinancialTrouble = false;
	m_iTurnLastProductionDirty = -1;

	//m_iUpgradeUnitsCacheTurn = -1;
	//m_iUpgradeUnitsCachedExpThreshold = 0;
	m_iUpgradeUnitsCachedGold = 0;

	m_aiAICitySites.clear();
	
	FAssert(m_aiBonusValue == NULL);
	m_aiBonusValue = new int[GC.getNumBonusInfos()];
	for (iI = 0; iI < GC.getNumBonusInfos(); iI++)
	{
		m_aiBonusValue[iI] = -1;		
	}
	
	FAssert(m_aiUnitClassWeights == NULL);
	m_aiUnitClassWeights = new int[GC.getNumUnitClassInfos()];
	for (iI = 0; iI < GC.getNumUnitClassInfos(); iI++)
	{
		m_aiUnitClassWeights[iI] = 0;		
	}

	FAssert(m_aiUnitCombatWeights == NULL);
	m_aiUnitCombatWeights = new int[GC.getNumUnitCombatInfos()];
	for (iI = 0; iI < GC.getNumUnitCombatInfos(); iI++)
	{
		m_aiUnitCombatWeights[iI] = 0;		
	}

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		m_aiCloseBordersAttitudeCache[iI] = 0;

		if (!bConstructor && getID() != NO_PLAYER)
		{
			GET_PLAYER((PlayerTypes) iI).m_aiCloseBordersAttitudeCache[getID()] = 0;
		}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      09/03/09                       poyuzhe & jdog5000     */
/*                                                                                              */
/* Efficiency                                                                                   */
/************************************************************************************************/
		// From Sanguo Mod Performance, ie the CAR Mod
		// Attitude cache
		m_aiAttitudeCache[iI] = MAX_INT;

		if (!bConstructor && getID() != NO_PLAYER)
		{
			GET_PLAYER((PlayerTypes) iI).m_aiAttitudeCache[getID()] = MAX_INT;
		}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
	}
}


int CvPlayerAI::AI_getFlavorValue(FlavorTypes eFlavor) const
{
	FAssertMsg((getPersonalityType() >= 0), "getPersonalityType() is less than zero");
	FAssertMsg((eFlavor >= 0), "eFlavor is less than zero");
	return GC.getLeaderHeadInfo(getPersonalityType()).getFlavorValue(eFlavor);
}

// K-Mod
void CvPlayerAI::updateCacheData()
{
	if (isAlive())
	{
		AI_calculateAverages();
		AI_updateVictoryStrategyHash();
		if (!isHuman())
		{
			AI_updateStrategyHash();
			//AI_updateGoldToUpgradeAllUnits();
		}
		// note. total upgrade gold is currently used in AI_hurry, which is used by production-automated.
		// Therefore, we need to get total upgrade gold for human players as well as AI players.
		AI_updateGoldToUpgradeAllUnits();
	}
}
// K-Mod end

void CvPlayerAI::AI_doTurnPre()
{
	PROFILE_FUNC();

	FAssertMsg(getPersonalityType() != NO_LEADER, "getPersonalityType() is not expected to be equal with NO_LEADER");
	FAssertMsg(getLeaderType() != NO_LEADER, "getLeaderType() is not expected to be equal with NO_LEADER");
	FAssertMsg(getCivilizationType() != NO_CIVILIZATION, "getCivilizationType() is not expected to be equal with NO_CIVILIZATION");

	AI_invalidateCloseBordersAttitudeCache();

	AI_doCounter();

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      09/03/09                       poyuzhe & jdog5000     */
/*                                                                                              */
/* Efficiency                                                                                   */
/************************************************************************************************/
	// From Sanguo Mod Performance, ie the CAR Mod
	// Attitude cache
	AI_invalidateAttitudeCache();
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	AI_updateBonusValue();
	// K-Mod. GP weights can take a little bit of time, so lets only do it once every 3 turns.
	if (!isHuman() && (GC.getGameINLINE().getGameTurn() + AI_getStrategyRand(0))%3 == 0)
		AI_updateGreatPersonWeights();
	// K-Mod end
	
	AI_doEnemyUnitData();

	// K-Mod: clear construction value cache
	AI_ClearConstructionValueCache();
	
	if (isHuman())
	{
		return;
	}

	AI_doResearch();

	AI_doCommerce();

	AI_doMilitary();

	AI_doCivics();

	AI_doReligion();

	AI_doCheckFinancialTrouble();

	if (isBarbarian())
	{
		return;
	}

	if (isMinorCiv())
	{
		return;
	}
}


void CvPlayerAI::AI_doTurnPost()
{
	PROFILE_FUNC();

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

	AI_doDiplo();
/************************************************************************************************/
/* UNOFFICIAL_PATCH                       06/16/09                                jdog5000      */
/*                                                                                              */
/* Bugfix                                                                                       */
/************************************************************************************************/
	// Moved per alexman's suggestion
	//AI_doSplit();	
/************************************************************************************************/
/* UNOFFICIAL_PATCH                        END                                                  */
/************************************************************************************************/

	for (int i = 0; i < GC.getNumVictoryInfos(); ++i)
	{
		AI_launch((VictoryTypes)i);
	}
}


void CvPlayerAI::AI_doTurnUnitsPre()
{
	PROFILE_FUNC();

	AI_updateFoundValues();

	if (GC.getGameINLINE().getSorenRandNum(8, "AI Update Area Targets") == 0) // XXX personality???
	{
		AI_updateAreaTargets();
	}

	if (isHuman())
	{
		return;
	}

	if (isBarbarian())
	{
		return;
	}
	
	// Uncommented by K-Mod, and added warplan condition.
	if (AI_isDoStrategy(AI_STRATEGY_CRUSH) && GET_TEAM(getTeam()).getAnyWarPlanCount(true) > 0)
	{
		AI_convertUnitAITypesForCrush();		
	}
}


void CvPlayerAI::AI_doTurnUnitsPost()
{
	PROFILE_FUNC();

	CvUnit* pLoopUnit;
	CvPlot* pUnitPlot;
	bool bValid;
	int iPass;
	int iLoop;

	if (!isHuman() || isOption(PLAYEROPTION_AUTO_PROMOTION))
	{
		for(pLoopUnit = firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = nextUnit(&iLoop))
		{
			pLoopUnit->AI_promote();
		}
	}

	if (isHuman())
	{
		return;
	}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      02/24/10                                jdog5000      */
/*                                                                                              */
/* Gold AI                                                                                      */
/************************************************************************************************/
	//bool bAnyWar = (GET_TEAM(getTeam()).getAnyWarPlanCount(true) > 0);
	int iStartingGold = getGold();
	/* BBAI code
	int iTargetGold = AI_goldTarget();
	int iUpgradeBudget = (AI_getGoldToUpgradeAllUnits() / (bAnyWar ? 1 : 2));
	iUpgradeBudget = std::min(iUpgradeBudget, iStartingGold - ((iTargetGold > iUpgradeBudget) ? (iTargetGold - iUpgradeBudget) : iStartingGold/2)); */
	// K-Mod. Note: AI_getGoldToUpgradeAllUnits() is actually one of the components of AI_goldTarget()
	int iCostPerMil = AI_unitCostPerMil(); // used for scrap decisions.
	int iUpgradeBudget = 0;
	if (GET_TEAM(getTeam()).getAnyWarPlanCount(true) > 0)
	{
		// when at war, upgrades get priority
		iUpgradeBudget = std::min(getGold(), AI_goldTarget(true));
	}
	else
	{
		int iMaxBudget = AI_goldTarget(true);
		iUpgradeBudget = std::min(iMaxBudget, getGold() * iMaxBudget / std::max(1, AI_goldTarget(false)));
	}
	// K-Mod end

	// Always willing to upgrade 1 unit if we have the money
	iUpgradeBudget = std::max(iUpgradeBudget,1);
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	CvPlot* pLastUpgradePlot = NULL;
	for (iPass = 0; iPass < 4; iPass++)
	{
		for(pLoopUnit = firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = nextUnit(&iLoop))
		{
			bool bNoDisband = false;
			bValid = false;

			switch (iPass)
			{
			case 0:
				// BBAI note:  Effectively only for galleys, triremes, and ironclads ... unit types which are limited in
				// what terrain they can operate in
				if (AI_unitImpassableCount(pLoopUnit->getUnitType()) > 0)
				{
					bValid = true;
				}
				break;
			case 1:
				pUnitPlot = pLoopUnit->plot();
				if (pUnitPlot->isCity())
				{
					if (pUnitPlot->getBestDefender(getID()) == pLoopUnit)
					{
						bNoDisband = true;
						bValid = true;
						pLastUpgradePlot = pUnitPlot;
					}

					// try to upgrade units which are in danger... but don't get obsessed
					if (!bValid && pLastUpgradePlot != pUnitPlot && AI_getAnyPlotDanger(pUnitPlot, 1, false))
					{
						bNoDisband = true;
						bValid = true;
						pLastUpgradePlot = pUnitPlot;
					}
				}
				break;
			case 2:
				/* original BTS code
				if (pLoopUnit->cargoSpace() > 0)
				{
					bValid = true;
				} */

				// Only normal transports
				if (pLoopUnit->cargoSpace() > 0 && pLoopUnit->specialCargo() == NO_SPECIALUNIT)
				{
					bValid = iStartingGold - getGold() < iUpgradeBudget;
				}
				// Also upgrade escort ships
				if (pLoopUnit->AI_getUnitAIType() == UNITAI_ESCORT_SEA)
				{
					bValid = iStartingGold - getGold() < iUpgradeBudget;
				}
				
				break;
			case 3:
				/* original BTS code
				bValid = true; */
				bValid = iStartingGold - getGold() < iUpgradeBudget;
				break;
			default:
				FAssert(false);
				break;
			}

			if (bValid)
			{
				bool bKilled = false;
				if (!bNoDisband)
				{
					//if (pLoopUnit->canFight()) // original bts code
					if (pLoopUnit->getUnitCombatType() != NO_UNITCOMBAT) // K-Mod - bug fix for the rare case of a barb city spawning on top of an animal
					{
						int iExp = pLoopUnit->getExperience();
						CvCity* pPlotCity = pLoopUnit->plot()->getPlotCity();
						if (pPlotCity != NULL && pPlotCity->getOwnerINLINE() == getID())
						{
							int iCityExp = 0;
							iCityExp += pPlotCity->getFreeExperience();
							iCityExp += pPlotCity->getDomainFreeExperience(pLoopUnit->getDomainType());
							iCityExp += pPlotCity->getUnitCombatFreeExperience(pLoopUnit->getUnitCombatType());
							if (iCityExp > 0)
							{
								/*if ((iExp == 0) || (iExp < (iCityExp + 1) / 2))
								{
									/* original bts code
									if ((pLoopUnit->getDomainType() != DOMAIN_LAND) || pLoopUnit->plot()->plotCount(PUF_isMilitaryHappiness, -1, -1, getID()) > 1)
									{
										if ((calculateUnitCost() > 0) && (AI_getPlotDanger( pLoopUnit->plot(), 2, false) == 0))
										{										
											pLoopUnit->kill(false);
											bKilled = true;
											pLastUpgradePlot = NULL;
										}
									}
								} */
								// K-Mod
								if (iExp < iCityExp && GC.getGameINLINE().getGameTurn() - pLoopUnit->getGameTurnCreated() > 8)
								{
									int iDefenders = pLoopUnit->plot()->plotCount(PUF_canDefendGroupHead, -1, -1, getID(), NO_TEAM, PUF_isCityAIType);
									if (iDefenders > pPlotCity->AI_minDefenders() && !AI_getAnyPlotDanger(pLoopUnit->plot(), 2, false))
									{
										if (iCostPerMil > AI_maxUnitCostPerMil(pLoopUnit->area(), 100) + 2*iExp + 4*std::max(0, pPlotCity->AI_neededDefenders() - iDefenders))
										{
											if (gUnitLogLevel > 2)
												logBBAI("    %S scraps %S, with %d exp, and %d / %d spending.", getCivilizationDescription(0), pLoopUnit->getName(0).GetCString(), iExp, iCostPerMil, AI_maxUnitCostPerMil(pLoopUnit->area(), 100));
											pLoopUnit->scrap();
											pLoopUnit->doDelayedDeath(); // I could have just done kill(), but who knows what extra work scrap() wants to do for us?
											bKilled = true;
											pLastUpgradePlot = NULL;
											iCostPerMil = AI_unitCostPerMil(); // recalculate
										}
									}
								}
								// K-Mod end
							}
						}
					}
				}
				if (!bKilled)
				{
					pLoopUnit->AI_upgrade(); // CAN DELETE UNIT!!!
				}
			}
		}
	}

	if (isBarbarian())
	{
		return;
	}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      02/24/10                                jdog5000      */
/*                                                                                              */
/* AI Logging                                                                                   */
/************************************************************************************************/
	if( gPlayerLogLevel > 2 )
	{
		if( iStartingGold - getGold() > 0 )
		{
			logBBAI("    %S spends %d on unit upgrades out of budget of %d, %d gold remaining", getCivilizationDescription(0), iStartingGold - getGold(), iUpgradeBudget, getGold());
		}
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

/************************************************************************************************/
/* UNOFFICIAL_PATCH                       06/16/09                                jdog5000      */
/*                                                                                              */
/* Bugfix                                                                                       */
/************************************************************************************************/
	// Moved here per alexman's suggestion
	AI_doSplit();	
/************************************************************************************************/
/* UNOFFICIAL_PATCH                        END                                                  */
/************************************************************************************************/

}


void CvPlayerAI::AI_doPeace()
{
	PROFILE_FUNC();

	CvDiploParameters* pDiplo;
	CvCity* pBestReceiveCity;
	CvCity* pBestGiveCity;
	CvCity* pLoopCity;
	CLinkList<TradeData> ourList;
	CLinkList<TradeData> theirList;
	bool abContacted[MAX_TEAMS];
	TradeData item;
	TechTypes eBestReceiveTech;
	TechTypes eBestGiveTech;
	int iReceiveGold;
	int iGiveGold;
	int iGold;
	int iValue;
	int iBestValue;
	int iOurValue;
	int iTheirValue;
	int iLoop;
	int iI, iJ;

	FAssert(!isHuman());
	FAssert(!isMinorCiv());
	FAssert(!isBarbarian());

	for (iI = 0; iI < MAX_TEAMS; iI++)
	{
		abContacted[iI] = false;
	}

	for (iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (iI != getID())
			{
				if (canContactAndTalk((PlayerTypes)iI))
				{
					if (!(GET_TEAM(getTeam()).isHuman()) && (GET_PLAYER((PlayerTypes)iI).isHuman() || !(GET_TEAM(GET_PLAYER((PlayerTypes)iI).getTeam()).isHuman())))
					{
						if (GET_TEAM(getTeam()).isAtWar(GET_PLAYER((PlayerTypes)iI).getTeam()))
						{
							if (!(GET_PLAYER((PlayerTypes)iI).isHuman()) || (GET_TEAM(getTeam()).getLeaderID() == getID()))
							{
								FAssertMsg(!(GET_PLAYER((PlayerTypes)iI).isBarbarian()), "(GET_PLAYER((PlayerTypes)iI).isBarbarian()) did not return false as expected");
								FAssertMsg(iI != getID(), "iI is not expected to be equal with getID()");
								FAssert(GET_PLAYER((PlayerTypes)iI).getTeam() != getTeam());

								if (GET_TEAM(getTeam()).AI_getAtWarCounter(GET_PLAYER((PlayerTypes)iI).getTeam()) > 10)
								{
									if (AI_getContactTimer(((PlayerTypes)iI), CONTACT_PEACE_TREATY) == 0)
									{
										bool bOffered = false;

										setTradeItem(&item, TRADE_SURRENDER);

										if (canTradeItem((PlayerTypes)iI, item, true))
										{
											ourList.clear();
											theirList.clear();

											ourList.insertAtEnd(item);

											bOffered = true;

											if (GET_PLAYER((PlayerTypes)iI).isHuman())
											{
												if (!(abContacted[GET_PLAYER((PlayerTypes)iI).getTeam()]))
												{
													AI_changeContactTimer(((PlayerTypes)iI), CONTACT_PEACE_TREATY, GC.getLeaderHeadInfo(getPersonalityType()).getContactDelay(CONTACT_PEACE_TREATY));
													pDiplo = new CvDiploParameters(getID());
													FAssertMsg(pDiplo != NULL, "pDiplo must be valid");
													pDiplo->setDiploComment((DiploCommentTypes)GC.getInfoTypeForString("AI_DIPLOCOMMENT_OFFER_PEACE"));
													pDiplo->setAIContact(true);
													pDiplo->setOurOfferList(theirList);
													pDiplo->setTheirOfferList(ourList);
													gDLL->beginDiplomacy(pDiplo, (PlayerTypes)iI);
													abContacted[GET_PLAYER((PlayerTypes)iI).getTeam()] = true;
												}
											}
											else
											{
												GC.getGameINLINE().implementDeal(getID(), ((PlayerTypes)iI), &ourList, &theirList);
											}
										}

										if (!bOffered)
										{
											if (GC.getGameINLINE().getSorenRandNum(GC.getLeaderHeadInfo(getPersonalityType()).getContactRand(CONTACT_PEACE_TREATY), "AI Diplo Peace Treaty") == 0)
											{
												setTradeItem(&item, TRADE_PEACE_TREATY);

												if (canTradeItem(((PlayerTypes)iI), item, true) && GET_PLAYER((PlayerTypes)iI).canTradeItem(getID(), item, true))
												{
													iOurValue = GET_TEAM(getTeam()).AI_endWarVal(GET_PLAYER((PlayerTypes)iI).getTeam());
													iTheirValue = GET_TEAM(GET_PLAYER((PlayerTypes)iI).getTeam()).AI_endWarVal(getTeam());

													eBestReceiveTech = NO_TECH;
													eBestGiveTech = NO_TECH;

													iReceiveGold = 0;
													iGiveGold = 0;

													pBestReceiveCity = NULL;
													pBestGiveCity = NULL;

													if (iTheirValue > iOurValue)
													{
														if (iTheirValue > iOurValue)
														{
															iBestValue = 0;

															for (iJ = 0; iJ < GC.getNumTechInfos(); iJ++)
															{
																setTradeItem(&item, TRADE_TECHNOLOGIES, iJ);

																if (GET_PLAYER((PlayerTypes)iI).canTradeItem(getID(), item, true))
																{
																	iValue = (1 + GC.getGameINLINE().getSorenRandNum(10000, "AI Peace Trading (Tech #1)"));

																	if (iValue > iBestValue)
																	{
																		iBestValue = iValue;
																		eBestReceiveTech = ((TechTypes)iJ);
																	}
																}
															}

															if (eBestReceiveTech != NO_TECH)
															{
																iOurValue += GET_TEAM(getTeam()).AI_techTradeVal(eBestReceiveTech, GET_PLAYER((PlayerTypes)iI).getTeam());
															}
														}

														// K-Mod note: ideally we'd use AI_goldTradeValuePercent()
														// to work out the gold's value gained by the receiver and the gold's value lost by the giver.
														// Instead, we just pretend that the receiver gets 1 point per gold, and the giver loses nothing. (effectly a value of 2)
														iGold = std::min((iTheirValue - iOurValue), GET_PLAYER((PlayerTypes)iI).AI_maxGoldTrade(getID()));

														if (iGold > 0)
														{
															setTradeItem(&item, TRADE_GOLD, iGold);

															if (GET_PLAYER((PlayerTypes)iI).canTradeItem(getID(), item, true))
															{
																iReceiveGold = iGold;
																iOurValue += iGold;
															}
														}

														if (iTheirValue > iOurValue)
														{
															iBestValue = 0;

															for (pLoopCity = GET_PLAYER((PlayerTypes)iI).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER((PlayerTypes)iI).nextCity(&iLoop))
															{
																setTradeItem(&item, TRADE_CITIES, pLoopCity->getID());

																if (GET_PLAYER((PlayerTypes)iI).canTradeItem(getID(), item, true))
																{
																	iValue = pLoopCity->plot()->calculateCulturePercent(getID());

																	if (iValue > iBestValue)
																	{
																		iBestValue = iValue;
																		pBestReceiveCity = pLoopCity;
																	}
																}
															}

															if (pBestReceiveCity != NULL)
															{
																iOurValue += AI_cityTradeVal(pBestReceiveCity);
															}
														}
													}
													else if (iOurValue > iTheirValue)
													{
														iBestValue = 0;

														for (iJ = 0; iJ < GC.getNumTechInfos(); iJ++)
														{
															setTradeItem(&item, TRADE_TECHNOLOGIES, iJ);

															if (canTradeItem(((PlayerTypes)iI), item, true))
															{
																if (GET_TEAM(GET_PLAYER((PlayerTypes)iI).getTeam()).AI_techTradeVal((TechTypes)iJ, getTeam()) <= (iOurValue - iTheirValue))
																{
																	iValue = (1 + GC.getGameINLINE().getSorenRandNum(10000, "AI Peace Trading (Tech #2)"));

																	if (iValue > iBestValue)
																	{
																		iBestValue = iValue;
																		eBestGiveTech = ((TechTypes)iJ);
																	}
																}
															}
														}

														if (eBestGiveTech != NO_TECH)
														{
															iTheirValue += GET_TEAM(GET_PLAYER((PlayerTypes)iI).getTeam()).AI_techTradeVal(eBestGiveTech, getTeam());
														}

														iGold = std::min((iOurValue - iTheirValue), AI_maxGoldTrade((PlayerTypes)iI));

														if (iGold > 0)
														{
															setTradeItem(&item, TRADE_GOLD, iGold);

															if (canTradeItem(((PlayerTypes)iI), item, true))
															{
																iGiveGold = iGold;
																iTheirValue += iGold;
															}
														}

														iBestValue = 0;

														for (pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
														{
															setTradeItem(&item, TRADE_CITIES, pLoopCity->getID());

															if (canTradeItem(((PlayerTypes)iI), item, true))
															{
																if (GET_PLAYER((PlayerTypes)iI).AI_cityTradeVal(pLoopCity) <= (iOurValue - iTheirValue))
																{
																	iValue = pLoopCity->plot()->calculateCulturePercent((PlayerTypes)iI);

																	if (iValue > iBestValue)
																	{
																		iBestValue = iValue;
																		pBestGiveCity = pLoopCity;
																	}
																}
															}
														}

														if (pBestGiveCity != NULL)
														{
															iTheirValue += GET_PLAYER((PlayerTypes)iI).AI_cityTradeVal(pBestGiveCity);
														}
													}

													//if ((GET_PLAYER((PlayerTypes)iI).isHuman()) ? (iOurValue >= iTheirValue) : ((iOurValue > ((iTheirValue * 3) / 5)) && (iTheirValue > ((iOurValue * 3) / 5))))
													if ((GET_PLAYER((PlayerTypes)iI).isHuman() ? iOurValue >= 4*iTheirValue/5 : iOurValue > iTheirValue*3/5) && iTheirValue > iOurValue*3/5) // K-Mod
													{
														ourList.clear();
														theirList.clear();

														setTradeItem(&item, TRADE_PEACE_TREATY);

														ourList.insertAtEnd(item);
														theirList.insertAtEnd(item);

														if (eBestGiveTech != NO_TECH)
														{
															setTradeItem(&item, TRADE_TECHNOLOGIES, eBestGiveTech);
															ourList.insertAtEnd(item);
														}

														if (eBestReceiveTech != NO_TECH)
														{
															setTradeItem(&item, TRADE_TECHNOLOGIES, eBestReceiveTech);
															theirList.insertAtEnd(item);
														}

														if (iGiveGold != 0)
														{
															setTradeItem(&item, TRADE_GOLD, iGiveGold);
															ourList.insertAtEnd(item);
														}

														if (iReceiveGold != 0)
														{
															setTradeItem(&item, TRADE_GOLD, iReceiveGold);
															theirList.insertAtEnd(item);
														}

														if (pBestGiveCity != NULL)
														{
															setTradeItem(&item, TRADE_CITIES, pBestGiveCity->getID());
															ourList.insertAtEnd(item);
														}

														if (pBestReceiveCity != NULL)
														{
															setTradeItem(&item, TRADE_CITIES, pBestReceiveCity->getID());
															theirList.insertAtEnd(item);
														}

														if (GET_PLAYER((PlayerTypes)iI).isHuman())
														{
															if (!(abContacted[GET_PLAYER((PlayerTypes)iI).getTeam()]))
															{
																AI_changeContactTimer(((PlayerTypes)iI), CONTACT_PEACE_TREATY, GC.getLeaderHeadInfo(getPersonalityType()).getContactDelay(CONTACT_PEACE_TREATY));
																pDiplo = new CvDiploParameters(getID());
																FAssertMsg(pDiplo != NULL, "pDiplo must be valid");
																pDiplo->setDiploComment((DiploCommentTypes)GC.getInfoTypeForString("AI_DIPLOCOMMENT_OFFER_PEACE"));
																pDiplo->setAIContact(true);
																pDiplo->setOurOfferList(theirList);
																pDiplo->setTheirOfferList(ourList);
																gDLL->beginDiplomacy(pDiplo, (PlayerTypes)iI);
																abContacted[GET_PLAYER((PlayerTypes)iI).getTeam()] = true;
															}
														}
														else
														{
															GC.getGameINLINE().implementDeal(getID(), ((PlayerTypes)iI), &ourList, &theirList);
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
	}
}


void CvPlayerAI::AI_updateFoundValues(bool bStartingLoc)
{
	PROFILE_FUNC();

	bool bCitySiteCalculations = (GC.getGame().getGameTurn() > GC.getGame().getStartTurn());
	
	int iLoop;
	for (CvArea* pLoopArea = GC.getMapINLINE().firstArea(&iLoop); pLoopArea != NULL; pLoopArea = GC.getMapINLINE().nextArea(&iLoop))
	{
		pLoopArea->setBestFoundValue(getID(), 0);
	}

	if (bStartingLoc)
	{
		for (int iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
		{
			GC.getMapINLINE().plotByIndexINLINE(iI)->setFoundValue(getID(), -1);
		}
	}
	else
	{
		// K-Mod. (I've also changed some of the code style all through this function)
		CvFoundSettings kFoundSet(*this, false);
		//

		if (!isBarbarian())
		{
			//AI_invalidateCitySites(AI_getMinFoundValue());
			AI_invalidateCitySites(-1); // K-Mod
		}
		for (int iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
		{
			CvPlot* pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);

			if (pLoopPlot->isRevealed(getTeam(), false))// || AI_isPrimaryArea(pLoopPlot->area()))
			{
				long iValue = -1;
				if (GC.getUSE_GET_CITY_FOUND_VALUE_CALLBACK())
				{
					CyArgsList argsList;
					argsList.add((int)getID());
					argsList.add(pLoopPlot->getX());
					argsList.add(pLoopPlot->getY());
					gDLL->getPythonIFace()->callFunction(PYGameModule, "getCityFoundValue", argsList.makeFunctionArgs(), &iValue);
				}

				if (iValue == -1)
				{
					//iValue = AI_foundValue(pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE());
					iValue = AI_foundValueBulk(pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE(), kFoundSet); // K-Mod
				}

				iValue = std::min((long)MAX_SHORT, iValue); // K-Mod
				pLoopPlot->setFoundValue(getID(), (short)iValue);

				if (iValue > pLoopPlot->area()->getBestFoundValue(getID()))
				{
					pLoopPlot->area()->setBestFoundValue(getID(), iValue);
				}
			}
		}
		if (!isBarbarian())
		{
			//int iMaxCityCount = 4;
			int iMaxCityCount = isHuman() ? 6 : 4; // K-Mod - because humans don't always walk towards the AI's top picks..
			AI_updateCitySites(AI_getMinFoundValue(), iMaxCityCount);
		}
	}
}


void CvPlayerAI::AI_updateAreaTargets()
{
	CvArea* pLoopArea;
	int iLoop;

	for(pLoopArea = GC.getMapINLINE().firstArea(&iLoop); pLoopArea != NULL; pLoopArea = GC.getMapINLINE().nextArea(&iLoop))
	{
		if (!(pLoopArea->isWater()))
		{
			if (GC.getGameINLINE().getSorenRandNum(3, "AI Target City") == 0)
			{
				pLoopArea->setTargetCity(getID(), NULL);
			}
			else
			{
				pLoopArea->setTargetCity(getID(), AI_findTargetCity(pLoopArea));
			}
		}
	}
}


// Returns priority for unit movement (lower values move first...)
// This function has been heavily edited for K-Mod
int CvPlayerAI::AI_movementPriority(CvSelectionGroup* pGroup) const
{
	// If the group is trying to do a stack attack, let it go first!
	// (this is required for amphibious assults; during which a low priority group can be ordered to attack before its turn.)
	if (pGroup->AI_isGroupAttack())
		return -1;

	const CvUnit* pHeadUnit = pGroup->getHeadUnit();

	if (pHeadUnit == NULL)
		return INT_MAX;

	if( pHeadUnit->isSpy() )
	{
		return 0;
	}

	if (pHeadUnit->getDomainType() != DOMAIN_LAND)
	{
		if (pHeadUnit->bombardRate() > 0)
			return 1;

		if (pHeadUnit->hasCargo())
		{
			if (pHeadUnit->specialCargo() != NO_SPECIALUNIT)
				return 2;
			else
				return 3;
		}

		if (pHeadUnit->getDomainType() == DOMAIN_AIR)
		{
			if (pHeadUnit->canAirDefend() || pHeadUnit->evasionProbability() > 10)
				return 4;
			else
				return 5;
		}

		if (pHeadUnit->canFight())
		{
			if (pHeadUnit->collateralDamage() > 0)
				return 6;
			else
				return 7;
		}
		else
			return 8;
	}

	FAssert(pHeadUnit->getDomainType() == DOMAIN_LAND);

	if (pHeadUnit->AI_getUnitAIType() == UNITAI_WORKER)
		return 9;

	if (pHeadUnit->AI_getUnitAIType() == UNITAI_EXPLORE)
		return 10;

	if (pHeadUnit->bombardRate() > 0)
		return 11;

	if (pHeadUnit->collateralDamage() > 0)
		return 12;

	if (pGroup->isStranded())
		return 505;

	if (pHeadUnit->canFight())
	{
		int iPriority = 65; // allow + or - 50

		iPriority += (GC.getGameINLINE().getBestLandUnitCombat()*100 - pHeadUnit->currCombatStr(NULL, NULL) + 10) / 20; // note: currCombatStr has a factor of 100 built in.

		if (pGroup->getNumUnits() > 1)
		{
			iPriority--;
			if (pGroup->getNumUnits() > 4)
				iPriority--;
		}
		else if (pGroup->AI_getMissionAIType() == MISSIONAI_GUARD_CITY)
			iPriority++;

		if (pHeadUnit->withdrawalProbability() > 0 || pHeadUnit->noDefensiveBonus())
		{
			iPriority--;
			if (pHeadUnit->withdrawalProbability() > 20)
				iPriority--;
		}

		switch (pHeadUnit->AI_getUnitAIType())
		{
		case UNITAI_ATTACK_CITY:
		case UNITAI_ATTACK:
			iPriority--;
		case UNITAI_COUNTER:
			iPriority--;
		case UNITAI_PILLAGE:
		case UNITAI_RESERVE:
			iPriority--;
		case UNITAI_CITY_DEFENSE:
		case UNITAI_CITY_COUNTER:
		case UNITAI_CITY_SPECIAL:
			break;

		case UNITAI_COLLATERAL:
			FAssertMsg(false, "AI_movementPriority: unit AI type doesn't seem to match traits.");
			break;

		default:
			break;
		}
	}
	/* old code
		if (pHeadUnit->canFight())
		{
			if (pHeadUnit->withdrawalProbability() > 20)
			{
				return 9;
			}

			if (pHeadUnit->withdrawalProbability() > 0)
			{
				return 10;
			}

			iCurrCombat = pHeadUnit->currCombatStr(NULL, NULL);
			iBestCombat = (GC.getGameINLINE().getBestLandUnitCombat() * 100);

			if (pHeadUnit->noDefensiveBonus())
			{
				iCurrCombat *= 3;
				iCurrCombat /= 2;
			}

			if (pHeadUnit->AI_isCityAIType())
			{
				iCurrCombat /= 2;
			}

			if (iCurrCombat > iBestCombat)
			{
				return 11;
			}
			else if (iCurrCombat > ((iBestCombat * 4) / 5))
			{
				return 12;
			}
			else if (iCurrCombat > ((iBestCombat * 3) / 5))
			{
				return 13;
			}
			else if (iCurrCombat > ((iBestCombat * 2) / 5))
			{
				return 14;
			}
			else if (iCurrCombat > ((iBestCombat * 1) / 5))
			{
				return 15;
			}
			else
			{
				return 16;
			}
		}
	*/

	return 200;
}


void CvPlayerAI::AI_unitUpdate()
{
	PROFILE_FUNC();

	CLLNode<int>* pCurrUnitNode;

	FAssert(m_groupCycle.getLength() == m_selectionGroups.getCount());

	if (!hasBusyUnit())
	{
		pCurrUnitNode = headGroupCycleNode();

		while (pCurrUnitNode != NULL)
		{
			CvSelectionGroup* pLoopSelectionGroup = getSelectionGroup(pCurrUnitNode->m_data);
			pCurrUnitNode = nextGroupCycleNode(pCurrUnitNode);

			if (pLoopSelectionGroup->AI_isForceSeparate())
			{
				// do not split groups that are in the midst of attacking
				if (pLoopSelectionGroup->isForceUpdate() || !pLoopSelectionGroup->AI_isGroupAttack())
				{
					pLoopSelectionGroup->AI_separate();	// pointers could become invalid...
				}
			}
		}

		/* original bts code
		if (isHuman())
		{
			pCurrUnitNode = headGroupCycleNode();

			while (pCurrUnitNode != NULL)
			{
				CvSelectionGroup* pLoopSelectionGroup = getSelectionGroup(pCurrUnitNode->m_data);
				pCurrUnitNode = nextGroupCycleNode(pCurrUnitNode);

				if (pLoopSelectionGroup->AI_update())
				{
					break; // pointers could become invalid...
				}
			}
		}
		else */
		if (!isHuman()) // K-Mod - automated actions are no longer done from this function. Instead, they are done in CvGame::updateMoves
		{
			/* original bts code
			tempGroupCycle.clear();
			finalGroupCycle.clear();

			pCurrUnitNode = headGroupCycleNode();

			while (pCurrUnitNode != NULL)
			{
				tempGroupCycle.insertAtEnd(pCurrUnitNode->m_data);
				pCurrUnitNode = nextGroupCycleNode(pCurrUnitNode);
			}

			iValue = 0;

			while (tempGroupCycle.getLength() > 0)
			{
				pCurrUnitNode = tempGroupCycle.head();

				while (pCurrUnitNode != NULL)
				{
					pLoopSelectionGroup = getSelectionGroup(pCurrUnitNode->m_data);
					FAssertMsg(pLoopSelectionGroup != NULL, "selection group node with NULL selection group");

					if (AI_movementPriority(pLoopSelectionGroup) <= iValue)
					{
						finalGroupCycle.insertAtEnd(pCurrUnitNode->m_data);
						pCurrUnitNode = tempGroupCycle.deleteNode(pCurrUnitNode);
					}
					else
					{
						pCurrUnitNode = tempGroupCycle.next(pCurrUnitNode);
					}
				}

				iValue++;
			}

			pCurrUnitNode = finalGroupCycle.head();

			while (pCurrUnitNode != NULL)
			{
				pLoopSelectionGroup = getSelectionGroup(pCurrUnitNode->m_data);

				if (NULL != pLoopSelectionGroup)  // group might have been killed by a previous group update
				{
					if (pLoopSelectionGroup->AI_update())
					{
						break; // pointers could become invalid...
					}
				}

				pCurrUnitNode = finalGroupCycle.next(pCurrUnitNode);
			} */
			// K-Mod. It seems to me that all we're trying to do is run AI_update
			// on the highest priority groups until one of them becomes busy.
			// ... if only they had used the STL, this would be a lot easier.
			bool bRepeat = true;
			do
			{
				std::vector<std::pair<int, int> > groupList;

				pCurrUnitNode = headGroupCycleNode();
				while (pCurrUnitNode != NULL)
				{
					CvSelectionGroup* pLoopSelectionGroup = getSelectionGroup(pCurrUnitNode->m_data);
					FAssert(pLoopSelectionGroup != NULL);

					int iPriority = AI_movementPriority(pLoopSelectionGroup);
					groupList.push_back(std::make_pair(iPriority, pCurrUnitNode->m_data));

					pCurrUnitNode = nextGroupCycleNode(pCurrUnitNode);
				}
				FAssert(groupList.size() == getNumSelectionGroups());

				std::sort(groupList.begin(), groupList.end());
				for (size_t i = 0; i < groupList.size(); i++)
				{
					CvSelectionGroup* pLoopSelectionGroup = getSelectionGroup(groupList[i].second);
					if (pLoopSelectionGroup && pLoopSelectionGroup->AI_update())
					{
						bRepeat = false;
						break;
					}
				}

				// one last trick that might save us a bit of time...
				// if the number of selection groups has increased, then lets try to take care of the new groups right away.
				// (there might be a faster way to look for the new groups, but I don't know it.)
				bRepeat = bRepeat && getNumSelectionGroups() > (int)groupList.size();
				// the repeat will do a stack of redundant checks,
				// but I still expect it to be much faster than waiting for the next turnslice.
			} while (bRepeat);
			// K-Mod end
		}
	}
}


void CvPlayerAI::AI_makeAssignWorkDirty()
{
	CvCity* pLoopCity;
	int iLoop;

	for (pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		pLoopCity->AI_setAssignWorkDirty(true);
	}
}


void CvPlayerAI::AI_assignWorkingPlots()
{
	CvCity* pLoopCity;
	int iLoop;

	for (pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		pLoopCity->AI_assignWorkingPlots();
	}
}


void CvPlayerAI::AI_updateAssignWork()
{
	CvCity* pLoopCity;
	int iLoop;

	for (pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		pLoopCity->AI_updateAssignWork();
	}
}

void CvPlayerAI::AI_doCentralizedProduction()
{
	PROFILE_FUNC();

	CvCity* pLoopCity;
	int iLoop;
	int iI;

	if( isHuman() )
	{
		return;
	}

	if( isBarbarian() )
	{
		return;
	}

	// Choose which cities should build floating defenders
	// evaluate military cities score and rank.
	// choose military cities that aren't building anything, or wait for good military cities to finish building.
	/*
	bool bCrushStrategy = kPlayer.AI_isDoStrategy(AI_STRATEGY_CRUSH);
	int iNeededFloatingDefenders = (isBarbarian() || bCrushStrategy) ?  0 : kPlayer.AI_getTotalFloatingDefendersNeeded(pArea);
 	int iTotalFloatingDefenders = (isBarbarian() ? 0 : kPlayer.AI_getTotalFloatingDefenders(pArea));
	
	UnitTypeWeightArray floatingDefenderTypes;
	floatingDefenderTypes.push_back(std::make_pair(UNITAI_CITY_DEFENSE, 125));
	floatingDefenderTypes.push_back(std::make_pair(UNITAI_CITY_COUNTER, 100));
	//floatingDefenderTypes.push_back(std::make_pair(UNITAI_CITY_SPECIAL, 0));
	floatingDefenderTypes.push_back(std::make_pair(UNITAI_RESERVE, 100));
	floatingDefenderTypes.push_back(std::make_pair(UNITAI_COLLATERAL, 100));
	
	if (iTotalFloatingDefenders < ((iNeededFloatingDefenders + 1) / (bGetBetterUnits ? 3 : 2)))
	if (!bExempt && AI_chooseLeastRepresentedUnit(floatingDefenderTypes))
	{
		if( gCityLogLevel >= 2 ) logBBAI("      City %S uses choose floating defender 1", getName().GetCString());
		return;
	}
	*/

	// Cycle through all buildings looking for limited buildings (wonders)
	// Identify the cities that will benefit the most. If the best city is not busy, build the wonder.
	/*
	for (iI = 0; iI < GC.getNumBuildingClassInfos(); iI++)
	{
		if (!(GET_PLAYER(getOwnerINLINE()).isBuildingClassMaxedOut(((BuildingClassTypes)iI), GC.getBuildingClassInfo((BuildingClassTypes)iI).getExtraPlayerInstances())))
		{
			BuildingTypes eLoopBuilding = (BuildingTypes)GC.getCivilizationInfo(getCivilizationType()).getCivilizationBuildings(iI);
			bool bWorld = isWorldWonderClass((BuildingClassTypes)iI);
			bool bNational = isNationalWonderClass((BuildingClassTypes)iI);

						if (canConstruct(eLoopBuilding))
						{
							// city loop.
							iValue = AI_buildingValueThreshold(eLoopBuilding);

							if (GC.getBuildingInfo(eLoopBuilding).getFreeBuildingClass() != NO_BUILDINGCLASS)
							{
								BuildingTypes eFreeBuilding = (BuildingTypes)GC.getCivilizationInfo(getCivilizationType()).getCivilizationBuildings(GC.getBuildingInfo(eLoopBuilding).getFreeBuildingClass());
								if (NO_BUILDING != eFreeBuilding)
								{
									iValue += (AI_buildingValue(eFreeBuilding, iFocusFlags) * (GET_PLAYER(getOwnerINLINE()).getNumCities() - GET_PLAYER(getOwnerINLINE()).getBuildingClassCountPlusMaking((BuildingClassTypes)GC.getBuildingInfo(eLoopBuilding).getFreeBuildingClass())));
								}
							}

							if (iValue > 0)
							{
								iTurnsLeft = getProductionTurnsLeft(eLoopBuilding, 0);

								if (isWorldWonderClass((BuildingClassTypes)iI))
								{
									if (iProductionRank <= std::min(3, ((GET_PLAYER(getOwnerINLINE()).getNumCities() + 2) / 3)))
									{
										if (bAsync)
										{
											iTempValue = GC.getASyncRand().get(GC.getLeaderHeadInfo(getPersonalityType()).getWonderConstructRand(), "Wonder Construction Rand ASYNC");
										}
										else
										{
											iTempValue = GC.getGameINLINE().getSorenRandNum(GC.getLeaderHeadInfo(getPersonalityType()).getWonderConstructRand(), "Wonder Construction Rand");
										}
										
										if (bAreaAlone)
										{
											iTempValue *= 2;
										}
										iValue += iTempValue;
									}
								}

								if (bAsync)
								{
									iValue *= (GC.getASyncRand().get(25, "AI Best Building ASYNC") + 100);
									iValue /= 100;
								}
								else
								{
									iValue *= (GC.getGameINLINE().getSorenRandNum(25, "AI Best Building") + 100);
									iValue /= 100;
								}

								iValue += getBuildingProduction(eLoopBuilding);
								
								
								bool bValid = ((iMaxTurns <= 0) ? true : false);
								if (!bValid)
								{
									bValid = (iTurnsLeft <= GC.getGameINLINE().AI_turnsPercent(iMaxTurns, GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).getConstructPercent()));
								}
								if (!bValid)
								{
									for (int iHurry = 0; iHurry < GC.getNumHurryInfos(); ++iHurry)
									{
										if (canHurryBuilding((HurryTypes)iHurry, eLoopBuilding, true))
										{
											if (AI_getHappyFromHurry((HurryTypes)iHurry, eLoopBuilding, true) > 0)
											{
												bValid = true;
												break;
											}
										}
									}
								}

								if (bValid)
								{
									FAssert((MAX_INT / 1000) > iValue);
									iValue *= 1000;
									iValue /= std::max(1, (iTurnsLeft + 3));

									iValue = std::max(1, iValue);

									if (iValue > iBestValue)
									{
										iBestValue = iValue;
										eBestBuilding = eLoopBuilding;
									}
								}
							}
						}
					}
				}
			}
		}
	}
	return eBestBuilding;
	*/


// K-Mod end, the rest is some unfinished code from BBAI

	// BBAI TODO: Temp testing
	//if( getID() % 2 == 1 )
	//{
		return;
	//}
	
	// Determine number of cities player can use building wonders currently
	int iMaxNumWonderCities = 1 + getNumCities()/5;
	bool bIndustrious = (getMaxPlayerBuildingProductionModifier() > 0);
	bool bAtWar = (GET_TEAM(getTeam()).getAtWarCount(true) > 0);

	if( bIndustrious )
	{
		iMaxNumWonderCities += 1;
	}

	// Dagger?
	// Power?
	// Research?

	if( bAtWar )
	{
		int iWarSuccessRating = GET_TEAM(getTeam()).AI_getWarSuccessRating();
		if( iWarSuccessRating < -90 )
		{
			iMaxNumWonderCities = 0;
		}
		else 
		{
			if( iWarSuccessRating < 30 )
			{
				iMaxNumWonderCities -= 1;
			}
			if( iWarSuccessRating < -50 )
			{
				iMaxNumWonderCities /= 2;
			}
		}
	}

	if( isMinorCiv() && (GET_TEAM(getTeam()).getHasMetCivCount(false) > 1) )
	{
		iMaxNumWonderCities /= 2;
	}

	iMaxNumWonderCities = std::min(iMaxNumWonderCities, getNumCities());

	// Gather city statistics
	// Could rank cities based on gold, production here, could be O(n) instead of O(n^2)
	int iWorldWonderCities = 0;
	int iLimitedWonderCities = 0;
	int iNumDangerCities = 0;
	for (pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		if( pLoopCity->isProductionBuilding() )
		{
			if( isLimitedWonderClass((BuildingClassTypes)(GC.getBuildingInfo(pLoopCity->getProductionBuilding()).getBuildingClassType())))
			{
				iLimitedWonderCities++;

				if (isWorldWonderClass((BuildingClassTypes)(GC.getBuildingInfo(pLoopCity->getProductionBuilding()).getBuildingClassType())))
				{
					iWorldWonderCities++;
				}
			}
		}

		if( pLoopCity->isProductionProject() )
		{
			if( isLimitedProject(pLoopCity->getProductionProject()))
			{
				iLimitedWonderCities++;
				if( isWorldProject(pLoopCity->getProductionProject()))
				{
					iWorldWonderCities++;
				}
			}
		}
	}

	// Check for any global wonders to build
	for (iI = 0; iI < GC.getNumBuildingClassInfos(); iI++)
	{
		if (isWorldWonderClass((BuildingClassTypes)iI))
		{

			//canConstruct(
		}
	}

	// Check for any projects to build
	for (iI = 0; iI < GC.getNumProjectInfos(); iI++)
	{
		
	}

	// Check for any national/team wonders to build
	
}

void CvPlayerAI::AI_makeProductionDirty()
{
	CvCity* pLoopCity;
	int iLoop;

	FAssertMsg(!isHuman(), "isHuman did not return false as expected");

	for (pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		pLoopCity->AI_setChooseProductionDirty(true);
	}
}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      05/16/10                              jdog5000        */
/*                                                                                              */
/* War tactics AI                                                                               */
/************************************************************************************************/
void CvPlayerAI::AI_conquerCity(CvCity* pCity)
{
	bool bRaze = false;

	if (canRaze(pCity))
	{
	    int iRazeValue = 0;
		int iCloseness = pCity->AI_playerCloseness(getID());

		// Reasons to always raze
		if( 2*pCity->getCulture(pCity->getPreviousOwner()) > pCity->getCultureThreshold(GC.getGameINLINE().culturalVictoryCultureLevel()) )
		{
			CvCity* pLoopCity;
			int iLoop;
			int iHighCultureCount = 1;

			for( pLoopCity = GET_PLAYER(pCity->getPreviousOwner()).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(pCity->getPreviousOwner()).nextCity(&iLoop) )
			{
				if( 2*pLoopCity->getCulture(pCity->getPreviousOwner()) > pLoopCity->getCultureThreshold(GC.getGameINLINE().culturalVictoryCultureLevel()) )
				{
					iHighCultureCount++;
					if( iHighCultureCount >= GC.getGameINLINE().culturalVictoryNumCultureCities() )
					{
						//Raze city enemy needs for cultural victory unless we greatly over power them
						logBBAI( "  Razing enemy cultural victory city" );
						bRaze = true;
					}
				}
			}
		}

		if( !bRaze )
		{
			// Reasons to not raze
			if( (getNumCities() <= 1) || (getNumCities() < 5 && iCloseness > 0) )
			{
				if( gPlayerLogLevel >= 1 )
				{
					logBBAI("    Player %d (%S) decides not to raze %S because they have few cities", getID(), getCivilizationDescription(0), pCity->getName().GetCString() );
				}
			}
			else if( AI_isDoVictoryStrategy(AI_VICTORY_DOMINATION3) && GET_TEAM(getTeam()).AI_isPrimaryArea(pCity->area()) )
			{
				// Do not raze, going for domination
				if( gPlayerLogLevel >= 1 )
				{
					logBBAI("    Player %d (%S) decides not to raze %S because they're going for domination", getID(), getCivilizationDescription(0), pCity->getName().GetCString() );
				}
			}
			else if( isBarbarian() )
			{
				if ( !(pCity->isHolyCity()) && !(pCity->hasActiveWorldWonder()))
				{
					if( (pCity->getPreviousOwner() != BARBARIAN_PLAYER) && (pCity->getOriginalOwner() != BARBARIAN_PLAYER) )
					{
						iRazeValue += GC.getLeaderHeadInfo(getPersonalityType()).getRazeCityProb();
						iRazeValue -= iCloseness;
					}
				}
			}	
			else
			{
				bool bFinancialTrouble = AI_isFinancialTrouble();
				bool bBarbCity = (pCity->getPreviousOwner() == BARBARIAN_PLAYER) && (pCity->getOriginalOwner() == BARBARIAN_PLAYER);
				bool bPrevOwnerBarb = (pCity->getPreviousOwner() == BARBARIAN_PLAYER);
				bool bTotalWar = GET_TEAM(getTeam()).AI_getWarPlan(GET_PLAYER(pCity->getPreviousOwner()).getTeam()) == WARPLAN_TOTAL; // K-Mod
				
				/* original code
				if (GET_TEAM(getTeam()).countNumCitiesByArea(pCity->area()) == 0)
				{
					// Conquered city in new continent/island
					int iBestValue;

					if( pCity->area()->getNumCities() == 1 && AI_getNumAreaCitySites(pCity->area()->getID(), iBestValue) == 0 )
					{
						// Probably small island
						if( iCloseness == 0 )
						{
							// Safe to raze these now that AI can do pick up ...
							iRazeValue += GC.getLeaderHeadInfo(getPersonalityType()).getRazeCityProb();
						}
					}
					else
					{
						// At least medium sized island
						if( iCloseness < 10 )
						{
							if( bFinancialTrouble )
							{
								// Raze if we might start incuring colony maintenance
								iRazeValue = 100;
							}
							else
							{
								if (pCity->getPreviousOwner() != NO_PLAYER && !bPrevOwnerBarb)
								{
									if (GET_TEAM(GET_PLAYER(pCity->getPreviousOwner()).getTeam()).countNumCitiesByArea(pCity->area()) > 3)
									{
										iRazeValue += GC.getLeaderHeadInfo(getPersonalityType()).getRazeCityProb();
									}
								}
							}
						}
					}
				}
				else */ // K-Mod disabled. I don't see the point treating the above as a special case.
				{
					// K-Mod (moved this from above)
					int iUnused;

					if (pCity->area()->getNumCities() == 1 && AI_getNumAreaCitySites(pCity->area()->getID(), iUnused) == 0)
					{
						// Probably small island
						if (iCloseness == 0)
							iRazeValue += 20;
					}
					// K-Mod end

					// Distance related aspects
					if (iCloseness > 0)
					{
						iRazeValue -= iCloseness;
					}
					else
					{
						iRazeValue += 40;

						CvCity* pNearestTeamAreaCity = GC.getMapINLINE().findCity(pCity->getX_INLINE(), pCity->getY_INLINE(), NO_PLAYER, getTeam(), true, false, NO_TEAM, NO_DIRECTION, pCity);

						/*if( pNearestTeamAreaCity == NULL )
						{
							// Shouldn't happen
							iRazeValue += 30;
						} */
						// K-Mod
						if (pNearestTeamAreaCity == NULL)
						{
							if (bTotalWar && GET_TEAM(GET_PLAYER(pCity->getPreviousOwner()).getTeam()).AI_isPrimaryArea(pCity->area()))
								iRazeValue += 5;
							else
								iRazeValue += 30;
						}
						// K-Mod end
						else
						{
							int iDistance = plotDistance(pCity->getX_INLINE(), pCity->getY_INLINE(), pNearestTeamAreaCity->getX_INLINE(), pNearestTeamAreaCity->getY_INLINE());
							iDistance -= DEFAULT_PLAYER_CLOSENESS + 2;
							if ( iDistance > 0 )
							{
								iRazeValue += iDistance * (bBarbCity ? 8 : 5);
							}
						}
					}

					if (bFinancialTrouble)
					{
						iRazeValue += std::max(0, (70 - 15 * pCity->getPopulation()));
					}

					// Scale down distance/maintenance effects for organized. (disabled by K-Mod)
					/*
					if( iRazeValue > 0 )
					{
						for (iI = 0; iI < GC.getNumTraitInfos(); iI++)
						{
							if (hasTrait((TraitTypes)iI))
							{
								iRazeValue *= (100 - (GC.getTraitInfo((TraitTypes)iI).getUpkeepModifier()));
								iRazeValue /= 100;

								if( (GC.getTraitInfo((TraitTypes)iI).getUpkeepModifier() > 0) && gPlayerLogLevel >= 1 )
								{
									logBBAI("      Reduction for upkeep modifier %d", (GC.getTraitInfo((TraitTypes)iI).getUpkeepModifier()) );
								}
							}
						}
					} */

					// Non-distance related aspects
					iRazeValue += GC.getLeaderHeadInfo(getPersonalityType()).getRazeCityProb();

					// K-Mod
					iRazeValue -= AI_isDoVictoryStrategy(AI_VICTORY_DOMINATION2) ? 20 : 0;
					// K-Mod end
		                
					if (getStateReligion() != NO_RELIGION)
					{
						if (pCity->isHasReligion(getStateReligion()))
						{
							if (GET_TEAM(getTeam()).hasShrine(getStateReligion()))
							{
								iRazeValue -= 50;

								if( gPlayerLogLevel >= 1 )
								{
									logBBAI("      Reduction for state religion with shrine" );
								}
							}
							else
							{
								iRazeValue -= 10;

								if( gPlayerLogLevel >= 1 )
								{
									logBBAI("      Reduction for state religion" );
								}
							}
						}
					}
					// K-Mod
					else
					{
						iRazeValue -= 5; // Free religion does not mean we hate everyone equally...
					}
					// K-Mod end
				}


				for (int iI = 0; iI < GC.getNumReligionInfos(); iI++)
				{
					if (pCity->isHolyCity((ReligionTypes)iI))
					{
						logBBAI("      Reduction for holy city" );
						if( getStateReligion() == iI )
						{
							iRazeValue -= 150;
						}
						else
						{
							iRazeValue -= 5 + GC.getGameINLINE().calculateReligionPercent((ReligionTypes)iI);
						}
					}
				}

				// K-Mod
				// corp HQ value.
				for (int iI = 0; iI < GC.getNumCorporationInfos(); iI++)
				{
					if (pCity->isHeadquarters((CorporationTypes)iI))
					{
						logBBAI("      Reduction for corp headquarters");
						iRazeValue -= 10 + 100 * GC.getGameINLINE().countCorporationLevels((CorporationTypes)iI) / GC.getGameINLINE().getNumCities();
					}
				}
				// great people
				iRazeValue -= 2 * pCity->getNumGreatPeople();
				iRazeValue += bBarbCity ? 5 : 0;
				// K-Mod end

				iRazeValue -= 15 * pCity->getNumActiveWorldWonders();

				CvPlot* pLoopPlot = NULL;
				for (int iI = 0; iI < NUM_CITY_PLOTS; iI++)
				{
					pLoopPlot = plotCity(pCity->getX_INLINE(), pCity->getY_INLINE(), iI);

					if (pLoopPlot != NULL)
					{
						if (pLoopPlot->getBonusType(getTeam()) != NO_BONUS)
						{
							iRazeValue -= std::max(2, AI_bonusVal(pLoopPlot->getBonusType(getTeam()))/2);
						}
					}
				}

				// More inclined to raze if we're unlikely to hold it
				if( GET_TEAM(getTeam()).getPower(false)*10 < GET_TEAM(GET_PLAYER(pCity->getPreviousOwner()).getTeam()).getPower(true)*8 )
				{
					int iTempValue = 20;
					iTempValue *= (GET_TEAM(GET_PLAYER(pCity->getPreviousOwner()).getTeam()).getPower(true) - GET_TEAM(getTeam()).getPower(false));
					iTempValue /= std::max( 100, GET_TEAM(getTeam()).getPower(false) );

					logBBAI("      Low power, so boost raze odds by %d", std::min( 75, iTempValue ) );

					iRazeValue += std::min( 75, iTempValue );
				}

				if( gPlayerLogLevel >= 1 )
				{
					if( bBarbCity ) logBBAI("      %S is a barb city", pCity->getName().GetCString() );
					if( bPrevOwnerBarb ) logBBAI("      %S was last owned by barbs", pCity->getName().GetCString() );
					logBBAI("      %S has area cities %d, closeness %d, bFinTrouble %d", pCity->getName().GetCString(), GET_TEAM(getTeam()).countNumCitiesByArea(pCity->area()), iCloseness, bFinancialTrouble );
				}
			}

			if( gPlayerLogLevel >= 1 )
			{
				logBBAI("    Player %d (%S) has odds %d to raze city %S", getID(), getCivilizationDescription(0), iRazeValue, pCity->getName().GetCString() );
			}
						
			if (iRazeValue > 0)
			{
				if (GC.getGameINLINE().getSorenRandNum(100, "AI Raze City") < iRazeValue)
				{
					bRaze = true;
				}
			}
		}	
	}

	if( bRaze )
	{
		logBBAI("    Player %d (%S) decides to to raze city %S!!!", getID(), getCivilizationDescription(0), pCity->getName().GetCString() );
		pCity->doTask(TASK_RAZE);
		//logBBAI("    Player %d (%S) decides to to raze city %S!!!", getID(), getCivilizationDescription(0), pCity->getName().GetCString() );
		// K-Mod moved the log message up - otherwise it will crash due to pCity being deleted!
	}
	else
	{
/************************************************************************************************/
/* UNOFFICIAL_PATCH                       06/14/09                       Maniac & jdog5000      */
/*                                                                                              */
/*                                                                                              */
/************************************************************************************************/
/* original bts code
		CvEventReporter::getInstance().cityAcquiredAndKept(GC.getGameINLINE().getActivePlayer(), pCity);
*/
		CvEventReporter::getInstance().cityAcquiredAndKept(getID(), pCity);
/************************************************************************************************/
/* UNOFFICIAL_PATCH                        END                                                  */
/************************************************************************************************/
	}
}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/


bool CvPlayerAI::AI_acceptUnit(CvUnit* pUnit) const
{
	if (isHuman())
	{
		return true;
	}

	if (AI_isFinancialTrouble())
	{
		if (pUnit->AI_getUnitAIType() == UNITAI_WORKER)
		{
			if (AI_neededWorkers(pUnit->area()) > 0)
			{
				return true;
			}
		}

		if (pUnit->AI_getUnitAIType() == UNITAI_WORKER_SEA)
		{
			return true;
		}

		if (pUnit->AI_getUnitAIType() == UNITAI_MISSIONARY)
		{
			return true; //XXX
		}

		// K-Mod
		switch (pUnit->AI_getUnitAIType())
		{
		case UNITAI_PROPHET:
		case UNITAI_ARTIST:
		case UNITAI_SCIENTIST:
		case UNITAI_GENERAL:
		case UNITAI_MERCHANT:
		case UNITAI_ENGINEER:
		case UNITAI_GREAT_SPY:
			return true;
		default:
			break;
		}
		// K-Mod end
		return false;
	}

	return true;
}


bool CvPlayerAI::AI_captureUnit(UnitTypes eUnit, CvPlot* pPlot) const
{
	CvCity* pNearestCity;

	FAssert(!isHuman());

	if (pPlot->getTeam() == getTeam())
	{
		return true;
	}

	pNearestCity = GC.getMapINLINE().findCity(pPlot->getX_INLINE(), pPlot->getY_INLINE(), NO_PLAYER, getTeam());

	if (pNearestCity != NULL)
	{
		if (plotDistance(pPlot->getX_INLINE(), pPlot->getY_INLINE(), pNearestCity->getX_INLINE(), pNearestCity->getY_INLINE()) <= 4)
		{
			return true;
		}
	}

	return false;
}


DomainTypes CvPlayerAI::AI_unitAIDomainType(UnitAITypes eUnitAI) const
{
	switch (eUnitAI)
	{
	case UNITAI_UNKNOWN:
		return NO_DOMAIN;
		break;

	case UNITAI_ANIMAL:
	case UNITAI_SETTLE:
	case UNITAI_WORKER:
	case UNITAI_ATTACK:
	case UNITAI_ATTACK_CITY:
	case UNITAI_COLLATERAL:
	case UNITAI_PILLAGE:
	case UNITAI_RESERVE:
	case UNITAI_COUNTER:
	case UNITAI_PARADROP:
	case UNITAI_CITY_DEFENSE:
	case UNITAI_CITY_COUNTER:
	case UNITAI_CITY_SPECIAL:
	case UNITAI_EXPLORE:
	case UNITAI_MISSIONARY:
	case UNITAI_PROPHET:
	case UNITAI_ARTIST:
	case UNITAI_SCIENTIST:
	case UNITAI_GENERAL:
	case UNITAI_MERCHANT:
	case UNITAI_ENGINEER:
	case UNITAI_GREAT_SPY: // K-Mod
	case UNITAI_SPY:
	case UNITAI_ATTACK_CITY_LEMMING:
		return DOMAIN_LAND;
		break;

	case UNITAI_ICBM:
		return DOMAIN_IMMOBILE;
		break;

	case UNITAI_WORKER_SEA:
	case UNITAI_ATTACK_SEA:
	case UNITAI_RESERVE_SEA:
	case UNITAI_ESCORT_SEA:
	case UNITAI_EXPLORE_SEA:
	case UNITAI_ASSAULT_SEA:
	case UNITAI_SETTLER_SEA:
	case UNITAI_MISSIONARY_SEA:
	case UNITAI_SPY_SEA:
	case UNITAI_CARRIER_SEA:
	case UNITAI_MISSILE_CARRIER_SEA:
	case UNITAI_PIRATE_SEA:
		return DOMAIN_SEA;
		break;

	case UNITAI_ATTACK_AIR:
	case UNITAI_DEFENSE_AIR:
	case UNITAI_CARRIER_AIR:
	case UNITAI_MISSILE_AIR:
		return DOMAIN_AIR;
		break;

	default:
		FAssert(false);
		break;
	}

	return NO_DOMAIN;
}


int CvPlayerAI::AI_yieldWeight(YieldTypes eYield, const CvCity* pCity) const // K-Mod added city argument
{
	/* original bts code
	if (eYield == YIELD_PRODUCTION)
	{
		int iProductionModifier = 100 + (30 * std::max(0, GC.getGame().getCurrentEra() - 1) / std::max(1, (GC.getNumEraInfos() - 2)));
		return (GC.getYieldInfo(eYield).getAIWeightPercent() * iProductionModifier) / 100;
	}

	return GC.getYieldInfo(eYield).getAIWeightPercent(); */

	// K-Mod. In the past, this function was always bundled with some code to boost the value of production and food...
	// For simplicity and consistency, I've brought the adjustments into this function.
	PROFILE_FUNC();

	int iWeight = GC.getYieldInfo(eYield).getAIWeightPercent();
	switch (eYield)
	{
	case YIELD_FOOD:
		if (pCity)
		{
			iWeight *= (pCity->happyLevel() - pCity->unhappyLevel(1) - pCity->foodDifference() >= 0) ? 3 : 2;
		}
		else
		{
			iWeight *= 250;
			iWeight /= 100;
		}
		break;
	case YIELD_PRODUCTION:
		iWeight *= 2;
		break;
	case YIELD_COMMERCE:
		if (AI_isFinancialTrouble())
		{
			iWeight *= 2;
		}
		break;
	}
	return iWeight;
	// K-Mod end
}


int CvPlayerAI::AI_commerceWeight(CommerceTypes eCommerce, const CvCity* pCity) const
{
	int iWeight;

	iWeight = GC.getCommerceInfo(eCommerce).getAIWeightPercent();

	//sorry but the merchant descrimination must stop.
	iWeight = std::min(110, iWeight);
	
	//XXX Add something for 100%/0% type situations
	switch (eCommerce)
	{
	case COMMERCE_RESEARCH:
		if (AI_avoidScience())
		{
			if (isNoResearchAvailable())
			{
				iWeight = 0;
			}
			else
			{
				iWeight /= 8;
			}
		}
		break;
	case COMMERCE_GOLD:
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      06/12/09                                jdog5000      */
/*                                                                                              */
/* Gold AI                                                                        */
/************************************************************************************************/
		if (getCommercePercent(COMMERCE_GOLD) > 70)
		{
			//avoid strikes
			if (getGoldPerTurn() < -getGold()/100)
			{
				iWeight += 15;
			}
		}
		else if (getCommercePercent(COMMERCE_GOLD) < 25)
		{
			//put more money towards other commerce types
			int iGoldPerTurn = getGoldPerTurn(); // K-Mod
			if (iGoldPerTurn > -getGold()/40)
			{
				iWeight -= 25 - getCommercePercent(COMMERCE_GOLD);
				iWeight -= (iGoldPerTurn > 0 && getCommercePercent(COMMERCE_GOLD) == 0) ? 10 : 0; // K-Mod. just a bit extra. (I'd like to compare getGold to AI_goldTarget; but it's too expensive.)
			}
		}
		// K-Mod.
		// (note. This factor is helpful for assigning merchant specialists in the right places, but it causes problems
		// when building wealth. Really we dont' want to scale all gold commerce like this, we only want to scale it when
		// it is independant of the commerce slider.)
		if (pCity != NULL)
		{
			iWeight *= pCity->getTotalCommerceRateModifier(COMMERCE_GOLD);
			iWeight /= AI_averageCommerceMultiplier(COMMERCE_GOLD);
		}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
		break;
	case COMMERCE_CULTURE:
		// COMMERCE_CULTURE AIWeightPercent is 25% in default xml
		// (significantly changed by bbai & K-Mod)
		// Adjustments for human player going for cultural victory (who won't have AI strategy set) 
		// so that governors do smart things
		if (pCity != NULL)
		{
			int iPressureWeight = iWeight * pCity->culturePressureFactor() / 100; // K-Mod

			if (pCity->getCultureTimes100(getID()) >= 100 * GC.getGameINLINE().getCultureThreshold((CultureLevelTypes)(GC.getNumCultureLevelInfos() - 1)))
			{
				iWeight /= 50;
			}
			// Slider check works for detection of whether human player is going for cultural victory
			// (all weights changed for K-Mod)
			else if (AI_isDoVictoryStrategy(AI_VICTORY_CULTURE2) || getCommercePercent(COMMERCE_CULTURE) >= 70)
			{
				int iCultureRateRank = pCity->findCommerceRateRank(COMMERCE_CULTURE);
				int iCulturalVictoryNumCultureCities = GC.getGameINLINE().culturalVictoryNumCultureCities();
				bool bC3 = AI_isDoVictoryStrategy(AI_VICTORY_CULTURE3) || getCommercePercent(COMMERCE_CULTURE); // K-Mod
				
				// if one of the currently best cities, then focus hard
				if (iCultureRateRank <= iCulturalVictoryNumCultureCities)
				{
					if (bC3)
					{
						// culture3
						iWeight *= 4 + iCultureRateRank;
					}
					else
					{
						// culture2
						iWeight *= 7;
						iWeight /= 2;
					}
				}
				// if one of the 3 close to the top, then still emphasize culture some
				else if (iCultureRateRank <= iCulturalVictoryNumCultureCities + 3)
				{
					iWeight *= bC3 ? 4 : 3;
				}
				else
				{
					iWeight *= 2;
				}
			}
			else if (AI_isDoVictoryStrategy(AI_VICTORY_CULTURE1) || getCommercePercent(COMMERCE_CULTURE) >= 50)
			{
				iWeight *= 2;
			}
			
			// K-Mod
			//iWeight += (100 - pCity->plot()->calculateCulturePercent(getID()));
			iWeight += iPressureWeight;
			// K-Mod end
			
			/* original bts code
			if (pCity->getCultureLevel() <= (CultureLevelTypes) 1)
			{
				iWeight = std::max(iWeight, 800);
			} */
			// Disabled by K-Mod. This massive weight boost messes up the evaluation of national wonders.
			// We can have this kind of adjustment elsewhere.
			// What we really want is a high value for having non-zero culture - the actual quantity of culture is less important.
		}
		// pCity == NULL
		else
		{
			// weight multipliers changed for K-Mod
			if (AI_isDoVictoryStrategy(AI_VICTORY_CULTURE3) || getCommercePercent(COMMERCE_CULTURE) >= 90)
			{
				iWeight *= 4;
			}
			else if (AI_isDoVictoryStrategy(AI_VICTORY_CULTURE2) || getCommercePercent(COMMERCE_CULTURE) >= 70)
			{
				iWeight *= 3;
			}
			else if (AI_isDoVictoryStrategy(AI_VICTORY_CULTURE1) || getCommercePercent(COMMERCE_CULTURE) >= 50)
			{
				iWeight *= 2;
			}
			iWeight *= AI_averageCulturePressure();
			iWeight /= 100;
		}
		// K-Mod
		if (GET_TEAM(getTeam()).getAnyWarPlanCount(true) > 0)
		{
			if (AI_isDoVictoryStrategy(AI_VICTORY_CULTURE1))
			{
				iWeight *= 2;
				iWeight /= 3;
			}
			else
			{
				iWeight /= 2;
			}
		}
		// K-Mod end
		break;
	case COMMERCE_ESPIONAGE:
		{
			// K-Mod version, based loosely on BBAI code.
			int iEspBehindWeight = 0;
			int iEspAttackWeight = 0;
			int iAllTeamTotalPoints = 0;
			int iTeamCount = 0;
			int iLocalTeamCount = 0;
			int iTotalUnspent = 0;
			for (int iTeam = 0; iTeam < MAX_CIV_TEAMS; ++iTeam)
			{
				CvTeamAI& kLoopTeam = GET_TEAM((TeamTypes)iTeam);

				if (kLoopTeam.isAlive() && iTeam != getTeam() && GET_TEAM(getTeam()).isHasMet((TeamTypes)iTeam) && !kLoopTeam.isVassal(getTeam()) && !GET_TEAM(getTeam()).isVassal((TeamTypes)iTeam))
				{
					iAllTeamTotalPoints += kLoopTeam.getEspionagePointsEver();
					iTeamCount++;

					int iTheirPoints = kLoopTeam.getEspionagePointsAgainstTeam(getTeam());
					int iOurPoints = GET_TEAM(getTeam()).getEspionagePointsAgainstTeam((TeamTypes)iTeam);
					iTotalUnspent += iOurPoints;
					int iAttitude = range(GET_TEAM(getTeam()).AI_getAttitudeVal((TeamTypes)iTeam), -12, 12);
					iTheirPoints -= (iTheirPoints*iAttitude)/(2*12);

					if (iTheirPoints > iOurPoints && (!isHuman() || getEspionageSpendingWeightAgainstTeam((TeamTypes)iTeam) > 0))
					{
						iEspBehindWeight += 1;
						if (kLoopTeam.AI_getAttitude(getTeam()) <= ATTITUDE_CAUTIOUS
							&& GET_TEAM(getTeam()).AI_hasCitiesInPrimaryArea((TeamTypes)iTeam))
						{
							iEspBehindWeight += 1;
						}
					}
					if (GET_TEAM(getTeam()).AI_hasCitiesInPrimaryArea((TeamTypes)iTeam))
					{
						iLocalTeamCount++;
						if (GET_TEAM(getTeam()).AI_getAttitude((TeamTypes)iTeam) <= ATTITUDE_ANNOYED
							|| GET_TEAM(getTeam()).AI_getWarPlan((TeamTypes)iTeam) != NO_WARPLAN)
						{
							iEspAttackWeight += 1;
						}
						if (AI_isDoStrategy(AI_STRATEGY_BIG_ESPIONAGE) && GET_PLAYER(kLoopTeam.getLeaderID()).getTechScore() > getTechScore())
						{
							iEspAttackWeight += 1;
						}
					}
				}
			}
			if (iTeamCount == 0)
				return iWeight / 10; // can't put points on anyone - but accumulating "total points ever" is still better than nothing.

			iAllTeamTotalPoints /= std::max(1, iTeamCount); // Get the average total points
			iWeight *= std::min(GET_TEAM(getTeam()).getEspionagePointsEver() + 3*iAllTeamTotalPoints + 16*GC.getGame().getGameTurn(), 5*iAllTeamTotalPoints);
			iWeight /= std::max(1, 4 * iAllTeamTotalPoints);
			// lower weight if we have spent less than a third of our total points
			if (getCommercePercent(COMMERCE_ESPIONAGE) == 0) // ...only if we aren't explicitly trying to get espionage.
			{
				iWeight *= 100 - 270*std::max(iTotalUnspent - std::max(2*GET_TEAM(getTeam()).getEspionagePointsEver()/3, 8*GC.getGame().getGameTurn()), 0)/std::max(1, GET_TEAM(getTeam()).getEspionagePointsEver());
				iWeight /= 100;
			}
			//
			if (AI_isDoStrategy(AI_STRATEGY_BIG_ESPIONAGE))
			{
				iWeight *= 3;
				iWeight /= 2;
			}
			iWeight *= 2*(iEspBehindWeight+iEspAttackWeight) + 3*iTeamCount/4 + 2;

			iWeight *= isHuman() ? 100 : AI_getEspionageWeight();
			iWeight /= (iLocalTeamCount + iTeamCount)/2 + 2;
			iWeight /= 100;

			if (AI_isDoStrategy(AI_STRATEGY_ESPIONAGE_ECONOMY) || (isHuman() && getCommercePercent(COMMERCE_ESPIONAGE) >= 60))
			{
				iWeight = std::max(iWeight, GC.getCommerceInfo(COMMERCE_RESEARCH).getAIWeightPercent());
			}
			else
			{
				iWeight *= 100 + 2*getCommercePercent(COMMERCE_ESPIONAGE);
				iWeight /= 110;
			}
			// K-Mod end
		}
		break;
		
	default:
		break;
	}

	return iWeight;
}

// K-Mod: I've moved the bulk of this function into AI_foundValueBulk...
short CvPlayerAI::AI_foundValue(int iX, int iY, int iMinRivalRange, bool bStartingLoc) const
{
	CvFoundSettings kSet(*this, bStartingLoc);
	kSet.iMinRivalRange = iMinRivalRange;
	return AI_foundValueBulk(iX, iY, kSet);
}

CvPlayerAI::CvFoundSettings::CvFoundSettings(const CvPlayerAI& kPlayer, bool bStartingLoc) : bStartingLoc(bStartingLoc)
{
	iMinRivalRange = -1;
	iGreed = 120;
	bEasyCulture = false;
	bAmbitious = false;
	bFinancial = false;
	bDefensive = false;
	bSeafaring = false;
	bExpansive = false;
	bAllSeeing = bStartingLoc || kPlayer.isBarbarian();

	if (!bStartingLoc)
	{
		for (int iI = 0; iI < GC.getNumTraitInfos(); iI++)
		{
			if (kPlayer.hasTrait((TraitTypes)iI))
			{
				if (GC.getTraitInfo((TraitTypes)iI).getCommerceChange(COMMERCE_CULTURE) > 0)
				{
					bEasyCulture = true;
					if (GC.getLeaderHeadInfo(kPlayer.getPersonalityType()).getBasePeaceWeight() <= 5)
						bAmbitious = true;
					iGreed += 15 * GC.getTraitInfo((TraitTypes)iI).getCommerceChange(COMMERCE_CULTURE);
				}
				if (GC.getTraitInfo((TraitTypes)iI).getExtraYieldThreshold(YIELD_COMMERCE) > 0)
				{
					bFinancial = true;
				}

				for (int iJ = 0; iJ < GC.getNumPromotionInfos(); iJ++)
				{
					if (GC.getTraitInfo((TraitTypes)iI).isFreePromotion(iJ))
					{
						// aggressive, protective... it doesn't really matter to me.
						if (GC.getLeaderHeadInfo(kPlayer.getPersonalityType()).getBasePeaceWeight() >= 5)
						{
							bDefensive = true;
						}
					}
				}

				for (int iJ = 0; iJ < GC.getNumUnitInfos(); iJ++)
				{
					if (GC.getUnitInfo((UnitTypes)iJ).isFound() &&
						GC.getUnitInfo((UnitTypes)iJ).getProductionTraits(iI) &&
						kPlayer.canTrain((UnitTypes)iJ))
					{
						iGreed += 20;
						if (GC.getLeaderHeadInfo(kPlayer.getPersonalityType()).getMaxWarRand() <= 150)
							bAmbitious = true;
					}
				}
			}
		}
		// seafaring test for unique unit and unique building
		if (kPlayer.getCoastalTradeRoutes() > 0)
			bSeafaring = true;
		if (!bSeafaring)
		{
			for (int iI = 0; iI < GC.getNumUnitClassInfos(); iI++)
			{
				UnitTypes eCivUnit = (UnitTypes)GC.getCivilizationInfo(kPlayer.getCivilizationType()).getCivilizationUnits(iI);
				UnitTypes eDefaultUnit = (UnitTypes)GC.getUnitClassInfo((UnitClassTypes)iI).getDefaultUnitIndex();
				if (eCivUnit != NO_UNIT && eCivUnit != eDefaultUnit)
				{
					if (GC.getUnitInfo(eCivUnit).getDomainType() == DOMAIN_SEA)
					{
						bSeafaring = true;
						break;
					}
				}
			}
		}
		if (!bSeafaring)
		{
			for (int iI = 0; iI < GC.getNumBuildingClassInfos(); iI++)
			{
				BuildingTypes eCivBuilding = (BuildingTypes)GC.getCivilizationInfo(kPlayer.getCivilizationType()).getCivilizationBuildings(iI);
				BuildingTypes eDefaultBuilding = (BuildingTypes)GC.getBuildingClassInfo((BuildingClassTypes)iI).getDefaultBuildingIndex();
				if (eCivBuilding != NO_UNIT && eCivBuilding != eDefaultBuilding)
				{
					if (GC.getBuildingInfo(eCivBuilding).isWater())
					{
						bSeafaring = true;
						break;
					}
				}
			}
		}
		// culture building process
		if (!bEasyCulture)
		{
			for (int iJ = 0; iJ < GC.getNumProcessInfos(); iJ++)
			{
				if (GET_TEAM(kPlayer.getTeam()).isHasTech((TechTypes)GC.getProcessInfo((ProcessTypes)iJ).getTechPrereq()) &&
					GC.getProcessInfo((ProcessTypes)iJ).getProductionToCommerceModifier(COMMERCE_CULTURE) > 0)
				{
					bEasyCulture = true;
					break;
				}
			}
		}
		// free culture building
		if (!bEasyCulture)
		{
			for (int iJ = 0; iJ < GC.getNumBuildingInfos(); iJ++)
			{
				if (kPlayer.isBuildingFree((BuildingTypes)iJ) && GC.getBuildingInfo((BuildingTypes)iJ).getObsoleteSafeCommerceChange(COMMERCE_CULTURE) > 0)
				{
					bEasyCulture = true;
					break;
				}
			}
		}
		// easy artists
		if (!bEasyCulture)
		{
			for (int iJ = 0; iJ < GC.getNumSpecialistInfos(); iJ++)
			{
				if (kPlayer.isSpecialistValid((SpecialistTypes)iJ) && kPlayer.specialistCommerce((SpecialistTypes)iJ, COMMERCE_CULTURE) > 0)
				{
					bEasyCulture = true;
					break;
				}
			}
		}
	}

	if (kPlayer.AI_getFlavorValue(FLAVOR_GROWTH) > 0)
		bExpansive = true;

	if (kPlayer.getAdvancedStartPoints() >= 0)
	{
		iGreed = 200; // overruling previous value;
	}

	if (kPlayer.isHuman())
	{
		// don't use personality based traits for human players.
		bAmbitious = false;
		bDefensive = false;
		bExpansive = false;
	}

    iClaimThreshold = GC.getGameINLINE().getCultureThreshold((CultureLevelTypes)(std::min(2, (GC.getNumCultureLevelInfos() - 1))));
    iClaimThreshold = std::max(1, iClaimThreshold);
	iClaimThreshold *= (bEasyCulture && kPlayer.getCurrentEra() < 2 ? 140 : 100);
	iClaimThreshold *= (bAmbitious ? 140 : 100);
	iClaimThreshold /= 200;
	// note, plot culture is 100x city culture. So I've left a factor of 100 on iClaimThreshold.
}

// Heavily edited for K-Mod (some changes marked, others not.)
// note, this function is called for every revealed plot for every player at the start of every turn.
// try to not make it too slow!
short CvPlayerAI::AI_foundValueBulk(int iX, int iY, const CvFoundSettings& kSet) const
{
	PROFILE_FUNC();

	CvCity* pNearestCity;
	bool bHasGoodBonus;
	int iOwnedTiles;
	int iBadTile;
	int iTakenTiles;
	int iTeammateTakenTiles;
	int iTeamAreaCities;
	int iHealth;
	int iValue;
	int iResourceValue = 0;
	int iSpecialFood = 0;
	int iSpecialFoodPlus = 0;
	int iSpecialFoodMinus = 0;
	int iSpecialProduction = 0;
	int iSpecialCommerce = 0;
	
	bool bNeutralTerritory = true;

	
	if (!canFound(iX, iY))
	{
		return 0;
	}
	
	CvPlot* pPlot = GC.getMapINLINE().plotINLINE(iX, iY);
	CvArea* pArea = pPlot->area();
	bool bIsCoastal = pPlot->isCoastalLand(GC.getMIN_WATER_SIZE_FOR_OCEAN());
	int iNumAreaCities = pArea->getCitiesPerPlayer(getID());
	
	bool bAdvancedStart = (getAdvancedStartPoints() >= 0);

	if (!kSet.bStartingLoc && !bAdvancedStart)
	{
		if (!bIsCoastal && iNumAreaCities == 0)
		{
			return 0;
		}
	}
	
	if (bAdvancedStart)
	{
		//FAssert(!bStartingLoc);
		FAssert(GC.getGameINLINE().isOption(GAMEOPTION_ADVANCED_START));
		if (kSet.bStartingLoc)
		{
			bAdvancedStart = false;
		}
	}
	
	// K-Mod. city site radius check used to be here. I've moved it down a bit.
	// ... and also the bonus vector initializtion

	if (kSet.iMinRivalRange != -1)
	{
		for (int iDX = -(kSet.iMinRivalRange); iDX <= kSet.iMinRivalRange; iDX++)
		{
			for (int iDY = -(kSet.iMinRivalRange); iDY <= kSet.iMinRivalRange; iDY++)
			{
				CvPlot* pLoopPlot = plotXY(iX, iY, iDX, iDY);

				if (pLoopPlot != NULL)
				{
					if (pLoopPlot->plotCheck(PUF_isOtherTeam, getID()) != NULL)
					{
						return 0;
					}
				}
			}
		}
	}

	if (kSet.bStartingLoc)
	{
		if (pPlot->isGoody())
		{
			return 0;
		}

		for (int iI = 0; iI < NUM_CITY_PLOTS; iI++)
		{
			CvPlot* pLoopPlot = plotCity(iX, iY, iI);

			if (pLoopPlot == NULL)
			{
				return 0;
			}
		}
	}

	iOwnedTiles = 0;

	for (int iI = 0; iI < NUM_CITY_PLOTS; iI++)
	{
		CvPlot* pLoopPlot = plotCity(iX, iY, iI);

		if (pLoopPlot == NULL)
		{
			iOwnedTiles++;
		}
		else if (pLoopPlot->isOwned())
        {
            if (pLoopPlot->getTeam() != getTeam())
            {
                iOwnedTiles++;
            }
        }
	}

	if (iOwnedTiles > (NUM_CITY_PLOTS / 3))
	{
		return 0;
	}

	// (K-Mod this site radius check code was moved from higher up)
	//Explaination of city site adjustment:
	//Any plot which is otherwise within the radius of a city site
	//is basically treated as if it's within an existing city radius
	std::vector<bool> abCitySiteRadius(NUM_CITY_PLOTS, false);

	/* original bts code
	if (!bStartingLoc)
	{
		if (!AI_isPlotCitySite(pPlot))
		{
			for (int iI = 0; iI < NUM_CITY_PLOTS; iI++)
			{
				pLoopPlot = plotCity(iX, iY, iI);
				if (pLoopPlot != NULL)
				{
					for (int iJ = 0; iJ < AI_getNumCitySites(); iJ++)
					{
						CvPlot* pCitySitePlot = AI_getCitySite(iJ);
						if (pCitySitePlot != pPlot)
						{
							if (plotDistance(pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE(), pCitySitePlot->getX_INLINE(), pCitySitePlot->getY_INLINE()) <= CITY_PLOTS_RADIUS)
							{
								//Plot is inside the radius of a city site
								abCitySiteRadius[iI] = true;
								break;
							}
						}
					}
				}
			}
		}
	}*/
	// K-Mod. bug fixes etc.
	if (!kSet.bStartingLoc)
	{
		for (int iJ = 0; iJ < AI_getNumCitySites(); iJ++)
		{
			CvPlot* pCitySitePlot = AI_getCitySite(iJ);
			if (pCitySitePlot == pPlot)
				continue;
			FAssert(pCitySitePlot != NULL);

			if (plotDistance(iX, iY, pCitySitePlot->getX_INLINE(), pCitySitePlot->getY_INLINE()) <= GC.getMIN_CITY_RANGE() &&
				pPlot->area() == pCitySitePlot->area())
			{
				// this tile is too close to one of the sites we've already chosen.
				return 0; // we can't settle here.
			}
			for (int iI = 0; iI < NUM_CITY_PLOTS; iI++)
			{
				CvPlot* pLoopPlot = plotCity(iX, iY, iI);
				if (pLoopPlot != NULL)
				{
					if (plotDistance(pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE(), pCitySitePlot->getX_INLINE(), pCitySitePlot->getY_INLINE()) <= CITY_PLOTS_RADIUS)
					{
						//Plot is inside the radius of a city site
						abCitySiteRadius[iI] = true;
					}
				}
			}
		}
	}
	// K-Mod end

	/* original bts code
	std::vector<int> paiBonusCount;

    for (int iI = 0; iI < GC.getNumBonusInfos(); iI++)
    {
        paiBonusCount.push_back(0);
    } */
	std::vector<int> paiBonusCount(GC.getNumBonusInfos(), 0); // K-Mod

	iBadTile = 0;

	for (int iI = 0; iI < NUM_CITY_PLOTS; iI++)
	{
		CvPlot* pLoopPlot = plotCity(iX, iY, iI);

		if (iI != CITY_HOME_PLOT)
		{
			if ((pLoopPlot == NULL) || pLoopPlot->isImpassable())
			{
				iBadTile += 2;
			}
			/* original bts code
			else if (!(pLoopPlot->isFreshWater()) && !(pLoopPlot->isHills()))
			{
				if ((pLoopPlot->calculateBestNatureYield(YIELD_FOOD, getTeam()) == 0) || (pLoopPlot->calculateTotalBestNatureYield(getTeam()) <= 1))
				{
					iBadTile += 2;
				}
				else if (pLoopPlot->isWater() && !bIsCoastal && (pLoopPlot->calculateBestNatureYield(YIELD_FOOD, getTeam()) <= 1))
				{
					iBadTile++;
				}
			}
            else if (pLoopPlot->isOwned())
            {
                if (pLoopPlot->getTeam() == getTeam())
                {
                    if (pLoopPlot->isCityRadius() || abCitySiteRadius[iI])
                    {
                        iBadTile += bAdvancedStart ? 2 : 1;
                    }
                }
            } */
			// K-Mod
			else if (!pLoopPlot->isFreshWater() && !pLoopPlot->isHills() &&
				(pLoopPlot->calculateBestNatureYield(YIELD_FOOD, getTeam()) == 0 || pLoopPlot->calculateTotalBestNatureYield(getTeam()) <= 1))
			{
				iBadTile += 2;
			}
			else if (pLoopPlot->isWater() && !bIsCoastal && pLoopPlot->calculateBestNatureYield(YIELD_FOOD, getTeam()) <= 1)
			{
				iBadTile++;
			}
			else if (pLoopPlot->isOwned())
			{
				if (pLoopPlot->getTeam() != getTeam() || pLoopPlot->isBeingWorked())
				{
					iBadTile++;
				}
				// note: this final condition is... not something I intend to keep permanently.
				else if (pLoopPlot->isCityRadius() || abCitySiteRadius[iI])
				{
					iBadTile++;
				}
			}
			// K-Mod end
		}
	}

	iBadTile /= 2;

	if (!kSet.bStartingLoc)
	{
		if ((iBadTile > (NUM_CITY_PLOTS / 2)) || (pArea->getNumTiles() <= 2))
		{
			bHasGoodBonus = false;

			for (int iI = 0; iI < NUM_CITY_PLOTS; iI++)
			{
				CvPlot* pLoopPlot = plotCity(iX, iY, iI);

				if (pLoopPlot != NULL && (kSet.bAllSeeing || pLoopPlot->isRevealed(getTeam(), false)))
				{
					if (!(pLoopPlot->isOwned()))
					{
						if (pLoopPlot->isWater() || (pLoopPlot->area() == pArea) || (pLoopPlot->area()->getCitiesPerPlayer(getID()) > 0))
						{
							//BonusTypes eBonus = pLoopPlot->getBonusType(getTeam());
							BonusTypes eBonus = pLoopPlot->getNonObsoleteBonusType(getTeam()); // K-Mod

							if (eBonus != NO_BONUS)
							{
								if ((getNumTradeableBonuses(eBonus) == 0) || (AI_bonusVal(eBonus) > 10)
									|| (GC.getBonusInfo(eBonus).getYieldChange(YIELD_FOOD) > 0))
								{
									bHasGoodBonus = true;
									break;
								}
							}
						}
					}
				}
			}

			if (!bHasGoodBonus)
			{
				return 0;
			}
		}
	}

	iTakenTiles = 0;
	iTeammateTakenTiles = 0;
	iHealth = 0;
	iValue = 800; // was 1000

	/* original bts code
	else if (!bStartingLoc)
    {
        for (iI = 0; iI < GC.getNumTraitInfos(); iI++)
        {
            if (hasTrait((TraitTypes)iI))
            {
                //Greedy founding means getting the best possible sites - fitting maximum
                //resources into the fat cross.
                iGreed += (GC.getTraitInfo((TraitTypes)iI).getUpkeepModifier() / 2);
                iGreed += 20 * (GC.getTraitInfo((TraitTypes)iI).getCommerceChange(COMMERCE_CULTURE));
            }
        }
    } */

    //iClaimThreshold is the culture required to pop the 2nd borders.
    /*int iClaimThreshold = GC.getGameINLINE().getCultureThreshold((CultureLevelTypes)(std::min(2, (GC.getNumCultureLevelInfos() - 1))));
    iClaimThreshold = std::max(1, iClaimThreshold);
    iClaimThreshold *= (std::max(100, iGreed));
	iClaimThreshold /= 100;*/
    
    int iYieldLostHere = 0;

	for (int iI = 0; iI < NUM_CITY_PLOTS; iI++)
	{
		CvPlot* pLoopPlot = plotCity(iX, iY, iI);

		if (pLoopPlot == NULL)
		{
			iTakenTiles++;
		}
		else if (pLoopPlot->isCityRadius() || abCitySiteRadius[iI])
		{
			iTakenTiles++;

			if (abCitySiteRadius[iI])
			{
				iTeammateTakenTiles++;
			}
		}
		// K-Mod Note: it kind of sucks that no value is counted for taken tiles. Tile sharing / stealing should be allowed.
		else if (kSet.bAllSeeing || pLoopPlot->isRevealed(getTeam(), false))
		{
			int iTempValue = 0;

			FeatureTypes eFeature = pLoopPlot->getFeatureType();
			BonusTypes eBonus = pLoopPlot->getBonusType((kSet.bStartingLoc) ? NO_TEAM : getTeam());
			ImprovementTypes eBonusImprovement = NO_IMPROVEMENT;

			// K-Mod
			bool bRemoveableFeature = false;
			bool bEventuallyRemoveableFeature = false;
			if (eFeature != NO_FEATURE)
			{
				for (int i = 0; i < GC.getNumBuildInfos(); ++i)
				{
					if (GC.getBuildInfo((BuildTypes)i).isFeatureRemove(eFeature))
					{
						bEventuallyRemoveableFeature = true;
						if (GET_TEAM(getTeam()).isHasTech((TechTypes)GC.getBuildInfo((BuildTypes)i).getTechPrereq()))
						{
							bRemoveableFeature = true;
							break;
						}
					}
				}
			}
			// K-Mod end

			int iCultureMultiplier;
            if (!pLoopPlot->isOwned() || (pLoopPlot->getOwnerINLINE() == getID()))
            {	
                iCultureMultiplier = 100;    
            }
            else
            {
            	bNeutralTerritory = false;
                int iOurCulture = pLoopPlot->getCulture(getID());
                int iOtherCulture = std::max(1, pLoopPlot->getCulture(pLoopPlot->getOwnerINLINE()));
                iCultureMultiplier = 100 * (iOurCulture + kSet.iClaimThreshold);
                iCultureMultiplier /= (iOtherCulture + kSet.iClaimThreshold);
                iCultureMultiplier = std::min(100, iCultureMultiplier);
                //The multiplier is basically normalized...
                //100% means we own (or rightfully own) the tile.
                //50% means the hostile culture is fairly firmly entrenched.
            }
            
            if (iCultureMultiplier < ((iNumAreaCities > 0) ? 25 : 50))
            {
                //discourage hopeless cases, especially on other continents.
                iTakenTiles += (iNumAreaCities > 0) ? 1 : 2;
            }

			if (eBonus != NO_BONUS)
			{
				for (int iImprovement = 0; iImprovement < GC.getNumImprovementInfos(); ++iImprovement)
				{
					CvImprovementInfo& kImprovement = GC.getImprovementInfo((ImprovementTypes)iImprovement);

					if (kImprovement.isImprovementBonusMakesValid(eBonus))
					{
						eBonusImprovement = (ImprovementTypes)iImprovement;
						break;
					}
				}
			}

			int aiYield[NUM_YIELD_TYPES];

			for (int iYieldType = 0; iYieldType < NUM_YIELD_TYPES; ++iYieldType)
			{
				YieldTypes eYield = (YieldTypes)iYieldType;
				//aiYield[eYield] = pLoopPlot->getYield(eYield);
				aiYield[eYield] = pLoopPlot->calculateNatureYield(eYield, getTeam(), bEventuallyRemoveableFeature); // K-Mod

				if (iI == CITY_HOME_PLOT)
				{
					int iBasePlotYield = aiYield[eYield];
					aiYield[eYield] += GC.getYieldInfo(eYield).getCityChange();

					if (eFeature != NO_FEATURE)
					{
						aiYield[eYield] -= GC.getFeatureInfo(eFeature).getYieldChange(eYield);
						iBasePlotYield = std::max(iBasePlotYield, aiYield[eYield]);
					}

					if (eBonus == NO_BONUS)
					{
						aiYield[eYield] = std::max(aiYield[eYield], GC.getYieldInfo(eYield).getMinCity());
					}
					else
					{
						int iBonusYieldChange = GC.getBonusInfo(eBonus).getYieldChange(eYield);
						aiYield[eYield] += iBonusYieldChange;
						iBasePlotYield += iBonusYieldChange;
						
						aiYield[eYield] = std::max(aiYield[eYield], GC.getYieldInfo(eYield).getMinCity());
					}
				
					if (eBonusImprovement != NO_IMPROVEMENT)
					{
						iBasePlotYield += GC.getImprovementInfo(eBonusImprovement).getImprovementBonusYield(eBonus, eYield);
						
						if (iBasePlotYield > aiYield[eYield])
						{
							aiYield[eYield] -= 2 * (iBasePlotYield - aiYield[eYield]);
						}
						else
						{
							aiYield[eYield] += aiYield[eYield] - iBasePlotYield;
						}
					}
					// K-Mod: emphasise plots that give us extra city yield.
					aiYield[eYield] -= pPlot->calculateNatureYield(eYield, NO_TEAM, bRemoveableFeature);
				}
				else // (not city tile), K-Mod: adjust for features
				{
					if (bEventuallyRemoveableFeature)
					{
						if (bRemoveableFeature)
						{
							iTempValue += 10 * GC.getFeatureInfo(eFeature).getYieldChange(eYield);
						}
						else
						{
							iTempValue -= 5;
							if (GC.getFeatureInfo(eFeature).getYieldChange(eYield) < 0)
							{
								iTempValue += 30 * GC.getFeatureInfo(eFeature).getYieldChange(eYield);
							}
						}
					}
				}
				// K-Mod end
			}

			/*if (iI == CITY_HOME_PLOT)
			{
				iTempValue += aiYield[YIELD_FOOD] * 60;
				iTempValue += aiYield[YIELD_PRODUCTION] * 60;
				iTempValue += aiYield[YIELD_COMMERCE] * 40;
			}
			else if (aiYield[YIELD_FOOD] >= GC.getFOOD_CONSUMPTION_PER_POPULATION())*/
			if (iI == CITY_HOME_PLOT || aiYield[YIELD_FOOD] >= GC.getFOOD_CONSUMPTION_PER_POPULATION()) // K-Mod
			{
				iTempValue += aiYield[YIELD_FOOD] * 40;
				iTempValue += aiYield[YIELD_PRODUCTION] * 40;
				iTempValue += aiYield[YIELD_COMMERCE] * 30;

				if (kSet.bStartingLoc)
				{
					iTempValue *= 2;
				}
			}
			else if (aiYield[YIELD_FOOD] == GC.getFOOD_CONSUMPTION_PER_POPULATION() - 1)
			{
				iTempValue += aiYield[YIELD_FOOD] * 25;
				iTempValue += aiYield[YIELD_PRODUCTION] * 25;
				iTempValue += aiYield[YIELD_COMMERCE] * 20;
			}
			else
			{
				iTempValue += aiYield[YIELD_FOOD] * 15;
				iTempValue += aiYield[YIELD_PRODUCTION] * 15;
				iTempValue += aiYield[YIELD_COMMERCE] * 10;
			}

			if (pLoopPlot->isWater())
			{
				if (aiYield[YIELD_COMMERCE] > 1)
				{
					/* orginal bts code
					iTempValue += bIsCoastal ? 30 : -20; */
					// K-Mod
					iTempValue += bIsCoastal ? 8*aiYield[YIELD_COMMERCE] : -8*aiYield[YIELD_COMMERCE];

					if (bIsCoastal && (aiYield[YIELD_FOOD] >= GC.getFOOD_CONSUMPTION_PER_POPULATION()))
					{
						iSpecialFoodPlus += 1;
					}
					if (kSet.bStartingLoc && !pPlot->isStartingPlot())
					{
						// I'm pretty much forbidding starting 1 tile inland non-coastal.
						// with more than a few coast tiles.
						iTempValue += bIsCoastal ? 0 : -400;
					}
				}
			}

			if (pLoopPlot->isRiver())
			{
				//iTempValue += 10;
				// K-Mod
				iTempValue += (kSet.bFinancial || kSet.bStartingLoc) ? 25 : 5;
				iTempValue += (pPlot->isRiver() ? 15 : 0);
			}
			// K-Mod
			if (pLoopPlot->canHavePotentialIrrigation())
			{
				// in addition to the river bonus
				iTempValue += 5 + (pLoopPlot->isFreshWater() ? 5 : 0);
			}

			/* original bts code
			if (iI == CITY_HOME_PLOT)
			{
				iTempValue *= 2;
			}
			else if ((pLoopPlot->getOwnerINLINE() == getID()) || (stepDistance(iX, iY, pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE()) == 1))
			{
				// BBAI Notes:  Extra weight on tiles which will be available immediately
				iTempValue *= 3;
				iTempValue /= 2;
			}
			else
			{
				iTempValue *= iGreed;
				iTempValue /= 100;
			}*/
			// K-Mod version
			if (kSet.bEasyCulture)
			{
				// 5/4 * 21 ~= 9 * 1.5 + 12 * 1;
				iTempValue *= 5;
				iTempValue /= 4;
			}
			else
			{
				if ((pLoopPlot->getOwnerINLINE() == getID()) || (stepDistance(iX, iY, pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE()) <= 1))
				{
					iTempValue *= 3;
					iTempValue /= 2;
				}
			}
			iTempValue *= kSet.iGreed;
			iTempValue /= 100;
			// K-Mod end

			
			iTempValue *= iCultureMultiplier;
			iTempValue /= 100;

			iValue += iTempValue;

			if (iCultureMultiplier > 33) //ignore hopelessly entrenched tiles.
			{
				if (eFeature != NO_FEATURE)
				{
					if (iI != CITY_HOME_PLOT)
					{
						iHealth += GC.getFeatureInfo(eFeature).getHealthPercent();

						iSpecialFoodPlus += std::max(0, aiYield[YIELD_FOOD] - GC.getFOOD_CONSUMPTION_PER_POPULATION());
					}
				}

				if ((eBonus != NO_BONUS) && ((pLoopPlot->area() == pPlot->area()) || 
					(pLoopPlot->area()->getCitiesPerPlayer(getID()) > 0)))
				{
                    paiBonusCount[eBonus]++;
                    FAssert(paiBonusCount[eBonus] > 0);

                    iTempValue = (AI_bonusVal(eBonus) * ((!kSet.bStartingLoc && (getNumTradeableBonuses(eBonus) == 0) && (paiBonusCount[eBonus] == 1)) ? 80 : 20));
                    iTempValue *= (kSet.bStartingLoc ? 100 : kSet.iGreed);
                    iTempValue /= 100;

                    if (iI != CITY_HOME_PLOT && !kSet.bStartingLoc)
                    {
                        if ((pLoopPlot->getOwnerINLINE() != getID()) && stepDistance(pPlot->getX_INLINE(),pPlot->getY_INLINE(), pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE()) > 1)
						{
                            iTempValue *= 2;
                            iTempValue /= 3;

                            /* iTempValue *= std::min(150, iGreed);
                            iTempValue /= 100; */
							// K-Mod
							iTempValue *= (kSet.bAmbitious ? 140 : 100);
							iTempValue /= 100;
						}
					}

                    iValue += (iTempValue + 10);

					if (iI != CITY_HOME_PLOT)
					{
						if (eBonusImprovement != NO_IMPROVEMENT)
						{
							int iSpecialFoodTemp;
							iSpecialFoodTemp = pLoopPlot->calculateBestNatureYield(YIELD_FOOD, getTeam()) + GC.getImprovementInfo(eBonusImprovement).getImprovementBonusYield(eBonus, YIELD_FOOD);

							iSpecialFood += iSpecialFoodTemp;

							iSpecialFoodTemp -= GC.getFOOD_CONSUMPTION_PER_POPULATION();

							iSpecialFoodPlus += std::max(0,iSpecialFoodTemp);
							iSpecialFoodMinus -= std::min(0,iSpecialFoodTemp);
							iSpecialProduction += pLoopPlot->calculateBestNatureYield(YIELD_PRODUCTION, getTeam()) + GC.getImprovementInfo(eBonusImprovement).getImprovementBonusYield(eBonus, YIELD_PRODUCTION);
							iSpecialCommerce += pLoopPlot->calculateBestNatureYield(YIELD_COMMERCE, getTeam()) + GC.getImprovementInfo(eBonusImprovement).getImprovementBonusYield(eBonus, YIELD_COMMERCE);
						}

						if (eFeature != NO_FEATURE)
						{
							//if (GC.getFeatureInfo(eFeature).getYieldChange(YIELD_FOOD) < 0)
							if (GC.getFeatureInfo(eFeature).getYieldChange(YIELD_FOOD) < 0 && !bRemoveableFeature)
							{
								iResourceValue -= 30;
							}
						}

                        if (pLoopPlot->isWater())
                        {
                            iValue += (bIsCoastal ? 100 : -800);
                        }
                    }
				}
			}
		}
	}

	iResourceValue += iSpecialFood * 50;
	iResourceValue += iSpecialProduction * 50;
	iResourceValue += iSpecialCommerce * 50;
    if (kSet.bStartingLoc)
    {
        iResourceValue /= 2;
    }

	iValue += std::max(0, iResourceValue);

	if (iTakenTiles > (NUM_CITY_PLOTS / 3) && iResourceValue < 250)
	{
		return 0;
	}

	/* original bts code (K-Mod: just go look at what "iTeammateTakenTiles" actually is...)
	if (iTeammateTakenTiles > 1)
	{
		return 0;
	}*/

	iValue += (iHealth / 5);

	if (bIsCoastal)
	{
		if (!kSet.bStartingLoc)
		{
			if (pArea->getCitiesPerPlayer(getID()) == 0)
			{
				if (bNeutralTerritory)
				{
					iValue += (iResourceValue > 0) ? 800 : 100;
				}
			}
			else
			{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      02/03/09                                jdog5000      */
/*                                                                                              */
/* Settler AI           (edited by K-Mod)                                                       */
/************************************************************************************************/
				//iValue += 200;
				int iSeaValue = 50;

				// Push players to get more coastal cities so they can build navies
				CvArea* pWaterArea = pPlot->waterArea(true);
				if( pWaterArea != NULL )
				{
					iSeaValue += 100 + (kSet.bSeafaring ? 160 : 0);

					if( GET_TEAM(getTeam()).AI_isWaterAreaRelevant(pWaterArea) )
					{
						iSeaValue += 100 + (kSet.bSeafaring ? 160 : 0);

						//if( (countNumCoastalCities() < (getNumCities()/4)) || (countNumCoastalCitiesByArea(pPlot->area()) == 0) )
						if (countNumCoastalCities() < getNumCities()/4 ||
							(pPlot->area()->getCitiesPerPlayer(getID()) > 0 && countNumCoastalCitiesByArea(pPlot->area()) == 0))
						{
							iSeaValue += 200;
						}
					}
				}
				iSeaValue *= kSet.iGreed;
				iSeaValue /= 100;
				iValue += iSeaValue;
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
			}
		}
		else
		{
		    //let other penalties bring this down.
		    iValue += 600;
		    if (!pPlot->isStartingPlot())
		    {
                if (pArea->getNumStartingPlots() == 0)
                {
                    iValue += 1000;                    
                }
			}
		}
	}

	if (pPlot->isHills())
	{
		// iValue += 200;
		// K-Mod
		iValue += 100 + (kSet.bDefensive ? 100 : 0);
	}

	if (pPlot->isFreshWater())
	{
		// iValue += 40; // K-Mod (commented this out, compensated by the river bonuses I added.)
		iValue += (GC.getDefineINT("FRESH_WATER_HEALTH_CHANGE") * 30);
	}

	if (kSet.bStartingLoc)
	{
		int iRange = GREATER_FOUND_RANGE;
		int iGreaterBadTile = 0;

		for (int iDX = -(iRange); iDX <= iRange; iDX++)
		{
			for (int iDY = -(iRange); iDY <= iRange; iDY++)
			{
				CvPlot* pLoopPlot = plotXY(iX, iY, iDX, iDY);

				if (pLoopPlot != NULL)
				{
					if (pLoopPlot->isWater() || (pLoopPlot->area() == pArea))
					{
						if (plotDistance(iX, iY, pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE()) <= iRange)
						{
						    int iTempValue = 0;
							iTempValue += (pLoopPlot->getYield(YIELD_FOOD) * 15);
							iTempValue += (pLoopPlot->getYield(YIELD_PRODUCTION) * 11);
							iTempValue += (pLoopPlot->getYield(YIELD_COMMERCE) * 5);
							iValue += iTempValue;
							if (iTempValue < 21)
							{

								iGreaterBadTile += 2;
								if (pLoopPlot->getFeatureType() != NO_FEATURE)
								{
							    	if (pLoopPlot->calculateBestNatureYield(YIELD_FOOD,getTeam()) > 1)
							    	{
										iGreaterBadTile--;
							    	}
								}
							}
						}
					}
				}
			}
		}

		if (!pPlot->isStartingPlot())
		{
			iGreaterBadTile /= 2;
			if (iGreaterBadTile > 12)
			{
				iValue *= 11;
				iValue /= iGreaterBadTile;
			}
		}
		
		int iWaterCount = 0;
		
		for (int iI = 0; iI < NUM_CITY_PLOTS; iI++)
		{
		    CvPlot* pLoopPlot = plotCity(iX, iY, iI);
            
            if (pLoopPlot != NULL)
		    {
		        if (pLoopPlot->isWater())
		        {
		            iWaterCount ++;
		            if (pLoopPlot->getYield(YIELD_FOOD) <= 1)
		            {
		                iWaterCount++;
					}
				}
			}
		}
		iWaterCount /= 2;
		
		int iLandCount = (NUM_CITY_PLOTS - iWaterCount);
		
		if (iLandCount < (NUM_CITY_PLOTS / 2))
		{
		    //discourage very water-heavy starts.
		    iValue *= 1 + iLandCount;
		    iValue /= (1 + (NUM_CITY_PLOTS / 2));
		}
	}

	if (kSet.bStartingLoc)
	{
		if (pPlot->getMinOriginalStartDist() == -1)
		{
			iValue += (GC.getMapINLINE().maxStepDistance() * 100);
		}
		else
		{
			iValue *= (1 + 4 * pPlot->getMinOriginalStartDist());
			iValue /= (1 + 2 * GC.getMapINLINE().maxStepDistance());
		}
		
		//nice hacky way to avoid this messing with normalizer, use elsewhere?
		if (!pPlot->isStartingPlot())
		{
			int iMinDistanceFactor = MAX_INT;
			int iMinRange = startingPlotRange();
			
			iValue *= 100;
			for (int iJ = 0; iJ < MAX_CIV_PLAYERS; iJ++)
			{
				if (GET_PLAYER((PlayerTypes)iJ).isAlive())
				{
					if (iJ != getID())
					{
						int iClosenessFactor = GET_PLAYER((PlayerTypes)iJ).startingPlotDistanceFactor(pPlot, getID(), iMinRange);
						iMinDistanceFactor = std::min(iClosenessFactor, iMinDistanceFactor);
						
						if (iClosenessFactor < 1000)
						{
							iValue *= 2000 + iClosenessFactor;
							iValue /= 3000;
						}
					}
				}
			}
			
			if (iMinDistanceFactor > 1000)
			{
				//give a maximum boost of 25% for somewhat distant locations, don't go overboard.
				iMinDistanceFactor = std::min(1500, iMinDistanceFactor);
				iValue *= (1000 + iMinDistanceFactor);				
				iValue /= 2000;
			}
			else if (iMinDistanceFactor < 1000)
			{
				//this is too close so penalize again.
				iValue *= iMinDistanceFactor;
				iValue /= 1000;
				iValue *= iMinDistanceFactor;
				iValue /= 1000;
			}
			
			iValue /= 10;
			
            if (pPlot->getBonusType() != NO_BONUS)
            {
                iValue /= 2;
            }
		}
	}
	
	if (bAdvancedStart)
	{
		if (pPlot->getBonusType() != NO_BONUS)
		{
			iValue *= 70;
			iValue /= 100;
		}
	}

	pNearestCity = GC.getMapINLINE().findCity(iX, iY, ((isBarbarian()) ? NO_PLAYER : getID()));

	if (pNearestCity != NULL)
	{
		if (isBarbarian())
		{
			iValue -= (std::max(0, (8 - plotDistance(iX, iY, pNearestCity->getX_INLINE(), pNearestCity->getY_INLINE()))) * 200);
		}
		else
		{
			int iDistance = plotDistance(iX, iY, pNearestCity->getX_INLINE(), pNearestCity->getY_INLINE());
			int iNumCities = getNumCities();
			/* original bts code
			if (iDistance > 5)
			{
				iValue -= (iDistance - 5) * 500;
			}
			else if (iDistance < 4)
			{
				iValue -= (4 - iDistance) * 2000;
			}
			iValue *= (8 + iNumCities * 4);
			iValue /= (2 + (iNumCities * 4) + iDistance);

			if (pNearestCity->isCapital())
			{
				iValue *= 150;
				iValue /= 100;
			}
			else if (getCapitalCity() != NULL) */
			// K-Mod.
			// Close cities are penalised in other ways
			int iTargetRange = (kSet.bExpansive ? 6 : 5);
			if (iDistance > iTargetRange)
			{
				iValue -= std::min(5, iDistance - iTargetRange) * 400; // with that max distance, we could fit a city in the middle!
			}
			iValue *= 8 + 4*iNumCities;
			iValue /= 2 + 4*iNumCities + std::max(5, iDistance); // 5, not iTargetRange, because 5 is better.

			if (!pNearestCity->isCapital() && getCapitalCity() != NULL)
			// K-Mod end
			{
				//Provide up to a 50% boost to value (80% for adv.start)
				//for city sites which are relatively close to the core
				//compared with the most distance city from the core
				//(having a boost rather than distance penalty avoids some distortion)
				
				//This is not primarly about maitenance but more about empire 
				//shape as such forbidden palace/state property are not big deal.
				CvCity* pLoopCity;
				int iLoop;
				int iMaxDistanceFromCapital = 0;
				
				int iCapitalX = getCapitalCity()->getX();
				int iCapitalY = getCapitalCity()->getY();

				for (pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
				{
					iMaxDistanceFromCapital = std::max(iMaxDistanceFromCapital, plotDistance(iCapitalX, iCapitalY, pLoopCity->getX_INLINE(), pLoopCity->getY_INLINE()));					
				}
				
				int iDistanceToCapital = plotDistance(iCapitalX, iCapitalY, iX, iY);
				
				FAssert(iMaxDistanceFromCapital > 0);
				/* original bts code
				iValue *= 100 + (((bAdvancedStart ? 80 : 50) * std::max(0, (iMaxDistanceFromCapital - iDistance))) / iMaxDistanceFromCapital);
				iValue /= 100; */
				// K-Mod. just a touch of flavour
				int iShapeWeight = bAdvancedStart ? 80 : (kSet.bAmbitious ? 30 : 50);
				iValue *= 100 + iShapeWeight * std::max(0, (iMaxDistanceFromCapital - iDistance)) / iMaxDistanceFromCapital;
				iValue /= 100 + iShapeWeight;
				// K-Mod end
			}
		}
	}
	else
	{
		pNearestCity = GC.getMapINLINE().findCity(iX, iY, ((isBarbarian()) ? NO_PLAYER : getID()), ((isBarbarian()) ? NO_TEAM : getTeam()), false);
		if (pNearestCity != NULL)
		{
			int iDistance = plotDistance(iX, iY, pNearestCity->getX_INLINE(), pNearestCity->getY_INLINE());
			iValue -= std::min(500 * iDistance, (8000 * iDistance) / GC.getMapINLINE().maxPlotDistance());
		}
	}
	
	if (iValue <= 0)
	{
		return 1;
	}

	if (pArea->getNumCities() == 0)
	{
		//iValue *= 2;
		// K-Mod: presumably this is meant to be a bonus for being the first on a new continent.
		// But I don't want it to be a bonus for settling on tiny islands, so I'm changing it.
		iValue *= range(100 * (pArea->getNumTiles() - 15) / 15, 100, 200);
		iValue /= 100;
		// K-Mod end
	}
	else
	{
		iTeamAreaCities = GET_TEAM(getTeam()).countNumCitiesByArea(pArea);

		if (pArea->getNumCities() == iTeamAreaCities)
		{
			iValue *= 3;
			iValue /= 2;
		}
		else if (pArea->getNumCities() == (iTeamAreaCities + GET_TEAM(BARBARIAN_TEAM).countNumCitiesByArea(pArea)))
		{
			iValue *= 4;
			iValue /= 3;
		}
		else if (iTeamAreaCities > 0)
		{
			iValue *= 5;
			iValue /= 4;
		}
	}

	if (!kSet.bStartingLoc)
	{
		int iFoodSurplus = std::max(0, iSpecialFoodPlus - iSpecialFoodMinus);
		int iFoodDeficit = std::max(0, iSpecialFoodMinus - iSpecialFoodPlus);

		iValue *= 100 + 20 * std::max(0, std::min(iFoodSurplus, 2 * GC.getFOOD_CONSUMPTION_PER_POPULATION()));
		iValue /= 100 + 20 * std::max(0, iFoodDeficit);
	}

	if (!kSet.bStartingLoc && getNumCities() > 0)
	{
	    int iBonusCount = 0;
	    int iUniqueBonusCount = 0;
	    for (int iI = 0; iI < GC.getNumBonusInfos(); iI++)
	    {
	        iBonusCount += paiBonusCount[iI];
	        iUniqueBonusCount += (paiBonusCount[iI] > 0) ? 1 : 0;
	    }
	    if (iBonusCount > 4)
	    {
	        iValue *= 5;
	        iValue /= (1 + iBonusCount);
	    }
	    else if (iUniqueBonusCount > 2)
	    {
	        iValue *= 5;
	        iValue /= (3 + iUniqueBonusCount);	        
	    }
	}
	
	if (!kSet.bStartingLoc)
	{
		int iDeadLockCount = AI_countDeadlockedBonuses(pPlot);
		if (bAdvancedStart && (iDeadLockCount > 0))
		{
			iDeadLockCount += 2;
		}
		iValue /= (1 + iDeadLockCount);
	}

	iValue /= (std::max(0, (iBadTile - (NUM_CITY_PLOTS / 4))) + 3);

	if (kSet.bStartingLoc)
	{
		int iDifferentAreaTile = 0;

		for (int iI = 0; iI < NUM_CITY_PLOTS; iI++)
		{
			CvPlot* pLoopPlot = plotCity(iX, iY, iI);

			if ((pLoopPlot == NULL) || !(pLoopPlot->isWater() || pLoopPlot->area() == pArea))
			{
				iDifferentAreaTile++;
			}
		}

		iValue /= (std::max(0, (iDifferentAreaTile - ((NUM_CITY_PLOTS * 2) / 3))) + 2);
	}

	return std::max(1, iValue);
}


bool CvPlayerAI::AI_isAreaAlone(CvArea* pArea) const
{
	return ((pArea->getNumCities() - GET_TEAM(BARBARIAN_TEAM).countNumCitiesByArea(pArea)) == GET_TEAM(getTeam()).countNumCitiesByArea(pArea));
}


bool CvPlayerAI::AI_isCapitalAreaAlone() const
{
	CvCity* pCapitalCity;

	pCapitalCity = getCapitalCity();

	if (pCapitalCity != NULL)
	{
		return AI_isAreaAlone(pCapitalCity->area());
	}

	return false;
}


bool CvPlayerAI::AI_isPrimaryArea(CvArea* pArea) const
{
	CvCity* pCapitalCity;

	if (pArea->isWater())
	{
		return false;
	}

	if (pArea->getCitiesPerPlayer(getID()) > 2)
	{
		return true;
	}

	pCapitalCity = getCapitalCity();

	if (pCapitalCity != NULL)
	{
		if (pCapitalCity->area() == pArea)
		{
			return true;
		}
	}

	return false;
}


int CvPlayerAI::AI_militaryWeight(CvArea* pArea) const
{
	return (pArea->getPopulationPerPlayer(getID()) + pArea->getCitiesPerPlayer(getID()) + 1);
}


int CvPlayerAI::AI_targetCityValue(CvCity* pCity, bool bRandomize, bool bIgnoreAttackers) const
{
	PROFILE_FUNC();

	CvCity* pNearestCity;
	CvPlot* pLoopPlot;
	int iValue;
	int iI;

	FAssertMsg(pCity != NULL, "City is not assigned a valid value");

	iValue = 1;

	iValue += ((pCity->getPopulation() * (50 + pCity->calculateCulturePercent(getID()))) / 100);

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      06/30/10                     Mongoose & jdog5000      */
/*                                                                                              */
/* War strategy AI                                                                              */
/************************************************************************************************/
	if (pCity->getDefenseDamage() > 0)
	{
		iValue += ((pCity->getDefenseDamage() / 30) + 1);
	}

	// Significant amounting of borrowing/adapting from Mongoose AITargetCityValueFix
	if (pCity->isCoastal(GC.getMIN_WATER_SIZE_FOR_OCEAN()))
	{
		iValue += 2;
	}

	iValue += 4*pCity->getNumActiveWorldWonders();

	for (iI = 0; iI < GC.getNumReligionInfos(); iI++)
	{
		if (pCity->isHolyCity((ReligionTypes)iI))
		{
			iValue += 2 + ((GC.getGameINLINE().calculateReligionPercent((ReligionTypes)iI)) / 5);

			if (GET_PLAYER(pCity->getOwnerINLINE()).getStateReligion() == iI)
			{
				iValue += 2;
			}
			if( getStateReligion() == iI )
			{
				iValue += 8;
			}
		}
	}

	if (pCity->isEverOwned(getID()))
	{
		iValue += 4;

		if( pCity->getOriginalOwner() == getID() )
		{
			iValue += 2;
		}
	}

	if (!bIgnoreAttackers)
	{
		iValue += std::min( 8, (AI_adjacentPotentialAttackers(pCity->plot()) + 2)/3 );
	}

	for (iI = 0; iI < NUM_CITY_PLOTS; iI++)
	{
		pLoopPlot = plotCity(pCity->getX_INLINE(), pCity->getY_INLINE(), iI);

		if (pLoopPlot != NULL)
		{
			if (pLoopPlot->getBonusType(getTeam()) != NO_BONUS)
			{
				iValue += std::max(1, AI_bonusVal(pLoopPlot->getBonusType(getTeam()))/5);
			}

			if (pLoopPlot->getOwnerINLINE() == getID())
			{
				iValue++;
			}

			if (pLoopPlot->isAdjacentPlayer(getID(), true))
			{
				iValue++;
			}
		}
	}

	if( GET_PLAYER(pCity->getOwnerINLINE()).AI_isDoVictoryStrategy(AI_VICTORY_CULTURE3) )
	{
		if( pCity->getCultureLevel() >= (GC.getGameINLINE().culturalVictoryCultureLevel() - 1) )
		{
			iValue += 15;
			
			if( GET_PLAYER(pCity->getOwnerINLINE()).AI_isDoVictoryStrategy(AI_VICTORY_CULTURE4) )
			{
				iValue += 25;

				if( pCity->getCultureLevel() >= (GC.getGameINLINE().culturalVictoryCultureLevel()) )
				{
					iValue += 10;
				}
			}
		}
	}

	if( GET_PLAYER(pCity->getOwnerINLINE()).AI_isDoVictoryStrategy(AI_VICTORY_SPACE3) )
	{
		if( pCity->isCapital() )
		{
			iValue += 10;

			if( GET_PLAYER(pCity->getOwnerINLINE()).AI_isDoVictoryStrategy(AI_VICTORY_SPACE4) )
			{
				iValue += 20;

				if( GET_TEAM(pCity->getTeam()).getVictoryCountdown(GC.getGameINLINE().getSpaceVictory()) >= 0 )
				{
					iValue += 30;
				}
			}
		}
	}

	pNearestCity = GC.getMapINLINE().findCity(pCity->getX_INLINE(), pCity->getY_INLINE(), getID());

	if (pNearestCity != NULL)
	{
		// Now scales sensibly with map size, on large maps this term was incredibly dominant in magnitude
		int iTempValue = 30;
		iTempValue *= std::max(1, ((GC.getMapINLINE().maxStepDistance() * 2) - GC.getMapINLINE().calculatePathDistance(pNearestCity->plot(), pCity->plot())));
		iTempValue /= std::max(1, (GC.getMapINLINE().maxStepDistance() * 2));

		iValue += iTempValue;
	}

	if (bRandomize)
	{
		iValue += GC.getGameINLINE().getSorenRandNum(((pCity->getPopulation() / 2) + 1), "AI Target City Value");
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	return iValue;
}


CvCity* CvPlayerAI::AI_findTargetCity(CvArea* pArea) const
{
	CvCity* pLoopCity;
	CvCity* pBestCity;
	int iValue;
	int iBestValue;
	int iLoop;
	int iI;

	iBestValue = 0;
	pBestCity = NULL;

	for (iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (isPotentialEnemy(getTeam(), GET_PLAYER((PlayerTypes)iI).getTeam()))
			{
				for (pLoopCity = GET_PLAYER((PlayerTypes)iI).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER((PlayerTypes)iI).nextCity(&iLoop))
				{
					if (pLoopCity->area() == pArea)
					{
						iValue = AI_targetCityValue(pLoopCity, true);

						if (iValue > iBestValue)
						{
							iBestValue = iValue;
							pBestCity = pLoopCity;
						}
					}
				}
			}
		}
	}

	return pBestCity;
}


bool CvPlayerAI::AI_isCommercePlot(CvPlot* pPlot) const
{
	return (pPlot->getYield(YIELD_FOOD) >= GC.getFOOD_CONSUMPTION_PER_POPULATION());
}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      08/20/09                                jdog5000      */
/*                                                                                              */
/* General AI, Efficiency                                                                       */
/************************************************************************************************/
// Plot danger cache

// The vast majority of checks for plot danger are boolean checks during path planning for non-combat
// units like workers, settlers, and GP.  Since these are simple checks for any danger they can be 
// cutoff early if danger is found.  To this end, the two caches tracked are for whether a given plot
// is either known to be safe for the player who is currently moving, or for whether the plot is
// known to be a plot bordering an enemy of this team and therefore unsafe.
//
// The safe plot cache is only for the active moving player and is only set if this is not a
// multiplayer game with simultaneous turns.  The safety cache for all plots is reset when the active
// player changes or a new game is loaded.
// 
// The border cache is done by team and works for all game types.  The border cache is reset for all
// plots when war or peace are declared, and reset over a limited range whenever a ownership over a plot
// changes.
bool CvPlayerAI::AI_getAnyPlotDanger(CvPlot* pPlot, int iRange, bool bTestMoves) const
{
	PROFILE_FUNC();

	if (iRange == -1)
	{
		iRange = DANGER_RANGE;
	}

	if( bTestMoves && isTurnActive() )
	{
		if( (iRange <= DANGER_RANGE) && pPlot->isActivePlayerNoDangerCache() )
		{
			return false;
		}
	}

	TeamTypes eTeam = getTeam();
	//bool bCheckBorder = (!isHuman() && !pPlot->isCity());
	// K-Mod. I don't want auto-workers on the frontline. Cities need to be excluded for some legacy AI code.
	bool bCheckBorder = !pPlot->isCity() && (!isHuman() || pPlot->plotCount(PUF_canDefend, -1, -1, getID(), NO_TEAM) == 0);
	// K-Mod end

	
	if( bCheckBorder )
	{
		if( (iRange >= DANGER_RANGE) && pPlot->isTeamBorderCache(eTeam) )
		{
			return true;
		}
	}

	CLLNode<IDInfo>* pUnitNode;
	CvUnit* pLoopUnit;
	CvPlot* pLoopPlot;
	int iDistance;
	int iDX, iDY;
	CvArea *pPlotArea = pPlot->area();
	int iDangerRange;

	for (iDX = -(iRange); iDX <= iRange; iDX++)
	{
		for (iDY = -(iRange); iDY <= iRange; iDY++)
		{
			pLoopPlot	= plotXY(pPlot->getX_INLINE(), pPlot->getY_INLINE(), iDX, iDY);

			if (pLoopPlot != NULL)
			{
				if (pLoopPlot->area() == pPlotArea)
				{
				    iDistance = stepDistance(pPlot->getX_INLINE(), pPlot->getY_INLINE(), pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE());
				    if( bCheckBorder )
					{
						if (atWar(pLoopPlot->getTeam(), eTeam))
						{
							// Border cache is reversible, set for both team and enemy
							if (iDistance == 1)
							{
								pPlot->setIsTeamBorderCache(eTeam, true);
								pPlot->setIsTeamBorderCache(pLoopPlot->getTeam(), true);
								pLoopPlot->setIsTeamBorderCache(eTeam, true);
								pLoopPlot->setIsTeamBorderCache(pLoopPlot->getTeam(), true);
								return true;
							}
							else if ((iDistance == 2) && (pLoopPlot->isRoute()))
							{
								pPlot->setIsTeamBorderCache(eTeam, true);
								pPlot->setIsTeamBorderCache(pLoopPlot->getTeam(), true);
								pLoopPlot->setIsTeamBorderCache(eTeam, true);
								pLoopPlot->setIsTeamBorderCache(pLoopPlot->getTeam(), true);
								return true;
							}
						}
					}

					pUnitNode = pLoopPlot->headUnitNode();

					while (pUnitNode != NULL)
					{
						pLoopUnit = ::getUnit(pUnitNode->m_data);
						pUnitNode = pLoopPlot->nextUnitNode(pUnitNode);

						// No need to loop over tiles full of our own units
						if( pLoopUnit->getTeam() == eTeam )
						{
							if( !(pLoopUnit->alwaysInvisible()) && (pLoopUnit->getInvisibleType() == NO_INVISIBLE) )
							{
								break;
							}
						}

						if (pLoopUnit->isEnemy(eTeam))
						{
							if (pLoopUnit->canAttack())
							{
								if (!(pLoopUnit->isInvisible(eTeam, false)))
								{
								    if (pLoopUnit->canMoveOrAttackInto(pPlot))
								    {
                                        if (!bTestMoves)
                                        {
                                            return true;
                                        }
                                        else
                                        {
                                            iDangerRange = pLoopUnit->baseMoves();
                                            iDangerRange += ((pLoopPlot->isValidRoute(pLoopUnit)) ? 1 : 0);
                                            if (iDangerRange >= iDistance)
											{
												return true;
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


	// The test moves case is a strict subset of the more general case,
	// either is appropriate for setting the cache.  However, since the test moves
	// case is called far more frequently, it is more important and the cache 
	// value being true is only assumed to mean that the plot is safe in the
	// test moves case.
	//if( bTestMoves )
	{
		if( isTurnActive() )
		{
			if( !(GC.getGameINLINE().isMPOption(MPOPTION_SIMULTANEOUS_TURNS)) && (GC.getGameINLINE().getNumGameTurnActive() == 1) )
			{
				pPlot->setIsActivePlayerNoDangerCache(true);
			}
		}
	}

	return false;
}


int CvPlayerAI::AI_getPlotDanger(CvPlot* pPlot, int iRange, bool bTestMoves) const
{
	PROFILE_FUNC();

	CLLNode<IDInfo>* pUnitNode;
	CvUnit* pLoopUnit;
	CvPlot* pLoopPlot;
	int iCount;
	int iDistance;
	int iBorderDanger;
	int iDX, iDY;
	CvArea *pPlotArea = pPlot->area();
	int iDangerRange;
	TeamTypes eTeam = getTeam();

	iCount = 0;
	iBorderDanger = 0;

	if (iRange == -1)
	{
		iRange = DANGER_RANGE;
	}

	if( bTestMoves && isTurnActive() )
	{
		if( (iRange <= DANGER_RANGE) && pPlot->isActivePlayerNoDangerCache() )
		{
			return 0;
		}
	}

	for (iDX = -(iRange); iDX <= iRange; iDX++)
	{
		for (iDY = -(iRange); iDY <= iRange; iDY++)
		{
			pLoopPlot	= plotXY(pPlot->getX_INLINE(), pPlot->getY_INLINE(), iDX, iDY);

			if (pLoopPlot != NULL)
			{
				if (pLoopPlot->area() == pPlotArea)
				{
				    iDistance = stepDistance(pPlot->getX_INLINE(), pPlot->getY_INLINE(), pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE());
				    if (atWar(pLoopPlot->getTeam(), eTeam))
				    {
				        if (iDistance == 1)
				        {
				            iBorderDanger++;
				        }
				        else if ((iDistance == 2) && (pLoopPlot->isRoute()))
				        {
				            iBorderDanger++;
				        }
				    }


					pUnitNode = pLoopPlot->headUnitNode();

					while (pUnitNode != NULL)
					{
						pLoopUnit = ::getUnit(pUnitNode->m_data);
						pUnitNode = pLoopPlot->nextUnitNode(pUnitNode);

						// No need to loop over tiles full of our own units
						if( pLoopUnit->getTeam() == eTeam )
						{
							if( !(pLoopUnit->alwaysInvisible()) && (pLoopUnit->getInvisibleType() == NO_INVISIBLE) )
							{
								break;
							}
						}

						if (pLoopUnit->isEnemy(eTeam))
						{
							if (pLoopUnit->canAttack())
							{
								if (!(pLoopUnit->isInvisible(eTeam, false)))
								{
								    if (pLoopUnit->canMoveOrAttackInto(pPlot))
								    {
                                        if (!bTestMoves)
                                        {
                                            iCount++;
                                        }
                                        else
                                        {
                                            iDangerRange = pLoopUnit->baseMoves();
                                            iDangerRange += ((pLoopPlot->isValidRoute(pLoopUnit)) ? 1 : 0);
                                            if (iDangerRange >= iDistance)
											{
												iCount++;
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

	if (iBorderDanger > 0)
	{
	    /* original bts code
		if (!isHuman() && !pPlot->isCity())
	    {
            iCount += iBorderDanger;
	    } */
		// K-Mod. I don't want auto-workers on the frontline. So unless the plot is defended, or a water, count border danger for humans too.
		// but on the other hand, I don't think two border tiles are really more dangerous than one border tile.
		if (!isHuman() || (!pPlot->isWater() && pPlot->plotCount(PUF_canDefend, -1, -1, getID(), NO_TEAM) == 0))
			iCount++;
		// K-Mod end
	}

	return iCount;
}

// Never used ...
/*
int CvPlayerAI::AI_getUnitDanger(CvUnit* pUnit, int iRange, bool bTestMoves, bool bAnyDanger) const
{
	PROFILE_FUNC();

	CLLNode<IDInfo>* pUnitNode;
	CvUnit* pLoopUnit;
	CvPlot* pLoopPlot;
	int iCount;
	int iDistance;
	int iBorderDanger;
	int iDX, iDY;

    CvPlot* pPlot = pUnit->plot();
	iCount = 0;
	iBorderDanger = 0;

	if (iRange == -1)
	{
		iRange = DANGER_RANGE;
	}
	
	for (iDX = -(iRange); iDX <= iRange; iDX++)
	{
		for (iDY = -(iRange); iDY <= iRange; iDY++)
		{
			pLoopPlot = plotXY(pPlot->getX_INLINE(), pPlot->getY_INLINE(), iDX, iDY);

			if (pLoopPlot != NULL)
			{
				if (pLoopPlot->area() == pPlot->area())
				{
				    iDistance = stepDistance(pPlot->getX_INLINE(), pPlot->getY_INLINE(), pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE());
				    if (atWar(pLoopPlot->getTeam(), getTeam()))
				    {
				        if (iDistance == 1)
				        {
				            iBorderDanger++;
				        }
				        else if ((iDistance == 2) && (pLoopPlot->isRoute()))
				        {
				            iBorderDanger++;
				        }
				    }


					pUnitNode = pLoopPlot->headUnitNode();

					while (pUnitNode != NULL)
					{
						pLoopUnit = ::getUnit(pUnitNode->m_data);
						pUnitNode = pLoopPlot->nextUnitNode(pUnitNode);

						if (atWar(pLoopUnit->getTeam(), getTeam()))
						{
							if (pLoopUnit->canAttack())
							{
								if (!(pLoopUnit->isInvisible(getTeam(), false)))
								{
								    if (pLoopUnit->canMoveOrAttackInto(pPlot))
								    {
                                        if (!bTestMoves)
                                        {
                                            iCount++;
                                        }
                                        else
                                        {
                                            int iDangerRange = pLoopUnit->baseMoves();
                                            iDangerRange += ((pLoopPlot->isValidRoute(pLoopUnit)) ? 1 : 0);
                                            if (iDangerRange >= iDistance)
                                            {
                                                iCount++;
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

	if (iBorderDanger > 0)
	{
	    if (!isHuman() || pUnit->isAutomated())
	    {
            iCount += iBorderDanger;
	    }
	}

	return iCount;
}
*/
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/


int CvPlayerAI::AI_getWaterDanger(CvPlot* pPlot, int iRange, bool bTestMoves) const
{
	PROFILE_FUNC();

	CLLNode<IDInfo>* pUnitNode;
	CvUnit* pLoopUnit;
	CvPlot* pLoopPlot;
	int iCount;
	int iDX, iDY;

	iCount = 0;

	if (iRange == -1)
	{
		iRange = DANGER_RANGE;
	}
	
	CvArea* pWaterArea = pPlot->waterArea();

	for (iDX = -(iRange); iDX <= iRange; iDX++)
	{
		for (iDY = -(iRange); iDY <= iRange; iDY++)
		{
			pLoopPlot = plotXY(pPlot->getX_INLINE(), pPlot->getY_INLINE(), iDX, iDY);

			if (pLoopPlot != NULL)
			{
				if (pLoopPlot->isWater())
				{
					if (pPlot->isAdjacentToArea(pLoopPlot->getArea()))
					{
						pUnitNode = pLoopPlot->headUnitNode();

						while (pUnitNode != NULL)
						{
							pLoopUnit = ::getUnit(pUnitNode->m_data);
							pUnitNode = pLoopPlot->nextUnitNode(pUnitNode);

							if (pLoopUnit->isEnemy(getTeam()))
							{
								if (pLoopUnit->canAttack())
								{
									if (!(pLoopUnit->isInvisible(getTeam(), false)))
									{
										iCount++;
									}
								}
							}
						}
					}
				}
			}
		}
	}
	
	return iCount;
}


bool CvPlayerAI::AI_avoidScience() const
{
	/* original bts / BBAI code
	if (AI_isDoVictoryStrategy(AI_VICTORY_CULTURE4))
    {
        return true;
    } */ // K-Mod. going for culture does not mean avoiding science.
	if (isCurrentResearchRepeat())
	{
		return true;
	}

	if (isNoResearchAvailable())
	{
		return true;
	}

	return false;
}


// XXX
bool CvPlayerAI::AI_isFinancialTrouble() const
{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      06/12/09                                jdog5000      */
/*                                                                                              */
/* Barbarian AI                                                                                 */
/************************************************************************************************/
	if( isBarbarian() )
	{
		return false;
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	//if (getCommercePercent(COMMERCE_GOLD) > 50)
	{
		/* original bts code
		int iNetCommerce = 1 + getCommerceRate(COMMERCE_GOLD) + getCommerceRate(COMMERCE_RESEARCH) + std::max(0, getGoldPerTurn()); */
		// K-Mod: have you heard of "cultural victory"? How about "espionage economy"?
		// Lets try it a more general way. (hopefully this doesn't slow it down too much...)
		int iTotalRaw = calculateTotalYield(YIELD_COMMERCE);

		int iNetCommerce = iTotalRaw * AI_averageCommerceMultiplier(COMMERCE_GOLD) / 100;
		iNetCommerce += std::max(0, getGoldPerTurn());
		iNetCommerce += getCommerceRate(COMMERCE_GOLD) - iTotalRaw * AI_averageCommerceMultiplier(COMMERCE_GOLD) * getCommercePercent(COMMERCE_GOLD) / 10000;
		// K-Mod end
/************************************************************************************************/
/* UNOFFICIAL_PATCH                       06/11/09                       jdog5000 & DanF5771    */
/*                                                                                              */
/* Bugfix                                                                                       */
/************************************************************************************************/
/* original BTS code
		int iNetExpenses = calculateInflatedCosts() + std::min(0, getGoldPerTurn());
*/
		int iNetExpenses = calculateInflatedCosts() + std::max(0, -getGoldPerTurn());
/************************************************************************************************/
/* UNOFFICIAL_PATCH                        END                                                  */
/************************************************************************************************/		
		
		int iFundedPercent = (100 * (iNetCommerce - iNetExpenses)) / std::max(1, iNetCommerce);
		
		int iSafePercent = 35; // was 40
		if (AI_avoidScience())
		{
			iSafePercent -= 8;
		}
		
		if (GET_TEAM(getTeam()).getAnyWarPlanCount(true))
		{
			iSafePercent -= 10; // was 12
		}
		
		if (isCurrentResearchRepeat())
		{
			iSafePercent -= 10;
		}
		
		if (iFundedPercent < iSafePercent)
		{
			return true;
		}
	}

	return false;
}


int CvPlayerAI::AI_goldTarget(bool bUpgradeBudgetOnly) const
{
	int iGold = 0;

	bool bAnyWar = GET_TEAM(getTeam()).getAnyWarPlanCount(true) > 0;

	// K-Mod.
	int iUpgradeBudget = AI_getGoldToUpgradeAllUnits();

	if (!bAnyWar)
	{
		iUpgradeBudget /= AI_isFinancialTrouble() ? 10 : 4;
	}
	else
	{
		if (GET_TEAM(getTeam()).AI_getWarSuccessRating() < -10)
			iUpgradeBudget *= 2; // cf. iTargetTurns in AI_doCommerce
		else if (AI_isFinancialTrouble())
		{
			iUpgradeBudget /= 2;
		}
	}

	if (bUpgradeBudgetOnly)
		return iUpgradeBudget;

	iGold += iUpgradeBudget;
	// K-Mod end

/************************************************************************************************/
/* UNOFFICIAL_PATCH                       02/24/10                                jdog5000      */
/*                                                                                              */
/* Bugfix                                                                                      */
/************************************************************************************************/
/* original bts code
	if (GC.getGameINLINE().getElapsedGameTurns() >= 40)
*/
	if (GC.getGameINLINE().getElapsedGameTurns() >= 40 || getNumCities() > 3)
/************************************************************************************************/
/* UNOFFICIAL_PATCH                        END                                                  */
/************************************************************************************************/
	{
		/* original bts code
		int iMultiplier = 0;
		iMultiplier += GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).getResearchPercent();
		iMultiplier += GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).getTrainPercent();
		iMultiplier += GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).getConstructPercent();
		iMultiplier /= 3;

		iGold += ((getNumCities() * 3) + (getTotalPopulation() / 3));

		iGold += (GC.getGameINLINE().getElapsedGameTurns() / 2);*/
		// K-mod. Does slower research mean we need to keep more gold? Does slower building?
		// Surely the raw turn count is the one that needs to be adjusted for speed!
		int iStockPile = 3*std::min(8, getNumCities()) + std::min(120, getTotalPopulation())/3;
		iGold += 100*GC.getGameINLINE().getElapsedGameTurns() / (2*GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).getResearchPercent());
		if (AI_getFlavorValue(FLAVOR_GOLD) > 0)
		{
			iStockPile *= 10 + AI_getFlavorValue(FLAVOR_GOLD);
			iStockPile /= 8;
		}
		// note: currently the highest flavor_gold is 5.
		iGold += iStockPile;
		// K-Mod end

		/*iGold *= iMultiplier;
		iGold /= 100;*/

		/* original bts code
		if (bAnyWar)
		{
			iGold *= 3;
			iGold /= 2;
		} */ // K-Mod. I don't think we need this anymore.

		if (AI_avoidScience())
		{
			iGold *= 3; // was 10
		}

		//iGold += (AI_getGoldToUpgradeAllUnits() / (bAnyWar ? 1 : 2)); // obsolete (K-Mod)

		/* original bts code
		CorporationTypes eActiveCorporation = NO_CORPORATION;
		for (int iI = 0; iI < GC.getNumCorporationInfos(); iI++)
		{
			if (getHasCorporationCount((CorporationTypes)iI) > 0)
			{
				eActiveCorporation = (CorporationTypes)iI;
				break;
			}
		}
		if (eActiveCorporation != NO_CORPORATION)
		{
			int iSpreadCost = std::max(0, GC.getCorporationInfo(eActiveCorporation).getSpreadCost() * (100 + calculateInflationRate()));
			iSpreadCost /= 50;
			iGold += iSpreadCost;
		} */
		// K-Mod
		int iSpreadCost = 0;

		if (!isNoCorporations())
		{
			for (int iI = 0; iI < GC.getNumCorporationInfos(); iI++)
			{
				if (getHasCorporationCount((CorporationTypes)iI) > 0)
				{
					int iExecs = countCorporationSpreadUnits(NULL, (CorporationTypes)iI, true);
					if (iExecs > 0)
					{
						int iTempCost = GC.getCorporationInfo((CorporationTypes)iI).getSpreadCost();
						iTempCost *= iExecs + 1; // execs + 1 because actual spread cost can be higher than the base cost.
						iSpreadCost = std::max(iTempCost, GC.getCorporationInfo((CorporationTypes)iI).getSpreadCost());
					}
				}
			}
		}
		if (iSpreadCost > 0)
		{
			iSpreadCost *= 100 + calculateInflationRate();
			iSpreadCost /= 100;
			iGold += iSpreadCost;
		}
		// K-Mod end
	}

	return iGold + AI_getExtraGoldTarget();
}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      05/14/10                                jdog5000      */
/*                                                                                              */
/* Tech AI                                                                                      */
/************************************************************************************************/
TechTypes CvPlayerAI::AI_bestTech(int iMaxPathLength, bool bIgnoreCost, bool bAsync, TechTypes eIgnoreTech, AdvisorTypes eIgnoreAdvisor) const
{
	PROFILE("CvPlayerAI::AI_bestTech");

	int iValue;
	int iBestValue = 0;
	TechTypes eBestTech = NO_TECH;
	int iPathLength;
	CvTeam& kTeam = GET_TEAM(getTeam());
	
	int* paiBonusClassRevealed;
	int* paiBonusClassUnrevealed;
	int* paiBonusClassHave;
	
	paiBonusClassRevealed = new int[GC.getNumBonusClassInfos()];
	paiBonusClassUnrevealed = new int[GC.getNumBonusClassInfos()];
	paiBonusClassHave = new int[GC.getNumBonusClassInfos()];
	
	for (int iI = 0; iI < GC.getNumBonusClassInfos(); iI++)
	{
        paiBonusClassRevealed[iI] = 0;
        paiBonusClassUnrevealed[iI] = 0;
        paiBonusClassHave[iI] = 0;	    
	}
	
	for (int iI = 0; iI < GC.getNumBonusInfos(); iI++)
	{
	    TechTypes eRevealTech = (TechTypes)GC.getBonusInfo((BonusTypes)iI).getTechReveal();
	    BonusClassTypes eBonusClass = (BonusClassTypes)GC.getBonusInfo((BonusTypes)iI).getBonusClassType();
	    if (eRevealTech != NO_TECH)
	    {
	        if ((kTeam.isHasTech(eRevealTech)))
	        {
	            paiBonusClassRevealed[eBonusClass]++;
	        }
	        else
	        {
	            paiBonusClassUnrevealed[eBonusClass]++;
	        }

            if (getNumAvailableBonuses((BonusTypes)iI) > 0)
            {
                paiBonusClassHave[eBonusClass]++;                
            }
            else if (countOwnedBonuses((BonusTypes)iI) > 0)
            {
                paiBonusClassHave[eBonusClass]++;
            }
	    }
	}

#ifdef DEBUG_TECH_CHOICES
	CvWString szPlayerName = getName();
	DEBUGLOG("AI_bestTech:%S\n", szPlayerName.GetCString());
#endif

	for (int iI = 0; iI < GC.getNumTechInfos(); iI++)
	{
		if ((eIgnoreTech == NO_TECH) || (iI != eIgnoreTech))
		{
			if ((eIgnoreAdvisor == NO_ADVISOR) || (GC.getTechInfo((TechTypes)iI).getAdvisorType() != eIgnoreAdvisor))
			{
				if (canEverResearch((TechTypes)iI))
				{
					if (!(kTeam.isHasTech((TechTypes)iI)))
					{
						if (GC.getTechInfo((TechTypes)iI).getEra() <= (getCurrentEra() + 1))
						{
							iPathLength = findPathLength(((TechTypes)iI), false);

							if (iPathLength <= iMaxPathLength)
							{
								iValue = AI_techValue( (TechTypes)iI, iPathLength, bIgnoreCost, bAsync, paiBonusClassRevealed, paiBonusClassUnrevealed, paiBonusClassHave );

								if( gPlayerLogLevel >= 3 )
								{
									logBBAI("      Player %d (%S) consider tech %S with value %d", getID(), getCivilizationDescription(0), GC.getTechInfo((TechTypes)iI).getDescription(), iValue );
								}

								if (iValue > iBestValue)
								{
									iBestValue = iValue;
									eBestTech = ((TechTypes)iI);
								}
							}
						}
					}
				}
			}
		}
	}

	if( gPlayerLogLevel >= 1 && eBestTech != NO_TECH )
	{
		logBBAI("  Player %d (%S) selects tech %S with value %d", getID(), getCivilizationDescription(0), GC.getTechInfo(eBestTech).getDescription(), iBestValue );
	}

    SAFE_DELETE_ARRAY(paiBonusClassRevealed);
    SAFE_DELETE_ARRAY(paiBonusClassUnrevealed);
    SAFE_DELETE_ARRAY(paiBonusClassHave);	

	return eBestTech;
}

int CvPlayerAI::AI_techValue( TechTypes eTech, int iPathLength, bool bIgnoreCost, bool bAsync, int* paiBonusClassRevealed, int* paiBonusClassUnrevealed, int* paiBonusClassHave ) const
{
	PROFILE_FUNC();

	CvCity* pCapitalCity;
	//ImprovementTypes eImprovement;
	//RouteTypes eRoute;
	
	//int iNumBonuses;
	//int iValue;
	long iValue; // K-Mod. (the int was overflowing in parts of the calculation)
	//int iTempValue;
	//int iBuildValue;
	//int iBonusValue;
	//int iI, iJ, iK, iL;

	pCapitalCity = getCapitalCity();

	CvTeam& kTeam = GET_TEAM(getTeam());

	//bool bWarPlan = (kTeam.getAnyWarPlanCount(true) > 0);
	bool bCapitalAlone = (GC.getGameINLINE().getElapsedGameTurns() > 0) ? AI_isCapitalAreaAlone() : false;
	bool bFinancialTrouble = AI_isFinancialTrouble();
	bool bAdvancedStart = getAdvancedStartPoints() >= 0;

	int iHasMetCount = kTeam.getHasMetCivCount(true);
	int iCoastalCities = countNumCoastalCities();
	int iConnectedForeignCities = countPotentialForeignTradeCitiesConnected();

	int iCityCount = getNumCities();
	//int iTeamCityCount = kTeam.getNumCities();
	
	iValue = 1;

	int iRandomFactor = ((bAsync) ? GC.getASyncRand().get(2000, "AI Research ASYNC") : GC.getGameINLINE().getSorenRandNum(2000, "AI Research"));
	int iRandomMax = 2000;
	iValue += iRandomFactor;

	iValue += kTeam.getResearchProgress(eTech);

	// Map stuff
	if (GC.getTechInfo(eTech).isExtraWaterSeeFrom())
	{
		if (iCoastalCities > 0)
		{
			iValue += 100;

			if (bCapitalAlone)
			{
				iValue += 400;
			}
		}
	}

	if (GC.getTechInfo(eTech).isMapCentering())
	{
		// K-Mod
		//iValue += 100;
	}

	if (GC.getTechInfo(eTech).isMapVisible())
	{
		iValue += 100;

		if (bCapitalAlone)
		{
			iValue += 400;
		}
	}

	// Expand trading options
	if (GC.getTechInfo(eTech).isMapTrading())
	{
		iValue += 100;

		if (bCapitalAlone)
		{
			iValue += 400;
		}
	}

	if (GC.getTechInfo(eTech).isTechTrading() && !GC.getGameINLINE().isOption(GAMEOPTION_NO_TECH_TRADING))
	{
		iValue += 500;

		iValue += 500 * iHasMetCount;
		// K-Mod TODO; increase the bonus for each known civ that we can't already tech trade with
	}

	if (GC.getTechInfo(eTech).isGoldTrading())
	{
		iValue += 200;

		if (iHasMetCount > 0)
		{
			iValue += 400;
		}
	}

	if (GC.getTechInfo(eTech).isOpenBordersTrading())
	{
		if (iHasMetCount > 0)
		{
			iValue += 500;

			if (iCoastalCities > 0)
			{
				iValue += 400;
			}
		}
	}

	if (GC.getTechInfo(eTech).isDefensivePactTrading())
	{
		iValue += 400;
	}

	if (GC.getTechInfo(eTech).isPermanentAllianceTrading() && (GC.getGameINLINE().isOption(GAMEOPTION_PERMANENT_ALLIANCES)))
	{
		iValue += 200;
	}

	if (GC.getTechInfo(eTech).isVassalStateTrading() && !(GC.getGameINLINE().isOption(GAMEOPTION_NO_VASSAL_STATES)))
	{
		iValue += 200;
	}

	// Tile improvement abilities
	if (GC.getTechInfo(eTech).isBridgeBuilding())
	{
		iValue += 200;
	}

	if (GC.getTechInfo(eTech).isIrrigation())
	{
		iValue += 400;
	}

	if (GC.getTechInfo(eTech).isIgnoreIrrigation())
	{
		iValue += 500;
	}

	if (GC.getTechInfo(eTech).isWaterWork())
	{
		iValue += (600 * iCoastalCities);
	}

	iValue += (GC.getTechInfo(eTech).getFeatureProductionModifier() * 2);
	iValue += (GC.getTechInfo(eTech).getWorkerSpeedModifier() * 4);
	iValue += (GC.getTechInfo(eTech).getTradeRoutes() * (std::max((getNumCities() + 2), iConnectedForeignCities) + 1) * ((bFinancialTrouble) ? 200 : 100));
	
	/* original bts code
	if ( AI_isDoVictoryStrategy(AI_VICTORY_DOMINATION4) )
	{
		iValue += (GC.getTechInfo(eTech).getHealth() * 350);
	}
	else
	{
		iValue += (GC.getTechInfo(eTech).getHealth() * 200);
	} */
	// K-Mod
	if (GC.getTechInfo(eTech).getHealth() != 0)
	{
		iValue += 4 * AI_getHealthWeight(GC.getTechInfo(eTech).getHealth(), 1);
	}
	if (GC.getTechInfo(eTech).getHappiness() != 0)
	{
		// (this part of the evaluation was completely missing from the original bts code)
		iValue += 6 * AI_getHappinessWeight(GC.getTechInfo(eTech).getHappiness(), 1);
	}
	// K-Mod end

	for (int iJ = 0; iJ < GC.getNumRouteInfos(); iJ++)
	{
		iValue += -(GC.getRouteInfo((RouteTypes)iJ).getTechMovementChange(eTech) * 100);
	}

	for (int iJ = 0; iJ < NUM_DOMAIN_TYPES; iJ++)
	{
		iValue += (GC.getTechInfo(eTech).getDomainExtraMoves(iJ) * 200);
	}

	for (int iJ = 0; iJ < NUM_COMMERCE_TYPES; iJ++)
	{
		if (GC.getTechInfo(eTech).isCommerceFlexible(iJ))
		{
			iValue += 100;
			if ((iJ == COMMERCE_CULTURE) && (AI_isDoVictoryStrategy(AI_VICTORY_CULTURE2)))
			{
				iValue += 1000;
			}
		}
	}

	for (int iJ = 0; iJ < GC.getNumTerrainInfos(); iJ++)
	{
		if (GC.getTechInfo(eTech).isTerrainTrade(iJ))
		{
			if (GC.getTerrainInfo((TerrainTypes)iJ).isWater())
			{
				if (pCapitalCity != NULL)
				{
					iValue += (countPotentialForeignTradeCities(pCapitalCity->area()) * 100);
				}

				if (iCoastalCities > 0)
				{
					iValue += ((bCapitalAlone) ? 950 : 350);
				}

				iValue += 50;
			}
			else
			{
				iValue += 1000;
			}
		}
	}

	if (GC.getTechInfo(eTech).isRiverTrade())
	{
		//iValue += 1000;
		iValue += 100; // K-Mod
	}

	/* ------------------ Tile Improvement Value  ------------------ */
	for (int iJ = 0; iJ < GC.getNumImprovementInfos(); iJ++)
	{
		for (int iK = 0; iK < NUM_YIELD_TYPES; iK++)
		{
			int iTempValue = 0;

			/* original code
			iTempValue += (GC.getImprovementInfo((ImprovementTypes)iJ).getTechYieldChanges(eTech, iK) * getImprovementCount((ImprovementTypes)iJ) * 50); */
			// Often, an improvment only becomes viable after it gets the tech bonus.
			// So it's silly to score the bonus proportionally to how many of the improvements we already have.
			iTempValue += GC.getImprovementInfo((ImprovementTypes)iJ).getTechYieldChanges(eTech, iK)
				* std::max(getImprovementCount((ImprovementTypes)iJ), 2*getNumCities()) * 30;
			// This new version is still bork, but at least it won't be worthless.

			iTempValue *= AI_yieldWeight((YieldTypes)iK);
			iTempValue /= 100;

			iValue += iTempValue;
		}
	}

	int iBuildValue = 0;
	for (int iJ = 0; iJ < GC.getNumBuildInfos(); iJ++)
	{
		if (GC.getBuildInfo((BuildTypes)iJ).getTechPrereq() == eTech)
		{
			ImprovementTypes eImprovement = (ImprovementTypes)(GC.getBuildInfo((BuildTypes)iJ).getImprovement());
			if (eImprovement != NO_IMPROVEMENT)
			{
				eImprovement = finalImprovementUpgrade(eImprovement);
			}
			else
			{
				// only increment build value if it is not an improvement, otherwise handle it there
				iBuildValue += 200;
			}

			if (eImprovement != NO_IMPROVEMENT)
			{
				CvImprovementInfo& kImprovement = GC.getImprovementInfo(eImprovement);
				
				int iImprovementValue = 300;

				// K-Mod note: I haven't changed much of this, but I think it's pretty bogus.
				// The xxMakesValid stuff should be scored and then _multiplied_ by the value of the improvement, not added.
				// Also, the improvement should be valued based on how much better it is compared to the improvements we can
				// already build.
				iImprovementValue += ((kImprovement.isActsAsCity()) ? 100 : 0);
				iImprovementValue += ((kImprovement.isHillsMakesValid()) ? 100 : 0);
				iImprovementValue += ((kImprovement.isFreshWaterMakesValid()) ? 200 : 0);
				iImprovementValue += ((kImprovement.isRiverSideMakesValid()) ? 100 : 0);
				iImprovementValue += ((kImprovement.isCarriesIrrigation()) ? 300 : 0);

				for (int iK = 0; iK < GC.getNumTerrainInfos(); iK++)
				{
					iImprovementValue += (kImprovement.getTerrainMakesValid(iK) ? 50 : 0);
				}

				for (int iK = 0; iK < GC.getNumFeatureInfos(); iK++)
				{
					iImprovementValue += (kImprovement.getFeatureMakesValid(iK) ? 50 : 0);
				}

				{
					int iYieldValue = 0; // K-Mod
					for (int iK = 0; iK < NUM_YIELD_TYPES; iK++)
					{
						int iTempValue = 0;

						iTempValue += (kImprovement.getYieldChange(iK) * 200);
						iTempValue += (kImprovement.getRiverSideYieldChange(iK) * 100);
						iTempValue += (kImprovement.getHillsYieldChange(iK) * 100);
						iTempValue += (kImprovement.getIrrigatedYieldChange(iK) * 150);

						/* original bts code
						// land food yield is more valueble
						if (iK == YIELD_FOOD && !kImprovement.isWater())
						{
							iTempValue *= 3;
							iTempValue /= 2;
						}
						
						if (bFinancialTrouble && iK == YIELD_COMMERCE)
						{
							iTempValue *= 2;
						} */
						// K-Mod note: value adjustments are now rolled into AI_yieldWeight

						iTempValue *= AI_yieldWeight((YieldTypes)iK);
						iTempValue /= 100;

						// iImprovementValue += iTempValue;
						iYieldValue += iTempValue;
					}
					// K-Mod
					iImprovementValue -= 100;
					if (kImprovement.isWater())
					{
						iYieldValue *= 2;
						iYieldValue /= 3;
					}
					iImprovementValue += std::max(0, iYieldValue); // workshops would be negative value!
					// K-Mod end
				}

				for (int iK = 0; iK < GC.getNumBonusInfos(); iK++)
				{
					int iBonusValue = 0;

					iBonusValue += ((kImprovement.isImprovementBonusMakesValid(iK)) ? 450 : 0);
					iBonusValue += ((kImprovement.isImprovementBonusTrade(iK)) ? (45 * AI_bonusVal((BonusTypes) iK)) : 0);

					// K-Mod
					int iNumBonuses = 0; // only spend time counting if it's worth something.
					if (iBonusValue > 0)
					{
						iNumBonuses = countOwnedBonuses((BonusTypes)iK);
						if (!kTeam.isHasTech((TechTypes)GC.getBonusInfo((BonusTypes)iK).getTechReveal()))
						{
							iBonusValue *= 2;
							iBonusValue /= 3;
						}
						else if (iNumBonuses == 0)
						{
							iBonusValue = 0;
						}
						else
						{
							iBonusValue += (iNumBonuses -1) * iBonusValue / 10;
						}
					}
					// K-Mod end
					if (iBonusValue > 0)
					{
						int iYieldValue = 0; // K-Mod
						for (int iL = 0; iL < NUM_YIELD_TYPES; iL++)
						{
							int iTempValue = 0;

							/* original bts code
							iTempValue += (kImprovement.getImprovementBonusYield(iK, iL) * 300);
							iTempValue += (kImprovement.getIrrigatedYieldChange(iL) * 200);

							// food bonuses are more valueble
							if (iL == YIELD_FOOD)
							{
								iTempValue *= 2;
							}
							// otherwise, devalue the bonus slightly
							else if (iL == YIELD_COMMERCE && bFinancialTrouble)
							{
								iTempValue *= 4;
								iTempValue /= 3;
							}
							else
							{
								iTempValue *= 3;
								iTempValue /= 4;
							}
							
							if (bAdvancedStart && getCurrentEra() < 2)
							{
								iValue *= (iL == YIELD_FOOD) ? 3 : 2;
							}
							iTempValue *= AI_yieldWeight((YieldTypes)iL);
							iTempValue /= 100;

							iBonusValue += iTempValue; */
							// K-Mod. Note: value adjustments are now rolled into AI_yieldWeight
							iTempValue += (kImprovement.getImprovementBonusYield(iK, iL) * 200);
							iTempValue += (kImprovement.getIrrigatedYieldChange(iL) * 100); // already taken into account?

							iTempValue *= AI_yieldWeight((YieldTypes)iL);
							iTempValue /= 100;

							iYieldValue += iTempValue;
							// K-Mod end
						}

						/* original bts code
						iNumBonuses = countOwnedBonuses((BonusTypes)iK);
						if (iNumBonuses > 0)
						{
							iBonusValue *= (iNumBonuses + 2);
							iBonusValue /= kImprovement.isWater() ? 4 : 3;	// water resources are worth less
							iImprovementValue += iBonusValue;
						} */
						// K-Mod
						if (kImprovement.isWater())
						{
							iYieldValue *= (iNumBonuses + 2);
							iYieldValue /= 4;
						}
						else
						{
							iYieldValue *= (2*iNumBonuses + 2);
							iYieldValue /= 3;
						}

						iImprovementValue += iBonusValue + iYieldValue;
						// K-Mod end
					}
				}
				
				// if water improvement, weight by coastal cities (weight the whole build)
				if (kImprovement.isWater())
				{
					iImprovementValue *= iCoastalCities;
					iImprovementValue /= std::max(1, iCityCount/2);
				}
				
				iBuildValue += iImprovementValue;
			}

			RouteTypes eRoute = ((RouteTypes)(GC.getBuildInfo((BuildTypes)iJ).getRoute()));

			if (eRoute != NO_ROUTE)
			{
				//iBuildValue += ((getBestRoute() == NO_ROUTE) ? 700 : 200) * (getNumCities() + (bAdvancedStart ? 4 : 0));
				int iRouteValue = ((getBestRoute() == NO_ROUTE) ? 700 : 200) * (getNumCities() + (bAdvancedStart ? 4 : 0)); // K-Mod

				for (int iK = 0; iK < NUM_YIELD_TYPES; iK++)
				{
					int iTempValue = 0;

					/* original bts code
					iTempValue += (GC.getRouteInfo(eRoute).getYieldChange(iK) * 100);

					for (int iL = 0; iL < GC.getNumImprovementInfos(); iL++)
					{
						iTempValue += (GC.getImprovementInfo((ImprovementTypes)iL).getRouteYieldChanges(eRoute, iK) * 50);
					} */
					// K-Mod. wtf? c.f. the improvement upgrade evaluation - which adds 50 for every improvement!
					iTempValue += getNumCities() * GC.getRouteInfo(eRoute).getYieldChange(iK) * 100; // epic bonus for epic effect
					for (int iL = 0; iL < GC.getNumImprovementInfos(); iL++)
					{
						iTempValue += GC.getImprovementInfo((ImprovementTypes)iL).getRouteYieldChanges(eRoute, iK) * std::max(getImprovementCount((ImprovementTypes)iJ), 3*getNumCities()/2) * 30;
					}
					// K-Mod end

					iTempValue *= AI_yieldWeight((YieldTypes)iK);
					iTempValue /= 100;

					//iBuildValue += iTempValue;
					iRouteValue += iTempValue; // K-Mod
				}
				// K-Mod. Devalue it if we don't have the resources. (based on my code for unit evaluation)
				bool bMaybeMissing = false; // if we don't know if we have the bonus or not
				bool bDefinitelyMissing = false; // if we can see the bonuses, and we know we don't have any.
				const CvTeam& kTeam = GET_TEAM(getTeam());

				for (int iK = 0; iK < GC.getNUM_ROUTE_PREREQ_OR_BONUSES(); ++iK)
				{
					BonusTypes ePrereqBonus = (BonusTypes)GC.getRouteInfo(eRoute).getPrereqOrBonus(iK);
					if (ePrereqBonus != NO_BONUS)
					{
						if (hasBonus(ePrereqBonus))
						{
							bDefinitelyMissing = false;
							bMaybeMissing = false;
							break;
						}
						else
						{
							if ((kTeam.isHasTech((TechTypes)(GC.getBonusInfo(ePrereqBonus).getTechReveal())) || kTeam.isForceRevealedBonus(ePrereqBonus)) && countOwnedBonuses(ePrereqBonus) == 0)
							{
								bDefinitelyMissing = true;
							}
							else
							{
								bMaybeMissing = true;
							}
						}
					}
				}
				BonusTypes ePrereqBonus = (BonusTypes)GC.getRouteInfo(eRoute).getPrereqBonus();
				if (ePrereqBonus != NO_BONUS && !hasBonus(ePrereqBonus))
				{
					if ((kTeam.isHasTech((TechTypes)(GC.getBonusInfo(ePrereqBonus).getTechReveal())) || kTeam.isForceRevealedBonus(ePrereqBonus)) &&
						countOwnedBonuses(ePrereqBonus) == 0)
					{							
						bDefinitelyMissing = true;
					}
					else
					{							
						bMaybeMissing = true;
					}
				}
				if (bDefinitelyMissing)
				{
					iRouteValue /= 3;
				}
				else if (bMaybeMissing)
				{
					iRouteValue /= 2;
				}
				iBuildValue += iRouteValue;
				// K-Mod end
			}
		}
	}

    //the way feature-remove is done in XML is pretty weird
    //I believe this code needs to be outside the general BuildTypes loop
    //to ensure the feature-remove is only counted once rather than once per build
	//which could be a lot since nearly every build clears jungle...

	for (int iJ = 0; iJ < GC.getNumFeatureInfos(); iJ++)
    {
        bool bIsFeatureRemove = false;
        for (int iK = 0; iK < GC.getNumBuildInfos(); iK++)
        {
            if (GC.getBuildInfo((BuildTypes)iK).getFeatureTech(iJ) == eTech)
            {
                bIsFeatureRemove = true;
                break;
            }
        }

        if (bIsFeatureRemove)
        {
            iBuildValue += 100;

            if ((GC.getFeatureInfo(FeatureTypes(iJ)).getHealthPercent() < 0) ||
                ((GC.getFeatureInfo(FeatureTypes(iJ)).getYieldChange(YIELD_FOOD) + GC.getFeatureInfo(FeatureTypes(iJ)).getYieldChange(YIELD_PRODUCTION) + GC.getFeatureInfo(FeatureTypes(iJ)).getYieldChange(YIELD_COMMERCE)) < 0))
            {
                iBuildValue += 25 * countCityFeatures((FeatureTypes)iJ);
            }
        }
    }

	/*if (AI_totalUnitAIs(UNITAI_WORKER) == 0)
	{
		iBuildValue /= 2;
	}*/

	iValue += iBuildValue;

	// does tech reveal bonus resources
	for (int iJ = 0; iJ < GC.getNumBonusInfos(); iJ++)
	{
		if (GC.getBonusInfo((BonusTypes)iJ).getTechReveal() == eTech)
		{
			int iRevealValue = 150;
			iRevealValue += (AI_bonusVal((BonusTypes)iJ) * 50);
			
			BonusClassTypes eBonusClass = (BonusClassTypes)GC.getBonusInfo((BonusTypes)iJ).getBonusClassType();
			int iBonusClassTotal = (paiBonusClassRevealed[eBonusClass] + paiBonusClassUnrevealed[eBonusClass]);
			
			//iMultiplier is basically a desperation value
			//it gets larger as the AI runs out of options
			//Copper after failing to get horses is +66%
			//Iron after failing to get horse or copper is +200%
			//but with either copper or horse, Iron is only +25%
			int iMultiplier = 0;
			if (iBonusClassTotal > 0)
			{
                iMultiplier = (paiBonusClassRevealed[eBonusClass] - paiBonusClassHave[eBonusClass]);
                iMultiplier *= 100;
                iMultiplier /= iBonusClassTotal;
                
                iMultiplier *= (paiBonusClassRevealed[eBonusClass] + 1);
                iMultiplier /= ((paiBonusClassHave[eBonusClass] * iBonusClassTotal) + 1);
			}
			
			iMultiplier *= std::min(3, getNumCities());
			iMultiplier /= 3;
			
			iRevealValue *= 100 + iMultiplier;
			iRevealValue /= 100;

			// K-Mod
			// If we don't yet have the 'enable' tech, reduce the value of the reveal.
			if (GC.getBonusInfo((BonusTypes)iJ).getTechCityTrade() != eTech &&
				!GET_TEAM(getTeam()).isHasTech((TechTypes)(GC.getBonusInfo((BonusTypes)iJ).getTechCityTrade())))
				iValue /= 3;
			// K-Mod end

			iValue += iRevealValue;
		}
		// K-Mod: Value for enabling resources that are already revealed
		else if (GC.getBonusInfo((BonusTypes)iJ).getTechCityTrade() == eTech &&
			GET_TEAM(getTeam()).isHasTech((TechTypes)(GC.getBonusInfo((BonusTypes)iJ).getTechReveal())))
		{
			int iOwned = countOwnedBonuses((BonusTypes)iJ);
			if (iOwned > 0)
			{
				int iEnableValue = 150;
				iEnableValue += (AI_bonusVal((BonusTypes)iJ) * 50);
				iEnableValue *= (iOwned > 1) ? 150 : 100;
				iEnableValue /= 100;

				iValue += iEnableValue;
			}
		}
		// K-Mod end
	}

	/* ------------------ Unit Value  ------------------ */
	bool bEnablesUnitWonder;
	iValue += AI_techUnitValue( eTech, iPathLength, bEnablesUnitWonder );
	
	if (bEnablesUnitWonder)
	{
		int iWonderRandom = ((bAsync) ? GC.getASyncRand().get(400, "AI Research Wonder Unit ASYNC") : GC.getGameINLINE().getSorenRandNum(400, "AI Research Wonder Unit"));
		iValue += iWonderRandom + (bCapitalAlone ? 200 : 0);

		iRandomMax += 400;
		iRandomFactor += iWonderRandom;
	}


	/* ------------------ Building Value  ------------------ */
	bool bEnablesWonder;
	iValue += AI_techBuildingValue( eTech, iPathLength, bEnablesWonder );
	iValue -= AI_obsoleteBuildingPenalty(eTech, bAsync); // K-Mod!

	// if it gives at least one wonder
	if (bEnablesWonder)
	{
		int iWonderRandom = ((bAsync) ? GC.getASyncRand().get(800, "AI Research Wonder Building ASYNC") : GC.getGameINLINE().getSorenRandNum(800, "AI Research Wonder Building"));
		iValue += (500 + iWonderRandom) / (bAdvancedStart ? 5 : 1);

		iRandomMax += 800;
		iRandomFactor += iWonderRandom;
	}

	/* ------------------ Project Value  ------------------ */
	bool bEnablesProjectWonder = false;
	for (int iJ = 0; iJ < GC.getNumProjectInfos(); iJ++)
	{
		if (GC.getProjectInfo((ProjectTypes)iJ).getTechPrereq() == eTech)
		{
			iValue += 1000;

			if( (VictoryTypes)GC.getProjectInfo((ProjectTypes)iJ).getVictoryPrereq() != NO_VICTORY )
			{
				if( !(GC.getProjectInfo((ProjectTypes)iJ).isSpaceship()) )
				{
					// Apollo
					iValue += (AI_isDoVictoryStrategy(AI_VICTORY_SPACE2) ? 2000 : 100);
				}
				else
				{
					// Space ship parts (changed by K-Mod)
					if (AI_isDoVictoryStrategy(AI_VICTORY_SPACE2))
					{
						iValue += 1000;
					}
					if (AI_isDoVictoryStrategy(AI_VICTORY_SPACE3))
					{
						iValue += 800; // (additional)
					}
				}
			}

			if (iPathLength <= 1)
			{
				if (getTotalPopulation() > 5)
				{
					if (isWorldProject((ProjectTypes)iJ))
					{
						if (!(GC.getGameINLINE().isProjectMaxedOut((ProjectTypes)iJ)))
						{
							bEnablesProjectWonder = true;

							if (bCapitalAlone)
							{
								iValue += 100;
							}
						}
					}
				}
			}
		}
	}
	if (bEnablesProjectWonder)
	{
		int iWonderRandom = ((bAsync) ? GC.getASyncRand().get(200, "AI Research Wonder Project ASYNC") : GC.getGameINLINE().getSorenRandNum(200, "AI Research Wonder Project"));
		iValue += iWonderRandom;

		iRandomMax += 200;
		iRandomFactor += iWonderRandom;
	}


	/* ------------------ Process Value  ------------------ */
	bool bIsGoodProcess = false;
	for (int iJ = 0; iJ < GC.getNumProcessInfos(); iJ++)
	{
		if (GC.getProcessInfo((ProcessTypes)iJ).getTechPrereq() == eTech)
		{
			iValue += 100;

			for (int iK = 0; iK < NUM_COMMERCE_TYPES; iK++)
			{
				int iTempValue = (GC.getProcessInfo((ProcessTypes)iJ).getProductionToCommerceModifier(iK) * 4);

				iTempValue *= AI_commerceWeight((CommerceTypes)iK);
				iTempValue /= 100;

				if (iK == COMMERCE_GOLD || iK == COMMERCE_RESEARCH)
				{
					bIsGoodProcess = true;
				}
				else if ((iK == COMMERCE_CULTURE) && AI_isDoVictoryStrategy(AI_VICTORY_CULTURE1))
				{
					iTempValue *= 3;
				}

				iValue += iTempValue;
			}
		}
	}

	if (bIsGoodProcess && bFinancialTrouble)
	{
		bool bHaveGoodProcess = false;
		for (int iJ = 0; iJ < GC.getNumProcessInfos(); iJ++)
		{
			if (kTeam.isHasTech((TechTypes)GC.getProcessInfo((ProcessTypes)iJ).getTechPrereq()))
			{
				bHaveGoodProcess = (GC.getProcessInfo((ProcessTypes)iJ).getProductionToCommerceModifier(COMMERCE_GOLD) + GC.getProcessInfo((ProcessTypes)iJ).getProductionToCommerceModifier(COMMERCE_RESEARCH)) > 0;
				if (bHaveGoodProcess)
				{
					break;
				}
			}
		}
		if (!bHaveGoodProcess)
		{
			iValue += 1500;										
		}
	}

	/* ------------------ Civic Value  ------------------ */
	for (int iJ = 0; iJ < GC.getNumCivicInfos(); iJ++)
	{
		if (GC.getCivicInfo((CivicTypes)iJ).getTechPrereq() == eTech)
		{
			//iValue += 200; // disabled by K-Mod

			CivicTypes eCivic = getCivics((CivicOptionTypes)(GC.getCivicInfo((CivicTypes)iJ).getCivicOptionType()));
			if (NO_CIVIC != eCivic)
			{
				int iCurrentCivicValue = AI_civicValue(eCivic);
				int iNewCivicValue = AI_civicValue((CivicTypes)iJ);

				if (iNewCivicValue > iCurrentCivicValue)
				{
					iValue += std::min(2400, (2400 * (iNewCivicValue - iCurrentCivicValue)) / std::max(1, iCurrentCivicValue));
				}
				iValue += std::max(0, 200 * iNewCivicValue / std::max(1, iCurrentCivicValue)); // K-Mod, replacing the flat 200 above.
				
				if (eCivic == GC.getLeaderHeadInfo(getPersonalityType()).getFavoriteCivic())
				{
					iValue += 600;
				}
			}
			// K-Mod
			else
				iValue += 1000;
			// K-Mod end
		}
	}

	if (iPathLength <= 2)
	{
		if (GC.getGameINLINE().countKnownTechNumTeams(eTech) == 0)
		{
			int iReligionValue = 0;
			int iPotentialReligions = 0;
			for (int iJ = 0; iJ < GC.getNumReligionInfos(); iJ++)
			{
				TechTypes eReligionTech = (TechTypes)GC.getReligionInfo((ReligionTypes)iJ).getTechPrereq();
				if (kTeam.isHasTech(eReligionTech))
				{
					if (!(GC.getGameINLINE().isReligionSlotTaken((ReligionTypes)iJ)))
					{
						iPotentialReligions++;
					}
				}
				if (eReligionTech == eTech)
				{
					if (!(GC.getGameINLINE().isReligionSlotTaken((ReligionTypes)iJ)))
					{
						int iRoll = 2400;
						if (!GC.getGame().isOption(GAMEOPTION_PICK_RELIGION))
						{
							ReligionTypes eFavorite = (ReligionTypes)GC.getLeaderHeadInfo(getLeaderType()).getFavoriteReligion();
							if (eFavorite != NO_RELIGION)
							{
								if (iJ == eFavorite)
								{
									iReligionValue += 1 + ((bAsync) ? GC.getASyncRand().get(1200, "AI Research Religion (Favorite) ASYNC") : GC.getGameINLINE().getSorenRandNum(1200, "AI Research Religion (Favorite)"));
									iRandomMax += 1200;
								}
								else
								{
									iRoll *= 2;
									iRoll /= 3;
								}
							}
						}
						iReligionValue += 1 + ((bAsync) ? GC.getASyncRand().get(iRoll, "AI Research Religion ASYNC") : GC.getGameINLINE().getSorenRandNum(iRoll, "AI Research Religion"));
						iRandomMax += iRoll;
						iRandomFactor += iReligionValue;

						if (iPathLength < 2)
						{
							iReligionValue *= 3;
							iReligionValue /= 2;
						}
					}
				}
			}
			
			if (iReligionValue > 0)
			{
				if (AI_isDoVictoryStrategy(AI_VICTORY_CULTURE1))
				{
					iReligionValue += 500;

					if (countHolyCities() < 1)
					{
						iReligionValue += 1000;
					}
				}
				else
				{
					iReligionValue /= (1 + countHolyCities() + ((iPotentialReligions > 0) ? 1 : 0));
				}

				if ((countTotalHasReligion() == 0) && (iPotentialReligions == 0))
				{
					iReligionValue *= 2;
					iReligionValue += 500;
				}

				if (AI_isDoStrategy(AI_STRATEGY_DAGGER))
				{
					iReligionValue /= 2;
				}
				iValue += iReligionValue;
			}

			// K-Mod. Made a list of corporation HQs that this tech enables.
			// (note: for convenience, I've assumed that each corp has at most one type of HQ building.)
			std::vector<BuildingTypes> corpHQs(GC.getNumCorporationInfos(), NO_BUILDING);

			for (int iB = 0; iB < GC.getNumBuildingInfos(); iB++)
			{
				const CvBuildingInfo& kBuildingInfo = GC.getBuildingInfo((BuildingTypes)iB);
				if (kBuildingInfo.getFoundsCorporation() != NO_CORPORATION)
				{
					if (isTechRequiredForBuilding(eTech, (BuildingTypes)iB))
					{
						FAssert(kBuildingInfo.getFoundsCorporation() < (int)corpHQs.size());
						corpHQs[kBuildingInfo.getFoundsCorporation()] = (BuildingTypes)iB;
					}
				}
			}
			// K-Mod end (the corpHQs map is used in the next section)

			for (int iJ = 0; iJ < GC.getNumCorporationInfos(); iJ++)
			{
				/* original bts code
				if (GC.getCorporationInfo((CorporationTypes)iJ).getTechPrereq() == eTech)
				{
					if (!(GC.getGameINLINE().isCorporationFounded((CorporationTypes)iJ)))
					{
						iValue += 100 + ((bAsync) ? GC.getASyncRand().get(2400, "AI Research Corporation ASYNC") : GC.getGameINLINE().getSorenRandNum(2400, "AI Research Corporation"));
					}
				} */
				// K-Mod
				if (!GC.getGameINLINE().isCorporationFounded((CorporationTypes)iJ))
				{
					int iMissingTechs = 0;
					bool bCorpTech = false;

					if (GC.getCorporationInfo((CorporationTypes)iJ).getTechPrereq() == eTech)
					{
						bCorpTech = true;
						// missing tech stays at 0, because this is a special case. (ie. no great person required)
					}
					else if (corpHQs[iJ] != NO_BUILDING)
					{
						const CvBuildingInfo& kBuildingInfo = GC.getBuildingInfo(corpHQs[iJ]);
						if (kBuildingInfo.getPrereqAndTech() == eTech ||
							kTeam.isHasTech((TechTypes)kBuildingInfo.getPrereqAndTech()) ||
							canResearch((TechTypes)kBuildingInfo.getPrereqAndTech()))
						{
							bCorpTech = true;
							// Count the required techs. (cf. isTechRequiredForBuilding)

							iMissingTechs += !kTeam.isHasTech((TechTypes)kBuildingInfo.getPrereqAndTech()) ? 1 : 0;

							for (int iP = 0; iP < GC.getNUM_BUILDING_AND_TECH_PREREQS(); iP++)
							{
								iMissingTechs += !kTeam.isHasTech((TechTypes)kBuildingInfo.getPrereqAndTechs(iP)) ? 1 : 0;
							}

							SpecialBuildingTypes eSpecial = (SpecialBuildingTypes)kBuildingInfo.getSpecialBuildingType();
							iMissingTechs += eSpecial != NO_SPECIALBUILDING && !kTeam.isHasTech((TechTypes)GC.getSpecialBuildingInfo(eSpecial).getTechPrereq()) ? 1 : 0;

							FAssert(iMissingTechs > 0);
						}
					}
					if (bCorpTech)
					{
						int iCorpValue = AI_corporationValue((CorporationTypes)iJ); // roughly 100x commerce from corp.
						if (iMissingTechs == 0)
							iCorpValue = 3 * iCorpValue / 2; // this won't happen in standard BtS - but it might in some mods.
						else
							iCorpValue /= iMissingTechs;

						if (iCorpValue > 0)
						{
							if (isNoCorporations())
								iCorpValue /= 3;

							if (AI_getFlavorValue(FLAVOR_GOLD) > 0)
							{
								iCorpValue *= 20 + AI_getFlavorValue(FLAVOR_GOLD);
								iCorpValue /= 20;
								iValue += iCorpValue/2;
								iValue += ((bAsync) ? GC.getASyncRand().get(iCorpValue, "AI Research Corporation ASYNC") : GC.getGameINLINE().getSorenRandNum(3*iCorpValue/2, "AI Research Corporation"));
								iRandomMax += iCorpValue;
							}
							else
							{
								iValue += iCorpValue/3;
								iValue += ((bAsync) ? GC.getASyncRand().get(iCorpValue, "AI Research Corporation ASYNC") : GC.getGameINLINE().getSorenRandNum(5*iCorpValue/3, "AI Research Corporation"));
								iRandomMax += iCorpValue;
							}
						}
					}
				}
				// K-Mod end
			}

			if (getTechFreeUnit(eTech) != NO_UNIT)
			{
				int iGreatPeopleRandom = ((bAsync) ? GC.getASyncRand().get(3200, "AI Research Great People ASYNC") : GC.getGameINLINE().getSorenRandNum(3200, "AI Research Great People"));
				iRandomMax += 3200; // K-Mod
				iValue += iGreatPeopleRandom;
				
				iRandomMax += 3200;
				iRandomFactor += iGreatPeopleRandom;

				if (bCapitalAlone)
				{
					iValue += 400;
				}

				iValue += 200;
			}

			//iValue += (GC.getTechInfo(eTech).getFirstFreeTechs() * (200 + ((bCapitalAlone) ? 400 : 0) + ((bAsync) ? GC.getASyncRand().get(3200, "AI Research Free Tech ASYNC") : GC.getGameINLINE().getSorenRandNum(3200, "AI Research Free Tech"))));
			// K-Mod, 22/jan/11, karadoc: more ad-hoc adjustments... this stuff sucks so much.
			iValue += (GC.getTechInfo(eTech).getFirstFreeTechs() * (500 + ((bCapitalAlone) ? 400 : 0) + ((bAsync) ? GC.getASyncRand().get(6000, "AI Research Free Tech ASYNC") : GC.getGameINLINE().getSorenRandNum(6000, "AI Research Free Tech"))));
			iRandomMax += 6000; // K-Mod
		}
	}

	iValue += GC.getTechInfo(eTech).getAIWeight();

	if (!isHuman())
	{
		for (int iJ = 0; iJ < GC.getNumFlavorTypes(); iJ++)
		{
			iValue += (AI_getFlavorValue((FlavorTypes)iJ) * GC.getTechInfo(eTech).getFlavorValue(iJ) * 20);
		}
	}

	if (GC.getTechInfo(eTech).isRepeat())
	{
		iValue /= 10;
	}

	if (bIgnoreCost)
	{
		iValue *= (1 + (getResearchTurnsLeft((eTech), false)));
		iValue /= 10;
	}
	else
	{
		if (iValue > 0)
		{
			//this stops quick speed messing up.... might want to adjust by other things too...
			int iAdjustment = GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).getResearchPercent();
	
			// Shouldn't run this during anarchy
			int iTurnsLeft = getResearchTurnsLeftTimes100((eTech), false);
			bool bCheapBooster = ((iTurnsLeft < (2 * iAdjustment)) && (0 == ((bAsync) ? GC.getASyncRand().get(5, "AI Choose Cheap Tech") : GC.getGameINLINE().getSorenRandNum(5, "AI Choose Cheap Tech"))));
			
			
			//iValue *= 100000;
			iValue *= 2000; // K-Mod
			
            iValue /= (iTurnsLeft + (bCheapBooster ? 1 : (5 * iAdjustment)));
		}
	}
	
	//Tech Whore								
	if (!GC.getGameINLINE().isOption(GAMEOPTION_NO_TECH_TRADING))
	{
		if (GC.getTechInfo(eTech).isTechTrading() || kTeam.isTechTrading())
		{
			// K-Mod TODO: Consider changing this so that it is less random
			if (((bAsync) ? GC.getASyncRand().get(100, "AI Tech Whore ASYNC") : GC.getGameINLINE().getSorenRandNum(100, "AI Tech Whore")) < (GC.getGameINLINE().isOption(GAMEOPTION_NO_TECH_BROKERING) ? 20 : 10))
			{
				int iKnownCount = 0;
				int iPossibleKnownCount = 0;

				for (int iTeam = 0; iTeam < MAX_CIV_TEAMS; iTeam++)
				{
					if (GET_TEAM((TeamTypes)iTeam).isAlive())
					{
						if (GET_TEAM(getTeam()).isHasMet((TeamTypes)iTeam))
						{
							if (GET_TEAM((TeamTypes)iTeam).isHasTech(eTech))
							{
								iKnownCount++;
							}
						}

						iPossibleKnownCount++;
					}
				}
				
				if (iKnownCount == 0)
				{
					if (iPossibleKnownCount > 2)
					{
						int iTradeModifier = std::min(150, 25 * (iPossibleKnownCount - 2));
						iValue *= 100 + iTradeModifier;
						iValue /= 100;
					}
				}
			}
		}
	}

	if (AI_isDoVictoryStrategy(AI_VICTORY_CULTURE3))
	{
		int iCVValue = AI_cultureVictoryTechValue(eTech);
		/* original bts code
		iValue *= (iCVValue + 10);
		iValue /= ((iCVValue < 100) ? 400 : 100); */
		// K-Mod. I don't think AI_cultureVictoryTechValue is accurate enough to play such a large role.
		iValue *= (iCVValue + 100);
		iValue /= 200;
		// K-Mod
	}

/***
**** K-Mod, 12/sep/10, Karadoc
**** Use a random _factor_ at the end.
***/
	iRandomFactor = ((bAsync) ? GC.getASyncRand().get(100, "AI Research factor ASYNC") : GC.getGameINLINE().getSorenRandNum(100, "AI Research factor"));
	iValue *= (950 + iRandomFactor); // between 95% and 105%
	iValue /= 1000;
/***
**** END
***/

	FAssert(iValue < INT_MAX);
	iValue = std::max(1, (int)iValue);

	return iValue;
}

// K-mod. This function returns the (positive) value of the buildings we will lose by researching eTech.
// (I think it's crazy that this stuff wasn't taken into account in original BtS)
int CvPlayerAI::AI_obsoleteBuildingPenalty(TechTypes eTech, bool bConstCache) const
{
	int iTotalPenalty = 0;
	for (int iI = 0; iI < GC.getNumBuildingClassInfos(); iI++)
	{
		BuildingTypes eLoopBuilding = ((BuildingTypes)(GC.getCivilizationInfo(getCivilizationType()).getCivilizationBuildings(iI)));

		if (eLoopBuilding != NO_BUILDING)
		{
			bool bObsolete = GC.getBuildingInfo(eLoopBuilding).getObsoleteTech() == eTech
				|| (GC.getBuildingInfo(eLoopBuilding).getSpecialBuildingType() != NO_SPECIALBUILDING
				&& GC.getSpecialBuildingInfo((SpecialBuildingTypes)GC.getBuildingInfo(eLoopBuilding).getSpecialBuildingType()).getObsoleteTech() == eTech);

			if (bObsolete && getBuildingClassCount((BuildingClassTypes)iI) > 0)
			{
				int iLoop;
				for (const CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
				{
					int n = pLoopCity->getNumActiveBuilding(eLoopBuilding);
					if (n > 0)
					{
						iTotalPenalty += n * pLoopCity->AI_buildingValue(eLoopBuilding, 0, 0, bConstCache);
					}
				}
			}
		}
	}
	// I don't really want to, but we have to divide by the number of cities
	// so that the penalty scales like the rest of the tech values.
	// iScale is set more or less arbitrarily based on trial and error.
	int iScale = 70;

	if (AI_getFlavorValue(FLAVOR_SCIENCE) > 0)
		iScale -= 10 + AI_getFlavorValue(FLAVOR_SCIENCE);

	iTotalPenalty *= iScale;
	iTotalPenalty /= 10 * std::max(1, getNumCities());

	return iTotalPenalty;
}
// K-Mod

int CvPlayerAI::AI_techBuildingValue( TechTypes eTech, int iPathLength, bool &bEnablesWonder ) const
{
	bool bCapitalAlone = (GC.getGameINLINE().getElapsedGameTurns() > 0) ? AI_isCapitalAreaAlone() : false;
	bool bFinancialTrouble = AI_isFinancialTrouble();
	int iTeamCityCount = GET_TEAM(getTeam()).getNumCities();
	int iCoastalCities = countNumCoastalCities();
	int iCityCount = getNumCities();

	BuildingTypes eLoopBuilding;
	int iTempValue = 0;
	int iValue = 0;

	int iBestLandBuildingValue = 0;
	bEnablesWonder = false;
	int iExistingCultureBuildingCount = 0;
	bool bIsCultureBuilding = false;
	for (int iJ = 0; iJ < GC.getNumBuildingClassInfos(); iJ++)
	{
		eLoopBuilding = ((BuildingTypes)(GC.getCivilizationInfo(getCivilizationType()).getCivilizationBuildings(iJ)));

		//if (eLoopBuilding != NO_BUILDING)
		// K-Mod. don't count corp hq, academies, or another buildings that require a great person.
		// (it's not that these buildings don't deserve to be counted; but rather that this function isn't
		// smart enough to properly account for their rarity. I'm not a fan of this function...)
		if (eLoopBuilding != NO_BUILDING && GC.getBuildingInfo(eLoopBuilding).getProductionCost() > 0)
		// K-Mod end
		{
			CvBuildingInfo& kLoopBuilding = GC.getBuildingInfo(eLoopBuilding);
			if (isTechRequiredForBuilding((eTech), eLoopBuilding))
			{
				int iBuildingValue = AI_isDoStrategy(AI_STRATEGY_ECONOMY_FOCUS) ? 200 : 100; // was 0 (moved the 100 from later)
				
				if (kLoopBuilding.getSpecialBuildingType() != NO_BUILDING)
				{
					iBuildingValue += ((bCapitalAlone) ? 50 : 25); // was 100 : 25
				}
				else
				{
					iBuildingValue += ((bCapitalAlone) ? 100 : 50); // was 200 : 50
				}

				//the granary effect is SO powerful it deserves special code
				if (kLoopBuilding.getFoodKept() > 0)
				{
					iBuildingValue += (15 * kLoopBuilding.getFoodKept());
				}

				if (kLoopBuilding.getMaintenanceModifier() < 0)
				{
					int iLoop;
					int iCount = 0;
					CvCity* pLoopCity;
					iTempValue = 0;
					for (pLoopCity = firstCity(&iLoop, true); pLoopCity != NULL; pLoopCity = nextCity(&iLoop, true))
					{
						iTempValue += pLoopCity->getMaintenanceTimes100();
						iCount++;
						if (iCount > 4)
						{
							break;
						}
					}
					iTempValue /= std::max(1, iCount);
					iTempValue *= -kLoopBuilding.getMaintenanceModifier();
					iTempValue /= 10 * 100;

					iValue += iTempValue;
				}

				// iBuildingValue += 100; // moved to the top

                if ((GC.getBuildingClassInfo((BuildingClassTypes)iJ).getDefaultBuildingIndex()) != (GC.getCivilizationInfo(getCivilizationType()).getCivilizationBuildings(iJ)))
                {
                    //UB
                    iBuildingValue += 600;
                }
                
				if( !isLimitedWonderClass((BuildingClassTypes)iJ) )
				{
					if (kLoopBuilding.getCommerceChange(COMMERCE_CULTURE) > 0 || kLoopBuilding.getObsoleteSafeCommerceChange(COMMERCE_CULTURE) > 0)
					{
						bIsCultureBuilding = true;
					}
				}
                
                if (AI_isDoVictoryStrategy(AI_VICTORY_CULTURE2))
                {
                    /* original bts code
					int iMultiplier = (isLimitedWonderClass((BuildingClassTypes)iJ) ? 1 : 3);
                    iBuildingValue += (150 * (kLoopBuilding.getCommerceChange(COMMERCE_CULTURE) + kLoopBuilding.getObsoleteSafeCommerceChange(COMMERCE_CULTURE))) * iMultiplier;
                    iBuildingValue += kLoopBuilding.getCommerceModifier(COMMERCE_CULTURE) * 4 * iMultiplier ; */
					// K-Mod (just tweaking some of the numbers)
					int iWeight = isLimitedWonderClass((BuildingClassTypes)iJ) ? 50 : 250;
                    iBuildingValue += iWeight * (kLoopBuilding.getCommerceChange(COMMERCE_CULTURE) + kLoopBuilding.getObsoleteSafeCommerceChange(COMMERCE_CULTURE));
                    iBuildingValue += (350 + 3 * iWeight) * kLoopBuilding.getCommerceModifier(COMMERCE_CULTURE) / 100;
					// K-Mod end
                }

				if (bFinancialTrouble)
				{
					iBuildingValue += (-kLoopBuilding.getMaintenanceModifier()) * 15;
					iBuildingValue += kLoopBuilding.getYieldModifier(YIELD_COMMERCE) * 8;
					iBuildingValue += kLoopBuilding.getCommerceModifier(COMMERCE_GOLD) * 15;
				}

				// if this is a religious building, its not as useful
				ReligionTypes eReligion = (ReligionTypes) kLoopBuilding.getReligionType();
				if (eReligion != NO_RELIGION)
				{
					
					// reduce by a factor based on how many cities we have with that relgion
					if (iTeamCityCount > 0)
					{
						int iCitiesWithReligion = GET_TEAM(getTeam()).getHasReligionCount(eReligion);
						
						iBuildingValue *= (4 + iCitiesWithReligion);
						iBuildingValue /= (4 + iTeamCityCount);
					}

					// if this building requires a religion, then only count it as 1/7th as much
					// or in other words, only count things like temples once, not 7 times
					// doing it this way in case some mods give buildings to only one religion
					iBuildingValue /= std::max(1, GC.getNumReligionInfos());
				}

				// if we're close to pop domination, we love medicine!
				// don't adjust for negative modifiers to prevent ignoring assembly line, etc.
				if ( AI_isDoVictoryStrategy(AI_VICTORY_DOMINATION3) && kLoopBuilding.getHealth() > 0)
				{
					iBuildingValue += kLoopBuilding.getHealth() * 100; // was 150
				}

				if (kLoopBuilding.getPrereqAndTech() == eTech)
				{
					if (iPathLength <= 1)
					{
						if (getTotalPopulation() > 5)
						{
							if (isWorldWonderClass((BuildingClassTypes)iJ))
							{
								if (!(GC.getGameINLINE().isBuildingClassMaxedOut((BuildingClassTypes)iJ)))
								{
									bEnablesWonder = true;

									if (AI_isDoVictoryStrategy(AI_VICTORY_CULTURE1))
									{
										/*if (kLoopBuilding.getCommerceChange(COMMERCE_CULTURE) >= 3 || 
											kLoopBuilding.getObsoleteSafeCommerceChange(COMMERCE_CULTURE) >= 3 ||
											kLoopBuilding.getCommerceModifier(COMMERCE_CULTURE) >= 10)*/
										// K-Mod. Lets not fall over ourselves to get a small-culture wonder.
										if (kLoopBuilding.getCommerceChange(COMMERCE_CULTURE) +
											kLoopBuilding.getObsoleteSafeCommerceChange(COMMERCE_CULTURE) >= 4 ||
											kLoopBuilding.getCommerceModifier(COMMERCE_CULTURE) >= 20)
										{
											iValue += 400;
										}
									}

									if (bCapitalAlone)
									{
										iBuildingValue += 400;
									}
								}
							}
						}
					}
				}

				if( AI_isDoVictoryStrategy(AI_VICTORY_DIPLOMACY1) )
				{
					if( kLoopBuilding.getVoteSourceType() >= 0 )
					{
						iValue += 400;
					}
				}

				if (iBuildingValue > iBestLandBuildingValue)
				{
					iBestLandBuildingValue = iBuildingValue;
				}

				// if water building, weight by coastal cities
				if (kLoopBuilding.isWater())
				{
					iBuildingValue *= iCoastalCities;
					iBuildingValue /= std::max(1, iCityCount/2);
				}
				// if land building, is it the best?
				else if (iBuildingValue > iBestLandBuildingValue)
				{
					iBestLandBuildingValue = iBuildingValue;
				}

				iValue += iBuildingValue;
			}
			else
			{
				if (canConstruct(eLoopBuilding))
				{
					if (!isLimitedWonderClass((BuildingClassTypes)iJ))
					{
						if (kLoopBuilding.getCommerceChange(COMMERCE_CULTURE) > 0 || kLoopBuilding.getObsoleteSafeCommerceChange(COMMERCE_CULTURE) > 0)
						{
							iExistingCultureBuildingCount++;
						}
					}												
				}
			}
		}
	}
	
	if (bIsCultureBuilding && iExistingCultureBuildingCount < 2)
	{
		if (getFreeCityCommerce(COMMERCE_CULTURE) == 0)
		{
			if (getNumCities() > 1)
			{
				iValue += 150 * std::max(1, (3 - 2 * iExistingCultureBuildingCount)) * (getNumCities() - 1);
			}
		}
	}
	// if tech gives at least one building (so we can count each individual building less)
	if (iBestLandBuildingValue > 0)
	{
		iValue += iBestLandBuildingValue;
	}

	return iValue;
}


int CvPlayerAI::AI_techUnitValue( TechTypes eTech, int iPathLength, bool &bEnablesUnitWonder ) const
{
	const CvTeamAI& kTeam = GET_TEAM(getTeam()); // K-Mod

	bool bWarPlan = (kTeam.getAnyWarPlanCount(true) > 0);
	if( !bWarPlan )
	{
		// Aggressive players will stick with war techs
		if (kTeam.AI_getTotalWarOddsTimes100() > 400)
		{
			bWarPlan = true;
		}
	}

	bool bCapitalAlone = (GC.getGameINLINE().getElapsedGameTurns() > 0) ? AI_isCapitalAreaAlone() : false;
	int iHasMetCount = kTeam.getHasMetCivCount(true);
	int iCoastalCities = countNumCoastalCities();
	CvCity* pCapitalCity = getCapitalCity();

	UnitTypes eLoopUnit;
	int iMilitaryValue = 0;
	int iValue = 0;

	bEnablesUnitWonder = false;
	for (int iJ = 0; iJ < GC.getNumUnitClassInfos(); iJ++)
	{
		eLoopUnit = ((UnitTypes)(GC.getCivilizationInfo(getCivilizationType()).getCivilizationUnits(iJ)));

		if (eLoopUnit != NO_UNIT)
		{
			if (isTechRequiredForUnit((eTech), eLoopUnit))
			{
				CvUnitInfo& kLoopUnit = GC.getUnitInfo(eLoopUnit);
				iValue += 200;
				int iUnitValue = 0;
				int iNavalValue = 0;
				
				if ((GC.getUnitClassInfo((UnitClassTypes)iJ).getDefaultUnitIndex()) != (GC.getCivilizationInfo(getCivilizationType()).getCivilizationUnits(iJ)))
                {
                    //UU
                    iUnitValue += 600;
                }

				//if (kLoopUnit.getPrereqAndTech() == eTech)
				if (kLoopUnit.getPrereqAndTech() == eTech || kTeam.isHasTech((TechTypes)kLoopUnit.getPrereqAndTech()) || canResearch((TechTypes)kLoopUnit.getPrereqAndTech())) // K-Mod
				{
					iMilitaryValue = 0;

					// BBAI TODO: Change this to evaluating all unitai types defined in XML for unit?
					// Without this change many unit types are hard to evaluate, like offensive value of rifles
					// or defensive value of collateral seige
					switch (kLoopUnit.getDefaultUnitAIType())
					{
					case UNITAI_UNKNOWN:
					case UNITAI_ANIMAL:
						break;

					case UNITAI_SETTLE:
						iUnitValue += 1200;
						break;

					case UNITAI_WORKER:
						iUnitValue += 800;
						break;

					case UNITAI_ATTACK:
						iMilitaryValue += ((bWarPlan) ? 600 : 300);
						iMilitaryValue += (AI_isDoStrategy(AI_STRATEGY_DAGGER) ? 600 : 0); // was 800
						iUnitValue += 100;
						break;

					case UNITAI_ATTACK_CITY:
						iMilitaryValue += ((bWarPlan) ? 800 : 400);
						iMilitaryValue += (AI_isDoStrategy(AI_STRATEGY_DAGGER ) ? 600 : 0); // was 800
						//if (kLoopUnit.getBombardRate() > 0)
						// ... moved out of the switch
						iUnitValue += 100;
						break;

					case UNITAI_COLLATERAL:
						iMilitaryValue += ((bWarPlan) ? 600 : 300);
						break;

					case UNITAI_PILLAGE:
						iMilitaryValue += ((bWarPlan) ? 200 : 100);
						break;

					case UNITAI_RESERVE:
						iMilitaryValue += ((bWarPlan) ? 200 : 100);
						break;

					case UNITAI_COUNTER:
						iMilitaryValue += ((bWarPlan) ? 600 : 300);
						iMilitaryValue += (AI_isDoStrategy(AI_STRATEGY_DAGGER) ? 300 : 0); // was 600
						break;

					case UNITAI_PARADROP:
						iMilitaryValue += ((bWarPlan) ? 600 : 300);
						break;

					case UNITAI_CITY_DEFENSE:
						iMilitaryValue += ((bWarPlan) ? 800 : 500); // K-Mod :400 -> :500
						iMilitaryValue += ((!bCapitalAlone) ? 400 : 200);
						iUnitValue += ((iHasMetCount > 0) ? 800 : 200);
						break;

					case UNITAI_CITY_COUNTER:
						iMilitaryValue += ((bWarPlan) ? 800 : 400);
						break;

					case UNITAI_CITY_SPECIAL:
						iMilitaryValue += ((bWarPlan) ? 800 : 400);
						break;

					case UNITAI_EXPLORE:
						iUnitValue += ((bCapitalAlone) ? 100 : 200);
						break;

					case UNITAI_MISSIONARY:
						iUnitValue += ((getStateReligion() != NO_RELIGION) ? 600 : 300);
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
						iMilitaryValue += ((bWarPlan) ? 100 : 50);
						break;

					case UNITAI_ICBM:
						iMilitaryValue += ((bWarPlan) ? 200 : 100);
						break;

					case UNITAI_WORKER_SEA:
						if (iCoastalCities > 0)
						{
							// note, workboat improvements are already counted in the improvement section
						}
						break;

					case UNITAI_ATTACK_SEA:
						// BBAI TODO: Boost value for maps where Barb ships are pestering us
						if (iCoastalCities > 0)
						{
							//iMilitaryValue += ((bWarPlan) ? 200 : 100);
							iMilitaryValue += (bWarPlan ? 2 : 1) * (100 + kLoopUnit.getCollateralDamage()/2);// K-Mod
						}
						iNavalValue += 100;
						break;

					case UNITAI_RESERVE_SEA:
						if (iCoastalCities > 0)
						{
							//iMilitaryValue += ((bWarPlan) ? 100 : 50);
							iMilitaryValue += (bWarPlan ? 10 : 5) * (10 + kLoopUnit.getCollateralDamage()/20);// K-Mod
							// K-Mod note: this naval value stuff seems a bit flakey...
						}
						iNavalValue += 100;
						break;

					case UNITAI_ESCORT_SEA:
						if (iCoastalCities > 0)
						{
							iMilitaryValue += ((bWarPlan) ? 100 : 50);
						}
						iNavalValue += 100;
						break;

					case UNITAI_EXPLORE_SEA:
						if (iCoastalCities > 0)
						{
							iUnitValue += ((bCapitalAlone) ? 1800 : 600);
						}
						break;

					case UNITAI_ASSAULT_SEA:
						if (iCoastalCities > 0)
						{
							iMilitaryValue += ((bWarPlan || bCapitalAlone) ? 400 : 200);
						}
						iNavalValue += 200;
						break;

					case UNITAI_SETTLER_SEA:
						if (iCoastalCities > 0)
						{
							iUnitValue += ((bWarPlan || bCapitalAlone) ? 100 : 200);
						}
						iNavalValue += 200;
						break;

					case UNITAI_MISSIONARY_SEA:
						if (iCoastalCities > 0)
						{
							iUnitValue += 100;
						}
						break;

					case UNITAI_SPY_SEA:
						if (iCoastalCities > 0)
						{
							iMilitaryValue += 100;
						}
						break;

					case UNITAI_CARRIER_SEA:
						if (iCoastalCities > 0)
						{
							iMilitaryValue += ((bWarPlan) ? 100 : 50);
						}
						break;

					case UNITAI_MISSILE_CARRIER_SEA:
						if (iCoastalCities > 0)
						{
							iMilitaryValue += ((bWarPlan) ? 100 : 50);
						}
						break;

					case UNITAI_PIRATE_SEA:
						if (iCoastalCities > 0)
						{
							iMilitaryValue += 100;
						}
						iNavalValue += 100;
						break;

					case UNITAI_ATTACK_AIR:
						//iMilitaryValue += ((bWarPlan) ? 1200 : 800);
						// K-Mod, I've decreased the value here but added something extra a bit lower down.
						iMilitaryValue += ((bWarPlan) ? 10 : 6) * (100 + kLoopUnit.getCollateralDamage()/2);
						break;

					case UNITAI_DEFENSE_AIR:
						//iMilitaryValue += ((bWarPlan) ? 1200 : 800);
						iMilitaryValue += ((bWarPlan) ? 1000 : 600);
						break;

					case UNITAI_CARRIER_AIR:
						if (iCoastalCities > 0)
						{
							iMilitaryValue += ((bWarPlan) ? 200 : 100);
						}
						iNavalValue += 400;
						break;

					case UNITAI_MISSILE_AIR:
						iMilitaryValue += ((bWarPlan) ? 200 : 100);
						break;

					default:
						FAssertMsg(false, "Missing UNITAI type in AI_techUnitValue");
						break;
					}

					// K-Mod
					if (kLoopUnit.getBombardRate() > 0) // block moved from UNITAI_ATTACK_CITY:
					{
						iMilitaryValue += 200;

						if (AI_calculateTotalBombard(DOMAIN_LAND) == 0)
						{
							iMilitaryValue += 800;
							if (AI_isDoStrategy(AI_STRATEGY_DAGGER))
							{
								iMilitaryValue += 400; // was 1000
							}
						}
					}

					if (kLoopUnit.getUnitAIType(UNITAI_COLLATERAL) && kLoopUnit.getCollateralDamage() > 0)
					{
						// note: the following boost is for land units only.
						// Sea and air units get their collateral bonus in the switch section above.
						int iOldValue = getTypicalUnitValue(UNITAI_COLLATERAL);
						if (iOldValue > 0)
						{
							int iDelta = std::max(0, GC.getGameINLINE().AI_combatValue(eLoopUnit) - iOldValue);
							iMilitaryValue += 150 * iDelta / iOldValue;
							// this boost is a bit ad hoc. But so is the rest of the stuff in here!
							// my goal with this component is to boost the value of canons.
						}
						else if (kLoopUnit.getDefaultUnitAIType() == getTypicalUnitValue(UNITAI_COLLATERAL))
						{
							// currently there are no units with this default AI; but anyway...
							iMilitaryValue += 150;
						}
					}
					// K-Mod end

					/* original BBAI code (mostly). I've decided that most of this stuff is slightly bogus.
					// So I've disabled it, and tried to reimplement the same concepts in a more flexible way. (further down)
					if( AI_isDoStrategy(AI_STRATEGY_ALERT1) )
					{
						if( kLoopUnit.getUnitAIType(UNITAI_COLLATERAL) )
						{
							//iUnitValue += 500;
							iMilitaryValue += 500 * GC.getGameINLINE().AI_combatValue(eLoopUnit)/100; // K-Mod
						}

						if( kLoopUnit.getUnitAIType(UNITAI_CITY_DEFENSE) )
						{
							iMilitaryValue += (1000 * GC.getGameINLINE().AI_combatValue(eLoopUnit))/100; // K-Mod, was iUnitValue +=
						}
					}

					if( AI_isDoStrategy(AI_STRATEGY_TURTLE) && iPathLength <= 1)
					{
						if( kLoopUnit.getUnitAIType(UNITAI_COLLATERAL) )
						{
							//iUnitValue += 1000;
							iMilitaryValue += 1000 * GC.getGameINLINE().AI_combatValue(eLoopUnit)/100; // K-Mod
						}

						if( kLoopUnit.getUnitAIType(UNITAI_CITY_DEFENSE) )
						{
							iMilitaryValue += (2000 * GC.getGameINLINE().AI_combatValue(eLoopUnit))/100; // K-Mod, was iUnitValue +=
						}
					}

					if( AI_isDoVictoryStrategy(AI_VICTORY_CONQUEST3) )
					{
						if( kLoopUnit.getUnitAIType(UNITAI_ATTACK_CITY) )
						{
							iMilitaryValue += (1500 * GC.getGameINLINE().AI_combatValue(eLoopUnit))/100; // K-Mod, was iUnitValue
						}
					}
					else if( AI_isDoVictoryStrategy(AI_VICTORY_CONQUEST1) )
					{
						if( kLoopUnit.getUnitAIType(UNITAI_ATTACK_CITY) )
						{
							iMilitaryValue += (500 * GC.getGameINLINE().AI_combatValue(eLoopUnit))/100; // K-Mod, was iUnitValue
						}
					} */

					if (kLoopUnit.getUnitAIType(UNITAI_ASSAULT_SEA) && iCoastalCities > 0)
					{
						int iAssaultValue = 0;
						UnitTypes eExistingUnit = NO_UNIT;
						if (AI_bestAreaUnitAIValue(UNITAI_ASSAULT_SEA, NULL, &eExistingUnit) == 0)
						{
							iAssaultValue += 250;
						}
						else if( eExistingUnit != NO_UNIT )
						{
							iAssaultValue += 1000 * std::max(0, AI_unitImpassableCount(eLoopUnit) - AI_unitImpassableCount(eExistingUnit));

							int iNewCapacity = kLoopUnit.getMoves() * kLoopUnit.getCargoSpace();
							int iOldCapacity = GC.getUnitInfo(eExistingUnit).getMoves() * GC.getUnitInfo(eExistingUnit).getCargoSpace();

							iAssaultValue += (800 * (iNewCapacity - iOldCapacity)) / std::max(1, iOldCapacity);
						}

						if (iAssaultValue > 0)
						{
							int iLoop;
							CvArea* pLoopArea;
							bool bIsAnyAssault = false;
							for(pLoopArea = GC.getMapINLINE().firstArea(&iLoop); pLoopArea != NULL; pLoopArea = GC.getMapINLINE().nextArea(&iLoop))
							{
								if (AI_isPrimaryArea(pLoopArea))
								{
									if (pLoopArea->getAreaAIType(getTeam()) == AREAAI_ASSAULT)
									{
										bIsAnyAssault = true;
										break;
									}
								}
							}
							if (bIsAnyAssault)
							{
								iUnitValue += iAssaultValue * 4;
							}
							else
							{
								iUnitValue += iAssaultValue;
							}
						}
					}

					// K-Mod
					if (kLoopUnit.getDomainType() == DOMAIN_AIR)
					{
						iMilitaryValue += (bWarPlan? 600 : 400) * GC.getGameINLINE().AI_combatValue(eLoopUnit)/100;
					}

					if (iNavalValue > 0)
					{
						if (getCapitalCity() != NULL)
						{
							// BBAI TODO: A little odd ... naval value is 0 if have no colonies.
							iNavalValue *= 2 * (getNumCities() - getCapitalCity()->area()->getCitiesPerPlayer(getID()));
							iNavalValue /= getNumCities();
							
							iUnitValue += iNavalValue;
						}
					}

					if (AI_totalUnitAIs((UnitAITypes)(kLoopUnit.getDefaultUnitAIType())) == 0)
					{
						// do not give bonus to seagoing units if they are worthless
						if (iUnitValue > 0)
						{
							iUnitValue *= 2;
						}

						if (kLoopUnit.getDefaultUnitAIType() == UNITAI_EXPLORE)
						{
							if (pCapitalCity != NULL)
							{
								iUnitValue += (AI_neededExplorers(pCapitalCity->area()) * 400);
							}
						}

						if (kLoopUnit.getDefaultUnitAIType() == UNITAI_EXPLORE_SEA)
						{
							iUnitValue += 400;
							iUnitValue += ((GC.getGameINLINE().countCivTeamsAlive() - iHasMetCount) * 200);
						}
					}

					if (kLoopUnit.getUnitAIType(UNITAI_SETTLER_SEA))
					{
						if (getCapitalCity() != NULL)
						{
							UnitTypes eExistingUnit = NO_UNIT;
							int iBestAreaValue = 0;
							AI_getNumAreaCitySites(getCapitalCity()->getArea(), iBestAreaValue);

							//Early Expansion by sea
							if (AI_bestAreaUnitAIValue(UNITAI_SETTLER_SEA, NULL, &eExistingUnit) == 0)
							{
								CvArea* pWaterArea = getCapitalCity()->waterArea();
								if (pWaterArea != NULL)
								{
									int iBestOtherValue = 0;
									AI_getNumAdjacentAreaCitySites(pWaterArea->getID(), getCapitalCity()->getArea(), iBestOtherValue);
									
									if (iBestAreaValue == 0)
									{
										iUnitValue += 2000;
									}
									else if (iBestAreaValue < iBestOtherValue)
									{
										iUnitValue += 1000;
									}
									else if (iBestOtherValue > 0)
									{
										iUnitValue += 500;
									}
								}
							}
							// Landlocked expansion over ocean
							else if( eExistingUnit != NO_UNIT )
							{
								if( AI_unitImpassableCount(eLoopUnit) < AI_unitImpassableCount(eExistingUnit) )
								{
									if( iBestAreaValue < AI_getMinFoundValue() )
									{
										iUnitValue += (AI_isDoVictoryStrategy(AI_VICTORY_DOMINATION2) ? 2000 : 500);
									}
								}
							}
						}
					}

					if( iMilitaryValue > 0 )
					{
						if (iHasMetCount == 0)
						{
							iMilitaryValue /= 2;
						}

						if (bCapitalAlone)
						{
							iMilitaryValue *= 2;
							iMilitaryValue /= 3;
						}

						// K-Mod
						if (AI_isDoStrategy(AI_STRATEGY_GET_BETTER_UNITS) && kLoopUnit.getDomainType() == DOMAIN_LAND)
						{
							iMilitaryValue += 3 * GC.getGameINLINE().AI_combatValue(eLoopUnit);
							iMilitaryValue *= 3;
							iMilitaryValue /= 2;
						}

						// This multiplier stuff is basically my version of the BBAI code I disabled further up.
						int iMultiplier = 100;
						if (AI_isDoVictoryStrategy(AI_VICTORY_CONQUEST1 | AI_VICTORY_DOMINATION2) || AI_isDoStrategy(AI_STRATEGY_ALERT1))
						{
							iMultiplier += 25;
							if (AI_isDoVictoryStrategy(AI_VICTORY_CONQUEST2 | AI_VICTORY_DOMINATION3) || AI_isDoStrategy(AI_STRATEGY_ALERT2))
							{
								iMultiplier += 25;
								if (AI_isDoVictoryStrategy(AI_VICTORY_CONQUEST3 | AI_VICTORY_DOMINATION4))
								{
									iMultiplier += 25;
								}
							}
						}
						if (AI_isDoStrategy(AI_STRATEGY_ALERT1 | AI_STRATEGY_TURTLE)
							&& (kLoopUnit.getUnitAIType(UNITAI_CITY_DEFENSE)
							|| kLoopUnit.getUnitAIType(UNITAI_CITY_SPECIAL)
							|| kLoopUnit.getUnitAIType(UNITAI_CITY_COUNTER)
							|| kLoopUnit.getUnitAIType(UNITAI_COLLATERAL)))
						{
							iMultiplier += AI_isDoStrategy(AI_STRATEGY_ALERT2) ? 50 : 25;
							if (iPathLength <= 1 && AI_isDoStrategy(AI_STRATEGY_TURTLE))
							{
								iMultiplier += 75;
							}
						}
						iMilitaryValue *= iMultiplier;
						iMilitaryValue /= 100;
						// K-Mod end

						iUnitValue += iMilitaryValue;
					}

					if (iPathLength <= 1)
					{
						if (getTotalPopulation() > 5)
						{
							if (isWorldUnitClass((UnitClassTypes)iJ))
							{
								if (!(GC.getGameINLINE().isUnitClassMaxedOut((UnitClassTypes)iJ)))
								{
									bEnablesUnitWonder = true;
								}
							}
						}
					}
					// K-Mod
					// Decrease the value if we are missing other prereqs.
					{
						int iMissingTechs = 0;
						if (!kTeam.isHasTech((TechTypes)kLoopUnit.getPrereqAndTech()))
							iMissingTechs++;

						for (int iI = 0; iI < GC.getNUM_UNIT_AND_TECH_PREREQS(); iI++)
						{
							if (!kTeam.isHasTech((TechTypes)kLoopUnit.getPrereqAndTechs(iI)))
							{
								iMissingTechs++;
								if (!canResearch((TechTypes)kLoopUnit.getPrereqAndTechs(iI)))
									iMissingTechs++;
							}
						}
						FAssert(iMissingTechs > 0);
						iUnitValue /= std::max(1, iMissingTechs);
					}

					// Decrease the value if we don't have the resources
					bool bMaybeMissing = false; // if we don't know if we have the bonus or not
					bool bDefinitelyMissing = false; // if we can see the bonuses, and we know we don't have any.

					for (int iI = 0; iI < GC.getNUM_UNIT_PREREQ_OR_BONUSES(); ++iI)
					{
						BonusTypes ePrereqBonus = (BonusTypes)kLoopUnit.getPrereqOrBonuses(iI);
						if (ePrereqBonus != NO_BONUS)
						{
							if (hasBonus(ePrereqBonus))
							{
								bDefinitelyMissing = false;
								bMaybeMissing = false;
								break;
							}
							else
							{
								if ((kTeam.isHasTech((TechTypes)(GC.getBonusInfo(ePrereqBonus).getTechReveal())) || kTeam.isForceRevealedBonus(ePrereqBonus)) && countOwnedBonuses(ePrereqBonus) == 0)
								{
									bDefinitelyMissing = true;
								}
								else
								{
									bMaybeMissing = true;
								}
							}
						}
					}
					BonusTypes ePrereqBonus = (BonusTypes)kLoopUnit.getPrereqAndBonus();
					if (ePrereqBonus != NO_BONUS && !hasBonus(ePrereqBonus))
					{
						if ((kTeam.isHasTech((TechTypes)(GC.getBonusInfo(ePrereqBonus).getTechReveal())) || kTeam.isForceRevealedBonus(ePrereqBonus)) &&
							countOwnedBonuses(ePrereqBonus) == 0)
						{							
							bDefinitelyMissing = true;
						}
						else
						{							
							bMaybeMissing = true;
						}
					}
					if (bDefinitelyMissing)
					{
						iUnitValue /= 3;
					}
					else if (bMaybeMissing)
					{
						iUnitValue *= 2;
						iUnitValue /= 3;
					}
					// K-Mod end

					iValue += iUnitValue;
				}
			}
		}
	}

	return iValue;
}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

int CvPlayerAI::AI_cultureVictoryTechValue(TechTypes eTech) const
{
	if (eTech == NO_TECH)
	{
		return 0;
	}

	int iValue = 0;

	if (GC.getTechInfo(eTech).isDefensivePactTrading())
	{
		iValue += 50;
	}

	if (GC.getTechInfo(eTech).isCommerceFlexible(COMMERCE_CULTURE))
	{
		iValue += 100;
	}

	//units
	bool bAnyWarplan = (GET_TEAM(getTeam()).getAnyWarPlanCount(true) > 0);
	int iBestUnitValue = 0;
	for (int iI = 0; iI < GC.getNumUnitClassInfos(); iI++)
	{
		UnitTypes eLoopUnit = ((UnitTypes)(GC.getCivilizationInfo(getCivilizationType()).getCivilizationUnits(iI)));

		if (eLoopUnit != NO_UNIT)
		{
			if (isTechRequiredForUnit((eTech), eLoopUnit))
			{
				int iTempValue = (GC.getUnitInfo(eLoopUnit).getCombat() * 100) / std::max(1, (GC.getGame().getBestLandUnitCombat()));
				iTempValue *= bAnyWarplan ? 2 : 1;

				iValue += iTempValue / 3;
				iBestUnitValue = std::max(iBestUnitValue, iTempValue);
			}
		}
	}
	iValue += std::max(0, iBestUnitValue - 15);

	//cultural things
	for (int iI = 0; iI < GC.getNumBuildingClassInfos(); iI++)
	{
		BuildingTypes eLoopBuilding = ((BuildingTypes)(GC.getCivilizationInfo(getCivilizationType()).getCivilizationBuildings(iI)));

		if (eLoopBuilding != NO_BUILDING)
		{
			if (isTechRequiredForBuilding((eTech), eLoopBuilding))
			{
				CvBuildingInfo& kLoopBuilding = GC.getBuildingInfo(eLoopBuilding);

				if ((GC.getBuildingClassInfo((BuildingClassTypes)iI).getDefaultBuildingIndex()) != (GC.getCivilizationInfo(getCivilizationType()).getCivilizationBuildings(iI)))
				{
					//UB
					iValue += 100;
				}

/************************************************************************************************/
/* UNOFFICIAL_PATCH                       05/25/10                          Fuyu & jdog5000     */
/*                                                                                              */
/* Bugfix                                                                                       */
/************************************************************************************************/
/* original bts code
				iValue += (150 * kLoopBuilding.getCommerceChange(COMMERCE_CULTURE)) * 20;
*/
				iValue += (150 * (kLoopBuilding.getCommerceChange(COMMERCE_CULTURE) + kLoopBuilding.getObsoleteSafeCommerceChange(COMMERCE_CULTURE))) / 20;
/************************************************************************************************/
/* UNOFFICIAL_PATCH                        END                                                  */
/************************************************************************************************/
				iValue += kLoopBuilding.getCommerceModifier(COMMERCE_CULTURE) * 2;
			}
		}
	}

	//important civics
	for (int iI = 0; iI < GC.getNumCivicInfos(); iI++)
	{
		if (GC.getCivicInfo((CivicTypes)iI).getTechPrereq() == eTech)
		{
			iValue += GC.getCivicInfo((CivicTypes)iI).getCommerceModifier(COMMERCE_CULTURE) * 2;
		}
	}

	return iValue;
}

void CvPlayerAI::AI_chooseFreeTech()
{
	TechTypes eBestTech = NO_TECH;

	clearResearchQueue();

	if (GC.getUSE_AI_CHOOSE_TECH_CALLBACK()) // K-Mod. block unused python callbacks
	{
		CyArgsList argsList;
		long lResult;
		argsList.add(getID());
		argsList.add(true);
		lResult = -1;
		gDLL->getPythonIFace()->callFunction(PYGameModule, "AI_chooseTech", argsList.makeFunctionArgs(), &lResult);
		eBestTech = ((TechTypes)lResult);
	}

	if (eBestTech == NO_TECH)
	{
		eBestTech = AI_bestTech(1, true);
	}

	if (eBestTech != NO_TECH)
	{
		GET_TEAM(getTeam()).setHasTech(eBestTech, true, getID(), true, true);
	}
}


void CvPlayerAI::AI_chooseResearch()
{
	//TechTypes eBestTech;
	//int iI;

	clearResearchQueue();

	if (getCurrentResearch() == NO_TECH)
	{
		for (int iI = 0; iI < MAX_PLAYERS; iI++)
		{
			if (GET_PLAYER((PlayerTypes)iI).isAlive())
			{
				if ((iI != getID()) && (GET_PLAYER((PlayerTypes)iI).getTeam() == getTeam()))
				{
					if (GET_PLAYER((PlayerTypes)iI).getCurrentResearch() != NO_TECH)
					{
						if (canResearch(GET_PLAYER((PlayerTypes)iI).getCurrentResearch()))
						{
							pushResearch(GET_PLAYER((PlayerTypes)iI).getCurrentResearch());
						}
					}
				}
			}
		}
	}

	if (getCurrentResearch() == NO_TECH)
	{
		TechTypes eBestTech = NO_TECH; // K-Mod
		if (GC.getUSE_AI_CHOOSE_TECH_CALLBACK()) // K-Mod. block unused python callbacks
		{
			CyArgsList argsList;
			long lResult;
			argsList.add(getID());
			argsList.add(false);
			lResult = -1;
			gDLL->getPythonIFace()->callFunction(PYGameModule, "AI_chooseTech", argsList.makeFunctionArgs(), &lResult);
			eBestTech = ((TechTypes)lResult);
		}

		if (eBestTech == NO_TECH)
		{
			int iAIResearchDepth;
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      03/08/10                                jdog5000      */
/*                                                                                              */
/* Victory Strategy AI                                                                          */
/************************************************************************************************/
			iAIResearchDepth = AI_isDoVictoryStrategy(AI_VICTORY_CULTURE3) ? 1 : 3;
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
			
			eBestTech = AI_bestTech((isHuman()) ? 1 : iAIResearchDepth);
		}

		if (eBestTech != NO_TECH)
		{
			pushResearch(eBestTech);
		}
	}
}

DiploCommentTypes CvPlayerAI::AI_getGreeting(PlayerTypes ePlayer) const
{
	TeamTypes eWorstEnemy;

	if (GET_PLAYER(ePlayer).getTeam() != getTeam())
	{
		eWorstEnemy = GET_TEAM(getTeam()).AI_getWorstEnemy();

		if ((eWorstEnemy != NO_TEAM) && (eWorstEnemy != GET_PLAYER(ePlayer).getTeam()) && GET_TEAM(GET_PLAYER(ePlayer).getTeam()).isHasMet(eWorstEnemy) && (GC.getASyncRand().get(4) == 0))
		{
			if (GET_PLAYER(ePlayer).AI_hasTradedWithTeam(eWorstEnemy) && !atWar(GET_PLAYER(ePlayer).getTeam(), eWorstEnemy))
			{
				return (DiploCommentTypes)GC.getInfoTypeForString("AI_DIPLOCOMMENT_WORST_ENEMY_TRADING");
			}
			else
			{
				return (DiploCommentTypes)GC.getInfoTypeForString("AI_DIPLOCOMMENT_WORST_ENEMY");
			}
		}
		else if ((getNumNukeUnits() > 0) && (GC.getASyncRand().get(4) == 0))
		{
			return (DiploCommentTypes)GC.getInfoTypeForString("AI_DIPLOCOMMENT_NUKES");
		}
		else if ((GET_PLAYER(ePlayer).getPower() < getPower()) && AI_getAttitude(ePlayer) < ATTITUDE_PLEASED && (GC.getASyncRand().get(4) == 0))
		{
			return (DiploCommentTypes)GC.getInfoTypeForString("AI_DIPLOCOMMENT_UNIT_BRAG");
		}
	}

	return (DiploCommentTypes)GC.getInfoTypeForString("AI_DIPLOCOMMENT_GREETINGS");
}

// return true if we are willing to talk to ePlayer
bool CvPlayerAI::AI_isWillingToTalk(PlayerTypes ePlayer) const
{
	FAssertMsg(getPersonalityType() != NO_LEADER, "getPersonalityType() is not expected to be equal with NO_LEADER");
	FAssertMsg(ePlayer != getID(), "shouldn't call this function on ourselves");

	const CvTeamAI& kOurTeam = GET_TEAM(getTeam()); // K-Mod
	const CvTeam& kTheirTeam = GET_TEAM(GET_PLAYER(ePlayer).getTeam()); // K-Mod

	if (GET_PLAYER(ePlayer).getTeam() == getTeam()
		|| kTheirTeam.isVassal(getTeam())
		|| kOurTeam.isVassal(GET_PLAYER(ePlayer).getTeam()))
	{
		return true;
	}

	/* original bts code
	if (GET_TEAM(getTeam()).isHuman())
	{
		return false;
	} */ // disabled by K-Mod
	// K-Mod
	if (isHuman())
		return true; // humans will speak to anyone, apparently.
	// K-Mod end

	if (atWar(getTeam(), GET_PLAYER(ePlayer).getTeam()))
	{
		// K-Mod
		if (kOurTeam.isHuman()) // ie. we are an AI player, but our team is human
			return false; // let the human speak for us.

		if (kOurTeam.AI_refusePeace(GET_PLAYER(ePlayer).getTeam()))
		{
			TradeData item;
			setTradeItem(&item, TRADE_SURRENDER);

			if (!GET_PLAYER(ePlayer).canTradeItem(getID(), item, true))
				return false; // if they can't (or won't) capitulate, don't talk to them.
		}
		// K-Mod end

		if (kOurTeam.isAVassal())
		{
			return false;
		}

		int iRefuseDuration = (GC.getLeaderHeadInfo(getPersonalityType()).getRefuseToTalkWarThreshold() * ((kOurTeam.AI_isChosenWar(GET_PLAYER(ePlayer).getTeam())) ? 2 : 1));
		
		int iOurSuccess = 1 + kOurTeam.AI_getWarSuccess(GET_PLAYER(ePlayer).getTeam());
		int iTheirSuccess = 1 + kTheirTeam.AI_getWarSuccess(getTeam());
		if (iTheirSuccess > iOurSuccess * 2)
		{
			iRefuseDuration *= 20 + ((80 * iOurSuccess * 2) / iTheirSuccess);
			iRefuseDuration /= 100;
		}

		if (kOurTeam.AI_getAtWarCounter(GET_PLAYER(ePlayer).getTeam()) < iRefuseDuration)
		{
			return false;
		}
	}
	else
	{
		if (AI_getMemoryCount(ePlayer, MEMORY_STOPPED_TRADING_RECENT) > 0)
		{
			return false;
		}
	}

	return true;
}


// XXX what if already at war???
// Returns true if the AI wants to sneak attack...
bool CvPlayerAI::AI_demandRebukedSneak(PlayerTypes ePlayer) const
{
	FAssertMsg(!isHuman(), "isHuman did not return false as expected");
	FAssertMsg(ePlayer != getID(), "shouldn't call this function on ourselves");

	FAssert(!(GET_TEAM(getTeam()).isAVassal()));
	FAssert(!(GET_TEAM(getTeam()).isHuman()));

	if (GC.getGameINLINE().getSorenRandNum(100, "AI Demand Rebuked") < GC.getLeaderHeadInfo(getPersonalityType()).getDemandRebukedSneakProb())
	{
		if (GET_TEAM(getTeam()).getPower(true) > GET_TEAM(GET_PLAYER(ePlayer).getTeam()).getDefensivePower(getTeam()))
		{
			return true;
		}
	}

	return false;
}


// XXX what if already at war???
// Returns true if the AI wants to declare war...
bool CvPlayerAI::AI_demandRebukedWar(PlayerTypes ePlayer) const
{
	FAssertMsg(!isHuman(), "isHuman did not return false as expected");
	FAssertMsg(ePlayer != getID(), "shouldn't call this function on ourselves");

	FAssert(!(GET_TEAM(getTeam()).isAVassal()));
	FAssert(!(GET_TEAM(getTeam()).isHuman()));

	// needs to be async because it only happens on the computer of the player who is in diplomacy...
	if (GC.getASyncRand().get(100, "AI Demand Rebuked ASYNC") < GC.getLeaderHeadInfo(getPersonalityType()).getDemandRebukedWarProb())
	{
		if (GET_TEAM(getTeam()).getPower(true) > GET_TEAM(GET_PLAYER(ePlayer).getTeam()).getDefensivePower(getTeam()))
		{
			if (GET_TEAM(getTeam()).AI_isAllyLandTarget(GET_PLAYER(ePlayer).getTeam()))
			{
				return true;
			}
		}
	}

	return false;
}


// XXX maybe make this a little looser (by time...)
bool CvPlayerAI::AI_hasTradedWithTeam(TeamTypes eTeam) const
{
	int iI;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == eTeam)
			{
				if ((AI_getPeacetimeGrantValue((PlayerTypes)iI) + AI_getPeacetimeTradeValue((PlayerTypes)iI)) > 0)
				{
					return true;
				}
			}
		}
	}

	return false;
}

// static
AttitudeTypes CvPlayerAI::AI_getAttitudeFromValue(int iAttitudeVal)
{
	if (iAttitudeVal >= 10)
	{
		return ATTITUDE_FRIENDLY;
	}
	else if (iAttitudeVal >= 3)
	{
		return ATTITUDE_PLEASED;
	}
	else if (iAttitudeVal <= -10)
	{
		return ATTITUDE_FURIOUS;
	}
	else if (iAttitudeVal <= -3)
	{
		return ATTITUDE_ANNOYED;
	}
	else
	{
		return ATTITUDE_CAUTIOUS;
	}
}

AttitudeTypes CvPlayerAI::AI_getAttitude(PlayerTypes ePlayer, bool bForced) const
{
	PROFILE_FUNC();

	FAssertMsg(ePlayer != getID(), "shouldn't call this function on ourselves");

	return (AI_getAttitudeFromValue(AI_getAttitudeVal(ePlayer, bForced)));
}


int CvPlayerAI::AI_getAttitudeVal(PlayerTypes ePlayer, bool bForced) const
{
	PROFILE_FUNC();

	int iRankDifference;
	int iAttitude;
	int iI;

	FAssertMsg(ePlayer != getID(), "shouldn't call this function on ourselves");

	if (bForced)
	{
		if (getTeam() == GET_PLAYER(ePlayer).getTeam() || (GET_TEAM(getTeam()).isVassal(GET_PLAYER(ePlayer).getTeam()) && !GET_TEAM(getTeam()).isCapitulated()))
		{
			return 100;
		}

		if (isBarbarian() || GET_PLAYER(ePlayer).isBarbarian())
		{
			return -100;
		}
	}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      09/03/09                       poyuzhe & jdog5000     */
/*                                                                                              */
/* Efficiency                                                                                   */
/************************************************************************************************/
	// From Sanguo Mod Performance, ie the CAR Mod
	// Attitude cache
	if (m_aiAttitudeCache[ePlayer] != MAX_INT)
	{
		return m_aiAttitudeCache[ePlayer];
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	iAttitude = GC.getLeaderHeadInfo(getPersonalityType()).getBaseAttitude();

	iAttitude += GC.getHandicapInfo(GET_PLAYER(ePlayer).getHandicapType()).getAttitudeChange();

//	if (GC.getGameINLINE().isOption(GAMEOPTION_AGGRESSIVE_AI))
//	{
//		if (GET_PLAYER(ePlayer).isHuman())
//		{
//			iAttitude -= 2;
//		}
//	}

	if (!(GET_PLAYER(ePlayer).isHuman()))
	{
		iAttitude += (4 - abs(AI_getPeaceWeight() - GET_PLAYER(ePlayer).AI_getPeaceWeight()));
		iAttitude += std::min(GC.getLeaderHeadInfo(getPersonalityType()).getWarmongerRespect(), GC.getLeaderHeadInfo(GET_PLAYER(ePlayer).getPersonalityType()).getWarmongerRespect());
	}

	iAttitude -= std::max(0, (GET_TEAM(GET_PLAYER(ePlayer).getTeam()).getNumMembers() - GET_TEAM(getTeam()).getNumMembers()));

	iRankDifference = (GC.getGameINLINE().getPlayerRank(getID()) - GC.getGameINLINE().getPlayerRank(ePlayer));

	if (iRankDifference > 0)
	{
		iAttitude += ((GC.getLeaderHeadInfo(getPersonalityType()).getWorseRankDifferenceAttitudeChange() * iRankDifference) / (GC.getGameINLINE().countCivPlayersEverAlive() + 1));
	}
	else
	{
		iAttitude += ((GC.getLeaderHeadInfo(getPersonalityType()).getBetterRankDifferenceAttitudeChange() * -(iRankDifference)) / (GC.getGameINLINE().countCivPlayersEverAlive() + 1));
	}

	if ((GC.getGameINLINE().getPlayerRank(getID()) >= (GC.getGameINLINE().countCivPlayersEverAlive() / 2)) &&
		  (GC.getGameINLINE().getPlayerRank(ePlayer) >= (GC.getGameINLINE().countCivPlayersEverAlive() / 2)))
	{
		iAttitude++;
	}

	if (GET_TEAM(GET_PLAYER(ePlayer).getTeam()).AI_getWarSuccess(getTeam()) > GET_TEAM(getTeam()).AI_getWarSuccess(GET_PLAYER(ePlayer).getTeam()))
	{
		iAttitude += GC.getLeaderHeadInfo(getPersonalityType()).getLostWarAttitudeChange();
	}

	iAttitude += AI_getCloseBordersAttitude(ePlayer);
	iAttitude += AI_getWarAttitude(ePlayer);
	iAttitude += AI_getPeaceAttitude(ePlayer);
	iAttitude += AI_getSameReligionAttitude(ePlayer);
	iAttitude += AI_getDifferentReligionAttitude(ePlayer);
	iAttitude += AI_getBonusTradeAttitude(ePlayer);
	iAttitude += AI_getOpenBordersAttitude(ePlayer);
	iAttitude += AI_getDefensivePactAttitude(ePlayer);
	iAttitude += AI_getRivalDefensivePactAttitude(ePlayer);
	iAttitude += AI_getRivalVassalAttitude(ePlayer);
	iAttitude += AI_getShareWarAttitude(ePlayer);
	iAttitude += AI_getFavoriteCivicAttitude(ePlayer);
	iAttitude += AI_getTradeAttitude(ePlayer);
	iAttitude += AI_getRivalTradeAttitude(ePlayer);

	for (iI = 0; iI < NUM_MEMORY_TYPES; iI++)
	{
		iAttitude += AI_getMemoryAttitude(ePlayer, ((MemoryTypes)iI));
	}

	iAttitude += AI_getColonyAttitude(ePlayer);
	iAttitude += AI_getAttitudeExtra(ePlayer);

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      09/03/09                       poyuzhe & jdog5000     */
/*                                                                                              */
/* Efficiency                                                                                   */
/************************************************************************************************/
	// From Sanguo Mod Performance, ie the CAR Mod
	// Attitude cache
	m_aiAttitudeCache[ePlayer] = range(iAttitude, -100, 100);
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	return range(iAttitude, -100, 100);
}


int CvPlayerAI::AI_calculateStolenCityRadiusPlots(PlayerTypes ePlayer) const
{
	PROFILE_FUNC();

	CvPlot* pLoopPlot;
	int iCount;
	int iI;

	FAssert(ePlayer != getID());

	iCount = 0;

	for (iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
	{
		pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);

		if (pLoopPlot->getOwnerINLINE() == ePlayer)
		{
			if (pLoopPlot->isPlayerCityRadius(getID()))
			{
				iCount++;
			}
		}
	}

	return iCount;
}


int CvPlayerAI::AI_getCloseBordersAttitude(PlayerTypes ePlayer) const
{
	if (m_aiCloseBordersAttitudeCache[ePlayer] == MAX_INT)
	{
		PROFILE_FUNC();
		int iPercent;

		if (getTeam() == GET_PLAYER(ePlayer).getTeam() || GET_TEAM(getTeam()).isVassal(GET_PLAYER(ePlayer).getTeam()) || GET_TEAM(GET_PLAYER(ePlayer).getTeam()).isVassal(getTeam()))
		{
			return 0;
		}

		iPercent = std::min(60, (AI_calculateStolenCityRadiusPlots(ePlayer) * 3));

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      06/12/10                                jdog5000      */
/*                                                                                              */
/* Bugfix, Victory Strategy AI                                                                  */
/************************************************************************************************/
		if (GET_TEAM(getTeam()).AI_isLandTarget(GET_PLAYER(ePlayer).getTeam(), true))
		{
			iPercent += 40;
		}

		if( AI_isDoStrategy(AI_VICTORY_CONQUEST3) )
		{
			iPercent = std::min( 120, (3 * iPercent)/2 );
		}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

		m_aiCloseBordersAttitudeCache[ePlayer] = ((GC.getLeaderHeadInfo(getPersonalityType()).getCloseBordersAttitudeChange() * iPercent) / 100);
	}

	return m_aiCloseBordersAttitudeCache[ePlayer];
}


int CvPlayerAI::AI_getWarAttitude(PlayerTypes ePlayer) const
{
	int iAttitudeChange;
	int iAttitude;

	iAttitude = 0;

	if (atWar(getTeam(), GET_PLAYER(ePlayer).getTeam()))
	{
		iAttitude -= 3;
	}

	if (GC.getLeaderHeadInfo(getPersonalityType()).getAtWarAttitudeDivisor() != 0)
	{
		iAttitudeChange = (GET_TEAM(getTeam()).AI_getAtWarCounter(GET_PLAYER(ePlayer).getTeam()) / GC.getLeaderHeadInfo(getPersonalityType()).getAtWarAttitudeDivisor());
		iAttitude += range(iAttitudeChange, -(abs(GC.getLeaderHeadInfo(getPersonalityType()).getAtWarAttitudeChangeLimit())), abs(GC.getLeaderHeadInfo(getPersonalityType()).getAtWarAttitudeChangeLimit()));
	}

	return iAttitude;
}


int CvPlayerAI::AI_getPeaceAttitude(PlayerTypes ePlayer) const
{
	int iAttitudeChange;

	if (GC.getLeaderHeadInfo(getPersonalityType()).getAtPeaceAttitudeDivisor() != 0)
	{
		iAttitudeChange = (GET_TEAM(getTeam()).AI_getAtPeaceCounter(GET_PLAYER(ePlayer).getTeam()) / GC.getLeaderHeadInfo(getPersonalityType()).getAtPeaceAttitudeDivisor());
		return range(iAttitudeChange, -(abs(GC.getLeaderHeadInfo(getPersonalityType()).getAtPeaceAttitudeChangeLimit())), abs(GC.getLeaderHeadInfo(getPersonalityType()).getAtPeaceAttitudeChangeLimit()));
	}

	return 0;
}


int CvPlayerAI::AI_getSameReligionAttitude(PlayerTypes ePlayer) const
{
	int iAttitudeChange;
	int iAttitude;

	iAttitude = 0;

	if ((getStateReligion() != NO_RELIGION) && (getStateReligion() == GET_PLAYER(ePlayer).getStateReligion()))
	{
		iAttitude += GC.getLeaderHeadInfo(getPersonalityType()).getSameReligionAttitudeChange();

		if (hasHolyCity(getStateReligion()))
		{
			iAttitude++;
		}

		if (GC.getLeaderHeadInfo(getPersonalityType()).getSameReligionAttitudeDivisor() != 0)
		{
			iAttitudeChange = (AI_getSameReligionCounter(ePlayer) / GC.getLeaderHeadInfo(getPersonalityType()).getSameReligionAttitudeDivisor());
			iAttitude += range(iAttitudeChange, -(abs(GC.getLeaderHeadInfo(getPersonalityType()).getSameReligionAttitudeChangeLimit())), abs(GC.getLeaderHeadInfo(getPersonalityType()).getSameReligionAttitudeChangeLimit()));
		}
	}

	return iAttitude;
}


int CvPlayerAI::AI_getDifferentReligionAttitude(PlayerTypes ePlayer) const
{
	int iAttitudeChange;
	int iAttitude;

	iAttitude = 0;

	if ((getStateReligion() != NO_RELIGION) && (GET_PLAYER(ePlayer).getStateReligion() != NO_RELIGION) && (getStateReligion() != GET_PLAYER(ePlayer).getStateReligion()))
	{
		iAttitude += GC.getLeaderHeadInfo(getPersonalityType()).getDifferentReligionAttitudeChange();

		if (hasHolyCity(getStateReligion()))
		{
			iAttitude--;
		}

		if (GC.getLeaderHeadInfo(getPersonalityType()).getDifferentReligionAttitudeDivisor() != 0)
		{
			iAttitudeChange = (AI_getDifferentReligionCounter(ePlayer) / GC.getLeaderHeadInfo(getPersonalityType()).getDifferentReligionAttitudeDivisor());
			iAttitude += range(iAttitudeChange, -(abs(GC.getLeaderHeadInfo(getPersonalityType()).getDifferentReligionAttitudeChangeLimit())), abs(GC.getLeaderHeadInfo(getPersonalityType()).getDifferentReligionAttitudeChangeLimit()));
		}
	}

	return iAttitude;
}


int CvPlayerAI::AI_getBonusTradeAttitude(PlayerTypes ePlayer) const
{
	int iAttitudeChange;

	if (!atWar(getTeam(), GET_PLAYER(ePlayer).getTeam()))
	{
		if (GC.getLeaderHeadInfo(getPersonalityType()).getBonusTradeAttitudeDivisor() != 0)
		{
			iAttitudeChange = (AI_getBonusTradeCounter(ePlayer) / GC.getLeaderHeadInfo(getPersonalityType()).getBonusTradeAttitudeDivisor());
			return range(iAttitudeChange, -(abs(GC.getLeaderHeadInfo(getPersonalityType()).getBonusTradeAttitudeChangeLimit())), abs(GC.getLeaderHeadInfo(getPersonalityType()).getBonusTradeAttitudeChangeLimit()));
		}
	}

	return 0;
}


int CvPlayerAI::AI_getOpenBordersAttitude(PlayerTypes ePlayer) const
{
	int iAttitudeChange;

	if (!atWar(getTeam(), GET_PLAYER(ePlayer).getTeam()))
	{
		if (GC.getLeaderHeadInfo(getPersonalityType()).getOpenBordersAttitudeDivisor() != 0)
		{
			iAttitudeChange = (GET_TEAM(getTeam()).AI_getOpenBordersCounter(GET_PLAYER(ePlayer).getTeam()) / GC.getLeaderHeadInfo(getPersonalityType()).getOpenBordersAttitudeDivisor());
			return range(iAttitudeChange, -(abs(GC.getLeaderHeadInfo(getPersonalityType()).getOpenBordersAttitudeChangeLimit())), abs(GC.getLeaderHeadInfo(getPersonalityType()).getOpenBordersAttitudeChangeLimit()));
		}
	}

	return 0;
}


int CvPlayerAI::AI_getDefensivePactAttitude(PlayerTypes ePlayer) const
{
	int iAttitudeChange;

	if (getTeam() != GET_PLAYER(ePlayer).getTeam() && (GET_TEAM(getTeam()).isVassal(GET_PLAYER(ePlayer).getTeam()) || GET_TEAM(GET_PLAYER(ePlayer).getTeam()).isVassal(getTeam())))
	{
		return GC.getLeaderHeadInfo(getPersonalityType()).getDefensivePactAttitudeChangeLimit();
	}

	if (!atWar(getTeam(), GET_PLAYER(ePlayer).getTeam()))
	{
		if (GC.getLeaderHeadInfo(getPersonalityType()).getDefensivePactAttitudeDivisor() != 0)
		{
			iAttitudeChange = (GET_TEAM(getTeam()).AI_getDefensivePactCounter(GET_PLAYER(ePlayer).getTeam()) / GC.getLeaderHeadInfo(getPersonalityType()).getDefensivePactAttitudeDivisor());
			return range(iAttitudeChange, -(abs(GC.getLeaderHeadInfo(getPersonalityType()).getDefensivePactAttitudeChangeLimit())), abs(GC.getLeaderHeadInfo(getPersonalityType()).getDefensivePactAttitudeChangeLimit()));
		}
	}

	return 0;
}


int CvPlayerAI::AI_getRivalDefensivePactAttitude(PlayerTypes ePlayer) const
{
	int iAttitude = 0;

	if (getTeam() == GET_PLAYER(ePlayer).getTeam() || GET_TEAM(getTeam()).isVassal(GET_PLAYER(ePlayer).getTeam()) || GET_TEAM(GET_PLAYER(ePlayer).getTeam()).isVassal(getTeam()))
	{
		return iAttitude;
	}

	if (!(GET_TEAM(getTeam()).isDefensivePact(GET_PLAYER(ePlayer).getTeam())))
	{
		iAttitude -= ((4 * GET_TEAM(GET_PLAYER(ePlayer).getTeam()).getDefensivePactCount(GET_PLAYER(ePlayer).getTeam())) / std::max(1, (GC.getGameINLINE().countCivTeamsAlive() - 2)));
	}

	return iAttitude;
}


int CvPlayerAI::AI_getRivalVassalAttitude(PlayerTypes ePlayer) const
{
	int iAttitude = 0;

	if (getTeam() == GET_PLAYER(ePlayer).getTeam() || GET_TEAM(getTeam()).isVassal(GET_PLAYER(ePlayer).getTeam()) || GET_TEAM(GET_PLAYER(ePlayer).getTeam()).isVassal(getTeam()))
	{
		return iAttitude;
	}

	if (GET_TEAM(GET_PLAYER(ePlayer).getTeam()).getVassalCount(getTeam()) > 0)
	{
		iAttitude -= (6 * GET_TEAM(GET_PLAYER(ePlayer).getTeam()).getPower(true)) / std::max(1, GC.getGameINLINE().countTotalCivPower());
	}

	return iAttitude;
}


int CvPlayerAI::AI_getShareWarAttitude(PlayerTypes ePlayer) const
{
	int iAttitudeChange;
	int iAttitude;

	iAttitude = 0;

	if (!atWar(getTeam(), GET_PLAYER(ePlayer).getTeam()))
	{
		if (GET_TEAM(getTeam()).AI_shareWar(GET_PLAYER(ePlayer).getTeam()))
		{
			iAttitude += GC.getLeaderHeadInfo(getPersonalityType()).getShareWarAttitudeChange();
		}

		if (GC.getLeaderHeadInfo(getPersonalityType()).getShareWarAttitudeDivisor() != 0)
		{
			iAttitudeChange = (GET_TEAM(getTeam()).AI_getShareWarCounter(GET_PLAYER(ePlayer).getTeam()) / GC.getLeaderHeadInfo(getPersonalityType()).getShareWarAttitudeDivisor());
			iAttitude += range(iAttitudeChange, -(abs(GC.getLeaderHeadInfo(getPersonalityType()).getShareWarAttitudeChangeLimit())), abs(GC.getLeaderHeadInfo(getPersonalityType()).getShareWarAttitudeChangeLimit()));
		}
	}

	return iAttitude;
}


int CvPlayerAI::AI_getFavoriteCivicAttitude(PlayerTypes ePlayer) const
{
	int iAttitudeChange;

	int iAttitude;

	iAttitude = 0;

	if (GC.getLeaderHeadInfo(getPersonalityType()).getFavoriteCivic() != NO_CIVIC)
	{
		if (isCivic((CivicTypes)(GC.getLeaderHeadInfo(getPersonalityType()).getFavoriteCivic())) && GET_PLAYER(ePlayer).isCivic((CivicTypes)(GC.getLeaderHeadInfo(getPersonalityType()).getFavoriteCivic())))
		{
			iAttitude += GC.getLeaderHeadInfo(getPersonalityType()).getFavoriteCivicAttitudeChange();

			if (GC.getLeaderHeadInfo(getPersonalityType()).getFavoriteCivicAttitudeDivisor() != 0)
			{
				iAttitudeChange = (AI_getFavoriteCivicCounter(ePlayer) / GC.getLeaderHeadInfo(getPersonalityType()).getFavoriteCivicAttitudeDivisor());
				iAttitude += range(iAttitudeChange, -(abs(GC.getLeaderHeadInfo(getPersonalityType()).getFavoriteCivicAttitudeChangeLimit())), abs(GC.getLeaderHeadInfo(getPersonalityType()).getFavoriteCivicAttitudeChangeLimit()));
			}
		}
	}

	return iAttitude;
}


int CvPlayerAI::AI_getTradeAttitude(PlayerTypes ePlayer) const
{
	// XXX human only?
	return range(((AI_getPeacetimeGrantValue(ePlayer) + std::max(0, (AI_getPeacetimeTradeValue(ePlayer) - GET_PLAYER(ePlayer).AI_getPeacetimeTradeValue(getID())))) / ((GET_TEAM(getTeam()).AI_getHasMetCounter(GET_PLAYER(ePlayer).getTeam()) + 1) * 5)), 0, 4);
}


int CvPlayerAI::AI_getRivalTradeAttitude(PlayerTypes ePlayer) const
{
	// XXX human only?
	return -(range(((GET_TEAM(getTeam()).AI_getEnemyPeacetimeGrantValue(GET_PLAYER(ePlayer).getTeam()) + (GET_TEAM(getTeam()).AI_getEnemyPeacetimeTradeValue(GET_PLAYER(ePlayer).getTeam()) / 3)) / ((GET_TEAM(getTeam()).AI_getHasMetCounter(GET_PLAYER(ePlayer).getTeam()) + 1) * 10)), 0, 4));
}


int CvPlayerAI::AI_getMemoryAttitude(PlayerTypes ePlayer, MemoryTypes eMemory) const
{
	return ((AI_getMemoryCount(ePlayer, eMemory) * GC.getLeaderHeadInfo(getPersonalityType()).getMemoryAttitudePercent(eMemory)) / 100);
}

int CvPlayerAI::AI_getColonyAttitude(PlayerTypes ePlayer) const
{
	int iAttitude = 0;

	if (getParent() == ePlayer)
	{
		iAttitude += GC.getLeaderHeadInfo(getPersonalityType()).getFreedomAppreciation();
	}

	return iAttitude;
}



PlayerVoteTypes CvPlayerAI::AI_diploVote(const VoteSelectionSubData& kVoteData, VoteSourceTypes eVoteSource, bool bPropose)
{
	PROFILE_FUNC();

	CivicTypes eBestCivic;
	int iOpenCount;
	int iClosedCount;
	int iValue;
	int iBestValue;
	int iI;

	VoteTypes eVote = kVoteData.eVote;

	if (GC.getGameINLINE().isTeamVote(eVote))
	{
		if (GC.getGameINLINE().isTeamVoteEligible(getTeam(), eVoteSource))
		{
			return (PlayerVoteTypes)getTeam();
		}

		if (GC.getVoteInfo(eVote).isVictory())
		{
			iBestValue = 7;
		}
		else
		{
			iBestValue = 0;
		}

		PlayerVoteTypes eBestTeam = PLAYER_VOTE_ABSTAIN;

		for (iI = 0; iI < MAX_TEAMS; iI++)
		{
			if (GET_TEAM((TeamTypes)iI).isAlive())
			{
				if (GC.getGameINLINE().isTeamVoteEligible((TeamTypes)iI, eVoteSource))
				{
					if (GET_TEAM(getTeam()).isVassal((TeamTypes)iI))
					{
						return (PlayerVoteTypes)iI;
					}

					iValue = GET_TEAM(getTeam()).AI_getAttitudeVal((TeamTypes)iI);

					if (iValue > iBestValue)
					{
						iBestValue = iValue;
						eBestTeam = (PlayerVoteTypes)iI;
					}
				}
			}
		}

		return eBestTeam;
	}
	else
	{
		TeamTypes eSecretaryGeneral = GC.getGameINLINE().getSecretaryGeneral(eVoteSource);

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      12/30/08                                jdog5000      */
/*                                                                                              */
/* Diplomacy AI                                                                                 */
/************************************************************************************************/
/* original BTS code
		if (!bPropose)
		{
			if (eSecretaryGeneral != NO_TEAM)
			{
				if (eSecretaryGeneral == getTeam() ||(GET_TEAM(getTeam()).AI_getAttitude(eSecretaryGeneral) == ATTITUDE_FRIENDLY))
				{
					return PLAYER_VOTE_YES;
				}
			}
		}						
*/
		// Remove blanket auto approval for friendly secretary
		bool bFriendlyToSecretary = false;
		if (!bPropose)
		{
			if (eSecretaryGeneral != NO_TEAM)
			{
				if (eSecretaryGeneral == getTeam())
				{
					return PLAYER_VOTE_YES;
				}
				else
				{
					bFriendlyToSecretary = (GET_TEAM(getTeam()).AI_getAttitude(eSecretaryGeneral) == ATTITUDE_FRIENDLY);
				}
			}
		}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
		
		bool bDefy = false;

		bool bValid = true;

		if (bValid)
		{
			for (iI = 0; iI < GC.getNumCivicInfos(); iI++)
			{
				if (GC.getVoteInfo(eVote).isForceCivic(iI))
				{
					if (!isCivic((CivicTypes)iI))
					{
						eBestCivic = AI_bestCivic((CivicOptionTypes)(GC.getCivicInfo((CivicTypes)iI).getCivicOptionType()));

						if (eBestCivic != NO_CIVIC)
						{
							if (eBestCivic != ((CivicTypes)iI))
							{
								int iBestCivicValue = AI_civicValue(eBestCivic);
								int iNewCivicValue = AI_civicValue((CivicTypes)iI);
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      12/30/08                                jdog5000      */
/*                                                                                              */
/* Diplomacy AI                                                                                 */
/************************************************************************************************/
/* original BTS code
								if (iBestCivicValue > ((iNewCivicValue * 120) / 100))
								{
									bValid = false;
									if (iBestCivicValue > ((iNewCivicValue * (140 + (GC.getGame().getSorenRandNum(120, "AI Erratic Defiance (Force Civic)"))) / 100)))
*/
								// Increase threshold of voting for friend's proposal
								if( bFriendlyToSecretary )
								{
									iNewCivicValue *= 6;
									iNewCivicValue /= 5;
								}

								if (iBestCivicValue > ((iNewCivicValue * 120) / 100))
								{
									bValid = false;

									// Increase odds of defiance, particularly on AggressiveAI
									if (iBestCivicValue > ((iNewCivicValue * (140 + (GC.getGame().getSorenRandNum((GC.getGameINLINE().isOption(GAMEOPTION_AGGRESSIVE_AI) ? 60 : 80), "AI Erratic Defiance (Force Civic)"))) / 100)))
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
									{
										bDefy = true;										
									}
									break;
								}
							}
						}
					}
				}
			}
		}

		if (bValid)
		{
			if (GC.getVoteInfo(eVote).getTradeRoutes() > 0)
			{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      12/30/08                                jdog5000      */
/*                                                                                              */
/* Diplomacy AI                                                                                 */
/************************************************************************************************/
				if( bFriendlyToSecretary )
				{
					return PLAYER_VOTE_YES;
				}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
				if (getNumCities() > ((GC.getGameINLINE().getNumCities() * 2) / (GC.getGameINLINE().countCivPlayersAlive() + 1)))
				{
					bValid = false;
				}
			}
		}

		if (bValid)
		{
			if (GC.getVoteInfo(eVote).isNoNukes())
			{
				int iVoteBanThreshold = 0;
				iVoteBanThreshold += GET_TEAM(getTeam()).getNukeInterception() / 3;
				iVoteBanThreshold += GC.getLeaderHeadInfo(getPersonalityType()).getBuildUnitProb();
				iVoteBanThreshold *= std::max(1, GC.getLeaderHeadInfo(getPersonalityType()).getWarmongerRespect());
				if (GC.getGameINLINE().isOption(GAMEOPTION_AGGRESSIVE_AI))
				{
					iVoteBanThreshold *= 2;
				}

				bool bAnyHasSdi = false;
				for (iI = 0; iI < MAX_TEAMS; iI++)
				{
					if (GET_TEAM((TeamTypes)iI).isAlive() && iI != getTeam())
					{
						if (GET_TEAM((TeamTypes)iI).getNukeInterception() > 0)
						{
							bAnyHasSdi = true;
							break;
						}
					}
				}

				if (!bAnyHasSdi && GET_TEAM(getTeam()).getNukeInterception() > 0 && GET_TEAM(getTeam()).getNumNukeUnits() > 0)
				{
					iVoteBanThreshold *= 2;
				}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      12/30/08                                jdog5000      */
/*                                                                                              */
/* Diplomacy AI                                                                                 */
/************************************************************************************************/
				if( bFriendlyToSecretary )
				{
					iVoteBanThreshold *= 2;
					iVoteBanThreshold /= 3;
				}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

				bValid = (GC.getGameINLINE().getSorenRandNum(100, "AI nuke ban vote") > iVoteBanThreshold);
				
				if (AI_isDoStrategy(AI_STRATEGY_OWABWNW))
				{
					bValid = false;
				}
				else if ((GET_TEAM(getTeam()).getNumNukeUnits() / std::max(1, GET_TEAM(getTeam()).getNumMembers())) < (GC.getGameINLINE().countTotalNukeUnits() / std::max(1, GC.getGameINLINE().countCivPlayersAlive())))
				{
					bValid = false;
				}
				if (!bValid && AI_getNumTrainAIUnits(UNITAI_ICBM) > 0)
				{
					if (GC.getGame().getSorenRandNum(AI_isDoStrategy(AI_STRATEGY_OWABWNW) ? 2 : 3, "AI Erratic Defiance (No Nukes)") == 0)
					{
						bDefy = true;
					}
				}
			}
		}

		if (bValid)
		{
			if (GC.getVoteInfo(eVote).isFreeTrade())
			{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      12/30/08                                jdog5000      */
/*                                                                                              */
/* Diplomacy AI                                                                                 */
/************************************************************************************************/
				if( bFriendlyToSecretary )
				{
					return PLAYER_VOTE_YES;
				}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

				iOpenCount = 0;
				iClosedCount = 0;

				for (iI = 0; iI < MAX_TEAMS; iI++)
				{
					if (GET_TEAM((TeamTypes)iI).isAlive())
					{
						if (iI != getTeam())
						{
							if (GET_TEAM(getTeam()).isOpenBorders((TeamTypes)iI))
							{
								iOpenCount += GET_TEAM((TeamTypes)iI).getNumCities();
							}
							else
							{
								iClosedCount += GET_TEAM((TeamTypes)iI).getNumCities();
							}
						}
					}
				}

				if (iOpenCount >= (getNumCities() * getTradeRoutes()))
				{
					bValid = false;
				}

				if (iClosedCount == 0)
				{
					bValid = false;
				}
			}
		}

		if (bValid)
		{
			if (GC.getVoteInfo(eVote).isOpenBorders())
			{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      12/30/08                                jdog5000      */
/*                                                                                              */
/* Diplomacy AI                                                                                 */
/************************************************************************************************/
				if( bFriendlyToSecretary )
				{
					return PLAYER_VOTE_YES;
				}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

				bValid = true;

				for (iI = 0; iI < MAX_CIV_TEAMS; iI++)
				{
					if (iI != getTeam())
					{
						if (GET_TEAM((TeamTypes)iI).isVotingMember(eVoteSource))
						{
							if (NO_DENIAL != GET_TEAM(getTeam()).AI_openBordersTrade((TeamTypes)iI))
							{
								bValid = false;
								break;
							}
						}
					}
				}
			}
			else if (GC.getVoteInfo(eVote).isDefensivePact())
			{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      12/30/08                                jdog5000      */
/*                                                                                              */
/* Diplomacy AI                                                                                 */
/************************************************************************************************/
				if( bFriendlyToSecretary )
				{
					return PLAYER_VOTE_YES;
				}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

				bValid = true;

				for (iI = 0; iI < MAX_CIV_TEAMS; iI++)
				{
					if (iI != getTeam())
					{
						if (GET_TEAM((TeamTypes)iI).isVotingMember(eVoteSource))
						{
							if (NO_DENIAL != GET_TEAM(getTeam()).AI_defensivePactTrade((TeamTypes)iI))
							{
								bValid = false;
								break;
							}
						}
					}
				}
			}
			else if (GC.getVoteInfo(eVote).isForcePeace())
			{
				FAssert(kVoteData.ePlayer != NO_PLAYER);
				TeamTypes ePeaceTeam = GET_PLAYER(kVoteData.ePlayer).getTeam();
				
				int iWarsWinning = 0;
				int iWarsLosing = 0;
				int iChosenWar = 0;
// K-Mod version of end-war vote decision.
// Note: this code is based on BBAI code. I'm not sure which bits are from BBAI and which are from BtS.
// So I haven't marked individual changes or preserved the original code. The way "winning" and "losing" are calculated are mine.
				bool bLosingBig = false;
				bool bWinningBig = false;
				bool bThisPlayerWinning = false;

				int iSuccessScale = GET_PLAYER(kVoteData.ePlayer).getNumMilitaryUnits() * GC.getDefineINT("WAR_SUCCESS_ATTACKING") / 5;

				bool bAggressiveAI = GC.getGameINLINE().isOption(GAMEOPTION_AGGRESSIVE_AI);
				if (bAggressiveAI)
				{
					iSuccessScale *= 3;
					iSuccessScale /= 2;
				}

				// Is ePeaceTeam winning wars?
				int iPeaceTeamPower = GET_TEAM(ePeaceTeam).getPower(true);

				for (iI = 0; iI < MAX_CIV_TEAMS; iI++)
				{
					if (GET_TEAM((TeamTypes)iI).isAlive())
					{
						if (iI != ePeaceTeam)
						{
							if (!GET_TEAM((TeamTypes)iI).isAVassal() && GET_TEAM((TeamTypes)iI).isAtWar(ePeaceTeam))
							{
								int iPeaceTeamSuccess = GET_TEAM(ePeaceTeam).AI_getWarSuccess((TeamTypes)iI);
								int iOtherTeamSuccess = GET_TEAM((TeamTypes)iI).AI_getWarSuccess(ePeaceTeam);

								int iOtherTeamPower = GET_TEAM((TeamTypes)iI).getPower(true);

								if (iPeaceTeamSuccess * iPeaceTeamPower > (iOtherTeamSuccess + iSuccessScale) * iOtherTeamPower)
								{
									// Have to be ahead by at least a few victories to count as win
									++iWarsWinning;

									if (iPeaceTeamSuccess * iPeaceTeamPower / std::max(1, (iOtherTeamSuccess + 2*iSuccessScale) * iOtherTeamPower) >= 2)
									{
										bWinningBig = true;
									}
								}
								else if (iOtherTeamSuccess * iOtherTeamPower > (iPeaceTeamSuccess + iSuccessScale) * iPeaceTeamPower)
								{
									// Have to have non-trivial loses
									++iWarsLosing;

									if (iOtherTeamSuccess * iOtherTeamPower / std::max(1, (iPeaceTeamSuccess + 2*iSuccessScale) * iPeaceTeamPower) >= 2)
									{
										bLosingBig = true;
									}						

									if (iI == getTeam())
									{
										bThisPlayerWinning = true;
									}
								}
								else if (GET_TEAM(ePeaceTeam).AI_getAtWarCounter((TeamTypes)iI) < 10)
								{
									// Not winning, just recently attacked, and in multiple wars, be pessimistic
									// Counts ties from no actual battles
									if ((GET_TEAM(ePeaceTeam).getAtWarCount(true) > 1) && !(GET_TEAM(ePeaceTeam).AI_isChosenWar((TeamTypes)iI)))
									{
										++iWarsLosing;
									}
								}

								if (GET_TEAM(ePeaceTeam).AI_isChosenWar((TeamTypes)iI))
								{
									++iChosenWar;									
								}
							}
						}
					}
				}
				
				if (ePeaceTeam == getTeam())
				{
					int iPeaceRand = GC.getLeaderHeadInfo(getPersonalityType()).getBasePeaceWeight();
					// Note, base peace weight ranges between 0 and 10.
					iPeaceRand /= (bAggressiveAI ? 2 : 1);
					iPeaceRand /= (AI_isDoVictoryStrategy(AI_VICTORY_CONQUEST2) ? 2 : 1);
					
					if( bLosingBig && (GC.getGame().getSorenRandNum(iPeaceRand, "AI Force Peace to avoid loss") || bPropose) )
					{
						// Non-warmongers want peace to escape loss
						bValid = true;
					}
					//else if ( !bLosingBig && (iChosenWar > iWarsLosing) )
					else if ( !bLosingBig && (iChosenWar > iWarsLosing || AI_isDoVictoryStrategy(AI_VICTORY_CONQUEST3))) // K-Mod
					{
						// If chosen to be in most wars, keep it going
						bValid = false;
					}
					else
					{
						// If losing most wars, vote for peace
						bValid = (iWarsLosing > iWarsWinning);
					}

					if (!bValid && !bLosingBig && bWinningBig)
					{
						// Can we continue this war with defiance penalties?
						if( !AI_isFinancialTrouble() )
						{
							if (!GC.getGame().getSorenRandNum(iPeaceRand, "AI defy Force Peace!"))
							{
								bDefy = true;
							}
						}
					}
				}
				else if (eSecretaryGeneral == getTeam() && !bPropose)
				{
					bValid = true;
				}
				else if (GET_TEAM(ePeaceTeam).isAtWar(getTeam()))
				{
					bool bWantsToEndWar = (GET_TEAM(getTeam()).AI_endWarVal(ePeaceTeam) > (3*GET_TEAM(ePeaceTeam).AI_endWarVal(getTeam()))/2);
					bValid = bWantsToEndWar;

					if( bValid )
					{
						bValid = bWinningBig || (iWarsWinning > iWarsLosing) || (GET_TEAM(getTeam()).getAtWarCount(true, true) > 1);
					}

					if (!bValid && bThisPlayerWinning && (iWarsLosing >= iWarsWinning) && !bPropose )
					{
						if( !GET_TEAM(getTeam()).isAVassal() )
						{
							if( (GET_TEAM(getTeam()).getAtWarCount(true) == 1) || bLosingBig )
							{
								// Can we continue this war with defiance penalties?
								if( !AI_isFinancialTrouble() )
								{
									int iDefyRand = GC.getLeaderHeadInfo(getPersonalityType()).getBasePeaceWeight();
									iDefyRand /= (bAggressiveAI ? 2 : 1);

									if (GC.getGame().getSorenRandNum(iDefyRand, "AI Erratic Defiance (Force Peace)") == 0)
									{
										bDefy = true;
									}
								}
							}
						}
					}

					if( !bValid && !bDefy && !bPropose )
					{
						if((GET_TEAM(getTeam()).AI_getAttitude(eSecretaryGeneral) > GC.getLeaderHeadInfo(getPersonalityType()).getVassalRefuseAttitudeThreshold()) )
						{
							// Influence by secretary
							if( NO_DENIAL == GET_TEAM(getTeam()).AI_makePeaceTrade(ePeaceTeam, eSecretaryGeneral) )
							{
								bValid = true;
							}
							else if( eSecretaryGeneral != NO_TEAM && GET_TEAM(getTeam()).isVassal(eSecretaryGeneral) )
							{
								bValid = true;
							}
						}
					}
				}
				else
				{
					if( GET_TEAM(getTeam()).AI_getWarPlan(ePeaceTeam) != NO_WARPLAN )
					{
						// Keep planned enemy occupied
						bValid = false;
					}
					else if( GET_TEAM(getTeam()).AI_shareWar(ePeaceTeam)  && !(GET_TEAM(getTeam()).isVassal(ePeaceTeam)) )
					{
						// Keep ePeaceTeam at war with our common enemies
						bValid = false;
					}
					else if(iWarsLosing > iWarsWinning)
					{
						// Feel pity for team that is losing (if like them enough to not declare war on them)
						bValid = (GET_TEAM(getTeam()).AI_getAttitude(ePeaceTeam) >= GC.getLeaderHeadInfo(getPersonalityType()).getDeclareWarThemRefuseAttitudeThreshold());
					}
					else 
					{
						// Stop a team that is winning (if don't like them enough to join them in war)
						bValid = (GET_TEAM(getTeam()).AI_getAttitude(ePeaceTeam) < GC.getLeaderHeadInfo(getPersonalityType()).getDeclareWarRefuseAttitudeThreshold());
					}

					if( !bValid )
					{
						if( bFriendlyToSecretary && !GET_TEAM(getTeam()).isVassal(ePeaceTeam) )
						{
							// Influence by secretary
							bValid = true;
						}
					}
				}
// K-Mod end
			}
			else if (GC.getVoteInfo(eVote).isForceNoTrade())
			{
				FAssert(kVoteData.ePlayer != NO_PLAYER);
				TeamTypes eEmbargoTeam = GET_PLAYER(kVoteData.ePlayer).getTeam();

				if (eSecretaryGeneral == getTeam() && !bPropose)
				{
					bValid = true;
				}
				else if (eEmbargoTeam == getTeam())
				{
					bValid = false;
					if (!isNoForeignTrade())
					{
						bDefy = true;
					}
				}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      12/30/08                                jdog5000      */
/*                                                                                              */
/* Diplomacy AI                                                                                 */
/************************************************************************************************/
				else
				{
					if( bFriendlyToSecretary )
					{
						return PLAYER_VOTE_YES;
					}
					else if( canStopTradingWithTeam(eEmbargoTeam) )
					{
						bValid = (NO_DENIAL == AI_stopTradingTrade(eEmbargoTeam, kVoteData.ePlayer));
						if (bValid)
						{
							bValid = (GET_TEAM(getTeam()).AI_getAttitude(eEmbargoTeam) <= ATTITUDE_CAUTIOUS);
						}
					}
					else
					{
						bValid = (GET_TEAM(getTeam()).AI_getAttitude(eEmbargoTeam) < ATTITUDE_CAUTIOUS);
					}
				}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
			}
			else if (GC.getVoteInfo(eVote).isForceWar())
			{
				FAssert(kVoteData.ePlayer != NO_PLAYER);
				TeamTypes eWarTeam = GET_PLAYER(kVoteData.ePlayer).getTeam();

				if (eSecretaryGeneral == getTeam() && !bPropose)
				{
					bValid = true;
				}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      12/30/08                                jdog5000      */
/*                                                                                              */
/* Diplomacy AI                                                                                 */
/************************************************************************************************/
/* original BTS code
				else if (eWarTeam == getTeam())
				{
					bValid = false;
				}
				else if (GET_TEAM(eWarTeam).isAtWar(getTeam()))
*/
				else if (eWarTeam == getTeam() || GET_TEAM(getTeam()).isVassal(eWarTeam))
				{
					// Explicit rejection by all who will definitely be attacked
					bValid = false;
				}
				else if ( GET_TEAM(getTeam()).AI_getWarPlan(eWarTeam) != NO_WARPLAN )
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
				{
					bValid = true;
				}
				else
				{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      07/20/09                                jdog5000      */
/*                                                                                              */
/* Diplomacy AI                                                                                 */
/************************************************************************************************/
/* original BTS code
					bValid = (bPropose || NO_DENIAL == GET_TEAM(getTeam()).AI_declareWarTrade(eWarTeam, eSecretaryGeneral));
					if (bValid)
					{
						bValid = (GET_TEAM(getTeam()).AI_getAttitude(eWarTeam) < ATTITUDE_CAUTIOUS);
					}
*/
					if( !bPropose && GET_TEAM(getTeam()).isAVassal() )
					{
						// Vassals always deny war trade requests and thus previously always voted no
						bValid = false;

						if( GET_TEAM(getTeam()).getAnyWarPlanCount(true) == 0 )
						{
							if( eSecretaryGeneral == NO_TEAM || (GET_TEAM(getTeam()).AI_getAttitude(eSecretaryGeneral) > GC.getLeaderHeadInfo(getPersonalityType()).getDeclareWarRefuseAttitudeThreshold()) )
							{
								if( eSecretaryGeneral != NO_TEAM && GET_TEAM(getTeam()).isVassal(eSecretaryGeneral) )
								{
									bValid = true;
								}
								else if( (GET_TEAM(getTeam()).isAVassal() ? GET_TEAM(getTeam()).getCurrentMasterPower(true) : GET_TEAM(getTeam()).getPower(true)) > GET_TEAM(eWarTeam).getDefensivePower() )
								{
									bValid = true;
								}
							}
						}
					}
					else
					{
						bValid = (bPropose || NO_DENIAL == GET_TEAM(getTeam()).AI_declareWarTrade(eWarTeam, eSecretaryGeneral));
					}

					if (bValid)
					{
						int iNoWarOdds = GC.getLeaderHeadInfo(getPersonalityType()).getNoWarAttitudeProb((GET_TEAM(getTeam()).AI_getAttitude(eWarTeam)));
						bValid = ((iNoWarOdds < 30) || (GC.getGame().getSorenRandNum(100, "AI War Vote Attitude Check (Force War)") > iNoWarOdds));
					}
					/*
					else
					{
						// Consider defying resolution
						if( !GET_TEAM(getTeam()).isAVassal() )
						{
							if( eSecretaryGeneral == NO_TEAM || GET_TEAM(getTeam()).AI_getAttitude(eWarTeam) > GET_TEAM(getTeam()).AI_getAttitude(eSecretaryGeneral) )
							{
								if( GET_TEAM(getTeam()).AI_getAttitude(eWarTeam) > GC.getLeaderHeadInfo(getPersonalityType()).getDefensivePactRefuseAttitudeThreshold() )
								{
									int iDefyRand = GC.getLeaderHeadInfo(getPersonalityType()).getBasePeaceWeight();
									iDefyRand /= (GC.getGameINLINE().isOption(GAMEOPTION_AGGRESSIVE_AI) ? 2 : 1);

									if (GC.getGame().getSorenRandNum(iDefyRand, "AI Erratic Defiance (Force War)") > 0)
									{
										bDefy = true;
									}
								}
							}
						}
					}
					*/
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
				}
			}
			else if (GC.getVoteInfo(eVote).isAssignCity())
			{
				bValid = false;

				FAssert(kVoteData.ePlayer != NO_PLAYER);
				CvPlayer& kPlayer = GET_PLAYER(kVoteData.ePlayer);
				CvCity* pCity = kPlayer.getCity(kVoteData.iCityId);
				if (NULL != pCity)
				{
					if (NO_PLAYER != kVoteData.eOtherPlayer && kVoteData.eOtherPlayer != pCity->getOwnerINLINE())
					{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      10/03/09                                jdog5000      */
/*                                                                                              */
/* Diplomacy AI                                                                                 */
/************************************************************************************************/
						if ((!bPropose && eSecretaryGeneral == getTeam()) || GET_PLAYER(kVoteData.eOtherPlayer).getTeam() == getTeam())
						{
							bValid = true;
						}
						else if (kPlayer.getTeam() == getTeam())
						{
							bValid = false;
							// BBAI TODO: Wonders, holy city, aggressive AI?
							if (GC.getGame().getSorenRandNum(3, "AI Erratic Defiance (Assign City)") == 0)
							{
								bDefy = true;
							}
						}
						else
						{
							bValid = (AI_getAttitude(kVoteData.ePlayer) < AI_getAttitude(kVoteData.eOtherPlayer));
						}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
					}
				}
			}
		}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      12/30/08                                jdog5000      */
/*                                                                                              */
/* Diplomacy AI                                                                                 */
/************************************************************************************************/
/* original BTS code
		if (bDefy && canDefyResolution(eVoteSource, kVoteData))
*/
		// Don't defy resolutions from friends
		if( bDefy && !bFriendlyToSecretary && canDefyResolution(eVoteSource, kVoteData))
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
		{
			return PLAYER_VOTE_NEVER;
		}

		return (bValid ? PLAYER_VOTE_YES : PLAYER_VOTE_NO);
	}

}

int CvPlayerAI::AI_dealVal(PlayerTypes ePlayer, const CLinkList<TradeData>* pList, bool bIgnoreAnnual, int iChange) const
{
	CLLNode<TradeData>* pNode;
	CvCity* pCity;
	int iValue;

	FAssertMsg(ePlayer != getID(), "shouldn't call this function on ourselves");

	iValue = 0;

	if (atWar(getTeam(), GET_PLAYER(ePlayer).getTeam()))
	{
		iValue += GET_TEAM(getTeam()).AI_endWarVal(GET_PLAYER(ePlayer).getTeam());
	}

	for (pNode = pList->head(); pNode; pNode = pList->next(pNode))
	{
		FAssertMsg(!(pNode->m_data.m_bHidden), "(pNode->m_data.m_bHidden) did not return false as expected");

		switch (pNode->m_data.m_eItemType)
		{
		case TRADE_TECHNOLOGIES:
			iValue += GET_TEAM(getTeam()).AI_techTradeVal((TechTypes)(pNode->m_data.m_iData), GET_PLAYER(ePlayer).getTeam());
			break;
		case TRADE_RESOURCES:
			if (!bIgnoreAnnual)
			{
				iValue += AI_bonusTradeVal(((BonusTypes)(pNode->m_data.m_iData)), ePlayer, iChange);
			}
			break;
		case TRADE_CITIES:
			pCity = GET_PLAYER(ePlayer).getCity(pNode->m_data.m_iData);
			if (pCity != NULL)
			{
				iValue += AI_cityTradeVal(pCity);
			}
			break;
		case TRADE_GOLD:
			iValue += (pNode->m_data.m_iData * AI_goldTradeValuePercent()) / 100;
			break;
		case TRADE_GOLD_PER_TURN:
			if (!bIgnoreAnnual)
			{
				iValue += AI_goldPerTurnTradeVal(pNode->m_data.m_iData);
			}
			break;
		case TRADE_MAPS:
			iValue += GET_TEAM(getTeam()).AI_mapTradeVal(GET_PLAYER(ePlayer).getTeam());
			break;
		case TRADE_SURRENDER:
			if (!bIgnoreAnnual)
			{
				iValue += GET_TEAM(getTeam()).AI_surrenderTradeVal(GET_PLAYER(ePlayer).getTeam());
			}
			break;
		case TRADE_VASSAL:
			if (!bIgnoreAnnual)
			{
				iValue += GET_TEAM(getTeam()).AI_vassalTradeVal(GET_PLAYER(ePlayer).getTeam());
			}
			break;
		case TRADE_OPEN_BORDERS:
			iValue += GET_TEAM(getTeam()).AI_openBordersTradeVal(GET_PLAYER(ePlayer).getTeam());
			break;
		case TRADE_DEFENSIVE_PACT:
			iValue += GET_TEAM(getTeam()).AI_defensivePactTradeVal(GET_PLAYER(ePlayer).getTeam());
			break;
		case TRADE_PEACE:
			iValue += GET_TEAM(getTeam()).AI_makePeaceTradeVal(((TeamTypes)(pNode->m_data.m_iData)), GET_PLAYER(ePlayer).getTeam());
			break;
		case TRADE_WAR:
			iValue += GET_TEAM(getTeam()).AI_declareWarTradeVal(((TeamTypes)(pNode->m_data.m_iData)), GET_PLAYER(ePlayer).getTeam());
			break;
		case TRADE_EMBARGO:
			iValue += AI_stopTradingTradeVal(((TeamTypes)(pNode->m_data.m_iData)), ePlayer);
			break;
		case TRADE_CIVIC:
			iValue += AI_civicTradeVal(((CivicTypes)(pNode->m_data.m_iData)), ePlayer);
			break;
		case TRADE_RELIGION:
			iValue += AI_religionTradeVal(((ReligionTypes)(pNode->m_data.m_iData)), ePlayer);
			break;
		}
	}

	return iValue;
}


bool CvPlayerAI::AI_goldDeal(const CLinkList<TradeData>* pList) const
{
	CLLNode<TradeData>* pNode;

	for (pNode = pList->head(); pNode; pNode = pList->next(pNode))
	{
		FAssert(!(pNode->m_data.m_bHidden));

		switch (pNode->m_data.m_eItemType)
		{
		case TRADE_GOLD:
		case TRADE_GOLD_PER_TURN:
			return true;
			break;
		}
	}

	return false;
}


/// \brief AI decision making on a proposal it is given
///
/// In this function the AI considers whether or not to accept another player's proposal.  This is used when
/// considering proposals from the human player made in the diplomacy window as well as a couple other places.
bool CvPlayerAI::AI_considerOffer(PlayerTypes ePlayer, const CLinkList<TradeData>* pTheirList, const CLinkList<TradeData>* pOurList, int iChange) const
{
	const CvTeamAI& kOurTeam = GET_TEAM(getTeam()); // K-Mod

	CLLNode<TradeData>* pNode;
	int iThreshold;

	FAssertMsg(ePlayer != getID(), "shouldn't call this function on ourselves");

	if (AI_goldDeal(pTheirList) && AI_goldDeal(pOurList))
	{
		return false;
	}

	if (iChange > -1)
	{
		for (pNode = pOurList->head(); pNode; pNode = pOurList->next(pNode))
		{
			if (getTradeDenial(ePlayer, pNode->m_data) != NO_DENIAL)
			{
				return false;
			}
		}
	}

	if (GET_PLAYER(ePlayer).getTeam() == getTeam())
	{
		return true;
	}

	// K-Mod. Refuse all war-time offers unless it's part of a peace deal
	if (atWar(getTeam(), GET_PLAYER(ePlayer).getTeam()))
	{
		bool bEndWar = false;
		for (pNode = pTheirList->head(); !bEndWar && pNode; pNode = pTheirList->next(pNode))
		{
			if (CvDeal::isEndWar(pNode->m_data.m_eItemType))
				bEndWar = true;
		}
		for (pNode = pOurList->head(); !bEndWar && pNode; pNode = pOurList->next(pNode))
		{
			if (CvDeal::isEndWar(pNode->m_data.m_eItemType))
				bEndWar = true;
		}

		if (!bEndWar)
			return false;
	}
	// K-Mod end

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      10/23/09                                jdog5000      */
/*                                                                                              */
/* Diplomacy AI                                                                                 */
/************************************************************************************************/
	// Don't always accept giving deals, TRADE_VASSAL and TRADE_SURRENDER come with strings attached
	bool bVassalTrade = false;
	if (iChange > -1) // K-Mod
	{
		for (pNode = pTheirList->head(); pNode; pNode = pTheirList->next(pNode))
		{
			if( pNode->m_data.m_eItemType == TRADE_VASSAL )
			{
				bVassalTrade = true;

				for (int iTeam = 0; iTeam < MAX_CIV_TEAMS; iTeam++)
				{
					if (GET_TEAM((TeamTypes)iTeam).isAlive())
					{
						if (iTeam != getTeam() && iTeam != GET_PLAYER(ePlayer).getTeam() && atWar(GET_PLAYER(ePlayer).getTeam(), (TeamTypes)iTeam) && !atWar(getTeam(), (TeamTypes)iTeam))
						{
							if (kOurTeam.AI_declareWarTrade((TeamTypes)iTeam, GET_PLAYER(ePlayer).getTeam(), false) != NO_DENIAL)
							{
								return false;
							}
						}
					}
				}
			}
			else if( pNode->m_data.m_eItemType == TRADE_SURRENDER )
			{
				bVassalTrade = true;

				if( !(kOurTeam.AI_acceptSurrender(GET_PLAYER(ePlayer).getTeam())) )
				{
					return false;
				}
			}
		}
	}
	
	if( !bVassalTrade )
	{
		if ((pOurList->getLength() == 0) && (pTheirList->getLength() > 0))
		{
			return true;
		}
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
	// K-Mod
	if (iChange < 0)
	{
		// There are only a few cases where the AI will allow ongoing trade with its worst enemy...
		if (kOurTeam.AI_getWorstEnemy() == GET_PLAYER(ePlayer).getTeam() && !kOurTeam.isVassal(GET_PLAYER(ePlayer).getTeam()))
		{
			if (pOurList->getLength() > 1 || pOurList->head()->m_data.m_eItemType != TRADE_GOLD_PER_TURN)
			{
				// this trade isn't one of the special cases.
				return false;
			}
		}
	}
	// K-Mod end

	int iOurValue = GET_PLAYER(ePlayer).AI_dealVal(getID(), pOurList, false, iChange);
	int iTheirValue = AI_dealVal(ePlayer, pTheirList, false, iChange);

	if (iOurValue > 0 && 0 == pTheirList->getLength() && 0 == iTheirValue)
	{
		// K-Mod
		// Don't give any gifts to civs that you are about to go to war with.
		if (kOurTeam.AI_getWarPlan(GET_PLAYER(ePlayer).getTeam()) != NO_WARPLAN)
			return false;
		//Don't cancel gift deals to vassals that you like, unless you need the gift back.
		if (iChange < 0 && GET_TEAM(GET_PLAYER(ePlayer).getTeam()).isVassal(getTeam()))
		{
			/*if (iOurValue > AI_dealVal(ePlayer, pOurList, false, 1))
				return true;
			else
				return false; */
			// Simply comparing deal values doesn't work because the value from voluntary vassals gets halved.
			// So unfortunately, I'm going to use a kludge:
			if (pOurList->getLength() == 1 && pOurList->head()->m_data.m_eItemType == TRADE_RESOURCES)
			{
				BonusTypes eBonus = (BonusTypes)pOurList->head()->m_data.m_iData;
				if (GET_PLAYER(ePlayer).AI_bonusVal(eBonus, -1) > AI_bonusVal(eBonus, 1))
					return true;
				else
					return false;
			}
		}
		// K-Mod end

		if (kOurTeam.isVassal(GET_PLAYER(ePlayer).getTeam()) && CvDeal::isVassalTributeDeal(pOurList))
		{
			if (AI_getAttitude(ePlayer, false) <= GC.getLeaderHeadInfo(getPersonalityType()).getVassalRefuseAttitudeThreshold()
				&& kOurTeam.getAtWarCount(true) == 0
				&& GET_TEAM(GET_PLAYER(ePlayer).getTeam()).getDefensivePactCount() == 0)
			{
				iOurValue *= (kOurTeam.getPower(false) + 10);
				iOurValue /= (GET_TEAM(GET_PLAYER(ePlayer).getTeam()).getPower(false) + 10);
			}
			else
			{
				return true;
			}
		}
		else
		{
			if (AI_getAttitude(ePlayer) < ATTITUDE_PLEASED)
			{
				if (kOurTeam.getPower(false) > ((GET_TEAM(GET_PLAYER(ePlayer).getTeam()).getPower(false) * 4) / 3))
				{
					return false;
				}
			}

			if (AI_getMemoryCount(ePlayer, MEMORY_MADE_DEMAND_RECENT) > 0)
			{
				return false;
			}
		}

		iThreshold = (kOurTeam.AI_getHasMetCounter(GET_PLAYER(ePlayer).getTeam()) + 50);

		iThreshold *= 2;

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      06/12/10                                jdog5000      */
/*                                                                                              */
/* Diplomacy AI                                                                                 */
/************************************************************************************************/
		if (GET_TEAM(GET_PLAYER(ePlayer).getTeam()).AI_isLandTarget(getTeam()))
		{
			iThreshold *= 3;
		}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

		iThreshold *= (GET_TEAM(GET_PLAYER(ePlayer).getTeam()).getPower(false) + 100);
		iThreshold /= (kOurTeam.getPower(false) + 100);

		iThreshold -= GET_PLAYER(ePlayer).AI_getPeacetimeGrantValue(getID());

		return (iOurValue < iThreshold);
	}

	if (iChange < 0)
	{
		//return (iTheirValue * 110 >= iOurValue * 100);
		return (iTheirValue * 125 >= iOurValue * 100); // K-Mod
	}

	return (iTheirValue >= iOurValue);
}


bool CvPlayerAI::AI_counterPropose(PlayerTypes ePlayer, const CLinkList<TradeData>* pTheirList, const CLinkList<TradeData>* pOurList, CLinkList<TradeData>* pTheirInventory, CLinkList<TradeData>* pOurInventory, CLinkList<TradeData>* pTheirCounter, CLinkList<TradeData>* pOurCounter) const
{
	CLLNode<TradeData>* pNode;
	CLLNode<TradeData>* pBestNode;
	CLLNode<TradeData>* pGoldPerTurnNode;
	CLLNode<TradeData>* pGoldNode;
	bool* pabBonusDeal;
	CvCity* pCity;
	bool bTheirGoldDeal;
	bool bOurGoldDeal;
	//int iHumanDealWeight;
	//int iAIDealWeight;
	int iGoldData;
	int iGoldWeight;
	int iWeight;
	int iBestWeight;
	int iValue;
	int iBestValue;
	int iI;

	bTheirGoldDeal = AI_goldDeal(pTheirList);
	bOurGoldDeal = AI_goldDeal(pOurList);

	if (bOurGoldDeal && bTheirGoldDeal)
	{
		return false;
	}

	pabBonusDeal = new bool[GC.getNumBonusInfos()];

	for (iI = 0; iI < GC.getNumBonusInfos(); iI++)
	{
		pabBonusDeal[iI] = false;
	}

	pGoldPerTurnNode = NULL;
	pGoldNode = NULL;

	/* original bts code
	iHumanDealWeight = AI_dealVal(ePlayer, pTheirList);
	iAIDealWeight = GET_PLAYER(ePlayer).AI_dealVal(getID(), pOurList); */
	// K-Mod note: the original code had the human and AI weights the wrong way around.
	// I found this confusing, so I've corrected it throughout this function.
	// (Under normal usage of this fuction, the AI player counters the human proposal - so "they" are human, not us.)
	int iValueForUs = AI_dealVal(ePlayer, pTheirList);
	int iValueForThem = GET_PLAYER(ePlayer).AI_dealVal(getID(), pOurList);

	int iGoldValuePercent = AI_goldTradeValuePercent();

	pTheirCounter->clear();
	pOurCounter->clear();

	// K-Mod. Refuse all war-time offers unless it's part of a peace deal
	if (atWar(getTeam(), GET_PLAYER(ePlayer).getTeam()))
	{
		// Check to see if there is already an end-war item on the table
		bool bEndWar = false;
		for (pNode = pTheirList->head(); !bEndWar && pNode; pNode = pTheirList->next(pNode))
		{
			if (CvDeal::isEndWar(pNode->m_data.m_eItemType))
				bEndWar = true;
		}
		for (pNode = pOurList->head(); !bEndWar && pNode; pNode = pOurList->next(pNode))
		{
			if (CvDeal::isEndWar(pNode->m_data.m_eItemType))
				bEndWar = true;
		}

		// see if someone is willing to surrender
		if (!bEndWar)
		{
			TradeData item;
			setTradeItem(&item, TRADE_SURRENDER);
			if (canTradeItem(ePlayer, item, true))
			{
				pOurCounter->insertAtEnd(item);
				bEndWar = true;
			}
			else if (GET_PLAYER(ePlayer).canTradeItem(getID(), item, true))
			{
				pTheirCounter->insertAtEnd(item);
				bEndWar = true;
			}
		}
		// last chance: try a peace treaty.
		if (!bEndWar)
		{
			TradeData item;
			setTradeItem(&item, TRADE_PEACE_TREATY);
			if (canTradeItem(ePlayer, item, true) && GET_PLAYER(ePlayer).canTradeItem(getID(), item, true))
			{
				//pOurCounter->insertAtEnd(item);
				//pTheirCounter->insertAtEnd(item);
				// unfortunately, there is some weirdness in the game engine which causes it to flip its wig
				// if trade items are added to both sides of a peace deal... so we have to do it like this:
				if (iValueForThem > iValueForUs)
					pTheirCounter->insertAtEnd(item);
				else
					pOurCounter->insertAtEnd(item);
				bEndWar = true;
			}
			else
			{
				return false; // if there is no peace, there will be no trade
			}
		}
	}
	// K-Mod end

	if (iValueForThem > iValueForUs)
	{
		if (atWar(getTeam(), GET_PLAYER(ePlayer).getTeam()))
		{
			iBestValue = 0;
			iBestWeight = 0;
			pBestNode = NULL;

			for (pNode = pTheirInventory->head(); pNode && iValueForThem > iValueForUs; pNode = pTheirInventory->next(pNode))
			{
				if (!pNode->m_data.m_bOffering && !pNode->m_data.m_bHidden)
				{
					if (pNode->m_data.m_eItemType == TRADE_CITIES)
					{
						FAssert(GET_PLAYER(ePlayer).canTradeItem(getID(), pNode->m_data));

						if (GET_PLAYER(ePlayer).getTradeDenial(getID(), pNode->m_data) == NO_DENIAL)
						{
							pCity = GET_PLAYER(ePlayer).getCity(pNode->m_data.m_iData);

							if (pCity != NULL)
							{
								iWeight = AI_cityTradeVal(pCity);

								if (iWeight > 0)
								{
									iValue = AI_targetCityValue(pCity, false);

									if (iValue > iBestValue)
									{
										iBestValue = iValue;
										iBestWeight = iWeight;
										pBestNode = pNode;
									}
								}
							}
						}
					}
				}
			}

			if (pBestNode != NULL)
			{
				iValueForUs += iBestWeight;
				pTheirCounter->insertAtEnd(pBestNode->m_data);
			}
		}

		for (pNode = pTheirInventory->head(); pNode && iValueForThem > iValueForUs; pNode = pTheirInventory->next(pNode))
		{
			if (!pNode->m_data.m_bOffering && !pNode->m_data.m_bHidden)
			{
				FAssert(GET_PLAYER(ePlayer).canTradeItem(getID(), pNode->m_data));

				if (GET_PLAYER(ePlayer).getTradeDenial(getID(), pNode->m_data) == NO_DENIAL)
				{
					switch (pNode->m_data.m_eItemType)
					{
					case TRADE_GOLD:
						if (!bOurGoldDeal)
						{
							pGoldNode = pNode;
						}
						break;
					case TRADE_GOLD_PER_TURN:
						if (!bOurGoldDeal)
						{
							pGoldPerTurnNode = pNode;
						}
						break;
					}
				}
			}
		}

		int iGoldWeight = iValueForThem - iValueForUs;

		if (iGoldWeight > 0)
		{
			if (pGoldNode)
			{
				iGoldData = iGoldWeight * 100;
				iGoldData /= iGoldValuePercent;
/************************************************************************************************/
/* UNOFFICIAL_PATCH                       09/17/09                     dilandau & jdog5000      */
/*                                                                                              */
/* Bugfix				                                                                        */
/************************************************************************************************/
/* original bts code
				if ((iGoldData * iGoldValuePercent) < iGoldWeight)
*/
				if ((iGoldData * iGoldValuePercent) < iGoldWeight * 100)
/************************************************************************************************/
/* UNOFFICIAL_PATCH                        END                                                  */
/************************************************************************************************/
				{
					iGoldData++;
				}
				if (GET_PLAYER(ePlayer).AI_maxGoldTrade(getID()) >= iGoldData)
				{
					pGoldNode->m_data.m_iData = iGoldData;
					iValueForUs += (iGoldData * iGoldValuePercent) / 100;
					pTheirCounter->insertAtEnd(pGoldNode->m_data);
					pGoldNode = NULL;
				}
			}
		}

		for (pNode = pOurList->head(); pNode; pNode = pOurList->next(pNode))
		{
			FAssert(!(pNode->m_data.m_bHidden));

			switch (pNode->m_data.m_eItemType)
			{
			case TRADE_RESOURCES:
				pabBonusDeal[pNode->m_data.m_iData] = true;
				break;
			}
		}
		
		for (pNode = pTheirInventory->head(); pNode && iValueForThem > iValueForUs; pNode = pTheirInventory->next(pNode))
		{
			if (!pNode->m_data.m_bOffering && !pNode->m_data.m_bHidden)
			{
				FAssert(GET_PLAYER(ePlayer).canTradeItem(getID(), pNode->m_data));

				if (GET_PLAYER(ePlayer).getTradeDenial(getID(), pNode->m_data) == NO_DENIAL)
				{
					iWeight = 0;

					switch (pNode->m_data.m_eItemType)
					{
					case TRADE_TECHNOLOGIES:
						iWeight += GET_TEAM(getTeam()).AI_techTradeVal((TechTypes)(pNode->m_data.m_iData), GET_PLAYER(ePlayer).getTeam());
						break;
					case TRADE_RESOURCES:
						if (!pabBonusDeal[pNode->m_data.m_iData])
						{
							if (GET_PLAYER(ePlayer).getNumTradeableBonuses((BonusTypes)(pNode->m_data.m_iData)) > 1)
							{
								if (GET_PLAYER(ePlayer).AI_corporationBonusVal((BonusTypes)(pNode->m_data.m_iData)) == 0)
								{
									iWeight += AI_bonusTradeVal(((BonusTypes)(pNode->m_data.m_iData)), ePlayer, 1);
									pabBonusDeal[pNode->m_data.m_iData] = true;
								}
							}
						}
						break;
					}

					if (iWeight > 0)
					{
						iValueForUs += iWeight;
						pTheirCounter->insertAtEnd(pNode->m_data);
					}
				}
			}
		}

		for (pNode = pTheirInventory->head(); pNode && iValueForThem > iValueForUs; pNode = pTheirInventory->next(pNode))
		{
			if (!pNode->m_data.m_bOffering && !pNode->m_data.m_bHidden)
			{
				if (pNode->m_data.m_eItemType == TRADE_MAPS)
				{
					FAssert(GET_PLAYER(ePlayer).canTradeItem(getID(), pNode->m_data));

					if (GET_PLAYER(ePlayer).getTradeDenial(getID(), pNode->m_data) == NO_DENIAL)
					{
						iWeight = GET_TEAM(getTeam()).AI_mapTradeVal(GET_PLAYER(ePlayer).getTeam());

						if (iWeight > 0)
						{
							iValueForUs += iWeight;
							pTheirCounter->insertAtEnd(pNode->m_data);
						}
					}
				}
			}
		}

		iGoldWeight = iValueForThem - iValueForUs;

		if (iGoldWeight > 0)
		{
			if (pGoldNode)
			{
				iGoldData = iGoldWeight * 100;
				iGoldData /= iGoldValuePercent;
				
				if ((iGoldWeight * 100) > (iGoldData * iGoldValuePercent))
				{
					iGoldData++;
				}

				iGoldData = std::min(iGoldData, GET_PLAYER(ePlayer).AI_maxGoldTrade(getID()));

				if (iGoldData > 0)
				{
					pGoldNode->m_data.m_iData = iGoldData;
					iValueForUs += (iGoldData * iGoldValuePercent) / 100;
					pTheirCounter->insertAtEnd(pGoldNode->m_data);
					pGoldNode = NULL;
				}
			}
		}

		if (iValueForThem > iValueForUs)
		{
			if (pGoldPerTurnNode)
			{
				iGoldData = 0;

				while (AI_goldPerTurnTradeVal(iGoldData) < (iValueForThem - iValueForUs))
				{
					iGoldData++;
				}

				iGoldData = std::min(iGoldData, GET_PLAYER(ePlayer).AI_maxGoldPerTurnTrade(getID()));

				if (iGoldData > 0)
				{
					pGoldPerTurnNode->m_data.m_iData = iGoldData;
					iValueForUs += AI_goldPerTurnTradeVal(pGoldPerTurnNode->m_data.m_iData);
					pTheirCounter->insertAtEnd(pGoldPerTurnNode->m_data);
					pGoldPerTurnNode = NULL;
				}
			}
		}

		for (pNode = pTheirInventory->head(); pNode && iValueForThem > iValueForUs; pNode = pTheirInventory->next(pNode))
		{
			if (!pNode->m_data.m_bOffering && !pNode->m_data.m_bHidden)
			{
				if (pNode->m_data.m_eItemType == TRADE_RESOURCES)
				{
					FAssert(GET_PLAYER(ePlayer).canTradeItem(getID(), pNode->m_data));

					if (GET_PLAYER(ePlayer).getTradeDenial(getID(), pNode->m_data) == NO_DENIAL)
					{
						iWeight = 0;

						if (!pabBonusDeal[pNode->m_data.m_iData])
						{
							if (GET_PLAYER(ePlayer).getNumTradeableBonuses((BonusTypes)(pNode->m_data.m_iData)) > 0)
							{
								iWeight += AI_bonusTradeVal(((BonusTypes)(pNode->m_data.m_iData)), ePlayer, 1);
								pabBonusDeal[pNode->m_data.m_iData] = true;
							}
						}

						if (iWeight > 0)
						{
							iValueForUs += iWeight;
							pTheirCounter->insertAtEnd(pNode->m_data);
						}
					}
				}
			}
		}
	}
	else if (iValueForUs > iValueForThem)
	{
		if (atWar(getTeam(), GET_PLAYER(ePlayer).getTeam()))
		{
			/* original bts code
			bool bSurrender = false;
			for (pNode = pOurInventory->head(); pNode; pNode = pOurInventory->next(pNode))
			{
				if (!pNode->m_data.m_bOffering && !pNode->m_data.m_bHidden)
				{
					if (pNode->m_data.m_eItemType == TRADE_SURRENDER)
					{
						if (getTradeDenial(ePlayer, pNode->m_data) == NO_DENIAL)
						{
							iValueForThem += GET_TEAM(GET_PLAYER(ePlayer).getTeam()).AI_surrenderTradeVal(getTeam());
							pOurCounter->insertAtEnd(pNode->m_data);
							bSurrender = true;
						}
						break;
					}
				}
			}

			if (!bSurrender)
			{
				for (pNode = pOurInventory->head(); pNode; pNode = pOurInventory->next(pNode))
				{
					if (!pNode->m_data.m_bOffering && !pNode->m_data.m_bHidden)
					{
						if (pNode->m_data.m_eItemType == TRADE_PEACE_TREATY)
						{
							pOurCounter->insertAtEnd(pNode->m_data);
							break;
						}
					}
				}
			} */ // Disabled by K-Mod. This is obsolete.

			iBestValue = 0;
			iBestWeight = 0;
			pBestNode = NULL;

			for (pNode = pOurInventory->head(); pNode && iValueForUs > iValueForThem; pNode = pOurInventory->next(pNode))
			{
				if (!pNode->m_data.m_bOffering && !pNode->m_data.m_bHidden)
				{
					if (pNode->m_data.m_eItemType == TRADE_CITIES)
					{
						FAssert(canTradeItem(ePlayer, pNode->m_data));

						if (getTradeDenial(ePlayer, pNode->m_data) == NO_DENIAL)
						{
							pCity = getCity(pNode->m_data.m_iData);

							if (pCity != NULL)
							{
								iWeight = GET_PLAYER(ePlayer).AI_cityTradeVal(pCity);

								if (iWeight > 0)
								{
									iValue = GET_PLAYER(ePlayer).AI_targetCityValue(pCity, false);

									if (iValue > iBestValue)
									{
										if (iValueForUs >= (iValueForThem + iWeight))
										{
											iBestValue = iValue;
											iBestWeight = iWeight;
											pBestNode = pNode;
										}
									}
								}
							}
						}
					}
				}
			}

			if (pBestNode != NULL)
			{
				iValueForThem += iBestWeight;
				pOurCounter->insertAtEnd(pBestNode->m_data);
			}
		}

		for (pNode = pOurInventory->head(); pNode && iValueForUs > iValueForThem; pNode = pOurInventory->next(pNode))
		{
			if (!pNode->m_data.m_bOffering && !pNode->m_data.m_bHidden)
			{
				FAssert(canTradeItem(ePlayer, pNode->m_data));

				if (getTradeDenial(ePlayer, pNode->m_data) == NO_DENIAL)
				{
					switch (pNode->m_data.m_eItemType)
					{
					case TRADE_GOLD:
						if (!bTheirGoldDeal)
						{
							pGoldNode = pNode;
						}
						break;
					case TRADE_GOLD_PER_TURN:
						if (!bTheirGoldDeal)
						{
							pGoldPerTurnNode = pNode;
						}
						break;
					}
				}
			}
		}

		iGoldWeight = iValueForUs - iValueForThem;

		if (iGoldWeight > 0)
		{
			if (pGoldNode)
			{
				int iGoldData = iGoldWeight * 100;
				iGoldData /= iGoldValuePercent;

				if (AI_maxGoldTrade(ePlayer) >= iGoldData)
				{
					pGoldNode->m_data.m_iData = iGoldData;
					iValueForThem += ((iGoldData * iGoldValuePercent) / 100);
					pOurCounter->insertAtEnd(pGoldNode->m_data);
					pGoldNode = NULL;
				}
			}
		}

		for (pNode = pTheirList->head(); pNode; pNode = pTheirList->next(pNode))
		{
			FAssert(!(pNode->m_data.m_bHidden));

			switch (pNode->m_data.m_eItemType)
			{
			case TRADE_RESOURCES:
				pabBonusDeal[pNode->m_data.m_iData] = true;
				break;
			}
		}

		for (pNode = pOurInventory->head(); pNode && iValueForUs > iValueForThem; pNode = pOurInventory->next(pNode))
		{
			if (!pNode->m_data.m_bOffering && !pNode->m_data.m_bHidden)
			{
				FAssert(canTradeItem(ePlayer, pNode->m_data));

				if (getTradeDenial(ePlayer, pNode->m_data) == NO_DENIAL)
				{
					iWeight = 0;

					switch (pNode->m_data.m_eItemType)
					{
					case TRADE_TECHNOLOGIES:
						iWeight += GET_TEAM(GET_PLAYER(ePlayer).getTeam()).AI_techTradeVal((TechTypes)(pNode->m_data.m_iData), getTeam());
						break;
					case TRADE_RESOURCES:
						if (!pabBonusDeal[pNode->m_data.m_iData])
						{
							if (getNumTradeableBonuses((BonusTypes)(pNode->m_data.m_iData)) > 1)
							{
								iWeight += GET_PLAYER(ePlayer).AI_bonusTradeVal(((BonusTypes)(pNode->m_data.m_iData)), getID(), 1);
								pabBonusDeal[pNode->m_data.m_iData] = true;
							}
						}
						break;
					}

					if (iWeight > 0)
					{
						if (iValueForUs >= (iValueForThem + iWeight))
						{
							iValueForThem += iWeight;
							pOurCounter->insertAtEnd(pNode->m_data);
						}
					}
				}
			}
		}

		for (pNode = pOurInventory->head(); pNode && iValueForUs > iValueForThem; pNode = pOurInventory->next(pNode))
		{
			if (!pNode->m_data.m_bOffering && !pNode->m_data.m_bHidden)
			{
				if (pNode->m_data.m_eItemType == TRADE_MAPS)
				{
					FAssert(canTradeItem(ePlayer, pNode->m_data));

					if (getTradeDenial(ePlayer, pNode->m_data) == NO_DENIAL)
					{
						iWeight = GET_TEAM(GET_PLAYER(ePlayer).getTeam()).AI_mapTradeVal(getTeam());

						if (iWeight > 0)
						{
							if (iValueForUs >= (iValueForThem + iWeight))
							{
								iValueForThem += iWeight;
								pOurCounter->insertAtEnd(pNode->m_data);
							}
						}
					}
				}
			}
		}

		iGoldWeight = iValueForUs - iValueForThem;
		if (iGoldWeight > 0)
		{
			if (pGoldNode)
			{
				iGoldData = iGoldWeight * 100;
				iGoldData /= AI_goldTradeValuePercent();

				iGoldData = std::min(iGoldData, AI_maxGoldTrade(ePlayer));

				if (iGoldData > 0)
				{
					pGoldNode->m_data.m_iData = iGoldData;
					iValueForThem += (iGoldData * AI_goldTradeValuePercent()) / 100;
					pOurCounter->insertAtEnd(pGoldNode->m_data);
					pGoldNode = NULL;
				}
			}
		}

		if (iValueForUs > iValueForThem)
		{
			if (pGoldPerTurnNode)
			{
				iGoldData = 0;

				while (GET_PLAYER(ePlayer).AI_goldPerTurnTradeVal(iGoldData + 1) <= (iValueForUs - iValueForThem))
				{
					iGoldData++;
				}

				iGoldData = std::min(iGoldData, AI_maxGoldPerTurnTrade(ePlayer));

				if (iGoldData > 0)
				{
					pGoldPerTurnNode->m_data.m_iData = iGoldData;
					iValueForThem += GET_PLAYER(ePlayer).AI_goldPerTurnTradeVal(pGoldPerTurnNode->m_data.m_iData);
					pOurCounter->insertAtEnd(pGoldPerTurnNode->m_data);
					pGoldPerTurnNode = NULL;
				}
			}
		}
	}

	SAFE_DELETE_ARRAY(pabBonusDeal);

	/* original bts code
	return ((iValueForThem <= iValueForUs) && ((pOurList->getLength() > 0) || (pOurCounter->getLength() > 0) || (pTheirCounter->getLength() > 0))); */

	// K-Mod. This function now needs to handle AI - AI deals, and human auto-counters to AI suggested deals.
	if (pOurList->getLength() == 0 && pOurCounter->getLength() == 0 && pTheirCounter->getLength() == 0)
		return false;

	bool bDeal = false;
	
	if (GET_PLAYER(ePlayer).isHuman())
	{
		// AI counting a human proposal:
		// no not compromise the AI's value, and don't even consider the human's value.
		bDeal = iValueForUs >= iValueForThem;
		FAssert(!isHuman());
	}
	else
	{
		if (isHuman())
		{
			// Human civ auto-negotiating an AI proposal before it is put to the player for confirmation:
			// let the AI show a little bit of leniency
			// don't bother putting it to the player if it is really bad value
			bDeal = 4*iValueForUs >= iValueForThem
				&& 100*iValueForThem >= GET_PLAYER(ePlayer).AI_tradeAcceptabilityThreshold(getID())*iValueForUs;
		}
		else
		{
			// Negotiations between two AIs:
			bDeal = 100*iValueForUs >= AI_tradeAcceptabilityThreshold(ePlayer)*iValueForThem
				&& 100*iValueForThem >= GET_PLAYER(ePlayer).AI_tradeAcceptabilityThreshold(getID())*iValueForUs;

		}
	}
	return bDeal;
	// K-Mod end
}

// K-Mod. Minimum percentage of trade value that this player will accept.
// ie. Accept trades if 100 * value_for_us >= residual * value_for_them .
int CvPlayerAI::AI_tradeAcceptabilityThreshold(PlayerTypes eTrader) const
{
	if (isHuman())
		return 75;

	int iDiscount = 25;
	iDiscount += 10 * AI_getAttitudeWeight(eTrader) / 100;
	// This puts us between 15 (furious) and 35 (friendly)
	// in some later version, I might make this personality based.

	// adjust for team rank.
	int iRankDelta = GC.getGameINLINE().getTeamRank(GET_PLAYER(eTrader).getTeam()) - GC.getGameINLINE().getTeamRank(getTeam());
	iDiscount += 5 * iRankDelta / std::max(6, GC.getGameINLINE().countCivTeamsAlive());

	if (GET_PLAYER(eTrader).isHuman())
	{
		// note. humans get no discount for trades that they propose.
		// The discount here only applies to deals that the AI offers to the human.
		iDiscount /= 2;
	}
	return 100 - iDiscount;
}
// K-Mod end

int CvPlayerAI::AI_maxGoldTrade(PlayerTypes ePlayer) const
{
	int iMaxGold;
	int iResearchBuffer;

	FAssert(ePlayer != getID());

	if (isHuman() || (GET_PLAYER(ePlayer).getTeam() == getTeam()))
	{
		iMaxGold = getGold();
	}
	else
	{
		iMaxGold = getTotalPopulation();

		iMaxGold *= (GET_TEAM(getTeam()).AI_getHasMetCounter(GET_PLAYER(ePlayer).getTeam()) + 10);

		iMaxGold *= GC.getLeaderHeadInfo(getPersonalityType()).getMaxGoldTradePercent();
		iMaxGold /= 100;

		iMaxGold -= AI_getGoldTradedTo(ePlayer);

        iResearchBuffer = -calculateGoldRate() * 12;
        iResearchBuffer *= GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).getResearchPercent();
        iResearchBuffer /= 100;

		iMaxGold = std::min(iMaxGold, getGold() - iResearchBuffer);

		iMaxGold = std::min(iMaxGold, getGold());

		iMaxGold -= (iMaxGold % GC.getDefineINT("DIPLOMACY_VALUE_REMAINDER"));
	}

	return std::max(0, iMaxGold);
}


int CvPlayerAI::AI_maxGoldPerTurnTrade(PlayerTypes ePlayer) const
{
	int iMaxGoldPerTurn;

	FAssert(ePlayer != getID());

	if (isHuman() || (GET_PLAYER(ePlayer).getTeam() == getTeam()))
	{
		iMaxGoldPerTurn = (calculateGoldRate() + (getGold() / GC.getDefineINT("PEACE_TREATY_LENGTH")));
	}
	else
	{
		iMaxGoldPerTurn = getTotalPopulation();

		iMaxGoldPerTurn *= GC.getLeaderHeadInfo(getPersonalityType()).getMaxGoldPerTurnTradePercent();
		iMaxGoldPerTurn /= 100;

		iMaxGoldPerTurn += std::min(0, getGoldPerTurnByPlayer(ePlayer));
	}

	return std::max(0, std::min(iMaxGoldPerTurn, calculateGoldRate()));
}


int CvPlayerAI::AI_goldPerTurnTradeVal(int iGoldPerTurn) const
{
	int iValue = iGoldPerTurn * GC.getDefineINT("PEACE_TREATY_LENGTH");
	iValue *= AI_goldTradeValuePercent();
	iValue /= 100;
	
	return iValue;
}

int CvPlayerAI::AI_bonusVal(BonusTypes eBonus, int iChange) const
{
	int iValue = 0;
	int iBonusCount = getNumAvailableBonuses(eBonus);
	if ((iChange == 0) || ((iChange == 1) && (iBonusCount == 0)) || ((iChange == -1) && (iBonusCount == 1)))
	{
		//This is assuming the none-to-one or one-to-none case.
		iValue += AI_baseBonusVal(eBonus);
		iValue += AI_corporationBonusVal(eBonus);
	}
	else
	{
		//This is basically the marginal value of an additional instance of a bonus.
		iValue += AI_baseBonusVal(eBonus) / 5;
		iValue += AI_corporationBonusVal(eBonus);		
	}
	return iValue;
}

//Value sans corporation
int CvPlayerAI::AI_baseBonusVal(BonusTypes eBonus) const
{
	PROFILE_FUNC();

	//recalculate if not defined
	if(m_aiBonusValue[eBonus] == -1)
	{
		PROFILE("CvPlayerAI::AI_baseBonusVal::recalculate");

		UnitTypes eLoopUnit;
		BuildingTypes eLoopBuilding;
		int iDiff;
		int iValue = 0;
		int iTempValue;
		int iI, iJ;

		if (!GET_TEAM(getTeam()).isBonusObsolete(eBonus))
		{
			iValue += (GC.getBonusInfo(eBonus).getHappiness() * 100);
			iValue += (GC.getBonusInfo(eBonus).getHealth() * 100);

			CvTeam& kTeam = GET_TEAM(getTeam());

			CvCity* pCapital = getCapitalCity();
			int iCityCount = getNumCities();
			int iCoastalCityCount = countNumCoastalCities();
			
			// find the first coastal city
			CvCity* pCoastalCity = NULL;
			CvCity* pUnconnectedCoastalCity = NULL;
			if (iCoastalCityCount > 0)
			{
				int iLoop;
					for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
				{
					if (pLoopCity->isCoastal(GC.getMIN_WATER_SIZE_FOR_OCEAN()))
					{
						if (pLoopCity->isConnectedToCapital(getID()))
						{
							pCoastalCity = pLoopCity;
								break;
						}
						else if (pUnconnectedCoastalCity == NULL)
						{
							pUnconnectedCoastalCity = pLoopCity;
						}
					}
				}
			}
			if (pCoastalCity == NULL && pUnconnectedCoastalCity != NULL)
			{
				pCoastalCity = pUnconnectedCoastalCity;
			}


			for (iI = 0; iI < GC.getNumUnitClassInfos(); iI++)
			{
				eLoopUnit = ((UnitTypes)(GC.getCivilizationInfo(getCivilizationType()).getCivilizationUnits(iI)));

				if (eLoopUnit != NO_UNIT)
				{
					CvUnitInfo& kLoopUnit = GC.getUnitInfo(eLoopUnit);

					iTempValue = 0;

					if (kLoopUnit.getPrereqAndBonus() == eBonus)
					{
						iTempValue += 50;
					}

					for (iJ = 0; iJ < GC.getNUM_UNIT_PREREQ_OR_BONUSES(); iJ++)
					{
						if (kLoopUnit.getPrereqOrBonuses(iJ) == eBonus)
						{
							iTempValue += 40;
						}
					}

					iTempValue += kLoopUnit.getBonusProductionModifier(eBonus) / 10;

					if (iTempValue > 0)
					{
						bool bIsWater = (kLoopUnit.getDomainType() == DOMAIN_SEA);
						
						// if non-limited water unit, weight by coastal cities
						if (bIsWater && !isLimitedUnitClass((UnitClassTypes)(kLoopUnit.getUnitClassType())))
						{
							iTempValue *= std::min(iCoastalCityCount * 2, iCityCount);	// double coastal cities, cap at total cities
							iTempValue /= std::max(1, iCityCount);
						}

						if (canTrain(eLoopUnit))
						{
							// is it a water unit and no coastal cities or our coastal city cannot build because its obsolete
							if ((bIsWater && (pCoastalCity == NULL || (pCoastalCity->allUpgradesAvailable(eLoopUnit) != NO_UNIT))) ||
								// or our capital cannot build because its obsolete (we can already build all its upgrades)
								(pCapital != NULL && pCapital->allUpgradesAvailable(eLoopUnit) != NO_UNIT))
							{
								// its worthless
								iTempValue = 2;
							}
							// otherwise, value units we could build if we had this bonus double
							else
							{
								iTempValue *= 2;
							}
						}

						if (kLoopUnit.getPrereqAndTech() != NO_TECH)
						{
							iDiff = abs(GC.getTechInfo((TechTypes)(kLoopUnit.getPrereqAndTech())).getEra() - getCurrentEra());

							if (iDiff == 0)
							{
								iTempValue *= 3;
								iTempValue /= 2;
							}
							else
							{
								iTempValue /= iDiff;
							}
						}

						iValue += iTempValue;
					}
				}
			}

			for (iI = 0; iI < GC.getNumBuildingClassInfos(); iI++)
			{
				eLoopBuilding = ((BuildingTypes)(GC.getCivilizationInfo(getCivilizationType()).getCivilizationBuildings(iI)));

				if (eLoopBuilding != NO_BUILDING)
				{
					CvBuildingInfo& kLoopBuilding = GC.getBuildingInfo(eLoopBuilding);
					
					iTempValue = 0;

					if (kLoopBuilding.getPrereqAndBonus() == eBonus)
					{
						iTempValue += 30;
					}

					for (iJ = 0; iJ < GC.getNUM_BUILDING_PREREQ_OR_BONUSES(); iJ++)
					{
						if (kLoopBuilding.getPrereqOrBonuses(iJ) == eBonus)
						{
							iTempValue += 20;
						}
					}

					iTempValue += kLoopBuilding.getBonusProductionModifier(eBonus) / 10;

					if (kLoopBuilding.getPowerBonus() == eBonus)
					{
						iTempValue += 60;
					}
					
					for (iJ = 0; iJ < NUM_YIELD_TYPES; iJ++)
					{
						iTempValue += kLoopBuilding.getBonusYieldModifier(eBonus, iJ) / 2;
						if (kLoopBuilding.getPowerBonus() == eBonus)
						{
							iTempValue += kLoopBuilding.getPowerYieldModifier(iJ);
						}
					}
					
					{
						// determine whether we have the tech for this building
						bool bHasTechForBuilding = true;
						if (!(kTeam.isHasTech((TechTypes)(kLoopBuilding.getPrereqAndTech()))))
						{
							bHasTechForBuilding = false;
						}
						for (int iPrereqIndex = 0; bHasTechForBuilding && iPrereqIndex < GC.getNUM_BUILDING_AND_TECH_PREREQS(); iPrereqIndex++)
						{
							if (kLoopBuilding.getPrereqAndTechs(iPrereqIndex) != NO_TECH)
							{
								if (!(kTeam.isHasTech((TechTypes)(kLoopBuilding.getPrereqAndTechs(iPrereqIndex)))))
								{
									bHasTechForBuilding = false;
								}
							}
						}
						
						bool bIsStateReligion = (((ReligionTypes) kLoopBuilding.getStateReligion()) != NO_RELIGION);

						//check if function call is cached
						bool bCanConstruct = canConstruct(eLoopBuilding, false, /*bTestVisible*/ true, /*bIgnoreCost*/ true);
						
						// bCanNeverBuild when true is accurate, it may be false in some cases where we will never be able to build 
						bool bCanNeverBuild = (bHasTechForBuilding && !bCanConstruct && !bIsStateReligion);

						// if we can never build this, it is worthless
						if (bCanNeverBuild)
						{
							iTempValue = 0;
						}
						// double value if we can build it right now
						else if (bCanConstruct)
						{
							iTempValue *= 2;
						}

						// if non-limited water building, weight by coastal cities
						if (kLoopBuilding.isWater() && !isLimitedWonderClass((BuildingClassTypes)(kLoopBuilding.getBuildingClassType())))
						{
							iTempValue *= iCoastalCityCount;
							iTempValue /= std::max(1, iCityCount/2);
						}

						if (kLoopBuilding.getPrereqAndTech() != NO_TECH)
						{
							iDiff = abs(GC.getTechInfo((TechTypes)(kLoopBuilding.getPrereqAndTech())).getEra() - getCurrentEra());

							if (iDiff == 0)
							{
								iTempValue *= 3;
								iTempValue /= 2;
							}
							else
							{
								iTempValue /= iDiff;
							}
						}

						iValue += iTempValue;
					}
				}
			}

			for (iI = 0; iI < GC.getNumProjectInfos(); iI++)
			{
				ProjectTypes eProject = (ProjectTypes) iI;
				CvProjectInfo& kLoopProject = GC.getProjectInfo(eProject);
				iTempValue = 0;

				iTempValue += kLoopProject.getBonusProductionModifier(eBonus) / 10;

				if (iTempValue > 0)
				{
					bool bMaxedOut = (GC.getGameINLINE().isProjectMaxedOut(eProject) || kTeam.isProjectMaxedOut(eProject));

					if (bMaxedOut)
					{
						// project worthless
						iTempValue = 0;
					}
					else if (canCreate(eProject))
					{
						iTempValue *= 2;
					}

					if (kLoopProject.getTechPrereq() != NO_TECH)
					{
						iDiff = abs(GC.getTechInfo((TechTypes)(kLoopProject.getTechPrereq())).getEra() - getCurrentEra());

						if (iDiff == 0)
						{
							iTempValue *= 3;
							iTempValue /= 2;
						}
						else
						{
							iTempValue /= iDiff;
						}
					}

					iValue += iTempValue;
				}
			}

			RouteTypes eBestRoute = getBestRoute();
			for (iI = 0; iI < GC.getNumBuildInfos(); iI++)
			{
				RouteTypes eRoute = (RouteTypes)(GC.getBuildInfo((BuildTypes)iI).getRoute());

				if (eRoute != NO_ROUTE)
				{
					iTempValue = 0;
					if (GC.getRouteInfo(eRoute).getPrereqBonus() == eBonus)
					{
						iTempValue += 80;
					}
					for (iJ = 0; iJ < GC.getNUM_ROUTE_PREREQ_OR_BONUSES(); iJ++)
					{
						if (GC.getRouteInfo(eRoute).getPrereqOrBonus(iJ) == eBonus)
						{
							iTempValue += 40;
						}
					}
					if ((eBestRoute != NO_ROUTE) && (GC.getRouteInfo(getBestRoute()).getValue() <= GC.getRouteInfo(eRoute).getValue()))
					{
						iValue += iTempValue;
					}
					else
					{
						iValue += iTempValue / 2;
					}
				}
			}

			//	int iCorporationValue = AI_corporationBonusVal(eBonus);
			//	iValue += iCorporationValue;
			//
			//	if (iCorporationValue <= 0 && getNumAvailableBonuses(eBonus) > 0)
			//	{
			//		iValue /= 3;
			//	}

			iValue /= 10;
		}


		//clamp value non-negative
		m_aiBonusValue[eBonus] = std::max(0, iValue);
	}

	return m_aiBonusValue[eBonus];
}

int CvPlayerAI::AI_corporationBonusVal(BonusTypes eBonus) const
{
	int iValue = 0;
	int iCityCount = getNumCities();
	iCityCount += iCityCount / 6 + 1;

	for (int iCorporation = 0; iCorporation < GC.getNumCorporationInfos(); ++iCorporation)
	{
		int iCorpCount = getHasCorporationCount((CorporationTypes)iCorporation);
		if (iCorpCount > 0)
		{
			int iNumCorpBonuses = 0;
			iCorpCount += getNumCities() / 6 + 1;
			CvCorporationInfo& kCorp = GC.getCorporationInfo((CorporationTypes)iCorporation);
			for (int i = 0; i < GC.getNUM_CORPORATION_PREREQ_BONUSES(); ++i)
			{
				if (eBonus == kCorp.getPrereqBonus(i))
				{
					iValue += (50 * kCorp.getYieldProduced(YIELD_FOOD) * iCorpCount) / iCityCount;
					iValue += (50 * kCorp.getYieldProduced(YIELD_PRODUCTION) * iCorpCount) / iCityCount;
					iValue += (30 * kCorp.getYieldProduced(YIELD_COMMERCE) * iCorpCount) / iCityCount;

					iValue += (30 * kCorp.getCommerceProduced(COMMERCE_GOLD) * iCorpCount) / iCityCount;
					iValue += (30 * kCorp.getCommerceProduced(COMMERCE_RESEARCH) * iCorpCount) / iCityCount;
					//iValue += (12 * kCorp.getCommerceProduced(COMMERCE_CULTURE) * iCorpCount) / iCityCount;
					// K-Mod, I'd love to calculate this stuff properly, but because of the way trade currently operates...
					iValue += (20 * kCorp.getCommerceProduced(COMMERCE_CULTURE) * iCorpCount) / iCityCount;
					iValue += (20 * kCorp.getCommerceProduced(COMMERCE_ESPIONAGE) * iCorpCount) / iCityCount;
					
					//Disabled since you can't found/spread a corp unless there is already a bonus,
					//and that bonus will provide the entirity of the bonusProduced benefit.

					/*if (NO_BONUS != kCorp.getBonusProduced())
					{
						if (getNumAvailableBonuses((BonusTypes)kCorp.getBonusProduced()) == 0)
						{
							iBonusValue += (1000 * iCorpCount * AI_baseBonusVal((BonusTypes)kCorp.getBonusProduced())) / (10 * iCityCount);
					}
					}*/
				}
			}
		}
	}

	iValue /= 100;	//percent
	iValue /= 10;	//match AI_baseBonusVal

	return iValue;
}


int CvPlayerAI::AI_bonusTradeVal(BonusTypes eBonus, PlayerTypes ePlayer, int iChange) const
{
	int iValue;

	FAssertMsg(ePlayer != getID(), "shouldn't call this function on ourselves");

	iValue = AI_bonusVal(eBonus, iChange);

	iValue *= ((std::min(getNumCities(), GET_PLAYER(ePlayer).getNumCities()) + 3) * 30);
	iValue /= 100;

	iValue *= std::max(0, (GC.getBonusInfo(eBonus).getAITradeModifier() + 100));
	iValue /= 100;

	if (GET_TEAM(GET_PLAYER(ePlayer).getTeam()).isVassal(getTeam()) && !GET_TEAM(GET_PLAYER(ePlayer).getTeam()).isCapitulated())
	{
		iValue /= 2;
	}

	return (iValue * GC.getDefineINT("PEACE_TREATY_LENGTH"));
}


DenialTypes CvPlayerAI::AI_bonusTrade(BonusTypes eBonus, PlayerTypes ePlayer) const
{
	PROFILE_FUNC();

	AttitudeTypes eAttitude;
	bool bStrategic;
	int iI, iJ;

	FAssertMsg(ePlayer != getID(), "shouldn't call this function on ourselves");

	if (isHuman() && GET_PLAYER(ePlayer).isHuman())
	{
		return NO_DENIAL;
	}

	if (GET_TEAM(getTeam()).isVassal(GET_PLAYER(ePlayer).getTeam()))
	{
		return NO_DENIAL;
	}

	if (atWar(getTeam(), GET_PLAYER(ePlayer).getTeam()))
	{
		return NO_DENIAL;
	}

	//if (GET_PLAYER(ePlayer).getTeam() == getTeam())
	if (GET_PLAYER(ePlayer).getTeam() == getTeam() && GET_PLAYER(ePlayer).isHuman()) // K-Mod
	{
		return NO_DENIAL;
	}

	if (GET_PLAYER(ePlayer).getNumAvailableBonuses(eBonus) > 0 && GET_PLAYER(ePlayer).AI_corporationBonusVal(eBonus) <= 0)
	{
		return (GET_PLAYER(ePlayer).isHuman() ? DENIAL_JOKING : DENIAL_NO_GAIN);
	}

	// K-Mod (The above case should be tested for humans trying to give stuff to AI teammates
	// - otherwise the human won't know if the AI can actually use the resource.)
	if (GET_PLAYER(ePlayer).getTeam() == getTeam())
		return NO_DENIAL;
	// K-Mod end

	if (isHuman())
	{
		return NO_DENIAL;
	}

	if (GET_TEAM(getTeam()).AI_getWorstEnemy() == GET_PLAYER(ePlayer).getTeam())
	{
		return DENIAL_WORST_ENEMY;
	}

	if (AI_corporationBonusVal(eBonus) > 0)
	{
		return DENIAL_JOKING;
	}

	bStrategic = false;

	for (iI = 0; iI < GC.getNumUnitInfos(); iI++)
	{
		if (GC.getUnitInfo((UnitTypes) iI).getPrereqAndBonus() == eBonus)
		{
			bStrategic = true;
		}

		for (iJ = 0; iJ < GC.getNUM_UNIT_PREREQ_OR_BONUSES(); iJ++)
		{
			if (GC.getUnitInfo((UnitTypes) iI).getPrereqOrBonuses(iJ) == eBonus)
			{
				bStrategic = true;
			}
		}
	}

	for (iI = 0; iI < GC.getNumBuildingInfos(); iI++)
	{
		if (GC.getBuildingInfo((BuildingTypes) iI).getPrereqAndBonus() == eBonus)
		{
			bStrategic = true;
		}

		for (iJ = 0; iJ < GC.getNUM_BUILDING_PREREQ_OR_BONUSES(); iJ++)
		{
			if (GC.getBuildingInfo((BuildingTypes) iI).getPrereqOrBonuses(iJ) == eBonus)
			{
				bStrategic = true;
			}
		}
	}

	// XXX marble and stone???

	eAttitude = AI_getAttitude(ePlayer);

	if (bStrategic)
	{
		if (eAttitude <= GC.getLeaderHeadInfo(getPersonalityType()).getStrategicBonusRefuseAttitudeThreshold())
		{
			return DENIAL_ATTITUDE;
		}
	}

	if (GC.getBonusInfo(eBonus).getHappiness() > 0)
	{
		if (eAttitude <= GC.getLeaderHeadInfo(getPersonalityType()).getHappinessBonusRefuseAttitudeThreshold())
		{
			return DENIAL_ATTITUDE;
		}
	}

	if (GC.getBonusInfo(eBonus).getHealth() > 0)
	{
		if (eAttitude <= GC.getLeaderHeadInfo(getPersonalityType()).getHealthBonusRefuseAttitudeThreshold())
		{
			return DENIAL_ATTITUDE;
		}
	}

	return NO_DENIAL;
}


int CvPlayerAI::AI_cityTradeVal(CvCity* pCity) const
{
	CvPlot* pLoopPlot;
	int iValue;
	int iI;

	FAssert(pCity->getOwnerINLINE() != getID());

	iValue = 300;

	iValue += (pCity->getPopulation() * 50);

	iValue += (pCity->getCultureLevel() * 200);

	iValue += (((((pCity->getPopulation() * 50) + GC.getGameINLINE().getElapsedGameTurns() + 100) * 4) * pCity->plot()->calculateCulturePercent(pCity->getOwnerINLINE())) / 100);

	for (iI = 0; iI < NUM_CITY_PLOTS; iI++)
	{
		pLoopPlot = plotCity(pCity->getX_INLINE(), pCity->getY_INLINE(), iI);

		if (pLoopPlot != NULL)
		{
			if (pLoopPlot->getBonusType(getTeam()) != NO_BONUS)
			{
				iValue += (AI_bonusVal(pLoopPlot->getBonusType(getTeam())) * 10);
			}
		}
	}

	if (!(pCity->isEverOwned(getID())))
	{
		iValue *= 3;
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


DenialTypes CvPlayerAI::AI_cityTrade(CvCity* pCity, PlayerTypes ePlayer) const
{
	CvCity* pNearestCity;

	FAssert(pCity->getOwnerINLINE() == getID());

	if (pCity->getLiberationPlayer(false) == ePlayer)
	{
		return NO_DENIAL;
	}

	if (!(GET_PLAYER(ePlayer).isHuman()))
	{
		if (GET_PLAYER(ePlayer).getTeam() != getTeam())
		{
			if ((pCity->plot()->calculateCulturePercent(ePlayer) == 0) && !(pCity->isEverOwned(ePlayer)) && (GET_PLAYER(ePlayer).getNumCities() > 3))
			{
				if (GET_PLAYER(ePlayer).AI_isFinancialTrouble())
				{
					return DENIAL_UNKNOWN;
				}

				pNearestCity = GC.getMapINLINE().findCity(pCity->getX_INLINE(), pCity->getY_INLINE(), ePlayer, NO_TEAM, true, false, NO_TEAM, NO_DIRECTION, pCity);

				if ((pNearestCity == NULL) || (plotDistance(pCity->getX_INLINE(), pCity->getY_INLINE(), pNearestCity->getX_INLINE(), pNearestCity->getY_INLINE()) > 9))
				{
					return DENIAL_UNKNOWN;
				}
			}
		}
	}

	if (isHuman())
	{
		return NO_DENIAL;
	}

	if (atWar(getTeam(), GET_PLAYER(ePlayer).getTeam()))
	{
		return NO_DENIAL;
	}

	if (GET_PLAYER(ePlayer).getTeam() != getTeam())
	{
		return DENIAL_NEVER;
	}

	if (pCity->calculateCulturePercent(getID()) > 50)
	{
		return DENIAL_TOO_MUCH;
	}

	return NO_DENIAL;
}


int CvPlayerAI::AI_stopTradingTradeVal(TeamTypes eTradeTeam, PlayerTypes ePlayer) const
{
	CvDeal* pLoopDeal;
	int iModifier;
	int iValue;
	int iLoop;

	FAssertMsg(ePlayer != getID(), "shouldn't call this function on ourselves");
	FAssertMsg(GET_PLAYER(ePlayer).getTeam() != getTeam(), "shouldn't call this function on ourselves");
	FAssertMsg(eTradeTeam != getTeam(), "shouldn't call this function on ourselves");
	FAssertMsg(GET_TEAM(eTradeTeam).isAlive(), "GET_TEAM(eWarTeam).isAlive is expected to be true");
	FAssertMsg(!atWar(eTradeTeam, GET_PLAYER(ePlayer).getTeam()), "eTeam should be at peace with eWarTeam");

	iValue = (50 + (GC.getGameINLINE().getGameTurn() / 2));
	iValue += (GET_TEAM(eTradeTeam).getNumCities() * 5);

	iModifier = 0;

	switch (GET_TEAM(GET_PLAYER(ePlayer).getTeam()).AI_getAttitude(eTradeTeam))
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
		iModifier += 100;
		break;

	case ATTITUDE_FRIENDLY:
		iModifier += 200;
		break;

	default:
		FAssert(false);
		break;
	}

	iValue *= std::max(0, (iModifier + 100));
	iValue /= 100;

	if (GET_TEAM(GET_PLAYER(ePlayer).getTeam()).isOpenBorders(eTradeTeam))
	{
		iValue *= 2;
	}

	if (GET_TEAM(GET_PLAYER(ePlayer).getTeam()).isDefensivePact(eTradeTeam))
	{
		iValue *= 3;
	}

	for(pLoopDeal = GC.getGameINLINE().firstDeal(&iLoop); pLoopDeal != NULL; pLoopDeal = GC.getGameINLINE().nextDeal(&iLoop))
	{
		if (pLoopDeal->isCancelable(getID()) && !(pLoopDeal->isPeaceDeal()))
		{
			if (GET_PLAYER(pLoopDeal->getFirstPlayer()).getTeam() == GET_PLAYER(ePlayer).getTeam())
			{
				if (pLoopDeal->getLengthSecondTrades() > 0)
				{
					iValue += (GET_PLAYER(pLoopDeal->getFirstPlayer()).AI_dealVal(pLoopDeal->getSecondPlayer(), pLoopDeal->getSecondTrades()) * ((pLoopDeal->getLengthFirstTrades() == 0) ? 2 : 1));
				}
			}

			if (GET_PLAYER(pLoopDeal->getSecondPlayer()).getTeam() == GET_PLAYER(ePlayer).getTeam())
			{
				if (pLoopDeal->getLengthFirstTrades() > 0)
				{
					iValue += (GET_PLAYER(pLoopDeal->getSecondPlayer()).AI_dealVal(pLoopDeal->getFirstPlayer(), pLoopDeal->getFirstTrades()) * ((pLoopDeal->getLengthSecondTrades() == 0) ? 2 : 1));
				}
			}
		}
	}

	if (GET_TEAM(GET_PLAYER(ePlayer).getTeam()).isVassal(getTeam()))
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


DenialTypes CvPlayerAI::AI_stopTradingTrade(TeamTypes eTradeTeam, PlayerTypes ePlayer) const
{
	AttitudeTypes eAttitude;
	AttitudeTypes eAttitudeThem;
	int iI;

	FAssertMsg(ePlayer != getID(), "shouldn't call this function on ourselves");
	FAssertMsg(GET_PLAYER(ePlayer).getTeam() != getTeam(), "shouldn't call this function on ourselves");
	FAssertMsg(eTradeTeam != getTeam(), "shouldn't call this function on ourselves");
	FAssertMsg(GET_TEAM(eTradeTeam).isAlive(), "GET_TEAM(eTradeTeam).isAlive is expected to be true");
	FAssertMsg(!atWar(getTeam(), eTradeTeam), "should be at peace with eTradeTeam");

	if (isHuman())
	{
		return NO_DENIAL;
	}

	if (GET_TEAM(getTeam()).isVassal(GET_PLAYER(ePlayer).getTeam()))
	{
		return NO_DENIAL;
	}

	if (GET_TEAM(getTeam()).isVassal(eTradeTeam))
	{
		return DENIAL_POWER_THEM;
	}

	eAttitude = GET_TEAM(getTeam()).AI_getAttitude(GET_PLAYER(ePlayer).getTeam());

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getTeam())
			{
				if (eAttitude <= GC.getLeaderHeadInfo(GET_PLAYER((PlayerTypes)iI).getPersonalityType()).getStopTradingRefuseAttitudeThreshold())
				{
					return DENIAL_ATTITUDE;
				}
			}
		}
	}

	eAttitudeThem = GET_TEAM(getTeam()).AI_getAttitude(eTradeTeam);

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() == getTeam())
			{
				if (eAttitudeThem > GC.getLeaderHeadInfo(GET_PLAYER((PlayerTypes)iI).getPersonalityType()).getStopTradingThemRefuseAttitudeThreshold())
				{
					return DENIAL_ATTITUDE_THEM;
				}
			}
		}
	}

	return NO_DENIAL;
}


int CvPlayerAI::AI_civicTradeVal(CivicTypes eCivic, PlayerTypes ePlayer) const
{
	CivicTypes eBestCivic;
	int iValue;

	iValue = (2 * (getTotalPopulation() + GET_PLAYER(ePlayer).getTotalPopulation())); // XXX

	eBestCivic = GET_PLAYER(ePlayer).AI_bestCivic((CivicOptionTypes)(GC.getCivicInfo(eCivic).getCivicOptionType()));

	if (eBestCivic != NO_CIVIC)
	{
		if (eBestCivic != eCivic)
		{
			iValue += std::max(0, ((GET_PLAYER(ePlayer).AI_civicValue(eBestCivic) - GET_PLAYER(ePlayer).AI_civicValue(eCivic)) * 2));
		}
	}

	if (GET_TEAM(GET_PLAYER(ePlayer).getTeam()).isVassal(getTeam()))
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


DenialTypes CvPlayerAI::AI_civicTrade(CivicTypes eCivic, PlayerTypes ePlayer) const
{
	if (isHuman())
	{
		return NO_DENIAL;
	}

	if (GET_TEAM(getTeam()).isVassal(GET_PLAYER(ePlayer).getTeam()))
	{
		return NO_DENIAL;
	}

	if (atWar(getTeam(), GET_PLAYER(ePlayer).getTeam()))
	{
		return NO_DENIAL;
	}

	if (GET_PLAYER(ePlayer).getTeam() == getTeam())
	{
		return NO_DENIAL;
	}

	if (getCivicPercentAnger(getCivics((CivicOptionTypes)(GC.getCivicInfo(eCivic).getCivicOptionType())),true) > getCivicPercentAnger(eCivic))
	{
		return DENIAL_ANGER_CIVIC;
	}

	CivicTypes eFavoriteCivic = (CivicTypes)GC.getLeaderHeadInfo(getPersonalityType()).getFavoriteCivic();
	if (eFavoriteCivic != NO_CIVIC)
	{
		if (isCivic(eFavoriteCivic) && (GC.getCivicInfo(eCivic).getCivicOptionType() == GC.getCivicInfo(eFavoriteCivic).getCivicOptionType()))
		{
			return DENIAL_FAVORITE_CIVIC;
		}
	}

	if (GC.getCivilizationInfo(getCivilizationType()).getCivilizationInitialCivics(GC.getCivicInfo(eCivic).getCivicOptionType()) == eCivic)
	{
		return DENIAL_JOKING;
	}

	if (AI_getAttitude(ePlayer) <= GC.getLeaderHeadInfo(getPersonalityType()).getAdoptCivicRefuseAttitudeThreshold())
	{
		return DENIAL_ATTITUDE;
	}

	return NO_DENIAL;
}


int CvPlayerAI::AI_religionTradeVal(ReligionTypes eReligion, PlayerTypes ePlayer) const
{
	ReligionTypes eBestReligion;
	int iValue;

	iValue = (3 * (getTotalPopulation() + GET_PLAYER(ePlayer).getTotalPopulation())); // XXX

	eBestReligion = GET_PLAYER(ePlayer).AI_bestReligion();

	if (eBestReligion != NO_RELIGION)
	{
		if (eBestReligion != eReligion)
		{
			//iValue += std::max(0, (GET_PLAYER(ePlayer).AI_religionValue(eBestReligion) - GET_PLAYER(ePlayer).AI_religionValue(eReligion)));
			// K-Mod. AI_religionValue has arbitrary units.
			// We need to give it some kind of scale for it to be meaningful in this function.
			iValue *= 100 + std::min(100, 100 * GET_PLAYER(ePlayer).AI_religionValue(eBestReligion) / std::max(1, GET_PLAYER(ePlayer).AI_religionValue(eReligion)));
			iValue /= 100;
			// K-Mod end
		}
	}

	if (GET_TEAM(GET_PLAYER(ePlayer).getTeam()).isVassal(getTeam()))
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


DenialTypes CvPlayerAI::AI_religionTrade(ReligionTypes eReligion, PlayerTypes ePlayer) const
{
	if (isHuman())
	{
		return NO_DENIAL;
	}

	if (GET_TEAM(getTeam()).isVassal(GET_PLAYER(ePlayer).getTeam()))
	{
		return NO_DENIAL;
	}

	if (atWar(getTeam(), GET_PLAYER(ePlayer).getTeam()))
	{
		return NO_DENIAL;
	}

	if (GET_PLAYER(ePlayer).getTeam() == getTeam())
	{
		return NO_DENIAL;
	}

	if (getStateReligion() != NO_RELIGION)
	{
		if (getHasReligionCount(eReligion) < std::min((getHasReligionCount(getStateReligion()) - 1), (getNumCities() / 2)))
		{
			return DENIAL_MINORITY_RELIGION;
		}
	}

	if (AI_getAttitude(ePlayer) <= GC.getLeaderHeadInfo(getPersonalityType()).getConvertReligionRefuseAttitudeThreshold())
	{
		return DENIAL_ATTITUDE;
	}

	return NO_DENIAL;
}


int CvPlayerAI::AI_unitImpassableCount(UnitTypes eUnit) const
{
	int iCount = 0;
	for (int iI = 0; iI < GC.getNumTerrainInfos(); iI++)
	{
		if (GC.getUnitInfo(eUnit).getTerrainImpassable(iI))
		{
			TechTypes eTech = (TechTypes)GC.getUnitInfo(eUnit).getTerrainPassableTech(iI);
			if (NO_TECH == eTech || !GET_TEAM(getTeam()).isHasTech(eTech))
			{
				iCount++;
			}
		}
	}

	for (int iI = 0; iI < GC.getNumFeatureInfos(); iI++)
	{
		if (GC.getUnitInfo(eUnit).getFeatureImpassable(iI))
		{
			TechTypes eTech = (TechTypes)GC.getUnitInfo(eUnit).getFeaturePassableTech(iI);
			if (NO_TECH == eTech || !GET_TEAM(getTeam()).isHasTech(eTech))
			{
				iCount++;
			}
		}
	}

	return iCount;
}


int CvPlayerAI::AI_unitValue(UnitTypes eUnit, UnitAITypes eUnitAI, CvArea* pArea) const
{
	PROFILE_FUNC();

	bool bValid;
	int iNeededMissionaries;
	int iCombatValue;
	int iValue;
	int iTempValue;
	int iI;

	FAssertMsg(eUnit != NO_UNIT, "Unit is not assigned a valid value");
	FAssertMsg(eUnitAI != NO_UNITAI, "UnitAI is not assigned a valid value");

	if (GC.getUnitInfo(eUnit).getDomainType() != AI_unitAIDomainType(eUnitAI))
	{
		if (eUnitAI != UNITAI_ICBM)//XXX
		{
			return 0;
		}
	}

	if (GC.getUnitInfo(eUnit).getNotUnitAIType(eUnitAI))
	{
		return 0;
	}

	bValid = GC.getUnitInfo(eUnit).getUnitAIType(eUnitAI);

	if (!bValid)
	{
		switch (eUnitAI)
		{
		case UNITAI_UNKNOWN:
			break;

		case UNITAI_ANIMAL:
			if (GC.getUnitInfo(eUnit).isAnimal())
			{
				bValid = true;
			}
			break;

		case UNITAI_SETTLE:
			if (GC.getUnitInfo(eUnit).isFound())
			{
				bValid = true;
			}
			break;

		case UNITAI_WORKER:
			for (iI = 0; iI < GC.getNumBuildInfos(); iI++)
			{
				if (GC.getUnitInfo(eUnit).getBuilds(iI))
				{
					bValid = true;
					break;
				}
			}
			break;

		case UNITAI_ATTACK:
			if (GC.getUnitInfo(eUnit).getCombat() > 0)
			{
				if (!(GC.getUnitInfo(eUnit).isOnlyDefensive()))
				{
					bValid = true;
				}
			}
			break;

		case UNITAI_ATTACK_CITY:
			if (GC.getUnitInfo(eUnit).getCombat() > 0)
			{
				if (!(GC.getUnitInfo(eUnit).isOnlyDefensive()))
				{
					if (!(GC.getUnitInfo(eUnit).isNoCapture()))
					{
						bValid = true;
					}
				}
			}
			break;

		case UNITAI_COLLATERAL:
			if (GC.getUnitInfo(eUnit).getCombat() > 0)
			{
				if (!(GC.getUnitInfo(eUnit).isOnlyDefensive()))
				{
					if (GC.getUnitInfo(eUnit).getCollateralDamage() > 0)
					{
						bValid = true;
					}
				}
			}
			break;

		case UNITAI_PILLAGE:
			if (GC.getUnitInfo(eUnit).getCombat() > 0)
			{
				if (!(GC.getUnitInfo(eUnit).isOnlyDefensive()))
				{
					bValid = true;
				}
			}
			break;

		case UNITAI_RESERVE:
			if (GC.getUnitInfo(eUnit).getCombat() > 0)
			{
				if (!(GC.getUnitInfo(eUnit).isOnlyDefensive()))
				{
						bValid = true;
					}
				}
			break;

		case UNITAI_COUNTER:
			if (GC.getUnitInfo(eUnit).getCombat() > 0)
			{
				if (!(GC.getUnitInfo(eUnit).isOnlyDefensive()))
				{
					if (GC.getUnitInfo(eUnit).getInterceptionProbability() > 0)
					{
						bValid = true;
					}

					if (!bValid)
					{
						for (iI = 0; iI < GC.getNumUnitClassInfos(); iI++)
						{
							if (GC.getUnitInfo(eUnit).getUnitClassAttackModifier(iI) > 0)
							{
								bValid = true;
								break;
							}

							if (GC.getUnitInfo(eUnit).getTargetUnitClass(iI))
							{
								bValid = true;
								break;
							}
						}
					}

					if (!bValid)
					{
						for (iI = 0; iI < GC.getNumUnitCombatInfos(); iI++)
						{
							if (GC.getUnitInfo(eUnit).getUnitCombatModifier(iI) > 0)
							{
								bValid = true;
								break;
							}

							if (GC.getUnitInfo(eUnit).getTargetUnitCombat(iI))
							{
								bValid = true;
								break;
							}
						}
					}

					if (!bValid)
					{

						for (iI = 0; iI < GC.getNumUnitInfos(); iI++)
						{
							int iUnitClass = GC.getUnitInfo(eUnit).getUnitClassType();
							if (NO_UNITCLASS != iUnitClass && GC.getUnitInfo((UnitTypes)iI).getDefenderUnitClass(iUnitClass))
							{
								bValid = true;
								break;
							}

							int iUnitCombat = GC.getUnitInfo(eUnit).getUnitCombatType();
							if (NO_UNITCOMBAT !=  iUnitCombat && GC.getUnitInfo((UnitTypes)iI).getDefenderUnitCombat(iUnitCombat))
							{
								bValid = true;
								break;
							}
						}
					}
				}
			}
			break;

		case UNITAI_CITY_DEFENSE:
			if (GC.getUnitInfo(eUnit).getCombat() > 0)
			{
				if (!(GC.getUnitInfo(eUnit).isNoDefensiveBonus()))
				{
					if (GC.getUnitInfo(eUnit).getCityDefenseModifier() > 0)
					{
						bValid = true;
					}
				}
			}
			break;

		case UNITAI_CITY_COUNTER:
			if (GC.getUnitInfo(eUnit).getCombat() > 0)
			{
				if (!(GC.getUnitInfo(eUnit).isNoDefensiveBonus()))
				{
					if (GC.getUnitInfo(eUnit).getInterceptionProbability() > 0)
					{
						bValid = true;
					}

					if (!bValid)
					{
						for (iI = 0; iI < GC.getNumUnitClassInfos(); iI++)
						{
							if (GC.getUnitInfo(eUnit).getUnitClassDefenseModifier(iI) > 0)
							{
								bValid = true;
								break;
							}
						}
					}

					if (!bValid)
					{
						for (iI = 0; iI < GC.getNumUnitCombatInfos(); iI++)
						{
							if (GC.getUnitInfo(eUnit).getUnitCombatModifier(iI) > 0)
							{
								bValid = true;
								break;
							}
						}
					}
				}
			}
			break;

		case UNITAI_CITY_SPECIAL:
			break;

		case UNITAI_PARADROP:
			if (GC.getUnitInfo(eUnit).getDropRange() > 0)
			{
				bValid = true;
			}
			break;

		case UNITAI_EXPLORE:
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      12/09/09                                jdog5000      */
/*                                                                                              */
/* Unit AI                                                                                      */
/************************************************************************************************/
			if (GC.getUnitInfo(eUnit).getCombat() > 0 && !(GC.getUnitInfo(eUnit).isNoRevealMap()))
			{
				if (0 == AI_unitImpassableCount(eUnit))
				{
					bValid = true;
				}
			}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
			break;

		case UNITAI_MISSIONARY:
			if (pArea != NULL)
			{
				for (iI = 0; iI < GC.getNumReligionInfos(); iI++)
				{
					if (GC.getUnitInfo(eUnit).getReligionSpreads((ReligionTypes)iI) > 0)
					{
						iNeededMissionaries = AI_neededMissionaries(pArea, ((ReligionTypes)iI));

						if (iNeededMissionaries > 0)
						{
							if (iNeededMissionaries > countReligionSpreadUnits(pArea, ((ReligionTypes)iI)))
							{
								bValid = true;
								break;
							}
						}
					}
				}

				for (iI = 0; iI < GC.getNumCorporationInfos(); iI++)
				{
					if (GC.getUnitInfo(eUnit).getCorporationSpreads((CorporationTypes)iI) > 0)
					{
						iNeededMissionaries = AI_neededExecutives(pArea, ((CorporationTypes)iI));

						if (iNeededMissionaries > 0)
						{
							if (iNeededMissionaries > countCorporationSpreadUnits(pArea, ((CorporationTypes)iI)))
							{
								bValid = true;
								break;
							}
						}
					}
				}
			}
			break;

		case UNITAI_PROPHET:
		case UNITAI_ARTIST:
		case UNITAI_SCIENTIST:
		case UNITAI_GENERAL:
		case UNITAI_MERCHANT:
		case UNITAI_ENGINEER:
		case UNITAI_GREAT_SPY: // K-Mod
		case UNITAI_SPY:
			break;

		case UNITAI_ICBM:
			if (GC.getUnitInfo(eUnit).getNukeRange() != -1)
			{
				bValid = true;
			}
			break;

		case UNITAI_WORKER_SEA:
			for (iI = 0; iI < GC.getNumBuildInfos(); iI++)
			{
				if (GC.getUnitInfo(eUnit).getBuilds(iI))
				{
					bValid = true;
					break;
				}
			}
			break;

		case UNITAI_ATTACK_SEA:
			if (GC.getUnitInfo(eUnit).getCombat() > 0)
			{
				if (GC.getUnitInfo(eUnit).getCargoSpace() == 0)
				{
					if (!(GC.getUnitInfo(eUnit).isInvisible()) && (GC.getUnitInfo(eUnit).getInvisibleType() == NO_INVISIBLE))
					{
						bValid = true;
					}
				}
			}
			break;

		case UNITAI_RESERVE_SEA:
			if (GC.getUnitInfo(eUnit).getCombat() > 0)
			{
				if (GC.getUnitInfo(eUnit).getCargoSpace() == 0)
				{
					bValid = true;
				}
			}
			break;

		case UNITAI_ESCORT_SEA:
			if (GC.getUnitInfo(eUnit).getCombat() > 0)
			{
				if (GC.getUnitInfo(eUnit).getCargoSpace() == 0)
				{
					if (0 == AI_unitImpassableCount(eUnit))
					{
						bValid = true;
					}
				}
			}
			break;

		case UNITAI_EXPLORE_SEA:
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      12/09/09                                jdog5000      */
/*                                                                                              */
/* Unit AI                                                                                      */
/************************************************************************************************/
			if (GC.getUnitInfo(eUnit).getCargoSpace() <= 1 && !(GC.getUnitInfo(eUnit).isNoRevealMap()))
			{
				bValid = true;
			}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
			break;

		case UNITAI_ASSAULT_SEA:
		case UNITAI_SETTLER_SEA:
			if (GC.getUnitInfo(eUnit).getCargoSpace() > 0)
			{
				if (GC.getUnitInfo(eUnit).getSpecialCargo() == NO_SPECIALUNIT)
				{
					bValid = true;
				}
			}
			break;

		case UNITAI_MISSIONARY_SEA:
		case UNITAI_SPY_SEA:
		case UNITAI_CARRIER_SEA:
		case UNITAI_MISSILE_CARRIER_SEA:
			if (GC.getUnitInfo(eUnit).getCargoSpace() > 0)
			{
				if (GC.getUnitInfo(eUnit).getSpecialCargo() != NO_SPECIALUNIT)
				{
					for (int i = 0; i < NUM_UNITAI_TYPES; ++i)
					{
						if (GC.getSpecialUnitInfo((SpecialUnitTypes)GC.getUnitInfo(eUnit).getSpecialCargo()).isCarrierUnitAIType(eUnitAI))
						{
							bValid = true;
							break;
						}
					}
				}
			}
			break;

		case UNITAI_PIRATE_SEA:
			if (GC.getUnitInfo(eUnit).isAlwaysHostile() && GC.getUnitInfo(eUnit).isHiddenNationality())
			{
				bValid = true;
			}
			break;

		case UNITAI_ATTACK_AIR:
			if (GC.getUnitInfo(eUnit).getAirCombat() > 0)
			{
				if (!GC.getUnitInfo(eUnit).isSuicide())
				{
					bValid = true;
				}
			}
			break;

		case UNITAI_DEFENSE_AIR:
			if (GC.getUnitInfo(eUnit).getInterceptionProbability() > 0)
			{
				bValid = true;
			}
			break;

		case UNITAI_CARRIER_AIR:
			if (GC.getUnitInfo(eUnit).getAirCombat() > 0)
			{
				if (GC.getUnitInfo(eUnit).getInterceptionProbability() > 0)
				{
					bValid = true;
				}
			}
			break;

		case UNITAI_MISSILE_AIR:
			if (GC.getUnitInfo(eUnit).getAirCombat() > 0)
			{
				if (GC.getUnitInfo(eUnit).isSuicide())
				{
					bValid = true;
				}
			}
			break;

		case UNITAI_ATTACK_CITY_LEMMING:
			bValid = false;
			break;

		default:
			FAssert(false);
			break;
		}
	}

	if (!bValid)
	{
		return 0;
	}

	iCombatValue = GC.getGameINLINE().AI_combatValue(eUnit);

	iValue = 1;

	iValue += GC.getUnitInfo(eUnit).getAIWeight();
	
	int iFastMoverMultiplier;

	switch (eUnitAI)
	{
	case UNITAI_UNKNOWN:
	case UNITAI_ANIMAL:
		break;

	case UNITAI_SETTLE:
		iValue += (GC.getUnitInfo(eUnit).getMoves() * 100);
		break;

	case UNITAI_WORKER:
		for (iI = 0; iI < GC.getNumBuildInfos(); iI++)
		{
			if (GC.getUnitInfo(eUnit).getBuilds(iI))
			{
				iValue += 50;
			}
		}
		iValue += (GC.getUnitInfo(eUnit).getMoves() * 100);
		break;

	case UNITAI_ATTACK:
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      06/12/09                                jdog5000      */
/*                                                                                              */
/* Unit AI                                                                                      */
/************************************************************************************************/
		iFastMoverMultiplier = AI_isDoStrategy(AI_STRATEGY_FASTMOVERS) ? 3 : 1;
		
		iValue += iCombatValue;
		iValue += ((iCombatValue * (GC.getUnitInfo(eUnit).getMoves()-1) * iFastMoverMultiplier) / 3); // K-Mod put in -1 !
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
		iValue += ((iCombatValue * GC.getUnitInfo(eUnit).getWithdrawalProbability()) / 100);
		if (GC.getUnitInfo(eUnit).getCombatLimit() < 100)
		{
			iValue -= (iCombatValue * (125 - GC.getUnitInfo(eUnit).getCombatLimit())) / 100;
		}
		
		break;

	case UNITAI_ATTACK_CITY:
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      02/24/10                                jdog5000      */
/*                                                                                              */
/* War strategy AI           (corrected by K-Mod)                                               */
/************************************************************************************************/
		// Effect army composition to have more collateral/bombard units
		iFastMoverMultiplier = AI_isDoStrategy(AI_STRATEGY_FASTMOVERS) ? 4 : 1;
		
		iTempValue = ((iCombatValue * iCombatValue) / 75) + (iCombatValue / 2);
		iValue += iTempValue;
		if (GC.getUnitInfo(eUnit).isNoDefensiveBonus())
		{
			iValue -= iTempValue / 2;
			//iValue -= iTempValue / 4; // K-Mod. (I'd say knights, tanks, etc. are very good for city attack...)
			// K-Mod note: I've reverted this back to the original code. Ideally, I'd use the reduced penalty, but
			// also take into account which promotions are available to the unit. Eg. knights cannot get city raider.
		}
		if (GC.getUnitInfo(eUnit).getDropRange() > 0)
		{
			//iValue -= iTempValue / 2;
			// K-Mod (how is drop range a disadvantage?)
		}
		if (GC.getUnitInfo(eUnit).isFirstStrikeImmune())
		{
			iValue += (iTempValue * 8) / 100;
		}		
		iValue += ((iCombatValue * GC.getUnitInfo(eUnit).getCityAttackModifier()) / 75);
		iValue += ((iCombatValue * GC.getUnitInfo(eUnit).getCollateralDamage()) / 200);
		iValue += ((iCombatValue * (GC.getUnitInfo(eUnit).getMoves()-1) * iFastMoverMultiplier) / 4); // K-Mod put in -1 !
		iValue += ((iCombatValue * GC.getUnitInfo(eUnit).getWithdrawalProbability()) / 100);

		if (!AI_isDoStrategy(AI_STRATEGY_AIR_BLITZ))
		{
			//int iBombardValue = GC.getUnitInfo(eUnit).getBombardRate() * 8;
			int iBombardValue = GC.getUnitInfo(eUnit).getBombardRate() * (GC.getUnitInfo(eUnit).isIgnoreBuildingDefense()?12 :8);
			if (iBombardValue > 0)
			{
				//int iGoalTotalBombardRate = 200;
				// K-Mod note: This goal has no dependancy on civ size, map size, era, strategy, or anything else that matters
				// a flat goal of 200... This needs to be fixed, but for now, I'll just replace it with something rough.
				//int iGoalTotalBombardRate = (getNumCities()+3) * (getCurrentEra()+2) * (AI_isDoStrategy(AI_STRATEGY_CRUSH)?10 :5);
				int iGoalTotalBombardRate = AI_totalUnitAIs(UNITAI_ATTACK_CITY) * (getCurrentEra()+3);
				if (AI_isDoStrategy(AI_STRATEGY_CRUSH))
				{
					iGoalTotalBombardRate *= 3;
					iGoalTotalBombardRate /= 2;
				}

				// Note: this also counts UNITAI_COLLATERAL units, which only play defense
				int iTotalBombardRate = AI_calculateTotalBombard(DOMAIN_LAND);

				if (iTotalBombardRate <= iGoalTotalBombardRate)
				{
					iBombardValue *= (5*iGoalTotalBombardRate - 4*iTotalBombardRate);
					iBombardValue /= std::max(1, iGoalTotalBombardRate);
				}
				else
				{
					/*iBombardValue *= iGoalTotalBombardRate;
					iBombardValue /= std::min(4*iGoalTotalBombardRate, 2*iTotalBombardRate - iGoalTotalBombardRate);*/
					iBombardValue *= 3*iGoalTotalBombardRate+iTotalBombardRate;
					iBombardValue /= 4*std::max(1, iTotalBombardRate);
				}

				iValue += iBombardValue;
			}
		}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
		break;

	case UNITAI_COLLATERAL:
		iValue += iCombatValue;
		iValue += ((iCombatValue * GC.getUnitInfo(eUnit).getCollateralDamage()) / 50);
		iValue += ((iCombatValue * GC.getUnitInfo(eUnit).getMoves()) / 4);
		iValue += ((iCombatValue * GC.getUnitInfo(eUnit).getWithdrawalProbability()) / 25);
		iValue -= ((iCombatValue * GC.getUnitInfo(eUnit).getCityAttackModifier()) / 100);
		break;

	case UNITAI_PILLAGE:
		iValue += iCombatValue;
		iValue += (iCombatValue * GC.getUnitInfo(eUnit).getMoves());
		break;

	case UNITAI_RESERVE:
		iValue += iCombatValue;
		iValue -= ((iCombatValue * GC.getUnitInfo(eUnit).getCollateralDamage()) / 200);
		for (iI = 0; iI < GC.getNumUnitCombatInfos(); iI++)
		{
//			int iCombatModifier = GC.getUnitInfo(eUnit).getUnitCombatModifier(iI);
//			iCombatModifier = (iCombatModifier < 40) ? iCombatModifier : (40 + (iCombatModifier - 40) / 2);
//			iValue += ((iCombatValue * iCombatModifier) / 100);
			iValue += ((iCombatValue * GC.getUnitInfo(eUnit).getUnitCombatModifier(iI) * AI_getUnitCombatWeight((UnitCombatTypes)iI)) / 12000);
		}
		iValue += ((iCombatValue * GC.getUnitInfo(eUnit).getMoves()) / 2);
		break;

	case UNITAI_COUNTER:
		iValue += (iCombatValue / 2);
		for (iI = 0; iI < GC.getNumUnitClassInfos(); iI++)
		{
			iValue += ((iCombatValue * GC.getUnitInfo(eUnit).getUnitClassAttackModifier(iI) * AI_getUnitClassWeight((UnitClassTypes)iI)) / 7500);
			iValue += ((iCombatValue * (GC.getUnitInfo(eUnit).getTargetUnitClass(iI) ? 50 : 0)) / 100);
		}
		for (iI = 0; iI < GC.getNumUnitCombatInfos(); iI++)
		{
//			int iCombatModifier = GC.getUnitInfo(eUnit).getUnitCombatModifier(iI);
//			iCombatModifier = (iCombatModifier < 40) ? iCombatModifier : (40 + (iCombatModifier - 40) / 2);
//			iValue += ((iCombatValue * iCombatModifier) / 100);
			iValue += ((iCombatValue * GC.getUnitInfo(eUnit).getUnitCombatModifier(iI) * AI_getUnitCombatWeight((UnitCombatTypes)iI)) / 10000);
			iValue += ((iCombatValue * (GC.getUnitInfo(eUnit).getTargetUnitCombat(iI) ? 50 : 0)) / 100);
		}
		for (iI = 0; iI < GC.getNumUnitInfos(); iI++)
		{
			int eUnitClass = GC.getUnitInfo(eUnit).getUnitClassType();
			if (NO_UNITCLASS != eUnitClass && GC.getUnitInfo((UnitTypes)iI).getDefenderUnitClass(eUnitClass))
			{
				iValue += (50 * iCombatValue) / 100;
			}

			int eUnitCombat = GC.getUnitInfo(eUnit).getUnitCombatType();
			if (NO_UNITCOMBAT != eUnitCombat && GC.getUnitInfo((UnitTypes)iI).getDefenderUnitCombat(eUnitCombat))
			{
				iValue += (50 * iCombatValue) / 100;
			}
		}

		iValue += ((iCombatValue * GC.getUnitInfo(eUnit).getMoves()) / 2);
		iValue += ((iCombatValue * GC.getUnitInfo(eUnit).getWithdrawalProbability()) / 100);
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      03/20/10                                jdog5000      */
/*                                                                                              */
/* War strategy AI                                                                              */
/************************************************************************************************/
		//iValue += (GC.getUnitInfo(eUnit).getInterceptionProbability() * 2);
		if( GC.getUnitInfo(eUnit).getInterceptionProbability() > 0 )
		{
			int iTempValue = GC.getUnitInfo(eUnit).getInterceptionProbability();

			iTempValue *= (25 + std::min(175, GET_TEAM(getTeam()).AI_getRivalAirPower()));
			iTempValue /= 100;

			iValue += iTempValue;
		}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/	
		break;

	case UNITAI_CITY_DEFENSE:
		iValue += ((iCombatValue * 2) / 3);
		iValue += ((iCombatValue * GC.getUnitInfo(eUnit).getCityDefenseModifier()) / 75);
		break;

	case UNITAI_CITY_COUNTER:
	case UNITAI_CITY_SPECIAL:
	case UNITAI_PARADROP:
		iValue += (iCombatValue / 2);
		iValue += ((iCombatValue * GC.getUnitInfo(eUnit).getCityDefenseModifier()) / 100);
		for (iI = 0; iI < GC.getNumUnitClassInfos(); iI++)
		{
			iValue += ((iCombatValue * GC.getUnitInfo(eUnit).getUnitClassAttackModifier(iI) * AI_getUnitClassWeight((UnitClassTypes)iI)) / 10000);
			iValue += ((iCombatValue * (GC.getUnitInfo(eUnit).getDefenderUnitClass(iI) ? 50 : 0)) / 100);
		}
		for (iI = 0; iI < GC.getNumUnitCombatInfos(); iI++)
		{
			iValue += ((iCombatValue * GC.getUnitInfo(eUnit).getUnitCombatModifier(iI) * AI_getUnitCombatWeight((UnitCombatTypes)iI)) / 10000);
			iValue += ((iCombatValue * (GC.getUnitInfo(eUnit).getDefenderUnitCombat(iI) ? 50 : 0)) / 100);
		}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      03/20/10                                jdog5000      */
/*                                                                                              */
/* War strategy AI                                                                              */
/************************************************************************************************/
		//iValue += (GC.getUnitInfo(eUnit).getInterceptionProbability() * 3);
		if( GC.getUnitInfo(eUnit).getInterceptionProbability() > 0 )
		{
			int iTempValue = GC.getUnitInfo(eUnit).getInterceptionProbability();

			iTempValue *= (25 + std::min(125, GET_TEAM(getTeam()).AI_getRivalAirPower()));
			iTempValue /= 50;

			iValue += iTempValue;
		}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/	
		break;

	case UNITAI_EXPLORE:
		iValue += (iCombatValue / 2);
		iValue += (GC.getUnitInfo(eUnit).getMoves() * 200);
		if (GC.getUnitInfo(eUnit).isNoBadGoodies())
		{
			iValue += 100;
		}
		break;

	case UNITAI_MISSIONARY:
		iValue += (GC.getUnitInfo(eUnit).getMoves() * 100);
		if (getStateReligion() != NO_RELIGION)
		{
			if (GC.getUnitInfo(eUnit).getReligionSpreads(getStateReligion()) > 0)
			{
				iValue += (5 * GC.getUnitInfo(eUnit).getReligionSpreads(getStateReligion())) / 2;
			}
		}
		for (iI = 0; iI < GC.getNumReligionInfos(); iI++)
		{
			if (GC.getUnitInfo(eUnit).getReligionSpreads((ReligionTypes)iI) && hasHolyCity((ReligionTypes)iI))
			{
				iValue += 80;
				break;
			}
		}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      03/08/10                                jdog5000      */
/*                                                                                              */
/* Victory Strategy AI                                                                          */
/************************************************************************************************/
		if (AI_isDoVictoryStrategy(AI_VICTORY_CULTURE2))
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
		{
		    int iTempValue = 0;
		    for (iI = 0; iI < GC.getNumReligionInfos(); iI++)
		    {
                if (GC.getUnitInfo(eUnit).getReligionSpreads((ReligionTypes)iI))
                {
                    iTempValue += (50 * getNumCities()) / (1 + getHasReligionCount((ReligionTypes)iI));
                }
		    }
		    iValue += iTempValue;		    
		}
		for (iI = 0; iI < GC.getNumCorporationInfos(); ++iI)
		{
			if (hasHeadquarters((CorporationTypes)iI))
			{
				if (GC.getUnitInfo(eUnit).getCorporationSpreads(iI) > 0)
				{
					iValue += (5 * GC.getUnitInfo(eUnit).getCorporationSpreads(iI)) / 2;
/************************************************************************************************/
/* UNOFFICIAL_PATCH                       06/03/09                                jdog5000      */
/*                                                                                              */
/* Bugfix				                                                                         */
/************************************************************************************************/
					// Fix potential crash, probably would only happen in mods
					if( pArea != NULL )
					{
						iValue += 300 / std::max(1, pArea->countHasCorporation((CorporationTypes)iI, getID()));
					}
/************************************************************************************************/
/* UNOFFICIAL_PATCH                        END                                                  */
/************************************************************************************************/
				}
			}
		}
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
		iValue += (GC.getUnitInfo(eUnit).getMoves() * 100);
		if (GC.getUnitInfo(eUnit).isSabotage())
		{
			iValue += 50;
		}
		if (GC.getUnitInfo(eUnit).isDestroy())
		{
			iValue += 50;
		}
		if (GC.getUnitInfo(eUnit).isCounterSpy())
		{
			iValue += 100;
		}
		break;

	case UNITAI_ICBM:
		if (GC.getUnitInfo(eUnit).getNukeRange() != -1)
		{
			iTempValue = 40 + (GC.getUnitInfo(eUnit).getNukeRange() * 40);
			if (GC.getUnitInfo(eUnit).getAirRange() == 0)
			{
				iValue += iTempValue;
			}
			else
			{
				iValue += (iTempValue * std::min(10, GC.getUnitInfo(eUnit).getAirRange())) / 10;
			}
			iValue += (iTempValue * (60 + GC.getUnitInfo(eUnit).getEvasionProbability())) / 100;
		}
		break;

	case UNITAI_WORKER_SEA:
		for (iI = 0; iI < GC.getNumBuildInfos(); iI++)
		{
			if (GC.getUnitInfo(eUnit).getBuilds(iI))
			{
				iValue += 50;
			}
		}
		iValue += (GC.getUnitInfo(eUnit).getMoves() * 100);
		break;

	case UNITAI_ATTACK_SEA:
		iValue += iCombatValue;
		iValue += ((iCombatValue * GC.getUnitInfo(eUnit).getMoves()) / 2);
		iValue += (GC.getUnitInfo(eUnit).getBombardRate() * 4);
		break;

	case UNITAI_RESERVE_SEA:
		iValue += iCombatValue;
		iValue += (iCombatValue * GC.getUnitInfo(eUnit).getMoves());
		break;

	case UNITAI_ESCORT_SEA:
		iValue += iCombatValue;
		iValue += (iCombatValue * GC.getUnitInfo(eUnit).getMoves());
		iValue += (GC.getUnitInfo(eUnit).getInterceptionProbability() * 3);
		if (GC.getUnitInfo(eUnit).getNumSeeInvisibleTypes() > 0)
		{
			iValue += 200;
		}
/************************************************************************************************/
/* UNOFFICIAL_PATCH                       06/03/09                                jdog5000      */
/*                                                                                              */
/* General AI                                                                                   */
/************************************************************************************************/
		// Boats which can't be seen don't play defense, don't make good escorts
		if (GC.getUnitInfo(eUnit).getInvisibleType() != NO_INVISIBLE)
		{
			iValue /= 2;
		}
/************************************************************************************************/
/* UNOFFICIAL_PATCH                        END                                                  */
/************************************************************************************************/
		break;

	case UNITAI_EXPLORE_SEA:
		{
			int iExploreValue = 100;
			if (pArea != NULL)
			{
				if (pArea->isWater())
				{
					if (pArea->getUnitsPerPlayer(BARBARIAN_PLAYER) > 0)
					{
						iExploreValue += (2 * iCombatValue);
					}
				}
			}
			iValue += (GC.getUnitInfo(eUnit).getMoves() * iExploreValue);
			if (GC.getUnitInfo(eUnit).isAlwaysHostile())
			{
				iValue /= 2;
			}
			iValue /= (1 + AI_unitImpassableCount(eUnit));
		}
		break;

	case UNITAI_ASSAULT_SEA:
	case UNITAI_SETTLER_SEA:
	case UNITAI_MISSIONARY_SEA:
	case UNITAI_SPY_SEA:
		iValue += (iCombatValue / 2);
		iValue += (GC.getUnitInfo(eUnit).getMoves() * 200);
		iValue += (GC.getUnitInfo(eUnit).getCargoSpace() * 300);
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      05/18/09                                jdog5000      */
/*                                                                                              */
/* City AI                                                                                      */
/************************************************************************************************/
		// Never build galley transports when ocean faring ones exist (issue mainly for Carracks)
		iValue /= (1 + AI_unitImpassableCount(eUnit));
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
		break;

	case UNITAI_CARRIER_SEA:
		iValue += iCombatValue;
		iValue += (GC.getUnitInfo(eUnit).getMoves() * 50);
		iValue += (GC.getUnitInfo(eUnit).getCargoSpace() * 400);
		break;

	case UNITAI_MISSILE_CARRIER_SEA:
		iValue += iCombatValue * GC.getUnitInfo(eUnit).getMoves();
		iValue += (25 + iCombatValue) * (3 + (GC.getUnitInfo(eUnit).getCargoSpace()));
		break;

	case UNITAI_PIRATE_SEA:
		iValue += iCombatValue;
		iValue += (iCombatValue * GC.getUnitInfo(eUnit).getMoves());
		break;

	case UNITAI_ATTACK_AIR:
		iValue += iCombatValue;
		iValue += (GC.getUnitInfo(eUnit).getCollateralDamage() * iCombatValue) / 100;
		iValue += 4 * GC.getUnitInfo(eUnit).getBombRate();
		iValue += (iCombatValue * (100 + 2 * GC.getUnitInfo(eUnit).getCollateralDamage()) * GC.getUnitInfo(eUnit).getAirRange()) / 100;
		break;

	case UNITAI_DEFENSE_AIR:
		iValue += iCombatValue;
		iValue += (GC.getUnitInfo(eUnit).getInterceptionProbability() * 3);
		iValue += (GC.getUnitInfo(eUnit).getAirRange() * iCombatValue);
		break;

	case UNITAI_CARRIER_AIR:
		iValue += (iCombatValue);
		iValue += (GC.getUnitInfo(eUnit).getInterceptionProbability() * 2);
		iValue += (GC.getUnitInfo(eUnit).getAirRange() * iCombatValue);
		break;

	case UNITAI_MISSILE_AIR:
		iValue += iCombatValue;
		iValue += 4 * GC.getUnitInfo(eUnit).getBombRate();
		iValue += GC.getUnitInfo(eUnit).getAirRange() * iCombatValue;
		break;

	case UNITAI_ATTACK_CITY_LEMMING:
		iValue += iCombatValue;
		break;

	default:
		FAssert(false);
		break;
	}
	
	if ((iCombatValue > 0) && ((eUnitAI == UNITAI_ATTACK) || (eUnitAI == UNITAI_ATTACK_CITY)))
	{
		if (pArea != NULL)
		{
			AreaAITypes eAreaAI = pArea->getAreaAIType(getTeam());
			if (eAreaAI == AREAAI_ASSAULT || eAreaAI == AREAAI_ASSAULT_MASSING)
			{
				for (int iI = 0; iI < GC.getNumPromotionInfos(); iI++)
				{
					if (GC.getUnitInfo(eUnit).getFreePromotions(iI))
					{
						if (GC.getPromotionInfo((PromotionTypes)iI).isAmphib())
						{
							iValue *= 133;
							iValue /= 100;
							break;
						}
					}
				}				
			}
		}
	}

	return std::max(0, iValue);
}


int CvPlayerAI::AI_totalUnitAIs(UnitAITypes eUnitAI) const
{
	return (AI_getNumTrainAIUnits(eUnitAI) + AI_getNumAIUnits(eUnitAI));
}


int CvPlayerAI::AI_totalAreaUnitAIs(CvArea* pArea, UnitAITypes eUnitAI) const
{
	return (pArea->getNumTrainAIUnits(getID(), eUnitAI) + pArea->getNumAIUnits(getID(), eUnitAI));
}


int CvPlayerAI::AI_totalWaterAreaUnitAIs(CvArea* pArea, UnitAITypes eUnitAI) const
{
	CvCity* pLoopCity;
	int iCount;
	int iLoop;
	int iI;

	iCount = AI_totalAreaUnitAIs(pArea, eUnitAI);

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			for (pLoopCity = GET_PLAYER((PlayerTypes)iI).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER((PlayerTypes)iI).nextCity(&iLoop))
			{
				if (pLoopCity->waterArea() == pArea)
				{
					iCount += pLoopCity->plot()->plotCount(PUF_isUnitAIType, eUnitAI, -1, getID());

					if (pLoopCity->getOwnerINLINE() == getID())
					{
						iCount += pLoopCity->getNumTrainUnitAI(eUnitAI);
					}
				}
			}
		}
	}


	return iCount;
}


int CvPlayerAI::AI_countCargoSpace(UnitAITypes eUnitAI) const
{
	CvUnit* pLoopUnit;
	int iCount;
	int iLoop;

	iCount = 0;

	for(pLoopUnit = firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = nextUnit(&iLoop))
	{
		if (pLoopUnit->AI_getUnitAIType() == eUnitAI)
		{
			iCount += pLoopUnit->cargoSpace();
		}
	}

	return iCount;
}


int CvPlayerAI::AI_neededExplorers(CvArea* pArea) const
{
	FAssert(pArea != NULL);
	int iNeeded = 0;

	if (pArea->isWater())
	{
		iNeeded = std::min(iNeeded + (pArea->getNumUnrevealedTiles(getTeam()) / 400), std::min(2, ((getNumCities() / 2) + 1)));
	}
	else
	{
		iNeeded = std::min(iNeeded + (pArea->getNumUnrevealedTiles(getTeam()) / 150), std::min(3, ((getNumCities() / 3) + 2)));
	}

	if (0 == iNeeded)
	{
		if ((GC.getGameINLINE().countCivTeamsAlive() - 1) > GET_TEAM(getTeam()).getHasMetCivCount(true))
		{
			if (pArea->isWater())
			{
				if (GC.getMap().findBiggestArea(true) == pArea)
				{
					iNeeded++;
				}
			}
			else
			{
			    if (getCapitalCity() != NULL && pArea->getID() == getCapitalCity()->getArea())
			    {
                    for (int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; iPlayer++)
                    {
                        CvPlayerAI& kPlayer = GET_PLAYER((PlayerTypes)iPlayer);
                        if (kPlayer.isAlive() && kPlayer.getTeam() != getTeam())
                        {
                            if (!GET_TEAM(getTeam()).isHasMet(kPlayer.getTeam()))
                            {
                                if (pArea->getCitiesPerPlayer(kPlayer.getID()) > 0)
                                {
                                    iNeeded++;
                                    break;
                                }
                            }
                        }
                    }
			    }
			}
		}
	}
	return iNeeded;

}


int CvPlayerAI::AI_neededWorkers(CvArea* pArea) const
{
	CvCity* pLoopCity;
	int iCount;
	int iLoop;

	iCount = countUnimprovedBonuses(pArea) * 2;

	for (pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		if (pLoopCity->getArea() == pArea->getID())
		{
			iCount += pLoopCity->AI_getWorkersNeeded() * 3;
		}
	}
	
	if (iCount == 0)
	{
		return 0;
	}

	if (getBestRoute() != NO_ROUTE)
	{
		iCount += pArea->getCitiesPerPlayer(getID()) / 2;
	}


	iCount += 1;
	iCount /= 3;
	iCount = std::min(iCount, 3 * pArea->getCitiesPerPlayer(getID()));
	iCount = std::min(iCount, (1 + getTotalPopulation()) / 2);

	return std::max(1, iCount);

}


int CvPlayerAI::AI_neededMissionaries(CvArea* pArea, ReligionTypes eReligion) const
{
    PROFILE_FUNC();
	int iCount;
	bool bHoly, bState, bHolyState;
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      03/08/10                                jdog5000      */
/*                                                                                              */
/* Victory Strategy AI                                                                          */
/************************************************************************************************/
	bool bCultureVictory = AI_isDoVictoryStrategy(AI_VICTORY_CULTURE2);
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	bHoly = hasHolyCity(eReligion);
	bState = (getStateReligion() == eReligion);
	bHolyState = ((getStateReligion() != NO_RELIGION) && hasHolyCity(getStateReligion()));

	iCount = 0;

    //internal spread.
    if (bCultureVictory || bState || bHoly)
    {
        iCount = std::max(iCount, (pArea->getCitiesPerPlayer(getID()) - pArea->countHasReligion(eReligion, getID())));
        if (iCount > 0)
        {
            if (!bCultureVictory)
	{
                iCount = std::max(1, iCount / (bHoly ? 2 : 4));
            }
            return iCount;
        }
	}

    //external spread.
    if ((bHoly && bState) || (bHoly && !bHolyState && (getStateReligion() != NO_RELIGION)))
    {
        iCount += ((pArea->getNumCities() * 2) - (pArea->countHasReligion(eReligion) * 3));
        iCount /= 8;

        iCount = std::max(0, iCount);

		if (AI_isPrimaryArea(pArea))
		{
			iCount++;
		}
    }


	return iCount;
}


int CvPlayerAI::AI_neededExecutives(CvArea* pArea, CorporationTypes eCorporation) const
{
	if (!hasHeadquarters(eCorporation))
	{
		return 0;
	}

	int iCount = ((pArea->getCitiesPerPlayer(getID()) - pArea->countHasCorporation(eCorporation, getID())) * 2);
	iCount += (pArea->getNumCities() - pArea->countHasCorporation(eCorporation));

	iCount /= 3;

	if (AI_isPrimaryArea(pArea))
	{
		++iCount;
	}

	return iCount;
}

// K-Mod. This function is used to replace the old (broken) "unit cost percentage" calculation used by the AI
int CvPlayerAI::AI_unitCostPerMil() const
{
	// original "cost percentage" = calculateUnitCost() * 100 / std::max(1, calculatePreInflatedCosts());
	// If iUnitCostPercentage is calculated as above, decreasing maintenance will actually decrease the max units.
	// If a builds a courthouse or switches to state property, it would then think it needs to get rid of units!
	// It makes no sense, and civs with a surplus of cash still won't want to build units. So lets try it another way...
	int iUnitCost = calculateUnitCost();
	if (iUnitCost <= getNumCities()/2) // cf with the final line
		return 0;

	int iTotalRaw = calculateTotalYield(YIELD_COMMERCE);

	int iFunds = iTotalRaw * AI_averageCommerceMultiplier(COMMERCE_GOLD) / 100;
	iFunds += getGoldPerTurn() - calculateInflatedCosts();
	iFunds += getCommerceRate(COMMERCE_GOLD) - iTotalRaw * AI_averageCommerceMultiplier(COMMERCE_GOLD) * getCommercePercent(COMMERCE_GOLD) / 10000;
	return std::max(0, iUnitCost-getNumCities()/2) * 1000 / std::max(1, iFunds); // # cities is there to offset early-game distortion.
}

// This function gives an approximate / recommended maximum on our unit spending. Note though that it isn't a hard cap.
// we might go as high has 20 point above the "maximum"; and of course, the maximum might later go down.
// So this should only be used as a guide.
int CvPlayerAI::AI_maxUnitCostPerMil(CvArea* pArea, int iBuildProb) const
{
	if (isBarbarian())
		return 500;

	if (GC.getGameINLINE().isOption(GAMEOPTION_ALWAYS_PEACE))
		return 20; // ??

	if (iBuildProb < 0)
		iBuildProb = GC.getLeaderHeadInfo(getPersonalityType()).getBuildUnitProb() + 6; // a rough estimate.

	bool bTotalWar = GET_TEAM(getTeam()).getWarPlanCount(WARPLAN_TOTAL, true);
	bool bAggressiveAI = GC.getGameINLINE().isOption(GAMEOPTION_AGGRESSIVE_AI);

	int iMaxUnitSpending = (bAggressiveAI ? 24 : 12) + iBuildProb;

	if (AI_isDoVictoryStrategy(AI_VICTORY_CONQUEST4))
	{
		iMaxUnitSpending += 25;
	}
	else if (AI_isDoVictoryStrategy(AI_VICTORY_CONQUEST3 | AI_VICTORY_DOMINATION3))
	{
		iMaxUnitSpending += 15;
	}
	else if (AI_isDoVictoryStrategy(AI_VICTORY_CONQUEST1))
	{
		iMaxUnitSpending += 5;
	}

	if (!bTotalWar)
	{
		iMaxUnitSpending += AI_isDoStrategy(AI_STRATEGY_ALERT1) ? 12 + iBuildProb / 4 : 0;
		iMaxUnitSpending += AI_isDoStrategy(AI_STRATEGY_ALERT2) ? 12 + iBuildProb / 4 : 0;
		// note. the boost from alert1 + alert2 matches the boost from total war. (see below).
	}

	if (AI_isDoStrategy(AI_STRATEGY_FINAL_WAR))
	{
		iMaxUnitSpending += 300;
	}
	else
	{
		iMaxUnitSpending += bTotalWar ? 25 + iBuildProb / 2 : 0;
		iMaxUnitSpending += AI_isDoStrategy(AI_STRATEGY_DAGGER) ? 10 + AI_getFlavorValue(FLAVOR_MILITARY) : 0;
		if (pArea)
		{
			switch (pArea->getAreaAIType(getTeam()))
			{
			case AREAAI_OFFENSIVE:
				iMaxUnitSpending += 25;
				break;

			case AREAAI_DEFENSIVE:
				iMaxUnitSpending += 45;
				break;

			case AREAAI_MASSING:
				iMaxUnitSpending += 45;
				break;

			case AREAAI_ASSAULT:
				iMaxUnitSpending += 25;
				break;

			case AREAAI_ASSAULT_MASSING:
				iMaxUnitSpending += 45;
				break;

			case AREAAI_ASSAULT_ASSIST:
				iMaxUnitSpending += 20;
				break;

			case AREAAI_NEUTRAL:
				break;
			default:
				FAssert(false);
			}
		}
		else
		{
			iMaxUnitSpending += GET_TEAM(getTeam()).getAnyWarPlanCount(true) ? 35 : 0;
		}
	}
	return iMaxUnitSpending;
}
// K-Mod end

int CvPlayerAI::AI_adjacentPotentialAttackers(CvPlot* pPlot, bool bTestCanMove) const
{
	CLLNode<IDInfo>* pUnitNode;
	CvUnit* pLoopUnit;
	CvPlot* pLoopPlot;
	int iCount;
	int iI;

	iCount = 0;

	for (iI = 0; iI < NUM_DIRECTION_TYPES; iI++)
	{
		pLoopPlot = plotDirection(pPlot->getX_INLINE(), pPlot->getY_INLINE(), ((DirectionTypes)iI));

		if (pLoopPlot != NULL)
		{
			if (pLoopPlot->area() == pPlot->area())
			{
				pUnitNode = pLoopPlot->headUnitNode();

				while (pUnitNode != NULL)
				{
					pLoopUnit = ::getUnit(pUnitNode->m_data);
					pUnitNode = pLoopPlot->nextUnitNode(pUnitNode);

					if (pLoopUnit->getOwnerINLINE() == getID())
					{
						if (pLoopUnit->getDomainType() == ((pPlot->isWater()) ? DOMAIN_SEA : DOMAIN_LAND))
						{
							if (pLoopUnit->canAttack())
							{
								if (!bTestCanMove || pLoopUnit->canMove())
								{
									if (!(pLoopUnit->AI_isCityAIType()))
									{
										iCount++;
									}
								}
							}
						}
					}
				}
			}
		}
	}

	return iCount;
}


int CvPlayerAI::AI_totalMissionAIs(MissionAITypes eMissionAI, CvSelectionGroup* pSkipSelectionGroup) const
{
	PROFILE_FUNC();

	CvSelectionGroup* pLoopSelectionGroup;
	int iCount;
	int iLoop;

	iCount = 0;

	for(pLoopSelectionGroup = firstSelectionGroup(&iLoop); pLoopSelectionGroup; pLoopSelectionGroup = nextSelectionGroup(&iLoop))
	{
		if (pLoopSelectionGroup != pSkipSelectionGroup)
		{
			if (pLoopSelectionGroup->AI_getMissionAIType() == eMissionAI)
			{
				iCount += pLoopSelectionGroup->getNumUnits();
			}
		}
	}

	return iCount;
}

int CvPlayerAI::AI_missionaryValue(CvArea* pArea, ReligionTypes eReligion, PlayerTypes* peBestPlayer) const
{
	CvTeam& kTeam = GET_TEAM(getTeam());
	CvGame& kGame = GC.getGame();
	
	int iSpreadInternalValue = 100;
	int iSpreadExternalValue = 0;

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      03/08/10                                jdog5000      */
/*                                                                                              */
/* Victory Strategy AI                                                                          */
/************************************************************************************************/
	// Obvious copy & paste bug
	if (AI_isDoVictoryStrategy(AI_VICTORY_CULTURE1))
	{
		iSpreadInternalValue += 500;
		if (AI_isDoVictoryStrategy(AI_VICTORY_CULTURE2))
		{
			iSpreadInternalValue += 1500;
			if (AI_isDoVictoryStrategy(AI_VICTORY_CULTURE3))
			{
				iSpreadInternalValue += 3000;
			}
		}
	}			
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      10/03/09                                jdog5000      */
/*                                                                                              */
/* Missionary AI                                                                                */
/************************************************************************************************/
	// In free religion, treat all religions like state religions
	bool bStateReligion = (getStateReligion() == eReligion);
	
	if (!isStateReligion())
	{
		// Free religion
		iSpreadInternalValue += 500;
		bStateReligion = true;
	}
	else if(bStateReligion)
	{
		iSpreadInternalValue += 1000;
	}
	else
	{
		iSpreadInternalValue += (500 * getHasReligionCount(eReligion)) / std::max(1, getNumCities());
	}
	
	int iGoldValue = 0;
	if (kTeam.hasHolyCity(eReligion))
	{
		iSpreadInternalValue += bStateReligion ? 1000 : 300;
		iSpreadExternalValue += bStateReligion ? 1000 : 150;
		if (kTeam.hasShrine(eReligion))
		{
			iSpreadInternalValue += bStateReligion ? 500 : 300;
			iSpreadExternalValue += bStateReligion ? 300 : 200;
			int iGoldMultiplier = kGame.getHolyCity(eReligion)->getTotalCommerceRateModifier(COMMERCE_GOLD);
			iGoldValue = 6 * iGoldMultiplier;
			// K-Mod. todo: use GC.getReligionInfo(eReligion).getGlobalReligionCommerce((CommerceTypes)iJ)
		}
	}
	
	int iOurCitiesHave = 0;
	int iOurCitiesCount = 0;
	
	if (NULL == pArea)
	{
		iOurCitiesHave = kTeam.getHasReligionCount(eReligion);
		iOurCitiesCount = kTeam.getNumCities();
	}
	else
	{
		iOurCitiesHave = pArea->countHasReligion(eReligion, getID()) + countReligionSpreadUnits(pArea, eReligion,true);
		iOurCitiesCount = pArea->getCitiesPerPlayer(getID());
	}
	
	if (iOurCitiesHave < iOurCitiesCount)
	{
		iSpreadInternalValue *= 30 + ((100 * (iOurCitiesCount - iOurCitiesHave))/ iOurCitiesCount);
		iSpreadInternalValue /= 100;
		iSpreadInternalValue += iGoldValue;
	}
	else
	{
		iSpreadInternalValue = 0;
	}
	
	if (iSpreadExternalValue > 0)
	{
		int iBestPlayer = NO_PLAYER;
		int iBestValue = 0;
		for (int iPlayer = 0; iPlayer < MAX_PLAYERS; iPlayer++)
		{
			if (iPlayer != getID())
			{
				CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iPlayer);
				if (kLoopPlayer.isAlive() && kLoopPlayer.getTeam() != getTeam() && kLoopPlayer.getNumCities() > 0)
				{
					if (GET_TEAM(kLoopPlayer.getTeam()).isOpenBorders(getTeam()))
					{
						int iCitiesCount = 0;
						int iCitiesHave = 0;
						int iMultiplier = AI_isDoStrategy(AI_STRATEGY_MISSIONARY) ? 60 : 25;
						if (!kLoopPlayer.isNoNonStateReligionSpread() || (kLoopPlayer.getStateReligion() == eReligion))
						{
							if (NULL == pArea)
							{
								iCitiesCount += 1 + (kLoopPlayer.getNumCities() * 75) / 100;
								iCitiesHave += std::min(iCitiesCount, kLoopPlayer.getHasReligionCount(eReligion));
							}
							else
							{
								int iPlayerSpreadPercent = (100 * kLoopPlayer.getHasReligionCount(eReligion)) / kLoopPlayer.getNumCities();
								iCitiesCount += pArea->getCitiesPerPlayer((PlayerTypes)iPlayer);
								iCitiesHave += std::min(iCitiesCount, (iCitiesCount * iPlayerSpreadPercent) / 75);
							}
						}
						
						if (kLoopPlayer.getStateReligion() == NO_RELIGION)
						{
							// Paganism counts as a state religion civic, that's what's caught below
							if (kLoopPlayer.getStateReligionCount() > 0)
							{
								int iTotalReligions = kLoopPlayer.countTotalHasReligion();
								iMultiplier += 100 * std::max(0, kLoopPlayer.getNumCities() - iTotalReligions);
								iMultiplier += (iTotalReligions == 0) ? 100 : 0;
							}
						}

						int iValue = (iMultiplier * iSpreadExternalValue * (iCitiesCount - iCitiesHave)) / std::max(1, iCitiesCount);
						iValue /= 100;
						iValue += iGoldValue;

						if (iValue > iBestValue)
						{
							iBestValue = iValue;
							iBestPlayer = iPlayer;
						}
					}
				}
			}
		}

		if (iBestValue > iSpreadInternalValue)
		{
			if (NULL != peBestPlayer)
			{
				*peBestPlayer = (PlayerTypes)iBestPlayer;
			}
			return iBestValue;
		}

	}

	if (NULL != peBestPlayer)
	{
		*peBestPlayer = getID();
	}

	return iSpreadInternalValue;
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
}

// K-Mod note: the original BtS code for this was totally inconsistant with the calculation in AI_missionaryValue
// -- which is bad news since the results are compared directly.
// I've rewritten most of this function so that it is more sane and more compariable to the missionary value.
// The original code is deleted.
int CvPlayerAI::AI_executiveValue(CvArea* pArea, CorporationTypes eCorporation, PlayerTypes* peBestPlayer, bool bSpreadOnly) const
{
	PROFILE_FUNC();
	CvGame& kGame = GC.getGame();
	CvCorporationInfo& kCorp = GC.getCorporationInfo(eCorporation);

	int iSpreadExternalValue = 0;
	int iExistingExecs = 0;
	if (pArea)
	{
		iExistingExecs += bSpreadOnly ? 0 : countCorporationSpreadUnits(pArea, eCorporation, true) - 1;
		// K-Mod note, -1 just so that we can build a spare, perhaps to airlift to another area. ("-1" execs is ok.)
		// bSpreadOnly means that we are not calculating the value of building a new exec. Just the value of spreading.
	}

	if (GET_TEAM(getTeam()).hasHeadquarters(eCorporation))
	{
		int iHqValue = 0;
		CvCity* kHqCity = kGame.getHeadquarters(eCorporation);
		for (int i = 0; i < NUM_COMMERCE_TYPES; i++)
		{
			iHqValue += kCorp.getHeadquarterCommerce(i) * kHqCity->getTotalCommerceRateModifier((CommerceTypes)i) * AI_commerceWeight((CommerceTypes)i)/100;
		}

		iSpreadExternalValue += iHqValue;
	}

	int iBestPlayer = NO_PLAYER;
	int iBestValue = 0;
	for (int iPlayer = 0; iPlayer < MAX_PLAYERS; iPlayer++)
	{
		const CvPlayerAI& kLoopPlayer = GET_PLAYER((PlayerTypes)iPlayer);
		int iNumCities = pArea ? pArea->getCitiesPerPlayer((PlayerTypes)iPlayer) : kLoopPlayer.getNumCities();
		if (kLoopPlayer.isAlive() && iNumCities > 0)
		{
			if (GET_TEAM(getTeam()).isFriendlyTerritory(kLoopPlayer.getTeam()) || GET_TEAM(kLoopPlayer.getTeam()).isOpenBorders(getTeam()))
			{
				if (!kLoopPlayer.isNoCorporations() && (iPlayer == getID() || !kLoopPlayer.isNoForeignCorporations()))
				{
					int iAttitudeWeight;

					if (kLoopPlayer.getTeam() == getTeam())
						iAttitudeWeight = 100;
					else if (GET_TEAM(kLoopPlayer.getTeam()).isVassal(getTeam()))
						iAttitudeWeight = 50;
					else
						iAttitudeWeight = AI_getAttitudeWeight((PlayerTypes)iPlayer) - 75;

					// a rough check to save us some time.
					if (iAttitudeWeight <= 0 && iSpreadExternalValue <= 0)
						continue;

					int iCitiesHave = kLoopPlayer.countCorporations(eCorporation, pArea);

					if (iCitiesHave + iExistingExecs >= iNumCities)
						continue;

					int iCorpValue = kLoopPlayer.AI_corporationValue(eCorporation);
					int iValue = iSpreadExternalValue;
					iValue += (iCorpValue * iAttitudeWeight)/100;
					if (iValue > 0 && iCorpValue > 0 && iCitiesHave == 0)
					{
						// if the player will spread the corp themselves, then that's good for us.
						if (iAttitudeWeight >= 50)
						{
							// estimate spread to 2/3 of total cities.
							iValue *= (2*kLoopPlayer.getNumCities()+1)/3;
						}
						else
						{
							// estimate spread to 1/4 of total cities, rounded up.
							iValue *= (kLoopPlayer.getNumCities()+3)/4;
						}
					}
					if (iValue > iBestValue)
					{
						for (int iCorp = 0; iCorp < GC.getNumCorporationInfos(); iCorp++)
						{
							if (kGame.isCorporationFounded((CorporationTypes)iCorp) && kGame.isCompetingCorporation(eCorporation, (CorporationTypes)iCorp))
							{
								int iExtra = kLoopPlayer.countCorporations((CorporationTypes)iCorp, pArea);

								if (iExtra > 0 && iCorpValue > kLoopPlayer.AI_corporationValue((CorporationTypes)iCorp))
									iExtra /= 2;

								iCitiesHave += iExtra;
							}
						}

						if (iCitiesHave + iExistingExecs >= iNumCities)
							continue;

						iBestValue = iValue;
						iBestPlayer = iPlayer;
					}
				}
			}
		}
	}

	if (NULL != peBestPlayer)
	{
		*peBestPlayer = (PlayerTypes)iBestPlayer;
	}
	// I'm putting in a fudge-factor of 10 just to bring the value up to scale with AI_missionaryValue.
	// This isn't something that i'm happy about, but it's easier than rewriting AI_missionaryValue.
	return 10 * iBestValue;
}

//Returns approximately 100 x gpt value of the corporation.
int CvPlayerAI::AI_corporationValue(CorporationTypes eCorporation, const CvCity* pCity) const
{
	/* original bts code
	if (pCity == NULL)
	{
		if (getCapitalCity() != NULL)
		{
			pCity = getCapitalCity();
		}
	}
	if (NULL == pCity)
	{
		return 0;
	}
	CvCorporationInfo& kCorp = GC.getCorporationInfo(eCorporation);
	int iBonusValue = 0;
	
	for (int iBonus = 0; iBonus < GC.getNumBonusInfos(); iBonus++)
	{
		BonusTypes eBonus = (BonusTypes)iBonus;
		int iBonusCount = pCity->getNumBonuses(eBonus);
		if (iBonusCount > 0)
		{
			for (int i = 0; i < GC.getNUM_CORPORATION_PREREQ_BONUSES(); ++i)
			{
				if (eBonus == kCorp.getPrereqBonus(i))
				{
					iBonusValue += (100 * kCorp.getYieldProduced(YIELD_FOOD) * iBonusCount);
					iBonusValue += (100 * kCorp.getYieldProduced(YIELD_PRODUCTION) * iBonusCount);
					iBonusValue += (60 * kCorp.getYieldProduced(YIELD_COMMERCE) * iBonusCount);

					iBonusValue += (60 * kCorp.getCommerceProduced(COMMERCE_GOLD) * iBonusCount);
					iBonusValue += (60 * kCorp.getCommerceProduced(COMMERCE_RESEARCH) * iBonusCount);
					iBonusValue += (25 * kCorp.getCommerceProduced(COMMERCE_CULTURE) * iBonusCount);
					iBonusValue += (40 * kCorp.getCommerceProduced(COMMERCE_ESPIONAGE) * iBonusCount);

					if (NO_BONUS != kCorp.getBonusProduced())
					{
						int iBonuses = getNumAvailableBonuses((BonusTypes)kCorp.getBonusProduced());
						iBonusValue += (AI_baseBonusVal((BonusTypes)kCorp.getBonusProduced()) * 1000) / (1 + 3 * iBonuses * iBonuses);						
					}
				}
			}
		}
	}
	iBonusValue *= 3;
		
	return iBonusValue; */

	// K-Mod. Well that was a load of bullshit... lets try to do it better.
	const CvTeamAI& kTeam = GET_TEAM(getTeam());
	CvCorporationInfo& kCorp = GC.getCorporationInfo(eCorporation);
	int iValue = 0;
	int iMaintenance = 0;

	int iBonuses = 0;
	for (int i = 0; i < GC.getNUM_CORPORATION_PREREQ_BONUSES(); ++i)
	{
		BonusTypes eBonus = (BonusTypes)kCorp.getPrereqBonus(i);
		if (NO_BONUS != eBonus)
		{
			if (pCity == NULL)
				iBonuses += countOwnedBonuses(eBonus);
			else
				iBonuses += pCity->getNumBonuses(eBonus);
			// maybe use getNumAvailableBonuses ?
			if (!kTeam.isHasTech((TechTypes)GC.getBonusInfo(eBonus).getTechReveal()) && !kTeam.isForceRevealedBonus(eBonus))
			{
				iBonuses++; // expect that we'll get one of each unrevealed resource
			}
		}
	}
	if (!GC.getGameINLINE().isCorporationFounded(eCorporation))
	{
		// expect that we'll be able to trade for at least 1 of the bonuses.
		iBonuses++;
	}

	for (int iI = (CommerceTypes)0; iI < NUM_COMMERCE_TYPES; ++iI)
	{
		int iTempValue = kCorp.getCommerceProduced((CommerceTypes)iI) * iBonuses;

		if (iTempValue != 0)
		{
			if (pCity == NULL)
				iTempValue *= AI_averageCommerceMultiplier((CommerceTypes)iI);
			else
				iTempValue *= pCity->getTotalCommerceRateModifier((CommerceTypes)iI);
			// I'd feel more comfortable if they named it "multiplier" when it included the base 100%.
			iTempValue /= 100;

			iTempValue *= GC.getWorldInfo(GC.getMapINLINE().getWorldSize()).getCorporationMaintenancePercent();
			iTempValue /= 100;

			iTempValue *= AI_commerceWeight((CommerceTypes)iI, pCity);
			iTempValue /= 100;

			iValue += iTempValue;
		}

		iMaintenance += 100 * kCorp.getHeadquarterCommerce(iI);
	}

	for (int iI = 0; iI < NUM_YIELD_TYPES; ++iI)
	{
		int iTempValue = kCorp.getYieldProduced((YieldTypes)iI) * iBonuses;
		if (iTempValue != 0)
		{
			if (pCity == NULL)
				iTempValue *= AI_averageYieldMultiplier((YieldTypes)iI);
			else
				iTempValue *= pCity->getBaseYieldRateModifier((YieldTypes)iI);

			iTempValue /= 100;
			iTempValue *= GC.getWorldInfo(GC.getMapINLINE().getWorldSize()).getCorporationMaintenancePercent();
			iTempValue /= 100;

			iTempValue *= AI_yieldWeight((YieldTypes)iI);
			iTempValue /= 100;

			iValue += iTempValue;
		}
	}

	// maintenance cost
	int iTempValue = kCorp.getMaintenance() * iBonuses;
	iTempValue *= GC.getWorldInfo(GC.getMapINLINE().getWorldSize()).getCorporationMaintenancePercent();
	iTempValue /= 100;
	iTempValue += iMaintenance;
	// Inflation, population, and maintenance modifiers... lets just approximate them like this:
	iTempValue *= 2;
	iTempValue /= 3;

	iTempValue *= AI_commerceWeight(COMMERCE_GOLD);
	iTempValue /= 100;

	iValue -= iTempValue;

	// bonus produced by the corp
	if (kCorp.getBonusProduced() != NO_BONUS)
	{
		int iBonuses = getNumAvailableBonuses((BonusTypes)kCorp.getBonusProduced());
		// pretend we have 1 bonus if it is not yet revealed. (so that we don't overvalue the corp before the resource gets revealed)
		iBonuses += !kTeam.isHasTech((TechTypes)GC.getBonusInfo((BonusTypes)kCorp.getBonusProduced()).getTechReveal()) ? 1 : 0;
		iValue += AI_baseBonusVal((BonusTypes)kCorp.getBonusProduced()) * 100 / (1 + 3 * iBonuses * iBonuses);
	}

	return iValue;
	// K-Mod end
}

int CvPlayerAI::AI_areaMissionAIs(CvArea* pArea, MissionAITypes eMissionAI, CvSelectionGroup* pSkipSelectionGroup) const
{
	PROFILE_FUNC();

	CvSelectionGroup* pLoopSelectionGroup;
	CvPlot* pMissionPlot;
	int iCount;
	int iLoop;

	iCount = 0;

	for(pLoopSelectionGroup = firstSelectionGroup(&iLoop); pLoopSelectionGroup; pLoopSelectionGroup = nextSelectionGroup(&iLoop))
	{
		if (pLoopSelectionGroup != pSkipSelectionGroup)
		{
			if (pLoopSelectionGroup->AI_getMissionAIType() == eMissionAI)
			{
				pMissionPlot = pLoopSelectionGroup->AI_getMissionAIPlot();

				if (pMissionPlot != NULL)
				{
					if (pMissionPlot->area() == pArea)
					{
						iCount += pLoopSelectionGroup->getNumUnits();
					}
				}
			}
		}
	}

	return iCount;
}


int CvPlayerAI::AI_plotTargetMissionAIs(CvPlot* pPlot, MissionAITypes eMissionAI, CvSelectionGroup* pSkipSelectionGroup, int iRange) const
{
	int iClosestTargetRange;
	return AI_plotTargetMissionAIs(pPlot, &eMissionAI, 1, iClosestTargetRange, pSkipSelectionGroup, iRange);
}

int CvPlayerAI::AI_plotTargetMissionAIs(CvPlot* pPlot, MissionAITypes eMissionAI, int& iClosestTargetRange, CvSelectionGroup* pSkipSelectionGroup, int iRange) const
{
	return AI_plotTargetMissionAIs(pPlot, &eMissionAI, 1, iClosestTargetRange, pSkipSelectionGroup, iRange);
}

int CvPlayerAI::AI_plotTargetMissionAIs(CvPlot* pPlot, MissionAITypes* aeMissionAI, int iMissionAICount, int& iClosestTargetRange, CvSelectionGroup* pSkipSelectionGroup, int iRange) const
{
	PROFILE_FUNC();

	int iCount = 0;
	iClosestTargetRange = MAX_INT;

	int iLoop;
	for(CvSelectionGroup* pLoopSelectionGroup = firstSelectionGroup(&iLoop); pLoopSelectionGroup; pLoopSelectionGroup = nextSelectionGroup(&iLoop))
	{
		if (pLoopSelectionGroup != pSkipSelectionGroup)
		{
			CvPlot* pMissionPlot = pLoopSelectionGroup->AI_getMissionAIPlot();

			if (pMissionPlot != NULL)
			{
				MissionAITypes eGroupMissionAI = pLoopSelectionGroup->AI_getMissionAIType();
				int iDistance = stepDistance(pPlot->getX_INLINE(), pPlot->getY_INLINE(), pMissionPlot->getX_INLINE(), pMissionPlot->getY_INLINE());

				if (iDistance <= iRange)
				{
					for (int iMissionAIIndex = 0; iMissionAIIndex < iMissionAICount; iMissionAIIndex++)
					{
						if (eGroupMissionAI == aeMissionAI[iMissionAIIndex] || aeMissionAI[iMissionAIIndex] == NO_MISSIONAI)
						{
							iCount += pLoopSelectionGroup->getNumUnits();

							if (iDistance < iClosestTargetRange)
							{
								iClosestTargetRange = iDistance;
							}
						}
					}
				}
			}
		}
	}

	return iCount;
}

// K-Mod

// Total defensive strength of units that can move iRange steps to reach pDefencePlot
int CvPlayerAI::AI_localDefenceStrength(const CvPlot* pDefencePlot, TeamTypes eDefenceTeam, DomainTypes eDomainType, int iRange, bool bAtTarget, bool bCheckMoves) const
{
	PROFILE_FUNC();

	int	iTotal = 0;

	FAssert(bAtTarget || !bCheckMoves); // it doesn't make much sense to check moves if the defenders are meant to stay put.

	for (int iDX = -iRange; iDX <= iRange; iDX++)
	{
		for (int iDY = -iRange; iDY <= iRange; iDY++)
		{
			CvPlot* pLoopPlot = plotXY(pDefencePlot->getX_INLINE(), pDefencePlot->getY_INLINE(), iDX, iDY);
			if (pLoopPlot == NULL || !pLoopPlot->isVisible(getTeam(), false))
				continue;

			CLLNode<IDInfo>* pUnitNode = pLoopPlot->headUnitNode();

			int iPlotTotal = 0;
			while (pUnitNode != NULL)
			{
				CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
				pUnitNode = pLoopPlot->nextUnitNode(pUnitNode);

				if (pLoopUnit->getTeam() == eDefenceTeam
					|| (eDefenceTeam != NO_TEAM && GET_TEAM(pLoopUnit->getTeam()).isVassal(eDefenceTeam))
					|| (eDefenceTeam == NO_TEAM && isPotentialEnemy(getTeam(), pLoopUnit->getTeam())))
				{
					if (eDomainType == NO_DOMAIN || (pLoopUnit->getDomainType() == eDomainType))
					{
						if (bCheckMoves)
						{
							// unfortunately, we can't use the global pathfinder here
							// - because the calling function might be waiting to use some pathfinding results
							// So this check will have to be really rough. :(
							int iMoves = pLoopUnit->baseMoves();
							iMoves += pLoopPlot->isValidRoute(pLoopUnit) ? 1 : 0;
							int iDistance = std::max(std::abs(iDX), std::abs(iDY));
							if (iDistance > iMoves)
								continue; // can't make it. (maybe?)
						}
						int iUnitStr = pLoopUnit->currEffectiveStr(bAtTarget ? pDefencePlot : pLoopPlot, NULL);
						// first strikes are not counted in currEffectiveStr.
						// actually, the value of first strikes is non-trivial to calculate... but we should so /something/ to take them into account.
						iUnitStr *= 100 + 4 * pLoopUnit->firstStrikes() + 2 * pLoopUnit->chanceFirstStrikes();
						iUnitStr /= 100;
						// note. Most other parts of the code use 5% per first strike, but I figure we should go lower because this unit may get clobbered by collateral damage before fighting.
						iPlotTotal += iUnitStr;
					}
				}
			}
			if (eDefenceTeam == NO_TEAM && eDomainType == DOMAIN_LAND && !bCheckMoves && (!bAtTarget || pLoopPlot == pDefencePlot) && !isHuman())
			{
				// while since we're here, we might as well update our memory.
				// (not for human players, otherwise the pathfinder might put us out of sync)
				GET_TEAM(getTeam()).AI_setStrengthMemory(pLoopPlot, iPlotTotal);
				FAssert(isTurnActive());
			}
			iTotal += iPlotTotal;
		}
	}

	return iTotal;
}

// Total attack strength of units that can move iRange steps to reach pAttackPlot
int CvPlayerAI::AI_localAttackStrength(const CvPlot* pTargetPlot, TeamTypes eAttackTeam, DomainTypes eDomainType, int iRange, bool bUseTarget, bool bCheckMoves, bool bCheckCanAttack) const
{
	PROFILE_FUNC();

	int iBaseCollateral = bUseTarget
		? estimateCollateralWeight(pTargetPlot, pTargetPlot->getTeam() == getTeam() ? NO_TEAM : pTargetPlot->getTeam())
		: estimateCollateralWeight(0, NO_TEAM);

	int	iTotal = 0;

	for (int iDX = -iRange; iDX <= iRange; iDX++)
	{
		for (int iDY = -iRange; iDY <= iRange; iDY++)
		{
			CvPlot* pLoopPlot = plotXY(pTargetPlot->getX_INLINE(), pTargetPlot->getY_INLINE(), iDX, iDY);
			if (pLoopPlot == NULL || !pLoopPlot->isVisible(getTeam(), false))
				continue;

			CLLNode<IDInfo>* pUnitNode = pLoopPlot->headUnitNode();

			while (pUnitNode != NULL)
			{
				CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
				pUnitNode = pLoopPlot->nextUnitNode(pUnitNode);

				if (pLoopUnit->getTeam() == eAttackTeam	|| (eAttackTeam == NO_TEAM && atWar(getTeam(), pLoopUnit->getTeam())))
				{
					if (eDomainType == NO_DOMAIN || (pLoopUnit->getDomainType() == eDomainType))
					{
						if (!pLoopUnit->canAttack()) // bCheckCanAttack means something else.
							continue;

						if (bCheckMoves)
						{
							// unfortunately, we can't use the global pathfinder here
							// - because the calling function might be waiting to use some pathfinding results
							// So this check will have to be really rough. :(
							int iMoves = pLoopUnit->baseMoves();
							iMoves += pLoopPlot->isValidRoute(pLoopUnit) ? 1 : 0;
							int iDistance = std::max(std::abs(iDX), std::abs(iDY));
							if (iDistance > iMoves)
								continue; // can't make it. (maybe?)
						}
						if (bCheckCanAttack)
						{
							if (!pLoopUnit->canMove() || (pLoopUnit->isMadeAttack() && !pLoopUnit->isBlitz())
								|| (bUseTarget && pLoopUnit->combatLimit() < 100 && pLoopPlot->isWater() && !pTargetPlot->isWater()))
							{
								continue; // can't attack
							}
						}

						int iUnitStr = pLoopUnit->currEffectiveStr(bUseTarget ? pTargetPlot : NULL, bUseTarget ? pLoopUnit : NULL);
						// first strikes. (see comments in AI_localDefenceStrength... although, maybe 5% would be better here?)
						iUnitStr *= 100 + 4 * pLoopUnit->firstStrikes() + 2 * pLoopUnit->chanceFirstStrikes();
						iUnitStr /= 100;
						//
						if (pLoopUnit->collateralDamage() > 0)
						{
							int iPossibleTargets = std::min(bUseTarget ? pTargetPlot->getNumVisibleEnemyDefenders(pLoopUnit) - 1 : INT_MAX, pLoopUnit->collateralDamageMaxUnits());

							if (iPossibleTargets > 0)
							{
								// collateral damage is not trivial to calculate. This estimate is pretty rough. (cf with calculation in AI_sumStrength)
								// (Note: collateralDamage() and iBaseCollateral both include factors of 100.)
								iUnitStr += pLoopUnit->baseCombatStr() * iBaseCollateral * pLoopUnit->collateralDamage() * iPossibleTargets / 10000;
							}
						}
						iTotal += iUnitStr;
					}
				}
			}
		}
	}

	return iTotal;
}
// K-Mod end

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      04/03/10                                jdog5000      */
/*                                                                                              */
/* General AI                                                                                   */
/************************************************************************************************/

// K-Mod. I've changed this function to calculation the attack power of our groups, rather than just the number of units.
// I've also changed it to use a separate instance of the path finder, so that it doesn't clear the reset the existing path data.

//int CvPlayerAI::AI_cityTargetUnitsByPath(CvCity* pCity, CvSelectionGroup* pSkipSelectionGroup, int iMaxPathTurns) const
int CvPlayerAI::AI_cityTargetStrengthByPath(CvCity* pCity, CvSelectionGroup* pSkipSelectionGroup, int iMaxPathTurns) const
{
	PROFILE_FUNC();

	//int iCount = 0;
	int iTotalStrength = 0;
	KmodPathFinder temp_finder;

	int iLoop;
	//int iPathTurns;
	for(CvSelectionGroup* pLoopSelectionGroup = firstSelectionGroup(&iLoop); pLoopSelectionGroup; pLoopSelectionGroup = nextSelectionGroup(&iLoop))
	{
		if (pLoopSelectionGroup != pSkipSelectionGroup && pLoopSelectionGroup->getNumUnits() > 0)
		{
			FAssert(pLoopSelectionGroup->plot() != NULL); // this use to be part of the above condition. I've turned it into an assert. K-Mod.
			CvPlot* pMissionPlot = pLoopSelectionGroup->AI_getMissionAIPlot();

			if (pMissionPlot != NULL)
			{
				int iDistance = stepDistance(pCity->getX_INLINE(), pCity->getY_INLINE(), pMissionPlot->getX_INLINE(), pMissionPlot->getY_INLINE());

				if (iDistance <= 1 && pLoopSelectionGroup->canFight())
				{
					/* original bbai code
					if( pLoopSelectionGroup->generatePath(pLoopSelectionGroup->plot(), pMissionPlot, 0, true, &iPathTurns, iMaxPathTurns) )
					{
						if( !(pLoopSelectionGroup->canAllMove()) )
						{
							iPathTurns++;
						}

						if( iPathTurns <= iMaxPathTurns )
						{
							iCount += pLoopSelectionGroup->getNumUnits();
						}
					} */
					temp_finder.SetSettings(CvPathSettings(pLoopSelectionGroup, 0, iMaxPathTurns, GC.getMOVE_DENOMINATOR()));
					if (temp_finder.GeneratePath(pMissionPlot))
					{
						iTotalStrength += pLoopSelectionGroup->AI_sumStrength(pCity->plot(), DOMAIN_LAND);
					}
				}
			}
		}
	}

	//return iCount;
	return iTotalStrength;
}

int CvPlayerAI::AI_unitTargetMissionAIs(CvUnit* pUnit, MissionAITypes eMissionAI, CvSelectionGroup* pSkipSelectionGroup) const
{
	return AI_unitTargetMissionAIs(pUnit, &eMissionAI, 1, pSkipSelectionGroup, -1);
}

int CvPlayerAI::AI_unitTargetMissionAIs(CvUnit* pUnit, MissionAITypes* aeMissionAI, int iMissionAICount, CvSelectionGroup* pSkipSelectionGroup) const
{
	return AI_unitTargetMissionAIs(pUnit, aeMissionAI, iMissionAICount, pSkipSelectionGroup, -1);
}

int CvPlayerAI::AI_unitTargetMissionAIs(CvUnit* pUnit, MissionAITypes* aeMissionAI, int iMissionAICount, CvSelectionGroup* pSkipSelectionGroup, int iMaxPathTurns) const
{
	PROFILE_FUNC();

	CvSelectionGroup* pLoopSelectionGroup;
	int iCount;
	int iLoop;

	iCount = 0;

	for(pLoopSelectionGroup = firstSelectionGroup(&iLoop); pLoopSelectionGroup; pLoopSelectionGroup = nextSelectionGroup(&iLoop))
	{
		if (pLoopSelectionGroup != pSkipSelectionGroup)
		{
			if (pLoopSelectionGroup->AI_getMissionAIUnit() == pUnit)
			{
				MissionAITypes eGroupMissionAI = pLoopSelectionGroup->AI_getMissionAIType();
				int iPathTurns = MAX_INT;

				if( iMaxPathTurns >= 0 && (pUnit->plot() != NULL) && (pLoopSelectionGroup->plot() != NULL))
				{
					pLoopSelectionGroup->generatePath(pLoopSelectionGroup->plot(), pUnit->plot(), 0, false, &iPathTurns);
					if( !(pLoopSelectionGroup->canAllMove()) )
					{
						iPathTurns++;
					}
				}

				if ((iMaxPathTurns == -1) || (iPathTurns <= iMaxPathTurns))
				{
					for (int iMissionAIIndex = 0; iMissionAIIndex < iMissionAICount; iMissionAIIndex++)
					{
						if (eGroupMissionAI == aeMissionAI[iMissionAIIndex] || NO_MISSIONAI == aeMissionAI[iMissionAIIndex])
						{
							iCount += pLoopSelectionGroup->getNumUnits();
						}
					}
				}
			}
		}
	}

	return iCount;
}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/		

int CvPlayerAI::AI_enemyTargetMissionAIs(MissionAITypes eMissionAI, CvSelectionGroup* pSkipSelectionGroup) const
{
	return AI_enemyTargetMissionAIs(&eMissionAI, 1, pSkipSelectionGroup);
}

int CvPlayerAI::AI_enemyTargetMissionAIs(MissionAITypes* aeMissionAI, int iMissionAICount, CvSelectionGroup* pSkipSelectionGroup) const
{
	PROFILE_FUNC();

	int iCount = 0;

	int iLoop;
	for(CvSelectionGroup* pLoopSelectionGroup = firstSelectionGroup(&iLoop); pLoopSelectionGroup; pLoopSelectionGroup = nextSelectionGroup(&iLoop))
	{
		if (pLoopSelectionGroup != pSkipSelectionGroup)
		{
			CvPlot* pMissionPlot = pLoopSelectionGroup->AI_getMissionAIPlot();

			if (NULL != pMissionPlot && pMissionPlot->isOwned())
			{
				MissionAITypes eGroupMissionAI = pLoopSelectionGroup->AI_getMissionAIType();
				for (int iMissionAIIndex = 0; iMissionAIIndex < iMissionAICount; iMissionAIIndex++)
				{
					if (eGroupMissionAI == aeMissionAI[iMissionAIIndex] || NO_MISSIONAI == aeMissionAI[iMissionAIIndex])
					{
						if (GET_TEAM(getTeam()).AI_isChosenWar(pMissionPlot->getTeam()))
						{
							iCount += pLoopSelectionGroup->getNumUnits();
							iCount += pLoopSelectionGroup->getCargo();
						}
					}
				}
			}
		}
	}

	return iCount;
}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      05/19/10                                jdog5000      */
/*                                                                                              */
/* General AI                                                                                   */
/************************************************************************************************/
int CvPlayerAI::AI_enemyTargetMissions(TeamTypes eTargetTeam, CvSelectionGroup* pSkipSelectionGroup) const
{
	PROFILE_FUNC();

	int iCount = 0;

	int iLoop;
	for(CvSelectionGroup* pLoopSelectionGroup = firstSelectionGroup(&iLoop); pLoopSelectionGroup; pLoopSelectionGroup = nextSelectionGroup(&iLoop))
	{
		if (pLoopSelectionGroup != pSkipSelectionGroup)
		{
			CvPlot* pMissionPlot = pLoopSelectionGroup->AI_getMissionAIPlot();

			if( pMissionPlot == NULL )
			{
				pMissionPlot = pLoopSelectionGroup->plot();
			}

			if (NULL != pMissionPlot )
			{
				if( pMissionPlot->isOwned() && pMissionPlot->getTeam() == eTargetTeam )
				{
					if (atWar(getTeam(),pMissionPlot->getTeam()) || pLoopSelectionGroup->AI_isDeclareWar(pMissionPlot))
					{
						iCount += pLoopSelectionGroup->getNumUnits();
						iCount += pLoopSelectionGroup->getCargo();
					}
				}
			}
		}
	}

	return iCount;
}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

int CvPlayerAI::AI_wakePlotTargetMissionAIs(CvPlot* pPlot, MissionAITypes eMissionAI, CvSelectionGroup* pSkipSelectionGroup) const
{
	PROFILE_FUNC();

	FAssert(pPlot != NULL);
	
	int iCount = 0;

	int iLoop;
	for(CvSelectionGroup* pLoopSelectionGroup = firstSelectionGroup(&iLoop); pLoopSelectionGroup; pLoopSelectionGroup = nextSelectionGroup(&iLoop))
	{
		if (pLoopSelectionGroup != pSkipSelectionGroup)
		{
			MissionAITypes eGroupMissionAI = pLoopSelectionGroup->AI_getMissionAIType();
			if (eMissionAI == NO_MISSIONAI || eMissionAI == eGroupMissionAI)
			{
				CvPlot* pMissionPlot = pLoopSelectionGroup->AI_getMissionAIPlot();
				if (pMissionPlot != NULL && pMissionPlot == pPlot)
				{
					iCount += pLoopSelectionGroup->getNumUnits();
					pLoopSelectionGroup->setActivityType(ACTIVITY_AWAKE);
				}
			}
		}
	}

	return iCount;
}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      07/19/09                                jdog5000      */
/*                                                                                              */
/* Civic AI                                                                                     */
/************************************************************************************************/
CivicTypes CvPlayerAI::AI_bestCivic(CivicOptionTypes eCivicOption) const
{
	int iBestValue;
	return AI_bestCivic( eCivicOption, &iBestValue );
}

CivicTypes CvPlayerAI::AI_bestCivic(CivicOptionTypes eCivicOption, int* iBestValue) const
{
	CivicTypes eBestCivic;
	int iValue;
	int iI;

	(*iBestValue) = MIN_INT;
	eBestCivic = NO_CIVIC;

	for (iI = 0; iI < GC.getNumCivicInfos(); iI++)
	{
		if (GC.getCivicInfo((CivicTypes)iI).getCivicOptionType() == eCivicOption)
		{
			if (canDoCivics((CivicTypes)iI))
			{
				iValue = AI_civicValue((CivicTypes)iI);

				if (iValue > (*iBestValue))
				{
					(*iBestValue) = iValue;
					eBestCivic = ((CivicTypes)iI);
				}
			}
		}
	}

	return eBestCivic;
}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/		

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      05/15/10                                jdog5000      */
/*                                                                                              */
/* Civic AI, Victory Strategy AI                                                                */
/************************************************************************************************/
int CvPlayerAI::AI_civicValue(CivicTypes eCivic) const
{
	PROFILE_FUNC();

	const CvTeamAI& kTeam = GET_TEAM(getTeam()); // K-Mod

	bool bWarPlan;
	int iConnectedForeignCities;
	int iTotalReligonCount;
	int iHighestReligionCount;
	int iWarmongerPercent;
	int iHappiness;
	int iValue;
	int iTempValue;
	int iI, iJ;
	int iCities = getNumCities();
	bool bCultureVictory3 = AI_isDoVictoryStrategy(AI_VICTORY_CULTURE3);
	bool bCultureVictory2 = AI_isDoVictoryStrategy(AI_VICTORY_CULTURE2);

	FAssertMsg(eCivic < GC.getNumCivicInfos(), "eCivic is expected to be within maximum bounds (invalid Index)");
	FAssertMsg(eCivic >= 0, "eCivic is expected to be non-negative (invalid Index)");

/************************************************************************************************/
/* UNOFFICIAL_PATCH                       10/05/09                                jdog5000      */
/*                                                                                              */
/* Bugfix                                                                                       */
/************************************************************************************************/
	// Circumvents crash bug in simultaneous turns MP games
	if( eCivic == NO_CIVIC )
	{
		return 1;
	}
/************************************************************************************************/
/* UNOFFICIAL_PATCH                        END                                                  */
/************************************************************************************************/

	if( isBarbarian() )
	{
		return 1;
	}

	CvCivicInfo& kCivic = GC.getCivicInfo(eCivic);

	int iS = (isCivic(eCivic)?-1 :1);// K-Mod, sign for whether we should be considering gaining a bonus, or losing a bonus

	bWarPlan = (kTeam.getAnyWarPlanCount(true) > 0);
	if( bWarPlan )
	{
		bWarPlan = false;
		int iEnemyWarSuccess = 0;

		for( int iTeam = 0; iTeam < MAX_CIV_TEAMS; iTeam++ )
		{
			if( GET_TEAM((TeamTypes)iTeam).isAlive() && !GET_TEAM((TeamTypes)iTeam).isMinorCiv() )
			{
				if( kTeam.AI_getWarPlan((TeamTypes)iTeam) != NO_WARPLAN )
				{
					if( kTeam.AI_getWarPlan((TeamTypes)iTeam) == WARPLAN_TOTAL || kTeam.AI_getWarPlan((TeamTypes)iTeam) == WARPLAN_PREPARING_TOTAL )
					{
						bWarPlan = true;
						break;
					}

					if( kTeam.AI_isLandTarget((TeamTypes)iTeam) )
					{
						bWarPlan = true;
						break;
					}

					iEnemyWarSuccess += GET_TEAM((TeamTypes)iTeam).AI_getWarSuccess(getTeam());
				}
			}
		}

		if( !bWarPlan )
		{
			if( iEnemyWarSuccess > std::min(iCities, 4) * GC.getWAR_SUCCESS_CITY_CAPTURING() )
			{
				// Lots of fighting, so war is real
				bWarPlan = true;
			}
			else if( iEnemyWarSuccess > std::min(iCities, 2) * GC.getWAR_SUCCESS_CITY_CAPTURING() )
			{
				if( kTeam.AI_getEnemyPowerPercent() > 120 )
				{
					bWarPlan = true;
				}
			}
		}
	}

	if( !bWarPlan )
	{
		// Aggressive players will stick with war civics
		if( kTeam.AI_getTotalWarOddsTimes100() > 200 )
		{
			bWarPlan = true;
		}
	}

	iConnectedForeignCities = countPotentialForeignTradeCitiesConnected();
	iTotalReligonCount = countTotalHasReligion();
	ReligionTypes eBestReligion = AI_bestReligion();
	if (eBestReligion == NO_RELIGION)
	{
		eBestReligion = getStateReligion();
	}
	iHighestReligionCount = ((eBestReligion == NO_RELIGION) ? 0 : getHasReligionCount(eBestReligion));
	iWarmongerPercent = 25000 / std::max(100, (100 + GC.getLeaderHeadInfo(getPersonalityType()).getMaxWarRand())); 

	iValue = (iCities * 6);

	iValue += (GC.getCivicInfo(eCivic).getAIWeight() * iCities);

	// K-Mod: civic anger is counted somewhere else
	//iValue += (getCivicPercentAnger(eCivic) / 10);

	iValue += -(GC.getCivicInfo(eCivic).getAnarchyLength() * iCities);

	iValue += -(getSingleCivicUpkeep(eCivic, true)*80)/100;

	int iTemp = 0;
	CvCity* pCapital = getCapitalCity();
	iValue += ((kCivic.getGreatPeopleRateModifier() * iCities) / 10);
	iValue += ((kCivic.getGreatGeneralRateModifier() * getNumMilitaryUnits()) / 50);
	iValue += ((kCivic.getDomesticGreatGeneralRateModifier() * getNumMilitaryUnits()) / 100);
	iValue += -((kCivic.getDistanceMaintenanceModifier() * std::max(0, (iCities - 3))) / 8);
	iValue += -((kCivic.getNumCitiesMaintenanceModifier() * std::max(0, (iCities - 3))) / 8);
	iTemp = kCivic.getFreeExperience();
	if( iTemp > 0 )
	{
		// Free experience increases value of hammers spent on units, population is an okay measure of base hammer production
		iTempValue = (iTemp * getTotalPopulation() * (bWarPlan ? 30 : 12))/100;
		iTempValue *= AI_averageYieldMultiplier(YIELD_PRODUCTION);
		iTempValue /= 100;
		iTempValue *= iWarmongerPercent;
		iTempValue /= 100;
		iValue += iTempValue;
	}

	iValue += ((kCivic.getWorkerSpeedModifier() * AI_getNumAIUnits(UNITAI_WORKER)) / 15);
	iValue += ((kCivic.getImprovementUpgradeRateModifier() * iCities) / 50);
	iValue += (kCivic.getMilitaryProductionModifier() * iCities * iWarmongerPercent) / (bWarPlan ? 300 : 500 ); 
	iValue += (kCivic.getBaseFreeUnits() / 2);
	iValue += (kCivic.getBaseFreeMilitaryUnits() / 2);
	iValue += ((kCivic.getFreeUnitsPopulationPercent() * getTotalPopulation()) / 200);
	iValue += ((kCivic.getFreeMilitaryUnitsPopulationPercent() * getTotalPopulation()) / 300);
	iValue += -(kCivic.getGoldPerUnit() * getNumUnits());
	iValue += -(kCivic.getGoldPerMilitaryUnit() * getNumMilitaryUnits() * iWarmongerPercent) / 200;

	//iValue += ((kCivic.isMilitaryFoodProduction()) ? 0 : 0);
	iTemp = getWorldSizeMaxConscript(eCivic);
	if( iTemp > 0 && (pCapital != NULL) )
	{
		UnitTypes eConscript = pCapital->getConscriptUnit();
		if( eConscript != NO_UNIT )
		{
			// Nationhood
			int iCombatValue = GC.getGameINLINE().AI_combatValue(eConscript);
			if( iCombatValue > 33 )
			{
				iTempValue = iCities + ((bWarPlan) ? 30 : 10);

				iTempValue *= range(kTeam.AI_getEnemyPowerPercent(), 50, 300);
				iTempValue /= 100;

				iTempValue *= iCombatValue;
				iTempValue /= 75;

				int iWarSuccessRating = kTeam.AI_getWarSuccessRating();
				if( iWarSuccessRating < -25 )
				{
					iTempValue *= 75 + range(-iWarSuccessRating, 25, 100);
					iTempValue /= 100;
				}
				// K-Mod (maybe I'll do some more tweaking later)
				// (NOTE: "conscript_population_per_cost" is actually "production_per_conscript_population". The developers didn't know what "per" means.)
				int iConscriptPop = std::max(1, GC.getUnitInfo(eConscript).getProductionCost() / GC.getDefineINT("CONSCRIPT_POPULATION_PER_COST"));
				iTempValue *= GC.getUnitInfo(eConscript).getProductionCost();
				iTempValue /= iConscriptPop * GC.getDefineINT("CONSCRIPT_POPULATION_PER_COST");
				iTempValue *= std::min(iCities, iTemp*3);
				iTempValue /= iTemp*3;
				// K-Mod end

				iValue += iTempValue;
			}
		}
	}
/*
** K-Mod.
** evaluation of my new unhealthiness modifier
*/
	iValue += (iCities * 6 * iS * AI_getHealthWeight(-iS*kCivic.getUnhealthyPopulationModifier(), 1, true)) / 100;
	// c.f	iValue += (iCities * 6 * AI_getHealthWeight(kCivic.getExtraHealth(), 1)) / 100;

	// If the GW threshold has been reached, increase the value based on GW anger
	if (GC.getGameINLINE().getGlobalWarmingIndex() > 0)
	{
		// estimate the happiness boost...
		// suppose pop pollution is 1/3 of total, and current relative contribution is around 100%
		// and anger percent scales like 2* relative contribution...
		// anger percent reduction will then be around (kCivic.getUnhealthyPopulationModifier()*2/3)%

		// Unfortunately, since adopting this civic will lower the very same anger percent that is giving the civic value...
		// the civic will be valued lower as soon as it is adopted. :(
		// Fixing this problem (and others like it) is a bit tricky. For now, I'll just try to fudge around it.
		int iGwAnger = getGwPercentAnger();
		if (isCivic(eCivic)) // Fudge factor
		{
			iGwAnger *= 100;
			iGwAnger /= 100 - 2*kCivic.getUnhealthyPopulationModifier()/3;
			// Note, this fudge factor is actually pretty good at estimating what the GwAnger would have been.
		}
		int iCleanValue = (iCities * 12 * iS * AI_getHappinessWeight(iS*ROUND_DIVIDE(-kCivic.getUnhealthyPopulationModifier()*iGwAnger*2,300), 1, true)) / 100;
		// This isn't a big reduction; and it should be the only part of this evaluation.
		// Maybe I'll add more later; such as some flavour factors.

		iValue += iCleanValue;
	}
// K-Mod end
	if (bWarPlan)
	{
		iValue += ((kCivic.getExpInBorderModifier() * getNumMilitaryUnits()) / 200);
	}
	iValue += ((kCivic.isBuildingOnlyHealthy()) ? (iCities * 3) : 0);
	iValue += -((kCivic.getWarWearinessModifier() * iCities) / ((bWarPlan) ? 10 : 50));
	//iValue += (kCivic.getFreeSpecialist() * iCities * 12);
	iValue += (kCivic.getFreeSpecialist() * iCities * 14); // this lacks the usual K-Mod rigour, but it will do for now.

	/*
	iValue += (kCivic.getTradeRoutes() * (std::max(0, iConnectedForeignCities - iCities * 3) * 6 + (iCities * 2)));
	iValue += -((kCivic.isNoForeignTrade()) ? (iConnectedForeignCities * 3) : 0); */

	// K-Mod - take a few more things into account for trade routes.
	iTempValue = (kCivic.getTradeRoutes() * (std::max(0, iConnectedForeignCities - iCities * 3) * 3 + (iCities * 1)));
	iTempValue += -((kCivic.isNoForeignTrade()) ? (iConnectedForeignCities * 3 / 2) : 0);
	// Trade routes increase in value as cities grow, and build trade multipliers
	iTempValue *= (getCurrentEra() + GC.getNumEraInfos());
	iTempValue /= GC.getNumEraInfos();
	// commerce multipliers
	iTempValue *= AI_averageYieldMultiplier(YIELD_COMMERCE);
	iTempValue /= 100;
	iValue += iTempValue;

	/* original bts code
	if (kCivic.isNoCorporations())
	{
		iValue -= countHeadquarters() * (40 + 3 * getNumCities());
	}
	if (kCivic.isNoForeignCorporations())
	{
		for (int iCorp = 0; iCorp < GC.getNumCorporationInfos(); ++iCorp)
		{
			if (!GET_TEAM(getTeam()).hasHeadquarters((CorporationTypes)iCorp))
			{
				iValue += countCorporations((CorporationTypes)iCorp) * 3;
			}
		}
	}
	if (kCivic.getCorporationMaintenanceModifier() != 0)
	{
		int iCorpCount = 0;
		int iHQCount = 0;
		for (int iCorp = 0; iCorp < GC.getNumCorporationInfos(); ++iCorp)
		{
			if (GET_TEAM(getTeam()).hasHeadquarters((CorporationTypes)iCorp))
			{
				iHQCount++;
			}
			iCorpCount += countCorporations((CorporationTypes)iCorp);
		}
		iValue += (-kCivic.getCorporationMaintenanceModifier() * (iHQCount * (25 + getNumCities() * 2) + iCorpCount * 7)) / 25;

	}*/

	// Corporations (K-Mod edition!)
	if (kCivic.isNoCorporations() || kCivic.isNoForeignCorporations() || kCivic.getCorporationMaintenanceModifier() != 0)
	{
		for (CorporationTypes eCorp = (CorporationTypes)0; eCorp < GC.getNumCorporationInfos(); eCorp=(CorporationTypes)(eCorp+1))
		{
			if (!GC.getGameINLINE().isCorporationFounded(eCorp))
				continue;

			bool bPlayerHQ = false;
			bool bTeamHQ = false;
			if (hasHeadquarters(eCorp))
			{
				bPlayerHQ = true;
				bTeamHQ = true;
			}
			else if (kTeam.hasHeadquarters(eCorp))
			{
				bTeamHQ = true;
			}

			int iBonuses = 0;
			int iCorpCities = countCorporations(eCorp);
			int iMaintenance = 0;
			// If the HQ is ours, assume we will spread the corp. If it is not our, assume we don't care.
			// (Note: this doesn't take into account the posibility of competing corps. Sorry.)
			if (bTeamHQ)
			{
				iCorpCities = ((bPlayerHQ ?1 :2)*iCorpCities + iCities)/(bPlayerHQ ?2 :3);
			}

			for (int i = 0; i < GC.getNUM_CORPORATION_PREREQ_BONUSES(); ++i)
			{
				BonusTypes eBonus = (BonusTypes)GC.getCorporationInfo(eCorp).getPrereqBonus(i);
				if (NO_BONUS != eBonus)
				{
					iBonuses += countOwnedBonuses(eBonus);
					// maybe use getNumAvailableBonuses ?
					iBonuses +=	!kTeam.isHasTech((TechTypes)GC.getBonusInfo(eBonus).getTechReveal()) && !kTeam.isForceRevealedBonus(eBonus)
						? 1 : 0; // expect that we'll get at least one of each unrevealed bonus.
				}
			}
			iBonuses += bPlayerHQ ? 1 : 0;

			for (iI = (CommerceTypes)0; iI < NUM_COMMERCE_TYPES; ++iI)
			{
				iTempValue = 0;

				// loss of the headquarter bonus from our cities.
				if (bTeamHQ &&
					( (kCivic.isNoForeignCorporations() && !bPlayerHQ) ||
					kCivic.isNoCorporations() ))
				{
					//iTempValue -= (3*GC.getCorporationInfo(eCorp).getHeadquarterCommerce((CommerceTypes)iI) * iCorpCities)/2;
					iTempValue -= (GC.getGameINLINE().getHeadquarters(eCorp)->getCommerceRateModifier((CommerceTypes)iI) * GC.getCorporationInfo(eCorp).getHeadquarterCommerce((CommerceTypes)iI) * iCorpCities)/100;
				}

				// loss of corp commerce bonuses
				if (kCivic.isNoCorporations() || (kCivic.isNoForeignCorporations() && !bPlayerHQ))
				{
					iTempValue -= iCorpCities * ((AI_averageCommerceMultiplier((CommerceTypes)iI) * GC.getCorporationInfo(eCorp).getCommerceProduced((CommerceTypes)iI))/100 * iBonuses * GC.getWorldInfo(GC.getMapINLINE().getWorldSize()).getCorporationMaintenancePercent()) / 10000;
				}

				iTempValue *= AI_commerceWeight((CommerceTypes)iI);
				iTempValue /= 100;

				iValue += iTempValue;

				iMaintenance += GC.getCorporationInfo(eCorp).getHeadquarterCommerce(iI) * iCorpCities;
			}
			// loss of corp yield bonuses
			if (kCivic.isNoCorporations() || (kCivic.isNoForeignCorporations() && !bPlayerHQ))
			{
				for (iI = 0; iI < NUM_YIELD_TYPES; ++iI)
				{
					iTempValue = -(iCorpCities * GC.getCorporationInfo(eCorp).getYieldProduced((YieldTypes)iI) * iBonuses);
					iTempValue *= AI_averageYieldMultiplier((YieldTypes)iI);
					iTempValue /= 100;
					iTempValue *= GC.getWorldInfo(GC.getMapINLINE().getWorldSize()).getCorporationMaintenancePercent();
					iTempValue /= 10000; // getYieldProduced is x100.

					iTempValue *= AI_yieldWeight((YieldTypes)iI);
					iTempValue /= 100;

					iValue += iTempValue;
				}
			}

			// loss of maintenance cost (money saved)
			iTempValue = GC.getCorporationInfo(eCorp).getMaintenance() * iBonuses * iCorpCities;
			iTempValue *= GC.getWorldInfo(GC.getMapINLINE().getWorldSize()).getCorporationMaintenancePercent();
			iTempValue /= 10000;
			iTempValue += iMaintenance;
			// Inflation, population, and maintenance modifiers... lets just approximate them like this:
			iTempValue *= 2;
			iTempValue /= 3;

			iTempValue *= AI_commerceWeight(COMMERCE_GOLD);
			iTempValue /= 100;

			if (kCivic.isNoCorporations() || (kCivic.isNoForeignCorporations() && !bPlayerHQ))
			{
				iValue += iTempValue;
			}
			else
			{
				iValue += (-kCivic.getCorporationMaintenanceModifier() * iTempValue)/100;
			}
		}
	}
	// K-Mod end


	if (kCivic.getCivicPercentAnger() != 0)
	{
		/* original bts code. (and let me just say "wtf". This is an abomination)
		int iNumOtherCities = GC.getGameINLINE().getNumCities() - getNumCities();
		iValue += (30 * getNumCities() * getCivicPercentAnger(eCivic, true)) / kCivic.getCivicPercentAnger();
		
		int iTargetGameTurn = 2 * getNumCities() * GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).getGrowthPercent();
		iTargetGameTurn /= GC.getGame().countCivPlayersEverAlive();
		iTargetGameTurn += GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).getGrowthPercent() * 30;
		
		iTargetGameTurn /= 100;
		iTargetGameTurn = std::max(10, iTargetGameTurn);
		
		int iElapsedTurns = GC.getGame().getElapsedGameTurns();

		if (iElapsedTurns > iTargetGameTurn)
		{
			iValue += (std::min(iTargetGameTurn, iElapsedTurns - iTargetGameTurn) * (iNumOtherCities * kCivic.getCivicPercentAnger())) / (15 * iTargetGameTurn);
		}*/

		// K-Mod version.
		// value for negation of unhappiness. Again, let me point out how _stupid_ it is that "percent_anger_divisor" is 1000, not 100.
		iValue += (iCities * 12 * iS * AI_getHappinessWeight(iS*getCivicPercentAnger(eCivic, true)*100/GC.getPERCENT_ANGER_DIVISOR(), 2, true)) / 100;
		// value for putting pressure on other civs. (this should probably take into account the civics of other civs)
		iValue += kCivic.getCivicPercentAnger() * (iCities * iCities - iCities) / (5*GC.getGameINLINE().getNumCities()); // the 5* on the end is because "percent" is really per mil.
	}

	if (kCivic.getExtraHealth() != 0)
	{
/* orginal bts code
		iValue += (getNumCities() * 6 * AI_getHealthWeight(isCivic(eCivic) ? -kCivic.getExtraHealth() : kCivic.getExtraHealth(), 1)) / 100;
*/
		iValue += (iCities * 6 * iS * AI_getHealthWeight(iS*kCivic.getExtraHealth(), 1)) / 100;
	}
			
	iTempValue = kCivic.getHappyPerMilitaryUnit() * 3;
	if (iTempValue != 0)
	{
/* orginal bts code
		iValue += (getNumCities() * 9 * AI_getHappinessWeight(isCivic(eCivic) ? -iTempValue : iTempValue, 1)) / 100;
*/
		iValue += (iCities * 9 * iS * AI_getHappinessWeight(iS*iTempValue, 1)) / 100;
	}
		
	iTempValue = kCivic.getLargestCityHappiness();
	if (iTempValue != 0)
	{
/* orginal bts code
		iValue += (12 * std::min(getNumCities(), GC.getWorldInfo(GC.getMapINLINE().getWorldSize()).getTargetNumCities()) * AI_getHappinessWeight(isCivic(eCivic) ? -iTempValue : iTempValue, 1)) / 100;
*/
		iValue += (14 * std::min(iCities, GC.getWorldInfo(GC.getMapINLINE().getWorldSize()).getTargetNumCities()) * iS * AI_getHappinessWeight(iS*iTempValue, 1)) / 100;
	}
	
	if (kCivic.getWarWearinessModifier() != 0)
	{
#if 0 // K-Mod: commenting out the old code
		int iAngerPercent = getWarWearinessPercentAnger();
		int iPopulation = 3 + (getTotalPopulation() / std::max(1, getNumCities()));

		int iTempValue = (-kCivic.getWarWearinessModifier() * iAngerPercent * iPopulation) / (GC.getPERCENT_ANGER_DIVISOR() * 100);
		if (iTempValue != 0)
		{
/************************************************************************************************/
/* UNOFFICIAL_PATCH                       10/21/09                                jdog5000      */
/*                                                                                              */
/* Bugfix                                                                                       */
/************************************************************************************************/
/* orginal bts code
			iValue += (11 * getNumCities() * AI_getHappinessWeight(isCivic(eCivic) ? -iTempValue : iTempValue, 1)) / 100;
*/
			iValue += (11 * getNumCities() * AI_getHappinessWeight(iTempValue, 1)) / 100;
/************************************************************************************************/
/* UNOFFICIAL_PATCH                        END                                                  */
/************************************************************************************************/
		}
#endif
		// K-Mod; my version:
		iValue += (12 * iCities * iS * AI_getHappinessWeight(iS*ROUND_DIVIDE(getWarWearinessPercentAnger() * -getWarWearinessModifier(), GC.getPERCENT_ANGER_DIVISOR()), 1, true)) / 100;
	}
	
	iValue += (kCivic.getNonStateReligionHappiness() * (iTotalReligonCount - iHighestReligionCount) * 5);

	if (kCivic.isStateReligion())
	{
		if (iHighestReligionCount > 0)
		{
			iValue += iHighestReligionCount;

			iValue += ((kCivic.isNoNonStateReligionSpread()) ? ((iCities - iHighestReligionCount) * 2) : 0);
			iValue += (kCivic.getStateReligionHappiness() * iHighestReligionCount * 4);
			iValue += ((kCivic.getStateReligionGreatPeopleRateModifier() * iHighestReligionCount) / 20);
			iValue += (kCivic.getStateReligionGreatPeopleRateModifier() / 4);
			iValue += ((kCivic.getStateReligionUnitProductionModifier() * iHighestReligionCount) / 4);
			iValue += ((kCivic.getStateReligionBuildingProductionModifier() * iHighestReligionCount) / 3);
			iValue += (kCivic.getStateReligionFreeExperience() * iHighestReligionCount * ((bWarPlan) ? 6 : 2));

			// Value civic based on current gains from having a state religion
			for (int iI = 0; iI < GC.getNumVoteSourceInfos(); ++iI)
			{
				if (GC.getGameINLINE().isDiploVote((VoteSourceTypes)iI))
				{
					ReligionTypes eReligion = GC.getGameINLINE().getVoteSourceReligion((VoteSourceTypes)iI);

					if( NO_RELIGION != eReligion && eReligion == eBestReligion )
					{
						// Are we leader of AP?
						if( getTeam() == GC.getGameINLINE().getSecretaryGeneral((VoteSourceTypes)iI) )
						{
							iValue += 100;
						}

						// Any benefits we get from AP tied to state religion?
						/*
						for (int iYield = 0; iYield < NUM_YIELD_TYPES; ++iYield)
						{
							iTempValue = iHighestReligionCount*GC.getVoteSourceInfo((VoteSourceTypes)iI).getReligionYield(iYield);

							iTempValue *= AI_yieldWeight((YieldTypes)iYield);
							iTempValue /= 100;

							iValue += iTempValue;
						}

						for (int iCommerce = 0; iCommerce < NUM_COMMERCE_TYPES; ++iCommerce)
						{
							iTempValue = (iHighestReligionCount*GC.getVoteSourceInfo((VoteSourceTypes)iI).getReligionCommerce(iCommerce))/2;

							iTempValue *= AI_commerceWeight((CommerceTypes)iCommerce);
							iTempValue = 100;

							iValue += iTempValue;
						}
						*/
					}
				}
			}

			// Value civic based on wonders granting state religion boosts
			for (int iCommerce = 0; iCommerce < NUM_COMMERCE_TYPES; ++iCommerce)
			{
				iTempValue = (iHighestReligionCount * getStateReligionBuildingCommerce((CommerceTypes)iCommerce))/2;

				// K-Mod
				iTempValue *= AI_averageCommerceMultiplier((CommerceTypes)iCommerce);
				iTempValue /= 100;
				// K-mod end

				iTempValue *= AI_commerceWeight((CommerceTypes)iCommerce);
				iTempValue /= 100;

				iValue += iTempValue;
			}
		}
	}

	for (iI = 0; iI < NUM_YIELD_TYPES; iI++)
	{
		iTempValue = 0;

		iTempValue += ((kCivic.getYieldModifier(iI) * iCities) / 2);
		
		if (pCapital) 
		{
			// Bureaucracy
			// Benefit of having a supercity is higher than just increases in yield since will also win more
			// wonder races, build things that much faster
			//iTempValue += ((kCivic.getCapitalYieldModifier(iI)) / 2);

			iTemp = (kCivic.getCapitalYieldModifier(iI) * pCapital->getBaseYieldRate((YieldTypes)iI));
			iTemp /= 80;
			//iTemp *= pCapital->AI_yieldMultiplier((YieldTypes)iI);
			//iTemp /= 100;
			iTempValue += iTemp;
		}
		iTempValue += ((kCivic.getTradeYieldModifier(iI) * iCities) / 11);

		for (iJ = 0; iJ < GC.getNumImprovementInfos(); iJ++)
		{
			// Free Speech
			iTempValue += (AI_averageYieldMultiplier((YieldTypes)iI) * (kCivic.getImprovementYieldChanges(iJ, iI) * (getImprovementCount((ImprovementTypes)iJ) + iCities/2))) / 100;
		}

		if (iI == YIELD_FOOD) 
		{ 
			iTempValue *= 3; 
		} 
		else if (iI == YIELD_PRODUCTION) 
		{ 
			iTempValue *= ((AI_avoidScience()) ? 6 : 2); 
		} 
		else if (iI == YIELD_COMMERCE) 
		{ 
			iTempValue *= ((AI_avoidScience()) ? 2 : 4);
			iTempValue /= 3;
		} 

		iValue += iTempValue;
	}

	// K-Mod: count bonus specialists,
	// so that we can more accurately gauge the representation bonus
	int iTotalBonusSpecialists = 0;
	int iTotalCurrentSpecialists = 0;

	{
		int iLoop;
		CvCity* pLoopCity;
		for (pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
		{
			iTotalBonusSpecialists += pLoopCity->getNumGreatPeople();
			iTotalBonusSpecialists += pLoopCity->totalFreeSpecialists();

			iTotalCurrentSpecialists += pLoopCity->getNumGreatPeople();
			iTotalCurrentSpecialists += pLoopCity->getSpecialistPopulation();
		}
	}
	// K-Mod end

	for (iI = 0; iI < NUM_COMMERCE_TYPES; iI++)
	{
		iTempValue = 0;

		// Nationhood
		/* original bts code (lolwut?)
		iTempValue += ((kCivic.getCommerceModifier(iI) * iCities) / 3);
		iTempValue += (kCivic.getCapitalCommerceModifier(iI) / 2);
		if (iI == COMMERCE_ESPIONAGE)
		{
			iTempValue *= AI_getEspionageWeight();
			iTempValue /= 500;
		}*/
		// K-Mod
		iTempValue += kCivic.getCommerceModifier(iI) * 100*getCommerceRate((CommerceTypes)iI) / AI_averageCommerceMultiplier((CommerceTypes)iI);
		if (pCapital != NULL)
		{
			iTempValue += kCivic.getCapitalCommerceModifier(iI) * pCapital->getBaseCommerceRate((CommerceTypes)iI);
		}

		// Representation
		//iTempValue += ((kCivic.getSpecialistExtraCommerce(iI) * getTotalPopulation()) / 15);
		// K-Mod
		iTempValue += AI_averageCommerceMultiplier((CommerceTypes)iI)*(kCivic.getSpecialistExtraCommerce(iI) * std::max((getTotalPopulation()+10*iTotalBonusSpecialists) / 10, iTotalCurrentSpecialists));

		iTempValue /= 100; // (for the 3 things above)

		iTempValue *= AI_commerceWeight((CommerceTypes)iI);

		/*if ((iI == COMMERCE_CULTURE) && bCultureVictory2)
		{
		    iTempValue *= 2;
		    if (bCultureVictory3)
		    {
		        iTempValue *= 2;		        
		    }
		}*/
		iTempValue /= 100;

		iValue += iTempValue;
	}

	for (iI = 0; iI < GC.getNumBuildingClassInfos(); iI++)
	{
		iTempValue = kCivic.getBuildingHappinessChanges(iI);
		/* original bts code
		if (iTempValue != 0)
		{
			// Nationalism
			if( !isNationalWonderClass((BuildingClassTypes)iI) )
			{
				iValue += (iTempValue * iCities)/2;
			}
			iValue += (iTempValue * getBuildingClassCountPlusMaking((BuildingClassTypes)iI) * 2);
		}*/
		// K-Mod
		if (iTempValue != 0)
		{
			int iExpectedBuildings = 0;
			if (canConstruct((BuildingTypes)GC.getCivilizationInfo(getCivilizationType()).getCivilizationBuildings(iI)))
			{
				iExpectedBuildings = (iCities + 2*getBuildingClassCountPlusMaking((BuildingClassTypes)iI))/3;
			}
			iValue += (10 * iExpectedBuildings * iS * AI_getHappinessWeight(iS * iTempValue, 1))/100;
		}
	}

	for (iI = 0; iI < GC.getNumFeatureInfos(); iI++)
	{
		// Environmentalism
		iHappiness = kCivic.getFeatureHappinessChanges(iI);

		if (iHappiness != 0)
		{
			iValue += (iHappiness * countCityFeatures((FeatureTypes)iI) * 5);
		}
	}

	for (iI = 0; iI < GC.getNumHurryInfos(); iI++)
	{
		if (kCivic.isHurry(iI))
		{
			iTempValue = 0;

			if (GC.getHurryInfo((HurryTypes)iI).getGoldPerProduction() > 0)
			{
				iTempValue += ((((AI_avoidScience()) ? 50 : 25) * iCities) / GC.getHurryInfo((HurryTypes)iI).getGoldPerProduction());
			}
			iTempValue += (GC.getHurryInfo((HurryTypes)iI).getProductionPerPopulation() * iCities * (bWarPlan ? 2 : 1)) / 5;
			iValue += iTempValue;
		}
	}

	for (iI = 0; iI < GC.getNumSpecialBuildingInfos(); iI++)
	{
		if (kCivic.isSpecialBuildingNotRequired(iI))
		{
			iValue += ((iCities / 2) + 1); // XXX
		}
	}

	for (iI = 0; iI < GC.getNumSpecialistInfos(); iI++) 
	{ 
		iTempValue = 0; 
		if (kCivic.isSpecialistValid(iI)) 
		{ 
			iTempValue += ((iCities *  (bCultureVictory3 ? 10 : 1)) + 6);
		} 
		iValue += (iTempValue / 2); 
	} 

	if (GC.getLeaderHeadInfo(getPersonalityType()).getFavoriteCivic() == eCivic)
	{
		if (!kCivic.isStateReligion() || iHighestReligionCount > 0)
		{
			iValue *= 5; 
			iValue /= 4; 
			iValue += 6 * iCities;
			iValue += 20; 
		}
	}

	if (AI_isDoVictoryStrategy(AI_VICTORY_CULTURE2) && (GC.getCivicInfo(eCivic).isNoNonStateReligionSpread()))
	{
	    iValue /= 10;	    
	}

	return iValue;
}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/


ReligionTypes CvPlayerAI::AI_bestReligion() const
{
	ReligionTypes eBestReligion;
	int iValue;
	int iBestValue;
	int iI;

	iBestValue = 0;
	eBestReligion = NO_RELIGION;

	ReligionTypes eFavorite = (ReligionTypes)GC.getLeaderHeadInfo(getLeaderType()).getFavoriteReligion();

	for (iI = 0; iI < GC.getNumReligionInfos(); iI++)
	{
		if (canDoReligion((ReligionTypes)iI))
		{
			iValue = AI_religionValue((ReligionTypes)iI);

			/* original bts code
			if (getStateReligion() == ((ReligionTypes)iI))
			{
				iValue *= 4;
				iValue /= 3;
			} */
			// K-Mod
			if (iI == getStateReligion() && getReligionAnarchyLength() > 0)
			{
				iValue *= 5;
				iValue /= 4;
			}
			// K-Mod end

			if (eFavorite == iI)
			{
				iValue *= 5;
				iValue /= 4;
			}

			if (iValue > iBestValue)
			{
				iBestValue = iValue;
				eBestReligion = ((ReligionTypes)iI);
			}
		}
	}

	if ((NO_RELIGION == eBestReligion) || AI_isDoStrategy(AI_STRATEGY_MISSIONARY))
	{
		return eBestReligion;
	}

	int iBestCount = getHasReligionCount(eBestReligion);
	int iSpreadPercent = (iBestCount * 100) / std::max(1, getNumCities());
	int iPurityPercent = (iBestCount * 100) / std::max(1, countTotalHasReligion());
	// K-Mod. Don't instantly convert to the first religion avaiable, unless it if your own religion.
	if (getStateReligion() == NO_RELIGION && iSpreadPercent < 29 - AI_getFlavorValue(FLAVOR_RELIGION)
		&& (GC.getGameINLINE().getHolyCity(eBestReligion) == NULL || GC.getGameINLINE().getHolyCity(eBestReligion)->getTeam() != getTeam()))
	{
		return NO_RELIGION;
	}
	// K-Mod end

	if (iPurityPercent < 49)
	{
		if (iSpreadPercent > ((eBestReligion == eFavorite) ? 65 : 75))
		{
			if (iPurityPercent > ((eBestReligion == eFavorite) ? 25 : 32))
			{
				return eBestReligion;
			}
		}
		return NO_RELIGION;
	}
	
	return eBestReligion;
}

// This function has been completely rewriten for K-Mod
int CvPlayerAI::AI_religionValue(ReligionTypes eReligion) const
{
	PROFILE_FUNC();

	int iValue = 0;

	if (getHasReligionCount(eReligion) == 0)
		return 0;

	int iReligionFlavor = AI_getFlavorValue(FLAVOR_RELIGION);

	int iLoop;
	for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		iValue += pLoopCity->getReligionGrip(eReligion) * pLoopCity->getPopulation();
		// note. this takes into account current state religion and number of religious buildings.
	}
	iValue *= getHasReligionCount(eReligion);
	iValue /= getNumCities();

	// holy city modifier
	CvCity* pHolyCity = GC.getGameINLINE().getHolyCity(eReligion);
	if (pHolyCity != NULL && pHolyCity->getTeam() == getTeam())
	{
		iValue *= 110 + 5 * iReligionFlavor + (10+iReligionFlavor)*GC.getGameINLINE().calculateReligionPercent(eReligion)/10;
		iValue /= 100;
	}

	// diplomatic modifier
	int iTotalCivs = 0; // x2
	int iLikedReligionCivs = 0; // x2 for friendly
	for (int i = 0; i < MAX_CIV_PLAYERS; i++)
	{
		const CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)i);
		if (i != getID() && kLoopPlayer.isAlive() && GET_TEAM(getTeam()).isHasMet(kLoopPlayer.getTeam()) && !kLoopPlayer.isMinorCiv())
		{
			iTotalCivs += 2;
			if (kLoopPlayer.getStateReligion() == eReligion)
			{
				AttitudeTypes eAttitude = AI_getAttitude((PlayerTypes)i, false);
				if (eAttitude >= ATTITUDE_PLEASED)
				{
					iLikedReligionCivs++;
					if (eAttitude >= ATTITUDE_FRIENDLY)
						iLikedReligionCivs++;
				}
			}
		}
	}

	// up to +100% boost for liked civs having this has their state religion.
	// less potential boost if we have aspirations of being a religious leader
	iTotalCivs *= 10 + iReligionFlavor;
	iTotalCivs /= 10;
	iValue *= 100 + 100 * iLikedReligionCivs / std::max(1, iTotalCivs);
	iValue /= 100;

	return iValue;
}

/// \brief Value of espionage mission at this plot.
///
/// Assigns value to espionage mission against ePlayer at pPlot, where iData can provide additional information about mission.
int CvPlayerAI::AI_espionageVal(PlayerTypes eTargetPlayer, EspionageMissionTypes eMission, CvPlot* pPlot, int iData) const
{
	TeamTypes eTargetTeam = GET_PLAYER(eTargetPlayer).getTeam();

	if (eTargetPlayer == NO_PLAYER)
	{
		return 0;
	}

	//int iCost = getEspionageMissionCost(eMission, eTargetPlayer, pPlot, iData);

	if (!canDoEspionageMission(eMission, eTargetPlayer, pPlot, iData, NULL))
	{
		return 0;
	}

	bool bMalicious = isMaliciousEspionageTarget(eTargetPlayer);

	int iValue = 0;
	if (bMalicious && GC.getEspionageMissionInfo(eMission).isDestroyImprovement())
	{
		if (NULL != pPlot)
		{
			if (pPlot->getOwnerINLINE() == eTargetPlayer)
			{
				ImprovementTypes eImprovement = pPlot->getImprovementType();
				if (eImprovement != NO_IMPROVEMENT)
				{
					BonusTypes eBonus = pPlot->getNonObsoleteBonusType(GET_PLAYER(eTargetPlayer).getTeam());
					if (NO_BONUS != eBonus)
					{
						iValue += 2*GET_PLAYER(eTargetPlayer).AI_bonusVal(eBonus, -1); // was 1*
						
						int iTempValue = 0;
						if (NULL != pPlot->getWorkingCity())
						{
							iTempValue += (pPlot->calculateImprovementYieldChange(eImprovement, YIELD_FOOD, pPlot->getOwnerINLINE()) * 2);
							iTempValue += (pPlot->calculateImprovementYieldChange(eImprovement, YIELD_PRODUCTION, pPlot->getOwnerINLINE()) * 1);
							iTempValue += (pPlot->calculateImprovementYieldChange(eImprovement, YIELD_COMMERCE, pPlot->getOwnerINLINE()) * 2);
							iTempValue += GC.getImprovementInfo(eImprovement).getUpgradeTime() / 2;
							iValue += iTempValue;
						}
					}
				}
			}
		}
	}

	if (bMalicious && GC.getEspionageMissionInfo(eMission).getDestroyBuildingCostFactor() > 0)
	{
		FAssert(iData >= 0 && iData < GC.getNumBuildingInfos());
		if (canSpyDestroyBuilding(eTargetPlayer, (BuildingTypes)iData))
		{
			if (NULL != pPlot)
			{
				CvCity* pCity = pPlot->getPlotCity();

				if (NULL != pCity)
				{
					if (pCity->getNumRealBuilding((BuildingTypes)iData) > 0)
					{
						CvBuildingInfo& kBuilding = GC.getBuildingInfo((BuildingTypes)iData);
						if ((kBuilding.getProductionCost() > 1) && !isWorldWonderClass((BuildingClassTypes)kBuilding.getBuildingClassType()))
						{
							//iValue += pCity->AI_buildingValue((BuildingTypes)iData);
							// K-Mod
							iValue += 2 * pCity->AI_buildingValue((BuildingTypes)iData);
							iValue *= 60 + kBuilding.getProductionCost();
							iValue /= 100;
						}
					}
				}
			}
		}
	}

	if (bMalicious && GC.getEspionageMissionInfo(eMission).getDestroyProjectCostFactor() > 0)
	{
		FAssert(iData >= 0 && iData < GC.getNumProjectInfos());
		if (canSpyDestroyProject(eTargetPlayer, (ProjectTypes)iData))
		{
			CvProjectInfo& kProject = GC.getProjectInfo((ProjectTypes)iData);
			
			iValue += getProductionNeeded((ProjectTypes)iData) * (kProject.getMaxTeamInstances() == 1 ? 6 : 4); // was 3 : 2
		}
	}

	if (bMalicious && GC.getEspionageMissionInfo(eMission).getDestroyProductionCostFactor() > 0)
	{
		if (NULL != pPlot)
		{
			CvCity* pCity = pPlot->getPlotCity();
			FAssert(pCity != NULL);
			if (pCity != NULL)
			{
				int iTempValue = pCity->getProduction();
				if (iTempValue > 0)
				{
					if (pCity->getProductionProject() != NO_PROJECT)
					{
						CvProjectInfo& kProject = GC.getProjectInfo(pCity->getProductionProject());
						iValue += iTempValue * ((kProject.getMaxTeamInstances() == 1) ? 6 : 3);	// was 4 : 2
					}
					else if (pCity->getProductionBuilding() != NO_BUILDING)
					{
						CvBuildingInfo& kBuilding = GC.getBuildingInfo(pCity->getProductionBuilding());
						if (isWorldWonderClass((BuildingClassTypes)kBuilding.getBuildingClassType()))
						{
							iValue += 3 * iTempValue;
						}
						iValue += iTempValue;
					}
					else
					{
						iValue += iTempValue;
					}
				}
			}
		}
	}


	if (bMalicious && (GC.getEspionageMissionInfo(eMission).getDestroyUnitCostFactor() > 0 || GC.getEspionageMissionInfo(eMission).getBuyUnitCostFactor() > 0) )
	{
		if (NULL != pPlot)
		{
			CvUnit* pUnit = GET_PLAYER(eTargetPlayer).getUnit(iData);

			if (NULL != pUnit)
			{
				UnitTypes eUnit = pUnit->getUnitType();

				iValue += GET_PLAYER(eTargetPlayer).AI_unitValue(eUnit, (UnitAITypes)GC.getUnitInfo(eUnit).getDefaultUnitAIType(), pUnit->area());

				if (GC.getEspionageMissionInfo(eMission).getBuyUnitCostFactor() > 0)
				{
					/*if (!canTrain(eUnit) || getProductionNeeded(eUnit) > iCost)
					{
						iValue += AI_unitValue(eUnit, (UnitAITypes)GC.getUnitInfo(eUnit).getDefaultUnitAIType(), pUnit->area());
					}*/
					// K-Mod. (this kind of mission is not enabled anyway.)
					iValue += AI_unitValue(eUnit, (UnitAITypes)GC.getUnitInfo(eUnit).getDefaultUnitAIType(), pUnit->area());
					iValue *= (canTrain(eUnit) ? 1 : 2);
				}
			}
		}
	}

	if (bMalicious && GC.getEspionageMissionInfo(eMission).getStealTreasuryTypes() > 0)
	{
		if( pPlot != NULL && pPlot->getPlotCity() != NULL )
		{
			/* int iGoldStolen = (GET_PLAYER(eTargetPlayer).getGold() * GC.getEspionageMissionInfo(eMission).getStealTreasuryTypes()) / 100;
			iGoldStolen *= pPlot->getPlotCity()->getPopulation();
			iGoldStolen /= std::max(1, GET_PLAYER(eTargetPlayer).getTotalPopulation());
			iValue += ((GET_PLAYER(eTargetPlayer).AI_isFinancialTrouble() || AI_isFinancialTrouble()) ? 4 : 2) * (2 * std::max(0, iGoldStolen - iCost));*/
			// K-Mod
			int iGoldStolen = getEspionageGoldQuantity(eMission, eTargetPlayer, pPlot->getPlotCity());
			iValue += (GET_PLAYER(eTargetPlayer).AI_isFinancialTrouble() || AI_isFinancialTrouble() ? 6 : 4) * iGoldStolen;
		}
	}

	if (GC.getEspionageMissionInfo(eMission).getCounterespionageNumTurns() > 0)
	{
		//iValue += 100 * GET_TEAM(getTeam()).AI_getAttitudeVal(GET_PLAYER(eTargetPlayer).getTeam());

		// K-Mod (I didn't comment that line out, btw.)
		const TeamTypes eTeam = GET_PLAYER(eTargetPlayer).getTeam();
		const int iEra = getCurrentEra();
		int iCounterValue = 5 + 3*iEra + (AI_isDoVictoryStrategy(AI_VICTORY_CULTURE3 || AI_VICTORY_SPACE3) ? 20 : 0);
		// this is pretty bogus. I'll try to come up with something better some other time.
		iCounterValue *= 50*iEra*(iEra+1)/2 + GET_TEAM(eTeam).getEspionagePointsAgainstTeam(getTeam());
		iCounterValue /= std::max(1, 50*iEra*(iEra+1) + GET_TEAM(getTeam()).getEspionagePointsAgainstTeam(eTeam))/2;
		iCounterValue *= AI_getMemoryCount(eTargetPlayer, MEMORY_SPY_CAUGHT) + (GET_TEAM(getTeam()).isAtWar(eTeam)?2 :0) + (AI_isDoVictoryStrategy(AI_VICTORY_CULTURE4 | AI_VICTORY_SPACE3)?2 : 0);
		iValue += iCounterValue;
	}

	if (bMalicious && GC.getEspionageMissionInfo(eMission).getBuyCityCostFactor() > 0)
	{
		if (NULL != pPlot)
		{
			CvCity* pCity = pPlot->getPlotCity();

			if (NULL != pCity)
			{
				iValue += AI_cityTradeVal(pCity);
			}
		}
	}

	if (GC.getEspionageMissionInfo(eMission).getCityInsertCultureAmountFactor() > 0)
	{
		if (NULL != pPlot)
		{
			CvCity* pCity = pPlot->getPlotCity();
			if (NULL != pCity)
			{
				if (pCity->getOwner() != getID())
				{
					int iCultureAmount = GC.getEspionageMissionInfo(eMission).getCityInsertCultureAmountFactor() * pPlot->getCulture(getID());
					iCultureAmount /= 100;
					/* if (pCity->calculateCulturePercent(getID()) > 40)
					{
						iValue += iCultureAmount * 3;
					}*/
					// K-Mod - defensive use of spread culture mission. (The first "if" is really just for effeciency.)
					if (pCity->calculateCulturePercent(getID()) > 10)
					{
						const CvCity* pOurClosestCity = GC.getMap().findCity(pPlot->getX(), pPlot->getY(), getID());
						if (pOurClosestCity != NULL)
						{
							int iDistance = pCity->cultureDistance(xDistance(pPlot->getX(), pOurClosestCity->getX()), yDistance(pPlot->getY(), pOurClosestCity->getY()));
							if (iDistance < 6)
							{
								int iMultiplier = std::min(2, (6 - iDistance) * pOurClosestCity->culturePressureFactor() / 600);
								iValue += iCultureAmount * iMultiplier;
							}
						}
					}
					// K-Mod end
				}
			}
		}
	}

	if (bMalicious && GC.getEspionageMissionInfo(eMission).getCityPoisonWaterCounter() > 0)
	{
		if (NULL != pPlot)
		{
			CvCity* pCity = pPlot->getPlotCity();

			if (NULL != pCity)
			{
				int iCityHealth = pCity->goodHealth() - pCity->badHealth(false, 0);
				//int iBaseUnhealth = GC.getEspionageMissionInfo(eMission).getCityPoisonWaterCounter();
				int iBaseUnhealth = GC.getEspionageMissionInfo(eMission).getCityPoisonWaterCounter() - std::max(pCity->getOccupationTimer(), GET_PLAYER(eTargetPlayer).getAnarchyTurns());

				// K-Mod: fixing some "wtf".
				/*
				int iAvgFoodShortage = std::max(0, iBaseUnhealth - iCityHealth) - pCity->foodDifference();
				iAvgFoodShortage += std::max(0, iBaseUnhealth/2 - iCityHealth) - pCity->foodDifference();
				
				iAvgFoodShortage /= 2;
				
				if( iAvgFoodShortage > 0 )
				{
					iValue += 8 * iAvgFoodShortage * iAvgFoodShortage;
				}*/
				int iAvgFoodShortage = std::max(0, iBaseUnhealth - iCityHealth) - pCity->foodDifference();
				iAvgFoodShortage += std::max(0, -iCityHealth) - pCity->foodDifference();
				
				iAvgFoodShortage /= 2;
				
				if( iAvgFoodShortage > 0 )
				{
					iValue += 3 * iAvgFoodShortage * iBaseUnhealth;
				}
			}
		}
	}

	if (bMalicious && GC.getEspionageMissionInfo(eMission).getCityUnhappinessCounter() > 0)
	{
		if (NULL != pPlot)
		{
			CvCity* pCity = pPlot->getPlotCity();

			if (NULL != pCity)
			{
				int iCityCurAngerLevel = pCity->happyLevel() - pCity->unhappyLevel(0);
				//int iBaseAnger = GC.getEspionageMissionInfo(eMission).getCityUnhappinessCounter();
				// K-Mod
				int iBaseAnger = GC.getEspionageMissionInfo(eMission).getCityUnhappinessCounter() - std::max(pCity->getOccupationTimer(), GET_PLAYER(eTargetPlayer).getAnarchyTurns());
				// K-Mod end
				int iAvgUnhappy = iCityCurAngerLevel - iBaseAnger/2;
				
				if (iAvgUnhappy < 0)
				{
					iValue += 8 * abs(iAvgUnhappy) * iBaseAnger;// down from 14
				}
			}
		}
	}

	if (bMalicious && GC.getEspionageMissionInfo(eMission).getCityRevoltCounter() > 0)
	{
		// Handled elsewhere
	}

	if (GC.getEspionageMissionInfo(eMission).getBuyTechCostFactor() > 0)
	{
		FAssert(iData >= 0 && iData < GC.getNumTechInfos());

		//if (iCost < GET_TEAM(getTeam()).getResearchLeft((TechTypes)iData) * 4 / 3)
		if (canStealTech(eTargetPlayer, (TechTypes)iData)) // K-Mod!
		{
			//int iTempValue = GET_TEAM(getTeam()).AI_techTradeVal((TechTypes)iData, GET_PLAYER(eTargetPlayer).getTeam());
			// K-Mod
			int iTempValue = GET_TEAM(getTeam()).AI_techTradeVal((TechTypes)iData, GET_PLAYER(eTargetPlayer).getTeam());
			iTempValue *= 3;
			iTempValue /= 2;
			// K-Mod end

			iValue += iTempValue;
		}
	}

	if (bMalicious && GC.getEspionageMissionInfo(eMission).getSwitchCivicCostFactor() > 0)
	{
		iValue += AI_civicTradeVal((CivicTypes)iData, eTargetPlayer);
	}

	if (bMalicious && GC.getEspionageMissionInfo(eMission).getSwitchReligionCostFactor() > 0)
	{
		iValue += AI_religionTradeVal((ReligionTypes)iData, eTargetPlayer);
	}

	if (bMalicious && GC.getEspionageMissionInfo(eMission).getPlayerAnarchyCounter() > 0)
	{
		// AI doesn't use Player Anarchy
	}

	return iValue;
}

bool CvPlayerAI::isMaliciousEspionageTarget(PlayerTypes eTarget) const
{
	if (GET_PLAYER(eTarget).getTeam() == getTeam())
		return false;
	return (AI_getAttitudeWeight(eTarget) < (GC.getGameINLINE().isOption(GAMEOPTION_AGGRESSIVE_AI) ? 51 : 1) || GET_TEAM(getTeam()).AI_getWarPlan(GET_PLAYER(eTarget).getTeam()) != NO_WARPLAN);
}

int CvPlayerAI::AI_getPeaceWeight() const
{
	return m_iPeaceWeight;
}


void CvPlayerAI::AI_setPeaceWeight(int iNewValue)
{
	m_iPeaceWeight = iNewValue;
}

int CvPlayerAI::AI_getEspionageWeight() const
{
	if (GC.getGameINLINE().isOption(GAMEOPTION_NO_ESPIONAGE))
	{
		return 0;
	}
	return m_iEspionageWeight;
}

void CvPlayerAI::AI_setEspionageWeight(int iNewValue)
{
	m_iEspionageWeight = iNewValue;
}


int CvPlayerAI::AI_getAttackOddsChange() const
{
	return m_iAttackOddsChange;
}


void CvPlayerAI::AI_setAttackOddsChange(int iNewValue)
{
	m_iAttackOddsChange = iNewValue;
}


int CvPlayerAI::AI_getCivicTimer() const
{
	return m_iCivicTimer;
}


void CvPlayerAI::AI_setCivicTimer(int iNewValue)
{
	m_iCivicTimer = iNewValue;
	FAssert(AI_getCivicTimer() >= 0);
}


void CvPlayerAI::AI_changeCivicTimer(int iChange)
{
	AI_setCivicTimer(AI_getCivicTimer() + iChange);
}


int CvPlayerAI::AI_getReligionTimer() const
{
	return m_iReligionTimer;
}


void CvPlayerAI::AI_setReligionTimer(int iNewValue)
{
	m_iReligionTimer = iNewValue;
	FAssert(AI_getReligionTimer() >= 0);
}


void CvPlayerAI::AI_changeReligionTimer(int iChange)
{
	AI_setReligionTimer(AI_getReligionTimer() + iChange);
}

int CvPlayerAI::AI_getExtraGoldTarget() const
{
	return m_iExtraGoldTarget;
}

void CvPlayerAI::AI_setExtraGoldTarget(int iNewValue)
{
	m_iExtraGoldTarget = iNewValue;
}

int CvPlayerAI::AI_getNumTrainAIUnits(UnitAITypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < NUM_UNITAI_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiNumTrainAIUnits[eIndex];
}


void CvPlayerAI::AI_changeNumTrainAIUnits(UnitAITypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < NUM_UNITAI_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");
	m_aiNumTrainAIUnits[eIndex] = (m_aiNumTrainAIUnits[eIndex] + iChange);
	FAssert(AI_getNumTrainAIUnits(eIndex) >= 0);
}


int CvPlayerAI::AI_getNumAIUnits(UnitAITypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < NUM_UNITAI_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiNumAIUnits[eIndex];
}


void CvPlayerAI::AI_changeNumAIUnits(UnitAITypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < NUM_UNITAI_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");
	m_aiNumAIUnits[eIndex] = (m_aiNumAIUnits[eIndex] + iChange);
	FAssert(AI_getNumAIUnits(eIndex) >= 0);
}


int CvPlayerAI::AI_getSameReligionCounter(PlayerTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiSameReligionCounter[eIndex];
}


void CvPlayerAI::AI_changeSameReligionCounter(PlayerTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex is expected to be within maximum bounds (invalid Index)");
	m_aiSameReligionCounter[eIndex] = (m_aiSameReligionCounter[eIndex] + iChange);
	FAssert(AI_getSameReligionCounter(eIndex) >= 0);
}


int CvPlayerAI::AI_getDifferentReligionCounter(PlayerTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiDifferentReligionCounter[eIndex];
}


void CvPlayerAI::AI_changeDifferentReligionCounter(PlayerTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex is expected to be within maximum bounds (invalid Index)");
	m_aiDifferentReligionCounter[eIndex] = (m_aiDifferentReligionCounter[eIndex] + iChange);
	FAssert(AI_getDifferentReligionCounter(eIndex) >= 0);
}


int CvPlayerAI::AI_getFavoriteCivicCounter(PlayerTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiFavoriteCivicCounter[eIndex];
}


void CvPlayerAI::AI_changeFavoriteCivicCounter(PlayerTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex is expected to be within maximum bounds (invalid Index)");
	m_aiFavoriteCivicCounter[eIndex] = (m_aiFavoriteCivicCounter[eIndex] + iChange);
	FAssert(AI_getFavoriteCivicCounter(eIndex) >= 0);
}


int CvPlayerAI::AI_getBonusTradeCounter(PlayerTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiBonusTradeCounter[eIndex];
}


void CvPlayerAI::AI_changeBonusTradeCounter(PlayerTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex is expected to be within maximum bounds (invalid Index)");
	m_aiBonusTradeCounter[eIndex] = (m_aiBonusTradeCounter[eIndex] + iChange);
	FAssert(AI_getBonusTradeCounter(eIndex) >= 0);
}


int CvPlayerAI::AI_getPeacetimeTradeValue(PlayerTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiPeacetimeTradeValue[eIndex];
}


void CvPlayerAI::AI_changePeacetimeTradeValue(PlayerTypes eIndex, int iChange)
{
	PROFILE_FUNC();

	int iI;

	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex is expected to be within maximum bounds (invalid Index)");

	if (iChange != 0)
	{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      09/03/09                       poyuzhe & jdog5000     */
/*                                                                                              */
/* Efficiency                                                                                   */
/************************************************************************************************/
		// From Sanguo Mod Performance, ie the CAR Mod
		// Attitude cache
		AI_invalidateAttitudeCache(eIndex);
		GET_PLAYER(eIndex).AI_invalidateAttitudeCache(getID());
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

		m_aiPeacetimeTradeValue[eIndex] = (m_aiPeacetimeTradeValue[eIndex] + iChange);
		FAssert(AI_getPeacetimeTradeValue(eIndex) >= 0);

		FAssert(iChange > 0);

		if (iChange > 0)
		{
			if (GET_PLAYER(eIndex).getTeam() != getTeam())
			{
				for (iI = 0; iI < MAX_CIV_TEAMS; iI++)
				{
					if (GET_TEAM((TeamTypes)iI).isAlive())
					{
						if (GET_TEAM((TeamTypes)iI).AI_getWorstEnemy() == getTeam())
						{
/************************************************************************************************/
/* UNOFFICIAL_PATCH                       03/02/10                                Sephi         */
/*                                                                                              */
/* Bug fix                                                                                      */
/************************************************************************************************/
/* orig bts code
							GET_TEAM((TeamTypes)iI).AI_changeEnemyPeacetimeTradeValue(GET_PLAYER(eIndex).getTeam(), iChange);
*/
                            //make sure that if A trades with B and A is C's worst enemy, C is only mad at B if C has met B before
                            //A = this
                            //B = eIndex
                            //C = (TeamTypes)iI
                            if (GET_TEAM((TeamTypes)iI).isHasMet(GET_PLAYER(eIndex).getTeam()))
                            {
								GET_TEAM((TeamTypes)iI).AI_changeEnemyPeacetimeTradeValue(GET_PLAYER(eIndex).getTeam(), iChange);
							}
/************************************************************************************************/
/* UNOFFICIAL_PATCH                        END                                                  */
/************************************************************************************************/
						}
					}
				}
			}
		}
	}
}


int CvPlayerAI::AI_getPeacetimeGrantValue(PlayerTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiPeacetimeGrantValue[eIndex];
}


void CvPlayerAI::AI_changePeacetimeGrantValue(PlayerTypes eIndex, int iChange)
{
	PROFILE_FUNC();

	int iI;

	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex is expected to be within maximum bounds (invalid Index)");

	if (iChange != 0)
	{
		m_aiPeacetimeGrantValue[eIndex] = (m_aiPeacetimeGrantValue[eIndex] + iChange);
		FAssert(AI_getPeacetimeGrantValue(eIndex) >= 0);

		AI_invalidateAttitudeCache(eIndex); // K-Mod. (missing from CAR mod)

		FAssert(iChange > 0);

		if (iChange > 0)
		{
			if (GET_PLAYER(eIndex).getTeam() != getTeam())
			{
				for (iI = 0; iI < MAX_CIV_TEAMS; iI++)
				{
					if (GET_TEAM((TeamTypes)iI).isAlive())
					{
						if (GET_TEAM((TeamTypes)iI).AI_getWorstEnemy() == getTeam())
						{
/************************************************************************************************/
/* UNOFFICIAL_PATCH                       03/02/10                                Sephi         */
/*                                                                                              */
/* Bug fix                                                                                      */
/************************************************************************************************/
/* orig bts code
							GET_TEAM((TeamTypes)iI).AI_changeEnemyPeacetimeGrantValue(GET_PLAYER(eIndex).getTeam(), iChange);
*/
                            //make sure that if A trades with B and A is C's worst enemy, C is only mad at B if C has met B before
                            //A = this
                            //B = eIndex
                            //C = (TeamTypes)iI
                            if (GET_TEAM((TeamTypes)iI).isHasMet(GET_PLAYER(eIndex).getTeam()))
                            {
								GET_TEAM((TeamTypes)iI).AI_changeEnemyPeacetimeGrantValue(GET_PLAYER(eIndex).getTeam(), iChange);
							}
/************************************************************************************************/
/* UNOFFICIAL_PATCH                        END                                                  */
/************************************************************************************************/
						}
					}
				}
			}
		}
	}
}


int CvPlayerAI::AI_getGoldTradedTo(PlayerTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiGoldTradedTo[eIndex];
}


void CvPlayerAI::AI_changeGoldTradedTo(PlayerTypes eIndex, int iChange)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex is expected to be within maximum bounds (invalid Index)");
	m_aiGoldTradedTo[eIndex] = (m_aiGoldTradedTo[eIndex] + iChange);
	FAssert(AI_getGoldTradedTo(eIndex) >= 0);
}


int CvPlayerAI::AI_getAttitudeExtra(PlayerTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiAttitudeExtra[eIndex];
}


void CvPlayerAI::AI_setAttitudeExtra(PlayerTypes eIndex, int iNewValue)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex is expected to be within maximum bounds (invalid Index)");
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      09/03/09                       poyuzhe & jdog5000     */
/*                                                                                              */
/* Efficiency                                                                                   */
/************************************************************************************************/
	// From Sanguo Mod Performance, ie the CAR Mod
	// Attitude cache
	if (m_aiAttitudeExtra[eIndex] != iNewValue)
	{
		GET_PLAYER(getID()).AI_invalidateAttitudeCache(eIndex);
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
	m_aiAttitudeExtra[eIndex] = iNewValue;
}


void CvPlayerAI::AI_changeAttitudeExtra(PlayerTypes eIndex, int iChange)
{
	AI_setAttitudeExtra(eIndex, (AI_getAttitudeExtra(eIndex) + iChange));
}


bool CvPlayerAI::AI_isFirstContact(PlayerTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_abFirstContact[eIndex];
}


void CvPlayerAI::AI_setFirstContact(PlayerTypes eIndex, bool bNewValue)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex is expected to be within maximum bounds (invalid Index)");
	m_abFirstContact[eIndex] = bNewValue;
}


int CvPlayerAI::AI_getContactTimer(PlayerTypes eIndex1, ContactTypes eIndex2) const
{
	FAssertMsg(eIndex1 >= 0, "eIndex1 is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex1 < MAX_PLAYERS, "eIndex1 is expected to be within maximum bounds (invalid Index)");
	FAssertMsg(eIndex2 >= 0, "eIndex2 is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex2 < NUM_CONTACT_TYPES, "eIndex2 is expected to be within maximum bounds (invalid Index)");
	return m_aaiContactTimer[eIndex1][eIndex2];
}


void CvPlayerAI::AI_changeContactTimer(PlayerTypes eIndex1, ContactTypes eIndex2, int iChange)
{
	FAssertMsg(eIndex1 >= 0, "eIndex1 is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex1 < MAX_PLAYERS, "eIndex1 is expected to be within maximum bounds (invalid Index)");
	FAssertMsg(eIndex2 >= 0, "eIndex2 is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex2 < NUM_CONTACT_TYPES, "eIndex2 is expected to be within maximum bounds (invalid Index)");
	m_aaiContactTimer[eIndex1][eIndex2] = (AI_getContactTimer(eIndex1, eIndex2) + iChange);
	FAssert(AI_getContactTimer(eIndex1, eIndex2) >= 0);
}


int CvPlayerAI::AI_getMemoryCount(PlayerTypes eIndex1, MemoryTypes eIndex2) const
{
	FAssertMsg(eIndex1 >= 0, "eIndex1 is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex1 < MAX_PLAYERS, "eIndex1 is expected to be within maximum bounds (invalid Index)");
	FAssertMsg(eIndex2 >= 0, "eIndex2 is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex2 < NUM_MEMORY_TYPES, "eIndex2 is expected to be within maximum bounds (invalid Index)");
	return m_aaiMemoryCount[eIndex1][eIndex2];
}


void CvPlayerAI::AI_changeMemoryCount(PlayerTypes eIndex1, MemoryTypes eIndex2, int iChange)
{
	FAssertMsg(eIndex1 >= 0, "eIndex1 is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex1 < MAX_PLAYERS, "eIndex1 is expected to be within maximum bounds (invalid Index)");
	FAssertMsg(eIndex2 >= 0, "eIndex2 is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex2 < NUM_MEMORY_TYPES, "eIndex2 is expected to be within maximum bounds (invalid Index)");
	m_aaiMemoryCount[eIndex1][eIndex2] += iChange;
	FAssert(AI_getMemoryCount(eIndex1, eIndex2) >= 0);
	AI_invalidateAttitudeCache(eIndex1); // K-Mod. (missing from CAR mod)
}

int CvPlayerAI::AI_calculateGoldenAgeValue() const
{
    int iValue;
    int iTempValue;
    int iI;

    iValue = 0;
    for (iI = 0; iI <  NUM_YIELD_TYPES; ++iI)
    {
		/* original bts code
        iTempValue = (GC.getYieldInfo((YieldTypes)iI).getGoldenAgeYield() * AI_yieldWeight((YieldTypes)iI));
        iTempValue /= std::max(1, (1 + GC.getYieldInfo((YieldTypes)iI).getGoldenAgeYieldThreshold())); */
		// K-Mod
		iTempValue = GC.getYieldInfo((YieldTypes)iI).getGoldenAgeYield() * AI_yieldWeight((YieldTypes)iI) * AI_averageYieldMultiplier((YieldTypes)iI);
		iTempValue /= 1 + GC.getYieldInfo((YieldTypes)iI).getGoldenAgeYieldThreshold();
		//
        iValue += iTempValue;
    }

    /* original bts code
	iValue *= getTotalPopulation();
    iValue *= GC.getGameINLINE().goldenAgeLength(); // original BtS code
    iValue /= 100; */
	// K-Mod
	iValue *= getTotalPopulation();
	iValue *= getGoldenAgeLength();
    iValue /= 10000;

	// K-Mod. Add some value if we would use the opportunity to switch civics
	// Note: this first "if" isn't necessary. It just saves us checking civics when we don't need to.
	if (getMaxAnarchyTurns() != 0 && !isGoldenAge() && getAnarchyModifier() + 100 > 0)
	{
		CivicTypes* paeBestCivic = new CivicTypes[GC.getNumCivicOptionInfos()];
		for (int iI = 0; iI < GC.getNumCivicOptionInfos(); iI++)
		{
			paeBestCivic[iI] = getCivics((CivicOptionTypes)iI);
		}

		for (int iI = 0; iI < GC.getNumCivicOptionInfos(); iI++)
		{
			int iCurrentValue = AI_civicValue(paeBestCivic[iI]);
			int iBestValue;
			CivicTypes eNewCivic = AI_bestCivic((CivicOptionTypes)iI, &iBestValue);
			
			// using a 7 percent thresold. (cf the higher threshold used in AI_doCivics)
			if (paeBestCivic[iI] != NO_CIVIC && 100*iBestValue > 107*iCurrentValue)
			{
				paeBestCivic[iI] = eNewCivic;
				if (gPlayerLogLevel > 0) logBBAI("      %S wants a golden age to switch to %S (value: %d vs %d)", getCivilizationDescription(0), GC.getCivicInfo(eNewCivic).getDescription(0), iBestValue, iCurrentValue);
			}
		}

		int iAnarchyLength = getCivicAnarchyLength(paeBestCivic);
		if (iAnarchyLength > 0)
		{
			// we would switch; so what is the negation of anarchy worth?
			for (int iI = 0; iI < NUM_COMMERCE_TYPES; iI++)
			{
				iTempValue = getCommerceRate((CommerceTypes)iI) * iAnarchyLength;
				iTempValue *= AI_commerceWeight((CommerceTypes)iI);
				iTempValue /= 100;
				iValue += iTempValue;
			}
			// production and GGP matter too, but I don't really want to try to evaluate them properly. Sorry.
			iValue += getTotalPopulation() * iAnarchyLength;
			// On the other hand, I'm ignoring the negation of maintenance cost.
		}
		SAFE_DELETE_ARRAY(paeBestCivic);
	}
	// K-Mod end

    return iValue;
}

// Protected Functions...

void CvPlayerAI::AI_doCounter()
{
	int iBonusImports;
	int iI, iJ;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			if (GET_PLAYER((PlayerTypes)iI).getTeam() != getTeam())
			{
				if (GET_TEAM(getTeam()).isHasMet(GET_PLAYER((PlayerTypes)iI).getTeam()))
				{
					if ((getStateReligion() != NO_RELIGION) &&
						  (getStateReligion() == GET_PLAYER((PlayerTypes)iI).getStateReligion()))
					{
						AI_changeSameReligionCounter(((PlayerTypes)iI), 1);
					}
					else
					{
						if (AI_getSameReligionCounter((PlayerTypes)iI) > 0)
						{
							AI_changeSameReligionCounter(((PlayerTypes)iI), -1);
						}
					}

					if ((getStateReligion() != NO_RELIGION) &&
						  (GET_PLAYER((PlayerTypes)iI).getStateReligion() != NO_RELIGION) &&
						  (getStateReligion() != GET_PLAYER((PlayerTypes)iI).getStateReligion()))
					{
						AI_changeDifferentReligionCounter(((PlayerTypes)iI), 1);
					}
					else
					{
						if (AI_getDifferentReligionCounter((PlayerTypes)iI) > 0)
						{
							AI_changeDifferentReligionCounter(((PlayerTypes)iI), -1);
						}
					}

					if (GC.getLeaderHeadInfo(getPersonalityType()).getFavoriteCivic() != NO_CIVIC)
					{
						if (isCivic((CivicTypes)(GC.getLeaderHeadInfo(getPersonalityType()).getFavoriteCivic())) &&
							  GET_PLAYER((PlayerTypes)iI).isCivic((CivicTypes)(GC.getLeaderHeadInfo(getPersonalityType()).getFavoriteCivic())))
						{
							AI_changeFavoriteCivicCounter(((PlayerTypes)iI), 1);
						}
						else
						{
							if (AI_getFavoriteCivicCounter((PlayerTypes)iI) > 0)
							{
								AI_changeFavoriteCivicCounter(((PlayerTypes)iI), -1);
							}
						}
					}

					iBonusImports = getNumTradeBonusImports((PlayerTypes)iI);

					if (iBonusImports > 0)
					{
						AI_changeBonusTradeCounter(((PlayerTypes)iI), iBonusImports);
					}
					else
					{
						AI_changeBonusTradeCounter(((PlayerTypes)iI), -(std::min(AI_getBonusTradeCounter((PlayerTypes)iI), ((GET_PLAYER((PlayerTypes)iI).getNumCities() / 4) + 1))));
					}
				}
			}
		}
	}

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			for (iJ = 0; iJ < NUM_CONTACT_TYPES; iJ++)
			{
				if (AI_getContactTimer(((PlayerTypes)iI), ((ContactTypes)iJ)) > 0)
				{
					AI_changeContactTimer(((PlayerTypes)iI), ((ContactTypes)iJ), -1);
				}
			}
		}
	}

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive())
		{
			for (iJ = 0; iJ < NUM_MEMORY_TYPES; iJ++)
			{
				if (AI_getMemoryCount(((PlayerTypes)iI), ((MemoryTypes)iJ)) > 0)
				{
					if (GC.getLeaderHeadInfo(getPersonalityType()).getMemoryDecayRand(iJ) > 0)
					{
						if (GC.getGameINLINE().getSorenRandNum(GC.getLeaderHeadInfo(getPersonalityType()).getMemoryDecayRand(iJ), "Memory Decay") == 0)
						{
							AI_changeMemoryCount(((PlayerTypes)iI), ((MemoryTypes)iJ), -1);
						}
					}
				}
			}
		}
	}
}


void CvPlayerAI::AI_doMilitary()
{
	/* original bts code
	if (GET_TEAM(getTeam()).getAnyWarPlanCount(true) == 0)
	{
		while (AI_isFinancialTrouble() && (calculateUnitCost() > 0))
		{
			if (!AI_disbandUnit(1, false))
			{
				break;
			}
		}
	} */
	// K-Mod
	PROFILE_FUNC();

	if (!isAnarchy() && AI_isFinancialTrouble() && GET_TEAM(getTeam()).AI_getWarSuccessRating() > -30)
	{
		int iCost = AI_unitCostPerMil();
		const int iMaxCost = AI_maxUnitCostPerMil();

		if (iCost > 0 && (iCost > iMaxCost || calculateGoldRate() < 0))
		{
			std::vector<std::pair<int, int> > unit_values; // <value, id>
			int iLoop;
			for (CvUnit* pLoopUnit = firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = nextUnit(&iLoop))
			{
				int iValue = AI_disbandValue(pLoopUnit);
				if (iValue < MAX_INT)
					unit_values.push_back(std::make_pair(iValue, pLoopUnit->getID()));
			}
			std::sort(unit_values.begin(), unit_values.end());

			size_t iDisbandCount = 0;
			do
			{
				if (unit_values.size() <= iDisbandCount)
					break;

				CvUnit* pDisbandUnit = getUnit(unit_values[iDisbandCount].second);
				FAssert(pDisbandUnit);
				if (gUnitLogLevel > 2)
				{
					CvWString aiTypeString;
					getUnitAIString(aiTypeString, pDisbandUnit->AI_getUnitAIType());
					logBBAI("    %S scraps '%S' %S, at (%d, %d), with value %d, due to financial trouble.", getCivilizationDescription(0), aiTypeString.GetCString(), pDisbandUnit->getName(0).GetCString(), pDisbandUnit->getX_INLINE(), pDisbandUnit->getY_INLINE(), unit_values[iDisbandCount].first);
				}

				pDisbandUnit->scrap();
				pDisbandUnit->doDelayedDeath();
				iDisbandCount++;

				iCost = AI_unitCostPerMil();
			} while (iCost > 0 && (iCost > iMaxCost || calculateGoldRate() < 0) && AI_isFinancialTrouble());
		}
	}
	// K-Mod end

	AI_setAttackOddsChange(GC.getLeaderHeadInfo(getPersonalityType()).getBaseAttackOddsChange() +
		GC.getGameINLINE().getSorenRandNum(GC.getLeaderHeadInfo(getPersonalityType()).getAttackOddsChangeRand(), "AI Attack Odds Change #1") +
		GC.getGameINLINE().getSorenRandNum(GC.getLeaderHeadInfo(getPersonalityType()).getAttackOddsChangeRand(), "AI Attack Odds Change #2"));
}


void CvPlayerAI::AI_doResearch()
{
	FAssertMsg(!isHuman(), "isHuman did not return false as expected");

	if (getCurrentResearch() == NO_TECH)
	{
		AI_chooseResearch();
		//AI_forceUpdateStrategies(); //to account for current research.
	}
}


void CvPlayerAI::AI_doCommerce()
{
	CvCity* pLoopCity;
	int iIdealPercent;
	int iGoldTarget;
	int iLoop;

	FAssertMsg(!isHuman(), "isHuman did not return false as expected");

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      07/20/09                                jdog5000      */
/*                                                                                              */
/* Barbarian AI, efficiency                                                                     */
/************************************************************************************************/
	if( isBarbarian() )
	{
		return;
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	iGoldTarget = AI_goldTarget();
	int iTargetTurns = 4 * GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).getResearchPercent();
	iTargetTurns /= 100;
	iTargetTurns = std::max(3, iTargetTurns);
	// K-Mod. make it faster on the way down
	bool bFinancialTrouble = AI_isFinancialTrouble();
	if (!bFinancialTrouble && getGold() > iGoldTarget)
		iTargetTurns = (iTargetTurns + 1)/2;
	// K-Mod end

    if (isCommerceFlexible(COMMERCE_RESEARCH) && !AI_avoidScience())
	{
        // set research rate to 100%
		setCommercePercent(COMMERCE_RESEARCH, 100);
		
		// if the gold rate is under 0 at 90% research
		int iGoldRate = calculateGoldRate();
		if (iGoldRate < 0)
		{
			TechTypes eCurrentResearch = getCurrentResearch();
			if (eCurrentResearch != NO_TECH)
			{
				int iResearchTurnsLeft = getResearchTurnsLeft(eCurrentResearch, true);
				
				// if we can finish the current research without running out of gold, let us spend 2/3rds of our gold 
				/* original bts code
				if (getGold() >= iResearchTurnsLeft * iGoldRate)
				{
					iGoldTarget /= 3;
				} */
				// K-Mod (what is the gold being stockpile for if not this?)
				if (getGold() >= iResearchTurnsLeft * -iGoldRate)
					iGoldTarget /= 5;
				// K-Mod end
			}
		}
	}

	bool bReset = false;

	if (isCommerceFlexible(COMMERCE_CULTURE))
	{
		if (getCommercePercent(COMMERCE_CULTURE) > 0)
		{
			setCommercePercent(COMMERCE_CULTURE, 0);

			bReset = true;
		}
	}

	if (isCommerceFlexible(COMMERCE_ESPIONAGE))
	{
		/* original BTS code
		if (getCommercePercent(COMMERCE_ESPIONAGE) > 0)
		{
			setCommercePercent(COMMERCE_ESPIONAGE, 0);

			for (int iTeam = 0; iTeam < MAX_CIV_TEAMS; ++iTeam)
			{
				setEspionageSpendingWeightAgainstTeam((TeamTypes)iTeam, 0);
			}

			bReset = true;
		} */
		//
		if (getCommercePercent(COMMERCE_ESPIONAGE) > 0)
		{
			setCommercePercent(COMMERCE_ESPIONAGE, 0);

			bReset = true;
		}
		//
	}

	if (bReset)
	{
		AI_assignWorkingPlots();
	}

	bool bFirstTech = AI_isFirstTech(getCurrentResearch());
	if (isCommerceFlexible(COMMERCE_CULTURE))
	{
		if (getNumCities() > 0)
		{
			iIdealPercent = 0;

			for (pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
			{
				if (pLoopCity->getCommerceHappinessPer(COMMERCE_CULTURE) > 0)
				{
					iIdealPercent += ((pLoopCity->angryPopulation() * 100) / pLoopCity->getCommerceHappinessPer(COMMERCE_CULTURE));
				}
			}

			iIdealPercent /= getNumCities();

			// K-Mod
			int iCap = 20;
			if (AI_isDoVictoryStrategy(AI_VICTORY_CULTURE2) && !bFirstTech)
			{
				iIdealPercent+=5;
				iCap += 10;
			}
			if (AI_isDoVictoryStrategy(AI_VICTORY_CULTURE3) && !bFirstTech)
			{
				iIdealPercent+=5;
				iCap += 20;
			}
			iIdealPercent += (AI_averageCulturePressure() - 100)/10;
			iIdealPercent = std::max(iIdealPercent, (AI_averageCulturePressure() - 100)/5);
			iIdealPercent -= (iIdealPercent % GC.getDefineINT("COMMERCE_PERCENT_CHANGE_INCREMENTS"));
			//iIdealPercent = std::min(iIdealPercent, 20);
			iIdealPercent = std::min(iIdealPercent, iCap);
			// K-Mod end

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      03/08/10                                jdog5000      */
/*                                                                                              */
/* Victory Strategy AI                                                                          */
/************************************************************************************************/
			if (AI_isDoVictoryStrategy(AI_VICTORY_CULTURE4))
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
			{
			    iIdealPercent = 100;
			}

			setCommercePercent(COMMERCE_CULTURE, iIdealPercent);
		}
	}

	if (isCommerceFlexible(COMMERCE_RESEARCH))
	{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      03/08/10                                jdog5000      */
/*                                                                                              */
/* Victory Strategy AI                                                                          */
/************************************************************************************************/
		if ((isNoResearchAvailable() || AI_isDoVictoryStrategy(AI_VICTORY_CULTURE4)) && !bFirstTech)
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
		{
			setCommercePercent(COMMERCE_RESEARCH, 0);
		}
		else
		{
			/* while (calculateGoldRate() > 0)
			{
				changeCommercePercent(COMMERCE_RESEARCH, GC.getDefineINT("COMMERCE_PERCENT_CHANGE_INCREMENTS"));

				if (getCommercePercent(COMMERCE_RESEARCH) == 100)
				{
					break;
				}
			} */
			// K-Mod disabled this!

			if (getGold() + iTargetTurns * calculateGoldRate() < iGoldTarget)
			{
				while (getGold() + iTargetTurns * calculateGoldRate() <= iGoldTarget)
				{
					changeCommercePercent(COMMERCE_RESEARCH, -(GC.getDefineINT("COMMERCE_PERCENT_CHANGE_INCREMENTS")));

					if ((getCommercePercent(COMMERCE_RESEARCH) == 0))
					{
						break;
					}
				}
			}
			else
			{
				if (AI_avoidScience())
				{
					changeCommercePercent(COMMERCE_RESEARCH, -(GC.getDefineINT("COMMERCE_PERCENT_CHANGE_INCREMENTS")));
				}
			}

			if ((GET_TEAM(getTeam()).getChosenWarCount(true) > 0) || (GET_TEAM(getTeam()).getWarPlanCount(WARPLAN_ATTACKED_RECENT, true) > 0))
			{
				changeCommercePercent(COMMERCE_RESEARCH, -(GC.getDefineINT("COMMERCE_PERCENT_CHANGE_INCREMENTS")));
			}

			if ((getCommercePercent(COMMERCE_RESEARCH) == 0) && (calculateGoldRate() > 0))
			{
				setCommercePercent(COMMERCE_RESEARCH, GC.getDefineINT("COMMERCE_PERCENT_CHANGE_INCREMENTS"));
			}
		}
	}

	// original BTS code
	/*
	if (isCommerceFlexible(COMMERCE_ESPIONAGE) && !bFirstTech)
	{
		int iEspionageTargetRate = 0;

		for (int iTeam = 0; iTeam < MAX_CIV_TEAMS; ++iTeam)
		{
			CvTeam& kLoopTeam = GET_TEAM((TeamTypes)iTeam);
			if (kLoopTeam.isAlive() && iTeam != getTeam() && !kLoopTeam.isVassal(getTeam()) && !GET_TEAM(getTeam()).isVassal((TeamTypes)iTeam))
			{
				int iTarget = (kLoopTeam.getEspionagePointsAgainstTeam(getTeam()) - GET_TEAM(getTeam()).getEspionagePointsAgainstTeam((TeamTypes)iTeam)) / 8;

				iTarget -= GET_TEAM(getTeam()).AI_getAttitudeVal((TeamTypes)iTeam);

				if (iTarget > 0)
				{
					iEspionageTargetRate += iTarget;
					changeEspionageSpendingWeightAgainstTeam((TeamTypes)iTeam, iTarget);
				}
			}
		}
		*/

	// K-Mod, partially based on the changes made by BETTER_BTS_AI_MOD
	if (isCommerceFlexible(COMMERCE_ESPIONAGE))
	{
		int iEspionageTargetRate = 0;
		int* aiTarget = new int[MAX_CIV_TEAMS];
		int* aiWeight = new int[MAX_CIV_TEAMS];
		int iHighestTarget = 0;
		int iMinModifier = INT_MAX;
		int iApproxTechCost = 0;
		TeamTypes eMinModTeam = NO_TEAM;
		bool bFocusEspionage = AI_isDoStrategy(AI_STRATEGY_BIG_ESPIONAGE) && GET_TEAM(getTeam()).getAnyWarPlanCount(true) == 0;

		// For this part of the AI, I will assume there is only mission for each purpose.
		EspionageMissionTypes eSeeDemographicsMission = NO_ESPIONAGEMISSION;
		EspionageMissionTypes eSeeResearchMission = NO_ESPIONAGEMISSION;
		EspionageMissionTypes eStealTechMission = NO_ESPIONAGEMISSION;
		EspionageMissionTypes eCityRevoltMission = NO_ESPIONAGEMISSION;

		for (int iMission = 0; iMission < GC.getNumEspionageMissionInfos(); ++iMission)
		{
			CvEspionageMissionInfo& kMissionInfo = GC.getEspionageMissionInfo((EspionageMissionTypes)iMission);
				
			if (kMissionInfo.isSeeDemographics())
				eSeeDemographicsMission = (EspionageMissionTypes)iMission;

			if (kMissionInfo.isSeeResearch())
				eSeeResearchMission = (EspionageMissionTypes)iMission;

			if (kMissionInfo.getBuyTechCostFactor() != 0)
				eStealTechMission = (EspionageMissionTypes)iMission;

			if (kMissionInfo.getCityRevoltCounter() > 0)
				eCityRevoltMission = (EspionageMissionTypes)iMission;
		}

		for (int iTeam = 0; iTeam < MAX_CIV_TEAMS; ++iTeam)
		{
			aiTarget[iTeam] = 0;
			aiWeight[iTeam] = 0;
		}

		for (int iTeam = 0; iTeam < MAX_CIV_TEAMS; ++iTeam)
		{
			CvTeam& kLoopTeam = GET_TEAM((TeamTypes)iTeam);
			if (kLoopTeam.isAlive() && iTeam != getTeam() && GET_TEAM(getTeam()).isHasMet((TeamTypes)iTeam) &&
				!kLoopTeam.isVassal(getTeam()) && !GET_TEAM(getTeam()).isVassal((TeamTypes)iTeam))
			{
				int iTheirEspPoints = kLoopTeam.getEspionagePointsAgainstTeam(getTeam());
				int iOurEspPoints = GET_TEAM(getTeam()).getEspionagePointsAgainstTeam((TeamTypes)iTeam);
				int iDesiredMissionPoints = 0;
				int iAttitude = range(GET_TEAM(getTeam()).AI_getAttitudeVal((TeamTypes)iTeam), -12, 12);
				WarPlanTypes eWarPlan = GET_TEAM(getTeam()).AI_getWarPlan((TeamTypes)iTeam);

				aiWeight[iTeam] = 6;
				int iRateDivisor = iTargetTurns*3;

				if (eWarPlan != NO_WARPLAN)
				{
					iTheirEspPoints *= 3;
					iTheirEspPoints /= 2;

					int iMissionCost = std::max(getEspionageMissionCost(eSeeResearchMission, kLoopTeam.getLeaderID()), getEspionageMissionCost(eSeeDemographicsMission, kLoopTeam.getLeaderID()));
					if (eWarPlan != WARPLAN_DOGPILE && AI_isDoStrategy(AI_STRATEGY_BIG_ESPIONAGE) && getCapitalCity())
					{
						CvCity* pTargetCity = getCapitalCity()->area()->getTargetCity(getID());
						if (pTargetCity && pTargetCity->getTeam() == iTeam && pTargetCity->area()->getAreaAIType(getTeam()) != AREAAI_DEFENSIVE && pTargetCity->area()->getNumAIUnits(getID(), UNITAI_SPY) > 0)
						{
							iMissionCost = std::max(iMissionCost, getEspionageMissionCost(eCityRevoltMission, pTargetCity->getOwnerINLINE(), pTargetCity->plot()));
							if (eWarPlan == WARPLAN_TOTAL || eWarPlan == WARPLAN_PREPARING_TOTAL)
							{
								iRateDivisor -= (iTargetTurns+2) / 4;
							}
						}
						else
							iMissionCost = std::max(iMissionCost, getEspionageMissionCost(eCityRevoltMission, kLoopTeam.getLeaderID()));
					}

					iMissionCost *= 12;
					iMissionCost /= 10;

					iDesiredMissionPoints = std::max(iDesiredMissionPoints, iMissionCost);

					iRateDivisor -= iTargetTurns/2;
					aiWeight[iTeam] += 6;

				}
				else
				{
					iTheirEspPoints -= (iTheirEspPoints*iAttitude)/(2*12);

					if (iAttitude <= -3)
					{
						int iMissionCost = std::max(getEspionageMissionCost(eSeeResearchMission, kLoopTeam.getLeaderID()), getEspionageMissionCost(eSeeDemographicsMission, kLoopTeam.getLeaderID()));
						iMissionCost *= 11;
						iMissionCost /= 10;
						iDesiredMissionPoints = std::max(iDesiredMissionPoints, iMissionCost);
					}
					else if (iAttitude < 3)
					{
						int iMissionCost = getEspionageMissionCost(eSeeDemographicsMission, kLoopTeam.getLeaderID());
						iMissionCost *= 11;
						iMissionCost /= 10;
						iDesiredMissionPoints = std::max(iDesiredMissionPoints, iMissionCost);
					}
				}

				iRateDivisor += 4*std::max(-4, iAttitude)/(5*iTargetTurns);
				iRateDivisor += AI_totalUnitAIs(UNITAI_SPY) == 0 ? 4 : 0;
				aiWeight[iTeam] -= (iAttitude/3);
				if (GET_TEAM(getTeam()).AI_hasCitiesInPrimaryArea((TeamTypes)iTeam))
				{
					aiWeight[iTeam] *= 2;
					aiWeight[iTeam] = std::max(0, aiWeight[iTeam]);
					iRateDivisor -= iTargetTurns;
				}

				// Individual player targeting
				if (bFocusEspionage)
				{
					for (int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; ++iPlayer)
					{
						CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iPlayer);
						if (kLoopPlayer.getTeam() == iTeam && kLoopPlayer.getNumCities() > 0)
						{
							std::vector<int> cityModifiers;
							CvCity* pLoopCity;
							int iLoop;
							int iTargetCities = 0;
							for (pLoopCity = kLoopPlayer.firstCity(&iLoop); pLoopCity != NULL; pLoopCity = kLoopPlayer.nextCity(&iLoop))
							{
								if (pLoopCity->isRevealed(getTeam(), false) && pLoopCity->area() != NULL && AI_isPrimaryArea(pLoopCity->area()))
								{
									cityModifiers.push_back(getEspionageMissionCostModifier(NO_ESPIONAGEMISSION, (PlayerTypes)iPlayer, pLoopCity->plot()));
								}
							}
							if (cityModifiers.size() > 0)
							{
								// Get the average of the lowest 3 cities.
								int iSampleSize = std::min(3, (int)cityModifiers.size());
								std::partial_sort(cityModifiers.begin(), cityModifiers.begin()+iSampleSize, cityModifiers.end());
								int iModifier = 0;
								for (std::vector<int>::iterator it = cityModifiers.begin(); it != cityModifiers.begin()+iSampleSize; ++it)
								{
									iModifier += *it;
								}
								iModifier /= iSampleSize;

								if (iModifier < iMinModifier ||
									(iModifier == iMinModifier && iAttitude < GET_TEAM(getTeam()).AI_getAttitudeVal(eMinModTeam)))
								{
									// do they have any techs we can steal?
									bool bValid = false;
									for (int iT = 0; iT < GC.getNumTechInfos(); iT++)
									{
										if (canStealTech((PlayerTypes)iPlayer, (TechTypes)iT))
										{
											bValid = iApproxTechCost > 0; // don't set it true unless there are at least 2 stealable techs.
											// get a (very rough) approximation of how much it will cost to steal a tech.
											iApproxTechCost = (GET_TEAM(getTeam()).getResearchCost((TechTypes)iT) + iApproxTechCost) / (iApproxTechCost != 0 ? 2 : 1);
											break;
										}
									}
									if (bValid)
									{
										iMinModifier = iModifier;
										eMinModTeam = (TeamTypes)iTeam;
									}
								}
							}
						}
					}
				}
				if (iTheirEspPoints > iDesiredMissionPoints
					&& GET_TEAM(getTeam()).AI_getAttitude((TeamTypes)iTeam) <= ATTITUDE_CAUTIOUS)
				{
					iRateDivisor += 1;
					// scale by esp points ever, so that we don't fall over ourselves trying to catch up with
					// a civ that happens to be running an espionage economy.
					int iOurTotal = std::max(4, GET_TEAM(getTeam()).getEspionagePointsEver());
					int iTheirTotal = std::max(4, kLoopTeam.getEspionagePointsEver());

					iTheirEspPoints *= (3*iOurTotal + iTheirTotal);
					iTheirEspPoints /= (2*iOurTotal + 4*iTheirTotal);
					// That's a factor between 3/2 and 1/4, centered at 4/6
					iDesiredMissionPoints = std::max(iTheirEspPoints, iDesiredMissionPoints);
				}

				aiTarget[iTeam] = (iDesiredMissionPoints - iOurEspPoints)/std::max(3*iTargetTurns/2,iRateDivisor);

				iEspionageTargetRate += std::max(0, aiTarget[iTeam]);
			}
		}

		// now, the big question is whether or not we can steal techs more easilly than we can research them.
		bool bCheapTechSteal = false;
		if (eMinModTeam != NO_TEAM && !AI_avoidScience() && !isNoResearchAvailable() && !AI_isDoVictoryStrategy(AI_VICTORY_SPACE3))
		{
			if (eStealTechMission != NO_ESPIONAGEMISSION)
			{
				iMinModifier *= 100 - GC.getDefineINT("MAX_FORTIFY_TURNS") * GC.getDefineINT("ESPIONAGE_EACH_TURN_UNIT_COST_DECREASE");
				iMinModifier /= 100;
				iMinModifier *= 100 + GC.getEspionageMissionInfo(eStealTechMission).getBuyTechCostFactor();
				iMinModifier /= 100;
				// This is the espionage cost modifier for stealing techs.

				// lets say "cheap" means 80% of the research cost.
				bCheapTechSteal = ((AI_isDoStrategy(AI_STRATEGY_ESPIONAGE_ECONOMY) ? 8500 : 8000) * AI_averageCommerceMultiplier(COMMERCE_ESPIONAGE) / std::max(1, iMinModifier) > AI_averageCommerceMultiplier(COMMERCE_RESEARCH)*calculateResearchModifier(getCurrentResearch()));
				if (bCheapTechSteal)
				{
					aiTarget[eMinModTeam] += iApproxTechCost / 10;
					iEspionageTargetRate += iApproxTechCost / 10;
					// I'm just using iApproxTechCost to get a rough sense of scale.
					// cf. (iDesiredEspPoints - iOurEspPoints)/std::max(6,iRateDivisor);
				}
			}
		}


		for (int iTeam = 0; iTeam < MAX_CIV_TEAMS; ++iTeam)
		{
			if (aiTarget[iTeam] > 0)
			{
				//aiWeight[iTeam] += (150*aiTarget[iTeam])/std::max(4,iEspionageTargetRate);
				aiWeight[iTeam] += (100*aiTarget[iTeam])/std::max(4,iEspionageTargetRate);
			}
			else if (aiTarget[iTeam] < 0)
			{
				if (iTeam != eMinModTeam &&
					(!isMaliciousEspionageTarget(GET_TEAM((TeamTypes)iTeam).getLeaderID()) || !GET_TEAM(getTeam()).AI_hasCitiesInPrimaryArea((TeamTypes)iTeam)))
					aiWeight[iTeam] = 0;
			}
			if (iTeam == eMinModTeam)
			{
				aiWeight[iTeam] = std::max(1, aiWeight[iTeam]);
				aiWeight[iTeam] *= 3;
				aiWeight[iTeam] /= 2;
			}
			else if (eMinModTeam != NO_TEAM && bCheapTechSteal)
			{
				aiWeight[iTeam] /= 2; // we want to focus hard on the best target
			}
			// note. bounds checks are done the set weight function
			setEspionageSpendingWeightAgainstTeam((TeamTypes)iTeam, aiWeight[iTeam]);
			// one last ad-hoc correction to the target espionage rate...
			if (aiWeight[iTeam] <= 0 && aiTarget[iTeam] > 0)
				iEspionageTargetRate -= aiTarget[iTeam];
		}
		SAFE_DELETE_ARRAY(aiTarget);
		SAFE_DELETE_ARRAY(aiWeight);
		// K-Mod end

		//if economy is weak, neglect espionage spending.
		//instead invest hammers into espionage via spies/builds
		if (bFinancialTrouble || AI_isDoVictoryStrategy(AI_VICTORY_CULTURE3) || bFirstTech) // K-Mod
		{
			//can still get trickle espionage income
			iEspionageTargetRate = 0;
		}
		else
		{
			if (!bCheapTechSteal) // K-Mod
			{
				iEspionageTargetRate *= (140 - getCommercePercent(COMMERCE_GOLD) * 2); // was 110 -
				iEspionageTargetRate /= 140; // was 110
			}

			iEspionageTargetRate *= GC.getLeaderHeadInfo(getLeaderType()).getEspionageWeight();
			iEspionageTargetRate /= 100;

			int iInitialResearchPercent = getCommercePercent(COMMERCE_RESEARCH);

			//while (getCommerceRate(COMMERCE_ESPIONAGE) < iEspionageTargetRate && getCommercePercent(COMMERCE_ESPIONAGE) < 20)
			// K-Mod
			int iCap = AI_isDoStrategy(AI_STRATEGY_BIG_ESPIONAGE) ? 20 : 10;
			iCap += bCheapTechSteal || AI_avoidScience() ? 50 : 0;

			FAssert(iCap <= 100);
			iCap = std::min(iCap, 100); // just in case.

			while (getCommerceRate(COMMERCE_ESPIONAGE) < iEspionageTargetRate && getCommercePercent(COMMERCE_ESPIONAGE) < iCap)
			{
				changeCommercePercent(COMMERCE_RESEARCH, -GC.getDefineINT("COMMERCE_PERCENT_CHANGE_INCREMENTS"));			
				changeCommercePercent(COMMERCE_ESPIONAGE, GC.getDefineINT("COMMERCE_PERCENT_CHANGE_INCREMENTS"));

				if (getGold() + iTargetTurns * calculateGoldRate() < iGoldTarget)
				{
					break;
				}

				if (!AI_avoidScience() && !isNoResearchAvailable() && !bCheapTechSteal)
				{
	//				if (2 * getCommercePercent(COMMERCE_RESEARCH) < iInitialResearchPercent)
	//				{
	//					break;
	//				}
					if (getCommercePercent(COMMERCE_RESEARCH) * 2 <= (getCommercePercent(COMMERCE_ESPIONAGE) + GC.getDefineINT("COMMERCE_PERCENT_CHANGE_INCREMENTS")) * 3)
					{
						break;
					}
				}
			}
		}
	}
	// K-Mod. prevent the AI from stockpiling excessive amounts of gold while in avoidScience.
	if (AI_avoidScience())
	{
		while (getCommercePercent(COMMERCE_GOLD) > 0 && getGold() + std::min(0, calculateGoldRate()) > iGoldTarget)
		{
			if (AI_isDoVictoryStrategy(AI_VICTORY_CULTURE3) && isCommerceFlexible(COMMERCE_CULTURE))
				changeCommercePercent(COMMERCE_CULTURE, GC.getDefineINT("COMMERCE_PERCENT_CHANGE_INCREMENTS"));
			else if (isCommerceFlexible(COMMERCE_ESPIONAGE))
				changeCommercePercent(COMMERCE_ESPIONAGE, GC.getDefineINT("COMMERCE_PERCENT_CHANGE_INCREMENTS"));
			else if (isCommerceFlexible(COMMERCE_RESEARCH)) // better than nothing...
				changeCommercePercent(COMMERCE_RESEARCH, GC.getDefineINT("COMMERCE_PERCENT_CHANGE_INCREMENTS"));
			else
				break;
		}
	}
	// K-Mod end
	
	if (!bFirstTech && (getGold() < iGoldTarget) && (getCommercePercent(COMMERCE_RESEARCH) > 40))
	{
		bool bHurryGold = false;
		for (int iHurry = 0; iHurry < GC.getNumHurryInfos(); iHurry++)
		{
			if ((GC.getHurryInfo((HurryTypes)iHurry).getGoldPerProduction() > 0) && canHurry((HurryTypes)iHurry))
			{
				bHurryGold = true;
				break;				
			}
		}
		if (bHurryGold)
		{
			if (getCommercePercent(COMMERCE_ESPIONAGE) > 0)
			{
				changeCommercePercent(COMMERCE_ESPIONAGE, -GC.getDefineINT("COMMERCE_PERCENT_CHANGE_INCREMENTS"));			
			}
			else
			{
				changeCommercePercent(COMMERCE_RESEARCH, -GC.getDefineINT("COMMERCE_PERCENT_CHANGE_INCREMENTS"));			
			}
			//changeCommercePercent(COMMERCE_GOLD, GC.getDefineINT("COMMERCE_PERCENT_CHANGE_INCREMENTS"));
		}
	}
	
	// this is called on doTurn, so make sure our gold is high enough keep us above zero gold.
	verifyGoldCommercePercent();
}

// K-Mod. I've rewriten most of this function, based on edits from BBAI. I don't know what's original bts code and what's not.
// (the BBAI implementation had some bugs)
void CvPlayerAI::AI_doCivics()
{
	FAssertMsg(!isHuman(), "isHuman did not return false as expected");

	if( isBarbarian() )
	{
		return;
	}

	if (AI_getCivicTimer() > 0)
	{
		AI_changeCivicTimer(-1);
		if (getGoldenAgeTurns() != 1) // K-Mod. If its the last turn of a golden age, consider switching civics anyway.
		{
			return;
		}
	}

	if (!canRevolution(NULL))
	{
		return;
	}

	// FAssertMsg(AI_getCivicTimer() == 0, "AI Civic timer is expected to be 0"); // Disabled by K-Mod

	std::vector<CivicTypes> aeBestCivic(GC.getNumCivicOptionInfos());
	std::vector<int> aiCurrentValue(GC.getNumCivicOptionInfos());

	for (int iI = 0; iI < GC.getNumCivicOptionInfos(); iI++)
	{
		aeBestCivic[iI] = getCivics((CivicOptionTypes)iI);
		aiCurrentValue[iI] = AI_civicValue(aeBestCivic[iI]);
	}

	int iAnarchyLength = 0;
	bool bWillSwitch;
	bool bWantSwitch;
	bool bFirstPass = true;
	do
	{
		bWillSwitch = false;
		bWantSwitch = false;
		for (int iI = 0; iI < GC.getNumCivicOptionInfos(); iI++)
		{
			int iBestValue;
			CivicTypes eNewCivic = AI_bestCivic((CivicOptionTypes)iI, &iBestValue);

			int iTestAnarchy = getCivicAnarchyLength(&aeBestCivic[0]);
			// using 20 percent as a rough estimate of revolution cost, and 2 percent just for a bit of inertia.
			// reduced threshold if we are already going to have a revolution.
			int iThreshold = (iTestAnarchy > iAnarchyLength ? (!bFirstPass | bWantSwitch ? 14 : 24) : 2);

			if (100*iBestValue > (100+iThreshold)*aiCurrentValue[iI])
			{
				FAssert(aeBestCivic[iI] != NO_CIVIC);
				if (gPlayerLogLevel > 0) logBBAI("    %S decides to switch to %S (value: %d vs %d%S)", getCivilizationDescription(0), GC.getCivicInfo(eNewCivic).getDescription(0), iBestValue, aiCurrentValue[iI], bFirstPass?"" :", on recheck");
				iAnarchyLength = iTestAnarchy;
				aeBestCivic[iI] = eNewCivic;
				aiCurrentValue[iI] = iBestValue;
				bWillSwitch = true;
			}
			else
			{
				if (100*iBestValue > 114*aiCurrentValue[iI])
					bWantSwitch = true;
			}
		}
		bFirstPass = false;
	} while (bWillSwitch && bWantSwitch);
	// Recheck, just in case we can switch another good civic without adding more anarchy.


	// finally, if our current research would give us a new civic, consider waiting for that.
	if (iAnarchyLength > 0 && bWillSwitch)
	{
		TechTypes eResearch = getCurrentResearch();
		int iResearchTurns;
		if (eResearch != NO_TECH && (iResearchTurns = getResearchTurnsLeft(eResearch, true)) < 2*CIVIC_CHANGE_DELAY/3)
		{
			for (int iI = 0; iI < GC.getNumCivicInfos(); iI++)
			{
				const CvCivicInfo& kCivic = GC.getCivicInfo((CivicTypes)iI);
				if (kCivic.getTechPrereq() == eResearch && !canDoCivics((CivicTypes)iI))
				{
					int iValue = AI_civicValue((CivicTypes)iI);
					if (100 * iValue > (114+2*iResearchTurns) * aiCurrentValue[kCivic.getCivicOptionType()])
					{
						CivicTypes eOtherCivic = aeBestCivic[kCivic.getCivicOptionType()];
						aeBestCivic[kCivic.getCivicOptionType()] = (CivicTypes)iI;
						if (getCivicAnarchyLength(&aeBestCivic[0]) <= iAnarchyLength)
						{
							if (gPlayerLogLevel > 0)
								logBBAI("    %S delays revolution to wait for %S (value: %d vs %d)", getCivilizationDescription(0), kCivic.getDescription(0), iValue, aiCurrentValue[kCivic.getCivicOptionType()]);
							AI_setCivicTimer(iResearchTurns*2/3);
							return;
						}
						aeBestCivic[kCivic.getCivicOptionType()] = eOtherCivic;
					}
				}
			}
		}
	}
	//

	if (canRevolution(&aeBestCivic[0]))
	{
		revolution(&aeBestCivic[0]);
		AI_setCivicTimer((getMaxAnarchyTurns() == 0) ? (GC.getDefineINT("MIN_REVOLUTION_TURNS") * 2) : CIVIC_CHANGE_DELAY);
	}
}

void CvPlayerAI::AI_doReligion()
{
	ReligionTypes eBestReligion;

	FAssertMsg(!isHuman(), "isHuman did not return false as expected");
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      07/20/09                                jdog5000      */
/*                                                                                              */
/* Barbarian AI, efficiency                                                                     */
/************************************************************************************************/
	if( isBarbarian() )
	{
		return;
	}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	if (AI_getReligionTimer() > 0)
	{
		AI_changeReligionTimer(-1);
		return;
	}

	if (!canChangeReligion())
	{
		return;
	}

	FAssertMsg(AI_getReligionTimer() == 0, "AI Religion timer is expected to be 0");

	eBestReligion = AI_bestReligion();

	if (eBestReligion == NO_RELIGION)
	{
		eBestReligion = getStateReligion();
	}

	if (canConvert(eBestReligion))
	{
		if (gPlayerLogLevel > 0)
		{
			logBBAI("    %S decides to convert to %S (value: %d vs %d)", getCivilizationDescription(0), GC.getReligionInfo(eBestReligion).getDescription(0),
				eBestReligion == NO_RELIGION ? 0 : AI_religionValue(eBestReligion), getStateReligion() == NO_RELIGION ? 0 : AI_religionValue(getStateReligion()));
		}
		convert(eBestReligion);
		AI_setReligionTimer((getMaxAnarchyTurns() == 0) ? (GC.getDefineINT("MIN_CONVERSION_TURNS") * 2) : RELIGION_CHANGE_DELAY);
	}
}

void CvPlayerAI::AI_doDiplo()
{
	PROFILE_FUNC();

	CLLNode<TradeData>* pNode;
	CvDiploParameters* pDiplo;
	CvDeal* pLoopDeal;
	CvCity* pLoopCity;
	CvPlot* pLoopPlot;
	CLinkList<TradeData> ourList;
	CLinkList<TradeData> theirList;
	bool abContacted[MAX_TEAMS];
	TradeData item;
	CivicTypes eFavoriteCivic;
	BonusTypes eBestReceiveBonus;
	BonusTypes eBestGiveBonus;
	TechTypes eBestReceiveTech;
	TechTypes eBestGiveTech;
	TeamTypes eBestTeam;
	bool bCancelDeal;
	int iReceiveGold;
	int iGiveGold;
	int iGold;
	//int iGoldData;
	//int iGoldWeight;
	int iGoldValuePercent;
	int iCount;
	int iPossibleCount;
	int iValue;
	int iBestValue;
	int iOurValue;
	int iTheirValue;
	int iPass;
	int iLoop;
	int iI, iJ;

	FAssert(!isHuman());
	FAssert(!isMinorCiv());
	FAssert(!isBarbarian());

	// allow python to handle it
	if (GC.getUSE_AI_DO_DIPLO_CALLBACK()) // K-Mod. block unused python callbacks
	{
		CyArgsList argsList;
		argsList.add(getID());
		long lResult=0;
		gDLL->getPythonIFace()->callFunction(PYGameModule, "AI_doDiplo", argsList.makeFunctionArgs(), &lResult);
		if (lResult == 1)
		{
			return;
		}
	}
	
	iGoldValuePercent = AI_goldTradeValuePercent();

	for (iI = 0; iI < MAX_TEAMS; iI++)
	{
		abContacted[iI] = false;
	}

	for (iPass = 0; iPass < 2; iPass++)
	{
		for (iI = 0; iI < MAX_CIV_PLAYERS; iI++)
		{
			if (GET_PLAYER((PlayerTypes)iI).isAlive())
			{
				if (GET_PLAYER((PlayerTypes)iI).isHuman() == (iPass == 1))
				{
					if (iI != getID())
					{
						//if (GET_PLAYER((PlayerTypes)iI).getTeam() != getTeam()) // disabled by K-Mod
						{
							for(pLoopDeal = GC.getGameINLINE().firstDeal(&iLoop); pLoopDeal != NULL; pLoopDeal = GC.getGameINLINE().nextDeal(&iLoop))
							{
								if (pLoopDeal->isCancelable(getID()))
								{
									// if ((GC.getGameINLINE().getGameTurn() - pLoopDeal->getInitialGameTurn()) >= (GC.getDefineINT("PEACE_TREATY_LENGTH") * 2)) // K-Mod disabled
									{
										bCancelDeal = false;

										if ((pLoopDeal->getFirstPlayer() == getID()) && (pLoopDeal->getSecondPlayer() == ((PlayerTypes)iI)))
										{
											/* original bts code
											if (GET_PLAYER((PlayerTypes)iI).isHuman())
											{
												if (!AI_considerOffer(((PlayerTypes)iI), pLoopDeal->getSecondTrades(), pLoopDeal->getFirstTrades(), -1))
												{
													bCancelDeal = true;
												}
											}
											else
											{
												for (pNode = pLoopDeal->getFirstTrades()->head(); pNode; pNode = pLoopDeal->getFirstTrades()->next(pNode))
												{
													if (getTradeDenial(((PlayerTypes)iI), pNode->m_data) != NO_DENIAL)
													{
														bCancelDeal = true;
														break;
													}
												}
											}*/
											// K-Mod. getTradeDenial is not equiped to consider deal cancelation properly.
											if (!AI_considerOffer(((PlayerTypes)iI), pLoopDeal->getSecondTrades(), pLoopDeal->getFirstTrades(), -1))
											{
												bCancelDeal = true;
											}
										}
										else if ((pLoopDeal->getFirstPlayer() == ((PlayerTypes)iI)) && (pLoopDeal->getSecondPlayer() == getID()))
										{
											/* original bts code
											if (GET_PLAYER((PlayerTypes)iI).isHuman())
											{
												if (!AI_considerOffer(((PlayerTypes)iI), pLoopDeal->getFirstTrades(), pLoopDeal->getSecondTrades(), -1))
												{
													bCancelDeal = true;
												}
											}
											else
											{
												for (pNode = pLoopDeal->getSecondTrades()->head(); pNode; pNode = pLoopDeal->getSecondTrades()->next(pNode))
												{
													if (getTradeDenial(((PlayerTypes)iI), pNode->m_data) != NO_DENIAL)
													{
														bCancelDeal = true;
														break;
													}
												}
											}*/
											// K-Mod
											if (!AI_considerOffer(((PlayerTypes)iI), pLoopDeal->getFirstTrades(), pLoopDeal->getSecondTrades(), -1))
											{
												bCancelDeal = true;
											}
										}

										if (bCancelDeal)
										{
											if (GET_PLAYER((PlayerTypes)iI).isHuman() && canContactAndTalk((PlayerTypes)iI))
											{
												ourList.clear();
												theirList.clear();
												bool bVassalDeal = pLoopDeal->isVassalDeal(); // K-Mod

												for (pNode = pLoopDeal->headFirstTradesNode(); (pNode != NULL); pNode = pLoopDeal->nextFirstTradesNode(pNode))
												{
													if (pLoopDeal->getFirstPlayer() == getID())
													{
														ourList.insertAtEnd(pNode->m_data);
													}
													else
													{
														theirList.insertAtEnd(pNode->m_data);
													}
												}

												for (pNode = pLoopDeal->headSecondTradesNode(); (pNode != NULL); pNode = pLoopDeal->nextSecondTradesNode(pNode))
												{
													if (pLoopDeal->getSecondPlayer() == getID())
													{
														ourList.insertAtEnd(pNode->m_data);
													}
													else
													{
														theirList.insertAtEnd(pNode->m_data);
													}
												}
												pLoopDeal->kill(); // K-Mod. Kill the old deal first.

												pDiplo = new CvDiploParameters(getID());
												FAssertMsg(pDiplo != NULL, "pDiplo must be valid");

												//if (pLoopDeal->isVassalDeal())
												if (bVassalDeal)
												{
													pDiplo->setDiploComment((DiploCommentTypes)GC.getInfoTypeForString("AI_DIPLOCOMMENT_NO_VASSAL"));
													pDiplo->setAIContact(true);
													gDLL->beginDiplomacy(pDiplo, ((PlayerTypes)iI));
												}
												else
												{
													pDiplo->setDiploComment((DiploCommentTypes)GC.getInfoTypeForString("AI_DIPLOCOMMENT_CANCEL_DEAL"));
													pDiplo->setAIContact(true);
													pDiplo->setOurOfferList(theirList);
													pDiplo->setTheirOfferList(ourList);
													gDLL->beginDiplomacy(pDiplo, (PlayerTypes)iI);
												}
												abContacted[GET_PLAYER((PlayerTypes)iI).getTeam()] = true;
											}
											// K-Mod.
											else
												pLoopDeal->kill();
											// K-Mod end

											//pLoopDeal->kill(); // XXX test this for AI...
											// K-Mod. I've rearranged stuff so that we can kill the deal before a diplomacy window.
										}
									}
								}
							}
						}

						if (canContactAndTalk((PlayerTypes)iI))
						{
							if (GET_PLAYER((PlayerTypes)iI).getTeam() == getTeam() || GET_TEAM(GET_PLAYER((PlayerTypes)iI).getTeam()).isVassal(getTeam()))
							{
								// XXX will it cancel this deal if it loses it's first resource???

								iBestValue = 0;
								eBestGiveBonus = NO_BONUS;

								for (iJ = 0; iJ < GC.getNumBonusInfos(); iJ++)
								{
									if (getNumTradeableBonuses((BonusTypes)iJ) > 1)
									{
										/* original bts code
										if ((GET_PLAYER((PlayerTypes)iI).AI_bonusTradeVal(((BonusTypes)iJ), getID(), 1) > 0)
											&& (GET_PLAYER((PlayerTypes)iI).AI_bonusVal((BonusTypes)iJ, 1) > AI_bonusVal((BonusTypes)iJ, -1))) */
										// K-Mod
										bool bHasBonus = GET_PLAYER((PlayerTypes)iI).getNumAvailableBonuses((BonusTypes)iJ) > 0;
										if (GET_PLAYER((PlayerTypes)iI).AI_bonusTradeVal(((BonusTypes)iJ), getID(), 1) > 0
											&& 2 * GET_PLAYER((PlayerTypes)iI).AI_bonusVal((BonusTypes)iJ, 1) > (bHasBonus? 2 : 3) * AI_bonusVal((BonusTypes)iJ, -1))
										// K-mod end
										{
											setTradeItem(&item, TRADE_RESOURCES, iJ);

											if (canTradeItem(((PlayerTypes)iI), item, true))
											{
												iValue = (1 + GC.getGameINLINE().getSorenRandNum(10000, "AI Bonus Trading #1"));

												if (iValue > iBestValue)
												{
													iBestValue = iValue;
													eBestGiveBonus = ((BonusTypes)iJ);
												}
											}
										}
									}
								}

								if (eBestGiveBonus != NO_BONUS)
								{
									ourList.clear();
									theirList.clear();

									setTradeItem(&item, TRADE_RESOURCES, eBestGiveBonus);
									ourList.insertAtEnd(item);

									if (GET_PLAYER((PlayerTypes)iI).isHuman())
									{
										if (!(abContacted[GET_PLAYER((PlayerTypes)iI).getTeam()]))
										{
											pDiplo = new CvDiploParameters(getID());
											FAssertMsg(pDiplo != NULL, "pDiplo must be valid");
											pDiplo->setDiploComment((DiploCommentTypes)GC.getInfoTypeForString("AI_DIPLOCOMMENT_GIVE_HELP"));
											pDiplo->setAIContact(true);
											pDiplo->setTheirOfferList(ourList);
											gDLL->beginDiplomacy(pDiplo, (PlayerTypes)iI);
											abContacted[GET_PLAYER((PlayerTypes)iI).getTeam()] = true;
										}
									}
									else
									{
										GC.getGameINLINE().implementDeal(getID(), ((PlayerTypes)iI), &ourList, &theirList);
									}
								}
							}

							if (GET_PLAYER((PlayerTypes)iI).getTeam() != getTeam() && GET_TEAM(GET_PLAYER((PlayerTypes)iI).getTeam()).isVassal(getTeam()))
							{
								iBestValue = 0;
								eBestGiveTech = NO_TECH;

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      12/06/09                                jdog5000      */
/*                                                                                              */
/* Diplomacy                                                                                    */
/************************************************************************************************/
								// Don't give techs for free to advanced vassals ...
								//if( GET_PLAYER((PlayerTypes)iI).getTechScore()*10 < getTechScore()*9 )
								if (GET_PLAYER((PlayerTypes)iI).getTechScore()*10 < getTechScore() * (GET_PLAYER((PlayerTypes)iI).isHuman() ? 7 : 9)) // K-Mod
								{
									for (iJ = 0; iJ < GC.getNumTechInfos(); iJ++)
									{
										if (GET_TEAM(getTeam()).AI_techTrade((TechTypes)iJ, GET_PLAYER((PlayerTypes)iI).getTeam()) == NO_DENIAL)
										{
											setTradeItem(&item, TRADE_TECHNOLOGIES, iJ);

											if (canTradeItem(((PlayerTypes)iI), item, true))
											{
												iValue = (1 + GC.getGameINLINE().getSorenRandNum(10000, "AI Vassal Tech gift"));

												if (iValue > iBestValue)
												{
													iBestValue = iValue;
													eBestGiveTech = ((TechTypes)iJ);
												}
											}
										}
									}
								}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

								if (eBestGiveTech != NO_TECH)
								{
									ourList.clear();
									theirList.clear();

									setTradeItem(&item, TRADE_TECHNOLOGIES, eBestGiveTech);
									ourList.insertAtEnd(item);

									if (GET_PLAYER((PlayerTypes)iI).isHuman())
									{
										if (!(abContacted[GET_PLAYER((PlayerTypes)iI).getTeam()]))
										{
											pDiplo = new CvDiploParameters(getID());
											FAssertMsg(pDiplo != NULL, "pDiplo must be valid");
											pDiplo->setDiploComment((DiploCommentTypes)GC.getInfoTypeForString("AI_DIPLOCOMMENT_GIVE_HELP"));
											pDiplo->setAIContact(true);
											pDiplo->setTheirOfferList(ourList);
											gDLL->beginDiplomacy(pDiplo, (PlayerTypes)iI);
											abContacted[GET_PLAYER((PlayerTypes)iI).getTeam()] = true;
										}
									}
									else
									{
										GC.getGameINLINE().implementDeal(getID(), ((PlayerTypes)iI), &ourList, &theirList);
									}
								}
							}

							if (GET_PLAYER((PlayerTypes)iI).getTeam() != getTeam() && !(GET_TEAM(getTeam()).isHuman()) && (GET_PLAYER((PlayerTypes)iI).isHuman() || !(GET_TEAM(GET_PLAYER((PlayerTypes)iI).getTeam()).isHuman())))
							{
								FAssertMsg(!(GET_PLAYER((PlayerTypes)iI).isBarbarian()), "(GET_PLAYER((PlayerTypes)iI).isBarbarian()) did not return false as expected");
								FAssertMsg(iI != getID(), "iI is not expected to be equal with getID()");

								if (GET_TEAM(GET_PLAYER((PlayerTypes)iI).getTeam()).isVassal(getTeam()))
								{
									iBestValue = 0;
									eBestGiveBonus = NO_BONUS;

									for (iJ = 0; iJ < GC.getNumBonusInfos(); iJ++)
									{
										if (GET_PLAYER((PlayerTypes)iI).getNumTradeableBonuses((BonusTypes)iJ) > 0 && getNumAvailableBonuses((BonusTypes)iJ) == 0)
										{
											iValue = AI_bonusTradeVal((BonusTypes)iJ, (PlayerTypes)iI, 1);

											if (iValue > iBestValue)
											{
												iBestValue = iValue;
												eBestGiveBonus = ((BonusTypes)iJ);
											}
										}
									}

									if (eBestGiveBonus != NO_BONUS)
									{
										theirList.clear();
										ourList.clear();

										setTradeItem(&item, TRADE_RESOURCES, eBestGiveBonus);
										theirList.insertAtEnd(item);

										if (GET_PLAYER((PlayerTypes)iI).isHuman())
										{
											if (!(abContacted[GET_PLAYER((PlayerTypes)iI).getTeam()]))
											{
												CvPopupInfo* pInfo = new CvPopupInfo(BUTTONPOPUP_VASSAL_GRANT_TRIBUTE, getID(), eBestGiveBonus);
												if (pInfo)
												{
													gDLL->getInterfaceIFace()->addPopup(pInfo, (PlayerTypes)iI);
													abContacted[GET_PLAYER((PlayerTypes)iI).getTeam()] = true;
												}
											}
										}
										else
										{
											GC.getGameINLINE().implementDeal(getID(), ((PlayerTypes)iI), &ourList, &theirList);
										}
									}
								}

								if (!(GET_TEAM(getTeam()).isAtWar(GET_PLAYER((PlayerTypes)iI).getTeam())))
								{
									if (AI_getAttitude((PlayerTypes)iI) >= ATTITUDE_CAUTIOUS)
									{
										for (pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
										{
											if (pLoopCity->getPreviousOwner() != ((PlayerTypes)iI))
											{
												if (((pLoopCity->getGameTurnAcquired() + 4) % 20) == (GC.getGameINLINE().getGameTurn() % 20))
												{
													iCount = 0;
													iPossibleCount = 0;

													for (iJ = 0; iJ < NUM_CITY_PLOTS; iJ++)
													{
														pLoopPlot = plotCity(pLoopCity->getX_INLINE(), pLoopCity->getY_INLINE(), iJ);

														if (pLoopPlot != NULL)
														{
															if (pLoopPlot->getOwnerINLINE() == iI)
															{
																iCount++;
															}

															iPossibleCount++;
														}
													}

													if (iCount >= (iPossibleCount / 2))
													{
														setTradeItem(&item, TRADE_CITIES, pLoopCity->getID());

														if (canTradeItem(((PlayerTypes)iI), item, true))
														{
															ourList.clear();

															ourList.insertAtEnd(item);

															if (GET_PLAYER((PlayerTypes)iI).isHuman())
															{
																//if (!(abContacted[GET_PLAYER((PlayerTypes)iI).getTeam()]))
																{
																	pDiplo = new CvDiploParameters(getID());
																	FAssertMsg(pDiplo != NULL, "pDiplo must be valid");
																	pDiplo->setDiploComment((DiploCommentTypes)GC.getInfoTypeForString("AI_DIPLOCOMMENT_OFFER_CITY"));
																	pDiplo->setAIContact(true);
																	pDiplo->setTheirOfferList(ourList);
																	gDLL->beginDiplomacy(pDiplo, (PlayerTypes)iI);
																	abContacted[GET_PLAYER((PlayerTypes)iI).getTeam()] = true;
																}
															}
															else
															{
																GC.getGameINLINE().implementDeal(getID(), ((PlayerTypes)iI), &ourList, NULL);
															}
														}
													}
												}
											}
										}
									}

									if (GET_TEAM(getTeam()).getLeaderID() == getID())
									{
										if (AI_getContactTimer(((PlayerTypes)iI), CONTACT_PERMANENT_ALLIANCE) == 0)
										{
											bool bOffered = false;
											if (GC.getGameINLINE().getSorenRandNum(GC.getLeaderHeadInfo(getPersonalityType()).getContactRand(CONTACT_PERMANENT_ALLIANCE), "AI Diplo Alliance") == 0)
											{
												setTradeItem(&item, TRADE_PERMANENT_ALLIANCE);

												if (canTradeItem(((PlayerTypes)iI), item, true) && GET_PLAYER((PlayerTypes)iI).canTradeItem(getID(), item, true))
												{
													ourList.clear();
													theirList.clear();

													ourList.insertAtEnd(item);
													theirList.insertAtEnd(item);

													bOffered = true;

													if (GET_PLAYER((PlayerTypes)iI).isHuman())
													{
														if (!(abContacted[GET_PLAYER((PlayerTypes)iI).getTeam()]))
														{
															AI_changeContactTimer(((PlayerTypes)iI), CONTACT_PERMANENT_ALLIANCE, GC.getLeaderHeadInfo(getPersonalityType()).getContactDelay(CONTACT_PERMANENT_ALLIANCE));
															pDiplo = new CvDiploParameters(getID());
															FAssertMsg(pDiplo != NULL, "pDiplo must be valid");
															pDiplo->setDiploComment((DiploCommentTypes)GC.getInfoTypeForString("AI_DIPLOCOMMENT_OFFER_DEAL"));
															pDiplo->setAIContact(true);
															pDiplo->setOurOfferList(theirList);
															pDiplo->setTheirOfferList(ourList);
															gDLL->beginDiplomacy(pDiplo, (PlayerTypes)iI);
															abContacted[GET_PLAYER((PlayerTypes)iI).getTeam()] = true;
														}
													}
													else
													{
														GC.getGameINLINE().implementDeal(getID(), ((PlayerTypes)iI), &ourList, &theirList);
														break; // move on to next player since we are on the same team now
													}
												}
											}

											if (!bOffered)
											{
												setTradeItem(&item, TRADE_VASSAL);

												if (canTradeItem((PlayerTypes)iI, item, true))
												{
													ourList.clear();
													theirList.clear();

													ourList.insertAtEnd(item);

													if (GET_PLAYER((PlayerTypes)iI).isHuman())
													{
														if (!(abContacted[GET_PLAYER((PlayerTypes)iI).getTeam()]))
														{
															AI_changeContactTimer(((PlayerTypes)iI), CONTACT_PERMANENT_ALLIANCE, GC.getLeaderHeadInfo(getPersonalityType()).getContactDelay(CONTACT_PERMANENT_ALLIANCE));
															pDiplo = new CvDiploParameters(getID());
															FAssertMsg(pDiplo != NULL, "pDiplo must be valid");
															pDiplo->setDiploComment((DiploCommentTypes)GC.getInfoTypeForString("AI_DIPLOCOMMENT_OFFER_VASSAL"));
															pDiplo->setAIContact(true);
															pDiplo->setOurOfferList(theirList);
															pDiplo->setTheirOfferList(ourList);
															gDLL->beginDiplomacy(pDiplo, (PlayerTypes)iI);
															abContacted[GET_PLAYER((PlayerTypes)iI).getTeam()] = true;
														}
													}
													else
													{
														bool bAccepted = true;
														TeamTypes eMasterTeam = GET_PLAYER((PlayerTypes)iI).getTeam();
														for (int iTeam = 0; iTeam < MAX_CIV_TEAMS; iTeam++)
														{
															if (GET_TEAM((TeamTypes)iTeam).isAlive())
															{
															if (iTeam != getTeam() && iTeam != eMasterTeam && atWar(getTeam(), (TeamTypes)iTeam) && !atWar(eMasterTeam, (TeamTypes)iTeam))
															{
																if (GET_TEAM(eMasterTeam).AI_declareWarTrade((TeamTypes)iTeam, getTeam(), false) != NO_DENIAL)
																{
																	bAccepted = false;
																	break;
																}
															}
														}
														}

														if (bAccepted)
														{
															GC.getGameINLINE().implementDeal(getID(), ((PlayerTypes)iI), &ourList, &theirList);
														}
													}
												}
											}
										}
									}

									if (GET_PLAYER((PlayerTypes)iI).isHuman() && (GET_TEAM(getTeam()).getLeaderID() == getID()) && !GET_TEAM(getTeam()).isVassal(GET_PLAYER((PlayerTypes)iI).getTeam()))
									{
										if (getStateReligion() != NO_RELIGION)
										{
											if (GET_PLAYER((PlayerTypes)iI).canConvert(getStateReligion()))
											{
												if (AI_getContactTimer(((PlayerTypes)iI), CONTACT_RELIGION_PRESSURE) == 0)
												{
													if (GC.getGameINLINE().getSorenRandNum(GC.getLeaderHeadInfo(getPersonalityType()).getContactRand(CONTACT_RELIGION_PRESSURE), "AI Diplo Religion Pressure") == 0)
													{
														if (!(abContacted[GET_PLAYER((PlayerTypes)iI).getTeam()]))
														{
															AI_changeContactTimer(((PlayerTypes)iI), CONTACT_RELIGION_PRESSURE, GC.getLeaderHeadInfo(getPersonalityType()).getContactDelay(CONTACT_RELIGION_PRESSURE));
															pDiplo = new CvDiploParameters(getID());
															FAssertMsg(pDiplo != NULL, "pDiplo must be valid");
															pDiplo->setDiploComment((DiploCommentTypes)GC.getInfoTypeForString("AI_DIPLOCOMMENT_RELIGION_PRESSURE"));
															pDiplo->setAIContact(true);
															gDLL->beginDiplomacy(pDiplo, (PlayerTypes)iI);
															abContacted[GET_PLAYER((PlayerTypes)iI).getTeam()] = true;
														}
													}
												}
											}
										}
									}

									if (GET_PLAYER((PlayerTypes)iI).isHuman() && (GET_TEAM(getTeam()).getLeaderID() == getID()) && !GET_TEAM(getTeam()).isVassal(GET_PLAYER((PlayerTypes)iI).getTeam()))
									{
										eFavoriteCivic = ((CivicTypes)(GC.getLeaderHeadInfo(getPersonalityType()).getFavoriteCivic()));

										if (eFavoriteCivic != NO_CIVIC)
										{
											if (isCivic(eFavoriteCivic))
											{
												if (GET_PLAYER((PlayerTypes)iI).canDoCivics(eFavoriteCivic) && !(GET_PLAYER((PlayerTypes)iI).isCivic(eFavoriteCivic)))
												{
													if (GET_PLAYER((PlayerTypes)iI).canRevolution(NULL))
													{
														if (AI_getContactTimer(((PlayerTypes)iI), CONTACT_CIVIC_PRESSURE) == 0)
														{
															if (GC.getGameINLINE().getSorenRandNum(GC.getLeaderHeadInfo(getPersonalityType()).getContactRand(CONTACT_CIVIC_PRESSURE), "AI Diplo Civic Pressure") == 0)
															{
																if (!(abContacted[GET_PLAYER((PlayerTypes)iI).getTeam()]))
																{
																	AI_changeContactTimer(((PlayerTypes)iI), CONTACT_CIVIC_PRESSURE, GC.getLeaderHeadInfo(getPersonalityType()).getContactDelay(CONTACT_CIVIC_PRESSURE));
																	pDiplo = new CvDiploParameters(getID());
																	FAssertMsg(pDiplo != NULL, "pDiplo must be valid");
																	pDiplo->setDiploComment((DiploCommentTypes)GC.getInfoTypeForString("AI_DIPLOCOMMENT_CIVIC_PRESSURE"), GC.getCivicInfo(eFavoriteCivic).getTextKeyWide());
																	pDiplo->setAIContact(true);
																	gDLL->beginDiplomacy(pDiplo, (PlayerTypes)iI);
																	abContacted[GET_PLAYER((PlayerTypes)iI).getTeam()] = true;
																}
															}
														}
													}
												}
											}
										}
									}

									if (GET_PLAYER((PlayerTypes)iI).isHuman() && (GET_TEAM(getTeam()).getLeaderID() == getID()))
									{
										if ((AI_getMemoryCount(((PlayerTypes)iI), MEMORY_DECLARED_WAR) == 0) && (AI_getMemoryCount(((PlayerTypes)iI), MEMORY_HIRED_WAR_ALLY) == 0))
										{
											if (AI_getContactTimer(((PlayerTypes)iI), CONTACT_JOIN_WAR) == 0)
											{
												if (GC.getGameINLINE().getSorenRandNum(GC.getLeaderHeadInfo(getPersonalityType()).getContactRand(CONTACT_JOIN_WAR), "AI Diplo Join War") == 0)
												{
													iBestValue = 0;
													eBestTeam = NO_TEAM;

													for (iJ = 0; iJ < MAX_CIV_TEAMS; iJ++)
													{
														if (GET_TEAM((TeamTypes)iJ).isAlive())
														{
															if (atWar(((TeamTypes)iJ), getTeam()) && !atWar(((TeamTypes)iJ), GET_PLAYER((PlayerTypes)iI).getTeam()))
															{
																if (GET_TEAM(GET_PLAYER((PlayerTypes)iI).getTeam()).isHasMet((TeamTypes)iJ))
																{
																	if (GET_TEAM(GET_PLAYER((PlayerTypes)iI).getTeam()).canDeclareWar((TeamTypes)iJ))
																	{
																		iValue = (1 + GC.getGameINLINE().getSorenRandNum(10000, "AI Joining War"));

																		if (iValue > iBestValue)
																		{
																			iBestValue = iValue;
																			eBestTeam = ((TeamTypes)iJ);
																		}
																	}
																}
															}
														}
													}

													if (eBestTeam != NO_TEAM)
													{
														if (!(abContacted[GET_PLAYER((PlayerTypes)iI).getTeam()]))
														{
															AI_changeContactTimer(((PlayerTypes)iI), CONTACT_JOIN_WAR, GC.getLeaderHeadInfo(getPersonalityType()).getContactDelay(CONTACT_JOIN_WAR));
															pDiplo = new CvDiploParameters(getID());
															FAssertMsg(pDiplo != NULL, "pDiplo must be valid");
															pDiplo->setDiploComment((DiploCommentTypes)GC.getInfoTypeForString("AI_DIPLOCOMMENT_JOIN_WAR"), GET_PLAYER(GET_TEAM(eBestTeam).getLeaderID()).getCivilizationAdjectiveKey());
															pDiplo->setAIContact(true);
															pDiplo->setData(eBestTeam);
															gDLL->beginDiplomacy(pDiplo, (PlayerTypes)iI);
															abContacted[GET_PLAYER((PlayerTypes)iI).getTeam()] = true;
														}
													}
												}
											}
										}
									}

									if (GET_PLAYER((PlayerTypes)iI).isHuman() && (GET_TEAM(getTeam()).getLeaderID() == getID()) && !GET_TEAM(getTeam()).isVassal(GET_PLAYER((PlayerTypes)iI).getTeam()))
									{
										if (AI_getContactTimer(((PlayerTypes)iI), CONTACT_STOP_TRADING) == 0)
										{
											if (GC.getGameINLINE().getSorenRandNum(GC.getLeaderHeadInfo(getPersonalityType()).getContactRand(CONTACT_STOP_TRADING), "AI Diplo Stop Trading") == 0)
											{
												eBestTeam = GET_TEAM(getTeam()).AI_getWorstEnemy();

												if ((eBestTeam != NO_TEAM) && GET_TEAM(GET_PLAYER((PlayerTypes)iI).getTeam()).isHasMet(eBestTeam) && !GET_TEAM(eBestTeam).isVassal(GET_PLAYER((PlayerTypes)iI).getTeam()))
												{
													if (GET_PLAYER((PlayerTypes)iI).canStopTradingWithTeam(eBestTeam))
													{
														FAssert(!atWar(GET_PLAYER((PlayerTypes)iI).getTeam(), eBestTeam));
														FAssert(GET_PLAYER((PlayerTypes)iI).getTeam() != eBestTeam);

														if (!(abContacted[GET_PLAYER((PlayerTypes)iI).getTeam()]))
														{
															AI_changeContactTimer(((PlayerTypes)iI), CONTACT_STOP_TRADING, GC.getLeaderHeadInfo(getPersonalityType()).getContactDelay(CONTACT_STOP_TRADING));
															pDiplo = new CvDiploParameters(getID());
															FAssertMsg(pDiplo != NULL, "pDiplo must be valid");
															pDiplo->setDiploComment((DiploCommentTypes)GC.getInfoTypeForString("AI_DIPLOCOMMENT_STOP_TRADING"), GET_PLAYER(GET_TEAM(eBestTeam).getLeaderID()).getCivilizationAdjectiveKey());
															pDiplo->setAIContact(true);
															pDiplo->setData(eBestTeam);
															gDLL->beginDiplomacy(pDiplo, (PlayerTypes)iI);
															abContacted[GET_PLAYER((PlayerTypes)iI).getTeam()] = true;
														}
													}
												}
											}
										}
									}

									if (GET_PLAYER((PlayerTypes)iI).isHuman() && (GET_TEAM(getTeam()).getLeaderID() == getID()))
									{
										if (GET_TEAM(GET_PLAYER((PlayerTypes)iI).getTeam()).getAssets() < (GET_TEAM(getTeam()).getAssets() / 2))
										{
											if (AI_getAttitude((PlayerTypes)iI) > GC.getLeaderHeadInfo(GET_PLAYER((PlayerTypes)iI).getPersonalityType()).getNoGiveHelpAttitudeThreshold())
											{
												if (AI_getContactTimer(((PlayerTypes)iI), CONTACT_GIVE_HELP) == 0)
												{
													if (GC.getGameINLINE().getSorenRandNum(GC.getLeaderHeadInfo(getPersonalityType()).getContactRand(CONTACT_GIVE_HELP), "AI Diplo Give Help") == 0)
													{
														// XXX maybe do gold instead???

														iBestValue = 0;
														eBestGiveTech = NO_TECH;

														for (iJ = 0; iJ < GC.getNumTechInfos(); iJ++)
														{
															setTradeItem(&item, TRADE_TECHNOLOGIES, iJ);

															if (canTradeItem(((PlayerTypes)iI), item, true))
															{
																iValue = (1 + GC.getGameINLINE().getSorenRandNum(10000, "AI Giving Help"));

																if (iValue > iBestValue)
																{
																	iBestValue = iValue;
																	eBestGiveTech = ((TechTypes)iJ);
																}
															}
														}

														if (eBestGiveTech != NO_TECH)
														{
															ourList.clear();

															setTradeItem(&item, TRADE_TECHNOLOGIES, eBestGiveTech);
															ourList.insertAtEnd(item);

															if (!(abContacted[GET_PLAYER((PlayerTypes)iI).getTeam()]))
															{
																AI_changeContactTimer(((PlayerTypes)iI), CONTACT_GIVE_HELP, GC.getLeaderHeadInfo(getPersonalityType()).getContactDelay(CONTACT_GIVE_HELP));
																pDiplo = new CvDiploParameters(getID());
																FAssertMsg(pDiplo != NULL, "pDiplo must be valid");
																pDiplo->setDiploComment((DiploCommentTypes)GC.getInfoTypeForString("AI_DIPLOCOMMENT_GIVE_HELP"));
																pDiplo->setAIContact(true);
																pDiplo->setTheirOfferList(ourList);
																gDLL->beginDiplomacy(pDiplo, (PlayerTypes)iI);
																abContacted[GET_PLAYER((PlayerTypes)iI).getTeam()] = true;
															}
														}
													}
												}
											}
										}
									}

									if (GET_PLAYER((PlayerTypes)iI).isHuman() && (GET_TEAM(getTeam()).getLeaderID() == getID()))
									{
										if (GET_TEAM(GET_PLAYER((PlayerTypes)iI).getTeam()).getAssets() > (GET_TEAM(getTeam()).getAssets() / 2))
										{
											if (AI_getContactTimer(((PlayerTypes)iI), CONTACT_ASK_FOR_HELP) == 0)
											{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      02/12/10                                jdog5000      */
/*                                                                                              */
/* Diplomacy                                                                                    */
/************************************************************************************************/
												int iRand = GC.getLeaderHeadInfo(getPersonalityType()).getContactRand(CONTACT_ASK_FOR_HELP);
												int iTechPerc = GET_TEAM(getTeam()).getBestKnownTechScorePercent();
												if( iTechPerc < 90 )
												{
													iRand *= std::max(1, iTechPerc - 60);
													iRand /= 30;
												}

												if (GC.getGameINLINE().getSorenRandNum(iRand, "AI Diplo Ask For Help") == 0)
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
												{
													iBestValue = 0;
													eBestReceiveTech = NO_TECH;

													for (iJ = 0; iJ < GC.getNumTechInfos(); iJ++)
													{
														setTradeItem(&item, TRADE_TECHNOLOGIES, iJ);

														if (GET_PLAYER((PlayerTypes)iI).canTradeItem(getID(), item, true))
														{
															iValue = (1 + GC.getGameINLINE().getSorenRandNum(10000, "AI Asking For Help"));

															if (iValue > iBestValue)
															{
																iBestValue = iValue;
																eBestReceiveTech = ((TechTypes)iJ);
															}
														}
													}

													if (eBestReceiveTech != NO_TECH)
													{
														theirList.clear();

														setTradeItem(&item, TRADE_TECHNOLOGIES, eBestReceiveTech);
														theirList.insertAtEnd(item);

														if (!(abContacted[GET_PLAYER((PlayerTypes)iI).getTeam()]))
														{
															AI_changeContactTimer(((PlayerTypes)iI), CONTACT_ASK_FOR_HELP, GC.getLeaderHeadInfo(getPersonalityType()).getContactDelay(CONTACT_ASK_FOR_HELP));
															pDiplo = new CvDiploParameters(getID());
															FAssertMsg(pDiplo != NULL, "pDiplo must be valid");
															pDiplo->setDiploComment((DiploCommentTypes)GC.getInfoTypeForString("AI_DIPLOCOMMENT_ASK_FOR_HELP"));
															pDiplo->setAIContact(true);
															pDiplo->setOurOfferList(theirList);
															gDLL->beginDiplomacy(pDiplo, (PlayerTypes)iI);
															abContacted[GET_PLAYER((PlayerTypes)iI).getTeam()] = true;
														}
													}
												}
											}
										}
									}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      05/06/09                                jdog5000      */
/*                                                                                              */
/* Bugfix, Diplomacy AI                                                                         */
/************************************************************************************************/
									// Reduced duplication so easier to maintain
									if (GET_PLAYER((PlayerTypes)iI).isHuman() && (GET_TEAM(getTeam()).getLeaderID() == getID()))
									{
/************************************************************************************************/
/* UNOFFICIAL_PATCH                       05/06/09                                jdog5000      */
/*                                                                                              */
/* Bugfix                                                                                       */
/************************************************************************************************/
/* original bts code
										if (GET_TEAM(getTeam()).canDeclareWar(GET_PLAYER((PlayerTypes)iI).getTeam()) && !GET_TEAM(getTeam()).AI_isSneakAttackPreparing(GET_PLAYER((PlayerTypes)iI).getTeam()))
*/
										// Bug fix: when team was sneak attack ready but hadn't declared, could demand tribute
										// If other team accepted, it blocked war declaration for 10 turns but AI didn't react.
										if (GET_TEAM(getTeam()).canDeclareWar(GET_PLAYER((PlayerTypes)iI).getTeam()) && !GET_TEAM(getTeam()).AI_isChosenWar(GET_PLAYER((PlayerTypes)iI).getTeam()))
/************************************************************************************************/
/* UNOFFICIAL_PATCH                        END                                                  */
/************************************************************************************************/
										{
											if (GET_TEAM(GET_PLAYER((PlayerTypes)iI).getTeam()).getDefensivePower() < GET_TEAM(getTeam()).getPower(true))
											{
												if (AI_getAttitude((PlayerTypes)iI) <= GC.getLeaderHeadInfo(GET_PLAYER((PlayerTypes)iI).getPersonalityType()).getDemandTributeAttitudeThreshold())
												{
													if (AI_getContactTimer(((PlayerTypes)iI), CONTACT_DEMAND_TRIBUTE) == 0)
													{
														if (GC.getGameINLINE().getSorenRandNum(GC.getLeaderHeadInfo(getPersonalityType()).getContactRand(CONTACT_DEMAND_TRIBUTE), "AI Diplo Demand Tribute") == 0)
														{
															iReceiveGold = std::min(std::max(0, (GET_PLAYER((PlayerTypes)iI).getGold() - 50)), GET_PLAYER((PlayerTypes)iI).AI_goldTarget());

															iReceiveGold -= (iReceiveGold % GC.getDefineINT("DIPLOMACY_VALUE_REMAINDER"));

															if (iReceiveGold > 50)
															{
																theirList.clear();

																setTradeItem(&item, TRADE_GOLD, iReceiveGold);
																theirList.insertAtEnd(item);

																if (!(abContacted[GET_PLAYER((PlayerTypes)iI).getTeam()]))
																{
																	AI_changeContactTimer(((PlayerTypes)iI), CONTACT_DEMAND_TRIBUTE, GC.getLeaderHeadInfo(getPersonalityType()).getContactDelay(CONTACT_DEMAND_TRIBUTE));
																	pDiplo = new CvDiploParameters(getID());
																	FAssertMsg(pDiplo != NULL, "pDiplo must be valid");
																	pDiplo->setDiploComment((DiploCommentTypes)GC.getInfoTypeForString("AI_DIPLOCOMMENT_DEMAND_TRIBUTE"));
																	pDiplo->setAIContact(true);
																	pDiplo->setOurOfferList(theirList);
																	gDLL->beginDiplomacy(pDiplo, (PlayerTypes)iI);
																	abContacted[GET_PLAYER((PlayerTypes)iI).getTeam()] = true;
																}
															}
														}

														if (GC.getGameINLINE().getSorenRandNum(GC.getLeaderHeadInfo(getPersonalityType()).getContactRand(CONTACT_DEMAND_TRIBUTE), "AI Diplo Demand Tribute") == 0)
														{
															if (GET_TEAM(getTeam()).AI_mapTradeVal(GET_PLAYER((PlayerTypes)iI).getTeam()) > 100)
															{
																theirList.clear();

																setTradeItem(&item, TRADE_MAPS);
																theirList.insertAtEnd(item);

																if (!(abContacted[GET_PLAYER((PlayerTypes)iI).getTeam()]))
																{
																	AI_changeContactTimer(((PlayerTypes)iI), CONTACT_DEMAND_TRIBUTE, GC.getLeaderHeadInfo(getPersonalityType()).getContactDelay(CONTACT_DEMAND_TRIBUTE));
																	pDiplo = new CvDiploParameters(getID());
																	FAssertMsg(pDiplo != NULL, "pDiplo must be valid");
																	pDiplo->setDiploComment((DiploCommentTypes)GC.getInfoTypeForString("AI_DIPLOCOMMENT_DEMAND_TRIBUTE"));
																	pDiplo->setAIContact(true);
																	pDiplo->setOurOfferList(theirList);
																	gDLL->beginDiplomacy(pDiplo, (PlayerTypes)iI);
																	abContacted[GET_PLAYER((PlayerTypes)iI).getTeam()] = true;
																}
															}
														}

														if (GC.getGameINLINE().getSorenRandNum(GC.getLeaderHeadInfo(getPersonalityType()).getContactRand(CONTACT_DEMAND_TRIBUTE), "AI Diplo Demand Tribute") == 0)
														{
															iBestValue = 0;
															eBestReceiveTech = NO_TECH;

															for (iJ = 0; iJ < GC.getNumTechInfos(); iJ++)
															{
																setTradeItem(&item, TRADE_TECHNOLOGIES, iJ);

																if (GET_PLAYER((PlayerTypes)iI).canTradeItem(getID(), item, true))
																{
																	if (GC.getGameINLINE().countKnownTechNumTeams((TechTypes)iJ) > 1)
																	{
																		iValue = (1 + GC.getGameINLINE().getSorenRandNum(10000, "AI Demanding Tribute (Tech)"));

																		if (iValue > iBestValue)
																		{
																			iBestValue = iValue;
																			eBestReceiveTech = ((TechTypes)iJ);
																		}
																	}
																}
															}

															if (eBestReceiveTech != NO_TECH)
															{
																theirList.clear();

																setTradeItem(&item, TRADE_TECHNOLOGIES, eBestReceiveTech);
																theirList.insertAtEnd(item);

																if (!(abContacted[GET_PLAYER((PlayerTypes)iI).getTeam()]))
																{
																	AI_changeContactTimer(((PlayerTypes)iI), CONTACT_DEMAND_TRIBUTE, GC.getLeaderHeadInfo(getPersonalityType()).getContactDelay(CONTACT_DEMAND_TRIBUTE));
																	pDiplo = new CvDiploParameters(getID());
																	FAssertMsg(pDiplo != NULL, "pDiplo must be valid");
																	pDiplo->setDiploComment((DiploCommentTypes)GC.getInfoTypeForString("AI_DIPLOCOMMENT_DEMAND_TRIBUTE"));
																	pDiplo->setAIContact(true);
																	pDiplo->setOurOfferList(theirList);
																	gDLL->beginDiplomacy(pDiplo, (PlayerTypes)iI);
																	abContacted[GET_PLAYER((PlayerTypes)iI).getTeam()] = true;
																}
															}
														}

														if (GC.getGameINLINE().getSorenRandNum(GC.getLeaderHeadInfo(getPersonalityType()).getContactRand(CONTACT_DEMAND_TRIBUTE), "AI Diplo Demand Tribute") == 0)
														{
															iBestValue = 0;
															eBestReceiveBonus = NO_BONUS;

															for (iJ = 0; iJ < GC.getNumBonusInfos(); iJ++)
															{
																if (GET_PLAYER((PlayerTypes)iI).getNumTradeableBonuses((BonusTypes)iJ) > 1)
																{
																	if (AI_bonusTradeVal(((BonusTypes)iJ), ((PlayerTypes)iI), 1) > 0)
																	{
																		setTradeItem(&item, TRADE_RESOURCES, iJ);

																		if (GET_PLAYER((PlayerTypes)iI).canTradeItem(getID(), item, true))
																		{
																			iValue = (1 + GC.getGameINLINE().getSorenRandNum(10000, "AI Demanding Tribute (Bonus)"));

																			if (iValue > iBestValue)
																			{
																				iBestValue = iValue;
																				eBestReceiveBonus = ((BonusTypes)iJ);
																			}
																		}
																	}
																}
															}

															if (eBestReceiveBonus != NO_BONUS)
															{
																theirList.clear();

																setTradeItem(&item, TRADE_RESOURCES, eBestReceiveBonus);
																theirList.insertAtEnd(item);

																if (!(abContacted[GET_PLAYER((PlayerTypes)iI).getTeam()]))
																{
																	AI_changeContactTimer(((PlayerTypes)iI), CONTACT_DEMAND_TRIBUTE, GC.getLeaderHeadInfo(getPersonalityType()).getContactDelay(CONTACT_DEMAND_TRIBUTE));
																	pDiplo = new CvDiploParameters(getID());
																	FAssertMsg(pDiplo != NULL, "pDiplo must be valid");
																	pDiplo->setDiploComment((DiploCommentTypes)GC.getInfoTypeForString("AI_DIPLOCOMMENT_DEMAND_TRIBUTE"));
																	pDiplo->setAIContact(true);
																	pDiplo->setOurOfferList(theirList);
																	gDLL->beginDiplomacy(pDiplo, (PlayerTypes)iI);
																	abContacted[GET_PLAYER((PlayerTypes)iI).getTeam()] = true;
																}
															}
														}
													}
												}
											}
										}
									}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

									if (GET_TEAM(getTeam()).getLeaderID() == getID())
									{
										if (AI_getContactTimer(((PlayerTypes)iI), CONTACT_OPEN_BORDERS) == 0)
										{
											if (GC.getGameINLINE().getSorenRandNum(GC.getLeaderHeadInfo(getPersonalityType()).getContactRand(CONTACT_OPEN_BORDERS), "AI Diplo Open Borders") == 0)
											{
												setTradeItem(&item, TRADE_OPEN_BORDERS);

												if (canTradeItem(((PlayerTypes)iI), item, true) && GET_PLAYER((PlayerTypes)iI).canTradeItem(getID(), item, true))
												{
													ourList.clear();
													theirList.clear();

													ourList.insertAtEnd(item);
													theirList.insertAtEnd(item);

													if (GET_PLAYER((PlayerTypes)iI).isHuman())
													{
														if (!(abContacted[GET_PLAYER((PlayerTypes)iI).getTeam()]))
														{
															AI_changeContactTimer(((PlayerTypes)iI), CONTACT_OPEN_BORDERS, GC.getLeaderHeadInfo(getPersonalityType()).getContactDelay(CONTACT_OPEN_BORDERS));
															pDiplo = new CvDiploParameters(getID());
															FAssertMsg(pDiplo != NULL, "pDiplo must be valid");
															pDiplo->setDiploComment((DiploCommentTypes)GC.getInfoTypeForString("AI_DIPLOCOMMENT_OFFER_DEAL"));
															pDiplo->setAIContact(true);
															pDiplo->setOurOfferList(theirList);
															pDiplo->setTheirOfferList(ourList);
															gDLL->beginDiplomacy(pDiplo, (PlayerTypes)iI);
															abContacted[GET_PLAYER((PlayerTypes)iI).getTeam()] = true;
														}
													}
													else
													{
														GC.getGameINLINE().implementDeal(getID(), ((PlayerTypes)iI), &ourList, &theirList);
													}
												}
											}
										}
									}

									if (GET_TEAM(getTeam()).getLeaderID() == getID())
									{
										if (AI_getContactTimer(((PlayerTypes)iI), CONTACT_DEFENSIVE_PACT) == 0)
										{
											if (GC.getGameINLINE().getSorenRandNum(GC.getLeaderHeadInfo(getPersonalityType()).getContactRand(CONTACT_DEFENSIVE_PACT), "AI Diplo Defensive Pact") == 0)
											{
												setTradeItem(&item, TRADE_DEFENSIVE_PACT);

												if (canTradeItem(((PlayerTypes)iI), item, true) && GET_PLAYER((PlayerTypes)iI).canTradeItem(getID(), item, true))
												{
													ourList.clear();
													theirList.clear();

													ourList.insertAtEnd(item);
													theirList.insertAtEnd(item);

													if (GET_PLAYER((PlayerTypes)iI).isHuman())
													{
														if (!(abContacted[GET_PLAYER((PlayerTypes)iI).getTeam()]))
														{
															AI_changeContactTimer(((PlayerTypes)iI), CONTACT_DEFENSIVE_PACT, GC.getLeaderHeadInfo(getPersonalityType()).getContactDelay(CONTACT_DEFENSIVE_PACT));
															pDiplo = new CvDiploParameters(getID());
															FAssertMsg(pDiplo != NULL, "pDiplo must be valid");
															pDiplo->setDiploComment((DiploCommentTypes)GC.getInfoTypeForString("AI_DIPLOCOMMENT_OFFER_DEAL"));
															pDiplo->setAIContact(true);
															pDiplo->setOurOfferList(theirList);
															pDiplo->setTheirOfferList(ourList);
															gDLL->beginDiplomacy(pDiplo, (PlayerTypes)iI);
															abContacted[GET_PLAYER((PlayerTypes)iI).getTeam()] = true;
														}
													}
													else
													{
														GC.getGameINLINE().implementDeal(getID(), ((PlayerTypes)iI), &ourList, &theirList);
													}
												}
											}
										}
									}

									if (!(GET_PLAYER((PlayerTypes)iI).isHuman()) || (GET_TEAM(getTeam()).getLeaderID() == getID()))
									{
										if (AI_getContactTimer(((PlayerTypes)iI), CONTACT_TRADE_TECH) == 0)
										{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      04/24/10                                jdog5000      */
/*                                                                                              */
/* Diplomacy                                                                                    */
/************************************************************************************************/
											int iRand = GC.getLeaderHeadInfo(getPersonalityType()).getContactRand(CONTACT_TRADE_TECH);
											int iTechPerc = GET_TEAM(getTeam()).getBestKnownTechScorePercent();
											if( iTechPerc < 90 )
											{
												/* BBAI
												iRand *= std::max(1, iTechPerc - 60);
												iRand /= 30; */
												// K-Mod. not so extreme.
												iRand *= 10 + iTechPerc;
												iRand /= 100;
												// K-Mod end
											}

											if( AI_isDoVictoryStrategy(AI_VICTORY_SPACE1) )
											{
												iRand /= 2;
											}
						
											iRand = std::max(1, iRand);
											if (GC.getGameINLINE().getSorenRandNum(iRand, "AI Diplo Trade Tech") == 0)
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
											{
												iBestValue = 0;
												eBestReceiveTech = NO_TECH;

												for (iJ = 0; iJ < GC.getNumTechInfos(); iJ++)
												{
													setTradeItem(&item, TRADE_TECHNOLOGIES, iJ);

													if (GET_PLAYER((PlayerTypes)iI).canTradeItem(getID(), item, true))
													{
														iValue = (1 + GC.getGameINLINE().getSorenRandNum(10000, "AI Tech Trading #1"));

														if (iValue > iBestValue)
														{
															iBestValue = iValue;
															eBestReceiveTech = ((TechTypes)iJ);
														}
													}
												}

												if (eBestReceiveTech != NO_TECH)
												{
													// K-Mod
													iOurValue = GET_TEAM(getTeam()).AI_techTradeVal(eBestReceiveTech, GET_PLAYER((PlayerTypes)iI).getTeam());
													iTheirValue = 0;
													int iBestDelta = iOurValue;
													// K-Mod end
													//iBestValue = 0;
													eBestGiveTech = NO_TECH;


													for (iJ = 0; iJ < GC.getNumTechInfos(); iJ++)
													{
														setTradeItem(&item, TRADE_TECHNOLOGIES, iJ);

														if (canTradeItem(((PlayerTypes)iI), item, true))
														{
															/* original bts code
															iValue = (1 + GC.getGameINLINE().getSorenRandNum(10000, "AI Tech Trading #2"));

															if (iValue > iBestValue)
															{
																iBestValue = iValue;
																eBestGiveTech = ((TechTypes)iJ);
															} */
															// K-Mod
															iValue = GET_TEAM(GET_PLAYER((PlayerTypes)iI).getTeam()).AI_techTradeVal((TechTypes)iJ, getTeam());
															int iDelta = std::abs(iOurValue - (90 + GC.getGameINLINE().getSorenRandNum(21, "AI Tech Trading #2"))*iValue / 100);
															if (iDelta < iBestDelta) // aim to trade values as close as possible
															{
																iBestDelta = iDelta;
																iTheirValue = iValue;
																eBestGiveTech = ((TechTypes)iJ);
															}
															// K-Mod end
														}
													}

													/* original bts code. (moved)
													iOurValue = GET_TEAM(getTeam()).AI_techTradeVal(eBestReceiveTech, GET_PLAYER((PlayerTypes)iI).getTeam());
													if (eBestGiveTech != NO_TECH)
													{
														iTheirValue = GET_TEAM(GET_PLAYER((PlayerTypes)iI).getTeam()).AI_techTradeVal(eBestGiveTech, getTeam());
													}
													else
													{
														iTheirValue = 0;
													}*/

													/* original bts code
													iReceiveGold = 0;
													iGiveGold = 0;

													if (iTheirValue > iOurValue)
													{
														iGoldWeight = iTheirValue - iOurValue;
														iGoldData = (iGoldWeight * 100) / iGoldValuePercent;
														if (iGoldWeight * 100 > iGoldData * iGoldValuePercent)
														{
															iGoldData++;															
														}
														
														iGold = std::min(iGoldData, GET_PLAYER((PlayerTypes)iI).AI_maxGoldTrade(getID()));

														if (iGold > 0)
														{
															setTradeItem(&item, TRADE_GOLD, iGold);

															if (GET_PLAYER((PlayerTypes)iI).canTradeItem(getID(), item, true))
															{
																iReceiveGold = iGold;
																iOurValue += (iGold * iGoldValuePercent) / 100;
															}
														}
													}
													else if (iOurValue > iTheirValue)
													{
														
														iGoldData = ((iOurValue - iTheirValue) * 100) / iGoldValuePercent;
														iGold = std::min(iGoldData, AI_maxGoldTrade((PlayerTypes)iI));

														if (iGold > 0)
														{
															setTradeItem(&item, TRADE_GOLD, iGold);

															if (canTradeItem(((PlayerTypes)iI), item, true))
															{
																iGiveGold = iGold;
																iTheirValue += (iGold * iGoldValuePercent) / 100;
															}
														}
													}

													if (!(GET_PLAYER((PlayerTypes)iI).isHuman()) || (iOurValue >= iTheirValue))
													{
														if ((iOurValue > ((iTheirValue * 2) / 3)) && (iTheirValue > ((iOurValue * 2) / 3)))
														{ */ // K-Mod.. (see what happens next)

													ourList.clear();
													theirList.clear();

													if (eBestGiveTech != NO_TECH)
													{
														setTradeItem(&item, TRADE_TECHNOLOGIES, eBestGiveTech);
														ourList.insertAtEnd(item);
													}

													setTradeItem(&item, TRADE_TECHNOLOGIES, eBestReceiveTech);
													theirList.insertAtEnd(item);

													/* original bts code
													if (iGiveGold != 0)
													{
														setTradeItem(&item, TRADE_GOLD, iGiveGold);
														ourList.insertAtEnd(item);
													}

													if (iReceiveGold != 0)
													{
														setTradeItem(&item, TRADE_GOLD, iReceiveGold);
														theirList.insertAtEnd(item);
													} */
													// K-Mod
													/* bool bDeal = GET_PLAYER((PlayerTypes)iI).isHuman()
														? 6*iOurValue > 5*iTheirValue && 3*iTheirValue > 2*iOurValue
														: 3*iOurValue > 2*iTheirValue && 3*iTheirValue > 2*iOurValue; */
													bool bDeal = 100*iOurValue >= AI_tradeAcceptabilityThreshold((PlayerTypes)iI)*iTheirValue
														&& 100*iTheirValue >= GET_PLAYER((PlayerTypes)iI).AI_tradeAcceptabilityThreshold(getID())*iOurValue;
													if (!bDeal)
													{
														PROFILE("AI_doDiplo self-counter-propose");
														// let the other team counter-propose (even if the other team is a human player)
														// unfortunately, the API is pretty clumsy for setting up this counter proposal.
														// please just bear with me.
														// (Note: this would be faster if we just built the lists directly, but by using the existing API, we are kind of future-proofing)
														CLinkList<TradeData> ourInventory, theirInventory, ourCounter, theirCounter;
														buildTradeTable((PlayerTypes)iI, ourInventory); // all tradeable items
														markTradeOffers(ourInventory, ourList); // K-Mod funciton - set m_bOffering on each item offered
														updateTradeList((PlayerTypes)iI, ourInventory, ourList, theirList); // hide what should be excluded
														GET_PLAYER((PlayerTypes)iI).buildTradeTable(getID(), theirInventory); // same for the other player
														GET_PLAYER((PlayerTypes)iI).markTradeOffers(theirInventory, theirList);
														GET_PLAYER((PlayerTypes)iI).updateTradeList(getID(), theirInventory, theirList, ourList);

														// Note: AI_counterPropose tends to favour the caller, which in this case is the other player.
														// ie. we asked for the trade, but they set the final terms. (unless the original deal is accepted)
														if (GET_PLAYER((PlayerTypes)iI).AI_counterPropose(getID(), &ourList, &theirList, &ourInventory, &theirInventory, &ourCounter, &theirCounter))
														{
															ourList.concatenate(ourCounter);
															theirList.concatenate(theirCounter);
															bDeal = true;
															// K-Mod note: the following log call will miss the second name if it is a non-standard name (for example, a human player name).
															// This is a problem in the way the getName function is implimented. Unfortunately, the heart of the problem is in the signature of
															// some of the dllexport functions - and so it can't easily be fixed.
															if (gTeamLogLevel >= 2) logBBAI("    %S makes a deal with %S using AI_counterPropose. (%d : %d)", getName(0), GET_PLAYER((PlayerTypes)iI).getName(0), ourList.getLength(), theirList.getLength());
														}
													}
													if (bDeal)
													{
													// K-Mod end
														if (GET_PLAYER((PlayerTypes)iI).isHuman())
														{
															if (!(abContacted[GET_PLAYER((PlayerTypes)iI).getTeam()]))
															{
																AI_changeContactTimer(((PlayerTypes)iI), CONTACT_TRADE_TECH, GC.getLeaderHeadInfo(getPersonalityType()).getContactDelay(CONTACT_TRADE_TECH));
																pDiplo = new CvDiploParameters(getID());
																FAssertMsg(pDiplo != NULL, "pDiplo must be valid");
																pDiplo->setDiploComment((DiploCommentTypes)GC.getInfoTypeForString("AI_DIPLOCOMMENT_OFFER_DEAL"));
																pDiplo->setAIContact(true);
																pDiplo->setOurOfferList(theirList);
																pDiplo->setTheirOfferList(ourList);
																gDLL->beginDiplomacy(pDiplo, (PlayerTypes)iI);
																abContacted[GET_PLAYER((PlayerTypes)iI).getTeam()] = true;
															}
														}
														else
														{
															GC.getGameINLINE().implementDeal(getID(), ((PlayerTypes)iI), &ourList, &theirList);
														}
													}
													//}
												}
											}
										}
									}

									if (AI_getContactTimer(((PlayerTypes)iI), CONTACT_TRADE_BONUS) == 0)
									{
										if (GC.getGameINLINE().getSorenRandNum(GC.getLeaderHeadInfo(getPersonalityType()).getContactRand(CONTACT_TRADE_BONUS), "AI Diplo Trade Bonus") == 0)
										{
											iBestValue = 0;
											eBestReceiveBonus = NO_BONUS;

											for (iJ = 0; iJ < GC.getNumBonusInfos(); iJ++)
											{
												if (GET_PLAYER((PlayerTypes)iI).getNumTradeableBonuses((BonusTypes)iJ) > 1)
												{
													if (GET_PLAYER((PlayerTypes)iI).AI_corporationBonusVal((BonusTypes)iJ) == 0)
													{
														if (AI_bonusTradeVal(((BonusTypes)iJ), ((PlayerTypes)iI), 1) > 0)
														{
															setTradeItem(&item, TRADE_RESOURCES, iJ);

															if (GET_PLAYER((PlayerTypes)iI).canTradeItem(getID(), item, true))
															{
																iValue = (1 + GC.getGameINLINE().getSorenRandNum(10000, "AI Bonus Trading #1"));

																if (iValue > iBestValue)
																{
																	iBestValue = iValue;
																	eBestReceiveBonus = ((BonusTypes)iJ);
																}
															}
														}
													}
												}
											}

											if (eBestReceiveBonus != NO_BONUS)
											{
												iBestValue = 0;
												eBestGiveBonus = NO_BONUS;

												for (iJ = 0; iJ < GC.getNumBonusInfos(); iJ++)
												{
													if (iJ != eBestReceiveBonus)
													{
														if (getNumTradeableBonuses((BonusTypes)iJ) > 1)
														{
															if (GET_PLAYER((PlayerTypes)iI).AI_bonusTradeVal(((BonusTypes)iJ), getID(), 1) > 0)
															{
																setTradeItem(&item, TRADE_RESOURCES, iJ);

																if (canTradeItem(((PlayerTypes)iI), item, true))
																{
																	iValue = (1 + GC.getGameINLINE().getSorenRandNum(10000, "AI Bonus Trading #2"));

																	if (iValue > iBestValue)
																	{
																		iBestValue = iValue;
																		eBestGiveBonus = ((BonusTypes)iJ);
																	}
																}
															}
														}
													}
												}

												if (eBestGiveBonus != NO_BONUS)
												{
													if (!(GET_PLAYER((PlayerTypes)iI).isHuman()) || (AI_bonusTradeVal(eBestReceiveBonus, ((PlayerTypes)iI), -1) >= GET_PLAYER((PlayerTypes)iI).AI_bonusTradeVal(eBestGiveBonus, getID(), 1)))
													{
														ourList.clear();
														theirList.clear();

														setTradeItem(&item, TRADE_RESOURCES, eBestGiveBonus);
														ourList.insertAtEnd(item);

														setTradeItem(&item, TRADE_RESOURCES, eBestReceiveBonus);
														theirList.insertAtEnd(item);

														if (GET_PLAYER((PlayerTypes)iI).isHuman())
														{
															if (!(abContacted[GET_PLAYER((PlayerTypes)iI).getTeam()]))
															{
																AI_changeContactTimer(((PlayerTypes)iI), CONTACT_TRADE_BONUS, GC.getLeaderHeadInfo(getPersonalityType()).getContactDelay(CONTACT_TRADE_BONUS));
																pDiplo = new CvDiploParameters(getID());
																FAssertMsg(pDiplo != NULL, "pDiplo must be valid");
																pDiplo->setDiploComment((DiploCommentTypes)GC.getInfoTypeForString("AI_DIPLOCOMMENT_OFFER_DEAL"));
																pDiplo->setAIContact(true);
																pDiplo->setOurOfferList(theirList);
																pDiplo->setTheirOfferList(ourList);
																gDLL->beginDiplomacy(pDiplo, (PlayerTypes)iI);
																abContacted[GET_PLAYER((PlayerTypes)iI).getTeam()] = true;
															}
														}
														else
														{
															GC.getGameINLINE().implementDeal(getID(), ((PlayerTypes)iI), &ourList, &theirList);
														}
													}
												}
											}
										}
									}

									if (AI_getContactTimer(((PlayerTypes)iI), CONTACT_TRADE_MAP) == 0)
									{
										if (GC.getGameINLINE().getSorenRandNum(GC.getLeaderHeadInfo(getPersonalityType()).getContactRand(CONTACT_TRADE_MAP), "AI Diplo Trade Map") == 0)
										{
											setTradeItem(&item, TRADE_MAPS);

											if (GET_PLAYER((PlayerTypes)iI).canTradeItem(getID(), item, true) && canTradeItem(((PlayerTypes)iI), item, true))
											{
												if (!(GET_PLAYER((PlayerTypes)iI).isHuman()) || (GET_TEAM(getTeam()).AI_mapTradeVal(GET_PLAYER((PlayerTypes)iI).getTeam()) >= GET_TEAM(GET_PLAYER((PlayerTypes)iI).getTeam()).AI_mapTradeVal(getTeam())))
												{
													ourList.clear();
													theirList.clear();

													setTradeItem(&item, TRADE_MAPS);
													ourList.insertAtEnd(item);

													setTradeItem(&item, TRADE_MAPS);
													theirList.insertAtEnd(item);

													if (GET_PLAYER((PlayerTypes)iI).isHuman())
													{
														if (!(abContacted[GET_PLAYER((PlayerTypes)iI).getTeam()]))
														{
															AI_changeContactTimer(((PlayerTypes)iI), CONTACT_TRADE_MAP, GC.getLeaderHeadInfo(getPersonalityType()).getContactDelay(CONTACT_TRADE_MAP));
															pDiplo = new CvDiploParameters(getID());
															FAssertMsg(pDiplo != NULL, "pDiplo must be valid");
															pDiplo->setDiploComment((DiploCommentTypes)GC.getInfoTypeForString("AI_DIPLOCOMMENT_OFFER_DEAL"));
															pDiplo->setAIContact(true);
															pDiplo->setOurOfferList(theirList);
															pDiplo->setTheirOfferList(ourList);
															gDLL->beginDiplomacy(pDiplo, (PlayerTypes)iI);
															abContacted[GET_PLAYER((PlayerTypes)iI).getTeam()] = true;
														}
													}
													else
													{
														GC.getGameINLINE().implementDeal(getID(), ((PlayerTypes)iI), &ourList, &theirList);
													}
												}
											}
										}
									}

									if (!(GET_PLAYER((PlayerTypes)iI).isHuman()) || !(abContacted[GET_PLAYER((PlayerTypes)iI).getTeam()]))
									{
										int iDeclareWarTradeRand = GC.getLeaderHeadInfo(getPersonalityType()).getDeclareWarTradeRand();
										int iMinAtWarCounter = MAX_INT;
										for (iJ = 0; iJ < MAX_CIV_TEAMS; iJ++)
										{
											if (GET_TEAM((TeamTypes)iJ).isAlive())
											{
												if (atWar(((TeamTypes)iJ), getTeam()))
												{
													int iAtWarCounter = GET_TEAM(getTeam()).AI_getAtWarCounter((TeamTypes)iJ);
													if (GET_TEAM(getTeam()).AI_getWarPlan((TeamTypes)iJ) == WARPLAN_DOGPILE)
													{
														iAtWarCounter *= 2;
														iAtWarCounter += 5;
													}
													iMinAtWarCounter = std::min(iAtWarCounter, iMinAtWarCounter);
												}
											}
										}
										
										if (iMinAtWarCounter < 10)
										{
											iDeclareWarTradeRand *= iMinAtWarCounter;
											iDeclareWarTradeRand /= 10;
											iDeclareWarTradeRand ++;
										}
										
										if (iMinAtWarCounter < 4)
										{
											iDeclareWarTradeRand /= 4;
											iDeclareWarTradeRand ++;
										}

										if (GC.getGameINLINE().getSorenRandNum(iDeclareWarTradeRand, "AI Diplo Declare War Trade") == 0)
										{
											iBestValue = 0;
											eBestTeam = NO_TEAM;

											for (iJ = 0; iJ < MAX_CIV_TEAMS; iJ++)
											{
												if (GET_TEAM((TeamTypes)iJ).isAlive())
												{
													if (atWar(((TeamTypes)iJ), getTeam()) && !atWar(((TeamTypes)iJ), GET_PLAYER((PlayerTypes)iI).getTeam()))
													{
														if (GET_TEAM((TeamTypes)iJ).getAtWarCount(true) < std::max(2, (GC.getGameINLINE().countCivTeamsAlive() / 2)))
														{
															setTradeItem(&item, TRADE_WAR, iJ);

															if (GET_PLAYER((PlayerTypes)iI).canTradeItem(getID(), item, true))
															{
																iValue = (1 + GC.getGameINLINE().getSorenRandNum(1000, "AI Declare War Trading"));
																
																iValue *= (101 + GET_TEAM((TeamTypes)iJ).AI_getAttitudeWeight(getTeam()));
																iValue /= 100;

																if (iValue > iBestValue)
																{
																	iBestValue = iValue;
																	eBestTeam = ((TeamTypes)iJ);
																}
															}
														}
													}
												}
											}

											if (eBestTeam != NO_TEAM)
											{
												iBestValue = 0;
												eBestGiveTech = NO_TECH;

												for (iJ = 0; iJ < GC.getNumTechInfos(); iJ++)
												{
													setTradeItem(&item, TRADE_TECHNOLOGIES, iJ);

													if (canTradeItem(((PlayerTypes)iI), item, true))
													{
														iValue = (1 + GC.getGameINLINE().getSorenRandNum(100, "AI Tech Trading #2"));
														
														iValue *= GET_TEAM(eBestTeam).getResearchLeft((TechTypes)iJ);

														if (iValue > iBestValue)
														{
															iBestValue = iValue;
															eBestGiveTech = ((TechTypes)iJ);
														}
													}
												}

												iOurValue = GET_TEAM(getTeam()).AI_declareWarTradeVal(eBestTeam, GET_PLAYER((PlayerTypes)iI).getTeam());
												if (eBestGiveTech != NO_TECH)
												{
													iTheirValue = GET_TEAM(GET_PLAYER((PlayerTypes)iI).getTeam()).AI_techTradeVal(eBestGiveTech, getTeam());
												}
												else
												{
													iTheirValue = 0;
												}

												int iBestValue2 = 0;
												TechTypes eBestGiveTech2 = NO_TECH;
												
												if ((iTheirValue < iOurValue) && (eBestGiveTech != NO_TECH))
												{
													for (iJ = 0; iJ < GC.getNumTechInfos(); iJ++)
													{
														if (iJ != eBestGiveTech)
														{
															setTradeItem(&item, TRADE_TECHNOLOGIES, iJ);

															if (canTradeItem(((PlayerTypes)iI), item, true))
															{
																iValue = (1 + GC.getGameINLINE().getSorenRandNum(100, "AI Tech Trading #2"));
																
																iValue *= GET_TEAM(eBestTeam).getResearchLeft((TechTypes)iJ);

																if (iValue > iBestValue)
																{
																	iBestValue2 = iValue;
																	eBestGiveTech2 = ((TechTypes)iJ);
																}
															}
														}
													}
													
													if (eBestGiveTech2 != NO_TECH)
													{
														iTheirValue += GET_TEAM(GET_PLAYER((PlayerTypes)iI).getTeam()).AI_techTradeVal(eBestGiveTech2, getTeam());
													}
												}

												iReceiveGold = 0;
												iGiveGold = 0;

												if (iTheirValue > iOurValue)
												{
													iGold = std::min(((iTheirValue - iOurValue) * 100) / iGoldValuePercent, GET_PLAYER((PlayerTypes)iI).AI_maxGoldTrade(getID()));

													if (iGold > 0)
													{
														setTradeItem(&item, TRADE_GOLD, iGold);

														if (GET_PLAYER((PlayerTypes)iI).canTradeItem(getID(), item, true))
														{
															iReceiveGold = iGold;
															iOurValue += (iGold * iGoldValuePercent) / 100;
														}
													}
												}
												else if (iOurValue > iTheirValue)
												{
													iGold = std::min(((iOurValue - iTheirValue) * 100) / iGoldValuePercent, AI_maxGoldTrade((PlayerTypes)iI));

													if (iGold > 0)
													{
														setTradeItem(&item, TRADE_GOLD, iGold);

														if (canTradeItem(((PlayerTypes)iI), item, true))
														{
															iGiveGold = iGold;
															iTheirValue += (iGold * iGoldValuePercent) / 100;
														}
													}
												}

												if (iTheirValue > (iOurValue * 3 / 4))
												{
													ourList.clear();
													theirList.clear();

													if (eBestGiveTech != NO_TECH)
													{
														setTradeItem(&item, TRADE_TECHNOLOGIES, eBestGiveTech);
														ourList.insertAtEnd(item);
													}
													
													if (eBestGiveTech2 != NO_TECH)
													{
														setTradeItem(&item, TRADE_TECHNOLOGIES, eBestGiveTech2);
														ourList.insertAtEnd(item);
													}

													setTradeItem(&item, TRADE_WAR, eBestTeam);
													theirList.insertAtEnd(item);

													if (iGiveGold != 0)
													{
														setTradeItem(&item, TRADE_GOLD, iGiveGold);
														ourList.insertAtEnd(item);
													}

													if (iReceiveGold != 0)
													{
														setTradeItem(&item, TRADE_GOLD, iReceiveGold);
														theirList.insertAtEnd(item);
													}

													GC.getGameINLINE().implementDeal(getID(), ((PlayerTypes)iI), &ourList, &theirList);
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


//
// read object from a stream
// used during load
//
void CvPlayerAI::read(FDataStreamBase* pStream)
{
	CvPlayer::read(pStream);	// read base class data first

	uint uiFlag=0;
	pStream->Read(&uiFlag);	// flags for expansion

	pStream->Read(&m_iPeaceWeight);
	pStream->Read(&m_iEspionageWeight);
	pStream->Read(&m_iAttackOddsChange);
	pStream->Read(&m_iCivicTimer);
	pStream->Read(&m_iReligionTimer);
	pStream->Read(&m_iExtraGoldTarget);

	pStream->Read(&m_iStrategyHash);
	//pStream->Read(&m_iStrategyHashCacheTurn); // disabled by K-Mod
// BBAI (edited for K-Mod)
	pStream->Read(&m_iStrategyRand);
	pStream->Read(&m_iVictoryStrategyHash);
// BBAI end
	//pStream->Read(&m_iAveragesCacheTurn); // disabled by K-Mod
	pStream->Read(&m_iAverageGreatPeopleMultiplier);
	pStream->Read(&m_iAverageCulturePressure); // K-Mod

	pStream->Read(NUM_YIELD_TYPES, m_aiAverageYieldMultiplier);
	pStream->Read(NUM_COMMERCE_TYPES, m_aiAverageCommerceMultiplier);
	pStream->Read(NUM_COMMERCE_TYPES, m_aiAverageCommerceExchange);

	//pStream->Read(&m_iUpgradeUnitsCacheTurn); // disabled by K-Mod
	//pStream->Read(&m_iUpgradeUnitsCachedExpThreshold); // disabled by K-Mod
	pStream->Read(&m_iUpgradeUnitsCachedGold);

	pStream->Read(NUM_UNITAI_TYPES, m_aiNumTrainAIUnits);
	pStream->Read(NUM_UNITAI_TYPES, m_aiNumAIUnits);

	pStream->Read(MAX_PLAYERS, m_aiSameReligionCounter);
	pStream->Read(MAX_PLAYERS, m_aiDifferentReligionCounter);
	pStream->Read(MAX_PLAYERS, m_aiFavoriteCivicCounter);
	pStream->Read(MAX_PLAYERS, m_aiBonusTradeCounter);
	pStream->Read(MAX_PLAYERS, m_aiPeacetimeTradeValue);
	pStream->Read(MAX_PLAYERS, m_aiPeacetimeGrantValue);
	pStream->Read(MAX_PLAYERS, m_aiGoldTradedTo);
	pStream->Read(MAX_PLAYERS, m_aiAttitudeExtra);
	// K-Mod. Load the attitude cache. (originally, in BBAI and the CAR Mod, this was not saved.
	// But there are rare situations in which it needs to be saved/read to avoid OOS errors.)
	if (uiFlag >= 4)
		pStream->Read(MAX_PLAYERS, m_aiAttitudeCache);
	else
		AI_invalidateAttitudeCache();
	// K-Mod end
	pStream->Read(MAX_PLAYERS, m_abFirstContact);

	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		pStream->Read(NUM_CONTACT_TYPES, m_aaiContactTimer[i]);
	}
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		pStream->Read(NUM_MEMORY_TYPES, m_aaiMemoryCount[i]);
	}

	pStream->Read(&m_bWasFinancialTrouble);
	pStream->Read(&m_iTurnLastProductionDirty);
	
	{
		m_aiAICitySites.clear();
		uint iSize;
		pStream->Read(&iSize);
		for (uint i = 0; i < iSize; i++)
		{
			int iCitySite;
			pStream->Read(&iCitySite);
			m_aiAICitySites.push_back(iCitySite);
		}
	}

	pStream->Read(GC.getNumBonusInfos(), m_aiBonusValue);
	pStream->Read(GC.getNumUnitClassInfos(), m_aiUnitClassWeights);
	pStream->Read(GC.getNumUnitCombatInfos(), m_aiUnitCombatWeights);
	// K-Mod
	m_GreatPersonWeights.clear();
	if (uiFlag >= 5)
	{
		int iItems;
		int iType, iWeight;
		pStream->Read(&iItems);
		for (int i = 0; i < iItems; i++)
		{
			pStream->Read(&iType);
			pStream->Read(&iWeight);
			m_GreatPersonWeights.insert(std::make_pair((UnitClassTypes)iType, iWeight));
		}
	}
	// K-Mod end
	pStream->Read(MAX_PLAYERS, m_aiCloseBordersAttitudeCache);
}


//
// save object to a stream
// used during save
//
void CvPlayerAI::write(FDataStreamBase* pStream)
{
	CvPlayer::write(pStream);	// write base class data first

/*
	uint uiFlag=0;
*/
	// Flag for type of save
	uint uiFlag=5;
	pStream->Write(uiFlag);		// flag for expansion

	pStream->Write(m_iPeaceWeight);
	pStream->Write(m_iEspionageWeight);
	pStream->Write(m_iAttackOddsChange);
	pStream->Write(m_iCivicTimer);
	pStream->Write(m_iReligionTimer);
	pStream->Write(m_iExtraGoldTarget);

	pStream->Write(m_iStrategyHash);
	//pStream->Write(m_iStrategyHashCacheTurn); // disabled by K-Mod
// BBAI
	pStream->Write(m_iStrategyRand);
	pStream->Write(m_iVictoryStrategyHash);
// BBAI end
	//pStream->Write(m_iAveragesCacheTurn);
	pStream->Write(m_iAverageGreatPeopleMultiplier);
	pStream->Write(m_iAverageCulturePressure); // K-Mod

	pStream->Write(NUM_YIELD_TYPES, m_aiAverageYieldMultiplier);
	pStream->Write(NUM_COMMERCE_TYPES, m_aiAverageCommerceMultiplier);
	pStream->Write(NUM_COMMERCE_TYPES, m_aiAverageCommerceExchange);

	//pStream->Write(m_iUpgradeUnitsCacheTurn); // disabled by K-Mod
	//pStream->Write(m_iUpgradeUnitsCachedExpThreshold); // disabled by K-Mod
	pStream->Write(m_iUpgradeUnitsCachedGold);

	pStream->Write(NUM_UNITAI_TYPES, m_aiNumTrainAIUnits);
	pStream->Write(NUM_UNITAI_TYPES, m_aiNumAIUnits);
	pStream->Write(MAX_PLAYERS, m_aiSameReligionCounter);
	pStream->Write(MAX_PLAYERS, m_aiDifferentReligionCounter);
	pStream->Write(MAX_PLAYERS, m_aiFavoriteCivicCounter);
	pStream->Write(MAX_PLAYERS, m_aiBonusTradeCounter);
	pStream->Write(MAX_PLAYERS, m_aiPeacetimeTradeValue);
	pStream->Write(MAX_PLAYERS, m_aiPeacetimeGrantValue);
	pStream->Write(MAX_PLAYERS, m_aiGoldTradedTo);
	pStream->Write(MAX_PLAYERS, m_aiAttitudeExtra);
	// K-Mod. save the attitude cache. (to avoid OOS problems)
	pStream->Write(MAX_PLAYERS, m_aiAttitudeCache); // uiFlag >= 4
	// K-Mod end
	pStream->Write(MAX_PLAYERS, m_abFirstContact);

	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		pStream->Write(NUM_CONTACT_TYPES, m_aaiContactTimer[i]);
	}
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		pStream->Write(NUM_MEMORY_TYPES, m_aaiMemoryCount[i]);
	}

	pStream->Write(m_bWasFinancialTrouble);
	pStream->Write(m_iTurnLastProductionDirty);
	
	{
		uint iSize = m_aiAICitySites.size();
		pStream->Write(iSize);
		std::vector<int>::iterator it;
		for (it = m_aiAICitySites.begin(); it != m_aiAICitySites.end(); ++it)
		{
			pStream->Write((*it));
		}
	}

	pStream->Write(GC.getNumBonusInfos(), m_aiBonusValue);
	pStream->Write(GC.getNumUnitClassInfos(), m_aiUnitClassWeights);
	pStream->Write(GC.getNumUnitCombatInfos(), m_aiUnitCombatWeights);
	// K-Mod. save great person weights. (uiFlag >= 5)
	{
		int iItems = m_GreatPersonWeights.size();
		pStream->Write(iItems);
		std::map<UnitClassTypes, int>::const_iterator it;
		for (it = m_GreatPersonWeights.begin(); it != m_GreatPersonWeights.end(); ++it)
		{
			pStream->Write((int)it->first);
			pStream->Write(it->second);
		}
	}
	// K-Mod end
	pStream->Write(MAX_PLAYERS, m_aiCloseBordersAttitudeCache);
}


int CvPlayerAI::AI_eventValue(EventTypes eEvent, const EventTriggeredData& kTriggeredData) const
{
	CvEventTriggerInfo& kTrigger = GC.getEventTriggerInfo(kTriggeredData.m_eTrigger);
	CvEventInfo& kEvent = GC.getEventInfo(eEvent);

	int iNumCities = getNumCities();
	CvCity* pCity = getCity(kTriggeredData.m_iCityId);
	CvPlot* pPlot = GC.getMapINLINE().plot(kTriggeredData.m_iPlotX, kTriggeredData.m_iPlotY);
	CvUnit* pUnit = getUnit(kTriggeredData.m_iUnitId);

	int iHappy = 0;
	int iHealth = 0;
	int aiYields[NUM_YIELD_TYPES];
	int aiCommerceYields[NUM_COMMERCE_TYPES];
	
	for (int iI = 0; iI < NUM_YIELD_TYPES; iI++)
	{
		aiYields[iI] = 0;
	}
	for (int iI = 0; iI < NUM_COMMERCE_TYPES; iI++)
	{
		aiCommerceYields[iI] = 0;
	}

	if (NO_PLAYER != kTriggeredData.m_eOtherPlayer)
	{
		if (kEvent.isDeclareWar())
		{
			switch (AI_getAttitude(kTriggeredData.m_eOtherPlayer))
			{
			case ATTITUDE_FURIOUS:
			case ATTITUDE_ANNOYED:
			case ATTITUDE_CAUTIOUS:
				if (GET_TEAM(getTeam()).getDefensivePower() < GET_TEAM(GET_PLAYER(kTriggeredData.m_eOtherPlayer).getTeam()).getPower(true))
				{
					return -MAX_INT + 1;
				}
				break;
			case ATTITUDE_PLEASED:
			case ATTITUDE_FRIENDLY:
				return -MAX_INT + 1;
				break;
			}
		}
	}

	//Proportional to #turns in the game...
	//(AI evaluation will generally assume proper game speed scaling!)
	int iGameSpeedPercent = GC.getGameSpeedInfo(GC.getGame().getGameSpeedType()).getVictoryDelayPercent();

	int iValue = GC.getGameINLINE().getSorenRandNum(kEvent.getAIValue(), "AI Event choice");
	iValue += (getEventCost(eEvent, kTriggeredData.m_eOtherPlayer, false) + getEventCost(eEvent, kTriggeredData.m_eOtherPlayer, true)) / 2;

	//iValue += kEvent.getEspionagePoints();
	iValue += kEvent.getEspionagePoints() * AI_commerceWeight(COMMERCE_ESPIONAGE) / 100; // K-Mod

	if (kEvent.getTech() != NO_TECH)
	{
		iValue += (GET_TEAM(getTeam()).getResearchCost((TechTypes)kEvent.getTech()) * kEvent.getTechPercent()) / 100;
	}

	if (kEvent.getUnitClass() != NO_UNITCLASS)
	{
		UnitTypes eUnit = (UnitTypes)GC.getCivilizationInfo(getCivilizationType()).getCivilizationUnits(kEvent.getUnitClass());
		if (eUnit != NO_UNIT)
		{
			//Altough AI_unitValue compares well within units, the value is somewhat independent of cost
			int iUnitValue = GC.getUnitInfo(eUnit).getProductionCost();
			if (iUnitValue > 0)
			{
				iUnitValue *= 2;
			}
			else if (iUnitValue == -1)
			{
				iUnitValue = 2000; //Great Person?  // (was 200)
			}

			iUnitValue *= GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).getTrainPercent();
			iUnitValue /= 100; // K-Mod
			iValue += kEvent.getNumUnits() * iUnitValue;
		}
	}
	
	if (kEvent.isDisbandUnit())
	{
		CvUnit* pUnit = getUnit(kTriggeredData.m_iUnitId);
		if (NULL != pUnit)
		{
			int iUnitValue = pUnit->getUnitInfo().getProductionCost();
			if (iUnitValue > 0)
			{
				iUnitValue *= 2;
			}
			else if (iUnitValue == -1)
			{
				iUnitValue = 2000; //Great Person?  // (was 200)
			}

			iUnitValue *= GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).getTrainPercent();
			iUnitValue /= 100; // K-Mod
			iValue -= iUnitValue;
		}
	}

	if (kEvent.getBuildingClass() != NO_BUILDINGCLASS)
	{
		BuildingTypes eBuilding = (BuildingTypes)GC.getCivilizationInfo(getCivilizationType()).getCivilizationBuildings(kEvent.getBuildingClass());
		if (eBuilding != NO_BUILDING)
		{
			if (pCity)
			{
				//iValue += kEvent.getBuildingChange() * pCity->AI_buildingValue(eBuilding);
				int iBuildingValue = GC.getBuildingInfo(eBuilding).getProductionCost();
				if (iBuildingValue > 0)
				{
					iBuildingValue *= 2;
				}
				else if (iBuildingValue == -1)
				{
					iBuildingValue = 300; //Shrine?
				}

				iBuildingValue *= GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).getConstructPercent();
				iValue += kEvent.getBuildingChange() * iBuildingValue;
			}
		}
	}

	TechTypes eBestTech = NO_TECH;
	int iBestValue = 0;
	for (int iTech = 0; iTech < GC.getNumTechInfos(); ++iTech)
	{
		if (canResearch((TechTypes)iTech))
		{
			if (NO_PLAYER == kTriggeredData.m_eOtherPlayer || GET_TEAM(GET_PLAYER(kTriggeredData.m_eOtherPlayer).getTeam()).isHasTech((TechTypes)iTech))
			{
				int iValue = 0;
				for (int i = 0; i < GC.getNumFlavorTypes(); ++i)
				{
					iValue += kEvent.getTechFlavorValue(i) * GC.getTechInfo((TechTypes)iTech).getFlavorValue(i);
				}

				if (iValue > iBestValue)
				{
					eBestTech = (TechTypes)iTech;
					iBestValue = iValue;
				}
			}
		}
	}

	if (eBestTech != NO_TECH)
	{
		iValue += (GET_TEAM(getTeam()).getResearchCost(eBestTech) * kEvent.getTechPercent()) / 100;
	}

	if (kEvent.isGoldenAge())
	{
		iValue += AI_calculateGoldenAgeValue();
	}

	{	//Yield and other changes
		if (kEvent.getNumBuildingYieldChanges() > 0)
		{
			for (int iBuildingClass = 0; iBuildingClass < GC.getNumBuildingClassInfos(); ++iBuildingClass)
			{
				for (int iYield = 0; iYield < NUM_YIELD_TYPES; ++iYield)
				{
					aiYields[iYield] += kEvent.getBuildingYieldChange(iBuildingClass, iYield);
				}
			}
		}

		if (kEvent.getNumBuildingCommerceChanges() > 0)
		{
			for (int iBuildingClass = 0; iBuildingClass < GC.getNumBuildingClassInfos(); ++iBuildingClass)
			{
				for (int iCommerce = 0; iCommerce < NUM_COMMERCE_TYPES; ++iCommerce)
				{
					aiCommerceYields[iCommerce] += kEvent.getBuildingCommerceChange(iBuildingClass, iCommerce);
				}
			}
		}

		if (kEvent.getNumBuildingHappyChanges() > 0)
		{
			for (int iBuildingClass = 0; iBuildingClass < GC.getNumBuildingClassInfos(); ++iBuildingClass)
			{
				iHappy += kEvent.getBuildingHappyChange(iBuildingClass);
			}
		}

		if (kEvent.getNumBuildingHealthChanges() > 0)
		{
			for (int iBuildingClass = 0; iBuildingClass < GC.getNumBuildingClassInfos(); ++iBuildingClass)
			{
				iHealth += kEvent.getBuildingHealthChange(iBuildingClass);
			}
		}
	}
	
	if (kEvent.isCityEffect())
	{
		int iCityPopulation = -1;
		int iCityTurnValue = 0;
		if (NULL != pCity)
		{
			iCityPopulation = pCity->getPopulation();
			for (int iSpecialist = 0; iSpecialist < GC.getNumSpecialistInfos(); ++iSpecialist)
			{
				if (kEvent.getFreeSpecialistCount(iSpecialist) > 0)
				{
					//iCityTurnValue += (pCity->AI_specialistValue((SpecialistTypes)iSpecialist, false, false) / 300);
					iCityTurnValue += pCity->AI_permanentSpecialistValue((SpecialistTypes)iSpecialist) / 100; // K-Mod
				}
			}
		}

		if (-1 == iCityPopulation)
		{
			//What is going on here?
			iCityPopulation = 5;
		}

		iCityTurnValue += ((iHappy + kEvent.getHappy()) * 8);
		iCityTurnValue += ((iHealth + kEvent.getHealth()) * 6);
		
		iCityTurnValue += aiYields[YIELD_FOOD] * 5;
		iCityTurnValue += aiYields[YIELD_PRODUCTION] * 5;
		iCityTurnValue += aiYields[YIELD_COMMERCE] * 3;
		
		iCityTurnValue += aiCommerceYields[COMMERCE_RESEARCH] * 3;
		iCityTurnValue += aiCommerceYields[COMMERCE_GOLD] * 3;
		iCityTurnValue += aiCommerceYields[COMMERCE_CULTURE] * 2; // was 1
		iCityTurnValue += aiCommerceYields[COMMERCE_ESPIONAGE] * 2;

		iValue += (iCityTurnValue * 20 * iGameSpeedPercent) / 100;

		iValue += kEvent.getFood();
		iValue += kEvent.getFoodPercent() / 4;
		iValue += kEvent.getPopulationChange() * 30;
		iValue -= kEvent.getRevoltTurns() * (12 + iCityPopulation * 16);
		iValue -= (kEvent.getHurryAnger() * 6 * GC.getDefineINT("HURRY_ANGER_DIVISOR") * GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).getHurryConscriptAngerPercent()) / 100;
		iValue += kEvent.getHappyTurns() * 10;
		iValue += kEvent.getCulture() / 2;
	}
	else if (!kEvent.isOtherPlayerCityEffect())
	{
		int iPerTurnValue = 0;
		iPerTurnValue += iNumCities * ((iHappy * 4) + (kEvent.getHappy() * 8));
		iPerTurnValue += iNumCities * ((iHealth * 3) + (kEvent.getHealth() * 6));
		
		iValue += (iPerTurnValue * 20 * iGameSpeedPercent) / 100;
		
		iValue += (kEvent.getFood() * iNumCities);
		iValue += (kEvent.getFoodPercent() * iNumCities) / 4;
		iValue += (kEvent.getPopulationChange() * iNumCities * 40);
		iValue -= (iNumCities * kEvent.getHurryAnger() * 6 * GC.getDefineINT("HURRY_ANGER_DIVISOR") * GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).getHurryConscriptAngerPercent()) / 100;
		iValue += iNumCities * kEvent.getHappyTurns() * 10;
		iValue += iNumCities * kEvent.getCulture() / 2;
	}
	
	int iBonusValue = 0;
	if (NO_BONUS != kEvent.getBonus())
	{
		//iBonusValue = AI_bonusVal((BonusTypes)kEvent.getBonus());
		iBonusValue = AI_bonusVal((BonusTypes)kEvent.getBonus(), 0); // K-Mod
	}

	if (NULL != pPlot)
	{
		if (kEvent.getFeatureChange() != 0)
		{
			int iOldFeatureValue = 0;
			int iNewFeatureValue = 0;
			if (pPlot->getFeatureType() != NO_FEATURE)
			{
				//*grumble* who tied feature production to builds rather than the feature...
				iOldFeatureValue = GC.getFeatureInfo(pPlot->getFeatureType()).getHealthPercent();

				if (kEvent.getFeatureChange() > 0)
				{
					FeatureTypes eFeature = (FeatureTypes)kEvent.getFeature();
					FAssert(eFeature != NO_FEATURE);
					if (eFeature != NO_FEATURE)
					{
						iNewFeatureValue = GC.getFeatureInfo(eFeature).getHealthPercent();
					}
				}

				iValue += ((iNewFeatureValue - iOldFeatureValue) * iGameSpeedPercent) / 100;
			}
		}

		if (kEvent.getImprovementChange() > 0)
		{
			iValue += (30 * iGameSpeedPercent) / 100;
		}
		else if (kEvent.getImprovementChange() < 0)
		{
			iValue -= (30 * iGameSpeedPercent) / 100;
		}

		if (kEvent.getRouteChange() > 0)
		{
			iValue += (10 * iGameSpeedPercent) / 100;
		}
		else if (kEvent.getRouteChange() < 0)
		{
			iValue -= (10 * iGameSpeedPercent) / 100;
		}

		if (kEvent.getBonusChange() > 0)
		{
			iValue += (iBonusValue * 15 * iGameSpeedPercent) / 100;
		}
		else if (kEvent.getBonusChange() < 0)
		{
			iValue -= (iBonusValue * 15 * iGameSpeedPercent) / 100;
		}
		
		for (int i = 0; i < NUM_YIELD_TYPES; ++i)
		{
			if (0 != kEvent.getPlotExtraYield(i))
			{
				if (pPlot->getWorkingCity() != NULL)
				{
					FAssertMsg(pPlot->getWorkingCity()->getOwner() == getID(), "Event creates a boni for another player?");
					aiYields[i] += kEvent.getPlotExtraYield(i);
				}
				else
				{
					iValue += (20 * 8 * kEvent.getPlotExtraYield(i) * iGameSpeedPercent) / 100;
				}
			}
		}
	}

	if (NO_BONUS != kEvent.getBonusRevealed())
	{
		iValue += (iBonusValue * 10 * iGameSpeedPercent) / 100;
	}

	if (NULL != pUnit)
	{
		iValue += (2 * pUnit->baseCombatStr() * kEvent.getUnitExperience() * GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).getTrainPercent()) / 100;

		iValue -= 10 * kEvent.getUnitImmobileTurns();
	}

	{
		int iPromotionValue = 0;
		
		for (int i = 0; i < GC.getNumUnitCombatInfos(); ++i)
		{
			if (NO_PROMOTION != kEvent.getUnitCombatPromotion(i))
			{
				int iLoop;
				for (CvUnit* pLoopUnit = firstUnit(&iLoop); NULL != pLoopUnit; pLoopUnit = nextUnit(&iLoop))
				{
					if (pLoopUnit->getUnitCombatType() == i)
					{
						if (!pLoopUnit->isHasPromotion((PromotionTypes)kEvent.getUnitCombatPromotion(i)))
						{
							iPromotionValue += 5 * pLoopUnit->baseCombatStr();
						}
					}
				}

				iPromotionValue += iNumCities * 50;
			}
		}
		
		iValue += (iPromotionValue * iGameSpeedPercent) / 100;
	}

	if (kEvent.getFreeUnitSupport() != 0)
	{
		iValue += (20 * kEvent.getFreeUnitSupport() * iGameSpeedPercent) / 100;
	}
	
	if (kEvent.getInflationModifier() != 0)
	{
		iValue -= (20 * kEvent.getInflationModifier() * calculatePreInflatedCosts() * iGameSpeedPercent) / 100;		
	}

	if (kEvent.getSpaceProductionModifier() != 0)
	{
		iValue += ((20 + iNumCities) * getSpaceProductionModifier() * iGameSpeedPercent) / 100;
	}

	int iOtherPlayerAttitudeWeight = 0;
	if (kTriggeredData.m_eOtherPlayer != NO_PLAYER)
	{
		iOtherPlayerAttitudeWeight = AI_getAttitudeWeight(kTriggeredData.m_eOtherPlayer);
		iOtherPlayerAttitudeWeight += 10 - GC.getGame().getSorenRandNum(20, "AI event value attitude");
	}

	//Religion
	if (kTriggeredData.m_eReligion != NO_RELIGION)
	{
		ReligionTypes eReligion = kTriggeredData.m_eReligion;
		
		int iReligionValue = 15;
		
		if (getStateReligion() == eReligion)
		{
			iReligionValue += 15;
		}
		if (hasHolyCity(eReligion))
		{
			iReligionValue += 15;
		}
	
		iValue += (kEvent.getConvertOwnCities() * iReligionValue * iGameSpeedPercent) / 100;

		if (kEvent.getConvertOtherCities() > 0)
		{
			//Don't like them much = fairly indifferent, hate them = negative.
			iValue += (kEvent.getConvertOtherCities() * (iOtherPlayerAttitudeWeight + 50) * iReligionValue * iGameSpeedPercent) / 15000;
		}
	}

	if (NO_PLAYER != kTriggeredData.m_eOtherPlayer)
	{
		CvPlayerAI& kOtherPlayer = GET_PLAYER(kTriggeredData.m_eOtherPlayer);

		int iDiploValue = 0;
		//if we like this player then value positive attitude, if however we really hate them then
		//actually value negative attitude.
		iDiploValue += ((iOtherPlayerAttitudeWeight + 50) * kEvent.getAttitudeModifier() * GET_PLAYER(kTriggeredData.m_eOtherPlayer).getPower()) / std::max(1, getPower());
		
		if (kEvent.getTheirEnemyAttitudeModifier() != 0)
		{
			//Oh wow this sure is mildly complicated.
			TeamTypes eWorstEnemy = GET_TEAM(GET_PLAYER(kTriggeredData.m_eOtherPlayer).getTeam()).AI_getWorstEnemy();
			
			if (NO_TEAM != eWorstEnemy && eWorstEnemy != getTeam())
			{
			int iThirdPartyAttitudeWeight = GET_TEAM(getTeam()).AI_getAttitudeWeight(eWorstEnemy);
			
			//If we like both teams, we want them to get along.
			//If we like otherPlayer but not enemy (or vice-verca), we don't want them to get along.
			//If we don't like either, we don't want them to get along.
			//Also just value stirring up trouble in general.
			
			int iThirdPartyDiploValue = 50 * kEvent.getTheirEnemyAttitudeModifier();
			iThirdPartyDiploValue *= (iThirdPartyAttitudeWeight - 10);
			iThirdPartyDiploValue *= (iOtherPlayerAttitudeWeight - 10);
			iThirdPartyDiploValue /= 10000;
			
			if ((iOtherPlayerAttitudeWeight) < 0 && (iThirdPartyAttitudeWeight < 0))
			{
				iThirdPartyDiploValue *= -1;
			}
			
			iThirdPartyDiploValue /= 2;
			
			iDiploValue += iThirdPartyDiploValue;
		}
		}
		
		iDiploValue *= iGameSpeedPercent;
		iDiploValue /= 100;
		
		if (NO_BONUS != kEvent.getBonusGift())
		{
			int iBonusValue = -AI_bonusVal((BonusTypes)kEvent.getBonusGift(), -1);
			iBonusValue += (iOtherPlayerAttitudeWeight - 40) * kOtherPlayer.AI_bonusVal((BonusTypes)kEvent.getBonusGift(), +1);
			//Positive for friends, negative for enemies.
			iDiploValue += (iBonusValue * GC.getDefineINT("PEACE_TREATY_LENGTH")) / 60;
		}
		
		if (GC.getGameINLINE().isOption(GAMEOPTION_AGGRESSIVE_AI))
		{
			//What is this "relationships" thing?
			iDiploValue /= 2;
		}
		
		if (kEvent.isGoldToPlayer())
		{
			//If the gold goes to another player instead of the void, then this is a positive
			//thing if we like the player, otherwise it's a negative thing.
			int iGiftValue = (getEventCost(eEvent, kTriggeredData.m_eOtherPlayer, false) + getEventCost(eEvent, kTriggeredData.m_eOtherPlayer, true)) / 2;
			iGiftValue *= -iOtherPlayerAttitudeWeight;
			iGiftValue /= 110;
			
			iValue += iGiftValue;
		}

		if (kEvent.isDeclareWar())
		{
			/* original bts code
			int iWarValue = GET_TEAM(getTeam()).getDefensivePower(GET_PLAYER(kTriggeredData.m_eOtherPlayer).getTeam())
				- GET_TEAM(GET_PLAYER(kTriggeredData.m_eOtherPlayer).getTeam()).getPower(true);// / std::max(1, GET_TEAM(getTeam()).getDefensivePower());
			iWarValue -= 30 * AI_getAttitudeVal(kTriggeredData.m_eOtherPlayer); */

			// K-Mod. Note: the original code doesn't touch iValue.
			// So whatever I do here is completely new.
			// TODO: if I ever get around to writing code for evalutating potential war targets, I should use that here!
			int iOurPower = GET_TEAM(getTeam()).getDefensivePower(GET_PLAYER(kTriggeredData.m_eOtherPlayer).getTeam());
			int iTheirPower = GET_TEAM(GET_PLAYER(kTriggeredData.m_eOtherPlayer).getTeam()).getPower(true);
			int iWarValue = 300 * (iOurPower - iTheirPower) / std::max(1, iOurPower + iTheirPower) - 25;// / std::max(1, GET_TEAM(getTeam()).getDefensivePower())

			iValue += iWarValue;
			// K-Mod end
		}
			
		if (kEvent.getMaxPillage() > 0)
		{
			int iPillageValue = (40 * (kEvent.getMinPillage() + kEvent.getMaxPillage())) / 2;
			//If we hate them, this is good to do.
			iPillageValue *= 25 - iOtherPlayerAttitudeWeight;
			iPillageValue *= iGameSpeedPercent;
			iPillageValue /= 12500;

			iValue += iPillageValue; // K-Mod!
		}

		iValue += (iDiploValue * iGameSpeedPercent) / 100;
	}

	int iThisEventValue = iValue;
	//XXX THIS IS VULNERABLE TO NON-TRIVIAL RECURSIONS!
	//Event A effects Event B, Event B effects Event A
	for (int iEvent = 0; iEvent < GC.getNumEventInfos(); ++iEvent)
	{
		if (kEvent.getAdditionalEventChance(iEvent) > 0)
		{
			if (iEvent == eEvent)
			{
				//Infinite recursion is not our friend.
				//Fortunately we have the event value for this event - sans values of other events
				//disabled or cleared. Hopefully no events will be that complicated...
				//Double the value since it's recursive.
				iValue += (kEvent.getAdditionalEventChance(iEvent) * iThisEventValue) / 50;
			}
			else
			{
				iValue += (kEvent.getAdditionalEventChance(iEvent) * AI_eventValue((EventTypes)iEvent, kTriggeredData)) / 100;
			}
		}
	
		if (kEvent.getClearEventChance(iEvent) > 0)
		{
			if (iEvent == eEvent)
			{
				iValue -= (kEvent.getClearEventChance(iEvent) * iThisEventValue) / 50;
			}
			else
			{
				iValue -= (kEvent.getClearEventChance(iEvent) * AI_eventValue((EventTypes)iEvent, kTriggeredData)) / 100;
			}
		}
	}

	iValue *= 100 + GC.getGameINLINE().getSorenRandNum(20, "AI Event choice");
	iValue /= 100;

	return iValue;
}

EventTypes CvPlayerAI::AI_chooseEvent(int iTriggeredId) const
{
	EventTriggeredData* pTriggeredData = getEventTriggered(iTriggeredId);
	if (NULL == pTriggeredData)
	{
		return NO_EVENT;
	}

	CvEventTriggerInfo& kTrigger = GC.getEventTriggerInfo(pTriggeredData->m_eTrigger);

	int iBestValue = -MAX_INT;
	EventTypes eBestEvent = NO_EVENT;

	for (int i = 0; i < kTrigger.getNumEvents(); i++)
	{
		int iValue = -MAX_INT;
		if (kTrigger.getEvent(i) != NO_EVENT)
		{
			CvEventInfo& kEvent = GC.getEventInfo((EventTypes)kTrigger.getEvent(i));
			if (canDoEvent((EventTypes)kTrigger.getEvent(i), *pTriggeredData))
			{
				iValue = AI_eventValue((EventTypes)kTrigger.getEvent(i), *pTriggeredData);
			}
		}

		if (iValue > iBestValue)
		{
			iBestValue = iValue;
			eBestEvent = (EventTypes)kTrigger.getEvent(i);
		}
	}

	return eBestEvent;
}

void CvPlayerAI::AI_doSplit()
{
	PROFILE_FUNC();

	if (!canSplitEmpire())
	{
		return;
	}

	int iLoop;
	std::map<int, int> mapAreaValues;

	for (CvArea* pLoopArea = GC.getMapINLINE().firstArea(&iLoop); pLoopArea != NULL; pLoopArea = GC.getMapINLINE().nextArea(&iLoop))
	{
		mapAreaValues[pLoopArea->getID()] = 0;
	}

	for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		mapAreaValues[pLoopCity->area()->getID()] += pLoopCity->AI_cityValue();
	}

	std::map<int, int>::iterator it;
	for (it = mapAreaValues.begin(); it != mapAreaValues.end(); ++it)
	{
		if (it->second < 0)
		{
			int iAreaId = it->first;

			if (canSplitArea(iAreaId))
			{
				splitEmpire(iAreaId);

				for (CvUnit* pUnit = firstUnit(&iLoop); pUnit != NULL; pUnit = nextUnit(&iLoop))
				{
					if (pUnit->area()->getID() == iAreaId)
					{
						TeamTypes ePlotTeam = pUnit->plot()->getTeam();

						if (NO_TEAM != ePlotTeam)
						{
							CvTeam& kPlotTeam = GET_TEAM(ePlotTeam);
							if (kPlotTeam.isVassal(getTeam()) && GET_TEAM(getTeam()).isParent(ePlotTeam))
							{
								pUnit->gift();
							}
						}
					}
				}
				break;
			}
		}
	}
}

void CvPlayerAI::AI_launch(VictoryTypes eVictory)
{
	if (GET_TEAM(getTeam()).isHuman())
	{
		return;
	}

	if (!GET_TEAM(getTeam()).canLaunch(eVictory))
	{
		return;
	}

	bool bLaunch = true;

	int iBestArrival = MAX_INT;
	TeamTypes eBestTeam = NO_TEAM;

	for (int iTeam = 0; iTeam < MAX_CIV_TEAMS; ++iTeam)
	{
		if (iTeam != getTeam())
		{
			CvTeam& kTeam = GET_TEAM((TeamTypes)iTeam);
			if (kTeam.isAlive())
			{
				int iCountdown = kTeam.getVictoryCountdown(eVictory);
				if (iCountdown > 0)
				{
					if (iCountdown < iBestArrival)
					{
						iBestArrival = iCountdown;
						eBestTeam = (TeamTypes)iTeam;
					}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      12/07/09                       r_rolo1 & jdog5000     */
/*                                                                                              */
/*                                                                                              */
/************************************************************************************************/
/* original BTS code
					if (iCountdown < GET_TEAM(getTeam()).getVictoryDelay(eVictory) && kTeam.getLaunchSuccessRate(eVictory) == 100)
					{
						bLaunch = false;
						break;
					}
*/
					// Other civs capital could still be captured, might as well send spaceship
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
				}
			}
		}
	}

	if (bLaunch)
	{
		if (NO_TEAM == eBestTeam || iBestArrival > GET_TEAM(getTeam()).getVictoryDelay(eVictory))
		{
			if (GET_TEAM(getTeam()).getLaunchSuccessRate(eVictory) < 100)
			{
				bLaunch = false;
			}
		}
	}

	if (bLaunch)
	{
		launch(eVictory);
	}
}

void CvPlayerAI::AI_doCheckFinancialTrouble()
{
	// if we just ran into financial trouble this turn
	bool bFinancialTrouble = AI_isFinancialTrouble();
	if (bFinancialTrouble != m_bWasFinancialTrouble)
	{
		if (bFinancialTrouble)
		{
			int iGameTurn = GC.getGameINLINE().getGameTurn();
			
			// only reset at most every 10 turns
			if (iGameTurn > m_iTurnLastProductionDirty + 10)
			{
				// redeterimine the best things to build in each city
				AI_makeProductionDirty();
			
				m_iTurnLastProductionDirty = iGameTurn;
			}
		}
		
		m_bWasFinancialTrouble = bFinancialTrouble;
	}
}

// bool CvPlayerAI::AI_disbandUnit(int iExpThreshold, bool bObsolete)
// K-Mod, deleted this function.


/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      04/25/10                                jdog5000      */
/*                                                                                              */
/* Victory Strategy AI                                                                          */
/************************************************************************************************/
int CvPlayerAI::AI_getCultureVictoryStage() const
{
    int iValue;

	if( GC.getDefineINT("BBAI_VICTORY_STRATEGY_CULTURE") <= 0 )
	{
		return 0;
	}
	
    if (!GC.getGameINLINE().culturalVictoryValid())
    {
        return 0;
    }

	// Necessary as capital city pointer is used later
    if (getCapitalCity() == NULL)
    {
        return 0;
    }

	int iHighCultureCount = 0;
	int iCloseToLegendaryCount = 0;
	int iLegendaryCount = 0;
	// K-Mod
	int iLegendaryCulture = GC.getGame().getCultureThreshold((CultureLevelTypes)(GC.getNumCultureLevelInfos() - 1));
	int iVictoryCities = GC.getGameINLINE().culturalVictoryNumCultureCities();

	int iHighCultureMark = 300; // turns
	iHighCultureMark *= (3*GC.getNumEraInfos() - 2*getCurrentEra());
	iHighCultureMark /= std::max(1, 3*GC.getNumEraInfos());
	iHighCultureMark *= GC.getGameSpeedInfo(GC.getGame().getGameSpeedType()).getVictoryDelayPercent();
	iHighCultureMark /= 100;

	std::vector<int> countdownList;
	// K-Mod end

	int iLoop;
	CvCity* pLoopCity;
	for (pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		if (pLoopCity->getCultureLevel() >= (GC.getGameINLINE().culturalVictoryCultureLevel() - 1))
		{
			//if (pLoopCity->getBaseCommerceRate(COMMERCE_CULTURE) > 100)
			// K-Mod. Try to tell us what the culture would be like if we were to turn up the slider...
			int iEstimatedRate = pLoopCity->getCommerceRate(COMMERCE_CULTURE);
			iEstimatedRate -= getCommercePercent(COMMERCE_CULTURE) * pLoopCity->getYieldRate(YIELD_COMMERCE) * pLoopCity->getTotalCommerceRateModifier(COMMERCE_CULTURE) / 10000;
			iEstimatedRate += (100 - getCommercePercent(COMMERCE_GOLD)) * pLoopCity->getYieldRate(YIELD_COMMERCE) * pLoopCity->getTotalCommerceRateModifier(COMMERCE_CULTURE) / 10000;
			int iCountdown = (iLegendaryCulture - pLoopCity->getCulture(getID())) / std::max(1, iEstimatedRate);
			countdownList.push_back(iCountdown);
			if (iCountdown < iHighCultureMark)
			{
				iHighCultureCount++;
			}

			// is over 1/2 of the way there?
			if( 2*pLoopCity->getCulture(getID()) > pLoopCity->getCultureThreshold(GC.getGameINLINE().culturalVictoryCultureLevel()) )
			{
				iCloseToLegendaryCount++;
			}

			// is already there?
			if( pLoopCity->getCulture(getID()) > pLoopCity->getCultureThreshold(GC.getGameINLINE().culturalVictoryCultureLevel()) )
			{
				iLegendaryCount++;
			}
		}
	}

	if( iLegendaryCount >= iVictoryCities )
	{
		// Already won, keep playing culture heavy but do some tech to keep pace if human wants to keep playing
		return 3;
	}

	if( iCloseToLegendaryCount >= iVictoryCities )
	{
		return 4;
	}
	else if( isHuman() )
	{
		if( getCommercePercent(COMMERCE_CULTURE) > 50 )
		{
			return 3;
		}
	}

	if( isHuman() && !(GC.getGameINLINE().isDebugMode()) )
	{
		return 0;
	}

	// K-Mod, disabling some stuff.
	// It is still possible to get a cultural victory in advanced start games.
	// and moving your captial city certainly does not indicate that you shouldn't go for a cultural victory!
	// ... and colonies don't get their capital on turn 1 anyway.
	/* original code
	if (GC.getGame().getStartEra() > 1)
    {
    	return 0;
    }

	if (getCapitalCity()->getGameTurnFounded() > (10 + GC.getGameINLINE().getStartTurn()))
    {
		if( iHighCultureCount < GC.getGameINLINE().culturalVictoryNumCultureCities() )
		{
			//the loss of the capital indicates it might be a good idea to abort any culture victory
			return 0;
		}
    } */

    iValue = GC.getLeaderHeadInfo(getPersonalityType()).getCultureVictoryWeight();

	if (GC.getGameINLINE().isOption(GAMEOPTION_ALWAYS_PEACE))
	{
    	iValue += 30;
	}
	
	iValue += (GC.getGameINLINE().isOption(GAMEOPTION_AGGRESSIVE_AI) ? -20 : 0);
	
	// K-Mod
	iValue += 30 * std::min(iCloseToLegendaryCount, iVictoryCities + 1);
	iValue += 10 * std::min(iHighCultureCount, iVictoryCities + 1);
	// K-Mod end
	if( iValue > 20 && getNumCities() >= iVictoryCities )
	{
		iValue += 5*countHolyCities(); // was 10*
		// K-Mod: be wary of going for a cultural victory if everyone hates us.
		int iScore = 0;
		int iTotalPop = 0;
		for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
		{
			const CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iI);
			if (iI != getID() && kLoopPlayer.isAlive() && GET_TEAM(getTeam()).isHasMet(kLoopPlayer.getTeam())
				&& !kLoopPlayer.isMinorCiv() && !GET_TEAM(kLoopPlayer.getTeam()).isAVassal())
			{
				iTotalPop += kLoopPlayer.getTotalPopulation();
				if (AI_getAttitude((PlayerTypes)iI) <= ATTITUDE_ANNOYED)
					iScore -= 20 * kLoopPlayer.getTotalPopulation();
				else if (AI_getAttitude((PlayerTypes)iI) >= ATTITUDE_PLEASED)
					iScore += 10 * kLoopPlayer.getTotalPopulation();
			}
		}
		iValue += iScore / std::max(1, iTotalPop);
		iValue -= AI_isDoVictoryStrategy(AI_VICTORY_CONQUEST1) ? 20 : 0;
		iValue -= AI_isDoVictoryStrategy(AI_VICTORY_SPACE2) ? 10 : 0;
		iValue -= AI_isDoVictoryStrategy(AI_VICTORY_SPACE3) ? 20 : 0;
		// K-Mod end
	}
	/*
	if ((GET_TEAM(getTeam()).isAVassal()) && (getNumCities() > 5))
	{
		int iReligionCount = countTotalHasReligion();
		if (((iReligionCount * 100) / getNumCities()) > 250)
		{
			iValue += 1;
			iValue += ((2 * iReligionCount) + 1) / getNumCities();
		}
	}
	*/

	//int iNonsense = AI_getStrategyRand() + 10;
	iValue += (AI_getStrategyRand(0) % 100);

	if (iValue < 100)
	{
		return 0;
	}

	int iWinningCountdown = INT_MAX;
	if ((int)countdownList.size() >= iVictoryCities && iVictoryCities > 0)
	{
		std::partial_sort(countdownList.begin(), countdownList.begin() + iVictoryCities, countdownList.end());
		iWinningCountdown = countdownList[iVictoryCities-1];
	}
    if (getCurrentEra() >= (GC.getNumEraInfos() - (2 + AI_getStrategyRand(1) % 2)))
    {
		bool bAt3 = false;
        
		// if we have enough high culture cities, go to stage 3
		if (iHighCultureCount >= iVictoryCities)
		{
			bAt3 = true;
		}

		// if we have a lot of religion, may be able to catch up quickly
		/* disabled by K-Mod
		if (countTotalHasReligion() >= getNumCities() * 3)
        {
			if( getNumCities() >= GC.getGameINLINE().culturalVictoryNumCultureCities() )
			{
				bAt3 = true;
			}
        } */

		if( bAt3 )
		{
			/* original code
			if (AI_cultureVictoryTechValue(getCurrentResearch()) < 100)
			{
				return 4;
			}*/
			// K-Mod
			int iCountdownTarget = 180;
			{
				int iDemoninator = 100;
				iDemoninator += 50 * AI_isDoVictoryStrategy(AI_VICTORY_CONQUEST2);
				iDemoninator += 50 * AI_isDoVictoryStrategy(AI_VICTORY_CONQUEST3);
				iDemoninator += 80 * AI_isDoVictoryStrategy(AI_VICTORY_DOMINATION3);
				iDemoninator += 70 * AI_isDoVictoryStrategy(AI_VICTORY_SPACE2);
				iDemoninator += 70 * AI_isDoVictoryStrategy(AI_VICTORY_SPACE3);
				iDemoninator += AI_cultureVictoryTechValue(getCurrentResearch()) > 100 ? 80 : 0;
				iDemoninator += AI_isDoStrategy(AI_STRATEGY_GET_BETTER_UNITS)? 80 : 0;
				iDemoninator += GET_TEAM(getTeam()).getAnyWarPlanCount(true)>0 ? 30 : 0;
				int iWarSuccessRating = GET_TEAM(getTeam()).AI_getWarSuccessRating();
				iDemoninator += iWarSuccessRating < 0 ? -2*iWarSuccessRating : 0;
				// and a little bit of personal variation.
				iDemoninator += 50 - GC.getLeaderHeadInfo(getPersonalityType()).getCultureVictoryWeight();

				iCountdownTarget *= GC.getGameSpeedInfo(GC.getGame().getGameSpeedType()).getVictoryDelayPercent();
				iCountdownTarget /= iDemoninator;
			}
			// Note: culture 4 is blocked completely by 4 in any of those other strategies.
			if (iWinningCountdown < iCountdownTarget)
			{
				return 4;
			}
			// K-Mod end

			return 3;
		}
    }

	//if (getCurrentEra() >= ((GC.getNumEraInfos() / 3) + iNonsense % 2))
	if (getCurrentEra() >= ((GC.getNumEraInfos() / 3) + AI_getStrategyRand(1) % 2) || iHighCultureCount >= iVictoryCities-1)
	{
		if (iHighCultureCount < getCurrentEra() + iVictoryCities - GC.getNumEraInfos())
			return 1;
	    return 2;
	}
        
	return 1;
}

int CvPlayerAI::AI_getSpaceVictoryStage() const
{
    int iValue;

	if( GC.getDefineINT("BBAI_VICTORY_STRATEGY_SPACE") <= 0 )
	{
		return 0;
	}

	if (getCapitalCity() == NULL)
    {
        return 0;
    }
	
	// Better way to determine if spaceship victory is valid?
	VictoryTypes eSpace = NO_VICTORY;
	for (int iI = 0; iI < GC.getNumVictoryInfos(); iI++)
	{
		if (GC.getGameINLINE().isVictoryValid((VictoryTypes) iI))
		{
			CvVictoryInfo& kVictoryInfo = GC.getVictoryInfo((VictoryTypes) iI);
			if( kVictoryInfo.getVictoryDelayTurns() > 0 )
			{
				eSpace = (VictoryTypes)iI;
				break;
			}
		}
	}

	if( eSpace == NO_VICTORY )
	{
		return 0;
	}

	// If have built Apollo, then the race is on!
	bool bHasApollo = false;
	bool bNearAllTechs = true;
	for( int iI = 0; iI < GC.getNumProjectInfos(); iI++ )
	{
		if( GC.getProjectInfo((ProjectTypes)iI).getVictoryPrereq() == eSpace )
		{
			if( GET_TEAM(getTeam()).getProjectCount((ProjectTypes)iI) > 0 )
			{
				bHasApollo = true;
			}
			else
			{
				if( !GET_TEAM(getTeam()).isHasTech((TechTypes)GC.getProjectInfo((ProjectTypes)iI).getTechPrereq()) )
				{
					if( !isResearchingTech((TechTypes)GC.getProjectInfo((ProjectTypes)iI).getTechPrereq()) )
					{
						bNearAllTechs = false;
					}
				}
			}
		}
	}

	if( bHasApollo )
	{
		if( bNearAllTechs )
		{
			bool bOtherLaunched = false;
			// K-Mod. (just tidying up a bit.)
			int iOurCountdown = GET_TEAM(getTeam()).getVictoryCountdown(eSpace);
			for (int iTeam = 0; iTeam < MAX_CIV_TEAMS; iTeam++)
			{
				if (iTeam == getTeam())
					continue;

				if (GET_TEAM((TeamTypes)iTeam).getVictoryCountdown(eSpace) >= 0 &&
					(iOurCountdown < 0 || GET_TEAM((TeamTypes)iTeam).getVictoryCountdown(eSpace) <= iOurCountdown))
				{
					bOtherLaunched = true;
					break;
				}
			}
			// K-Mod end

			if( !bOtherLaunched )
			{
				return 4;
			}

			return 3;
		}

		/* original bbai code
		if( GET_TEAM(getTeam()).getBestKnownTechScorePercent() > (m_iVictoryStrategyHash & AI_VICTORY_SPACE3 ? 80 : 85) )
		{
			return 3;
		} */
		// K-Mod. I don't think that's a good way to do it.
		// instead, compare our spaceship progress to that of our rivals.
		int iOurProgress = 0;
		for (int i = 0; i < GC.getNumProjectInfos(); i++)
		{
			if (GC.getProjectInfo((ProjectTypes)i).isSpaceship())
			{
				int iBuilt = GET_TEAM(getTeam()).getProjectCount((ProjectTypes)i);

				if (iBuilt > 0 || GET_TEAM(getTeam()).isHasTech((TechTypes)(GC.getProjectInfo((ProjectTypes)i).getTechPrereq())))
					iOurProgress += 2 + iBuilt;
			}
		}
		int iKnownTeams = 0;
		int iSpaceTeams = 0; // teams ahead of us in the space race
		for (int iTeam = 0; iTeam < MAX_CIV_TEAMS; iTeam++)
		{
			const CvTeam& kLoopTeam = GET_TEAM((TeamTypes)iTeam);
			if (iTeam != getTeam() && kLoopTeam.isAlive() && kLoopTeam.isHasMet(getTeam()))
			{
				int iProgress = 0;
				for (int i = 0; i < GC.getNumProjectInfos(); i++)
				{
					if (GC.getProjectInfo((ProjectTypes)i).isSpaceship())
					{
						int iBuilt = kLoopTeam.getProjectCount((ProjectTypes)i);

						if (iBuilt > 0 || kLoopTeam.isHasTech((TechTypes)(GC.getProjectInfo((ProjectTypes)i).getTechPrereq())))
							iProgress += 2 + iBuilt;
					}
				}
				iKnownTeams++;
				if (iProgress > iOurProgress)
					iSpaceTeams++;
			}
		}
		if (200 * iSpaceTeams / (1+iKnownTeams)
			<= GC.getLeaderHeadInfo(getPersonalityType()).getSpaceVictoryWeight() + AI_getStrategyRand(3) % 100) // note, correlated with number used lower down.
			return 3;
		// K-Mod end
	}

	if( isHuman() && !(GC.getGameINLINE().isDebugMode()) )
	{
		return 0;
	}

	// If can't build Apollo yet, then consider making player push for this victory type
	{
		iValue = GC.getLeaderHeadInfo(getPersonalityType()).getSpaceVictoryWeight();

		if (GC.getGameINLINE().isOption(GAMEOPTION_ALWAYS_PEACE))
		{
    		iValue += 30;
		}

		if(GET_TEAM(getTeam()).isAVassal())
		{
			iValue += 20;
		}

		iValue += (GC.getGameINLINE().isOption(GAMEOPTION_AGGRESSIVE_AI) ? -20 : 0);

		// K-Mod: take into account how we're doing in tech
		int iScore = 0;
		int iTotalPop = 0;
		for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
		{
			const CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iI);
			if (iI != getID() && kLoopPlayer.isAlive() && GET_TEAM(getTeam()).isHasMet(kLoopPlayer.getTeam()))
			{
				iTotalPop += kLoopPlayer.getTotalPopulation();
				iScore += (getTechScore() > kLoopPlayer.getTechScore() ? 20 : -20) * kLoopPlayer.getTotalPopulation();
			}
		}
		iValue += iScore / std::max(1, iTotalPop);
		// K-Mod end

		//int iNonsense = AI_getStrategyRand() + 50;
		iValue += (AI_getStrategyRand(3) % 100);

		if (iValue >= 100)
		{
			if( getCurrentEra() >= GC.getNumEraInfos() - 3 )
			{
				return 2;
			}

			return 1;
		}
	}

	return 0;
}

int CvPlayerAI::AI_getConquestVictoryStage() const
{
	int iValue;

	if (GC.getGameINLINE().isOption(GAMEOPTION_ALWAYS_PEACE))
	{
    	return 0;
	}

	if(GET_TEAM(getTeam()).isAVassal())
	{
		return 0;
	}

	if( GC.getDefineINT("BBAI_VICTORY_STRATEGY_CONQUEST") <= 0 )
	{
		return 0;
	}

	VictoryTypes eConquest = NO_VICTORY;
	for (int iI = 0; iI < GC.getNumVictoryInfos(); iI++)
	{
		if (GC.getGameINLINE().isVictoryValid((VictoryTypes) iI))
		{
			CvVictoryInfo& kVictoryInfo = GC.getVictoryInfo((VictoryTypes) iI);
			if( kVictoryInfo.isConquest() )
			{
				eConquest = (VictoryTypes)iI;
				break;
			}
		}
	}

	if( eConquest == NO_VICTORY )
	{
		return 0;
	}

	// Check for whether we are very powerful, looking good for conquest
	int iOurPower = GET_TEAM(getTeam()).getPower(true);
	int iOurPowerRank = 1;
	int iTotalPower = 0;
	int iNumNonVassals = 0;
	for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
	{
		if( iI != getTeam() )
		{
			CvTeam& kTeam = GET_TEAM((TeamTypes) iI);
			if (kTeam.isAlive() && !(kTeam.isMinorCiv()))
			{
				if( !kTeam.isAVassal() )
				{
					iTotalPower += kTeam.getPower(true);
					iNumNonVassals++;

					if( GET_TEAM(getTeam()).isHasMet((TeamTypes) iI) )
					{
						if( 95*kTeam.getPower(false) > 100*iOurPower )
						{
							iOurPowerRank++;
						}
					}
				}
			}
		}
	}
	int iAverageOtherPower = iTotalPower / std::max(1, iNumNonVassals);

	if( 3*iOurPower > 4*iAverageOtherPower )
	{
		// BBAI TODO: Have we declared total war on anyone?  Need some aggressive action taken, maybe past war success
		int iOthersWarMemoryOfUs = 0;
		for( int iPlayer = 0; iPlayer < MAX_CIV_PLAYERS; iPlayer++ )
		{
			if( GET_PLAYER((PlayerTypes)iPlayer).getTeam() != getTeam() && GET_PLAYER((PlayerTypes)iPlayer).isEverAlive() )
			{
				iOthersWarMemoryOfUs += GET_PLAYER((PlayerTypes)iPlayer).AI_getMemoryCount(getID(), MEMORY_DECLARED_WAR);
			}
		}

		if( GET_TEAM(getTeam()).getHasMetCivCount(false) > GC.getGameINLINE().countCivPlayersAlive()/4 )
		{
			if( iOurPowerRank <= 1 + (GET_TEAM(getTeam()).getHasMetCivCount(true)/10) )
			{
				if( (iOurPower > 2*iAverageOtherPower) && (iOurPower - iAverageOtherPower > 100) )
				{
					if( iOthersWarMemoryOfUs > 0  )
					{
						return 4;
					}
				}
			}
		}

		if (getCurrentEra() >= ((GC.getNumEraInfos() / 3)))
		{
			if( iOurPowerRank <= 1 + (GET_TEAM(getTeam()).getHasMetCivCount(true)/7) )
			{
				if( iOthersWarMemoryOfUs > 2  )
				{
					return 3;
				}
			}
		}
	}

	if( isHuman() && !(GC.getGameINLINE().isDebugMode()) )
	{
		return 0;
	}

	// Check for whether we are inclined to pursue a conquest strategy
	{
		iValue = GC.getLeaderHeadInfo(getPersonalityType()).getConquestVictoryWeight();
		
		iValue += (GC.getGameINLINE().isOption(GAMEOPTION_AGGRESSIVE_AI) ? 20 : 0);

		//int iNonsense = AI_getStrategyRand() + 30;
		iValue += (AI_getStrategyRand(4) % 100);

		if (iValue >= 100)
		{
			// K-Mod note: I don't know why this is here. We need better units therefore we should go for conquest?
			if( m_iStrategyHash & AI_STRATEGY_GET_BETTER_UNITS )
			{
				if( (getNumCities() > 3) && (4*iOurPower > 5*iAverageOtherPower) )
				{
					return 2;
				}
			}

			return 1;
		}
	}

	return 0;
}

int CvPlayerAI::AI_getDominationVictoryStage() const
{
	int iValue = 0;

	if (GC.getGameINLINE().isOption(GAMEOPTION_ALWAYS_PEACE))
	{
    	return 0;
	}

	if(GET_TEAM(getTeam()).isAVassal())
	{
		return 0;
	}

	if( GC.getDefineINT("BBAI_VICTORY_STRATEGY_DOMINATION") <= 0 )
	{
		return 0;
	}

	VictoryTypes eDomination = NO_VICTORY;
	for (int iI = 0; iI < GC.getNumVictoryInfos(); iI++)
	{
		if (GC.getGameINLINE().isVictoryValid((VictoryTypes) iI))
		{
			CvVictoryInfo& kVictoryInfo = GC.getVictoryInfo((VictoryTypes) iI);
			if( kVictoryInfo.getLandPercent() > 0 && kVictoryInfo.getPopulationPercentLead() )
			{
				eDomination = (VictoryTypes)iI;
				break;
			}
		}
	}

	if( eDomination == NO_VICTORY )
	{
		return 0;
	}

	int iPercentOfDomination = 0;
	int iOurPopPercent = (100 * GET_TEAM(getTeam()).getTotalPopulation()) / std::max(1, GC.getGameINLINE().getTotalPopulation());
	int iOurLandPercent = (100 * GET_TEAM(getTeam()).getTotalLand()) / std::max(1, GC.getMapINLINE().getLandPlots());
	
	iPercentOfDomination = (100 * iOurPopPercent) / std::max(1, GC.getGameINLINE().getAdjustedPopulationPercent(eDomination));
	iPercentOfDomination = std::min( iPercentOfDomination, (100 * iOurLandPercent) / std::max(1, GC.getGameINLINE().getAdjustedLandPercent(eDomination)) );


	if( iPercentOfDomination > 80 )
	{
		return 4;
	}

	if( iPercentOfDomination > 50 )
	{
		return 3;
	}

	if( isHuman() && !(GC.getGameINLINE().isDebugMode()) )
	{
		return 0;
	}

	// Check for whether we are inclined to pursue a domination strategy
	{
		iValue = GC.getLeaderHeadInfo(getPersonalityType()).getDominationVictoryWeight();
		
		iValue += (GC.getGameINLINE().isOption(GAMEOPTION_AGGRESSIVE_AI) ? 20 : 0);

		//int iNonsense = AI_getStrategyRand() + 70;
		iValue += (AI_getStrategyRand(5) % 100);

		if (iValue >= 100)
		{
			if( getNumCities() > 3 && (GC.getGameINLINE().getPlayerRank(getID()) < (GC.getGameINLINE().countCivPlayersAlive() + 1)/2) )
			{
				return 2;
			}

			return 1;
		}
	}

	return 0;
}

int CvPlayerAI::AI_getDiplomacyVictoryStage() const
{
	int iValue = 0;

	if( GC.getDefineINT("BBAI_VICTORY_STRATEGY_DIPLOMACY") <= 0 )
	{
		return 0;
	}

	std::vector<VictoryTypes> veDiplomacy;
	for (int iI = 0; iI < GC.getNumVictoryInfos(); iI++)
	{
		if (GC.getGameINLINE().isVictoryValid((VictoryTypes) iI))
		{
			CvVictoryInfo& kVictoryInfo = GC.getVictoryInfo((VictoryTypes) iI);
			if( kVictoryInfo.isDiploVote() )
			{
				veDiplomacy.push_back((VictoryTypes)iI);
			}
		}
	}

	if( veDiplomacy.size() == 0 )
	{
		return 0;
	}

	// Check for whether we are elligible for election
	bool bVoteEligible = false;
	for( int iVoteSource = 0; iVoteSource < GC.getNumVoteSourceInfos(); iVoteSource++ )
	{
		if( GC.getGameINLINE().isDiploVote((VoteSourceTypes)iVoteSource) )
		{
			if( GC.getGameINLINE().isTeamVoteEligible(getTeam(),(VoteSourceTypes)iVoteSource) )
			{
				bVoteEligible = true;
				break;
			}
		}
	}

	bool bDiploInclined = false;

	// Check for whether we are inclined to pursue a diplomacy strategy
	{
		iValue = GC.getLeaderHeadInfo(getPersonalityType()).getDiplomacyVictoryWeight();

		if (GC.getGameINLINE().isOption(GAMEOPTION_ALWAYS_PEACE))
		{
    		iValue += 30;
		}
		
		iValue += (GC.getGameINLINE().isOption(GAMEOPTION_AGGRESSIVE_AI) ? -20 : 0);

		//int iNonsense = AI_getStrategyRand() + 90;
		iValue += (AI_getStrategyRand(6) % 100);

		// BBAI TODO: Level 2?

		if (iValue >= 100)
		{
			bDiploInclined = true;
		}
	}

	if( bVoteEligible && (bDiploInclined || isHuman()) )
	{
		// BBAI TODO: Level 4 - close to enough to win a vote?

		return 3;
	}

	if( isHuman() && !(GC.getGameINLINE().isDebugMode()) )
	{
		return 0;
	}

	if( bDiploInclined )
	{
		return 1;
	}

	return 0;
}

/// \brief Returns whether player is pursuing a particular victory strategy.
///
/// Victory strategies are computed on demand once per turn and stored for the rest
/// of the turn.  Each victory strategy type has 4 levels, the first two are
/// determined largely from AI tendencies and randomn dice rolls.  The second
/// two are based on measurables and past actions, so the AI can use them to
/// determine what other players (including the human player) are doing.
bool CvPlayerAI::AI_isDoVictoryStrategy(int iVictoryStrategy) const
{
	if( isBarbarian() || isMinorCiv() || !isAlive() )
	{
		return false;
	}
	
    return (iVictoryStrategy & AI_getVictoryStrategyHash());
}

bool CvPlayerAI::AI_isDoVictoryStrategyLevel4() const
{
	if( AI_isDoVictoryStrategy(AI_VICTORY_SPACE4) )
	{
		return true;
	}

	if( AI_isDoVictoryStrategy(AI_VICTORY_CONQUEST4) )
	{
		return true;
	}

	if( AI_isDoVictoryStrategy(AI_VICTORY_CULTURE4) )
	{
		return true;
	}

	if( AI_isDoVictoryStrategy(AI_VICTORY_DOMINATION4) )
	{
		return true;
	}

	if( AI_isDoVictoryStrategy(AI_VICTORY_DIPLOMACY4) )
	{
		return true;
	}

	return false;
}

bool CvPlayerAI::AI_isDoVictoryStrategyLevel3() const
{
	if( AI_isDoVictoryStrategy(AI_VICTORY_SPACE3) )
	{
		return true;
	}

	if( AI_isDoVictoryStrategy(AI_VICTORY_CONQUEST3) )
	{
		return true;
	}

	if( AI_isDoVictoryStrategy(AI_VICTORY_CULTURE3) )
	{
		return true;
	}

	if( AI_isDoVictoryStrategy(AI_VICTORY_DOMINATION3) )
	{
		return true;
	}

	if( AI_isDoVictoryStrategy(AI_VICTORY_DIPLOMACY3) )
	{
		return true;
	}

	return false;
}

void CvPlayerAI::AI_updateVictoryStrategyHash()
{
	PROFILE_FUNC();

	if( isBarbarian() || isMinorCiv() || !isAlive() )
	{
		m_iVictoryStrategyHash = 0;
		return;
	}

	m_iVictoryStrategyHash = AI_DEFAULT_VICTORY_STRATEGY;
    //m_iVictoryStrategyHashCacheTurn = GC.getGameINLINE().getGameTurn();

	if (getCapitalCity() == NULL)
    {
        return;
    }

	bool bStartedOtherLevel3 = false;
	bool bStartedOtherLevel4 = false;

	// Space victory
	int iVictoryStage = AI_getSpaceVictoryStage();

	if( iVictoryStage >= 1 )
	{
		m_iVictoryStrategyHash |= AI_VICTORY_SPACE1;
	    if (iVictoryStage > 1)
	    {
	        m_iVictoryStrategyHash |= AI_VICTORY_SPACE2;
            if (iVictoryStage > 2)
            {
				bStartedOtherLevel3 = true;
                m_iVictoryStrategyHash |= AI_VICTORY_SPACE3;

				if (iVictoryStage > 3 && !bStartedOtherLevel4)
				{
					bStartedOtherLevel4 = true;
                	m_iVictoryStrategyHash |= AI_VICTORY_SPACE4;
                }
            }
	    }
	}

	// Conquest victory
	iVictoryStage = AI_getConquestVictoryStage();

	if( iVictoryStage >= 1 )
	{
		m_iVictoryStrategyHash |= AI_VICTORY_CONQUEST1;
	    if (iVictoryStage > 1)
	    {
	        m_iVictoryStrategyHash |= AI_VICTORY_CONQUEST2;
            if (iVictoryStage > 2)
            {
				bStartedOtherLevel3 = true;
                m_iVictoryStrategyHash |= AI_VICTORY_CONQUEST3;

				if (iVictoryStage > 3 && !bStartedOtherLevel4)
				{
					bStartedOtherLevel4 = true;
                	m_iVictoryStrategyHash |= AI_VICTORY_CONQUEST4;
                }
            }
	    }
	}

	// Domination victory
	iVictoryStage = AI_getDominationVictoryStage();

	if( iVictoryStage >= 1 )
	{
		m_iVictoryStrategyHash |= AI_VICTORY_DOMINATION1;
	    if (iVictoryStage > 1)
	    {
	        m_iVictoryStrategyHash |= AI_VICTORY_DOMINATION2;
            if (iVictoryStage > 2)
            {
				bStartedOtherLevel3 = true;
                m_iVictoryStrategyHash |= AI_VICTORY_DOMINATION3;

				if (iVictoryStage > 3 && !bStartedOtherLevel4)
				{
					bStartedOtherLevel4 = true;
                	m_iVictoryStrategyHash |= AI_VICTORY_DOMINATION4;
                }
            }
	    }
	}

	// Cultural victory
	// K-Mod Note: AI_getCultureVictoryStage now checks some of the other victory strategies,
	// so it is important that they are set first.
	iVictoryStage = AI_getCultureVictoryStage();

	if( iVictoryStage >= 1 )
	{
		m_iVictoryStrategyHash |= AI_VICTORY_CULTURE1;
	    if (iVictoryStage > 1)
	    {
	        m_iVictoryStrategyHash |= AI_VICTORY_CULTURE2;
            if (iVictoryStage > 2)
            {
				bStartedOtherLevel3 = true;
                m_iVictoryStrategyHash |= AI_VICTORY_CULTURE3;

				if (iVictoryStage > 3 && !bStartedOtherLevel4)
				{
					bStartedOtherLevel4 = true;
                	m_iVictoryStrategyHash |= AI_VICTORY_CULTURE4;
                }
            }
	    }
	}

	// Diplomacy victory
	iVictoryStage = AI_getDiplomacyVictoryStage();

	if( iVictoryStage >= 1 )
	{
		m_iVictoryStrategyHash |= AI_VICTORY_DIPLOMACY1;
	    if (iVictoryStage > 1)
	    {
	        m_iVictoryStrategyHash |= AI_VICTORY_DIPLOMACY2;
            if (iVictoryStage > 2 && !bStartedOtherLevel3)
            {
				bStartedOtherLevel3 = true;
                m_iVictoryStrategyHash |= AI_VICTORY_DIPLOMACY3;

				if (iVictoryStage > 3 && !bStartedOtherLevel4)
				{
					bStartedOtherLevel4 = true;
                	m_iVictoryStrategyHash |= AI_VICTORY_DIPLOMACY4;
                }
            }
	    }
	}
}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

// K-Mod, based on BBAI
void CvPlayerAI::AI_initStrategyRand()
{
	const unsigned iBits = 24;
	m_iStrategyRand = GC.getGameINLINE().getSorenRandNum((1<<(iBits+1))-1, "AI Strategy Rand");
}

int CvPlayerAI::AI_getStrategyRand(int iShift) const
{
	const unsigned iBits = 24; // cf bits in AI_initStrategyRand

	iShift += getCurrentEra();
	while (iShift < 0)
		iShift += iBits;
	iShift %= iBits;

	FAssert(m_iStrategyRand != UINT_MAX);
	unsigned x = 2654435761; // golden ratio of 2^32
	x *= (m_iStrategyRand << iShift) + (m_iStrategyRand >> (iBits - iShift));
	x >>= 1; // force positive;
	FAssert(x <= INT_MAX);
	return x;
}
// K-Mod end

bool CvPlayerAI::AI_isDoStrategy(int iStrategy) const
{
    if (isHuman())
    {
        return false;
    }

	if( isBarbarian() || isMinorCiv() || !isAlive() )
	{
		return false;
	}

    return (iStrategy & AI_getStrategyHash());
}

/* obsolete original bts code
void CvPlayerAI::AI_forceUpdateStrategies()
{
	//this forces a recache.
	m_iStrategyHashCacheTurn = -1;
} */

// K-mod. The body of this function use to be inside "AI_getStrategyHash"
void CvPlayerAI::AI_updateStrategyHash()
{

// K-Mod. Macros to help log changes in the AI strategy.
#define log_strat(s) \
	if (gPlayerLogLevel >= 2) \
	{ \
		if ((m_iStrategyHash & s) != (iLastStrategyHash & s)) \
		{ \
			logBBAI( "    Player %d (%S) %s strategy "#s" on turn %d", getID(), getCivilizationDescription(0), m_iStrategyHash & s ? "starts" : "stops", GC.getGameINLINE().getGameTurn()); \
		} \
	}
#define log_strat2(s, x) \
	if (gPlayerLogLevel >= 2) \
	{ \
		if ((m_iStrategyHash & s) != (iLastStrategyHash & s)) \
		{ \
			logBBAI( "    Player %d (%S) %s strategy "#s" on turn %d with "#x" %d", getID(), getCivilizationDescription(0), m_iStrategyHash & s ? "starts" : "stops", GC.getGameINLINE().getGameTurn(), x); \
		} \
	}
//

	const CvTeamAI& kTeam = GET_TEAM(getTeam()); // K-Mod. (and replaced all through this function)

    UnitTypes eLoopUnit;

	int iLastStrategyHash = m_iStrategyHash;
    
    m_iStrategyHash = AI_DEFAULT_STRATEGY;
    
	/* original bts code
	if (AI_getFlavorValue(FLAVOR_PRODUCTION) >= 2) // 0, 2, 5 or 10 in default xml [augustus 5, frederick 10, huayna 2, jc 2, chinese leader 2, qin 5, ramsess 2, roosevelt 5, stalin 2]
	{
		m_iStrategyHash |= AI_STRATEGY_PRODUCTION;
	} */ // K-Mod. This strategy is now set later on, with new conditions.
	
	if (getCapitalCity() == NULL)
    {
        return;
    }
    
    //int iNonsense = AI_getStrategyRand();
    
	int iMetCount = kTeam.getHasMetCivCount(true);
    
    //Unit Analysis
    int iBestSlowUnitCombat = -1;
    int iBestFastUnitCombat = -1;
    
    bool bHasMobileArtillery = false;
    bool bHasMobileAntiair = false;
    bool bHasBomber = false;
    
    int iNukeCount = 0;
    
    int iAttackUnitCount = 0;
	// K-Mod
	int iAverageEnemyUnit = 0;
	int iTypicalAttack = getTypicalUnitValue(UNITAI_ATTACK);
	int iTypicalDefence = getTypicalUnitValue(UNITAI_CITY_DEFENSE);
	int iWarSuccessRating = kTeam.AI_getWarSuccessRating();
	// K-Mod end

	for (int iI = 0; iI < GC.getNumUnitClassInfos(); iI++)
	{
		eLoopUnit = ((UnitTypes)(GC.getCivilizationInfo(getCivilizationType()).getCivilizationUnits(iI)));
		
		if (NO_UNIT != eLoopUnit)
		{
			if (getCapitalCity() != NULL)
			{
				if (getCapitalCity()->canTrain(eLoopUnit))
				{
					CvUnitInfo& kLoopUnit = GC.getUnitInfo(eLoopUnit);
					bool bIsUU = (GC.getUnitClassInfo((UnitClassTypes)iI).getDefaultUnitIndex()) != (GC.getCivilizationInfo(getCivilizationType()).getCivilizationUnits(iI));
					/* original bts code
					if (kLoopUnit.getUnitAIType(UNITAI_RESERVE) || kLoopUnit.getUnitAIType(UNITAI_ATTACK_CITY)
						|| kLoopUnit.getUnitAIType(UNITAI_COUNTER) || kLoopUnit.getUnitAIType(UNITAI_PILLAGE)) */
					// K-Mod. Original code was missing the obvious: UNITAI_ATTACK. Was this a bug? (I'm skipping "pillage".)
					if (kLoopUnit.getUnitAIType(UNITAI_ATTACK) || kLoopUnit.getUnitAIType(UNITAI_ATTACK_CITY)
						|| kLoopUnit.getUnitAIType(UNITAI_RESERVE) || kLoopUnit.getUnitAIType(UNITAI_COUNTER))
					// K-Mod end
					{
						iAttackUnitCount++;

						//UU love
						if (bIsUU)
						{
							if (kLoopUnit.getUnitAIType(UNITAI_ATTACK_CITY) || 
								(kLoopUnit.getUnitAIType(UNITAI_ATTACK)	&& !kLoopUnit.getUnitAIType(UNITAI_CITY_DEFENSE)))
							{
								iAttackUnitCount++;					
							}
						}
						int iCombat = kLoopUnit.getCombat();
						int iMoves = kLoopUnit.getMoves();
						if (iMoves == 1)
						{
							iBestSlowUnitCombat = std::max(iBestSlowUnitCombat, iCombat);
						}
						else if (iMoves > 1)
						{
							iBestFastUnitCombat = std::max(iBestFastUnitCombat, iCombat);
							if (bIsUU)
							{
								iBestFastUnitCombat += 1;
							}
						}
					}
/************************************************************************************************/
/* UNOFFICIAL_PATCH                       09/10/08                                jdog5000      */
/*                                                                                              */
/* Bugfix				                                                                         */
/************************************************************************************************/
/* original BTS code
					if (kLoopUnit.getMoves() > 1)
*/
					// Mobile anti-air and artillery flags only meant for land units
					if ( kLoopUnit.getDomainType() == DOMAIN_LAND && kLoopUnit.getMoves() > 1 )
/************************************************************************************************/
/* UNOFFICIAL_PATCH                        END                                                  */
/************************************************************************************************/
					{
						if (kLoopUnit.getInterceptionProbability() > 25)
						{
							bHasMobileAntiair = true;
						}
						if (kLoopUnit.getBombardRate() > 10)
						{
							bHasMobileArtillery = true;
						}
					}

					if (kLoopUnit.getAirRange() > 1)
					{
						if (!kLoopUnit.isSuicide())
						{
							if ((kLoopUnit.getBombRate() > 10) && (kLoopUnit.getAirCombat() > 0))
							{
								bHasBomber = true;								
							}
						}
					}
					
					if (kLoopUnit.getNukeRange() > 0)
					{
						iNukeCount++;
					}
				}
			}
		}
	}

	// K-Mod
	{
		int iTotalPower = 0;
		int iTotalWeightedValue = 0;
		for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
		{
			CvPlayer &kLoopPlayer = GET_PLAYER((PlayerTypes)iI);
			if (kLoopPlayer.getTeam() != getTeam())
			{
				if (kLoopPlayer.isAlive() && kTeam.isHasMet(kLoopPlayer.getTeam()))
				{
					// Attack units are scaled down to roughly reflect their limitations.
					// (eg. Knights (10) vs Macemen (8). Cavalry (15) vs Rifles (14). Tank (28) vs Infantry (20) / Marine (24) )
					int iValue = std::max(100*kLoopPlayer.getTypicalUnitValue(UNITAI_ATTACK)/110, kLoopPlayer.getTypicalUnitValue(UNITAI_CITY_DEFENSE));

					iTotalWeightedValue += kLoopPlayer.getPower() * iValue;
					iTotalPower += kLoopPlayer.getPower();
				}
			}
		}
		if (iTotalPower == 0)
			iAverageEnemyUnit = 0;
		else
			iAverageEnemyUnit = iTotalWeightedValue / iTotalPower;
		// A bit of random variation...
		iAverageEnemyUnit *= (91+AI_getStrategyRand(1)%20);
		iAverageEnemyUnit /= 100;
	}

	//if (iAttackUnitCount <= 1)
	if ((iAttackUnitCount <= 1 && GC.getGame().getGameTurn() > GC.getGameSpeedInfo(GC.getGame().getGameSpeedType()).getBarbPercent()/20)
		|| (100*iAverageEnemyUnit > 135*iTypicalAttack && 100*iAverageEnemyUnit > 135*iTypicalDefence))
	{
		m_iStrategyHash |= AI_STRATEGY_GET_BETTER_UNITS;
	}
	// K-Mod end
	if (iBestFastUnitCombat > iBestSlowUnitCombat)
	{
		m_iStrategyHash |= AI_STRATEGY_FASTMOVERS;
		if (bHasMobileArtillery && bHasMobileAntiair)
		{
			m_iStrategyHash |= AI_STRATEGY_LAND_BLITZ;
		}
	}
	if (iNukeCount > 0)
	{
		if ((GC.getLeaderHeadInfo(getPersonalityType()).getBuildUnitProb() + AI_getStrategyRand(7) % 15) >= (GC.getGameINLINE().isOption(GAMEOPTION_AGGRESSIVE_AI) ? 37 : 43))
		{
			m_iStrategyHash |= AI_STRATEGY_OWABWNW;
		}
	}
	if (bHasBomber)
	{
		if (!(m_iStrategyHash & AI_STRATEGY_LAND_BLITZ))
		{
			m_iStrategyHash |= AI_STRATEGY_AIR_BLITZ;
		}
		else
		{
			if ((AI_getStrategyRand(8) % 2) == 0)
			{
				m_iStrategyHash |= AI_STRATEGY_AIR_BLITZ;
				m_iStrategyHash &= ~AI_STRATEGY_LAND_BLITZ;				
			}
		}
	}

	log_strat(AI_STRATEGY_LAND_BLITZ)
	log_strat(AI_STRATEGY_AIR_BLITZ)
    
	//missionary
	{
	    if (getStateReligion() != NO_RELIGION)
	    {
            int iHolyCityCount = countHolyCities();
            if ((iHolyCityCount > 0) && hasHolyCity(getStateReligion()))
            {
                int iMissionary = 0;
                //Missionary
                iMissionary += AI_getFlavorValue(FLAVOR_GROWTH) * 2; // up to 10
                iMissionary += AI_getFlavorValue(FLAVOR_CULTURE) * 4; // up to 40
                iMissionary += AI_getFlavorValue(FLAVOR_RELIGION) * 6; // up to 60
                
                CivicTypes eCivic = (CivicTypes)GC.getLeaderHeadInfo(getPersonalityType()).getFavoriteCivic();
                if ((eCivic != NO_CIVIC) && (GC.getCivicInfo(eCivic).isNoNonStateReligionSpread()))
                {
                	iMissionary += 20;
                }
                
                iMissionary += (iHolyCityCount - 1) * 5;
                
				iMissionary += std::min(iMetCount, 5) * 7;

                for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
                {
					if (iI != getID())
                    {
						if (GET_PLAYER((PlayerTypes)iI).isAlive() && kTeam.isHasMet(GET_PLAYER((PlayerTypes)iI).getTeam()))
						{
                            if (kTeam.isOpenBorders(GET_PLAYER((PlayerTypes)iI).getTeam()))
                            {
								if ((GET_PLAYER((PlayerTypes)iI).getStateReligion() == getStateReligion()))
								{
									iMissionary += 10;
								}
								else if( !GET_PLAYER((PlayerTypes)iI).isNoNonStateReligionSpread() )
								{
									iMissionary += (GET_PLAYER((PlayerTypes)iI).countHolyCities() == 0) ? 12 : 4;
								}
							}
                        }
                    }
                }
                
                iMissionary += (AI_getStrategyRand(9) % 7) * 3;
                
                if (iMissionary > 100)
                {
                    m_iStrategyHash |= AI_STRATEGY_MISSIONARY;
                }
            }
	    }
	}

	// Espionage
	// K-Mod
	// Apparently BBAI wanted to use "big espionage" to save points when our espionage is weak.
	// I've got other plans.
	if (!GC.getGameINLINE().isOption(GAMEOPTION_NO_ESPIONAGE))
	{
		// don't start espionage strategy if we have no spies
		if (iLastStrategyHash & AI_STRATEGY_BIG_ESPIONAGE || AI_getNumAIUnits(UNITAI_SPY) > 0)
		{
			int iTempValue = 0;
			iTempValue += AI_commerceWeight(COMMERCE_ESPIONAGE) / 8;
			// Note: although AI_commerceWeight is doubled for Big Espionage,
			// the value here is unaffected because the strategy hash has been cleared.
			iTempValue += kTeam.getBestKnownTechScorePercent() < 85 ? 3 : 0;
			iTempValue += kTeam.getAnyWarPlanCount(true) > kTeam.getAtWarCount(true) ? 2 : 0; // build up espionage before the start of a war
			if (iWarSuccessRating < 0)
				iTempValue += iWarSuccessRating/15 - 1;
			iTempValue += AI_getStrategyRand(10) % 8;

			if (iTempValue > 11)
			{
				// Don't allow big esp if we have no local esp targets
				for (int i = 0; i < MAX_CIV_TEAMS; i++)
				{
					if (i != getTeam() && kTeam.isHasMet((TeamTypes)i) && kTeam.AI_hasCitiesInPrimaryArea((TeamTypes)i))
					{
						m_iStrategyHash |= AI_STRATEGY_BIG_ESPIONAGE;
						break;
					}
				}
			}
		}

		// The espionage economy decision is actually somewhere else. This is just a marker.
		if (getCommercePercent(COMMERCE_ESPIONAGE) > 20)
			m_iStrategyHash |= AI_STRATEGY_ESPIONAGE_ECONOMY;
	}
	log_strat(AI_STRATEGY_ESPIONAGE_ECONOMY)
	// K-Mod end

	// Turtle strategy
	if( kTeam.getAtWarCount(true) > 0 && getNumCities() > 0 )
	{
		int iMaxWarCounter = 0;
		for( int iTeam = 0; iTeam < MAX_CIV_TEAMS; iTeam++ )
		{
			if( iTeam != getTeam() )
			{
				if( GET_TEAM((TeamTypes)iTeam).isAlive() && !GET_TEAM((TeamTypes)iTeam).isMinorCiv() )
				{
					iMaxWarCounter = std::max( iMaxWarCounter, kTeam.AI_getAtWarCounter((TeamTypes)iTeam) );
				}
			}
		}

		// Are we losing badly or recently attacked?
		if( iWarSuccessRating < -50 || iMaxWarCounter < 10 )
		{
			if( kTeam.AI_getEnemyPowerPercent(true) > std::max(150, GC.getDefineINT("BBAI_TURTLE_ENEMY_POWER_RATIO")) )
			{
				m_iStrategyHash |= AI_STRATEGY_TURTLE;
			}
		}
	}
	log_strat(AI_STRATEGY_TURTLE)

	int iCurrentEra = getCurrentEra();
	int iParanoia = 0;
	int iCloseTargets = 0;
	int iOurDefensivePower = kTeam.getDefensivePower();

    for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
    {
		const CvPlayerAI& kLoopPlayer = GET_PLAYER((PlayerTypes)iI);
		const CvTeamAI& kLoopTeam = GET_TEAM(kLoopPlayer.getTeam());
		if (kLoopPlayer.isAlive() && !kLoopPlayer.isMinorCiv() && !kLoopPlayer.isBarbarian() )
		{
			if (kLoopPlayer.getTeam() != getTeam() && kTeam.isHasMet(kLoopPlayer.getTeam()))
			{
				if (!kLoopTeam.isAVassal() && !kTeam.isVassal(kLoopPlayer.getTeam()))
    			{
					bool bCitiesInPrime = kTeam.AI_hasCitiesInPrimaryArea(kLoopPlayer.getTeam()); // K-Mod

					if (kTeam.AI_getWarPlan(kLoopPlayer.getTeam()) != NO_WARPLAN)
					{
						iCloseTargets++;
					}
					else
					{
						// Are they a threat?
						int iTempParanoia = 0;

						int iTheirPower = kLoopTeam.getPower(true);
						if (4*iTheirPower > 3*iOurDefensivePower)
						{
							if (kLoopTeam.getAtWarCount(true) == 0 || kLoopTeam.AI_getEnemyPowerPercent(false) < 140 )
							{
								// Memory of them declaring on us and our friends
								int iWarMemory = AI_getMemoryCount((PlayerTypes)iI, MEMORY_DECLARED_WAR);
								iWarMemory += (AI_getMemoryCount((PlayerTypes)iI, MEMORY_DECLARED_WAR_ON_FRIEND) + 1)/2;

								if (iWarMemory > 0)
								{
									//they are a snake
									iTempParanoia += 50 + 50 * iWarMemory;

									if( gPlayerLogLevel >= 2 )
									{
										logBBAI( "    Player %d (%S) wary of %S because of war memory %d", getID(), getCivilizationDescription(0), GET_PLAYER((PlayerTypes)iI).getCivilizationDescription(0), iWarMemory);
									}
								}
							}
						}

						// Do we think our relations are bad?
						int iCloseness = AI_playerCloseness((PlayerTypes)iI, DEFAULT_PLAYER_CLOSENESS);
						// if (iCloseness > 0)
						if (iCloseness > 0 || bCitiesInPrime) // K-Mod
						{
							int iAttitudeWarProb = 100 - GC.getLeaderHeadInfo(getPersonalityType()).getNoWarAttitudeProb(AI_getAttitude((PlayerTypes)iI));
							/* original BBAI code
							if( iAttitudeWarProb > 10 )
							{
								if( 4*iTheirPower > 3*iOurDefensivePower )
								{
									iTempParanoia += iAttitudeWarProb/2;
								}

								iCloseTargets++;
							} */
							// K-Mod. Paranoia gets scaled by relative power anyway...
							iTempParanoia += std::max(0, iAttitudeWarProb/2);
							if (iAttitudeWarProb > 10 && iCloseness > 0)
							{
								iCloseTargets++;
							}
							// K-Mod end

							if( iTheirPower > 2*iOurDefensivePower )
							{
								if( AI_getAttitude((PlayerTypes)iI) != ATTITUDE_FRIENDLY )
								{
									iTempParanoia += 25;
								}
							}
						}

						if( iTempParanoia > 0 )
						{
							iTempParanoia *= iTheirPower;
							iTempParanoia /= std::max(1, iOurDefensivePower);
							// K-Mod
							if (kLoopTeam.AI_getWorstEnemy() == getTeam())
							{
								iTempParanoia *= 2;
							}
							// K-Mod end
						}

						// Do they look like they're going for militaristic victory?
						if( kLoopPlayer.AI_isDoVictoryStrategy(AI_VICTORY_CONQUEST4) )
						{
							iTempParanoia += 200;
						}
						else if( kLoopPlayer.AI_isDoVictoryStrategy(AI_VICTORY_CONQUEST3) )
						{
							iTempParanoia += 100;
						}
						else if( kLoopPlayer.AI_isDoVictoryStrategy(AI_VICTORY_DOMINATION3) )
						{
							iTempParanoia += 50;
						}

						if( iTempParanoia > 0 )
						{
							if( iCloseness == 0 )
							{
								iTempParanoia /= 2;
							}

							iParanoia += iTempParanoia;
						}
					}
				}
			}
    	}
    }

	if( m_iStrategyHash & AI_STRATEGY_GET_BETTER_UNITS )
	{
		iParanoia *= 3;
		iParanoia /= 2;
	}

	// Scale paranoia in later eras/larger games
	//iParanoia -= (100*(iCurrentEra + 1)) / std::max(1, GC.getNumEraInfos());

	// K-Mod. You call that scaling for "later eras/larger games"? It isn't scaling, and it doesn't use the map size.
	// Lets try something else. Rough and ad hoc, but hopefully a bit better.
	iParanoia *= (3*GC.getNumEraInfos() - 2*iCurrentEra);
	iParanoia /= 3*(std::max(1, GC.getNumEraInfos()));
	// That starts as a factor of 1, and drop to 1/3.  And now for game size...
	iParanoia *= 14;
	iParanoia /= (7+std::max(kTeam.getHasMetCivCount(true), GC.getWorldInfo(GC.getMapINLINE().getWorldSize()).getDefaultPlayers()));

	// Alert strategy
	if( iParanoia >= 200 )
	{
		m_iStrategyHash |= AI_STRATEGY_ALERT1;
		if( iParanoia >= 400 )
		{
			m_iStrategyHash |= AI_STRATEGY_ALERT2;
		}
	}
	log_strat2(AI_STRATEGY_ALERT1, iParanoia)
	log_strat2(AI_STRATEGY_ALERT2, iParanoia)

	// Economic focus (K-Mod) - This strategy is a gambit. The goal is to tech faster by neglecting military.
	if (kTeam.getAnyWarPlanCount(true) == 0)
	{
		int iFocus = (100 - iParanoia) / 20;
		iFocus += std::max(iAverageEnemyUnit - std::max(iTypicalAttack, iTypicalDefence), std::min(iTypicalAttack, iTypicalDefence) - iAverageEnemyUnit) / 12;
		//Note: if we haven't met anyone then average enemy is zero. So this essentially assures economic strategy when in isolation.
		iFocus += (AI_getPeaceWeight() + AI_getStrategyRand(2)%10)/3; // note: peace weight will be between 0 and 12

		if (iFocus >= 12)
			m_iStrategyHash |= AI_STRATEGY_ECONOMY_FOCUS;
	}
	log_strat(AI_STRATEGY_ECONOMY_FOCUS)

	// BBAI TODO: Integrate Dagger with new conquest victory strategy, have Dagger focus on early rushes
    //dagger
	if( !(AI_isDoVictoryStrategy(AI_VICTORY_CULTURE2)) 
	 && !(m_iStrategyHash & AI_STRATEGY_MISSIONARY)
     && (iCurrentEra <= (2+(AI_getStrategyRand(11)%2))) && (iCloseTargets > 0) )
    {	    
	    int iDagger = 0;
	    iDagger += 12000 / std::max(100, (50 + GC.getLeaderHeadInfo(getPersonalityType()).getMaxWarRand()));
	    iDagger *= (AI_getStrategyRand(12) % 11);
	    iDagger /= 10;
	    iDagger += 5 * std::min(8, AI_getFlavorValue(FLAVOR_MILITARY));
	    
        for (int iI = 0; iI < GC.getNumUnitClassInfos(); iI++)
        {
            eLoopUnit = ((UnitTypes)(GC.getCivilizationInfo(getCivilizationType()).getCivilizationUnits(iI)));

            if ((eLoopUnit != NO_UNIT) && (GC.getUnitInfo(eLoopUnit).getCombat() > 0))
            {
                if ((GC.getUnitClassInfo((UnitClassTypes)iI).getDefaultUnitIndex()) != (GC.getCivilizationInfo(getCivilizationType()).getCivilizationUnits(iI)))
                {
                	bool bIsDefensive = (GC.getUnitInfo(eLoopUnit).getUnitAIType(UNITAI_CITY_DEFENSE) &&
                		!GC.getUnitInfo(eLoopUnit).getUnitAIType(UNITAI_RESERVE));
					
					iDagger += bIsDefensive ? -10 : 0;
                       
                    if (getCapitalCity()->canTrain(eLoopUnit))
                    {
                        iDagger += bIsDefensive ? 10 : 40;
                        
                        int iUUStr = GC.getUnitInfo(eLoopUnit).getCombat();
                        int iNormalStr = GC.getUnitInfo((UnitTypes)(GC.getUnitClassInfo((UnitClassTypes)iI).getDefaultUnitIndex())).getCombat();
                        iDagger += 20 * range((iUUStr - iNormalStr), 0, 2);
						if (GC.getUnitInfo(eLoopUnit).getPrereqAndTech() == NO_TECH)
                        {
                            iDagger += 20;
                        }
                    }
                    else
                    {
                        if (GC.getUnitInfo(eLoopUnit).getPrereqAndTech() != NO_TECH)
                        {
                            if (GC.getTechInfo((TechTypes)(GC.getUnitInfo(eLoopUnit).getPrereqAndTech())).getEra() <= (iCurrentEra + 1))
                            {
                                if (kTeam.isHasTech((TechTypes)GC.getUnitInfo(eLoopUnit).getPrereqAndTech()))
                                {
                                	//we have the tech but can't train the unit, dejection.
                                    iDagger += 10;
                                }
                                else
                                {
                                	//we don't have the tech, it's understandable we can't train.
                                    iDagger += 30;
                                }
                            }
                        }
                                
                        bool bNeedsAndBonus = false;
                        int iOrBonusCount = 0;
                        int iOrBonusHave = 0;
                        
                        for (int iJ = 0; iJ < GC.getNumBonusInfos(); iJ++)
                        {
                            BonusTypes eBonus = (BonusTypes)iJ;
                            if (eBonus != NO_BONUS)
                            {
                                if (GC.getUnitInfo(eLoopUnit).getPrereqAndBonus() == eBonus)
                                {
                                    if (getNumTradeableBonuses(eBonus) == 0)
                                    {
                                        bNeedsAndBonus = true;
                                    }
                                }

                                for (int iK = 0; iK < GC.getNUM_UNIT_PREREQ_OR_BONUSES(); iK++)
                                {
                                    if (GC.getUnitInfo(eLoopUnit).getPrereqOrBonuses(iK) == eBonus)
                                    {
                                        iOrBonusCount++;
                                        if (getNumTradeableBonuses(eBonus) > 0)
                                        {
                                            iOrBonusHave++;
                                        }
                                    }
                                }
                            }
                        }
                        
                        
                        iDagger += 20;
                        if (bNeedsAndBonus)
                        {
                            iDagger -= 20;
                        }
                        if ((iOrBonusCount > 0) && (iOrBonusHave == 0))
                        {
                            iDagger -= 20;
                        }
                    }
                }
            }
        }
        
        if (!GC.getGameINLINE().isOption(GAMEOPTION_AGGRESSIVE_AI))
        {
			iDagger += range(100 - GC.getHandicapInfo(GC.getGameINLINE().getHandicapType()).getAITrainPercent(), 0, 15);
        }
        
        if ((getCapitalCity()->area()->getAreaAIType(getTeam()) == AREAAI_OFFENSIVE) || (getCapitalCity()->area()->getAreaAIType(getTeam()) == AREAAI_DEFENSIVE))
        {
            iDagger += (iAttackUnitCount > 0) ? 40 : 20;
        }
        
        if (iDagger >= AI_DAGGER_THRESHOLD)
        {
            m_iStrategyHash |= AI_STRATEGY_DAGGER;            
        }
		else
		{
			if( iLastStrategyHash &= AI_STRATEGY_DAGGER )
			{
				if (iDagger >= (9*AI_DAGGER_THRESHOLD)/10)
				{
					m_iStrategyHash |= AI_STRATEGY_DAGGER;            
				}
			}
		}

		log_strat2(AI_STRATEGY_DAGGER, iDagger)
	}
	
	if (!(m_iStrategyHash & AI_STRATEGY_ALERT2) && !(m_iStrategyHash & AI_STRATEGY_TURTLE))
	{//Crush
		int iWarCount = 0;
		int iCrushValue = 0;
		

	// K-Mod. (experimental)
		//iCrushValue += (iNonsense % 4);
		// A leader dependant value. (MaxWarRand is roughly between 50 and 200. Gandi is 400.)
		//iCrushValue += (iNonsense % 3000) / (400+GC.getLeaderHeadInfo(getPersonalityType()).getMaxWarRand());
	// On second thought, lets try this
		iCrushValue += AI_getStrategyRand(13) % (4 + AI_getFlavorValue(FLAVOR_MILITARY)/2);
		iCrushValue += std::min(0, kTeam.AI_getWarSuccessRating()/15);
		// note: flavor military is between 0 and 10
		// K-Mod end
		if (AI_isDoVictoryStrategy(AI_VICTORY_CONQUEST3))
		{
			iCrushValue += 1;
		}
		
		if (m_iStrategyHash & AI_STRATEGY_DAGGER)
		{
			iCrushValue += 3;			
		}
		if (GC.getGameINLINE().isOption(GAMEOPTION_AGGRESSIVE_AI))
		{
			iCrushValue += 3;
		}

		for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
		{
			if ((GET_TEAM((TeamTypes)iI).isAlive()) && (iI != getTeam()))
			{
				if (kTeam.AI_getWarPlan((TeamTypes)iI) != NO_WARPLAN)
				{
					if (!GET_TEAM((TeamTypes)iI).isAVassal())
					{
						if (kTeam.AI_teamCloseness((TeamTypes)iI) > 0)
						{
							iWarCount++;
						}

						// K-Mod. (if we attack with our defenders, would they beat their defenders?)
						if (100*iTypicalDefence >= 110 * GET_TEAM((TeamTypes)iI).getTypicalUnitValue(UNITAI_CITY_DEFENSE))
						{
							iCrushValue += 2;
						}						
					}

					if (kTeam.AI_getWarPlan((TeamTypes)iI) == WARPLAN_PREPARING_TOTAL)
					{
						iCrushValue += 6;					
					}
					else if ((kTeam.AI_getWarPlan((TeamTypes)iI) == WARPLAN_TOTAL) && (kTeam.AI_getWarPlanStateCounter((TeamTypes)iI) < 20))
					{
						iCrushValue += 6;						
					}
					
					if ((kTeam.AI_getWarPlan((TeamTypes)iI) == WARPLAN_DOGPILE) && (kTeam.AI_getWarPlanStateCounter((TeamTypes)iI) < 20))
					{
						for (int iJ = 0; iJ < MAX_CIV_TEAMS; iJ++)
						{
							if ((iJ != iI) && iJ != getTeam() && GET_TEAM((TeamTypes)iJ).isAlive())
							{
								if ((atWar((TeamTypes)iI, (TeamTypes)iJ)) && !GET_TEAM((TeamTypes)iI).isAVassal())
								{
									iCrushValue += 4;
								}
							}
						}
					}
				}
			}
		}
		//if ((iWarCount <= 1) && (iCrushValue >= ((iLastStrategyHash & AI_STRATEGY_CRUSH) ? 9 :10)))
		if ((iWarCount == 1) && (iCrushValue >= ((iLastStrategyHash & AI_STRATEGY_CRUSH) ? 9 :10))) // K-Mod
		{
			m_iStrategyHash |= AI_STRATEGY_CRUSH;
		}

		log_strat2(AI_STRATEGY_CRUSH, iCrushValue)
	}

	// K-Mod
	{//production
		int iProductionValue = AI_getStrategyRand(2) % (5 + AI_getFlavorValue(FLAVOR_PRODUCTION)/2);
		iProductionValue += (iLastStrategyHash & AI_STRATEGY_PRODUCTION) ? 1 : 0;
		iProductionValue += AI_getFlavorValue(FLAVOR_PRODUCTION) > 0 ? 1 : 0;
		iProductionValue += (m_iStrategyHash & AI_STRATEGY_DAGGER) ? 1 : 0;
		iProductionValue += (m_iStrategyHash & AI_STRATEGY_CRUSH) ? 1 : 0;
		iProductionValue += AI_isDoVictoryStrategy(AI_VICTORY_CONQUEST4 | AI_VICTORY_SPACE4) ? 3 : 0;
		// warplans. (done manually rather than using getWarPlanCount, so that we only have to do the loop once.)
		bool bAnyWarPlans = false;
		bool bTotalWar = false;
		for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
		{
			const CvTeam& kLoopTeam = GET_TEAM((TeamTypes)iI);
			if (kLoopTeam.isAlive() && !kLoopTeam.isMinorCiv())
			{
				switch (kTeam.AI_getWarPlan((TeamTypes)iI))
				{
				case NO_WARPLAN:
					break;
				case WARPLAN_PREPARING_TOTAL:
				case WARPLAN_TOTAL:
					bTotalWar = true;
				default:
					bAnyWarPlans = true;
					break;
				}
			}
		}
		iProductionValue += bAnyWarPlans ? 1 : 0;
		iProductionValue += bTotalWar ? 3 : 0;

		if (iProductionValue >= 10)
		{
			m_iStrategyHash |= AI_STRATEGY_PRODUCTION;
		}
		log_strat2(AI_STRATEGY_PRODUCTION, iProductionValue)
	}
	// K-Mod end

	{
		int iOurVictoryCountdown = kTeam.AI_getLowestVictoryCountdown();

		int iTheirVictoryCountdown = MAX_INT;
		
		for (int iI = 0; iI < MAX_CIV_TEAMS; iI++)
		{
/************************************************************************************************/
/* UNOFFICIAL_PATCH                       02/14/10                             jdog5000         */
/*                                                                                              */
/* Bugfix                                                                                       */
/************************************************************************************************/
/* original bts code
			if ((GET_TEAM((TeamTypes)iI).isAlive()) && (iI != getID()))
*/
			if ((GET_TEAM((TeamTypes)iI).isAlive()) && (iI != getTeam()))
/************************************************************************************************/
/* UNOFFICIAL_PATCH                        END                                                  */
/************************************************************************************************/
			{
				CvTeamAI& kOtherTeam = GET_TEAM((TeamTypes)iI);
				iTheirVictoryCountdown = std::min(iTheirVictoryCountdown, kOtherTeam.AI_getLowestVictoryCountdown());
			}
		}
		
		if (MAX_INT == iTheirVictoryCountdown)
		{
			iTheirVictoryCountdown = -1;
		}

		if ((iOurVictoryCountdown >= 0) && (iTheirVictoryCountdown < 0 || iOurVictoryCountdown <= iTheirVictoryCountdown))
		{
			m_iStrategyHash |= AI_STRATEGY_LAST_STAND;
		}
		else if ((iTheirVictoryCountdown >= 0))
		{
			if((iTheirVictoryCountdown < iOurVictoryCountdown))
			{
				m_iStrategyHash |= AI_STRATEGY_FINAL_WAR;			
			}
			else if( GC.getGameINLINE().isOption(GAMEOPTION_AGGRESSIVE_AI) )
			{
				m_iStrategyHash |= AI_STRATEGY_FINAL_WAR;
			}
			else if( AI_isDoVictoryStrategyLevel4() || AI_isDoVictoryStrategy(AI_VICTORY_SPACE3) )
			{
				m_iStrategyHash |= AI_STRATEGY_FINAL_WAR;
			}
		}
		
		if (iOurVictoryCountdown < 0)
		{
			if (isCurrentResearchRepeat())
			{
				int iStronger = 0;
				int iAlive = 1;
				for (int iTeam = 0; iTeam < MAX_CIV_TEAMS; iTeam++)
				{
					if (iTeam != getTeam())
					{
						CvTeamAI& kLoopTeam = GET_TEAM((TeamTypes)iTeam);
						if (kLoopTeam.isAlive())
						{
							iAlive++;
							if (kTeam.getPower(true) < kLoopTeam.getPower(true))
							{
								iStronger++;
							}
						}
					}
				}
				
				if ((iStronger <= 1) || (iStronger <= iAlive / 4))
				{
					m_iStrategyHash |= AI_STRATEGY_FINAL_WAR;
				}
			}
		}
		
	}

	if (isCurrentResearchRepeat())
	{
		int iTotalVictories = 0;
		int iAchieveVictories = 0;
		int iWarVictories = 0;
		
		
		int iThreshold = std::max(1, (GC.getGame().countCivTeamsAlive() + 1) / 4);
		
		for (int iVictory = 0; iVictory < GC.getNumVictoryInfos(); iVictory++)
		{
			CvVictoryInfo& kVictory = GC.getVictoryInfo((VictoryTypes)iVictory);
			if (GC.getGame().isVictoryValid((VictoryTypes)iVictory))
			{
				iTotalVictories ++;
				if (kVictory.isDiploVote())
				{
					//
				}
				else if (kVictory.isEndScore())
				{
					int iHigherCount = 0;
					int IWeakerCount = 0;
					for (int iTeam = 0; iTeam < MAX_CIV_TEAMS; iTeam++)
					{
						if (iTeam != getTeam())
						{
							CvTeamAI& kLoopTeam = GET_TEAM((TeamTypes)iTeam);
							if (kLoopTeam.isAlive())
							{
								if (GC.getGame().getTeamScore(getTeam()) < ((GC.getGame().getTeamScore((TeamTypes)iTeam) * 90) / 100))
								{
									iHigherCount++;
									if (kTeam.getPower(true) > kLoopTeam.getPower(true))
									{
										IWeakerCount++;
									}
								}
							}
						}
					}
						
					if (iHigherCount > 0)
					{
						if (IWeakerCount == iHigherCount)
						{
							iWarVictories++;
						}
					}
				}
				else if (kVictory.getCityCulture() > 0)
				{
					if (m_iStrategyHash & AI_VICTORY_CULTURE1)
					{
						iAchieveVictories++;
					}
				}
				else if (kVictory.getMinLandPercent() > 0 || kVictory.getLandPercent() > 0)
				{
					int iLargerCount = 0;
					for (int iTeam = 0; iTeam < MAX_CIV_TEAMS; iTeam++)
					{
						if (iTeam != getTeam())
						{
							CvTeamAI& kLoopTeam = GET_TEAM((TeamTypes)iTeam);
							if (kLoopTeam.isAlive())
							{
								if (kTeam.getTotalLand(true) < kLoopTeam.getTotalLand(true))
								{
									iLargerCount++;
								}
							}
						}
					}
					if (iLargerCount <= iThreshold)
					{
						iWarVictories++;
					}
				}
				else if (kVictory.isConquest())
				{
					int iStrongerCount = 0;
					for (int iTeam = 0; iTeam < MAX_CIV_TEAMS; iTeam++)
					{
						if (iTeam != getTeam())
						{
							CvTeamAI& kLoopTeam = GET_TEAM((TeamTypes)iTeam);
							if (kLoopTeam.isAlive())
							{
								if (kTeam.getPower(true) < kLoopTeam.getPower(true))
								{
									iStrongerCount++;
								}
							}
						}
					}
					if (iStrongerCount <= iThreshold)
					{
						iWarVictories++;
					}
				}
				else
				{
					if (kTeam.getVictoryCountdown((VictoryTypes)iVictory) > 0)
					{
						iAchieveVictories++;
					}
				}
			}
		}
		
		if (iAchieveVictories == 0)
		{
			if (iWarVictories > 0)
			{
				m_iStrategyHash |= AI_STRATEGY_FINAL_WAR;
			}
		}
	}
	
	
	//Turn off inappropriate strategies.
	if (GC.getGameINLINE().isOption(GAMEOPTION_ALWAYS_PEACE))
	{
		m_iStrategyHash &= ~AI_STRATEGY_DAGGER;
		m_iStrategyHash &= ~AI_STRATEGY_CRUSH;
		m_iStrategyHash &= ~AI_STRATEGY_ALERT1;
		m_iStrategyHash &= ~AI_STRATEGY_ALERT2;
		m_iStrategyHash &= ~AI_STRATEGY_TURTLE;
		m_iStrategyHash &= ~AI_STRATEGY_FINAL_WAR;
		m_iStrategyHash &= ~AI_STRATEGY_LAST_STAND;

		m_iStrategyHash &= ~AI_STRATEGY_OWABWNW;
		m_iStrategyHash &= ~AI_STRATEGY_FASTMOVERS;
	}
#undef log_strat
#undef log_strat2
}

// K-Mod
void CvPlayerAI::AI_updateGreatPersonWeights()
{
	PROFILE_FUNC();
	int iLoop;

	m_GreatPersonWeights.clear();

	for (int i = 0; i < GC.getNumSpecialistInfos(); i++)
	{
		UnitClassTypes eGreatPersonClass = (UnitClassTypes)GC.getSpecialistInfo((SpecialistTypes)i).getGreatPeopleUnitClass();

		if (eGreatPersonClass == NO_UNITCLASS)
			continue;

		FAssert(GC.getSpecialistInfo((SpecialistTypes)i).getGreatPeopleRateChange() > 0);

		if (m_GreatPersonWeights.find(eGreatPersonClass) != m_GreatPersonWeights.end())
			continue; // already evaluated

		UnitTypes eGreatPerson = (UnitTypes)GC.getCivilizationInfo(getCivilizationType()).getCivilizationUnits(eGreatPersonClass);
		if (eGreatPerson == NO_UNIT)
			continue;

		/* I've disabled the validity check, because 'invalid' specialists still affect the value of the things that enable them.
		bool bValid = isSpecialistValid((SpecialistTypes)i) || i == GC.getDefineINT("DEFAULT_SPECIALIST");
		int iLoop; // moved.
		for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity && !bValid; pLoopCity = nextCity(&iLoop))
		{
			if (pLoopCity->getMaxSpecialistCount((SpecialistTypes)i) > 0)
			{
				bValid = true;
				break;
			}
		}

		if (!bValid)
			continue; */

		// We've estabilish that we can use this specialist, and that they provide great-person points for a unit which
		// we have not yet evaluated. So now we just have to evaluate the unit!
		const CvUnitInfo& kInfo = GC.getUnitInfo(eGreatPerson);

		if (kInfo.getUnitCombatType() != NO_UNITCOMBAT)
			continue; // don't try to evaluate combat units.

		int iValue = 0;
		// value of joining a city as a super-specialist
		for (int j = 0; j < GC.getNumSpecialistInfos(); j++)
		{
			if (kInfo.getGreatPeoples((SpecialistTypes)j))
			{
				for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
				{
					// Note, specialistValue is roughly 400x the commerce it provides. So /= 4 to make it 100x.
					int iTempValue = pLoopCity->AI_permanentSpecialistValue((SpecialistTypes)j)/4;
					iValue = std::max(iValue, iTempValue);
				}
			}
		}
		// value of building something.
		for (int j = 0; j < GC.getNumBuildingClassInfos(); j++)
		{
			BuildingTypes eBuilding = (BuildingTypes)GC.getCivilizationInfo(getCivilizationType()).getCivilizationBuildings(j);

			if (eBuilding != NO_BUILDING)
			{
				if (kInfo.getForceBuildings(eBuilding) ||
					(kInfo.getBuildings(eBuilding) && canConstruct(eBuilding, false, false, true)))
				{
					for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
					{
						// cf. conditions used in CvUnit::canConstruct
						if (pLoopCity->getNumRealBuilding(eBuilding) > 0 ||
							(!kInfo.getForceBuildings(eBuilding) && !pLoopCity->canConstruct(eBuilding, false, false, true)))
						{
							continue;
						}

						// Note, building value is roughly 4x the value of the commerce it provides.
						// so we * 25 to match the scale of specialist value.
						int iTempValue = pLoopCity->AI_buildingValue(eBuilding) * 25;

						// if the building is a world wonder, increase the value.
						if (isWorldWonderClass((BuildingClassTypes)j))
							iTempValue = 3*iTempValue/2;

						// reduce the value if we already have great people of this type
						iTempValue /= 1 + AI_totalUnitAIs((UnitAITypes)kInfo.getDefaultUnitAIType());

						iValue = std::max(iValue, iTempValue);
					}
				}
			}
		}
		// don't bother trying to evaluate bulbing etc. That kind of value is too volatile anyway.

		// store the value in the weights map - but remember, this isn't yet the actual weight.
		FAssert(iValue >= 0);
		m_GreatPersonWeights[eGreatPersonClass] = iValue;
	}

	// find the mean value.
	int iSum = 0;

	std::map<UnitClassTypes, int>::iterator it;
	for (it = m_GreatPersonWeights.begin(); it != m_GreatPersonWeights.end(); ++it)
	{
		iSum += it->second;
	}
	int iMean = iSum / std::max(1, (int)m_GreatPersonWeights.size());

	// scale the values so that they are between 50 and 500 (depending on flavour), with the mean value translating to 100.
	// (note: I don't expect it to get anywhere near the maximum. The maximum only occurs when the value is infinite!)
	const int iMin = AI_getFlavorValue(FLAVOR_SCIENCE) == 0 ? 50 : 35 - AI_getFlavorValue(FLAVOR_SCIENCE);
	FAssert(iMin > 10 && iMin < 95);
	// (smaller iMin means more focus on high-value specialists)
	for (it = m_GreatPersonWeights.begin(); it != m_GreatPersonWeights.end(); ++it)
	{
		int iValue = it->second;
		iValue = 100 * iValue / std::max(1, iMean);
		//iValue = (40000 + 500 * iValue) / (800 + iValue);
		iValue = (800*iMin + (9*100-8*iMin) * iValue) / (800 + iValue);
		it->second = iValue;
	}
}

int CvPlayerAI::AI_getGreatPersonWeight(UnitClassTypes eGreatPerson) const
{
	std::map<UnitClassTypes, int>::const_iterator it = m_GreatPersonWeights.find(eGreatPerson);
	if (it == m_GreatPersonWeights.end())
		return 100;
	else
		return it->second;
}
// K-Mod end

void CvPlayerAI::AI_nowHasTech(TechTypes eTech)
{
	// while its _possible_ to do checks, for financial trouble, and this tech adds financial buildings
	// if in war and this tech adds important war units
	// etc
	// it makes more sense to just redetermine what to produce
	// that is already done every time a civ meets a new civ, it makes sense to do it when a new tech is learned
	// if this is changed, then at a minimum, AI_isFinancialTrouble should be checked
	if (!isHuman())
	{
		int iGameTurn = GC.getGameINLINE().getGameTurn();
		
		// only reset at most every 10 turns
		if (iGameTurn > m_iTurnLastProductionDirty + 10)
		{
			// redeterimine the best things to build in each city
			AI_makeProductionDirty();
		
			m_iTurnLastProductionDirty = iGameTurn;
		}
	}

}


int CvPlayerAI::AI_countDeadlockedBonuses(CvPlot* pPlot) const
{
    CvPlot* pLoopPlot;
    CvPlot* pLoopPlot2;
    int iDX, iDY;
    int iI;
    
    int iMinRange = GC.getMIN_CITY_RANGE();
    int iRange = iMinRange * 2;
    int iCount = 0;

    for (iDX = -(iRange); iDX <= iRange; iDX++)
    {
        for (iDY = -(iRange); iDY <= iRange; iDY++)
        {
            if (plotDistance(iDX, iDY, 0, 0) > CITY_PLOTS_RADIUS)
            {
                pLoopPlot = plotXY(pPlot->getX_INLINE(), pPlot->getY_INLINE(), iDX, iDY);

                if (pLoopPlot != NULL)
                {
                    if (pLoopPlot->getBonusType(getTeam()) != NO_BONUS)
                    {
                        if (!pLoopPlot->isCityRadius() && ((pLoopPlot->area() == pPlot->area()) || pLoopPlot->isWater()))
                        {
                            bool bCanFound = false;
                            bool bNeverFound = true;
                            //potentially blockable resource
                            //look for a city site within a city radius
                            for (iI = 0; iI < NUM_CITY_PLOTS; iI++)
                            {
                                pLoopPlot2 = plotCity(pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE(), iI);
                                if (pLoopPlot2 != NULL)
                                {
                                    //canFound usually returns very quickly
                                    if (canFound(pLoopPlot2->getX_INLINE(), pLoopPlot2->getY_INLINE(), false))
                                    {
                                        bNeverFound = false;
                                        if (stepDistance(pPlot->getX_INLINE(), pPlot->getY_INLINE(), pLoopPlot2->getX_INLINE(), pLoopPlot2->getY_INLINE()) > iMinRange)
                                        {
                                            bCanFound = true;
                                            break;
                                        }
                                    }
                                }
                            }
                            if (!bNeverFound && !bCanFound)
                            {
                                iCount++;
                            }
                        }
                    }
                }
            }
        }
    }
    
    return iCount;
}

#if 0 // K-Mod disabled these functions.
int CvPlayerAI::AI_getOurPlotStrength(CvPlot* pPlot, int iRange, bool bDefensiveBonuses, bool bTestMoves) const
{
	PROFILE_FUNC();

	CLLNode<IDInfo>* pUnitNode;
	CvUnit* pLoopUnit;
	CvPlot* pLoopPlot;
	int iValue;
	int iDistance;
	int iDX, iDY;

	iValue = 0;

	for (iDX = -(iRange); iDX <= iRange; iDX++)
	{
		for (iDY = -(iRange); iDY <= iRange; iDY++)
		{
			pLoopPlot = plotXY(pPlot->getX_INLINE(), pPlot->getY_INLINE(), iDX, iDY);

			if (pLoopPlot != NULL)
			{
				if (pLoopPlot->area() == pPlot->area())
				{
				    iDistance = stepDistance(pPlot->getX_INLINE(), pPlot->getY_INLINE(), pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE());
					pUnitNode = pLoopPlot->headUnitNode();

					while (pUnitNode != NULL)
					{
						pLoopUnit = ::getUnit(pUnitNode->m_data);
						pUnitNode = pLoopPlot->nextUnitNode(pUnitNode);

						if (pLoopUnit->getOwnerINLINE() == getID())
						{
							if ((bDefensiveBonuses && pLoopUnit->canDefend()) || pLoopUnit->canAttack())
							{
								if (!(pLoopUnit->isInvisible(getTeam(), false)))
								{
								    if (pLoopUnit->atPlot(pPlot) || pLoopUnit->canMoveInto(pPlot) || pLoopUnit->canMoveInto(pPlot, /*bAttack*/ true))
								    {
                                        if (!bTestMoves)
                                        {
                                        	iValue += pLoopUnit->currEffectiveStr((bDefensiveBonuses ? pPlot : NULL), NULL);
                                        }
                                        else
                                        {
											if (pLoopUnit->baseMoves() >= iDistance)
                                            {
                                                iValue += pLoopUnit->currEffectiveStr((bDefensiveBonuses ? pPlot : NULL), NULL);
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


	return iValue;
}

int CvPlayerAI::AI_getEnemyPlotStrength(CvPlot* pPlot, int iRange, bool bDefensiveBonuses, bool bTestMoves) const
{
	PROFILE_FUNC();

	CLLNode<IDInfo>* pUnitNode;
	CvUnit* pLoopUnit;
	CvPlot* pLoopPlot;
	int iValue;
	int iDistance;
	int iDX, iDY;

	iValue = 0;

	for (iDX = -(iRange); iDX <= iRange; iDX++)
	{
		for (iDY = -(iRange); iDY <= iRange; iDY++)
		{
			pLoopPlot = plotXY(pPlot->getX_INLINE(), pPlot->getY_INLINE(), iDX, iDY);

			if (pLoopPlot != NULL)
			{
				if (pLoopPlot->area() == pPlot->area())
				{
				    iDistance = stepDistance(pPlot->getX_INLINE(), pPlot->getY_INLINE(), pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE());
					pUnitNode = pLoopPlot->headUnitNode();

					while (pUnitNode != NULL)
					{
						pLoopUnit = ::getUnit(pUnitNode->m_data);
						pUnitNode = pLoopPlot->nextUnitNode(pUnitNode);

						if (atWar(pLoopUnit->getTeam(), getTeam()))
						{
							if ((bDefensiveBonuses && pLoopUnit->canDefend()) || pLoopUnit->canAttack())
							{
								if (!(pLoopUnit->isInvisible(getTeam(), false)))
								{
								    if (pPlot->isValidDomainForAction(*pLoopUnit))
								    {
                                        if (!bTestMoves)
                                        {
                                            iValue += pLoopUnit->currEffectiveStr((bDefensiveBonuses ? pPlot : NULL), NULL);
                                        }
                                        else
                                        {
                                            int iDangerRange = pLoopUnit->baseMoves();
                                            iDangerRange += ((pLoopPlot->isValidRoute(pLoopUnit)) ? 1 : 0);
                                            if (iDangerRange >= iDistance)
                                            {
                                                iValue += pLoopUnit->currEffectiveStr((bDefensiveBonuses ? pPlot : NULL), NULL);
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


	return iValue;
	
}
#endif // K-Mod disabled these functions

// K-Mod. This function use to be the bulk of AI_goldToUpgradeAllUnits()
void CvPlayerAI::AI_updateGoldToUpgradeAllUnits()
{
	/*if (m_iUpgradeUnitsCacheTurn == GC.getGameINLINE().getGameTurn() && m_iUpgradeUnitsCachedExpThreshold == iExpThreshold)
	{
		return m_iUpgradeUnitsCachedGold;
	}*/

	int iTotalGold = 0;
	
	CvCivilizationInfo& kCivilizationInfo = GC.getCivilizationInfo(getCivilizationType());
	
	// cache the value for each unit type
	std::vector<int> aiUnitUpgradePrice(GC.getNumUnitInfos(), 0);	// initializes to zeros

	int iLoop;
	for (CvUnit* pLoopUnit = firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = nextUnit(&iLoop))
	{
		/*
		// if experience is below threshold, skip this unit
		if (pLoopUnit->getExperience() < iExpThreshold)
		{
			continue;
		}*/
		
		UnitTypes eUnitType = pLoopUnit->getUnitType();

		// check cached value for this unit type
		int iCachedUnitGold = aiUnitUpgradePrice[eUnitType];
		if (iCachedUnitGold != 0)
		{
			// if positive, add it to the sum
			if (iCachedUnitGold > 0)
			{
				iTotalGold += iCachedUnitGold;
			}
			
			// either way, done with this unit
			continue;
		}
		
		int iUnitGold = 0;
		int iUnitUpgradePossibilities = 0;
		
		UnitAITypes eUnitAIType = pLoopUnit->AI_getUnitAIType();
		CvArea* pUnitArea = pLoopUnit->area();
		int iUnitValue = AI_unitValue(eUnitType, eUnitAIType, pUnitArea);

		for (int iI = 0; iI < GC.getNumUnitClassInfos(); iI++)
		{
			UnitClassTypes eUpgradeUnitClassType = (UnitClassTypes) iI;
			UnitTypes eUpgradeUnitType = (UnitTypes)(kCivilizationInfo.getCivilizationUnits(iI));

			if (NO_UNIT != eUpgradeUnitType)
			{
				// is it better?
				int iUpgradeValue = AI_unitValue(eUpgradeUnitType, eUnitAIType, pUnitArea);
				if (iUpgradeValue > iUnitValue)
				{
					// is this a valid upgrade?
					if (pLoopUnit->upgradeAvailable(eUnitType, eUpgradeUnitClassType))
					{
						// can we actually make this upgrade?
						bool bCanUpgrade = false;
						CvCity* pCapitalCity = getCapitalCity();
						if (pCapitalCity != NULL && pCapitalCity->canTrain(eUpgradeUnitType))
						{
							bCanUpgrade = true;
						}
						else
						{
							CvCity* pCloseCity = GC.getMapINLINE().findCity(pLoopUnit->getX_INLINE(), pLoopUnit->getY_INLINE(), getID(), NO_TEAM, true, (pLoopUnit->getDomainType() == DOMAIN_SEA));
							if (pCloseCity != NULL && pCloseCity->canTrain(eUpgradeUnitType))
							{
								bCanUpgrade = true;
							}
						}

						if (bCanUpgrade)
						{
							iUnitGold += pLoopUnit->upgradePrice(eUpgradeUnitType);
							iUnitUpgradePossibilities++; 
						}
					}
				}
			}
		}
		
		// if we found any, find average and add to total
		if (iUnitUpgradePossibilities > 0)
		{
			iUnitGold /= iUnitUpgradePossibilities;

			// add to cache
			aiUnitUpgradePrice[eUnitType] = iUnitGold;

			// add to sum
			iTotalGold += iUnitGold;
		}
		else
		{
			// add to cache, dont upgrade to this type
			aiUnitUpgradePrice[eUnitType] = -1;
		}
	}

	/*m_iUpgradeUnitsCacheTurn = GC.getGameINLINE().getGameTurn();
	m_iUpgradeUnitsCachedExpThreshold = iExpThreshold; */
	m_iUpgradeUnitsCachedGold = iTotalGold;

	//return iTotalGold;
}

int CvPlayerAI::AI_goldTradeValuePercent() const
{
	int iValue = 2;
	if (AI_isFinancialTrouble())
	{
		iValue += 1;
	}
	return 100 * iValue;
	
}

int CvPlayerAI::AI_averageYieldMultiplier(YieldTypes eYield) const
{
	FAssert(eYield > -1);
	FAssert(eYield < NUM_YIELD_TYPES);
	
	/*if (m_iAveragesCacheTurn != GC.getGameINLINE().getGameTurn())
	{
		AI_calculateAverages();
	}*/
	
	FAssert(m_aiAverageYieldMultiplier[eYield] > 0);
	return m_aiAverageYieldMultiplier[eYield];
}

int CvPlayerAI::AI_averageCommerceMultiplier(CommerceTypes eCommerce) const
{
	FAssert(eCommerce > -1);
	FAssert(eCommerce < NUM_COMMERCE_TYPES);
	
	/*if (m_iAveragesCacheTurn != GC.getGameINLINE().getGameTurn())
	{
		AI_calculateAverages();
	}*/
	
	return m_aiAverageCommerceMultiplier[eCommerce];	
}

int CvPlayerAI::AI_averageGreatPeopleMultiplier() const
{
	/*if (m_iAveragesCacheTurn != GC.getGameINLINE().getGameTurn())
	{
		AI_calculateAverages();
	}*/
	return m_iAverageGreatPeopleMultiplier;	
}

// K-Mod
int CvPlayerAI::AI_averageCulturePressure() const
{
	/*if (m_iAveragesCacheTurn != GC.getGameINLINE().getGameTurn())
	{
		AI_calculateAverages();
	}*/
	return m_iAverageCulturePressure;	
}

//"100 eCommerce is worth (return) raw YIELD_COMMERCE
int CvPlayerAI::AI_averageCommerceExchange(CommerceTypes eCommerce) const
{
	FAssert(eCommerce > -1);
	FAssert(eCommerce < NUM_COMMERCE_TYPES);
	
	/*if (m_iAveragesCacheTurn != GC.getGameINLINE().getGameTurn())
	{
		AI_calculateAverages();
	}*/
	
	return m_aiAverageCommerceExchange[eCommerce];
}

void CvPlayerAI::AI_calculateAverages()
{
	CvCity* pLoopCity;
	int iLoop;

	for (int iI = 0; iI < NUM_YIELD_TYPES; iI++)
	{
		m_aiAverageYieldMultiplier[iI] = 0;		
	}
	for (int iI = 0; iI < NUM_COMMERCE_TYPES; iI++)
	{
		m_aiAverageCommerceMultiplier[iI] = 0;
	}
	m_iAverageGreatPeopleMultiplier = 0;

	int iTotalPopulation = 0;

	for (pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		int iPopulation = std::max(pLoopCity->getPopulation(), NUM_CITY_PLOTS);
		iTotalPopulation += iPopulation;
			
		for (int iI = 0; iI < NUM_YIELD_TYPES; iI++)
		{
			m_aiAverageYieldMultiplier[iI] += iPopulation * pLoopCity->AI_yieldMultiplier((YieldTypes)iI);
		}
		for (int iI = 0; iI < NUM_COMMERCE_TYPES; iI++)
		{
			m_aiAverageCommerceMultiplier[iI] += iPopulation * pLoopCity->getTotalCommerceRateModifier((CommerceTypes)iI);
		}
		m_iAverageGreatPeopleMultiplier += iPopulation * pLoopCity->getTotalGreatPeopleRateModifier();
	}


	if (iTotalPopulation > 0)
	{
		for (int iI = 0; iI < NUM_YIELD_TYPES; iI++)
		{
			m_aiAverageYieldMultiplier[iI] /= iTotalPopulation;
			FAssert(m_aiAverageYieldMultiplier[iI] > 0);
		}
		for (int iI = 0; iI < NUM_COMMERCE_TYPES; iI++)
		{
			m_aiAverageCommerceMultiplier[iI] /= iTotalPopulation;
			FAssert(m_aiAverageCommerceMultiplier[iI] > 0);	
		}
		m_iAverageGreatPeopleMultiplier /= iTotalPopulation;
		FAssert(m_iAverageGreatPeopleMultiplier > 0);
	}
	else
	{
		for (int iI = 0; iI < NUM_YIELD_TYPES; iI++)
		{
			m_aiAverageYieldMultiplier[iI] = 100;
		}
		for (int iI = 0; iI < NUM_COMMERCE_TYPES; iI++)
		{
			m_aiAverageCommerceMultiplier[iI] = 100;
		}
		m_iAverageGreatPeopleMultiplier = 100;
	}


	//Calculate Exchange Rate

	for (int iI = 0; iI < NUM_COMMERCE_TYPES; iI++)
	{
		m_aiAverageCommerceExchange[iI] = 0;		
	}

	int iCommerce = 0;
	int iTotalCommerce = 0;

	for (pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		iCommerce = pLoopCity->getYieldRate(YIELD_COMMERCE);
		iTotalCommerce += iCommerce;
		
		int iExtraCommerce = 0;
		for (int iI = 0; iI < NUM_COMMERCE_TYPES; iI++)
		{
			iExtraCommerce +=((pLoopCity->getSpecialistPopulation() + pLoopCity->getNumGreatPeople()) * getSpecialistExtraCommerce((CommerceTypes)iI));
			iExtraCommerce += (pLoopCity->getBuildingCommerce((CommerceTypes)iI) + pLoopCity->getSpecialistCommerce((CommerceTypes)iI) + pLoopCity->getReligionCommerce((CommerceTypes)iI) + getFreeCityCommerce((CommerceTypes)iI));
		}
		iTotalCommerce += iExtraCommerce;
		
		for (int iI = 0; iI < NUM_COMMERCE_TYPES; iI++)
		{
			m_aiAverageCommerceExchange[iI] += ((iCommerce + iExtraCommerce) * pLoopCity->getTotalCommerceRateModifier((CommerceTypes)iI)) / 100;		
		}
	}

	for (int iI = 0; iI < NUM_COMMERCE_TYPES; iI++)
	{
		if (m_aiAverageCommerceExchange[iI] > 0)
		{
			m_aiAverageCommerceExchange[iI] = (100 * iTotalCommerce) / m_aiAverageCommerceExchange[iI];
		}
		else
		{
			m_aiAverageCommerceExchange[iI] = 100;
		}
	}

	// K-Mod Culture pressure

	int iTotal = 0;
	int iWeightedTotal = 0;

	for (pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		int iCultureRate = pLoopCity->getCommerceRateTimes100(COMMERCE_CULTURE);
		iTotal += iCultureRate;
		iWeightedTotal += iCultureRate * pLoopCity->culturePressureFactor();
	}
	if (iTotal == 0)
		m_iAverageCulturePressure = 100;
	else
		m_iAverageCulturePressure = iWeightedTotal / iTotal;

	//m_iAveragesCacheTurn = GC.getGameINLINE().getGameTurn();
}

// K-Mod edition
void CvPlayerAI::AI_convertUnitAITypesForCrush()
{
	int iLoop;

	std::map<int, int> spare_units;
	std::vector<std::pair<int, int> > unit_list; // { score, unitID }.
	// note unitID is used rather than CvUnit* to ensure that the list gives the same order for players on different computers.

	for (CvArea *pLoopArea = GC.getMapINLINE().firstArea(&iLoop); pLoopArea != NULL; pLoopArea = GC.getMapINLINE().nextArea(&iLoop))
	{
		// Keep 1/2 of recommended floating defenders.
		if (!pLoopArea || pLoopArea->getAreaAIType(getTeam()) == AREAAI_ASSAULT
			|| pLoopArea->getAreaAIType(getTeam()) == AREAAI_DEFENSIVE)
		{
			spare_units[pLoopArea->getID()] = 0;
		}
		else
		{
			spare_units[pLoopArea->getID()] = (2 * AI_getTotalFloatingDefenders(pLoopArea) - AI_getTotalFloatingDefendersNeeded(pLoopArea))/2;
		}
	}
	
	for (CvUnit* pLoopUnit = firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = nextUnit(&iLoop))
	{
		bool bValid = false;
		const CvPlot* pPlot = pLoopUnit->plot();

		/* if (pLoopUnit->AI_getUnitAIType() == UNITAI_RESERVE
			|| pLoopUnit->AI_isCityAIType() && (pLoopUnit->getExtraCityDefensePercent() <= 0)) */
		// K-Mod, protective leaders might still want to use their gunpowder units...
		if (pLoopUnit->AI_getUnitAIType() == UNITAI_RESERVE || pLoopUnit->AI_getUnitAIType() == UNITAI_COLLATERAL
			|| (pLoopUnit->AI_isCityAIType() && pLoopUnit->getExtraCityDefensePercent() <= 30))
		{
			bValid = true;
		}

		/*if ((pLoopUnit->area()->getAreaAIType(getTeam()) == AREAAI_ASSAULT)
			|| (pLoopUnit->area()->getAreaAIType(getTeam()) == AREAAI_DEFENSIVE))
		{
			bValid = false;
		}*/
		
		if (!pLoopUnit->canAttack() || (pLoopUnit->AI_getUnitAIType() == UNITAI_CITY_SPECIAL))
		{
			bValid = false;
		}

		if (spare_units[pLoopUnit->area()->getID()] <= 0)
			bValid = false;
	
		if (bValid)
		{
			if (pPlot->isCity())
			{
				if (pPlot->getPlotCity()->getOwner() == getID())
				{
					if (pPlot->getBestDefender(getID()) == pLoopUnit)
					{
						bValid = false;
					}
				}
				/*if (pLoopUnit->AI_getUnitAIType() == UNITAI_CITY_DEFENSE && GET_TEAM(getTeam()).getAtWarCount(true) > 0)
				{
					int iCityDefenders = pLoopUnit->plot()->plotCount(PUF_isUnitAIType, UNITAI_CITY_DEFENSE, -1, getID());

					if (iCityDefenders <= pLoopUnit->plot()->getPlotCity()->AI_minDefenders())
						bValid = false;
				}*/
				if (pLoopUnit->getGroup()->AI_getMissionAIType() == MISSIONAI_GUARD_CITY)
				{
					int iDefendersHere = pPlot->plotCount(PUF_isMissionAIType, MISSIONAI_GUARD_CITY, -1, getID());
					int iDefendersWant = pLoopUnit->area()->getAreaAIType(getTeam()) == AREAAI_DEFENSIVE
						? pPlot->getPlotCity()->AI_neededDefenders()
						: pPlot->getPlotCity()->AI_minDefenders();

					if (iDefendersHere <= iDefendersWant)
						bValid = false;
				}
			}
		}
		
		if (bValid)
		{
			int iValue = AI_unitValue(pLoopUnit->getUnitType(), UNITAI_ATTACK_CITY, pLoopUnit->area());
			unit_list.push_back(std::make_pair(iValue, pLoopUnit->getID()));
		}
	}

	// convert the highest scoring units first.
	std::sort(unit_list.begin(), unit_list.end(), std::greater<std::pair<int, int> >());
	std::vector<std::pair<int, int> >::iterator it;
	for (it = unit_list.begin(); it != unit_list.end(); ++it)
	{
		if (it->first > 0 && spare_units[getUnit(it->second)->area()->getID()] > 0)
		{
			if (gPlayerLogLevel >= 2)
			{
				CvWString sOldType;
				getUnitAIString(sOldType, getUnit(it->second)->AI_getUnitAIType());
				logBBAI("    %S converts %S from %S to attack city for crush. (%d)", getName(), getUnit(it->second)->getName().GetCString(), sOldType.GetCString(), getUnit(it->second)->getID());
			}

			getUnit(it->second)->AI_setUnitAIType(UNITAI_ATTACK_CITY);
			// only convert half of our spare units, so that we can reevaluate which units we need before converting more.
			spare_units[getUnit(it->second)->area()->getID()]-=2;
		}
	}
}

int CvPlayerAI::AI_playerCloseness(PlayerTypes eIndex, int iMaxDistance) const
{
	PROFILE_FUNC();	
	FAssert(GET_PLAYER(eIndex).isAlive());
	FAssert(eIndex != getID());
	
	int iValue = 0;
	int iLoop;
	for (CvCity* pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		iValue += pLoopCity->AI_playerCloseness(eIndex, iMaxDistance);
	}
	
	return iValue;
}


int CvPlayerAI::AI_getTotalAreaCityThreat(CvArea* pArea) const
{
	PROFILE_FUNC();
	CvCity* pLoopCity;
	int iLoop;
	int iValue;
	
	iValue = 0;
	for (pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		if (pLoopCity->getArea() == pArea->getID())
		{
			iValue += pLoopCity->AI_cityThreat();
		}
	}
	return iValue;
}

int CvPlayerAI::AI_countNumAreaHostileUnits(CvArea* pArea, bool bPlayer, bool bTeam, bool bNeutral, bool bHostile) const
{
	PROFILE_FUNC();
	CvPlot* pLoopPlot;
	int iCount;
	int iI;

	iCount = 0;

	for (iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
	{
		pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);
		if ((pLoopPlot->area() == pArea) && pLoopPlot->isVisible(getTeam(), false) && 
			((bPlayer && pLoopPlot->getOwnerINLINE() == getID()) || (bTeam && pLoopPlot->getTeam() == getTeam()) 
				|| (bNeutral && !pLoopPlot->isOwned()) || (bHostile && pLoopPlot->isOwned() && GET_TEAM(getTeam()).isAtWar(pLoopPlot->getTeam()))))
			{
			iCount += pLoopPlot->plotCount(PUF_isEnemy, getID(), false, NO_PLAYER, NO_TEAM, PUF_isVisible, getID());
		}
	}
	return iCount;
}

//this doesn't include the minimal one or two garrison units in each city.
int CvPlayerAI::AI_getTotalFloatingDefendersNeeded(CvArea* pArea) const
{
	PROFILE_FUNC();
	int iDefenders;
	int iCurrentEra = getCurrentEra();
	int iAreaCities = pArea->getCitiesPerPlayer(getID());
	
	iCurrentEra = std::max(0, iCurrentEra - GC.getGame().getStartEra() / 2);
	
	/* original bts code
	iDefenders = 1 + ((iCurrentEra + ((GC.getGameINLINE().getMaxCityElimination() > 0) ? 3 : 2)) * iAreaCities);
	iDefenders /= 3;
	iDefenders += pArea->getPopulationPerPlayer(getID()) / 7; */
	// K-Mod
	iDefenders = 1 + iAreaCities + AI_totalAreaUnitAIs(pArea, UNITAI_SETTLE);
	iDefenders += pArea->getPopulationPerPlayer(getID()) / 7;
	iDefenders *= iCurrentEra + (GC.getGameINLINE().getMaxCityElimination() > 0 ? 3 : 2);
	iDefenders /= 3;
	// K-Mod end

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      04/01/10                                jdog5000      */
/*                                                                                              */
/* War strategy AI, Victory Strategy AI                                                         */
/************************************************************************************************/
	if( pArea->getAreaAIType(getTeam()) == AREAAI_DEFENSIVE || pArea->getAreaAIType(getTeam()) == AREAAI_OFFENSIVE || pArea->getAreaAIType(getTeam()) == AREAAI_MASSING )
	{
		if( iAreaCities <= std::min(4, pArea->getNumCities()/3) )
		{
			// Land war here, as floating defenders are based on cities/population need to make sure
			// AI defends its footholds in new continents well.
			iDefenders += GET_TEAM(getTeam()).countEnemyPopulationByArea(pArea) / 14;
		}
	}

	if (pArea->getAreaAIType(getTeam()) == AREAAI_DEFENSIVE)
	{
		iDefenders *= 2;
	}
	else 
	{
		if( AI_isDoStrategy(AI_STRATEGY_ALERT2) )
		{
			iDefenders *= 2;
		}
		else if( AI_isDoStrategy(AI_STRATEGY_ALERT1) )
		{
			iDefenders *= 3;
			iDefenders /= 2;
		}
		else if (pArea->getAreaAIType(getTeam()) == AREAAI_OFFENSIVE)
		{
			iDefenders *= 2;
			iDefenders /= 3;
		}
		else if (pArea->getAreaAIType(getTeam()) == AREAAI_MASSING)
		{
			if( GET_TEAM(getTeam()).AI_getEnemyPowerPercent(true) < (10 + GC.getLeaderHeadInfo(getPersonalityType()).getMaxWarNearbyPowerRatio()) )
			{
				iDefenders *= 2;
				iDefenders /= 3;
			}
		}
	}
	
	if (AI_getTotalAreaCityThreat(pArea) == 0)
	{
		iDefenders /= 2;
	}
	
	if (!GC.getGameINLINE().isOption(GAMEOPTION_AGGRESSIVE_AI))
	{
		iDefenders *= 2;
		iDefenders /= 3;
	}

	// Removed AI_STRATEGY_GET_BETTER_UNITS reduction, it was reducing defenses twice
	
	if (AI_isDoVictoryStrategy(AI_VICTORY_CULTURE3))
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
	{
		iDefenders += 2 * iAreaCities;
		//if (pArea->getAreaAIType(getTeam()) == AREAAI_DEFENSIVE)
		if (pArea->getAreaAIType(getTeam()) == AREAAI_DEFENSIVE && AI_isDoVictoryStrategy(AI_VICTORY_CULTURE4)) // K-Mod
		{
			iDefenders *= 2; //go crazy
		}
	}
	
	iDefenders *= 60;
	iDefenders /= std::max(30, (GC.getHandicapInfo(GC.getGameINLINE().getHandicapType()).getAITrainPercent() - 20));
	
	if ((iCurrentEra < 3) && (GC.getGameINLINE().isOption(GAMEOPTION_RAGING_BARBARIANS)))
	{
		iDefenders += 2;
	}
	
	if (getCapitalCity() != NULL)
	{
		if (getCapitalCity()->area() != pArea)
		{

/************************************************************************************************/
/* UNOFFICIAL_PATCH                       01/23/09                                jdog5000      */
/*                                                                                              */
/* Bugfix, War tactics AI                                                                       */
/************************************************************************************************/
/* original BTS code
			//Defend offshore islands only lightly.
			iDefenders = std::min(iDefenders, iAreaCities * iAreaCities - 1);
*/
			// Lessen defensive requirements only if not being attacked locally
			if( pArea->getAreaAIType(getTeam()) != AREAAI_DEFENSIVE )
			{
				// This may be our first city captured on a large enemy continent, need defenses to scale up based
				// on total number of area cities not just ours
				iDefenders = std::min(iDefenders, iAreaCities * iAreaCities + pArea->getNumCities() - iAreaCities - 1);
			}
/************************************************************************************************/
/* UNOFFICIAL_PATCH                        END                                                  */
/************************************************************************************************/
		}
	}
	
	return iDefenders;
}

int CvPlayerAI::AI_getTotalFloatingDefenders(CvArea* pArea) const
{
	PROFILE_FUNC();
	int iCount = 0;

	iCount += AI_totalAreaUnitAIs(pArea, UNITAI_COLLATERAL);
	iCount += AI_totalAreaUnitAIs(pArea, UNITAI_RESERVE);
	iCount += std::max(0, (AI_totalAreaUnitAIs(pArea, UNITAI_CITY_DEFENSE) - (pArea->getCitiesPerPlayer(getID()) * 2)));
	iCount += AI_totalAreaUnitAIs(pArea, UNITAI_CITY_COUNTER);
	iCount += AI_totalAreaUnitAIs(pArea, UNITAI_CITY_SPECIAL);
	return iCount;
}

RouteTypes CvPlayerAI::AI_bestAdvancedStartRoute(CvPlot* pPlot, int* piYieldValue) const
{
	RouteTypes eBestRoute = NO_ROUTE;
	int iBestValue = -1;
    for (int iI = 0; iI < GC.getNumRouteInfos(); iI++)
    {
        RouteTypes eRoute = (RouteTypes)iI;

		int iValue = 0;
		int iCost = getAdvancedStartRouteCost(eRoute, true, pPlot);
		
		if (iCost >= 0)
		{
			iValue += GC.getRouteInfo(eRoute).getValue();
									
			if (iValue > 0)
			{
				int iYieldValue = 0;
				if (pPlot->getImprovementType() != NO_IMPROVEMENT)
				{
					iYieldValue += ((GC.getImprovementInfo(pPlot->getImprovementType()).getRouteYieldChanges(eRoute, YIELD_FOOD)) * 100);
					iYieldValue += ((GC.getImprovementInfo(pPlot->getImprovementType()).getRouteYieldChanges(eRoute, YIELD_PRODUCTION)) * 60);
					iYieldValue += ((GC.getImprovementInfo(pPlot->getImprovementType()).getRouteYieldChanges(eRoute, YIELD_COMMERCE)) * 40);
				}
				iValue *= 1000;
				iValue /= (1 + iCost);

				if (iValue > iBestValue)
				{
					iBestValue = iValue;
					eBestRoute = eRoute;
					if (piYieldValue != NULL)
					{
						*piYieldValue = iYieldValue;
					}
				}
			}
		}
	}
	return eBestRoute;
}

UnitTypes CvPlayerAI::AI_bestAdvancedStartUnitAI(CvPlot* pPlot, UnitAITypes eUnitAI) const
{
	UnitTypes eLoopUnit;
	UnitTypes eBestUnit;
	int iValue;
	int iBestValue;
	int iI, iJ, iK;
	
	FAssertMsg(eUnitAI != NO_UNITAI, "UnitAI is not assigned a valid value");

	iBestValue = 0;
	eBestUnit = NO_UNIT;

	for (iI = 0; iI < GC.getNumUnitClassInfos(); iI++)
	{
		eLoopUnit = ((UnitTypes)(GC.getCivilizationInfo(getCivilizationType()).getCivilizationUnits(iI)));

		if (eLoopUnit != NO_UNIT)
		{
			//if (!isHuman() || (GC.getUnitInfo(eLoopUnit).getDefaultUnitAIType() == eUnitAI))
			{
				int iUnitCost = getAdvancedStartUnitCost(eLoopUnit, true, pPlot);
				if (iUnitCost >= 0)
				{
					iValue = AI_unitValue(eLoopUnit, eUnitAI, pPlot->area());

					if (iValue > 0)
					{
						//free promotions. slow?
						//only 1 promotion per source is counted (ie protective isn't counted twice)
						int iPromotionValue = 0;

						//special to the unit
						for (iJ = 0; iJ < GC.getNumPromotionInfos(); iJ++)
						{
							if (GC.getUnitInfo(eLoopUnit).getFreePromotions(iJ))
							{
								iPromotionValue += 15;
								break;
							}
						}

						for (iK = 0; iK < GC.getNumPromotionInfos(); iK++)
						{
							if (isFreePromotion((UnitCombatTypes)GC.getUnitInfo(eLoopUnit).getUnitCombatType(), (PromotionTypes)iK))
							{
								iPromotionValue += 15;
								break;
							}

							if (isFreePromotion((UnitClassTypes)GC.getUnitInfo(eLoopUnit).getUnitClassType(), (PromotionTypes)iK))
							{
								iPromotionValue += 15;
								break;
							}
						}

						//traits
						for (iJ = 0; iJ < GC.getNumTraitInfos(); iJ++)
						{
							if (hasTrait((TraitTypes)iJ))
							{
								for (iK = 0; iK < GC.getNumPromotionInfos(); iK++)
								{
									if (GC.getTraitInfo((TraitTypes) iJ).isFreePromotion(iK))
									{
										if ((GC.getUnitInfo(eLoopUnit).getUnitCombatType() != NO_UNITCOMBAT) && GC.getTraitInfo((TraitTypes) iJ).isFreePromotionUnitCombat(GC.getUnitInfo(eLoopUnit).getUnitCombatType()))
										{
											iPromotionValue += 15;
											break;
										}
									}
								}
							}
						}

						iValue *= (iPromotionValue + 100);
						iValue /= 100;

						iValue *= (GC.getGameINLINE().getSorenRandNum(40, "AI Best Advanced Start Unit") + 100);
						iValue /= 100;

						iValue *= (getNumCities() + 2);
						iValue /= (getUnitClassCountPlusMaking((UnitClassTypes)iI) + getNumCities() + 2);

						FAssert((MAX_INT / 1000) > iValue);
						iValue *= 1000;
						
						iValue /= 1 + iUnitCost;

						iValue = std::max(1, iValue);

						if (iValue > iBestValue)
						{
							iBestValue = iValue;
							eBestUnit = eLoopUnit;
						}
					}
				}
			}
		}
	}

	return eBestUnit;
}

CvPlot* CvPlayerAI::AI_advancedStartFindCapitalPlot()
{
	CvPlot* pBestPlot = NULL;
	int iBestValue = -1;
	
	for (int iPlayer = 0; iPlayer < MAX_PLAYERS; iPlayer++)
	{
		CvPlayer& kPlayer = GET_PLAYER((PlayerTypes)iPlayer);
		if (kPlayer.isAlive())
		{
			if (kPlayer.getTeam() == getTeam())
			{
				CvPlot* pLoopPlot = kPlayer.getStartingPlot();
				if (pLoopPlot != NULL)
				{
					if (getAdvancedStartCityCost(true, pLoopPlot) > 0)
					{
					int iX = pLoopPlot->getX_INLINE();
					int iY = pLoopPlot->getY_INLINE();
						
						int iValue = 1000;
						if (iPlayer == getID())
						{
							iValue += 1000;
						}
						else
						{
							iValue += GC.getGame().getSorenRandNum(100, "AI Advanced Start Choose Team Start");
						}
						CvCity * pNearestCity = GC.getMapINLINE().findCity(iX, iY, NO_PLAYER, getTeam());
						if (NULL != pNearestCity)
						{
							FAssert(pNearestCity->getTeam() == getTeam());
							int iDistance = stepDistance(iX, iY, pNearestCity->getX_INLINE(), pNearestCity->getY_INLINE());
							if (iDistance < 10)
							{
								iValue /= (10 - iDistance);
							}
						}
						
						if (iValue > iBestValue)
						{
							iBestValue = iValue;
							pBestPlot = pLoopPlot;							
						}
					}
				}
				else
				{
					FAssertMsg(false, "StartingPlot for a live player is NULL!");
				}
			}
		}
	}
	
	if (pBestPlot != NULL)
	{
		return pBestPlot;
	}
	
	FAssertMsg(false, "AS: Failed to find a starting plot for a player");
	
	//Execution should almost never reach here.
	
	//Update found values just in case - particulary important for simultaneous turns.
	AI_updateFoundValues();
	
	pBestPlot = NULL;
	iBestValue = -1;
	
	if (NULL != getStartingPlot())
	{
		for (int iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
		{
			CvPlot* pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);
			if (pLoopPlot->getArea() == getStartingPlot()->getArea())
			{
				int iValue = pLoopPlot->getFoundValue(getID());
				if (iValue > 0)
				{
					if (getAdvancedStartCityCost(true, pLoopPlot) > 0)
					{
						if (iValue > iBestValue)
						{
							iBestValue = iValue;
							pBestPlot = pLoopPlot;
						}
					}
				}
			}
		}
	}
	
	if (pBestPlot != NULL)
	{
		return pBestPlot;
	}
	
	//Commence panic.
	FAssertMsg(false, "Failed to find an advanced start starting plot");
	return NULL;
}


bool CvPlayerAI::AI_advancedStartPlaceExploreUnits(bool bLand)
{
	CvPlot* pBestExplorePlot = NULL;
	int iBestExploreValue = 0;
	UnitTypes eBestUnitType = NO_UNIT;
	
	UnitAITypes eUnitAI = NO_UNITAI;
	if (bLand)
	{
		eUnitAI = UNITAI_EXPLORE;
	}
	else
	{
		eUnitAI = UNITAI_EXPLORE_SEA;
	}
	
	int iLoop;
	CvCity* pLoopCity;
	for (pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		CvPlot* pLoopPlot = pLoopCity->plot();
		CvArea* pLoopArea = bLand ? pLoopCity->area() : pLoopPlot->waterArea();
		
		if (pLoopArea != NULL)
			{
			int iValue = std::max(0, pLoopArea->getNumUnrevealedTiles(getTeam()) - 10) * 10;
			iValue += std::max(0, pLoopArea->getNumTiles() - 50);
				
				if (iValue > 0)
				{
					int iOtherPlotCount = 0;
					int iGoodyCount = 0;
					int iExplorerCount = 0;
				int iAreaId = pLoopArea->getID();
					
				int iRange = 4;
					for (int iX = -iRange; iX <= iRange; iX++)
					{
						for (int iY = -iRange; iY <= iRange; iY++)
						{
							CvPlot* pLoopPlot2 = plotXY(pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE(), iX, iY);
						if (NULL != pLoopPlot2) 
							{
								iExplorerCount += pLoopPlot2->plotCount(PUF_isUnitAIType, eUnitAI, -1, NO_PLAYER, getTeam());
							if (pLoopPlot2->getArea() == iAreaId)
							{
								if (pLoopPlot2->isGoody())
								{
									iGoodyCount++;
								}
								if (pLoopPlot2->getTeam() != getTeam())
								{
									iOtherPlotCount++;
								}
							}
						}
					}
				}
					
					iValue -= 300 * iExplorerCount;
					iValue += 200 * iGoodyCount;
					iValue += 10 * iOtherPlotCount;
					if (iValue > iBestExploreValue)
					{
						UnitTypes eUnit = AI_bestAdvancedStartUnitAI(pLoopPlot, eUnitAI);
						if (eUnit != NO_UNIT)
						{
							eBestUnitType = eUnit;
							iBestExploreValue = iValue;
							pBestExplorePlot = pLoopPlot;
						}
					}
				}
			}
		}
	
	if (pBestExplorePlot != NULL)
	{
		doAdvancedStartAction(ADVANCEDSTARTACTION_UNIT, pBestExplorePlot->getX_INLINE(), pBestExplorePlot->getY_INLINE(), eBestUnitType, true);
		return true;
	}
	return false;	
}

void CvPlayerAI::AI_advancedStartRevealRadius(CvPlot* pPlot, int iRadius)
{
	for (int iRange = 1; iRange <=iRadius; iRange++)
	{
		for (int iX = -iRange; iX <= iRange; iX++)
		{
			for (int iY = -iRange; iY <= iRange; iY++)
			{
				if (plotDistance(0, 0, iX, iY) <= iRadius)
				{
					CvPlot* pLoopPlot = plotXY(pPlot->getX_INLINE(), pPlot->getY_INLINE(), iX, iY);

					if (NULL != pLoopPlot)
					{
						if (getAdvancedStartVisibilityCost(true, pLoopPlot) > 0)
						{
							doAdvancedStartAction(ADVANCEDSTARTACTION_VISIBILITY, pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE(), -1, true);
						}
					}
				}
			}
		}
	}
}

bool CvPlayerAI::AI_advancedStartPlaceCity(CvPlot* pPlot)
{
	//If there is already a city, then improve it.
	CvCity* pCity = pPlot->getPlotCity();
	if (pCity == NULL)
	{
		doAdvancedStartAction(ADVANCEDSTARTACTION_CITY, pPlot->getX(), pPlot->getY(), -1, true);

		pCity = pPlot->getPlotCity();
		if ((pCity == NULL) || (pCity->getOwnerINLINE() != getID()))
		{
			//this should never happen since the cost for a city should be 0 if
			//the city can't be placed.
			//(It can happen if another player has placed a city in the fog)
			FAssertMsg(false, "ADVANCEDSTARTACTION_CITY failed in unexpected way");
			return false;
		}
	}
	
	if (pCity->getCultureLevel() <= 1)
	{
		doAdvancedStartAction(ADVANCEDSTARTACTION_CULTURE, pPlot->getX(), pPlot->getY(), -1, true);
	}
	
	//to account for culture expansion.
	pCity->AI_updateBestBuild();
	
	int iPlotsImproved = 0;
	for (int iI = 0; iI < NUM_CITY_PLOTS; iI++)
	{
		if (iI != CITY_HOME_PLOT)
		{
			CvPlot* pLoopPlot = plotCity(pPlot->getX_INLINE(), pPlot->getY_INLINE(), iI);
			if ((pLoopPlot != NULL) && (pLoopPlot->getWorkingCity() == pCity))
			{
				if (pLoopPlot->getImprovementType() != NO_IMPROVEMENT)
				{
					iPlotsImproved++;
				}
			}
		}
	}

	int iTargetPopulation = pCity->happyLevel() + (getCurrentEra() / 2);
	
	while (iPlotsImproved < iTargetPopulation)
	{
		CvPlot* pBestPlot;
		ImprovementTypes eBestImprovement = NO_IMPROVEMENT;
		int iBestValue = 0;
		for (int iI = 0; iI < NUM_CITY_PLOTS; iI++)
		{
			int iValue = pCity->AI_getBestBuildValue(iI);
			if (iValue > iBestValue)
			{
				BuildTypes eBuild = pCity->AI_getBestBuild(iI);
				if (eBuild != NO_BUILD)
				{
					ImprovementTypes eImprovement = (ImprovementTypes)GC.getBuildInfo(eBuild).getImprovement();
					if (eImprovement != NO_IMPROVEMENT)
					{
						CvPlot* pLoopPlot = plotCity(pCity->getX_INLINE(), pCity->getY_INLINE(), iI);
						if ((pLoopPlot != NULL) && (pLoopPlot->getImprovementType() != eImprovement))
						{
							eBestImprovement = eImprovement;
							pBestPlot = pLoopPlot;
							iBestValue = iValue;
						}
					}
				}
			}
		}
		
		if (iBestValue > 0)
		{
			
			FAssert(pBestPlot != NULL);
			doAdvancedStartAction(ADVANCEDSTARTACTION_IMPROVEMENT, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), eBestImprovement, true);
			iPlotsImproved++;
			if (pCity->getPopulation() < iPlotsImproved)
			{
				doAdvancedStartAction(ADVANCEDSTARTACTION_POP, pBestPlot->getX_INLINE(), pBestPlot->getY_INLINE(), -1, true);
			}
		}
		else
		{
			break;
		}
	}
		

	while (iPlotsImproved > pCity->getPopulation())
	{
		int iPopCost = getAdvancedStartPopCost(true, pCity);
		if (iPopCost <= 0 || iPopCost > getAdvancedStartPoints())
		{
			break;
		}
		if (pCity->healthRate() < 0)
		{
			break;
		}
		doAdvancedStartAction(ADVANCEDSTARTACTION_POP, pPlot->getX_INLINE(), pPlot->getY_INLINE(), -1, true);
	}
	
	pCity->AI_updateAssignWork();

	return true;
}




//Returns false if we have no more points.
bool CvPlayerAI::AI_advancedStartDoRoute(CvPlot* pFromPlot, CvPlot* pToPlot)
{
	FAssert(pFromPlot != NULL);
	FAssert(pToPlot != NULL);
	
	FAStarNode* pNode;
	gDLL->getFAStarIFace()->ForceReset(&GC.getStepFinder());
	if (gDLL->getFAStarIFace()->GeneratePath(&GC.getStepFinder(), pFromPlot->getX_INLINE(), pFromPlot->getY_INLINE(), pToPlot->getX_INLINE(), pToPlot->getY_INLINE(), false, 0, true))
	{
		pNode = gDLL->getFAStarIFace()->GetLastNode(&GC.getStepFinder());
		if (pNode != NULL)
		{
			if (pNode->m_iData1 > (1 + stepDistance(pFromPlot->getX(), pFromPlot->getY(), pToPlot->getX(), pToPlot->getY())))
			{
				//Don't build convulted paths.
				return true;
			}
		}

		while (pNode != NULL)
		{
			CvPlot* pPlot = GC.getMapINLINE().plotSorenINLINE(pNode->m_iX, pNode->m_iY);
			RouteTypes eRoute = AI_bestAdvancedStartRoute(pPlot);
			if (eRoute != NO_ROUTE)
			{
				if (getAdvancedStartRouteCost(eRoute, true, pPlot) > getAdvancedStartPoints())
				{
					return false;
				}
				doAdvancedStartAction(ADVANCEDSTARTACTION_ROUTE, pNode->m_iX, pNode->m_iY, eRoute, true);
			}
			pNode = pNode->m_pParent;			
		}
	}
	return true;
}
void CvPlayerAI::AI_advancedStartRouteTerritory()
{
//	//This uses a heuristic to create a road network
//	//which is at least effecient if not all inclusive
//	//Basically a human will place roads where they connect
//	//the maximum number of trade groups and this
//	//mimics that.
//	
//	
//	CvPlot* pLoopPlot;
//	CvPlot* pLoopPlot2;
//	int iI, iJ;
//	int iPass;
//	
//
//	std::vector<int> aiPlotGroups;
//	for (iPass = 4; iPass > 1; --iPass)
//	{
//		for (iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
//		{
//			pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);
//			if ((pLoopPlot != NULL) && (pLoopPlot->getOwner() == getID()) && (pLoopPlot->getRouteType() == NO_ROUTE))
//			{
//				aiPlotGroups.clear();
//				if (pLoopPlot->getPlotGroup(getID()) != NULL)
//				{
//					aiPlotGroups.push_back(pLoopPlot->getPlotGroup(getID())->getID());
//				}
//				for (iJ = 0; iJ < NUM_DIRECTION_TYPES; iJ++)
//				{
//					pLoopPlot2 = plotDirection(pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE(), (DirectionTypes)iJ);
//					if ((pLoopPlot2 != NULL) && (pLoopPlot2->getRouteType() != NO_ROUTE))
//					{
//						CvPlotGroup* pPlotGroup = pLoopPlot2->getPlotGroup(getID());
//						if (pPlotGroup != NULL)
//						{
//							if (std::find(aiPlotGroups.begin(),aiPlotGroups.end(), pPlotGroup->getID()) == aiPlotGroups.end())
//							{
//								aiPlotGroups.push_back(pPlotGroup->getID());
//							}
//						}
//					}
//				}
//				if ((int)aiPlotGroups.size() >= iPass)
//				{
//					RouteTypes eBestRoute = AI_bestAdvancedStartRoute(pLoopPlot);
//					if (eBestRoute != NO_ROUTE)
//					{
//						doAdvancedStartAction(ADVANCEDSTARTACTION_ROUTE, pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE(), eBestRoute, true);
//					}
//				}
//			}
//		}
//	}
//	
//	//Maybe try to build road network for mobility but bearing in mind
//	//that routes can't be built outside culture atm. I think workers
//	//can do that just fine.

	CvPlot* pLoopPlot;
	int iI;
	
	for (iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
	{
		pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);
		if ((pLoopPlot != NULL) && (pLoopPlot->getOwner() == getID()) && (pLoopPlot->getRouteType() == NO_ROUTE))
		{
			if (pLoopPlot->getImprovementType() != NO_IMPROVEMENT)
			{
				BonusTypes eBonus = pLoopPlot->getBonusType(getTeam());
				if (eBonus != NO_BONUS)
				{
					if (GC.getImprovementInfo(pLoopPlot->getImprovementType()).isImprovementBonusTrade(eBonus))
					{
						int iBonusValue = AI_bonusVal(eBonus, 1);
						if (iBonusValue > 9)
						{
							int iBestValue = 0;
							CvPlot* pBestPlot = NULL;
							int iRange = 2;
							for (int iX = -iRange; iX <= iRange; iX++)
							{
								for (int iY = -iRange; iY <= iRange; iY++)
								{
									CvPlot* pLoopPlot2 = plotXY(pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE(), iX, iY);
									if (pLoopPlot2 != NULL)
									{
										if (pLoopPlot2->getOwner() == getID())
										{
											if ((pLoopPlot2->isConnectedToCapital()) || pLoopPlot2->isCity())
											{
												int iValue = 1000;
												if (pLoopPlot2->isCity())
												{
													iValue += 100;
													if (pLoopPlot2->getPlotCity()->isCapital())
													{
														iValue += 100;
													}
												}
												if (pLoopPlot2->isRoute())
												{
													iValue += 100;
												}
												int iDistance = GC.getMapINLINE().calculatePathDistance(pLoopPlot, pLoopPlot2);
												if (iDistance > 0)
												{
													iValue /= (1 + iDistance);
													
													if (iValue > iBestValue)
													{
														iBestValue = iValue;
														pBestPlot = pLoopPlot2;
													}
												}
											}
										}
									}
								}
							}
							if (pBestPlot != NULL)
							{
								if (!AI_advancedStartDoRoute(pLoopPlot, pBestPlot))
								{
									return;
								}
							}
						}
					}
				}
				if (pLoopPlot->getRouteType() == NO_ROUTE)
				{
					int iRouteYieldValue = 0;
					RouteTypes eRoute = (AI_bestAdvancedStartRoute(pLoopPlot, &iRouteYieldValue));
					if (eRoute != NO_ROUTE && iRouteYieldValue > 0)
					{
						doAdvancedStartAction(ADVANCEDSTARTACTION_ROUTE, pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE(), eRoute, true);
					}
				}
			}
		}
	}
	
	//Connect Cities
	int iLoop;
	CvCity* pLoopCity;

	for (pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		if (!pLoopCity->isCapital() && !pLoopCity->isConnectedToCapital())
		{
			int iBestValue = 0;
			CvPlot* pBestPlot = NULL;
			int iRange = 5;
			for (int iX = -iRange; iX <= iRange; iX++)
			{
				for (int iY = -iRange; iY <= iRange; iY++)
				{
					CvPlot* pLoopPlot = plotXY(pLoopCity->getX_INLINE(), pLoopCity->getY_INLINE(), iX, iY);
					if ((pLoopPlot != NULL) && (pLoopPlot->getOwner() == getID()))
					{
						if ((pLoopPlot->isConnectedToCapital()) || pLoopPlot->isCity())
						{
							int iValue = 1000;
							if (pLoopPlot->isCity())
							{
								iValue += 500;
								if (pLoopPlot->getPlotCity()->isCapital())
								{
									iValue += 500;
								}
							}
							if (pLoopPlot->isRoute())
							{
								iValue += 100;
							}
							int iDistance = GC.getMapINLINE().calculatePathDistance(pLoopCity->plot(), pLoopPlot);
							if (iDistance > 0)
							{
								iValue /= (1 + iDistance);
								
								if (iValue > iBestValue)
								{
									iBestValue = iValue;
									pBestPlot = pLoopPlot;
								}
							}
						}
					}
				}
			}
			if (NULL != pBestPlot)
			{
				if (!AI_advancedStartDoRoute(pBestPlot, pLoopCity->plot()))
				{
					return;
				}
			}
		}
	}
}


void CvPlayerAI::AI_doAdvancedStart(bool bNoExit)
{
	FAssertMsg(!isBarbarian(), "Should not be called for barbarians!");

	if (NULL == getStartingPlot())
	{
		FAssert(false);
		return;
	}

	int iLoop;
	CvCity* pLoopCity;
	
	int iStartingPoints = getAdvancedStartPoints();
	int iRevealPoints = (iStartingPoints * 10) / 100;
	int iMilitaryPoints = (iStartingPoints * (isHuman() ? 17 : (10 + (GC.getLeaderHeadInfo(getPersonalityType()).getBuildUnitProb() / 3)))) / 100;
	int iCityPoints = iStartingPoints - (iMilitaryPoints + iRevealPoints);

	if (getCapitalCity() != NULL)
	{
		AI_advancedStartPlaceCity(getCapitalCity()->plot());
	}
	else
	{
		for (int iPass = 0; iPass < 2 && NULL == getCapitalCity(); ++iPass)
		{
			CvPlot* pBestCapitalPlot = AI_advancedStartFindCapitalPlot();

			if (pBestCapitalPlot != NULL)
			{
				if (!AI_advancedStartPlaceCity(pBestCapitalPlot))
				{
					FAssertMsg(false, "AS AI: Unexpected failure placing capital");
				}
				break;
			}
			else
			{
				//If this point is reached, the advanced start system is broken.
				//Find a new starting plot for this player
				setStartingPlot(findStartingPlot(false), true);
				//Redo Starting visibility
				CvPlot* pStartingPlot = getStartingPlot();
				if (NULL != pStartingPlot)
				{
					for (int iPlotLoop = 0; iPlotLoop < GC.getMapINLINE().numPlots(); ++iPlotLoop)
					{
						CvPlot* pPlot = GC.getMapINLINE().plotByIndex(iPlotLoop);

						if (plotDistance(pPlot->getX_INLINE(), pPlot->getY_INLINE(), pStartingPlot->getX_INLINE(), pStartingPlot->getY_INLINE()) <= GC.getDefineINT("ADVANCED_START_SIGHT_RANGE"))
						{
							pPlot->setRevealed(getTeam(), true, false, NO_TEAM, false);
						}
					}
				}
			}
		}

		if (getCapitalCity() == NULL)
		{
			if (!bNoExit)
			{
				doAdvancedStartAction(ADVANCEDSTARTACTION_EXIT, -1, -1, -1, true);
			}
			return;
		}
	}
	
	iCityPoints -= (iStartingPoints - getAdvancedStartPoints());
	
	int iLastPointsTotal = getAdvancedStartPoints();
	
	for (int iPass = 0; iPass < 6; iPass++)
	{
		for (int iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
		{
			CvPlot* pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);
			if (pLoopPlot->isRevealed(getTeam(), false))
			{
				if (pLoopPlot->getBonusType(getTeam()) != NO_BONUS)
				{
					AI_advancedStartRevealRadius(pLoopPlot, CITY_PLOTS_RADIUS);					
				}
				else
				{
					for (int iJ = 0; iJ < NUM_CARDINALDIRECTION_TYPES; iJ++)
					{
						CvPlot* pLoopPlot2 = plotCardinalDirection(pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE(), (CardinalDirectionTypes)iJ);
						if ((pLoopPlot2 != NULL) && (getAdvancedStartVisibilityCost(true, pLoopPlot2) > 0))
						{
							//Mildly maphackery but any smart human can see the terrain type of a tile.
							pLoopPlot2->getTerrainType();
							int iFoodYield = GC.getTerrainInfo(pLoopPlot2->getTerrainType()).getYield(YIELD_FOOD);
							if (pLoopPlot2->getFeatureType() != NO_FEATURE)
							{
								iFoodYield += GC.getFeatureInfo(pLoopPlot2->getFeatureType()).getYieldChange(YIELD_FOOD);
							}
							if (((iFoodYield >= 2) && !pLoopPlot2->isFreshWater()) || pLoopPlot2->isHills() || pLoopPlot2->isRiver())
							{
								doAdvancedStartAction(ADVANCEDSTARTACTION_VISIBILITY, pLoopPlot2->getX_INLINE(), pLoopPlot2->getY_INLINE(), -1, true);						
							}
						}
					}
				}
			}
			if ((iLastPointsTotal - getAdvancedStartPoints()) > iRevealPoints)
			{
				break;
			}
		}
	}
	
	iLastPointsTotal = getAdvancedStartPoints();
	iCityPoints = std::min(iCityPoints, iLastPointsTotal);
	int iArea = -1; //getStartingPlot()->getArea();
	
	//Spend econ points on a tech?
	int iTechRand = 90 + GC.getGame().getSorenRandNum(20, "AI AS Buy Tech 1");
	int iTotalTechSpending = 0;
	
	if (getCurrentEra() == 0)
	{
		TechTypes eTech = AI_bestTech(1);
		if ((eTech != NO_TECH) && !GC.getTechInfo(eTech).isRepeat())
		{
			int iTechCost = getAdvancedStartTechCost(eTech, true);
			if (iTechCost > 0)
			{
				doAdvancedStartAction(ADVANCEDSTARTACTION_TECH, -1, -1, eTech, true);
				iTechRand -= 50;
				iTotalTechSpending += iTechCost;
			}
		}
	}
	
	bool bDonePlacingCities = false;	
	for (int iPass = 0; iPass < 100; ++iPass)
	{
		int iRand = iTechRand + 10 * getNumCities();
		if ((iRand > 0) && (GC.getGame().getSorenRandNum(100, "AI AS Buy Tech 2") < iRand))
		{
			TechTypes eTech = AI_bestTech(1);
			if ((eTech != NO_TECH) && !GC.getTechInfo(eTech).isRepeat())
			{
				int iTechCost = getAdvancedStartTechCost(eTech, true);
				if ((iTechCost > 0) && ((iTechCost + iTotalTechSpending) < (iCityPoints / 4)))
				{
					doAdvancedStartAction(ADVANCEDSTARTACTION_TECH, -1, -1, eTech, true);
					iTechRand -= 50;
					iTotalTechSpending += iTechCost;
					
					for (pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
					{	
						AI_advancedStartPlaceCity(pLoopCity->plot());
					}
				}
			}
		}
		int iBestFoundValue = 0;
		CvPlot* pBestFoundPlot = NULL;
		AI_updateFoundValues(false);
		for (int iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
		{
			CvPlot* pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);
			//if (pLoopPlot->area() == getStartingPlot()->area())
			{
				if (plotDistance(getStartingPlot()->getX_INLINE(), getStartingPlot()->getY_INLINE(), pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE()) < 9)
				{
					if (pLoopPlot->getFoundValue(getID()) > iBestFoundValue)
					{
						if (getAdvancedStartCityCost(true, pLoopPlot) > 0)
						{
							pBestFoundPlot = pLoopPlot;
							iBestFoundValue = pLoopPlot->getFoundValue(getID());
						}
					}
				}
			}
		}
		
		if (iBestFoundValue < ((getNumCities() == 0) ? 1 : (500 + 250 * getNumCities())))
		{
			bDonePlacingCities = true;			
		}
		if (!bDonePlacingCities)
		{
			int iCost = getAdvancedStartCityCost(true, pBestFoundPlot);
			if (iCost > getAdvancedStartPoints())
			{
				bDonePlacingCities = true;
			}// at 500pts, we have 200, we spend 100. 
			else if (((iLastPointsTotal - getAdvancedStartPoints()) + iCost) > iCityPoints)
			{
				bDonePlacingCities = true;
			}
		}
		
		if (!bDonePlacingCities)
		{
			if (!AI_advancedStartPlaceCity(pBestFoundPlot))
			{
				FAssertMsg(false, "AS AI: Failed to place city (non-capital)");
				bDonePlacingCities = true;
			}
		}

		if (bDonePlacingCities)
		{
			break;
		}
	}


	bool bDoneWithTechs = false;
	while (!bDoneWithTechs)
	{
		bDoneWithTechs = true;
		TechTypes eTech = AI_bestTech(1);
		if (eTech != NO_TECH && !GC.getTechInfo(eTech).isRepeat())
		{
			int iTechCost = getAdvancedStartTechCost(eTech, true);
			if ((iTechCost > 0) && ((iTechCost + iLastPointsTotal - getAdvancedStartPoints()) <= iCityPoints))
			{
				doAdvancedStartAction(ADVANCEDSTARTACTION_TECH, -1, -1, eTech, true);
				bDoneWithTechs = false;
			}
		}
	}
	
	{
		//Land
		AI_advancedStartPlaceExploreUnits(true);
		if (getCurrentEra() > 2)
		{
			//Sea
			AI_advancedStartPlaceExploreUnits(false);
			if (GC.getGameINLINE().circumnavigationAvailable())
			{
				if (GC.getGameINLINE().getSorenRandNum(GC.getGameINLINE().countCivPlayersAlive(), "AI AS buy 2nd sea explorer") < 2)
				{
					AI_advancedStartPlaceExploreUnits(false);
				}
			}
		}
	}
	
	AI_advancedStartRouteTerritory();
		
	bool bDoneBuildings = (iLastPointsTotal - getAdvancedStartPoints()) > iCityPoints;
	for (int iPass = 0; iPass < 10 && !bDoneBuildings; ++iPass)
	{
		for (pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
		{
			BuildingTypes eBuilding = pLoopCity->AI_bestAdvancedStartBuilding(iPass);
			if (eBuilding != NO_BUILDING)
			{
				bDoneBuildings = (iLastPointsTotal - (getAdvancedStartPoints() - getAdvancedStartBuildingCost(eBuilding, true, pLoopCity))) > iCityPoints;
				if (!bDoneBuildings)
				{
					doAdvancedStartAction(ADVANCEDSTARTACTION_BUILDING, pLoopCity->getX_INLINE(), pLoopCity->getY_INLINE(), eBuilding, true);
				}
				else
				{
					//continue there might be cheaper buildings in other cities we can afford
				}
			}
		}
	}
	
	//Units
	std::vector<UnitAITypes> aeUnitAITypes;
	aeUnitAITypes.push_back(UNITAI_CITY_DEFENSE);
	aeUnitAITypes.push_back(UNITAI_WORKER);
	aeUnitAITypes.push_back(UNITAI_RESERVE);
	aeUnitAITypes.push_back(UNITAI_COUNTER);
	
	
	bool bDone = false;
	for (int iPass = 0; iPass < 10; ++iPass)
	{
		for (pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
		{
			if ((iPass == 0) || (pLoopCity->getArea() == getStartingPlot()->getArea()))
			{
				CvPlot* pUnitPlot = pLoopCity->plot();
				//Token defender
				UnitTypes eBestUnit = AI_bestAdvancedStartUnitAI(pUnitPlot, aeUnitAITypes[iPass % aeUnitAITypes.size()]);
				if (eBestUnit != NO_UNIT)
				{
					if (getAdvancedStartUnitCost(eBestUnit, true, pUnitPlot) > getAdvancedStartPoints())
					{
						bDone = true;
						break;
					}
					doAdvancedStartAction(ADVANCEDSTARTACTION_UNIT, pUnitPlot->getX(), pUnitPlot->getY(), eBestUnit, true);
				}
			}
		}
	}
	
	if (isHuman())
	{
		// remove unhappy population
		for (pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
		{
			while (pLoopCity->angryPopulation() > 0 && getAdvancedStartPopCost(false, pLoopCity) > 0)
			{
				doAdvancedStartAction(ADVANCEDSTARTACTION_POP, pLoopCity->getX_INLINE(), pLoopCity->getY_INLINE(), -1, false);
			}
		}
	}

	if (!bNoExit)
	{
		doAdvancedStartAction(ADVANCEDSTARTACTION_EXIT, -1, -1, -1, true);
	}

}


void CvPlayerAI::AI_recalculateFoundValues(int iX, int iY, int iInnerRadius, int iOuterRadius) const
{
	CvPlot* pLoopPlot;
	int iLoopX, iLoopY;
	//int iValue;
	
	for (iLoopX = -iOuterRadius; iLoopX <= iOuterRadius; iLoopX++)
	{
		for (iLoopY = -iOuterRadius; iLoopY <= iOuterRadius; iLoopY++)
		{
			pLoopPlot = plotXY(iX, iY, iLoopX, iLoopY);
			if ((NULL != pLoopPlot) && !AI_isPlotCitySite(pLoopPlot))
			{
				if (stepDistance(0, 0, iLoopX, iLoopY) <= iInnerRadius)
				{
					if (!((iLoopX == 0) && (iLoopY == 0)))
					{
						pLoopPlot->setFoundValue(getID(), 0);
					}
				}
				else
				{
					if ((pLoopPlot != NULL) && (pLoopPlot->isRevealed(getTeam(), false)))
					{
						long lResult=-1;
						if(GC.getUSE_GET_CITY_FOUND_VALUE_CALLBACK())
						{
							CyArgsList argsList;
							argsList.add((int)getID());
							argsList.add(pLoopPlot->getX());
							argsList.add(pLoopPlot->getY());
							gDLL->getPythonIFace()->callFunction(PYGameModule, "getCityFoundValue", argsList.makeFunctionArgs(), &lResult);
						}

						short iValue; // K-Mod
						if (lResult == -1)
						{
							iValue = AI_foundValue(pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE());
						}
						else
						{
							//iValue = lResult;
							iValue = (short)std::min((long)MAX_SHORT, lResult); // K-Mod
						}

						pLoopPlot->setFoundValue(getID(), iValue);

						if (iValue > pLoopPlot->area()->getBestFoundValue(getID()))
						{
							pLoopPlot->area()->setBestFoundValue(getID(), iValue);
						}
					}
				}
			}
		}
	}
}

	
int CvPlayerAI::AI_getMinFoundValue() const
{
	//int iValue = 600;
	int iValue = GC.getDefineINT("BBAI_MINIMUM_FOUND_VALUE"); // K-Mod
	int iNetCommerce = 1 + getCommerceRate(COMMERCE_GOLD) + getCommerceRate(COMMERCE_RESEARCH) + std::max(0, getGoldPerTurn());
/************************************************************************************************/
/* UNOFFICIAL_PATCH                       06/11/09                       jdog5000 & DanF5771    */
/*                                                                                              */
/* Bugfix                                                                                       */
/************************************************************************************************/
/* original BTS code
	int iNetExpenses = calculateInflatedCosts() + std::min(0, getGoldPerTurn());
*/
	int iNetExpenses = calculateInflatedCosts() + std::max(0, -getGoldPerTurn());
/************************************************************************************************/
/* UNOFFICIAL_PATCH                        END                                                  */
/************************************************************************************************/

	iValue *= iNetCommerce;
	iValue /= std::max(std::max(1, iNetCommerce / 4), iNetCommerce - iNetExpenses);
	
	if (GET_TEAM(getTeam()).getAnyWarPlanCount(1) > 0)
	{
		iValue *= 2;
	}

	// K-Mod. # of cities maintenance cost increase...
	int iNumCitiesPercent = 100;
	//iNumCitiesPercent *= (getAveragePopulation() + 17);
	//iNumCitiesPercent /= 18;

	iNumCitiesPercent *= GC.getWorldInfo(GC.getMapINLINE().getWorldSize()).getNumCitiesMaintenancePercent();
	iNumCitiesPercent /= 100;

	iNumCitiesPercent *= GC.getHandicapInfo(getHandicapType()).getNumCitiesMaintenancePercent();
	iNumCitiesPercent /= 100;

	//iNumCitiesPercent *= std::max(0, getNumCitiesMaintenanceModifier() + 100);
	//iNumCitiesPercent /= 100;

	// The marginal cost increase is roughly equal to double the cost of a current city...
	// But we're really going to have to fudge it anyway, because the city value is in arbitrary units
	// lets just say each gold per turn is worth roughly 60 'value points'.
	// In the future, this could be AI flavour based.
	iValue += iNumCitiesPercent * getNumCities() * 60 / 100;
	// K-Mod end
	
	return iValue;
}
	
void CvPlayerAI::AI_updateCitySites(int iMinFoundValueThreshold, int iMaxSites)
{
	std::vector<int>::iterator it;
	int iValue;
	int iI;
	
	int iPass = 0;
	while (iPass < iMaxSites)
	{
		//Add a city to the list.
		int iBestFoundValue = 0;
		CvPlot* pBestFoundPlot = NULL;

		for (iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
		{
			CvPlot* pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);
			if (pLoopPlot->isRevealed(getTeam(), false))
			{
				iValue = pLoopPlot->getFoundValue(getID());
				if (iValue > iMinFoundValueThreshold)
				{
					if (!AI_isPlotCitySite(pLoopPlot))
					{
						iValue *= std::min(NUM_CITY_PLOTS * 2, pLoopPlot->area()->getNumUnownedTiles());					

						if (iValue > iBestFoundValue)
						{
							iBestFoundValue = iValue;
							pBestFoundPlot = pLoopPlot;
						}
					}
				}
			}
		}
		if (pBestFoundPlot != NULL)
		{
			m_aiAICitySites.push_back(GC.getMapINLINE().plotNum(pBestFoundPlot->getX_INLINE(), pBestFoundPlot->getY_INLINE()));
			AI_recalculateFoundValues(pBestFoundPlot->getX_INLINE(), pBestFoundPlot->getY_INLINE(), CITY_PLOTS_RADIUS, 2 * CITY_PLOTS_RADIUS);
		}
		else
		{
			break;
		}
		iPass++;
	}
}

void CvPlayerAI::AI_invalidateCitySites(int iMinFoundValueThreshold)
{
	/* original bts code
	m_aiAICitySites.clear(); */
	// K-Mod. note: this clear-by-value stuff isn't actually used yet... but at least it works now.
	std::vector<int> keptSites;
	if (iMinFoundValueThreshold > 0) // less than zero means clear all.
	{
		for (size_t iI = 0; iI < m_aiAICitySites.size(); iI++)
		{
			if (GC.getMapINLINE().plotByIndexINLINE(m_aiAICitySites[iI])->getFoundValue(getID()) >= iMinFoundValueThreshold)
				keptSites.push_back(m_aiAICitySites[iI]);
		}
	}
	m_aiAICitySites.swap(keptSites);
	// K-Mod end
}

int CvPlayerAI::AI_getNumCitySites() const
{
	return m_aiAICitySites.size();
}

bool CvPlayerAI::AI_isPlotCitySite(CvPlot* pPlot) const
{
	std::vector<int>::const_iterator it;
	int iPlotIndex = GC.getMapINLINE().plotNumINLINE(pPlot->getX_INLINE(), pPlot->getY_INLINE());
	
	for (it = m_aiAICitySites.begin(); it != m_aiAICitySites.end(); it++)
	{
		if ((*it) == iPlotIndex)
		{
			return true;
		}		
	}
	return false;
}

int CvPlayerAI::AI_getNumAreaCitySites(int iAreaID, int& iBestValue) const
{
	std::vector<int>::const_iterator it;
	int iCount = 0;
	iBestValue = 0;
	
	for (it = m_aiAICitySites.begin(); it != m_aiAICitySites.end(); it++)
	{
		CvPlot* pCitySitePlot = GC.getMapINLINE().plotByIndex((*it));
		if (pCitySitePlot->getArea() == iAreaID)
		{
			iCount++;
			iBestValue = std::max(iBestValue, pCitySitePlot->getFoundValue(getID()));
		}		
	}
	return iCount;
}

int CvPlayerAI::AI_getNumAdjacentAreaCitySites(int iWaterAreaID, int iExcludeArea, int& iBestValue) const
{
	std::vector<int>::const_iterator it;
	int iCount = 0;
	iBestValue = 0;

	for (it = m_aiAICitySites.begin(); it != m_aiAICitySites.end(); it++)
	{
		CvPlot* pCitySitePlot = GC.getMapINLINE().plotByIndex((*it));
		if (pCitySitePlot->getArea() != iExcludeArea)
		{
			if (pCitySitePlot->isAdjacentToArea(iWaterAreaID))
			{
				iCount++;
				iBestValue = std::max(iBestValue, pCitySitePlot->getFoundValue(getID()));
			}	
		}
	}
	return iCount;
}

CvPlot* CvPlayerAI::AI_getCitySite(int iIndex) const
{
	FAssert(iIndex < (int)m_aiAICitySites.size());
	return GC.getMapINLINE().plotByIndex(m_aiAICitySites[iIndex]);
}

// K-Mod
// return true if is fair enough for the AI to know there is a city here
bool CvPlayerAI::AI_deduceCitySite(CvCity* pCity) const
{
	if (pCity->isRevealed(getTeam(), false))
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

			if (pLoopPlot && pLoopPlot->getRevealedOwner(getTeam(), false) == pCity->getOwnerINLINE())
			{
				iPoints += 1 + iLevel - iDist;

				if (iPoints > iLevel)
					return true;
			}
		}
	}
	return false;
}
// K-Mod end

int CvPlayerAI::AI_bestAreaUnitAIValue(UnitAITypes eUnitAI, CvArea* pArea, UnitTypes* peBestUnitType) const
{
	
	CvCity* pCity = NULL;
	
	if (pArea != NULL)
	{
		if (getCapitalCity() != NULL)
		{
			if (pArea->isWater())
			{
				if (getCapitalCity()->plot()->isAdjacentToArea(pArea))
				{
					pCity = getCapitalCity();
				}			
			}
			else
			{
				if (getCapitalCity()->getArea() == pArea->getID())
				{
					pCity = getCapitalCity();
				}
			}
		}
	
		if (NULL == pCity)
		{
			CvCity* pLoopCity;
			int iLoop;
			for (pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
			{
				if (pArea->isWater())
				{
					if (pLoopCity->plot()->isAdjacentToArea(pArea))
					{
						pCity = pLoopCity;
						break;
					}
				}
				else
				{
					if (pLoopCity->getArea() == pArea->getID())
					{
						pCity = pLoopCity;
						break;
					}
				}
			}
		}
	}

	return AI_bestCityUnitAIValue(eUnitAI, pCity, peBestUnitType);
}

int CvPlayerAI::AI_bestCityUnitAIValue(UnitAITypes eUnitAI, CvCity* pCity, UnitTypes* peBestUnitType) const
{
	UnitTypes eLoopUnit;
	int iValue;
	int iBestValue;
	int iI;
	
	FAssertMsg(eUnitAI != NO_UNITAI, "UnitAI is not assigned a valid value");
	
	iBestValue = 0;
	
	for (iI = 0; iI < GC.getNumUnitClassInfos(); iI++)
	{
		eLoopUnit = ((UnitTypes)(GC.getCivilizationInfo(getCivilizationType()).getCivilizationUnits(iI)));

		if (eLoopUnit != NO_UNIT)
		{
			if (!isHuman() || (GC.getUnitInfo(eLoopUnit).getDefaultUnitAIType() == eUnitAI))
			{
				if (NULL == pCity ? canTrain(eLoopUnit) : pCity->canTrain(eLoopUnit))
				{
					iValue = AI_unitValue(eLoopUnit, eUnitAI, (pCity == NULL) ? NULL : pCity->area());
					if (iValue > iBestValue)
					{
						iBestValue = iValue;
						if (peBestUnitType != NULL)
						{
							*peBestUnitType = eLoopUnit;
						}
					}
				}
			}
		}
	}

	return iBestValue;
}

int CvPlayerAI::AI_calculateTotalBombard(DomainTypes eDomain) const
{
	int iI;
	int iTotalBombard = 0;
	
	for (iI = 0; iI < GC.getNumUnitClassInfos(); iI++)
	{
		UnitTypes eLoopUnit = ((UnitTypes)(GC.getCivilizationInfo(getCivilizationType()).getCivilizationUnits(iI)));
		if (eLoopUnit != NO_UNIT)
		{
			if (GC.getUnitInfo(eLoopUnit).getDomainType() == eDomain)
			{
				int iBombardRate = GC.getUnitInfo(eLoopUnit).getBombardRate();
				
				if (iBombardRate > 0)
				{
					if( GC.getUnitInfo(eLoopUnit).isIgnoreBuildingDefense() )
					{
						iBombardRate *= 3;
						iBombardRate /= 2;
					}

					iTotalBombard += iBombardRate * getUnitClassCount((UnitClassTypes)iI);
				}
				
				int iBombRate = GC.getUnitInfo(eLoopUnit).getBombRate();
				if (iBombRate > 0)
				{
					iTotalBombard += iBombRate * getUnitClassCount((UnitClassTypes)iI);
				}
			}
		}
	}
	
	return iTotalBombard;
}

void CvPlayerAI::AI_updateBonusValue(BonusTypes eBonus)
{
	FAssert(m_aiBonusValue != NULL);

	//reset
    m_aiBonusValue[eBonus] = -1;
}

void CvPlayerAI::AI_updateBonusValue()
{
	PROFILE_FUNC();

	FAssert(m_aiBonusValue != NULL);

	for (int iI = 0; iI < GC.getNumBonusInfos(); iI++)
	{
		AI_updateBonusValue((BonusTypes)iI);		
	}
}

int CvPlayerAI::AI_getUnitClassWeight(UnitClassTypes eUnitClass) const
{
	return m_aiUnitClassWeights[eUnitClass] / 100;
}

int CvPlayerAI::AI_getUnitCombatWeight(UnitCombatTypes eUnitCombat) const
{
	return m_aiUnitCombatWeights[eUnitCombat] / 100;
}

void CvPlayerAI::AI_doEnemyUnitData()
{
	std::vector<int> aiUnitCounts(GC.getNumUnitInfos(), 0);
	
	std::vector<int> aiDomainSums(NUM_DOMAIN_TYPES, 0);
	
	CLLNode<IDInfo>* pUnitNode;
	CvUnit* pLoopUnit;
	int iI;
	
	int iOldTotal = 0;
	int iNewTotal = 0;
	
	// Count enemy land and sea units visible to us
	for (iI = 0; iI < GC.getMapINLINE().numPlotsINLINE(); iI++)
	{
		CvPlot* pLoopPlot = GC.getMapINLINE().plotByIndexINLINE(iI);
		int iAdjacentAttackers = -1;
		if (pLoopPlot->isVisible(getTeam(), false))
		{
			pUnitNode = pLoopPlot->headUnitNode();

			while (pUnitNode != NULL)
			{
				pLoopUnit = ::getUnit(pUnitNode->m_data);
				pUnitNode = pLoopPlot->nextUnitNode(pUnitNode);
				
				if (pLoopUnit->canFight())
				{
					int iUnitValue = 1;
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      03/18/10                                jdog5000      */
/*                                                                                              */
/* War Strategy AI                                                                              */
/************************************************************************************************/
					//if (atWar(getTeam(), pLoopUnit->getTeam()))
					if( GET_TEAM(getTeam()).AI_getWarPlan(pLoopUnit->getTeam()) != NO_WARPLAN )
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
					{
						iUnitValue += 10;
					
						if ((pLoopPlot->getOwnerINLINE() == getID()))
						{
							iUnitValue += 15;
						}
						else if (atWar(getTeam(), pLoopPlot->getTeam()))
						{
							if (iAdjacentAttackers == -1)
							{
								iAdjacentAttackers = GET_PLAYER(pLoopPlot->getOwnerINLINE()).AI_adjacentPotentialAttackers(pLoopPlot);
							}
							if (iAdjacentAttackers > 0)
							{
								iUnitValue += 15;
							}
						}
					}
					else if (pLoopUnit->getOwnerINLINE() != getID())
					{
						iUnitValue += pLoopUnit->canAttack() ? 4 : 1;
						if (pLoopPlot->getCulture(getID()) > 0)
						{
							iUnitValue += pLoopUnit->canAttack() ? 4 : 1;
						}	
					}
					
					// If we hadn't seen any of this class before
					if (m_aiUnitClassWeights[pLoopUnit->getUnitClassType()] == 0)
					{
						iUnitValue *= 4;
					}
					
					iUnitValue *= pLoopUnit->baseCombatStr();
					aiUnitCounts[pLoopUnit->getUnitType()] += iUnitValue;
					aiDomainSums[pLoopUnit->getDomainType()] += iUnitValue;
					iNewTotal += iUnitValue;
				}
			}
		}
	}

	if (iNewTotal == 0)
	{
		//This should rarely happen.
		return;
	}

	//Decay
	for (iI = 0; iI < GC.getNumUnitClassInfos(); iI++)
	{
		m_aiUnitClassWeights[iI] -= 100;
		m_aiUnitClassWeights[iI] *= 3;
		m_aiUnitClassWeights[iI] /= 4;
		m_aiUnitClassWeights[iI] = std::max(0, m_aiUnitClassWeights[iI]);
	}
	
	for (iI = 0; iI < GC.getNumUnitInfos(); iI++)
	{
		if (aiUnitCounts[iI] > 0)
		{
			UnitTypes eLoopUnit = (UnitTypes)iI;

			TechTypes eTech = (TechTypes)GC.getUnitInfo((UnitTypes)iI).getPrereqAndTech();
			
			int iEraDiff = (eTech == NO_TECH) ? 4 : std::min(4, getCurrentEra() - GC.getTechInfo(eTech).getEra());

			if (iEraDiff > 1)
			{
				iEraDiff -= 1;
				aiUnitCounts[iI] *= 3 - iEraDiff;
				aiUnitCounts[iI] /= 3;
			}
			FAssert(aiDomainSums[GC.getUnitInfo(eLoopUnit).getDomainType()] > 0);
			m_aiUnitClassWeights[GC.getUnitInfo(eLoopUnit).getUnitClassType()] += (5000 * aiUnitCounts[iI]) / std::max(1, aiDomainSums[GC.getUnitInfo(eLoopUnit).getDomainType()]);
		}
	}
	
	for (iI = 0; iI < GC.getNumUnitCombatInfos(); ++iI)
	{
		m_aiUnitCombatWeights[iI] = 0;
	}
	
	for (iI = 0; iI < GC.getNumUnitClassInfos(); iI++)
	{
		if (m_aiUnitClassWeights[iI] > 0)
		{
			UnitTypes eUnit = (UnitTypes)GC.getUnitClassInfo((UnitClassTypes)iI).getDefaultUnitIndex();
			m_aiUnitCombatWeights[GC.getUnitInfo(eUnit).getUnitCombatType()] += m_aiUnitClassWeights[iI];

		}
	}
	
	for (iI = 0; iI < GC.getNumUnitCombatInfos(); iI++)
	{
		if (m_aiUnitCombatWeights[iI] > 25)
		{
			m_aiUnitCombatWeights[iI] += 2500;
		}
		else if (m_aiUnitCombatWeights[iI] > 0)
		{
			m_aiUnitCombatWeights[iI] += 1000;
		}
	}
}

int CvPlayerAI::AI_calculateUnitAIViability(UnitAITypes eUnitAI, DomainTypes eDomain) const
{
	int iBestUnitAIStrength = 0;
	int iBestOtherStrength = 0;
	
	for (int iI = 0; iI < GC.getNumUnitClassInfos(); iI++)
	{
		UnitTypes eLoopUnit = (UnitTypes)GC.getUnitClassInfo((UnitClassTypes)iI).getDefaultUnitIndex();
/************************************************************************************************/
/* UNOFFICIAL_PATCH                       01/15/09                                jdog5000      */
/*                                                                                              */
/* Bugfix                                                                                       */
/************************************************************************************************/
/* original BTS code
		CvUnitInfo& kUnitInfo = GC.getUnitInfo((UnitTypes)iI);
*/
		CvUnitInfo& kUnitInfo = GC.getUnitInfo(eLoopUnit);
/************************************************************************************************/
/* UNOFFICIAL_PATCH                        END                                                  */
/************************************************************************************************/
		if (kUnitInfo.getDomainType() == eDomain)
		{

			TechTypes eTech = (TechTypes)kUnitInfo.getPrereqAndTech();
			if ((m_aiUnitClassWeights[iI] > 0) || GET_TEAM(getTeam()).isHasTech((eTech)))
			{
				if (kUnitInfo.getUnitAIType(eUnitAI))
				{
					iBestUnitAIStrength = std::max(iBestUnitAIStrength, kUnitInfo.getCombat());
				}

				iBestOtherStrength = std::max(iBestOtherStrength, kUnitInfo.getCombat());
			}
		}
	}
	
	return (100 * iBestUnitAIStrength) / std::max(1, iBestOtherStrength);
}

// K-Mod. Units with the lowest "disband value" will be disbanded first.
// (this code is based on the old code from CvPlayerAI::AI_disbandUnit)
int CvPlayerAI::AI_disbandValue(const CvUnit* pUnit, bool bMilitaryOnly) const
{
	if (pUnit->hasCargo() || pUnit->isGoldenAge() || pUnit->getUnitInfo().getProductionCost() < 0 || (bMilitaryOnly && !pUnit->canFight()))
		return MAX_INT;

	if (pUnit->isMilitaryHappiness() && pUnit->plot()->isCity() && pUnit->plot()->plotCount(PUF_isMissionAIType, MISSIONAI_GUARD_CITY, -1, getID()) < 2)
		return MAX_INT;

	int iValue = 1000 + GC.getGameINLINE().getSorenRandNum(200, "Disband Value");

	iValue *= 100 + (pUnit->getUnitInfo().getProductionCost() * 3);
	iValue /= 100;

	iValue *= 100 + std::max(pUnit->getExperience(), pUnit->getLevel() * pUnit->getLevel() * 2 / 3) * 15;
	iValue /= 100;

	/*if (plot()->getTeam() == getTeam())
	{
		iValue *= 2;

		if (canDefend() && plot()->isCity())
		{
			iValue *= 2;
		}
	}*/

	switch (pUnit->getGroup()->AI_getMissionAIType())
	{
	case MISSIONAI_GUARD_CITY:
	case MISSIONAI_PICKUP:
	case MISSIONAI_FOUND:
	case MISSIONAI_SPREAD:
		iValue *= 3; // high value
		break;

	case MISSIONAI_STRANDED:
		iValue /= 2;
	case MISSIONAI_RETREAT:
	case MISSIONAI_PATROL:
	case NO_MISSIONAI:
		break; // low value

	default:
		iValue *= 2; // medium
		break;
	}

	// Multiplying by higher number means unit has higher priority, less likely to be disbanded
	switch (pUnit->AI_getUnitAIType())
	{
	case UNITAI_UNKNOWN:
	case UNITAI_ANIMAL:
		break;

	case UNITAI_SETTLE:
		iValue *= 16;
		break;

	case UNITAI_WORKER:
		if (GC.getGame().getGameTurn() - pUnit->getGameTurnCreated() <= 10 ||
			!pUnit->plot()->isCity() ||
			pUnit->plot()->getPlotCity()->AI_getWorkersNeeded() > 0)
		{
			iValue *= 10;
		}
		break;

	case UNITAI_ATTACK:
	case UNITAI_COUNTER:
		iValue *= 4;
		break;

	case UNITAI_ATTACK_CITY:
	case UNITAI_COLLATERAL:
	case UNITAI_PILLAGE:
	case UNITAI_RESERVE:
		iValue *= 3;
		break;

	case UNITAI_CITY_DEFENSE:
	case UNITAI_CITY_COUNTER:
	case UNITAI_CITY_SPECIAL:
	case UNITAI_PARADROP:
		iValue *= 6;
		break;

	case UNITAI_EXPLORE:
		if (GC.getGame().getGameTurn() - pUnit->getGameTurnCreated() < 20)
			iValue *= 2;
		if (pUnit->plot()->getTeam() != getTeam())
			iValue *= 2;
		break;

	case UNITAI_MISSIONARY:
		if (GC.getGame().getGameTurn() - pUnit->getGameTurnCreated() < 10
			|| pUnit->plot()->getTeam() != getTeam())
		{
			iValue *= 8;
		}
		break;

	case UNITAI_PROPHET:
	case UNITAI_ARTIST:
	case UNITAI_SCIENTIST:
	case UNITAI_GENERAL:
	case UNITAI_MERCHANT:
	case UNITAI_ENGINEER:
	case UNITAI_GREAT_SPY:
		iValue *= 20;
		break;

	case UNITAI_SPY:
		iValue *= 10;
		break;

	case UNITAI_ICBM:
		iValue *= 5;
		break;

	case UNITAI_WORKER_SEA:
		iValue *= 15;
		break;

	case UNITAI_ATTACK_SEA:
	case UNITAI_RESERVE_SEA:
	case UNITAI_ESCORT_SEA:
		iValue *= 2;
		break;

	case UNITAI_EXPLORE_SEA:
		if (GC.getGame().getGameTurn() - pUnit->getGameTurnCreated() < 20)
			iValue *= 3;
		if (pUnit->plot()->getTeam() != getTeam())
			iValue *= 3;
		break;

	case UNITAI_SETTLER_SEA:
		iValue *= 6;
		break;

	case UNITAI_MISSIONARY_SEA:
	case UNITAI_SPY_SEA:
		iValue *= 4;
		break;

	case UNITAI_ASSAULT_SEA:
	case UNITAI_CARRIER_SEA:
	case UNITAI_MISSILE_CARRIER_SEA:
		if (GET_TEAM(getTeam()).getAnyWarPlanCount(true) > 0)
		{
			iValue *= 5;
		}
		else
		{
			iValue *= 2;
		}
		break;

	case UNITAI_PIRATE_SEA:
	case UNITAI_ATTACK_AIR:
		break;

	case UNITAI_DEFENSE_AIR:
	case UNITAI_CARRIER_AIR:
	case UNITAI_MISSILE_AIR:
		if (GET_TEAM(getTeam()).getAnyWarPlanCount(true) > 0)
		{
			iValue *= 5;
		}
		else
		{
			iValue *= 3;
		}
		break;

	default:
		FAssert(false);
		break;
	}

	if (pUnit->getUnitInfo().getExtraCost() > 0)
	{
		iValue /= pUnit->getUnitInfo().getExtraCost() + 1;
	}

	return iValue;
}
// K-Mod end

ReligionTypes CvPlayerAI::AI_chooseReligion()
{
	ReligionTypes eFavorite = (ReligionTypes)GC.getLeaderHeadInfo(getLeaderType()).getFavoriteReligion();
	if (NO_RELIGION != eFavorite && !GC.getGameINLINE().isReligionFounded(eFavorite))
	{
		return eFavorite;
	}

	std::vector<ReligionTypes> aeReligions;
	for (int iReligion = 0; iReligion < GC.getNumReligionInfos(); ++iReligion)
	{
		if (!GC.getGameINLINE().isReligionFounded((ReligionTypes)iReligion))
		{
			aeReligions.push_back((ReligionTypes)iReligion);
		}
	}

	if (aeReligions.size() > 0)
	{
		return aeReligions[GC.getGameINLINE().getSorenRandNum(aeReligions.size(), "AI pick religion")];
	}

	return NO_RELIGION;
}

int CvPlayerAI::AI_getAttitudeWeight(PlayerTypes ePlayer) const
{
	int iAttitudeWeight = 0;
	switch (AI_getAttitude(ePlayer))
	{
	case ATTITUDE_FURIOUS:
		iAttitudeWeight = -100;
		break;
	case ATTITUDE_ANNOYED:
		iAttitudeWeight = -50;
		break;
	case ATTITUDE_CAUTIOUS:
		iAttitudeWeight = 0;
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

int CvPlayerAI::AI_getPlotAirbaseValue(CvPlot* pPlot) const
{
	PROFILE_FUNC();
	
	FAssert(pPlot != NULL);
	
	if (pPlot->getTeam() != getTeam())
	{
		return 0;
	}
	
	if (pPlot->isCityRadius())
	{
		CvCity* pWorkingCity = pPlot->getWorkingCity();
		if (pWorkingCity != NULL)
		{
			if (pWorkingCity->AI_getBestBuild(pWorkingCity->getCityPlotIndex(pPlot)) != NO_BUILD)
			{
				return 0;
			}
			if (pPlot->getImprovementType() != NO_IMPROVEMENT)
			{
				CvImprovementInfo &kImprovementInfo = GC.getImprovementInfo(pPlot->getImprovementType());
				if (!kImprovementInfo.isActsAsCity())
				{
					return 0;
				}
			}
		}
	}
	
	int iMinOtherCityDistance = MAX_INT;
	CvPlot* iMinOtherCityPlot = NULL;
	
	int iMinFriendlyCityDistance = MAX_INT;
	CvPlot* iMinFriendlyCityPlot = NULL;
	
	int iOtherCityCount = 0;
	
	int iRange = 4;
	for (int iX = -iRange; iX <= iRange; iX++)
	{
		for (int iY = -iRange; iY <= iRange; iY++)
		{
			CvPlot* pLoopPlot = plotXY(pPlot->getX_INLINE(), pPlot->getY_INLINE(), iX, iY);
			if ((pLoopPlot != NULL) && (pPlot != pLoopPlot))
			{
				int iDistance = plotDistance(pPlot->getX_INLINE(), pPlot->getY_INLINE(), pLoopPlot->getX_INLINE(), pLoopPlot->getY_INLINE());
				
				if (pLoopPlot->getTeam() == getTeam())
				{
					if (pLoopPlot->isCity(true))
					{
						if (1 == iDistance)
						{
							return 0;
						}
						if (iDistance < iMinFriendlyCityDistance)
						{
							iMinFriendlyCityDistance = iDistance;
							iMinFriendlyCityPlot = pLoopPlot;
						}
					}
				}
				else
				{
					if (pLoopPlot->isOwned())
					{
						if (pLoopPlot->isCity(false))
						{
							if (iDistance < iMinOtherCityDistance)
							{
								iMinOtherCityDistance = iDistance;
								iMinOtherCityPlot = pLoopPlot;
								iOtherCityCount++;
							}
						}
					}
				}
			}
		}
	}
	
	if (0 == iOtherCityCount)
	{
		return 0;
	}
	
//	if (iMinFriendlyCityPlot != NULL)
//	{
//		FAssert(iMinOtherCityPlot != NULL);
//		if (plotDistance(iMinFriendlyCityPlot->getX_INLINE(), iMinFriendlyCityPlot->getY_INLINE(), iMinOtherCityPlot->getX_INLINE(), iMinOtherCityPlot->getY_INLINE()) < (iMinOtherCityDistance - 1))
//		{
//			return 0;
//		}
//	}

//	if (iMinOtherCityPlot != NULL)
//	{
//		CvCity* pNearestCity = GC.getMapINLINE().findCity(iMinOtherCityPlot->getX_INLINE(), iMinOtherCityPlot->getY_INLINE(), NO_PLAYER, getTeam(), false);
//		if (NULL == pNearestCity)
//		{
//			return 0;
//		}
//		if (plotDistance(pNearestCity->getX_INLINE(), pNearestCity->getY_INLINE(), iMinOtherCityPlot->getX_INLINE(), iMinOtherCityPlot->getY_INLINE()) < iRange)
//		{
//			return 0;
//		}
//	}
		
	
	int iDefenseModifier = pPlot->defenseModifier(getTeam(), false);
//	if (iDefenseModifier <= 0)
//	{
//		return 0;
//	}
	
	int iValue = iOtherCityCount * 50;
	iValue *= 100 + (2 * (iDefenseModifier + (pPlot->isHills() ? 25 : 0)));
	iValue /= 100;
	
	return iValue;
}

int CvPlayerAI::AI_getPlotCanalValue(CvPlot* pPlot) const
{
	PROFILE_FUNC();
	
	FAssert(pPlot != NULL);
	
	if (pPlot->isOwned())
	{
		if (pPlot->getTeam() != getTeam())
		{
			return 0;
		}
		if (pPlot->isCityRadius())
		{
			CvCity* pWorkingCity = pPlot->getWorkingCity();
			if (pWorkingCity != NULL)
			{
				if (pWorkingCity->AI_getBestBuild(pWorkingCity->getCityPlotIndex(pPlot)) != NO_BUILD)
				{
					return 0;
				}
				if (pPlot->getImprovementType() != NO_IMPROVEMENT)
				{
					CvImprovementInfo &kImprovementInfo = GC.getImprovementInfo(pPlot->getImprovementType());
					if (!kImprovementInfo.isActsAsCity())
					{
						return 0;
					}
				}
			}
		}
	}
	
	for (int iI = 0; iI < NUM_DIRECTION_TYPES; iI++)
	{
		CvPlot* pLoopPlot = plotDirection(pPlot->getX_INLINE(), pPlot->getY_INLINE(), (DirectionTypes)iI);
		if (pLoopPlot != NULL)
		{
			if (pLoopPlot->isCity(true))
			{
				return 0;
			}
		}
	}
	
	CvArea* pSecondWaterArea = pPlot->secondWaterArea();
	if (pSecondWaterArea == NULL)
	{
		return 0;
	}
	
	return 10 * std::min(0, pSecondWaterArea->getNumTiles() - 2);
}

//This returns approximately to the sum
//of the percentage values of each unit (there is no need to scale the output by iHappy)
//100 * iHappy means a high value.
int CvPlayerAI::AI_getHappinessWeight(int iHappy, int iExtraPop, bool bPercent) const
{
	int iWorstHappy = 0;
	int iBestHappy = 0;
	int iTotalUnhappy = 0;
	int iTotalHappy = 0;
	int iLoop;
	CvCity* pLoopCity;
	int iCount = 0;
	
	if (0 == iHappy)
	{
		iHappy = 1;
	}
	int iValue = 0;
	for (pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		/* original bts code
		int iCityHappy = pLoopCity->happyLevel() - pLoopCity->unhappyLevel(iExtraPop);
		
		iCityHappy -= std::max(0, pLoopCity->getCommerceHappiness());
		int iHappyNow = iCityHappy;
		int iHappyThen = iCityHappy +
			(bPercent ? ROUND_DIVIDE(pLoopCity->getPopulation()*iHappy, 100) : iHappy);
		
		//Integration
		int iTempValue = (((100 * iHappyThen - 10 * iHappyThen * iHappyThen)) - (100 * iHappyNow - 10 * iHappyNow * iHappyNow));
		if (iHappy > 0)
		{
			iValue += std::max(0, iTempValue);
		}
		else
		{
			iValue += std::min(0, iTempValue);
		} */

		// K-Mod. I have no idea what they were trying to 'integrate', and it was giving some strange answers.
		// So lets try it my way.
		if (pLoopCity->isNoUnhappiness())
			continue;
		int iCurrentHappy = 100*(pLoopCity->happyLevel() - pLoopCity->unhappyLevel(iExtraPop));
		// I'm only going to subtract half of the commerce happiness, because we might not be using that commerce for only happiness.
		iCurrentHappy -= 50*std::max(0, pLoopCity->getCommerceHappiness());
		int iTestHappy = iCurrentHappy +
			(bPercent ? ((pLoopCity->getPopulation()+iExtraPop)*iHappy) : 100 * iHappy);
		iValue += std::max(0, -iCurrentHappy) - std::max(0, -iTestHappy); // change in the number of angry citizens
		// a small bonus for happiness beyond what we need
		iValue += 100*(std::max(0, iTestHappy) - std::max(0, iCurrentHappy))/(400 + std::max(0, iTestHappy) + std::max(0, iCurrentHappy));
		// K-Mod end
		
		iCount++;
		/* original bts code - (huh?)
		if (iCount > 6)
		{
			break;
		}*/
	}
	
	return (0 == iCount) ? 50 * iHappy : iValue / iCount;
}

int CvPlayerAI::AI_getHealthWeight(int iHealth, int iExtraPop, bool bPercent) const
{
	int iWorstHealth = 0;
	int iBestHealth = 0;
	int iTotalUnhappy = 0;
	int iTotalHealth = 0;
	int iLoop;
	CvCity* pLoopCity;
	int iCount = 0;
	
	if (0 == iHealth)
	{
		iHealth = 1;
	}
	int iValue = 0;
	for (pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		/* original bts code
		int iCityHealth = pLoopCity->goodHealth() - pLoopCity->badHealth(false, iExtraPop);
		
		int iHealthNow = iCityHealth;
		int iHealthThen = iCityHealth +
			(bPercent ? ROUND_DIVIDE(pLoopCity->getPopulation()*iHealth, 100) : iHealth);
		
		//Integration
		int iTempValue = (((100 * iHealthThen - 6 * iHealthThen * iHealthThen)) - (100 * iHealthNow - 6 * iHealthNow * iHealthNow));
		if (iHealth > 0)
		{
			iValue += std::max(0, iTempValue);
		}
		else
		{
			iValue += std::min(0, iTempValue);
		} */

		// K-Mod. Changed to match the happiness function.
		int iCurrentHealth = 100*(pLoopCity->goodHealth() - pLoopCity->badHealth(false, iExtraPop));
		int iTestHealth = iCurrentHealth +
			(bPercent ? ((pLoopCity->getPopulation()+iExtraPop)*iHealth) : 100 * iHealth);
		iValue += std::max(0, -iCurrentHealth) - std::max(0, -iTestHealth); // change in the number of angry citizens
		// a small bonus for happiness beyond what we need
		iValue += 100*(std::max(0, iTestHealth) - std::max(0, iCurrentHealth))/(400 + std::max(0, iTestHealth) + std::max(0, iCurrentHealth));
		// K-Mod end

		iCount++;
		/* original bts code
		if (iCount > 6)
		{
			break;
		} */
	}
	
/************************************************************************************************/
/* UNOFFICIAL_PATCH                       10/21/09                                jdog5000      */
/*                                                                                              */
/* Bugfix                                                                                       */
/************************************************************************************************/
/* orginal bts code
	return (0 == iCount) ? 50 : iValue / iCount;
*/
	// Mirror happiness valuation code
	return (0 == iCount) ? 50*iHealth : iValue / iCount;
/************************************************************************************************/
/* UNOFFICIAL_PATCH                        END                                                  */
/************************************************************************************************/
}
	
void CvPlayerAI::AI_invalidateCloseBordersAttitudeCache()
{
	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		m_aiCloseBordersAttitudeCache[i] = MAX_INT;
		// From Sanguo Mod Performance, ie the CAR Mod
		// Attitude cache
		AI_invalidateAttitudeCache((PlayerTypes)i);
		//
	}
}

bool CvPlayerAI::AI_isPlotThreatened(CvPlot* pPlot, int iRange, bool bTestMoves) const
{
	PROFILE_FUNC();

	CvArea *pPlotArea = pPlot->area();

	if (iRange == -1)
	{
		iRange = DANGER_RANGE;
	}

	for (int iDX = -iRange; iDX <= iRange; iDX++)
	{
		for (int iDY = -iRange; iDY <= iRange; iDY++)
		{
			CvPlot* pLoopPlot = plotXY(pPlot->getX_INLINE(), pPlot->getY_INLINE(), iDX, iDY);

			if (pLoopPlot != NULL)
			{
				if (pLoopPlot->area() == pPlotArea)
				{
					for (CLLNode<IDInfo>* pUnitNode = pLoopPlot->headUnitNode(); pUnitNode != NULL; pUnitNode = pLoopPlot->nextUnitNode(pUnitNode))
					{
						CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
						if (pLoopUnit->isEnemy(getTeam()) && pLoopUnit->canAttack() && !pLoopUnit->isInvisible(getTeam(), false))
						{
							if (pLoopUnit->canMoveOrAttackInto(pPlot))
							{
								int iPathTurns = 0;
								if (bTestMoves)
								{
									/* original bts code
									if (!pLoopUnit->getGroup()->generatePath(pLoopPlot, pPlot, MOVE_MAX_MOVES | MOVE_IGNORE_DANGER, false, &iPathTurns))
									{
										iPathTurns = MAX_INT;
									}*/

									// K-Mod. Use a temp pathfinder, so as not to interfere with the normal one.
									KmodPathFinder temp_finder;
									temp_finder.SetSettings(pLoopUnit->getGroup(), MOVE_MAX_MOVES | MOVE_IGNORE_DANGER, 1, GC.getMOVE_DENOMINATOR());
									if (temp_finder.GeneratePath(pPlot))
										iPathTurns = 1;
									// K-Mod end
								}

								if (iPathTurns <= 1)
								{
									return true;
								}
							}
						}
					}
				}
			}
		}
	}

	return false;
}

bool CvPlayerAI::AI_isFirstTech(TechTypes eTech) const
{
	for (int iI = 0; iI < GC.getNumReligionInfos(); iI++)
	{
		if (GC.getReligionInfo((ReligionTypes)iI).getTechPrereq() == eTech)
		{
			if (!(GC.getGameINLINE().isReligionSlotTaken((ReligionTypes)iI)))
			{
				return true;
			}
		}
	}

	if (GC.getGameINLINE().countKnownTechNumTeams(eTech) == 0)
	{
		if ((getTechFreeUnit(eTech) != NO_UNIT) ||
			(GC.getTechInfo(eTech).getFirstFreeTechs() > 0))
		{
			return true;
		}
	}

	return false;
}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      09/03/09                       poyuzhe & jdog5000     */
/*                                                                                              */
/* Efficiency                                                                                   */
/************************************************************************************************/
// From Sanguo Mod Performance, ie the CAR Mod
// Attitude cache
void CvPlayerAI::AI_invalidateAttitudeCache(PlayerTypes ePlayer) const
{
	m_aiAttitudeCache[ePlayer] = MAX_INT;
}

void CvPlayerAI::AI_invalidateAttitudeCache() const
{
	for( int iI = 0; iI < MAX_PLAYERS; iI++ )
	{
		AI_invalidateAttitudeCache((PlayerTypes)iI);
	}
}

// K-Mod
void CvPlayerAI::AI_ClearConstructionValueCache()
{
	CvCity *pLoopCity;
	int iLoop;
	for (pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
	{
		// Have I meantioned that the way the "AI" classes are used in this code is an abomination and an insult to OO programming?
		static_cast<CvCityAI*>(pLoopCity)->AI_ClearConstructionValueCache();
	}
}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/


