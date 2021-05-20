//==============================
//	win_main.c
//==============================

#include "qe3.h"
#include <process.h>
#include "io.h"


/*
==============================================================================

	MENU

==============================================================================
*/

// sikk---> Texture Popup Menu
/*
============
DoTexturesPopup
============
*/
void DoTexturesPopup ()
{
	BOOL	retval;
	POINT	point;

	HMENU	menu = GetSubMenu(LoadMenu(g_qeglobals.d_hInstance, MAKEINTRESOURCE(IDR_POPUP_TEXTURE)), 0);

	GetCursorPos(&point);

	retval = TrackPopupMenu(menu, TPM_LEFTALIGN | TPM_NONOTIFY | TPM_RETURNCMD | TPM_RIGHTBUTTON, point.x, point.y, 0, g_qeglobals.d_hwndRebar, NULL);

	PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, retval, 0);

	DestroyMenu(menu);
}
// <---sikk

/*
==============
RunBsp
==============
*/
void RunBsp (char *command)
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

	QE_SetInspectorMode(W_CONSOLE);

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
	if(basepath && !(*basepath))
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

	BringWindowToTop(g_qeglobals.d_hwndMain);	// pop us back on top
	SetFocus(g_qeglobals.d_hwndCamera);
}


bool DoColorSelect(const vec3 rgbIn, vec3 &rgbOut)
{
	CHOOSECOLOR		cc;
	static COLORREF	custom[16];

	cc.lStructSize = sizeof(cc);
	cc.hwndOwner = g_qeglobals.d_hwndMain;
	cc.hInstance = g_qeglobals.d_hInstanceColor;	// eerie
	cc.lpCustColors = custom;
	cc.Flags = CC_FULLOPEN | CC_RGBINIT;

	cc.rgbResult = (int)(rgbIn.r * 255) +
					(((int)(rgbIn.g * 255)) << 8) +
					(((int)(rgbIn.b * 255)) << 16);

	if (!ChooseColor(&cc))
		return false;

	rgbOut.r = (cc.rgbResult & 255);
	rgbOut.g = ((cc.rgbResult >> 8) & 255);
	rgbOut.b = ((cc.rgbResult >> 16) & 255);
	rgbOut /= 255.0f;
	return true;
}

/*
=============
DoColor
=============
*/
BOOL DoColor (int iIndex)
{/*
	CHOOSECOLOR		cc;
	static COLORREF	custom[16];

	cc.lStructSize = sizeof(cc);
	cc.hwndOwner = g_qeglobals.d_hwndMain;
//	cc.hInstance = g_qeglobals.d_hInstance;
	cc.hInstance = g_qeglobals.d_hInstanceColor;	// eerie
	
	cc.rgbResult = (int)(g_qeglobals.d_savedinfo.v3Colors[iIndex][0] * 255) +
				 (((int)(g_qeglobals.d_savedinfo.v3Colors[iIndex][1] * 255)) << 8) +
				 (((int)(g_qeglobals.d_savedinfo.v3Colors[iIndex][2] * 255)) << 16);
    cc.lpCustColors = custom;
    cc.Flags = CC_FULLOPEN | CC_RGBINIT;
//	cc.lCustData;
//	cc.lpfnHook;
//	cc.lpTemplateName

	if (!ChooseColor(&cc))
		return false;

	g_qeglobals.d_savedinfo.v3Colors[iIndex][0] = (cc.rgbResult & 255) / 255.0;
	g_qeglobals.d_savedinfo.v3Colors[iIndex][1] = ((cc.rgbResult >> 8) & 255) / 255.0;
	g_qeglobals.d_savedinfo.v3Colors[iIndex][2] = ((cc.rgbResult >> 16) & 255) / 255.0;

	// scale colors so that at least one component is at 1.0F 
	// if this is meant to select an entity color
	if (iIndex == COLOR_ENTITY)
	{
		float largest = 0.0F;

		if (g_qeglobals.d_savedinfo.v3Colors[iIndex][0] > largest)
			largest = g_qeglobals.d_savedinfo.v3Colors[iIndex][0];
		if (g_qeglobals.d_savedinfo.v3Colors[iIndex][1] > largest)
			largest = g_qeglobals.d_savedinfo.v3Colors[iIndex][1];
		if (g_qeglobals.d_savedinfo.v3Colors[iIndex][2] > largest)
			largest = g_qeglobals.d_savedinfo.v3Colors[iIndex][2];

		if (largest == 0.0F)
		{
			g_qeglobals.d_savedinfo.v3Colors[iIndex][0] = 1.0F;
			g_qeglobals.d_savedinfo.v3Colors[iIndex][1] = 1.0F;
			g_qeglobals.d_savedinfo.v3Colors[iIndex][2] = 1.0F;
		}
		else
		{
			float scaler = 1.0F / largest;

			g_qeglobals.d_savedinfo.v3Colors[iIndex][0] *= scaler;
			g_qeglobals.d_savedinfo.v3Colors[iIndex][1] *= scaler;
			g_qeglobals.d_savedinfo.v3Colors[iIndex][2] *= scaler;
		}
	}

	Sys_UpdateWindows(W_ENTITY|W_CAMERA);
*/
	return true;
}

// sikk--> Color Themes
/*
=============
DoTheme
=============
*/
void DoTheme (vec3 v[])
{
	g_colors.brush[0] = v[0][0];
	g_colors.brush[1] = v[0][1];
	g_colors.brush[2] = v[0][2];

	g_colors.camBackground[0] = v[1][0];
	g_colors.camBackground[1] = v[1][1];
	g_colors.camBackground[2] = v[1][2];

	g_colors.camGrid[0] = v[2][0];
	g_colors.camGrid[1] = v[2][1];
	g_colors.camGrid[2] = v[2][2];

	g_colors.gridBackground[0] = v[3][0];
	g_colors.gridBackground[1] = v[3][1];
	g_colors.gridBackground[2] = v[3][2];

	g_colors.gridBlock[0] = v[4][0];
	g_colors.gridBlock[1] = v[4][1];
	g_colors.gridBlock[2] = v[4][2];

	g_colors.gridMajor[0] = v[5][0];
	g_colors.gridMajor[1] = v[5][1];
	g_colors.gridMajor[2] = v[5][2];

	g_colors.gridMinor[0] = v[6][0];
	g_colors.gridMinor[1] = v[6][1];
	g_colors.gridMinor[2] = v[6][2];

	g_colors.gridText[0] = v[7][0];
	g_colors.gridText[1] = v[7][1];
	g_colors.gridText[2] = v[7][2];

	g_colors.camGrid[0] = v[8][0];
	g_colors.camGrid[1] = v[8][1];
	g_colors.camGrid[2] = v[8][2];

	g_colors.gridText[0] = v[9][0];
	g_colors.gridText[1] = v[9][1];
	g_colors.gridText[2] = v[9][2];

	Sys_UpdateWindows(W_SCENE|W_TEXTURE);
}
// <---sikk

/*
==============
DoMru

Copied from MSDN
==============
*/
BOOL DoMru (HWND hWnd, WORD wId)
{
	char		szFileName[128];
	OFSTRUCT	of;
	BOOL		fExist;

	// lunaran: save confirmation from MRU
	if (!ConfirmModified())
		return FALSE;

	GetMenuItem(g_qeglobals.d_lpMruMenu, wId, TRUE, szFileName, sizeof(szFileName));

	// Test if the file exists.
	fExist = OpenFile(szFileName, &of, OF_EXIST) != HFILE_ERROR;

	if (fExist)
	{
		// Place the file on the top of MRU
		AddNewItem(g_qeglobals.d_lpMruMenu, (LPSTR)szFileName);

		// Now perform opening this file !!!
		g_map.LoadFromFile(szFileName);
	}
	else
		// Remove the file on MRU.
		DelMenuItem(g_qeglobals.d_lpMruMenu, wId, TRUE);

	// Refresh the File menu.
	PlaceMenuMRUItem(g_qeglobals.d_lpMruMenu, GetSubMenu(GetMenu(hWnd), 0), ID_FILE_EXIT);

	return fExist;
}

// sikk---> Test Map
/*
==============
DoTestMap
=============
*/
void DoTestMap ()
{
	char	szWorkDir[MAX_PATH] = "";
	char	szParam[256] = "";
	char   *szMapName = (char*)&g_map.name[strlen(g_qeglobals.d_entityProject->GetKeyValue("mapspath"))];
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
		MessageBox(g_qeglobals.d_hwndMain, "The respective BSP file does not exist.\nYou must build current map first", "QuakeEd 3: Error", MB_OK | MB_ICONEXCLAMATION);
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

// sikk---> Window Positions
/*
==============
SetWindowRect
=============
*/
void SetWindowRect (HWND hwnd, RECT *rc)
{
	MoveWindow(hwnd, rc->left, rc->top, rc->right - rc->left, rc->bottom - rc->top, TRUE);
}

void QE_SwapGridCam()
{
	g_qeglobals.d_wndCamera->Swap(*g_qeglobals.d_wndGrid[0]);
}

void QE_ForceInspectorMode(int nType)
{
	g_qeglobals.d_wndEntity->Hide();
	g_qeglobals.d_wndTexture->Hide();
	g_qeglobals.d_wndConsole->Hide();
	switch (nType)
	{
	case W_ENTITY:
		g_qeglobals.d_wndEntity->Show();
		break;
	case W_TEXTURE:
		g_qeglobals.d_wndTexture->Show();
		break;
	case W_CONSOLE:
	default:
		g_qeglobals.d_wndConsole->Show();
		break;
	}

	g_qeglobals.d_nInspectorMode = nType;
}

void QE_SetInspectorMode(int nType)
{
	WndView *from, *to;
	switch (g_qeglobals.d_nInspectorMode)
	{
	case W_ENTITY:
		from = g_qeglobals.d_wndEntity;
		break;
	case W_TEXTURE:
		from = g_qeglobals.d_wndTexture;
		break;
	case W_CONSOLE:
	default:
		from = g_qeglobals.d_wndConsole;
		break;
	}
	switch (nType)
	{
	case W_ENTITY:
		to = g_qeglobals.d_wndEntity;
		break;
	case W_TEXTURE:
		to = g_qeglobals.d_wndTexture;
		break;
	case W_CONSOLE:
	default:
		to = g_qeglobals.d_wndConsole;
		break;
	}

	to->Swap(*from);
	to->Focus();
	g_qeglobals.d_nInspectorMode = nType;
}

/*
==============
DoWindowPosition
=============
*/
void DoWindowPosition (int nStyle)
{
	RECT	parent, status, toolbar;
	int		hleft, h1, h2, h3, hright;
	int		vtop, v1, vbottom;

	GetWindowRect(g_qeglobals.d_hwndRebar, &toolbar);
	GetWindowRect(g_qeglobals.d_hwndStatus, &status);
	GetClientRect(g_qeglobals.d_hwndMain, &parent);
	parent.top = toolbar.bottom - toolbar.top;
	parent.bottom -= status.bottom - status.top;

	hleft = parent.left;
	hright = parent.right;
	vbottom = parent.bottom;
	vtop = parent.top;
	v1 = min(vbottom - 480, (vtop + vbottom) / 2);

	g_qeglobals.d_nInspectorMode = W_CONSOLE;
	switch (nStyle)
	{
	case 0:	// QE3 Default
		h1 = 64;
		h2 = hright * 0.625f;
		g_qeglobals.d_wndZ->SetPosition		 (hleft, vtop, h1, vbottom, true );

		g_qeglobals.d_wndGrid[0]->SetPosition(h1, vtop, h2, vbottom, true );
		g_qeglobals.d_wndGrid[1]->SetPosition(h1, vtop, h2, vbottom, false );
		g_qeglobals.d_wndGrid[2]->SetPosition(h1, vtop, h2, vbottom, false );

		g_qeglobals.d_wndCamera->SetPosition (h2, vtop, hright, v1, true );
		g_qeglobals.d_wndConsole->SetPosition(h2, v1, hright, vbottom, true );
		g_qeglobals.d_wndEntity->SetPosition (h2, v1, hright, vbottom, false );
		g_qeglobals.d_wndTexture->SetPosition(h2, v1, hright, vbottom, false );
		break;
	case 1:	// QE3 Reverse
		h1 = hright * 0.375f;
		h2 = hright - 64;
		g_qeglobals.d_wndCamera->SetPosition(hleft, vtop, h1, v1, true);
		g_qeglobals.d_wndConsole->SetPosition(hleft, v1, h1, vbottom, true);
		g_qeglobals.d_wndEntity->SetPosition(hleft, v1, h1, vbottom, false);
		g_qeglobals.d_wndTexture->SetPosition(hleft, v1, h1, vbottom, false);

		g_qeglobals.d_wndGrid[0]->SetPosition(h1, vtop, h2, vbottom, true);
		g_qeglobals.d_wndGrid[1]->SetPosition(h1, vtop, h2, vbottom, false);
		g_qeglobals.d_wndGrid[2]->SetPosition(h1, vtop, h2, vbottom, false);

		g_qeglobals.d_wndZ->SetPosition(h2, vtop, hright, vbottom, true);
		break;
	case 2:	// 4 Window w/ Z (Cam Left)
		h1 = 64;
		h3 = hright - 320;
		h2 = (h1 + h3) / 2;
		g_qeglobals.d_wndZ->SetPosition(hleft, vtop, h1, vbottom, true);

		g_qeglobals.d_wndCamera->SetPosition(h1, vtop, h2, v1, true);
		g_qeglobals.d_wndGrid[0]->SetPosition(h2, vtop, h3, v1, true);
		g_qeglobals.d_wndGrid[1]->SetPosition(h1, v1, h2, vbottom, true);
		g_qeglobals.d_wndGrid[2]->SetPosition(h2, v1, h3, vbottom, true);

		g_qeglobals.d_wndConsole->SetPosition(h3, vtop, hright, vbottom, true);
		g_qeglobals.d_wndEntity->SetPosition(h3, vtop, hright, vbottom, false);
		g_qeglobals.d_wndTexture->SetPosition(h3, vtop, hright, vbottom, false);
		break;
	case 3:	// 4 Window w/ Z (Cam Right)
		h1 = 64;
		h3 = hright - 320;
		h2 = (h1 + h3) / 2;
		g_qeglobals.d_wndZ->SetPosition(hleft, vtop, h1, vbottom, true);

		g_qeglobals.d_wndCamera->SetPosition(h2, vtop, h3, v1, true);
		g_qeglobals.d_wndGrid[0]->SetPosition(h1, vtop, h2, v1, true);
		g_qeglobals.d_wndGrid[1]->SetPosition(h1, v1, h2, vbottom, true);
		g_qeglobals.d_wndGrid[2]->SetPosition(h2, v1, h3, vbottom, true);

		g_qeglobals.d_wndConsole->SetPosition(h3, vtop, hright, vbottom, true);
		g_qeglobals.d_wndEntity->SetPosition(h3, vtop, hright, vbottom, false);
		g_qeglobals.d_wndTexture->SetPosition(h3, vtop, hright, vbottom, false);
		break;
	case 4:	// 4 Window w/o Z (Cam Left)
		h1 = 64;
		h3 = hright - 320;
		h2 = h3 / 2;
		g_qeglobals.d_wndZ->SetPosition(hleft, vtop, h1, vbottom, false);

		g_qeglobals.d_wndCamera->SetPosition(hleft, vtop, h2, v1, true);
		g_qeglobals.d_wndGrid[0]->SetPosition(h2, vtop, h3, v1, true);
		g_qeglobals.d_wndGrid[1]->SetPosition(hleft, v1, h2, vbottom, true);
		g_qeglobals.d_wndGrid[2]->SetPosition(h2, v1, h3, vbottom, true);

		g_qeglobals.d_wndConsole->SetPosition(h3, vtop, hright, vbottom, true);
		g_qeglobals.d_wndEntity->SetPosition(h3, vtop, hright, vbottom, false);
		g_qeglobals.d_wndTexture->SetPosition(h3, vtop, hright, vbottom, false);
		break;
	case 5:	// 4 Window w/o Z (Cam Right)
		h1 = 64;
		h3 = hright - 320;
		h2 = h3 / 2;
		g_qeglobals.d_wndZ->SetPosition(hleft, vtop, h1, vbottom, false);

		g_qeglobals.d_wndCamera->SetPosition(h2, vtop, h3, v1, true);
		g_qeglobals.d_wndGrid[0]->SetPosition(hleft, vtop, h2, v1, true);
		g_qeglobals.d_wndGrid[1]->SetPosition(hleft, v1, h2, vbottom, true);
		g_qeglobals.d_wndGrid[2]->SetPosition(h2, v1, h3, vbottom, true);

		g_qeglobals.d_wndConsole->SetPosition(h3, vtop, hright, vbottom, true);
		g_qeglobals.d_wndEntity->SetPosition(h3, vtop, hright, vbottom, false);
		g_qeglobals.d_wndTexture->SetPosition(h3, vtop, hright, vbottom, false);
		break;
	case 6:	// 4 Window Reverse w/ Z (Cam Left)
		h1 = 320;
		h3 = hright - 64;
		h2 = (h1 + h3) / 2;
		g_qeglobals.d_wndConsole->SetPosition(hleft, vtop, h1, vbottom, true);
		g_qeglobals.d_wndEntity->SetPosition(hleft, vtop, h1, vbottom, false);
		g_qeglobals.d_wndTexture->SetPosition(hleft, vtop, h1, vbottom, false);

		g_qeglobals.d_wndCamera->SetPosition(h1, vtop, h2, v1, true);
		g_qeglobals.d_wndGrid[0]->SetPosition(h2, vtop, h3, v1, true);
		g_qeglobals.d_wndGrid[1]->SetPosition(h1, v1, h2, vbottom, true);
		g_qeglobals.d_wndGrid[2]->SetPosition(h2, v1, h3, vbottom, true);

		g_qeglobals.d_wndZ->SetPosition(h3, vtop, hright, vbottom, true);
		break;
	case 7:	// 4 Window Reverse w/ Z (Cam Right)
		h1 = 320;
		h3 = hright - 64;
		h2 = (h1 + h3) / 2;
		g_qeglobals.d_wndConsole->SetPosition(hleft, vtop, h1, vbottom, true);
		g_qeglobals.d_wndEntity->SetPosition(hleft, vtop, h1, vbottom, false);
		g_qeglobals.d_wndTexture->SetPosition(hleft, vtop, h1, vbottom, false);

		g_qeglobals.d_wndCamera->SetPosition(h2, vtop, h3, v1, true);
		g_qeglobals.d_wndGrid[0]->SetPosition(h1, vtop, h2, v1, true);
		g_qeglobals.d_wndGrid[1]->SetPosition(h1, v1, h2, vbottom, true);
		g_qeglobals.d_wndGrid[2]->SetPosition(h2, v1, h3, vbottom, true);

		g_qeglobals.d_wndZ->SetPosition(h3, vtop, hright, vbottom, true);
		break;
	case 8:	// 4 Window Reverse w/o Z (Cam Left)
		h1 = 320;
		h3 = hright - 64;
		h2 = (h1 + hright) / 2;
		g_qeglobals.d_wndConsole->SetPosition(hleft, vtop, h1, vbottom, true);
		g_qeglobals.d_wndEntity->SetPosition(hleft, vtop, h1, vbottom, false);
		g_qeglobals.d_wndTexture->SetPosition(hleft, vtop, h1, vbottom, false);

		g_qeglobals.d_wndCamera->SetPosition(h1, vtop, h2, v1, true);
		g_qeglobals.d_wndGrid[0]->SetPosition(h2, vtop, hright, v1, true);
		g_qeglobals.d_wndGrid[1]->SetPosition(h1, v1, h2, vbottom, true);
		g_qeglobals.d_wndGrid[2]->SetPosition(h2, v1, hright, vbottom, true);

		g_qeglobals.d_wndZ->SetPosition(h3, vtop, hright, vbottom, false);
		break;
	case 9:	// 4 Window Reverse w/o Z (Cam Right)
		h1 = 320;
		h3 = hright - 64;
		h2 = (h1 + hright) / 2;
		g_qeglobals.d_wndConsole->SetPosition(hleft, vtop, h1, vbottom, true);
		g_qeglobals.d_wndEntity->SetPosition(hleft, vtop, h1, vbottom, false);
		g_qeglobals.d_wndTexture->SetPosition(hleft, vtop, h1, vbottom, false);

		g_qeglobals.d_wndCamera->SetPosition(h2, vtop, hright, v1, true);
		g_qeglobals.d_wndGrid[0]->SetPosition(h1, vtop, h2, v1, true);
		g_qeglobals.d_wndGrid[1]->SetPosition(h1, v1, h2, vbottom, true);
		g_qeglobals.d_wndGrid[2]->SetPosition(h2, v1, hright, vbottom, true);

		g_qeglobals.d_wndZ->SetPosition(h3, vtop, hright, vbottom, false);
		break;
	}
	//Sys_UpdateWindows(W_ALL);
}


static int RotateAngleForModifiers()
{
	if (GetKeyState(VK_SHIFT) < 0) return 180;
	if (GetKeyState(VK_CONTROL) < 0) return -90;
	return 90;
}

/*
=============================================================

	REGISTRY INFO

=============================================================
*/

/*
==============
SaveRegistryInfo
==============
*/
BOOL SaveRegistryInfo (const char *pszName, void *pvBuf, long lSize)
{
	LONG	lres;
	DWORD	dwDisp;
	HKEY	hKeyId;

	/*
// sikk---> Preferences: Reset Registry
	if (g_qeglobals.d_bResetRegistry)
	{
		RegDeleteKey(HKEY_CURRENT_USER, QE3_WIN_REGISTRY_MRU);
		RegDeleteKey(HKEY_CURRENT_USER, QE3_WIN_REGISTRY);
		return FALSE;
	}
// <---sikk
	*/

	lres = RegCreateKeyEx(HKEY_CURRENT_USER, QE3_WIN_REGISTRY, 0, NULL,
			REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeyId, &dwDisp);

	if (lres != ERROR_SUCCESS)
		return FALSE;


	lres = RegSetValueEx(hKeyId, pszName, 0, REG_BINARY, (BYTE*)pvBuf, lSize);

	RegCloseKey(hKeyId);

	if (lres != ERROR_SUCCESS)
		return FALSE;

	return TRUE;
}

/*
==============
LoadRegistryInfo
==============
*/
BOOL LoadRegistryInfo (const char *pszName, void *pvBuf, long *plSize)
{
	HKEY	hKey;
	long	lres, lType, lSize;

	if (plSize == NULL)
		plSize = &lSize;

	lres = RegOpenKeyEx(HKEY_CURRENT_USER, QE3_WIN_REGISTRY, 0, KEY_READ, &hKey);

	if (lres != ERROR_SUCCESS)
		return FALSE;

	lres = RegQueryValueEx(hKey, pszName, NULL, (LPDWORD)&lType, (LPBYTE)pvBuf, (LPDWORD)plSize);

	RegCloseKey(hKey);

	if (lres != ERROR_SUCCESS)
		return FALSE;

	return TRUE;
}

/*
==============
SaveWindowState
==============
*/
BOOL SaveWindowState (HWND hWnd, const char *pszName)
{
	RECT rc;
	
	GetWindowRect(hWnd, &rc);
	if (hWnd != g_qeglobals.d_hwndMain)
		MapWindowPoints(NULL, g_qeglobals.d_hwndMain, (POINT *)&rc, 2);
	return SaveRegistryInfo(pszName, &rc, sizeof(rc));
}

/*
==============
LoadWindowState
==============
*/
BOOL LoadWindowState (HWND hWnd, const char *pszName)
{
	RECT rc;
	LONG lSize = sizeof(rc);

	if (LoadRegistryInfo(pszName, &rc, &lSize))
	{
		if (rc.left < 0)
			rc.left = 0;
		if (rc.top < 0)
			rc.top = 0;
		if (rc.right < rc.left + 16)
			rc.right = rc.left + 16;
		if (rc.bottom < rc.top + 16)
			rc.bottom = rc.top + 16;

		MoveWindow(hWnd, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, FALSE);
		return TRUE;
	}

	return FALSE;
}


/*
===============================================================

	STATUS WINDOW

===============================================================
*/

/*
==============
Sys_UpdateBrushStatusBar
==============
*/
void Sys_UpdateBrushStatusBar ()
{
	char		numbrushbuffer[128] = "";

	sprintf(numbrushbuffer, "Brushes: %d  Entities: %d  Textures: %d",
			g_map.numBrushes, g_map.numEntities, g_map.numTextures);
	Sys_Status(numbrushbuffer, 2);
}

/*
==============
Sys_UpdateGridStatusBar
==============
*/
void Sys_UpdateGridStatusBar ()
{
	char gridstatus[128] = "";

	sprintf(gridstatus, "Grid Size: %d  Grid Snap: %s  Tool: %s  Texture Lock: %s  Cubic Clip: %d", 
			g_qeglobals.d_nGridSize, 
			g_qeglobals.bGridSnap ? "On" : "OFF", 
			g_qeglobals.d_tools.back()->modal ? g_qeglobals.d_tools.back()->name : "None",
			g_qeglobals.d_bTextureLock ? "On" : "Off",
			(int)g_cfgEditor.CubicScale);	// sikk - Cubic Clipping
	Sys_Status(gridstatus, 1);
}

/*
==============
Sys_Status
==============
*/
void Sys_Status (const char *psz, int part )
{
	SendMessage(g_qeglobals.d_hwndStatus, SB_SETTEXT, part, (LPARAM)psz);
}

/*
==============
CreateStatusBar
==============
*/
HWND CreateStatusBar (HWND hWnd)
{
	HWND hwndSB;
	int partsize[4] = { 256, 720, 960, -1 };

	hwndSB = CreateStatusWindow(
		QE3_STATUSBAR_STYLE,	// window styles
		"Ready", 				// text for first pane 
		hWnd, 					// parent window
		ID_STATUSBAR);			// window ID
			

	SendMessage(hwndSB, SB_SETPARTS, 4, (long)partsize); // EER

	return hwndSB;
}

/*
===============================================================

	TOOLBAR

===============================================================
*/

/*
==============
CreateReBar
==============
*/
HWND CreateReBar (HWND hWnd, HINSTANCE hInst)
{
	HWND		hwndRB;
	REBARINFO	rbi;
	INITCOMMONCONTROLSEX icex;

	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC	= ICC_COOL_CLASSES | ICC_BAR_CLASSES;
	InitCommonControlsEx(&icex);

	hwndRB = CreateWindowEx(
		WS_EX_TOOLWINDOW,	// extended window style
		REBARCLASSNAME,		// registered class name
		NULL,				// window name
		QE3_REBAR_STYLE,	// window style
		0, 0, 0, 0,			// window position and size
		hWnd,				// parent or owner window
		(HMENU)ID_REBAR,	// menu or child-window identifier
		hInst,				// application instance
		NULL);				// window-creation data

	if(!hwndRB)
		return NULL;

	// Initialize and send the REBARINFO structure.
	rbi.cbSize	= sizeof(REBARINFO);	// Required when using this struct.
	rbi.fMask	= 0;
	rbi.himl	= (HIMAGELIST)NULL;
	if(!SendMessage(hwndRB, RB_SETBARINFO, 0, (LPARAM)&rbi))
		return NULL;

	return (hwndRB);
}

#define NUMBUTTONS 66

/*
==============
CreateToolBar
==============
*/
HWND CreateToolBar (HWND hWnd, HINSTANCE hInst, int nIndex, int nPos, int nButtons, int nSize)
{ 
	HWND			hwndTB; 
	//TBBUTTON		tbb[NUMBUTTONS];
	REBARBANDINFO	rbBand;
	DWORD			dwBtnSize;

	// Ensure that the common control DLL is loaded. 
	InitCommonControls(); 

	// Fill the TBBUTTON array with button information, 
	// and add the buttons to the toolbar.
	TBBUTTON tbbtns[] = {
		// bitmap, command, state, style, data, string
		{ 0, ID_FILE_NEW, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 1, ID_FILE_OPEN, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 2, ID_FILE_SAVE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 3, ID_FILE_SAVEAS, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0, 0 },

		{ 4, ID_FILE_PRINTXY, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 5, ID_HELP_ABOUT, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 6, ID_EDIT_CUT, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 7, ID_EDIT_COPY, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 8, ID_EDIT_PASTE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0, 0 },

		{ 9, ID_SELECTION_CLONE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 10, ID_SELECTION_INVERT, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 11, ID_SELECTION_DELETE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0, 0 },

		{ 12, ID_EDIT_UNDO, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 13, ID_EDIT_REDO, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0, 0 },

		{ 14, ID_EDIT_FINDBRUSH, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0, 0 },

		{ 15, ID_EDIT_PREFERENCES, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 16, ID_EDIT_MAPINFO, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 17, ID_EDIT_ENTITYINFO, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 18, ID_SELECTION_FLIPX, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 19, ID_SELECTION_ROTATEX, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 20, ID_SELECTION_FLIPY, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 21, ID_SELECTION_ROTATEY, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 22, ID_SELECTION_FLIPZ, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 23, ID_SELECTION_ROTATEZ, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 24, ID_SELECTION_SELECTCOMPLETETALL, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 25, ID_SELECTION_SELECTTOUCHING, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 26, ID_SELECTION_SELECTPARTIALTALL, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 27, ID_SELECTION_SELECTINSIDE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 28, ID_SELECTION_CSGHOLLOW, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 29, ID_SELECTION_CSGSUBTRACT, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 30, ID_SELECTION_CSGMERGE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 31, ID_SELECTION_CLIPPER, TBSTATE_ENABLED, TBSTYLE_CHECK, 0, 0 },
		{ 32, ID_SELECTION_DRAGEDGES, TBSTATE_ENABLED, TBSTYLE_CHECK, 0, 0 },
		{ 33, ID_SELECTION_DRAGVERTICES, TBSTATE_ENABLED, TBSTYLE_CHECK, 0, 0 },
		{ 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0, 0 },

		{ 34, ID_SELECTION_SCALELOCKX, TBSTATE_ENABLED, TBSTYLE_CHECK, 0, 0 },
		{ 35, ID_SELECTION_SCALELOCKY, TBSTATE_ENABLED, TBSTYLE_CHECK, 0, 0 },
		{ 36, ID_SELECTION_SCALELOCKZ, TBSTATE_ENABLED, TBSTYLE_CHECK, 0, 0 },
		{ 37, ID_SELECTION_UNGROUPENTITY, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 38, ID_SELECTION_CONNECT, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 39, ID_SELECTION_GROUPNEXTBRUSH, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 40, ID_BRUSH_CYLINDER, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 41, ID_BRUSH_CONE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 42, ID_BRUSH_SPHERE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 43, ID_TEXTURES_REPLACEALL, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 44, ID_TEXTURES_INSPECTOR, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 45, ID_VIEW_CONSOLE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 46, ID_VIEW_ENTITY, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 47, ID_VIEW_TEXTURE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0, 0 },

		{ 48, ID_VIEW_NEXTVIEW, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 49, ID_VIEW_CENTERONSELECTION, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0, 0 },

		{ 50, ID_TEXTURES_POPUP, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0, 0 },

		{ 51, ID_VIEW_CAMSPEED, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 52, ID_VIEW_CUBICCLIP, TBSTATE_ENABLED, TBSTYLE_CHECK, 0, 0 },
		{ 53, ID_VIEW_CUBICCLIPZOOMOUT, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 54, ID_VIEW_CUBICCLIPZOOMIN, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 55, ID_MISC_TESTMAP, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
		{ 56, ID_HELP_HELP, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 }
	};

	/*
	TBADDBITMAP tbab;
	// Create a toolbar that the user can customize and that has a 
	// tooltip associated with it. 
	hwndTB = CreateWindowEx(0, TOOLBARCLASSNAME, (LPSTR)NULL,
		WS_CHILD | TBSTYLE_TOOLTIPS | CCS_ADJUSTABLE | WS_BORDER,
		0, 0, 0, 0, hWnd, (HMENU)IDR_TOOLBAR2, hInst, NULL);

	// Send the TB_BUTTONSTRUCTSIZE message, which is required for 
	// backward compatibility. 
	SendMessage(hwndTB, TB_BUTTONSTRUCTSIZE, (WPARAM) sizeof(TBBUTTON), 0);

	// Add the bitmap containing button images to the toolbar. 
	tbab.hInst = hInst;
	tbab.nID = IDR_TOOLBAR2;
	SendMessage(hwndTB, TB_ADDBITMAP, (WPARAM)NUMBUTTONS, (WPARAM)&tbab);
	SendMessage(hwndTB, TB_ADDBUTTONS, (WPARAM)nButtons, (LPARAM)(LPTBBUTTON)&tbbtns[nPos]);
	SendMessage(hwndTB, TB_AUTOSIZE, 0, 0);

	ShowWindow(hwndTB, SW_SHOW);
	*/

	hwndTB = CreateToolbarEx(hWnd,	// Handle to the parent window 
		QE3_TOOLBAR_STYLE,			// Styles
		ID_TOOLBAR,					// Control identifier
		nButtons,					// Number of button images 
		(HINSTANCE)hInst,			// Module instance 
		IDR_TOOLBAR2,				// Resource identifier for the bitmap resource
		(LPCTBBUTTON)&tbbtns[nPos],	// Address of an array of TBBUTTON 
		nButtons,					// Number of buttons to add to toolbar
		0, 0,						// width & height of buttons
		0, 0,						// width & height of bitmaps
		sizeof(TBBUTTON));			// Size of a TBBUTTON structure

	// Get the height of the toolbar buttons.
	dwBtnSize = SendMessage(hwndTB, TB_GETBUTTONSIZE, 0, 0);

	// Initialize REBARBANDINFO for all coolbar bands.
	rbBand.cbSize = REBARBANDINFO_V3_SIZE;// sizeof(REBARBANDINFO);
	rbBand.fMask  = RBBIM_STYLE |		// fStyle is valid
					RBBIM_CHILD |		// hwndChild is valid
					RBBIM_CHILDSIZE |	// cxMinChild and cyMinChild are valid
					RBBIM_SIZE |		// cx is valid
					RBBIM_IDEALSIZE |	// cxIdeal is valid
					RBBIM_ID;			// wID is valid
	rbBand.fStyle		= RBBS_CHILDEDGE | RBBS_GRIPPERALWAYS;
	rbBand.hwndChild	= hwndTB;
	rbBand.cxMinChild	= nSize;
	rbBand.cyMinChild	= HIWORD(dwBtnSize);
	rbBand.cx			= nSize;
	rbBand.cxIdeal		= nSize;
	rbBand.wID			= ID_TOOLBAR + nIndex;

	// Insert band into rebar.
	if (!SendMessage(hWnd, RB_INSERTBAND, (WPARAM)nIndex, (LPARAM)(LPREBARBANDINFO)&rbBand))
	{
		return NULL;
	}
	return hwndTB; 
}

/*
==============
CreateTrackBar
==============
*//*
HWND CreateTrackBar (HWND hWnd, HINSTANCE hInst, int nIndex)
{ 
	HWND			hwndTrack;
	REBARINFO		rbi;
	REBARBANDINFO	rbBand;
	HIMAGELIST		hIml;
	HBITMAP			hBitmap, hMask;

	// Load bitmap for Trackbar Icon and add to ImageList
	hIml	= ImageList_Create(24, 16, ILC_COLORDDB | ILC_MASK, 1, 0);
	hBitmap	= LoadBitmap(hInst, MAKEINTRESOURCE(IDB_CAMSPEED));
	hMask	= LoadBitmap(hInst, MAKEINTRESOURCE(IDB_CAMSPEEDMASK));
	ImageList_Add(hIml, hBitmap, hMask);

	//set up the ReBar
	rbi.fMask	= RBIM_IMAGELIST;
	rbi.cbSize	= sizeof(rbi);
	rbi.himl	= hIml;
	SendMessage(g_qeglobals.d_hwndRebar, RB_SETBARINFO, 0, (LPARAM)&rbi);

	hwndTrack = CreateWindowEx(0,	// extended window style
		TRACKBAR_CLASS,				// registered class name
		"Trackbar Control",			// window name
		QE3_TRACKBAR_STYLE,			// window style
		0, 0,						// window position
		0, 0,						// window size
		hWnd,						// parent or owner window
		(HMENU)ID_TRACKBAR,			// menu or child-window identifier
		hInst,						// application instance
		NULL						// window-creation data
		); 

	// Sets the range of minimum and maximum logical positions for the slider in the trackbar
	// WPARAM: redraw flag	LPARAM: min & max positions
	SendMessage(hwndTrack, TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(0, 4096));
	// Sets the interval frequency for tick marks in a trackbar
	// WPARAM: frequency 
	SendMessage(hwndTrack, TBM_SETTICFREQ, (WPARAM)1024, (LPARAM)0);
	// Sets the number of logical positions the trackbar's slider moves with arrow keys
	// LPARAM: new line size
	SendMessage(hwndTrack, TBM_SETLINESIZE, (WPARAM)0, (LPARAM)64);
	// Sets the number of logical positions the trackbar's slider moves with PgUp/PgDn keys
	// LPARAM: new page size
	SendMessage(hwndTrack, TBM_SETPAGESIZE, (WPARAM)0, (LPARAM)256);
	//Sets the length of the slider in a trackbar.
	// WPARAM: Length 
	SendMessage(hwndTrack, TBM_SETTHUMBLENGTH, (WPARAM)12, (LPARAM)0);

	// Initialize REBARBANDINFO for rebar band.
	rbBand.cbSize		= sizeof(REBARBANDINFO);
	rbBand.fMask		= RBBIM_STYLE |		// fStyle is valid
						  RBBIM_IMAGE	|	// iImage is valid
						  RBBIM_CHILD |		// hwndChild is valid
						  RBBIM_CHILDSIZE |	// cxMinChild and cyMinChild are valid
						  RBBIM_SIZE |		// cx is valid
						  RBBIM_IDEALSIZE |	// cxIdeal is valid
						  RBBIM_ID;			// wID is valid
	rbBand.fStyle		= RBBS_CHILDEDGE | RBBS_GRIPPERALWAYS;
	rbBand.iImage		= 0;
	rbBand.hwndChild	= hwndTrack;
	rbBand.cxMinChild	= 46;
	rbBand.cyMinChild	= 22;
	rbBand.cx			= 48;
	rbBand.cxIdeal		= 48;
	rbBand.wID			= ID_TRACKBAR;

	// Insert band into rebar.
	SendMessage(g_qeglobals.d_hwndRebar, RB_INSERTBAND, (WPARAM)nIndex, (LPARAM)(LPREBARBANDINFO)&rbBand);

	return hwndTrack;
}*/






/*
=============================================================

MAIN WINDOW

=============================================================
*/

bool	g_bHaveQuit;


/*
==============
CommandHandler

handle all WM_COMMAND messages here
==============
*/
LONG WINAPI CommandHandler (
	HWND	hWnd,
	WPARAM	wParam,
	LPARAM	lParam
	)
{
	HMENU hMenu = GetMenu(hWnd);
	int	nBandIndex;

	switch (LOWORD(wParam))
	{
//===================================
// File menu
//===================================
		case ID_FILE_NEW:
			if (!ConfirmModified())
				return TRUE;
			g_map.New();
			break;
		case ID_FILE_OPEN:
			if (!ConfirmModified())
				return TRUE;
			OpenDialog();
			break;
		case ID_FILE_SAVE:
			if (!g_map.hasFilename)
				SaveAsDialog();
			else
				QE_SaveMap();
			if (g_qeglobals.d_nPointfileDisplayList)
				Pointfile_Clear();
			break;
		case ID_FILE_SAVEAS:
			SaveAsDialog();
			break;
		case ID_FILE_IMPORTMAP:	// sikk - Import Map Dialog
			ImportDialog();
			break;

		case ID_FILE_POINTFILE:
			if (g_qeglobals.d_nPointfileDisplayList)
				Pointfile_Clear();
			else
				Pointfile_Check();
			break;

		case IDMRU + 1:
		case IDMRU + 2:
		case IDMRU + 3:
		case IDMRU + 4:
		case IDMRU + 5:
		case IDMRU + 6:
		case IDMRU + 7:
		case IDMRU + 8:
		case IDMRU + 9:
			DoMru(hWnd, LOWORD(wParam));
			break;

		case ID_FILE_EXIT:	// exit application
			if (!ConfirmModified())
				return TRUE;
			PostMessage(hWnd, WM_CLOSE, 0, 0L);
			break;

//===================================
// Edit menu
//===================================
		case ID_EDIT_UNDO:
			g_cmdQueue.Undo();
			break;
		case ID_EDIT_REDO:
			g_cmdQueue.Redo();
			break;

		// lunaran cut/copy/paste need to work in entity edit fields and console
		case ID_EDIT_CUT:
			if (!g_qeglobals.d_wndEntity->TryCut())
				g_map.Cut();
			break;
		case ID_EDIT_COPY:
			if (g_qeglobals.d_wndEntity->TryCopy()) break;
			if (g_qeglobals.d_wndConsole->TryCopy()) break;
			g_map.Copy();
			break;
		case ID_EDIT_PASTE:
			if (!g_qeglobals.d_wndEntity->TryPaste())
				g_map.Paste();
			break;

		case ID_EDIT_FINDBRUSH:
			DoFindBrush();
			break;

		case ID_EDIT_MAPINFO:	// sikk - Map Info Dialog
			DoMapInfo();
			break;
		case ID_EDIT_ENTITYINFO:	// sikk - Entity Info Dialog
			DoEntityInfo();
			break;

		case ID_EDIT_PREFERENCES:	// sikk - Preferences Dialog
			//DoPreferences();
			DoConfigWindow();
			break;

//===================================
// View menu
//===================================
// sikk---> Toolbar/Statusbar Toggle
		case ID_VIEW_TOOLBAR_FILEBAND:
			nBandIndex = SendMessage(g_qeglobals.d_hwndRebar, RB_IDTOINDEX, (WPARAM)ID_TOOLBAR + 0, (LPARAM)0);
			if (IsWindowVisible(g_qeglobals.d_hwndToolbar[0]))
				SendMessage(g_qeglobals.d_hwndRebar, RB_SHOWBAND, (WPARAM)nBandIndex, (LPARAM)FALSE);
			else
				SendMessage(g_qeglobals.d_hwndRebar, RB_SHOWBAND, (WPARAM)nBandIndex, (LPARAM)TRUE);
			break;
		case ID_VIEW_TOOLBAR_EDITBAND:
			nBandIndex = SendMessage(g_qeglobals.d_hwndRebar, RB_IDTOINDEX, (WPARAM)ID_TOOLBAR + 1, (LPARAM)0);
			if (IsWindowVisible(g_qeglobals.d_hwndToolbar[1]))
				SendMessage(g_qeglobals.d_hwndRebar, RB_SHOWBAND, (WPARAM)nBandIndex, (LPARAM)FALSE);
			else
				SendMessage(g_qeglobals.d_hwndRebar, RB_SHOWBAND, (WPARAM)nBandIndex, (LPARAM)TRUE);
			break;
		case ID_VIEW_TOOLBAR_EDIT2BAND:
			nBandIndex = SendMessage(g_qeglobals.d_hwndRebar, RB_IDTOINDEX, (WPARAM)ID_TOOLBAR + 2, (LPARAM)0);
			if (IsWindowVisible(g_qeglobals.d_hwndToolbar[2]))
				SendMessage(g_qeglobals.d_hwndRebar, RB_SHOWBAND, (WPARAM)nBandIndex, (LPARAM)FALSE);
			else
				SendMessage(g_qeglobals.d_hwndRebar, RB_SHOWBAND, (WPARAM)nBandIndex, (LPARAM)TRUE);
			break;
		case ID_VIEW_TOOLBAR_SELECTBAND:
			nBandIndex = SendMessage(g_qeglobals.d_hwndRebar, RB_IDTOINDEX, (WPARAM)ID_TOOLBAR + 3, (LPARAM)0);
			if (IsWindowVisible(g_qeglobals.d_hwndToolbar[3]))
				SendMessage(g_qeglobals.d_hwndRebar, RB_SHOWBAND, (WPARAM)nBandIndex, (LPARAM)FALSE);
			else
				SendMessage(g_qeglobals.d_hwndRebar, RB_SHOWBAND, (WPARAM)nBandIndex, (LPARAM)TRUE);
			break;
		case ID_VIEW_TOOLBAR_CSGBAND:
			nBandIndex = SendMessage(g_qeglobals.d_hwndRebar, RB_IDTOINDEX, (WPARAM)ID_TOOLBAR + 4, (LPARAM)0);
			if (IsWindowVisible(g_qeglobals.d_hwndToolbar[4]))
				SendMessage(g_qeglobals.d_hwndRebar, RB_SHOWBAND, (WPARAM)nBandIndex, (LPARAM)FALSE);
			else
				SendMessage(g_qeglobals.d_hwndRebar, RB_SHOWBAND, (WPARAM)nBandIndex, (LPARAM)TRUE);
			break;
		case ID_VIEW_TOOLBAR_MODEBAND:
			nBandIndex = SendMessage(g_qeglobals.d_hwndRebar, RB_IDTOINDEX, (WPARAM)ID_TOOLBAR + 5, (LPARAM)0);
			if (IsWindowVisible(g_qeglobals.d_hwndToolbar[5]))
				SendMessage(g_qeglobals.d_hwndRebar, RB_SHOWBAND, (WPARAM)nBandIndex, (LPARAM)FALSE);
			else
				SendMessage(g_qeglobals.d_hwndRebar, RB_SHOWBAND, (WPARAM)nBandIndex, (LPARAM)TRUE);
			break;
		case ID_VIEW_TOOLBAR_ENTITYBAND:
			nBandIndex = SendMessage(g_qeglobals.d_hwndRebar, RB_IDTOINDEX, (WPARAM)ID_TOOLBAR + 6, (LPARAM)0);
			if (IsWindowVisible(g_qeglobals.d_hwndToolbar[6]))
				SendMessage(g_qeglobals.d_hwndRebar, RB_SHOWBAND, (WPARAM)nBandIndex, (LPARAM)FALSE);
			else
				SendMessage(g_qeglobals.d_hwndRebar, RB_SHOWBAND, (WPARAM)nBandIndex, (LPARAM)TRUE);
			break;
		case ID_VIEW_TOOLBAR_BRUSHBAND:
			nBandIndex = SendMessage(g_qeglobals.d_hwndRebar, RB_IDTOINDEX, (WPARAM)ID_TOOLBAR + 7, (LPARAM)0);
			if (IsWindowVisible(g_qeglobals.d_hwndToolbar[7]))
				SendMessage(g_qeglobals.d_hwndRebar, RB_SHOWBAND, (WPARAM)nBandIndex, (LPARAM)FALSE);
			else
				SendMessage(g_qeglobals.d_hwndRebar, RB_SHOWBAND, (WPARAM)nBandIndex, (LPARAM)TRUE);
			break;
		case ID_VIEW_TOOLBAR_TEXTUREBAND:
			nBandIndex = SendMessage(g_qeglobals.d_hwndRebar, RB_IDTOINDEX, (WPARAM)ID_TOOLBAR + 8, (LPARAM)0);
			if (IsWindowVisible(g_qeglobals.d_hwndToolbar[8]))
				SendMessage(g_qeglobals.d_hwndRebar, RB_SHOWBAND, (WPARAM)nBandIndex, (LPARAM)FALSE);
			else
				SendMessage(g_qeglobals.d_hwndRebar, RB_SHOWBAND, (WPARAM)nBandIndex, (LPARAM)TRUE);
			break;
		case ID_VIEW_TOOLBAR_VIEWBAND:
			nBandIndex = SendMessage(g_qeglobals.d_hwndRebar, RB_IDTOINDEX, (WPARAM)ID_TOOLBAR + 9, (LPARAM)0);
			if (IsWindowVisible(g_qeglobals.d_hwndToolbar[9]))
				SendMessage(g_qeglobals.d_hwndRebar, RB_SHOWBAND, (WPARAM)nBandIndex, (LPARAM)FALSE);
			else
				SendMessage(g_qeglobals.d_hwndRebar, RB_SHOWBAND, (WPARAM)nBandIndex, (LPARAM)TRUE);
			break;
		case ID_VIEW_TOOLBAR_MISCBAND:
			nBandIndex = SendMessage(g_qeglobals.d_hwndRebar, RB_IDTOINDEX, (WPARAM)ID_TOOLBAR + 10, (LPARAM)0);
			if (IsWindowVisible(g_qeglobals.d_hwndToolbar[10]))
				SendMessage(g_qeglobals.d_hwndRebar, RB_SHOWBAND, (WPARAM)nBandIndex, (LPARAM)FALSE);
			else
				SendMessage(g_qeglobals.d_hwndRebar, RB_SHOWBAND, (WPARAM)nBandIndex, (LPARAM)TRUE);
			break;

		case ID_VIEW_STATUSBAR:
			if (IsWindowVisible(g_qeglobals.d_hwndStatus))
				DestroyWindow(g_qeglobals.d_hwndStatus);
			else
			{
				g_qeglobals.d_hwndStatus = CreateStatusBar(g_qeglobals.d_hwndMain);
				Sys_UpdateGridStatusBar();
				Sys_UpdateBrushStatusBar();
			}
			break;
// <---sikk

		case ID_VIEW_SHOWAXIS:	// sikk - Show Axis
			g_cfgUI.ShowAxis ^= true;
			Sys_UpdateWindows(W_XY | W_CAMERA);
			break;
		case ID_VIEW_SHOWBLOCKS:
			g_cfgUI.ShowBlocks ^= true;
			Sys_UpdateWindows(W_XY);
			break;
		case ID_VIEW_SHOWCAMERAGRID:	// sikk - Camera Grid
			g_cfgUI.ShowCameraGrid ^= true;
			Sys_UpdateWindows(W_CAMERA);
			break;
		case ID_VIEW_SHOWCOORDINATES:
			g_cfgUI.ShowCoordinates ^= true;
			Sys_UpdateWindows(W_XY | W_Z);
			break;
		case ID_VIEW_SHOWLIGHTRADIUS:	// sikk - Show Light Radius
			g_cfgUI.ShowLightRadius ^= true;
			Sys_UpdateWindows(W_XY);
			break;
		case ID_VIEW_SHOWMAPBOUNDARY:	// sikk - Show Map Boundary Box
			g_cfgUI.ShowMapBoundary ^= true;
			Sys_UpdateWindows(W_CAMERA);
			break;
		case ID_VIEW_SHOWNAMES:
			g_cfgUI.ShowNames ^= true;
			Sys_UpdateWindows(W_XY);
			break;
		case ID_VIEW_SHOWSIZEINFO:
			g_cfgUI.ShowSizeInfo ^= true;
			Sys_UpdateWindows(W_XY);
			break;
		case ID_VIEW_SHOWWORKZONE:
			g_cfgUI.ShowWorkzone ^= true;
			Sys_UpdateWindows(W_XY);
			break;
		case ID_VIEW_SHOWANGLES:
			g_cfgUI.ShowAngles ^= true;
			Sys_UpdateWindows(W_XY | W_CAMERA);
			break;
		case ID_VIEW_SHOWPATH:
			g_cfgUI.ShowPaths ^= true;
			Sys_UpdateWindows(W_XY | W_CAMERA);
			break;

		case ID_VIEW_SHOWCLIP:
			g_cfgUI.ViewFilter ^= BFL_CLIP;
			Sys_UpdateWindows(W_XY | W_CAMERA);
			break;
		case ID_VIEW_SHOWPOINTENTS:
			g_cfgUI.ViewFilter ^= EFL_POINTENTITY;
			Sys_UpdateWindows(W_XY | W_CAMERA);
			break;
		case ID_VIEW_SHOWBRUSHENTS:
			g_cfgUI.ViewFilter ^= EFL_BRUSHENTITY;
			Sys_UpdateWindows(W_XY | W_CAMERA);
			break;
		case ID_VIEW_SHOWFUNCWALL:
			g_cfgUI.ViewFilter ^= EFL_FUNCWALL;
			Sys_UpdateWindows(W_XY | W_CAMERA);
			break;
		case ID_VIEW_SHOWLIGHTS:
			g_cfgUI.ViewFilter ^= EFL_LIGHT;
			Sys_UpdateWindows(W_XY | W_CAMERA);
			break;
		case ID_VIEW_SHOWSKY:
			g_cfgUI.ViewFilter ^= BFL_SKY;
			Sys_UpdateWindows(W_XY | W_CAMERA);
			break;
		case ID_VIEW_SHOWWATER:
			g_cfgUI.ViewFilter ^= BFL_LIQUID;
			Sys_UpdateWindows(W_XY | W_CAMERA);
			break;
		case ID_VIEW_SHOWWORLD:
			g_cfgUI.ViewFilter ^= EFL_WORLDSPAWN;
			Sys_UpdateWindows(W_XY | W_CAMERA);
			break;
		case ID_VIEW_SHOWHINT:
			g_cfgUI.ViewFilter ^= BFL_HINT;
			Sys_UpdateWindows(W_XY | W_CAMERA);
			break;
		case ID_VIEW_SHOWDETAIL:
			g_cfgUI.ViewFilter ^= EFL_DETAIL;
			Sys_UpdateWindows(W_XY | W_CAMERA);
			break;
		case ID_VIEW_SHOWMONSTERS:
			g_cfgUI.ViewFilter ^= EFL_MONSTER;
			Sys_UpdateWindows(W_XY | W_CAMERA);
			break;
		case ID_VIEW_SHOWTRIGGERS:
			g_cfgUI.ViewFilter ^= EFL_TRIGGER;
			Sys_UpdateWindows(W_XY | W_CAMERA);
			break;

		case ID_FILTER_SHOWALLSKILLS:
			g_cfgUI.ViewFilter -= g_cfgUI.ViewFilter & (EFL_EASY | EFL_MEDIUM | EFL_HARD | EFL_DEATHMATCH);
			Sys_UpdateWindows(W_SCENE|W_ENTITY);
			break;
		case ID_FILTER_SHOWEASYSKILL:
			g_cfgUI.ViewFilter -= g_cfgUI.ViewFilter & (EFL_EASY | EFL_MEDIUM | EFL_HARD | EFL_DEATHMATCH);
			g_cfgUI.ViewFilter |= EFL_EASY;
			Sys_UpdateWindows(W_SCENE | W_ENTITY);
			break;
		case ID_FILTER_SHOWMEDIUMSKILL:
			g_cfgUI.ViewFilter -= g_cfgUI.ViewFilter & (EFL_EASY | EFL_MEDIUM | EFL_HARD | EFL_DEATHMATCH);
			g_cfgUI.ViewFilter |= EFL_MEDIUM;
			Sys_UpdateWindows(W_SCENE | W_ENTITY);
			break;
		case ID_FILTER_SHOWHARDSKILL:
			g_cfgUI.ViewFilter -= g_cfgUI.ViewFilter & (EFL_EASY | EFL_MEDIUM | EFL_HARD | EFL_DEATHMATCH);
			g_cfgUI.ViewFilter |= EFL_HARD;
			Sys_UpdateWindows(W_SCENE | W_ENTITY);
			break;
		case ID_FILTER_SHOWDEATHMATCH:
			g_cfgUI.ViewFilter -= g_cfgUI.ViewFilter & (EFL_EASY | EFL_MEDIUM | EFL_HARD | EFL_DEATHMATCH);
			g_cfgUI.ViewFilter |= EFL_DEATHMATCH;
			Sys_UpdateWindows(W_SCENE | W_ENTITY);
			break;


		case ID_VIEW_CAMERA:	// sikk - Toggle Camera View
			g_qeglobals.d_wndCamera->Toggle();
			break;
		
		case ID_VIEW_ENTITY:
			QE_SetInspectorMode(W_ENTITY);
			break;
		case ID_VIEW_CONSOLE:
			QE_SetInspectorMode(W_CONSOLE);
			break;
		case ID_VIEW_TEXTURE:
			QE_SetInspectorMode(W_TEXTURE);
			break;

		case ID_VIEW_TOGGLE_XY:
			g_qeglobals.d_wndGrid[0]->Toggle();
			break;
		case ID_VIEW_TOGGLE_XZ:
			g_qeglobals.d_wndGrid[2]->Toggle();
			break;
		case ID_VIEW_TOGGLE_YZ:
			g_qeglobals.d_wndGrid[1]->Toggle();
			break;
		case ID_VIEW_TOGGLE_Z:
			g_qeglobals.d_wndZ->Toggle();
			break;

		case ID_VIEW_CENTER:
			g_qeglobals.d_vCamera.LevelView();
			break;
		case ID_VIEW_UPFLOOR:
			g_qeglobals.d_vCamera.ChangeFloor(true);
			break;
		case ID_VIEW_DOWNFLOOR:
			g_qeglobals.d_vCamera.ChangeFloor(false);
			break;

		case ID_VIEW_CENTERONSELECTION:	// sikk - Center Views on Selection
			for (int i = 0; i < 4; i++)
				g_qeglobals.d_vXYZ[i].PositionView();
			g_qeglobals.d_vCamera.PositionCenter();
			Sys_UpdateWindows(W_XY|W_CAMERA);
			break;

		case ID_VIEW_NEXTVIEW:
			g_qeglobals.d_wndGrid[0]->CycleAxis();
			break;
		case ID_VIEW_XY:
			g_qeglobals.d_wndGrid[0]->SetAxis(XY);
			break;
		case ID_VIEW_XZ:
			g_qeglobals.d_wndGrid[0]->SetAxis(XZ);
			break;
		case ID_VIEW_YZ:
			g_qeglobals.d_wndGrid[0]->SetAxis(YZ);
			break;
		case ID_VIEW_SWAPGRIDCAM:
			QE_SwapGridCam();
			Sys_UpdateWindows(W_XY | W_CAMERA);
			break;

		case ID_VIEW_100:
			g_qeglobals.d_vXYZ[0].scale = 1;
			Sys_UpdateWindows(W_XY);
			break;
		case ID_VIEW_ZOOMIN:
			g_qeglobals.d_vXYZ[0].scale *= 5.0 / 4;
			if (g_qeglobals.d_vXYZ[0].scale > 32)
				g_qeglobals.d_vXYZ[0].scale = 32;
			Sys_UpdateWindows(W_XY);
			break;
		case ID_VIEW_ZOOMOUT:
			g_qeglobals.d_vXYZ[0].scale *= 4.0f / 5;
			if (g_qeglobals.d_vXYZ[0].scale < 0.05)
				g_qeglobals.d_vXYZ[0].scale = 0.05f;
			Sys_UpdateWindows(W_XY);
			break;

		case ID_VIEW_Z100:
			g_qeglobals.d_vZ.scale = 1;
			Sys_UpdateWindows(W_Z);
			break;
		case ID_VIEW_ZZOOMIN:
			g_qeglobals.d_vZ.scale *= 5.0 / 4;
			if (g_qeglobals.d_vZ.scale > 16)
				g_qeglobals.d_vZ.scale = 16;
			Sys_UpdateWindows(W_Z);
			break;
		case ID_VIEW_ZZOOMOUT:
			g_qeglobals.d_vZ.scale *= 4.0f / 5;
			if (g_qeglobals.d_vZ.scale < 0.1)
				g_qeglobals.d_vZ.scale = 0.1f;
			Sys_UpdateWindows(W_Z);
			break;

		case ID_VIEW_CAMSPEED:	// sikk - Camera Speed Trackbar in toolbar
			DoCamSpeed();
			break;

// sikk---> Cubic Clipping
		case ID_VIEW_CUBICCLIP:
			g_cfgEditor.CubicClip ^= true;
			Sys_UpdateWindows(W_XY | W_CAMERA);
			break;
		case ID_VIEW_CUBICCLIPZOOMIN:
			g_cfgEditor.CubicScale = max((int)g_cfgEditor.CubicScale - 1, 1);
			Sys_UpdateGridStatusBar();
			Sys_UpdateWindows(W_CAMERA);
			break;

		case ID_VIEW_CUBICCLIPZOOMOUT:
			g_cfgEditor.CubicScale = min((int)g_cfgEditor.CubicScale + 1, 64);
			Sys_UpdateGridStatusBar();
			Sys_UpdateWindows(W_CAMERA);
			break;
// <---sikk



		case ID_VIEW_HIDESHOW_HIDESELECTED:
			Modify::HideSelected();
			Selection::DeselectAll();
			break;
		case ID_VIEW_HIDESHOW_HIDEUNSELECTED:
			Modify::HideUnselected();
			Selection::DeselectAll();
			break;
		case ID_VIEW_HIDESHOW_SHOWHIDDEN:
			Modify::ShowHidden();
			break;

//===================================
// Selection menu
//===================================
		case ID_SELECTION_DRAGEDGES:
			GeoTool::ToggleMode(GeoTool::GT_EDGE);
			Sys_UpdateWindows(W_XY | W_CAMERA);
			break;
		case ID_SELECTION_DRAGVERTICES:
			GeoTool::ToggleMode(GeoTool::GT_VERTEX);
			Sys_UpdateWindows(W_XY | W_CAMERA);
			break;
		case ID_SELECTION_DRAGFACES:
			GeoTool::ToggleMode(GeoTool::GT_FACE);
			Sys_UpdateWindows(W_XY | W_CAMERA);
			break;
			
	//	case ID_SELECTION_CLONE:
	//		Modify::Clone();
	//		break;
		case ID_SELECTION_DESELECT:
			if (!Tool::HotTool())
			{
				Tool *mt = Tool::ModalTool();
				if (mt)
				{
					delete mt;
					Sys_UpdateWindows(W_SCENE);
				}
				else
					Selection::DeselectAll();
			}
			break;
		case ID_SELECTION_DELETE:
			Modify::Delete();
			break;

		case ID_SELECTION_FLIPX:
			Transform_FlipAxis(0);
			break;
		case ID_SELECTION_FLIPY:
			Transform_FlipAxis(1);
			break;
		case ID_SELECTION_FLIPZ:
			Transform_FlipAxis(2);
			break;

		case ID_SELECTION_ROTATEX:
			Transform_RotateAxis(0, RotateAngleForModifiers(), false);
			break;
		case ID_SELECTION_ROTATEY:
			Transform_RotateAxis(1, RotateAngleForModifiers(), false);
			break;
		case ID_SELECTION_ROTATEZ:
			Transform_RotateAxis(2, RotateAngleForModifiers(), false);
			break;
		case ID_SELECTION_ARBITRARYROTATION:
			DoRotate();
			break;

		case ID_SELECTION_SCALELOCKX:	// sikk - Brush Scaling Axis Lock
			g_qeglobals.d_savedinfo.bScaleLockX ^= true;
			Sys_UpdateWindows(W_XY | W_Z | W_CAMERA);
			break;
		case ID_SELECTION_SCALELOCKY:	// sikk - Brush Scaling Axis Lock
			g_qeglobals.d_savedinfo.bScaleLockY ^= true;
			Sys_UpdateWindows(W_XY | W_Z | W_CAMERA);
			break;
		case ID_SELECTION_SCALELOCKZ:	// sikk - Brush Scaling Axis Lock
			g_qeglobals.d_savedinfo.bScaleLockZ ^= true;
			Sys_UpdateWindows(W_XY | W_Z | W_CAMERA);
			break;
		case ID_SELECTION_SCALE:	// sikk - Brush Scaling Dialog
			DoScale();
			break;

		case ID_SELECTION_CSGHOLLOW:
			CSG::Hollow();
			break;
		case ID_SELECTION_CSGSUBTRACT:
			CSG::Subtract();
			break;
		case ID_SELECTION_CSGMERGE:
			CSG::Merge();
			break;

		case ID_TOOLS_SETENTITYKEYS:
			DoSetKeyValues();
			break;

		case ID_SELECTION_CLIPPER:
			if (g_qeglobals.d_selSelectMode != sel_brush)
			{
				Selection::DeselectAllFaces();
				g_qeglobals.d_selSelectMode = sel_brush;
				Sys_UpdateWindows(W_XY | W_CAMERA);
			}
			if (dynamic_cast<ClipTool*>(g_qeglobals.d_tools.back()))
				delete g_qeglobals.d_tools.back();
			else
				new ClipTool();
			break;

		case ID_SELECTION_CONNECT:
			Modify::ConnectEntities();
			break;
		case ID_SELECTION_UNGROUPENTITY:
			Modify::Ungroup();
			break;
		case ID_SELECTION_GROUPNEXTBRUSH:
			Selection::NextBrushInGroup();
			break;
		case ID_SELECTION_INSERTBRUSH:	// sikk - Insert Brush into Entity
			Modify::InsertBrush();
			break;

		case ID_SELECTION_EXPORTMAP:	// sikk - Export Selection Dialog
			ExportDialog();
			break;

//===================================
// BSP menu
//===================================
		case CMD_BSPCOMMAND:
		case CMD_BSPCOMMAND + 1:
		case CMD_BSPCOMMAND + 2:
		case CMD_BSPCOMMAND + 3:
		case CMD_BSPCOMMAND + 4:
		case CMD_BSPCOMMAND + 5:
		case CMD_BSPCOMMAND + 6:
		case CMD_BSPCOMMAND + 7:
		case CMD_BSPCOMMAND + 8:
		case CMD_BSPCOMMAND + 9:
		case CMD_BSPCOMMAND + 10:
		case CMD_BSPCOMMAND + 11:
		case CMD_BSPCOMMAND + 12:
		case CMD_BSPCOMMAND + 13:
		case CMD_BSPCOMMAND + 14:
		case CMD_BSPCOMMAND + 15:
		case CMD_BSPCOMMAND + 16:
		case CMD_BSPCOMMAND + 17:
		case CMD_BSPCOMMAND + 18:
		case CMD_BSPCOMMAND + 19:
		case CMD_BSPCOMMAND + 20:
		case CMD_BSPCOMMAND + 21:
		case CMD_BSPCOMMAND + 22:
		case CMD_BSPCOMMAND + 23:
		case CMD_BSPCOMMAND + 24:
		case CMD_BSPCOMMAND + 25:
		case CMD_BSPCOMMAND + 26:
		case CMD_BSPCOMMAND + 27:
		case CMD_BSPCOMMAND + 28:
		case CMD_BSPCOMMAND + 29:
		case CMD_BSPCOMMAND + 30:
		case CMD_BSPCOMMAND + 31:
			RunBsp(g_szBSP_Commands[LOWORD(wParam - CMD_BSPCOMMAND)]);
			break;

//===================================
// grid menu
//===================================
		case ID_GRID_1:
		case ID_GRID_2:
		case ID_GRID_4:
		case ID_GRID_8:
		case ID_GRID_16:
		case ID_GRID_32:
		case ID_GRID_64:
		case ID_GRID_128:
		case ID_GRID_256:
			CheckMenuItem(hMenu, ID_GRID_1, MF_UNCHECKED);
			CheckMenuItem(hMenu, ID_GRID_2, MF_UNCHECKED);
			CheckMenuItem(hMenu, ID_GRID_4, MF_UNCHECKED);
			CheckMenuItem(hMenu, ID_GRID_8, MF_UNCHECKED);
			CheckMenuItem(hMenu, ID_GRID_16, MF_UNCHECKED);
			CheckMenuItem(hMenu, ID_GRID_32, MF_UNCHECKED);
			CheckMenuItem(hMenu, ID_GRID_64, MF_UNCHECKED);
			CheckMenuItem(hMenu, ID_GRID_128, MF_UNCHECKED);
			CheckMenuItem(hMenu, ID_GRID_256, MF_UNCHECKED);

			switch (LOWORD(wParam))
			{
				case ID_GRID_1:		g_qeglobals.d_nGridSize = 1;	break;
				case ID_GRID_2:		g_qeglobals.d_nGridSize = 2;	break;
				case ID_GRID_4:		g_qeglobals.d_nGridSize = 4;	break;
				case ID_GRID_8:		g_qeglobals.d_nGridSize = 8;	break;
				case ID_GRID_16:	g_qeglobals.d_nGridSize = 16;	break;
				case ID_GRID_32:	g_qeglobals.d_nGridSize = 32;	break;
				case ID_GRID_64:	g_qeglobals.d_nGridSize = 64;	break;
				case ID_GRID_128:	g_qeglobals.d_nGridSize = 128;	break;
				case ID_GRID_256:	g_qeglobals.d_nGridSize = 256;	break;
			}

			CheckMenuItem(hMenu, LOWORD(wParam), MF_CHECKED);
			Sys_UpdateWindows(W_XY | W_Z);
			Sys_UpdateGridStatusBar();
			break;

		case ID_GRID_TOGGLE:	// sikk - Gridd Toggle
			g_qeglobals.d_bShowGrid ^= true;
			//PostMessage(g_qeglobals.d_hwndXYZ[0], WM_PAINT, 0, 0);
			Sys_UpdateWindows(W_Z | W_XY);
			break;

		case ID_GRID_SNAPTOGRID:
			g_qeglobals.bGridSnap ^= true;
			Sys_UpdateGridStatusBar();
			break;

//===================================
// Texture menu
//===================================
		case ID_TEXTURES_UPDATEMENU:
			//Sys_BeginWait();
			FillTextureMenu();
			QE_SetInspectorMode(W_CONSOLE);
			break;

		case ID_TEXTURES_FLUSH_ALL:
			//Sys_BeginWait();
			Textures::Flush();
			QE_SetInspectorMode(W_TEXTURE);
			Sys_UpdateWindows(W_TEXTURE);
			break;
		case ID_TEXTURES_FLUSH_UNUSED:
			//Sys_BeginWait();
			Textures::FlushUnused();
			QE_SetInspectorMode(W_TEXTURE);
			Sys_UpdateWindows(W_TEXTURE);
			break;
		case ID_TEXTURES_SHOWINUSE:
			Textures::RefreshUsedStatus();
			QE_SetInspectorMode(W_TEXTURE);
			break;

		case ID_TEXTURES_RESETSCALE:	// sikk - Reset Texture View Scale
			g_qeglobals.d_vTexture.SetScale(1.0f);
			Sys_UpdateWindows(W_TEXTURE);
			break;

		/*
		case ID_TEXTURES_REPLACEALL:
			TextureTool::FindTextureDialog();
			//DoFindTexture();
			break;*/
		case ID_TEXTURES_LOCK:
			g_qeglobals.d_bTextureLock ^= true;
			Sys_UpdateWindows(W_CAMERA);
			Sys_UpdateGridStatusBar();
			break;
		case ID_TEXTURES_DEFAULTSCALE:	// sikk - Default Texture Scale Dialog
			DoDefaultTexScale();
			break;

		case ID_TEXTURES_INSPECTOR:
			WndSurf_Create();
			break;

		case ID_TEXTURES_POPUP:	// sikk - Toolbar Button
			DoTexturesPopup();
			break;

		case ID_DRAWMODE_WIREFRAME:
			g_cfgUI.DrawMode = cd_wire;
			Textures::SetDrawMode(cd_wire);
			g_map.BuildBrushData();
			Sys_UpdateWindows(W_CAMERA);
			break;
		case ID_DRAWMODE_FLATSHADE:
			g_cfgUI.DrawMode = cd_solid;
			Textures::SetDrawMode(cd_solid);
			g_map.BuildBrushData();
			Sys_UpdateWindows(W_CAMERA);
			break;
		case ID_DRAWMODE_TEXTURED:
			g_cfgUI.DrawMode = cd_texture;
			Textures::SetDrawMode(cd_texture);
			g_map.BuildBrushData();
			Sys_UpdateWindows(W_CAMERA);
			break;

		case CMD_TEXTUREWAD:
		case CMD_TEXTUREWAD + 1:
		case CMD_TEXTUREWAD + 2:
		case CMD_TEXTUREWAD + 3:
		case CMD_TEXTUREWAD + 4:
		case CMD_TEXTUREWAD + 5:
		case CMD_TEXTUREWAD + 6:
		case CMD_TEXTUREWAD + 7:
		case CMD_TEXTUREWAD + 8:
		case CMD_TEXTUREWAD + 9:
		case CMD_TEXTUREWAD + 10:
		case CMD_TEXTUREWAD + 11:
		case CMD_TEXTUREWAD + 12:
		case CMD_TEXTUREWAD + 13:
		case CMD_TEXTUREWAD + 14:
		case CMD_TEXTUREWAD + 15:
		case CMD_TEXTUREWAD + 16:
		case CMD_TEXTUREWAD + 17:
		case CMD_TEXTUREWAD + 18:
		case CMD_TEXTUREWAD + 19:
		case CMD_TEXTUREWAD + 20:
		case CMD_TEXTUREWAD + 21:
		case CMD_TEXTUREWAD + 22:
		case CMD_TEXTUREWAD + 23:
		case CMD_TEXTUREWAD + 24:
		case CMD_TEXTUREWAD + 25:
		case CMD_TEXTUREWAD + 26:
		case CMD_TEXTUREWAD + 27:
		case CMD_TEXTUREWAD + 28:
		case CMD_TEXTUREWAD + 29:
		case CMD_TEXTUREWAD + 30:
		case CMD_TEXTUREWAD + 31:
		case CMD_TEXTUREWAD + 32:
		case CMD_TEXTUREWAD + 33:
		case CMD_TEXTUREWAD + 34:
		case CMD_TEXTUREWAD + 35:
		case CMD_TEXTUREWAD + 36:
		case CMD_TEXTUREWAD + 37:
		case CMD_TEXTUREWAD + 38:
		case CMD_TEXTUREWAD + 39:
		case CMD_TEXTUREWAD + 40:
		case CMD_TEXTUREWAD + 41:
		case CMD_TEXTUREWAD + 42:
		case CMD_TEXTUREWAD + 43:
		case CMD_TEXTUREWAD + 44:
		case CMD_TEXTUREWAD + 45:
		case CMD_TEXTUREWAD + 46:
		case CMD_TEXTUREWAD + 47:
		case CMD_TEXTUREWAD + 48:
		case CMD_TEXTUREWAD + 49:
		case CMD_TEXTUREWAD + 50:
		case CMD_TEXTUREWAD + 51:
		case CMD_TEXTUREWAD + 52:
		case CMD_TEXTUREWAD + 53:
		case CMD_TEXTUREWAD + 54:
		case CMD_TEXTUREWAD + 55:
		case CMD_TEXTUREWAD + 56:
		case CMD_TEXTUREWAD + 57:
		case CMD_TEXTUREWAD + 58:
		case CMD_TEXTUREWAD + 59:
		case CMD_TEXTUREWAD + 60:
		case CMD_TEXTUREWAD + 61:
		case CMD_TEXTUREWAD + 62:
		case CMD_TEXTUREWAD + 63:
			Sys_BeginWait();
			Textures::MenuLoadWad(LOWORD(wParam));
			QE_SetInspectorMode(W_TEXTURE);
			break;

//===================================
// Misc menu
//===================================
		case ID_MISC_BENCHMARK:
			SendMessage(g_qeglobals.d_hwndCamera, WM_BENCHMARK, 0, 0);
			QE_SetInspectorMode(W_CONSOLE);
			break;

		case ID_MISC_NEXTLEAKSPOT:
			Pointfile_Next();
			break;
		case ID_MISC_PREVIOUSLEAKSPOT:
			Pointfile_Prev();
			break;

		case ID_MISC_SELECTENTITYCOLOR:
			g_qeglobals.d_wndEntity->SelectEntityColor();
			break;

		case ID_MISC_TESTMAP:
			//DoTestMap();
			break;

//===================================
// Region menu
//===================================
		case ID_REGION_OFF:
			g_map.RegionOff();
			break;

		case ID_REGION_SETXY:
			g_map.RegionXY();
			break;
// sikk---> Multiple Orthographic Views
		case ID_REGION_SETXZ:
			g_map.RegionXZ();
			break;
		case ID_REGION_SETYZ:
			g_map.RegionYZ();
			break;
// <---sikk
		case ID_REGION_SETTALLBRUSH:
			g_map.RegionTallBrush();
			break;
		case ID_REGION_SETBRUSH:
			g_map.RegionBrush();
			break;
		case ID_REGION_SETSELECTION:
			g_map.RegionSelectedBrushes();
			break;

//===================================
// Brush menu
//===================================

// sikk---> Brush Primitives
		case ID_BRUSH_CYLINDER:
			DoSides(0);
			break;
		case ID_BRUSH_CONE:
			DoSides(1);
			break;
		case ID_BRUSH_SPHERE:
			DoSides(2);
			break;
// <---sikk
		case ID_PRIMITIVES_CZGCYLINDER1:
			Modify::MakeCzgCylinder(1);
			break;
		case ID_PRIMITIVES_CZGCYLINDER2:
			Modify::MakeCzgCylinder(2);
			break;


//===================================
// Window menu
//===================================
		case ID_WINDOW_QE3DEFAULT:
			DoWindowPosition(0);
			break;
		case ID_WINDOW_QE3REVERSE:
			DoWindowPosition(1);
			break;
		case ID_WINDOW_4WINDOWZCAMLEFT:
			DoWindowPosition(2);
			break;
		case ID_WINDOW_4WINDOWZCAMRIGHT:
			DoWindowPosition(3);
			break;
		case ID_WINDOW_4WINDOWNOZCAMLEFT:
			DoWindowPosition(4);
			break;
		case ID_WINDOW_4WINDOWNOZCAMRIGHT:
			DoWindowPosition(5);
			break;
		case ID_WINDOW_4REVERSEZCAMLEFT:
			DoWindowPosition(6);
			break;
		case ID_WINDOW_4REVERSEZCAMRIGHT:
			DoWindowPosition(7);
			break;
		case ID_WINDOW_4REVERSENOZCAMLEFT:
			DoWindowPosition(8);
			break;
		case ID_WINDOW_4REVERSENOZCAMRIGHT:
			DoWindowPosition(9);
			break;

//===================================
// help menu
//===================================
		case ID_HELP_HELP:
			// sikk - TODO: Compile a manual for QE3 and change link
			ShellExecute(NULL, "open", "docs\\QuakeEd3.chm", NULL, NULL, 1);
			break;

		case ID_HELP_KEYLIST:
			DoKeylist();
			break;
		case ID_HELP_MOUSELIST:
			DoMouselist();
			break;

		case ID_HELP_ABOUT:
			DoAbout();
			break;

		default:
			return FALSE;
	}

	// sikk - Update Menu & Toolbar so state chages take effect
	QE_UpdateCommandUI();

	return TRUE;
}

void WMain_CreateViews()
{
	Sys_Printf("Creating windows\n");
	g_qeglobals.d_wndConsole = new WndConsole();
	g_qeglobals.d_wndConsole->Initialize();

	g_qeglobals.d_wndCamera = new WndCamera();
	g_qeglobals.d_wndCamera->Initialize();
	for (int i = 0; i < 3; i++)
	{
		g_qeglobals.d_wndGrid[i] = new WndGrid();
		g_qeglobals.d_wndGrid[i]->Initialize(i);
	}
	g_qeglobals.d_wndZ = new WndZChecker();
	g_qeglobals.d_wndZ->Initialize();

	g_qeglobals.d_wndTexture = new WndTexture();
	g_qeglobals.d_wndTexture->Initialize();
	g_qeglobals.d_wndEntity = new WndEntity();
	g_qeglobals.d_wndEntity->Initialize();

	/*
	// this could happen just fine in WndView::OnCreate, but delaying wglShareLists fixes
	// troubles in some contexts (namely the GLX emulation in WINE)
	// TODO: write new fantasy renderer that doesn't rely on crufty old stuff like wglShareLists
	g_qeglobals.d_wndCamera->ShareLists();
	for (int i = 0; i < 3; i++)
		g_qeglobals.d_wndGrid[i]->ShareLists();
	g_qeglobals.d_wndZ->ShareLists();
	g_qeglobals.d_wndTexture->ShareLists();
	*/
	QE_ForceInspectorMode(W_CONSOLE);
}

/*
============
WMain_HandleMWheel
============
*/
void WMain_HandleMWheel(MSG &msg)
{
	if (msg.message != WM_MOUSEWHEEL) return;

	POINT	point;

	point.x = (short)LOWORD(msg.lParam);
	point.y = (short)HIWORD(msg.lParam);
	msg.hwnd = ChildWindowFromPoint(g_qeglobals.d_hwndMain, point);
}

/*
============
WMain_WndProc
============
*/
LONG WINAPI WMain_WndProc (
	HWND	hWnd,	// handle to window
	UINT	uMsg,	// message
	WPARAM	wParam,	// first message parameter
	LPARAM	lParam 	// second message parameter
	)
{
	RECT	rect;
	LPTOOLTIPTEXT	lpToolTipText;
	char	szToolTip[128];
    time_t	lTime;
	int		i, nBandIndex;	// sikk - Save Rebar Band Info

	GetClientRect(hWnd, &rect);

	switch (uMsg)
	{
	case WM_TIMER:	// 1/sec
		QE_CheckAutoSave();
		return 0;

	case WM_DESTROY:
		SaveMruInReg(g_qeglobals.d_lpMruMenu, QE3_WIN_REGISTRY_MRU);
		DeleteMruMenu(g_qeglobals.d_lpMruMenu);
		g_qeconfig.Save();
		PostQuitMessage(0);
		KillTimer(hWnd, QE_TIMER0);
		return 0;

	case WM_CREATE:
		g_qeglobals.d_lpMruMenu = CreateMruMenuDefault();
		LoadMruInReg(g_qeglobals.d_lpMruMenu, QE3_WIN_REGISTRY_MRU);

		// Refresh the File menu.
		PlaceMenuMRUItem(g_qeglobals.d_lpMruMenu, GetSubMenu(GetMenu(hWnd), 0), ID_FILE_EXIT);
		return 0;

	case WM_SIZE:
		// resize the status window
		MoveWindow(g_qeglobals.d_hwndStatus, -100, 100, 10, 10, true);
		return 0;

	case WM_CLOSE:
		// call destroy window to cleanup and go away
		if (!ConfirmModified())
			return TRUE;
		for (auto wvIt = WndView::wndviews.begin(); wvIt != WndView::wndviews.end(); ++wvIt)
			(*wvIt)->SavePosition();

		SaveWindowState(g_qeglobals.d_hwndMain, "MainWindow");

		// sikk---> Save Rebar Band Info
		for (i = 0; i < 11; i++)
		{
			nBandIndex = SendMessage(g_qeglobals.d_hwndRebar, RB_IDTOINDEX, (WPARAM)ID_TOOLBAR + i, (LPARAM)0);
			//Sys_Printf("Band %d\n", nBandIndex);
			g_qeglobals.d_savedinfo.rbiSettings[i].cbSize = sizeof(REBARBANDINFO);
			g_qeglobals.d_savedinfo.rbiSettings[i].fMask = RBBIM_CHILDSIZE | RBBIM_STYLE;
			SendMessage(g_qeglobals.d_hwndRebar, RB_GETBANDINFO, (WPARAM)nBandIndex, (LPARAM)(LPREBARBANDINFO)&g_qeglobals.d_savedinfo.rbiSettings[i]);
		}
		/*	Code below saves the current Band order but there is a problem with
			updating this at program start. (check QE_Init() in qe3.c)
				j = 0;
				while (j < 11)
				{
					for (i = 0; i < 11; i++)
					{
						nBandIndex = SendMessage(g_qeglobals.d_hwndRebar, RB_IDTOINDEX, (WPARAM)ID_TOOLBAR + i, (LPARAM)0);
						if (nBandIndex == j)
							g_qeglobals.d_savedinfo.nRebarSavedIndex[j] = ID_TOOLBAR + i;
					}
					j++;
				}
		*/
		// <---sikk

		// FIXME: is this right?
		SaveRegistryInfo("SavedInfo", &g_qeglobals.d_savedinfo, sizeof(g_qeglobals.d_savedinfo));

		time(&lTime);	// sikk - Print current time for logging purposes
		Sys_Printf("\nSession Stopped: %s", ctime(&lTime));
		
		// TODO:
		// at program quit, the static slab lists are emptied before the global command queue
		// object is destroyed, so commands can't clean up any of their data
		g_cmdQueue.Clear();		// so we're doing this for now, until I come back and 
								// figure out why, ie forever

		DestroyWindow(hWnd);
		return 0;

	case WM_INITMENUPOPUP:
		QE_UpdateCommandUI();	// sikk - Update Menu & Toolbar Items
		return 0;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code)
		{
			// sikk---> Tooltip 
		case TTN_NEEDTEXT:
			// Display tool tip text.
			lpToolTipText = (LPTOOLTIPTEXT)lParam;
			LoadString(g_qeglobals.d_hInstance,
				lpToolTipText->hdr.idFrom,	// string ID == cmd ID
				szToolTip,
				sizeof(szToolTip));
			lpToolTipText->lpszText = szToolTip;
			break;
			// <---sikk

		case RBN_BEGINDRAG:	// sikk - Rebar (to handle movement)
			break;
		case RBN_ENDDRAG:	// sikk - Rebar (to handle movement)
			break;

		case TBN_HOTITEMCHANGE:	// sikk - For Toolbar Button Highlighting
			break;

		default:
			return TRUE;
			break;
		}
		return 0;

	case WM_KEYDOWN:
		if (Tool::HandleInput(uMsg, wParam, lParam))
			return 0;
		return QE_KeyDown(wParam);
	/*case WM_KEYUP:
		if (Tool::HandleInput(uMsg, wParam, lParam))
			return 0;
		return QE_KeyUp(wParam);
	*/
	case WM_MOUSEMOVE:
		if (Tool::HandleInput(uMsg, wParam, lParam))
			return 0;
		return 1;
	case WM_COMMAND:
		if (Tool::HandleInput(uMsg, wParam, lParam))
			return 0;
		return CommandHandler(hWnd, wParam, lParam);
	default:
		break;
	}
	/*
	// lunaran: give tools a last chance to interpret key/mouse/menu inputs
	if (Tool::FilterInput(uMsg))
	{
		if (Tool::HandleInput(uMsg, wParam, lParam))
			return FALSE;

		if (uMsg == WM_KEYDOWN)
			return QE_KeyDown(wParam);
		//else if (uMsg == WM_KEYUP)
		//	return QE_KeyUp(wParam);
		else if (uMsg == WM_COMMAND)
			return CommandHandler(hWnd, wParam, lParam);
	}
	*/
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

/*
==============
WMain_Create
==============
*/
void WMain_Create ()
{
	WNDCLASS	wc;
	//int			i;
	HMENU		hMenu;
	HINSTANCE hInstance = g_qeglobals.d_hInstance;

	/* Register the main class */
	memset(&wc, 0, sizeof(wc));

	wc.style		 = 0;
	wc.lpfnWndProc	 = (WNDPROC)WMain_WndProc;
	wc.cbClsExtra	 = 0;
	wc.cbWndExtra	 = 0;
	wc.hInstance	 = hInstance;
	wc.hIcon		 = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));	// sikk - MainFrame Icon (pretty)
	wc.hCursor		 = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_APPWORKSPACE + 1);
	wc.lpszMenuName  = MAKEINTRESOURCE(IDR_MENU1);
	wc.lpszClassName = MAIN_WINDOW_CLASS;

	if (!RegisterClass(&wc))
		Error("Main_Register: Failed.");

	g_qeglobals.d_hwndMain = CreateWindow(MAIN_WINDOW_CLASS,		// registered class name
		"QuakeEd 3",												// window name
		QE3_MAIN_STYLE,												// window style
		0, 0,														// position of window
		g_nScreenWidth, g_nScreenHeight + GetSystemMetrics(SM_CYSIZE),	// window size
		0,															// parent or owner window
		0,															// menu or child-window identifier
		hInstance,													// application instance
		NULL);														// window-creation data

	if (!g_qeglobals.d_hwndMain)
		Error("Could not create Main Window.");

	// autosave timer
	SetTimer(g_qeglobals.d_hwndMain, QE_TIMER0, 1000, NULL);

#ifdef _DEBUG
	sprintf(g_qeAppName, "QuakeEd 3.%i DEBUG (build %i)", QE_VERSION_MINOR, QE_VERSION_BUILD);
#else
	sprintf(g_qeAppName, "QuakeEd 3.%i (build %i)", QE_VERSION_MINOR, QE_VERSION_BUILD);
#endif

	LoadWindowState(g_qeglobals.d_hwndMain, "MainWindow");

	g_qeglobals.d_hwndRebar			= CreateReBar(g_qeglobals.d_hwndMain, hInstance);
	g_qeglobals.d_hwndToolbar[0]	= CreateToolBar(g_qeglobals.d_hwndRebar, hInstance, 0, 0, 7, 146);
	g_qeglobals.d_hwndToolbar[1]	= CreateToolBar(g_qeglobals.d_hwndRebar, hInstance, 1, 7, 16, 308);
	g_qeglobals.d_hwndToolbar[2]	= CreateToolBar(g_qeglobals.d_hwndRebar, hInstance, 2, 23, 6, 144);
	g_qeglobals.d_hwndToolbar[3]	= CreateToolBar(g_qeglobals.d_hwndRebar, hInstance, 3, 29, 4, 96);
	g_qeglobals.d_hwndToolbar[4]	= CreateToolBar(g_qeglobals.d_hwndRebar, hInstance, 4, 33, 3, 72);
	g_qeglobals.d_hwndToolbar[5]	= CreateToolBar(g_qeglobals.d_hwndRebar, hInstance, 5, 36, 7, 148);
	g_qeglobals.d_hwndToolbar[6]	= CreateToolBar(g_qeglobals.d_hwndRebar, hInstance, 6, 43, 3, 72);
	g_qeglobals.d_hwndToolbar[7]	= CreateToolBar(g_qeglobals.d_hwndRebar, hInstance, 7, 46, 3, 72);
	g_qeglobals.d_hwndToolbar[8]	= CreateToolBar(g_qeglobals.d_hwndRebar, hInstance, 8, 49, 2, 48);
	g_qeglobals.d_hwndToolbar[9]	= CreateToolBar(g_qeglobals.d_hwndRebar, hInstance, 9, 51, 13, 256);
	g_qeglobals.d_hwndToolbar[10]	= CreateToolBar(g_qeglobals.d_hwndRebar, hInstance, 10, 64, 2, 48);
	/*
	g_qeglobals.d_hwndRebar = CreateToolBar(g_qeglobals.d_hwndMain, hInstance, 0, 0, 0, 0);
	*/
	g_qeglobals.d_hwndStatus	= CreateStatusBar(g_qeglobals.d_hwndMain);

	// load misc info from registry
	/*
	long l = sizeof(g_qeglobals.d_savedinfo);
	LoadRegistryInfo("SavedInfo", &g_qeglobals.d_savedinfo, &l);
	
	if (g_qeglobals.d_savedinfo.nSize != sizeof(g_qeglobals.d_savedinfo))
	{
		// fill in new defaults
		g_qeglobals.d_savedinfo.nSize				= sizeof(g_qeglobals.d_savedinfo);
		g_qeglobals.d_savedinfo.nRenderMode			= ID_TEXTURES_TRILINEAR;

		// all 5 obsolete:
		g_qeglobals.d_savedinfo.bShow_XYZ[0]		= true;	// lunaran - grid view reunification
		g_qeglobals.d_savedinfo.bShow_XYZ[1]		= false;
		g_qeglobals.d_savedinfo.bShow_XYZ[2]		= false;
		g_qeglobals.d_savedinfo.bShow_XYZ[3]		= false;
		g_qeglobals.d_savedinfo.bShow_Z				= true;		// Saved Window Toggle

		g_qeglobals.d_savedinfo.nViewFilter			= BFL_HIDDEN;
		g_qeglobals.d_savedinfo.bShow_Axis			= true;		// sikk - Show Axis
		g_qeglobals.d_savedinfo.bShow_Blocks		= false;
		g_qeglobals.d_savedinfo.bShow_CameraGrid	= true;		// sikk - Show Camera Grid
		g_qeglobals.d_savedinfo.bShow_Coordinates	= true;
		g_qeglobals.d_savedinfo.bShow_LightRadius	= false;	// sikk - Show Light Radius
		g_qeglobals.d_savedinfo.bShow_MapBoundary	= true;		// sikk - Show Map Boundary Box
		g_qeglobals.d_savedinfo.bShow_Names			= true;
		g_qeglobals.d_savedinfo.bShow_SizeInfo		= true;
		g_qeglobals.d_savedinfo.bShow_Viewname		= true;		// sikk - Show View Name
		g_qeglobals.d_savedinfo.bShow_Workzone		= false;
		g_qeglobals.d_savedinfo.bScaleLockX			= false;
		g_qeglobals.d_savedinfo.bScaleLockY			= false;
		g_qeglobals.d_savedinfo.bScaleLockZ			= false;
		g_qeglobals.d_savedinfo.bCubicClip			= false;	// sikk - Cubic Clipping
		g_qeglobals.d_savedinfo.nCubicScale			= 32;		// sikk - Cubic Clipping
		g_qeglobals.d_savedinfo.nCameraSpeed		= 1024;		// sikk - Camera Speed Trackbar
// sikk---> Preferences Dialog
		g_qeglobals.d_savedinfo.bAutosave			= true;
		g_qeglobals.d_savedinfo.bLogConsole			= true;
		g_qeglobals.d_savedinfo.bRadiantLights		= true;
		g_qeglobals.d_savedinfo.bVFEModesExclusive	= true;
		g_qeglobals.d_savedinfo.nAutosave			= 5;
		g_qeglobals.d_savedinfo.nMapSize			= 8192;
		g_qeglobals.d_savedinfo.nUndoLevels			= 32;
		g_qeglobals.d_savedinfo.fGamma				= 1.0;
		strcpy(g_qeglobals.d_savedinfo.szHeapsize, "16384");
		strcpy(g_qeglobals.d_savedinfo.szSkill, "1");
// <---sikk

		for (i = 0; i < 3; i++)
		{
			g_colors.brush[i]		= 0.0f;
			g_colors.camBackground[i]	= 0.25f;
			g_colors.camGrid[i]	= 0.2f;
			g_colors.gridBackground[i]		= 1.0f;
			g_colors.gridMajor[i]	= 0.5f;
			g_colors.gridMinor[i]	= 0.75f;
			g_colors.gridText[i]		= 0.0f;
			g_colors.camGrid[i]	= 0.0f;
			g_colors.texBackground[i]	= 0.25f;
			g_colors.texText[i]	= 0.0f;
		}

		g_colors.tool[0]		= 0.0f;
		g_colors.tool[1]		= 0.0f;
		g_colors.tool[2]		= 1.0f;

		g_colors.gridBlock[0]	= 0.0f;
		g_colors.gridBlock[1]	= 0.0f;
		g_colors.gridBlock[2]	= 1.0f;

		g_colors.selection[0]	= 1.0f;
		g_colors.selection[1]	= 0.0f;
		g_colors.selection[2]	= 0.0f;

		g_colors.gridText[0]		= 0.5f;
		g_colors.gridText[1]		= 0.0f;
		g_colors.gridText[2]		= 0.75f;

		// sikk - Set window positions to QE3 Default
		PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_WINDOW_QE3DEFAULT, 0);
	}
	*/

	if ((hMenu = GetMenu(g_qeglobals.d_hwndMain)) != 0)
	{
		QE_UpdateCommandUIFilters(hMenu);
		// by default all of these are checked because that's how they're defined in the menu editor

		if (g_qeglobals.d_savedinfo.bScaleLockX)
		{
			CheckMenuItem(hMenu, ID_SELECTION_SCALELOCKX, MF_CHECKED);
			SendMessage(g_qeglobals.d_hwndToolbar[5], TB_CHECKBUTTON, (WPARAM)ID_SELECTION_SCALELOCKX, (g_qeglobals.d_savedinfo.bScaleLockX ? (LPARAM)true : (LPARAM)false));
		}
		if (g_qeglobals.d_savedinfo.bScaleLockY)
		{
			CheckMenuItem(hMenu, ID_SELECTION_SCALELOCKY, MF_CHECKED);
			SendMessage(g_qeglobals.d_hwndToolbar[5], TB_CHECKBUTTON, (WPARAM)ID_SELECTION_SCALELOCKY, (g_qeglobals.d_savedinfo.bScaleLockY ? (LPARAM)true : (LPARAM)false));
		}
		if (g_qeglobals.d_savedinfo.bScaleLockZ)
		{
			CheckMenuItem(hMenu, ID_SELECTION_SCALELOCKZ, MF_CHECKED);
			SendMessage(g_qeglobals.d_hwndToolbar[5], TB_CHECKBUTTON, (WPARAM)ID_SELECTION_SCALELOCKZ, (g_qeglobals.d_savedinfo.bScaleLockZ ? (LPARAM)true : (LPARAM)false));
		}

		if (g_cfgEditor.CubicClip)
		{
			CheckMenuItem(hMenu, ID_VIEW_CUBICCLIP, MF_CHECKED);
			SendMessage(g_qeglobals.d_hwndToolbar[6], TB_CHECKBUTTON, (WPARAM)ID_VIEW_CUBICCLIP, (g_cfgEditor.CubicClip ? (LPARAM)true : (LPARAM)false));
		}
	}

	ShowWindow(g_qeglobals.d_hwndMain, SW_SHOWMAXIMIZED);	// sikk - changed from "SW_SHOWDEFAULT" (personal preference)
}

// sikk---> Quickly made Splash Screen
clock_t	g_clSplashTimer;

/*
============
SplashDlgProc
============
*/
BOOL CALLBACK SplashDlgProc(
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
	HACCEL	accelerators;
    time_t	lTime;

	g_qeglobals.d_hInstance = hInstance;
// sikk - Quickly made Splash Screen
#ifndef _DEBUG
	HWND	hwndSplash;
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

	GetCurrentDirectory(MAX_PATH - 1, g_qePath);

	WMain_Create();

#ifndef _DEBUG
	try
	{
#endif

	Sys_LogFile();

	WMain_CreateViews();

	// sikk - Print App name and current time for logging purposes
	time(&lTime);	
	Sys_Printf("%s\nSesson Started: %s\n", g_qeAppName, ctime(&lTime));

	if (lpCmdLine && strlen(lpCmdLine))
	{
		ParseCommandLine(lpCmdLine);
		if (g_pszArgV[1])
			Sys_Printf("Command line: %s\n", lpCmdLine);
	}

	QE_Init();

	Sys_DeltaTime();

	Sys_Printf("Entering message loop...\n");

#ifndef _DEBUG
	}
	catch (std::exception &ex)
	{
		//DestroyWindow(hwndSplash);
		MessageBox(g_qeglobals.d_hwndMain, ex.what(), "QuakeEd 3: Initialization Exception", MB_OK | MB_ICONEXCLAMATION);

		// close logging if necessary
		g_cfgEditor.LogConsole = false;
		Sys_LogFile();

		exit(1);
	}
#endif

	while (!g_bHaveQuit)
	{
		Sys_EndWait();	// remove wait cursor if active
		
#ifndef _DEBUG
		try
		{
#endif

			Sys_DeltaTime();
			while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				WMain_HandleMWheel(msg);

				// lunaran - send keys to dialogs first before trying accelerators, so we can tab through
				// the surface dialog and such
				if (!IsDialogMessage(g_qeglobals.d_hwndSurfaceDlg, &msg) &&
					!IsDialogMessage(g_qeglobals.d_texTool->hwndReplaceDlg, &msg) &&
					!IsDialogMessage(g_qeglobals.d_hwndSetKeyvalsDlg, &msg)
					)
				{
					// sikk - We don't want QE3 to handle accelerator shortcuts when editing text in the Entity & Console Windows
					if (!TranslateAccelerator(g_qeglobals.d_hwndMain, accelerators, &msg))
					{
						TranslateMessage(&msg);
						DispatchMessage(&msg);
					}
				}

				if (msg.message == WM_QUIT)
					g_bHaveQuit = true;
			}

			// lunaran - all consequences of selection alteration put in one place for consistent behavior
			Selection::HandleChange();

			Sys_CheckBspProcess();
			// sikk---> Quickly made Splash Screen
#ifndef _DEBUG
			if (hwndSplash)
				if (clock() - g_clSplashTimer > CLOCKS_PER_SEC * 3)
					DestroyWindow(hwndSplash);
#endif
			// <---sikk

			// run time dependent behavior
			SendMessage(g_qeglobals.d_hwndCamera, WM_REALTIME, 0, 0);
			//g_qeglobals.d_vCamera.RealtimeControl(g_deltaTime);

			// update any windows now
			Sys_ForceUpdateWindows(g_nUpdateBits);

			// if not driving in the camera view, block
			//if (!g_qeglobals.d_vCamera.nCamButtonState && !g_bHaveQuit)
			if (!Tool::HotTool() && !g_bHaveQuit)
				WaitMessage();
#ifndef _DEBUG
		}
		catch (std::exception &ex)
		{
			MessageBox(g_qeglobals.d_hwndMain, ex.what(), "QuakeEd 3: Unhandled Exception", MB_OK | MB_ICONEXCLAMATION);

			// close logging if necessary
			g_cfgEditor.LogConsole = false;
			Sys_LogFile();

			exit(1);
		}
#endif
	}
    /* return success of application */
    return TRUE;
}
