

#include "CvGameCoreDLL.h"
#include "KmodPathFinder.h"
#include "FAStarNode.h"
#include "CvGameCoreUtils.h"

void KmodPathFinder::OpenNextInQueue()
{
	boost::shared_ptr<FAStarNode> node = open_list[0].second;

	for (int i = 0; i < NUM_DIRECTION_TYPES; i++)
	{
		CvPlot* pAdjacentPlot = plotDirection(node->m_iX, node->m_iX, (DirectionTypes)i);
		if (pAdjacentPlot)
		{
			// not the fastest way to get the plot number... but it's good enough for now.
			int iPlotNum = GC.getMapINLINE().plotNumINLINE(pAdjacentPlot->getX_INLINE(), pAdjacentPlot->getY_INLINE());
			NodeMap_t::iterator it = node_map.find(iPlotNum);
			if (it == node_map.end())
			{
				boost::shared_ptr<FAStarNode> new_node(new FAStarNode);
				//pathAdd(node.get() , new_node.get(), int data, const void* pointer, FAStar* finder);
			}
		}
	}
}
