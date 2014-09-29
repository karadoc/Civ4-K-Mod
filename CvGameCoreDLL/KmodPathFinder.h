#pragma once

#include <boost/multi_array.hpp>
#include <vector>

struct CvPathSettings
{
	CvPathSettings(const CvSelectionGroup* pGroup = 0, int iFlags = 0, int iMaxPath = -1, int iHW = -1);

	CvSelectionGroup* pGroup;
	int iFlags;
	int iMaxPath;
	int iHeuristicWeight;
};

class FAStarNode;

class KmodPathFinder
{
public:
	static void InitHeuristicWeights();
	static int MinimumStepCost(int BaseMoves);

	KmodPathFinder();

	bool ValidateNodeMap(); // Called when SetSettings is used.

	bool GeneratePath(int x1, int y1, int x2, int y2);
	bool GeneratePath(const CvPlot* pToPlot); // just a wrapper for convenience
	FAStarNode* GetEndNode() const { FAssert(end_node); return end_node; } // Note: the returned pointer becomes invalid if the pathfinder is destroyed.
	bool IsPathComplete() const { return end_node; }
	int GetPathTurns() const;
	int GetFinalMoves() const;
	CvPlot* GetPathFirstPlot() const;
	CvPlot* GetPathEndTurnPlot() const;
	void SetSettings(const CvPathSettings& new_settings);
	void SetSettings(const CvSelectionGroup* pGroup, int iFlags = 0, int iMaxPath = -1, int iHW=-1) { SetSettings(CvPathSettings(pGroup, iFlags, iMaxPath, iHW)); }
	void Reset();

protected:
	void AddStartNode();
	void RecalculateHeuristics();
	bool ProcessNode();
	void ForwardPropagate(FAStarNode* head, int cost_delta);
	typedef boost::multi_array<FAStarNode, 2> NodeMap_t;
	typedef std::vector<FAStarNode*> OpenList_t;

	struct OpenList_sortPred
	{
		bool operator()(const FAStarNode* &left, const FAStarNode* &right);
	};

	NodeMap_t node_map;
	OpenList_t open_list;

	int dest_x, dest_y;
	int start_x, start_y;
	FAStarNode* end_node;
	CvPathSettings settings;

	static int admissible_scaled_weight;
	static int admissible_base_weight;
};
