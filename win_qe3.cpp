//==============================
//	win_qe3.c
//==============================

#include "qe3.h"

// for the logging part
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <exception>

double	g_deltaTime;
int		g_nUpdateBits;
int		g_nScreenWidth;
int		g_nScreenHeight;

char   *g_szBSP_Commands[256];
HANDLE	g_hBSP_Process;

HCURSOR	g_hcursorWait;

char	g_qeAppName[64];

//===========================================

void Sys_DeltaTime()
{
	static double oldtime, curtime;

	curtime = Sys_DoubleTime();
	g_deltaTime = curtime - oldtime;
	oldtime = curtime;

	g_deltaTime = fmin(0.2, fmax(0.001, g_deltaTime));
}

/*
==================
Sys_SetTitle
==================
*/
void Sys_SetTitle (char *text)
{
	char	buf[512];
	int		len;

	len = sprintf(buf, "%s - [%s]", g_qeAppName, text);	// sikk - Put "QuakeEd 3" in title
	/*
	for(int i = 0; i < len; i++) // Make pathnames look more "Win32" (HI HEFFO!)
		if(buf[i] == '/')
			buf[i] = '\\';
	*/
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
//	Sys_Printf("Updating 0x%X\n", bits);
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

	if (g_nUpdateBits & W_TITLE)
		QE_UpdateTitle();
	if (g_nUpdateBits & W_SURF)
		SurfWnd_UpdateUI();

	g_nUpdateBits = 0;
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
	if (g_cfgEditor.LogConsole && !g_qeglobals.d_nLogFile)
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
			Sys_Printf("Console Logging: Started...\n");
		else
			MessageBox(g_qeglobals.d_hwndMain, "Failed to create log file. Check write permissions in QE3 directory.", "QuakeEd 3: Console Logging Error", MB_OK | MB_ICONEXCLAMATION);
	}	
	else if (g_qeglobals.d_nLogFile)
	{
		Sys_Printf("Console Logging: Stopped\n");
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

	va_start(argptr, text);
	vsprintf(buf, text, argptr);
	va_end(argptr);

	if (g_qeglobals.d_nLogFile)
	{
		_write(g_qeglobals.d_nLogFile, buf, strlen(buf));
		_commit(g_qeglobals.d_nLogFile);
	}

	out = TranslateString(buf);

	WndConsole::AddText(out);
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
//static char szPrefabFilter[260] =	/* filter string for pfb files   */ 
//	"QuakeEd Prefab (*.pfb)\0*.pfb\0\0";
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
	strcpy(szDirName, g_project.mapPath);
	if (strlen(szDirName) == 0)
	{
		strcpy(szDirName, g_project.basePath);
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
	strcpy(szDirName, g_project.mapPath);
	if (strlen(szDirName) == 0)
	{
		strcpy(szDirName, g_project.basePath);
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
	g_map.hasFilename = true;

	// Add the file in MRU.
	AddNewItem(g_qeglobals.d_lpMruMenu, ofn.lpstrFile);

	// Refresh the File menu.
	PlaceMenuMRUItem(g_qeglobals.d_lpMruMenu, GetSubMenu(GetMenu(g_qeglobals.d_hwndMain), 0), ID_FILE_EXIT);

	QE_SaveMap();
	//g_map.SaveToFile(ofn.lpstrFile, false);	// ignore region
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

	if (!g_cmdQueue.IsModified())
		return true;

	sprintf(szMessage, "Save Changes to %s?", g_map.name[0] ? g_map.name : "untitled");
	switch (MessageBox(g_qeglobals.d_hwndMain, szMessage, "QuakeEd 3: Save Changes?", MB_YESNOCANCEL | MB_ICONEXCLAMATION))
	{
	case IDYES:
		if (!g_map.hasFilename)
			SaveAsDialog();
		else
			QE_SaveMap();
			//g_map.SaveToFile(g_map.name, false);	// ignore region
		
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
==================
*/
void ImportDialog()
{
	// Obtain the system directory name and store it in szDirName. 
	strcpy(szDirName, g_project.mapPath);
	if (strlen(szDirName) == 0)
	{
		strcpy(szDirName, g_project.basePath);
		strcat(szDirName, "/maps");
	}
	// Filter string for Map files
	ofn.lpstrFilter = szFilter; 

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
==================
*/
void ExportDialog()
{ 
	// Obtain the system directory name and store it in szDirName. 
	strcpy(szDirName, g_project.mapPath);
	if (strlen(szDirName) == 0)
	{
		strcpy(szDirName, g_project.mapPath);
		strcat(szDirName, "/maps");
	}
	// Filter string for Map files
	ofn.lpstrFilter = szFilter; 

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

	if (!g_qeglobals.d_entityProject)
		return;
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

