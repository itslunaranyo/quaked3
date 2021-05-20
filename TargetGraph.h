//==============================
//	TargetGraph.h
//==============================

#ifndef __TARGET_GRAPH_H__
#define __TARGET_GRAPH_H__

#include <vector>

class TargetGraph
{
public:
	TargetGraph() { mark = edgeList.end(); };
	~TargetGraph() {};

	const enum mode {
		tgm_none,
		tgm_selected,
		tgm_selected_path,
		tgm_all
	};

	struct edgeGeo {
		vec3 start, end, color;
	};

	void Refresh(Entity &elist);
	bool YieldEdge(edgeGeo &edge);
private:
	struct edge {
		Entity &from, &to;
		edge(Entity &f, Entity &t) : from(f), to(t) {};
	};
	std::vector<edge> edgeList;
	std::vector<edge>::iterator mark;

	bool FilterByVisibility(Entity *e);
	bool FilterBySelection(Entity *e);
};

#endif // __TARGET_GRAPH_H__