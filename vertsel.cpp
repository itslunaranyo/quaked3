//==============================
//	vertsel.c
//==============================

#include "qe3.h"


/*
============
FindPoint
============
*/
int	FindPoint (vec3_t point)
{
	int	i, j;

	for (i = 0; i < g_qeglobals.d_nNumPoints; i++)
	{
		for (j = 0; j < 3; j++)
			if (fabs(point[j] - g_qeglobals.d_v3Points[i][j]) > 0.1)
				break;

		if (j == 3)
			return i;
	}

	VectorCopy(point, g_qeglobals.d_v3Points[g_qeglobals.d_nNumPoints]);
	g_qeglobals.d_nNumPoints++;

	return g_qeglobals.d_nNumPoints - 1;
}

/*
============
FindEdge
============
*/
int FindEdge (int p1, int p2, Face *f)
{
	int i;

	for (i = 0; i < g_qeglobals.d_nNumEdges; i++)
	{
		if (g_qeglobals.d_pEdges[i].p1 == p2 && g_qeglobals.d_pEdges[i].p2 == p1)
		{
			g_qeglobals.d_pEdges[i].f2 = f;
			return i;
		}
	}

	g_qeglobals.d_pEdges[g_qeglobals.d_nNumEdges].p1 = p1;
	g_qeglobals.d_pEdges[g_qeglobals.d_nNumEdges].p2 = p2;
	g_qeglobals.d_pEdges[g_qeglobals.d_nNumEdges].f1 = f;
	g_qeglobals.d_nNumEdges++;

	return g_qeglobals.d_nNumEdges - 1;
}

/*
============
MakeFace
============
*/
void MakeFace (Face *f)
{
	int			i;
	int			pnum[128];
	winding_t  *w;

	w = g_brSelectedBrushes.next->MakeFaceWinding(f);
	if (!w)
		return;
	for (i = 0; i < w->numpoints; i++)
		pnum[i] = FindPoint(w->points[i]);
	for (i = 0; i < w->numpoints; i++)
		FindEdge(pnum[i], pnum[(i + 1) % w->numpoints], f);

	Winding::Free(w);
}

/*
============
SetupVertexSelection
============
*/
void SetupVertexSelection ()
{
	Face	*f;
	Brush *b;

	g_qeglobals.d_nNumPoints = 0;
	g_qeglobals.d_nNumEdges = 0;

	if (!QE_SingleBrush())
		return;
	
	b = g_brSelectedBrushes.next;
	
	for (f = b->basis.faces; f; f = f->next)
		MakeFace(f);

	Sys_UpdateWindows(W_ALL);
}

/*
============
SelectFaceEdge
============
*/
void SelectFaceEdge (Face *f, int p1, int p2)
{
	int			i, j, k;
	int			pnum[128];
	winding_t  *w;

	w = g_brSelectedBrushes.next->MakeFaceWinding(f);

	if (!w)
		return;
	
	for (i = 0; i < w->numpoints; i++)
		pnum[i] = FindPoint(w->points[i]);

	for (i = 0; i < w->numpoints; i++)
	{
		if (pnum[i] == p1 && pnum[(i + 1) % w->numpoints] == p2)
		{
			VectorCopy(g_qeglobals.d_v3Points[pnum[i]], f->planepts[0]);
			VectorCopy(g_qeglobals.d_v3Points[pnum[(i + 1) % w->numpoints]], f->planepts[1]);
			VectorCopy(g_qeglobals.d_v3Points[pnum[(i + 2) % w->numpoints]], f->planepts[2]);
			
			for (j = 0; j < 3; j++)
				for (k = 0; k < 3; k++)
					f->planepts[j][k] = floor(f->planepts[j][k] / g_qeglobals.d_nGridSize + 0.5) * g_qeglobals.d_nGridSize;

			AddPlanePoint(f->planepts[0]);
			AddPlanePoint(f->planepts[1]);
			break;
		}
	}

	if (i == w->numpoints)
		Sys_Printf("WARNING: Select face edge failed.\n");

	Winding::Free(w);
}

/*
============
SelectVertex
============
*/
void SelectVertex (int p1)
{
	int			i, j, k;
	Brush	   *b;
	Face	   *f;
	winding_t  *w;

	b = g_brSelectedBrushes.next;
	for (f = b->basis.faces; f; f = f->next)
	{
		w = b->MakeFaceWinding(f);

		if (!w)
			continue;

		for (i = 0; i < w->numpoints; i++)
		{
			if (FindPoint(w->points[i]) == p1)
			{ 
				VectorCopy(w->points[(i + w->numpoints - 1) % w->numpoints], f->planepts[0]);
				VectorCopy(w->points[i], f->planepts[1]);
				VectorCopy(w->points[(i + 1) % w->numpoints], f->planepts[2]);
				
				for (j = 0; j < 3; j++)
					for (k = 0; k < 3; k++)
						f->planepts[j][k] = floor(f->planepts[j][k] / g_qeglobals.d_nGridSize + 0.5) * g_qeglobals.d_nGridSize;

				AddPlanePoint(f->planepts[1]);
				break;
			}
		}
		Winding::Free(w);
	}
}

/*
============
SelectEdgeByRay
============
*/
void SelectEdgeByRay (vec3_t org, vec3_t dir)
{
	int			i, j, besti;
	float		d, bestd;
	vec3_t		mid, temp;
	pedge_t	   *e;

	// find the edge closest to the ray
	besti = -1;
	bestd = 8;

	for (i = 0; i < g_qeglobals.d_nNumEdges; i++)
	{
		for (j = 0; j < 3; j++)
			mid[j] = 0.5 * (g_qeglobals.d_v3Points[g_qeglobals.d_pEdges[i].p1][j] + g_qeglobals.d_v3Points[g_qeglobals.d_pEdges[i].p2][j]);

		VectorSubtract(mid, org, temp);
		d = DotProduct(temp, dir);
		VectorMA(org, d, dir, temp);
		VectorSubtract(mid, temp, temp);
		d = VectorLength(temp);

		if (d < bestd)
		{
			bestd = d;
			besti = i;
		}
	}

	if (besti == -1)
	{
		Sys_Printf("WARNING: Click did not hit an edge.\n");
		return;
	}

	SetCursor(LoadCursor(NULL, IDC_CROSS)); // sikk - Set Cursor to Cross when moving point
//	Sys_Printf("MSG: Hit edge.\n");
	Sys_Printf("CMD: Dragging edge.\n");

	// make the two faces that border the edge use the two edge points
	// as primary drag points
	g_qeglobals.d_nNumMovePoints = 0;
	e = &g_qeglobals.d_pEdges[besti];
	SelectFaceEdge(e->f1, e->p1, e->p2);
	SelectFaceEdge(e->f2, e->p2, e->p1);
}

/*
============
SelectVertexByRay
============
*/
void SelectVertexByRay (vec3_t org, vec3_t dir)
{
	int		i, besti;
	float	d, bestd;
	vec3_t	temp;

	// find the point closest to the ray
	besti = -1;
	bestd = 8;

	for (i = 0; i < g_qeglobals.d_nNumPoints; i++)
	{
		VectorSubtract(g_qeglobals.d_v3Points[i], org, temp);
		d = DotProduct(temp, dir);
		VectorMA(org, d, dir, temp);
		VectorSubtract(g_qeglobals.d_v3Points[i], temp, temp);
		d = VectorLength(temp);

		if (d < bestd)
		{
			bestd = d;
			besti = i;
		}
	}

	if (besti == -1)
	{
		Sys_Printf("WARNING: Click did not hit a vertex.\n");
		return;
	}

	SetCursor(LoadCursor(NULL, IDC_CROSS)); // sikk - Set Cursor to Cross when moving point
//	Sys_Printf("MSG: Hit vertex.\n");
	Sys_Printf("CMD: Dragging vertex.\n");

	// sikk---> Vertex Editing Splits Face
	g_qeglobals.d_fMovePoints[g_qeglobals.d_nNumMovePoints++] = g_qeglobals.d_v3Points[besti];

	// old vertex editing mode
	if (!g_qeglobals.d_savedinfo.bVertexSplitsFace)
		SelectVertex(besti);
// <---sikk
}
