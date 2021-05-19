//==============================
//	win_ent.c
//==============================

#include "qe3.h"


BOOL CALLBACK FieldWndProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK EntityListWndProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK EntityWndProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

LRESULT(CALLBACK* OldFieldWindowProc) (HWND, UINT, WPARAM, LPARAM);
LRESULT(CALLBACK* OldEntityListWindowProc) (HWND, UINT, WPARAM, LPARAM);


/*
===============================================================

ENTITY WINDOW

===============================================================
*/

#define DLGBORDER_X		4
#define DLGBORDER_Y		4
#define KVMIXED			"__QE3MIXEDVALUES_"
#define KVMIXEDLABEL	"[multiple values]"

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

// lunaran - entity window now interacts through a dummy entity that acts as the union of 
// all selected entities (for displaying mixed selections)
//bool		g_bMultipleEntities;
//entity_t   *g_peEditEntity;
entity_t	g_eEditEntity;
char		g_nEditEntFlags[12];	// spawnflags in the entity inspector can be off/on/ambiguous
char		g_szEditFlagNames[8][32];

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
				SetFocus(g_hwndEnt[ENT_VALUEFIELD]);
				SendMessage(g_hwndEnt[ENT_VALUEFIELD], EM_SETSEL, 0, -1);
			}
			else
			{
				SetFocus(g_hwndEnt[ENT_KEYFIELD]);
				SendMessage(g_hwndEnt[ENT_KEYFIELD], EM_SETSEL, 0, -1);
			}
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
		// sikk---> LMB Bring to Top
		if (GetTopWindow(g_qeglobals.d_hwndMain) != g_qeglobals.d_hwndInspector)
			BringWindowToTop(g_qeglobals.d_hwndInspector);
		// <---sikk
		SetFocus(hWnd);
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
			SendMessage(g_qeglobals.d_hwndInspector, WM_COMMAND, (LBN_DBLCLK << 16) + IDC_E_LIST, 0);
			return 0;
		}
		break;

// sikk---> LMB Bring to Top
	case WM_LBUTTONDOWN:
		if (GetTopWindow(g_qeglobals.d_hwndMain) != g_qeglobals.d_hwndInspector)
			BringWindowToTop(g_qeglobals.d_hwndInspector);
		break;
// <---sikk
	}
	return CallWindowProc(OldEntityListWindowProc, hWnd, uMsg, wParam, lParam);
}

/*
================
EntWnd_CreateControls
================
*/
void EntWnd_CreateControls(HINSTANCE hInstance)
{
	int i;
	HWND h;

	// kind of silly: create a dialog that contains all the controls, move the buttons and labels
	// to the desired window, then destroy the dialog

	h = CreateDialog(hInstance, (char *)IDD_ENTITY, g_qeglobals.d_hwndMain, (DLGPROC)NULL);
	if (!h)
		Error("CreateDialog: Failed.");

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
	DestroyWindow(h);

	LoadLibraryA("RICHED32.DLL");	// sikk - For Enhanced Editing	

	// SetParent apears to not modify some internal state
	// on listboxes, so create it from scratch...

// sikk---> Misc aesthetic changes
	g_hwndEnt[ENT_CLASSLIST] = CreateWindowEx(WS_EX_CLIENTEDGE,
		"listbox",
		NULL,
		LBS_NOTIFY | LBS_SORT | LBS_NOINTEGRALHEIGHT | LBS_WANTKEYBOARDINPUT | WS_CHILD | WS_VISIBLE | WS_VSCROLL,
		5, 5, 180, 99,
		g_qeglobals.d_hwndEntity,
		(HMENU)IDC_E_LIST,
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
		(HMENU)IDC_E_PROPS,
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
		(HMENU)IDC_E_COMMENT,
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
		(HMENU)IDC_E_KEY_FIELD,
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
		(HMENU)IDC_E_VALUE_FIELD,
		g_qeglobals.d_hInstance,
		NULL);
	if (!g_hwndEnt[ENT_VALUEFIELD])
		Error("CreateWindowEx: Failed.");

	// <---sikk

#if 0
	for (i = 0; i < 12; i++)
	{
		g_hwndEnt[ENT_CHECK1 + i] = CreateWindow("button",
			NULL,
			BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE,
			5, 100, 180, 99,
			entwindow,
			(HMENU)IDC_E_STATUS,
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
}


/*
==============
EntWnd_Create
==============
*/
void EntWnd_Create (HINSTANCE hInstance)
{
	WNDCLASS	wc;

	/* Register the texture class */
	memset(&wc, 0, sizeof(wc));

	wc.style = 0;
	wc.lpfnWndProc = (WNDPROC)EntityWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = g_qeglobals.d_hInstance;
	wc.hIcon = 0;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);	// sikk - was-> GetStockObject (LTGRAY_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = ENTITY_WINDOW_CLASS;

	if (!RegisterClass(&wc))
		Error("EntWnd_Register: Failed.");

	g_qeglobals.d_hwndEntity = CreateWindowEx(0,//WS_EX_CLIENTEDGE,	// extended window style
		ENTITY_WINDOW_CLASS,				// registered class name
		"Entity View",						// window name
		WS_BORDER | WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,	// window style
		20, 20, 64, 64,						// size and position of window
		g_qeglobals.d_hwndInspector,		// parent or owner window
		0,									// menu or child-window identifier
		hInstance,							// application instance
		NULL);								// window-creation data

	if (!g_qeglobals.d_hwndEntity)
		Error("Could not create Entity Window.");

	EntWnd_CreateControls(hInstance);

	OldFieldWindowProc = (WNDPROC)GetWindowLong(g_hwndEnt[ENT_KEYFIELD], GWL_WNDPROC);
	SetWindowLong(g_hwndEnt[ENT_KEYFIELD], GWL_WNDPROC, (long)FieldWndProc);
	SetWindowLong(g_hwndEnt[ENT_VALUEFIELD], GWL_WNDPROC, (long)FieldWndProc);

	OldEntityListWindowProc = (WNDPROC)GetWindowLong(g_hwndEnt[ENT_CLASSLIST], GWL_WNDPROC);
	SetWindowLong(g_hwndEnt[ENT_CLASSLIST], GWL_WNDPROC, (long)EntityListWndProc);

	EntWnd_FillClassList();

}

/*
==============
EntWnd_FillClassList
==============
*/
void EntWnd_FillClassList()
{
	eclass_t   *pec;
	int			iIndex;

	SendMessage(g_hwndEnt[ENT_CLASSLIST], LB_RESETCONTENT, 0, 0);

	for (pec = g_pecEclass; pec; pec = pec->next)
	{
		iIndex = SendMessage(g_hwndEnt[ENT_CLASSLIST], LB_ADDSTRING, 0, (LPARAM)pec->name);
		SendMessage(g_hwndEnt[ENT_CLASSLIST], LB_SETITEMDATA, iIndex, (LPARAM)pec);
	}
}






/*
==============
EntWnd_UpdateListSel
==============
*/
void EntWnd_UpdateListSel()
{
	int			iIndex;
	eclass_t   *pec;
	iIndex = SendMessage(g_hwndEnt[ENT_CLASSLIST], LB_GETCURSEL, 0, 0);
	pec = (eclass_t *)SendMessage(g_hwndEnt[ENT_CLASSLIST], LB_GETITEMDATA, iIndex, 0);

	if (iIndex != LB_ERR)
		SendMessage(g_hwndEnt[ENT_CLASSLIST], LB_SETCURSEL, iIndex, 0);

	if (pec)
		SendMessage(g_hwndEnt[ENT_COMMENT], WM_SETTEXT, 0, (LPARAM)TranslateString(pec->comments));
}

/*
==============
EntWnd_RefreshEditEntityFlags
==============
*/
void EntWnd_RefreshEditEntityFlags(int inFlags, bool first)
{
	int f;

	for (int i = 0; i < 12; i++)
	{
		if (g_nEditEntFlags[i] == BST_INDETERMINATE)
			continue;

		f = !!(inFlags & (1 << i));
		if (first)
		{
			g_nEditEntFlags[i] = f;
		}
		else if (f != g_nEditEntFlags[i])
		{
			g_nEditEntFlags[i] = BST_INDETERMINATE;
		}
	}
}

/*
==============
EntWnd_RefreshEditEntity
==============
*/
void EntWnd_RefreshEditEntity()
{
	entity_t *eo;
	epair_t *ep;
	bool first;

	Entity_FreeEpairs(&g_eEditEntity);
	g_eEditEntity.eclass = NULL;
	memset(g_nEditEntFlags, 0, 12 * sizeof(char));

	eo = NULL;
	first = true;
	for (brush_t* b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
	{
		// since brushes in the selection are (mostly) sorted by entity, this is a 
		// decent enough way to shuffle through them, but it does no harm if a 
		// fragmented brush entity contributes its own values more than once
		if (b->owner == eo)
			continue;

		eo = b->owner;
		if (first)
		{
			for (int i = 0; i < 8; i++)
			{
				strncpy(g_szEditFlagNames[i], eo->eclass->flagnames[i], 32);
			}
			for (ep = eo->epairs; ep; ep = ep->next)
			{
				if (!_strcmpi(ep->key, "spawnflags"))
				{
					EntWnd_RefreshEditEntityFlags(IntForKey(eo, "spawnflags"), true);
				}
				SetKeyValue(&g_eEditEntity, ep->key, ep->value);
			}
			first = false;
		}
		else for (ep = eo->epairs; ep; ep = ep->next)
		{
			for (int i = 0; i < 8; i++)
			{
				if (_stricmp(g_szEditFlagNames[i], eo->eclass->flagnames[i]))
					strncpy(g_szEditFlagNames[i], "~\0", 2);
			}
			if (!_strcmpi(ep->key, "spawnflags"))
			{
				EntWnd_RefreshEditEntityFlags(IntForKey(eo, "spawnflags"), false);
			}
			if (!*ValueForKey(&g_eEditEntity, ep->key))
				SetKeyValue(&g_eEditEntity, ep->key, ep->value);
			else if (_strcmpi(ValueForKey(&g_eEditEntity, ep->key), ep->value) == 0)
				continue;
			else
				SetKeyValue(&g_eEditEntity, ep->key, KVMIXED);
		}
	}
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
	int			nTabs[] = {64};	// sikk - Tab fix

	// set key/value pair list
	GetWindowRect(g_hwndEnt[ENT_PROPS], &rc);
	SendMessage(g_hwndEnt[ENT_PROPS], LB_SETCOLUMNWIDTH, (rc.right - rc.left) / 2, 0);
	SendMessage(g_hwndEnt[ENT_PROPS], LB_RESETCONTENT, 0, 0);
	SendMessage(g_hwndEnt[ENT_PROPS], LB_SETTABSTOPS, (WPARAM)1, (LPARAM)(LPINT)nTabs);	// sikk - Tab fix

	// Walk through list and add pairs
	for (pep = g_eEditEntity.epairs; pep; pep = pep->next)
	{
		if (!_strcmpi(pep->value, KVMIXED))
			sprintf(sz, "%s\t%s", pep->key, KVMIXEDLABEL);
		else
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

	for (brush_t *b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
		SetKeyValue(b->owner, "angle", sz);
	SetKeyValue(&g_eEditEntity, "angle", sz);

	EntWnd_RefreshKeyValues();
	Sys_UpdateWindows(W_CAMERA);
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

	bool on = !(SendMessage(g_hwndEnt[ENT_CHECK1 + flag - 1], BM_GETCHECK, 0, 0) == BST_CHECKED);
	SendMessage(g_hwndEnt[ENT_CHECK1 + flag - 1], BM_SETCHECK, on, 0);

	int f = 1 << (flag-1);

	entity_t *eo;
	eo = NULL;
	for (brush_t *b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
	{
		if (b->owner == eo)
			continue;
		else
			eo = b->owner;

		SetSpawnFlag(b->owner, f, on);
	}

	EntWnd_UpdateUI();
}

/*
==============
EntWnd_FlagsFromEnt

Update the checkboxes to reflect the flag state of the entity
==============
*/
void EntWnd_FlagsFromEnt ()
{
	for (int i = 0; i < 12; i++)
	{
		SendMessage(g_hwndEnt[ENT_CHECK1 + i], BM_SETCHECK, g_nEditEntFlags[i], 0);
	}
}

/*
==============
EntWnd_UpdateUI

Update the listbox, checkboxes and k/v pairs to reflect the new selection
==============
*/
void EntWnd_UpdateUI ()
{
	int			i;

	SendMessage(g_qeglobals.d_hwndEntity, WM_SETREDRAW, 0, 0);
	// clear the window
	EntWnd_RefreshEditEntity();

	if (!Select_HasBrushes())
	{
		SendMessage(g_hwndEnt[ENT_COMMENT], WM_SETTEXT, 0, (LPARAM)"");
		SendMessage(g_hwndEnt[ENT_CLASSLIST], LB_SETCURSEL, -1, 0);
		for (i = 0; i < 8; i++)
		{
			HWND hwnd = g_hwndEnt[ENT_CHECK1 + i];
			// disable check box
			SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)" ");
			EnableWindow(hwnd, FALSE);
		}
	}
	else
	{
		eclass_t *pec = g_brSelectedBrushes.next->owner->eclass;
		int nIndex = (int)SendMessage(g_hwndEnt[ENT_CLASSLIST], LB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)pec->name);
		if (nIndex != LB_ERR)
			SendMessage(g_hwndEnt[ENT_CLASSLIST], LB_SETCURSEL, nIndex, 0);

		// Set up the description
		SendMessage(g_hwndEnt[ENT_COMMENT], WM_SETTEXT, 0, (LPARAM)TranslateString(pec->comments));

		for (i = 0; i < 8; i++)
		{
			HWND hwnd = g_hwndEnt[ENT_CHECK1 + i];

			if (*g_szEditFlagNames[i])
			{
				EnableWindow(hwnd, TRUE);
				SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)g_szEditFlagNames[i]);
			}
			else
			{
				// disable check box
				SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)" ");
				EnableWindow(hwnd, FALSE);
			}
		}
	}
	EntWnd_FlagsFromEnt();
	EntWnd_RefreshKeyValues();

	SendMessage(g_qeglobals.d_hwndEntity, WM_SETREDRAW, 1, 0);
	RedrawWindow(g_qeglobals.d_hwndInspector, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_FRAME | RDW_ERASENOW | RDW_UPDATENOW | RDW_ALLCHILDREN);
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
	if (!Select_HasBrushes())
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

	if (!_stricmp(sz, "worldspawn"))
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

	Select_DeselectAll(true);
	Select_HandleBrush(petNew->brushes.onext, true);

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

	// Get current selection text
	SendMessage(g_hwndEnt[ENT_KEYFIELD], WM_GETTEXT, sizeof(key) - 1, (LPARAM)key);	
	SendMessage(g_hwndEnt[ENT_VALUEFIELD], WM_GETTEXT, sizeof(value) - 1, (LPARAM)value);	

	for (brush_t *b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
		SetKeyValue(b->owner, key, value);
	SetKeyValue(&g_eEditEntity, key, value);

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

	// Get current selection text
	SendMessage(g_hwndEnt[ENT_KEYFIELD], WM_GETTEXT, sizeof(sz) - 1, (LPARAM)sz);	

	for (brush_t *b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
		DeleteKey(b->owner, sz);
	DeleteKey(&g_eEditEntity, sz);

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
	if (_strcmpi(val, KVMIXEDLABEL))
		SendMessage(g_hwndEnt[ENT_VALUEFIELD], WM_SETTEXT, 0, (LPARAM)val);
	else
		SendMessage(g_hwndEnt[ENT_VALUEFIELD], WM_SETTEXT, 0, (LPARAM)"");
}

//HDWP	defer;	// sikk - unused

/*
===============
EntWnd_Resize
===============
*/
void EntWnd_Resize(RECT rc)
{
	int nWidth, nHeight;
	nWidth = rc.right;
	nHeight = rc.bottom;

	int		i, x, y, w;
	int		xCheck, yCheck, fold;
	RECT	rectClasses, rectComment, rectProps, rectFlags, rectKV, rectAngle;
	int		flagRows, flagCols, flagOffset;

	SendMessage(g_qeglobals.d_hwndEntity, WM_SETREDRAW, 0, 0);
	InspWnd_MoveRect(g_qeglobals.d_hwndEntity, rc);

	yCheck = 20;	// distance from top of one check to the next

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
	rectAngle.bottom = nHeight - rectAngle.top - DLGBORDER_Y;

	InspWnd_MoveRect(g_hwndEnt[ENT_CLASSLIST], rectClasses);
	InspWnd_MoveRect(g_hwndEnt[ENT_COMMENT], rectComment);

	x = rectFlags.left;
	xCheck = nWidth / flagCols;	// xCheck = width of a single check box
	for (flagOffset = 0; flagOffset < 12; flagOffset += flagRows)
	{
		y = rectFlags.top;
		for (i = 0; i < flagRows; i++)
		{
			InspWnd_Move(g_hwndEnt[ENT_CHECK1 + i + flagOffset], x, y, xCheck, yCheck);
			y += yCheck;
		}
		x += xCheck;
	}

	InspWnd_MoveRect(g_hwndEnt[ENT_PROPS], rectProps);

	w = rectKV.right - (DLGBORDER_X + 45);
	InspWnd_Move(g_hwndEnt[ENT_KEYLABEL], DLGBORDER_X, rectKV.top, 40, yCheck);
	InspWnd_Move(g_hwndEnt[ENT_KEYFIELD], DLGBORDER_X + 40, rectKV.top, rectKV.right, yCheck);
	InspWnd_Move(g_hwndEnt[ENT_VALUELABEL], DLGBORDER_X, rectKV.top + yCheck, 40, yCheck);
	InspWnd_Move(g_hwndEnt[ENT_VALUEFIELD], DLGBORDER_X + 40, rectKV.top + yCheck, rectKV.right, yCheck);

	x = rectAngle.right / 9;
	y = rectAngle.bottom / 3;
	InspWnd_Move(g_hwndEnt[ENT_DIR135], rectAngle.left, rectAngle.top, x, y);
	InspWnd_Move(g_hwndEnt[ENT_DIR90], rectAngle.left + x, rectAngle.top, x, y);
	InspWnd_Move(g_hwndEnt[ENT_DIR45], rectAngle.left + 2 * x, rectAngle.top, x, y);

	InspWnd_Move(g_hwndEnt[ENT_DIR180], rectAngle.left, rectAngle.top + y, x, y);
	InspWnd_Move(g_hwndEnt[ENT_DIR0], rectAngle.left + 2 * x, rectAngle.top + y, x, y);

	InspWnd_Move(g_hwndEnt[ENT_DIR225], rectAngle.left, rectAngle.top + 2 * y, x, y);
	InspWnd_Move(g_hwndEnt[ENT_DIR270], rectAngle.left + x, rectAngle.top + 2 * y, x, y);
	InspWnd_Move(g_hwndEnt[ENT_DIR315], rectAngle.left + 2 * x, rectAngle.top + 2 * y, x, y);

	InspWnd_Move(g_hwndEnt[ENT_DIRUP], rectAngle.left + 3.5*x, rectAngle.top + 0.5 * y, x, y);
	InspWnd_Move(g_hwndEnt[ENT_DIRDOWN], rectAngle.left + 3.5*x, rectAngle.top + 1.5 * y, x, y);

	InspWnd_Move(g_hwndEnt[ENT_ADDPROP], rectAngle.left + 5 * x, rectAngle.top, x * 1.5, y);	// sikk - Entity Window Addition
	InspWnd_Move(g_hwndEnt[ENT_DELPROP], rectAngle.left + 5 * x, rectAngle.top + y, x * 1.5, y);
	InspWnd_Move(g_hwndEnt[ENT_COLOR], rectAngle.left + 5 * x, rectAngle.top + y * 2, x * 1.5, y);	// sikk - Entity Window Addition

	InspWnd_Move(g_hwndEnt[ENT_CREATE], rectAngle.left + 7 * x, rectAngle.top + y * 0.5, x * 2, y * 2);	// sikk - Entity Window Addition

	SendMessage(g_qeglobals.d_hwndEntity, WM_SETREDRAW, 1, 0);
	RedrawWindow(g_qeglobals.d_hwndInspector, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_FRAME | RDW_ERASENOW | RDW_UPDATENOW | RDW_ALLCHILDREN);
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
    switch (uMsg)
    {
	case WM_COMMAND: 
		switch (LOWORD(wParam)) 
		{
		case IDC_E_CREATE:
			InspWnd_ToTop();
			EntWnd_CreateEntity();
			break;
		case IDC_E_COLOR:
			InspWnd_ToTop();
			if ((g_qeglobals.d_nInspectorMode == W_ENTITY) && DoColor(COLOR_ENTITY) == TRUE)
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
			InspWnd_ToTop();
			EntWnd_AddKeyValue();
			break;
		case IDC_E_DELPROP:
			InspWnd_ToTop();
			EntWnd_RemoveKeyValue();
			break;
		case IDC_E_0:
			InspWnd_ToTop();
			EntWnd_ApplyAngle(0);
			break;
		case IDC_E_45:
			InspWnd_ToTop();
			EntWnd_ApplyAngle(45);
			break;
		case IDC_E_90:
			InspWnd_ToTop();
			EntWnd_ApplyAngle(90);
			break;
		case IDC_E_135:
			InspWnd_ToTop();
			EntWnd_ApplyAngle(135);
			break;
		case IDC_E_180:
			InspWnd_ToTop();
			EntWnd_ApplyAngle(180);
			break;
		case IDC_E_225:
			InspWnd_ToTop();
			EntWnd_ApplyAngle(225);
			break;
		case IDC_E_270:
			InspWnd_ToTop();
			EntWnd_ApplyAngle(270);
			break;
		case IDC_E_315:
			InspWnd_ToTop();
			EntWnd_ApplyAngle(315);
			break;
		case IDC_E_UP:
			InspWnd_ToTop();
			EntWnd_ApplyAngle(-1);
			break;
		case IDC_E_DOWN:
			InspWnd_ToTop();
			EntWnd_ApplyAngle(-2);
			break;

		case IDC_E_FLAG1:
			InspWnd_ToTop();
			EntWnd_FlagChecked(1);
			break;
		case IDC_E_FLAG2:
			InspWnd_ToTop();
			EntWnd_FlagChecked(2);
			break;
		case IDC_E_FLAG3:
			InspWnd_ToTop();
			EntWnd_FlagChecked(3);
			break;
		case IDC_E_FLAG4:
			InspWnd_ToTop();
			EntWnd_FlagChecked(4);
			break;
		case IDC_E_FLAG5:
			InspWnd_ToTop();
			EntWnd_FlagChecked(5);
			break;
		case IDC_E_FLAG6:
			InspWnd_ToTop();
			EntWnd_FlagChecked(6);
			break;
		case IDC_E_FLAG7:
			InspWnd_ToTop();
			EntWnd_FlagChecked(7);
			break;
		case IDC_E_FLAG8:
			InspWnd_ToTop();
			EntWnd_FlagChecked(8);
			break;
		case IDC_E_FLAG9:
			InspWnd_ToTop();
			EntWnd_FlagChecked(9);
			break;
		case IDC_E_FLAG10:
			InspWnd_ToTop();
			EntWnd_FlagChecked(10);
			break;
		case IDC_E_FLAG11:
			InspWnd_ToTop();
			EntWnd_FlagChecked(11);
			break;
		case IDC_E_FLAG12:
			InspWnd_ToTop();
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

					if (GetTopWindow(g_qeglobals.d_hwndMain) != hwndDlg)
						BringWindowToTop(hwndDlg);

				
					EntWnd_UpdateListSel();

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
	case WM_RBUTTONDOWN:
	case WM_MBUTTONDOWN:
		InspWnd_ToTop();
		break;
// <---sikk
	}
    return DefWindowProc(hwndDlg, uMsg, wParam, lParam);
}
