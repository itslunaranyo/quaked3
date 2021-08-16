//==============================
//	win_dlg.c
//==============================

#include "pre.h"
#include "qe3.h"
#include "win_dlg.h"
#include "select.h"
#include "map.h"
#include "CameraView.h"
#include "GridView.h"
#include "ZView.h"
#include "CmdImportMap.h"
#include "WndEntity.h"
#include "modify.h"
#include "transform.h"
#include "strlib.h"

#pragma warning(disable: 4800)	// 'LRESULT': forcing value to bool 'true' or 'false' (performance warning)

HWND g_hwndSetKeyvalsDlg;


std::string GetDialogText(HWND hDlg, int dlgItem)
{
	char buf[1024];
	GetDlgItemText(hDlg, dlgItem, buf, 1024);
	return std::string(buf);
}
int GetDialogInt(HWND hDlg, int dlgItem)
{
	char buf[64];
	GetDlgItemText(hDlg, dlgItem, buf, 64);
	return atoi(buf);
}
double GetDialogFloat(HWND hDlg, int dlgItem)
{
	char buf[128];
	GetDlgItemText(hDlg, dlgItem, buf, 128);
	return atof(buf);
}

BOOL SetDialogText(HWND hDlg, int dlgItem, std::string& str)
{
	return SetDlgItemText(hDlg, dlgItem, str.data());
}
BOOL SetDialogInt(HWND hDlg, int dlgItem, int i)
{
	char buf[64];
	sprintf_s(buf, "%i", i);
	return SetDlgItemText(hDlg, dlgItem, buf);
}
BOOL SetDialogFloat(HWND hDlg, int dlgItem, double f)
{
	std::string buf = strlib::DoubleToStringNice(f);
	return SetDialogText(hDlg, dlgItem, buf);
}

/*
=====================================================================

	COLOR

=====================================================================
*/

vec3 g_lastColor;

/*
============
DoColorSelect
============
*/
bool DoColorSelect(vec3 &rgbOut)
{
	return DoColorSelect(g_lastColor, rgbOut);
}

bool DoColorSelect(const vec3 rgbIn, vec3 &rgbOut)
{
	CHOOSECOLOR		cc;
	static COLORREF	custom[16];

	cc.lStructSize = sizeof(cc);
	cc.hwndOwner = g_hwndMain;
	cc.lpCustColors = custom;
	cc.Flags = CC_FULLOPEN | CC_RGBINIT;

	cc.rgbResult = (int)(rgbIn.r * 255) +
		(((int)(rgbIn.g * 255)) << 8) +
		(((int)(rgbIn.b * 255)) << 16);

	if (!ChooseColor(&cc))
		return false;

	rgbOut.r = (cc.rgbResult & 255);
	rgbOut.g = ((cc.rgbResult >> 8) & 255);
	rgbOut.b = ((cc.rgbResult >> 16) & 255);
	rgbOut /= 255.0f;
	g_lastColor = rgbOut;
	return true;
}


/*
=====================================================================

	ABOUT

=====================================================================
*/

/*
============
AboutDlgProc
============
*/
INT_PTR CALLBACK AboutDlgProc( 
    HWND	hwndDlg,// handle to dialog box
    UINT	uMsg,	// message
    WPARAM	wParam,	// first message parameter
    LPARAM	lParam 	// second message parameter
   )
{
	switch (uMsg)
    {
	case WM_INITDIALOG:
	{
		char	szTemp[1024];
		char	szExtensions[16384];	// lunaran: was 4096, this should work for another few years
		char	szExtensionscopy[16384];
		char	*psz;

		SetDialogText(hwndDlg, IDC_ABOUT_APPNAME, g_qeAppName);

		sprintf(szTemp, "Renderer:\t%s", glGetString(GL_RENDERER));
		SetDlgItemText(hwndDlg, IDC_ABOUT_GLRENDERER, szTemp);

		sprintf(szTemp, "Version:\t\t%s", glGetString(GL_VERSION));
		SetDlgItemText(hwndDlg, IDC_ABOUT_GLVERSION, szTemp);

		sprintf(szTemp, "Vendor:\t\t%s", glGetString(GL_VENDOR));
		SetDlgItemText(hwndDlg, IDC_ABOUT_GLVENDOR, szTemp);

		strncpy(szExtensions, (char*)glGetString(GL_EXTENSIONS), 8192);
		szExtensionscopy[0] = 0;

		for (psz = strtok(szExtensions, " \n\t"); psz; psz = strtok(NULL, " \n"))
		{
			strcat(szExtensionscopy, psz);
			strcat(szExtensionscopy, "\r\n");
		}

		SetDlgItemText(hwndDlg, IDC_ABOUT_GLEXTENSIONS, szExtensionscopy);
		return TRUE;
	}
	case WM_CLOSE:
		EndDialog(hwndDlg, 0);
		return 0;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			EndDialog(hwndDlg, 1);
			break;
		case IDCANCEL:
			EndDialog(hwndDlg, 0);
			break;
		}
		return 0;
	}
	return FALSE;
}

/*
============
DoAbout
============
*/
void DoAbout ()
{
	DialogBox(g_qeglobals.d_hInstance, MAKEINTRESOURCE(IDD_ABOUT), g_hwndMain, AboutDlgProc);
}


/*
=====================================================================

	FIND BRUSH

=====================================================================
*/

/*
============
FindBrush
============
*/
void FindBrush (int entitynum, int brushnum)
{
	Brush	   *b;
	Entity   *e;

	if (entitynum == 0)
		e = g_map.world;
	else
	{
		e = g_map.entities.Next();
		while (--entitynum)
		{
			e = e->Next();
			if (e == &g_map.entities)
			{
				Log::Warning("No such entity.");
				return;
			}
		}
	}

	b = e->brushes.ENext();
	if (b == &e->brushes)
	{
		Log::Warning("No such brush.");
		return;
	}

	while (brushnum--)
	{
		b = b->ENext();
		if (b == &e->brushes)
		{
			Log::Warning("No such brush.");
			return;
		}
	}

	Selection::SelectBrush(b);

	g_vGrid[0].Center((b->mins + b->maxs) * 0.5f);

	Log::Print("Selected.\n");
}

/*
=================
GetSelectionIndex
=================
*/
void GetSelectionIndex (int *entity, int *brush)
{
	Brush		*b, *b2;
	Entity	*e;

	*entity = *brush = 0;

	b = g_brSelectedBrushes.Next();
	if (b == &g_brSelectedBrushes)
		return;

	// find entity
	if (!b->owner->IsWorld())
	{
		(*entity)++;
		for (e = g_map.entities.Next(); e != &g_map.entities; e = e->Next(), (*entity)++)
			;
	}

	// find brush
	for (b2 = b->owner->brushes.ENext(); (b2 != b) && (b2 != &b->owner->brushes); b2 = b2->ENext(), (*brush)++)
		;
}

/*
============
FindBrushDlgProc
============
*/
INT_PTR CALLBACK FindBrushDlgProc (
    HWND	hwndDlg,// handle to dialog box
    UINT	uMsg,	// message
    WPARAM	wParam,	// first message parameter
    LPARAM	lParam 	// second message parameter
   )
{
	char	szEntity[256], szBrush[256];
	int		nEntity, nBrush;

	switch (uMsg)
    {
	case WM_INITDIALOG:
		// set entity and brush number
		GetSelectionIndex(&nEntity, &nBrush);
		sprintf(szEntity, "%d", nEntity);
		sprintf(szBrush, "%d", nBrush);
		SetDlgItemText(hwndDlg, IDC_EDIT_FINDENTITY, szEntity);
		SetDlgItemText(hwndDlg, IDC_EDIT_FINDBRUSH, szBrush);

		SetFocus(GetDlgItem(hwndDlg, IDC_EDIT_FINDBRUSH));
		return FALSE;

	case WM_COMMAND: 
		switch (LOWORD(wParam)) 
		{ 
		case IDOK:
			GetDlgItemText(hwndDlg, IDC_EDIT_FINDENTITY, szEntity, 255);
			GetDlgItemText(hwndDlg, IDC_EDIT_FINDBRUSH, szBrush, 255);
			FindBrush(atoi(szEntity), atoi(szBrush));
			EndDialog(hwndDlg, 1);
			return TRUE;

		case IDCANCEL:
			EndDialog(hwndDlg, 0);
			return TRUE;
		}
		return 0;
	}
	return FALSE;
}

/*
============
DoFindBrush
============
*/
void DoFindBrush ()
{
	DialogBox(g_qeglobals.d_hInstance, MAKEINTRESOURCE(IDD_FINDBRUSH), g_hwndMain, FindBrushDlgProc);
}	


/*
=====================================================================

	ARBITRARY ROTATE

=====================================================================
*/


/*
============
RotateDlgProc
============
*/
INT_PTR CALLBACK RotateDlgProc (
    HWND	hwndDlg,// handle to dialog box
    UINT	uMsg,	// message
    WPARAM	wParam,	// first message parameter
    LPARAM	lParam 	// second message parameter
   )
{
	int		nRotX, nRotY, nRotZ;	// sikk - Spinners
	float	f;
	char	sz[4], szCheck[8];

	switch (uMsg)
    {
	case WM_INITDIALOG:
		SetFocus(GetDlgItem(hwndDlg, IDC_EDIT_ROTX));
// sikk--->	For more precise rotation
		if (g_qeglobals.bGridSnap)
		{
			g_qeglobals.bGridSnap = false;
			g_bSnapCheck = true;
		}
// <---sikk
		return FALSE;

	case WM_COMMAND: 
		switch (LOWORD(wParam)) 
		{ 
		case IDOK:
			GetDlgItemText(hwndDlg, IDCANCEL, szCheck, 8);
			if (strncmp(szCheck, "Close", 5))
			{
				// lunaran TODO: rotate once instead of issuing multiple commands for a rotate on more than one axis
				GetDlgItemText(hwndDlg, IDC_EDIT_ROTX, sz, 255);
				f = atof(sz);
				if (f)
					Transform_RotateAxis(0, f, false);

				GetDlgItemText(hwndDlg, IDC_EDIT_ROTY, sz, 255);
				f = atof(sz);
				if (f)
					Transform_RotateAxis(1, f, false);

				GetDlgItemText(hwndDlg, IDC_EDIT_ROTZ, sz, 255);
				f = atof(sz);
				if (f)
					Transform_RotateAxis(2, f, false);
			}

// sikk--->	For more precise rotation
			if (g_bSnapCheck)
			{
				g_qeglobals.bGridSnap = true;
				g_bSnapCheck = false;
			}
// <---sikk
			EndDialog(hwndDlg, 1);
			return TRUE;

// sikk---> Added Apply Button
		case IDAPPLY:
			GetDlgItemText(hwndDlg, IDC_EDIT_ROTX, sz, 255);
			f = atof(sz);
			if (f)
				Transform_RotateAxis(0, f, false);

			GetDlgItemText(hwndDlg, IDC_EDIT_ROTY, sz, 255);
			f = atof(sz);
			if (f)
				Transform_RotateAxis(1, f, false);

			GetDlgItemText(hwndDlg, IDC_EDIT_ROTZ, sz, 255);
			f = atof(sz);
			if (f)
				Transform_RotateAxis(2, f, false);

			SetDlgItemText(hwndDlg, IDCANCEL, "Close");
			
			WndMain_ForceUpdateWindows(W_SCENE);

			return TRUE;
// <---sikk

		case IDCANCEL:
// sikk--->	For more precise rotation
			if (g_bSnapCheck)
			{
				g_qeglobals.bGridSnap = true;
				g_bSnapCheck = false;
			}
// <---sikk
			EndDialog(hwndDlg, 0);
			return TRUE;
		}	
		return 0;

// sikk---> Spinners
	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) 
        {
		case UDN_DELTAPOS:
			switch ((int)wParam)
			{
			case IDC_SPIN_ROTX:
				if (((LPNMUPDOWN)lParam)->iDelta < 0) 
				{
					GetDlgItemText(hwndDlg, IDC_EDIT_ROTX, sz, 255);
					nRotX = atoi(sz);
					if (GetKeyState(VK_SHIFT) < 0)
						nRotX += 90;
					else if (GetKeyState(VK_CONTROL) < 0)
						nRotX += 1;
					else
						nRotX += 15;
					if (nRotX >= 360)
						nRotX -= 360;
					sprintf(sz, "%d", nRotX);
					SetDlgItemText(hwndDlg, IDC_EDIT_ROTX, sz);
				}
				else
				{
					GetDlgItemText(hwndDlg, IDC_EDIT_ROTX, sz, 255);
					nRotX = atoi(sz);
					if (GetKeyState(VK_SHIFT) < 0)
						nRotX -= 90;
					else if (GetKeyState(VK_CONTROL) < 0)
						nRotX -= 1;
					else
						nRotX -= 15;
					if (nRotX < 0)
						nRotX += 360;
					sprintf(sz, "%d", nRotX);
					SetDlgItemText(hwndDlg, IDC_EDIT_ROTX, sz);
				}
				break;

			case IDC_SPIN_ROTY:
				if (((LPNMUPDOWN)lParam)->iDelta < 0) 
				{
					GetDlgItemText(hwndDlg, IDC_EDIT_ROTY, sz, 255);
					nRotY = atoi(sz);
					if (GetKeyState(VK_SHIFT) < 0)
						nRotY += 90;
					else if (GetKeyState(VK_CONTROL) < 0)
						nRotY += 1;
					else
						nRotY += 15;
					if (nRotY >= 360)
						nRotY -= 360;
					sprintf(sz, "%d", nRotY);
					SetDlgItemText(hwndDlg, IDC_EDIT_ROTY, sz);
				}
				else
				{
					GetDlgItemText(hwndDlg, IDC_EDIT_ROTY, sz, 255);
					nRotY = atoi(sz);
					if (GetKeyState(VK_SHIFT) < 0)
						nRotY -= 90;
					else if (GetKeyState(VK_CONTROL) < 0)
						nRotY -= 1;
					else
						nRotY -= 15;
					if (nRotY < 0)
						nRotY += 360;
					sprintf(sz, "%d", nRotY);
					SetDlgItemText(hwndDlg, IDC_EDIT_ROTY, sz);
				}
				break;

			case IDC_SPIN_ROTZ:
				if (((LPNMUPDOWN)lParam)->iDelta < 0) 
				{
					GetDlgItemText(hwndDlg, IDC_EDIT_ROTZ, sz, 255);
					nRotZ = atoi(sz);
					if (GetKeyState(VK_SHIFT) < 0)
						nRotZ += 90;
					else if (GetKeyState(VK_CONTROL) < 0)
						nRotZ += 1;
					else
						nRotZ += 15;
					if (nRotZ >= 360)
						nRotZ -= 360;
					sprintf(sz, "%d", nRotZ);
					SetDlgItemText(hwndDlg, IDC_EDIT_ROTZ, sz);
				}
				else
				{
					GetDlgItemText(hwndDlg, IDC_EDIT_ROTZ, sz, 255);
					nRotZ = atoi(sz);
					if (GetKeyState(VK_SHIFT) < 0)
						nRotZ -= 90;
					else if (GetKeyState(VK_CONTROL) < 0)
						nRotZ -= 1;
					else
						nRotZ -= 15;
					if (nRotZ < 0)
						nRotZ += 360;
					sprintf(sz, "%d", nRotZ);
					SetDlgItemText(hwndDlg, IDC_EDIT_ROTZ, sz);
				}
				break;
			}
			return 0;
		}
		return 0;
// <---sikk 
	}

	return FALSE;
}

/*
============
DoRotate
============
*/
void DoRotate ()
{
	DialogBox(g_qeglobals.d_hInstance, MAKEINTRESOURCE(IDD_ROTATE), g_hwndMain, RotateDlgProc);
}

		
/*
=====================================================================

	ARBITRARY SIDES

=====================================================================
*/

int	g_nType = 0;	// sikk - Brush Primitives

/*
============
SidesDlgProc
============
*/
INT_PTR CALLBACK SidesDlgProc (
    HWND	hwndDlg,// handle to dialog box
    UINT	uMsg,	// message
    WPARAM	wParam,	// first message parameter
    LPARAM	lParam 	// second message parameter
   )
{
	HWND	h;
	char	sz[3];
	int		nSides;	// sikk - Spinner

	switch (uMsg)
    {
	case WM_INITDIALOG:
		if (g_nType == 1)
			SetWindowText(hwndDlg, "Arbitrary Sided Cone");
		if (g_nType == 2)
			SetWindowText(hwndDlg, "Arbitrary Sided Sphere");
		h = GetDlgItem(hwndDlg, IDC_EDIT_SIDES);
		SetFocus (h);
// sikk--->	Spinners
		nSides = 3;
		sprintf(sz, "%d", nSides);
		SetDlgItemText(hwndDlg, IDC_EDIT_SIDES, sz);
// <---sikk
// sikk--->	For more precision
		if (g_qeglobals.bGridSnap)
		{
			g_qeglobals.bGridSnap = false;
			g_bSnapCheck = true;
		}
// <---sikk
		return FALSE;

	case WM_COMMAND: 
		switch (LOWORD(wParam)) 
		{ 
		
		case IDOK:
			GetDlgItemText(hwndDlg, IDC_EDIT_SIDES, sz, 255);

// sikk---> Brush Primitives
			switch (g_nType)
			{
			case (1):
				Modify::MakeSidedCone(atoi(sz));
				break;
			case (2):
				Modify::MakeSidedSphere(atoi(sz));
				break;
//			case (3):
//				Brush_MakeSidedTorus(atoi(sz));
//				break;
			default:
				Modify::MakeSided(atoi(sz));
			}
// <---sikk
// sikk--->	For more precision
			if (g_bSnapCheck)
			{
				g_qeglobals.bGridSnap = true;
				g_bSnapCheck = false;
			}
// <---sikk

			EndDialog(hwndDlg, 1);
			break;

		case IDCANCEL:
// sikk--->	For more precision
			if (g_bSnapCheck)
			{
				g_qeglobals.bGridSnap = true;
				g_bSnapCheck = false;
			}
// <---sikk
			EndDialog(hwndDlg, 0);
			break;
		}
		return 0;

// sikk---> Spinners	
	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) 
        {
		case UDN_DELTAPOS:
			if (((LPNMUPDOWN)lParam)->iDelta < 0)
			{
				GetDlgItemText(hwndDlg, IDC_EDIT_SIDES, sz, 255);
				nSides = atoi(sz);
				nSides += 1;
				sprintf(sz, "%d", nSides);
				SetDlgItemText(hwndDlg, IDC_EDIT_SIDES, sz);
			}
			else
			{
				GetDlgItemText(hwndDlg, IDC_EDIT_SIDES, sz, 255);
				nSides = atoi(sz);
				if (nSides > 3)
					nSides -= 1;
				else
					nSides = 3;
				sprintf(sz, "%d", nSides);
				SetDlgItemText(hwndDlg, IDC_EDIT_SIDES, sz);
			}
			return 0;
		}
		return 0;
// <---sikk	

	default:
		return FALSE;
	}
}

/*
============
DoSides
============
*/
void DoSides (int nType)	// sikk - Brush Primitives (previously took no arguments)
{
	g_nType = nType;	// sikk - Brush Primitives
	DialogBox(g_qeglobals.d_hInstance, MAKEINTRESOURCE(IDD_SIDES), g_hwndMain, SidesDlgProc);
}		



/*
=====================================================================

	KEYLIST/MOUSELIST

=====================================================================
*/

/*
============
KeylistDlgProc
============
*/
INT_PTR CALLBACK MouselistDlgProc(
	HWND	hwndDlg,// handle to dialog box
	UINT	uMsg,	// message
	WPARAM	wParam,	// first message parameter
	LPARAM	lParam 	// second message parameter
)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		SetDlgItemText(hwndDlg, IDC_EDIT_MOUSELIST,
			"XY Window\r\n"
			"============\r\n"
			"LMB\t\tDrag Brush/Face\r\n"
			"\t\tCreate Brush\r\n"
			"\t\tSelect/Drag Edge/Vertex/Face (V/E/F Mode)\r\n"
			"\t\tPlace/Drag Clip Point (Clip Mode)\r\n"
			"LMB+Ctrl\t\tShear Drag Face\r\n"
			"LMB+Shift\tSelect Brush\r\n"
			"LMB+Shift+Alt\tDrill-Select Single Brush\r\n"
			"LMB+Ctrl+Alt\tJump Drag Selection\r\n"
			"LMB+Alt\t\tQuick-Clip (Clip Mode)\r\n"
			"\r\n"
			"RMB\t\tDrag View\r\n"
			"\t\tContext Menu\r\n"
			"RMB+Ctrl\t\tZoom View\r\n"
			"RMB+Shift\tFree Scale Brush\r\n"
			"RMB+Alt\t\tFree Rotate Brush\r\n"
			"\r\n"
			"MMB\t\tAim Camera\r\n"
			"MMB+Ctrl\t\tPlace Camera\r\n"
			"MMB+Shift\tDrag Z-Checker Axis\r\n"
			"\r\n"
			"MHWL\t\tZoom\r\n"
			"\r\n"
			"\r\n"
			"Camera Window\r\n"
			"============\r\n"
			"LMB\t\tDrag Brush/Face\r\n"
			"\t\tSelect/Drag Edge/Vertex/Face (V/E/F Mode)\r\n"
			"\t\tPlace/Drag Clip Point (Clip Mode)\r\n"
			"LMB+Ctrl\t\tShear Drag Face\r\n"
			"LMB+Shift\tSelect\r\n"
			"LMB+Shift+Alt\tDrill-Select Single Brush\r\n"
			"LMB+Ctrl+Shift\tSelect Faces\r\n"
			"LMB+Ctrl+Shift+Alt\tSelect Single Face\r\n"
			"LMB+Alt\t\tQuick-Clip (Clip Mode)\r\n"
			"\r\n"
			"RMB\t\tCamera Drive Mode (Forward/Back/Turn Left/Turn Right)\r\n"
			"RMB+Ctrl\t\tDrag Camera Position (StrafeUp/Down/Left/Right)\r\n"
			"RMB+Shift\tFree Orbit Camera\r\n"
			"RMB+Ctrl+Shift\tFree Look Camera\r\n"
			"\r\n"
			"MMB\t\tGrab Texture+Alignment from Face, Bounds from Brush\r\n"
			"MMB+Ctrl\t\tApply Current Texture+Alignment to Brush\r\n"
			"MMB+Ctrl+Shift\tApply Current Texture+Alignment to Face\r\n"
			"MMB+Shift\tApply Current Texture to Face\r\n"
			"\r\n"
			"MWHL\t\tDolly Camera\r\n"
			"MWHL+Shift\tFast Dolly Camera\r\n"
			"MWHL+Ctrl\tRaise/Lower Camera To Next Floor\r\n"
			"\r\n"
			"\r\n"
			"Z Window\r\n"
			"============\r\n"
			"LMB\t\tDrag Brush/Face\r\n"
			"\t\tDrag Edge/Vertex\r\n"
			"LMB+Ctrl\t\tPlace Camera\r\n"
			"LMB+Shift\tSelect Brush\r\n"
			"\r\n"
			"RMB\t\tDrag View\r\n"
			"RMB+Ctrl\t\tZoom View\r\n"
			"\r\n"
			"MMB+Ctrl\t\tMove Camera Icon\r\n"
			"\r\n"
			"MWHL\t\tScroll View\r\n"
			"MWHL+Shift\tFast Scroll View\r\n"
			"MWHL+Ctrl\tZoom View\r\n"
			"\r\n"
			"\r\n"
			"Texture Window\r\n"
			"============\r\n"
			"LMB\t\tSelect Texture, Apply to Selection, Zero Alignment on Selection\r\n"
			"\r\n"
			"MMB\t\tSelect Texture, Apply to Selection\r\n"
			"\r\n"
			"RMB\t\tDrag View\r\n"
			"RMB+Shift\tFast Scroll View\r\n"
			"RMB+Ctrl\t\tScale Textures\r\n"
			"\r\n"
			"MWHL\t\tScroll View\r\n"
			"MWHL+Shift\tFast Scroll View\r\n"
			"MWHL+Ctrl\tScale Textures\r\n"
			"\r\n"
			"\r\n"
			"Entity Window\r\n"
			"============\r\n"
			"LMB\t\tDouble Click on Classname Creates Entity\r\n"
			"\t\tChanges Selection Classname\r\n"
		);
		return TRUE;

	case WM_CLOSE:
		EndDialog(hwndDlg, 0);
		return 0;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDCLOSE:
			EndDialog(hwndDlg, 1);
			break;
		}
		return 0;
	}
	return FALSE;
}

void DoMouselist()
{
	DialogBox(g_qeglobals.d_hInstance, MAKEINTRESOURCE(IDD_MOUSELIST), g_hwndMain, MouselistDlgProc);
}

/*
============
KeylistDlgProc
============
*/
INT_PTR CALLBACK KeylistDlgProc ( 
    HWND	hwndDlg,// handle to dialog box
    UINT	uMsg,	// message
    WPARAM	wParam,	// first message parameter
    LPARAM	lParam 	// second message parameter
   )
{
	switch (uMsg)
    {
	case WM_INITDIALOG:
		SetDlgItemText(hwndDlg, IDC_EDIT_KEYLIST,	
			"Windows\r\n"
			"============\r\n"
			"Q\t\tCamera View\r\n"
			"1\t\tXY View\r\n"
			"2\t\tYZ View\r\n"
			"3\t\tXZ View\r\n"
			"4\t\tZ Checker\r\n"
			"T\t\tTexture Palette\r\n"
			"O\t\tConsole\r\n"
			"N\t\tEntity Inspector\r\n"
			"S\t\tSurface Inspector\r\n"
			"F1\t\t\"Help\"\r\n"
			"F2\t\tMap Info\r\n"
			"F3\t\tEntity Info\r\n"
			"F4\t\tPreferences\r\n"
			"\r\n"
			"Navigation\r\n"
			"============\r\n"
			"Escape\t\tCancel Tool, Deselect All\r\n"
			"Up\t\tCamera Forward\r\n"
			"Down\t\tCamera Back\r\n"
			"Left\t\tCamera Turn Left\r\n"
			"Right\t\tCamera Turn Right\r\n"
			"A\t\tCamera Look Up\r\n"
			"Z\t\tCamera Look Down\r\n"
			"D\t\tCamera Strafe Up\r\n"
			"C\t\tCamera Strafe Down\r\n"
			",\t\tCamera Strafe Left\r\n"
			".\t\tCamera Strafe Right\r\n"
			"PgDn\t\tCamera Drop to Next Lower Floor\r\n"
			"PgDn\t\tCamera Drop to Next Higher Floor\r\n"
			"End\t\tCamera Recenter\r\n"
			"\\\t\tCenter All Views\r\n"
			"Tab\t\tCycle Grid View Axis (XY/YZ/XZ)\r\n"
			"Insert\t\tGrid View Zoom In\r\n"
			"Delete\t\tGrid View Zoom Out\r\n"
			"Ctrl+Insert\t\tZ View Zoom In\r\n"
			"Ctrl+Delete\tZ View Zoom Out\r\n"
			"Shift+Tab\t\tSwap Camera & Grid Views\r\n"
			"1-9\t\tGrid Size 1-256\r\n"
			"-\t\tGrid Size Down\r\n"
			"+\t\tGrid Size Up\r\n"
			"0\t\tToggle Grid Visibility\r\n"
			"L\t\tToggle Texture Lock\r\n"
			"Ctrl+F\t\tFind Brush\r\n"
			"Ctrl+,\t\tPrevious Leak Point\r\n"
			"Ctrl+.\t\tNext Leak Point\r\n"
			"Alt+O\t\tRegion Off\r\n"
			"Alt+B\t\tRegion to Brush\r\n"
			"Alt+X\t\tRegion to XY\r\n"
			"Alt+Y\t\tRegion to YZ\r\n"
			"Alt+Z\t\tRegion to XZ\r\n"
			"M\t\tImport Map\r\n"
			"Ctrl+Shift+M\tExport Map\r\n"
			"Ctrl+\\\t\tToggle Cubic Clipping\r\n"
			"Ctrl+[\t\tReduce Cubic Clip Range\r\n"
			"Ctrl+]\t\tEnlarge Cubic Clip Range\r\n"
			"\r\n"
			"Editing\r\n"
			"============\r\n"
			"Ctrl+Z\t\tUndo\r\n"
			"Ctrl+Y\t\tRedo\r\n"
			"Space\t\tClone Selected\r\n"
			"Backspace\tDelete Selected\r\n"
			"V\t\tToggle Vertex Manipulation Tool\r\n"
			"E\t\tToggle Edge Manipulation Tool\r\n"
			"F\t\tToggle Face Manipulation Tool\r\n"
			"X\t\tToggle 3-Point Clip Tool\r\n"
			"Enter\t\tClip\r\n"
			"Ctrl+Enter\t\tClip Flip\r\n"
			"Shift+Enter\tSplit\r\n"
			"B\t\tMake Cylinder\r\n"
			"Ctrl+B\t\tMake Cone\r\n"
			"Ctrl+3-9\t\tMake 3-9 Sided Cylinder\r\n"
			"Ctrl+K\t\tTarget/Targetname Connect Entities\r\n"
			"Alt+M\t\tCSG Merge\r\n"
			"Alt+H\t\tCSG Hollow\r\n"
			"Alt+S\t\tCSG Subtract\r\n"
			"Ctrl+I\t\tAdd Brush to Entity (Insert)\r\n"
			"Ctrl+U\t\tMove Brush to World (Ungroup)\r\n"
			"Ctrl+R\t\tReplace Textures\r\n"
			"K\t\tSelect Entity Color\r\n"
			"R\t\tArbitrary Rotation\r\n"
			"F12\t\tCompile & Run\r\n"
			"\r\n"
			"Selection\r\n"
			"============\r\n"
			"H\t\tHide Selected\r\n"
			"Shift+H\t\tHide Unselected\r\n"
			"Ctrl+H\t\tUnhide All\r\n"
			"I\t\tInvert Selection\r\n"
			"Ctrl+A\t\tSelect All\r\n"
			"Ctrl+Shift+A\tSelect All of Type\r\n"
			"Ctrl+Tab\t\tNext Brush in Entity\r\n"
			"Alt+C\t\tSelect Complete Tall\r\n"
			"Alt+P\t\tSelect Partial Tall\r\n"
			"Alt+I\t\tSelect Inside\r\n"
			"Alt+T\t\tSelect Touching\r\n"
			"Ctrl+E\t\tSelect Matching Keyvalue\r\n"
			"Ctrl+T\t\tSelect Matching Texture\r\n"
			"\r\n"
			"Texturing\r\n"
			"============\r\n"
			"U\t\tShow Textures In Use\r\n"
			"Shift+Up\t\tShift Up\r\n"
			"Shift+Down\tShift Down\r\n"
			"Shift+Left\t\tShift Left\r\n"
			"Shift+Right\tShift Right\r\n"
			"Ctrl+Up\t\tScale Up Vertical\r\n"
			"Ctrl+Down\tScale Down Vertical\r\n"
			"Ctrl+Left\t\tScale Up Horizontal\r\n"
			"Ctrl+Right\t\tScale Down Horizontal\r\n"
			"Ctrl+Shift+Up\tRotate 1\r\n"
			"Ctrl+Shift+Down\tRotate -1\r\n"
			"Ctrl+Shift+Left\tRotate 15\r\n"
			"Ctrl+Shift+Right\tRotate -15\r\n"
		);
		return TRUE;

	case WM_CLOSE:
		EndDialog(hwndDlg, 0);
		return 0;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDCLOSE:
			EndDialog(hwndDlg, 1);
			break;
		}
		return 0;
	}
	return FALSE;
}

/*
============
DoKeylist
============
*/
void DoKeylist ()
{
	DialogBox(g_qeglobals.d_hInstance, MAKEINTRESOURCE(IDD_KEYLIST), g_hwndMain, KeylistDlgProc );
}
// <---sikk





// sikk---> Create Entity Dialog (*ripped from QE5)
/*
=====================================================================

	CREATE ENTITY

=====================================================================
*/

bool g_bPointBased, g_bBrushBased, g_bFromSelection;

/*
============
FillEntityListbox
============
*/
void FillEntityListbox (HWND hwnd, bool bPointbased, bool bBrushbased)
{
	EntClass   *pec;
	int			index;

	SendMessage(hwnd, LB_RESETCONTENT, 0, 0);

	for (auto ecIt = EntClass::cbegin(); ecIt != EntClass::cend(); ecIt++)
	{
		pec = *ecIt;
		if (pec->IsPointClass())
		{
			if (bPointbased)
			{
				index = SendMessage(hwnd, LB_ADDSTRING, 0, (LPARAM)pec->name.data());
				SendMessage(hwnd, LB_SETITEMDATA, index, (LPARAM)pec);
			}
		}
		else
		{
			if (bBrushbased)
			{
				index = SendMessage(hwnd, LB_ADDSTRING, 0, (LPARAM)pec->name.data());
				SendMessage(hwnd, LB_SETITEMDATA, index, (LPARAM)pec);
			}
		}
	}	
	SendMessage(hwnd, LB_SETCURSEL, (WPARAM)0, (LPARAM)0);
}

/*
============
ConfirmClassnameHack
============
*/
bool ConfirmClassnameHack(EntClass *desired)
{
	char	text[768];

	if (desired->IsPointClass())
		sprintf(text, "%s is a point entity. Are you sure you want to\ncreate one out of the selected brushes?", desired->name.c_str());
	else
		sprintf(text, "%s is a brush-based entity. Are you sure you want to create one with no brushes?", desired->name.c_str());

	return (MessageBox(g_hwndMain, text, "QuakeEd 3: Confirm Entity Creation", MB_OKCANCEL | MB_ICONQUESTION) == IDOK);
}

/*
============
CreateEntityDlg_Make
============
*/
bool CreateEntityDlg_Make (HWND h)
{
	int			index;
	EntClass	*pec;

	index = SendMessage(h, LB_GETCURSEL, 0, 0);
	pec = (EntClass *)SendMessage(h, LB_GETITEMDATA, index, 0);

	return Entity::Create(pec);
}

/*
============
CreateEntityDlgProc
============
*/
INT_PTR CALLBACK CreateEntityDlgProc (
    HWND	hwndDlg,// handle to dialog box
    UINT	uMsg,	// message
    WPARAM	wParam,	// first message parameter
    LPARAM	lParam 	// second message parameter
   )
{
	static HWND	h;

	switch (uMsg)
    {
	case WM_INITDIALOG:
		h = GetDlgItem(hwndDlg, IDC_LIST_CREATEENTITY);
		FillEntityListbox(h, g_bPointBased, g_bBrushBased);
		SetFocus(h);
		return FALSE;

	case WM_COMMAND: 
		switch (LOWORD(wParam)) 
		{
		case IDOK:
			if (CreateEntityDlg_Make(h))
				EndDialog(hwndDlg, 1);
			else
				SetFocus(hwndDlg);
			return TRUE;
			
		case IDCANCEL:
			EndDialog(hwndDlg, 0);
			return TRUE;
			
		case IDC_LIST_CREATEENTITY:
			switch (HIWORD(wParam))
			{
			case LBN_DBLCLK:
				if (CreateEntityDlg_Make(h))
					EndDialog(hwndDlg, 1);
				else
					SetFocus(hwndDlg);
				break;
			}
			break;
		}
		return 0;
	}
	return FALSE;
}

/*
============
DoCreateEntity
============
*/
void DoCreateEntity (bool bPointbased, bool bBrushbased, bool bSel, const vec3 origin)
{
	g_bPointBased = bPointbased;
	g_bBrushBased = bBrushbased;
	g_bFromSelection = bSel;
	//if (!bSel)
		g_brSelectedBrushes.mins = origin;

	DialogBox(g_qeglobals.d_hInstance, MAKEINTRESOURCE(IDD_CREATEENTITY), g_hwndMain, CreateEntityDlgProc);
}
// <---sikk


// sikk---> Map Info Dialog
/*
=====================================================================

	MAP INFO

=====================================================================
*/

/*
============
MapInfoDlgProc
============
*/
INT_PTR CALLBACK MapInfoDlgProc (
    HWND	hwndDlg,// handle to dialog box
    UINT	uMsg,	// message
    WPARAM	wParam,	// first message parameter
    LPARAM	lParam 	// second message parameter
   )
{
	static HWND	hList;
	int			nTotalBrushes = 0, 
				nTotalPointEnts = 0, 
				nTotalBrushEnts = 0, 
				nTotalFaces = 0,
				nCount = 0,
				nSkyBrushes = 0,
				nWaterBrushes = 0,
				nClipBrushes = 0;
	Brush    *pBrush;
	Face	   *pFace;
	Entity   *pEntity;
	EntClass   *pEClass;
	char		sz[256];
	int			nTabs[] = {128};

	switch (uMsg)
    {
	case WM_INITDIALOG:
		hList = GetDlgItem(hwndDlg, IDC_LIST_ENTITYBREAKDOWN);
		SendMessage(hList, LB_SETTABSTOPS, (WPARAM)1, (LPARAM)(LPINT)nTabs);

		for (pBrush = g_map.brActive.Next(); pBrush != &g_map.brActive; pBrush = pBrush->Next())
		{
			if (pBrush->owner->IsWorld())
			{
				// we don't want to count point entity faces 
				for (pFace = pBrush->faces; pFace; pFace = pFace->fnext)
					nTotalFaces++;
				nTotalBrushes++;
			}
			else if (pBrush->owner->IsPoint())
				nTotalPointEnts++;
			else
				nTotalBrushEnts++;

			if (pBrush->faces->texdef.name._Starts_with("sky"))
				nSkyBrushes++;
			if (pBrush->faces->texdef.name._Starts_with("*"))
				nWaterBrushes++;
			if (pBrush->faces->texdef.name._Starts_with("clip"))
				nClipBrushes++;
		}

		for (auto ecIt = EntClass::cbegin(); ecIt != EntClass::cend(); ecIt++) 
		{
			pEClass = *ecIt;
			for (pEntity = g_map.entities.Next(); pEntity != &g_map.entities; pEntity = pEntity->Next())
				if (pEntity->eclass->name == pEClass->name)
					nCount++;

			if (nCount)
			{
				sprintf(sz, "%s\t%d", pEClass->name.c_str(), nCount);
				SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)(LPCTSTR)sz);
				nCount = 0;
			}
		}	
	
		sprintf(sz, "%d", nTotalBrushes);
		SetDlgItemText(hwndDlg, IDC_EDIT_TOTALBRUSHES, sz);
		sprintf(sz, "%d", nTotalBrushEnts);
		SetDlgItemText(hwndDlg, IDC_EDIT_TOTALBRUSHENTS, sz);
		sprintf(sz, "%d", nTotalPointEnts);
		SetDlgItemText(hwndDlg, IDC_EDIT_TOTALPOINTENTS, sz);
		sprintf(sz, "%d", nTotalFaces);
		SetDlgItemText(hwndDlg, IDC_EDIT_TOTALFACES, sz);
		sprintf(sz, "%d", g_map.numTextures);
		SetDlgItemText(hwndDlg, IDC_EDIT_TOTALTEXTURES, sz);
		sprintf(sz, "%d", nSkyBrushes);
		SetDlgItemText(hwndDlg, IDC_EDIT_SKYBRUSHES, sz);
		sprintf(sz, "%d", nWaterBrushes);
		SetDlgItemText(hwndDlg, IDC_EDIT_WATERBRUSHES, sz);
		sprintf(sz, "%d", nClipBrushes);
		SetDlgItemText(hwndDlg, IDC_EDIT_CLIPBRUSHES, sz);
		return TRUE;

	case WM_CLOSE:
		EndDialog(hwndDlg, 0);
		return 0;

	case WM_COMMAND: 
		switch (LOWORD(wParam)) 
		{ 
		case IDOK:
			EndDialog(hwndDlg, 1);
			return TRUE;
		case IDCANCEL:
			EndDialog(hwndDlg, 0);
			break;
		}
		return 0;
	}
	return FALSE;
}

/*
============
DoMapInfo
============
*/
void DoMapInfo ()
{
	DialogBox(g_qeglobals.d_hInstance, MAKEINTRESOURCE(IDD_MAPINFO), g_hwndMain, MapInfoDlgProc);
}
// <---sikk


// sikk---> Entity Info Dialog
/*
=====================================================================

	ENTITY INFO

FIXME: EntityInfo TreeView does not update  entities deleted from main
		app until map is saved/autosaved. Radiant's works the same way. 
=====================================================================
*/

/*
============
AddOneItem

This function fills in the TV_ITEM and TV_INSERTSTRUCT structures 
and adds the item to the tree view.
============
*/
HTREEITEM AddOneItem (HWND hWnd, HTREEITEM hParent, HTREEITEM hInsAfter,
					  LPSTR szText, Entity *pEntity)
{
	HTREEITEM		hItem;
	TVITEM			tvI;
	TVINSERTSTRUCT	tvIns;

	// The .pszText, .cchTextMax, and .lParam are filled in.
	tvI.mask = TVIF_TEXT | TVIF_PARAM;
	tvI.pszText = szText;
	tvI.cchTextMax = lstrlen(szText);
	tvI.lParam = (LPARAM)pEntity;

	tvIns.item = tvI;
	tvIns.hInsertAfter = hInsAfter;
	tvIns.hParent = hParent;
  
	// Insert the item into the tree.
	hItem = TreeView_InsertItem(hWnd, &tvIns);

	return (hItem);
}

/*
============
AddTreeViewItems
============
*/
void AddTreeViewItems (HWND hWnd, Entity *pEntArray[], int nCount)
{
	HTREEITEM hTRoot;
	int	i;


	// add root item
	hTRoot = AddOneItem(hWnd, TVI_ROOT, (HTREEITEM)TVI_ROOT, pEntArray[0]->eclass->name.data(), pEntArray[0]);
	
	// then add all valid array elements as subitems
	for (i = 0; i < nCount; i++)
		AddOneItem(hWnd, hTRoot, (HTREEITEM)TVI_LAST, pEntArray[i]->eclass->name.data(), pEntArray[i]);
}

/*
============
OnSelectionChange
============
*/
void OnSelectionChange (HWND hTree, HWND hList)
{
	HTREEITEM		hItem = TreeView_GetSelection(hTree);
	TVITEM			tvI;
	LVITEM			lvItem;
	Entity       *pEntity;
	EPair        *pEpair;
	std::string temp;

	// If root item is selected, clear List Control and return
	if (TreeView_GetChild(hTree, hItem))
	{
		ListView_DeleteAllItems(hList);
		return;
	}

	tvI.hItem = hItem;
	tvI.mask = TVIF_PARAM;

	ListView_DeleteAllItems(hList);
	lvItem.mask = LVIF_TEXT;

	if (hItem)
	{
		TreeView_GetItem(hTree, &tvI);
			
		pEntity = (Entity *)tvI.lParam;
					
		for (pEpair = pEntity->epairs; pEpair; pEpair = pEpair->next)
		{
			lvItem.iItem = 0;
			lvItem.iSubItem = 0;
			temp = pEpair->GetKey();
			lvItem.pszText = temp.data();
			ListView_InsertItem(hList, &lvItem);

			lvItem.iItem = 0;
			lvItem.iSubItem = 1;
			temp = pEpair->GetValue();
			lvItem.pszText = temp.data();
			ListView_SetItem(hList, &lvItem);
		}
	}
}

/*
============
OnSelect
============
*/
void OnSelect (HWND hTree)
{
	HTREEITEM hItem = TreeView_GetSelection(hTree);
	TVITEM tvI;

	// If root item is selected, do nothing and return
	if (TreeView_GetChild(hTree, hItem))
		return;

	tvI.hItem = hItem;
	tvI.mask = TVIF_PARAM;
	TreeView_GetItem(hTree, &tvI);

	if (hItem)
	{
		Entity *pEntity = (Entity *)tvI.lParam;
		if (pEntity)
		{
			Selection::DeselectAll();
			Selection::HandleBrush(pEntity->brushes.ENext(), true);
		}
	}

	// Center on selected entity and update the windows
	for (int i = 0; i < 4; i++)
		g_vGrid[i].PositionView();
	g_vCamera.PositionCenter();
	g_vZ.PositionCenter();
	WndMain_ForceUpdateWindows(W_SCENE);
}

/*
============
OnDelete
============
*/
void OnDelete (HWND hTree, HWND hList)
{
	HTREEITEM	hItem = TreeView_GetSelection(hTree);
	TVITEM		tvI;

	// If root item is selected, do nothing and return
	if (TreeView_GetChild(hTree, hItem))
		return;

	tvI.hItem = hItem;
	tvI.mask = 	TVIF_TEXT;
	TreeView_GetItem(hTree, &tvI);
	// If worldspawn is selected, do nothing and return
	// I guess is selects the last brush made but it is 
	// unnecessary to delete brushes this way
	if (!strcmp(tvI.pszText, "worldspawn"))
		return;

	if (hItem)
	{
		OnSelect(hTree);
		Modify::Delete();
		TreeView_DeleteItem(hTree, hItem);
		ListView_DeleteAllItems(hList);
	}

	WndMain_ForceUpdateWindows(W_SCENE); 
}

/*
============
EntityInfoDlgProc
============
*/
INT_PTR CALLBACK EntityInfoDlgProc (
    HWND	hwndDlg,// handle to dialog box
    UINT	uMsg,	// message
    WPARAM	wParam,	// first message parameter
    LPARAM	lParam 	// second message parameter
   )
{
	static HWND	hTree, hList;
	Entity   *pEntity;
	Entity   *pEntArray[1024];
	EntClass   *pEClass;
	int			nCount = 0;
	HTREEITEM	hTRoot;


    LVCOLUMN	lvC;
	RECT rect;

	switch (uMsg)
    {
	case WM_INITDIALOG:
		hTree = GetDlgItem(hwndDlg, IDC_TREE_ENTITY);
		hList = GetDlgItem(hwndDlg, IDC_LIST_ENTITY);

		GetClientRect(hList, &rect);

		lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
		lvC.fmt = LVCFMT_LEFT;		// left align the column
		lvC.cx = rect.right / 2;	// width of the column, in pixels

		lvC.pszText = "Key";
		lvC.iSubItem = 1;
		ListView_InsertColumn(hList, 0, &lvC);

		lvC.pszText = "Value";
		ListView_InsertColumn(hList, 1, &lvC);
		ListView_DeleteColumn(hList, 2);

		// add worldspawn entity first
		hTRoot = AddOneItem(hTree, TVI_ROOT, (HTREEITEM)TVI_ROOT, g_map.world->eclass->name.data(), g_map.world);
		AddOneItem(hTree, hTRoot, (HTREEITEM)TVI_LAST, g_map.world->eclass->name.data(), g_map.world);

		// Nested for loop to group entites. Take each avalible entity class
		// and check them with all active entites, pulling matches into an array.
		for (auto ecIt = EntClass::cbegin(); ecIt != EntClass::cend(); ecIt++)
		{
			pEClass = *ecIt;
			for (pEntity = g_map.entities.Next(); pEntity != &g_map.entities; pEntity = pEntity->Next())
			{
				if (pEntity->eclass->name == pEClass->name)
				{
					pEntArray[nCount] = pEntity;
					nCount++;
				}
			}

			if (nCount)
			{
				AddTreeViewItems(hTree, pEntArray, nCount);
				nCount = 0;
			}
		}
		return TRUE;

	case WM_CLOSE:
		EndDialog(hwndDlg, 0);
		return 0;

	case WM_COMMAND: 
		switch (LOWORD(wParam)) 
		{ 
		case IDOK:
			EndDialog(hwndDlg, 1);
			return TRUE;
		case IDCANCEL:
			EndDialog(hwndDlg, 0);
			break;
		case IDC_BUTTON_SELECT:
			OnSelect(hTree);
			return TRUE;
		case IDC_BUTTON_DELETE:
			OnDelete(hTree, hList);	// FIXME: crashes
			return TRUE;
		}
		return 0;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code)
		{
		case TVN_SELCHANGED:
			OnSelectionChange(hTree, hList);
			break;

		case NM_DBLCLK:
			OnSelect(hTree);
			break;
		}
		return 0;
	}
	return FALSE;
}

/*
============
DoEntityInfo
============
*/
void DoEntityInfo ()
{
	DialogBox(g_qeglobals.d_hInstance, MAKEINTRESOURCE(IDD_ENTITYINFO), g_hwndMain, EntityInfoDlgProc);
}
// <---sikk



// sikk---> Brush Scaling Dialog
/*
=====================================================================

	SCALE BRUSH

=====================================================================
*/

/*
============
ScaleDlgProc
============
*/
INT_PTR CALLBACK ScaleDlgProc (
    HWND	hwndDlg,// handle to dialog box
    UINT	uMsg,	// message
    WPARAM	wParam,	// first message parameter
    LPARAM	lParam 	// second message parameter
   )
{
	float	fScaleX, fScaleY, fScaleZ;	// sikk - Spinners
	float	x, y, z;
	char	sz[4], szCheck[8];

	switch (uMsg)
    {
	case WM_INITDIALOG:
		SetFocus(GetDlgItem(hwndDlg, IDC_EDIT_SCALEX));
		// For more precise scaling
		if (g_qeglobals.bGridSnap)
		{
			g_qeglobals.bGridSnap = false;
			g_bSnapCheck = true;
		}

		SetDlgItemText(hwndDlg, IDC_EDIT_SCALEX, "1.00");
		SetDlgItemText(hwndDlg, IDC_EDIT_SCALEY, "1.00");
		SetDlgItemText(hwndDlg, IDC_EDIT_SCALEZ, "1.00");
		return FALSE;

	case WM_COMMAND: 
		switch (LOWORD(wParam)) 
		{ 
		case IDOK:
			GetDlgItemText(hwndDlg, IDCANCEL, szCheck, 8);
			if (strncmp(szCheck, "Close", 5))
			{
				GetDlgItemText(hwndDlg, IDC_EDIT_SCALEX, sz, 255);
				x = atof(sz);
				GetDlgItemText(hwndDlg, IDC_EDIT_SCALEY, sz, 255);
				y = atof(sz);
				GetDlgItemText(hwndDlg, IDC_EDIT_SCALEZ, sz, 255);
				z = atof(sz);

				Transform_Scale(x, y, z);
			}

			// For more precise scaling
			if (g_bSnapCheck)
			{
				g_qeglobals.bGridSnap = true;
				g_bSnapCheck = false;
			}
			EndDialog(hwndDlg, 1);
			WndMain_UpdateWindows(W_XY | W_Z | W_CAMERA);
			return TRUE;

		case IDAPPLY:
			GetDlgItemText(hwndDlg, IDC_EDIT_SCALEX, sz, 255);
			x = atof(sz);
			GetDlgItemText(hwndDlg, IDC_EDIT_SCALEY, sz, 255);
			y = atof(sz);
			GetDlgItemText(hwndDlg, IDC_EDIT_SCALEZ, sz, 255);
			z = atof(sz);

			Transform_Scale(x, y, z);

			SetDlgItemText(hwndDlg, IDCANCEL, "Close");
			WndMain_ForceUpdateWindows(W_SCENE);			
			return TRUE;

		case IDCANCEL:
			// For more precise scaling
			if (g_bSnapCheck)
			{
				g_qeglobals.bGridSnap = true;
				g_bSnapCheck = false;
			}

			EndDialog(hwndDlg, 0);
			return TRUE;
		}	
		return 0;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) 
        {
		case UDN_DELTAPOS:
			switch ((int)wParam)
			{
			case IDC_SPIN_SCALEX:
				if (((LPNMUPDOWN)lParam)->iDelta < 0) 
				{
					GetDlgItemText(hwndDlg, IDC_EDIT_SCALEX, sz, 255);
					fScaleX = atof(sz);
					if (GetKeyState(VK_SHIFT) < 0)
						fScaleX += 0.10f;
					else if (GetKeyState(VK_CONTROL) < 0)
						fScaleX += 0.01f;
					else
						fScaleX += 1.00f;
					sprintf(sz, "%1.2f", fScaleX);
					SetDlgItemText(hwndDlg, IDC_EDIT_SCALEX, sz);
				}
				else
				{
					GetDlgItemText(hwndDlg, IDC_EDIT_SCALEX, sz, 255);
					fScaleX = atof(sz);
					if (GetKeyState(VK_SHIFT) < 0)
						fScaleX -= 0.10f;
					else if (GetKeyState(VK_CONTROL) < 0)
						fScaleX -= 0.01f;
					else
						fScaleX -= 1.00f;
					if (fScaleX <= 0)
						fScaleX = 0.01f;
					sprintf(sz, "%1.2f", fScaleX);
					SetDlgItemText(hwndDlg, IDC_EDIT_SCALEX, sz);
				}
				break;

			case IDC_SPIN_SCALEY:
				if (((LPNMUPDOWN)lParam)->iDelta < 0) 
				{
					GetDlgItemText(hwndDlg, IDC_EDIT_SCALEY, sz, 255);
					fScaleY = atof(sz);
					if (GetKeyState(VK_SHIFT) < 0)
						fScaleY += 0.10f;
					else if (GetKeyState(VK_CONTROL) < 0)
						fScaleY += 0.01f;
					else
						fScaleY += 1.00f;
					sprintf(sz, "%1.2f", fScaleY);
					SetDlgItemText(hwndDlg, IDC_EDIT_SCALEY, sz);
				}
				else
				{
					GetDlgItemText(hwndDlg, IDC_EDIT_SCALEY, sz, 255);
					fScaleY = atof(sz);
					if (GetKeyState(VK_SHIFT) < 0)
						fScaleY -= 0.10f;
					else if (GetKeyState(VK_CONTROL) < 0)
						fScaleY -= 0.01f;
					else
						fScaleY -= 1.00f;
					if (fScaleY <= 0)
						fScaleY = 0.01f;
					sprintf(sz, "%1.2f", fScaleY);
					SetDlgItemText(hwndDlg, IDC_EDIT_SCALEY, sz);
				}
				break;

			case IDC_SPIN_SCALEZ:
				if (((LPNMUPDOWN)lParam)->iDelta < 0) 
				{
					GetDlgItemText(hwndDlg, IDC_EDIT_SCALEZ, sz, 255);
					fScaleZ = atof(sz);
					if (GetKeyState(VK_SHIFT) < 0)
						fScaleZ += 0.10f;
					else if (GetKeyState(VK_CONTROL) < 0)
						fScaleZ += 0.01f;
					else
						fScaleZ += 1.00f;
					sprintf(sz, "%1.2f", fScaleZ);
					SetDlgItemText(hwndDlg, IDC_EDIT_SCALEZ, sz);
				}
				else
				{
					GetDlgItemText(hwndDlg, IDC_EDIT_SCALEZ, sz, 255);
					fScaleZ = atof(sz);
					if (GetKeyState(VK_SHIFT) < 0)
						fScaleZ -= 0.10f;
					else if (GetKeyState(VK_CONTROL) < 0)
						fScaleZ -= 0.01f;
					else
						fScaleZ -= 1.00f;
					if (fScaleZ <= 0)
						fScaleZ = 0.01f;
					sprintf(sz, "%1.2f", fScaleZ);
					SetDlgItemText(hwndDlg, IDC_EDIT_SCALEZ, sz);
				}
				break;
			}
			return 0;
		}
		return 0;
	}

	return FALSE;
}

/*
============
DoScale
============
*/
void DoScale ()
{
	DialogBox(g_qeglobals.d_hInstance, MAKEINTRESOURCE(IDD_SCALE), g_hwndMain, ScaleDlgProc);
}
// <---sikk 


// sikk---> Camera Speed Dialog
/*
=====================================================================

	CAMERA SPEED

=====================================================================
*/

/*
============
CamSpeedDlgProc
============
*/
INT_PTR CALLBACK CamSpeedDlgProc( 
    HWND	hwndDlg,// handle to dialog box
    UINT	uMsg,	// message
    WPARAM	wParam,	// first message parameter
    LPARAM	lParam 	// second message parameter
   )
{
	POINT	point;
//	char	sz[8];

	switch (uMsg)
    {
	case WM_INITDIALOG:
		GetCursorPos(&point);
		MoveWindow(hwndDlg, point.x - 4, point.y - 4, 42, 134, TRUE);

		SendDlgItemMessage(hwndDlg, IDC_SLIDER_CAMSPEED, TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(0, 4096));
		// Sets the interval frequency for tick marks in a trackbar
		// WPARAM: frequency 
		SendDlgItemMessage(hwndDlg, IDC_SLIDER_CAMSPEED, TBM_SETTICFREQ, (WPARAM)409.6f, (LPARAM)0);
		// Sets the number of logical positions the trackbar's slider moves with arrow keys
		// LPARAM: new line size
		SendDlgItemMessage(hwndDlg, IDC_SLIDER_CAMSPEED, TBM_SETLINESIZE, (WPARAM)0, (LPARAM)64);
		// Sets the number of logical positions the trackbar's slider moves with PgUp/PgDn keys
		// LPARAM: new page size
		SendDlgItemMessage(hwndDlg, IDC_SLIDER_CAMSPEED, TBM_SETPAGESIZE, (WPARAM)0, (LPARAM)256);
		// Sets the current logical position of the slider in a trackbar. 
		// WPARAM: Redraw flag 
		// LPARAM: New logical position of the slider
		SendDlgItemMessage(hwndDlg, IDC_SLIDER_CAMSPEED, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)g_cfgEditor.CameraSpeed);
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDCANCEL:
			g_cfgEditor.CameraSpeed = SendDlgItemMessage(hwndDlg, IDC_SLIDER_CAMSPEED, TBM_GETPOS, 0, 0);
			ReleaseCapture();
			EndDialog(hwndDlg, 1);
			break;
		}
		return 0;

	case WM_NCHITTEST:
		if (DefWindowProc(hwndDlg, uMsg, wParam, lParam) != HTCLIENT)
		{
			g_cfgEditor.CameraSpeed = SendDlgItemMessage(hwndDlg, IDC_SLIDER_CAMSPEED, TBM_GETPOS, 0, 0);
			EndDialog(hwndDlg, 1);
		}
		return 0;
	}
	return FALSE;
}

/*
============
DoCamSpeed
============
*/
void DoCamSpeed ()
{
	DialogBox(g_qeglobals.d_hInstance, MAKEINTRESOURCE(IDD_CAMSPEED), g_hwndMain, CamSpeedDlgProc);
}
// <---sikk


// sikk---> Default Texture Scale Dialog
/*
=====================================================================

	DEFAULT TEXTURE SCALE

=====================================================================
*/

/*
============
DefaultTexScaleDlgProc
============
*/
INT_PTR CALLBACK DefaultTexScaleDlgProc (
    HWND	hwndDlg,// handle to dialog box
    UINT	uMsg,	// message
    WPARAM	wParam,	// first message parameter
    LPARAM	lParam 	// second message parameter
   )
{
	HWND	h;
	char	sz[8];
	float	fScale;

	switch (uMsg)
    {
	case WM_INITDIALOG:
		h = GetDlgItem(hwndDlg, IDC_EDIT_TEXSCALE);
		SetFocus (h);
		sprintf(sz, "%4.2f", g_qeglobals.d_fDefaultTexScale);
		SetDlgItemText(hwndDlg, IDC_EDIT_TEXSCALE, sz);
		return FALSE;

	case WM_COMMAND: 
		switch (LOWORD(wParam)) 
		{ 
		case IDOK:
			GetDlgItemText(hwndDlg, IDC_EDIT_TEXSCALE, sz, 255);
			g_qeglobals.d_fDefaultTexScale = atof(sz);
			EndDialog(hwndDlg, 1);
			break;

		case IDCANCEL:
			EndDialog(hwndDlg, 0);
			break;
		}
		return 0;

// sikk---> Spinners	
	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) 
        {
		case UDN_DELTAPOS:
			if (((LPNMUPDOWN)lParam)->iDelta < 0)
			{
				GetDlgItemText(hwndDlg, IDC_EDIT_TEXSCALE, sz, 255);
				fScale = atof(sz);
				fScale += 0.25f;
				sprintf(sz, "%4.2f", fScale);
				SetDlgItemText(hwndDlg, IDC_EDIT_TEXSCALE, sz);
			}
			else
			{
				GetDlgItemText(hwndDlg, IDC_EDIT_TEXSCALE, sz, 255);
				fScale = atof(sz);
				if (fScale > 0.25f)
					fScale -= 0.25f;
				else
					fScale = 0.25f;
				sprintf(sz, "%4.2f", fScale);
				SetDlgItemText(hwndDlg, IDC_EDIT_TEXSCALE, sz);
			}
			return 0;
		}
		return 0;
// <---sikk	

	default:
		return FALSE;
	}
}

/*
============
DoDefaultTexScale
============
*/
void DoDefaultTexScale ()
{
	DialogBox(g_qeglobals.d_hInstance, MAKEINTRESOURCE(IDD_TEXTURESCALE), g_hwndMain, DefaultTexScaleDlgProc);
}
// <---sikk


// sikk---> Find Key/Value Dialog
/*
=====================================================================

	FIND KEY/VALUE

=====================================================================
*/

/*
============
FindKeyValueDlgProc
============
*/
INT_PTR CALLBACK FindKeyValueDlgProc (
    HWND	hwndDlg,// handle to dialog box
    UINT	uMsg,	// message
    WPARAM	wParam,	// first message parameter
    LPARAM	lParam 	// second message parameter
   )
{
	char	szKey[256];
	char	szValue[256];

	switch (uMsg)
    {
	case WM_INITDIALOG:
	{
		HWND	h;
		h = GetDlgItem(hwndDlg, IDC_EDIT_FINDKEY);

		GetDlgItemText(g_wndEntity->w_hwndEntDialog, IDC_E_KEY_FIELD, szKey, 255);
		GetDlgItemText(g_wndEntity->w_hwndEntDialog, IDC_E_VALUE_FIELD, szValue, 255);
		SetDlgItemText(hwndDlg, IDC_EDIT_FINDKEY, szKey);
		SetDlgItemText(hwndDlg, IDC_EDIT_FINDVALUE, szValue);
		SetFocus(h);
		return FALSE;
	}
	case WM_COMMAND: 
		switch (LOWORD(wParam)) 
		{ 
		
		case IDOK:
			GetDlgItemText(hwndDlg, IDC_EDIT_FINDKEY, szKey, 255);
			GetDlgItemText(hwndDlg, IDC_EDIT_FINDVALUE, szValue, 255);
			Selection::MatchingKeyValue(szKey, szValue);

			EndDialog(hwndDlg, 1);
			return TRUE;

		case IDCANCEL:
			EndDialog(hwndDlg, 0);
			return TRUE;
		}	
	}
	return FALSE;
}

/*
============
DoFindKeyValue
============
*/
void DoFindKeyValue ()
{
	DialogBox(g_qeglobals.d_hInstance, MAKEINTRESOURCE(IDD_FINDKEYVALUE), g_hwndMain, FindKeyValueDlgProc);
}
// <---sikk







/*
=====================================================================

	SET KEY/VALUES

=====================================================================
*/


/*
============
SetKeyvalsDlgProc
============
*/
INT_PTR CALLBACK SetKeyvalsDlgProc(
	HWND	hwndDlg,// handle to dialog box
	UINT	uMsg,	// message
	WPARAM	wParam,	// first message parameter
	LPARAM	lParam 	// second message parameter
)
{
	char	szKey[256];
	char	szValue[256];
	char	szStart[4];
	int		prefix;
	HWND	h;

	switch (uMsg)
	{
	case WM_INITDIALOG:
		h = GetDlgItem(hwndDlg, IDC_EDIT_SETKVKEY);
		SetFocus(h);
		return FALSE;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{

		case IDAPPLY:
			GetDlgItemText(hwndDlg, IDC_EDIT_SETKVKEY, szKey, 256);
			GetDlgItemText(hwndDlg, IDC_EDIT_SETKVVALUE, szValue, 256);
			prefix = SendDlgItemMessage(hwndDlg, IDC_CHECK_SETKVINCR, BM_GETCHECK, 0, 0);
			GetDlgItemText(hwndDlg, IDC_EDIT_SETKVSTART, szStart, 4);

			if (prefix)
			{
				if (!szStart[0])
				{
					MessageBox(hwndDlg, "No starting number or letter specified", "QuakeEd 3: Error", MB_ICONERROR | MB_OK);
					return TRUE;
				}
				Modify::SetKeyValueSeries(szKey, szValue, szStart);
			}
			else
			{
				Modify::SetKeyValue(szKey, szValue);
			}
			return TRUE;

		case IDCLOSE:
			EndDialog(hwndDlg, 0);
			g_hwndSetKeyvalsDlg = NULL;
			return TRUE;

		case IDC_EDIT_SETKVSTART:
		{
			int farts = HIWORD(wParam);
			if (farts == EN_CHANGE)
			{
				GetDlgItemText(hwndDlg, IDC_EDIT_SETKVSTART, szStart, 4);
				if (szStart[0])
					SendDlgItemMessage(hwndDlg, IDC_CHECK_SETKVINCR, BM_SETCHECK, 1, 0);
			}
			return FALSE;
		}
		}
	}
	return FALSE;
}
/*
============
DoSetKeyValues
============
*/
void DoSetKeyValues()
{
	if (g_hwndSetKeyvalsDlg)
	{
		SetFocus(g_hwndSetKeyvalsDlg);
		return;
	}

	g_hwndSetKeyvalsDlg = CreateDialog(g_qeglobals.d_hInstance, MAKEINTRESOURCE(IDD_SETKEYVALUES), g_hwndMain, SetKeyvalsDlgProc);
	ShowWindow(g_hwndSetKeyvalsDlg, SW_SHOW);
}


// =======================================================

INT_PTR CALLBACK ImportOptionsDlgProc(
	HWND	hwndDlg,// handle to dialog box
	UINT	uMsg,	// message
	WPARAM	wParam,	// first message parameter
	LPARAM	lParam 	// second message parameter
)
{
	int out = 0;

	switch (uMsg)
	{
	case WM_INITDIALOG:
		EnableWindow(GetDlgItem(hwndDlg, IDC_RADIO_WKM_ADD), 0);
		EnableWindow(GetDlgItem(hwndDlg, IDC_RADIO_WKM_OVERWRITE), 0);
		SendDlgItemMessage(hwndDlg, IDC_RADIO_WKM_ADD, BM_SETCHECK, 1, 0);
		return TRUE;

	case WM_CLOSE:
		EndDialog(hwndDlg, 0);
		return TRUE;

	case WM_COMMAND:
		if (HIWORD(wParam) == BN_CLICKED)
		{
			switch (LOWORD(wParam))
			{
			case IDC_CHECK_WKV:
			{
				bool on = (SendDlgItemMessage(hwndDlg, IDC_CHECK_WKV, BM_GETCHECK, 0, 0) != 0);
				EnableWindow(GetDlgItem(hwndDlg, IDC_RADIO_WKM_ADD), on);
				EnableWindow(GetDlgItem(hwndDlg, IDC_RADIO_WKM_OVERWRITE), on);
				return TRUE;
			}
			case IDOK:
				if (SendDlgItemMessage(hwndDlg, IDC_CHECK_WKV, BM_GETCHECK, 0, 0) != 0) out += 1;
				if (SendDlgItemMessage(hwndDlg, IDC_CHECK_WAD, BM_GETCHECK, 0, 0) != 0) out += 2;
				if (SendDlgItemMessage(hwndDlg, IDC_CHECK_TARGETS, BM_GETCHECK, 0, 0) != 0) out += 4;
				if (SendDlgItemMessage(hwndDlg, IDC_RADIO_WKM_OVERWRITE, BM_GETCHECK, 0, 0) != 0) out += 8;
				EndDialog(hwndDlg, out);
				return TRUE;
			case IDCANCEL:
				EndDialog(hwndDlg, -1);
				return TRUE;
			}
		}
	}
	return FALSE;
}

bool ImportOptionsDialog(CmdImportMap *cmdIM)
{
	int result = DialogBoxParamA(g_qeglobals.d_hInstance, MAKEINTRESOURCE(IDD_IMPORT), g_hwndMain, ImportOptionsDlgProc, 0L);

	if (result < 0)
		return false;

	if (result & 1)
	{
		if (result & 8)
			cmdIM->MergeWorldKeys(CmdImportMap::KVM_OVERWRITE);
		else
			cmdIM->MergeWorldKeys(CmdImportMap::KVM_ADD);
	}
	if (result & 2)
		cmdIM->MergeWads(true);
	if (result & 4)
		cmdIM->AdjustTargets(true);

	return true;
}

