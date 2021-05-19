//==============================
//	TextureTool.cpp
//==============================

#include "qe3.h"
#include "TextureTool.h"

// 400ms debounce time for adding a new texmod cmd to the undo queue vs reusing the last one
#define TEXCMD_COMBINE_TIME		(CLOCKS_PER_SEC * 0.4f)



TextureTool::TextureTool() : 
	lastTexMod(nullptr),
	Tool("Texturing", false)	// always on (not modal)
{
	g_qeglobals.d_texTool = this;
}

TextureTool::~TextureTool()
{
	g_qeglobals.d_texTool = nullptr;
}

// ----------------------------------------------------------------


bool TextureTool::Input3D(UINT uMsg, WPARAM wParam, LPARAM lParam, CameraView &v, WndView & vWnd)
{
	if (uMsg != WM_MBUTTONDOWN)
		return false;

	vec3 ray;
	trace_t t;
	int x, y;

	switch (uMsg)
	{
	case WM_MBUTTONDOWN:
		//hot = true;
		//SetCapture(vWnd.w_hwnd);
		vWnd.GetMsgXY(lParam, x, y);
		v.PointToRay(x, y, ray);
		t = Selection::TestRay(v.origin, ray, SF_NOFIXEDSIZE);
		if (!t.brush)
			return true;

		CmdTextureApply *cmdTA = new CmdTextureApply();

		if (wParam & MK_CONTROL && wParam & MK_SHIFT)
		{
			// ctrl shift mmb = apply texture and alignment to hit face
			cmdTA->UseFace(t.face);
			cmdTA->Apply(g_qeglobals.d_workTexDef);
		}
		else if (wParam & MK_SHIFT)
		{
			// shift mmb = apply texture to hit face, not alignment
			cmdTA->UseFace(t.face);
			cmdTA->Apply(g_qeglobals.d_workTexDef, SFI_ALL - SFI_NAME);
		}
		else if (wParam & MK_CONTROL)
		{
			// ctrl mmb = apply texture and alignment to entire hit brush
			cmdTA->UseBrush(t.brush);
			cmdTA->Apply(g_qeglobals.d_workTexDef);
		}
		else
		{
			// mmb = sample hit face, apply texture and alignment to to selection
			UpdateWorkzone(t.brush);
			g_qeglobals.d_vTexture.ChooseTexture(&t.face->texdef);
			if (Selection::NumFaces())
				cmdTA->UseFaces(Selection::faces);
			else if (Selection::HasBrushes())
				cmdTA->UseBrushes(&g_brSelectedBrushes);

			cmdTA->Apply(g_qeglobals.d_workTexDef);
		}
		g_cmdQueue.Complete(cmdTA);
		return true;

	/*
	case WM_MOUSEMOVE:
		if (!hot) return false;
		v.PointToRay(x, y, ray);
		return true;

	case WM_MBUTTONUP:
		if (!hot) return false;
		hot = false;
		if (!(wParam & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON)))
			ReleaseCapture();
		return true;
	*/
	}
	return false;
}

bool TextureTool::Input2D(UINT uMsg, WPARAM wParam, LPARAM lParam, XYZView &v, WndView & vWnd)
{
	return false;
}


void TextureTool::SelectionChanged()
{
	lastTexMod = nullptr;
	lastTexModTime = 0;
}


// ----------------------------------------------------------------

void TextureTool::FitTexture(float x, float y)
{
	CmdTextureFit *cmdTF = new CmdTextureFit();

	if (Selection::HasBrushes())
		cmdTF->UseBrushes(&g_brSelectedBrushes);
	else if (Selection::NumFaces())
		cmdTF->UseFaces(Selection::faces);

	cmdTF->Fit(x, y);
	g_cmdQueue.Complete(cmdTF);
}

// ----------------------------------------------------------------

// the TextureTool will reuse the last TextureMod command on the undo stack if it 
// was used to perform the same action and if less than a short interval has gone
// by. this makes quick repeated texture nudges resolve to one discrete 'event',
// and thus one undo step.
void TextureTool::ShiftTexture(int x, int y)
{
	GetTexModCommand(TM_SHIFT);
	lastTexMod->Shift(x, y);

	if (lastTexMod->state == Command::LIVE)
		g_cmdQueue.Complete(lastTexMod);
	lastTexModTime = clock();
}

void TextureTool::ScaleTexture(float x, float y)
{
	GetTexModCommand(TM_SCALE);
	lastTexMod->Scale(x, y);

	if (lastTexMod->state == Command::LIVE)
		g_cmdQueue.Complete(lastTexMod);
	lastTexModTime = clock();
}

void TextureTool::RotateTexture(float r)
{
	GetTexModCommand(TM_ROTATE);
	lastTexMod->Rotate(r);

	if (lastTexMod->state == Command::LIVE)
		g_cmdQueue.Complete(lastTexMod);
	lastTexModTime = clock();
}

void TextureTool::GetTexModCommand(texModType_t tm)
{
	if (!lastTexMod || !g_cmdQueue.CanUndo() ||
		g_cmdQueue.LastUndo() != lastTexMod ||
		lastTexMod->action != tm ||
		lastTexModTime + TEXCMD_COMBINE_TIME < clock())
	{
		lastTexMod = new CmdTextureMod();

		if (Selection::HasBrushes())
			lastTexMod->UseBrushes(&g_brSelectedBrushes);
		else if (Selection::NumFaces())
			lastTexMod->UseFaces(Selection::faces);
	}
}

