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
char	g_qePath[MAX_PATH];

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
	g_nUpdateBits |= bits;
}

/*
==================
Sys_ForceUpdateWindows
==================
*/
void Sys_ForceUpdateWindows(int bits)
{
	// redo target lines first so they're available to draw right away
	if (g_nUpdateBits & W_TARGETGRAPH)
		g_map.targetGraph.Refresh(g_map.entities);

	// ensure the layout is rebuilt if necessary even if the texture window isn't on top yet
	if (g_nUpdateBits & W_TEXTURE)
		g_qeglobals.d_vTexture.Arrange();

	// update any windows now
	for (auto wvIt = WndView::wndviews.begin(); wvIt != WndView::wndviews.end(); ++wvIt)
	{
		if ((*wvIt)->vbits & bits)
			(*wvIt)->ForceUpdate();
	}

	if (g_nUpdateBits & W_TITLE)
		QE_UpdateTitle();
	if (g_nUpdateBits & W_SURF)
		WndSurf_UpdateUI();
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

	// why does this not yield smooth results?
	//return clock() / CLOCKS_PER_SEC;
}

/*
==================
PrintPixels
==================
*/
/*
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
*/

//==========================================================================



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

//	if (!Pointfile_Check() && g_qeglobals.d_savedinfo.bTestAfterBSP)
//		DoTestMap();
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

