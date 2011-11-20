

#include "CvGameCoreDLL.h"
#include "KmodPathFinder.h"
#include "FAStarNode.h"
#include "CvGameCoreUtils.h"
#include <boost/bind.hpp>
#include <boost/lambda/lambda.hpp>

bool KmodPathFinder::OpenList_sortPred::operator()(const boost::shared_ptr<FAStarNode> &left, const boost::shared_ptr<FAStarNode> &right)
{
	return left->m_iTotalCost < right->m_iTotalCost;
}

bool KmodPathFinder::GeneratePath(int x1, int y1, int x2, int y2)
{
	end_node.reset();

	if (!settings.pGroup)
		return false;

	if (!pathDestValid(x2, y2, &settings, 0))
		return false;

	bool bRecalcHeuristics = false;
	if (x1 != start_x || y1 != start_y)
	{
		// it may be possible to salvage some of the old data.
		// if the moves recorded on the node match the group,
		// just delete everything that isn't a direct descendant of the new start.
		// and then subtract the start cost & moves off all the remaining nodes
		Reset(); // but this is easier.
	}
	if (dest_x != x2 || dest_y != y2)
		bRecalcHeuristics = true;

	// note: even if we have the same starting position, the data might be invalid if our group's moves have changed.
	// .. I might fix that problem later.

	start_x = x1;
	start_y = y1;
	dest_x = x2;
	dest_y = y2;

	{
		int iPlotNum = GC.getMapINLINE().plotNumINLINE(x1, y1);
		FAssert(iPlotNum >= 0 && iPlotNum < GC.getMapINLINE().numPlotsINLINE());
		NodeMap_t::iterator it = node_map.find(iPlotNum);
		if (it == node_map.end())
		{
			// add initial node.
			boost::shared_ptr<FAStarNode> start_node(new FAStarNode);
			start_node->m_iX = x1;
			start_node->m_iY = y1;
			pathAdd(0, start_node.get(), ASNC_INITIALADD, &settings, 0);
			start_node->m_iKnownCost = 0;

			open_list.push_back(start_node);
			node_map[iPlotNum] = start_node;

			bRecalcHeuristics = true;
		}
	}

	if (bRecalcHeuristics)
	{
		// recalculate heuristic cost for all open nodes.
		for (OpenList_t::iterator i = open_list.begin(); i != open_list.end(); ++i)
		{
			int h = pathHeuristic((*i)->m_iX, (*i)->m_iY, dest_x, dest_y);
			(*i)->m_iHeuristicCost = h;
			(*i)->m_iTotalCost = h + (*i)->m_iKnownCost;
		}
	}

	{
		int iEndPlot = GC.getMapINLINE().plotNumINLINE(x2, y2);
		FAssert(iEndPlot >= 0 && iEndPlot < GC.getMapINLINE().numPlotsINLINE());

		NodeMap_t::iterator end_it = node_map.find(iEndPlot);

		if (end_it != node_map.end())
			end_node = end_it->second;
	}

	while (ProcessNode())
	{
		// nothing
	}

	if (end_node && (settings.iMaxPath < 0 || end_node->m_iData2 <= settings.iMaxPath))
		return true;

	return false;
}

void KmodPathFinder::SetSettings(const CvPathSettings& new_settings)
{
	if (settings.pGroup != new_settings.pGroup || settings.iFlags != new_settings.iFlags)
	{
		Reset();
	}
	settings = new_settings;
}

void KmodPathFinder::Reset()
{
	node_map.clear();
	open_list.clear();
	// start & dest don't matter.
	// settings is set separately.
}

bool KmodPathFinder::ProcessNode()
{
	OpenList_t::iterator best_it = open_list.end();
	{
		int iLowestCost = end_node ? end_node->m_iKnownCost : MAX_INT;
		for (OpenList_t::iterator it = open_list.begin(); it != open_list.end(); ++it)
		{
			if ((*it)->m_iTotalCost < iLowestCost &&
				(settings.iMaxPath < 0 || (*it)->m_iData2 <= settings.iMaxPath))
			{
				best_it = it;
				iLowestCost = (*it)->m_iTotalCost;
			}
		}
	}

	// if we didn't find a suitable node to process, then quit.
	if (best_it == open_list.end())
		return false;

	boost::shared_ptr<FAStarNode> parent_node = (*best_it);

	// erase the node from the open_list.
	// Note: this needs to be done before pushing new entries, otherwise the iterator will be invalid.
	open_list.erase(best_it);

	FAssert(node_map[GC.getMapINLINE().plotNumINLINE(parent_node->m_iX, parent_node->m_iY)] == parent_node);

	// open a new node for each direction coming off the chosen node.
	for (int i = 0; i < NUM_DIRECTION_TYPES; i++)
	{
		CvPlot* pAdjacentPlot = plotDirection(parent_node->m_iX, parent_node->m_iY, (DirectionTypes)i);
		if (!pAdjacentPlot)
			continue;

		const int& x = pAdjacentPlot->getX_INLINE(); // convenience
		const int& y = pAdjacentPlot->getY_INLINE(); //

		if (parent_node->m_pParent && parent_node->m_pParent->m_iX == x && parent_node->m_pParent->m_iY == y)
			continue; // no need to backtrack.

		int iPlotNum = GC.getMapINLINE().plotNumINLINE(x, y);
		FAssert(iPlotNum >= 0 && iPlotNum < GC.getMapINLINE().numPlotsINLINE());
		bool bDestination = iPlotNum == GC.getMapINLINE().plotNumINLINE(dest_x, dest_y);

		NodeMap_t::iterator it = node_map.find(iPlotNum);
		bool bNewNode = it == node_map.end();

		boost::shared_ptr<FAStarNode> child_node;

		if (bNewNode)
		{
			child_node.reset(new FAStarNode);
			child_node->m_iX = x;
			child_node->m_iY = y;
			pathAdd(parent_node.get(), child_node.get(), ASNC_NEWADD, &settings, 0);

			child_node->m_iKnownCost = MAX_INT;
			child_node->m_iHeuristicCost = pathHeuristic(x, y, dest_x, dest_y);
			if (pathValid_join(parent_node.get(), child_node.get(), settings.pGroup , settings.iFlags))
			{
				node_map[iPlotNum] = child_node;
				if (pathValid_source(child_node.get(), settings.pGroup , settings.iFlags))
					open_list.push_back(child_node);
			}
			else
			{
				// can't get here.
				child_node.reset();
			}
		}
		else
		{
			if (pathValid_join(parent_node.get(), it->second.get(), settings.pGroup , settings.iFlags))
				child_node = it->second;
		}

		if (!child_node)
			continue;

		FAssert(child_node->m_iX == x && child_node->m_iY == y);
		FAssert(node_map[iPlotNum] == child_node);

		if (bDestination)
			end_node = child_node;

		if (parent_node->m_iKnownCost < child_node->m_iKnownCost)
		{
			int iNewCost = parent_node->m_iKnownCost
				+ pathCost(parent_node.get(), child_node.get(), 666, &settings, 0);
			FAssert(iNewCost > 0);
			if (iNewCost < child_node->m_iKnownCost)
			{
				int cost_delta = iNewCost - child_node->m_iKnownCost; // new minus old. Negative value.

				child_node->m_iKnownCost = iNewCost;
				child_node->m_iTotalCost = child_node->m_iKnownCost + child_node->m_iHeuristicCost;

				// remove child from the list of the previous parent.
				if (child_node->m_pParent)
				{
					FAssert(!bNewNode);
					FAStarNode* x_parent = child_node->m_pParent;
					// x_parent just lost one of its children. We have to break the news to them.
					// this would easier if we had stl instead of bog arrays.
					int temp = x_parent->m_iNumChildren;
					for (int j = 0; j < x_parent->m_iNumChildren; j++)
					{
						if (x_parent->m_apChildren[j] == child_node.get())
						{
							// found it.
							for (j++ ; j < x_parent->m_iNumChildren; j++)
								x_parent->m_apChildren[j-1] = x_parent->m_apChildren[j];
							x_parent->m_apChildren[j-1] = 0; // not necessary, but easy enough to keep things neat.

							x_parent->m_iNumChildren--;
						}
					}
					FAssert(x_parent->m_iNumChildren == temp -1);
				}

				// add child to the list of the new parent
				FAssert(parent_node->m_iNumChildren < NUM_DIRECTION_TYPES);
				parent_node->m_apChildren[parent_node->m_iNumChildren] = child_node.get();
				parent_node->m_iNumChildren++;

				// update the new (reduced) costs for all the grandchildren.
				FAssert(child_node->m_iNumChildren == 0 || !bNewNode);
				ForwardPropagate(child_node.get(), cost_delta);

				child_node->m_pParent = parent_node.get();

				FAssert(child_node->m_iKnownCost > parent_node->m_iKnownCost);
			}
		}
		// else parent has higher cost. So there must already be a faster root to the child.
	}
	return true;
}

void KmodPathFinder::ForwardPropagate(FAStarNode* head, int cost_delta)
{
	FAssert(cost_delta < 0 || head->m_iNumChildren == 0);
	// change the known cost of all children by cost_delta, recursively
	for (int i = 0; i < head->m_iNumChildren; i++)
	{
		head->m_apChildren[i]->m_iKnownCost += cost_delta;
		head->m_apChildren[i]->m_iTotalCost += cost_delta;
		FAssert(head->m_apChildren[i]->m_pParent == head);
		FAssert(head->m_apChildren[i]->m_iKnownCost > head->m_iKnownCost);

		ForwardPropagate(head->m_apChildren[i], cost_delta);
	}
}
