//==============================
//	TargetGraph.cpp
//==============================

#include "qe3.h"


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
bool TargetGraph::FilterByVisibility(Entity *e)
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


bool TargetGraph::FilterBySelection(Entity *e)
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
		edge.start = mark->from.GetCenter();
		edge.end = mark->to.GetCenter();
		edge.color = mark->from.eclass->color;

		++mark;
		return true;
	}

	mark = edgeList.begin();
	return false;
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

TODO: is calling IsEntitySelected() so many redundant times worth caching
	a selected entity list instead?
==================
*/
void TargetGraph::Refresh(Entity &elist)
{
	if (!g_cfgUI.PathlineMode)
		return;

	bool srcIsPath, dstIsPath;
	bool srcOK, dstOK;

	edgeList.clear();
	srcOK = dstOK = false;

	for (Entity *es = elist.next; es != &elist; es = es->next)
	{
		if (FilterByVisibility(es))
			continue;

		srcOK = !FilterBySelection(es);
		srcIsPath = ((es->eclass->showFlags & EFL_PATH) != 0);

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

				if (!srcOK)	// don't check twice if you don't have to
					dstOK = !FilterBySelection(ed);
				dstIsPath = ((ed->eclass->showFlags & EFL_PATH) != 0);

				for (EPair *dest = ed->epairs; dest; dest = dest->next)
				{
					if (!dest->IsTargetName())
						continue;

					if (!strcmp((char*)*src->value, (char*)*dest->value))
					{
						if (srcOK || dstOK ||
							((g_cfgUI.PathlineMode == TargetGraph::tgm_selected_path) && srcIsPath && dstIsPath))
							edgeList.emplace_back(*es, *ed);
					}
				}
			}
		}
	}
	mark = edgeList.begin();
}
