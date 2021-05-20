//==============================
//	WndEntity.cpp
//==============================

#include "qe3.h"
#include "CmdSetSpawnflag.h"

#define DLGBORDER_X		4
#define DLGBORDER_Y		4
#define KVMIXED			"__QE3MIXEDVALUES_"
#define KVMIXEDLABEL	"[multiple values]"

int g_nEntDialogIDs[ENT_LAST] =
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



WndEntity::WndEntity()
{
	name = "EntityWindow";
	vbits = W_ENTITY;
	minWidth = 320;
	minHeight = 480;
}

WndEntity::~WndEntity()
{
}

void WndEntity::Initialize()
{
	CreateWnd();
	SetTitle("Entity Editor");
	g_qeglobals.d_hwndEntity = w_hwnd;
	CreateControls();
	RestorePosition();	// something in ResizeControls forces the window to visible
}



/*
========================================================================

	PROCS

========================================================================
*/

void WndEntity::SelectEntityColor()
{
	QE_SetInspectorMode(W_ENTITY);

	vec3 color;
	if (!g_eEditEntity.GetKeyValueVector("_color", color))
		color = g_qeglobals.d_lastColor;
	if (color == vec3(0))
		color = vec3(1);
	if (DoColorSelect(color, color))
	{
		char buffer[64];
		VecToString(color, buffer);

		SetWindowText(w_hwndCtrls[ENT_KEYFIELD], "_color");
		SetWindowText(w_hwndCtrls[ENT_VALUEFIELD], buffer);
		SetKeyValue();
	}
	Sys_UpdateWindows(W_SCENE | W_ENTITY);
}

/*
=========================
WndEntity::FieldProc

handle tab and enter
=========================
*/
BOOL CALLBACK WndFieldProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	return g_qeglobals.d_wndEntity->FieldProc(hWnd, uMsg, wParam, lParam);
}

BOOL WndEntity::FieldProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
			g_qeglobals.d_wndCamera->Focus();
			return FALSE;
		}
		break;

	case WM_KEYDOWN:
		if (LOWORD(wParam) == VK_TAB)
		{
			if (hWnd == w_hwndCtrls[ENT_KEYFIELD])
			{
				SetFocus(w_hwndCtrls[ENT_VALUEFIELD]);
				SendMessage(w_hwndCtrls[ENT_VALUEFIELD], EM_SETSEL, 0, -1);
			}
			else
			{
				SetFocus(w_hwndCtrls[ENT_KEYFIELD]);
				SendMessage(w_hwndCtrls[ENT_KEYFIELD], EM_SETSEL, 0, -1);
			}
		}
		if (LOWORD(wParam) == VK_RETURN)
		{
			if (hWnd == w_hwndCtrls[ENT_KEYFIELD])
			{
				SendMessage(w_hwndCtrls[ENT_VALUEFIELD], WM_SETTEXT, 0, (long)"");
				SetFocus(w_hwndCtrls[ENT_VALUEFIELD]);
			}
			else
			{
				SetKeyValue();
				SetFocus(w_hwndCtrls[ENT_PROPS]);	// sikk - Made sense to keep focus 
			}
		}
		break;

	case WM_LBUTTONDOWN:
		g_qeglobals.d_wndEntity->BringToTop();
		SetFocus(w_hwnd);
		break;
	}
	return CallWindowProc(DefaultFieldProc, hWnd, uMsg, wParam, lParam);
}

/*
=========================
WndEntity::EntityListProc

handle enter
=========================
*/
BOOL CALLBACK WndEntityListProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return g_qeglobals.d_wndEntity->EntityListProc(hWnd, uMsg, wParam, lParam);
}

BOOL WndEntity::EntityListProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_KEYDOWN:
		if (LOWORD(wParam) == VK_RETURN)
		{
			SendMessage(w_hwndEntDialog, WM_COMMAND, (LBN_DBLCLK << 16) + IDC_E_LIST, 0);
			return 0;
		}
		break;

	case WM_LBUTTONDOWN:
		Focus();
		break;
	}
	return CallWindowProc(DefaultEntityListProc, hWnd, uMsg, wParam, lParam);
}

/*
=========================
EntityWndProc
=========================
*/

BOOL CALLBACK WndEntityProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	//assert(hwndDlg == g_qeglobals.d_wndEntity->w_hwndEntDialog);
	if (!g_qeglobals.d_wndEntity->w_hwndEntDialog)
		g_qeglobals.d_wndEntity->w_hwndEntDialog = hWnd;
	return g_qeglobals.d_wndEntity->EntityDlgProc(uMsg, wParam, lParam);
}

// FIXME: htf do you keep the entity window bar focus-blue when the dialog controls are focused?
BOOL CALLBACK WndEntity::EntityDlgProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_E_CREATE:
			Focus();
			CreateEntity();
			break;
		case IDC_E_COLOR:
			SelectEntityColor();
			break;
		case IDC_E_ADDPROP:
			Focus();
			SetKeyValue();
			break;
		case IDC_E_DELPROP:
			Focus();
			RemoveKeyValue();
			break;
		case IDC_E_0:
			Focus();
			ApplyAngle(0);
			break;
		case IDC_E_45:
			Focus();
			ApplyAngle(45);
			break;
		case IDC_E_90:
			Focus();
			ApplyAngle(90);
			break;
		case IDC_E_135:
			Focus();
			ApplyAngle(135);
			break;
		case IDC_E_180:
			Focus();
			ApplyAngle(180);
			break;
		case IDC_E_225:
			Focus();
			ApplyAngle(225);
			break;
		case IDC_E_270:
			Focus();
			ApplyAngle(270);
			break;
		case IDC_E_315:
			Focus();
			ApplyAngle(315);
			break;
		case IDC_E_UP:
			Focus();
			ApplyAngle(-1);
			break;
		case IDC_E_DOWN:
			Focus();
			ApplyAngle(-2);
			break;

		case IDC_E_FLAG1:
		case IDC_E_FLAG2:
		case IDC_E_FLAG3:
		case IDC_E_FLAG4:
		case IDC_E_FLAG5:
		case IDC_E_FLAG6:
		case IDC_E_FLAG7:
		case IDC_E_FLAG8:
		case IDC_E_FLAG9:
		case IDC_E_FLAG10:
		case IDC_E_FLAG11:
		case IDC_E_FLAG12:
			Focus();
			FlagChecked(LOWORD(wParam) - IDC_E_FLAG1 + 1);
			break;

		case IDC_E_PROPS:
			switch (HIWORD(wParam))
			{
			case LBN_SELCHANGE:
				EditKeyValue();
				return TRUE;
			}
			break;

		case IDC_E_LIST:
			switch (HIWORD(wParam))
			{
			case LBN_SELCHANGE:
			{
				UpdateListSel();

				return TRUE;
				break;
			}

			case LBN_DBLCLK:
				CreateEntity();
				g_qeglobals.d_wndCamera->Focus();
				break;
			}
			break;

		default:
			return DefWindowProc(w_hwndEntDialog, uMsg, wParam, lParam);
		}
		return 0;

		// sikk---> LMB Bring to Top
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_MBUTTONDOWN:
		break;
		// <---sikk
	}
	return DefWindowProc(w_hwndEntDialog, uMsg, wParam, lParam);
}

/*
==================
WndEntity::WindowProcedure
==================
*/
int WndEntity::WindowProcedure(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_MOUSEWHEEL:
		Focus();
		POINT	point;
		HWND	wnd;
		point.x = (short)LOWORD(lParam);
		point.y = (short)HIWORD(lParam);
		wnd = WindowFromPoint(point);	// why doesn't ChildWindowFromPoint return anything?
		SendMessage(wnd, uMsg, wParam, lParam);
		return 0;

	case WM_SIZE:
		GetClientRect(w_hwnd, &clientRect);
		ResizeControls();
		return 0;

	case WM_SIZING:
		if (TryDocking(wParam, (LPRECT)lParam))
			return 1;
		break;

	case WM_MOVING:
		if (TryDocking(0, (LPRECT)lParam))
			return 1;
		break;

	case WM_GETMINMAXINFO:
		{
			LPMINMAXINFO lpmmi = (LPMINMAXINFO)lParam;
			lpmmi->ptMinTrackSize.x = minWidth;
			lpmmi->ptMinTrackSize.y = minHeight;
		}
		return 0;

	// lunaran FIXME: this keeps the entity title bar activated when child ctrls are focused, but prevents 
	// it being darkened again when they're unfocused for another window, because no message goes through
	// this hwnd in that case
	case WM_KILLFOCUS:
	//	SendMessage(w_hwnd, WM_NCACTIVATE, (GetParent((HWND)wParam) == w_hwndEntDialog), 0);
	//	return 0;
	case WM_SETFOCUS:
		SendMessage(w_hwnd, WM_NCACTIVATE, (uMsg == WM_SETFOCUS), 0);
		return 0;

	case WM_NCLBUTTONDOWN:
	case WM_NCRBUTTONDOWN:
		Focus();
		break;
	}

	if (!OnMessage(uMsg, wParam, lParam))
		return 0;

	return DefWindowProc(w_hwnd, uMsg, wParam, lParam);
}

int WndEntity::OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_KEYDOWN)
		return QE_KeyDown(wParam);
	if (uMsg == WM_KEYUP)
		return QE_KeyUp(wParam);

	return 1;
}




/*
========================================================================

	WINDOW UPDATES

========================================================================
*/

/*
===============
WndEntity::FillClassList
===============
*/
void WndEntity::FillClassList()
{
	int			iIndex;

	SendMessage(w_hwndCtrls[ENT_CLASSLIST], LB_RESETCONTENT, 0, 0);

	for (auto ecIt = EntClass::begin(); ecIt != EntClass::end(); ecIt++)
	{
		iIndex = SendMessage(w_hwndCtrls[ENT_CLASSLIST], LB_ADDSTRING, 0, (LPARAM)(*ecIt)->name);
		SendMessage(w_hwndCtrls[ENT_CLASSLIST], LB_SETITEMDATA, iIndex, (LPARAM)(*ecIt));
	}
}

/*
===============
WndEntity::CreateControls
===============
*/
void WndEntity::CreateControls()
{
	w_hwndEntDialog = nullptr;
	w_hwndEntDialog = CreateDialog(g_qeglobals.d_hInstance, MAKEINTRESOURCE(IDD_ENTITY), w_hwnd, (DLGPROC)WndEntityProc);
	if (!w_hwndEntDialog)
		Error("CreateDialog failed building entity editor");

	GetClientRect(w_hwnd, &clientRect);
	MoveRect(w_hwndEntDialog, clientRect);
	ShowWindow(w_hwndEntDialog, SW_SHOWNORMAL);
	UpdateWindow(w_hwndEntDialog);

	for (int i = 0; i < ENT_LAST; i++)
	{
		w_hwndCtrls[i] = GetDlgItem(w_hwndEntDialog, g_nEntDialogIDs[i]);
		if (w_hwndCtrls[i])
		{
			SendMessage(w_hwndCtrls[i], WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), (LPARAM)TRUE);
		}
	}

	DefaultFieldProc = (WNDPROC)GetWindowLong(w_hwndCtrls[ENT_KEYFIELD], GWL_WNDPROC);
	SetWindowLong(w_hwndCtrls[ENT_KEYFIELD], GWL_WNDPROC, (long)WndFieldProc);
	SetWindowLong(w_hwndCtrls[ENT_VALUEFIELD], GWL_WNDPROC, (long)WndFieldProc);

	DefaultEntityListProc = (WNDPROC)GetWindowLong(w_hwndCtrls[ENT_CLASSLIST], GWL_WNDPROC);
	SetWindowLong(w_hwndCtrls[ENT_CLASSLIST], GWL_WNDPROC, (long)WndEntityListProc);

	ResizeControls();
	FillClassList();
	return;
}

/*
==============
WndEntity::UpdateListSel
==============
*/
void WndEntity::UpdateListSel()
{
	int			iIndex;
	EntClass   *pec;
	iIndex = SendMessage(w_hwndCtrls[ENT_CLASSLIST], LB_GETCURSEL, 0, 0);
	pec = (EntClass *)SendMessage(w_hwndCtrls[ENT_CLASSLIST], LB_GETITEMDATA, iIndex, 0);

	if (iIndex != LB_ERR)
		SendMessage(w_hwndCtrls[ENT_CLASSLIST], LB_SETCURSEL, iIndex, 0);

	if (pec)
		SendMessage(w_hwndCtrls[ENT_COMMENT], WM_SETTEXT, 0, (LPARAM)TranslateString((char*)*pec->comments));
}

/*
==============
WndEntity::UpdateUI

Update the listbox, checkboxes and k/v pairs to reflect the new selection
==============
*/
void WndEntity::UpdateUI()
{
	int i;

	SendMessage(g_qeglobals.d_hwndEntity, WM_SETREDRAW, 0, 0);
	RefreshEditEntity();

	if (!Selection::HasBrushes())
	{
		SendMessage(w_hwndCtrls[ENT_COMMENT], WM_SETTEXT, 0, (LPARAM)"");
		SendMessage(w_hwndCtrls[ENT_CLASSLIST], LB_SETCURSEL, -1, 0);
		for (i = 0; i < 8; i++)
		{
			HWND hwnd = w_hwndCtrls[ENT_CHECK1 + i];
			// disable check box
			SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)" ");
			EnableWindow(hwnd, FALSE);
		}
	}
	else
	{
		EntClass *pec = g_brSelectedBrushes.next->owner->eclass;
		int nIndex = (int)SendMessage(w_hwndCtrls[ENT_CLASSLIST], LB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)pec->name);
		if (nIndex != LB_ERR)
			SendMessage(w_hwndCtrls[ENT_CLASSLIST], LB_SETCURSEL, nIndex, 0);

		// Set up the description
		SendMessage(w_hwndCtrls[ENT_COMMENT], WM_SETTEXT, 0, (LPARAM)TranslateString((char*)*pec->comments));

		for (i = 0; i < 8; i++)
		{
			HWND hwnd = w_hwndCtrls[ENT_CHECK1 + i];

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
	FlagsFromEnt();
	RefreshKeyValues();

	SendMessage(g_qeglobals.d_hwndEntity, WM_SETREDRAW, 1, 0);
	RedrawWindow(g_qeglobals.d_hwndEntity, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_FRAME | RDW_ERASENOW | RDW_UPDATENOW | RDW_ALLCHILDREN);
}

void WndEntity::ForceUpdate()
{
	UpdateUI();
}


/*
==============
WndEntity::TryFieldCNP

message passing is still a mess...
==============
*/
bool WndEntity::TryFieldCNP(int tryMsg)
{
	HWND focus = GetFocus();
	if (focus == w_hwndCtrls[ENT_KEYFIELD])
	{
		SendMessage(w_hwndCtrls[ENT_KEYFIELD], tryMsg, 0, 0);
		return true;
	}
	if (focus == w_hwndCtrls[ENT_VALUEFIELD])
	{
		SendMessage(w_hwndCtrls[ENT_VALUEFIELD], tryMsg, 0, 0);
		return true;
	}
	return false;
}

bool WndEntity::TryCut() { return TryFieldCNP(WM_CUT); }
bool WndEntity::TryCopy() { return TryFieldCNP(WM_COPY); }
bool WndEntity::TryPaste() { return TryFieldCNP(WM_PASTE); }

/*
==============
WndEntity::ResizeControls

Update the listbox, checkboxes and k/v pairs to reflect the new selection
==============
*/
void WndEntity::ResizeControls()
{
	int nWidth, nHeight;
	nWidth = clientRect.right;
	nHeight = clientRect.bottom;

	int		i, x, y, w;
	int		xCheck, yCheck, fold;
	RECT	rectClasses, rectComment, rectProps, rectFlags, rectKV, rectAngle;
	int		flagRows, flagCols, flagOffset;

	SendMessage(g_qeglobals.d_hwndEntity, WM_SETREDRAW, 0, 0);
	MoveRect(w_hwndEntDialog, clientRect);

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

	MoveRect(w_hwndCtrls[ENT_CLASSLIST], rectClasses);
	MoveRect(w_hwndCtrls[ENT_COMMENT], rectComment);

	x = rectFlags.left;
	xCheck = nWidth / flagCols;	// xCheck = width of a single check box
	for (flagOffset = 0; flagOffset < 12; flagOffset += flagRows)
	{
		y = rectFlags.top;
		for (i = 0; i < flagRows; i++)
		{
			MoveRect(w_hwndCtrls[ENT_CHECK1 + i + flagOffset], x, y, xCheck, yCheck);
			y += yCheck;
		}
		x += xCheck;
	}

	MoveRect(w_hwndCtrls[ENT_PROPS], rectProps);

	w = rectKV.right - (DLGBORDER_X + 45);
	MoveRect(w_hwndCtrls[ENT_KEYLABEL], DLGBORDER_X, rectKV.top, 40, yCheck);
	MoveRect(w_hwndCtrls[ENT_KEYFIELD], DLGBORDER_X + 40, rectKV.top, rectKV.right, yCheck);
	MoveRect(w_hwndCtrls[ENT_VALUELABEL], DLGBORDER_X, rectKV.top + yCheck, 40, yCheck);
	MoveRect(w_hwndCtrls[ENT_VALUEFIELD], DLGBORDER_X + 40, rectKV.top + yCheck, rectKV.right, yCheck);

	x = rectAngle.right / 9;
	y = rectAngle.bottom / 3;
	MoveRect(w_hwndCtrls[ENT_DIR135], rectAngle.left, rectAngle.top, x, y);
	MoveRect(w_hwndCtrls[ENT_DIR90], rectAngle.left + x, rectAngle.top, x, y);
	MoveRect(w_hwndCtrls[ENT_DIR45], rectAngle.left + 2 * x, rectAngle.top, x, y);

	MoveRect(w_hwndCtrls[ENT_DIR180], rectAngle.left, rectAngle.top + y, x, y);
	MoveRect(w_hwndCtrls[ENT_DIR0], rectAngle.left + 2 * x, rectAngle.top + y, x, y);

	MoveRect(w_hwndCtrls[ENT_DIR225], rectAngle.left, rectAngle.top + 2 * y, x, y);
	MoveRect(w_hwndCtrls[ENT_DIR270], rectAngle.left + x, rectAngle.top + 2 * y, x, y);
	MoveRect(w_hwndCtrls[ENT_DIR315], rectAngle.left + 2 * x, rectAngle.top + 2 * y, x, y);

	MoveRect(w_hwndCtrls[ENT_DIRUP], rectAngle.left + 3.5*x, rectAngle.top + 0.5 * y, x, y);
	MoveRect(w_hwndCtrls[ENT_DIRDOWN], rectAngle.left + 3.5*x, rectAngle.top + 1.5 * y, x, y);

	MoveRect(w_hwndCtrls[ENT_ADDPROP], rectAngle.left + 5 * x, rectAngle.top, x * 1.5, y);	// sikk - Entity Window Addition
	MoveRect(w_hwndCtrls[ENT_DELPROP], rectAngle.left + 5 * x, rectAngle.top + y, x * 1.5, y);
	MoveRect(w_hwndCtrls[ENT_COLOR], rectAngle.left + 5 * x, rectAngle.top + y * 2, x * 1.5, y);	// sikk - Entity Window Addition

	MoveRect(w_hwndCtrls[ENT_CREATE], rectAngle.left + 7 * x, rectAngle.top + y * 0.5, x * 2, y * 2);	// sikk - Entity Window Addition

	SendMessage(g_qeglobals.d_hwndEntity, WM_SETREDRAW, 1, 0);
	RedrawWindow(g_qeglobals.d_hwndEntity, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_FRAME | RDW_ERASENOW | RDW_UPDATENOW | RDW_ALLCHILDREN);
}










/*
========================================================================

	ENTITY EDITING OPS

========================================================================
*/

/*
===============
WndEntity::ApplyAngle
===============
*/
void WndEntity::ApplyAngle(int ang)
{
	char sz[8];
	sprintf(sz, "%d", ang);
	SetKeyValue("angle", sz);

	Sys_UpdateWindows(W_CAMERA);
}

/*
===============
WndEntity::FlagChecked
===============
*/
void WndEntity::FlagChecked(int flag)
{
	if (flag < 1 || flag > 12)
		return;	// sanity check, no such flag

	bool on = !(SendMessage(w_hwndCtrls[ENT_CHECK1 + flag - 1], BM_GETCHECK, 0, 0) == BST_CHECKED);
	SendMessage(w_hwndCtrls[ENT_CHECK1 + flag - 1], BM_SETCHECK, on, 0);

	int f = 1 << (flag - 1);

	Entity *last;

	last = nullptr;
	try
	{
		CmdSetSpawnflag *cmd = new CmdSetSpawnflag(flag, on);
		for (Brush *b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
		{
			// skip entity brushes in sequence
			if (b->owner == last)
				continue;
			last = b->owner;
			cmd->AddEntity(last);
		}
		g_cmdQueue.Complete(cmd);
	}
	catch (...)
	{
		return;
	}

	Sys_UpdateWindows(W_SCENE|W_ENTITY);
}

/*
===============
WndEntity::SetKeyValue
===============
*/
void WndEntity::SetKeyValue(const char* key, const char* value)
{
	Modify::SetKeyValue(key,value);
}

void WndEntity::SetKeyValue()
{
	char	key[1024];
	char	value[1024];

	// Get current selection text
	// FIXME: strip accidental whitespace padding
	SendMessage(w_hwndCtrls[ENT_KEYFIELD], WM_GETTEXT, sizeof(key) - 1, (LPARAM)key);
	SendMessage(w_hwndCtrls[ENT_VALUEFIELD], WM_GETTEXT, sizeof(value) - 1, (LPARAM)value);

	Modify::SetKeyValue(key, value);
}

/*
===============
WndEntity::RemoveKeyValue
===============
*/
void WndEntity::RemoveKeyValue ()
{
	char	sz[4096];

	// Get current selection text
	SendMessage(w_hwndCtrls[ENT_KEYFIELD], WM_GETTEXT, sizeof(sz) - 1, (LPARAM)sz);	

	SetKeyValue(sz, "");
}

/*
===============
WndEntity::EditKeyValue
===============
*/
void WndEntity::EditKeyValue ()
{
	int		i;
	HWND	hwnd;
	char	sz[4096];
	char   *val;

	hwnd = w_hwndCtrls[ENT_PROPS];

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

	SendMessage(w_hwndCtrls[ENT_KEYFIELD], WM_SETTEXT, 0, (LPARAM)sz);
	if (_strcmpi(val, KVMIXEDLABEL))
		SendMessage(w_hwndCtrls[ENT_VALUEFIELD], WM_SETTEXT, 0, (LPARAM)val);
	else
		SendMessage(w_hwndCtrls[ENT_VALUEFIELD], WM_SETTEXT, 0, (LPARAM)"");
}

/*
==============
WndEntity::CreateEntity

Creates a new entity based on the currently selected brush and entity type.
==============
*/
void WndEntity::CreateEntity()
{
	EntClass	*ec;
	//	Entity		*e;
	int			i;
	HWND		hwnd;
	//char		sz[1024];

	// find out what type of entity we are trying to create
	hwnd = w_hwndCtrls[ENT_CLASSLIST];
	i = SendMessage(w_hwndCtrls[ENT_CLASSLIST], LB_GETCURSEL, 0, 0);

	if (i < 0)
	{
		MessageBox(g_qeglobals.d_hwndMain, "You must have a class selected to create an entity.", "QuakeEd 3: Entity Creation Info", MB_OK | MB_ICONINFORMATION);
		return;
	}

	//SendMessage(hwnd, LB_GETTEXT, i, (LPARAM)sz);
	ec = (EntClass *)SendMessage(hwnd, LB_GETITEMDATA, i, 0);

	if (!Selection::HasBrushes())
	{
		g_qeglobals.d_vCamera.GetAimPoint(g_brSelectedBrushes.mins);	// FIXME: dum
	}
	else if (Selection::OnlyPointEntities())
	{
		SetKeyValue("classname", ec->name);
	}
	else
	{
		Entity::Create(ec);
	}
}

/*
==============
WndEntity::RefreshKeyValues

Reset the key/value listbox and fill it with the kv pairs from the entity being edited
==============
*/
void WndEntity::RefreshKeyValues()
{
	EPair	   *pep;
	RECT		rc;
	char		sz[4096];
	int			nTabs[] = { 64 };	// sikk - Tab fix

	// set key/value pair list
	GetWindowRect(w_hwndCtrls[ENT_PROPS], &rc);
	SendMessage(w_hwndCtrls[ENT_PROPS], LB_SETCOLUMNWIDTH, (rc.right - rc.left) / 2, 0);
	SendMessage(w_hwndCtrls[ENT_PROPS], LB_RESETCONTENT, 0, 0);
	SendMessage(w_hwndCtrls[ENT_PROPS], LB_SETTABSTOPS, (WPARAM)1, (LPARAM)(LPINT)nTabs);	// sikk - Tab fix

	// Walk through list and add pairs
	for (pep = g_eEditEntity.epairs; pep; pep = pep->next)
	{
		if (pep->value == KVMIXED)
			sprintf(sz, "%s\t%s", (char*)*pep->key, KVMIXEDLABEL);
		else
			sprintf(sz, "%s\t%s", (char*)*pep->key, (char*)*pep->value);

		SendMessage(w_hwndCtrls[ENT_PROPS], LB_ADDSTRING, 0, (LPARAM)sz);
	}
}

/*
==============
WndEntity::FlagsFromEnt

Update the checkboxes to reflect the flag state of the entity
==============
*/
void WndEntity::FlagsFromEnt()
{
	for (int i = 0; i < 12; i++)
	{
		SendMessage(w_hwndCtrls[ENT_CHECK1 + i], BM_SETCHECK, g_nEditEntFlags[i], 0);
	}
}

/*
==============
WndEntity::RefreshEditEntityFlags
==============
*/
void WndEntity::RefreshEditEntityFlags(int inFlags, bool first)
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
WndEntity::RefreshEditEntity
==============
*/
void WndEntity::RefreshEditEntity()
{
	Entity *eo;
	EPair *ep;
	bool first;

	g_eEditEntity.FreeEpairs();
	g_eEditEntity.eclass = NULL;
	memset(g_nEditEntFlags, 0, 12 * sizeof(char));

	eo = NULL;
	first = true;
	for (Brush* b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
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
				if (ep->key == "spawnflags")
				{
					RefreshEditEntityFlags(eo->GetKeyValueInt("spawnflags"), true);
				}
				g_eEditEntity.SetKeyValue((char*)*ep->key, (char*)*ep->value);
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
			if (ep->key == "spawnflags")
			{
				RefreshEditEntityFlags(eo->GetKeyValueInt("spawnflags"), false);
			}
			if (!*g_eEditEntity.GetKeyValue((char*)*ep->key))
				g_eEditEntity.SetKeyValue((char*)*ep->key, (char*)*ep->value);
			//			else if (_strcmpi(ValueForKey(&g_eEditEntity, (char*)*ep->key), (char*)*ep->value) == 0)
			else if (ep->value == g_eEditEntity.GetKeyValue((char*)*ep->key))
				continue;
			else
				g_eEditEntity.SetKeyValue((char*)*ep->key, KVMIXED);
		}
	}
}


