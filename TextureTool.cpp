//==============================
//	TextureTool.cpp
//==============================

#include "qe3.h"
#include "TextureTool.h"
#include "CmdCompound.h"
#include "CmdTextureApply.h"
#include "CmdTextureFit.h"
#include "CmdTextureMod.h"


TextureTool::TextureTool() : 
	lastTexMod(nullptr), lastWrap(nullptr), lastWrap2(nullptr), cmdCmp(nullptr),
	hwndReplaceDlg(NULL), sampleState(TTSS_NONE),
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
	vec3 ray;
	trace_t t;
	int x, y;

	if (sampleState && hwndReplaceDlg)
	{
		if (uMsg == WM_MBUTTONDOWN || uMsg == WM_LBUTTONDOWN)
		{
			vWnd.GetMsgXY(lParam, x, y);
			v.PointToRay(x, y, ray);
			t = Selection::TestRay(v.origin, ray, SF_NOFIXEDSIZE);
			if (!t.brush)
				return false;

			if (sampleState == TTSS_FIND)
				SendDlgItemMessage(hwndReplaceDlg, IDC_COMBO_TEXFIND, WM_SETTEXT, 0, (LPARAM)t.face->texdef.name);
			else if (sampleState == TTSS_REPLACE)
				SendDlgItemMessage(hwndReplaceDlg, IDC_COMBO_TEXREPLACE, WM_SETTEXT, 0, (LPARAM)t.face->texdef.name);

			//Crosshair(false);
			hot = false;
			sampleState = TTSS_NONE;
		}
		return true;
	}

	switch (uMsg)
	{
	case WM_MBUTTONDOWN:
		{
			//hot = true;
			//SetCapture(vWnd.w_hwnd);
			vWnd.GetMsgXY(lParam, x, y);
			v.PointToRay(x, y, ray);
			t = Selection::TestRay(v.origin, ray, SF_NOFIXEDSIZE);
			if (!t.brush)
				return true;

			if (AltDown())
			{
				hot = true;
				SetCapture(vWnd.w_hwnd);
				lastWrap2 = lastWrap;
				lastWrap = t.face;
			}
			else
			{
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
			}
			return true;
		}
		return false;

	case WM_MOUSEMOVE:
		if (!hot)
			return false;
		vWnd.GetMsgXY(lParam, x, y);
		v.PointToRay(x, y, ray);
		t = Selection::TestRay(v.origin, ray, SF_NOFIXEDSIZE);
		if (!t.brush)
			return false;
		if (!lastWrap)
			return false;
		if (t.face != lastWrap)
		{
			if (t.face != lastWrap2)	// don't wrap back to where we just wrapped from
			{
				if (!cmdCmp)
					cmdCmp = new CmdCompound("Wrap Texture");
				TexDef td = lastWrap->texdef;
				Surface::WrapProjection(lastWrap, t.face, td);
				CmdTextureApply *cmdTA = new CmdTextureApply();
				cmdTA->UseFace(t.face);
				cmdTA->Apply(td);
				cmdCmp->Complete(cmdTA);
				Sys_UpdateWindows(W_CAMERA);
			}
			lastWrap2 = lastWrap;
			lastWrap = t.face;
			return true;
		}
		return false;

	case WM_MBUTTONUP:
		if (!hot) return false;
		hot = false;
		lastWrap = nullptr;
		lastWrap2 = nullptr;
		if (cmdCmp)
		{
			g_cmdQueue.Complete(cmdCmp);
			cmdCmp = nullptr;
		}
		if (!(wParam & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON)))
			ReleaseCapture();
		return true;

	}
	return false;
}

bool TextureTool::InputTex(UINT uMsg, WPARAM wParam, LPARAM lParam, TextureView &v, WndView &vWnd)
{
	int xPos, yPos, fwKeys;
	fwKeys = wParam;
	xPos = (short)LOWORD(lParam);  // horizontal position of cursor 
	yPos = (short)HIWORD(lParam);  // vertical position of cursor 

	switch (uMsg)
	{
	case WM_MBUTTONDOWN:
	case WM_LBUTTONDOWN:
		{
			TexDef texdef;
			Texture* tw = v.TexAtCursorPos(xPos, yPos);

			if (tw && sampleState && hwndReplaceDlg)
			{
				if (sampleState == TTSS_FIND)
				{
					SendDlgItemMessage(hwndReplaceDlg, IDC_COMBO_TEXFIND, WM_SETTEXT, 0, (LPARAM)tw->name);
				}
				else if (sampleState == TTSS_REPLACE)
				{
					SendDlgItemMessage(hwndReplaceDlg, IDC_COMBO_TEXREPLACE, WM_SETTEXT, 0, (LPARAM)tw->name);
				}
				//Crosshair(false); // only works in MOUSEMOVE messages
				hot = false;
				sampleState = TTSS_NONE;
				return true;
			}

			if (!tw)
				return false;
			texdef.Set(tw);
			v.ChooseTexture(&texdef);
			if (uMsg == WM_MBUTTONDOWN)
				Surface::SetTexdef(texdef, SFI_ALL - SFI_NAME);
			else
				Surface::SetTexdef(texdef, 0);
			return true;
		}
		return false;
	}

	return false;
}

bool TextureTool::Input(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_COMMAND)
	{
		if (LOWORD(wParam) == ID_TEXTURES_REPLACEALL)
		{
			FindTextureDialog();
			return true;
		}
	}
	return false;
}

void TextureTool::SelectionChanged()
{
	lastTexMod = nullptr;
	lastTexModTime = 0;
}


// ----------------------------------------------------------------

/*
========================

Find & Replace Dialog

========================
*/

void TextureTool::UpdateFindReplaceHistories()
{
	char	szFind[MAX_TEXNAME];
	char	szReplace[MAX_TEXNAME];

	GetDlgItemText(hwndReplaceDlg, IDC_COMBO_TEXFIND, szFind, MAX_TEXNAME);
	if (szFind[0] > ' ')
	{
		for (auto fhIt = findHistory.begin(); fhIt != findHistory.end(); ++fhIt)
		{
			if (*fhIt != szFind)
				continue;
			findHistory.erase(fhIt);
			break;
		}
		findHistory.emplace_back(szFind);
		SendDlgItemMessage(hwndReplaceDlg, IDC_COMBO_TEXFIND, CB_INSERTSTRING, 0, (LPARAM)szFind);
	}

	GetDlgItemText(hwndReplaceDlg, IDC_COMBO_TEXREPLACE, szReplace, MAX_TEXNAME);
	if (szReplace[0] > ' ')
	{
		for (auto rhIt = replaceHistory.begin(); rhIt != replaceHistory.end(); ++rhIt)
		{
			if (*rhIt != szReplace)
				continue;
			replaceHistory.erase(rhIt);
			break;
		}
		replaceHistory.emplace_back(szReplace);
		SendDlgItemMessage(hwndReplaceDlg, IDC_COMBO_TEXREPLACE, CB_INSERTSTRING, 0, (LPARAM)szReplace);
	}
}

BOOL CALLBACK FindTextureDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	assert(g_qeglobals.d_texTool);
	return g_qeglobals.d_texTool->InputReplaceDlg(hwndDlg, uMsg, wParam, lParam);
}

bool TextureTool::InputReplaceDlg(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	bool	bSelected;
	TexDef	*texdef;

	texdef = &g_qeglobals.d_workTexDef;

	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		HBITMAP hb = (HBITMAP)LoadImage(g_qeglobals.d_hInstance, (LPCTSTR)IDB_SAMPLE, IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE | LR_LOADTRANSPARENT | LR_LOADMAP3DCOLORS);
		hwndReplaceDlg = hwndDlg;

		for (auto fhIt = findHistory.rbegin(); fhIt != findHistory.rend(); ++fhIt)
			SendDlgItemMessage(hwndReplaceDlg, IDC_COMBO_TEXFIND, CB_ADDSTRING, 0, (LPARAM)fhIt->n);
		for (auto rhIt = replaceHistory.rbegin(); rhIt != replaceHistory.rend(); ++rhIt)
			SendDlgItemMessage(hwndReplaceDlg, IDC_COMBO_TEXREPLACE, CB_ADDSTRING, 0, (LPARAM)rhIt->n);

		SetFocus(GetDlgItem(hwndReplaceDlg, IDC_COMBO_TEXFIND));
		SetDlgItemText(hwndReplaceDlg, IDC_COMBO_TEXFIND, texdef->name);
		bSelected = SendDlgItemMessage(hwndReplaceDlg, IDC_CHECK_SELECTED, BM_GETCHECK, 0, 0) > 0;

		SendDlgItemMessage(hwndReplaceDlg, IDC_TEXFIND_SAMPLE, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hb);
		SendDlgItemMessage(hwndReplaceDlg, IDC_TEXREPLACE_SAMPLE, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hb);

		ShowWindow(hwndReplaceDlg, SW_SHOW);
		return TRUE;
	}
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_TEXFIND_SAMPLE:
			sampleState = TTSS_FIND;
			Sys_Status("Click a texture or brush to use its name", 0);
			hot = true;
			Crosshair(true);
			return TRUE;
		case IDC_TEXREPLACE_SAMPLE:
			sampleState = TTSS_REPLACE;
			Sys_Status("Click a texture or brush to use its name", 0);
			hot = true;
			Crosshair(true);
			return TRUE;

		case IDAPPLY:
			UpdateFindReplaceHistories();
			bSelected = SendDlgItemMessage(hwndReplaceDlg, IDC_CHECK_SELECTED, BM_GETCHECK, 0, 0) > 0;
			Surface::FindReplace(findHistory.back().n, replaceHistory.back().n, bSelected);// , false);
			Sys_UpdateWindows(W_CAMERA);
			return TRUE;

		case IDCLOSE:
			UpdateFindReplaceHistories();
			EndDialog(hwndReplaceDlg, 0);
			hwndReplaceDlg = NULL;
			return TRUE;
		}
		return FALSE;
	}
	return FALSE;
}

void TextureTool::FindTextureDialog()
{
	if (hwndReplaceDlg)
	{
		SetFocus(hwndReplaceDlg);
		return;
	}

	// lunaran: modeless f&r dialog
	hwndReplaceDlg = CreateDialog(g_qeglobals.d_hInstance, MAKEINTRESOURCE(IDD_FINDREPLACE), g_qeglobals.d_hwndMain, FindTextureDlgProc);
	ShowWindow(hwndReplaceDlg, SW_SHOW);
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
		lastTexModTime + CMD_COMBINE_TIME < clock())
	{
		lastTexMod = new CmdTextureMod();

		if (Selection::HasBrushes())
			lastTexMod->UseBrushes(&g_brSelectedBrushes);
		else if (Selection::NumFaces())
			lastTexMod->UseFaces(Selection::faces);
	}
	else
		Sys_Printf("combining texmod\n");
}

