//==============================
//	WndMain_Toolbar.cpp
//==============================

#include "pre.h"
#include "qe3.h"
#include "WndMain.h"

/*
===============================================================

	TOOLBAR

===============================================================
*/

HWND g_hwndToolbar[REBARBANDCOUNT];
HWND g_hwndRebar;

const char qe_rebarRegKey[] = "QERebar";

struct rebarPos_t {
	int id;
	RECT rc;
	BOOL vis;
};

struct qeRebarPositions_t {
	long nSize;
	long nCount;
	rebarPos_t positions[REBARBANDCOUNT];
};

/*
==============
WndMain_GetRebarPosition
==============
*/
rebarPos_t WndMain_GetRebarPosition(int tbID)
{
	rebarPos_t pos;
	pos.id = tbID;
	pos.vis = IsWindowVisible(g_hwndToolbar[tbID]);
	// screen space to rebar space
	GetWindowRect(g_hwndToolbar[tbID], &pos.rc);
	MapWindowPoints(NULL, g_hwndRebar, (POINT *)&pos.rc, 2);
	return pos;
}

/*
==============
WndMain_SetRebarPosition
==============
*/
void WndMain_SetRebarPosition(rebarPos_t pos)
{
	HWND hwnd = g_hwndToolbar[pos.id];

	//ShowWindow(hwnd, pos.vis ? SW_SHOW : SW_HIDE);
	if (pos.vis)
		SendMessage(g_hwndRebar, RB_SHOWBAND, (WPARAM)pos.id, (LPARAM)TRUE);
	else
		SendMessage(g_hwndRebar, RB_SHOWBAND, (WPARAM)pos.id, (LPARAM)FALSE);
	MoveWindow(hwnd, pos.rc.left, pos.rc.top, pos.rc.right - pos.rc.left, pos.rc.bottom - pos.rc.top, TRUE);
}

/*
==============
WndMain_LoadRebarPositions
==============
*/
bool WndMain_LoadRebarPositions(qeRebarPositions_t &posst)
{
	long l = sizeof(posst);
	if (!Sys_RegistryLoadInfo(qe_rebarRegKey, &posst, &l))
		return false;
	return (posst.nSize == l && posst.nCount == REBARBANDCOUNT);

	// sikk---> Save Rebar Band Info
/*	for (int i = 0; i < REBARBANDCOUNT; i++)
	{
		g_qeglobals.d_savedinfo.rbiSettings[i].cbSize	= sizeof(REBARBANDINFO);
		SendMessage(g_hwndRebar, RB_SETBANDINFO, (WPARAM)i, (LPARAM)(LPREBARBANDINFO)&g_qeglobals.d_savedinfo.rbiSettings[i]);
	}
	Code below Sets the saved Band order but doesn't function correctly
	as it doesn't update until a band is moved. ???
	int j = 0;
	while (j < REBARBANDCOUNT)
	{
		for (int i = 0; i < REBARBANDCOUNT; i++)
			if (ID_TOOLBAR + i == g_qeglobals.d_savedinfo.nRebarSavedIndex[j])
				SendMessage(g_hwndRebar, RB_MOVEBAND, (WPARAM)i, (LPARAM)j);

		j++;
	}
*/
// <---sikk


}

/*
==============
WndMain_SaveRebarPositions
==============
*/
void WndMain_SaveRebarPositions()
{
	qeRebarPositions_t posst;
	posst.nSize = sizeof(qeRebarPositions_t);
	posst.nCount = REBARBANDCOUNT;

	for (int i = 0; i < REBARBANDCOUNT; i++)
		posst.positions[i] = WndMain_GetRebarPosition(i);

	Sys_RegistrySaveInfo(qe_rebarRegKey, &posst, sizeof(posst));

	// below: sikk's old bullshit that didn't work either

	// sikk---> Save Rebar Band Info
/*	for (int i = 0; i < REBARBANDCOUNT; i++)
	{
		int nBandIndex = SendMessage(g_hwndRebar, RB_IDTOINDEX, (WPARAM)ID_TOOLBAR + i, (LPARAM)0);
		//Log::Print("Band %d\n", nBandIndex);
		g_qeglobals.d_savedinfo.rbiSettings[i].cbSize = sizeof(REBARBANDINFO);
		g_qeglobals.d_savedinfo.rbiSettings[i].fMask = RBBIM_CHILDSIZE | RBBIM_STYLE;
		SendMessage(g_hwndRebar, RB_GETBANDINFO, (WPARAM)nBandIndex, (LPARAM)(LPREBARBANDINFO)&g_qeglobals.d_savedinfo.rbiSettings[i]);
	}

	Code below saves the current Band order but there is a problem with
	updating this at program start. (check QE_Init() in qe3.c)
	int j = 0;
	while (j < REBARBANDCOUNT)
	{
		for (i = 0; i < REBARBANDCOUNT; i++)
		{
			nBandIndex = SendMessage(g_hwndRebar, RB_IDTOINDEX, (WPARAM)ID_TOOLBAR + i, (LPARAM)0);
			if (nBandIndex == j)
				g_qeglobals.d_savedinfo.nRebarSavedIndex[j] = ID_TOOLBAR + i;
		}
		j++;
	}
	*/
	// <---sikk

	// FIXME: is this right?
	//Sys_RegistrySaveInfo("SavedInfo", &g_qeglobals.d_savedinfo, sizeof(g_qeglobals.d_savedinfo));

}

/*
==============
WndMain_CreateToolBar
==============
*/
HWND WndMain_CreateToolBar(HWND hWnd, HINSTANCE hInst, int nIndex, TBBUTTON* tbbuttons, int nButtons)
{
	HWND			hwndTB;
	REBARBANDINFO	rbBand;
	DWORD			dwBtnSize;
	int				barSize;

	hwndTB = CreateToolbarEx(hWnd,	// Handle to the parent window 
		QE3_TOOLBAR_STYLE,			// Styles
		ID_TOOLBAR,					// Control identifier
		nButtons,					// Number of button images 
		(HINSTANCE)hInst,			// Module instance 
		IDR_TOOLBAR2,				// Resource identifier for the bitmap resource
		(LPCTBBUTTON)tbbuttons,		// Address of an array of TBBUTTON 
		nButtons,					// Number of buttons to add to toolbar
		0, 0,						// width & height of buttons
		0, 0,						// width & height of bitmaps
		sizeof(TBBUTTON));			// Size of a TBBUTTON structure

	// Get the height of the toolbar buttons.
	dwBtnSize = SendMessage(hwndTB, TB_GETBUTTONSIZE, 0, 0);

	barSize = 4;
	for (int i = 0; i < nButtons; i++)
	{
		if (tbbuttons[i].fsStyle == TBSTYLE_SEP)
			barSize += 10;
		else
			barSize += HIWORD(dwBtnSize); // 22
	}

	// Initialize REBARBANDINFO for all coolbar bands.
	rbBand.cbSize = REBARBANDINFO_V3_SIZE;// sizeof(REBARBANDINFO);
	rbBand.fMask = RBBIM_STYLE |		// fStyle is valid
		RBBIM_CHILD |		// hwndChild is valid
		RBBIM_CHILDSIZE |	// cxMinChild and cyMinChild are valid
		RBBIM_SIZE |		// cx is valid
		RBBIM_IDEALSIZE |	// cxIdeal is valid
		RBBIM_ID;			// wID is valid
	rbBand.fStyle = RBBS_CHILDEDGE | RBBS_GRIPPERALWAYS;
	rbBand.hwndChild = hwndTB;
	rbBand.cxMinChild = barSize;
	rbBand.cyMinChild = HIWORD(dwBtnSize);
	rbBand.cx = barSize;
	rbBand.cxIdeal = barSize;
	rbBand.wID = ID_TOOLBAR + nIndex;

	// Insert band into rebar.
	if (!SendMessage(hWnd, RB_INSERTBAND, (WPARAM)nIndex, (LPARAM)(LPREBARBANDINFO)&rbBand))
	{
		return NULL;
	}
	return hwndTB;
}


/*
==============
WndMain_CreateReBar
==============
*/
void WndMain_CreateReBar(HWND hWnd, HINSTANCE hInst)
{
	HWND		hwndRB;
	REBARINFO	rbi;
	INITCOMMONCONTROLSEX icex;

	// Ensure that the common control DLL is loaded. 
	InitCommonControls();
	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC = ICC_COOL_CLASSES | ICC_BAR_CLASSES;
	InitCommonControlsEx(&icex);

	hwndRB = CreateWindowEx(
		WS_EX_TOOLWINDOW,	// extended window style
		REBARCLASSNAME,		// registered class name
		NULL,				// window name
		QE3_REBAR_STYLE,	// window style
		0, 0, 0, 0,			// window position and size
		hWnd,				// parent or owner window
		(HMENU)ID_REBAR,	// menu or child-window identifier
		hInst,				// application instance
		NULL);				// window-creation data

	if (!hwndRB)
		return;

	// Initialize and send the REBARINFO structure.
	rbi.cbSize = sizeof(REBARINFO);	// Required when using this struct.
	rbi.fMask = 0;
	rbi.himl = (HIMAGELIST)NULL;
	if (!SendMessage(hwndRB, RB_SETBARINFO, 0, (LPARAM)&rbi))
		return;

	int tb = 0;

#define QEMAKETOOLBARBAND(x)	g_hwndToolbar[tb] = WndMain_CreateToolBar(hwndRB, hInst, tb++, (x), (sizeof(x)/sizeof(TBBUTTON)))

	TBBUTTON tb_file[] = {	// 0
		// bitmap, command, state, style, data, string
		{ 0, ID_FILE_NEW, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 1, ID_FILE_OPEN, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 2, ID_FILE_SAVE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 3, ID_FILE_SAVEAS, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
	};
	QEMAKETOOLBARBAND(tb_file);

	TBBUTTON tb_edit[] = {	// 1
		{ 6, ID_EDIT_CUT, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 7, ID_EDIT_COPY, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 8, ID_EDIT_PASTE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 14, ID_EDIT_FINDBRUSH, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0, 0 },
		{ 9, ID_SELECTION_CLONE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 10, ID_SELECTION_INVERT, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 11, ID_SELECTION_DELETE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0, 0 },
		{ 12, ID_EDIT_UNDO, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 13, ID_EDIT_REDO, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
	};
	QEMAKETOOLBARBAND(tb_edit);

	TBBUTTON tb_window[] = {	// 2
		{ 45, ID_VIEW_CONSOLE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 46, ID_VIEW_ENTITY, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 47, ID_VIEW_TEXTURE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0, 0 },
		{ 16, ID_EDIT_MAPINFO, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 17, ID_EDIT_ENTITYINFO, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 15, ID_EDIT_PREFERENCES, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
	};
	QEMAKETOOLBARBAND(tb_window);

	TBBUTTON tb_trans[] = {	// 3
		{ 18, ID_SELECTION_FLIPX, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 19, ID_SELECTION_ROTATEX, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 20, ID_SELECTION_FLIPY, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 21, ID_SELECTION_ROTATEY, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 22, ID_SELECTION_FLIPZ, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 23, ID_SELECTION_ROTATEZ, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
	};
	QEMAKETOOLBARBAND(tb_trans);

	TBBUTTON tb_sel[] = {	// 4
		{ 24, ID_SELECTION_SELECTCOMPLETETALL, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 25, ID_SELECTION_SELECTTOUCHING, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 26, ID_SELECTION_SELECTPARTIALTALL, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 27, ID_SELECTION_SELECTINSIDE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
	};
	QEMAKETOOLBARBAND(tb_sel);

	TBBUTTON tb_csg[] = {	// 5
		{ 28, ID_SELECTION_CSGHOLLOW, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 29, ID_SELECTION_CSGSUBTRACT, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 30, ID_SELECTION_CSGMERGE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
	};
	QEMAKETOOLBARBAND(tb_csg);

	TBBUTTON tb_tools[] = {	// 6
		{ 58, ID_TOOLS_DRAWBRUSHESTOOL, TBSTATE_ENABLED, TBSTYLE_CHECK, 0, 0 },
		{ 31, ID_SELECTION_CLIPPER, TBSTATE_ENABLED, TBSTYLE_CHECK, 0, 0 },
		{ 32, ID_SELECTION_DRAGEDGES, TBSTATE_ENABLED, TBSTYLE_CHECK, 0, 0 },
		{ 33, ID_SELECTION_DRAGVERTICES, TBSTATE_ENABLED, TBSTYLE_CHECK, 0, 0 },
		{ 57, ID_SELECTION_DRAGFACES, TBSTATE_ENABLED, TBSTYLE_CHECK, 0, 0 },
	};
	QEMAKETOOLBARBAND(tb_tools);

	TBBUTTON tb_ent[] = {	// 7
		{ 37, ID_SELECTION_UNGROUPENTITY, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 38, ID_SELECTION_CONNECT, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 39, ID_SELECTION_GROUPNEXTBRUSH, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
	};
	QEMAKETOOLBARBAND(tb_ent);

	TBBUTTON tb_czg[] = {	// 8
		{ 40, ID_PRIMITIVES_CZGCYLINDER1, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 41, ID_PRIMITIVES_CZGCYLINDER2, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
	};
	QEMAKETOOLBARBAND(tb_czg);

	TBBUTTON tb_tex[] = {	// 9
		{ 43, ID_TEXTURES_REPLACEALL, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 44, ID_TEXTURES_INSPECTOR, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
	};
	QEMAKETOOLBARBAND(tb_tex);

	TBBUTTON tb_view[] = {	// 10
		{ 59, ID_VIEW_FILTER_POPUP, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 48, ID_VIEW_NEXTVIEW, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 49, ID_VIEW_CENTERONSELECTION, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0, 0 },
		{ 51, ID_VIEW_CAMSPEED, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 52, ID_VIEW_CUBICCLIP, TBSTATE_ENABLED, TBSTYLE_CHECK, 0, 0 },
		{ 53, ID_VIEW_CUBICCLIPNEARER, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 54, ID_VIEW_CUBICCLIPFARTHER, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
	};
	QEMAKETOOLBARBAND(tb_view);

	TBBUTTON tb_misc[] = {	// 11
		{ 55, ID_MISC_TESTMAP, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 56, ID_HELP_HELP, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 5, ID_HELP_ABOUT, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 }
	};
	QEMAKETOOLBARBAND(tb_misc);

	assert(tb == REBARBANDCOUNT);

	g_hwndRebar = hwndRB;

	qeRebarPositions_t posst;
	if (WndMain_LoadRebarPositions(posst))
	{
		for (int i = 0; i < REBARBANDCOUNT; i++)
		{
			WndMain_SetRebarPosition(posst.positions[i]);
		}
	}
	else
	{
		for (int i = 0; i < REBARBANDCOUNT; i++)
		{
			SendMessage(g_hwndRebar, RB_SHOWBAND, (WPARAM)i, (LPARAM)TRUE);
		}
	}		

	InvalidateRect(g_hwndRebar, NULL, FALSE);
}

