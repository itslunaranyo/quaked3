//==============================
//	WndSurf.c
//==============================

#include "pre.h"
#include "qe3.h"
#include "win_dlg.h"
#include "WndSurf.h"
#include "TextureTool.h"
#include "select.h"
#include "surface.h"


/*
=====================================================================

SURFACE INSPECTOR

=====================================================================
*/

HWND		g_hwndSurfaceDlg;
float		g_fTexFitW = 1.0f, g_fTexFitH = 1.0f;
TexDef		g_texdefEdit;
unsigned	g_nEditSurfMixed;

/*
==============
WndSurf_ClearEditTexdef
===============
*/
void WndSurf_ClearEditTexdef()
{
	memset(&g_texdefEdit, 0, sizeof(g_texdefEdit));
	g_nEditSurfMixed = 0;
}

/*
==============
WndSurf_AddToEditTexdef
===============
*/
void WndSurf_AddToEditTexdef(Face* f)
{
	// it either matches the value we already have, or it's a mixed field and thus blank
	if (!(g_nEditSurfMixed & SFI_NAME) && (f->texdef.name != g_texdefEdit.name))
		g_nEditSurfMixed |= SFI_NAME;

	if (!(g_nEditSurfMixed & SFI_SHIFTX) && f->texdef.shift[0] != g_texdefEdit.shift[0])
		g_nEditSurfMixed |= SFI_SHIFTX;
	if (!(g_nEditSurfMixed & SFI_SHIFTY) && f->texdef.shift[1] != g_texdefEdit.shift[1])
		g_nEditSurfMixed |= SFI_SHIFTY;

	if (!(g_nEditSurfMixed & SFI_SCALEX) && f->texdef.scale[0] != g_texdefEdit.scale[0])
		g_nEditSurfMixed |= SFI_SCALEX;
	if (!(g_nEditSurfMixed & SFI_SCALEY) && f->texdef.scale[1] != g_texdefEdit.scale[1])
		g_nEditSurfMixed |= SFI_SCALEY;

	if (!(g_nEditSurfMixed & SFI_ROTATE) && f->texdef.rotate != g_texdefEdit.rotate)
		g_nEditSurfMixed |= SFI_ROTATE;
}

/*
==============
WndSurf_RefreshEditTexdef

Union the texdefs of every face in the selection
===============
*/
void WndSurf_RefreshEditTexdef()
{
	if (!g_hwndSurfaceDlg)
		return;

	WndSurf_ClearEditTexdef();
	g_texdefEdit = g_qeglobals.d_workTexDef;

	if (Selection::NumFaces())
	{
		for (auto fIt = Selection::faces.begin(); fIt != Selection::faces.end(); ++fIt)
		{
			if (fIt == Selection::faces.begin())
				g_texdefEdit = (*fIt)->texdef;
			else
				WndSurf_AddToEditTexdef((*fIt));
		}
		return;
	}

	if (Selection::HasBrushes())
	{
		bool first = true;
		for (Brush *b = g_brSelectedBrushes.Next(); b != &g_brSelectedBrushes; b = b->Next())
		{
			if (b->owner->IsPoint())
				continue;
			for (Face *f = b->faces; f; f = f->fnext)
			{
				if (first)
				{
					g_texdefEdit = f->texdef;
					first = false;
					continue;
				}
				WndSurf_AddToEditTexdef(f);
			}
		}
		return;
	}
}


/*
==============
WndSurf_FromEditTexdef

Set the window fields to mirror the edit texdef
===============
*/
void WndSurf_FromEditTexdef()
{
	TexDef  *texdef;

	// sikk - So Dialog is updated with texture info from first selected face
	texdef = &g_texdefEdit;
	texdef->name[MAX_TEXNAME - 1] = 0;

	SendMessage(g_hwndSurfaceDlg, WM_SETREDRAW, 0, 0);

	if (g_nEditSurfMixed & SFI_NAME)
		SetDlgItemText(g_hwndSurfaceDlg, IDC_EDIT_TEXTURE, "");
	else
		SetDialogText(g_hwndSurfaceDlg, IDC_EDIT_TEXTURE, texdef->name);

	// lunaran: trunc safety
	//shiftxp = texdef->shift[0] + ((texdef->shift[0] < 0) ? -0.01f : 0.01f);
	//shiftyp = texdef->shift[1] + ((texdef->shift[1] < 0) ? -0.01f : 0.01f);
	//rotp = texdef->rotate + ((texdef->rotate < 0) ? -0.01f : 0.01f);

	if (g_nEditSurfMixed & SFI_SHIFTX)
		SetDlgItemText(g_hwndSurfaceDlg, IDC_EDIT_HSHIFT, "");
	else
		SetDialogFloat(g_hwndSurfaceDlg, IDC_EDIT_HSHIFT, texdef->shift[0]);

	if (g_nEditSurfMixed & SFI_SHIFTY)
		SetDlgItemText(g_hwndSurfaceDlg, IDC_EDIT_VSHIFT, "");
	else
		SetDialogFloat(g_hwndSurfaceDlg, IDC_EDIT_VSHIFT, texdef->shift[1]);

	if (g_nEditSurfMixed & SFI_SCALEX)
		SetDlgItemText(g_hwndSurfaceDlg, IDC_EDIT_HSCALE, "");
	else
		SetDialogFloat(g_hwndSurfaceDlg, IDC_EDIT_HSCALE, texdef->scale[0]);

	if (g_nEditSurfMixed & SFI_SCALEY)
		SetDlgItemText(g_hwndSurfaceDlg, IDC_EDIT_VSCALE, "");
	else
		SetDialogFloat(g_hwndSurfaceDlg, IDC_EDIT_VSCALE, texdef->scale[1]);

	if (g_nEditSurfMixed & SFI_ROTATE)
		SetDlgItemText(g_hwndSurfaceDlg, IDC_EDIT_ROTATE, "");
	else
		SetDialogFloat(g_hwndSurfaceDlg, IDC_EDIT_ROTATE, texdef->rotate);

	SendMessage(g_hwndSurfaceDlg, WM_SETREDRAW, 1, 0);
	InvalidateRect(g_hwndSurfaceDlg, NULL, TRUE);
}


/*
==============
WndSurf_Apply

Reads the window fields and changes relevant texdef members on selection
===============
*/
void WndSurf_Apply()
{
	char		sz[MAX_TEXNAME];
	std::string	name;
	TexDef		texdef;
	unsigned	mixed = 0;

	texdef.name = GetDialogText(g_hwndSurfaceDlg, IDC_EDIT_TEXTURE);
	if (texdef.name[0] <= ' ')
	{
		texdef.tex = nullptr;
		mixed |= SFI_NAME;
	}
	else
	{
		texdef.tex = Textures::ForName(texdef.name);
	}

	GetDlgItemText(g_hwndSurfaceDlg, IDC_EDIT_HSHIFT, sz, 64);
	if (sz[0] <= ' ')
		mixed |= SFI_SHIFTX;
	texdef.shift[0] = atof(sz);

	GetDlgItemText(g_hwndSurfaceDlg, IDC_EDIT_VSHIFT, sz, 64);
	if (sz[0] <= ' ')
		mixed |= SFI_SHIFTY;
	texdef.shift[1] = atof(sz);


	GetDlgItemText(g_hwndSurfaceDlg, IDC_EDIT_HSCALE, sz, 64);
	if (sz[0] <= ' ')
		mixed |= SFI_SCALEX;
	texdef.scale[0] = atof(sz);

	GetDlgItemText(g_hwndSurfaceDlg, IDC_EDIT_VSCALE, sz, 127);
	if (sz[0] <= ' ')
		mixed |= SFI_SCALEX;
	texdef.scale[1] = atof(sz);


	GetDlgItemText(g_hwndSurfaceDlg, IDC_EDIT_ROTATE, sz, 64);
	if (sz[0] <= ' ')
		mixed |= SFI_ROTATE;
	texdef.rotate = atof(sz);

	// every field in the inspector is mixed (blank), so this won't do anything anyway
	if (mixed == SFI_ALL)
		return;

	Surface::SetTexdef(texdef, mixed);
}

/*
=================
WndSurf_UpdateFit
=================
*/
void WndSurf_UpdateFit(int idc, float dif)
{
	char sz[8];
	float num;

	GetDlgItemText(g_hwndSurfaceDlg, idc, sz, 8);
	num = atof(sz) + dif;
	FloatToString(num, sz);
	SetDlgItemText(g_hwndSurfaceDlg, idc, sz);
}

/*
=================
WndSurf_UpdateSpinners
=================
*/
void WndSurf_UpdateSpinners(WPARAM wParam, LPARAM lParam)
{
	int i;
	float f;

	// sikk---> Added Modifiers
	switch (((LPNMHDR)lParam)->code)
	{
	case UDN_DELTAPOS:
		switch ((int)wParam)
		{
		case IDC_SPIN_HSHIFT:
			if (GetKeyState(VK_SHIFT) < 0)
				i = 32;
			else if (GetKeyState(VK_CONTROL) < 0)
				i = 1;
			else
				i = 8;
			if (((LPNMUPDOWN)lParam)->iDelta > 0)
				i *= -1;
		
			//Surf_ShiftTexture(i, 0);
			g_texTool->ShiftTexture(i, 0);
			break;

		case IDC_SPIN_VSHIFT:
			if (GetKeyState(VK_SHIFT) < 0)
				i = 32;
			else if (GetKeyState(VK_CONTROL) < 0)
				i = 1;
			else
				i = 8;
			if (((LPNMUPDOWN)lParam)->iDelta > 0)
				i *= -1;

			//Surf_ShiftTexture(0, i);
			g_texTool->ShiftTexture(0, i);
			break;

		case IDC_SPIN_HSCALE:
			if (GetKeyState(VK_SHIFT) < 0)
				f = 0.25f;
			else if (GetKeyState(VK_CONTROL) < 0)
				f = 0.01f;
			else
				f = 0.05f;
			if (((LPNMUPDOWN)lParam)->iDelta > 0)
				f *= -1.0f;

			//Surf_ScaleTexture(i, 0);
			g_texTool->ScaleTexture(f, 0);
			break;

		case IDC_SPIN_VSCALE:
			if (GetKeyState(VK_SHIFT) < 0)
				f = 0.25f;
			else if (GetKeyState(VK_CONTROL) < 0)
				f = 0.01f;
			else
				f = 0.05f;
			if (((LPNMUPDOWN)lParam)->iDelta > 0)
				f *= -1.0f;

			//Surf_ScaleTexture(0,i);
			g_texTool->ScaleTexture(0, f);
			break;

		case IDC_SPIN_ROTATE:
			if (GetKeyState(VK_SHIFT) < 0)
				i = 90;
			else if (GetKeyState(VK_CONTROL) < 0)
				i = 1;
			else
				i = 15;
			if (((LPNMUPDOWN)lParam)->iDelta > 0)
				i *= -1;

			//Surf_RotateTexture(i);
			g_texTool->RotateTexture(i);
			break;

		case IDC_SPIN_HFIT:
			if (GetKeyState(VK_SHIFT) < 0)
				f = 0.25f;
			else if (GetKeyState(VK_CONTROL) < 0)
				f = 0.01f;
			else
				f = 0.05f;
			if (((LPNMUPDOWN)lParam)->iDelta > 0)
				f *= -1;

			WndSurf_UpdateFit(IDC_EDIT_HFIT, f);
			break;

		case IDC_SPIN_WFIT:
			if (GetKeyState(VK_SHIFT) < 0)
				f = 0.25f;
			else if (GetKeyState(VK_CONTROL) < 0)
				f = 0.01f;
			else
				f = 0.05f;
			if (((LPNMUPDOWN)lParam)->iDelta > 0)
				f *= -1;

			WndSurf_UpdateFit(IDC_EDIT_WFIT, f);
			break;
		}
	}
}

/*
============
SurfaceDlgProc
============
*/
INT_PTR CALLBACK SurfaceDlgProc(
	HWND	hwndDlg,// handle to dialog box
	UINT	uMsg,	// message
	WPARAM	wParam,	// first message parameter
	LPARAM	lParam 	// second message parameter
	)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		g_hwndSurfaceDlg = hwndDlg;
		{
			char sz[8];
			FloatToString(g_fTexFitH, sz);
			SetDlgItemText(hwndDlg, IDC_EDIT_HFIT, sz);
			FloatToString(g_fTexFitW, sz);
			SetDlgItemText(hwndDlg, IDC_EDIT_WFIT, sz);
			WndSurf_UpdateUI();
		}
		return TRUE;

	case WM_CLOSE:
		WndSurf_Close();
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDAPPLY:
			WndSurf_Apply();
			WndMain_UpdateWindows(W_CAMERA);
			return TRUE;

		case IDCLOSE:
		case IDCANCEL:
			WndSurf_Close();
			return TRUE;

		case IDC_RADIO_TEXAXIAL:
			g_cfgEditor.TexProjectionMode = TEX_PROJECT_AXIAL;
			return TRUE;
		case IDC_RADIO_TEXFACE:
			g_cfgEditor.TexProjectionMode = TEX_PROJECT_FACE;
			return TRUE;

		case IDC_BUTTON_FIT:
			{
				char sz[8];
				GetDlgItemText(hwndDlg, IDC_EDIT_HFIT, sz, 8);
				g_fTexFitH = atof(sz);
				GetDlgItemText(hwndDlg, IDC_EDIT_WFIT, sz, 8);
				g_fTexFitW = atof(sz);

				g_texTool->FitTexture(g_fTexFitH, g_fTexFitW);
				//WndSurf_UpdateUI();
				WndMain_UpdateWindows(W_CAMERA|W_SURF);
			}
			break;
		case IDC_BUTTON_FITRESET:
			SetDlgItemText(hwndDlg, IDC_EDIT_HFIT, "1.0");
			SetDlgItemText(hwndDlg, IDC_EDIT_WFIT, "1.0");
			break;
		}
		return 0;

	case WM_NOTIFY:
		WndSurf_UpdateSpinners(wParam, lParam);
		//WndSurf_UpdateUI();
		WndMain_UpdateWindows(W_CAMERA | W_SURF);
		return 0;
	}
	return FALSE; // eerie
}


// ========================================================


/*
==============
WndSurf_UpdateUI
===============
*/
void WndSurf_UpdateUI()
{
	if (!g_hwndSurfaceDlg)
		return;

	if (g_cfgEditor.TexProjectionMode == TEX_PROJECT_FACE)
		SendDlgItemMessage(g_hwndSurfaceDlg, IDC_RADIO_TEXFACE, BM_SETCHECK, 1, 0);
	else
		SendDlgItemMessage(g_hwndSurfaceDlg, IDC_RADIO_TEXAXIAL, BM_SETCHECK, 1, 0);

	WndSurf_RefreshEditTexdef();
	WndSurf_FromEditTexdef();
}

/*
============
WndSurf_Create
============
*/
void WndSurf_Close()
{
	char sz[8];
	GetDlgItemText(g_hwndSurfaceDlg, IDC_EDIT_HFIT, sz, 8);
	g_fTexFitH = atof(sz);
	GetDlgItemText(g_hwndSurfaceDlg, IDC_EDIT_WFIT, sz, 8);
	g_fTexFitW = atof(sz);

	EndDialog(g_hwndSurfaceDlg, 0);
	g_hwndSurfaceDlg = NULL;
}

/*
============
WndSurf_Create
============
*/
void WndSurf_Create()
{
	if (g_hwndSurfaceDlg)
	{
		SetFocus(g_hwndSurfaceDlg);
		return;
	}

	// lunaran: modeless surface editor
	CreateDialog(g_qeglobals.d_hInstance, MAKEINTRESOURCE(IDD_SURFACE), g_hwndMain, SurfaceDlgProc);
}
