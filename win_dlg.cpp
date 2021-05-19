//==============================
//	win_dlg.c
//==============================

#include "qe3.h"

#pragma warning(disable: 4800)	// 'LRESULT': forcing value to bool 'true' or 'false' (performance warning)

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
BOOL CALLBACK AboutDlgProc( 
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

		SetDlgItemText(hwndDlg, IDC_ABOUT_APPNAME, g_qeAppName);

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
	DialogBox(g_qeglobals.d_hInstance, MAKEINTRESOURCE(IDD_ABOUT), g_qeglobals.d_hwndMain, AboutDlgProc);
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
	int			i;
	Brush	   *b;
	Entity   *e;

	if (entitynum == 0)
		e = g_map.world;
	else
	{
		e = g_map.entities.next;
		while (--entitynum)
		{
			e = e->next;
			if (e == &g_map.entities)
			{
				Sys_Printf("WARNING: No such entity.\n");
				return;
			}
		}
	}

	b = e->brushes.onext;
	if (b == &e->brushes)
	{
		Sys_Printf("WARNING: No such brush.\n");
		return;
	}

	while (brushnum--)
	{
		b = b->onext;
		if (b == &e->brushes)
		{
			Sys_Printf("WARNING: No such brush.\n");
			return;
		}
	}

	Selection::SelectBrush(b);

	for (i = 0; i < 3; i++)
		g_qeglobals.d_vXYZ[0].origin[i] = (b->mins[i] + b->maxs[i]) / 2;

	Sys_Printf("MSG: Selected.\n");
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

	b = g_brSelectedBrushes.next;
	if (b == &g_brSelectedBrushes)
		return;

	// find entity
	if (!b->owner->IsWorld())
	{
		(*entity)++;
		for (e = g_map.entities.next; e != &g_map.entities; e = e->next, (*entity)++)
			;
	}

	// find brush
	for (b2 = b->owner->brushes.onext; (b2 != b) && (b2 != &b->owner->brushes); b2 = b2->onext, (*brush)++)
		;
}

/*
============
FindBrushDlgProc
============
*/
BOOL CALLBACK FindBrushDlgProc (
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
	DialogBox(g_qeglobals.d_hInstance, MAKEINTRESOURCE(IDD_FINDBRUSH), g_qeglobals.d_hwndMain, FindBrushDlgProc);
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
BOOL CALLBACK RotateDlgProc (
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
		if (!g_qeglobals.d_savedinfo.bNoClamp)
		{
			g_qeglobals.d_savedinfo.bNoClamp = true;
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
				g_qeglobals.d_savedinfo.bNoClamp = false;
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
			
			Sys_ForceUpdateWindows(W_SCENE);

			return TRUE;
// <---sikk

		case IDCANCEL:
// sikk--->	For more precise rotation
			if (g_bSnapCheck)
			{
				g_qeglobals.d_savedinfo.bNoClamp = false;
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
	DialogBox(g_qeglobals.d_hInstance, MAKEINTRESOURCE(IDD_ROTATE), g_qeglobals.d_hwndMain, RotateDlgProc);
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
BOOL CALLBACK SidesDlgProc (
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
		if (!g_qeglobals.d_savedinfo.bNoClamp)
		{
			g_qeglobals.d_savedinfo.bNoClamp = true;
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
				Brush::MakeSidedCone(atoi(sz));
				break;
			case (2):
				Brush::MakeSidedSphere(atoi(sz));
				break;
//			case (3):
//				Brush_MakeSidedTorus(atoi(sz));
//				break;
			default:
				Brush::MakeSided(atoi(sz));
			}
// <---sikk
// sikk--->	For more precision
			if (g_bSnapCheck)
			{
				g_qeglobals.d_savedinfo.bNoClamp = false;
				g_bSnapCheck = false;
			}
// <---sikk

			EndDialog(hwndDlg, 1);
			break;

		case IDCANCEL:
// sikk--->	For more precision
			if (g_bSnapCheck)
			{
				g_qeglobals.d_savedinfo.bNoClamp = false;
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
	DialogBox(g_qeglobals.d_hInstance, MAKEINTRESOURCE(IDD_SIDES), g_qeglobals.d_hwndMain, SidesDlgProc);
}		


// sikk---> Command List Dialog
/*
=====================================================================

	KEYLIST

=====================================================================
*/

/*
============
KeylistDlgProc
============
*/
BOOL CALLBACK KeylistDlgProc ( 
    HWND	hwndDlg,// handle to dialog box
    UINT	uMsg,	// message
    WPARAM	wParam,	// first message parameter
    LPARAM	lParam 	// second message parameter
   )
{
	switch (uMsg)
    {
	case WM_INITDIALOG:
		SetDlgItemText(hwndDlg, IDC_RICHEDIT_KEYLIST ,	"Benchmark\t\tF10\n"
														"Brush3Sided\t\tCtrl+3\n"
														"Brush4Sided\t\tCtrl+4\n" 
														"Brush5Sided\t\tCtrl+5\n"
														"Brush6Sided\t\tCtrl+6\n"
														"Brush7Sided\t\tCtrl+7\n"
														"Brush8Sided\t\tCtrl+8\n"
														"Brush9Sided\t\tCtrl+9\n"
														"BrushCylinder\t\tB\n"
														"BrushCone\t\tCtrl+B\n"
														"BrushSphere\t\tCtrl+Shift+B\n"
														"CameraAngleDown\t\tZ\n"
														"CameraAngleUp\t\tA\n"
														"CameraBack\t\tDOWN\n"
														"CameraDown\t\tC\n"
														"CameraFloorDown\t\tPAGE DOWN\n"
														"CameraFloorUp\t\tPAGE UP\n"
														"CameraForward\t\tUP\n"
														"CameraLeft\t\tLEFT\n"
														"CameraRight\t\tRIGHT\n"
														"CameraStrafeLeft\t\tCOMMA\n"
														"CameraStrafeLeft\t\tLEFT\n"
														"CameraStrafeRight\t\tPERIOD\n"
														"CameraUp\t\tD\n"
														"CenterCamView\t\tEND\n"
														"CenterOnSelection\t\t\\\n"
														"ClipperClip\t\tENTER\n"
														"ClipperFlip\t\tCtrl+ENTER\n"
														"ClipperSplit\t\tShift+ENTER\n"
														"ClipperToggle\t\tX\n"
														"CSGHollow\t\tAlt+H\n"
														"CSGMerge\t\tAlt+M\n"
														"CSGSubtract\t\tAlt+S\n"
														"CubicClip\t\t\tCtrl+\\\n"
														"CubicClipZoomIn\t\tCtrl+]\n"
														"CubicClipZoomOut\t\tCtrl+[\n"
														"DlgEntityInfo\t\tF3\n"
														"DlgMapInfo\t\tF2\n"
														"DragEdges\t\tE\n"
														"DragVertices\t\tV\n"
														"EditCopy\t\t\tCtrl+C\n"
														"EditCut\t\t\tCtrl+X\n"
														"EditPaste\t\t\tCtrl+V\n"
														"EditRedo\t\t\tCtrl+Y\n"
														"EditUndo\t\t\tCtrl+Z\n"
														"EntitiesConnect\t\tCtrl+K\n"
														"EntitiesUngroup\t\tCtrl+U\n"
														"EntitiesInsertBrush\t\tCtrl+I\n"
														"EntityColor\t\tK\n"
														"ExportAsMap\t\tCtrl+Shift+M\n"
														"ExportAsPrefab\t\tCtrl+Shift+P\n"
														"FileNew\t\t\tCtrl+N\n"
														"FileOpen\t\t\tCtrl+O\n"
														"FileSave\t\t\tCtrl+S\n"
														"FileSaveAs\t\tCtrl+Shift+S\n"
														"Find\t\t\tCtrl+F\n"
														"Grid1\t\t\t1\n"
														"Grid2\t\t\t2\n"
														"Grid4\t\t\t3\n"
														"Grid8\t\t\t4\n"
														"Grid16\t\t\t5\n"
														"Grid32\t\t\t6\n"
														"Grid64\t\t\t7\n"
														"Grid128\t\t\t8\n"
														"Grid256\t\t\t9\n"
														"GridDown\t\t\t-\n"
														"GridUp\t\t\t+\n"
														"GridToggle\t\t0\n"
														"GridSnap\t\t\tG\n"
														"Help\t\t\tF1\n"
														"HideSelection\t\tH\n"
														"ImportMap\t\tM\n"
														"ImportPrefab\t\tP\n"
														"LeakSpotNext\t\tCtrl+>\n"
														"LeakSpotPrev\t\tCtrl+<\n"
														"NextBrushInGroup\t\tCtrl+TAB\n"
														"NextXYView\t\tTAB\n"
														"Preferences\t\tF4\n"
														"PrintXYView\t\tCtrl+P\n"
														"ProjectEdit\t\tF5\n"
														"ProjectLoad\t\tCtrl+L\n"
														"ProjectNew\t\tCtrl+Shift+N\n"
														"RegionOff\t\tAlt+O\n"
														"RegionSetXY\t\tAlt+X\n"
														"RegionSetXZ\t\tAlt+Z\n"
														"RegionSetYZ\t\tAlt+Y\n"
														"RegionSetBrush\t\tAlt+B\n"
														"RotateBrush\t\tR\n"
														"SelectAll\t\t\tCtrl+A\n"
														"SelectAllType\t\tCtrl+Shift+A\n"
														"SelectMatchingTexture\tCtrl+T\n"
														"SelectMatchingKey/Value\tCtrl+E\n"
														"SelectCompleteTall\t\tAlt+C\n"
														"SelectTouching\t\tAlt+T\n"
														"SelectPartialTall\t\tAlt+P\n"
														"SelectInside\t\tAlt+I\n"
														"SelectionClone\t\tSPACE\n"
														"SelectionDelete\t\tBACKSPACE\n"
														"SelectionDeselect\t\tESCAPE\n"
														"SelectionInvert\t\tI\n"
														"ShowHidden\t\tCtrl+H\n"
														"SurfaceInspector\t\tS\n"
														"TestMap\t\t\tF12\n"
														"TexShiftUp\t\tShift+UP\n"
														"TexShiftDown\t\tShift+DOWN\n"
														"TexShiftLeft\t\tShift+LEFT\n"
														"TexShiftRight\t\tShift+RIGHT\n"
														"TexScaleHorizUp\t\tCtrl+LEFT\n"
														"TexScaleHorizDown\tCtrl+RIGHT\n"
														"TexScaleVertUp\t\tCtrl+UP\n"
														"TexScaleVertDown\t\tCtrl+DOWN\n"
														"TexRotate+1\t\tCtrl+Shift+UP\n"
														"TexRotate-1\t\tCtrl+Shift+DOWN\n"
														"TexRotate+15\t\tCtrl+Shift+RIGHT\n"
														"TexRotate-15\t\tCtrl+Shift+LEFT\n"
														"TexturesFlushUnused\tF\n"
														"TexturesLock\t\tL\n"
														"TexturesReplace\t\tCtrl+R\n"
														"TexturesShowInUse\tU\n"
														"ToggleCameraView\t\tQ\n"
														"ToggleConsoleView\t\tO\n"
														"ToggleEntityView\t\tN\n"
														"ToggleTexturesView\tT\n"
														"ToggleXYView\t\tAlt+1\n"
														"ToggleXZView\t\tAlt+2\n"
														"ToggleYZView\t\tAlt+3\n"
														"ToggleZView\t\tAlt+4\n"
														"Zoom100%\t\tHOME\n"
														"ZoomIn\t\t\tINSERT\n"
														"ZoomOut\t\t\tDELETE\n"
														"ZZoom100%\t\tCtrl+HOME\n"
														"ZZoomIn\t\t\tCtrl+INSERT\n"
														"ZZoomOut\t\tCtrl+DELETE");
		return TRUE;

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
DoKeylist
============
*/
void DoKeylist ()
{
	DialogBox(g_qeglobals.d_hInstance, MAKEINTRESOURCE(IDD_KEYLIST), g_qeglobals.d_hwndMain, KeylistDlgProc );
}
// <---sikk




/*
=====================================================================

	FIND TEXTURE

=====================================================================
*/

/*
============
FindTextureDlgProc
============
*/
BOOL CALLBACK FindTextureDlgProc (
    HWND	hwndDlg,// handle to dialog box
    UINT	uMsg,	// message
    WPARAM	wParam,	// first message parameter
    LPARAM	lParam 	// second message parameter
   )
{
	char	szFind[64];
	char	szReplace[64];
	bool	bSelected, bForce;
	TexDef	*texdef;
	//Texture	*txFind, *txRepl;

	texdef = &g_qeglobals.d_workTexDef;

	switch (uMsg)
    {
	case WM_INITDIALOG:
		SendMessage(hwndDlg, WM_SETREDRAW, 0, 0);
		SetFocus(GetDlgItem(hwndDlg, IDC_EDIT_FIND));
		strcpy(szFind, texdef->name);
		strcpy(szReplace, texdef->name);
		SetDlgItemText(hwndDlg, IDC_EDIT_FIND, szFind);
		SetDlgItemText(hwndDlg, IDC_EDIT_REPLACE, szReplace);
		bSelected = SendDlgItemMessage(hwndDlg, IDC_CHECK_SELECTED, BM_GETCHECK, 0, 0);
		bForce = SendDlgItemMessage(hwndDlg, IDC_CHECK_FORCE, BM_GETCHECK, 0, 0);
		return FALSE;

	case WM_COMMAND: 
		switch (LOWORD(wParam)) 
		{ 
		case IDAPPLY:
			GetDlgItemText(hwndDlg, IDC_EDIT_FIND, szFind, 64);
			strncpy (texdef->name, szFind, sizeof(texdef->name) - 1);
			if (texdef->name[0] <= ' ')
				strcpy(texdef->name, "none");
			
			GetDlgItemText(hwndDlg, IDC_EDIT_REPLACE, szReplace, 64);
			strncpy(texdef->name, szReplace, sizeof(texdef->name) - 1);
			if (texdef->name[0] <= ' ')
				strcpy(texdef->name, "none");

			bSelected = SendDlgItemMessage(hwndDlg, IDC_CHECK_SELECTED, BM_GETCHECK, 0, 0);
			bForce = SendDlgItemMessage(hwndDlg, IDC_CHECK_FORCE, BM_GETCHECK, 0, 0);

			Surf_FindReplace(szFind, szReplace, bSelected, bForce);

			// because F&R dialog is modal, the camera window never gets a normal redraw message
			// and apply appears to do nothing until the dialog is closed, so force update here
			Sys_ForceUpdateWindows(W_CAMERA);

			return TRUE;

		case IDCLOSE:
			EndDialog(hwndDlg, 0);
			return TRUE;
		}
		return 0;
	}
	return FALSE;
}

/*
============
DoFindTexture
============
*/
void DoFindTexture ()
{
	DialogBox(g_qeglobals.d_hInstance, MAKEINTRESOURCE(IDD_FINDREPLACE), g_qeglobals.d_hwndMain, FindTextureDlgProc);
}


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

	for (auto ecIt = EntClass::begin(); ecIt != EntClass::end(); ecIt++)
	{
		pec = *ecIt;
		if (pec->IsPointClass())
		{
			if (bPointbased)
			{
				index = SendMessage(hwnd, LB_ADDSTRING, 0, (LPARAM)pec->name);
				SendMessage(hwnd, LB_SETITEMDATA, index, (LPARAM)pec);
			}
		}
		else
		{
			if (bBrushbased)
			{
				index = SendMessage(hwnd, LB_ADDSTRING, 0, (LPARAM)pec->name);
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
		sprintf(text, "%s is a point entity. Are you sure you want to\ncreate one out of the selected brushes?", desired->name);
	else
		sprintf(text, "%s is a brush-based entity. Are you sure you want to create one with no brushes?", desired->name);

	return (MessageBox(g_qeglobals.d_hwndMain, text, "QuakeEd 3: Confirm Entity Creation", MB_OKCANCEL | MB_ICONQUESTION) == IDOK);
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
//	Entity*		out;

	index = SendMessage(h, LB_GETCURSEL, 0, 0);
	pec = (EntClass *)SendMessage(h, LB_GETITEMDATA, index, 0);

//	Undo::Start("Create Entity");
//	Undo::AddBrushList(&g_brSelectedBrushes);
	return Entity::Create(pec);
//	Undo::EndBrushList(&g_brSelectedBrushes);
//	Undo::End();

//	return out;
//	Sys_UpdateWindows(W_CAMERA | W_XY | W_Z);
}

/*
============
CreateEntityDlgProc
============
*/
BOOL CALLBACK CreateEntityDlgProc (
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

	DialogBox(g_qeglobals.d_hInstance, MAKEINTRESOURCE(IDD_CREATEENTITY), g_qeglobals.d_hwndMain, CreateEntityDlgProc);
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
BOOL CALLBACK MapInfoDlgProc (
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

		for (pBrush = g_map.brActive.next; pBrush != &g_map.brActive; pBrush = pBrush->next)
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

			if (!strncmp(pBrush->faces->texdef.name, "sky", 3))
				nSkyBrushes++;
			if (pBrush->faces->texdef.name[0] == '*')
				nWaterBrushes++;
			if (!strncmp(pBrush->faces->texdef.name, "clip", 4))
				nClipBrushes++;
		}

		for (auto ecIt = EntClass::begin(); ecIt != EntClass::end(); ecIt++) 
		{
			pEClass = *ecIt;
			for (pEntity = g_map.entities.next; pEntity != &g_map.entities; pEntity = pEntity->next)
				if (pEntity->eclass->name == pEClass->name)
					nCount++;

			if (nCount)
			{
				sprintf(sz, "%s\t%d", pEClass->name, nCount);
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
	DialogBox(g_qeglobals.d_hInstance, MAKEINTRESOURCE(IDD_MAPINFO), g_qeglobals.d_hwndMain, MapInfoDlgProc);
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
	hTRoot = AddOneItem(hWnd, TVI_ROOT, (HTREEITEM)TVI_ROOT, pEntArray[0]->eclass->name, pEntArray[0]);
	
	// then add all valid array elements as subitems
	for (i = 0; i < nCount; i++)
		AddOneItem(hWnd, hTRoot, (HTREEITEM)TVI_LAST, pEntArray[i]->eclass->name, pEntArray[i]);
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
			lvItem.pszText = (char*)*pEpair->key;
			ListView_InsertItem(hList, &lvItem);

			lvItem.iItem = 0;
			lvItem.iSubItem = 1;
			lvItem.pszText = (char*)*pEpair->value;
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
			Selection::HandleBrush(pEntity->brushes.onext, true);
		}
	}

	// Center on selected entity and update the windows
	XYZView::PositionAllViews();
	g_qeglobals.d_vCamera.PositionCenter();
	Sys_ForceUpdateWindows(W_SCENE);
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
		Modify_Delete();
		TreeView_DeleteItem(hTree, hItem);
		ListView_DeleteAllItems(hList);
	}

	Sys_ForceUpdateWindows(W_SCENE); 
}

/*
============
EntityInfoDlgProc
============
*/
BOOL CALLBACK EntityInfoDlgProc (
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
		hTRoot = AddOneItem(hTree, TVI_ROOT, (HTREEITEM)TVI_ROOT, g_map.world->eclass->name, g_map.world);
		AddOneItem(hTree, hTRoot, (HTREEITEM)TVI_LAST, g_map.world->eclass->name, g_map.world);

		// Nested for loop to group entites. Take each avalible entity class
		// and check them with all active entites, pulling matches into an array.
		for (auto ecIt = EntClass::begin(); ecIt != EntClass::end(); ecIt++)
		{
			pEClass = *ecIt;
			for (pEntity = g_map.entities.next; pEntity != &g_map.entities; pEntity = pEntity->next)
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
	DialogBox(g_qeglobals.d_hInstance, MAKEINTRESOURCE(IDD_ENTITYINFO), g_qeglobals.d_hwndMain, EntityInfoDlgProc);
}
// <---sikk


// sikk---> Preferences Dialog
/*
=====================================================================

	PREFERENCES

=====================================================================
*/

static OPENFILENAME ofn;			// common dialog box structure
static char szDirName[_MAX_PATH];   // directory string
static char szFile[_MAX_PATH];		// filename string
static char szFileTitle[_MAX_FNAME];// file title string
static char szFilter[64] = "Quake Executables (*.exe)\0*.exe\0\0";	// filter string

/*
==================
OnGamePath
==================
*/
void OnGamePath (HWND h)
{
	szFile[0] = 0;

	GetCurrentDirectory(_MAX_PATH - 1, szDirName);

	ofn.lStructSize		= sizeof(OPENFILENAME);
	ofn.hwndOwner		= h;
	ofn.lpstrFilter		= szFilter;
	ofn.nFilterIndex	= 1;
	ofn.lpstrFile		= szFile;
	ofn.nMaxFile		= sizeof(szFile);
	ofn.lpstrFileTitle	= szFileTitle;
	ofn.nMaxFileTitle	= sizeof(szFileTitle);
	ofn.lpstrInitialDir	= szDirName;
	ofn.Flags			= OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
	ofn.lpstrTitle		= "Select Quake Executable File";

	if (GetOpenFileName(&ofn))
	{
			SetDlgItemText(h, IDC_EDIT_GAMEPATH, szFile);
			sprintf(g_qeglobals.d_savedinfo.szGameName, "%s", szFileTitle);
	}
}

/*
==================
OnPrefabPath
==================
*/
void OnPrefabPath (HWND h)
{
	HWND hwndEdit = GetDlgItem(h, IDC_EDIT_PREFABPATH);
	SelectDir(hwndEdit);
}

/*
============
PreferencesDlgProc
============
*/
BOOL CALLBACK PreferencesDlgProc (
    HWND	hwndDlg,// handle to dialog box
    UINT	uMsg,	// message
    WPARAM	wParam,	// first message parameter
    LPARAM	lParam 	// second message parameter
   )
{
//	HWND	hwndTrack = GetDlgItem(hwndDlg, IDC_SLIDER_GAMMA);
	HBITMAP	hb;
	int		nUndoLevel;
	int		nAutosave;
	int		nMapSize = g_qeglobals.d_savedinfo.nMapSize;
	int		nGamma = g_qeglobals.d_savedinfo.fGamma * 10;
	int		nMBCheck;
	char	sz[256];
	bool	bGammaCheck = false;
	bool	bLogCheck = g_qeglobals.d_savedinfo.bLogConsole;

	switch (uMsg)
    {
	case WM_INITDIALOG:
		// Initialize TrackBar 
		SendDlgItemMessage(hwndDlg, IDC_SLIDER_GAMMA, TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(0, 20));
		SendDlgItemMessage(hwndDlg, IDC_SLIDER_GAMMA, TBM_SETTICFREQ, (WPARAM)2, (LPARAM)0);
		SendDlgItemMessage(hwndDlg, IDC_SLIDER_GAMMA, TBM_SETLINESIZE, (WPARAM)0, (LPARAM)1);
		SendDlgItemMessage(hwndDlg, IDC_SLIDER_GAMMA, TBM_SETPAGESIZE, (WPARAM)0, (LPARAM)2);
		SendDlgItemMessage(hwndDlg, IDC_SLIDER_GAMMA, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)nGamma); 

		// Set Button Bitmaps
		hb = (HBITMAP)LoadImage(g_qeglobals.d_hInstance, (LPCTSTR)IDB_FIND, IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE | LR_LOADTRANSPARENT | LR_LOADMAP3DCOLORS);
		SendDlgItemMessage(hwndDlg, IDC_BUTTON_GAMEPATH, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hb);
		SendDlgItemMessage(hwndDlg, IDC_BUTTON_PREFABPATH, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hb);

		// Initialize Edit Boxes 
		SetDlgItemText(hwndDlg, IDC_EDIT_GAMEPATH, g_qeglobals.d_savedinfo.szGamePath);
		SetDlgItemText(hwndDlg, IDC_EDIT_PREFABPATH, g_qeglobals.d_savedinfo.szPrefabPath);
		SetDlgItemText(hwndDlg, IDC_EDIT_PARAMGAME, g_qeglobals.d_savedinfo.szModName);
		sprintf(sz, "%d", g_qeglobals.d_savedinfo.nAutosave);
		SetDlgItemText(hwndDlg, IDC_EDIT_AUTOSAVE, sz);
		sprintf(sz, "%d", g_qeglobals.d_savedinfo.nUndoLevels);
		SetDlgItemText(hwndDlg, IDC_EDIT_UNDOLEVELS, sz);

		// Initialize Check Boxes 
		SendDlgItemMessage(hwndDlg, IDC_CHECK_AUTOSAVE,			BM_SETCHECK, (g_qeglobals.d_savedinfo.bAutosave			? BST_CHECKED : BST_UNCHECKED), 0);
		SendDlgItemMessage(hwndDlg, IDC_CHECK_LOGCONSOLE,		BM_SETCHECK, (g_qeglobals.d_savedinfo.bLogConsole		? BST_CHECKED : BST_UNCHECKED), 0);
		SendDlgItemMessage(hwndDlg, IDC_CHECK_NOSTIPPLE,		BM_SETCHECK, (g_qeglobals.d_savedinfo.bNoStipple		? BST_CHECKED : BST_UNCHECKED), 0);
		SendDlgItemMessage(hwndDlg, IDC_CHECK_RADIANTLIGHTS,	BM_SETCHECK, (g_qeglobals.d_savedinfo.bRadiantLights	? BST_CHECKED : BST_UNCHECKED), 0);
		SendDlgItemMessage(hwndDlg, IDC_CHECK_VERTEXSPLITSFACE,	BM_SETCHECK, (g_qeglobals.d_savedinfo.bVertexSplitsFace	? BST_CHECKED : BST_UNCHECKED), 0);
		SendDlgItemMessage(hwndDlg, IDC_CHECK_BRUSHPRECISION,	BM_SETCHECK, (g_qeglobals.d_savedinfo.bBrushPrecision	? BST_CHECKED : BST_UNCHECKED), 0);
		SendDlgItemMessage(hwndDlg, IDC_CHECK_TESTAFTERBSP,		BM_SETCHECK, (g_qeglobals.d_savedinfo.bTestAfterBSP		? BST_CHECKED : BST_UNCHECKED), 0);
		SendDlgItemMessage(hwndDlg, IDC_CHECK_LOADLASTPROJECT,	BM_SETCHECK, (g_qeglobals.d_savedinfo.bLoadLastProject	? BST_CHECKED : BST_UNCHECKED), 0);
		SendDlgItemMessage(hwndDlg, IDC_CHECK_LOADLASTMAP,		BM_SETCHECK, (g_qeglobals.d_savedinfo.bLoadLastMap		? BST_CHECKED : BST_UNCHECKED), 0);
	//	SendDlgItemMessage(hwndDlg, IDC_CHECK_SORTTEXBYWAD,		BM_SETCHECK, (g_qeglobals.d_savedinfo.bSortTexByWad		? BST_CHECKED : BST_UNCHECKED), 0);
		SendDlgItemMessage(hwndDlg, IDC_CHECK_PARAMGAME,		BM_SETCHECK, (g_qeglobals.d_savedinfo.bModName			? BST_CHECKED : BST_UNCHECKED), 0);
		SendDlgItemMessage(hwndDlg, IDC_CHECK_PARAMHEAPSIZE,	BM_SETCHECK, (g_qeglobals.d_savedinfo.bHeapsize			? BST_CHECKED : BST_UNCHECKED), 0);
		SendDlgItemMessage(hwndDlg, IDC_CHECK_PARAMSKILL,		BM_SETCHECK, (g_qeglobals.d_savedinfo.bSkill			? BST_CHECKED : BST_UNCHECKED), 0);
		SendDlgItemMessage(hwndDlg, IDC_CHECK_PARAMDEATHMATCH,	BM_SETCHECK, (g_qeglobals.d_savedinfo.bDeathmatch		? BST_CHECKED : BST_UNCHECKED), 0);
		SendDlgItemMessage(hwndDlg, IDC_CHECK_PARAMDEVELOPER,	BM_SETCHECK, (g_qeglobals.d_savedinfo.bDeveloper		? BST_CHECKED : BST_UNCHECKED), 0);
		SendDlgItemMessage(hwndDlg, IDC_CHECK_PARAMRSPEEDS,		BM_SETCHECK, (g_qeglobals.d_savedinfo.bRSpeeds			? BST_CHECKED : BST_UNCHECKED), 0);
		SendDlgItemMessage(hwndDlg, IDC_CHECK_PARAMPOINTFILE,	BM_SETCHECK, (g_qeglobals.d_savedinfo.bPointfile		? BST_CHECKED : BST_UNCHECKED), 0);

		// Initialize Combo Boxes 
		SendDlgItemMessage(hwndDlg, IDC_COMBO_MAPSIZE, CB_ADDSTRING, (WPARAM)0, (LPARAM)"8192 (default)");
		SendDlgItemMessage(hwndDlg, IDC_COMBO_MAPSIZE, CB_ADDSTRING, (WPARAM)0, (LPARAM)"16384");
		SendDlgItemMessage(hwndDlg, IDC_COMBO_MAPSIZE, CB_ADDSTRING, (WPARAM)0, (LPARAM)"32768");
		SendDlgItemMessage(hwndDlg, IDC_COMBO_MAPSIZE, CB_ADDSTRING, (WPARAM)0, (LPARAM)"65536");
		sprintf(sz, "%d", g_qeglobals.d_savedinfo.nMapSize);
		SendDlgItemMessage(hwndDlg, IDC_COMBO_MAPSIZE, CB_SELECTSTRING, (WPARAM)-1, (LPARAM)sz);

		SendDlgItemMessage(hwndDlg, IDC_COMBO_PARAMHEAPSIZE, CB_ADDSTRING, (WPARAM)0, (LPARAM)"16384");
		SendDlgItemMessage(hwndDlg, IDC_COMBO_PARAMHEAPSIZE, CB_ADDSTRING, (WPARAM)0, (LPARAM)"32768");
		SendDlgItemMessage(hwndDlg, IDC_COMBO_PARAMHEAPSIZE, CB_ADDSTRING, (WPARAM)0, (LPARAM)"65536");
		SendDlgItemMessage(hwndDlg, IDC_COMBO_PARAMHEAPSIZE, CB_ADDSTRING, (WPARAM)0, (LPARAM)"131072");
		SendDlgItemMessage(hwndDlg, IDC_COMBO_PARAMHEAPSIZE, CB_ADDSTRING, (WPARAM)0, (LPARAM)"262144");
		SendDlgItemMessage(hwndDlg, IDC_COMBO_PARAMHEAPSIZE, CB_SELECTSTRING, (WPARAM)-1, (LPARAM)g_qeglobals.d_savedinfo.szHeapsize);

		SendDlgItemMessage(hwndDlg, IDC_COMBO_PARAMSKILL, CB_ADDSTRING, (WPARAM)0, (LPARAM)"0");
		SendDlgItemMessage(hwndDlg, IDC_COMBO_PARAMSKILL, CB_ADDSTRING, (WPARAM)0, (LPARAM)"1");
		SendDlgItemMessage(hwndDlg, IDC_COMBO_PARAMSKILL, CB_ADDSTRING, (WPARAM)0, (LPARAM)"2");
		SendDlgItemMessage(hwndDlg, IDC_COMBO_PARAMSKILL, CB_ADDSTRING, (WPARAM)0, (LPARAM)"3");
		SendDlgItemMessage(hwndDlg, IDC_COMBO_PARAMSKILL, CB_SELECTSTRING, (WPARAM)-1, (LPARAM)g_qeglobals.d_savedinfo.szSkill);
		return TRUE;

	case WM_COMMAND: 
		switch (LOWORD(wParam)) 
		{ 
		case IDOK:
			nGamma = SendDlgItemMessage(hwndDlg, IDC_SLIDER_GAMMA, TBM_GETPOS, 0, 0);
			if (nGamma != g_qeglobals.d_savedinfo.fGamma * 10)
				bGammaCheck = true;
			g_qeglobals.d_savedinfo.fGamma				= nGamma * 0.1;	
			g_qeglobals.d_savedinfo.bAutosave			= SendDlgItemMessage(hwndDlg, IDC_CHECK_AUTOSAVE,			BM_GETCHECK, 0, 0);
			g_qeglobals.d_savedinfo.bLogConsole			= SendDlgItemMessage(hwndDlg, IDC_CHECK_LOGCONSOLE,			BM_GETCHECK, 0, 0);
			g_qeglobals.d_savedinfo.bNoStipple			= SendDlgItemMessage(hwndDlg, IDC_CHECK_NOSTIPPLE,			BM_GETCHECK, 0, 0);
			g_qeglobals.d_savedinfo.bRadiantLights		= SendDlgItemMessage(hwndDlg, IDC_CHECK_RADIANTLIGHTS,		BM_GETCHECK, 0, 0);
			g_qeglobals.d_savedinfo.bVertexSplitsFace	= SendDlgItemMessage(hwndDlg, IDC_CHECK_VERTEXSPLITSFACE,	BM_GETCHECK, 0, 0);
			g_qeglobals.d_savedinfo.bBrushPrecision		= SendDlgItemMessage(hwndDlg, IDC_CHECK_BRUSHPRECISION,		BM_GETCHECK, 0, 0);
			g_qeglobals.d_savedinfo.bTestAfterBSP		= SendDlgItemMessage(hwndDlg, IDC_CHECK_TESTAFTERBSP,		BM_GETCHECK, 0, 0);
			g_qeglobals.d_savedinfo.bLoadLastProject	= SendDlgItemMessage(hwndDlg, IDC_CHECK_LOADLASTPROJECT,	BM_GETCHECK, 0, 0);
			g_qeglobals.d_savedinfo.bLoadLastMap		= SendDlgItemMessage(hwndDlg, IDC_CHECK_LOADLASTMAP,		BM_GETCHECK, 0, 0);
		//	g_qeglobals.d_savedinfo.bSortTexByWad		= SendDlgItemMessage(hwndDlg, IDC_CHECK_SORTTEXBYWAD,		BM_GETCHECK, 0, 0);
			g_qeglobals.d_savedinfo.bModName			= SendDlgItemMessage(hwndDlg, IDC_CHECK_PARAMGAME,			BM_GETCHECK, 0, 0);
			g_qeglobals.d_savedinfo.bHeapsize			= SendDlgItemMessage(hwndDlg, IDC_CHECK_PARAMHEAPSIZE,		BM_GETCHECK, 0, 0);
			g_qeglobals.d_savedinfo.bSkill				= SendDlgItemMessage(hwndDlg, IDC_CHECK_PARAMSKILL,			BM_GETCHECK, 0, 0);
			g_qeglobals.d_savedinfo.bDeathmatch			= SendDlgItemMessage(hwndDlg, IDC_CHECK_PARAMDEATHMATCH,	BM_GETCHECK, 0, 0);
			g_qeglobals.d_savedinfo.bDeveloper			= SendDlgItemMessage(hwndDlg, IDC_CHECK_PARAMDEVELOPER,		BM_GETCHECK, 0, 0);
			g_qeglobals.d_savedinfo.bRSpeeds			= SendDlgItemMessage(hwndDlg, IDC_CHECK_PARAMRSPEEDS,		BM_GETCHECK, 0, 0);
			g_qeglobals.d_savedinfo.bPointfile			= SendDlgItemMessage(hwndDlg, IDC_CHECK_PARAMPOINTFILE,		BM_GETCHECK, 0, 0);

			GetDlgItemText(hwndDlg, IDC_EDIT_GAMEPATH, g_qeglobals.d_savedinfo.szGamePath, 255);
			GetDlgItemText(hwndDlg, IDC_EDIT_PREFABPATH, g_qeglobals.d_savedinfo.szPrefabPath, 255);
			GetDlgItemText(hwndDlg, IDC_EDIT_PARAMGAME, g_qeglobals.d_savedinfo.szModName, 255);
			GetDlgItemText(hwndDlg, IDC_EDIT_AUTOSAVE, sz, 2);
			g_qeglobals.d_savedinfo.nAutosave = atoi(sz);
			GetDlgItemText(hwndDlg, IDC_EDIT_UNDOLEVELS, sz, 2);
			g_cmdQueue.SetSize(atoi(sz));
			//g_qeglobals.d_savedinfo.nUndoLevels = atoi(sz);
			//Undo::SetMaxSize(g_qeglobals.d_savedinfo.nUndoLevels);

			GetDlgItemText(hwndDlg, IDC_COMBO_MAPSIZE, sz, 8);
			g_qeglobals.d_savedinfo.nMapSize = atoi(sz);
			if (nMapSize != g_qeglobals.d_savedinfo.nMapSize)
				g_map.RegionOff();
			GetDlgItemText(hwndDlg, IDC_COMBO_PARAMHEAPSIZE, g_qeglobals.d_savedinfo.szHeapsize, 8);
			GetDlgItemText(hwndDlg, IDC_COMBO_PARAMSKILL, g_qeglobals.d_savedinfo.szSkill, 2);

			if (bGammaCheck)
				MessageBox(hwndDlg, "New Gamma setting requires a restart to take effect.", "QuakeEd 3: Preferences Info", MB_OK | MB_ICONINFORMATION);
			if (bLogCheck != g_qeglobals.d_savedinfo.bLogConsole)
				Sys_LogFile();
			EndDialog(hwndDlg, 1);
			Sys_UpdateWindows(W_ALL);
			return TRUE;

		case IDCANCEL:
			EndDialog(hwndDlg, 0);
			return TRUE;

		case IDC_BUTTON_RESETREGISTRY:
			nMBCheck = MessageBox(hwndDlg, "Registry will be reset to QuakeEd's default settings.\n\tDo you wish to continue?", "QuakeEd 3: Reset Registry?", MB_YESNO | MB_ICONQUESTION);
			if (nMBCheck == IDYES)
				g_qeglobals.d_bResetRegistry = true;
			return TRUE;

		case IDC_BUTTON_GAMEPATH:
			OnGamePath(hwndDlg);
			break;
		case IDC_BUTTON_PREFABPATH:
			OnPrefabPath(hwndDlg);
			break;
		}
		return 0;

	case WM_HSCROLL:
		nGamma = SendDlgItemMessage(hwndDlg, IDC_SLIDER_GAMMA, TBM_GETPOS, 0, 0);
		return 0;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code)
		{
		case UDN_DELTAPOS:
			switch ((int)wParam)
			{
			case IDC_SPIN_UNDOLEVELS:
				if (((LPNMUPDOWN)lParam)->iDelta < 0) 
				{
					GetDlgItemText(hwndDlg, IDC_EDIT_UNDOLEVELS, sz, 255);
					nUndoLevel = atoi(sz);
					if (nUndoLevel < 64)
						nUndoLevel++;
					else
						nUndoLevel = 1;
					sprintf(sz, "%d", nUndoLevel);
					SetDlgItemText(hwndDlg, IDC_EDIT_UNDOLEVELS, sz);
				}
				else
				{
					GetDlgItemText(hwndDlg, IDC_EDIT_UNDOLEVELS, sz, 255);
					nUndoLevel = atoi(sz);
					if (nUndoLevel > 1)
						nUndoLevel--;
					else
						nUndoLevel = 64;
					sprintf(sz, "%d", nUndoLevel);
					SetDlgItemText(hwndDlg, IDC_EDIT_UNDOLEVELS, sz);
				}
				break;
			case IDC_SPIN_AUTOSAVE:
				if (((LPNMUPDOWN)lParam)->iDelta < 0) 
				{
					GetDlgItemText(hwndDlg, IDC_EDIT_AUTOSAVE, sz, 255);
					nAutosave = atoi(sz);
					if (nAutosave < 60)
						nAutosave++;
					else
						nAutosave = 1;
					sprintf(sz, "%d", nAutosave);
					SetDlgItemText(hwndDlg, IDC_EDIT_AUTOSAVE, sz);
				}
				else
				{
					GetDlgItemText(hwndDlg, IDC_EDIT_AUTOSAVE, sz, 255);
					nAutosave = atoi(sz);
					if (nAutosave > 1)
						nAutosave--;
					else
						nAutosave = 60;
					sprintf(sz, "%d", nAutosave);
					SetDlgItemText(hwndDlg, IDC_EDIT_AUTOSAVE, sz);
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
DoPreferences
============
*/
void DoPreferences ()
{
	DialogBox(g_qeglobals.d_hInstance, MAKEINTRESOURCE(IDD_PREFERENCES), g_qeglobals.d_hwndMain, PreferencesDlgProc);
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
BOOL CALLBACK ScaleDlgProc (
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
		if (!g_qeglobals.d_savedinfo.bNoClamp)
		{
			g_qeglobals.d_savedinfo.bNoClamp = true;
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
				g_qeglobals.d_savedinfo.bNoClamp = false;
				g_bSnapCheck = false;
			}
			EndDialog(hwndDlg, 1);
			Sys_UpdateWindows(W_XY | W_Z | W_CAMERA);
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
			Sys_ForceUpdateWindows(W_SCENE);			
			return TRUE;

		case IDCANCEL:
			// For more precise scaling
			if (g_bSnapCheck)
			{
				g_qeglobals.d_savedinfo.bNoClamp = false;
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
	DialogBox(g_qeglobals.d_hInstance, MAKEINTRESOURCE(IDD_SCALE), g_qeglobals.d_hwndMain, ScaleDlgProc);
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
BOOL CALLBACK CamSpeedDlgProc( 
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
		SendDlgItemMessage(hwndDlg, IDC_SLIDER_CAMSPEED, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)g_qeglobals.d_savedinfo.nCameraSpeed); 
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDCANCEL:
			g_qeglobals.d_savedinfo.nCameraSpeed = SendDlgItemMessage(hwndDlg, IDC_SLIDER_CAMSPEED, TBM_GETPOS, 0, 0);
			ReleaseCapture();
			EndDialog(hwndDlg, 1);
			break;
		}
		return 0;

	case WM_NCHITTEST:
		if (DefWindowProc(hwndDlg, uMsg, wParam, lParam) != HTCLIENT)
		{
			g_qeglobals.d_savedinfo.nCameraSpeed = SendDlgItemMessage(hwndDlg, IDC_SLIDER_CAMSPEED, TBM_GETPOS, 0, 0);
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
	DialogBox(g_qeglobals.d_hInstance, MAKEINTRESOURCE(IDD_CAMSPEED), g_qeglobals.d_hwndMain, CamSpeedDlgProc);
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
BOOL CALLBACK DefaultTexScaleDlgProc (
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
	DialogBox(g_qeglobals.d_hInstance, MAKEINTRESOURCE(IDD_TEXTURESCALE), g_qeglobals.d_hwndMain, DefaultTexScaleDlgProc);
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
BOOL CALLBACK FindKeyValueDlgProc (
    HWND	hwndDlg,// handle to dialog box
    UINT	uMsg,	// message
    WPARAM	wParam,	// first message parameter
    LPARAM	lParam 	// second message parameter
   )
{
	char	szKey[256];
	char	szValue[256];
	HWND	h;

	switch (uMsg)
    {
	case WM_INITDIALOG:
		h = GetDlgItem(hwndDlg, IDC_EDIT_FINDKEY);
		SetFocus(h);
		return FALSE;

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
	DialogBox(g_qeglobals.d_hInstance, MAKEINTRESOURCE(IDD_FINDKEYVALUE), g_qeglobals.d_hwndMain, FindKeyValueDlgProc);
}
// <---sikk