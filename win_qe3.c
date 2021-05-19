//==============================
//	win_qe3.c
//==============================

#include "qe3.h"

// for the logging part
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>

// sikk---> Mousewheel Handling
#include <zmouse.h>

UINT	g_unMouseWheel;
#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL 0x020A
#endif
// <---sikk

int		g_nUpdateBits;
int		g_nScreenWidth;
int		g_nScreenHeight;

bool	g_bHaveQuit;

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
	char	text2[1024];
	int		err;

	err = GetLastError();

	va_start(argptr,error);
	vsprintf(text, error,argptr);
	va_end(argptr);

	sprintf(text2, "%s\nGetLastError() = %d", text, err);
    MessageBox(g_qeglobals.d_hwndMain, text2, "QuakeEd 3: Error", MB_OK | MB_ICONEXCLAMATION);

	// close logging if necessary
	g_qeglobals.d_savedinfo.bLogConsole = false;
	Sys_LogFile();

	exit(1);
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
	strcpy(szDirName, ValueForKey(g_qeglobals.d_entityProject, "mapspath"));
	if (strlen(szDirName) == 0)
	{
		strcpy(szDirName, ValueForKey(g_qeglobals.d_entityProject, "basepath"));
		strcat(szDirName, "/maps");
	}

	// Place the terminating null character in the szFile.
	szFile[0] = '\0'; 
 
	// Set the members of the OPENFILENAME structure. 
	ofn.lStructSize		= sizeof(OPENFILENAME); 
	ofn.hwndOwner		= g_qeglobals.d_hwndCamera;
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
	Map_LoadFile(ofn.lpstrFile);	
}

/*
==================
SaveAsDialog
==================
*/
void SaveAsDialog ()
{ 
	strcpy(szDirName, ValueForKey(g_qeglobals.d_entityProject, "mapspath"));
	if (strlen(szDirName) == 0)
	{
		strcpy(szDirName, ValueForKey(g_qeglobals.d_entityProject, "basepath"));
		strcat(szDirName, "/maps");
	}

	// Place the terminating null character in the szFile. 
	szFile[0] = '\0'; 
 
	// Set the members of the OPENFILENAME structure.
	ofn.lStructSize		= sizeof(OPENFILENAME); 
	ofn.hwndOwner		= g_qeglobals.d_hwndCamera;
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
	strcpy(g_szCurrentMap, ofn.lpstrFile);

	// Add the file in MRU.
	AddNewItem(g_qeglobals.d_lpMruMenu, ofn.lpstrFile);

	// Refresh the File menu.
	PlaceMenuMRUItem(g_qeglobals.d_lpMruMenu, GetSubMenu(GetMenu(g_qeglobals.d_hwndMain), 0), ID_FILE_EXIT);

	Map_SaveFile(ofn.lpstrFile, false);	// ignore region
}

/*
==================
ProjectDialog
==================
*/
void ProjectDialog ()
{
	//Obtain the system directory name and store it in szDirName.
 	strcpy(szDirName, ValueForKey(g_qeglobals.d_entityProject, "entitypath"));
	if (strlen(szDirName) == 0)
	{
		strcpy(szDirName, ValueForKey(g_qeglobals.d_entityProject, "basepath"));
		strcat(szDirName, "/scripts");
	}

	// Place the terminating null character in the szFile.
 	szFile[0] = '\0'; 
 
	// Set the members of the OPENFILENAME structure. 
 	ofn.lStructSize		= sizeof(OPENFILENAME); 
	ofn.hwndOwner		= g_qeglobals.d_hwndCamera;
	ofn.lpstrFilter		= szProjectFilter; 
	ofn.nFilterIndex	= 1; 
	ofn.lpstrFile		= szFile; 
	ofn.nMaxFile		= sizeof(szFile); 
	ofn.lpstrFileTitle	= szFileTitle; 
	ofn.nMaxFileTitle	= sizeof(szFileTitle); 
	ofn.lpstrInitialDir = szDirName; 
	ofn.Flags			= OFN_SHOWHELP | OFN_PATHMUSTEXIST;

	// Display the Open dialog box.
 	if (!GetOpenFileName(&ofn))
		return;	// canceled
 
	// Refresh the File menu.
	PlaceMenuMRUItem(g_qeglobals.d_lpMruMenu, GetSubMenu(GetMenu(g_qeglobals.d_hwndMain), 0), ID_FILE_EXIT);

	// Open the file.
	if (!QE_LoadProject(ofn.lpstrFile))
//		Error("Could not load project file.");
		DoProject(true);

	// sikk - save loaded project file
	strcpy(g_qeglobals.d_savedinfo.szLastProject, ofn.lpstrFile);
}

// sikk---> New Project Dialog
/*
==================
NewProjectDialog
==================
*/
void NewProjectDialog ()
{ 

 	strcpy(szDirName, ValueForKey(g_qeglobals.d_entityProject, "entitypath"));
	if (strlen(szDirName) == 0)
	{
		strcpy(szDirName, ValueForKey(g_qeglobals.d_entityProject, "basepath"));
		strcat(szDirName, "/scripts");
	}

	// Place the terminating null character in the szFile. 
	szFile[0] = '\0'; 
 
	// Set the members of the OPENFILENAME structure.
	ofn.lStructSize		= sizeof(OPENFILENAME); 
	ofn.hwndOwner		= g_qeglobals.d_hwndCamera;
	ofn.lpstrFilter		= szProjectFilter; 
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
  
	DefaultExtension(ofn.lpstrFile, ".qe3");
	strcpy(g_qeglobals.d_savedinfo.szLastProject, ofn.lpstrFile);
	
	DoProject(false);
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

	if (!g_bModified)
		return true;

	sprintf(szMessage, "Save Changes to %s?", g_szCurrentMap);
	switch (MessageBox(g_qeglobals.d_hwndMain, szMessage, "QuakeEd 3: Save Changes?", MB_YESNOCANCEL | MB_ICONEXCLAMATION))
	{
	case IDYES:
		if (!strcmp(g_szCurrentMap, "unnamed.map"))
			SaveAsDialog();
		else
			Map_SaveFile(g_szCurrentMap, false);	// ignore region
		
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
		strcpy(szDirName, ValueForKey(g_qeglobals.d_entityProject, "mapspath"));
		if (strlen(szDirName) == 0)
		{
			strcpy(szDirName, ValueForKey(g_qeglobals.d_entityProject, "basepath"));
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
			strcpy(szDirName, ValueForKey(g_qeglobals.d_entityProject, "mapspath"));
		// Filter string for Prefab files
		ofn.lpstrFilter = szPrefabFilter; 

	}

	// Place the terminating null character in the szFile.
	szFile[0] = '\0'; 
 
	// Set the members of the OPENFILENAME structure. 
	ofn.lStructSize		= sizeof(OPENFILENAME); 
	ofn.hwndOwner		= g_qeglobals.d_hwndCamera;
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
	Map_ImportFile(ofn.lpstrFile, bCheck);	
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
		strcpy(szDirName, ValueForKey(g_qeglobals.d_entityProject, "mapspath"));
		if (strlen(szDirName) == 0)
		{
			strcpy(szDirName, ValueForKey(g_qeglobals.d_entityProject, "basepath"));
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
			strcpy(szDirName, ValueForKey(g_qeglobals.d_entityProject, "mapspath"));
		// Filter string for Prefab files
		ofn.lpstrFilter = szPrefabFilter; 
	}

	// Place the terminating null character in the szFile. 
	szFile[0] = '\0'; 
 
	// Set the members of the OPENFILENAME structure.
	ofn.lStructSize		= sizeof(OPENFILENAME); 
	ofn.hwndOwner		= g_qeglobals.d_hwndCamera;
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
  
	if (bCheck)
		DefaultExtension(ofn.lpstrFile, ".map");
	else
		DefaultExtension(ofn.lpstrFile, ".pfb");

	Map_ExportFile(ofn.lpstrFile, bCheck);	// ignore region
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
	epair_t	   *ep;
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
			g_szBSP_Commands[i] = ep->key;
			AppendMenu(hmenu, MF_ENABLED | MF_STRING, CMD_BSPCOMMAND + i, (LPCTSTR)ep->key);
			i++;
		}
	}
	count = i;
}

//==============================================

/*
===============
CheckBspProcess

See if the BSP is done yet
===============
*/
void CheckBspProcess (void)
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
	LoadFile(outputpath, (void **)&out);
	Sys_Printf("%s", out);

/*	Sys_Printf("\n\n======================================\nMSG: BSP Output\n");
	sprintf(outputpath, "%sqbsp.log", ValueForKey(g_qeglobals.d_entityProject, "basepath"));
	LoadFile(outputpath, (void **)&out);
	Sys_Printf("\n%s\n", out);
	sprintf(outputpath, "%svis.log", ValueForKey(g_qeglobals.d_entityProject, "basepath"));
	LoadFile(outputpath, (void **)&out);
	Sys_Printf("\n%s\n", out);
	sprintf(outputpath, "%slight.log", ValueForKey(g_qeglobals.d_entityProject, "basepath"));
	LoadFile(outputpath, (void **)&out);
	Sys_Printf("\n%s\n", out);
	sprintf(outputpath, "%sremove_skip.log", ValueForKey(g_qeglobals.d_entityProject, "basepath"));
	LoadFile(outputpath, (void **)&out);
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
    MSG		msg;
	double	curtime, oldtime, delta;
	HACCEL	accelerators;
	HWND	hwndSplash;
	char	szProject[_MAX_PATH];	// sikk - Load Last Project
    time_t	lTime;

	g_qeglobals.d_hInstance = hInstance;

// sikk - Quickly made Splash Screen
#ifndef _DEBUG
	hwndSplash = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_SPLASH), g_qeglobals.d_hwndMain, SplashDlgProc);
#endif

	InitCommonControls ();

	g_nScreenWidth = GetSystemMetrics(SM_CXFULLSCREEN);
	g_nScreenHeight = GetSystemMetrics(SM_CYFULLSCREEN);

	// hack for broken NT 4.0 dual screen
	if (g_nScreenWidth > 2 * g_nScreenHeight)
		g_nScreenWidth /= 2;

	accelerators = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR1));
	if (!accelerators)
		Error("LoadAccelerators: Failed.");

	WMain_Create(hInstance);

	Sys_LogFile();

	// lunaran TEMP
	g_qeglobals.d_savedinfo.bSortTexByWad = true;

	WCam_Create(hInstance);
	WXY_Create(hInstance);
	WXZ_Create(hInstance);	// sikk - Multiple Orthographic Views
	WYZ_Create(hInstance);	// sikk - Multiple Orthographic Views
	WZ_Create(hInstance);
	InspWnd_Create(hInstance);

	// sikk - Print App name and current time for logging purposes
	time(&lTime);	
	Sys_Printf("QuakeEd 3 beta (build 105)\nSesson Started: %s\n", ctime(&lTime));

	// sikk - If 'Load Last Project' is checked, load that file,
	// else load default file
	if (g_qeglobals.d_savedinfo.bLoadLastProject)
	{
		if (strlen(g_qeglobals.d_savedinfo.szLastProject) > 0)
			strcpy(szProject, g_qeglobals.d_savedinfo.szLastProject);
		else
		{
			strcpy(g_qeglobals.d_savedinfo.szLastProject, "scripts/quake.qe3");
			strcpy(szProject, g_qeglobals.d_savedinfo.szLastProject);
		}
	}
	else
	{
		strcpy(szProject, "scripts/quake.qe3");
		strcpy(g_qeglobals.d_savedinfo.szLastProject, szProject);
	}
	// the project file can be specified on the command line,
	// or implicitly found in the scripts directory
	if (lpCmdLine && strlen(lpCmdLine))
	{
		ParseCommandLine(lpCmdLine);
		if (!QE_LoadProject(g_pszArgV[1]))
			Error("Could not load %s project file.", g_pszArgV[1]);
	}
	else if (!QE_LoadProject(szProject))
	{
		DoProject(true);	// sikk - Manually create project file if none is found
		if (!QE_LoadProject("scripts/quake.qe3"))
			Error("Could not load scripts/quake.qe3 project file.");
	}

	QE_Init();

	g_unMouseWheel = GetMouseWheelMsg();	// sikk - Mousewheel Handling

	Sys_Printf("MSG: Entering message loop...\n");

	oldtime = Sys_DoubleTime();

	// sikk - Load Last Map if option is set in Preferences
	if (g_qeglobals.d_savedinfo.bLoadLastMap && strcmp(g_qeglobals.d_savedinfo.szLastMap, "unnamed.map"))
		Map_LoadFile(g_qeglobals.d_savedinfo.szLastMap);

	while (!g_bHaveQuit)
	{
		Sys_EndWait();	// remove wait cursor if active

		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{	// sikk - We don't want QE3 to handle accelerator shortcuts when 
			// editing text in the Entity & Console Windows
			if (!TranslateAccelerator(g_qeglobals.d_hwndMain, accelerators, &msg) ||
				(GetTopWindow(g_qeglobals.d_hwndMain) == g_qeglobals.d_hwndInspector))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}

			if (msg.message == WM_QUIT)
				g_bHaveQuit = true;
		}

		// lunaran - all consequences of selection alteration put in one place for consistent behavior
		Select_HandleChange();

		CheckBspProcess();

		curtime = Sys_DoubleTime();
		delta = curtime - oldtime;
		oldtime = curtime;
		if (delta > 0.2)
			delta = 0.2;

// sikk---> Quickly made Splash Screen
#ifndef _DEBUG
		if (hwndSplash)
			if (clock() - g_clSplashTimer > CLOCKS_PER_SEC * 3)
				DestroyWindow(hwndSplash);
#endif
// <---sikk

		// run time dependant behavior
		Cam_MouseControl(delta);

		// update any windows now
		if (g_nUpdateBits & W_CAMERA)
		{
			InvalidateRect(g_qeglobals.d_hwndCamera, NULL, FALSE);
			UpdateWindow(g_qeglobals.d_hwndCamera);
		}
		if (g_nUpdateBits & (W_XY))
		{
			InvalidateRect(g_qeglobals.d_hwndXY, NULL, FALSE);
			UpdateWindow(g_qeglobals.d_hwndXY);
// sikk---> Multiple Orthographic Views
			if (g_qeglobals.d_savedinfo.bShow_XZ)
			{
				InvalidateRect(g_qeglobals.d_hwndXZ, NULL, FALSE);
				UpdateWindow(g_qeglobals.d_hwndXZ);
			}
			if (g_qeglobals.d_savedinfo.bShow_YZ)
			{
				InvalidateRect(g_qeglobals.d_hwndYZ, NULL, FALSE);
				UpdateWindow(g_qeglobals.d_hwndYZ);
			}
// <---sikk
		}
		if (g_nUpdateBits & (W_Z))
		{
			InvalidateRect(g_qeglobals.d_hwndZ, NULL, FALSE);
			UpdateWindow(g_qeglobals.d_hwndZ);
		}
		if (g_nUpdateBits & W_TEXTURE)
		{
			InvalidateRect(g_qeglobals.d_hwndTexture, NULL, FALSE);
			UpdateWindow(g_qeglobals.d_hwndInspector);
		}

		g_nUpdateBits = 0;

		// if not driving in the camera view, block
		if (!g_nCamButtonState && !g_bHaveQuit)
			WaitMessage();
	}

    /* return success of application */
    return TRUE;
}
