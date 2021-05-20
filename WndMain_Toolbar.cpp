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

HWND g_hwndToolbar[11];
HWND g_hwndRebar;

/*
==============
WndMain_CreateToolBar
==============
*/
HWND WndMain_CreateToolBar(HWND hWnd, HINSTANCE hInst, int nIndex, int nPos, int nButtons)
{
	HWND			hwndTB;
	REBARBANDINFO	rbBand;
	DWORD			dwBtnSize;
	int				barSize;

	// Ensure that the common control DLL is loaded. 
	InitCommonControls();

	// Fill the TBBUTTON array with button information, 
	// and add the buttons to the toolbar.
	TBBUTTON tbbtns[] = {
		// bitmap, command, state, style, data, string
		{ 0, ID_FILE_NEW, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 1, ID_FILE_OPEN, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 2, ID_FILE_SAVE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 3, ID_FILE_SAVEAS, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		//{ 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0, 0 },
		//{ 4, ID_FILE_PRINTXY, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },

		{ 6, ID_EDIT_CUT, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 7, ID_EDIT_COPY, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 8, ID_EDIT_PASTE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0, 0 },
		{ 9, ID_SELECTION_CLONE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 10, ID_SELECTION_INVERT, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 11, ID_SELECTION_DELETE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0, 0 },
		{ 12, ID_EDIT_UNDO, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 13, ID_EDIT_REDO, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0, 0 },
		{ 14, ID_EDIT_FINDBRUSH, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 59, ID_VIEW_FILTER_POPUP, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0, 0 },
		{ 15, ID_EDIT_PREFERENCES, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 16, ID_EDIT_MAPINFO, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 17, ID_EDIT_ENTITYINFO, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },

		{ 18, ID_SELECTION_FLIPX, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 19, ID_SELECTION_ROTATEX, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 20, ID_SELECTION_FLIPY, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 21, ID_SELECTION_ROTATEY, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 22, ID_SELECTION_FLIPZ, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 23, ID_SELECTION_ROTATEZ, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },

		{ 24, ID_SELECTION_SELECTCOMPLETETALL, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 25, ID_SELECTION_SELECTTOUCHING, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 26, ID_SELECTION_SELECTPARTIALTALL, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 27, ID_SELECTION_SELECTINSIDE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },

		{ 28, ID_SELECTION_CSGHOLLOW, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 29, ID_SELECTION_CSGSUBTRACT, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 30, ID_SELECTION_CSGMERGE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },

		{ 58, ID_TOOLS_DRAWBRUSHESTOOL, TBSTATE_ENABLED, TBSTYLE_CHECK, 0, 0 },
		{ 31, ID_SELECTION_CLIPPER, TBSTATE_ENABLED, TBSTYLE_CHECK, 0, 0 },
		{ 32, ID_SELECTION_DRAGEDGES, TBSTATE_ENABLED, TBSTYLE_CHECK, 0, 0 },
		{ 33, ID_SELECTION_DRAGVERTICES, TBSTATE_ENABLED, TBSTYLE_CHECK, 0, 0 },
		{ 57, ID_SELECTION_DRAGFACES, TBSTATE_ENABLED, TBSTYLE_CHECK, 0, 0 },

		{ 37, ID_SELECTION_UNGROUPENTITY, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 38, ID_SELECTION_CONNECT, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 39, ID_SELECTION_GROUPNEXTBRUSH, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		
		{ 40, ID_PRIMITIVES_CZGCYLINDER1, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 41, ID_PRIMITIVES_CZGCYLINDER2, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		
		{ 43, ID_TEXTURES_REPLACEALL, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 44, ID_TEXTURES_INSPECTOR, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },

		{ 45, ID_VIEW_CONSOLE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 46, ID_VIEW_ENTITY, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 47, ID_VIEW_TEXTURE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0, 0 },
		{ 48, ID_VIEW_NEXTVIEW, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 49, ID_VIEW_CENTERONSELECTION, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0, 0 },
		{ 51, ID_VIEW_CAMSPEED, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 52, ID_VIEW_CUBICCLIP, TBSTATE_ENABLED, TBSTYLE_CHECK, 0, 0 },
		{ 53, ID_VIEW_CUBICCLIPZOOMOUT, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 54, ID_VIEW_CUBICCLIPZOOMIN, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },

		{ 55, ID_MISC_TESTMAP, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 56, ID_HELP_HELP, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 5, ID_HELP_ABOUT, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 }
	};

	hwndTB = CreateToolbarEx(hWnd,	// Handle to the parent window 
		QE3_TOOLBAR_STYLE,			// Styles
		ID_TOOLBAR,					// Control identifier
		nButtons,					// Number of button images 
		(HINSTANCE)hInst,			// Module instance 
		IDR_TOOLBAR2,				// Resource identifier for the bitmap resource
		(LPCTBBUTTON)&tbbtns[nPos],	// Address of an array of TBBUTTON 
		nButtons,					// Number of buttons to add to toolbar
		0, 0,						// width & height of buttons
		0, 0,						// width & height of bitmaps
		sizeof(TBBUTTON));			// Size of a TBBUTTON structure

	// Get the height of the toolbar buttons.
	dwBtnSize = SendMessage(hwndTB, TB_GETBUTTONSIZE, 0, 0);

	barSize = 4;
	for (int i = 0; i < nButtons; i++)
	{
		if (tbbtns[nPos + i].fsStyle == TBSTYLE_SEP)
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

	int tbSizes[11] = { 4, 16, 6, 4, 3, 5, 3, 2, 2, 11, 3 };
	int offset = 0;
	for (int i = 0; i < 11; i++)
	{
		g_hwndToolbar[i] = WndMain_CreateToolBar(hwndRB, hInst, i, offset, tbSizes[i]);
		offset += tbSizes[i];
	}

	g_hwndRebar = hwndRB;
}

