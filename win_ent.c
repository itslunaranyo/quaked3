//==============================
//	win_ent.c
//==============================

#include "qe3.h"


#define DLGBORDER_X		5
#define DLGBORDER_Y		5

int g_nEntDlgIds[ENT_LAST] = 
{
	IDC_E_LIST,
	IDC_E_COMMENT,
	IDC_E_FLAG1,
	IDC_E_FLAG2,
	IDC_E_FLAG3,
	IDC_E_FLAG4,
	IDC_E_FLAG5,
	IDC_E_FLAG6,
	IDC_E_FLAG7,
	IDC_E_FLAG8,
	IDC_E_FLAG9,
	IDC_E_FLAG10,
	IDC_E_FLAG11,
	IDC_E_FLAG12,
	IDC_E_PROPS,
	IDC_E_0,
	IDC_E_45,
	IDC_E_90,
	IDC_E_135,
	IDC_E_180,
	IDC_E_225,
	IDC_E_270,
	IDC_E_315,
	IDC_E_UP,
	IDC_E_DOWN,
	IDC_E_ADDPROP,	// sikk - Entity Window Addition
	IDC_E_DELPROP,
	IDC_E_CREATE,	// sikk - Entity Window Addition

	IDC_STATIC_KEY,
	IDC_E_KEY_FIELD,
	IDC_STATIC_VALUE,
	IDC_E_VALUE_FIELD,

	IDC_E_COLOR
};

HWND		g_hwndEnt[ENT_LAST];
bool		g_bMultipleEntities;
entity_t   *g_peEditEntity;

static void EntWnd_ToTop()
{
	if (GetTopWindow(g_qeglobals.d_hwndMain) != g_qeglobals.d_hwndEntity)
		BringWindowToTop(g_qeglobals.d_hwndEntity);
	SetFocus(g_qeglobals.d_hwndCamera);
}



/*
=========================
FieldWndProc

Just to handle tab and enter...
=========================
*/
BOOL CALLBACK FieldWndProc (
    HWND	hWnd,	// handle to window
    UINT	uMsg,	// message
    WPARAM	wParam,	// first message parameter
    LPARAM	lParam 	// second message parameter
	)
{
    switch (uMsg)
    {
	case WM_CHAR:
		if (LOWORD(wParam) == VK_TAB)
			return FALSE;
		if (LOWORD(wParam) == VK_RETURN)
			return FALSE;
		if (LOWORD(wParam) == VK_ESCAPE)
		{
			SetFocus(g_qeglobals.d_hwndCamera);
			return FALSE;
		}
		break;

	case WM_KEYDOWN:
		if (LOWORD(wParam) == VK_TAB)
		{
			if (hWnd == g_hwndEnt[ENT_KEYFIELD])
			{
				SendMessage(g_hwndEnt[ENT_VALUEFIELD], WM_SETTEXT, 0, (long)"");
				SetFocus(g_hwndEnt[ENT_VALUEFIELD]);
			}
			else
				SetFocus(g_hwndEnt[ENT_KEYFIELD]);
		}
		if (LOWORD(wParam) == VK_RETURN)
		{
			if (hWnd == g_hwndEnt[ENT_KEYFIELD])
			{
				SendMessage(g_hwndEnt[ENT_VALUEFIELD], WM_SETTEXT, 0, (long)"");
				SetFocus(g_hwndEnt[ENT_VALUEFIELD]);
			}
			else
			{
				EntWnd_AddKeyValue();
//				SetFocus(g_qeglobals.d_hwndCamera);
				SetFocus(g_hwndEnt[ENT_PROPS]);	// sikk - Made sense to keep focus 
			}
		}
		break;

//	case WM_NCHITTEST:

	case WM_LBUTTONDOWN:
		EntWnd_ToTop();
		break;
	}
	return CallWindowProc(OldFieldWindowProc, hWnd, uMsg, wParam, lParam);
}

/*
=========================
EntityListWndProc

Just to handle enter...
=========================
*/
BOOL CALLBACK EntityListWndProc (
    HWND	hWnd,	// handle to window
    UINT	uMsg,	// message
    WPARAM	wParam,	// first message parameter
    LPARAM	lParam 	// second message parameter
	)
{
    switch (uMsg)
    {
	case WM_KEYDOWN:
		if (LOWORD(wParam) == VK_RETURN)
		{
			SendMessage(g_qeglobals.d_hwndEntity, WM_COMMAND, (LBN_DBLCLK << 16) + IDC_E_LIST, 0);
			return 0;
		}
		break;

// sikk---> LMB Bring to Top
	case WM_LBUTTONDOWN:
		if (GetTopWindow(g_qeglobals.d_hwndMain) != g_qeglobals.d_hwndEntity)
			BringWindowToTop(g_qeglobals.d_hwndEntity);
		break;
// <---sikk
	}
	return CallWindowProc(OldEntityListWindowProc, hWnd, uMsg, wParam, lParam);
}

/*
================
GetEntityControls

Finds the controls from the dialog and
moves them to the window
================
*/
void GetEntityControls (HWND h)
{
	int i;

	for (i = 0; i < ENT_LAST; i++)
	{
		if (i == ENT_CLASSLIST || i == ENT_PROPS || i == ENT_COMMENT)
			continue;
		if (i == ENT_KEYFIELD || i == ENT_VALUEFIELD)
			continue;
		g_hwndEnt[i] = GetDlgItem(h, g_nEntDlgIds[i]);		
		if (g_hwndEnt[i])
		{
			SetParent(g_hwndEnt[i], g_qeglobals.d_hwndEntity);
			SendMessage(g_hwndEnt[i], WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), (LPARAM)TRUE);
		}
	}

	LoadLibraryA("RICHED32.DLL") ;	// sikk - For Enhanced Editing	

	// SetParent apears to not modify some internal state
	// on listboxes, so create it from scratch...

// sikk---> Misc aesthetic changes
	g_hwndEnt[ENT_CLASSLIST] = CreateWindowEx(WS_EX_CLIENTEDGE, 
		"listbox", 
		NULL, 
		LBS_NOTIFY | LBS_SORT | LBS_NOINTEGRALHEIGHT | LBS_WANTKEYBOARDINPUT | WS_CHILD | WS_VISIBLE | WS_VSCROLL,
		5, 5, 180, 99,
		g_qeglobals.d_hwndEntity,
		(void *)IDC_E_LIST,
		g_qeglobals.d_hInstance,
		NULL);
	if (!g_hwndEnt[ENT_CLASSLIST])
		Error("CreateWindowEx: Failed.");

	g_hwndEnt[ENT_PROPS] = CreateWindowEx(WS_EX_CLIENTEDGE, 
		"listbox", 
		NULL, 
		LBS_NOTIFY | LBS_SORT | LBS_NOINTEGRALHEIGHT | LBS_USETABSTOPS | WS_CHILD | WS_VISIBLE | WS_VSCROLL,
		5, 100, 180, 99,
		g_qeglobals.d_hwndEntity,
		(void *)IDC_E_PROPS,
		g_qeglobals.d_hInstance,
		NULL);
	if (!g_hwndEnt[ENT_PROPS])
		Error("CreateWindowEx: Failed.");

	g_hwndEnt[ENT_COMMENT] = CreateWindowEx(WS_EX_CLIENTEDGE, 
		"edit", 
		NULL, 
		ES_MULTILINE | ES_READONLY | WS_CHILD | WS_TABSTOP | WS_VISIBLE | WS_VSCROLL,
		5, 100, 180, 99,
		g_qeglobals.d_hwndEntity,
		(void *)IDC_E_COMMENT,
		g_qeglobals.d_hInstance,
		NULL);
	if (!g_hwndEnt[ENT_COMMENT])
		Error("CreateWindowEx: Failed.");

	g_hwndEnt[ENT_KEYFIELD] = CreateWindowEx(WS_EX_CLIENTEDGE, 
		"RichEdit", 
		NULL, 
		ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE,
		5, 100, 180, 99,
		g_qeglobals.d_hwndEntity,
		(void *)IDC_E_KEY_FIELD,
		g_qeglobals.d_hInstance,
		NULL);
	if (!g_hwndEnt[ENT_KEYFIELD])
		Error("CreateWindowEx: Failed.");

	g_hwndEnt[ENT_VALUEFIELD] = CreateWindowEx(WS_EX_CLIENTEDGE, 
		"RichEdit", 
		NULL, 
		ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE,
		5, 100, 180, 99,
		g_qeglobals.d_hwndEntity,
		(void *)IDC_E_VALUE_FIELD,
		g_qeglobals.d_hInstance,
		NULL);
	if (!g_hwndEnt[ENT_VALUEFIELD])
		Error("CreateWindowEx: Failed.");

	g_qeglobals.d_hwndConsole = CreateWindowEx(WS_EX_CLIENTEDGE, 
		"RichEdit", 
		NULL, 
		ES_MULTILINE | ES_READONLY | WS_CHILD | WS_TABSTOP | WS_VISIBLE | WS_VSCROLL,
		5, 100, 180, 99,
		g_qeglobals.d_hwndEntity,
		(void *)IDC_E_STATUS,
		g_qeglobals.d_hInstance,
		NULL);
	if (!g_qeglobals.d_hwndConsole)
		Error("CreateWindowEx: Failed.");
// <---sikk

	g_qeglobals.d_hwndTexture = WTex_Create(g_qeglobals.d_hInstance);

#if 0
	for (i = 0; i < 12; i++)
	{
		g_hwndEnt[ENT_CHECK1 + i] = CreateWindow("button", 
			NULL, 
			BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE,
			5, 100, 180, 99,
			entwindow,
			(void *)IDC_E_STATUS,
			main_instance,
			NULL);
		if (!g_hwndEnt[ENT_CHECK1 + i])
			Error("CreateWindow: Failed.");
	}
#endif

	SendMessage(g_hwndEnt[ENT_CLASSLIST], WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), (LPARAM)TRUE);
	SendMessage(g_hwndEnt[ENT_PROPS], WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), (LPARAM)TRUE);
	SendMessage(g_hwndEnt[ENT_COMMENT], WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), (LPARAM)TRUE);
	SendMessage(g_hwndEnt[ENT_KEYFIELD], WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), (LPARAM)TRUE);
	SendMessage(g_hwndEnt[ENT_VALUEFIELD], WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), (LPARAM)TRUE);

    SendMessage(g_qeglobals.d_hwndConsole, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), (LPARAM)TRUE);
}

/*
===============================================================

ENTITY WINDOW

===============================================================
*/

/*
==============
EntWnd_FillClassList
==============
*/
void EntWnd_FillClassList ()
{
	eclass_t   *pec;
	int			iIndex;

	SendMessage(g_hwndEnt[ENT_CLASSLIST], LB_RESETCONTENT, 0 , 0);

	for (pec = g_pecEclass; pec; pec = pec->next)
	{
		iIndex = SendMessage(g_hwndEnt[ENT_CLASSLIST], LB_ADDSTRING, 0 , (LPARAM)pec->name);
		SendMessage(g_hwndEnt[ENT_CLASSLIST], LB_SETITEMDATA, iIndex, (LPARAM)pec);
	}	

}

/*
==============
EntWnd_CreateWindow
==============
*/
static void EntWnd_CreateWindow (HINSTANCE hInstance)
{
    WNDCLASS   wc;

    /* Register the entity class */
	memset(&wc, 0, sizeof(wc));

    wc.style         = 0;
    wc.lpfnWndProc   = (WNDPROC)EntityWndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = 0;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);	// sikk - was-> GetStockObject (LTGRAY_BRUSH);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = ENT_WINDOW_CLASS;

    if (!RegisterClass(&wc))
        Error("WEnt_Register: Failed.");

// sikk---> changed window to "tool window"
	g_qeglobals.d_hwndEntity = CreateWindowEx(
		WS_EX_TOOLWINDOW,							// Extended window style
		ENT_WINDOW_CLASS ,							// registered class name
		"Entity View",								// window name
		QE3_CHILD_STYLE,							// window style
		g_nScreenWidth - 320, g_nScreenHeight - 520,	// position of window
		100, 480,									// window size
		g_qeglobals.d_hwndMain,						// parent or owner window
		0,											// menu or child-window identifier
		hInstance,									// application instance
		NULL);										// window-creation data
// <---sikk

	if (!g_qeglobals.d_hwndEntity)
		Error("Could not create Entity Window.");
}

/*
==============
EntWnd_Create
==============
*/
void EntWnd_Create (HINSTANCE hInstance)
{
	HWND h;

	g_qeglobals.d_nInspectorMode = W_ENTITY;

	EntWnd_CreateWindow(hInstance);

	h = CreateDialog(hInstance, (char *)IDD_ENTITY, g_qeglobals.d_hwndMain, (DLGPROC)NULL);
	if (!h)
		Error("CreateDialog: Failed.");

	GetEntityControls(h);
	DestroyWindow(h);

	OldFieldWindowProc = (void *)GetWindowLong(g_hwndEnt[ENT_KEYFIELD], GWL_WNDPROC);
	SetWindowLong(g_hwndEnt[ENT_KEYFIELD], GWL_WNDPROC, (long)FieldWndProc);
	SetWindowLong(g_hwndEnt[ENT_VALUEFIELD], GWL_WNDPROC, (long)FieldWndProc);

	OldEntityListWindowProc = (void *)GetWindowLong(g_hwndEnt[ENT_CLASSLIST], GWL_WNDPROC);
	SetWindowLong(g_hwndEnt[ENT_CLASSLIST], GWL_WNDPROC, (long)EntityListWndProc);

	EntWnd_FillClassList();

	LoadWindowState(g_qeglobals.d_hwndEntity, "EntityWindow");

	ShowWindow(g_qeglobals.d_hwndEntity, SW_SHOW);
	SetInspectorMode(W_CONSOLE);
}

/*
==============
SetInspectorMode
==============
*/
void SetInspectorMode (int nType)
{
	RECT	rc;
	HMENU	hMenu = GetMenu(g_qeglobals.d_hwndMain);

	// Is the caller asking us to cycle to the next window?
	if (nType == -1)
	{
		if (g_qeglobals.d_nInspectorMode == W_ENTITY)
			nType = W_TEXTURE;
		else if (g_qeglobals.d_nInspectorMode == W_TEXTURE)
			nType = W_CONSOLE;
		else
			nType = W_ENTITY;
	}		

	g_qeglobals.d_nInspectorMode = nType;
	switch(nType)
	{
	case W_ENTITY:
		SetWindowText(g_qeglobals.d_hwndEntity, "Entities");
		EnableMenuItem(hMenu, ID_MISC_SELECTENTITYCOLOR, MF_ENABLED);
		break;

	case W_TEXTURE:
//		SetWindowText(g_qeglobals.d_hwndEntity, "Textures"); // title is set by textures.c
		EnableMenuItem(hMenu, ID_MISC_SELECTENTITYCOLOR, MF_GRAYED | MF_DISABLED);
		break;

	case W_CONSOLE:
		SetWindowText(g_qeglobals.d_hwndEntity, "Console");		
		EnableMenuItem(hMenu, ID_MISC_SELECTENTITYCOLOR, MF_GRAYED | MF_DISABLED);
		break;

	default:
		break;
	}

	GetWindowRect(g_qeglobals.d_hwndEntity, &rc);
	EntWnd_Resize(rc.right - rc.left - 8, rc.bottom - rc.top - 32);

	RedrawWindow(g_qeglobals.d_hwndEntity, NULL, NULL, 
				 RDW_ERASE | RDW_INVALIDATE | RDW_ERASENOW | RDW_UPDATENOW | RDW_ALLCHILDREN);

	SetWindowPos(g_qeglobals.d_hwndEntity, HWND_TOP, 
				 rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, 
				 SWP_NOSIZE | SWP_NOMOVE);
}

/*
==============
EntWnd_RefreshKeyValues

Reset the key/value listbox and fill it with the kv pairs from the entity being edited
==============
*/
void EntWnd_RefreshKeyValues ()
{
	epair_t	   *pep;
	RECT		rc;
	char		sz[4096];
	int			nTabs[] = {96};	// sikk - Tab fix

	if (g_peEditEntity == NULL)
		return;

	// set key/value pair list
	GetWindowRect(g_hwndEnt[ENT_PROPS], &rc);
	SendMessage(g_hwndEnt[ENT_PROPS], LB_SETCOLUMNWIDTH, (rc.right - rc.left) / 2, 0);
	SendMessage(g_hwndEnt[ENT_PROPS], LB_RESETCONTENT, 0, 0);
	SendMessage(g_hwndEnt[ENT_PROPS], LB_SETTABSTOPS, (WPARAM)1, (LPARAM)(LPINT)nTabs);	// sikk - Tab fix

	// Walk through list and add pairs
	for (pep = g_peEditEntity->epairs; pep; pep = pep->next)
	{
		sprintf(sz, "%s\t%s", pep->key, pep->value);
		SendMessage(g_hwndEnt[ENT_PROPS], LB_ADDSTRING, 0, (LPARAM)sz);
	}
}


/*
==============
EntWnd_ApplyAngle

Apply the angle keyvalue to all selected entities
==============
*/
static void EntWnd_ApplyAngle(int ang)
{
	char sz[8];
	sprintf(sz, "%d", ang);

	if (g_bMultipleEntities)
	{
		for (brush_t *b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
			SetKeyValue(b->owner, "angle", sz);
	}
	else
	{
		SetKeyValue(g_peEditEntity, "angle", sz);
	}

	EntWnd_RefreshKeyValues();
	Sys_UpdateWindows(W_XY | W_CAMERA);
}

/*
==============
EntWnd_FlagChecked

Handle one spawnflag checkbox being clicked
==============
*/
static void EntWnd_FlagChecked(int flag)
{
	if (flag < 1 || flag > 12)
		return;	// sanity check, no such flag

	bool on = (SendMessage(g_hwndEnt[ENT_CHECK1 + flag-1], BM_GETCHECK, 0, 0) == BST_CHECKED);
	int f = 1 << (flag-1);

	if (g_bMultipleEntities)
	{
		for (brush_t *b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
			SetSpawnFlag(b->owner, f, on);
	}
	else
	{
		SetSpawnFlag(g_peEditEntity, f, on);
	}

	EntWnd_RefreshKeyValues();
}

/*
==============
EntWnd_FlagsFromEnt

Update the checkboxes to reflect the flag state of the entity
==============
*/
void EntWnd_FlagsFromEnt ()
{
	int		f, i, v;

	f = atoi(ValueForKey(g_peEditEntity, "spawnflags"));
	for (i = 0; i < 12; i++)
	{
		v = !!(f & (1 << i));
		SendMessage(g_hwndEnt[ENT_CHECK1 + i], BM_SETCHECK, v, 0);
	}
}

/*
==============
EntWnd_FlagsToEnt

Update the entity flags to reflect the state of the checkboxes
==============
*/
void EntWnd_FlagsToEnt ()
{
	int		f;
	int		i, v;
	char	sz[32];

	f = 0;
	for (i = 0; i < 12; i++)
	{
		v = SendMessage(g_hwndEnt[ENT_CHECK1 + i], BM_GETCHECK, 0, 0);
		f |= v << i;
	}

	sprintf(sz, "%d", f);

	if (g_bMultipleEntities)
	{
		brush_t	*b;

		for (b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
			SetKeyValue(b->owner, "spawnflags", sz);
	}
	else
		SetKeyValue(g_peEditEntity, "spawnflags", sz);

	EntWnd_RefreshKeyValues();
}

/*
==============
EntWnd_UpdateSel

Update the listbox, checkboxes and k/v pairs to reflect the new selection
==============
*/
bool EntWnd_UpdateSel (int nIndex, eclass_t *pec)
{
	int			i;
	brush_t	   *b;

	if (g_brSelectedBrushes.next == &g_brSelectedBrushes)
	{
		g_peEditEntity = g_peWorldEntity;
		g_bMultipleEntities = false;
	}
	else
	{
		g_peEditEntity = g_brSelectedBrushes.next->owner;
		for (b = g_brSelectedBrushes.next->next; b != &g_brSelectedBrushes; b = b->next)
		{
			if (b->owner != g_peEditEntity)
			{
				g_bMultipleEntities = true;
				break;
			}
		}
	}

	if (nIndex != LB_ERR)
		SendMessage(g_hwndEnt[ENT_CLASSLIST], LB_SETCURSEL, nIndex, 0);

	if (pec == NULL)
		return true;

	// Set up the description
	SendMessage(g_hwndEnt[ENT_COMMENT], WM_SETTEXT, 0, (LPARAM)TranslateString(pec->comments));

	for (i = 0; i < 8; i++)
	{
		HWND hwnd = g_hwndEnt[ENT_CHECK1 + i];
		if (pec->flagnames[i] && pec->flagnames[i][0] != 0)
		{
			EnableWindow(hwnd, TRUE);
			SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)pec->flagnames[i]);
		} 
		else 
		{
			// disable check box
			SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)" ");
			EnableWindow(hwnd, FALSE);
		}
	}

	EntWnd_FlagsFromEnt();
	EntWnd_RefreshKeyValues();
	return false;
}

/*
==============
EntWnd_UpdateEntitySel
==============
*/
bool EntWnd_UpdateEntitySel (eclass_t *pec)
{
	int iIndex;
	
	iIndex = (int)SendMessage(g_hwndEnt[ENT_CLASSLIST], LB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)pec->name);

	return EntWnd_UpdateSel(iIndex, pec);
}

/*
==============
EntWnd_CreateEntity

Creates a new entity based on the currently selected brush and entity type.
==============
*/
void EntWnd_CreateEntity ()
{
	eclass_t   *pecNew;
	entity_t   *petNew;
	int			i;
	HWND		hwnd;
	char		sz[1024];

	// check to make sure we have a brush
	if (g_brSelectedBrushes.next == &g_brSelectedBrushes)
	{
	    MessageBox(g_qeglobals.d_hwndMain, "You must have a brush selected to create an entity.", "QuakeEd 3: Entity Creation Info", MB_OK | MB_ICONINFORMATION);
		return;
	}

	// find out what type of entity we are trying to create
	hwnd = g_hwndEnt[ENT_CLASSLIST];

	i = SendMessage(g_hwndEnt[ENT_CLASSLIST], LB_GETCURSEL, 0, 0);

	if (i < 0)
	{
	    MessageBox(g_qeglobals.d_hwndMain, "You must have a class selected to create an entity.", "QuakeEd 3: Entity Creation Info", MB_OK | MB_ICONINFORMATION);
		return;
	}

	SendMessage(hwnd, LB_GETTEXT, i, (LPARAM)sz);

	if (!stricmp(sz, "worldspawn"))
	{
	    MessageBox(g_qeglobals.d_hwndMain, "Cannot create a new worldspawn entity.", "QuakeEd 3: Entity Creation Info", MB_OK | MB_ICONINFORMATION);
		return;
	}

	Undo_Start("Create Entity");	// sikk - Undo/Redo
	Undo_AddBrushList(&g_brSelectedBrushes);

	pecNew = Eclass_ForName(sz, false);

	// create it
	petNew = Entity_Create(pecNew);

	if (petNew == NULL)
	{
	    MessageBox(g_qeglobals.d_hwndMain, "Failed to create entity.", "QuakeEd 3: Entity Creation Error", MB_OK | MB_ICONEXCLAMATION);
		return;
	}

	if (g_brSelectedBrushes.next == &g_brSelectedBrushes)
		g_peEditEntity = g_peWorldEntity;
	else
		g_peEditEntity = g_brSelectedBrushes.next->owner;

	EntWnd_RefreshKeyValues();
	Select_Deselect(true);
	Select_Brush(g_peEditEntity->brushes.onext, true);

	Undo_EndBrushList(&g_brSelectedBrushes);
	Undo_End();	// sikk - Undo/Redo
}

/*
===============
EntWnd_AddKeyValue
===============
*/
void EntWnd_AddKeyValue ()
{
	char	key[4096];
	char	value[4096];

	if (g_peEditEntity == NULL)
		return;

	// Get current selection text
	SendMessage(g_hwndEnt[ENT_KEYFIELD], WM_GETTEXT, sizeof(key) - 1, (LPARAM)key);	
	SendMessage(g_hwndEnt[ENT_VALUEFIELD], WM_GETTEXT, sizeof(value) - 1, (LPARAM)value);	

	if (g_bMultipleEntities)
	{
		brush_t	*b;

		for (b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
			SetKeyValue(b->owner, key, value);
	}
	else
		SetKeyValue(g_peEditEntity, key, value);

	// refresh the prop listbox
	EntWnd_RefreshKeyValues();
}

/*
===============
EntWnd_RemoveKeyValue
===============
*/
void EntWnd_RemoveKeyValue ()
{
	char	sz[4096];

	if (g_peEditEntity == NULL)
		return;

	// Get current selection text
	SendMessage(g_hwndEnt[ENT_KEYFIELD], WM_GETTEXT, sizeof(sz) - 1, (LPARAM)sz);	

	if (g_bMultipleEntities)
	{
		brush_t	*b;

		for (b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
			DeleteKey(b->owner, sz);
	}
	else
		DeleteKey(g_peEditEntity, sz);

	// refresh the prop listbox
	EntWnd_RefreshKeyValues();	
}

/*
===============
EntWnd_EditKeyValue
===============
*/
void EntWnd_EditKeyValue ()
{
	int		i;
	HWND	hwnd;
	char	sz[4096];
	char   *val;

	if (g_peEditEntity == NULL)
		return;

	hwnd = g_hwndEnt[ENT_PROPS];

	// Get current selection text
	i = SendMessage(hwnd, LB_GETCURSEL, 0, 0);	

	if (i < 0)
		return;

	SendMessage(hwnd, LB_GETTEXT, i, (LPARAM)sz);	

	// strip it down to the key name
	for (i = 0; sz[i] != '\t'; i++)
		;

	sz[i] = '\0';

	val = sz + i + 1;
	if (*val == '\t')
		val++;

	SendMessage(g_hwndEnt[ENT_KEYFIELD], WM_SETTEXT, 0, (LPARAM)sz);	
	SendMessage(g_hwndEnt[ENT_VALUEFIELD], WM_SETTEXT, 0, (LPARAM)val);	
}

//HDWP	defer;	// sikk - unused
int		g_nCol;

/*
==============
MOVE
==============
*/
static void MOVE(HWND e, int x, int y, int w, int h)
{
	SetWindowPos(e, HWND_TOP, g_nCol + x, y, w, h, SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOZORDER);
}

static void MOVERECT(HWND e, RECT r)
{
	SetWindowPos(e, HWND_TOP, g_nCol + r.left, r.top, r.right, r.bottom, SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOZORDER);
}

static RECT ShrinkRect(RECT in, int shx, int shy)
{
	RECT out = in;

	out.left += shx;
	out.right -= 2 * shx;
	out.top += shy;
	out.bottom -= 2 * shy;

	return out;
}

/*
===============
EntWnd_Resize
===============
*/

void EntWnd_Resize(int nWidth, int nHeight)
{
	int		y, x, xCheck, yCheck;
	int		i, flagOffset;
	int		w, h, fold;

	if (nWidth < 32 || nHeight < 32)
		return;

	SendMessage(g_qeglobals.d_hwndEntity, WM_SETREDRAW, 0, 0);

	{
		//==========================================
		//
		// console
		//
		if (g_qeglobals.d_nInspectorMode == W_CONSOLE)
			g_nCol = 0;
		else
			g_nCol = nWidth;

		MOVE(g_qeglobals.d_hwndConsole, 0, 0, nWidth, nHeight);// DLGBORDER_X, DLGBORDER_Y, nWidth - (2 * DLGBORDER_X), nHeight - (2 * DLGBORDER_Y));

		//==========================================

		//
		// texture controls
		//
		if (g_qeglobals.d_nInspectorMode == W_TEXTURE)
			g_nCol = 0;
		else
			g_nCol = nWidth;

		MOVE(g_qeglobals.d_hwndTexture, 0, 0, nWidth, nHeight);// DLGBORDER_X, DLGBORDER_Y, nWidth - (2 * DLGBORDER_X), nHeight - (2 * DLGBORDER_Y));

		//==========================================

		//
		// entity controls
		//
		if (g_qeglobals.d_nInspectorMode == W_ENTITY)
			g_nCol = 0;
		else
			g_nCol = nWidth;
	}

	// yCheck = distance from top of one check to the next
	yCheck = 20;

	RECT rectClasses, rectComment, rectProps, rectFlags, rectKV, rectAngle;
	int flagRows, flagCols;
	fold = (4 * nHeight) / 9;

	if (nWidth > 450)
	{
		rectClasses.left = DLGBORDER_X;
		rectClasses.top = DLGBORDER_Y;
		rectClasses.right = 150 - DLGBORDER_X;	// width
		rectClasses.bottom = fold - DLGBORDER_Y * 2;	// height

		rectComment.left = 150 + DLGBORDER_X;
		rectComment.top = DLGBORDER_Y;
		rectComment.right = nWidth - 150 - DLGBORDER_X * 2;
		rectComment.bottom = fold - DLGBORDER_Y * 2;

		rectFlags.left = nWidth - 125;
		rectFlags.top = fold;
		rectFlags.right = 125 - DLGBORDER_X;
		rectFlags.bottom = nHeight - fold - DLGBORDER_Y;

		rectProps.left = DLGBORDER_X;
		rectProps.top = fold;
		rectProps.right = nWidth - 125 - DLGBORDER_X * 2;
		rectProps.bottom = nHeight - rectProps.top - 110;
		flagRows = 12;
		flagCols = 1;

		// keyvalue entry boxes
		rectKV.top = rectProps.top + rectProps.bottom + DLGBORDER_Y;
		rectKV.left = DLGBORDER_X;
		rectKV.bottom = 40 + DLGBORDER_Y;
		rectKV.right = nWidth - 175 - DLGBORDER_X * 2;
	}
	else
	{
		rectClasses.left = DLGBORDER_X;
		rectClasses.top = DLGBORDER_Y;
		rectClasses.right = nWidth - DLGBORDER_X * 2;	// width
		rectClasses.bottom = fold / 2 - DLGBORDER_Y;	// height

		rectComment.left = DLGBORDER_X;
		rectComment.top = DLGBORDER_Y + fold / 2;
		rectComment.right = nWidth - DLGBORDER_X * 2;
		rectComment.bottom = fold / 2 - DLGBORDER_Y * 2;

		rectFlags.left = DLGBORDER_X;
		rectFlags.top = fold;
		rectFlags.right = nWidth - 2 * DLGBORDER_X;
		rectFlags.bottom = yCheck * 4;

		flagRows = 4;
		flagCols = 3;

		rectProps.left = DLGBORDER_X;
		rectProps.top = rectFlags.top + rectFlags.bottom + DLGBORDER_Y;
		rectProps.right = nWidth - 2 * DLGBORDER_X;
		rectProps.bottom = nHeight - rectProps.top - 110;

		// keyvalue entry boxes
		rectKV.top = rectProps.top + rectProps.bottom + DLGBORDER_Y;
		rectKV.left = DLGBORDER_X;
		rectKV.bottom = 40 + DLGBORDER_Y;
		rectKV.right = nWidth - 50 - DLGBORDER_X * 2;
	}

	// angle selection and stuff
	rectAngle.left = DLGBORDER_X;
	rectAngle.top = rectKV.top + rectKV.bottom;
	rectAngle.right = 300;
	rectAngle.bottom = nHeight - rectAngle.top;
	
	MOVERECT(g_hwndEnt[ENT_CLASSLIST], rectClasses);
	MOVERECT(g_hwndEnt[ENT_COMMENT], rectComment);

	x = rectFlags.left;
	xCheck = nWidth / flagCols;	// xCheck = width of a single check box
	for (flagOffset = 0; flagOffset < 12; flagOffset += flagRows)
	{
		y = rectFlags.top;
		for (i = 0; i < flagRows; i++)
		{
			MOVE(g_hwndEnt[ENT_CHECK1 + i + flagOffset], x, y, xCheck, yCheck);
			y += yCheck;
		}
		x += xCheck;
	}

	MOVERECT(g_hwndEnt[ENT_PROPS], rectProps);

	w = rectKV.right - (DLGBORDER_X + 45);
	MOVE(g_hwndEnt[ENT_KEYLABEL], DLGBORDER_X, rectKV.top, 40, yCheck);
	MOVE(g_hwndEnt[ENT_KEYFIELD], DLGBORDER_X + 40, rectKV.top, rectKV.right, yCheck);
	MOVE(g_hwndEnt[ENT_VALUELABEL], DLGBORDER_X, rectKV.top + yCheck, 40, yCheck);
	MOVE(g_hwndEnt[ENT_VALUEFIELD], DLGBORDER_X + 40, rectKV.top + yCheck, rectKV.right, yCheck);

	x = rectAngle.right / 9;
	y = rectAngle.bottom / 3;
	MOVE(g_hwndEnt[ENT_DIR135], rectAngle.left, rectAngle.top, x, y);
	MOVE(g_hwndEnt[ENT_DIR90], rectAngle.left + x, rectAngle.top, x, y);
	MOVE(g_hwndEnt[ENT_DIR45], rectAngle.left + 2 * x, rectAngle.top, x, y);

	MOVE(g_hwndEnt[ENT_DIR180], rectAngle.left, rectAngle.top + y, x, y);
	MOVE(g_hwndEnt[ENT_DIR0], rectAngle.left + 2 * x, rectAngle.top + y, x, y);

	MOVE(g_hwndEnt[ENT_DIR225], rectAngle.left, rectAngle.top + 2*y, x, y);
	MOVE(g_hwndEnt[ENT_DIR270], rectAngle.left + x, rectAngle.top + 2 * y, x, y);
	MOVE(g_hwndEnt[ENT_DIR315], rectAngle.left + 2 * x, rectAngle.top + 2 * y, x, y);

	MOVE(g_hwndEnt[ENT_DIRUP], rectAngle.left + 3.5*x, rectAngle.top + 0.5 * y, x, y);
	MOVE(g_hwndEnt[ENT_DIRDOWN], rectAngle.left + 3.5*x, rectAngle.top + 1.5 * y, x, y);

	MOVE(g_hwndEnt[ENT_ADDPROP], rectAngle.left + 5*x, rectAngle.top, x * 1.5, y);	// sikk - Entity Window Addition
	MOVE(g_hwndEnt[ENT_DELPROP], rectAngle.left + 5*x, rectAngle.top + y, x * 1.5, y);
	MOVE(g_hwndEnt[ENT_COLOR], rectAngle.left + 5*x, rectAngle.top + y * 2, x * 1.5, y);	// sikk - Entity Window Addition

	MOVE(g_hwndEnt[ENT_CREATE], rectAngle.left + 7 * x, rectAngle.top + y * 0.5, x * 2, y * 2);	// sikk - Entity Window Addition

	SendMessage(g_qeglobals.d_hwndEntity, WM_SETREDRAW, 1, 0);
//	InvalidateRect(entwindow, NULL, TRUE);
}

/*
=========================
EntityWndProc
=========================
*/
BOOL CALLBACK EntityWndProc (
    HWND	hwndDlg,// handle to dialog box
    UINT	uMsg,	// message
    WPARAM	wParam,	// first message parameter
    LPARAM	lParam	// second message parameter
	)
{
	RECT	rc;

	GetClientRect(hwndDlg, &rc);

    switch (uMsg)
    {
	case WM_GETMINMAXINFO:
		{
			LPMINMAXINFO lpmmi = (LPMINMAXINFO)lParam;;
			lpmmi->ptMinTrackSize.x = 320;
			lpmmi->ptMinTrackSize.y = 480; // 500
		}
		return 0;

	case WM_WINDOWPOSCHANGING:
		{
			LPWINDOWPOS lpwp = (LPWINDOWPOS)lParam;
			DefWindowProc(hwndDlg, uMsg, wParam, lParam);
			lpwp->flags |= SWP_NOCOPYBITS;
			EntWnd_Resize(lpwp->cx - 8, lpwp->cy - 32);
		}
		return 0;

// sikk---> Window Management
	case WM_SIZING:
		if (TryDocking(hwndDlg, wParam, (LPRECT)lParam, 300))
			return TRUE;
		break;

	case WM_MOVING:
		if (TryDocking(hwndDlg, wParam, (LPRECT)lParam, 0))
			return TRUE;
		break;
// <---sikk

	case WM_COMMAND: 
		switch (LOWORD(wParam)) 
		{
		case IDC_E_CREATE:
			EntWnd_ToTop();
			EntWnd_CreateEntity();
			break;
		case IDC_E_COLOR:
			EntWnd_ToTop();
			if ((g_qeglobals.d_nInspectorMode == W_ENTITY) && DoColor(COLOR_ENTITY) == true)
			{
				extern void EntWnd_AddKeyValue(void);
				
				char buffer[64];
				
				sprintf(buffer, "%f %f %f", g_qeglobals.d_savedinfo.v3Colors[COLOR_ENTITY][0],
											g_qeglobals.d_savedinfo.v3Colors[COLOR_ENTITY][1],
											g_qeglobals.d_savedinfo.v3Colors[COLOR_ENTITY][2]);
					
				SetWindowText(g_hwndEnt[ENT_KEYFIELD], "_color");
				SetWindowText(g_hwndEnt[ENT_VALUEFIELD], buffer);
				EntWnd_AddKeyValue();
			}
			Sys_UpdateWindows(W_ALL);
			break;

		case IDC_E_ADDPROP:
			EntWnd_ToTop();
			EntWnd_AddKeyValue();
			break;
		case IDC_E_DELPROP:
			EntWnd_ToTop();
			EntWnd_RemoveKeyValue();
			break;
		case IDC_E_0:
			EntWnd_ToTop();
			EntWnd_ApplyAngle(0);
			break;
		case IDC_E_45:
			EntWnd_ToTop();
			EntWnd_ApplyAngle(45);
			break;
		case IDC_E_90:
			EntWnd_ToTop();
			EntWnd_ApplyAngle(90);
			break;
		case IDC_E_135:
			EntWnd_ToTop();
			EntWnd_ApplyAngle(135);
			break;
		case IDC_E_180:
			EntWnd_ToTop();
			EntWnd_ApplyAngle(180);
			break;
		case IDC_E_225:
			EntWnd_ToTop();
			EntWnd_ApplyAngle(225);
			break;
		case IDC_E_270:
			EntWnd_ToTop();
			EntWnd_ApplyAngle(270);
			break;
		case IDC_E_315:
			EntWnd_ToTop();
			EntWnd_ApplyAngle(315);
			break;
		case IDC_E_UP:
			EntWnd_ToTop();
			EntWnd_ApplyAngle(-1);
			break;
		case IDC_E_DOWN:
			EntWnd_ToTop();
			EntWnd_ApplyAngle(-2);
			break;

		case IDC_E_FLAG1:
			EntWnd_ToTop();
			EntWnd_FlagChecked(1);
			break;
		case IDC_E_FLAG2:
			EntWnd_ToTop();
			EntWnd_FlagChecked(2);
			break;
		case IDC_E_FLAG3:
			EntWnd_ToTop();
			EntWnd_FlagChecked(3);
			break;
		case IDC_E_FLAG4:
			EntWnd_ToTop();
			EntWnd_FlagChecked(4);
			break;
		case IDC_E_FLAG5:
			EntWnd_ToTop();
			EntWnd_FlagChecked(5);
			break;
		case IDC_E_FLAG6:
			EntWnd_ToTop();
			EntWnd_FlagChecked(6);
			break;
		case IDC_E_FLAG7:
			EntWnd_ToTop();
			EntWnd_FlagChecked(7);
			break;
		case IDC_E_FLAG8:
			EntWnd_ToTop();
			EntWnd_FlagChecked(8);
			break;
		case IDC_E_FLAG9:
			EntWnd_ToTop();
			EntWnd_FlagChecked(9);
			break;
		case IDC_E_FLAG10:
			EntWnd_ToTop();
			EntWnd_FlagChecked(10);
			break;
		case IDC_E_FLAG11:
			EntWnd_ToTop();
			EntWnd_FlagChecked(11);
			break;
		case IDC_E_FLAG12:
			EntWnd_ToTop();
			EntWnd_FlagChecked(12);
			break;

		case IDC_E_PROPS: 
			switch (HIWORD(wParam))
			{ 
			case LBN_SELCHANGE:
				if (GetTopWindow(g_qeglobals.d_hwndMain) != hwndDlg)
					BringWindowToTop(hwndDlg);

				EntWnd_EditKeyValue();
				return TRUE; 
			}
			break;

		case IDC_E_LIST: 
	   		switch (HIWORD(wParam)) 
			{ 
			case LBN_SELCHANGE: 
				{
					int			iIndex;
					eclass_t   *pec;

					if (GetTopWindow(g_qeglobals.d_hwndMain) != hwndDlg)
						BringWindowToTop(hwndDlg);

					iIndex = SendMessage(g_hwndEnt[ENT_CLASSLIST], LB_GETCURSEL, 0, 0);	
					pec = (eclass_t *)SendMessage(g_hwndEnt[ENT_CLASSLIST], LB_GETITEMDATA, iIndex, 0); 
				
					EntWnd_UpdateSel(iIndex, pec);

					return TRUE; 
					break; 
				}

			case LBN_DBLCLK: 
				EntWnd_CreateEntity();
				SetFocus(g_qeglobals.d_hwndCamera);
				break; 
			} 
            break; 

		default: 
			return DefWindowProc(hwndDlg, uMsg, wParam, lParam);
        } 
		return 0;

// sikk---> LMB Bring to Top
	case WM_LBUTTONDOWN:
		if (GetTopWindow(g_qeglobals.d_hwndMain) != hwndDlg)
			BringWindowToTop(hwndDlg);
		break;
// <---sikk
	}
    return DefWindowProc(hwndDlg, uMsg, wParam, lParam);
}
