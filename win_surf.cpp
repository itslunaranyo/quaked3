//==============================
//	win_surf.c
//==============================

#include "qe3.h"

/*
=====================================================================

SURFACE INSPECTOR

=====================================================================
*/

texdef_t	g_texdefEdit;
int			g_nEditSurfMixed;


/*
==============
SurfWnd_ClearEditTexdef
===============
*/
void SurfWnd_ClearEditTexdef()
{
	memset(&g_texdefEdit, 0, sizeof(g_texdefEdit));
	g_nEditSurfMixed = 0;
}

/*
==============
SurfWnd_AddToEditTexdef
===============
*/
void SurfWnd_AddToEditTexdef(Face* f)
{
	// it either matches the value we already have, or it's a mixed field and thus blank
	if (!(g_nEditSurfMixed & SURF_MIXEDNAME) && (strcmp(f->texdef.name, g_texdefEdit.name)))
		g_nEditSurfMixed |= SURF_MIXEDNAME;

	if (!(g_nEditSurfMixed & SURF_MIXEDSHIFTX) && f->texdef.shift[0] != g_texdefEdit.shift[0])
		g_nEditSurfMixed |= SURF_MIXEDSHIFTX;
	if (!(g_nEditSurfMixed & SURF_MIXEDSHIFTY) && f->texdef.shift[1] != g_texdefEdit.shift[1])
		g_nEditSurfMixed |= SURF_MIXEDSHIFTY;

	if (!(g_nEditSurfMixed & SURF_MIXEDSCALEX) && f->texdef.scale[0] != g_texdefEdit.scale[0])
		g_nEditSurfMixed |= SURF_MIXEDSCALEX;
	if (!(g_nEditSurfMixed & SURF_MIXEDSCALEY) && f->texdef.scale[1] != g_texdefEdit.scale[1])
		g_nEditSurfMixed |= SURF_MIXEDSCALEY;

	if (!(g_nEditSurfMixed & SURF_MIXEDROTATE) && f->texdef.rotate != g_texdefEdit.rotate)
		g_nEditSurfMixed |= SURF_MIXEDROTATE;
}

/*
==============
SurfWnd_RefreshEditTexdef

Union the texdefs of every face in the selection
===============
*/
void SurfWnd_RefreshEditTexdef()
{
	if (!g_qeglobals.d_hwndSurfaceDlg)
		return;

	SurfWnd_ClearEditTexdef();

	if (Selection::FaceCount())
	{
		for (int i = 0; i < Selection::FaceCount(); i++)
		{
			if (i == 0)
				g_texdefEdit = g_vfSelectedFaces[i]->texdef;
			else
				SurfWnd_AddToEditTexdef(g_vfSelectedFaces[i]);
		}
		return;
	}

	if (Selection::HasBrushes())
	{
		bool first = true;
		for (Brush *b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
		{
			for (Face *f = b->basis.faces; f; f = f->fnext)
			{
				if (first)
				{
					g_texdefEdit = f->texdef;
					first = false;
					continue;
				}
				SurfWnd_AddToEditTexdef(f);
			}
		}
		return;
	}

	g_texdefEdit = g_qeglobals.d_workTexDef;
}

/*
==============
prettyftoa
===============
*/
void prettyftoa(char* sz, float f)
{
	// lunaran: trunc safety
	float fp = f + ((f < 0) ? -0.0001f : 0.0001f);

	sprintf(sz, "%4.3f", fp);

	// hack off trailing zeros
	int l = strlen(sz) - 1;
	while (sz[l] == '0' || sz[l] == '.')
		sz[l--] = 0;
}

/*
==============
SurfWnd_FromEditTexdef

Set the window fields to mirror the edit texdef
===============
*/
void SurfWnd_FromEditTexdef()
{
	char		sz[16];
	texdef_t   *texdef;
	float shiftxp, shiftyp, rotp;

	// sikk - So Dialog is updated with texture info from first selected face
	texdef = &g_texdefEdit;

	SendMessage(g_qeglobals.d_hwndSurfaceDlg, WM_SETREDRAW, 0, 0);

	if (g_nEditSurfMixed & SURF_MIXEDNAME)
		SetDlgItemText(g_qeglobals.d_hwndSurfaceDlg, IDC_EDIT_TEXTURE, "");
	else
		SetDlgItemText(g_qeglobals.d_hwndSurfaceDlg, IDC_EDIT_TEXTURE, texdef->name);

	// lunaran: trunc safety
	shiftxp = texdef->shift[0] + ((texdef->shift[0] < 0) ? -0.01f : 0.01f);
	shiftyp = texdef->shift[1] + ((texdef->shift[1] < 0) ? -0.01f : 0.01f);
	rotp = texdef->rotate + ((texdef->rotate < 0) ? -0.01f : 0.01f);

	if (g_nEditSurfMixed & SURF_MIXEDSHIFTX)
		sz[0] = 0;
	else
		sprintf(sz, "%d", (int)shiftxp);
	SetDlgItemText(g_qeglobals.d_hwndSurfaceDlg, IDC_EDIT_HSHIFT, sz);

	if (g_nEditSurfMixed & SURF_MIXEDSHIFTY)
		sz[0] = 0;
	else
		sprintf(sz, "%d", (int)shiftyp);
	SetDlgItemText(g_qeglobals.d_hwndSurfaceDlg, IDC_EDIT_VSHIFT, sz);

	if (g_nEditSurfMixed & SURF_MIXEDSCALEX)
		sz[0] = 0;
	else
		prettyftoa(sz, texdef->scale[0]);
	SetDlgItemText(g_qeglobals.d_hwndSurfaceDlg, IDC_EDIT_HSCALE, sz);

	if (g_nEditSurfMixed & SURF_MIXEDSCALEY)
		sz[0] = 0;
	else
		prettyftoa(sz, texdef->scale[1]);
	SetDlgItemText(g_qeglobals.d_hwndSurfaceDlg, IDC_EDIT_VSCALE, sz);

	if (g_nEditSurfMixed & SURF_MIXEDROTATE)
		sz[0] = 0;
	else
		sprintf(sz, "%d", (int)rotp);
	SetDlgItemText(g_qeglobals.d_hwndSurfaceDlg, IDC_EDIT_ROTATE, sz);

	SendMessage(g_qeglobals.d_hwndSurfaceDlg, WM_SETREDRAW, 1, 0);
	InvalidateRect(g_qeglobals.d_hwndSurfaceDlg, NULL, TRUE);
}


/*
==============
SurfWnd_Apply

Reads the window fields and changes relevant texdef members on selection
===============
*/
void SurfWnd_Apply()
{
	char		sz[64];
	texdef_t	texdef;
	int			mixed = 0;

	GetDlgItemText(g_qeglobals.d_hwndSurfaceDlg, IDC_EDIT_TEXTURE, sz, 64);
	strncpy(texdef.name, sz, sizeof(texdef.name));// -1);
	if (texdef.name[0] <= ' ')
		mixed |= SURF_MIXEDNAME;

	GetDlgItemText(g_qeglobals.d_hwndSurfaceDlg, IDC_EDIT_HSHIFT, sz, 64);
	if (sz[0] <= ' ')
		mixed |= SURF_MIXEDSHIFTX;
	texdef.shift[0] = atof(sz);

	GetDlgItemText(g_qeglobals.d_hwndSurfaceDlg, IDC_EDIT_VSHIFT, sz, 64);
	if (sz[0] <= ' ')
		mixed |= SURF_MIXEDSHIFTY;
	texdef.shift[1] = atof(sz);


	GetDlgItemText(g_qeglobals.d_hwndSurfaceDlg, IDC_EDIT_HSCALE, sz, 64);
	if (sz[0] <= ' ')
		mixed |= SURF_MIXEDSCALEX;
	texdef.scale[0] = atof(sz);

	GetDlgItemText(g_qeglobals.d_hwndSurfaceDlg, IDC_EDIT_VSCALE, sz, 127);
	if (sz[0] <= ' ')
		mixed |= SURF_MIXEDSCALEX;
	texdef.scale[1] = atof(sz);


	GetDlgItemText(g_qeglobals.d_hwndSurfaceDlg, IDC_EDIT_ROTATE, sz, 64);
	if (sz[0] <= ' ')
		mixed |= SURF_MIXEDROTATE;
	texdef.rotate = atof(sz);

	// every field in the inspector is mixed (blank), so this won't do anything anyway
	if (mixed == SURF_MIXEDALL)
		return;

	Surf_SetTexdef(&texdef, mixed);
}

/*
==============
SurfWnd_UpdateUI
===============
*/
void SurfWnd_UpdateUI()
{
	if (!g_qeglobals.d_hwndSurfaceDlg)
		return;

	SurfWnd_RefreshEditTexdef();
	SurfWnd_FromEditTexdef();
}

/*
=================
SurfWnd_UpdateFit
=================
*/
void SurfWnd_UpdateFit(int idc, float dif)
{
	char sz[8];
	float num;

	GetDlgItemText(g_qeglobals.d_hwndSurfaceDlg, idc, sz, 8);
	num = atof(sz) + dif;
	prettyftoa(sz, num);
	SetDlgItemText(g_qeglobals.d_hwndSurfaceDlg, idc, sz);
}

/*
=================
SurfWnd_UpdateSpinners
=================
*/
void SurfWnd_UpdateSpinners(WPARAM wParam, LPARAM lParam)
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
		
			Surf_ShiftTexture(i, 0);
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

			Surf_ShiftTexture(0, i);
			break;

		case IDC_SPIN_HSCALE:
			if (GetKeyState(VK_SHIFT) < 0)
				i = 25;
			else if (GetKeyState(VK_CONTROL) < 0)
				i = 1;
			else
				i = 5;
			if (((LPNMUPDOWN)lParam)->iDelta > 0)
				i *= -1;

			Surf_ScaleTexture(i, 0);
			break;

		case IDC_SPIN_VSCALE:
			if (GetKeyState(VK_SHIFT) < 0)
				i = 25;
			else if (GetKeyState(VK_CONTROL) < 0)
				i = 1;
			else
				i = 5;
			if (((LPNMUPDOWN)lParam)->iDelta > 0)
				i *= -1;

			Surf_ScaleTexture(0,i);
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

			Surf_RotateTexture(i);
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

			SurfWnd_UpdateFit(IDC_EDIT_HFIT, f);
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

			SurfWnd_UpdateFit(IDC_EDIT_WFIT, f);
			break;
		}
	}
}

/*
============
SurfaceDlgProc
============
*/
BOOL CALLBACK SurfaceDlgProc(
	HWND	hwndDlg,// handle to dialog box
	UINT	uMsg,	// message
	WPARAM	wParam,	// first message parameter
	LPARAM	lParam 	// second message parameter
	)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		g_qeglobals.d_hwndSurfaceDlg = hwndDlg;
		SetDlgItemText(hwndDlg, IDC_EDIT_HFIT, "1.0");
		SetDlgItemText(hwndDlg, IDC_EDIT_WFIT, "1.0");
		SurfWnd_UpdateUI();
		return TRUE;

	case WM_CLOSE:
		SurfWnd_Close();
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDAPPLY:
			SurfWnd_Apply();
			Sys_UpdateWindows(W_CAMERA);
			return TRUE;

		case IDCLOSE:
		case IDCANCEL:
			SurfWnd_Close();
			return TRUE;

		case IDC_BUTTON_FIT:
			{
				char sz[8];
				float nHeight, nWidth;

				GetDlgItemText(hwndDlg, IDC_EDIT_HFIT, sz, 8);
				nHeight = atof(sz);
				GetDlgItemText(hwndDlg, IDC_EDIT_WFIT, sz, 8);
				nWidth = atof(sz);

				Surf_FitTexture(nHeight, nWidth);
				SurfWnd_UpdateUI();
				Sys_UpdateWindows(W_CAMERA);
			}
			break;
		}
		return 0;

	case WM_NOTIFY:
		SurfWnd_UpdateSpinners(wParam, lParam);
		SurfWnd_UpdateUI();
		Sys_UpdateWindows(W_CAMERA);
		return 0;
	}
	return FALSE; // eerie
}

/*
============
SurfWnd_Create
============
*/
void SurfWnd_Close()
{
	EndDialog(g_qeglobals.d_hwndSurfaceDlg, 0);
	g_qeglobals.d_hwndSurfaceDlg = NULL;
}

/*
============
SurfWnd_Create
============
*/
void SurfWnd_Create()
{
	if (g_qeglobals.d_hwndSurfaceDlg)
	{
		SetFocus(g_qeglobals.d_hwndSurfaceDlg);
		return;
	}

	// lunaran: modeless surface editor
	CreateDialog(g_qeglobals.d_hInstance, MAKEINTRESOURCE(IDD_SURFACE), g_qeglobals.d_hwndMain, SurfaceDlgProc);
}
