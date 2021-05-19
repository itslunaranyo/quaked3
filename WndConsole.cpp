//==============================
//	WndConsole.cpp
//==============================

#include "qe3.h"

WndConsole::WndConsole()
{
	name = "ConsoleWindow";
	vbits = W_CONSOLE;
	minWidth = 64;
	minHeight = 48;
}


WndConsole::~WndConsole()
{
}

void WndConsole::Initialize()
{
	CreateWnd();
	SetTitle("Console");
	GetClientRect(w_hwnd, &clientRect);

	w_hwndCons = CreateWindowEx(WS_EX_CLIENTEDGE,
		"EDIT",
		NULL,
		ES_MULTILINE | ES_READONLY | WS_CHILD | WS_TABSTOP | WS_VISIBLE | WS_VSCROLL,
		clientRect.left, clientRect.top, clientRect.right - clientRect.left, clientRect.bottom - clientRect.top,
		w_hwnd,
		(HMENU)IDC_E_STATUS,
		g_qeglobals.d_hInstance,
		NULL);

	if (!w_hwndCons)
		Error("CreateWindowEx: Failed %i", GetLastError());

	SendMessage(w_hwndCons, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), (LPARAM)TRUE);

	g_qeglobals.d_hwndConsole = w_hwndCons;
}

#define SCROLLBACK_MAX_LINES	600 // PGM
#define SCROLLBACK_DEL_CHARS	500 // PGM


void WndConsole::AddText(const char *txt)
{
	LRESULT		result;				// PGM
	DWORD		oldPosS, oldPosE;	// PGM
	HWND cons = g_qeglobals.d_hwndConsole;

	result = SendMessage(cons, EM_GETLINECOUNT, 0, 0);
	// sikk - place the caret at the end of Console before text is inserted. 
	// This is necessary for RichEdit Console to function correctly when text 
	// is selected or caret position moved
	SendMessage(cons, EM_SETSEL, -1, -1);

	if (result >= SCROLLBACK_MAX_LINES)
	{
		char replaceText[5];

		replaceText[0] = '\0';

		SendMessage(cons, WM_SETREDRAW, (WPARAM)0, (LPARAM)0);
		SendMessage(cons, EM_GETSEL, (WPARAM)&oldPosS, (LPARAM)&oldPosE);
		SendMessage(cons, EM_SETSEL, 0, SCROLLBACK_DEL_CHARS);
		SendMessage(cons, EM_REPLACESEL, (WPARAM)0, (LPARAM)replaceText);
		SendMessage(cons, EM_SETSEL, oldPosS, oldPosE);
		SendMessage(cons, WM_SETREDRAW, (WPARAM)1, (LPARAM)0);
	}
	// <---PGM

	SendMessage(cons, EM_REPLACESEL, 0, (LPARAM)txt);
	SendMessage(cons, EM_SCROLLCARET, 0, 0); // eerie // sikk - removed comment
}

int WndConsole::WindowProcedure(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_MOUSEWHEEL:
		Focus();
		if (wParam & MK_SHIFT)
		{
			SendMessage(w_hwndCons, EM_SCROLL, ((short)HIWORD(wParam) < 0) ? (WPARAM)SB_PAGEDOWN : (WPARAM)SB_PAGEUP, 0);
		}
		else
		{
			SendMessage(w_hwndCons, EM_SCROLL, ((short)HIWORD(wParam) < 0) ? (WPARAM)SB_LINEDOWN : (WPARAM)SB_LINEUP, 0);
		}

		return 0;

	case WM_SIZE:
		GetClientRect(w_hwnd, &clientRect);
		MoveWindow(w_hwndCons, clientRect.left, clientRect.top, clientRect.right - clientRect.left, clientRect.bottom - clientRect.top, false);
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

	case WM_KILLFOCUS:
	case WM_SETFOCUS:
		SendMessage(w_hwnd, WM_NCACTIVATE, uMsg == WM_SETFOCUS, 0);
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

int WndConsole::OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_KEYDOWN)
		return QE_KeyDown(wParam);

	return 1;
}
