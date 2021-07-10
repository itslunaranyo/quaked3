//==============================
//	win_qe3.c
//==============================

#include "pre.h"
#include "qe3.h"
#include "WndConsole.h"

// for the logging part
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


/*
=============================================================

	REGISTRY INFO

=============================================================
*/

/*
==============
Sys_RegistrySaveInfo
==============
*/
bool Sys_RegistrySaveInfo(const char *pszName, void *pvBuf, long lSize)
{
	LONG	lres;
	DWORD	dwDisp;
	HKEY	hKeyId;

	lres = RegCreateKeyEx(HKEY_CURRENT_USER, QE3_WIN_REGISTRY, 0, NULL,
		REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeyId, &dwDisp);

	if (lres != ERROR_SUCCESS)
		return false;

	lres = RegSetValueEx(hKeyId, pszName, 0, REG_BINARY, (BYTE*)pvBuf, lSize);

	RegCloseKey(hKeyId);

	if (lres != ERROR_SUCCESS)
		return false;

	return true;
}

/*
==============
Sys_RegistryLoadInfo
==============
*/
bool Sys_RegistryLoadInfo(const char *pszName, void *pvBuf, long *plSize)
{
	HKEY	hKey;
	long	lres, lType, lSize;

	if (plSize == NULL)
		plSize = &lSize;

	lres = RegOpenKeyEx(HKEY_CURRENT_USER, QE3_WIN_REGISTRY, 0, KEY_READ, &hKey);

	if (lres != ERROR_SUCCESS)
		return false;

	lres = RegQueryValueEx(hKey, pszName, NULL, (LPDWORD)&lType, (LPBYTE)pvBuf, (LPDWORD)plSize);

	RegCloseKey(hKey);

	if (lres != ERROR_SUCCESS)
		return false;

	return true;
}
