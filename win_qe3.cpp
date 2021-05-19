//==============================
//	win_qe3.c
//==============================

#include "qe3.h"

// for the logging part
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <exception>

/*
// sikk---> Mousewheel Handling
#include <zmouse.h>

UINT	g_unMouseWheel;
#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL 0x020A
#endif
// <---sikk
*/
int		g_nUpdateBits;
int		g_nScreenWidth;
int		g_nScreenHeight;

char   *g_szBSP_Commands[256];
HANDLE	g_hBSP_Process;

HCURSOR	g_hcursorWait;

//===========================================

/*
==================
Sys_SetTitle
==================
*/
void Sys_SetTitle (char *text)
{
// sikk---> Put "QuakeEd 3" in title
	char	buf[512];
	int		i, len;

	len = sprintf(buf, "QuakeEd 3 - [%s]", text);
	
	for(i = 0; i < len; i++) // Make pathnames look more "Win32" (HI HEFFO!)
		if(buf[i] == '/')
			buf[i] = '\\';
// <---sikk
	SetWindowText(g_qeglobals.d_hwndMain, buf);
}

/*
==================
Sys_BeginWait
==================
*/
void Sys_BeginWait ()
{
	g_hcursorWait = SetCursor(LoadCursor(NULL, IDC_WAIT));
}

/*
==================
Sys_EndWait
==================
*/
void Sys_EndWait ()
{
	if (g_hcursorWait)
	{
		SetCursor(g_hcursorWait);
		g_hcursorWait = NULL;
	}
}

/*
==================
Sys_GetCursorPos
==================
*/
void Sys_GetCursorPos (int *x, int *y)
{
	POINT lpPoint;

	GetCursorPos(&lpPoint);
	*x = lpPoint.x;
	*y = lpPoint.y;
}

/*
==================
Sys_SetCursorPos
==================
*/
void Sys_SetCursorPos (int x, int y)
{
	SetCursorPos(x, y);
}

/*
==================
Sys_UpdateWindows
==================
*/
void Sys_UpdateWindows (int bits)
{
//	Sys_Printf("MSG: Updating 0x%X\n", bits);
	g_nUpdateBits |= bits;
//	g_nUpdateBits = -1;
}

/*
==================
Sys_ForceUpdateWindows
==================
*/
void Sys_ForceUpdateWindows(int bits)
{
	// update any windows now
	for (auto wvIt = WndView::wndviews.begin(); wvIt != WndView::wndviews.end(); ++wvIt)
	{
		if ((*wvIt)->vbits & bits)
			(*wvIt)->ForceUpdate();
	}
	return;


	if (bits & W_CAMERA)
	{
	//	InvalidateRect(g_qeglobals.d_hwndCamera, NULL, FALSE);
	//	UpdateWindow(g_qeglobals.d_hwndCamera);
		g_qeglobals.d_wndCamera->ForceUpdate();
	}
	if (bits & W_XY)
	{
		InvalidateRect(g_qeglobals.d_hwndXYZ[0], NULL, FALSE);
		UpdateWindow(g_qeglobals.d_hwndXYZ[0]);
		// sikk---> Multiple Orthographic Views
		if (g_qeglobals.d_savedinfo.bShow_XYZ[2])
		{
			InvalidateRect(g_qeglobals.d_hwndXYZ[2], NULL, FALSE);
			UpdateWindow(g_qeglobals.d_hwndXYZ[2]);
		}
		if (g_qeglobals.d_savedinfo.bShow_XYZ[1])
		{
			InvalidateRect(g_qeglobals.d_hwndXYZ[1], NULL, FALSE);
			UpdateWindow(g_qeglobals.d_hwndXYZ[1]);
		}
		// <---sikk
	}
	if (bits & W_Z)
	{
		InvalidateRect(g_qeglobals.d_hwndZ, NULL, FALSE);
		UpdateWindow(g_qeglobals.d_hwndZ);
	}
	if (bits & W_TEXTURE)
	{
		InvalidateRect(g_qeglobals.d_hwndTexture, NULL, FALSE);
		UpdateWindow(g_qeglobals.d_hwndInspector);
	}
}

/*
==================
Sys_Beep
==================
*/
void Sys_Beep ()
{
	MessageBeep(MB_ICONASTERISK);
}

/*
==================
TranslateString
==================
*/
char *TranslateString (char *buf)
{
	static char	buf2[32768];
	int			i, l;
	char	   *out;

	l = strlen(buf);
	out = buf2;
	for (i = 0; i < l; i++)
	{
		if (buf[i] == '\n')
		{
			*out++ = '\r';
			*out++ = '\n';
		}
		else
			*out++ = buf[i];
	}
	*out++ = 0;

	return buf2;
}

/*
==================
Sys_LogFile

called whenever we need to open/close/check the console log file
==================
*/
void Sys_LogFile ()
{
	if (g_qeglobals.d_savedinfo.bLogConsole && !g_qeglobals.d_nLogFile)
	{
		// open a file to log the console (if user prefs say so)
		// the file handle is g_qeglobals.logfile
		// the log file is erased
		char name[_MAX_PATH];
		char path[_MAX_PATH];
		GetCurrentDirectory(_MAX_PATH, path);
		sprintf(name, "%s/qe3.log", path);
		g_qeglobals.d_nLogFile = _open(name, _O_TRUNC | _O_CREAT | _O_WRONLY, _S_IREAD | _S_IWRITE);
		if (g_qeglobals.d_nLogFile)
			Sys_Printf("CMD: Console Logging: Started...\n");
		else
			MessageBox(g_qeglobals.d_hwndMain, "Failed to create log file. Check write permissions in QE3 directory.", "QuakeEd 3: Console Logging Error", MB_OK | MB_ICONEXCLAMATION);
	}	
	else if (g_qeglobals.d_nLogFile)
	{
		Sys_Printf("CMD: Console Logging: Stopped\n");
		_close(g_qeglobals.d_nLogFile);
		g_qeglobals.d_nLogFile = 0;
	}
}

/*
==================
Sys_ClearPrintf
==================
*/
void Sys_ClearPrintf ()
{
	char text[4];

	text[0] = 0;

	SendMessage(g_qeglobals.d_hwndConsole, WM_SETTEXT, 0, (LPARAM)text);
}

#define SCROLLBACK_MAX_LINES	600 // PGM
#define SCROLLBACK_DEL_CHARS	500 // PGM

/*
==================
Sys_Printf
==================
*/
void Sys_Printf (char *text, ...)
{
	va_list		argptr;
	char		buf[32768];
	char	   *out;
	LRESULT		result;				// PGM
	DWORD		oldPosS, oldPosE;	// PGM

	va_start(argptr, text);
	vsprintf(buf, text, argptr);
	va_end(argptr);

	if (g_qeglobals.d_nLogFile)
	{
		_write(g_qeglobals.d_nLogFile, buf, strlen(buf));
		_commit(g_qeglobals.d_nLogFile);
	}

	out = TranslateString(buf);

#ifdef LATER
	Sys_Status(out);
#else

// PGM--->
	result = SendMessage(g_qeglobals.d_hwndConsole, EM_GETLINECOUNT, 0, 0);
	// sikk - place the caret at the end of Console before text is inserted. 
	// This is necessary for RichEdit Console to function correctly when text 
	// is selected or caret position moved
	SendMessage(g_qeglobals.d_hwndConsole, EM_SETSEL, -1, -1);	

	if (result >= SCROLLBACK_MAX_LINES)
	{
		char replaceText[5];
		
		replaceText[0] = '\0';

		SendMessage(g_qeglobals.d_hwndConsole, WM_SETREDRAW, (WPARAM)0, (LPARAM)0);
		SendMessage(g_qeglobals.d_hwndConsole, EM_GETSEL, (WPARAM)&oldPosS, (LPARAM)&oldPosE);
		SendMessage(g_qeglobals.d_hwndConsole, EM_SETSEL, 0, SCROLLBACK_DEL_CHARS);
		SendMessage(g_qeglobals.d_hwndConsole, EM_REPLACESEL, (WPARAM)0, (LPARAM)replaceText);
		SendMessage(g_qeglobals.d_hwndConsole, EM_SETSEL, oldPosS, oldPosE);
		SendMessage(g_qeglobals.d_hwndConsole, WM_SETREDRAW, (WPARAM)1, (LPARAM)0);
	}
// <---PGM

	SendMessage(g_qeglobals.d_hwndConsole, EM_REPLACESEL, 0, (LPARAM)out);
	SendMessage(g_qeglobals.d_hwndConsole, EM_SCROLLCARET, 0, 0); // eerie // sikk - removed comment

#endif
}

/*
==================
Sys_DoubleTime
==================
*/
double Sys_DoubleTime ()
{
	return clock() / 1000.0;
}

/*
==================
PrintPixels
==================
*/
void PrintPixels (HDC hDC)
{
	int		i;
	PIXELFORMATDESCRIPTOR p[64];

	printf("### flags color layer\n");
	for (i = 1; i < 64; i++)
	{
		if (!DescribePixelFormat(hDC, i, sizeof(p[0]), &p[i]))
			break;
		printf("%3i %5i %5i %5i\n", i, p[i].dwFlags, p[i].cColorBits, p[i].bReserved);
	}
	printf("%d modes\n", i - 1);
}

//==========================================================================

/*
==================
QEW_StopGL
==================
*/
void QEW_StopGL (HWND hWnd, HGLRC hGLRC, HDC hDC)
{
	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(hGLRC);
	ReleaseDC(hWnd, hDC);
}
		
/*
==================
QEW_SetupPixelFormat
==================
*/
int QEW_SetupPixelFormat (HDC hDC, bool zbuffer)
{
    static PIXELFORMATDESCRIPTOR pfd = 
	{
		sizeof(PIXELFORMATDESCRIPTOR),	// size of this pfd
		1,								// version number
		PFD_DRAW_TO_WINDOW |			// support window
		PFD_SUPPORT_OPENGL |			// support OpenGL
		PFD_DOUBLEBUFFER,				// double buffered
		PFD_TYPE_RGBA,					// RGBA type
		24,								// 24-bit color depth
		0, 0, 0, 0, 0, 0,				// color bits ignored
		0,								// no alpha buffer
		0,								// shift bit ignored
		0,								// no accumulation buffer
		0, 0, 0, 0,						// accum bits ignored
		32,							    // depth bits
		0,								// no stencil buffer
		0,								// no auxiliary buffer
		PFD_MAIN_PLANE,					// main layer
		0,								// reserved
		0, 0, 0							// layer masks ignored
    };
    int pixelformat = 0;

	zbuffer = true;
	if (!zbuffer )
		pfd.cDepthBits = 0;

    if ((pixelformat = ChoosePixelFormat(hDC, &pfd)) == 0)
	{
		printf("%d",GetLastError());
        Error("ChoosePixelFormat: Failed");
	}

    if (!SetPixelFormat(hDC, pixelformat, &pfd))
        Error("SetPixelFormat: Failed");

	return pixelformat;
}

/*
=================
Error

For abnormal program terminations
=================
*/
void Error (char *error, ...)
{
	va_list argptr;
	char	text[1024];
//	char	text2[1024];
//	int		err;

	va_start(argptr,error);
	vsprintf(text, error,argptr);
	va_end(argptr);

	Sys_Printf("ERROR: %s\n", text);
	throw std::exception(text);

//	err = GetLastError();
//	sprintf(text2, "%s\nGetLastError() = %d", text, err);
//	MessageBox(g_qeglobals.d_hwndMain, text2, "QuakeEd 3: Error", MB_OK | MB_ICONEXCLAMATION);

	// close logging if necessary
//	g_qeglobals.d_savedinfo.bLogConsole = false;
//	Sys_LogFile();

//	exit(1);
}

/*
======================================================================

FILE DIALOGS

======================================================================
*/
 
static OPENFILENAME ofn;			/* common dialog box structure   */ 
static char	szDirName[_MAX_PATH];   /* directory string              */ 
static char szFile[_MAX_PATH];		/* filename string               */ 
static char szFileTitle[_MAX_FNAME];/* file title string             */ 
static char szFilter[260] =			/* filter string for map files   */ 
	"QuakeEd Map (*.map)\0*.map\0\0";
static char szProjectFilter[260] =	/* filter string for qe3 files   */ 
	"QuakeEd Project (*.qe3)\0*.qe3\0\0";
static char szPrefabFilter[260] =	/* filter string for pfb files   */ 
	"QuakeEd Prefab (*.pfb)\0*.pfb\0\0";
static char chReplace;				/* string separator for szFilter */ 
static int	i, cbString;			/* integer count variables       */ 
static HANDLE hf;					/* file handle                   */ 

/*
==================
OpenDialog
==================
*/
void OpenDialog ()
{
	// Obtain the system directory name and store it in szDirName. 
	strcpy(szDirName, g_qeglobals.d_entityProject->GetKeyValue("mapspath"));
	if (strlen(szDirName) == 0)
	{
		strcpy(szDirName, g_qeglobals.d_entityProject->GetKeyValue("basepath"));
		strcat(szDirName, "/maps");
	}

	// Place the terminating null character in the szFile.
	szFile[0] = '\0'; 
 
	// Set the members of the OPENFILENAME structure. 
	ofn.lStructSize		= sizeof(OPENFILENAME); 
	ofn.hwndOwner		= g_qeglobals.d_hwndMain;//g_qeglobals.d_hwndCamera;
	ofn.lpstrFilter		= szFilter; 
	ofn.nFilterIndex	= 1; 
	ofn.lpstrFile		= szFile; 
	ofn.nMaxFile		= sizeof(szFile); 
	ofn.lpstrFileTitle	= szFileTitle; 
	ofn.nMaxFileTitle	= sizeof(szFileTitle); 
	ofn.lpstrInitialDir	= szDirName; 
	ofn.Flags			= OFN_SHOWHELP | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST; 

	// Display the Open dialog box.
	if (!GetOpenFileName(&ofn))
		return;	// canceled
 
	// Add the file in MRU.
	AddNewItem(g_qeglobals.d_lpMruMenu, ofn.lpstrFile);

	// Refresh the File menu.
	PlaceMenuMRUItem(g_qeglobals.d_lpMruMenu, GetSubMenu(GetMenu(g_qeglobals.d_hwndMain), 0), ID_FILE_EXIT);

	// Open the file.
	g_map.LoadFromFile(ofn.lpstrFile);
}

/*
==================
SaveAsDialog
==================
*/
void SaveAsDialog ()
{ 
	strcpy(szDirName, g_qeglobals.d_entityProject->GetKeyValue("mapspath"));
	if (strlen(szDirName) == 0)
	{
		strcpy(szDirName, g_qeglobals.d_entityProject->GetKeyValue("basepath"));
		strcat(szDirName, "/maps");
	}

	// Place the terminating null character in the szFile. 
	szFile[0] = '\0'; 
 
	// Set the members of the OPENFILENAME structure.
	ofn.lStructSize		= sizeof(OPENFILENAME); 
	ofn.hwndOwner		= g_qeglobals.d_hwndMain;//g_qeglobals.d_hwndCamera;
	ofn.lpstrFilter		= szFilter; 
	ofn.nFilterIndex	= 1; 
	ofn.lpstrFile		= szFile; 
	ofn.nMaxFile		= sizeof(szFile); 
	ofn.lpstrFileTitle	= szFileTitle; 
	ofn.nMaxFileTitle	= sizeof(szFileTitle); 
	ofn.lpstrInitialDir	= szDirName; 
	ofn.Flags			= OFN_SHOWHELP | OFN_PATHMUSTEXIST | 
						  OFN_FILEMUSTEXIST | OFN_OVERWRITEPROMPT; 

	// Display the Open dialog box. 
	if (!GetSaveFileName(&ofn))
		return;	// canceled
  
	DefaultExtension(ofn.lpstrFile, ".map");
	strcpy(g_map.name, ofn.lpstrFile);

	// Add the file in MRU.
	AddNewItem(g_qeglobals.d_lpMruMenu, ofn.lpstrFile);

	// Refresh the File menu.
	PlaceMenuMRUItem(g_qeglobals.d_lpMruMenu, GetSubMenu(GetMenu(g_qeglobals.d_hwndMain), 0), ID_FILE_EXIT);

	g_map.SaveToFile(ofn.lpstrFile, false);	// ignore region
}


// <---sikk

// sikk---> Changed to Save Changes Dialog
/*
==================
ConfirmModified
==================
*/
bool ConfirmModified ()
{
	char szMessage[128];

	if (!g_map.modified)
		return true;

	sprintf(szMessage, "Save Changes to %s?", g_map.name);
	switch (MessageBox(g_qeglobals.d_hwndMain, szMessage, "QuakeEd 3: Save Changes?", MB_YESNOCANCEL | MB_ICONEXCLAMATION))
	{
	case IDYES:
		if (!strcmp(g_map.name, "unnamed.map"))
			SaveAsDialog();
		else
			g_map.SaveToFile(g_map.name, false);	// ignore region
		
		return true;
		break;

	case IDNO:
		return true;
		break;

	case IDCANCEL:
		return false;
		break;
	}
	return true;	
}
// <---sikk

// sikk---> Import/Export Dialogs for Map/Prefab
/*
==================
ImportDialog

Argument: true = Map | false = Prefab
==================
*/
void ImportDialog (bool bCheck)
{
	if (bCheck)	// Importing a Map File
	{
		// Obtain the system directory name and store it in szDirName. 
		strcpy(szDirName, g_qeglobals.d_entityProject->GetKeyValue("mapspath"));
		if (strlen(szDirName) == 0)
		{
			strcpy(szDirName, g_qeglobals.d_entityProject->GetKeyValue("basepath"));
			strcat(szDirName, "/maps");
		}
		// Filter string for Map files
		ofn.lpstrFilter = szFilter; 
	}
	else	// Importing a Prefab File
	{
		// Obtain Prefab Path and store it in szDirName. 
		strcpy(szDirName, g_qeglobals.d_savedinfo.szPrefabPath);
		if (strlen(g_qeglobals.d_savedinfo.szPrefabPath) == 0)
			// if Prefab Path is empty, use mapspath as default
			strcpy(szDirName, g_qeglobals.d_entityProject->GetKeyValue("mapspath"));
		// Filter string for Prefab files
		ofn.lpstrFilter = szPrefabFilter; 

	}

	// Place the terminating null character in the szFile.
	szFile[0] = '\0'; 
 
	// Set the members of the OPENFILENAME structure. 
	ofn.lStructSize		= sizeof(OPENFILENAME); 
	ofn.hwndOwner		= g_qeglobals.d_hwndMain;//g_qeglobals.d_hwndCamera;
	ofn.nFilterIndex	= 1; 
	ofn.lpstrFile		= szFile; 
	ofn.nMaxFile		= sizeof(szFile); 
	ofn.lpstrFileTitle	= szFileTitle; 
	ofn.nMaxFileTitle	= sizeof(szFileTitle); 
	ofn.lpstrInitialDir	= szDirName; 
	ofn.Flags			= OFN_SHOWHELP | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST; 

	// Display the Open dialog box.
	if (!GetOpenFileName(&ofn))
		return;	// canceled
 
	// Open the file.
	g_map.ImportFromFile(ofn.lpstrFile);	
}

/*
==================
ExportDialog

Argument: true = Map | false = Prefab
==================
*/
void ExportDialog (bool bCheck)
{ 
	if (bCheck)	// Exporting a Map File
	{
		// Obtain the system directory name and store it in szDirName. 
		strcpy(szDirName, g_qeglobals.d_entityProject->GetKeyValue("mapspath"));
		if (strlen(szDirName) == 0)
		{
			strcpy(szDirName, g_qeglobals.d_entityProject->GetKeyValue("basepath"));
			strcat(szDirName, "/maps");
		}
		// Filter string for Map files
		ofn.lpstrFilter = szFilter; 
	}
	else	// Exporting a Prefab File
	{
		// Obtain Prefab Path and store it in szDirName. 
		strcpy(szDirName, g_qeglobals.d_savedinfo.szPrefabPath);
		// if Prefab Path is empty, use mapspath as default
		if (strlen(g_qeglobals.d_savedinfo.szPrefabPath) == 0)
			strcpy(szDirName, g_qeglobals.d_entityProject->GetKeyValue("mapspath"));
		// Filter string for Prefab files
		ofn.lpstrFilter = szPrefabFilter; 
	}

	// Place the terminating null character in the szFile. 
	szFile[0] = '\0'; 
 
	// Set the members of the OPENFILENAME structure.
	ofn.lStructSize		= sizeof(OPENFILENAME); 
	ofn.hwndOwner		= g_qeglobals.d_hwndMain;//g_qeglobals.d_hwndCamera;
	ofn.nFilterIndex	= 1; 
	ofn.lpstrFile		= szFile; 
	ofn.nMaxFile		= sizeof(szFile); 
	ofn.lpstrFileTitle	= szFileTitle; 
	ofn.nMaxFileTitle	= sizeof(szFileTitle); 
	ofn.lpstrInitialDir	= szDirName; 
	ofn.Flags			= OFN_SHOWHELP | OFN_PATHMUSTEXIST | 
						  OFN_FILEMUSTEXIST | OFN_OVERWRITEPROMPT; 

	// Display the Open dialog box. 
	if (!GetSaveFileName(&ofn))
		return;	// canceled
  
	//if (bCheck)
		DefaultExtension(ofn.lpstrFile, ".map");
	//else
	//	DefaultExtension(ofn.lpstrFile, ".pfb");

	g_map.ExportToFile(ofn.lpstrFile);	// ignore region
}
// <---sikk


/*
=======================================================

Menu modifications

=======================================================
*/

/*
==================
FillBSPMenu
==================
*/
void FillBSPMenu ()
{
	HMENU		hmenu;
	EPair		*ep;
	int			i;
	static int	count;

	hmenu = GetSubMenu(GetMenu(g_qeglobals.d_hwndMain), MENU_BSP);

	for (i = 0; i < count; i++)
		DeleteMenu(hmenu, CMD_BSPCOMMAND + i, MF_BYCOMMAND);
	count = 0;

	i = 0;
	for (ep = g_qeglobals.d_entityProject->epairs; ep; ep=ep->next)
	{
		if (ep->key[0] == 'b' && ep->key[1] == 's' && ep->key[2] == 'p')
		{
			g_szBSP_Commands[i] = (char*)*ep->key;
			AppendMenu(hmenu, MF_ENABLED | MF_STRING, CMD_BSPCOMMAND + i, (LPCTSTR)*ep->key);
			i++;
		}
	}
	count = i;
}

//==============================================

/*
===============
Sys_CheckBspProcess

See if the BSP is done yet
===============
*/
void Sys_CheckBspProcess (void)
{
	char	outputpath[MAX_PATH];
	char	temppath[1024];
	DWORD	exitcode;
	char   *out;
	BOOL	ret;

	if (!g_hBSP_Process)
		return;

	ret = GetExitCodeProcess(g_hBSP_Process, &exitcode);
	if (!ret)
		Error("GetExitCodeProcess: Failed.");
	if (exitcode == STILL_ACTIVE)
		return;

	g_hBSP_Process = 0;

// sikk - TODO: This will be changed when I find a way for realtime progress to console
	GetTempPath(1024, temppath);
	sprintf(outputpath, "%sjunk.txt", temppath);
	IO_LoadFile(outputpath, (void **)&out);
	Sys_Printf("%s", out);

/*	Sys_Printf("\n\n======================================\nMSG: BSP Output\n");
	sprintf(outputpath, "%sqbsp.log", g_qeglobals.d_entityProject->GetKeyValue("basepath"));
	IO_LoadFile(outputpath, (void **)&out);
	Sys_Printf("\n%s\n", out);
	sprintf(outputpath, "%svis.log", g_qeglobals.d_entityProject->GetKeyValue("basepath"));
	IO_LoadFile(outputpath, (void **)&out);
	Sys_Printf("\n%s\n", out);
	sprintf(outputpath, "%slight.log", g_qeglobals.d_entityProject->GetKeyValue("basepath"));
	IO_LoadFile(outputpath, (void **)&out);
	Sys_Printf("\n%s\n", out);
	sprintf(outputpath, "%sremove_skip.log", g_qeglobals.d_entityProject->GetKeyValue("basepath"));
	IO_LoadFile(outputpath, (void **)&out);
	Sys_Printf("\n%s\n", out);
	Sys_Printf("\n======================================\nMSG: BSP Completed.\n");
*/
	free(out);
	Sys_Beep();

	if (!Pointfile_Check() && g_qeglobals.d_savedinfo.bTestAfterBSP)
		DoTestMap();
}

// sikk---> Mousewheel Handling
/*
============
GetMouseWheelMsg

taken from QE5
============
*/
/*
UINT GetMouseWheelMsg ()
{
	OSVERSIONINFO osvi;

	osvi.dwOSVersionInfoSize = sizeof(osvi);

	if (!GetVersionEx(&osvi))
		return WM_MOUSEWHEEL;	// Got a better idea?

	// NT 4 and later supports WM_MOUSEWHEEL
	if (VER_PLATFORM_WIN32_NT == osvi.dwPlatformId)
        if (osvi.dwMajorVersion >= 4)
			return WM_MOUSEWHEEL;

	// Win98 Supports WM_MOUSEWHEEL, assume that anything above it does too
    if ((osvi.dwMajorVersion >= 5) || (osvi.dwMajorVersion == 4 &&
		 osvi.dwMinorVersion >= 10 && osvi.dwBuildNumber >= 1998))
		return WM_MOUSEWHEEL;

    // Hmmm... an older version. The mouse driver support app should
    // have registered a window message for it. By registering the
    // same message, we should get back the same message number.
    // Note that "MSWHEEL_ROLLMSG" below is a #define taken from ZMOUSE.H,
    // which is from the "Intellimouse SDK".

    return RegisterWindowMessage("MSWHEEL_ROLLMSG");
}
// <---sikk
*/

// sikk---> Quickly made Splash Screen
clock_t	g_clSplashTimer;

/*
============
SplashDlgProc
============
*/
BOOL CALLBACK SplashDlgProc (
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
		g_clSplashTimer = clock();
		return FALSE;

	case WM_LBUTTONDOWN:
		DestroyWindow(hwndDlg);
		return 0;
	}
	return 0;
}
// <---sikk

