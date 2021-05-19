//==============================
//	ManipTool.cpp
//==============================

#include "qe3.h"


ManipTool::ManipTool() :
	state(NONE),
	//xDown(0), yDown(0), 
	brDragNew(nullptr), cmdPS(nullptr), cmdTr(nullptr),
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
		if (keys & (MK_SHIFT | MK_CONTROL | MK_ALT))
			return false;
		SetCapture(vWnd.w_hwnd);
		hot = true;

		// do stuff
		vWnd.GetMsgXY(lParam, mx, my);
		mc = v.GetMouseContext(mx, my);
		DragStart(mc);

		Sys_UpdateWindows(W_SCENE);
		return true;
	case WM_MOUSEMOVE:
		if (keys & MK_LBUTTON)
		{
			vWnd.GetMsgXY(lParam, mx, my);
			mc = v.GetMouseContext(mx, my);
			DragMove(mc);

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
		if (keys & (MK_SHIFT | MK_CONTROL | MK_ALT ))
			return false;
		SetCapture(vWnd.w_hwnd);
		hot = true;

		vWnd.GetMsgXY(lParam, mx, my);
		v.ToPoint(mx, my, x, y);
		mc = v.GetMouseContext(mx, my);
		//ptDown = (float)x * mc.right + (float)y * mc.up;
		ptDown = mc.org;

		DragStart(mc);
		Sys_UpdateWindows(W_SCENE);
		return true;

	case WM_MOUSEMOVE:
		if (keys & MK_LBUTTON)
		{
			//assert(hot);	// happens after a context menu click
			vWnd.GetMsgXY(lParam, mx, my);
			v.ToPoint(mx, my, x, y);
			mc = v.GetMouseContext(mx, my);
			DragMove(mc);
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
		if (keys & (MK_SHIFT | MK_CONTROL | MK_ALT))
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
			DragMove(mc);

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
		//for (int i = 0; i < Selection::NumFaces(); i++)
		//{
		//	fSides.push_back(g_vfSelectedFaces[i]);
		//}
		fSides = Selection::faces;
	}
	else
	{
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
				cmdTr = new CmdTranslate();
				cmdTr->UseBrushes(&g_brSelectedBrushes);
				// set tlock on the command once up front rather than letting it ping the 
				// setting itself, so it remains contained
				cmdTr->TextureLock(g_qeglobals.d_bTextureLock);
				state = DRAGMOVE;
				return;
			}

			// make a ray to encompass each brush, so that the manip catches everything on XY
			ray = (b->Center() - mc.org) * vec3(1, 1, 0);
			if (!VectorNormalize(ray))	// z-center is centered on this brush
			{
				ray = (b->basis.maxs - mc.org) * vec3(1, 1, 0);
				if (!VectorNormalize(ray))
					Error("literally impossible raytest failure on z plane detect\n");
			}
			for (Face *f = b->basis.faces; f; f = f->fnext)
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
			for (Face *f = b->basis.faces; f; f = f->fnext)
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

	state = DRAGPLANE;
	cmdPS = new CmdPlaneShift();
	cmdPS->SetFaces(fSides);
}

void ManipTool::DragStart(const mouseContext_t &mc)
{
	// no selection: in 2D view, start dragging new brush
	if (Selection::IsEmpty())
	{
		if (mc.dims == 2)
		{
			// set state but don't make a new brush until a significant enough mouse move has happened
			state = DRAGNEW;
			ptDown = pointOnGrid(ptDown);
			ptDown = ptDown * (mc.ray + vec3(1));	// null out the extra large third dimension
		}
		return;
	}

	// ----------------

	mousePlane.normal = CrossProduct(mc.right, mc.up);

	trace_t t;
	vec3 tpoint, selmins, selmaxs;
	std::vector<Face*> fSides, fBackSides;
	ClearBounds(selmins, selmaxs);

	// faces selected: always go for a plane drag
	if (Selection::NumFaces())
	{
		t = Selection::TestRay(mc.org, mc.ray, SF_FACES);
		if (t.face && Selection::IsFaceSelected(t.face))
		{
			tpoint = mc.org + mc.ray * t.dist;

			//for (int i = 0; i < Selection::NumFaces(); i++)
			//{
			//	fSides.push_back(g_vfSelectedFaces[i]);
			//}
		}
		else
		{
			for (auto fIt = Selection::faces.begin(); fIt != Selection::faces.end(); ++fIt)
			{
				(*fIt)->AddBounds(selmins, selmaxs);
				//fSides.push_back(g_vfSelectedFaces[i]);
			}
			tpoint = (selmins + selmaxs) * 0.5f;
		}
		fSides = Selection::faces;
	}
	else	// ----------------
	{
		// brushes selected: translate if click hit the selection, side-detect for auto plane drag otherwise
		t = Selection::TestRay(mc.org, mc.ray, SF_SELECTED_ONLY);

		// hit selection, translate it
		if (t.selected)
		{
			assert(!cmdTr);
			if (mc.dims == 3)
			{
				// we'll trace future mouse events against an implied plane centered on where we hit
				// the selection, so it feels like we're dragging the selection by the point we 'grabbed'
				tpoint = mc.org + mc.ray * t.dist;
				ptDown = tpoint;
				mousePlane.dist = DotProduct(tpoint, mousePlane.normal);
			}
			cmdTr = new CmdTranslate();
			cmdTr->UseBrushes(&g_brSelectedBrushes);
			// set tlock on the command once up front rather than letting it ping the 
			// setting itself, so it remains contained
			cmdTr->TextureLock(g_qeglobals.d_bTextureLock);

			state = DRAGMOVE;
			return;
		}

		// missed selection, select nearby faces for slide
		for (Brush* b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
		{
			for (Face *f = b->basis.faces; f; f = f->fnext)
			{
				if (f->TestSideSelect(mc.org, mc.ray))
					fSides.push_back(f);
			}
			if (mc.dims == 3)
			{
				selmins = glm::min(selmins, b->basis.mins);
				selmaxs = glm::max(selmaxs, b->basis.maxs);
			}
		}

		if (fSides.empty())
			return;		// is this even possible

		// match backfaces to sliding front faces, so brush contacts move with their planes
		for (Brush* b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
		{
			for (Face *f = b->basis.faces; f; f = f->fnext)
			{
				for (auto fsIt = fSides.begin(); fsIt != fSides.end(); ++fsIt)
				{
					if (f->plane.EqualTo(&(*fsIt)->plane, true))
						fBackSides.push_back(f);
				}
			}
		}
		fSides.insert(fSides.end(), fBackSides.begin(), fBackSides.end());
		tpoint = (selmins + selmaxs) * 0.5f;
	}

	if (fSides.empty())
		return;
	if (mc.dims == 3)
	{
		mousePlane.dist = DotProduct(tpoint, mousePlane.normal);

		ptDown = mc.org;
		tpoint = mc.org + mc.ray * (float)g_qeglobals.d_savedinfo.nMapSize;
		mousePlane.ClipLine(ptDown, tpoint);
	}
	state = DRAGPLANE;
	cmdPS = new CmdPlaneShift();
	cmdPS->SetFaces(fSides);
}

void ManipTool::DragMove(const mouseContext_t &mc)
{
	vec3 ptDelta;

	switch (state)
	{
	case DRAGNEW:
	{
		vec3 mins, maxs;
		float dMin, dMax;

		dMin = round(DotProduct(g_qeglobals.d_v3WorkMin, mc.ray), g_qeglobals.d_nGridSize);
		dMax = round(DotProduct(g_qeglobals.d_v3WorkMax, mc.ray), g_qeglobals.d_nGridSize);
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
	case DRAGMOVE:
		assert(cmdTr);
		if (mc.dims == 3)
		{
			vec3 pt1, pt2;
			pt1 = mc.org;
			pt2 = mc.org + mc.ray * (float)g_qeglobals.d_savedinfo.nMapSize;
			mousePlane.ClipLine(pt1, pt2);
			ptDelta = pointOnGrid(pt1 - ptDown);
		}
		else
		{
			ptDelta = pointOnGrid(mc.org - ptDown);
		}
		cmdTr->Translate(ptDelta);
		return;
	case DRAGPLANE:
	{
		assert(cmdPS);
		if (mc.dims == 3)
		{
			vec3 pt1, pt2;
			pt1 = mc.org;
			pt2 = mc.org + mc.ray * (float)g_qeglobals.d_savedinfo.nMapSize;
			mousePlane.ClipLine(pt1, pt2);
			ptDelta = pointOnGrid(pt1 - ptDown);
		}
		else
		{
			ptDelta = pointOnGrid(mc.org - ptDown);
		}
		cmdPS->Translate(ptDelta);
		return;
	}
	default:
		return;
	}
}

void ManipTool::DragFinish(const mouseContext_t &vc)
{
	switch (state)
	{
	case DRAGNEW:
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
	case DRAGMOVE:
		assert(cmdTr);
		g_cmdQueue.Complete(cmdTr);
		cmdTr = nullptr;
		break;

	case DRAGPLANE:
		assert(cmdPS);
		g_cmdQueue.Complete(cmdPS);
		cmdPS = nullptr;
		break;
	default:
		break;
	}
	state = NONE;
}

// ----------------------------------------------------------------

bool ManipTool::Draw3D(CameraView &v)
{
	switch (state)
	{
	case DRAGNEW:
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
		for (Face *face = brDragNew->basis.faces; face; face = face->fnext)
			face->Draw();
		/*
		// non-zbuffered outline
		glDisable(GL_BLEND);
		glDisable(GL_DEPTH_TEST);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glColor3f(1, 1, 1);

		for (Face *face = brDragNew->basis.faces; face; face = face->fnext)
			face->Draw();
		*/
		return true;

	case DRAGMOVE:
		assert(cmdTr);
		if (!cmdTr->postDrag)
			return false;
		glTranslatef(cmdTr->trans[0], cmdTr->trans[1], cmdTr->trans[2]);
		v.DrawSelected(&g_brSelectedBrushes);
		glTranslatef(-cmdTr->trans[0], -cmdTr->trans[1], -cmdTr->trans[2]);
		return true;

	case DRAGPLANE:
	default:
		break;
	}
	return false;
}

bool ManipTool::Draw2D(XYZView &v)
{
	switch (state)
	{
	case DRAGNEW:
		if (!brDragNew)
			break;

		v.BeginDrawSelection();
		brDragNew->DrawXY(v.GetAxis());
		v.EndDrawSelection();
		v.DrawSizeInfo(brDragNew->basis.mins, brDragNew->basis.maxs);
		return true;

	case DRAGMOVE:
		assert(cmdTr);
		if (!cmdTr->postDrag)
			return false;
		glTranslatef(cmdTr->trans[0], cmdTr->trans[1], cmdTr->trans[2]);
		v.DrawSelection();
		glTranslatef(-cmdTr->trans[0], -cmdTr->trans[1], -cmdTr->trans[2]);
		return true;

	case DRAGPLANE:
	default:
		return false;
	}
	return false;
}

bool ManipTool::Draw1D(ZView &v)
{
	switch (state)
	{
	case DRAGMOVE:
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

