//==============================
//	win_qe3.c
//==============================

#include "pre.h"
#include "qe3.h"
#include "WndConsole.h"

// for the logging part
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <exception>

// handle to the console log file
// we use low level I/O to get rid of buffering and have everything on file if we crash
int		g_nLogFile;

HCURSOR	g_hcursorWait;
double	g_deltaTime;

//===========================================

/*
==================
Sys_DoubleTime
==================
*/
double Sys_DoubleTime()
{
	return clock() / 1000.0;

	// why does this not yield smooth results?
	//return clock() / CLOCKS_PER_SEC;
}

void Sys_DeltaTime()
{
	static double oldtime, curtime;

	curtime = Sys_DoubleTime();
	g_deltaTime = curtime - oldtime;
	oldtime = curtime;

	g_deltaTime = fmin(0.2, fmax(0.001, g_deltaTime));
}

//===========================================

/*
==================
Sys_ClearPrintf
==================
*/
void Sys_ClearPrintf ()
{
	char text[4];

	text[0] = 0;

	SendMessage(g_hwndConsole, WM_SETTEXT, 0, (LPARAM)text);
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

	if (g_nLogFile)
	{
		_write(g_nLogFile, buf, strlen(buf));
		_commit(g_nLogFile);
	}

	out = Sys_TranslateString(buf);

	WndConsole::AddText(out);
}



/*
==================
Sys_LogFile

called whenever we need to open/close/check the console log file
==================
*/
void Sys_LogFile ()
{
	if (g_cfgEditor.LogConsole && !g_nLogFile)
	{
		// open a file to log the console (if user prefs say so)
		// the file handle is g_nLogFile
		// the log file is erased
		char name[_MAX_PATH];
		char path[_MAX_PATH];
		GetCurrentDirectory(_MAX_PATH, path);
		sprintf(name, "%s/qe3.log", path);
		g_nLogFile = _open(name, _O_TRUNC | _O_CREAT | _O_WRONLY, _S_IREAD | _S_IWRITE);
		if (g_nLogFile)
			Sys_Printf("Console Logging: Started...\n");
		else
			MessageBox(g_hwndMain, "Failed to create log file. Check write permissions in QE3 directory.", "QuakeEd 3: Console Logging Error", MB_OK | MB_ICONEXCLAMATION);
	}	
	else if (g_nLogFile)
	{
		Sys_Printf("Console Logging: Stopped\n");
		_close(g_nLogFile);
		g_nLogFile = 0;
	}
}

//===========================================

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

//===========================================
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
Sys_Beep
==================
*/
void Sys_Beep ()
{
	MessageBeep(MB_ICONASTERISK);
}


//===========================================

/*
==================
Sys_ConvertDOSToUnixName
==================
*/
void Sys_ConvertDOSToUnixName(char *dst, const char *src)
{
	while (*src)
	{
		if (*src == '\\')
			*dst = '/';
		else
			*dst = *src;
		dst++; src++;
	}
	*dst = 0;
}

/*
==================
Sys_TranslateString
==================
*/
char *Sys_TranslateString (char *buf)
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

