//==============================
//	ManipTool.cpp
//==============================

#include "pre.h"
#include "qe3.h"
#include "ManipTool.h"
#include "map.h"
#include "select.h"
#include "winding.h"
#include "modify.h"
#include "CameraView.h"
#include "XYZView.h"
#include "ZView.h"
#include "WndView.h"

#include "CmdGeoMod.h"
#include "CmdClone.h"
#include "CmdCompound.h"
#include "CmdPlaneShift.h"
#include "CmdTranslate.h"
#include "CmdAddRemove.h"

ManipTool::ManipTool() :
	state(MT_OFF), cloneReady(false),
	brDragNew(nullptr), cmdPS(nullptr), cmdTr(nullptr), cmdGM(nullptr), cmdCmpClone(nullptr),
	lastNudge(nullptr), lastNudgeTime(0),
	Tool("Manipulate", false)	// not modal
{
}

ManipTool::~ManipTool()
{
}

// ----------------------------------------------------------------

bool ManipTool::Input3D(UINT uMsg, WPARAM wParam, LPARAM lParam, CameraView &v, WndView &vWnd)
{
	if (Input(uMsg, wParam, lParam))
		return true;

	int keys = wParam;
	int mx, my;
	mouseContext_t mc;

	switch (uMsg)
	{
	//case WM_COMMAND:
	//	return InputCommand(wParam);
	case WM_LBUTTONDOWN:
		if (ShiftDown() || AltDown())
			return false;
		SetCapture(vWnd.w_hwnd);
		hot = true;

		vWnd.GetMsgXY(lParam, mx, my);
		mc = v.GetMouseContext(mx, my);
		DragStart3D(mc);

		WndMain_UpdateWindows(W_SCENE);
		return true;
	case WM_MOUSEMOVE:
		if (keys & MK_LBUTTON)
		{
			vec3 pt;
			vWnd.GetMsgXY(lParam, mx, my);
			mc = v.GetMouseContext(mx, my);
			mousePlane.TestRay(mc.org, mc.ray, pt);
			DragMove(mc, pt);

			WndMain_UpdateWindows(W_SCENE);
			return true;
		}
		return false;
	case WM_LBUTTONUP:
		if (!hot) return false;

		vWnd.GetMsgXY(lParam, mx, my);
		mc = v.GetMouseContext(mx, my);
		DragFinish();

		WndMain_UpdateWindows(W_SCENE);
		if (!(keys & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON)))
			ReleaseCapture();
		hot = false;
		return true;
	}
	return hot;
}

bool ManipTool::Input2D(UINT uMsg, WPARAM wParam, LPARAM lParam, XYZView &v, WndView &vWnd)
{
	if (Input(uMsg, wParam, lParam))
		return true;

	int keys = wParam;
	int x, y, mx, my;
	mouseContext_t mc;

	switch (uMsg)
	{
	case WM_COMMAND:
		if (hot) return true;
		switch (LOWORD(wParam)) {
		case ID_NUDGE_UP:
		case ID_NUDGE_DOWN:
		case ID_NUDGE_LEFT:
		case ID_NUDGE_RIGHT:
			mc = v.GetMouseContext(0,0);
			Nudge(LOWORD(wParam), mc.right, mc.up);
			break;
		default:
			return false;
		}
	case WM_LBUTTONDOWN:
		if (ShiftDown() || (!CtrlDown() && AltDown()))
			return false;
		SetCapture(vWnd.w_hwnd);
		hot = true;

		vWnd.GetMsgXY(lParam, mx, my);
		v.ToPoint(mx, my, x, y);
		mc = v.GetMouseContext(mx, my);
		DragStart2D(mc, v.dViewType);
		WndMain_UpdateWindows(W_SCENE);
		return true;

	case WM_MOUSEMOVE:
		if (keys & MK_LBUTTON)
		{
			//assert(hot);	// happens after a context menu click
			vWnd.GetMsgXY(lParam, mx, my);
			v.ToPoint(mx, my, x, y);
			mc = v.GetMouseContext(mx, my);
			DragMove(mc, mc.org);
			WndMain_UpdateWindows(W_SCENE);
			return true;
		}
		return false;

	case WM_LBUTTONUP:
		if (!hot) return false;
		vWnd.GetMsgXY(lParam, mx, my);
		v.ToPoint(mx, my, x, y);
		mc = v.GetMouseContext(mx, my);
		DragFinish();

		WndMain_UpdateWindows(W_SCENE);
		if (!(keys & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON)))
			ReleaseCapture();
		hot = false;
		return true;
	}
	return hot;
}

bool ManipTool::Input1D(UINT uMsg, WPARAM wParam, LPARAM lParam, ZView &v, WndView &vWnd)
{
	if (Input(uMsg, wParam, lParam))
		return true;

	int keys = wParam;
	int mx, my;
	mouseContext_t mc;

	switch (uMsg)
	{
	//case WM_COMMAND:
	//	return InputCommand(wParam);
	case WM_LBUTTONDOWN:
		if (ShiftDown() || CtrlDown() || AltDown())
			return false;
		SetCapture(vWnd.w_hwnd);
		hot = true;

		// do stuff
		vWnd.GetMsgXY(lParam, mx, my);
		mc = v.GetMouseContext(mx, my);
		DragStart1D(mc);

		WndMain_UpdateWindows(W_SCENE);
		return true;
	case WM_MOUSEMOVE:
		if (keys & MK_LBUTTON)
		{
			vWnd.GetMsgXY(lParam, mx, my);
			mc = v.GetMouseContext(mx, my);
			DragMove(mc, mc.org);

			WndMain_UpdateWindows(W_SCENE);
			return true;
		}
		return false;
	case WM_LBUTTONUP:
		if (!hot) return false;

		// do stuff
		vWnd.GetMsgXY(lParam, mx, my);
		mc = v.GetMouseContext(mx, my);
		DragFinish();

		WndMain_UpdateWindows(W_SCENE);
		if (!(keys & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON)))
			ReleaseCapture();
		hot = false;
		return true;
	}
	return hot;
}

bool ManipTool::Input(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_COMMAND)
	{
		int what = LOWORD(wParam);
		if (LOWORD(wParam) == ID_SELECTION_CLONE && !hot)
		{
			Modify::Clone();
			return true;
		}
		return false;
	}
	if (uMsg == WM_KEYUP)
	{
		if (wParam == VK_SPACE)
		{
			if (g_cfgEditor.CloneStyle == CLONE_DRAG)
			{
				cloneReady = false;
				WndMain_UpdateWindows(W_SCENE);
			}
			return true;
		}
		return false;
	}
	if (state != MT_OFF) return false;

	if (uMsg == WM_KEYDOWN)
	{
		if (wParam == VK_SPACE)
		{
			if (g_cfgEditor.CloneStyle == CLONE_DRAG)
			{
				cloneReady = true;
				WndMain_UpdateWindows(W_SCENE);
			}
			else
				PostMessage(g_hwndMain, WM_COMMAND, ID_SELECTION_CLONE, 0);
			return true;
		}
		return false;
	}


	return false;
}

// ----------------------------------------------------------------

void ManipTool::SelectionChanged()
{
	lastNudge = nullptr;
	lastNudgeTime = 0;
}

// ----------------------------------------------------------------

void ManipTool::DragStart1D(const mouseContext_t &mc)
{
	if (Selection::IsEmpty())
		return;

	ptDown = mc.org;

	std::vector<Face*> fSides, fSidesTest;
	Brush* b;

	// faces selected: always go for a plane drag
	if (Selection::NumFaces())
	{
		fSides = Selection::faces;
	}
	else
	{
		if (!Selection::OnlyBrushEntities())
		{
			SetupTranslate();
			return;
		}
		vec3 ray;
		for (b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
		{
			if (b->IsFiltered())
				continue;

			// check select hit first
			if (mc.org.z < b->maxs.z && mc.org.z > b->mins.z) // select by bounds because that's the outline the user sees
			{
				SetupTranslate();
				return;
			}

			for (Face *f = b->faces; f; f = f->fnext)
			{
				if (f->TestSideSelectAxis(mc.org, XY))
					fSidesTest.push_back(f);
			}
		}

		if (fSidesTest.empty())
			return;		// is this even possible

		// match backfaces to sliding front faces, so brush contacts move with their planes
		for (Brush* b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
		{
			for (Face *f = b->faces; f; f = f->fnext)
			{
				for (auto fsIt = fSidesTest.begin(); fsIt != fSidesTest.end(); ++fsIt)
				{
					if (f->plane.EqualTo(&(*fsIt)->plane, SIDE_ON)) // match front faces too
						fSides.push_back(f);
				}
			}
		}
	}

	if (fSides.empty())
		return;

	state = MT_PLANESHIFT;
	cmdPS = new CmdPlaneShift();
	cmdPS->SetFaces(fSides);
}

void ManipTool::DragStart2D(const mouseContext_t &mc, int vDim)
{
	// no selection: in 2D view, start dragging new brush
	if (Selection::IsEmpty())
	{
		// set state but don't make a new brush until a significant enough mouse move has happened
		ptDown = pointOnGrid(mc.org);
		ptDown[vDim] = 0;
		state = MT_NEW;
		return;
	}

	if (Selection::HasBrushes())
	{
		// ctrl-alt-click to start a translate with an offset, for 'teleporting' the selection
		if (CtrlDown() && AltDown())
		{
			vec3 ptDelta;
			ptDown = Selection::GetMid();
			ptDown[vDim] = mc.org[vDim];
			ptDelta = pointOnGrid(mc.org) - ptDown;

			SetupTranslate();
			StartTranslate();
			cmdTr->Translate(ptDelta);
			return;
		}

		ptDown = mc.org;

		if (!Selection::OnlyBrushEntities())
		{
			SetupTranslate();
			return;
		}
		// translate if click hit the selection, side-detect for auto plane drag otherwise
		trace_t t = Selection::TestRay(mc.org, mc.ray, SF_SELECTED_ONLY);	
			std::vector<Face*> fSides;
			if (CtrlDown())
			{
				if (t.selected)
					fSides.push_back(t.face);
				else
					SideSelectShearFaces(mc.org, mc.ray, fSides);
				StartQuickShear(fSides);
			}
			else
			{
				if (t.selected)
				{
					SetupTranslate();
					return;
				}
				else
				{
					SideSelectFaces(mc.org, mc.ray, fSides);
					StartPlaneShift(fSides);
				}
			}
			return;
	}
	else if (Selection::NumFaces())
	{
		ptDown = mc.org;
		if (CtrlDown())
			//StartQuickShear(Selection::faces);
			return;	// can't shear w/selected faces because CmdGeoMod replaces face pointers,
					// which would have to nuke the selection annoyingly
		else
			StartPlaneShift(Selection::faces);
	}
}

void ManipTool::DragStart3D(const mouseContext_t &mc)
{
	vec3 mpN = CrossProduct(mc.right, mc.up);
	mousePlane.normal = mpN;

	trace_t t;

	// brushes selected: translate if click hit the selection, shear if ctrl-click, side-detect for auto plane drag otherwise
	if (Selection::HasBrushes())
	{
		if (!Selection::OnlyBrushEntities())
		{
			mousePlane.dist = DotProduct(Selection::GetMid(), mpN);
			mousePlane.TestRay(mc.org, mc.ray, ptDown);
			SetupTranslate();
			return;
		}
		std::vector<Face*> fSides;
		t = Selection::TestRay(mc.org, mc.ray, SF_SELECTED_ONLY);
		if (t.selected)
		{
			// we'll trace future mouse events against an implied plane centered on where we hit
			// the selection, so it feels like we're dragging the selection by the point we 'grabbed'
			ptDown = mc.org + mc.ray * t.dist;
			mousePlane.dist = DotProduct(ptDown, mpN);

			if (CtrlDown())
			{
				FrontSelectShearFaces(t.face, fSides);
				StartQuickShear(fSides);
			}
			else
				SetupTranslate();
		}
		else
		{
			mousePlane.dist = DotProduct(Selection::GetMid(), mpN);
			mousePlane.TestRay(mc.org, mc.ray, ptDown);

			if (CtrlDown())
			{
				SideSelectShearFaces(mc.org, mc.ray, fSides);
				StartQuickShear(fSides);
			}
			else
			{
				SideSelectFaces(mc.org, mc.ray, fSides);
				StartPlaneShift(fSides);
			}
		}
		return;
	}
	else if (Selection::NumFaces())
	{
		// faces selected: always go for a plane drag
		t = Selection::TestRay(mc.org, mc.ray, SF_FACES);
		if (t.face && Selection::IsFaceSelected(t.face))
		{
			ptDown = mc.org + mc.ray * t.dist;
			mousePlane.dist = DotProduct(ptDown, mpN);
		}
		else
		{
			mousePlane.dist = DotProduct(Selection::GetMid(), mpN);
			mousePlane.TestRay(mc.org, mc.ray, ptDown);
		}
		if (CtrlDown())
			//StartQuickShear(Selection::faces);
			return;	// can't shear w/selected faces because CmdGeoMod replaces face pointers,
					// which would have to nuke the selection annoyingly
					// user can go into face component mode if they want this
		else
			StartPlaneShift(Selection::faces);
	}
}

void ManipTool::DragMove(const mouseContext_t &mc, vec3 point)
{
	vec3 ptDelta;

	switch (state)
	{
	case MT_NEW:
	{
		vec3 mins, maxs;
		float dMin, dMax;

		dMin = qround(DotProduct(g_qeglobals.d_v3WorkMin, mc.ray), g_qeglobals.d_nGridSize);
		dMax = qround(DotProduct(g_qeglobals.d_v3WorkMax, mc.ray), g_qeglobals.d_nGridSize);
		if (dMin == dMax)
			dMax -= g_qeglobals.d_nGridSize;	// subtract because mc.ray is negative

		// snap the total delta rather than snapping start and end, feels better
		ptDelta = pointOnGrid(mc.org) * (mc.ray + vec3(1));	// null out the extra large third dimension

		mins = dMin * mc.ray + glm::min(ptDown, ptDelta);
		maxs = dMax * mc.ray + glm::max(ptDown, ptDelta);

		if (glm::all(glm::lessThan(mins, maxs)))	// if all min components are < all max components
		{
			if (brDragNew)
				brDragNew->Recreate(mins, maxs, &g_qeglobals.d_workTexDef);
			else
				brDragNew = Brush::Create(mins, maxs, &g_qeglobals.d_workTexDef);
			brDragNew->owner = g_map.world;
			brDragNew->Build();
		}
		else
		{
			// drag doesn't make a valid brush (0 on at least one axis)
			delete brDragNew;
			brDragNew = nullptr;
		}
		return;
	}
	case MT_TRANSLATE:
	case MT_CLONE:
		if (!cmdTr)
			StartTranslate();
		ptDelta = pointOnGrid(point - ptDown);
		cmdTr->Translate(ptDelta);
		return;
	case MT_PLANESHIFT:
		assert(cmdPS);
		ptDelta = pointOnGrid(point - ptDown);
		cmdPS->Translate(ptDelta);
		return;
	case MT_SHEAR:
		assert(cmdGM);
		ptDelta = pointOnGrid(point - ptDown);
		cmdGM->Translate(ptDelta);
		return;
	}
}

void ManipTool::DragFinish()
{
	switch (state)
	{
	case MT_NEW:
	{
		if (!brDragNew)
			break;	// didn't drag a valid brush

		// new brush has been floating outside all lists
		// feed it to a command for undoable addition
		CmdAddRemove *cmdAR = new CmdAddRemove();
		cmdAR->AddedBrush(brDragNew);
		g_cmdQueue.Complete(cmdAR);
		cmdAR->Select();
		brDragNew = nullptr;
		break;
	}
	case MT_CLONE:
		assert(!!cmdCmpClone == !!cmdTr);
		if (!cmdCmpClone && !cmdTr)
			break;
		cmdCmpClone->Complete(cmdTr);
		g_cmdQueue.Complete(cmdCmpClone);
		cmdTr = nullptr;
		cmdCmpClone = nullptr;
		break;
	case MT_TRANSLATE:
		if(!cmdTr)
			break;
		g_cmdQueue.Complete(cmdTr);
		cmdTr = nullptr;
		break;
	case MT_PLANESHIFT:
		assert(cmdPS);
		g_cmdQueue.Complete(cmdPS);
		cmdPS = nullptr;
		break;
	case MT_SHEAR:
		assert(cmdGM);
		g_cmdQueue.Complete(cmdGM);
		cmdGM = nullptr;
		break;
	default:
		break;
	}
	state = MT_OFF;
}

// ----------------------------------------------------------------

// needs a lot more work
// CmdTranslate needs to be modifiable post-do
void ManipTool::Nudge(int nudge, vec3 right, vec3 up)
{
	/*
	if (Selection::IsEmpty()) return;

	SetupTranslate();
	StartTranslate();

	switch (nudge)
	{
	case ID_NUDGE_UP:
		cmdTr->Translate(up * (float)g_qeglobals.d_nGridSize);
		break;
	case ID_NUDGE_DOWN:
		cmdTr->Translate(up * -(float)g_qeglobals.d_nGridSize);
		break;
	case ID_NUDGE_LEFT:
		cmdTr->Translate(right * -(float)g_qeglobals.d_nGridSize);
		break;
	case ID_NUDGE_RIGHT:
		cmdTr->Translate(right * (float)g_qeglobals.d_nGridSize);
		break;
	}

	DragFinish();
	*/
}



void ManipTool::StartQuickShear(std::vector<Face*> &fSides)
{
	assert(!cmdGM);
	if (fSides.empty())
		return;
	cmdGM = new CmdGeoMod();
	std::vector<vec3> points;
	std::vector<Brush*> brList;

	// we can't rely on the list of face pointers, because as soon as the
	// CmdGeoMod gets a hold of them they'll be duped and replaced and
	// the faces we're pointing to will be in 'undo space' without windings
	std::vector<winding_t*> fSideWindings;
	fSideWindings.reserve(fSides.size());
	for (auto fIt = fSides.begin(); fIt != fSides.end(); ++fIt)
		fSideWindings.push_back((*fIt)->face_winding);

	if (Selection::HasBrushes())
	{
		cmdGM->SetBrushes(&g_brSelectedBrushes);
	}
	else
	{
		for (auto fIt = fSides.begin(); fIt != fSides.end(); ++fIt)
		{
			// face list is probably sorted by brush, but might not be, so verify but in reverse
			// so we at least pick up the most recently added brush first
			if (std::find(brList.rbegin(), brList.rend(), (*fIt)->owner) == brList.rend())
				brList.push_back((*fIt)->owner);
		}
		cmdGM->SetBrushes(brList);
	}
	
	auto wIt = fSideWindings.begin();
	for (auto fIt = fSides.begin(); fIt != fSides.end(); ++fIt)
	{
		points.clear();
		for (int i = 0; i < (*fIt)->face_winding->numpoints; i++)
			points.push_back((*fIt)->face_winding->points[i].point);
		cmdGM->SetPoints((*fIt)->owner, points);
		++wIt;
	}
	state = MT_SHEAR;
}

void ManipTool::StartPlaneShift(std::vector<Face*> &fSides)
{
	assert(!cmdPS);
	if (fSides.empty())
		return;
	cmdPS = new CmdPlaneShift();
	cmdPS->SetFaces(fSides);
	state = MT_PLANESHIFT;
}

void ManipTool::SetupTranslate()
{
	assert(!cmdTr);
	if (g_cfgEditor.CloneStyle == CLONE_DRAG && GetKeyState(VK_SPACE) < 0)
		state = MT_CLONE;
	else
		state = MT_TRANSLATE;
}

void ManipTool::StartTranslate()
{
	if (state == MT_CLONE)
	{
		assert(!cmdCmpClone);
		cmdCmpClone = new CmdCompound("Clone Drag");
		CmdClone *cmdCl = new CmdClone(&g_brSelectedBrushes);
		Selection::DeselectAll();
		cmdCmpClone->Complete(cmdCl);
	}
	cmdTr = new CmdTranslate();
	cmdTr->UseBrushes(&g_brSelectedBrushes);
	cmdTr->TextureLock(g_qeglobals.d_bTextureLock);
}

void ManipTool::SideSelectFaces(const vec3 org, const vec3 ray, std::vector<Face*> &fSides)
{
	for (Brush* b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
	{
		if (b->owner->IsPoint())
			continue;
		for (Face *f = b->faces; f; f = f->fnext)
		{
			if (f->TestSideSelect(org, ray))
				fSides.push_back(f);
		}
	}

	SideSelectBackFaces(fSides);
}

void ManipTool::SideSelectShearFaces(const vec3 org, const vec3 ray, std::vector<Face*> &fSides)
{
	for (Brush* b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
	{
		if (b->owner->IsPoint())
			continue;
		for (Face *f = b->faces; f; f = f->fnext)
		{
			if (f->TestSideSelect(org, ray))
				fSides.push_back(f);
		}
	}
}

void ManipTool::SideSelectBackFaces(std::vector<Face*> &fSides)
{
	if (fSides.empty())
		return;

	std::vector<Face*> backSides;
	// match backfaces to sliding front faces, so brush contacts move with their planes
	for (Brush* b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
	{
		if (b->owner->IsPoint())
			continue;
		for (Face *f = b->faces; f; f = f->fnext)
		{
			for (auto fsIt = fSides.begin(); fsIt != fSides.end(); ++fsIt)
			{
				if (f->plane.EqualTo(&(*fsIt)->plane, true))
					backSides.push_back(f);
			}
		}
	}
	fSides.insert(fSides.end(), backSides.begin(), backSides.end());
}

void ManipTool::FrontSelectShearFaces(const Face *hit, std::vector<Face*> &fSides)
{
	// match backfaces to sliding front faces, so brush contacts move with their planes
	for (Brush* b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
	{
		for (Face *f = b->faces; f; f = f->fnext)
		{
			if (f->plane.EqualTo(&hit->plane, false))
				fSides.push_back(f);
		}
	}
}

// ----------------------------------------------------------------

bool ManipTool::Draw3D(CameraView &v)
{
	if (cloneReady)
	{
		if (cmdTr && cmdTr->postDrag)
		{
			glTranslatef(cmdTr->trans[0], cmdTr->trans[1], cmdTr->trans[2]);
			v.DrawSelected(&g_brSelectedBrushes, g_colors.tool);
			glTranslatef(-cmdTr->trans[0], -cmdTr->trans[1], -cmdTr->trans[2]);
		}
		else
			v.DrawSelected(&g_brSelectedBrushes, g_colors.tool);
		return true;
	}

	switch (state)
	{
	case MT_NEW:
		if (!brDragNew)
			break;

		glMatrixMode(GL_PROJECTION);

		brDragNew->Draw();

		// redraw tint on top
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glEnable(GL_BLEND);

		// lunaran: brighten & clarify selection tint, use selection color preference
		glColor4f(g_colors.selection[0],
			g_colors.selection[1],
			g_colors.selection[2],
			0.3f);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		glDisable(GL_TEXTURE_2D);
		for (Face *face = brDragNew->faces; face; face = face->fnext)
			face->Draw();
		
		// non-zbuffered outline
		glDisable(GL_BLEND);
		glDisable(GL_DEPTH_TEST);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glColor3f(1, 1, 1);

		for (Face *face = brDragNew->faces; face; face = face->fnext)
			face->Draw();
		
		return true;

	case MT_TRANSLATE:
	case MT_CLONE:
		if (!cmdTr)
			return false;
		if (!cmdTr->postDrag)
			return false;
		glTranslatef(cmdTr->trans[0], cmdTr->trans[1], cmdTr->trans[2]);
		v.DrawSelected(&g_brSelectedBrushes, g_colors.selection);
		glTranslatef(-cmdTr->trans[0], -cmdTr->trans[1], -cmdTr->trans[2]);
		return true;

	case MT_PLANESHIFT:
	default:
		break;
	}
	return false;
}

bool ManipTool::Draw2D(XYZView &v)
{
	if (cloneReady)
	{
		if (cmdTr && cmdTr->postDrag)
		{
			glTranslatef(cmdTr->trans[0], cmdTr->trans[1], cmdTr->trans[2]);
			v.DrawSelection(g_colors.tool);
			glTranslatef(-cmdTr->trans[0], -cmdTr->trans[1], -cmdTr->trans[2]);
		}
		else
			v.DrawSelection(g_colors.tool);
		return true;
	}

	switch (state)
	{
	case MT_NEW:
		if (!brDragNew)
			break;

		v.BeginDrawSelection(g_colors.selection);
		brDragNew->DrawXY(v.GetAxis());
		v.EndDrawSelection();
		v.DrawSizeInfo(brDragNew->mins, brDragNew->maxs);
		return true;

	case MT_TRANSLATE:
	case MT_CLONE:
		if (!cmdTr || !cmdTr->postDrag)
			return false;
		glTranslatef(cmdTr->trans[0], cmdTr->trans[1], cmdTr->trans[2]);
		v.DrawSelection(g_colors.selection);
		glTranslatef(-cmdTr->trans[0], -cmdTr->trans[1], -cmdTr->trans[2]);
		return true;

	case MT_PLANESHIFT:
	default:
		return false;
	}
	return false;
}

bool ManipTool::Draw1D(ZView &v)
{
	if (cloneReady)
	{
		if (cmdTr && cmdTr->postDrag)
		{
			glTranslatef(0, cmdTr->trans[2], 0);
			v.DrawSelection(g_colors.tool);
			glTranslatef(0, -cmdTr->trans[2], 0);
		}
		else
			v.DrawSelection(g_colors.tool);
		return true;
	}
	switch (state)
	{
	case MT_NEW:
		if (!brDragNew)
			break;

		v.DrawBrush(brDragNew, g_colors.selection);
		return true;
	case MT_TRANSLATE:
	case MT_CLONE:
		if (!cmdTr || !cmdTr->postDrag)
			return false;
		glTranslatef(0, cmdTr->trans[2], 0);
		v.DrawSelection(g_colors.selection);
		glTranslatef(0, -cmdTr->trans[2], 0);
		return true;
	default:
		return false;
	}
	return false;
}

