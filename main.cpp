//==============================
//	win_main.c
//==============================

#include "pre.h"
#include "qe3.h"
#include "select.h"
#include "TextureTool.h"
#include "WndMain.h"
#include "WndCamera.h"
#include "WndConsole.h"
#include "WndSurf.h"
#include "win_dlg.h"

char	g_qeAppName[64];
char	g_qePath[MAX_PATH];

/*
==============
SetWindowRect
=============
*/
void SetWindowRect(HWND hwnd, RECT *rc)
{
	MoveWindow(hwnd, rc->left, rc->top, rc->right - rc->left, rc->bottom - rc->top, TRUE);
}


/*
==============================================================================

	MAIN BUSINESS

==============================================================================
*/

/*
============
SplashDlgProc
============
*/
BOOL CALLBACK SplashDlgProc(
	HWND	hwndDlg,// handle to dialog box
	UINT	uMsg,	// message
	WPARAM	wParam,	// first message parameter
	LPARAM	lParam 	// second message parameter
	)
{

	switch (uMsg)
	{
	case WM_INITDIALOG:
		ShowWindow(hwndDlg, SW_SHOW);
		InvalidateRect(hwndDlg, NULL, FALSE);
		UpdateWindow(hwndDlg);
		SetTimer(hwndDlg, QE_TIMERSPLASH, 3000, nullptr);
		return FALSE;

	case WM_TIMER:
	case WM_LBUTTONDOWN:
		DestroyWindow(hwndDlg);
		return FALSE;
	case WM_CLOSE:
		KillTimer(hwndDlg, QE_TIMERSPLASH);
		return FALSE;
	}
	return 0;
}


/*
============
Main_HandleMWheel
============
*/
void Main_HandleMWheel(MSG &msg)
{
	if (msg.message != WM_MOUSEWHEEL) return;

	POINT p;
	p.x = (short)LOWORD(msg.lParam);
	p.y = (short)HIWORD(msg.lParam);

	msg.hwnd = WndMain_WindowForPoint(p);
	// mouse message coords are client-area relative, except for the mousewheel which
	// is screen-relative, just to make win32 programming a fun adventure
	ScreenToClient(msg.hwnd, &p);
//#define LOWORD(l)		((WORD)(((DWORD_PTR)(l)) & 0xffff))
//#define HIWORD(l)		((WORD)((((DWORD_PTR)(l)) >> 16) & 0xffff))
	p.x = p.x & 0xffff;
	p.y = (p.y & 0xffff) << 16;
	msg.lParam = p.x + p.y;
}

/*
==================
Main_Init
==================
*/
void Main_Init()
{
	time_t	lTime;

#ifndef _DEBUG
	try
	{
#endif
		Sys_LogFile();
		time(&lTime);
		Sys_Printf("%s\nSesson Started: %s\n", g_qeAppName, ctime(&lTime));

		WndMain_Create();

		GLenum glewerr = glewInit();
		if (glewerr != GLEW_OK)
			Error("GLEW init failed! %s", glewGetErrorString(glewerr));

		QE_Init();
		Sys_DeltaTime();
#ifndef _DEBUG
	}
	catch (std::exception &ex)
	{
		QE_Exit(ex.what());
	}
#endif
}

/*
==================
Main_Loop
==================
*/
void Main_Loop()
{
	MSG		msg;
	HACCEL	accelerators;
	bool	haveQuit = false;

	Sys_Printf("Entering message loop...\n");

	accelerators = LoadAccelerators(g_qeglobals.d_hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR1));
	if (!accelerators)
		Error("LoadAccelerators: Failed.");

	while (!haveQuit)
	{
		Sys_EndWait();	// remove wait cursor if active

#ifndef _DEBUG
		try {
#endif
			Sys_DeltaTime();
			while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				Main_HandleMWheel(msg);

				// lunaran - send keys to dialogs first before trying accelerators, so we can tab through
				// the surface dialog and such
				if (!IsDialogMessage(g_hwndSurfaceDlg, &msg) &&
					!IsDialogMessage(g_texTool->hwndReplaceDlg, &msg) &&
					!IsDialogMessage(g_hwndSetKeyvalsDlg, &msg)
					)
				{
					// sikk - We don't want QE3 to handle accelerator shortcuts when editing text in the Entity & Console Windows
					if (!TranslateAccelerator(g_hwndMain, accelerators, &msg))
					{
						try {
							TranslateMessage(&msg);
							DispatchMessage(&msg);
						}
						catch (qe3_exception &ex)
						{
							ReportError(ex);
						}
					}
				}

				if (msg.message == WM_QUIT)
					haveQuit = true;
			}

			// lunaran - all consequences of selection alteration put in one place for consistent behavior
			Selection::HandleChange();
			// some consequences of texture library changes are also best kept out of the loop
			Textures::HandleChange();

			if (g_bWarningOrError)
			{
				g_bWarningOrError = false;
				WndMain_SetInspectorMode(W_CONSOLE);
				WndMain_Status("There were problems: see console for details", 1);
			}

			//Sys_CheckBspProcess();

			// run time dependent behavior
			SendMessage(g_hwndCamera, WM_REALTIME, 0, 0);

			// update any windows now
			WndMain_ForceUpdateWindows();

			// if not driving in the camera view, block
			if (!Tool::HotTool() && !haveQuit)
				WaitMessage();
#ifndef _DEBUG
		}
		catch (std::exception &ex)
		{
			// TODO: none of this ever happens if windows decides to snag the exception first
			CrashSave(ex.what());
		}
#endif
	}
}


/*
==================
WinMain
==================
*/
int WINAPI WinMain (
	HINSTANCE hInstance, 
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, 
	int nCmdShow
	)
{
	g_qeglobals.d_hInstance = hInstance;
// sikk - Quickly made Splash Screen
#ifndef _DEBUG
	HWND hwndSplash;
	hwndSplash = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_SPLASH), g_hwndMain, SplashDlgProc);
#endif
	InitCommonControls ();

	GetCurrentDirectory(MAX_PATH - 1, g_qePath);
	g_bWarningOrError = false;

	// hack for broken NT 4.0 dual screen
	//if (g_nScreenWidth > 2 * g_nScreenHeight)
	//	g_nScreenWidth /= 2;

	if (lpCmdLine && strlen(lpCmdLine))
	{
		ParseCommandLine(lpCmdLine);
		if (g_pszArgV[1])
			Sys_Printf("Command line: %s\n", lpCmdLine);
	}

	// lunaran: can't mix win32 exceptions with c++ exceptions in the same function without
	// compiler flags we don't want to add, so Init and Loop are separated out to allow 
	// catching access violations and stuff at this level only, to try and save work to 
	// disk before finally crashing out
	Main_Init();	
#ifndef _DEBUG
	__try {
#endif
		Main_Loop();
#ifndef _DEBUG
	} __except (CrashSave(SEHExceptionString(GetExceptionCode()))) {
		return FALSE;	// never reached, CrashSave does the work
	}
#endif

    /* return success of application */
    return TRUE;
}
