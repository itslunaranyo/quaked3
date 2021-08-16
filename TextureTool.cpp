//==============================
//	TextureTool.cpp
//==============================

#include "pre.h"
#include "qe3.h"
#include "TextureTool.h"
#include "select.h"
#include "Command.h"
#include "CmdCompound.h"
#include "CmdTextureApply.h"
#include "CmdTextureFit.h"
#include "CmdTextureMod.h"
#include "CmdSetKeyvalue.h"
#include "CameraView.h"
#include "TextureView.h"
#include "surface.h"
#include "map.h"
#include "WndCamera.h"
#include "win_dlg.h"

TextureTool* g_texTool;

TextureTool::TextureTool() : 
	lastTexMod(nullptr), lastWrap(nullptr), lastWrap2(nullptr), cmdCmp(nullptr),
	hwndReplaceDlg(NULL), sampleState(TTSS_NONE),
	Tool("Texturing", false)	// always on (not modal)
{
	g_texTool = this;
}

TextureTool::~TextureTool()
{
	g_texTool = nullptr;
}

// ----------------------------------------------------------------


bool TextureTool::Input3D(UINT uMsg, WPARAM wParam, LPARAM lParam, CameraView &v, WndCamera &vWnd)
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
			t = Selection::TestRay(v.GetOrigin(), ray, SF_NOFIXEDSIZE);
			if (!t.brush)
				return false;

			if (sampleState == TTSS_FIND)
				SendDlgItemMessage(hwndReplaceDlg, IDC_COMBO_TEXFIND, WM_SETTEXT, 0, (LPARAM)t.face->texdef.name.data());
			else if (sampleState == TTSS_REPLACE)
				SendDlgItemMessage(hwndReplaceDlg, IDC_COMBO_TEXREPLACE, WM_SETTEXT, 0, (LPARAM)t.face->texdef.name.data());

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
			t = Selection::TestRay(v.GetOrigin(), ray, SF_NOFIXEDSIZE);
			if (!t.brush)
				return true;

			if (AltDown())
			{
				hot = true;
				SetCapture(vWnd.wHwnd);
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
					QE_UpdateWorkzone(t.brush);
					g_vTexture.ChooseTexture(&t.face->texdef);
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
		t = Selection::TestRay(v.GetOrigin(), ray, SF_NOFIXEDSIZE);
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
				WndMain_UpdateWindows(W_CAMERA);
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

bool TextureTool::InputTex(UINT uMsg, WPARAM wParam, LPARAM lParam, TextureView &v, WndTexture &vWnd)
{
	int xPos, yPos, fwKeys;
	fwKeys = wParam;
	xPos = (short)LOWORD(lParam);  // horizontal position of cursor 
	yPos = (short)HIWORD(lParam);  // vertical position of cursor 

	switch (uMsg)
	{
	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
		{
			TexDef texdef;
			Texture* tw = v.TexAtCursorPos(xPos, yPos);

			if (!tw) return false;

			// are we currently sampling for the F&R dialog?
			if (tw && sampleState && hwndReplaceDlg)
			{
				if (sampleState == TTSS_FIND)
				{
					SendDlgItemMessage(hwndReplaceDlg, IDC_COMBO_TEXFIND, WM_SETTEXT, 0, (LPARAM)tw->name.data());
				}
				else if (sampleState == TTSS_REPLACE)
				{
					SendDlgItemMessage(hwndReplaceDlg, IDC_COMBO_TEXREPLACE, WM_SETTEXT, 0, (LPARAM)tw->name.data());
				}
				//Crosshair(false); // only works in MOUSEMOVE messages
				hot = false;
				sampleState = TTSS_NONE;
				return true;
			}

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
		switch (LOWORD(wParam))
		{
		case ID_TEXTURES_HIDEUNUSED:
			WndMain_SetInspectorMode(W_TEXTURE);
			//Textures::RefreshUsedStatus();
			g_cfgUI.HideUnusedTextures = !g_cfgUI.HideUnusedTextures;
			g_vTexture.Refresh();
			WndMain_UpdateWindows(W_TEXTURE);
			return true;
		case ID_TEXTURES_REPLACEALL:
			FindTextureDialog();
			return true;
		case ID_TEXTURES_RELOAD:
			Sys_BeginWait();
			WndMain_SetInspectorMode(W_TEXTURE);
			Textures::MenuReloadAll();
			WndMain_UpdateWindows(W_TEXTURE);
			break;

		case ID_TEXTURES_RESETSCALE:	// sikk - Reset Texture View Scale
			g_vTexture.SetScale(1.0f);
			WndMain_UpdateWindows(W_TEXTURE);
			break;

		case ID_TEXTURES_LOCK:
			g_qeglobals.d_bTextureLock ^= true;
			WndMain_UpdateWindows(W_CAMERA);
			WndMain_UpdateGridStatusBar();
			break;
		case ID_TEXTURES_DEFAULTSCALE:	// sikk - Default Texture Scale Dialog
			DoDefaultTexScale();
			break;
		case ID_TEXTURES_ADDWAD:
			DoAddWad();
			break;
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

INT_PTR CALLBACK FindTextureDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	assert(g_texTool);
	return g_texTool->InputReplaceDlg(hwndDlg, uMsg, wParam, lParam);
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
			SendDlgItemMessage(hwndReplaceDlg, IDC_COMBO_TEXFIND, CB_ADDSTRING, 0, (LPARAM)fhIt->data());
		for (auto rhIt = replaceHistory.rbegin(); rhIt != replaceHistory.rend(); ++rhIt)
			SendDlgItemMessage(hwndReplaceDlg, IDC_COMBO_TEXREPLACE, CB_ADDSTRING, 0, (LPARAM)rhIt->data());

		SetFocus(GetDlgItem(hwndReplaceDlg, IDC_COMBO_TEXFIND));
		SetDialogText(hwndReplaceDlg, IDC_COMBO_TEXFIND, texdef->name);
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
			WndMain_Status("Click a texture or brush to use its name", 0);
			hot = true;
			Crosshair(true);
			return TRUE;
		case IDC_TEXREPLACE_SAMPLE:
			sampleState = TTSS_REPLACE;
			WndMain_Status("Click a texture or brush to use its name", 0);
			hot = true;
			Crosshair(true);
			return TRUE;

		case IDAPPLY:
			UpdateFindReplaceHistories();
			bSelected = SendDlgItemMessage(hwndReplaceDlg, IDC_CHECK_SELECTED, BM_GETCHECK, 0, 0) > 0;
			Surface::FindReplace(findHistory.back().data(), replaceHistory.back().data(), bSelected);// , false);
			WndMain_UpdateWindows(W_CAMERA);
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

void TextureTool::FindTextureDialog(Texture* f)
{
	if (hwndReplaceDlg)
	{
		SetFocus(hwndReplaceDlg);
		return;
	}

	// lunaran: modeless f&r dialog
	hwndReplaceDlg = CreateDialog(g_qeglobals.d_hInstance, MAKEINTRESOURCE(IDD_FINDREPLACE), g_hwndMain, FindTextureDlgProc);
	ShowWindow(hwndReplaceDlg, SW_SHOW);
	if (f)
		SendDlgItemMessage(hwndReplaceDlg, IDC_COMBO_TEXFIND, WM_SETTEXT, 0, (LPARAM)f->name.data());
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
	if (!Selection::NumFaces() && !Selection::HasBrushes())
		return;

	GetTexModCommand(TM_SHIFT);
	lastTexMod->Shift(x, y);

	if (lastTexMod->state == Command::LIVE)
		g_cmdQueue.Complete(lastTexMod);
	lastTexModTime = clock();
}

void TextureTool::ScaleTexture(float x, float y)
{
	if (!Selection::NumFaces() && !Selection::HasBrushes())
		return;

	GetTexModCommand(TM_SCALE);
	lastTexMod->Scale(x, y);

	if (lastTexMod->state == Command::LIVE)
		g_cmdQueue.Complete(lastTexMod);
	lastTexModTime = clock();
}

void TextureTool::RotateTexture(float r)
{
	if (!Selection::NumFaces() && !Selection::HasBrushes())
		return;

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
//	else
//		Log::Print("combining texmod\n");
}


// ================================================================


bool TextureTool::SelectWadDlg(std::string& outstr)
{
	OPENFILENAME ofn;			// common dialog box structure
	char szFile[_MAX_PATH];		// filename string
	char outFile[_MAX_PATH];
	char szFileTitle[_MAX_FNAME];// file title string

	szFile[0] = 0;
	ofn = { 0 };
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = g_hwndMain;
	ofn.lpstrFilter = "QuakeEd Wad File (*.wad)\0*.wad\0\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFileTitle = szFileTitle;
	ofn.nMaxFileTitle = sizeof(szFileTitle);
	ofn.lpstrInitialDir = g_project.wadPath.data();
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_LONGNAMES | OFN_EXPLORER;
	ofn.lpstrTitle = "Select Wad File(s)";

	if (GetOpenFileName(&ofn))
	{
		if (strlen(ofn.lpstrFile))
		{
			char* rel, * lastSlash;
			rel = szFile;
			lastSlash = szFile;
			for (unsigned i = 0; i < g_project.wadPath.length(); i++)
			{
				if (*rel == '\\') while (*(rel + 1) == '\\')
					rel++;
				if (*rel == '\\')
				{
					*rel = '/';
					lastSlash = rel;
				}
				if (*rel != g_project.wadPath[i])
					break;
				rel++;
			}
			lastSlash++;
			char* outc = outFile;
			char* srcc = lastSlash;
			while (*srcc)
			{
				if (*srcc == '\\' || * srcc == '/')
				{
					*outc = '/';
					while (*srcc == '\\' || *srcc == '/')
						srcc++;
					outc++;
					continue;
				}
				*outc++ = *srcc++;
			}
			*outc = 0;
			outstr = outFile;
			return true;
		}
	}
	return false;
}

void TextureTool::DoAddWad()
{
	std::string wadfile, wadstr;
	if (SelectWadDlg(wadfile))
	{
		Sys_BeginWait();
		Textures::LoadWad(wadfile);

		// TODO: don't ask if wad name is already in the key
		// ask after to add to world wad key
		if (MessageBox(g_hwndMain, "Add wad to worldspawn key?", "QuakeEd 3", MB_YESNO | MB_ICONQUESTION) == IDYES)
		{
			Textures::MergeWadStrings(g_map.world->GetKeyValue("wad"), wadfile, wadstr);

			CmdSetKeyvalue* cmdKV = new CmdSetKeyvalue("wad", wadstr);
			cmdKV->AddEntity(g_map.world);
			g_cmdQueue.Complete(cmdKV);
		}

		g_map.BuildBrushData();
		Sys_EndWait();
		WndMain_SetInspectorMode(W_TEXTURE);
	}
}