#include "CvDLLFAStarIFaceBase.h"
#include <hash_map>
#include <boost/shared_ptr.hpp>

/*struct KmodPathNode;
struct KmodPathNode
{
	int turns;
	int moves;
	int total_cost;
	int known_cost;
	int heuristic_cost;

	bool bFinal;

	KModPathNode* parent_node;
};*/

class FAStarNode;

class KmodPathFinder
{
	void OpenNextInQueue();
	typedef stdext::hash_map<int, boost::shared_ptr<FAStarNode> > NodeMap_t;
	typedef std::vector<std::pair<int, boost::shared_ptr<FAStarNode> > > OpenList_t;

	NodeMap_t node_map;
	OpenList_t open_list;

	int dest_x, dest_y;
	int start_x, start_y;
};
