//==============================
//	TargetGraph.h
//==============================

#ifndef __TARGET_GRAPH_H__
#define __TARGET_GRAPH_H__

#include <vector>
class Map;

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

	void Clear() { edgeList.clear(); mark = edgeList.end(); }
	void Refresh(const Entity &elist);
	bool YieldEdge(edgeGeo &edge);
private:
	struct edge {
		Entity *from, *to;
		edge& operator=(const edge &other) { from = other.from; to = other.to; return *this; };
		edge(const edge &other) : from(other.from), to(other.to) {};
		edge(Entity *f, Entity *t) : from(f), to(t) {};
	};
	std::vector<edge> edgeList;
	std::vector<edge>::iterator mark;

	static bool LineFilter(const edge& e);

	bool FilterByVisibility(const Entity *e);
	bool FilterBySelection(const Entity *e);
};

#endif // __TARGET_GRAPH_H__