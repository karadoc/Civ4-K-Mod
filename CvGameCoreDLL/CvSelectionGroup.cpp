// selectionGroup.cpp

#include "CvGameCoreDLL.h"
#include "CvGlobals.h"
#include "CvSelectionGroup.h"
#include "CvGameAI.h"
#include "CvPlayerAI.h"
#include "CvTeamAI.h"
#include "CvUnit.h"
#include "CvGameCoreUtils.h"
#include "CvMap.h"
#include "CvPlot.h"
#include "CvDLLEntityIFaceBase.h"
#include "CvDLLInterfaceIFaceBase.h"
#include "CvDLLFAStarIFaceBase.h"
#include "FAStarNode.h"
#include "CvInfos.h"
#include "FProfiler.h"
#include "CyPlot.h"
#include "CySelectionGroup.h"
#include "CyArgsList.h"
#include "CvDLLPythonIFaceBase.h"
#include <set>
#include "CvEventReporter.h"
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      03/30/10                                jdog5000      */
/*                                                                                              */
/* AI Logging                                                                                   */
/************************************************************************************************/
#include "BetterBTSAI.h"
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

//	KOSHLING MOD - path validity cache, used only within a single
//	call to GneratePath()
const CvSelectionGroup* CvSelectionGroup::lastPathGeneratedFor = NULL; // K-Mod
KmodPathFinder CvSelectionGroup::path_finder; // K-Mod
int CvSelectionGroup::m_cachedPathValidityFromPlotX = -2;
int CvSelectionGroup::m_cachedPathValidityFromPlotY;
bool CvSelectionGroup::m_cachedPathValidityResult;
int CvSelectionGroup::m_pathValidityCacheReads = 0;
int CvSelectionGroup::m_pathValidityCacheHits = 0;

// Public Functions...

CvSelectionGroup::CvSelectionGroup()
{
	reset(0, NO_PLAYER, true);
}


CvSelectionGroup::~CvSelectionGroup()
{
	uninit();
}


void CvSelectionGroup::init(int iID, PlayerTypes eOwner)
{
	//--------------------------------
	// Init saved data
	reset(iID, eOwner);

	//--------------------------------
	// Init non-saved data

	//--------------------------------
	// Init other game data
	AI_init();
}


void CvSelectionGroup::uninit()
{
	m_units.clear();

	m_missionQueue.clear();
}


// FUNCTION: reset()
// Initializes data members that are serialized.
void CvSelectionGroup::reset(int iID, PlayerTypes eOwner, bool bConstructorCall)
{
	//--------------------------------
	// Uninit class
	uninit();

	m_iID = iID;
	m_iMissionTimer = 0;

	m_bForceUpdate = false;

	m_eOwner = eOwner;

	m_eActivityType = ACTIVITY_AWAKE;
	m_eAutomateType = NO_AUTOMATE;
	m_bIsBusyCache = false;

	if (!bConstructorCall)
	{
		AI_reset();
	}
}


void CvSelectionGroup::kill()
{
	FAssert(getOwnerINLINE() != NO_PLAYER);
	FAssertMsg(getID() != FFreeList::INVALID_INDEX, "getID() is not expected to be equal with FFreeList::INVALID_INDEX");
	FAssertMsg(getNumUnits() == 0, "The number of units is expected to be 0");

	GET_PLAYER(getOwnerINLINE()).removeGroupCycle(getID());

	GET_PLAYER(getOwnerINLINE()).deleteSelectionGroup(getID());
}

bool CvSelectionGroup::sentryAlert() const
{
	CvUnit* pHeadUnit = NULL;
	int iMaxRange = 0;
	CLLNode<IDInfo>* pUnitNode = headUnitNode();
	while (pUnitNode != NULL)
	{
		CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
		pUnitNode = nextUnitNode(pUnitNode);

		int iRange = pLoopUnit->visibilityRange() + 1;

		if (iRange > iMaxRange)
		{
			iMaxRange = iRange;
			pHeadUnit = pLoopUnit;
		}
	}

	if (NULL != pHeadUnit)
	{
		for (int iX = -iMaxRange; iX <= iMaxRange; ++iX)
		{
			for (int iY = -iMaxRange; iY <= iMaxRange; ++iY)
			{
				CvPlot* pPlot = ::plotXY(pHeadUnit->getX_INLINE(), pHeadUnit->getY_INLINE(), iX, iY);
				if (NULL != pPlot)
				{
					if (pHeadUnit->plot()->canSeePlot(pPlot, pHeadUnit->getTeam(), iMaxRange - 1, NO_DIRECTION))
					{
						if (pPlot->isVisibleEnemyUnit(pHeadUnit))
						{
							return true;
						}
					}
				}
			}
		}
	}

	return false;
}

void CvSelectionGroup::doTurn()
{
	PROFILE("CvSelectionGroup::doTurn()")

	CLLNode<IDInfo>* pUnitNode;
	CvUnit* pLoopUnit;
	int iWaitTurns;
	int iBestWaitTurns;

	FAssert(getOwnerINLINE() != NO_PLAYER);

	if (getNumUnits() > 0)
	{
		bool bHurt = false;
		
		// do unit's turns (checking for damage)
		pUnitNode = headUnitNode();
		while (pUnitNode != NULL)
		{
			pLoopUnit = ::getUnit(pUnitNode->m_data);
			pUnitNode = nextUnitNode(pUnitNode);

			pLoopUnit->doTurn();

			if (pLoopUnit->isHurt())
			{
				bHurt = true;
			}
		}

		ActivityTypes eActivityType = getActivityType();

		// wake unit if skipped last turn 
		//		or healing and automated or no longer hurt (automated healing is one turn at a time)
		//		or on sentry and there is danger
		if ((eActivityType == ACTIVITY_HOLD) ||
			((eActivityType == ACTIVITY_HEAL) && (AI_isControlled() || !bHurt)) ||
			((eActivityType == ACTIVITY_SENTRY) && (sentryAlert())))
		{
			setActivityType(ACTIVITY_AWAKE);
		}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      09/21/08                                jdog5000      */
/*                                                                                              */
/* Air AI                                                                                       */
/************************************************************************************************/
		// with improvements to launching air patrols, now can wake every turn
		if ( (eActivityType == ACTIVITY_INTERCEPT) && !isHuman() )
		{
			setActivityType(ACTIVITY_AWAKE);
		}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/	

		if (AI_isControlled())
		{
			if ((getActivityType() != ACTIVITY_MISSION) || (!canFight() && (GET_PLAYER(getOwnerINLINE()).AI_getAnyPlotDanger(plot(), 2))))
			{
				//setForceUpdate(true);
				doForceUpdate(); // K-Mod. (this prevents the forced update from interupting amphibious assaults.
			}
		}
		else
		{
			if (getActivityType() == ACTIVITY_MISSION)
			{
				bool bNonSpy = false;
				for (CLLNode<IDInfo>* pUnitNode = headUnitNode(); pUnitNode != NULL; pUnitNode = nextUnitNode(pUnitNode))
				{
					CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
					if (!pLoopUnit->isSpy())
					{
						bNonSpy = true;
						break;
					}
				}

				//if (bNonSpy && GET_PLAYER(getOwnerINLINE()).AI_getPlotDanger(plot(), 2) > 0)
				if (bNonSpy && GET_PLAYER(getOwnerINLINE()).AI_getAnyPlotDanger(plot(), 2)) //bbai
				{
					clearMissionQueue();
				}
			}
		}

		if (isHuman())
		{
			if (GC.getGameINLINE().isMPOption(MPOPTION_SIMULTANEOUS_TURNS))
			{
				iBestWaitTurns = 0;

				pUnitNode = headUnitNode();

				while (pUnitNode != NULL)
				{
					pLoopUnit = ::getUnit(pUnitNode->m_data);
					pUnitNode = nextUnitNode(pUnitNode);

					iWaitTurns = (GC.getDefineINT("MIN_TIMER_UNIT_DOUBLE_MOVES") - (GC.getGameINLINE().getTurnSlice() - pLoopUnit->getLastMoveTurn()));

					if (iWaitTurns > iBestWaitTurns)
					{
						iBestWaitTurns = iWaitTurns;
					}
				}

				setMissionTimer(std::max(iBestWaitTurns, getMissionTimer()));

				if (iBestWaitTurns > 0)
				{
					// Cycle selection if the current group is selected
					CvUnit* pSelectedUnit = gDLL->getInterfaceIFace()->getHeadSelectedUnit();
					if (pSelectedUnit && pSelectedUnit->getGroup() == this)
					{
						gDLL->getInterfaceIFace()->selectGroup(pSelectedUnit, false, false, false);
					}
				}
			}
		}
	}

	doDelayedDeath();
}

bool CvSelectionGroup::showMoves() const
{
	if (GC.getGameINLINE().isMPOption(MPOPTION_SIMULTANEOUS_TURNS) || GC.getGameINLINE().isSimultaneousTeamTurns())
	{
		return false;
	}

	for (int iI = 0; iI < MAX_CIV_PLAYERS; iI++)
	{
		CvPlayer& kLoopPlayer = GET_PLAYER((PlayerTypes)iI);
		if (kLoopPlayer.isAlive())
		{
			if (kLoopPlayer.isHuman())
			{
				CvUnit* pHeadUnit = getHeadUnit();

				if (NULL != pHeadUnit)
				{
					if (pHeadUnit->isEnemy(kLoopPlayer.getTeam()))
					{
						if (kLoopPlayer.isOption(PLAYEROPTION_SHOW_ENEMY_MOVES))
						{
							return true;
						}
					}
					else
					{
						if (kLoopPlayer.isOption(PLAYEROPTION_SHOW_FRIENDLY_MOVES))
						{
							return true;
						}
					}
				}
			}
		}
	}

	return false;
}


void CvSelectionGroup::updateTimers()
{
	CLLNode<IDInfo>* pUnitNode;
	CvUnit* pLoopUnit;
	bool bCombat;

	FAssert(getOwnerINLINE() != NO_PLAYER);

	if (getNumUnits() > 0)
	{
		bCombat = false;

		pUnitNode = headUnitNode();

		while (pUnitNode != NULL)
		{
			pLoopUnit = ::getUnit(pUnitNode->m_data);
			pUnitNode = nextUnitNode(pUnitNode);

			if (pLoopUnit->isCombat())
			{
				if (pLoopUnit->isAirCombat())
				{
					pLoopUnit->updateAirCombat();
				}
				else
				{
					pLoopUnit->updateCombat();
				}

				bCombat = true;
				//break; // disabled by K-Mod.
				// (I've changed groupAttack to fix a problem, and now multiple units in a single group can be queued for combat.)
				// so this break can cause an infinite loop, as the currently fighting unit might not get updated.
			}
		}

		if (!bCombat)
		{
			updateMission();
		}
	}

	doDelayedDeath();
}


// Returns true if group was killed...
bool CvSelectionGroup::doDelayedDeath()
{
	CLLNode<IDInfo>* pUnitNode;
	CvUnit* pLoopUnit;

	FAssert(getOwnerINLINE() != NO_PLAYER);

	if (isBusy())
	{
		return false;
	}

	pUnitNode = headUnitNode();

	while (pUnitNode != NULL)
	{
		pLoopUnit = ::getUnit(pUnitNode->m_data);
		pUnitNode = nextUnitNode(pUnitNode);

		pLoopUnit->doDelayedDeath();
	}

	if (getNumUnits() == 0)
	{
		kill();
		return true;
	}

	return false;
}


void CvSelectionGroup::playActionSound()
{
	// Pitboss should not be playing sounds!
#ifndef PITBOSS

	CvUnit *pHeadUnit;
	int iScriptId = -1;

	pHeadUnit = getHeadUnit();
	if ( pHeadUnit )
	{
		iScriptId = pHeadUnit->getArtInfo(0, GET_PLAYER(getOwnerINLINE()).getCurrentEra())->getActionSoundScriptId();
	}

	if ( (iScriptId == -1) && pHeadUnit )
	{
		CvCivilizationInfo *pCivInfo;
		pCivInfo = &GC.getCivilizationInfo( pHeadUnit->getCivilizationType() );
		if ( pCivInfo )
		{
			iScriptId = pCivInfo->getActionSoundScriptId();
		}
	}

	if ( (iScriptId != -1) && pHeadUnit )
	{
		CvPlot *pPlot = GC.getMapINLINE().plotINLINE(pHeadUnit->getX_INLINE(),pHeadUnit->getY_INLINE());
		if ( pPlot )
		{
			gDLL->Do3DSound( iScriptId, pPlot->getPoint() );
		}
	}

#endif // n PITBOSS
}


void CvSelectionGroup::pushMission(MissionTypes eMission, int iData1, int iData2, int iFlags, bool bAppend, bool bManual, MissionAITypes eMissionAI, CvPlot* pMissionAIPlot, CvUnit* pMissionAIUnit)
{
	PROFILE_FUNC();

	MissionData mission;

	FAssert(getOwnerINLINE() != NO_PLAYER);

	if (!bAppend)
	{
		if (isBusy())
		{
			return;
		}

		clearMissionQueue();
	}

	if (bManual)
	{
		setAutomateType(NO_AUTOMATE);
	}

	mission.eMissionType = eMission;
	mission.iData1 = iData1;
	mission.iData2 = iData2;
	mission.iFlags = iFlags;
	mission.iPushTurn = GC.getGameINLINE().getGameTurn();

	if (canAllMove()) // K-Mod. Do not set the AI mission type if this is just a "follow" command!
		AI_setMissionAI(eMissionAI, pMissionAIPlot, pMissionAIUnit);

	//insertAtEndMissionQueue(mission, !bAppend);
	insertAtEndMissionQueue(mission, !bAppend || AI_isControlled()); // K-Mod (AI commands should execute immediately)

	if (bManual)
	{
		if (getOwnerINLINE() == GC.getGameINLINE().getActivePlayer())
		{
			if (isBusy() && GC.getMissionInfo(eMission).isSound())
			{
				playActionSound();
			}

			gDLL->getInterfaceIFace()->setHasMovedUnit(true);
		}

		CvEventReporter::getInstance().selectionGroupPushMission(this, eMission);

		doDelayedDeath();
	}
}


void CvSelectionGroup::popMission()
{
	CLLNode<MissionData>* pTailNode;

	FAssert(getOwnerINLINE() != NO_PLAYER);

	pTailNode = tailMissionQueueNode();

	if (pTailNode != NULL)
	{
		deleteMissionQueueNode(pTailNode);
	}
}


void CvSelectionGroup::autoMission()
{
	FAssert(getOwnerINLINE() != NO_PLAYER);

	if (getNumUnits() > 0)
	{
		if (headMissionQueueNode() != NULL)
		{
			if (!isBusy())
			{
				bool bVisibleHuman = false;
				//if (isHuman())
				if (!AI_isControlled()) // K-Mod. (otherwise the automation will just reissue commands immediately after they are cleared, resulting in an infinite loop.)
				{
					for (CLLNode<IDInfo>* pUnitNode = headUnitNode(); pUnitNode != NULL; pUnitNode = nextUnitNode(pUnitNode))
					{
						CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
						if (!pLoopUnit->alwaysInvisible())
						{
							bVisibleHuman = true;
							break;
						}
					}
				}

				//if (bVisibleHuman && GET_PLAYER(getOwnerINLINE()).AI_getPlotDanger(plot(), 1) > 0)
				// K-Mod. I want to allow players to queue actions when in danger without being overruled by this clause.
				if (bVisibleHuman && GET_PLAYER(getOwnerINLINE()).AI_getAnyPlotDanger(plot(), 1) && headMissionQueueNode()->m_data.iPushTurn != GC.getGameINLINE().getGameTurn())
				// K-Mod end
				{
					clearMissionQueue();
				}
				else
				{
					if (getActivityType() == ACTIVITY_MISSION)
					{
						continueMission();
					}
					else
					{
						startMission();
					}
				}
			}
		}
	}

	doDelayedDeath();
}


void CvSelectionGroup::updateMission()
{
	FAssert(getOwnerINLINE() != NO_PLAYER);

	if (getMissionTimer() > 0)
	{
		changeMissionTimer(-1);

		if (getMissionTimer() == 0)
		{
			if (getActivityType() == ACTIVITY_MISSION)
			{
				continueMission();
			}
			else
			{
				if (getOwnerINLINE() == GC.getGameINLINE().getActivePlayer())
				{
					if (gDLL->getInterfaceIFace()->getHeadSelectedUnit() == NULL)
					{
						GC.getGameINLINE().cycleSelectionGroups_delayed(1, true);
					}
				}
			}
		}
	}
}


CvPlot* CvSelectionGroup::lastMissionPlot()
{
	CLLNode<MissionData>* pMissionNode;
	CvUnit* pTargetUnit;

	pMissionNode = tailMissionQueueNode();

	while (pMissionNode != NULL)
	{
		switch (pMissionNode->m_data.eMissionType)
		{
		case MISSION_MOVE_TO:
		case MISSION_ROUTE_TO:
			return GC.getMapINLINE().plotINLINE(pMissionNode->m_data.iData1, pMissionNode->m_data.iData2);
			break;

		case MISSION_MOVE_TO_UNIT:
			pTargetUnit = GET_PLAYER((PlayerTypes)pMissionNode->m_data.iData1).getUnit(pMissionNode->m_data.iData2);
			if (pTargetUnit != NULL)
			{
				return pTargetUnit->plot();
			}
			break;

		case MISSION_SKIP:
		case MISSION_SLEEP:
		case MISSION_FORTIFY:
		case MISSION_PLUNDER:
		case MISSION_AIRPATROL:
		case MISSION_SEAPATROL:
		case MISSION_HEAL:
		case MISSION_SENTRY:
		case MISSION_AIRLIFT:
		case MISSION_NUKE:
		case MISSION_RECON:
		case MISSION_PARADROP:
		case MISSION_AIRBOMB:
		case MISSION_BOMBARD:
		case MISSION_RANGE_ATTACK:
		case MISSION_PILLAGE:
		case MISSION_SABOTAGE:
		case MISSION_DESTROY:
		case MISSION_STEAL_PLANS:
		case MISSION_FOUND:
		case MISSION_SPREAD:
		case MISSION_SPREAD_CORPORATION:
		case MISSION_JOIN:
		case MISSION_CONSTRUCT:
		case MISSION_DISCOVER:
		case MISSION_HURRY:
		case MISSION_TRADE:
		case MISSION_GREAT_WORK:
		case MISSION_INFILTRATE:
		case MISSION_GOLDEN_AGE:
		case MISSION_BUILD:
		case MISSION_LEAD:
		case MISSION_ESPIONAGE:
		case MISSION_DIE_ANIMATION:
			break;

		default:
			FAssert(false);
			break;
		}

		pMissionNode = prevMissionQueueNode(pMissionNode);
	}

	return plot();
}


bool CvSelectionGroup::canStartMission(int iMission, int iData1, int iData2, CvPlot* pPlot, bool bTestVisible, bool bUseCache)
{
	//PROFILE_FUNC();

	CLLNode<IDInfo>* pUnitNode;
	CvUnit* pTargetUnit;
	CvUnit* pLoopUnit;

	//cache isBusy
	if(bUseCache)
	{
		if(m_bIsBusyCache)
		{
			return false;
		}
	}
	else
	{
		if (isBusy())
		{
			return false;
		}
	}

	if (pPlot == NULL)
	{
		pPlot = plot();
	}

	pUnitNode = headUnitNode();

	while (pUnitNode != NULL)
	{
		pLoopUnit = ::getUnit(pUnitNode->m_data);
		pUnitNode = nextUnitNode(pUnitNode);

		switch (iMission)
		{
		case MISSION_MOVE_TO:
			if (!(pPlot->at(iData1, iData2)))
			{
				return true;
			}
			break;

		case MISSION_ROUTE_TO:
			if (!(pPlot->at(iData1, iData2)) || (getBestBuildRoute(pPlot) != NO_ROUTE))
			{
				return true;
			}
			break;

		case MISSION_MOVE_TO_UNIT:
			FAssertMsg(iData1 != NO_PLAYER, "iData1 should be a valid Player");
			pTargetUnit = GET_PLAYER((PlayerTypes)iData1).getUnit(iData2);
			if ((pTargetUnit != NULL) && !(pTargetUnit->atPlot(pPlot)))
			{
				return true;
			}
			break;

		case MISSION_SKIP:
			if (pLoopUnit->canHold(pPlot))
			{
				return true;
			}
			break;

		case MISSION_SLEEP:
			if (pLoopUnit->canSleep(pPlot))
			{
				return true;
			}
			break;

		case MISSION_FORTIFY:
			if (pLoopUnit->canFortify(pPlot))
			{
				return true;
			}
			break;

		case MISSION_AIRPATROL:
			if (pLoopUnit->canAirPatrol(pPlot))
			{
				return true;
			}
			break;

		case MISSION_SEAPATROL:
			if (pLoopUnit->canSeaPatrol(pPlot))
			{
				return true;
			}
			break;

		case MISSION_HEAL:
			if (pLoopUnit->canHeal(pPlot))
			{
				return true;
			}
			break;

		case MISSION_SENTRY:
			if (pLoopUnit->canSentry(pPlot))
			{
				return true;
			}
			break;

		case MISSION_AIRLIFT:
			if (pLoopUnit->canAirliftAt(pPlot, iData1, iData2))
			{
				return true;
			}
			break;

		case MISSION_NUKE:
			if (pLoopUnit->canNukeAt(pPlot, iData1, iData2))
			{
				return true;
			}
			break;

		case MISSION_RECON:
			if (pLoopUnit->canReconAt(pPlot, iData1, iData2))
			{
				return true;
			}
			break;

		case MISSION_PARADROP:
			if (pLoopUnit->canParadropAt(pPlot, iData1, iData2))
			{
				return true;
			}
			break;

		case MISSION_AIRBOMB:
			if (pLoopUnit->canAirBombAt(pPlot, iData1, iData2))
			{
				return true;
			}
			break;

		case MISSION_BOMBARD:
			if (pLoopUnit->canBombard(pPlot))
			{
				return true;
			}
			break;

		case MISSION_RANGE_ATTACK:
			if (pLoopUnit->canRangeStrikeAt(pPlot, iData1, iData2))
			{
				return true;
			}
			break;

		case MISSION_PLUNDER:
			if (pLoopUnit->canPlunder(pPlot, bTestVisible))
			{
				return true;
			}
			break;

		case MISSION_PILLAGE:
			if (pLoopUnit->canPillage(pPlot))
			{
				return true;
			}
			break;

		case MISSION_SABOTAGE:
			if (pLoopUnit->canSabotage(pPlot, bTestVisible))
			{
				return true;
			}
			break;

		case MISSION_DESTROY:
			if (pLoopUnit->canDestroy(pPlot, bTestVisible))
			{
				return true;
			}
			break;

		case MISSION_STEAL_PLANS:
			if (pLoopUnit->canStealPlans(pPlot, bTestVisible))
			{
				return true;
			}
			break;

		case MISSION_FOUND:
			if (pLoopUnit->canFound(pPlot, bTestVisible))
			{
				return true;
			}
			break;

		case MISSION_SPREAD:
			if (pLoopUnit->canSpread(pPlot, ((ReligionTypes)iData1), bTestVisible))
			{
				return true;
			}
			break;

		case MISSION_SPREAD_CORPORATION:
			if (pLoopUnit->canSpreadCorporation(pPlot, ((CorporationTypes)iData1), bTestVisible))
			{
				return true;
			}
			break;

		case MISSION_JOIN:
			if (pLoopUnit->canJoin(pPlot, ((SpecialistTypes)iData1)))
			{
				return true;
			}
			break;

		case MISSION_CONSTRUCT:
			if (pLoopUnit->canConstruct(pPlot, ((BuildingTypes)iData1), bTestVisible))
			{
				return true;
			}
			break;

		case MISSION_DISCOVER:
			if (pLoopUnit->canDiscover(pPlot))
			{
				return true;
			}
			break;

		case MISSION_HURRY:
			if (pLoopUnit->canHurry(pPlot, bTestVisible))
			{
				return true;
			}
			break;

		case MISSION_TRADE:
			if (pLoopUnit->canTrade(pPlot, bTestVisible))
			{
				return true;
			}
			break;

		case MISSION_GREAT_WORK:
			if (pLoopUnit->canGreatWork(pPlot))
			{
				return true;
			}
			break;

		case MISSION_INFILTRATE:
			if (pLoopUnit->canInfiltrate(pPlot, bTestVisible))
			{
				return true;
			}
			break;

		case MISSION_GOLDEN_AGE:
			//this means to play the animation only			
			if (iData1 != -1)
			{
				return true;
			}

			if (pLoopUnit->canGoldenAge(pPlot, bTestVisible))
			{
				return true;
			}
			break;

		case MISSION_BUILD:

            FAssertMsg(((BuildTypes)iData1) < GC.getNumBuildInfos(), "Invalid Build");
            if (pLoopUnit->canBuild(pPlot, ((BuildTypes)iData1), bTestVisible))
            {
                return true;
            }
			break;

		case MISSION_LEAD:
			if (pLoopUnit->canLead(pPlot, iData1))
			{
				return true;
			}
			break;

		case MISSION_ESPIONAGE:
			if (pLoopUnit->canEspionage(pPlot, bTestVisible))
			{
				return true;
			}
			break;

		case MISSION_DIE_ANIMATION:
			return false;
			break;

		case MISSION_BEGIN_COMBAT:
		case MISSION_END_COMBAT:
		case MISSION_AIRSTRIKE:
		case MISSION_SURRENDER:
		case MISSION_IDLE:
		case MISSION_DIE:
		case MISSION_DAMAGE:
		case MISSION_MULTI_SELECT:
		case MISSION_MULTI_DESELECT:
			break;

		default:
			FAssert(false);
			break;
		}
	}

	return false;
}


void CvSelectionGroup::startMission()
{
	//PROFILE_FUNC();

	CLLNode<IDInfo>* pUnitNode;
	CvUnit* pLoopUnit;
	bool bDelete;
	bool bAction;
	bool bNuke;
	bool bNotify;

	FAssert(!isBusy());
	FAssert(getOwnerINLINE() != NO_PLAYER);
	FAssert(headMissionQueueNode() != NULL);

	if (!GC.getGameINLINE().isMPOption(MPOPTION_SIMULTANEOUS_TURNS))
	{
		if (!GET_PLAYER(getOwnerINLINE()).isTurnActive())
		{
			if (getOwnerINLINE() == GC.getGameINLINE().getActivePlayer())
			{
				if (IsSelected())
				{
					GC.getGameINLINE().cycleSelectionGroups_delayed(1, true);
				}
			}

			return;
		}
	}

	//if (canAllMove())
	if (canAllMove() || (headMissionQueueNode()->m_data.iFlags & MOVE_DIRECT_ATTACK && headMissionQueueNode()->m_data.eMissionType == MISSION_MOVE_TO && canAnyMove())) // K-Mod
	{
		setActivityType(ACTIVITY_MISSION);
	}
	else
	{
		setActivityType(ACTIVITY_HOLD);
	}


	bDelete = false;
	bAction = false;
	bNuke = false;
	bNotify = false;

	if (!canStartMission(headMissionQueueNode()->m_data.eMissionType, headMissionQueueNode()->m_data.iData1, headMissionQueueNode()->m_data.iData2, plot()))
	{
		bDelete = true;
	}
	else
	{
		FAssertMsg(GET_PLAYER(getOwnerINLINE()).isTurnActive() || GET_PLAYER(getOwnerINLINE()).isHuman(), "It's expected that either the turn is active for this player or the player is human");

		switch (headMissionQueueNode()->m_data.eMissionType)
		{
		case MISSION_MOVE_TO:
			// K-Mod. Prevent human players from accidentally attacking units that they can't see.
			if (isHuman() && !GC.getMapINLINE().plotINLINE(headMissionQueueNode()->m_data.iData1, headMissionQueueNode()->m_data.iData2)->isVisible(getTeam(), false))
				headMissionQueueNode()->m_data.iFlags |= MOVE_NO_ATTACK;

			// also, we should allow an amphibious landing even if we are out of moves.
			if (!canAllMove())
			{
				if (groupAmphibMove(GC.getMapINLINE().plotINLINE(headMissionQueueNode()->m_data.iData1, headMissionQueueNode()->m_data.iData2), headMissionQueueNode()->m_data.iFlags))
				{
					bDelete = true;
				}
			}
			// K-Mod end
		case MISSION_ROUTE_TO:
		case MISSION_MOVE_TO_UNIT:
			break;

		case MISSION_SKIP:
			setActivityType(ACTIVITY_HOLD);
			bDelete = true;
			break;

		case MISSION_SLEEP:
			setActivityType(ACTIVITY_SLEEP);
			bNotify = true;
			bDelete = true;
			break;

		case MISSION_FORTIFY:
			setActivityType(ACTIVITY_SLEEP);
			bNotify = true;
			bDelete = true;
			break;

		case MISSION_PLUNDER:
			setActivityType(ACTIVITY_PLUNDER);
			bNotify = true;
			bDelete = true;
			break;

		case MISSION_AIRPATROL:
			setActivityType(ACTIVITY_INTERCEPT);
			bDelete = true;
			break;

		case MISSION_SEAPATROL:
			setActivityType(ACTIVITY_PATROL);
			bDelete = true;
			break;

		case MISSION_HEAL:
			setActivityType(ACTIVITY_HEAL);
			bNotify = true;
			bDelete = true;
			break;

		case MISSION_SENTRY:
			setActivityType(ACTIVITY_SENTRY);
			bNotify = true;
			bDelete = true;
			break;

		case MISSION_AIRLIFT:
		case MISSION_NUKE:
		case MISSION_RECON:
		case MISSION_PARADROP:
		case MISSION_AIRBOMB:
		case MISSION_BOMBARD:
		case MISSION_RANGE_ATTACK:
		case MISSION_PILLAGE:
		case MISSION_SABOTAGE:
		case MISSION_DESTROY:
		case MISSION_STEAL_PLANS:
		case MISSION_FOUND:
		case MISSION_SPREAD:
		case MISSION_SPREAD_CORPORATION:
		case MISSION_JOIN:
		case MISSION_CONSTRUCT:
		case MISSION_DISCOVER:
		case MISSION_HURRY:
		case MISSION_TRADE:
		case MISSION_GREAT_WORK:
		case MISSION_INFILTRATE:
		case MISSION_GOLDEN_AGE:
		case MISSION_BUILD:
		case MISSION_LEAD:
		case MISSION_ESPIONAGE:
		case MISSION_DIE_ANIMATION:
			break;

		default:
			FAssert(false);
			break;
		}

		if ( bNotify )
		{
			NotifyEntity( headMissionQueueNode()->m_data.eMissionType );
		}

		// K-Mod. Let fast units carry out the pillage action first. (based on the idea from BBAI, which had a buggy implementation)
		if (headMissionQueueNode()->m_data.eMissionType == MISSION_PILLAGE)
		{
			// Fast units pillage first
			std::vector<std::pair<int, int> > unit_list;
			pUnitNode = headUnitNode();

			while (pUnitNode != NULL)
			{
				pLoopUnit = ::getUnit(pUnitNode->m_data);
				pUnitNode = nextUnitNode(pUnitNode);

				if (pLoopUnit->canMove() && pLoopUnit->canPillage(plot()))
				{
					int iPriority = 0;
					if (pLoopUnit->bombardRate() > 0)
						iPriority--;
					if (pLoopUnit->isMadeAttack())
						iPriority++;

					iPriority = (3 + iPriority)*pLoopUnit->movesLeft() / 3;
					iPriority *= pLoopUnit->currHitPoints();
					iPriority /= std::max(1, pLoopUnit->maxHitPoints());
					unit_list.push_back(std::make_pair(iPriority, pLoopUnit->getID()));
				}
			}
			std::sort(unit_list.begin(), unit_list.end(), std::greater<std::pair<int, int> >());

			CvPlayer& kOwner = GET_PLAYER(getOwnerINLINE());
			for (size_t i = 0; i < unit_list.size(); i++)
			{
				pLoopUnit = kOwner.getUnit(unit_list[i].second);

				if (pLoopUnit->pillage())
				{
					bAction = true;
					if (isHuman() || canAllMove())
						break;
				}
				if (pLoopUnit->isAttacking())
					break; // Sea patrol intercept
			}
		}
		// K-Mod end. (note: I'm not sure what the original bts code was. I deleted the BBAI code.)
		else
		{
			std::vector<CvUnit*> units_left_behind; // K-Mod
			pUnitNode = headUnitNode();

			while (pUnitNode != NULL)
			{
				pLoopUnit = ::getUnit(pUnitNode->m_data);
				pUnitNode = nextUnitNode(pUnitNode);

				if (pLoopUnit->canMove())
				{
					switch (headMissionQueueNode()->m_data.eMissionType)
					{
					case MISSION_MOVE_TO:
					case MISSION_ROUTE_TO:
					case MISSION_MOVE_TO_UNIT:
					case MISSION_SKIP:
					case MISSION_SLEEP:
					case MISSION_FORTIFY:
					case MISSION_AIRPATROL:
					case MISSION_SEAPATROL:
					case MISSION_HEAL:
					case MISSION_SENTRY:
						pUnitNode = 0; // K-Mod. Nothing to do, so we might as well abort the unit loop.
						break;

					case MISSION_AIRLIFT:
						if (pLoopUnit->airlift(headMissionQueueNode()->m_data.iData1, headMissionQueueNode()->m_data.iData2))
						{
							bAction = true;
						}
						break;

					case MISSION_NUKE:
						if (pLoopUnit->nuke(headMissionQueueNode()->m_data.iData1, headMissionQueueNode()->m_data.iData2))
						{
							bAction = true;

							if (GC.getMapINLINE().plotINLINE(headMissionQueueNode()->m_data.iData1, headMissionQueueNode()->m_data.iData2)->isVisibleToWatchingHuman())
							{
								bNuke = true;
							}
						}
						break;

					case MISSION_RECON:
						if (pLoopUnit->recon(headMissionQueueNode()->m_data.iData1, headMissionQueueNode()->m_data.iData2))
						{
							bAction = true;
						}
						break;

					case MISSION_PARADROP:
						if (pLoopUnit->paradrop(headMissionQueueNode()->m_data.iData1, headMissionQueueNode()->m_data.iData2))
						{
							bAction = true;
						}
						// K-Mod
						else
							units_left_behind.push_back(pLoopUnit);
						// K-Mod end
						break;

					case MISSION_AIRBOMB:
						if (pLoopUnit->airBomb(headMissionQueueNode()->m_data.iData1, headMissionQueueNode()->m_data.iData2))
						{
							bAction = true;
						}
						break;

					case MISSION_BOMBARD:
						if (pLoopUnit->bombard())
						{
							bAction = true;
						}
						break;

					case MISSION_RANGE_ATTACK:
						if (pLoopUnit->rangeStrike(headMissionQueueNode()->m_data.iData1, headMissionQueueNode()->m_data.iData2))
						{
							bAction = true;
						}
						break;

					case MISSION_PILLAGE:
						if (pLoopUnit->pillage())
						{
							bAction = true;
						}
						break;

					case MISSION_PLUNDER:
						if (pLoopUnit->plunder())
						{
							bAction = true;
						}
						break;

					case MISSION_SABOTAGE:
						if (pLoopUnit->sabotage())
						{
							bAction = true;
						}
						break;

					case MISSION_DESTROY:
						if (pLoopUnit->destroy())
						{
							bAction = true;
						}
						break;

					case MISSION_STEAL_PLANS:
						if (pLoopUnit->stealPlans())
						{
							bAction = true;
						}
						break;

					case MISSION_FOUND:
						if (pLoopUnit->found())
						{
							bAction = true;
						}
						break;

					case MISSION_SPREAD:
						if (pLoopUnit->spread((ReligionTypes)(headMissionQueueNode()->m_data.iData1)))
						{
							bAction = true;
						}
						break;

					case MISSION_SPREAD_CORPORATION:
						if (pLoopUnit->spreadCorporation((CorporationTypes)(headMissionQueueNode()->m_data.iData1)))
						{
							bAction = true;
						}
						break;

					case MISSION_JOIN:
						if (pLoopUnit->join((SpecialistTypes)(headMissionQueueNode()->m_data.iData1)))
						{
							bAction = true;
						}
						break;

					case MISSION_CONSTRUCT:
						if (pLoopUnit->construct((BuildingTypes)(headMissionQueueNode()->m_data.iData1)))
						{
							bAction = true;
						}
						break;

					case MISSION_DISCOVER:
						if (pLoopUnit->discover())
						{
							bAction = true;
						}
						break;

					case MISSION_HURRY:
						if (pLoopUnit->hurry())
						{
							bAction = true;
						}
						break;

					case MISSION_TRADE:
						if (pLoopUnit->trade())
						{
							bAction = true;
						}
						break;

					case MISSION_GREAT_WORK:
						if (pLoopUnit->greatWork())
						{
							bAction = true;
						}
						break;

					case MISSION_INFILTRATE:
						if (pLoopUnit->infiltrate())
						{
							bAction = true;
						}
						break;

					case MISSION_GOLDEN_AGE:
						//just play animation, not golden age - JW
						if (headMissionQueueNode()->m_data.iData1 != -1)
						{
							CvMissionDefinition kMission;
							kMission.setMissionTime(GC.getMissionInfo(MISSION_GOLDEN_AGE).getTime() * gDLL->getSecsPerTurn());
							kMission.setUnit(BATTLE_UNIT_ATTACKER, pLoopUnit);
							kMission.setUnit(BATTLE_UNIT_DEFENDER, NULL);
							kMission.setPlot(pLoopUnit->plot());
							kMission.setMissionType(MISSION_GOLDEN_AGE);
							gDLL->getEntityIFace()->AddMission(&kMission);
							pLoopUnit->NotifyEntity(MISSION_GOLDEN_AGE);
							bAction = true;
						}
						else
						{
							if (pLoopUnit->goldenAge())
							{
								bAction = true;
							}
						}
						break;

					case MISSION_BUILD:
						break;

					case MISSION_LEAD:
						if (pLoopUnit->lead(headMissionQueueNode()->m_data.iData1))
						{
							bAction = true;
						}
						break;

					case MISSION_ESPIONAGE:
						if (pLoopUnit->espionage((EspionageMissionTypes)headMissionQueueNode()->m_data.iData1, headMissionQueueNode()->m_data.iData2))
						{
							bAction = true;
						}
						pUnitNode = NULL; // allow one unit at a time to do espionage
						break;

					case MISSION_DIE_ANIMATION:
						bAction = true;
						break;

					default:
						FAssert(false);
						break;
					}

					if (getNumUnits() == 0)
					{
						break;
					}

					if (headMissionQueueNode() == NULL)
					{
						break;
					}
				}
			}
			// K-Mod
			if (!units_left_behind.empty())
			{
				units_left_behind[0]->joinGroup(NULL, true);
				CvSelectionGroup* pNewGroup = units_left_behind[0]->getGroup();
				for (size_t i = 1; i < units_left_behind.size(); i++)
				{
					units_left_behind[i]->joinGroup(pNewGroup, true);
				}
			}
			// K-Mod end
		} // end if (mission != pillage)
	} // end if (can start mission)

	if ((getNumUnits() > 0) && (headMissionQueueNode() != NULL))
	{
		if (bAction)
		{
			if (isHuman())
			{
				if (plot()->isVisibleToWatchingHuman())
				{
					updateMissionTimer();
				}
			}
		}

		if (bNuke)
		{
			setMissionTimer(GC.getMissionInfo(MISSION_NUKE).getTime());
		}

		if (!isBusy())
		{
			if (bDelete)
			{
				if (getOwnerINLINE() == GC.getGameINLINE().getActivePlayer())
				{
					if (IsSelected())
					{
						GC.getGameINLINE().cycleSelectionGroups_delayed(GET_PLAYER(getOwnerINLINE()).isOption(PLAYEROPTION_QUICK_MOVES) ? 1 : 2, true);
					}
				}

				deleteMissionQueueNode(headMissionQueueNode());
			}
			else if (getActivityType() == ACTIVITY_MISSION)
			{
				continueMission();
			}
		}
	}
}

void CvSelectionGroup::continueMission(int iSteps)
{
	CvUnit* pTargetUnit;
	bool bDone;
	bool bAction;

	FAssert(!isBusy());
	FAssert(headMissionQueueNode() != NULL);
	FAssert(getOwnerINLINE() != NO_PLAYER);
	FAssert(getActivityType() == ACTIVITY_MISSION);

	if (headMissionQueueNode() == NULL)
	{
		// just in case...
		setActivityType(ACTIVITY_AWAKE);
		return;
	}

	bDone = false;
	bAction = false;

	if (!(headMissionQueueNode()->m_data.iFlags & MOVE_NO_ATTACK) && // K-Mod
		(headMissionQueueNode()->m_data.iPushTurn == GC.getGameINLINE().getGameTurn() || headMissionQueueNode()->m_data.iFlags & MOVE_THROUGH_ENEMY))
	{
		if (headMissionQueueNode()->m_data.eMissionType == MISSION_MOVE_TO)
		{
			bool bFailedAlreadyFighting;
			if (groupAttack(headMissionQueueNode()->m_data.iData1, headMissionQueueNode()->m_data.iData2, headMissionQueueNode()->m_data.iFlags, bFailedAlreadyFighting))
			{
				bDone = true;
			}
		}
		// K-Mod. We need to do a similar check for MISSION_MOVE_TO_UNIT,
		// because with the MOVE_ATTACK_STACK flag, MOVE_TO_UNIT might actually want to attack something!
		// Note: the "else" is not just for efficiency. The code above may have actually killed "this" unit!
		else if (headMissionQueueNode()->m_data.eMissionType == MISSION_MOVE_TO_UNIT && headMissionQueueNode()->m_data.iFlags & MOVE_ATTACK_STACK)
		{
			bool bDummy;
			pTargetUnit = GET_PLAYER((PlayerTypes)headMissionQueueNode()->m_data.iData1).getUnit(headMissionQueueNode()->m_data.iData2);
			if (pTargetUnit && groupAttack(pTargetUnit->getX_INLINE(), pTargetUnit->getY_INLINE(), headMissionQueueNode()->m_data.iFlags, bDummy))
				bDone = true;
		}
		// K-Mod end
	}

	// extra crash protection, should never happen (but a previous bug in groupAttack was causing a NULL here)
	// while that bug is fixed, no reason to not be a little more careful
	if (headMissionQueueNode() == NULL)
	{
		setActivityType(ACTIVITY_AWAKE);
		return;
	}

	FAssert(bDone || !(headMissionQueueNode()->m_data.iFlags & MOVE_DIRECT_ATTACK)); // K-Mod. ('direct attack' should be used for attack commands only)

	if (!bDone)
	{
		if (getNumUnits() > 0)
		{
			if (canAllMove())
			{
				switch (headMissionQueueNode()->m_data.eMissionType)
				{
				case MISSION_MOVE_TO:
					if (getDomainType() == DOMAIN_AIR)
					{
						groupPathTo(headMissionQueueNode()->m_data.iData1, headMissionQueueNode()->m_data.iData2, headMissionQueueNode()->m_data.iFlags);
						bDone = true;
					}
					else if (groupPathTo(headMissionQueueNode()->m_data.iData1, headMissionQueueNode()->m_data.iData2, headMissionQueueNode()->m_data.iFlags))
					{
						bAction = true;

						if (getNumUnits() > 0)
						{
							if (!canAllMove())
							{
								if (headMissionQueueNode() != NULL)
								{
									if (groupAmphibMove(GC.getMapINLINE().plotINLINE(headMissionQueueNode()->m_data.iData1, headMissionQueueNode()->m_data.iData2), headMissionQueueNode()->m_data.iFlags))
									{
										bAction = false;
										bDone = true;
									}
								}
							}
						}
					}
					else
					{
						bDone = true;
					}
					break;

				case MISSION_ROUTE_TO:
					if (groupRoadTo(headMissionQueueNode()->m_data.iData1, headMissionQueueNode()->m_data.iData2, headMissionQueueNode()->m_data.iFlags))
					{
						bAction = true;
					}
					else
					{
						bDone = true;
					}
					break;

				case MISSION_MOVE_TO_UNIT:
					if ((getHeadUnitAI() == UNITAI_CITY_DEFENSE) && plot()->isCity() && (plot()->getTeam() == getTeam()))
					{
						if (plot()->getBestDefender(getOwnerINLINE())->getGroup() == this)
						{
							bAction = false;
							bDone = true;
							break;
						}
					}
					pTargetUnit = GET_PLAYER((PlayerTypes)headMissionQueueNode()->m_data.iData1).getUnit(headMissionQueueNode()->m_data.iData2);
					if (pTargetUnit != NULL)
					{
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      12/07/08                                jdog5000      */
/*                                                                                              */
/* General AI                                                                                   */
/************************************************************************************************/
						// Handling for mission to retrieve a unit
						if( AI_getMissionAIType() == MISSIONAI_PICKUP )
						{
							if( !(pTargetUnit->getGroup()->isStranded()) || isFull() || (pTargetUnit->plot() == NULL) )
							{
								bDone = true;
								bAction = false;
								break;
							}

							CvPlot* pPickupPlot = NULL;
							CvPlot* pAdjacentPlot = NULL;
							int iPathTurns;
							int iBestPathTurns = MAX_INT;

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      08/08/09                        Maniac & jdog5000     */
/*                                                                                              */
/* General AI                                                                                   */
/************************************************************************************************/
/* original bts code
							if( (pTargetUnit->plot()->isWater() || pTargetUnit->plot()->isFriendlyCity(*getHeadUnit(), true)) && generatePath(plot(), pTargetUnit->plot(), 0, false, &iPathTurns) )
*/
							if( (canMoveAllTerrain() || pTargetUnit->plot()->isWater() || pTargetUnit->plot()->isFriendlyCity(*getHeadUnit(), true)) && generatePath(plot(), pTargetUnit->plot(), 0, true, &iPathTurns) )
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
							{
								pPickupPlot = pTargetUnit->plot();
							}
							else
							{
								for (int iI = 0; iI < NUM_DIRECTION_TYPES; iI++)
								{
									pAdjacentPlot = plotDirection(pTargetUnit->plot()->getX_INLINE(), pTargetUnit->plot()->getY_INLINE(), ((DirectionTypes)iI));

									if (pAdjacentPlot != NULL)
									{
										if( atPlot(pAdjacentPlot) )
										{
											pPickupPlot = pAdjacentPlot;
											break;
										}

										if( pAdjacentPlot->isWater() || pAdjacentPlot->isFriendlyCity(*getHeadUnit(), true) )
										{
											if( generatePath(plot(), pAdjacentPlot, 0, true, &iPathTurns, iBestPathTurns) )
											{
												if( iPathTurns < iBestPathTurns )
												{
													pPickupPlot = pAdjacentPlot;
													iBestPathTurns = iPathTurns;
												}
											}
										}
									}
								}
							}

							if( pPickupPlot != NULL )
							{
								if( atPlot(pPickupPlot) )
								{
									CLLNode<IDInfo>* pEntityNode;
									CvUnit* pLoopUnit;

									pEntityNode = headUnitNode();

									while (pEntityNode != NULL)
									{
										pLoopUnit = ::getUnit(pEntityNode->m_data);
										pEntityNode = nextUnitNode(pEntityNode);

										if( !(pLoopUnit->isFull()) )
										{
											pTargetUnit->getGroup()->setRemoteTransportUnit(pLoopUnit);
										}
									}

									bAction = true;
									bDone = true;
								}
								else
								{
									if (groupPathTo(pPickupPlot->getX_INLINE(), pPickupPlot->getY_INLINE(), headMissionQueueNode()->m_data.iFlags))
									{
										bAction = true;
									}
									else
									{
										bDone = true;
									}
								}
							}
							else
							{
								bDone = true;
							}
							break;
						}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

						if (AI_getMissionAIType() != MISSIONAI_SHADOW && AI_getMissionAIType() != MISSIONAI_GROUP)
						{
							if (!plot()->isOwned() || plot()->getOwnerINLINE() == getOwnerINLINE())
							{
								CvPlot* pMissionPlot = pTargetUnit->getGroup()->AI_getMissionAIPlot();
								if (pMissionPlot != NULL && NO_TEAM != pMissionPlot->getTeam())
								{
									if (pMissionPlot->isOwned() && pTargetUnit->isPotentialEnemy(pMissionPlot->getTeam(), pMissionPlot))
									{
										bAction = false;
										bDone = true;
										break;								
									}
								}
							}
						}
							 
						if (groupPathTo(pTargetUnit->getX_INLINE(), pTargetUnit->getY_INLINE(), headMissionQueueNode()->m_data.iFlags))
						{
							bAction = true;
						}
						else
						{
							bDone = true;
						}
					}
					else
					{
						bDone = true;
					}
					break;

				case MISSION_SKIP:
				case MISSION_SLEEP:
				case MISSION_FORTIFY:
				case MISSION_PLUNDER:
				case MISSION_AIRPATROL:
				case MISSION_SEAPATROL:
				case MISSION_HEAL:
				case MISSION_SENTRY:
					FAssert(false);
					break;

				case MISSION_AIRLIFT:
				case MISSION_NUKE:
				case MISSION_RECON:
				case MISSION_PARADROP:
				case MISSION_AIRBOMB:
				case MISSION_BOMBARD:
				case MISSION_RANGE_ATTACK:
				case MISSION_PILLAGE:
				case MISSION_SABOTAGE:
				case MISSION_DESTROY:
				case MISSION_STEAL_PLANS:
				case MISSION_FOUND:
				case MISSION_SPREAD:
				case MISSION_SPREAD_CORPORATION:
				case MISSION_JOIN:
				case MISSION_CONSTRUCT:
				case MISSION_DISCOVER:
				case MISSION_HURRY:
				case MISSION_TRADE:
				case MISSION_GREAT_WORK:
				case MISSION_INFILTRATE:
				case MISSION_GOLDEN_AGE:
				case MISSION_LEAD:
				case MISSION_ESPIONAGE:
				case MISSION_DIE_ANIMATION:
					break;

				case MISSION_BUILD:
					if (!groupBuild((BuildTypes)(headMissionQueueNode()->m_data.iData1)))
					{
						bDone = true;
					}
					break;

				default:
					FAssert(false);
					break;
				}
			}
		}
	}

	if ((getNumUnits() > 0) && (headMissionQueueNode() != NULL))
	{
		if (!bDone)
		{
			switch (headMissionQueueNode()->m_data.eMissionType)
			{
			case MISSION_MOVE_TO:
				if (at(headMissionQueueNode()->m_data.iData1, headMissionQueueNode()->m_data.iData2))
				{
					bDone = true;
				}
				break;

			case MISSION_ROUTE_TO:
				if (at(headMissionQueueNode()->m_data.iData1, headMissionQueueNode()->m_data.iData2))
				{
					if (getBestBuildRoute(plot()) == NO_ROUTE)
					{
						bDone = true;
					}
				}
				break;

			case MISSION_MOVE_TO_UNIT:
				pTargetUnit = GET_PLAYER((PlayerTypes)headMissionQueueNode()->m_data.iData1).getUnit(headMissionQueueNode()->m_data.iData2);
				if ((pTargetUnit == NULL) || atPlot(pTargetUnit->plot()))
				{
					bDone = true;
				}
				break;

			case MISSION_SKIP:
			case MISSION_SLEEP:
			case MISSION_FORTIFY:
			case MISSION_PLUNDER:
			case MISSION_AIRPATROL:
			case MISSION_SEAPATROL:
			case MISSION_HEAL:
			case MISSION_SENTRY:
				FAssert(false);
				break;

			case MISSION_AIRLIFT:
			case MISSION_NUKE:
			case MISSION_RECON:
			case MISSION_PARADROP:
			case MISSION_AIRBOMB:
			case MISSION_BOMBARD:
			case MISSION_RANGE_ATTACK:
			case MISSION_PILLAGE:
			case MISSION_SABOTAGE:
			case MISSION_DESTROY:
			case MISSION_STEAL_PLANS:
			case MISSION_FOUND:
			case MISSION_SPREAD:
			case MISSION_SPREAD_CORPORATION:
			case MISSION_JOIN:
			case MISSION_CONSTRUCT:
			case MISSION_DISCOVER:
			case MISSION_HURRY:
			case MISSION_TRADE:
			case MISSION_GREAT_WORK:
			case MISSION_INFILTRATE:
			case MISSION_GOLDEN_AGE:
			case MISSION_LEAD:
			case MISSION_ESPIONAGE:
			case MISSION_DIE_ANIMATION:
				bDone = true;
				break;

			case MISSION_BUILD:
				// XXX what happens if two separate worker groups are both building the mine...
				/*if (plot()->getBuildType() != ((BuildTypes)(headMissionQueueNode()->m_data.iData1)))
				{
					bDone = true;
				}*/
				break;

			default:
				FAssert(false);
				break;
			}
		}
	}

	if ((getNumUnits() > 0) && (headMissionQueueNode() != NULL))
	{
		if (bAction)
		{
			if (bDone || !canAllMove())
			{
				if (plot()->isVisibleToWatchingHuman())
				{
					updateMissionTimer(iSteps);

					if (showMoves())
					{
						if (GC.getGameINLINE().getActivePlayer() != NO_PLAYER)
						{
							if (getOwnerINLINE() != GC.getGameINLINE().getActivePlayer())
							{
								if (plot()->isActiveVisible(false) && !isInvisible(GC.getGameINLINE().getActiveTeam()))
								{
									gDLL->getInterfaceIFace()->lookAt(plot()->getPoint(), CAMERALOOKAT_NORMAL);
								}
							}
						}
					}
				}
			}
		}

		if (bDone)
		{
			if (!isBusy())
			{
				if (getOwnerINLINE() == GC.getGameINLINE().getActivePlayer())
				{
					if (IsSelected())
					{
						if ((headMissionQueueNode()->m_data.eMissionType == MISSION_MOVE_TO) ||
							(headMissionQueueNode()->m_data.eMissionType == MISSION_ROUTE_TO) ||
							(headMissionQueueNode()->m_data.eMissionType == MISSION_MOVE_TO_UNIT))
						{
							GC.getGameINLINE().cycleSelectionGroups_delayed(GET_PLAYER(getOwnerINLINE()).isOption(PLAYEROPTION_QUICK_MOVES) ? 1 : 2, true, true); // K-Mod
						}
					}
				}

				deleteMissionQueueNode(headMissionQueueNode());
				// K-Mod Note: the "unofficial patch" had a condition for deleting the mission node equivalent to the following:
				//if (!isHuman() || headMissionQueueNode()->m_data.eMissionType != MISSION_MOVE_TO || canAllMove() || nextMissionQueueNode(headMissionQueueNode()) == NULL)
				// I didn't understand what good it did, and it was causing problems with Rapid Unit Cycling. So I removed it.
			}
		}
		else
		{
			if (canAllMove())
			{
				continueMission(iSteps + 1);
			}
			else if (!isBusy())
			{
				if (getOwnerINLINE() == GC.getGameINLINE().getActivePlayer())
				{
					if (IsSelected())
					{
						GC.getGameINLINE().cycleSelectionGroups_delayed(1, true);
					}
				}
			}
		}
	}
}


bool CvSelectionGroup::canDoCommand(CommandTypes eCommand, int iData1, int iData2, bool bTestVisible, bool bUseCache)
{
	PROFILE_FUNC();

	CLLNode<IDInfo>* pUnitNode;
	CvUnit* pLoopUnit;

	//cache isBusy
	if(bUseCache)
	{
		if(m_bIsBusyCache)
		{
			return false;
		}
	}
	else
	{
		if (isBusy())
		{
			return false;
		}
	}

	if(!canEverDoCommand(eCommand, iData1, iData2, bTestVisible, bUseCache))
		return false;

	pUnitNode = headUnitNode();

	while (pUnitNode != NULL)
	{
		pLoopUnit = ::getUnit(pUnitNode->m_data);
		pUnitNode = nextUnitNode(pUnitNode);

		if (pLoopUnit->canDoCommand(eCommand, iData1, iData2, bTestVisible, false))
		{
			return true;
		}
	}

	return false;
}

bool CvSelectionGroup::canEverDoCommand(CommandTypes eCommand, int iData1, int iData2, bool bTestVisible, bool bUseCache)
{
	if(eCommand == COMMAND_LOAD)
	{
		CLLNode<IDInfo>* pUnitNode = plot()->headUnitNode();

		while (pUnitNode != NULL)
		{
			CvUnit *pLoopUnit = ::getUnit(pUnitNode->m_data);
			pUnitNode = plot()->nextUnitNode(pUnitNode);

			if (!pLoopUnit->isFull())
			{
				return true;
			}
		}

		//no cargo space on this plot
		return false;
	}
	else if(eCommand == COMMAND_UNLOAD)
	{
		CLLNode<IDInfo>* pUnitNode = headUnitNode();

		while (pUnitNode != NULL)
		{
			CvUnit *pLoopUnit = ::getUnit(pUnitNode->m_data);
			pUnitNode = nextUnitNode(pUnitNode);

			if (pLoopUnit->isCargo())
			{
				return true;
			}
		}

		//no loaded unit
		return false;
	}
	else if(eCommand == COMMAND_UPGRADE)
	{
		if(bUseCache)
		{
			//see if any of the different units can upgrade to this unit type
			for(int i=0;i<(int)m_aDifferentUnitCache.size();i++)
			{
				CvUnit *unit = m_aDifferentUnitCache[i];
				if(unit->canDoCommand(eCommand, iData1, iData2, bTestVisible, false))
					return true;
			}

			return false;
		}
	}

	return true;
}

void CvSelectionGroup::setupActionCache()
{
	//cache busy calculation
	m_bIsBusyCache = isBusy();

    //cache different unit types
	m_aDifferentUnitCache.erase(m_aDifferentUnitCache.begin(), m_aDifferentUnitCache.end());
	CLLNode<IDInfo> *pUnitNode = headUnitNode();
	while(pUnitNode != NULL)
	{
		CvUnit *unit = ::getUnit(pUnitNode->m_data);
		pUnitNode = nextUnitNode(pUnitNode);

		if(unit->isReadyForUpgrade())
		{
			UnitTypes unitType = unit->getUnitType();
			bool bFound = false;
			for(int i=0;i<(int)m_aDifferentUnitCache.size();i++)
			{
				if(unitType == m_aDifferentUnitCache[i]->getUnitType())
				{
					bFound = true;
					break;
				}
			}

			if(!bFound)
				m_aDifferentUnitCache.push_back(unit);
		}
	}
}

// Returns true if one of the units can support the interface mode...
bool CvSelectionGroup::canDoInterfaceMode(InterfaceModeTypes eInterfaceMode)
{
	PROFILE_FUNC();

	CLLNode<IDInfo>* pUnitNode;
	CvUnit* pLoopUnit;

	FAssertMsg(eInterfaceMode != NO_INTERFACEMODE, "InterfaceMode is not assigned a valid value");

	if (isBusy())
	{
		return false;
	}

	pUnitNode = headUnitNode();

	while (pUnitNode != NULL)
	{
		pLoopUnit = ::getUnit(pUnitNode->m_data);

		switch (eInterfaceMode)
		{
		case INTERFACEMODE_GO_TO:
			if ((getDomainType() != DOMAIN_AIR) && (getDomainType() != DOMAIN_IMMOBILE))
			{
				return true;
			}
			break;

		case INTERFACEMODE_GO_TO_TYPE:
			if ((getDomainType() != DOMAIN_AIR) && (getDomainType() != DOMAIN_IMMOBILE))
			{
				if (pLoopUnit->plot()->plotCount(PUF_isUnitType, pLoopUnit->getUnitType(), -1, pLoopUnit->getOwnerINLINE()) > 1)
				{
					return true;
				}
			}
			break;

		case INTERFACEMODE_GO_TO_ALL:
			if ((getDomainType() != DOMAIN_AIR) && (getDomainType() != DOMAIN_IMMOBILE))
			{
				if (pLoopUnit->plot()->plotCount(NULL, -1, -1, pLoopUnit->getOwnerINLINE()) > 1)
				{
					return true;
				}
			}
			break;

		case INTERFACEMODE_ROUTE_TO:
			if (pLoopUnit->AI_getUnitAIType() == UNITAI_WORKER)
			{
				if (pLoopUnit->canBuildRoute())
				{
					return true;
				}
			}
			break;

		case INTERFACEMODE_AIRLIFT:
			if (pLoopUnit->canAirlift(pLoopUnit->plot()))
			{
				return true;
			}
			break;

		case INTERFACEMODE_NUKE:
			if (pLoopUnit->canNuke(pLoopUnit->plot()))
			{
				return true;
			}
			break;

		case INTERFACEMODE_RECON:
			if (pLoopUnit->canRecon(pLoopUnit->plot()))
			{
				return true;
			}
			break;

		case INTERFACEMODE_PARADROP:
			if (pLoopUnit->canParadrop(pLoopUnit->plot()))
			{
				return true;
			}
			break;

		case INTERFACEMODE_AIRBOMB:
			if (pLoopUnit->canAirBomb(pLoopUnit->plot()))
			{
				return true;
			}
			break;

		case INTERFACEMODE_RANGE_ATTACK:
			if (pLoopUnit->canRangeStrike())
			{
				return true;
			}
			break;

		case INTERFACEMODE_AIRSTRIKE:
			if (pLoopUnit->getDomainType() == DOMAIN_AIR)
			{
				if (pLoopUnit->canAirAttack())
				{
					return true;
				}
			}
			break;

		case INTERFACEMODE_REBASE:
			if (pLoopUnit->getDomainType() == DOMAIN_AIR)
			{
				return true;
			}
			break;
		}

		pUnitNode = nextUnitNode(pUnitNode);
	}

	return false;
}


// Returns true if one of the units can execute the interface mode at the specified plot...
bool CvSelectionGroup::canDoInterfaceModeAt(InterfaceModeTypes eInterfaceMode, CvPlot* pPlot)
{
	CLLNode<IDInfo>* pUnitNode;
	CvUnit* pLoopUnit;

	FAssertMsg(eInterfaceMode != NO_INTERFACEMODE, "InterfaceMode is not assigned a valid value");

	pUnitNode = headUnitNode();

	while (pUnitNode != NULL)
	{
		pLoopUnit = ::getUnit(pUnitNode->m_data);

		switch (eInterfaceMode)
		{
		case INTERFACEMODE_AIRLIFT:
			if (pLoopUnit != NULL)
			{
				if (pLoopUnit->canAirliftAt(pLoopUnit->plot(), pPlot->getX_INLINE(), pPlot->getY_INLINE()))
				{
					return true;
				}
			}
			break;

		case INTERFACEMODE_NUKE:
			if (pLoopUnit != NULL)
			{
				if (pLoopUnit->canNukeAt(pLoopUnit->plot(), pPlot->getX_INLINE(), pPlot->getY_INLINE()))
				{
					return true;
				}
			}
			break;

		case INTERFACEMODE_RECON:
			if (pLoopUnit != NULL)
			{
				if (pLoopUnit->canReconAt(pLoopUnit->plot(), pPlot->getX_INLINE(), pPlot->getY_INLINE()))
				{
					return true;
				}
			}
			break;

		case INTERFACEMODE_PARADROP:
			if (pLoopUnit != NULL)
			{
				if (pLoopUnit->canParadropAt(pLoopUnit->plot(), pPlot->getX_INLINE(), pPlot->getY_INLINE()))
				{
					return true;
				}
			}
			break;

		case INTERFACEMODE_AIRBOMB:
			if (pLoopUnit != NULL)
			{
				if (pLoopUnit->canAirBombAt(pLoopUnit->plot(), pPlot->getX_INLINE(), pPlot->getY_INLINE()))
				{
					return true;
				}
			}
			break;

		case INTERFACEMODE_RANGE_ATTACK:
			if (pLoopUnit != NULL)
			{
				if (pLoopUnit->canRangeStrikeAt(pLoopUnit->plot(), pPlot->getX_INLINE(), pPlot->getY_INLINE()))
				{
					return true;
				}
			}
			break;

		case INTERFACEMODE_AIRSTRIKE:
			if (pLoopUnit != NULL)
			{
				if (pLoopUnit->canMoveInto(pPlot, true))
				{
					return true;
				}
			}
			break;

		case INTERFACEMODE_REBASE:
			if (pLoopUnit != NULL)
			{
				if (pLoopUnit->canMoveInto(pPlot))
				{
					return true;
				}
			}
			break;

		default:
			return true;
			break;
		}

		pUnitNode = nextUnitNode(pUnitNode);
	}

	return false;
}


bool CvSelectionGroup::isHuman()
{
	if (getOwnerINLINE() != NO_PLAYER)
	{
		return GET_PLAYER(getOwnerINLINE()).isHuman();
	}

	return true;
}


bool CvSelectionGroup::isBusy()
{
	CLLNode<IDInfo>* pUnitNode;
	CvUnit* pLoopUnit;
	CvPlot* pPlot;

	if (getNumUnits() == 0)
	{
		return false;
	}

	if (getMissionTimer() > 0)
	{
		return true;
	}

	pPlot = plot();

	pUnitNode = headUnitNode();

	while (pUnitNode != NULL)
	{
		pLoopUnit = ::getUnit(pUnitNode->m_data);
		pUnitNode = nextUnitNode(pUnitNode);

		if (pLoopUnit != NULL)
		{
			if (pLoopUnit->isCombat())
			{
				return true;
			}
		}
	}

	return false;
}


bool CvSelectionGroup::isCargoBusy()
{
	CLLNode<IDInfo>* pUnitNode1;
	CLLNode<IDInfo>* pUnitNode2;
	CvUnit* pLoopUnit1;
	CvUnit* pLoopUnit2;
	CvPlot* pPlot;

	if (getNumUnits() == 0)
	{
		return false;
	}

	pPlot = plot();

	pUnitNode1 = headUnitNode();

	while (pUnitNode1 != NULL)
	{
		pLoopUnit1 = ::getUnit(pUnitNode1->m_data);
		pUnitNode1 = nextUnitNode(pUnitNode1);

		if (pLoopUnit1 != NULL)
		{
			if (pLoopUnit1->getCargo() > 0)
			{
				pUnitNode2 = pPlot->headUnitNode();

				while (pUnitNode2 != NULL)
				{
					pLoopUnit2 = ::getUnit(pUnitNode2->m_data);
					pUnitNode2 = pPlot->nextUnitNode(pUnitNode2);

					if (pLoopUnit2->getTransportUnit() == pLoopUnit1)
					{
						if (pLoopUnit2->getGroup()->isBusy())
						{
							return true;
						}
					}
				}
			}
		}
	}

	return false;
}


int CvSelectionGroup::baseMoves() const
{
	CLLNode<IDInfo>* pUnitNode;
	CvUnit* pLoopUnit;
	int iValue;
	int iBestValue;

	iBestValue = MAX_INT;

	pUnitNode = headUnitNode();

	while (pUnitNode != NULL)
	{
		pLoopUnit = ::getUnit(pUnitNode->m_data);
		pUnitNode = nextUnitNode(pUnitNode);

		iValue = pLoopUnit->baseMoves();

		if (iValue < iBestValue)
		{
			iBestValue = iValue;
		}
	}

	return iBestValue;
}

// K-Mod
int CvSelectionGroup::maxMoves() const
{
	int iMoves = 0;

	for (CLLNode<IDInfo>* pUnitNode = headUnitNode(); pUnitNode != NULL; pUnitNode = nextUnitNode(pUnitNode))
	{
		CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
		iMoves = std::max(iMoves, pLoopUnit->maxMoves());
	}
	return iMoves;
}

int CvSelectionGroup::movesLeft() const
{
	int iMoves = INT_MAX;

	for (CLLNode<IDInfo>* pUnitNode = headUnitNode(); pUnitNode != NULL; pUnitNode = nextUnitNode(pUnitNode))
	{
		CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
		iMoves = std::min(iMoves, pLoopUnit->movesLeft());
	}
	return iMoves;
}
// K-Mod end


bool CvSelectionGroup::isWaiting() const
{
	return ((getActivityType() == ACTIVITY_HOLD) ||
		      (getActivityType() == ACTIVITY_SLEEP) ||
					(getActivityType() == ACTIVITY_HEAL) ||
					(getActivityType() == ACTIVITY_SENTRY) ||
					(getActivityType() == ACTIVITY_PATROL) ||
					(getActivityType() == ACTIVITY_PLUNDER) ||
					(getActivityType() == ACTIVITY_INTERCEPT));
}


bool CvSelectionGroup::isFull()
{
	CLLNode<IDInfo>* pUnitNode;
	CvUnit* pLoopUnit;

	if (getNumUnits() > 0)
	{
		// do two passes, the first pass, we ignore units with speical cargo
		int iSpecialCargoCount = 0;
		int iCargoCount = 0;
		
		// first pass, count but ignore special cargo units
		pUnitNode = headUnitNode();

		while (pUnitNode != NULL)
		{
			pLoopUnit = ::getUnit(pUnitNode->m_data);
			pUnitNode = nextUnitNode(pUnitNode);
			
			if (pLoopUnit->cargoSpace() > 0)
			{
				iCargoCount++;
			}

			if (pLoopUnit->specialCargo() != NO_SPECIALUNIT)
			{
				iSpecialCargoCount++;
			}
			else if (!(pLoopUnit->isFull()))
			{
				return false;
			}
		}
		
		// if every unit in the group has special cargo, then check those, otherwise, consider ourselves full
		if (iSpecialCargoCount >= iCargoCount)
		{
			pUnitNode = headUnitNode();
			while (pUnitNode != NULL)
			{
				pLoopUnit = ::getUnit(pUnitNode->m_data);
				pUnitNode = nextUnitNode(pUnitNode);
				
				if (!(pLoopUnit->isFull()))
				{
					return false;
				}
			}
		}

		return true;
	}

	return false;
}


bool CvSelectionGroup::hasCargo()
{
	CLLNode<IDInfo>* pUnitNode = headUnitNode();

	while (pUnitNode != NULL)
	{
		CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
		pUnitNode = nextUnitNode(pUnitNode);

		if (pLoopUnit->hasCargo())
		{
			return true;
		}
	}

	return false;
}

int CvSelectionGroup::getCargo() const
{
	int iCargoCount = 0;

	CLLNode<IDInfo>* pUnitNode = headUnitNode();
	while (pUnitNode != NULL)
	{
		CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
		pUnitNode = nextUnitNode(pUnitNode);

		iCargoCount += pLoopUnit->getCargo();
	}

	return iCargoCount;
}

bool CvSelectionGroup::canAllMove()
{
	CLLNode<IDInfo>* pUnitNode;
	CvUnit* pLoopUnit;

	if (getNumUnits() > 0)
	{
		pUnitNode = headUnitNode();

		while (pUnitNode != NULL)
		{
			pLoopUnit = ::getUnit(pUnitNode->m_data);
			pUnitNode = nextUnitNode(pUnitNode);

			FAssertMsg(pLoopUnit != NULL, "existing node, but NULL unit");

			if (pLoopUnit != NULL && !(pLoopUnit->canMove()))
			{
				return false;
			}
		}

		return true;
	}

	return false;
}


bool CvSelectionGroup::canAnyMove()
{
	CLLNode<IDInfo>* pUnitNode;
	CvUnit* pLoopUnit;

	pUnitNode = headUnitNode();

	while (pUnitNode != NULL)
	{
		pLoopUnit = ::getUnit(pUnitNode->m_data);
		pUnitNode = nextUnitNode(pUnitNode);

		if (pLoopUnit->canMove())
		{
			return true;
		}
	}

	return false;
}

bool CvSelectionGroup::hasMoved()
{
	CLLNode<IDInfo>* pUnitNode;
	CvUnit* pLoopUnit;

	pUnitNode = headUnitNode();

	while (pUnitNode != NULL)
	{
		pLoopUnit = ::getUnit(pUnitNode->m_data);
		pUnitNode = nextUnitNode(pUnitNode);

		if (pLoopUnit->hasMoved())
		{
			return true;
		}
	}

	return false;
}


bool CvSelectionGroup::canEnterTerritory(TeamTypes eTeam, bool bIgnoreRightOfPassage) const
{
	CLLNode<IDInfo>* pUnitNode;
	CvUnit* pLoopUnit;

	if (getNumUnits() > 0)
	{
		pUnitNode = headUnitNode();

		while (pUnitNode != NULL)
		{
			pLoopUnit = ::getUnit(pUnitNode->m_data);
			pUnitNode = nextUnitNode(pUnitNode);

			if (!(pLoopUnit->canEnterTerritory(eTeam, bIgnoreRightOfPassage)))
			{
				return false;
			}
		}

		return true;
	}

	return false;
}

bool CvSelectionGroup::canEnterArea(TeamTypes eTeam, const CvArea* pArea, bool bIgnoreRightOfPassage) const
{
	CLLNode<IDInfo>* pUnitNode;
	CvUnit* pLoopUnit;

	if (getNumUnits() > 0)
	{
		pUnitNode = headUnitNode();

		while (pUnitNode != NULL)
		{
			pLoopUnit = ::getUnit(pUnitNode->m_data);
			pUnitNode = nextUnitNode(pUnitNode);

			if (!(pLoopUnit->canEnterArea(eTeam, pArea, bIgnoreRightOfPassage)))
			{
				return false;
			}
		}

		return true;
	}

	return false;
}


bool CvSelectionGroup::canMoveInto(CvPlot* pPlot, bool bAttack)
{
	CLLNode<IDInfo>* pUnitNode;
	CvUnit* pLoopUnit;

	if (getNumUnits() > 0)
	{
		pUnitNode = headUnitNode();

		while (pUnitNode != NULL)
		{
			pLoopUnit = ::getUnit(pUnitNode->m_data);
			pUnitNode = nextUnitNode(pUnitNode);

			if (pLoopUnit->canMoveInto(pPlot, bAttack))
			{
				return true;
			}
		}
	}

	return false;
}


bool CvSelectionGroup::canMoveOrAttackInto(CvPlot* pPlot, bool bDeclareWar)
{
	CLLNode<IDInfo>* pUnitNode;
	CvUnit* pLoopUnit;

	if (getNumUnits() > 0)
	{
		pUnitNode = headUnitNode();

		while (pUnitNode != NULL)
		{
			pLoopUnit = ::getUnit(pUnitNode->m_data);
			pUnitNode = nextUnitNode(pUnitNode);

			if (pLoopUnit->canMoveOrAttackInto(pPlot, bDeclareWar))
			{
				return true;
			}
		}
	}

	return false;
}


bool CvSelectionGroup::canMoveThrough(CvPlot* pPlot, bool bDeclareWar) const
{
	CLLNode<IDInfo>* pUnitNode;
	CvUnit* pLoopUnit;

	if (getNumUnits() > 0)
	{
		pUnitNode = headUnitNode();

		while (pUnitNode != NULL)
		{
			pLoopUnit = ::getUnit(pUnitNode->m_data);
			pUnitNode = nextUnitNode(pUnitNode);

			if (!pLoopUnit->canMoveThrough(pPlot, bDeclareWar))
			{
				return false;
			}
		}

		return true;
	}

	return false;
}


bool CvSelectionGroup::canFight()
{
	CLLNode<IDInfo>* pUnitNode;
	CvUnit* pLoopUnit;

	pUnitNode = headUnitNode();

	while (pUnitNode != NULL)
	{
		pLoopUnit = ::getUnit(pUnitNode->m_data);
		pUnitNode = nextUnitNode(pUnitNode);

		if (pLoopUnit->canFight())
		{
			return true;
		}
	}

	return false;
}


bool CvSelectionGroup::canDefend()
{
	CLLNode<IDInfo>* pUnitNode;
	CvUnit* pLoopUnit;

	pUnitNode = headUnitNode();

	while (pUnitNode != NULL)
	{
		pLoopUnit = ::getUnit(pUnitNode->m_data);
		pUnitNode = nextUnitNode(pUnitNode);

		if (pLoopUnit->canDefend())
		{
			return true;
		}
	}

	return false;
}

bool CvSelectionGroup::canBombard(const CvPlot* pPlot)
{
	CLLNode<IDInfo>* pUnitNode = headUnitNode();
	while (pUnitNode != NULL)
	{
		CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
		pUnitNode = nextUnitNode(pUnitNode);

		if (pLoopUnit->canBombard(pPlot))
		{
			return true;
		}
	}

	return false;
}

bool CvSelectionGroup::visibilityRange()
{
	int iMaxRange = 0;
	
	CLLNode<IDInfo>* pUnitNode = headUnitNode();
	while (pUnitNode != NULL)
	{
		CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
		pUnitNode = nextUnitNode(pUnitNode);

		int iRange = pLoopUnit->visibilityRange();
		if (iRange > iMaxRange)
		{
			iMaxRange = iRange;
		}
	}

	return iMaxRange;
}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      03/30/10                                jdog5000      */
/*                                                                                              */
/* General AI                                                                                   */
/************************************************************************************************/
//
// Approximate how many turns this group would take to reduce pCity's defense modifier to zero
//
int CvSelectionGroup::getBombardTurns(CvCity* pCity)
{
	PROFILE_FUNC();

	bool bHasBomber = (getOwnerINLINE() != NO_PLAYER ? (GET_PLAYER(getOwnerINLINE()).AI_calculateTotalBombard(DOMAIN_AIR) > 0) : false);
	bool bIgnoreBuildingDefense = bHasBomber;
	int iTotalBombardRate = (bHasBomber ? 16 : 0);
	int iUnitBombardRate = 0;

	CLLNode<IDInfo>* pUnitNode = headUnitNode();
	while (pUnitNode != NULL)
	{
		CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
		pUnitNode = nextUnitNode(pUnitNode);

		if( pLoopUnit->bombardRate() > 0 )
		{
			iUnitBombardRate = pLoopUnit->bombardRate();

			if( pLoopUnit->ignoreBuildingDefense() )
			{
				bIgnoreBuildingDefense = true;
			}
			else
			{
				iUnitBombardRate *= std::max(25, (100 - pCity->getBuildingBombardDefense()));
				iUnitBombardRate /= 100;
			}

			iTotalBombardRate += iUnitBombardRate;
		}
	}


	if( pCity->getTotalDefense(bIgnoreBuildingDefense) == 0 )
	{
		return 0;
	}

	int iBombardTurns = pCity->getTotalDefense(bIgnoreBuildingDefense);

	if( iTotalBombardRate > 0 )
	{
		iBombardTurns = (GC.getMAX_CITY_DEFENSE_DAMAGE() - pCity->getDefenseDamage());
		iBombardTurns *= pCity->getTotalDefense(false);
		iBombardTurns += (GC.getMAX_CITY_DEFENSE_DAMAGE() * iTotalBombardRate) - 1;
		iBombardTurns /= std::max(1, (GC.getMAX_CITY_DEFENSE_DAMAGE() * iTotalBombardRate));
	}

	//if( gUnitLogLevel > 2 ) logBBAI("      Bombard of %S will take %d turns at rate %d and current damage %d with bombard def %d", pCity->getName().GetCString(), iBombardTurns, iTotalBombardRate, pCity->getDefenseDamage(), (bIgnoreBuildingDefense ? 0 : pCity->getBuildingBombardDefense()));

	return iBombardTurns;
}

bool CvSelectionGroup::isHasPathToAreaPlayerCity( PlayerTypes ePlayer, int iFlags, int iMaxPathTurns )
{
	PROFILE_FUNC();

	CvCity* pLoopCity = NULL;
	int iLoop;
	int iPathTurns;

	for (pLoopCity = GET_PLAYER(ePlayer).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(ePlayer).nextCity(&iLoop))
	{
		if( pLoopCity->area() == area() )
		{
			if( generatePath(plot(), pLoopCity->plot(), iFlags, true, &iPathTurns, iMaxPathTurns) )
			{
				if( (iMaxPathTurns < 0) || (iPathTurns <= iMaxPathTurns) )
				{
					return true;
				}
			}
		}
	}

	return false;
}

bool CvSelectionGroup::isHasPathToAreaEnemyCity( bool bIgnoreMinors, int iFlags, int iMaxPathTurns )
{
	PROFILE_FUNC();

	int iI;
	int iPass = 0;

	for( iI = 0; iI < MAX_PLAYERS; iI++ )
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive() && isPotentialEnemy(getTeam(), GET_PLAYER((PlayerTypes)iI).getTeam()) )
		{
			if( !bIgnoreMinors || (!GET_PLAYER((PlayerTypes)iI).isBarbarian() && !GET_PLAYER((PlayerTypes)iI).isMinorCiv()) )
			{
				if( isHasPathToAreaPlayerCity((PlayerTypes)iI, iFlags, iMaxPathTurns) )
				{
					return true;
				}
			}
		}
	}

	return false;
}

bool CvSelectionGroup::isStranded() const
{
	/*PROFILE_FUNC();

	if( !(m_bIsStrandedCacheValid) )
	{
		m_bIsStrandedCache = calculateIsStranded();
		m_bIsStrandedCacheValid = true;
	}

	return m_bIsStrandedCache; */

	return AI_getMissionAIType() == MISSIONAI_STRANDED; // K-Mod
}

bool CvSelectionGroup::canMoveAllTerrain() const
{
	PROFILE_FUNC();

	CLLNode<IDInfo>* pUnitNode;
	CvUnit* pLoopUnit;

	pUnitNode = headUnitNode();

	while (pUnitNode != NULL)
	{
		pLoopUnit = ::getUnit(pUnitNode->m_data);
		pUnitNode = nextUnitNode(pUnitNode);

		if (!(pLoopUnit->canMoveAllTerrain()))
		{
			return false;
		}
	}

	return true;
}

void CvSelectionGroup::unloadAll()
{
	CLLNode<IDInfo>* pUnitNode = headUnitNode();
	while (pUnitNode != NULL)
	{
		CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
		pUnitNode = nextUnitNode(pUnitNode);

		if (pLoopUnit != NULL)
		{
			pLoopUnit->unloadAll();
		}
	}
}

bool CvSelectionGroup::alwaysInvisible() const
{
	PROFILE_FUNC();

	CLLNode<IDInfo>* pUnitNode;
	CvUnit* pLoopUnit;

	if (getNumUnits() > 0)
	{
		pUnitNode = headUnitNode();

		while (pUnitNode != NULL)
		{
			pLoopUnit = ::getUnit(pUnitNode->m_data);
			pUnitNode = nextUnitNode(pUnitNode);

			if (!(pLoopUnit->alwaysInvisible()))
			{
				return false;
			}
		}

		return true;
	}

	return false;
}


bool CvSelectionGroup::isInvisible(TeamTypes eTeam) const
{
	if (getNumUnits() > 0)
	{
		CLLNode<IDInfo>* pUnitNode = headUnitNode();

		while (pUnitNode != NULL)
		{
			CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
			pUnitNode = nextUnitNode(pUnitNode);

			if (!pLoopUnit->isInvisible(eTeam, false))
			{
				return false;
			}
		}

		return true;
	}

	return false;
}


int CvSelectionGroup::countNumUnitAIType(UnitAITypes eUnitAI)
{
	CLLNode<IDInfo>* pUnitNode;
	CvUnit* pLoopUnit;
	int iCount;

	FAssertMsg(headUnitNode() != NULL, "headUnitNode() is not expected to be equal with NULL");

	iCount = 0;

	pUnitNode = headUnitNode();

	while (pUnitNode != NULL)
	{
		pLoopUnit = ::getUnit(pUnitNode->m_data);
		pUnitNode = nextUnitNode(pUnitNode);
		
		// count all units if NO_UNITAI passed in
		if (NO_UNITAI == eUnitAI || pLoopUnit->AI_getUnitAIType() == eUnitAI)
		{
			iCount++;
		}
	}

	return iCount;
}


bool CvSelectionGroup::hasWorker()
{
	return ((countNumUnitAIType(UNITAI_WORKER) > 0) || (countNumUnitAIType(UNITAI_WORKER_SEA) > 0));
}


bool CvSelectionGroup::IsSelected()
{
	CLLNode<IDInfo>* pUnitNode;
	CvUnit* pLoopUnit;

	pUnitNode = headUnitNode();

	while (pUnitNode != NULL)
	{
		pLoopUnit = ::getUnit(pUnitNode->m_data);
		pUnitNode = nextUnitNode(pUnitNode);

		if (pLoopUnit->IsSelected())
		{
			return true;
		}
	}

	return false;
}


void CvSelectionGroup::NotifyEntity(MissionTypes eMission)
{
	CLLNode<IDInfo>* pUnitNode;

	pUnitNode = headUnitNode();

	while (pUnitNode != NULL)
	{
		::getUnit(pUnitNode->m_data)->NotifyEntity(eMission);
		pUnitNode = nextUnitNode(pUnitNode);
	}
}


void CvSelectionGroup::airCircle(bool bStart)
{
	CLLNode<IDInfo>* pUnitNode;

	pUnitNode = headUnitNode();

	while (pUnitNode != NULL)
	{
		::getUnit(pUnitNode->m_data)->airCircle(bStart);
		pUnitNode = nextUnitNode(pUnitNode);
	}
}


void CvSelectionGroup::setBlockading(bool bStart)
{
	CLLNode<IDInfo>* pUnitNode = headUnitNode();

	while (pUnitNode != NULL)
	{
		::getUnit(pUnitNode->m_data)->setBlockading(bStart);
		pUnitNode = nextUnitNode(pUnitNode);
	}
}


int CvSelectionGroup::getX() const
{
	CvUnit* pHeadUnit;

	pHeadUnit = getHeadUnit();

	if (pHeadUnit != NULL)
	{
		return getHeadUnit()->getX_INLINE();
	}
	else
	{
		return INVALID_PLOT_COORD;
	}
}


int CvSelectionGroup::getY() const
{
	CvUnit* pHeadUnit;

	pHeadUnit = getHeadUnit();

	if (pHeadUnit != NULL)
	{
		return getHeadUnit()->getY_INLINE();
	}
	else
	{
		return INVALID_PLOT_COORD;
	}
}


bool CvSelectionGroup::at(int iX, int iY) const
{
	return((getX() == iX) && (getY() == iY));
}


bool CvSelectionGroup::atPlot( const CvPlot* pPlot) const
{
	return (plot() == pPlot);
}


CvPlot* CvSelectionGroup::plot() const
{
	CvUnit* pHeadUnit;

	pHeadUnit = getHeadUnit();

	if (pHeadUnit != NULL)
	{
		return getHeadUnit()->plot();
	}
	else
	{
		return NULL;
	}
}


int CvSelectionGroup::getArea() const
{
	CvUnit* pHeadUnit;

	pHeadUnit = getHeadUnit();

	if (pHeadUnit != NULL)
	{
		return getHeadUnit()->getArea();
	}
	else
	{
		return NULL;
	}
}

CvArea* CvSelectionGroup::area() const
{
	CvUnit* pHeadUnit;

	pHeadUnit = getHeadUnit();

	if (pHeadUnit != NULL)
	{
		return getHeadUnit()->area();
	}
	else
	{
		return NULL;
	}
}


DomainTypes CvSelectionGroup::getDomainType() const
{
	CvUnit* pHeadUnit;

	pHeadUnit = getHeadUnit();

	if (pHeadUnit != NULL)
	{
		return getHeadUnit()->getDomainType();
	}
	else
	{
		return NO_DOMAIN;
	}
}


RouteTypes CvSelectionGroup::getBestBuildRoute(CvPlot* pPlot, BuildTypes* peBestBuild) const
{
	PROFILE_FUNC();

	CLLNode<IDInfo>* pUnitNode;
	CvUnit* pLoopUnit;
	RouteTypes eRoute;
	RouteTypes eBestRoute;
	int iValue;
	int iBestValue;
	int iI;

	if (peBestBuild != NULL)
	{
		*peBestBuild = NO_BUILD;
	}

	iBestValue = 0;
	eBestRoute = NO_ROUTE;

	pUnitNode = headUnitNode();

	while (pUnitNode != NULL)
	{
		pLoopUnit = ::getUnit(pUnitNode->m_data);
		pUnitNode = nextUnitNode(pUnitNode);

		for (iI = 0; iI < GC.getNumBuildInfos(); iI++)
		{
			eRoute = ((RouteTypes)(GC.getBuildInfo((BuildTypes) iI).getRoute()));

			if (eRoute != NO_ROUTE)
			{
				if (pLoopUnit->canBuild(pPlot, ((BuildTypes)iI)))
				{
					iValue = GC.getRouteInfo(eRoute).getValue();

					if (iValue > iBestValue)
					{
						iBestValue = iValue;
						eBestRoute = eRoute;
						if (peBestBuild != NULL)
						{
							*peBestBuild = ((BuildTypes)iI);
						}
					}
				}
			}
		}
	}

	return eBestRoute;
}


// Returns true if group was bumped...
/* original bts code. disabled by K-Mod
bool CvSelectionGroup::groupDeclareWar(CvPlot* pPlot, bool bForce)
{
	CvTeamAI& kTeam = GET_TEAM(getTeam());
	TeamTypes ePlotTeam = pPlot->getTeam();
	
	if (!AI_isDeclareWar(pPlot))
	{
		return false;
	}

	int iNumUnits = getNumUnits();

	if (bForce || !canEnterArea(ePlotTeam, pPlot->area(), true))
	{
		if (ePlotTeam != NO_TEAM && kTeam.AI_isSneakAttackReady(ePlotTeam))
		{
			if (kTeam.canDeclareWar(ePlotTeam))
			{
				FAssertMsg(false, "war declared using groupDeclareWar"); // K-Mod, I'm trying to phase this out.
				if (gUnitLogLevel > 0) logBBAI("    %S declares war on %S with groupDeclareWar (%S - %S).", kTeam.getName().GetCString(), GET_TEAM(ePlotTeam).getName().GetCString(), getHeadUnit()->getName(0).GetCString(), GC.getUnitAIInfo(getHeadUnitAI()).getDescription());
				kTeam.declareWar(ePlotTeam, true, NO_WARPLAN);
			}
		}
	}

	return (iNumUnits != getNumUnits());
} */


// Returns true if attack was made...
bool CvSelectionGroup::groupAttack(int iX, int iY, int iFlags, bool& bFailedAlreadyFighting)
{
	PROFILE_FUNC();

	CvPlot* pDestPlot = GC.getMapINLINE().plotINLINE(iX, iY);

	// K-Mod. Rather than clearing the existing path data; use a temporary pathfinder.
	KmodPathFinder final_path;
	final_path.SetSettings(this, iFlags & ~MOVE_DECLARE_WAR);

	/* original bts code
	if (iFlags & MOVE_THROUGH_ENEMY)
	{
		if (generatePath(plot(), pDestPlot, iFlags))
		{
			pDestPlot = getPathFirstPlot();
		}
	} */
	// K-Mod
	if (iFlags & (MOVE_THROUGH_ENEMY | MOVE_ATTACK_STACK) && !(iFlags & MOVE_DIRECT_ATTACK))
	{
		if (final_path.GeneratePath(pDestPlot))
		{
			pDestPlot = final_path.GetPathFirstPlot();
		}
	}
	// K-Mod end

	FAssertMsg(pDestPlot != NULL, "DestPlot is not assigned a valid value");

	bool bStack = (isHuman() && ((getDomainType() == DOMAIN_AIR) || GET_PLAYER(getOwnerINLINE()).isOption(PLAYEROPTION_STACK_ATTACK)));
    
	bool bAttack = false;
	bFailedAlreadyFighting = false;

	if (getNumUnits() > 0)
	{
		if ((getDomainType() == DOMAIN_AIR) || (stepDistance(getX(), getY(), pDestPlot->getX_INLINE(), pDestPlot->getY_INLINE()) == 1))
		{
			//if ((iFlags & MOVE_DIRECT_ATTACK) || (getDomainType() == DOMAIN_AIR) || (iFlags & MOVE_THROUGH_ENEMY) || (generatePath(plot(), pDestPlot, iFlags) && (getPathFirstPlot() == pDestPlot)))
			if (iFlags & (MOVE_THROUGH_ENEMY | MOVE_ATTACK_STACK | MOVE_DIRECT_ATTACK) || getDomainType() == DOMAIN_AIR || (final_path.GeneratePath(pDestPlot) && final_path.GetPathFirstPlot() == pDestPlot)) // K-Mod
			{
				int iAttackOdds;
				CvUnit* pBestAttackUnit = AI_getBestGroupAttacker(pDestPlot, true, iAttackOdds);

				if (pBestAttackUnit)
				{
					// K-Mod, bugfix. This needs to happen before hadDefender, since hasDefender tests for war..
					// (note: this check is no longer going to be important at all once my new AI DOW code is complete.)
					/*if (groupDeclareWar(pDestPlot))
					{
						return true;
					}*/
					// K-Mod end

					// if there are no defenders, do not attack
					/* original
					CvUnit* pBestDefender = pDestPlot->getBestDefender(NO_PLAYER, getOwnerINLINE(), pBestAttackUnit, true);
					if (NULL == pBestDefender)
					{
						return false;
					} */
					// Lead From Behind by UncutDragon
					if (!pDestPlot->hasDefender(false, NO_PLAYER, getOwnerINLINE(), pBestAttackUnit, true))
						return false;
					//

					bool bNoBlitz = (!pBestAttackUnit->isBlitz() || !pBestAttackUnit->isMadeAttack());

					/*if (groupDeclareWar(pDestPlot))
					{
						return true;
					}*/ // K-Mod, moved up.

					while (true)
					{
						pBestAttackUnit = AI_getBestGroupAttacker(pDestPlot, false, iAttackOdds, false, bNoBlitz);
						if (pBestAttackUnit == NULL)
						{
							break;
						}

						if (iAttackOdds < 68)
						{
						    CvUnit * pBestSacrifice = AI_getBestGroupSacrifice(pDestPlot, false, false, bNoBlitz);
						    if (pBestSacrifice != NULL)
						    {
                                pBestAttackUnit = pBestSacrifice;
						    }
						}

						bAttack = true;

						if (GC.getUSE_DO_COMBAT_CALLBACK()) // K-Mod. block unused python callbacks
						{
							CySelectionGroup* pyGroup = new CySelectionGroup(this);
							CyPlot* pyPlot = new CyPlot(pDestPlot);
							CyArgsList argsList;
							argsList.add(gDLL->getPythonIFace()->makePythonObject(pyGroup));	// pass in Selection Group class
							argsList.add(gDLL->getPythonIFace()->makePythonObject(pyPlot));	// pass in Plot class
							long lResult=0;
							gDLL->getPythonIFace()->callFunction(PYGameModule, "doCombat", argsList.makeFunctionArgs(), &lResult);
							delete pyGroup;	// python fxn must not hold on to this pointer 
							delete pyPlot;	// python fxn must not hold on to this pointer 
							if (lResult == 1)
							{
								break;
							}
						}

						if (getNumUnits() > 1)
						{
							/* original bts code
							if (pBestAttackUnit->plot()->isFighting() || pDestPlot->isFighting())
							{
								bFailedAlreadyFighting = true;
							}
							else
							{
								pBestAttackUnit->attack(pDestPlot, bStack);
							} */
							// K-Mod
							if (pBestAttackUnit->plot()->isFighting() || pDestPlot->isFighting())
							{
								bFailedAlreadyFighting = true;
							}
							if (!pBestAttackUnit->isAttacking())
							{
								// we need to issue the attack order to start the attack
								pBestAttackUnit->attack(pDestPlot, bStack);
							}
							// K-Mod end
						}
						else
						{
							// K-Mod note. We should do this even if the fight can't happen right away.
							pBestAttackUnit->attack(pDestPlot, false);
							break;
						}

						if (bFailedAlreadyFighting || !bStack)
						{
							// if this is AI stack, follow through with the attack to the end
							//if (!isHuman() && getNumUnits() > 1)
							if (!isHuman() && getNumUnits() > 1 && !(iFlags & MOVE_SINGLE_ATTACK)) // K-Mod
							{
								//AI_queueGroupAttack(iX, iY);
								AI_queueGroupAttack(pDestPlot->getX_INLINE(), pDestPlot->getY_INLINE()); // K-Mod
							}

							break;
						}
					}
				}
			}
		}
	}

	return bAttack;
}


void CvSelectionGroup::groupMove(CvPlot* pPlot, bool bCombat, CvUnit* pCombatUnit, bool bEndMove)
{
	//PROFILE_FUNC();

	CLLNode<IDInfo>* pUnitNode;
	CvUnit* pLoopUnit;

	pUnitNode = headUnitNode();
	CvSelectionGroup* pStaticGroup = 0; // K-Mod

	while (pUnitNode != NULL)
	{
		pLoopUnit = ::getUnit(pUnitNode->m_data);
		pUnitNode = nextUnitNode(pUnitNode);

		if ((pLoopUnit->canMove() && ((bCombat && (!(pLoopUnit->isNoCapture()) || !(pPlot->isEnemyCity(*pLoopUnit)))) ? pLoopUnit->canMoveOrAttackInto(pPlot) : pLoopUnit->canMoveInto(pPlot))) || (pLoopUnit == pCombatUnit))
		{
			FAssert(pLoopUnit->canEnterTerritory(pPlot->getTeam())); // K-Mod (trying to clean up some of the incidious mingling of AI and mechanics)
			pLoopUnit->move(pPlot, true);
		}
		else
		{
			// pLoopUnit->joinGroup(NULL, true);
			// K-Mod. all units left behind should stay in the same group.
			// (Note: it is important that units left behind are not in the original group.
			// The later code assume that the original group has moved, and if it hasn't, there will be an infinite loop.)
			if (pStaticGroup)
				pLoopUnit->joinGroup(pStaticGroup, true);
			else
			{
				pLoopUnit->joinGroup(0, true);
				pStaticGroup = pLoopUnit->getGroup();
			}
			// K-Mod end
			pLoopUnit->ExecuteMove(((float)(GC.getMissionInfo(MISSION_MOVE_TO).getTime() * gDLL->getMillisecsPerTurn())) / 1000.0f, false);
		}
	}

	//execute move
	if(bEndMove || !canAllMove())
	{
		pUnitNode = headUnitNode();
		while(pUnitNode != NULL)
		{
			pLoopUnit = ::getUnit(pUnitNode->m_data);
			pUnitNode = nextUnitNode(pUnitNode);

			pLoopUnit->ExecuteMove(((float)(GC.getMissionInfo(MISSION_MOVE_TO).getTime() * gDLL->getMillisecsPerTurn())) / 1000.0f, false);
		}
	}
}


// Returns true if move was made...
bool CvSelectionGroup::groupPathTo(int iX, int iY, int iFlags)
{
	KmodPathFinder final_path; // K-Mod

	if (at(iX, iY))
	{
		return false; // XXX is this necessary?
	}

	FAssert(!isBusy());
	FAssert(getOwnerINLINE() != NO_PLAYER);
	FAssert(headMissionQueueNode() != NULL);

	CvPlot* pDestPlot = GC.getMapINLINE().plotINLINE(iX, iY);
	FAssertMsg(pDestPlot != NULL, "DestPlot is not assigned a valid value");

	FAssertMsg(canAllMove(), "canAllMove is expected to be true");

	CvPlot* pPathPlot;

	if (getDomainType() == DOMAIN_AIR)
	{
		if (!canMoveInto(pDestPlot))
		{
			return false;
		}

		pPathPlot = pDestPlot;
	}
	else
	{
		/* original bts code
		if (!generatePath(plot(), pDestPlot, iFlags & ~MOVE_DECLARE_WAR))
		{
			return false;
		}

		pPathPlot = getPathFirstPlot(); */
		// K-Mod. I've added & ~MOVE_DECLARE_WAR so that if we need to declare war at this point, and haven't yet done so,
		// the move will fail here rather than splitting the group inside groupMove.
		// Also, I've change it to use a different pathfinder, to avoid clearing the path data - and to avoid OOS errors.
		final_path.SetSettings(this, iFlags & ~MOVE_DECLARE_WAR);
		if (!final_path.GeneratePath(pDestPlot))
		{
			return false;
		}

		pPathPlot = final_path.GetPathFirstPlot();
		// K-Mod end

		if (groupAmphibMove(pPathPlot, iFlags))
		{
			return false;
		}
	}
	
	/* original bts code
	bool bForce = false;
	MissionAITypes eMissionAI = AI_getMissionAIType();
	if (eMissionAI == MISSIONAI_BLOCKADE || eMissionAI == MISSIONAI_PILLAGE)
	{
		bForce = true;
	}

	if (groupDeclareWar(pPathPlot, bForce))
	{
		return false;
	} */ // Disabled by K-Mod. AI war decisions have no business being here.

	bool bEndMove = false;
	if(pPathPlot == pDestPlot)
		bEndMove = true;

	groupMove(pPathPlot, iFlags & MOVE_THROUGH_ENEMY, NULL, bEndMove);

	FAssert(getNumUnits() == 0 || atPlot(pPathPlot)); // K-Mod

	// K-Mod. If the step we just took will make us change our path to something longer, then cancel the move.
	// This prevents units from wasting all their moves by trying to walk around enemy units.
	if (!AI_isControlled() && !bEndMove)
	{
		FAssert(final_path.GetEndNode());
		std::pair<int, int> old_moves = std::make_pair(final_path.GetEndNode()->m_iData2, -final_path.GetEndNode()->m_iData1);
		if (!final_path.GeneratePath(pDestPlot)
			|| std::make_pair(final_path.GetEndNode()->m_iData2, -final_path.GetEndNode()->m_iData1) > old_moves)
		{
			clearMissionQueue();
		}
	}
	// K-Mod end

	return true;
}


// Returns true if move was made...
bool CvSelectionGroup::groupRoadTo(int iX, int iY, int iFlags)
{
	CvPlot* pPlot;
	RouteTypes eBestRoute;
	BuildTypes eBestBuild;

	if (!AI_isControlled() || !at(iX, iY) || (getLengthMissionQueue() == 1))
	{
		pPlot = plot();

		eBestRoute = getBestBuildRoute(pPlot, &eBestBuild);

		if (eBestBuild != NO_BUILD)
		{
			groupBuild(eBestBuild);
			return true;
		}
	}

	return groupPathTo(iX, iY, iFlags);
}


// Returns true if build should continue...
bool CvSelectionGroup::groupBuild(BuildTypes eBuild)
{
	CLLNode<IDInfo>* pUnitNode;
	CvUnit* pLoopUnit;
	CvPlot* pPlot;
	bool bContinue;

	FAssert(getOwnerINLINE() != NO_PLAYER);
	FAssertMsg(eBuild < GC.getNumBuildInfos(), "Invalid Build");

	bContinue = false;

	pPlot = plot();

    ImprovementTypes eImprovement = (ImprovementTypes)GC.getBuildInfo(eBuild).getImprovement();
	if (eImprovement != NO_IMPROVEMENT)
	{
		if (AI_isControlled())
		{
			if (GET_PLAYER(getOwnerINLINE()).isOption(PLAYEROPTION_SAFE_AUTOMATION))
			{
				if ((pPlot->getImprovementType() != NO_IMPROVEMENT) && (pPlot->getImprovementType() != (ImprovementTypes)(GC.getDefineINT("RUINS_IMPROVEMENT"))))
				{
				    BonusTypes eBonus = (BonusTypes)pPlot->getNonObsoleteBonusType(GET_PLAYER(getOwnerINLINE()).getTeam());
				    if ((eBonus == NO_BONUS) || !GC.getImprovementInfo(eImprovement).isImprovementBonusTrade(eBonus))
				    {
                        if (GC.getImprovementInfo(eImprovement).getImprovementPillage() != NO_IMPROVEMENT)
                        {
                            return false;
                        }
				    }
				}
			}
//
//			if (AI_getMissionAIType() == MISSION_BUILD)
//			{
//                CvCity* pWorkingCity = pPlot->getWorkingCity();
//                if ((pWorkingCity != NULL) && (AI_getMissionAIPlot() == pPlot))
//                {
//                    if (pWorkingCity->AI_getBestBuild(pWorkingCity->getCityPlotIndex(pPlot)) != eBuild)
//                    {
//                        return false;
//                    }
//                }
//			}
		}
	}

	pUnitNode = headUnitNode();

	while (pUnitNode != NULL)
	{
		pLoopUnit = ::getUnit(pUnitNode->m_data);
		pUnitNode = nextUnitNode(pUnitNode);

		FAssertMsg(pLoopUnit->atPlot(pPlot), "pLoopUnit is expected to be at pPlot");

		if (pLoopUnit->canBuild(pPlot, eBuild))
		{
			bContinue = true;

			if (pLoopUnit->build(eBuild))
			{
				bContinue = false;
				break;
			}
		}
	}

	return bContinue;
}

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      04/18/10                                jdog5000      */
/*                                                                                              */
/* General AI                                                                                   */
/************************************************************************************************/
void CvSelectionGroup::setTransportUnit(CvUnit* pTransportUnit, CvSelectionGroup** pOtherGroup)
{
	// if we are loading
	if (pTransportUnit != NULL)
	{
		CvUnit* pHeadUnit = getHeadUnit();
		FAssertMsg(pHeadUnit != NULL, "non-zero group without head unit");
		
		int iCargoSpaceAvailable = pTransportUnit->cargoSpaceAvailable(pHeadUnit->getSpecialUnitType(), pHeadUnit->getDomainType());
		
		// if no space at all, give up
		if (iCargoSpaceAvailable < 1)
		{
			return;
		}

		// if there is space, but not enough to fit whole group, then split us, and set on the new group
		if (iCargoSpaceAvailable < getNumUnits())
		{
			CvSelectionGroup* pSplitGroup = splitGroup(iCargoSpaceAvailable, NULL, pOtherGroup);
			if (pSplitGroup != NULL)
			{
				pSplitGroup->setTransportUnit(pTransportUnit);
			}

			return;
		}
		
		FAssertMsg(iCargoSpaceAvailable >= getNumUnits(), "cargo size too small");
		
		// setTransportUnit removes the unit from the current group (at least currently), so we have to be careful in the loop here
		// so, loop until we do not load one
		bool bLoadedOne;
		do
		{
			bLoadedOne = false;

			// loop over all the units, finding one to load
			CLLNode<IDInfo>* pUnitNode = headUnitNode();
			while (pUnitNode != NULL && !bLoadedOne)
			{
				CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
				pUnitNode = nextUnitNode(pUnitNode);
				
				// just in case implementation of setTransportUnit changes, check to make sure this unit is not already loaded
				if (pLoopUnit != NULL && pLoopUnit->getTransportUnit() != pTransportUnit)
				{
					// if there is room, load the unit and stop the loop (since setTransportUnit ungroups this unit currently)
					bool bSpaceAvailable = pTransportUnit->cargoSpaceAvailable(pLoopUnit->getSpecialUnitType(), pLoopUnit->getDomainType());
					if (bSpaceAvailable)
					{
						pLoopUnit->setTransportUnit(pTransportUnit);
						bLoadedOne = true;

					}
				}
			}
		}
		while (bLoadedOne);
	}
	// otherwise we are unloading
	else
	{
		// loop over all the units, unloading them
		CLLNode<IDInfo>* pUnitNode = headUnitNode();
		while (pUnitNode != NULL)
		{
			CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
			pUnitNode = nextUnitNode(pUnitNode);
			
			if (pLoopUnit != NULL)
			{
				// unload unit
				pLoopUnit->setTransportUnit(NULL);
			}
		}
	}
}


/// \brief Function for loading stranded units onto an offshore transport
///
void CvSelectionGroup::setRemoteTransportUnit(CvUnit* pTransportUnit)
{
	// if we are loading
	if (pTransportUnit != NULL)
	{
		CvUnit* pHeadUnit = getHeadUnit();
		FAssertMsg(pHeadUnit != NULL, "non-zero group without head unit");
		
		int iCargoSpaceAvailable = pTransportUnit->cargoSpaceAvailable(pHeadUnit->getSpecialUnitType(), pHeadUnit->getDomainType());
		
		// if no space at all, give up
		if (iCargoSpaceAvailable < 1)
		{
			return;
		}

		// if there is space, but not enough to fit whole group, then split us, and set on the new group
		if (iCargoSpaceAvailable < getNumUnits())
		{
			CvSelectionGroup* pSplitGroup = splitGroup(iCargoSpaceAvailable);
			if (pSplitGroup != NULL)
			{
				pSplitGroup->setRemoteTransportUnit(pTransportUnit);
			}
			return;
		}
		
		FAssertMsg(iCargoSpaceAvailable >= getNumUnits(), "cargo size too small");

		bool bLoadedOne;
		do
		{
			bLoadedOne = false;

			// loop over all the units on the plot, looping through this selection group did not work
			CLLNode<IDInfo>* pUnitNode = headUnitNode();
			while (pUnitNode != NULL && !bLoadedOne)
			{
				CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
				pUnitNode = nextUnitNode(pUnitNode);
				
				if (pLoopUnit != NULL && pLoopUnit->getTransportUnit() != pTransportUnit && pLoopUnit->getOwnerINLINE() == pTransportUnit->getOwnerINLINE())
				{
					bool bSpaceAvailable = pTransportUnit->cargoSpaceAvailable(pLoopUnit->getSpecialUnitType(), pLoopUnit->getDomainType());
					if (bSpaceAvailable)
					{
						if( !(pLoopUnit->atPlot(pTransportUnit->plot())) )
						{
							// Putting a land unit on water automatically loads it
							pLoopUnit->setXY(pTransportUnit->getX_INLINE(),pTransportUnit->getY_INLINE());
						}

						if( pLoopUnit->getTransportUnit() != pTransportUnit ) 
						{
							pLoopUnit->setTransportUnit(pTransportUnit);
						}

						bLoadedOne = true;
					}
				}
			}
		}
		while (bLoadedOne);
	}
	// otherwise we are unloading
	else
	{
		// loop over all the units, unloading them
		CLLNode<IDInfo>* pUnitNode = headUnitNode();
		while (pUnitNode != NULL)
		{
			CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
			pUnitNode = nextUnitNode(pUnitNode);
			
			if (pLoopUnit != NULL)
			{
				// unload unit
				pLoopUnit->setTransportUnit(NULL);
			}
		}
	}
}

bool CvSelectionGroup::isAmphibPlot(const CvPlot* pPlot) const
{
	bool bFriendly = true;
	CvUnit* pUnit = getHeadUnit();
	if (NULL != pUnit)
	{
		bFriendly = pPlot->isFriendlyCity(*pUnit, true);
	}

	//return ((getDomainType() == DOMAIN_SEA) && pPlot->isCoastalLand() && !bFriendly && !canMoveAllTerrain());

	if (getDomainType() == DOMAIN_SEA)
	{
		if (pPlot->isCity() && !bFriendly && (pPlot->isCoastalLand() || pPlot->isWater() || canMoveAllTerrain()))
		{
			return true;
		}
		return (pPlot->isCoastalLand() && !bFriendly && !canMoveAllTerrain());
	}
	return false;
}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

// Returns true if attempted an amphib landing...
bool CvSelectionGroup::groupAmphibMove(CvPlot* pPlot, int iFlags)
{
	CLLNode<IDInfo>* pUnitNode1;
	CvUnit* pLoopUnit1;
	bool bLanding = false;

	FAssert(getOwnerINLINE() != NO_PLAYER);

	/* original bts code
	if (groupDeclareWar(pPlot))
	{
		return true;
	} */
	// K-Mod. I've disabled this groupDeclareWar for a bunch of reasons. Suffice to say, it shouldn't be here.

	if (isAmphibPlot(pPlot))
	{
		if (stepDistance(getX(), getY(), pPlot->getX_INLINE(), pPlot->getY_INLINE()) == 1)
		{
			pUnitNode1 = headUnitNode();

			// K-Mod: I've rearranged some stuff in the following section to fix a bug.
			// originally, the cargo groups loop was done for each cargo-carrying unit - which is incorrect.
			std::vector<CvSelectionGroup*> aCargoGroups;
			while (pUnitNode1 != NULL)
			{
				pLoopUnit1 = ::getUnit(pUnitNode1->m_data);
				pUnitNode1 = nextUnitNode(pUnitNode1);

				if (pLoopUnit1->getCargo() > 0 && pLoopUnit1->domainCargo() == DOMAIN_LAND)
				{
					std::vector<CvUnit*> aCargoUnits;
					pLoopUnit1->getCargoUnits(aCargoUnits);
					for (size_t i = 0; i < aCargoUnits.size(); ++i)
					{
						CvSelectionGroup* pGroup = aCargoUnits[i]->getGroup();
						if (std::find(aCargoGroups.begin(), aCargoGroups.end(), pGroup) == aCargoGroups.end())
						{
							aCargoGroups.push_back(aCargoUnits[i]->getGroup());
						}
					}
				}
			}
			for (size_t i = 0; i < aCargoGroups.size(); ++i)
			{
				CvSelectionGroup* pGroup = aCargoGroups[i];
				if (pGroup->canAllMove())
				{
					FAssert(!pGroup->at(pPlot->getX_INLINE(), pPlot->getY_INLINE()));
					pGroup->pushMission(MISSION_MOVE_TO, pPlot->getX_INLINE(), pPlot->getY_INLINE(), (MOVE_IGNORE_DANGER | iFlags));
					bLanding = true;
				}
			}
			// K-Mod end
		}
	}

	return bLanding;
}


bool CvSelectionGroup::readyToSelect(bool bAny)
{
	return (readyToMove(bAny) && !isAutomated());
}


bool CvSelectionGroup::readyToMove(bool bAny)
{
	return (((bAny) ? canAnyMove() : canAllMove()) && (headMissionQueueNode() == NULL) && (getActivityType() == ACTIVITY_AWAKE) && !isBusy() && !isCargoBusy());
}


bool CvSelectionGroup::readyToAuto()
{
	//return (canAllMove() && (headMissionQueueNode() != NULL));
	return canAllMove() && (headMissionQueueNode() != NULL || (getActivityType() == ACTIVITY_AWAKE && isAutomated()));
}


int CvSelectionGroup::getID() const
{
	return m_iID;
}


void CvSelectionGroup::setID(int iID)
{
	m_iID = iID;
}


PlayerTypes CvSelectionGroup::getOwner() const
{
	return getOwnerINLINE();
}


TeamTypes CvSelectionGroup::getTeam() const
{
	if (getOwnerINLINE() != NO_PLAYER)
	{
		return GET_PLAYER(getOwnerINLINE()).getTeam();
	}

	return NO_TEAM;
}


int CvSelectionGroup::getMissionTimer() const
{
	return m_iMissionTimer;
}


void CvSelectionGroup::setMissionTimer(int iNewValue)
{
	FAssert(getOwnerINLINE() != NO_PLAYER);

	m_iMissionTimer = iNewValue;
	FAssert(getMissionTimer() >= 0);
}


void CvSelectionGroup::changeMissionTimer(int iChange)
{
	setMissionTimer(getMissionTimer() + iChange);
}


void CvSelectionGroup::updateMissionTimer(int iSteps)
{
	CvUnit* pTargetUnit;
	CvPlot* pTargetPlot;
	int iTime;

	if (!isHuman() && !showMoves())
	{
		iTime = 0;
	}
	else if (headMissionQueueNode() != NULL)
	{
		iTime = GC.getMissionInfo((MissionTypes)(headMissionQueueNode()->m_data.eMissionType)).getTime();

		if ((headMissionQueueNode()->m_data.eMissionType == MISSION_MOVE_TO) ||
				(headMissionQueueNode()->m_data.eMissionType == MISSION_ROUTE_TO) ||
				(headMissionQueueNode()->m_data.eMissionType == MISSION_MOVE_TO_UNIT))
		{
			if (headMissionQueueNode()->m_data.eMissionType == MISSION_MOVE_TO_UNIT)
			{
				pTargetUnit = GET_PLAYER((PlayerTypes)headMissionQueueNode()->m_data.iData1).getUnit(headMissionQueueNode()->m_data.iData2);
				if (pTargetUnit != NULL)
				{
					pTargetPlot = pTargetUnit->plot();
				}
				else
				{
					pTargetPlot = NULL;
				}
			}
			else
			{
				pTargetPlot = GC.getMapINLINE().plotINLINE(headMissionQueueNode()->m_data.iData1, headMissionQueueNode()->m_data.iData2);
			}

			if (atPlot(pTargetPlot))
			{
				iTime += iSteps;
			}
			else
			{
				iTime = std::min(iTime, 2);
			}
		}

		if (isHuman() && (isAutomated() || (GET_PLAYER((GC.getGameINLINE().isNetworkMultiPlayer()) ? getOwnerINLINE() : GC.getGameINLINE().getActivePlayer()).isOption(PLAYEROPTION_QUICK_MOVES))))
		{
			iTime = std::min(iTime, 1);
		}
	}
	else
	{
		iTime = 0;
	}

	setMissionTimer(iTime);
}


bool CvSelectionGroup::isForceUpdate()
{
	return m_bForceUpdate;
}


void CvSelectionGroup::setForceUpdate(bool bNewValue)
{
	m_bForceUpdate = bNewValue;
}

// K-Mod
void CvSelectionGroup::doForceUpdate()
{
	clearMissionQueue();
	setActivityType(ACTIVITY_AWAKE);
	setForceUpdate(false);

	// if we are in the middle of attacking with a stack, cancel it
	AI_cancelGroupAttack();
}
// K-Mod end

ActivityTypes CvSelectionGroup::getActivityType() const
{
	return m_eActivityType;
}


void CvSelectionGroup::setActivityType(ActivityTypes eNewValue)
{
	CLLNode<IDInfo>* pUnitNode;
	CvUnit* pLoopUnit;
	CvPlot* pPlot;

	FAssert(getOwnerINLINE() != NO_PLAYER);
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      04/28/10                                jdog5000      */
/*                                                                                              */
/* Unit AI                                                                                      */
/************************************************************************************************/
	// For debugging activities, but can cause crashes very occasionally times
	//FAssert(isHuman() || getHeadUnit()->isCargo() || eNewValue != ACTIVITY_SLEEP);
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

	ActivityTypes eOldActivity = getActivityType();

	if (eOldActivity != eNewValue)
	{
		pPlot = plot();

		if (eOldActivity == ACTIVITY_INTERCEPT)
		{
			airCircle(false);
		}

		setBlockading(false);

		m_eActivityType = eNewValue;

		if (getActivityType() == ACTIVITY_INTERCEPT)
		{
			airCircle(true);
		}

		if (getActivityType() != ACTIVITY_MISSION)
		{
			pUnitNode = headUnitNode();

			if (getActivityType() != ACTIVITY_INTERCEPT)
			{
				//don't idle intercept animation
				while (pUnitNode != NULL)
				{
					pLoopUnit = ::getUnit(pUnitNode->m_data);
					pUnitNode = nextUnitNode(pUnitNode);

					pLoopUnit->NotifyEntity(MISSION_IDLE);
				}
			}

			if (getTeam() == GC.getGameINLINE().getActiveTeam())
			{
				if (pPlot != NULL)
				{
					pPlot->setFlagDirty(true);
				}
			}
		}

		if (pPlot == gDLL->getInterfaceIFace()->getSelectionPlot())
		{
			gDLL->getInterfaceIFace()->setDirty(PlotListButtons_DIRTY_BIT, true);
			gDLL->getInterfaceIFace()->setDirty(SelectionButtons_DIRTY_BIT, true);
		}
	}
}


AutomateTypes CvSelectionGroup::getAutomateType() const														
{
	return m_eAutomateType;
}


bool CvSelectionGroup::isAutomated()
{
	return (getAutomateType() != NO_AUTOMATE);
}


void CvSelectionGroup::setAutomateType(AutomateTypes eNewValue)
{
	FAssert(getOwnerINLINE() != NO_PLAYER);

	if (getAutomateType() != eNewValue)
	{
		m_eAutomateType = eNewValue;

		clearMissionQueue();
		setActivityType(ACTIVITY_AWAKE);

		// if canceling automation, cancel on cargo as well
		if (eNewValue == NO_AUTOMATE)
		{
			CvPlot* pPlot = plot();
			if (pPlot != NULL)
			{
				CLLNode<IDInfo>* pUnitNode = pPlot->headUnitNode();
				while (pUnitNode != NULL)
				{
					CvUnit* pCargoUnit = ::getUnit(pUnitNode->m_data);
					pUnitNode = pPlot->nextUnitNode(pUnitNode);
					
					CvUnit* pTransportUnit = pCargoUnit->getTransportUnit();
					if (pTransportUnit != NULL && pTransportUnit->getGroup() == this)
					{
						pCargoUnit->getGroup()->setAutomateType(NO_AUTOMATE);
						pCargoUnit->getGroup()->setActivityType(ACTIVITY_AWAKE);
					}
				}
			}
		}
	}
}


FAStarNode* CvSelectionGroup::getPathLastNode() const
{
#ifdef KMOD_PATH_FINDER
	return path_finder.GetEndNode();
#else
	return gDLL->getFAStarIFace()->GetLastNode(&GC.getPathFinder());
#endif
}


CvPlot* CvSelectionGroup::getPathFirstPlot() const
{
	FAStarNode* pNode;

	pNode = getPathLastNode();

	if (pNode->m_pParent == NULL)
	{
		return GC.getMapINLINE().plotSorenINLINE(pNode->m_iX, pNode->m_iY);
	}

	while (pNode != NULL)
	{
		if (pNode->m_pParent->m_pParent == NULL)
		{
			return GC.getMapINLINE().plotSorenINLINE(pNode->m_iX, pNode->m_iY);
		}

		pNode = pNode->m_pParent;
	}

	FAssert(false);

	return NULL;
}


CvPlot* CvSelectionGroup::getPathEndTurnPlot() const
{
	FAStarNode* pNode;

	pNode = getPathLastNode();

	if (NULL != pNode)
	{
		if ((pNode->m_pParent == NULL) || (pNode->m_iData2 == 1))
		{
			return GC.getMapINLINE().plotSorenINLINE(pNode->m_iX, pNode->m_iY);
		}

		while (pNode->m_pParent != NULL)
		{
			if (pNode->m_pParent->m_iData2 == 1)
			{
				return GC.getMapINLINE().plotSorenINLINE(pNode->m_pParent->m_iX, pNode->m_pParent->m_iY);
			}

			pNode = pNode->m_pParent;
		}
	}

	FAssert(false);

	return NULL;
}


bool CvSelectionGroup::generatePath( const CvPlot* pFromPlot, const CvPlot* pToPlot, int iFlags, bool bReuse, int* piPathTurns, int iMaxPath) const
{
	//	Path generation reuse is only ok for a given stack, not between stacks
	//	so we reset whether to allow path reuse globally for each stack
	//static const CvSelectionGroup* lastPathGeneratedFor = NULL;
	// K-Mod, moved to class scope.

	// K-Mod - if I can top the UI from messing with this pathfinder, I might be able to reduce OOS bugs.
	// (note, the const-cast is just to get around the bad code from the original developers)
	FAssert(const_cast<CvSelectionGroup*>(this)->AI_isControlled());
	// K-Mod end

	PROFILE("CvSelectionGroup::generatePath()")

	FAStarNode* pNode;
	bool bSuccess;
/************************************************************************************************/
/* Afforess	                  Start		 04/09/10                                               */
/*                                                                                              */
/* CTD Fix                                                                                      */
/************************************************************************************************/
	if (pFromPlot == NULL || pToPlot == NULL) return false;
/************************************************************************************************/
/* Afforess	                     END                                                            */
/************************************************************************************************/

	gDLL->getFAStarIFace()->SetData(&GC.getPathFinder(), this);

	//	KOSHLING MOD - the path finder in the core engine is somewhat inefficient and calls pathValid() multiple times for single
	//	plots, so we can cache the results.  Reset the cache prior to each path generation
	m_cachedPathValidityFromPlotX = -1;

	//	Koshling - the path generator screws up if you allow it to reuse info previosuly
	//	generated by a different stack (it gets the move counts wrong potentially)
	// K-Mod. originally this was fixed by setting bReuse = false.
	// I've changed it to instead call resetPath - and only if lastPathGeneratedFor != NULL
	if (lastPathGeneratedFor && lastPathGeneratedFor != this)
	{
		resetPath();
	}

#ifdef KMOD_PATH_FINDER
	/*if (!bReuse)
		path_finder.Reset();*/
	path_finder.SetSettings(this, iFlags, iMaxPath);
	bSuccess = path_finder.GeneratePath(pFromPlot->getX_INLINE(), pFromPlot->getY_INLINE(), pToPlot->getX_INLINE(), pToPlot->getY_INLINE());
	/* test.
	if (bSuccess != gDLL->getFAStarIFace()->GeneratePath(&GC.getPathFinder(), pFromPlot->getX_INLINE(), pFromPlot->getY_INLINE(), pToPlot->getX_INLINE(), pToPlot->getY_INLINE(), false, iFlags, bReuse))
	{
		pNode = gDLL->getFAStarIFace()->GetLastNode(&GC.getPathFinder());
		if (bSuccess || iMaxPath < 0 || !pNode || pNode->m_iData2 <= iMaxPath)
		{
			//::MessageBoxA(NULL,"pathfind mismatch","CvGameCore",MB_OK);
			FAssert(false);
		}
	}
	*/
#else
	iMaxPath = -1; // max path doesn't work with the standard pathfinder.
	bSuccess = gDLL->getFAStarIFace()->GeneratePath(&GC.getPathFinder(), pFromPlot->getX_INLINE(), pFromPlot->getY_INLINE(), pToPlot->getX_INLINE(), pToPlot->getY_INLINE(), false, iFlags, bReuse);
#endif

	//	Invalidate the cache again so that callers from outside the DLL to path generation don't
	//	invoke a stale cache
	m_cachedPathValidityFromPlotX = -2;	//	-2 blocks all attempts to use the cache
	lastPathGeneratedFor = this;

	if (piPathTurns != NULL)
	{
		*piPathTurns = MAX_INT;

		if (bSuccess)
		{
			pNode = getPathLastNode();

			if (pNode != NULL)
			{
				*piPathTurns = pNode->m_iData2;
#ifdef KMOD_PATH_FINDER
				FAssert(iMaxPath <= 0 || iMaxPath >= pNode->m_iData2);
#else
				bSuccess = iMaxPath <= 0 || iMaxPath >= pNode->m_iData2;
#endif
			}
		}
	}

	return bSuccess;
}

//	KOSHLING Mod - add path validity results cache
bool CvSelectionGroup::HaveCachedPathValidityResult(CvPlot* pFromPlot, bool* cachedResult)
{
	m_pathValidityCacheReads++;
	if ( m_cachedPathValidityFromPlotX == pFromPlot->getX_INLINE() &&
		 m_cachedPathValidityFromPlotY == pFromPlot->getY_INLINE() )
	{
		m_pathValidityCacheHits++;
		*cachedResult = m_cachedPathValidityResult;
		return true;
	}
	else
	{
		return false;
	}
}

void CvSelectionGroup::CachePathValidityResult(CvPlot* pFromPlot, bool cachedResult)
{
	//	This is set to -2 outside the scope of calls from CvSelectionGroup::generatePath()
	//	to prevent invlaid caching in other contexts
	if ( m_cachedPathValidityFromPlotX != -2 )
	{
		m_cachedPathValidityFromPlotX = pFromPlot->getX_INLINE();
		m_cachedPathValidityFromPlotY = pFromPlot->getY_INLINE();
		m_cachedPathValidityResult = cachedResult;
	}
}

void CvSelectionGroup::resetPath() const
{
	lastPathGeneratedFor = NULL; // K-Mod
	//path_finder.Reset(); // note. the K-Mod finder doesn't need resetting in all the same places.
	gDLL->getFAStarIFace()->ForceReset(&GC.getPathFinder());
}


void CvSelectionGroup::clearUnits()
{
	CLLNode<IDInfo>* pUnitNode;

	pUnitNode = headUnitNode();

	while (pUnitNode != NULL)
	{
		pUnitNode = deleteUnitNode(pUnitNode);
	}
}


// Returns true if the unit is added...
bool CvSelectionGroup::addUnit(CvUnit* pUnit, bool bMinimalChange)
{
	//PROFILE_FUNC();

	CLLNode<IDInfo>* pUnitNode;
	CvUnit* pLoopUnit;
	bool bAdded;
 
	if (!(pUnit->canJoinGroup(pUnit->plot(), this)))
	{
		return false;
	}

	bAdded = false;

	pUnitNode = headUnitNode();

	while (pUnitNode != NULL)
	{
		pLoopUnit = ::getUnit(pUnitNode->m_data);
		if ((pUnit->AI_groupFirstVal() > pLoopUnit->AI_groupFirstVal()) ||
			  ((pUnit->AI_groupFirstVal() == pLoopUnit->AI_groupFirstVal()) &&
				 (pUnit->AI_groupSecondVal() > pLoopUnit->AI_groupSecondVal())))
		{
			m_units.insertBefore(pUnit->getIDInfo(), pUnitNode);
			bAdded = true;
			break;
		}
		pUnitNode = nextUnitNode(pUnitNode);
	}

	if (!bAdded)
	{
		m_units.insertAtEnd(pUnit->getIDInfo());
	}

	if(!bMinimalChange)
	{
		if (getOwnerINLINE() == NO_PLAYER)
		{
			if (getNumUnits() > 0)
			{
				pUnitNode = headUnitNode();
				while (pUnitNode != NULL)
				{
					//if (pUnitNode != headUnitNode())
					//{
						::getUnit(pUnitNode->m_data)->NotifyEntity(MISSION_MULTI_SELECT);
					//}
					pUnitNode = nextUnitNode(pUnitNode);
				}
			}
		}
	}

	return true;
}


void CvSelectionGroup::removeUnit(CvUnit* pUnit)
{
	CLLNode<IDInfo>* pUnitNode;

	pUnitNode = headUnitNode();

	while (pUnitNode != NULL)
	{
		if (::getUnit(pUnitNode->m_data) == pUnit)
		{
			deleteUnitNode(pUnitNode);
			break;
		}
		else
		{
			pUnitNode = nextUnitNode(pUnitNode);
		}
	}
}


CLLNode<IDInfo>* CvSelectionGroup::deleteUnitNode(CLLNode<IDInfo>* pNode)
{
	CLLNode<IDInfo>* pNextUnitNode;

	if (getOwnerINLINE() != NO_PLAYER)
	{
		setAutomateType(NO_AUTOMATE);
		clearMissionQueue();

		switch (getActivityType())
		{
		case ACTIVITY_SLEEP:
		case ACTIVITY_INTERCEPT:
		case ACTIVITY_PATROL:
		case ACTIVITY_PLUNDER:
			break;
		default:
			setActivityType(ACTIVITY_AWAKE);
			break;
		}
	}

	pNextUnitNode = m_units.deleteNode(pNode);

	return pNextUnitNode;
}


CLLNode<IDInfo>* CvSelectionGroup::nextUnitNode(CLLNode<IDInfo>* pNode) const
{
	return m_units.next(pNode);
}


int CvSelectionGroup::getNumUnits() const
{
	return m_units.getLength();
}

void CvSelectionGroup::mergeIntoGroup(CvSelectionGroup* pSelectionGroup)
{
	CvPlayerAI& kPlayer = GET_PLAYER(getOwnerINLINE());

	// merge groups, but make sure we do not change the head unit AI
	// this means that if a new unit is going to become the head, change its AI to match, if possible
	// AI_setUnitAIType removes the unit from the current group (at least currently), so we have to be careful in the loop here
	// so, loop until we have not changed unit AIs
	bool bChangedUnitAI;
	do
	{
		bChangedUnitAI = false;

		// loop over all the units, moving them to the new group, 
		// stopping if we had to change a unit AI, because doing so removes that unit from our group, so we have to start over
		CLLNode<IDInfo>* pUnitNode = headUnitNode();
		while (pUnitNode != NULL && !bChangedUnitAI)
		{
			CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
			pUnitNode = nextUnitNode(pUnitNode);
			
			if (pLoopUnit != NULL)
			{
				UnitAITypes eUnitAI = pLoopUnit->AI_getUnitAIType();

				// if the unitAIs are different, and the loop unit has a higher val, then the group unitAI would change
				// change this UnitAI to the old group UnitAI if possible
				CvUnit* pNewHeadUnit = pSelectionGroup->getHeadUnit();
				UnitAITypes eNewHeadUnitAI = pSelectionGroup->getHeadUnitAI();
				if (pNewHeadUnit!= NULL && eUnitAI != eNewHeadUnitAI && pLoopUnit->AI_groupFirstVal() > pNewHeadUnit->AI_groupFirstVal())
				{
					// non-zero AI_unitValue means that this UnitAI is valid for this unit (that is the check used everywhere)
					if (kPlayer.AI_unitValue(pLoopUnit->getUnitType(), eNewHeadUnitAI, NULL) > 0)
					{
						// this will remove pLoopUnit from the current group
						pLoopUnit->AI_setUnitAIType(eNewHeadUnitAI);

						bChangedUnitAI = true;
					}
				}

				pLoopUnit->joinGroup(pSelectionGroup);
			}
		}
	}
	while (bChangedUnitAI);
}

// split this group into two new groups, one of iSplitSize, the other the remaining units
// split up each unit AI type as evenly as possible
CvSelectionGroup* CvSelectionGroup::splitGroup(int iSplitSize, CvUnit* pNewHeadUnit, CvSelectionGroup** ppOtherGroup)
{
	FAssertMsg(iSplitSize > 0, "non-positive splitGroup size");
	if (!(iSplitSize > 0))
	{
		return NULL;
	}

	// are we already small enough?
	if (getNumUnits() <= iSplitSize)
	{
		return this;
	}
	
	CLLNode<IDInfo>* pUnitNode = headUnitNode();
	CvUnit* pOldHeadUnit = ::getUnit(pUnitNode->m_data);
	FAssertMsg(pOldHeadUnit != NULL, "non-zero group without head unit");
	if (pOldHeadUnit == NULL)
	{
		return NULL;
	}

	UnitAITypes eOldHeadAI = pOldHeadUnit->AI_getUnitAIType();

	// if pNewHeadUnit NULL, then we will use our current head to head the new split group of target size
	if (pNewHeadUnit == NULL)
	{
		pNewHeadUnit = pOldHeadUnit;
	}

	// the AI of the new head (the remainder will get the AI of the old head)
	// UnitAITypes eNewHeadAI = pNewHeadUnit->AI_getUnitAIType();

	// pRemainderHeadUnit is the head unit of the group that contains the remainder of units 
	CvUnit* pRemainderHeadUnit = NULL;

	// if the new head is not the old head, then make the old head the remainder head
	bool bSplitingHead = (pOldHeadUnit == pNewHeadUnit);
	if (!bSplitingHead)
	{
		pRemainderHeadUnit = pOldHeadUnit;
	}
	
	// try to find remainder head with same AI as head, if we cannot find one, we will split the rest of the group up
	if (pRemainderHeadUnit == NULL)
	{
		// loop over all the units
		pUnitNode = headUnitNode();
		while (pUnitNode != NULL && pRemainderHeadUnit == NULL)
		{
			CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
			pUnitNode = nextUnitNode(pUnitNode);
			
			if (pLoopUnit != NULL && pLoopUnit != pNewHeadUnit)
			{
				UnitAITypes eLoopUnitAI = pLoopUnit->AI_getUnitAIType();
				if (eLoopUnitAI == eOldHeadAI)
				{
					pRemainderHeadUnit = pLoopUnit;
				}
			}
		}
	}
	
	CvSelectionGroup* pSplitGroup = NULL;
	CvSelectionGroup* pRemainderGroup = NULL;
	
	// make the new group for the new head
	pNewHeadUnit->joinGroup(NULL);
	pSplitGroup = pNewHeadUnit->getGroup();
	FAssertMsg(pSplitGroup != NULL, "join resulted in NULL group");

	// make a new group for the remainder, if non-null
	if (pRemainderHeadUnit != NULL)
	{
		pRemainderHeadUnit->joinGroup(NULL);
		pRemainderGroup = pRemainderHeadUnit->getGroup();
		FAssertMsg(pRemainderGroup != NULL, "join resulted in NULL group");
	}

	// loop until this group is empty, trying to move different AI types each time
	
	
	//Exhibit of why i HATE iustus code sometimes
	//unsigned int unitAIBitField = 0;
	//setBit(unitAIBitField, eNewHeadAI);
	
	bool abUnitAIField[NUM_UNITAI_TYPES];
	for (int iI = 0; iI < NUM_UNITAI_TYPES; iI++)
	{
		abUnitAIField[iI] = false;
	}
	
	while (getNumUnits())
	{
		UnitAITypes eTargetUnitAI = NO_UNITAI;
	
		// loop over all the units, find the next different UnitAI and move one of each
		bool bDestinationSplit = (pSplitGroup->getNumUnits() < iSplitSize);
		pUnitNode = headUnitNode();
		while (pUnitNode != NULL)
		{
			CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
			pUnitNode = nextUnitNode(pUnitNode);
			
			if (pLoopUnit != NULL)
			{
				UnitAITypes eLoopUnitAI = pLoopUnit->AI_getUnitAIType();
				
				// if we have not found a new type to move, is this a new unitai?
				// note, if there eventually are unitAIs above 31, we will just always move those, which is fine
				if (eTargetUnitAI == NO_UNITAI && !abUnitAIField[eLoopUnitAI])
				{
					eTargetUnitAI =  eLoopUnitAI;
					abUnitAIField[eLoopUnitAI] = true;
				}
				
				// is this the right UnitAI?
				if (eLoopUnitAI == eTargetUnitAI)
				{
					// move this unit to the appropriate group 
					if (bDestinationSplit)
					{
						pLoopUnit->joinGroup(pSplitGroup);
					}
					else
					{
						pLoopUnit->joinGroup(pRemainderGroup);
						// (if pRemainderGroup NULL, it gets its own group)
						pRemainderGroup = pLoopUnit->getGroup();
					}
					
					// if we moved to remainder, try for next unit AI
					if (!bDestinationSplit)
					{
						eTargetUnitAI = NO_UNITAI;

						bDestinationSplit = (pSplitGroup->getNumUnits() < iSplitSize);
					}
					else
					{
						// next unit goes to the remainder group
						bDestinationSplit = false;
					}
				}
			}

		}
		
		// clear bitfield, all types are valid again
		for (int iI = 0; iI < NUM_UNITAI_TYPES; iI++)
		{
			abUnitAIField[iI] = false;
		}
	}

	FAssertMsg(pSplitGroup->getNumUnits() <= iSplitSize, "somehow our split group is too large");
	
	if (ppOtherGroup != NULL)
	{
		*ppOtherGroup = pRemainderGroup;
	}

	return pSplitGroup;
}

// K-Mod
// If the group has units of different plots, this function will create one new group for each of those plots
void CvSelectionGroup::regroupSeparatedUnits()
{
	const CvUnit* pHeadUnit = getHeadUnit();
	std::vector<CvSelectionGroup*> new_groups;

	CLLNode<IDInfo>* pUnitNode = headUnitNode();
	FAssert(pHeadUnit || !pUnitNode);
	while (pUnitNode)
	{
		CvUnit* pLoopUnit = ::getUnit(pUnitNode->m_data);
		pUnitNode = nextUnitNode(pUnitNode);
		if (pLoopUnit->plot() != pHeadUnit->plot())
		{
			bool bFoundGroup = false;
			for (size_t i = 0; !bFoundGroup && i < new_groups.size(); i++)
			{
				if (pLoopUnit->plot() == new_groups[i]->plot())
				{
					pLoopUnit->joinGroup(new_groups[i], true);
					bFoundGroup = true;
				}
			}
			if (!bFoundGroup)
			{
				pLoopUnit->joinGroup(0, true);
				new_groups.push_back(pLoopUnit->getGroup());
			}
		}
	}
}
// K-Mod end

//------------------------------------------------------------------------------------------------
// FUNCTION:    CvSelectionGroup::getUnitIndex
//! \brief      Returns the index of the given unit in the selection group
//! \param      pUnit The unit to find the index of within the group
//! \retval     The zero-based index of the unit within the group, or -1 if it is not in the group.
//------------------------------------------------------------------------------------------------
int CvSelectionGroup::getUnitIndex(CvUnit* pUnit, int maxIndex /* = -1 */) const
{
	CLLNode<IDInfo>* pUnitNode;
	CvUnit* pLoopUnit;
	int iIndex;

	iIndex = 0;

	pUnitNode = headUnitNode();

	while (pUnitNode != NULL)
	{
		pLoopUnit = ::getUnit(pUnitNode->m_data);
		pUnitNode = nextUnitNode(pUnitNode);

		if (pLoopUnit == pUnit)
		{
			return iIndex;
		}

		iIndex++;

		//early out if not interested beyond maxIndex
		if((maxIndex >= 0) && (iIndex >= maxIndex))
			return -1;
	}

	return -1;
}

CLLNode<IDInfo>* CvSelectionGroup::headUnitNode() const
{
	return m_units.head();
}


CvUnit* CvSelectionGroup::getHeadUnit() const
{
	CLLNode<IDInfo>* pUnitNode = headUnitNode();

	if (pUnitNode != NULL)
	{
		return ::getUnit(pUnitNode->m_data);
	}
	else
	{
		return NULL;
	}
}

CvUnit* CvSelectionGroup::getUnitAt(int index) const
{
	int numUnits = getNumUnits();
	if(index >= numUnits)
	{
		FAssertMsg(false, "[Jason] Selectiongroup unit index out of bounds.");
		return NULL;
	}
	else
	{
		CLLNode<IDInfo>* pUnitNode = headUnitNode();
		for(int i=0;i<index;i++)
			pUnitNode = nextUnitNode(pUnitNode);
		
		CvUnit *pUnit = ::getUnit(pUnitNode->m_data);
		return pUnit;
	}
}


UnitAITypes CvSelectionGroup::getHeadUnitAI() const
{
	CvUnit* pHeadUnit;

	pHeadUnit = getHeadUnit();

	if (pHeadUnit != NULL)
	{
		return pHeadUnit->AI_getUnitAIType();
	}

	return NO_UNITAI;
}


PlayerTypes CvSelectionGroup::getHeadOwner() const
{
	CvUnit* pHeadUnit;

	pHeadUnit = getHeadUnit();

	if (pHeadUnit != NULL)
	{
		return pHeadUnit->getOwnerINLINE();
	}

	return NO_PLAYER;
}


TeamTypes CvSelectionGroup::getHeadTeam() const
{
	CvUnit* pHeadUnit;

	pHeadUnit = getHeadUnit();

	if (pHeadUnit != NULL)
	{
		return pHeadUnit->getTeam();
	}

	return NO_TEAM;
}


void CvSelectionGroup::clearMissionQueue()
{
	FAssert(getOwnerINLINE() != NO_PLAYER);

	deactivateHeadMission();

	m_missionQueue.clear();

	if ((getOwnerINLINE() == GC.getGameINLINE().getActivePlayer()) && IsSelected())
	{
		gDLL->getInterfaceIFace()->setDirty(Waypoints_DIRTY_BIT, true);
		gDLL->getInterfaceIFace()->setDirty(SelectionButtons_DIRTY_BIT, true);
		gDLL->getInterfaceIFace()->setDirty(InfoPane_DIRTY_BIT, true);
	}
}


int CvSelectionGroup::getLengthMissionQueue() const																
{
	return m_missionQueue.getLength();
}


MissionData* CvSelectionGroup::getMissionFromQueue(int iIndex) const
{
	CLLNode<MissionData>* pMissionNode;

	pMissionNode = m_missionQueue.nodeNum(iIndex);

	if (pMissionNode != NULL)
	{
		return &(pMissionNode->m_data);
	}
	else
	{
		return NULL;
	}
}


void CvSelectionGroup::insertAtEndMissionQueue(MissionData mission, bool bStart)
{
	//PROFILE_FUNC();

	FAssert(getOwnerINLINE() != NO_PLAYER);

	m_missionQueue.insertAtEnd(mission);

	if ((getLengthMissionQueue() == 1) && bStart)
	{
		activateHeadMission();
	}

	if ((getOwnerINLINE() == GC.getGameINLINE().getActivePlayer()) && IsSelected())
	{
		gDLL->getInterfaceIFace()->setDirty(Waypoints_DIRTY_BIT, true);
		gDLL->getInterfaceIFace()->setDirty(SelectionButtons_DIRTY_BIT, true);
		gDLL->getInterfaceIFace()->setDirty(InfoPane_DIRTY_BIT, true);
	}
}


CLLNode<MissionData>* CvSelectionGroup::deleteMissionQueueNode(CLLNode<MissionData>* pNode)
{
	CLLNode<MissionData>* pNextMissionNode;

	FAssertMsg(pNode != NULL, "Node is not assigned a valid value");
	FAssert(getOwnerINLINE() != NO_PLAYER);

	if (pNode == headMissionQueueNode())
	{
		deactivateHeadMission();
	}

	pNextMissionNode = m_missionQueue.deleteNode(pNode);

	if (pNextMissionNode == headMissionQueueNode())
	{
		activateHeadMission();
	}

	if ((getOwnerINLINE() == GC.getGameINLINE().getActivePlayer()) && IsSelected())
	{
		gDLL->getInterfaceIFace()->setDirty(Waypoints_DIRTY_BIT, true);
		gDLL->getInterfaceIFace()->setDirty(SelectionButtons_DIRTY_BIT, true);
		gDLL->getInterfaceIFace()->setDirty(InfoPane_DIRTY_BIT, true);
	}

	return pNextMissionNode;
}


CLLNode<MissionData>* CvSelectionGroup::nextMissionQueueNode(CLLNode<MissionData>* pNode) const
{
	return m_missionQueue.next(pNode);
}


CLLNode<MissionData>* CvSelectionGroup::prevMissionQueueNode(CLLNode<MissionData>* pNode) const
{
	return m_missionQueue.prev(pNode);
}


CLLNode<MissionData>* CvSelectionGroup::headMissionQueueNode() const
{
	return m_missionQueue.head();
}


CLLNode<MissionData>* CvSelectionGroup::tailMissionQueueNode() const
{
	return m_missionQueue.tail();
}


int CvSelectionGroup::getMissionType(int iNode) const
{
	int iCount = 0;
	CLLNode<MissionData>* pMissionNode;

	pMissionNode = headMissionQueueNode();

	while (pMissionNode != NULL)
	{
		if ( iNode == iCount )
		{
			return pMissionNode->m_data.eMissionType;
		}

		iCount++;

		pMissionNode = nextMissionQueueNode(pMissionNode);
	}

	return -1;
}


int CvSelectionGroup::getMissionData1(int iNode) const
{
	int iCount = 0;
	CLLNode<MissionData>* pMissionNode;

	pMissionNode = headMissionQueueNode();

	while (pMissionNode != NULL)
	{
		if ( iNode == iCount )
		{
			return pMissionNode->m_data.iData1;
		}

		iCount++;

		pMissionNode = nextMissionQueueNode(pMissionNode);
	}

	return -1;
}


int CvSelectionGroup::getMissionData2(int iNode) const
{
	int iCount = 0;
	CLLNode<MissionData>* pMissionNode;

	pMissionNode = headMissionQueueNode();

	while (pMissionNode != NULL)
	{
		if ( iNode == iCount )
		{
			return pMissionNode->m_data.iData2;
		}

		iCount++;

		pMissionNode = nextMissionQueueNode(pMissionNode);
	}

	return -1;
}


void CvSelectionGroup::read(FDataStreamBase* pStream)
{
	// Init saved data
	reset();

	uint uiFlag=0;
	pStream->Read(&uiFlag);	// flags for expansion

	pStream->Read(&m_iID);
	pStream->Read(&m_iMissionTimer);

	pStream->Read(&m_bForceUpdate);

	pStream->Read((int*)&m_eOwner);
	pStream->Read((int*)&m_eActivityType);
	pStream->Read((int*)&m_eAutomateType);

	m_units.Read(pStream);
	m_missionQueue.Read(pStream);
}


void CvSelectionGroup::write(FDataStreamBase* pStream)
{
	uint uiFlag=0;
	pStream->Write(uiFlag);		// flag for expansion

	pStream->Write(m_iID);
	pStream->Write(m_iMissionTimer);

	pStream->Write(m_bForceUpdate);

	pStream->Write(m_eOwner);
	pStream->Write(m_eActivityType);
	pStream->Write(m_eAutomateType);

	m_units.Write(pStream);
	m_missionQueue.Write(pStream);
}

// Protected Functions...

void CvSelectionGroup::activateHeadMission()
{
	FAssert(getOwnerINLINE() != NO_PLAYER);

	if (headMissionQueueNode() != NULL)
	{
		if (!isBusy())
		{
			startMission();
		}
	}
}


void CvSelectionGroup::deactivateHeadMission()
{
	FAssert(getOwnerINLINE() != NO_PLAYER);

	if (headMissionQueueNode() != NULL)
	{
		if (getActivityType() == ACTIVITY_MISSION)
		{
			setActivityType(ACTIVITY_AWAKE);
		}

		setMissionTimer(0);

		if (getOwnerINLINE() == GC.getGameINLINE().getActivePlayer())
		{
			if (IsSelected())
			{
				GC.getGameINLINE().cycleSelectionGroups_delayed(1, true, canAnyMove());
			}
		}
	}
}
