//==============================
//	ManipTool.cpp
//==============================

#include "qe3.h"


ManipTool::ManipTool() :
	state(NONE),
	xDown(0), yDown(0), vDim1(-1), vDim2(-1),
	brDragNew(nullptr),
	Tool("Manipulate", false)	// not modal
{
}


ManipTool::~ManipTool()
{
}

bool ManipTool::Input3D(UINT uMsg, WPARAM wParam, LPARAM lParam, CameraView &v, WndView &vWnd)
{

	return false;
}

bool ManipTool::Input2D(UINT uMsg, WPARAM wParam, LPARAM lParam, XYZView &v, WndView &vWnd)
{
	int keys = wParam;
	int x, y, mx, my;
	vec3 pt;

	switch (uMsg)
	{
	//case WM_COMMAND:
	//	return InputCommand(wParam);
	case WM_LBUTTONDOWN:
		if (wParam & (MK_SHIFT | MK_CONTROL | MK_ALT ))
			return false;
		SetCapture(vWnd.w_hwnd);
		hot = true;

		vWnd.GetMsgXY(lParam, mx, my);
		v.ToGridPoint(mx, my, x, y);
		v.GetDims(vDim1, vDim2, vType);

		DragStart(x,y);

		Sys_UpdateWindows(W_SCENE);
		return true;

	case WM_MOUSEMOVE:
		if (keys & MK_LBUTTON)
		{
			assert(hot);
			vWnd.GetMsgXY(lParam, mx, my);
			v.ToGridPoint(mx, my, x, y);
			DragMove(x,y);
			Sys_UpdateWindows(W_SCENE);
			return true;
		}
		return false;

	case WM_LBUTTONUP:
		if (!hot) return false;
		vWnd.GetMsgXY(lParam, mx, my);
		v.ToGridPoint(mx, my, x, y);
		DragFinish(x,y);

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
	return false;
}




void ManipTool::DragStart(int x, int y)
{
	xDown = x;
	yDown = y;

	// set up states but wait for significant mouse movement before doing anything real
	if (Selection::IsEmpty())
	{
		// plan to start dragging a new brush
		state = DRAGNEW;
		return;
	}


}

void ManipTool::DragMove(int x, int y)
{
	int xDelta, yDelta;
	xDelta = x - xDown;
	yDelta = y - yDown;

	switch (state)
	{
	case DRAGNEW:
		// dragging a new brush
		if (abs(xDelta) && abs(yDelta))
		{
			vec3 mins, maxs;

			mins[vDim1] = min(x, xDown);
			maxs[vDim1] = max(x, xDown);
			mins[vDim2] = min(y, yDown);
			maxs[vDim2] = max(y, yDown);

			// snap unknown bounds, since grid might have changed since the workzone was set
			mins[vType] = floor((float)g_qeglobals.d_v3WorkMin[vType] / g_qeglobals.d_nGridSize + 0.5) * g_qeglobals.d_nGridSize;
			maxs[vType] = floor((float)g_qeglobals.d_v3WorkMax[vType] / g_qeglobals.d_nGridSize + 0.5) * g_qeglobals.d_nGridSize;
			if (mins[vType] == maxs[vType])
				maxs[vType] += g_qeglobals.d_nGridSize;

			if (brDragNew)
				brDragNew->Recreate(mins, maxs, &g_qeglobals.d_workTexDef);
			else
				brDragNew = Brush::Create(mins, maxs, &g_qeglobals.d_workTexDef);
			brDragNew->owner = g_map.world;
			brDragNew->Build();
		}
		else
		{
			delete brDragNew;
			brDragNew = nullptr;
		}
		return;

	case DRAGMOVE:
	case DRAGPLANE:
	default:
		return;
	}
}

void ManipTool::DragFinish(int x, int y)
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
	case DRAGPLANE:
	default:
		break;
	}
	state = NONE;
}




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

		// non-zbuffered outline
		glDisable(GL_BLEND);
		glDisable(GL_DEPTH_TEST);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glColor3f(1, 1, 1);

		for (Face *face = brDragNew->basis.faces; face; face = face->fnext)
			face->Draw();
		return true;

	case DRAGMOVE:
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
	case DRAGPLANE:
	default:
		return false;
	}
	return false;
}

