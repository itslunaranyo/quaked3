//==============================
//	build.h
//==============================

#ifndef __BUILD_H__
#define __BUILD_H__

extern char		*g_szBSP_Commands[256];
extern HANDLE	g_hBSP_Process;

void	Sys_CheckBspProcess(void);
void	FillBSPMenu();
void	RunBsp(char *command);
void	DoTestMap();	// sikk - Test Map
void	QE_ExpandBspString(char *bspaction, char *out, char *mapname);


#endif