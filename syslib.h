//==============================
//	syslib.h
//	formerly win_qe3
//==============================
#ifndef __SYSLIB_H__
#define __SYSLIB_H__

// system functions
double	Sys_DoubleTime();
void	Sys_DeltaTime();

void    Sys_ClearPrintf();
void    Sys_Printf(char *text, ...);
void	Sys_LogFile();

void    Sys_GetCursorPos(int *x, int *y);
void    Sys_SetCursorPos(int x, int y);

void    Sys_BeginWait();
void    Sys_EndWait();
void    Sys_Beep();

void	Sys_ConvertDOSToUnixName(char *dst, const char *src);
char   *Sys_TranslateString(char *buf);

#endif	// __SYSLIB_H__