#include "pre.h"
#include "qe3.h"
#include "build.h"
#include "map.h"
#include "points.h"
#include "WndMain.h"
#include "WndCamera.h"
#include <process.h>

char   *g_szBSP_Commands[256];
HANDLE	g_hBSP_Process;

/*
===============
Sys_CheckBspProcess

See if the BSP is done yet
===============
*/
void Sys_CheckBspProcess(void)
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

		Sys_Printf("\n\n======================================\nMSG: BSP Output\n");
		sprintf(outputpath, "%sqbsp.log", g_project.basePath);
		IO_LoadFile(outputpath, (void **)&out);
		Sys_Printf("\n%s\n", out);
		sprintf(outputpath, "%svis.log", g_project.basePath);
		IO_LoadFile(outputpath, (void **)&out);
		Sys_Printf("\n%s\n", out);
		sprintf(outputpath, "%slight.log", g_project.basePath);
		IO_LoadFile(outputpath, (void **)&out);
		Sys_Printf("\n%s\n", out);
		sprintf(outputpath, "%sremove_skip.log", g_project.basePath);
		IO_LoadFile(outputpath, (void **)&out);
		Sys_Printf("\n%s\n", out);
		Sys_Printf("\n======================================\nMSG: BSP Completed.\n");
	
	free(out);
	Sys_Beep();

	//	if (!Pointfile_Check() && g_qeglobals.d_savedinfo.bTestAfterBSP)
	//		DoTestMap();
}


/*
==================
FillBSPMenu
==================
*/
void FillBSPMenu()
{
	HMENU		hmenu;
	EPair		*ep;
	int			i;
	static int	count;

	hmenu = GetSubMenu(GetMenu(g_hwndMain), MENU_BSP);
	ep = 0;
	for (i = 0; i < count; i++)
		DeleteMenu(hmenu, CMD_BSPCOMMAND + i, MF_BYCOMMAND);
	count = 0;

	//if (!g_qeglobals.d_entityProject)
	//	return;
	i = 0;
	//for (ep = g_qeglobals.d_entityProject->epairs; ep; ep = ep->next)
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

/*
==============
QE_ExpandBspString
==============
*/
void QE_ExpandBspString(char *bspaction, char *out, char *mapname)
{
	char   *in;
	char	src[1024];
	char	rsh[1024];
	char	base[512];

	ExtractFileName(mapname, base);
	// sikk - using "mapspath" instead.... What other use is "mapspath" for?
	//	sprintf(src, "%s/maps/%s", g_qeglobals.d_entityProject->GetKeyValue("remotebasepath"), base);
	sprintf(src, "%s%s", g_project.mapPath, base);

	StripExtension(src);
	strcpy(rsh, g_project.basePath);

	//in = g_qeglobals.d_entityProject->GetKeyValue(bspaction);
	in = 0;
	while (*in)
	{
		if (in[0] == '!')
		{
			strcpy(out, rsh);
			out += strlen(rsh);
			in++;
			continue;
		}
		if (in[0] == '$')
		{
			strcpy(out, src);
			out += strlen(src);
			in++;
			continue;
		}
		if (in[0] == '@')
		{
			*out++ = '"';
			in++;
			continue;
		}
		*out++ = *in++;
	}
	*out = 0;
}

/*
==============
RunBsp
==============
*/
void RunBsp(char *command)
{
	char	sys[1024];
	char	batpath[1024];
	char	outputpath[1024];
	char	temppath[1024];
	char	name[1024];
	char   *basepath;
	FILE   *hFile;
	BOOL	ret;
	STARTUPINFO	startupinfo;
	PROCESS_INFORMATION	ProcessInformation;

	WndMain_SetInspectorMode(W_CONSOLE);

	if (g_hBSP_Process)
	{
		Sys_Printf("BSP is still running...\n");
		return;
	}

	GetTempPath(1024, temppath);
	sprintf(outputpath, "%sjunk.txt", temppath);

	strcpy(name, g_map.name);
	if (g_map.regionActive)
	{
		g_map.SaveToFile(name, false);
		StripExtension(name);
		strcat(name, ".reg");
	}

	g_map.SaveToFile(name, g_map.regionActive);

	QE_ExpandBspString(command, sys, name);

	//	Sys_ClearPrintf();
	Sys_Printf("======================================\nCMD: Running BSP...\n");
	Sys_Printf("\n%s\n", sys);

	//
	// write qe3bsp.bat
	//
	sprintf(batpath, "%sqe3bsp.bat", temppath);
	hFile = fopen(batpath, "w");
	if (!hFile)
		Error("Cannot write to %s", batpath);
	fprintf(hFile, sys);
	fclose(hFile);

	// sikk - TODO: This will be changed when I find a way for realtime progress to console
		//
		// write qe3bsp2.bat
		//
	sprintf(batpath, "%sqe3bsp2.bat", temppath);
	hFile = fopen(batpath, "w");
	if (!hFile)
		Error("Cannot write to %s", batpath);
	fprintf(hFile, "%sqe3bsp.bat > %s", temppath, outputpath);
	fclose(hFile);

	Pointfile_Delete();

	basepath = g_project.basePath;
	if (basepath && !(*basepath))
		basepath = NULL;

	GetStartupInfo(&startupinfo);

	ret = CreateProcess(
		batpath,				// pointer to name of executable module
		NULL,					// pointer to command line string
		NULL,					// pointer to process security attributes
		NULL,					// pointer to thread security attributes
		FALSE,					// handle inheritance flag
		0 /*DETACHED_PROCESS*/,	// creation flags
		NULL,					// pointer to new environment block
		basepath,				// pointer to current directory name
		&startupinfo,			// pointer to STARTUPINFO
		&ProcessInformation 	// pointer to PROCESS_INFORMATION
	);

	if (!ret)
		Error("CreateProcess: Failed.");

	g_hBSP_Process = ProcessInformation.hProcess;

	Sleep(100);	// give the new process a chance to open it's window

	BringWindowToTop(g_hwndMain);	// pop us back on top
	SetFocus(g_hwndCamera);
}

// sikk---> Test Map
/*
==============
DoTestMap
=============
*/
void DoTestMap()
{
	char	szWorkDir[MAX_PATH] = "";
	char	szParam[256] = "";
	char   *szMapName = (char*)&g_map.name[strlen(g_project.mapPath)];
	char	szBSPName[MAX_PATH] = "";
	int		handle;
	struct _finddata_t fileinfo;

	// strip "map" extension 
	strncpy(szBSPName, g_map.name, strlen(g_map.name) - 3);
	// append "bsp" extension
	strcat(szBSPName, "bsp");

	handle = _findfirst(szBSPName, &fileinfo);

	// if bsp file does not exist, return
	if (handle == -1)
	{
		MessageBox(g_hwndMain, "The respective BSP file does not exist.\nYou must build current map first", "QuakeEd 3: Error", MB_OK | MB_ICONEXCLAMATION);
		return;
	}

	/*
	if (g_qeglobals.d_savedinfo.bModName)
	{
		strcat(szParam, " -game ");
		strcat(szParam, g_qeglobals.d_savedinfo.szModName);
	}
	if (g_qeglobals.d_savedinfo.bHeapsize)
	{
		strcat(szParam, " -heapsize ");
		strcat(szParam, g_qeglobals.d_savedinfo.szHeapsize);
	}
	if (g_qeglobals.d_savedinfo.bSkill)
	{
		strcat(szParam, " +skill ");
		strcat(szParam, g_qeglobals.d_savedinfo.szSkill);
	}
	if (g_qeglobals.d_savedinfo.bDeathmatch)
		strcat(szParam, " +deathmatch 1");
	if (g_qeglobals.d_savedinfo.bDeveloper)
		strcat(szParam, " +developer 1");
	if (g_qeglobals.d_savedinfo.bRSpeeds)
		strcat(szParam, " +r_speeds 1");
	if (g_qeglobals.d_savedinfo.bPointfile)
		strcat(szParam, " +pointfile 1");

	strcat(szParam, " +map ");
	// strip file extension
	strncat(szParam, szMapName, strlen(szMapName) - 4);
	// strip executable file name for Working Directory
	strncpy(szWorkDir, g_qeglobals.d_savedinfo.szGamePath,
		strlen(g_qeglobals.d_savedinfo.szGamePath) -
		strlen(g_qeglobals.d_savedinfo.szGameName));

	Sys_Printf("======================================\nCMD: Testing map: %s...\n", szMapName);
	Sys_Printf("Parameters: %s\n", szParam);
	ShellExecute(NULL, "open", g_qeglobals.d_savedinfo.szGamePath, szParam, szWorkDir, SW_SHOW);
	*/
}
// <---sikk