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

void    Sys_GetCursorPos(int *x, int *y);
void    Sys_SetCursorPos(int x, int y);

void    Sys_BeginWait();
void    Sys_EndWait();
void    Sys_Beep();

char   *Sys_TranslateString(char *buf);
void	Sys_TranslateString(std::string& str);

bool	Sys_RegistrySaveInfo(const char *pszName, void *pvBuf, long lSize);
bool	Sys_RegistryLoadInfo(const char *pszName, void *pvBuf, long *plSize);

#endif	// __SYSLIB_H__