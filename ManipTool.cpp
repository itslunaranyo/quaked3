//==============================
//	ManipTool.cpp
//==============================

#include "qe3.h"

#include "CmdGeoMod.h"
#include "CmdPlaneShift.h"
#include "CmdTranslate.h"
#include "CmdAddRemove.h"

ManipTool::ManipTool() :
	state(MT_OFF),
	brDragNew(nullptr), cmdPS(nullptr), cmdTr(nullptr), cmdGM(nullptr),
	Tool("Manipulate", false)	// not modal
{
}

ManipTool::~ManipTool()
{
}

// ----------------------------------------------------------------

bool ManipTool::Input3D(UINT uMsg, WPARAM wParam, LPARAM lParam, CameraView &v, WndView &vWnd)
{
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

		Sys_UpdateWindows(W_SCENE);
		return true;
	case WM_MOUSEMOVE:
		if (keys & MK_LBUTTON)
		{
			vec3 pt;
			vWnd.GetMsgXY(lParam, mx, my);
			mc = v.GetMouseContext(mx, my);
			mousePlane.TestRay(mc.org, mc.ray, pt);
			DragMove(mc, pt);

			Sys_UpdateWindows(W_SCENE);
			return true;
		}
		return false;
	case WM_LBUTTONUP:
		if (!hot) return false;

		vWnd.GetMsgXY(lParam, mx, my);
		mc = v.GetMouseContext(mx, my);
		DragFinish(mc);

		Sys_UpdateWindows(W_SCENE);
		if (!(keys & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON)))
			ReleaseCapture();
		hot = false;
		return true;
	}
	return hot;
}

bool ManipTool::Input2D(UINT uMsg, WPARAM wParam, LPARAM lParam, XYZView &v, WndView &vWnd)
{
	int keys = wParam;
	int x, y, mx, my;
	mouseContext_t mc;

	switch (uMsg)
	{
	//case WM_COMMAND:
	//	return InputCommand(wParam);
	case WM_LBUTTONDOWN:
		if (ShiftDown() || (!CtrlDown() && AltDown()))
			return false;
		SetCapture(vWnd.w_hwnd);
		hot = true;

		vWnd.GetMsgXY(lParam, mx, my);
		v.ToPoint(mx, my, x, y);
		mc = v.GetMouseContext(mx, my);
		DragStart2D(mc, v.dViewType);
		Sys_UpdateWindows(W_SCENE);
		return true;

	case WM_MOUSEMOVE:
		if (keys & MK_LBUTTON)
		{
			//assert(hot);	// happens after a context menu click
			vWnd.GetMsgXY(lParam, mx, my);
			v.ToPoint(mx, my, x, y);
			mc = v.GetMouseContext(mx, my);
			DragMove(mc, mc.org);
			Sys_UpdateWindows(W_SCENE);
			return true;
		}
		return false;

	case WM_LBUTTONUP:
		if (!hot) return false;
		vWnd.GetMsgXY(lParam, mx, my);
		v.ToPoint(mx, my, x, y);
		mc = v.GetMouseContext(mx, my);
		DragFinish(mc);

		Sys_UpdateWindows(W_SCENE);
		if (!(keys & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON)))
			ReleaseCapture();
		hot = false;
		return true;
	}
	return hot;
}

bool ManipTool::Input1D(UINT uMsg, WPARAM wParam, LPARAM lParam, ZView &v, WndView &vWnd)
{
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

		Sys_UpdateWindows(W_SCENE);
		return true;
	case WM_MOUSEMOVE:
		if (keys & MK_LBUTTON)
		{
			vWnd.GetMsgXY(lParam, mx, my);
			mc = v.GetMouseContext(mx, my);
			DragMove(mc, mc.org);

			Sys_UpdateWindows(W_SCENE);
			return true;
		}
		return false;
	case WM_LBUTTONUP:
		if (!hot) return false;

		// do stuff
		vWnd.GetMsgXY(lParam, mx, my);
		mc = v.GetMouseContext(mx, my);
		DragFinish(mc);

		Sys_UpdateWindows(W_SCENE);
		if (!(keys & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON)))
			ReleaseCapture();
		hot = false;
		return true;
	}
	return hot;
}

// ----------------------------------------------------------------

void ManipTool::DragStart1D(const mouseContext_t &mc)
{
	if (Selection::IsEmpty())
		return;

	ptDown = mc.org;

	std::vector<Face*> fSides, fBackSides;
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
			StartTranslate();
			return;
		}
		vec3 ray;
		for (b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
		{
			if (b->IsFiltered())
				continue;

			// check select hit first
			ray = b->Center();
			ray.z = mc.org.z;
			if (b->PointTest(ray))
			{
				StartTranslate();
				return;
			}

			// make a ray to encompass each brush, so that the manip catches everything on XY
			ray = (b->Center() - mc.org) * vec3(1, 1, 0);
			if (!VectorNormalize(ray))	// z-center is centered on this brush
			{
				ray = (b->maxs - mc.org) * vec3(1, 1, 0);
				if (!VectorNormalize(ray))
					Error("literally impossible raytest failure on z plane detect\n");
			}
			for (Face *f = b->faces; f; f = f->fnext)
			{
				if (f->TestSideSelect(mc.org, ray))
					fSides.push_back(f);
			}
		}

		if (fSides.empty())
			return;		// is this even possible

		// match backfaces to sliding front faces, so brush contacts move with their planes
		for (Brush* b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
		{
			for (Face *f = b->faces; f; f = f->fnext)
			{
				for (auto fsIt = fSides.begin(); fsIt != fSides.end(); ++fsIt)
				{
					if (f->plane.EqualTo(&(*fsIt)->plane, true))
						fBackSides.push_back(f);
				}
			}
		}
		fSides.insert(fSides.end(), fBackSides.begin(), fBackSides.end());
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
			ptDelta = ptDown - pointOnGrid(mc.org);

			StartTranslate();
			cmdTr->Translate(ptDelta);
			return;
		}

		ptDown = mc.org;

		if (!Selection::OnlyBrushEntities())
		{
			StartTranslate();
			return;
		}
		// translate if click hit the selection, side-detect for auto plane drag otherwise
		trace_t t = Selection::TestRay(mc.org, mc.ray, SF_SELECTED_ONLY);	
		if (t.selected)
		{
			StartTranslate();
			return;
		}
		else
		{
			std::vector<Face*> fSides;
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
			return;
		}
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
	mousePlane.normal = CrossProduct(mc.right, mc.up);

	trace_t t;

	// brushes selected: translate if click hit the selection, shear if ctrl-click, side-detect for auto plane drag otherwise
	if (Selection::HasBrushes())
	{
		if (!Selection::OnlyBrushEntities())
		{
			mousePlane.dist = DotProduct(Selection::GetMid(), mousePlane.normal);
			mousePlane.TestRay(mc.org, mc.ray, ptDown);
			StartTranslate();
			return;
		}
		std::vector<Face*> fSides;
		t = Selection::TestRay(mc.org, mc.ray, SF_SELECTED_ONLY);
		if (t.selected)
		{
			// we'll trace future mouse events against an implied plane centered on where we hit
			// the selection, so it feels like we're dragging the selection by the point we 'grabbed'
			ptDown = mc.org + mc.ray * t.dist;
			mousePlane.dist = DotProduct(ptDown, mousePlane.normal);

			if (CtrlDown())
			{
				FrontSelectShearFaces(t.face, fSides);
				StartQuickShear(fSides);
			}
			else
				StartTranslate();
		}
		else
		{
			mousePlane.dist = DotProduct(Selection::GetMid(), mousePlane.normal);
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
			mousePlane.dist = DotProduct(ptDown, mousePlane.normal);
		}
		else
		{
			mousePlane.dist = DotProduct(Selection::GetMid(), mousePlane.normal);
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
		assert(cmdTr);
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

void ManipTool::DragFinish(const mouseContext_t &vc)
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
	case MT_TRANSLATE:
		assert(cmdTr);
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

void ManipTool::StartTranslate()
{
	assert(!cmdTr);
	cmdTr = new CmdTranslate();
	cmdTr->UseBrushes(&g_brSelectedBrushes);
	cmdTr->TextureLock(g_qeglobals.d_bTextureLock);
	state = MT_TRANSLATE;
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
		glColor4f(g_qeglobals.d_savedinfo.v3Colors[COLOR_SELBRUSHES][0],
				g_qeglobals.d_savedinfo.v3Colors[COLOR_SELBRUSHES][1],
				g_qeglobals.d_savedinfo.v3Colors[COLOR_SELBRUSHES][2],
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
		assert(cmdTr);
		if (!cmdTr->postDrag)
			return false;
		glTranslatef(cmdTr->trans[0], cmdTr->trans[1], cmdTr->trans[2]);
		v.DrawSelected(&g_brSelectedBrushes);
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
	switch (state)
	{
	case MT_NEW:
		if (!brDragNew)
			break;

		v.BeginDrawSelection();
		brDragNew->DrawXY(v.GetAxis());
		v.EndDrawSelection();
		v.DrawSizeInfo(brDragNew->mins, brDragNew->maxs);
		return true;

	case MT_TRANSLATE:
		assert(cmdTr);
		if (!cmdTr->postDrag)
			return false;
		glTranslatef(cmdTr->trans[0], cmdTr->trans[1], cmdTr->trans[2]);
		v.DrawSelection();
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
	switch (state)
	{
	// lunaran TODO: draw the MT_NEW brush if it intersects the z-core
	case MT_TRANSLATE:
		assert(cmdTr);
		if (!cmdTr->postDrag)
			return false;
		glTranslatef(0, cmdTr->trans[2], 0);
		v.DrawSelection();
		glTranslatef(0, -cmdTr->trans[2], 0);
		return true;
	default:
		return false;
	}
	return false;
}

