#pragma once

#include <hash_map>
#include <boost/shared_ptr.hpp>
#include <list>

struct CvPathSettings
{
	CvPathSettings(const CvSelectionGroup* pGroup = 0, int iFlags = 0, int iMaxPath = -1)
		: pGroup(const_cast<CvSelectionGroup*>(pGroup)), iFlags(iFlags), iMaxPath(iMaxPath) { }
	// I'm really sorry about the const_cast. I can't fix the const-correctness of all the relevant functions,
	// because some of them are dllexports. The original code essentially does the same thing anyway, with void* casts.

	CvSelectionGroup* pGroup;
	int iFlags;
	int iMaxPath;
};

class FAStarNode;

class KmodPathFinder
{
public:
	bool GeneratePath(int x1, int y1, int x2, int y2);
	FAStarNode* GetEndNode() { FAssert(end_node); return end_node.get(); }
	void SetSettings(const CvPathSettings& new_settings);
	void SetSettings(const CvSelectionGroup* pGroup, int iFlags = 0, int iMaxPath = -1) { SetSettings(CvPathSettings(pGroup, iFlags, iMaxPath)); }
	void Reset();

protected:
	void AddStartNode();
	void RecalculateHeuristics();
	bool ProcessNode();
	void ForwardPropagate(FAStarNode* head, int cost_delta);
	typedef stdext::hash_map<int, boost::shared_ptr<FAStarNode> > NodeMap_t;
	typedef std::list<boost::shared_ptr<FAStarNode> > OpenList_t;

	struct OpenList_sortPred
	{
		bool operator()(const boost::shared_ptr<FAStarNode> &left, const boost::shared_ptr<FAStarNode> &right);
	};

	NodeMap_t node_map;
	OpenList_t open_list;

	int dest_x, dest_y;
	int start_x, start_y;
	boost::shared_ptr<FAStarNode> end_node;
	CvPathSettings settings;
};
