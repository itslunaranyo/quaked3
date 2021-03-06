//==============================
//	GeoTool.cpp
//==============================

#include "pre.h"
#include "qe3.h"
#include "GeoTool.h"
#include "CmdGeoMod.h"

#include "CameraView.h"
#include "CameraRenderer.h"
#include "WndCamera.h"
#include "GridView.h"
#include "GridViewRenderer.h"
#include "WndGrid.h"

#include "DisplayView.h"
#include "select.h"
#include "winding.h"

#include <algorithm>

#define GT_HANDLE_HITSIZE	7

GeoTool::GeoTool(gt_mode_t gtm) :
	mode(gtm), state(GT_NONE),
	cmdGM(nullptr), pointBuf(nullptr), hotView(nullptr),
	Tool("Geometry", true)	// modal
{
	GenerateHandles();
}

GeoTool::~GeoTool()
{
	if (pointBuf)
		delete[] pointBuf;
}

// static
void GeoTool::ToggleMode(gt_mode_t gtm)
{
	GeoTool *gt = dynamic_cast<GeoTool*>(Tool::ModalTool());
	if (!gt)
		new GeoTool(gtm);
	else
	{
		gt->_toggleMode(gtm);
		if (!gt->mode)
			delete gt;
	}
}

void GeoTool::_toggleMode(gt_mode_t gtm)
{
	if (g_cfgEditor.VFEModesExclusive)
	{
		DeselectAllHandles();
		SortHandles();
		mode &= gtm;
	}
	mode ^= gtm;
	if (!mode) return;
	if (g_cfgEditor.VFEModesExclusive)
		return;
	
	if (!(mode & gtm))
	{
		switch (gtm)	// mode was toggled off, deselect any handles of that type
		{
		case GT_VERTEX:
			for (auto hIt = handles.begin(); hIt != handles.end() && hIt->selected; ++hIt)
			{
				if (hIt->numPoints == 1)
					hIt->selected = false;
			}
			break;
		case GT_EDGE:
			for (auto hIt = handles.begin(); hIt != handles.end() && hIt->selected; ++hIt)
			{
				if (hIt->numPoints == 2)
					hIt->selected = false;
			}
			break;
		case GT_FACE:
			for (auto hIt = handles.begin(); hIt != handles.end() && hIt->selected; ++hIt)
			{
				if (hIt->numPoints > 2)
					hIt->selected = false;
			}
			break;
		}
		SortHandles();
	}
}

void GeoTool::SelectionChanged()
{
	GenerateHandles();
}

//==============================

bool GeoTool::Input3D(UINT uMsg, WPARAM wParam, LPARAM lParam, CameraView &v, WndCamera &vWnd)
{
	int keys = wParam;
	int mx, my;
	mouseContext_t mc, mca, mcb;

	switch (uMsg)
	{
		//case WM_COMMAND:
		//	return InputCommand(wParam);
	case WM_LBUTTONDOWN:
		if (ShiftDown())
			return false;

		hot = true;
		hotView = &v;
		vWnd.GetMsgXY(lParam, mx, my);
		mcCurrent = mcDown = v.GetMouseContext(mx, my);
		mca = v.GetMouseContext(mcDown.pt[0] - GT_HANDLE_HITSIZE, mcDown.pt[1] - GT_HANDLE_HITSIZE);
		mcb = v.GetMouseContext(mcDown.pt[0] + GT_HANDLE_HITSIZE, mcDown.pt[1] + GT_HANDLE_HITSIZE);
		DragStart(mca, mcb);
		SetCapture(vWnd.wHwnd);

		WndMain_UpdateWindows(W_SCENE);
		return true;
	case WM_MOUSEMOVE:
		vWnd.GetMsgXY(lParam, mx, my);
		mc = v.GetMouseContext(mx, my);
		if (keys & MK_LBUTTON && hot)
		{
			DragMove(mc);

			WndMain_UpdateWindows(W_SCENE);
			return true;
		}
		mca = v.GetMouseContext(mc.pt[0] - GT_HANDLE_HITSIZE, mc.pt[1] - GT_HANDLE_HITSIZE);
		mcb = v.GetMouseContext(mc.pt[0] + GT_HANDLE_HITSIZE, mc.pt[1] + GT_HANDLE_HITSIZE);
		{
			vec3 up, right, junk;
			v.GetBasis(right, up, junk);
			Hover(mca, mcb, up, right);
		}

		return false;
	case WM_LBUTTONUP:
		if (!hot) return false;

		vWnd.GetMsgXY(lParam, mx, my);
		mc = v.GetMouseContext(mx, my);
		DragFinish(mc);

		WndMain_UpdateWindows(W_SCENE);
		if (!(keys & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON)))
			ReleaseCapture();
		hot = false;
		hotView = nullptr;
		return true;
	}
	return hot;
}

bool GeoTool::Input2D(UINT uMsg, WPARAM wParam, LPARAM lParam, GridView &v, WndGrid &vWnd)
{
	int keys = wParam;
	int mx, my;
	mouseContext_t mc, mca, mcb;
	vec3 up, right, f;

	switch (uMsg)
	{
	case WM_LBUTTONDOWN:
		if (ShiftDown())
			return false;

		hot = true;
		hotView = &v;
		SetCapture(vWnd.wHwnd);
		vWnd.GetMsgXY(lParam, mx, my);
		mcCurrent = mcDown = v.GetMouseContext(mx, my);
		mca = v.GetMouseContext(mcDown.pt[0] - GT_HANDLE_HITSIZE, mcDown.pt[1] - GT_HANDLE_HITSIZE);
		mcb = v.GetMouseContext(mcDown.pt[0] + GT_HANDLE_HITSIZE, mcDown.pt[1] + GT_HANDLE_HITSIZE);
		DragStart(mca, mcb);

		WndMain_UpdateWindows(W_SCENE);
		return true;
	case WM_MOUSEMOVE:
		vWnd.GetMsgXY(lParam, mx, my);
		mc = v.GetMouseContext(mx, my);
		if (keys & MK_LBUTTON && hot)
		{
			DragMove(mc);

			WndMain_UpdateWindows(W_SCENE);
			return true;
		}
		mca = v.GetMouseContext(mc.pt[0] - GT_HANDLE_HITSIZE, mc.pt[1] - GT_HANDLE_HITSIZE);
		mcb = v.GetMouseContext(mc.pt[0] + GT_HANDLE_HITSIZE, mc.pt[1] + GT_HANDLE_HITSIZE);
		v.GetBasis(right, up, f);
		Hover(mca, mcb, up, right);
		return false;
	case WM_LBUTTONUP:
		if (!hot) return false;

		vWnd.GetMsgXY(lParam, mx, my);
		mc = v.GetMouseContext(mx, my);
		DragFinish(mc);

		WndMain_UpdateWindows(W_SCENE);
		if (!(keys & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON)))
			ReleaseCapture();
		hot = false;
		hotView = nullptr;
		return true;
	}
	return hot;
}

bool GeoTool::Input(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return hot;
}

//==============================

/*
single click on handle:
	select (replace)
ctrl-click on handle:
	select (add/remove)
click and drag on handle:
	if selected, move selection
	if unselected, select (replace) and move
	if alt: change up/down axis to in/out
single click not on handle:
	deselect handles
click and drag not on handle:
	box select (replace)
ctrl click and drag not on handle:
	box select (add)
shift click:
	still brush select/deselect
*/

void GeoTool::DragStart(const mouseContext_t &mca, const mouseContext_t &mcb)
{
	vec3 up, right, junk;
	bool needSort = false;
	handlesHit.clear();

	trans = vec3(0);


	// check if we hit any handles directly
	hotView->GetBasis(right, up, junk);
	if (!BoxTestHandles(mca.org, mca.ray, mcb.org, mcb.ray, up, right, handlesHit))
	{
		// start dragging a selection box
		state = GT_BOXSEL;
		return;
	}

	// ctrl click on a handle is select/deselect only
	if (CtrlDown())
	{
		if (AllSelected(handlesHit))
			DeselectHandles(handlesHit);
		else
			SelectHandles(handlesHit);
		SortHandles();
		return;
	}

	vec3 closest;
	float dot, bestdot;
	bestdot = 999999;
	for (auto hIt = handlesHit.begin(); hIt != handlesHit.end(); ++hIt)
	{
		dot = glm::dot((*hIt)->origin, mca.ray);
		if (dot < bestdot)
		{
			bestdot = dot;
			closest = (*hIt)->origin;
		}
	}
	mousePlane.FromNormPoint(glm::cross(mca.up, mca.right), closest);
	ptDownWorld = closest;
	snapTrans = (pointOnGrid(ptDownWorld) - ptDownWorld) * (mca.up + mca.right);

	if (!AnySelected(handlesHit))
	{
		DeselectAllHandles();
		SelectHandles(handlesHit);
		needSort = true;
	}

	if (AnySelected(handlesHit))
		state = GT_MOVE;

	if (needSort)	// ruins the pointers in handlesHit, must come last
	{
		SortHandles();
		handlesHit.clear();
	}
}

void GeoTool::DragMove(const mouseContext_t &mc)
{
	if (state == GT_NONE)
		return;

	vec3 pt, ptDelta, trDelta;
	mousePlane.TestRay(mc.org, mc.ray, pt);

	if (state == GT_BOXSEL)
	{
		// remember whatever corner this is so we can draw a selection box someday
		mcCurrent = mc;
	}
	else if (state == GT_MOVE)
	{
		ptDelta = pointOnGrid(pt - ptDownWorld) + snapTrans;
		if (!cmdGM && !glm::all(glm::equal(ptDelta, vec3(0))))
		{
			cmdGM = new CmdGeoMod();
			cmdGM->SetBrushes(&g_brSelectedBrushes);
			for (auto hIt = handles.begin(); hIt != handles.end() && hIt->selected; ++hIt)
			{
				for (int i = 0; i < hIt->numPoints; i++)
					cmdGM->SetPoint(hIt->points[i], hIt->brAffected);
			}
		}
		if (cmdGM)
		{
			if (cmdGM->Translate(ptDelta))
			{
				trDelta = ptDelta - trans;
				trans = ptDelta;
				for (auto hIt = handles.begin(); hIt != handles.end() && hIt->selected; ++hIt)
				{
					hIt->origin += trDelta;
				//	for (int i = 0; i < 3; i++)
				//	{
				//		if (trDelta[i] != 0)
				//			hIt->origin[i] = qround(hIt->origin[i], g_qeglobals.d_nGridSize);
				//	}
				}
			}
		}
	}
}

void GeoTool::DragFinish(const mouseContext_t &mc)
{
	if (state == GT_NONE)
		return;

	// check if mouse moved significantly since mousedown
	if (fabs(mc.pt[0] - mcDown.pt[0]) > 2 || fabs(mc.pt[1] - mcDown.pt[1]) > 2 || cmdGM)
	{
		// if so, consider it a drag
		if (state == GT_BOXSEL)
		{
			vec3 up, right, junk;
			std::vector<handle*> handlesBoxed;
			hotView->GetBasis(right, up, junk);
			if (BoxTestHandles(mc.org, mc.ray, mcDown.org, mcDown.ray, up, right, handlesBoxed))
				DoSelect(handlesBoxed);
		}
		else if (state == GT_MOVE)
		{
			// complete translate
			if (cmdGM)
			{
				g_cmdQueue.Complete(cmdGM);
				cmdGM = nullptr;
				GenerateHandles();
			}
		}
	}
	else	// consider it a click
	{
		if (state == GT_MOVE && !handlesHit.empty())
		{
			// we hit some handles but didn't move the mouse, count this as a selection
			DeselectAllHandles();
			SelectHandles(handlesHit);
			SortHandles();
		}
		else if (state == GT_BOXSEL)
		{
			// we didn't hit a point on mdown, so no significant movement means we still haven't
			if (!CtrlDown())
			{
				DeselectAllHandles();
				SortHandles();
			}
		}
	}
	assert(!cmdGM);
	ptDownWorld = vec3(0);
	trans = vec3(0);
	snapTrans = vec3(0);
	state = GT_NONE;
}

void GeoTool::Hover(const mouseContext_t &mca, const mouseContext_t &mcb, const vec3 up, const vec3 right)
{
	if (BoxTestHandles(mca.org, mca.ray, mcb.org, mcb.ray, up, right, handlesHit))
		Crosshair(true);
	else
		Crosshair(false);
}

//==============================

void GeoTool::DoSelect(std::vector<handle*>& hlist)
{
	if (CtrlDown())
	{
		if (AllSelected(hlist))
			DeselectHandles(hlist);
		else
			SelectHandles(hlist);
	}
	else
	{
		DeselectAllHandles();
		SelectHandles(hlist);
	}
	SortHandles();
}

bool GeoTool::BoxTestHandles(const vec3 org1, const vec3 dir1, const vec3 org2, const vec3 dir2, const vec3 up, const vec3 right, std::vector<handle*> &hlist)
{
	bool found = false;
	float xf, yf;
	vec3 vpn;
	Plane p[5];
	if (dir1 == dir2)	// orthographic
	{
		vpn = dir1;
		vec3 center = (org1 + org2) / 2.0f;
		p[0].FromNormPoint(dir1, org1);	// near extent of map bounds

		xf = (glm::dot(org1, right) > glm::dot(org2, right)) ? -1 : 1;
		yf = (glm::dot(org1, up) > glm::dot(org2, up)) ? 1 : -1;
	}
	else if (org1 == org2)	// perspective
	{
		// we have to use the camera's vpn/up/right here so that a selection box in the corner of
		// the window turns into a correctly skewed frustum
		vpn = glm::normalize(glm::cross(up, right));
		p[0].FromNormPoint(vpn, org1);	// view plane

		xf = (glm::dot(dir1, right) > glm::dot(dir2, right)) ? -1 : 1;
		yf = (glm::dot(dir1, up) > glm::dot(dir2, up)) ? 1 : -1;
	}
	else assert(0);		// madness

	// project the box's four edges as four planes, normals pointing in
	p[1].FromNormPoint(xf * glm::normalize(glm::cross(dir1, up)), org1);
	p[2].FromNormPoint(xf * glm::normalize(glm::cross(up, dir2)), org2);
	p[3].FromNormPoint(yf * glm::normalize(glm::cross(dir1, right)), org1);
	p[4].FromNormPoint(yf * glm::normalize(glm::cross(right, dir2)), org2);

	for (auto hIt = handles.begin(); hIt != handles.end(); ++hIt)
	{
		if (!(mode & GT_VERTEX) && hIt->numPoints == 1 ||
			!(mode & GT_EDGE) && hIt->numPoints == 2 ||
			!(mode & GT_FACE) && hIt->numPoints > 2)
			continue;
		if (DotProduct(hIt->origin, vec3(p[0].normal)) < p[0].dist) continue;
		if (DotProduct(hIt->origin, vec3(p[1].normal)) < p[1].dist) continue;
		if (DotProduct(hIt->origin, vec3(p[2].normal)) < p[2].dist) continue;
		if (DotProduct(hIt->origin, vec3(p[3].normal)) < p[3].dist) continue;
		if (DotProduct(hIt->origin, vec3(p[4].normal)) < p[4].dist) continue;

		hlist.push_back(&*hIt);
		found = true;
	}

	return found;
}

void GeoTool::SelectHandles(std::vector<handle*>& hlist)
{
	for (auto hIt = hlist.begin(); hIt != hlist.end(); ++hIt)
	{
		if (mode & GT_VERTEX && (*hIt)->numPoints == 1 ||
			mode & GT_EDGE && (*hIt)->numPoints == 2 ||
			mode & GT_FACE && (*hIt)->numPoints > 2 )
			(*hIt)->selected = true;
	}
}

void GeoTool::DeselectHandles(std::vector<handle*>& hlist)
{
	for (auto hIt = hlist.begin(); hIt != hlist.end(); ++hIt)
		(*hIt)->selected = false;
}

bool GeoTool::AllSelected(std::vector<handle*>& hlist)
{
	for (auto hIt = hlist.begin(); hIt != hlist.end(); ++hIt)
	{
		if (!(mode & GT_VERTEX) && (*hIt)->numPoints == 1 ||
			!(mode & GT_EDGE) && (*hIt)->numPoints == 2 ||
			!(mode & GT_FACE) && (*hIt)->numPoints > 2)
			continue;
		if (!(*hIt)->selected)
			return false;
	}
	return true;
}

bool GeoTool::AnySelected(std::vector<handle*>& hlist)
{
	for (auto hIt = hlist.begin(); hIt != hlist.end(); ++hIt)
	{
		if ((*hIt)->selected)
			return true;
	}
	return false;
}

void GeoTool::DeselectAllHandles()
{
	for (auto hIt = handles.begin(); hIt != handles.end(); ++hIt)
		hIt->selected = false;
}


//==============================

void GeoTool::GenerateHandles()
{
	int wPoints, wFaces, i, pIdx;
	vec3 end;
	std::vector<vec3> oldSelection;

	// remember what handles were selected so regenerating the list doesn't appear to lose them
	if (!handles.empty() && handles.front().selected)
	{
		for (auto hIt = handles.begin(); hIt != handles.end() && hIt->selected; ++hIt)
		{
			oldSelection.push_back(hIt->origin);
		}
	}

	handles.clear();

	if (!Selection::HasBrushes())
		return;

	wPoints = 0;
	wFaces = 0;
	for (Brush *b = g_brSelectedBrushes.Next(); b != &g_brSelectedBrushes; b = b->Next())
	{
		if (b->owner->IsPoint())
			continue;
		for (Face* f = b->faces; f; f = f->fnext)
		{
			if (f->HasWinding())
			{
				wPoints += f->GetWinding().Count() + 1;
				wFaces++;
			}
		}
	}

	handles.reserve(wPoints * 2 + wFaces);

	if (pointBuf)
		delete[] pointBuf;
	pointBuf = new vec3[wPoints];

	// stick a handle of each type at the first point it affects
	pIdx = 0;
	for (Brush *b = g_brSelectedBrushes.Next(); b != &g_brSelectedBrushes; b = b->Next())
	{
		if (b->owner->IsPoint())
			continue;
		for (Face* f = b->faces; f; f = f->fnext)
		{
			if (!f->HasWinding())
				continue;

			Winding& w = f->GetWinding();
			end = w[0]->point;
			handles.push_back(handle(w[0]->point, &pointBuf[pIdx], w.Count(), b));
			for (i = 0; i < w.Count(); i++)
			{
				pointBuf[pIdx] = w[i]->point;
				handles.push_back(handle(pointBuf[pIdx], &pointBuf[pIdx], 1, b));
				handles.push_back(handle(pointBuf[pIdx], &pointBuf[pIdx], 2, b));
				pIdx++;
			}
			pointBuf[pIdx++] = end;	// duplicate last point for last edge
		}
	}

	// center all the handles on their affected points
	for (auto hIt = handles.begin(); hIt != handles.end(); ++hIt)
	{
		if (hIt->numPoints > 1)
		{
			vec3 center = vec3(0);
			for (int i = 0; i < hIt->numPoints; i++)
			{
				center += hIt->points[i];
			}
			
			// snap to integers, or else micro float differences keep dupes from collapsing
			hIt->origin = glm::round(center / (float)hIt->numPoints);
		}
	}

	// collapse duplicates to single handles
	CollapseHandles();

	if (!oldSelection.empty())
	{
		for (auto hIt = handles.begin(); hIt != handles.end(); ++hIt)
		{
			if (std::find(oldSelection.begin(), oldSelection.end(), hIt->origin) != oldSelection.end())
				hIt->selected = true;
		}
		SortHandles();
	}
}

void GeoTool::CollapseHandles()
{
	std::vector<handle>::iterator blockStart, blockEnd, mark;

	SortHandles();	// put all exact duplicates adjacent & group by handle type

	blockStart = handles.begin();
	blockEnd = handles.begin();
	mark = handles.begin();

	// collapse all runs of coincident handles to a single handle, and add the parent 
	// brushes of each of those handles to the brush list of the single handle
	while (blockStart != handles.end())
	{
		while (blockEnd != handles.end() && !handlecmp(*blockStart, *blockEnd))
			++blockEnd;

		*mark = *blockStart;
		mark->brAffected.reserve(blockEnd - blockStart + 1);
		while (blockStart != blockEnd)
		{
			mark->brAffected.push_back(blockStart->brSrc);
			++blockStart;
		}
		++mark;
	}
	handles.resize(mark - handles.begin());

	for (auto hIt = handles.begin(); hIt != handles.end(); ++hIt)
		hIt->brAffected.erase(std::unique(hIt->brAffected.begin(), hIt->brAffected.end()), hIt->brAffected.end());
}

bool GeoTool::handlecmp(const handle &a, const handle &b)
{
	// selected handles first, then verts, edges, faces
	int aGrp, bGrp;
	aGrp = b.selected * 64 + a.numPoints;
	bGrp = a.selected * 64 + b.numPoints;

	if (aGrp == bGrp)
		return (VectorCompareLT(a.origin, b.origin));
	return (aGrp < bGrp);
}

void GeoTool::SortHandles()
{
	std::sort(handles.begin(), handles.end(), GeoTool::handlecmp);
}

//==============================

void GeoTool::DrawSelectionBox()
{
	if (state != GT_BOXSEL)
		return;
	if (mcDown.pt == mcCurrent.pt)
		return;

	mat4 mat;

	glGetFloatv(GL_PROJECTION_MATRIX, (GLfloat*)&mat);
	glLoadIdentity();
	glOrtho(0, hotView->GetWidth(), 0, -hotView->GetHeight(), -8, 8);

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	glColor3fv(&g_colors.selection.r);

	glBegin(GL_LINE_LOOP);
	glVertex3f(mcDown.pt.x, -mcDown.pt.y, 0);
	glVertex3f(mcDown.pt.x, -mcCurrent.pt.y, 0);
	glVertex3f(mcCurrent.pt.x, -mcCurrent.pt.y, 0);
	glVertex3f(mcCurrent.pt.x, -mcDown.pt.y, 0);

	glEnd();
	glEnable(GL_TEXTURE_2D);

	glLoadMatrixf((GLfloat*)&mat);
}

void GeoTool::DrawPoints()
{
	std::vector<handle>::iterator sstart, vstart, estart, fstart;

	glDisable(GL_TEXTURE_2D);

	// handles are already sorted by selected, then verts, edges, faces
	// find all the blocks in advance so we can draw the groups in reverse order, so
	// selected draws after (and on top of) verts, verts after edges, etc
	// TODO: properly depth sort the points instead, without clearing existing
	// depth buffer
	sstart = handles.begin();
	vstart = sstart;
	while (vstart != handles.end() && vstart->selected) ++vstart;
	estart = vstart;
	while (estart != handles.end() && estart->numPoints < 2) ++estart;
	fstart = estart;
	while (fstart != handles.end() && fstart->numPoints < 3) ++fstart;

	if (!cmdGM)
	{
		if (mode & GT_FACE)
			DrawPointSet(fstart, handles.end(), vec3(1, 0.5, 0));
		if (mode & GT_EDGE)
			DrawPointSet(estart, fstart, vec3(0, 0.5, 1));
		if (mode & GT_VERTEX)
			DrawPointSet(vstart, estart, vec3(0, 1, 0));
	}
	DrawPointSet(sstart, vstart, vec3(1, 0, 0));

	glPointSize(1);
	glEnable(GL_TEXTURE_2D);
}

void GeoTool::DrawPointSet(std::vector<handle>::iterator &start, std::vector<handle>::iterator &end, vec3 color)
{
	glPointSize(8);
	glColor3f(0,0,0);
	glBegin(GL_POINTS);
	for (auto hIt = start; hIt != end; ++hIt)
		glVertex3fv(&hIt->origin.x);
	glEnd();

	glPointSize(6);
	glColor3f(color.r, color.g, color.b);
	glBegin(GL_POINTS);
	for (auto hIt = start; hIt != end; ++hIt)
		glVertex3fv(&hIt->origin.x);
	glEnd();
}

bool GeoTool::Draw3D(CameraRenderer &rc)
{
	rc.DrawSelected(&g_brSelectedBrushes, g_colors.selection);
	glDisable(GL_DEPTH_TEST);
	DrawPoints();
	if (hotView == rc.GetDisplayView())
		DrawSelectionBox();
	glEnable(GL_DEPTH_TEST);
	return true;
}

bool GeoTool::Draw2D(GridViewRenderer &gr)
{
	gr.DrawSelection(g_colors.selection);
	DrawPoints();
	if (hotView == gr.GetDisplayView())
		DrawSelectionBox();
	return true;
}

