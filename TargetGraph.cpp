//==============================
//	TargetGraph.cpp
//==============================

#include "pre.h"
#include "qe3.h"
#include "map.h"
#include "select.h"
#include <algorithm>

/*
a line is valid
	BOTH ends are visible (not regioned, not filtered)

	in selected mode
		one end must be selected
	in selected/path mode
		one end must be selected OR both ends must be paths

*/



/*
==================
TargetGraph::FilterByVisibility

return true if entity is invisible for one of the several reasons it can be :(
==================
*/
bool TargetGraph::FilterByVisibility(const Entity *e)
{
	if (e->IsFiltered())	// by entity flag
		return true;

	// if any brush is visible, so is the entity
	for (Brush* b = e->brushes.onext; b && b != &e->brushes; b = b->onext)
	{
		if (((g_cfgUI.ViewFilter & b->showFlags) == 0) &&	// by brush flag
			!g_map.IsBrushFiltered(b))	// by region
			return false;
	}

	return true;
}

bool TargetGraph::FilterBySelection(const Entity *e)
{
	if (g_cfgUI.PathlineMode == TargetGraph::tgm_all)
		return false;

	// if any brush is selected in select-filter mode, the entity is selected
	return (!Selection::IsEntitySelected(e));
}


/*
==================
TargetGraph::YieldEdge
==================
*/
bool TargetGraph::YieldEdge(edgeGeo &edge)
{
	if (!g_cfgUI.PathlineMode)
		return false;

	if (edgeList.empty())
		return false;

	while (mark != edgeList.end())
	{
		edge.start = mark->from->GetCenter();
		edge.end = mark->to->GetCenter();
		edge.color = mark->from->eclass->color;

		++mark;
		return true;
	}

	mark = edgeList.begin();
	return false;
}


bool TargetGraph::LineFilter(const edge &e)
{
	if (g_cfgUI.PathlineMode == TargetGraph::tgm_selected_path)
	{
		if (((e.from->eclass->showFlags & e.to->eclass->showFlags) & EFL_PATH) == EFL_PATH)
			return false;
	}
	if (Selection::IsEntitySelected(e.from))
		return false;
	if (Selection::IsEntitySelected(e.to))
		return false;
	return true;
}


/*
==================
TargetGraph::Refresh

tgm_none: no pathlines
tgm_all: show all pathlines, so all sources and destinations are valid
tgm_selected: show pathlines only to or from selected entities (so a line
	is valid if the src OR dest is selected)
tgm_selected_path: show pathlines only to or from a selected entity, or
	between path_corners regardless of selection (so a line is valid if
	the src OR dest is selected, or if the src AND dest are both paths)

FIXME: IsEntitySelected is n^2 complex. building the entire graph (tgm_all)
	is way faster than trying to filter by selection as the graph is built,
	because every node is tested for selected status twice. so, just build
	the whole graph, then use isSelected to throw away graph lines we don't
	need (small set) rather than skip nodes we don't need (large set). 
TODO: is calling IsEntitySelected() so many redundant times worth caching
	a selected entity list instead?
==================
*/
void TargetGraph::Refresh(const Entity &elist)
{
	if (!g_cfgUI.PathlineMode)
		return;

	//bool srcIsPath, dstIsPath;
	//bool srcOK, dstOK;
	//srcOK = dstOK = false;

	edgeList.clear();

	for (Entity *es = elist.next; es != &elist; es = es->next)
	{
		if (FilterByVisibility(es))
			continue;

		//srcOK = !FilterBySelection(es);
		//srcIsPath = ((es->eclass->showFlags & EFL_PATH) != 0);

		for (EPair *src = es->epairs; src; src = src->next)
		{
			if (!src->IsTarget())
				continue;

			for (Entity *ed = elist.next; ed != &elist; ed = ed->next)
			{
				if (es == ed)
					continue;
				if (FilterByVisibility(ed))
					continue;

				//if (!srcOK)	// don't check twice if you don't have to
				//	dstOK = !FilterBySelection(ed);
				//dstIsPath = ((ed->eclass->showFlags & EFL_PATH) != 0);

				for (EPair *dest = ed->epairs; dest; dest = dest->next)
				{
					if (!dest->IsTargetName())
						continue;

					if (!strcmp((char*)*src->value, (char*)*dest->value))
					{
						//if (srcOK || dstOK ||
						//	((g_cfgUI.PathlineMode == TargetGraph::tgm_selected_path) && srcIsPath && dstIsPath))
							edgeList.emplace_back(es, ed);
					}
				}
			}
		}
	}

	// prune lines from the graph based on selection
	// most entities in a map are not targets/targetnames, so the set of connections
	// between nodes is sparse. thus it's faster to build the whole graph and filter
	// by selection after, because IsEntitySelected() is very slow for big selections
	if (g_cfgUI.PathlineMode != TargetGraph::tgm_all)
	{
		edgeList.erase(std::remove_if(edgeList.begin(), edgeList.end(), &TargetGraph::LineFilter), edgeList.end());
	}

	mark = edgeList.begin();
}
