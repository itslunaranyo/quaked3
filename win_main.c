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

	InspWnd_SetMode(W_CONSOLE);

	if (g_hBSP_Process)
	{
		Sys_Printf("MSG: BSP is still running...\n");
		return;
	}

	GetTempPath(1024, temppath);
	sprintf(outputpath, "%sjunk.txt", temppath);

	strcpy(name, g_szCurrentMap);
	if (g_bRegionActive)
	{
		Map_SaveFile(name, false);
		StripExtension(name);
		strcat(name, ".reg");
	}

	Map_SaveFile(name, g_bRegionActive);

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

	basepath = ValueForKey(g_qeglobals.d_entityProject, "basepath");
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

/*
=============
DoColor
=============
*/
BOOL DoColor (int iIndex)
{
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

	/*
	** scale colors so that at least one component is at 1.0F 
	** if this is meant to select an entity color
	*/
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

	Sys_UpdateWindows(W_ALL);

	return true;
}

// sikk--> Color Themes
/*
=============
DoTheme
=============
*/
void DoTheme (vec3_t v[])
{
	g_qeglobals.d_savedinfo.v3Colors[COLOR_BRUSHES][0] = v[0][0];
	g_qeglobals.d_savedinfo.v3Colors[COLOR_BRUSHES][1] = v[0][1];
	g_qeglobals.d_savedinfo.v3Colors[COLOR_BRUSHES][2] = v[0][2];

	g_qeglobals.d_savedinfo.v3Colors[COLOR_CAMERABACK][0] = v[1][0];
	g_qeglobals.d_savedinfo.v3Colors[COLOR_CAMERABACK][1] = v[1][1];
	g_qeglobals.d_savedinfo.v3Colors[COLOR_CAMERABACK][2] = v[1][2];

	g_qeglobals.d_savedinfo.v3Colors[COLOR_CAMERAGRID][0] = v[2][0];
	g_qeglobals.d_savedinfo.v3Colors[COLOR_CAMERAGRID][1] = v[2][1];
	g_qeglobals.d_savedinfo.v3Colors[COLOR_CAMERAGRID][2] = v[2][2];

	g_qeglobals.d_savedinfo.v3Colors[COLOR_GRIDBACK][0] = v[3][0];
	g_qeglobals.d_savedinfo.v3Colors[COLOR_GRIDBACK][1] = v[3][1];
	g_qeglobals.d_savedinfo.v3Colors[COLOR_GRIDBACK][2] = v[3][2];

	g_qeglobals.d_savedinfo.v3Colors[COLOR_GRIDBLOCK][0] = v[4][0];
	g_qeglobals.d_savedinfo.v3Colors[COLOR_GRIDBLOCK][1] = v[4][1];
	g_qeglobals.d_savedinfo.v3Colors[COLOR_GRIDBLOCK][2] = v[4][2];

	g_qeglobals.d_savedinfo.v3Colors[COLOR_GRIDMAJOR][0] = v[5][0];
	g_qeglobals.d_savedinfo.v3Colors[COLOR_GRIDMAJOR][1] = v[5][1];
	g_qeglobals.d_savedinfo.v3Colors[COLOR_GRIDMAJOR][2] = v[5][2];

	g_qeglobals.d_savedinfo.v3Colors[COLOR_GRIDMINOR][0] = v[6][0];
	g_qeglobals.d_savedinfo.v3Colors[COLOR_GRIDMINOR][1] = v[6][1];
	g_qeglobals.d_savedinfo.v3Colors[COLOR_GRIDMINOR][2] = v[6][2];

	g_qeglobals.d_savedinfo.v3Colors[COLOR_GRIDTEXT][0] = v[7][0];
	g_qeglobals.d_savedinfo.v3Colors[COLOR_GRIDTEXT][1] = v[7][1];
	g_qeglobals.d_savedinfo.v3Colors[COLOR_GRIDTEXT][2] = v[7][2];

	g_qeglobals.d_savedinfo.v3Colors[COLOR_MAPBOUNDRY][0] = v[8][0];
	g_qeglobals.d_savedinfo.v3Colors[COLOR_MAPBOUNDRY][1] = v[8][1];
	g_qeglobals.d_savedinfo.v3Colors[COLOR_MAPBOUNDRY][2] = v[8][2];

	g_qeglobals.d_savedinfo.v3Colors[COLOR_VIEWNAME][0] = v[9][0];
	g_qeglobals.d_savedinfo.v3Colors[COLOR_VIEWNAME][1] = v[9][1];
	g_qeglobals.d_savedinfo.v3Colors[COLOR_VIEWNAME][2] = v[9][2];

	Sys_UpdateWindows(W_ALL);
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

	GetMenuItem(g_qeglobals.d_lpMruMenu, wId, TRUE, szFileName, sizeof(szFileName));

	// Test if the file exists.
	fExist = OpenFile(szFileName, &of, OF_EXIST) != HFILE_ERROR;

	if (fExist)
	{
		// Place the file on the top of MRU
		AddNewItem(g_qeglobals.d_lpMruMenu, (LPSTR)szFileName);

		// Now perform opening this file !!!
		Map_LoadFile(szFileName);	
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
	char   *szMapName = &g_szCurrentMap[strlen(ValueForKey(g_qeglobals.d_entityProject, "mapspath"))];
	char	szBSPName[MAX_PATH] = "";
	int		handle;
	struct _finddata_t fileinfo;

	// strip "map" extension 
	strncpy(szBSPName, g_szCurrentMap, strlen(g_szCurrentMap) - 3);
	// append "bsp" extension
	strcat(szBSPName, "bsp");

	handle = _findfirst(szBSPName, &fileinfo);
	
	// if bsp file does not exist, return
	if (handle == -1)
	{
		MessageBox(g_qeglobals.d_hwndMain, "The respective BSP file does not exist.\nYou must build current map first", "QuakeEd 3: Error", MB_OK | MB_ICONEXCLAMATION);
		return;
	}


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

void SwapGridCam()
{
	WINDOWPLACEMENT camWP, gridWP;

	memset(&camWP, 0, sizeof(camWP));
	memset(&gridWP, 0, sizeof(gridWP));
	camWP.length = gridWP.length = sizeof(gridWP);

	GetWindowPlacement(g_qeglobals.d_hwndCamera, &camWP);
	GetWindowPlacement(g_qeglobals.d_hwndXYZ[0], &gridWP);

	// either of these appears to do the trick. do both anyway.
	SetWindowRect(g_qeglobals.d_hwndCamera, &gridWP.rcNormalPosition);
	SetWindowPlacement(g_qeglobals.d_hwndCamera, &gridWP);

	SetWindowRect(g_qeglobals.d_hwndXYZ[0], &camWP.rcNormalPosition);
	SetWindowPlacement(g_qeglobals.d_hwndXYZ[0], &camWP);
}

/*
==============
DoWindowPosition
=============
*/
void DoWindowPosition (int nStyle)
{
	RECT	parent, status, toolbar;
	RECT	z, xy, xz, yz, cam, entity;
	int		midy, midx;

	GetWindowRect(g_qeglobals.d_hwndRebar, &toolbar);
	GetWindowRect(g_qeglobals.d_hwndStatus, &status);
	GetClientRect(g_qeglobals.d_hwndMain, &parent);
	parent.top = toolbar.bottom - toolbar.top;
	parent.bottom -= status.bottom - status.top;
	midy = parent.top + (parent.bottom - parent.top) * 0.5;

	switch (nStyle)
	{
	case 0:	// QE3 Default
		// Show/Hide necessary windows
		ShowWindow(g_qeglobals.d_hwndInspector, SW_SHOW);
		ShowWindow(g_qeglobals.d_hwndCamera, SW_SHOW);
		ShowWindow(g_qeglobals.d_hwndXYZ[0], SW_SHOW);
		ShowWindow(g_qeglobals.d_hwndXYZ[2], SW_HIDE);
		ShowWindow(g_qeglobals.d_hwndXYZ[1], SW_HIDE);
		ShowWindow(g_qeglobals.d_hwndZ, SW_SHOW);
		g_qeglobals.d_savedinfo.bShow_XZ = false;
		g_qeglobals.d_savedinfo.bShow_YZ = false;
		g_qeglobals.d_savedinfo.bShow_Z = true;

		midx = (parent.right - parent.left) / 3;

		// Set Z rect
		z.top = parent.top;
		z.bottom = parent.bottom;
		z.left = parent.left;
		z.right = ZWIN_WIDTH;
		SetWindowRect(g_qeglobals.d_hwndZ, &z);
		
		// Set Entity rect
		entity.top = midy;
		entity.bottom = parent.bottom;
		entity.left = parent.right - midx;
		entity.right = parent.right;
		SetWindowRect(g_qeglobals.d_hwndInspector, &entity);

		// Set XY rect
		xy.top = parent.top;
		xy.bottom = parent.bottom;
		xy.left = z.right;
		xy.right = entity.left;
		SetWindowRect(g_qeglobals.d_hwndXYZ[0], &xy);

		// Set Camera rect
		cam.top = parent.top;
		cam.bottom = entity.top;
		cam.left = xy.right;
		cam.right = parent.right;
		SetWindowRect(g_qeglobals.d_hwndCamera, &cam);
		break;

	case 1:	// QE3 Reverse
		// Show/Hide necessary windows
		ShowWindow(g_qeglobals.d_hwndInspector, SW_SHOW);
		ShowWindow(g_qeglobals.d_hwndCamera, SW_SHOW);
		ShowWindow(g_qeglobals.d_hwndXYZ[0], SW_SHOW);
		ShowWindow(g_qeglobals.d_hwndXYZ[2], SW_HIDE);
		ShowWindow(g_qeglobals.d_hwndXYZ[1], SW_HIDE);
		ShowWindow(g_qeglobals.d_hwndZ, SW_SHOW);
		g_qeglobals.d_savedinfo.bShow_XZ = false;
		g_qeglobals.d_savedinfo.bShow_YZ = false;
		g_qeglobals.d_savedinfo.bShow_Z = true;

		midx = (parent.right - parent.left) / 3;

		// Set Z rect
		z.top = parent.top;
		z.bottom = parent.bottom;
		z.left = parent.right - ZWIN_WIDTH;
		z.right = parent.right;;
		SetWindowRect(g_qeglobals.d_hwndZ, &z);
		
		// Set Entity rect
		entity.top = midy;
		entity.bottom = parent.bottom;
		entity.left = parent.left;
		entity.right = parent.left + midx;
		SetWindowRect(g_qeglobals.d_hwndInspector, &entity);
		
		// Set XY rect
		xy.top = parent.top;
		xy.bottom = parent.bottom;
		xy.left = entity.right;
		xy.right = z.left;
		SetWindowRect(g_qeglobals.d_hwndXYZ[0], &xy);

		// Set Camera rect
		cam.top = parent.top;
		cam.bottom = entity.top;
		cam.left = parent.left;
		cam.right = xy.left;
		SetWindowRect(g_qeglobals.d_hwndCamera, &cam);
		break;

	case 2:	// 4 Window w/ Z (Cam Left)
		// Show/Hide necessary windows
		ShowWindow(g_qeglobals.d_hwndInspector, SW_SHOW);
		ShowWindow(g_qeglobals.d_hwndCamera, SW_SHOW);
		ShowWindow(g_qeglobals.d_hwndXYZ[0], SW_SHOW);
		ShowWindow(g_qeglobals.d_hwndXYZ[2], SW_SHOW);
		ShowWindow(g_qeglobals.d_hwndXYZ[1], SW_SHOW);
		ShowWindow(g_qeglobals.d_hwndZ, SW_SHOW);
		g_qeglobals.d_savedinfo.bShow_XZ = true;
		g_qeglobals.d_savedinfo.bShow_YZ = true;
		g_qeglobals.d_savedinfo.bShow_Z = true;

		// Set Z rect
		z.top = parent.top;
		z.bottom = parent.bottom;
		z.left = parent.left;
		z.right = ZWIN_WIDTH;
		SetWindowRect(g_qeglobals.d_hwndZ, &z);
		
		// Set Entity rect
		entity.top = parent.top;
		entity.bottom = parent.bottom;
		entity.left = parent.right - 320;
		entity.right = parent.right;
		SetWindowRect(g_qeglobals.d_hwndInspector, &entity);
		
		midx = z.right + (entity.left - z.right) * 0.5;
		
		// Set XY rect
		xy.top = parent.top;
		xy.bottom = midy;
		xy.left = midx;
		xy.right = entity.left;
		SetWindowRect(g_qeglobals.d_hwndXYZ[0], &xy);

		// Set XZ rect
		xz.top = midy;
		xz.bottom = parent.bottom;
		xz.left = z.right;
		xz.right = midx;
		SetWindowRect(g_qeglobals.d_hwndXYZ[2], &xz);
		
		// Set YZ rect
		yz.top = midy;
		yz.bottom = parent.bottom;
		yz.left = midx;
		yz.right = entity.left;
		SetWindowRect(g_qeglobals.d_hwndXYZ[1], &yz);
		
		// Set Camera rect
		cam.top = parent.top;
		cam.bottom = midy;
		cam.left = z.right;
		cam.right = midx;
		SetWindowRect(g_qeglobals.d_hwndCamera, &cam);
		break;

	case 3:	// 4 Window w/ Z (Cam Right)
		// Show/Hide necessary windows
		ShowWindow(g_qeglobals.d_hwndInspector, SW_SHOW);
		ShowWindow(g_qeglobals.d_hwndCamera, SW_SHOW);
		ShowWindow(g_qeglobals.d_hwndXYZ[0], SW_SHOW);
		ShowWindow(g_qeglobals.d_hwndXYZ[2], SW_SHOW);
		ShowWindow(g_qeglobals.d_hwndXYZ[1], SW_SHOW);
		ShowWindow(g_qeglobals.d_hwndZ, SW_SHOW);
		g_qeglobals.d_savedinfo.bShow_XZ = true;
		g_qeglobals.d_savedinfo.bShow_YZ = true;
		g_qeglobals.d_savedinfo.bShow_Z = true;

		// Set Z rect
		z.top = parent.top;
		z.bottom = parent.bottom;
		z.left = parent.left;
		z.right = ZWIN_WIDTH;
		SetWindowRect(g_qeglobals.d_hwndZ, &z);
		
		// Set Entity rect
		entity.top = parent.top;
		entity.bottom = parent.bottom;
		entity.left = parent.right - 320;
		entity.right = parent.right;
		SetWindowRect(g_qeglobals.d_hwndInspector, &entity);
		
		midx = z.right + (entity.left - z.right) * 0.5;
		
		// Set XY rect
		xy.top = parent.top;
		xy.bottom = midy;
		xy.left = z.right;
		xy.right = midx;
		SetWindowRect(g_qeglobals.d_hwndXYZ[0], &xy);

		// Set XZ rect
		xz.top = midy;
		xz.bottom = parent.bottom;
		xz.left = z.right;
		xz.right = midx;
		SetWindowRect(g_qeglobals.d_hwndXYZ[2], &xz);
		
		// Set YZ rect
		yz.top = midy;
		yz.bottom = parent.bottom;
		yz.left = midx;
		yz.right = entity.left;
		SetWindowRect(g_qeglobals.d_hwndXYZ[1], &yz);
		
		// Set Camera rect
		cam.top = parent.top;
		cam.bottom = midy;
		cam.left = midx;
		cam.right = entity.left;
		SetWindowRect(g_qeglobals.d_hwndCamera, &cam);
		break;

	case 4:	// 4 Window w/o Z (Cam Left)
		// Show/Hide necessary windows
		ShowWindow(g_qeglobals.d_hwndInspector, SW_SHOW);
		ShowWindow(g_qeglobals.d_hwndCamera, SW_SHOW);
		ShowWindow(g_qeglobals.d_hwndXYZ[0], SW_SHOW);
		ShowWindow(g_qeglobals.d_hwndXYZ[2], SW_SHOW);
		ShowWindow(g_qeglobals.d_hwndXYZ[1], SW_SHOW);
		ShowWindow(g_qeglobals.d_hwndZ, SW_HIDE);
		g_qeglobals.d_savedinfo.bShow_XZ = true;
		g_qeglobals.d_savedinfo.bShow_YZ = true;
		g_qeglobals.d_savedinfo.bShow_Z = false;

		// Set Entity rect
		entity.top = parent.top;
		entity.bottom = parent.bottom;
		entity.left = parent.right - 320;
		entity.right = parent.right;
		SetWindowRect(g_qeglobals.d_hwndInspector, &entity);
		
		midx = (entity.left - parent.left) * 0.5;
		
		// Set XY rect
		xy.top = parent.top;
		xy.bottom = midy;
		xy.left = midx;
		xy.right = entity.left;
		SetWindowRect(g_qeglobals.d_hwndXYZ[0], &xy);

		// Set XZ rect
		xz.top = midy;
		xz.bottom = parent.bottom;
		xz.left = parent.left;
		xz.right = midx;
		SetWindowRect(g_qeglobals.d_hwndXYZ[2], &xz);
		
		// Set YZ rect
		yz.top = midy;
		yz.bottom = parent.bottom;
		yz.left = midx;
		yz.right = entity.left;
		SetWindowRect(g_qeglobals.d_hwndXYZ[1], &yz);
		
		// Set Camera rect
		cam.top = parent.top;
		cam.bottom = midy;
		cam.left = parent.left;
		cam.right = midx;
		SetWindowRect(g_qeglobals.d_hwndCamera, &cam);
		break;

	case 5:	// 4 Window w/o Z (Cam Right)
		// Show/Hide necessary windows
		ShowWindow(g_qeglobals.d_hwndInspector, SW_SHOW);
		ShowWindow(g_qeglobals.d_hwndCamera, SW_SHOW);
		ShowWindow(g_qeglobals.d_hwndXYZ[0], SW_SHOW);
		ShowWindow(g_qeglobals.d_hwndXYZ[2], SW_SHOW);
		ShowWindow(g_qeglobals.d_hwndXYZ[1], SW_SHOW);
		ShowWindow(g_qeglobals.d_hwndZ, SW_HIDE);
		g_qeglobals.d_savedinfo.bShow_XZ = true;
		g_qeglobals.d_savedinfo.bShow_YZ = true;
		g_qeglobals.d_savedinfo.bShow_Z = false;

		// Set Entity rect
		entity.top = parent.top;
		entity.bottom = parent.bottom;
		entity.left = parent.right - 320;
		entity.right = parent.right;
		SetWindowRect(g_qeglobals.d_hwndInspector, &entity);
		
		midx = (entity.left - parent.left) * 0.5;
		
		// Set XY rect
		xy.top = parent.top;
		xy.bottom = midy;
		xy.left = parent.left;
		xy.right = midx;
		SetWindowRect(g_qeglobals.d_hwndXYZ[0], &xy);

		// Set XZ rect
		xz.top = midy;
		xz.bottom = parent.bottom;
		xz.left = parent.left;
		xz.right = midx;
		SetWindowRect(g_qeglobals.d_hwndXYZ[2], &xz);
		
		// Set YZ rect
		yz.top = midy;
		yz.bottom = parent.bottom;
		yz.left = midx;
		yz.right = entity.left;
		SetWindowRect(g_qeglobals.d_hwndXYZ[1], &yz);
		
		// Set Camera rect
		cam.top = parent.top;
		cam.bottom = midy;
		cam.left = midx;
		cam.right = entity.left;
		SetWindowRect(g_qeglobals.d_hwndCamera, &cam);
		break;
	
	case 6:	// 4 Window Full w/ Z (Cam Left)
		// Show/Hide necessary windows
		ShowWindow(g_qeglobals.d_hwndInspector, SW_SHOW);
		ShowWindow(g_qeglobals.d_hwndCamera, SW_SHOW);
		ShowWindow(g_qeglobals.d_hwndXYZ[0], SW_SHOW);
		ShowWindow(g_qeglobals.d_hwndXYZ[2], SW_SHOW);
		ShowWindow(g_qeglobals.d_hwndXYZ[1], SW_SHOW);
		ShowWindow(g_qeglobals.d_hwndZ, SW_SHOW);
		g_qeglobals.d_savedinfo.bShow_XZ = true;
		g_qeglobals.d_savedinfo.bShow_YZ = true;
		g_qeglobals.d_savedinfo.bShow_Z = true;

		// Set Z rect
		z.top = parent.top;
		z.bottom = parent.bottom;
		z.left = parent.left;
		z.right = ZWIN_WIDTH;
		SetWindowRect(g_qeglobals.d_hwndZ, &z);
		
		// Set Entity rect
		entity.top = parent.bottom - 480;
		entity.bottom = parent.bottom;
		entity.left = parent.right - 320;
		entity.right = parent.right;
		SetWindowRect(g_qeglobals.d_hwndInspector, &entity);
		
		midx = z.right + (parent.right - z.right) * 0.5;
		
		// Set XY rect
		xy.top = parent.top;
		xy.bottom = midy;
		xy.left = midx;
		xy.right = parent.right;
		SetWindowRect(g_qeglobals.d_hwndXYZ[0], &xy);

		// Set XZ rect
		xz.top = midy;
		xz.bottom = parent.bottom;
		xz.left = z.right;
		xz.right = midx;
		SetWindowRect(g_qeglobals.d_hwndXYZ[2], &xz);
		
		// Set YZ rect
		yz.top = midy;
		yz.bottom = parent.bottom;
		yz.left = midx;
		yz.right = parent.right;
		SetWindowRect(g_qeglobals.d_hwndXYZ[1], &yz);
		
		// Set Camera rect
		cam.top = parent.top;
		cam.bottom = midy;
		cam.left = z.right;
		cam.right = midx;
		SetWindowRect(g_qeglobals.d_hwndCamera, &cam);
		break;

	case 7:	// 4 Window Full w/ Z (Cam Right)
		// Show/Hide necessary windows
		ShowWindow(g_qeglobals.d_hwndInspector, SW_SHOW);
		ShowWindow(g_qeglobals.d_hwndCamera, SW_SHOW);
		ShowWindow(g_qeglobals.d_hwndXYZ[0], SW_SHOW);
		ShowWindow(g_qeglobals.d_hwndXYZ[2], SW_SHOW);
		ShowWindow(g_qeglobals.d_hwndXYZ[1], SW_SHOW);
		ShowWindow(g_qeglobals.d_hwndZ, SW_SHOW);
		g_qeglobals.d_savedinfo.bShow_XZ = true;
		g_qeglobals.d_savedinfo.bShow_YZ = true;
		g_qeglobals.d_savedinfo.bShow_Z = true;

		// Set Z rect
		z.top = parent.top;
		z.bottom = parent.bottom;
		z.left = parent.left;
		z.right = ZWIN_WIDTH;
		SetWindowRect(g_qeglobals.d_hwndZ, &z);
		
		// Set Entity rect
		entity.top = parent.bottom - 480;
		entity.bottom = parent.bottom;
		entity.left = parent.right - 320;
		entity.right = parent.right;
		SetWindowRect(g_qeglobals.d_hwndInspector, &entity);
		
		midx = z.right + (parent.right - z.right) * 0.5;
		
		// Set XY rect
		xy.top = parent.top;
		xy.bottom = midy;
		xy.left = z.right;
		xy.right = midx;
		SetWindowRect(g_qeglobals.d_hwndXYZ[0], &xy);

		// Set XZ rect
		xz.top = midy;
		xz.bottom = parent.bottom;
		xz.left = z.right;
		xz.right = midx;
		SetWindowRect(g_qeglobals.d_hwndXYZ[2], &xz);
		
		// Set YZ rect
		yz.top = midy;
		yz.bottom = parent.bottom;
		yz.left = midx;
		yz.right = parent.right;
		SetWindowRect(g_qeglobals.d_hwndXYZ[1], &yz);
		
		// Set Camera rect
		cam.top = parent.top;
		cam.bottom = midy;
		cam.left = midx;
		cam.right = parent.right;
		SetWindowRect(g_qeglobals.d_hwndCamera, &cam);
		break;

	case 8:	// 4 Window Full w/o Z (Cam Left)
		// Show/Hide necessary windows
		ShowWindow(g_qeglobals.d_hwndInspector, SW_SHOW);
		ShowWindow(g_qeglobals.d_hwndCamera, SW_SHOW);
		ShowWindow(g_qeglobals.d_hwndXYZ[0], SW_SHOW);
		ShowWindow(g_qeglobals.d_hwndXYZ[2], SW_SHOW);
		ShowWindow(g_qeglobals.d_hwndXYZ[1], SW_SHOW);
		ShowWindow(g_qeglobals.d_hwndZ, SW_HIDE);
		g_qeglobals.d_savedinfo.bShow_XZ = true;
		g_qeglobals.d_savedinfo.bShow_YZ = true;
		g_qeglobals.d_savedinfo.bShow_Z = false;

		// Set Entity rect
		entity.top = parent.bottom - 480;
		entity.bottom = parent.bottom;
		entity.left = parent.right - 320;
		entity.right = parent.right;
		SetWindowRect(g_qeglobals.d_hwndInspector, &entity);
		
		midx = (parent.right - parent.left) * 0.5;
		
		// Set XY rect
		xy.top = parent.top;
		xy.bottom = midy;
		xy.left = midx;
		xy.right = parent.right;
		SetWindowRect(g_qeglobals.d_hwndXYZ[0], &xy);

		// Set XZ rect
		xz.top = midy;
		xz.bottom = parent.bottom;
		xz.left = parent.left;
		xz.right = midx;
		SetWindowRect(g_qeglobals.d_hwndXYZ[2], &xz);
		
		// Set YZ rect
		yz.top = midy;
		yz.bottom = parent.bottom;
		yz.left = midx;
		yz.right = parent.right;
		SetWindowRect(g_qeglobals.d_hwndXYZ[1], &yz);
		
		// Set Camera rect
		cam.top = parent.top;
		cam.bottom = midy;
		cam.left = parent.left;
		cam.right = midx;
		SetWindowRect(g_qeglobals.d_hwndCamera, &cam);
		break;

	case 9:	// 4 Window Full w/o Z (Cam Right)
		// Show/Hide necessary windows
		ShowWindow(g_qeglobals.d_hwndInspector, SW_SHOW);
		ShowWindow(g_qeglobals.d_hwndCamera, SW_SHOW);
		ShowWindow(g_qeglobals.d_hwndXYZ[0], SW_SHOW);
		ShowWindow(g_qeglobals.d_hwndXYZ[2], SW_SHOW);
		ShowWindow(g_qeglobals.d_hwndXYZ[1], SW_SHOW);
		ShowWindow(g_qeglobals.d_hwndZ, SW_HIDE);
		g_qeglobals.d_savedinfo.bShow_XZ = true;
		g_qeglobals.d_savedinfo.bShow_YZ = true;
		g_qeglobals.d_savedinfo.bShow_Z = false;

		// Set Entity rect
		entity.top = parent.bottom - 480;
		entity.bottom = parent.bottom;
		entity.left = parent.right - 320;
		entity.right = parent.right;
		SetWindowRect(g_qeglobals.d_hwndInspector, &entity);
		
		midx = (parent.right - parent.left) * 0.5;
		
		// Set XY rect
		xy.top = parent.top;
		xy.bottom = midy;
		xy.left = parent.left;
		xy.right = midx;
		SetWindowRect(g_qeglobals.d_hwndXYZ[0], &xy);

		// Set XZ rect
		xz.top = midy;
		xz.bottom = parent.bottom;
		xz.left = parent.left;
		xz.right = midx;
		SetWindowRect(g_qeglobals.d_hwndXYZ[2], &xz);
		
		// Set YZ rect
		yz.top = midy;
		yz.bottom = parent.bottom;
		yz.left = midx;
		yz.right = parent.right;
		SetWindowRect(g_qeglobals.d_hwndXYZ[1], &yz);
		
		// Set Camera rect
		cam.top = parent.top;
		cam.bottom = midy;
		cam.left = midx;
		cam.right = parent.right;
		SetWindowRect(g_qeglobals.d_hwndCamera, &cam);
		break;
	
	case 10:	// 2 Window (Cam Top)
		// Show/Hide necessary windows
		ShowWindow(g_qeglobals.d_hwndInspector, SW_SHOW);
		ShowWindow(g_qeglobals.d_hwndCamera, SW_SHOW);
		ShowWindow(g_qeglobals.d_hwndXYZ[0], SW_SHOW);
		ShowWindow(g_qeglobals.d_hwndXYZ[2], SW_HIDE);
		ShowWindow(g_qeglobals.d_hwndXYZ[1], SW_HIDE);
		ShowWindow(g_qeglobals.d_hwndZ, SW_SHOW);
		g_qeglobals.d_savedinfo.bShow_XZ = false;
		g_qeglobals.d_savedinfo.bShow_YZ = false;
		g_qeglobals.d_savedinfo.bShow_Z = true;

		// Set Z rect
		z.top = parent.top;
		z.bottom = parent.bottom;
		z.left = parent.left;
		z.right = ZWIN_WIDTH;
		SetWindowRect(g_qeglobals.d_hwndZ, &z);
		
		// Set Entity rect
		entity.top = parent.top;
		entity.bottom = parent.bottom;
		entity.left = parent.right - 320;
		entity.right = parent.right;
		SetWindowRect(g_qeglobals.d_hwndInspector, &entity);
		
		// Set XY rect
		xy.top = midy;
		xy.bottom = parent.bottom;
		xy.left = z.right;
		xy.right = entity.left;
		SetWindowRect(g_qeglobals.d_hwndXYZ[0], &xy);

		// Set Camera rect
		cam.top = parent.top;
		cam.bottom = midy;
		cam.left = z.right;
		cam.right = entity.left;
		SetWindowRect(g_qeglobals.d_hwndCamera, &cam);
		break;

	case 11:	// 2 Window (Cam Bottom)
		// Show/Hide necessary windows
		ShowWindow(g_qeglobals.d_hwndInspector, SW_SHOW);
		ShowWindow(g_qeglobals.d_hwndCamera, SW_SHOW);
		ShowWindow(g_qeglobals.d_hwndXYZ[0], SW_SHOW);
		ShowWindow(g_qeglobals.d_hwndXYZ[2], SW_HIDE);
		ShowWindow(g_qeglobals.d_hwndXYZ[1], SW_HIDE);
		ShowWindow(g_qeglobals.d_hwndZ, SW_SHOW);
		g_qeglobals.d_savedinfo.bShow_XZ = false;
		g_qeglobals.d_savedinfo.bShow_YZ = false;
		g_qeglobals.d_savedinfo.bShow_Z = true;

		// Set Z rect
		z.top = parent.top;
		z.bottom = parent.bottom;
		z.left = parent.left;
		z.right = ZWIN_WIDTH;
		SetWindowRect(g_qeglobals.d_hwndZ, &z);
		
		// Set Entity rect
		entity.top = parent.top;
		entity.bottom = parent.bottom;
		entity.left = parent.right - 320;
		entity.right = parent.right;
		SetWindowRect(g_qeglobals.d_hwndInspector, &entity);
		
		// Set XY rect
		xy.top = parent.top;
		xy.bottom = midy;
		xy.left = z.right;
		xy.right = entity.left;
		SetWindowRect(g_qeglobals.d_hwndXYZ[0], &xy);

		// Set Camera rect
		cam.top = midy;
		cam.bottom = parent.bottom;
		cam.left = z.right;
		cam.right = entity.left;
		SetWindowRect(g_qeglobals.d_hwndCamera, &cam);
		break;

	case 12:	// 2 Window (Cam Left)
		// Show/Hide necessary windows
		ShowWindow(g_qeglobals.d_hwndInspector, SW_SHOW);
		ShowWindow(g_qeglobals.d_hwndCamera, SW_SHOW);
		ShowWindow(g_qeglobals.d_hwndXYZ[0], SW_SHOW);
		ShowWindow(g_qeglobals.d_hwndXYZ[2], SW_HIDE);
		ShowWindow(g_qeglobals.d_hwndXYZ[1], SW_HIDE);
		ShowWindow(g_qeglobals.d_hwndZ, SW_SHOW);
		g_qeglobals.d_savedinfo.bShow_XZ = false;
		g_qeglobals.d_savedinfo.bShow_YZ = false;
		g_qeglobals.d_savedinfo.bShow_Z = true;

		// Set Z rect
		z.top = parent.top;
		z.bottom = parent.bottom;
		z.left = parent.left;
		z.right = ZWIN_WIDTH;
		SetWindowRect(g_qeglobals.d_hwndZ, &z);
		
		// Set Entity rect
		entity.top = parent.bottom - 480;
		entity.bottom = parent.bottom;
		entity.left = parent.right - 320;
		entity.right = parent.right;
		SetWindowRect(g_qeglobals.d_hwndInspector, &entity);
		
		midx = z.right + (parent.right - z.right) * 0.5;
		
		// Set XY rect
		xy.top = parent.top;
		xy.bottom = parent.bottom;
		xy.left = midx;
		xy.right = parent.right;
		SetWindowRect(g_qeglobals.d_hwndXYZ[0], &xy);

		// Set Camera rect
		cam.top = parent.top;
		cam.bottom = parent.bottom;
		cam.left = z.right;
		cam.right = midx;
		SetWindowRect(g_qeglobals.d_hwndCamera, &cam);
		break;

	case 13:	// 2 Window (Cam Right)
		// Show/Hide necessary windows
		ShowWindow(g_qeglobals.d_hwndInspector, SW_SHOW);
		ShowWindow(g_qeglobals.d_hwndCamera, SW_SHOW);
		ShowWindow(g_qeglobals.d_hwndXYZ[0], SW_SHOW);
		ShowWindow(g_qeglobals.d_hwndXYZ[2], SW_HIDE);
		ShowWindow(g_qeglobals.d_hwndXYZ[1], SW_HIDE);
		ShowWindow(g_qeglobals.d_hwndZ, SW_SHOW);
		g_qeglobals.d_savedinfo.bShow_XZ = false;
		g_qeglobals.d_savedinfo.bShow_YZ = false;
		g_qeglobals.d_savedinfo.bShow_Z = true;

		// Set Z rect
		z.top = parent.top;
		z.bottom = parent.bottom;
		z.left = parent.left;
		z.right = ZWIN_WIDTH;
		SetWindowRect(g_qeglobals.d_hwndZ, &z);
		
		// Set Entity rect
		entity.top = parent.bottom - 480;
		entity.bottom = parent.bottom;
		entity.left = parent.right - 320;
		entity.right = parent.right;
		SetWindowRect(g_qeglobals.d_hwndInspector, &entity);
		
		midx = z.right + (parent.right - z.right) * 0.5;
		
		// Set XY rect
		xy.top = parent.top;
		xy.bottom = parent.bottom;
		xy.left = z.right;
		xy.right = midx;
		SetWindowRect(g_qeglobals.d_hwndXYZ[0], &xy);

		// Set Camera rect
		cam.top = parent.top;
		cam.bottom = parent.bottom;
		cam.left = midx;
		cam.right = parent.right;
		SetWindowRect(g_qeglobals.d_hwndCamera, &cam);
		break;
	}

	Sys_UpdateWindows(W_ALL);
}
// <---sikk


static int RotateAngleForModifiers()
{
	if (GetKeyState(VK_SHIFT) < 0) return 180;
	if (GetKeyState(VK_CONTROL) < 0) return -90;
	return 90;
}

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
			Map_New();
			break;
		case ID_FILE_OPEN:
			if (!ConfirmModified())
				return TRUE;
			OpenDialog();
			break;
		case ID_FILE_SAVE:
			if (!strcmp(g_szCurrentMap, "unnamed.map"))
				SaveAsDialog();
			else
				Map_SaveFile(g_szCurrentMap, false);	// ignore region
			if (g_qeglobals.d_nPointfileDisplayList)
				Pointfile_Clear();
			break;
		case ID_FILE_SAVEAS:
			SaveAsDialog();
			break;

		case ID_FILE_NEWPROJECT:	// sikk - New Project Dialog
			NewProjectDialog();
			break;
		case ID_FILE_LOADPROJECT:
			if (!ConfirmModified())
				return TRUE;
			ProjectDialog();
			break;
		case ID_FILE_EDITPROJECT:	// sikk - Project Settings Dialog
			DoProject(false);
			break;

		case ID_FILE_IMPORTMAP:	// sikk - Import Map Dialog
			ImportDialog(true);
			break;
		case ID_FILE_IMPORTPREFAB:	// sikk - Import Prefab Dialog
			ImportDialog(false);
			break;

		case ID_FILE_POINTFILE:
			if (g_qeglobals.d_nPointfileDisplayList)
				Pointfile_Clear();
			else
				Pointfile_Check();
			break;

		case ID_FILE_PRINTXY:
			WXY_Print();
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
		case ID_EDIT_UNDO:	// sikk - Undo/Redo
			Undo_Undo();
			break;
		case ID_EDIT_REDO:	// sikk - Undo/Redo
			Undo_Redo();
			break;

// sikk---> TODO: Copy still broken
		case ID_EDIT_CUT:
			// sikk - This check enables standard text editing shortcuts can be used in the Entity Window
			if (GetTopWindow(g_qeglobals.d_hwndMain) == g_qeglobals.d_hwndInspector)
				break;
			Undo_Start("Cut");
			Undo_AddBrushList(&g_brSelectedBrushes);
			Select_Cut();
			Undo_EndBrushList(&g_brSelectedBrushes);
			Undo_End();
			break;
		case ID_EDIT_COPY:
			// sikk - This check enables standard text editing shortcuts can be used in the Entity Window
			if (GetTopWindow(g_qeglobals.d_hwndMain) == g_qeglobals.d_hwndInspector)
				break;
			Select_Copy();
			break;
		case ID_EDIT_PASTE:
			// sikk - This check enables standard text editing shortcuts can be used in the Entity Window
			if (GetTopWindow(g_qeglobals.d_hwndMain) == g_qeglobals.d_hwndInspector)
				break;
			Undo_Start("Paste");
			Select_Paste();
			Undo_End();
			break;
// <---sikk

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
			DoPreferences();
			break;

//===================================
// View menu
//===================================
// sikk---> Toolbar/Statusbar Toggle
		case ID_VIEW_TOOLBAR_FILEBAND:
			nBandIndex = SendMessage(g_qeglobals.d_hwndRebar, RB_IDTOINDEX, (WPARAM)ID_TOOLBAR + 0, (LPARAM)0);
			if (IsWindowVisible(g_qeglobals.d_hwndToolbar1))
				SendMessage(g_qeglobals.d_hwndRebar, RB_SHOWBAND, (WPARAM)nBandIndex, (LPARAM)FALSE);
			else
				SendMessage(g_qeglobals.d_hwndRebar, RB_SHOWBAND, (WPARAM)nBandIndex, (LPARAM)TRUE);
			break;
		case ID_VIEW_TOOLBAR_EDITBAND:
			nBandIndex = SendMessage(g_qeglobals.d_hwndRebar, RB_IDTOINDEX, (WPARAM)ID_TOOLBAR + 1, (LPARAM)0);
			if (IsWindowVisible(g_qeglobals.d_hwndToolbar2))
				SendMessage(g_qeglobals.d_hwndRebar, RB_SHOWBAND, (WPARAM)nBandIndex, (LPARAM)FALSE);
			else
				SendMessage(g_qeglobals.d_hwndRebar, RB_SHOWBAND, (WPARAM)nBandIndex, (LPARAM)TRUE);
			break;
		case ID_VIEW_TOOLBAR_EDIT2BAND:
			nBandIndex = SendMessage(g_qeglobals.d_hwndRebar, RB_IDTOINDEX, (WPARAM)ID_TOOLBAR + 2, (LPARAM)0);
			if (IsWindowVisible(g_qeglobals.d_hwndToolbar3))
				SendMessage(g_qeglobals.d_hwndRebar, RB_SHOWBAND, (WPARAM)nBandIndex, (LPARAM)FALSE);
			else
				SendMessage(g_qeglobals.d_hwndRebar, RB_SHOWBAND, (WPARAM)nBandIndex, (LPARAM)TRUE);
			break;
		case ID_VIEW_TOOLBAR_SELECTBAND:
			nBandIndex = SendMessage(g_qeglobals.d_hwndRebar, RB_IDTOINDEX, (WPARAM)ID_TOOLBAR + 3, (LPARAM)0);
			if (IsWindowVisible(g_qeglobals.d_hwndToolbar4))
				SendMessage(g_qeglobals.d_hwndRebar, RB_SHOWBAND, (WPARAM)nBandIndex, (LPARAM)FALSE);
			else
				SendMessage(g_qeglobals.d_hwndRebar, RB_SHOWBAND, (WPARAM)nBandIndex, (LPARAM)TRUE);
			break;
		case ID_VIEW_TOOLBAR_CSGBAND:
			nBandIndex = SendMessage(g_qeglobals.d_hwndRebar, RB_IDTOINDEX, (WPARAM)ID_TOOLBAR + 4, (LPARAM)0);
			if (IsWindowVisible(g_qeglobals.d_hwndToolbar5))
				SendMessage(g_qeglobals.d_hwndRebar, RB_SHOWBAND, (WPARAM)nBandIndex, (LPARAM)FALSE);
			else
				SendMessage(g_qeglobals.d_hwndRebar, RB_SHOWBAND, (WPARAM)nBandIndex, (LPARAM)TRUE);
			break;
		case ID_VIEW_TOOLBAR_MODEBAND:
			nBandIndex = SendMessage(g_qeglobals.d_hwndRebar, RB_IDTOINDEX, (WPARAM)ID_TOOLBAR + 5, (LPARAM)0);
			if (IsWindowVisible(g_qeglobals.d_hwndToolbar6))
				SendMessage(g_qeglobals.d_hwndRebar, RB_SHOWBAND, (WPARAM)nBandIndex, (LPARAM)FALSE);
			else
				SendMessage(g_qeglobals.d_hwndRebar, RB_SHOWBAND, (WPARAM)nBandIndex, (LPARAM)TRUE);
			break;
		case ID_VIEW_TOOLBAR_ENTITYBAND:
			nBandIndex = SendMessage(g_qeglobals.d_hwndRebar, RB_IDTOINDEX, (WPARAM)ID_TOOLBAR + 6, (LPARAM)0);
			if (IsWindowVisible(g_qeglobals.d_hwndToolbar7))
				SendMessage(g_qeglobals.d_hwndRebar, RB_SHOWBAND, (WPARAM)nBandIndex, (LPARAM)FALSE);
			else
				SendMessage(g_qeglobals.d_hwndRebar, RB_SHOWBAND, (WPARAM)nBandIndex, (LPARAM)TRUE);
			break;
		case ID_VIEW_TOOLBAR_BRUSHBAND:
			nBandIndex = SendMessage(g_qeglobals.d_hwndRebar, RB_IDTOINDEX, (WPARAM)ID_TOOLBAR + 7, (LPARAM)0);
			if (IsWindowVisible(g_qeglobals.d_hwndToolbar8))
				SendMessage(g_qeglobals.d_hwndRebar, RB_SHOWBAND, (WPARAM)nBandIndex, (LPARAM)FALSE);
			else
				SendMessage(g_qeglobals.d_hwndRebar, RB_SHOWBAND, (WPARAM)nBandIndex, (LPARAM)TRUE);
			break;
		case ID_VIEW_TOOLBAR_TEXTUREBAND:
			nBandIndex = SendMessage(g_qeglobals.d_hwndRebar, RB_IDTOINDEX, (WPARAM)ID_TOOLBAR + 8, (LPARAM)0);
			if (IsWindowVisible(g_qeglobals.d_hwndToolbar9))
				SendMessage(g_qeglobals.d_hwndRebar, RB_SHOWBAND, (WPARAM)nBandIndex, (LPARAM)FALSE);
			else
				SendMessage(g_qeglobals.d_hwndRebar, RB_SHOWBAND, (WPARAM)nBandIndex, (LPARAM)TRUE);
			break;
		case ID_VIEW_TOOLBAR_VIEWBAND:
			nBandIndex = SendMessage(g_qeglobals.d_hwndRebar, RB_IDTOINDEX, (WPARAM)ID_TOOLBAR + 9, (LPARAM)0);
			if (IsWindowVisible(g_qeglobals.d_hwndToolbar10))
				SendMessage(g_qeglobals.d_hwndRebar, RB_SHOWBAND, (WPARAM)nBandIndex, (LPARAM)FALSE);
			else
				SendMessage(g_qeglobals.d_hwndRebar, RB_SHOWBAND, (WPARAM)nBandIndex, (LPARAM)TRUE);
			break;
		case ID_VIEW_TOOLBAR_MISCBAND:
			nBandIndex = SendMessage(g_qeglobals.d_hwndRebar, RB_IDTOINDEX, (WPARAM)ID_TOOLBAR + 10, (LPARAM)0);
			if (IsWindowVisible(g_qeglobals.d_hwndToolbar11))
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

		case ID_VIEW_CAMERA:	// sikk - Toggle Camera View
			if (IsWindowVisible(g_qeglobals.d_hwndCamera))
			{
				if (GetTopWindow(hWnd) != g_qeglobals.d_hwndCamera)
					BringWindowToTop(g_qeglobals.d_hwndCamera);
				else
					ShowWindow(g_qeglobals.d_hwndCamera, SW_HIDE);
			}
			else
			{
				ShowWindow(g_qeglobals.d_hwndCamera, SW_SHOW);
				BringWindowToTop(g_qeglobals.d_hwndCamera);
			}
			break;
		
		case ID_VIEW_ENTITY:
			if (g_qeglobals.d_nInspectorMode != W_ENTITY)
				InspWnd_SetMode(W_ENTITY);
			else
				InspWnd_ToTop();
			break;
		case ID_VIEW_CONSOLE:
			if (g_qeglobals.d_nInspectorMode != W_CONSOLE)
				InspWnd_SetMode(W_CONSOLE);
			else
				InspWnd_ToTop();
			break;
		case ID_VIEW_TEXTURE:
			if (g_qeglobals.d_nInspectorMode != W_TEXTURE)
				InspWnd_SetMode(W_TEXTURE);
			else
				InspWnd_ToTop();
			break;


// sikk---> Multiple Orthographic Views
		case ID_VIEW_TOGGLE_XY: // NOT saved
			if (IsWindowVisible(g_qeglobals.d_hwndXYZ[0]))
			{
				if (GetTopWindow(hWnd) != g_qeglobals.d_hwndXYZ[0])
					BringWindowToTop(g_qeglobals.d_hwndXYZ[0]);
				else
					ShowWindow(g_qeglobals.d_hwndXYZ[0], SW_HIDE);
			}
			else
			{
				ShowWindow(g_qeglobals.d_hwndXYZ[0], SW_SHOW);
				BringWindowToTop(g_qeglobals.d_hwndXYZ[0]);
			}
			break;

		case ID_VIEW_TOGGLE_XZ:	// Saved
			if (IsWindowVisible(g_qeglobals.d_hwndXYZ[2]))
			{
				if (GetTopWindow(hWnd) != g_qeglobals.d_hwndXYZ[2])
					BringWindowToTop(g_qeglobals.d_hwndXYZ[2]);
				else
				{
					ShowWindow(g_qeglobals.d_hwndXYZ[2], SW_HIDE);
					g_qeglobals.d_savedinfo.bShow_XZ = false;
				}
			}
			else
			{
				ShowWindow(g_qeglobals.d_hwndXYZ[2], SW_SHOW);
				BringWindowToTop(g_qeglobals.d_hwndXYZ[2]);
				g_qeglobals.d_savedinfo.bShow_XZ = true;
			}
			break;

		case ID_VIEW_TOGGLE_YZ:	// Saved
			if (IsWindowVisible(g_qeglobals.d_hwndXYZ[1]))
			{
				if (GetTopWindow(hWnd) != g_qeglobals.d_hwndXYZ[1])
					BringWindowToTop(g_qeglobals.d_hwndXYZ[1]);
				else
				{
					ShowWindow(g_qeglobals.d_hwndXYZ[1], SW_HIDE);
					g_qeglobals.d_savedinfo.bShow_YZ = false;
				}
			}
			else
			{
				ShowWindow(g_qeglobals.d_hwndXYZ[1], SW_SHOW);
				BringWindowToTop(g_qeglobals.d_hwndXYZ[1]);
				g_qeglobals.d_savedinfo.bShow_YZ = true;
			}
			break;
// <---sikk
		case ID_VIEW_TOGGLE_Z:	// Saved
			if (IsWindowVisible(g_qeglobals.d_hwndZ))
			{
				if (GetTopWindow(hWnd) != g_qeglobals.d_hwndZ)
					BringWindowToTop(g_qeglobals.d_hwndZ);
				else
				{
					ShowWindow(g_qeglobals.d_hwndZ, SW_HIDE);
					g_qeglobals.d_savedinfo.bShow_Z = false;
				}
			}
			else
			{
				ShowWindow(g_qeglobals.d_hwndZ, SW_SHOW);
				BringWindowToTop(g_qeglobals.d_hwndZ);
				g_qeglobals.d_savedinfo.bShow_Z = true;
			}
			break;

		case ID_VIEW_CENTER:
			g_qeglobals.d_camera.angles[ROLL] = g_qeglobals.d_camera.angles[PITCH] = 0;
			g_qeglobals.d_camera.angles[YAW] = 22.5f * floor((g_qeglobals.d_camera.angles[YAW] + 11) / 22.5f);
			Sys_UpdateWindows(W_CAMERA|W_XY);
			break;
		case ID_VIEW_UPFLOOR:
			Cam_ChangeFloor(true);
			break;
		case ID_VIEW_DOWNFLOOR:
			Cam_ChangeFloor(false);
			break;

		case ID_VIEW_CENTERONSELECTION:	// sikk - Center Views on Selection
			XYZ_PositionView(&g_qeglobals.d_xyz[0]);
			XYZ_PositionView(&g_qeglobals.d_xyz[1]);	// sikk - Multiple Orthographic Views
			XYZ_PositionView(&g_qeglobals.d_xyz[2]);	// sikk - Multiple Orthographic Views
			Cam_PositionView();
			Sys_UpdateWindows(W_ALL);
			break;

		case ID_VIEW_NEXTVIEW:
			XYZWnd_CycleViewAxis(g_qeglobals.d_hwndXYZ[0]);
			break;
		case ID_VIEW_XY:
			XYZWnd_SetViewAxis(g_qeglobals.d_hwndXYZ[0], XY);
			break;
		case ID_VIEW_XZ:
			XYZWnd_SetViewAxis(g_qeglobals.d_hwndXYZ[0], XZ);
			break;
		case ID_VIEW_YZ:
			XYZWnd_SetViewAxis(g_qeglobals.d_hwndXYZ[0], YZ);
			break;
		case ID_VIEW_SWAPGRIDCAM:
			SwapGridCam();
			Sys_UpdateWindows(W_XY | W_CAMERA);
			break;

		case ID_VIEW_100:
			g_qeglobals.d_xyz[0].scale = 1;
			Sys_UpdateWindows(W_XY);
			break;
		case ID_VIEW_ZOOMIN:
			g_qeglobals.d_xyz[0].scale *= 5.0 / 4;
			if (g_qeglobals.d_xyz[0].scale > 32)
				g_qeglobals.d_xyz[0].scale = 32;
			Sys_UpdateWindows(W_XY);
			break;
		case ID_VIEW_ZOOMOUT:
			g_qeglobals.d_xyz[0].scale *= 4.0f / 5;
			if (g_qeglobals.d_xyz[0].scale < 0.05)
				g_qeglobals.d_xyz[0].scale = 0.05f;
			Sys_UpdateWindows(W_XY);
			break;

		case ID_VIEW_Z100:
			g_qeglobals.d_z.scale = 1;
			Sys_UpdateWindows(W_Z);
			break;
		case ID_VIEW_ZZOOMIN:
			g_qeglobals.d_z.scale *= 5.0 / 4;
			if (g_qeglobals.d_z.scale > 16)
				g_qeglobals.d_z.scale = 16;
			Sys_UpdateWindows(W_Z);
			break;
		case ID_VIEW_ZZOOMOUT:
			g_qeglobals.d_z.scale *= 4.0f / 5;
			if (g_qeglobals.d_z.scale < 0.1)
				g_qeglobals.d_z.scale = 0.1f;
			Sys_UpdateWindows(W_Z);
			break;

		case ID_VIEW_CAMSPEED:	// sikk - Camera Speed Trackbar in toolbar
			DoCamSpeed();
			break;

// sikk---> Cubic Clipping
		case ID_VIEW_CUBICCLIP:
			g_qeglobals.d_savedinfo.bCubicClip ^= true;
			Sys_UpdateWindows(W_XY | W_CAMERA);
			break;
		case ID_VIEW_CUBICCLIPZOOMIN:
			g_qeglobals.d_savedinfo.nCubicScale--;
			if (g_qeglobals.d_savedinfo.nCubicScale < 1)
				g_qeglobals.d_savedinfo.nCubicScale = 1;
			Sys_UpdateGridStatusBar();
			Sys_UpdateWindows(W_CAMERA);
			break;

		case ID_VIEW_CUBICCLIPZOOMOUT:
			g_qeglobals.d_savedinfo.nCubicScale++;
			if (g_qeglobals.d_savedinfo.nCubicScale > 64)
				g_qeglobals.d_savedinfo.nCubicScale = 64;
			Sys_UpdateGridStatusBar();
			Sys_UpdateWindows(W_CAMERA);
			break;
// <---sikk

		case ID_VIEW_SHOWAXIS:	// sikk - Show Axis
			g_qeglobals.d_savedinfo.bShow_Axis ^= true;
			Sys_UpdateWindows(W_XY | W_CAMERA);
			break;
		case ID_VIEW_SHOWBLOCKS:
			g_qeglobals.d_savedinfo.bShow_Blocks ^= true;
			Sys_UpdateWindows(W_XY);
			break;
		case ID_VIEW_SHOWCAMERAGRID:	// sikk - Camera Grid
			g_qeglobals.d_savedinfo.bShow_CameraGrid ^= true;
			Sys_UpdateWindows(W_CAMERA);
			break;
		case ID_VIEW_SHOWCOORDINATES:
			g_qeglobals.d_savedinfo.bShow_Coordinates ^= true;
			Sys_UpdateWindows(W_XY | W_Z);
			break;
		case ID_VIEW_SHOWLIGHTRADIUS:	// sikk - Show Light Radius
			g_qeglobals.d_savedinfo.bShow_LightRadius ^= true;
			Sys_UpdateWindows(W_XY);
			break;
		case ID_VIEW_SHOWMAPBOUNDRY:	// sikk - Show Map Boundry Box
			g_qeglobals.d_savedinfo.bShow_MapBoundry ^= true;
			Sys_UpdateWindows(W_CAMERA);
			break;
		case ID_VIEW_SHOWNAMES:
			g_qeglobals.d_savedinfo.bShow_Names ^= true;
			Map_BuildBrushData();
			Sys_UpdateWindows(W_XY);
			break;
		case ID_VIEW_SHOWSIZEINFO:
			g_qeglobals.d_savedinfo.bShow_SizeInfo ^= true;
			Map_BuildBrushData();
			Sys_UpdateWindows(W_XY);
			break;
		case ID_VIEW_SHOWVIEWNAME:
			g_qeglobals.d_savedinfo.bShow_Viewname ^= true;
			Sys_UpdateWindows(W_XY);
			break;
		case ID_VIEW_SHOWWORKZONE:
			g_qeglobals.d_savedinfo.bShow_Workzone ^= true;
			Sys_UpdateWindows(W_XY);
			break;

		case ID_VIEW_SHOWANGLES:
			g_qeglobals.d_savedinfo.nExclude ^= EXCLUDE_ANGLES;
			Sys_UpdateWindows(W_XY | W_CAMERA);
			break;
		case ID_VIEW_SHOWCLIP:
			g_qeglobals.d_savedinfo.nExclude ^= EXCLUDE_CLIP;
			Sys_UpdateWindows(W_XY | W_CAMERA);
			break;
		case ID_VIEW_SHOWENT:
			g_qeglobals.d_savedinfo.nExclude ^= EXCLUDE_ENT;
			Sys_UpdateWindows(W_XY | W_CAMERA);
			break;
		case ID_VIEW_SHOWFUNCWALL:
			g_qeglobals.d_savedinfo.nExclude ^= EXCLUDE_FUNC_WALL;
			Sys_UpdateWindows(W_XY | W_CAMERA);
			break;
		case ID_VIEW_SHOWLIGHTS:
			g_qeglobals.d_savedinfo.nExclude ^= EXCLUDE_LIGHTS;
			Sys_UpdateWindows(W_XY | W_CAMERA);
			break;
		case ID_VIEW_SHOWPATH:
			g_qeglobals.d_savedinfo.nExclude ^= EXCLUDE_PATHS;
			Sys_UpdateWindows(W_XY | W_CAMERA);
			break;
		case ID_VIEW_SHOWSKY:
			g_qeglobals.d_savedinfo.nExclude ^= EXCLUDE_SKY;
			Sys_UpdateWindows(W_XY | W_CAMERA);
			break;
		case ID_VIEW_SHOWWATER:
			g_qeglobals.d_savedinfo.nExclude ^= EXCLUDE_WATER;
			Sys_UpdateWindows(W_XY | W_CAMERA);
			break;
		case ID_VIEW_SHOWWORLD:
			g_qeglobals.d_savedinfo.nExclude ^= EXCLUDE_WORLD;
			Sys_UpdateWindows(W_XY | W_CAMERA);
			break;
		case ID_VIEW_SHOWHINT:
			g_qeglobals.d_savedinfo.nExclude ^= EXCLUDE_HINT;
			Sys_UpdateWindows(W_XY | W_CAMERA);
			break;
		case ID_VIEW_SHOWDETAIL:
			g_qeglobals.d_savedinfo.nExclude ^= EXCLUDE_DETAIL;
			Sys_UpdateWindows(W_XY | W_CAMERA);
			break;

		case ID_VIEW_HIDESHOW_HIDESELECTED:
			Select_Hide();
			Select_DeselectAll(true);
			break;
		case ID_VIEW_HIDESHOW_SHOWHIDDEN:
			Select_ShowAllHidden();
			break;

//===================================
// Selection menu
//===================================
		case ID_SELECTION_DRAGEDGES:
			if (g_qeglobals.d_selSelectMode == sel_edge)
			{
				g_qeglobals.d_selSelectMode = sel_brush;
				Sys_UpdateWindows(W_ALL);
			}
			else
			{
				Clip_UnsetMode();
				SetupVertexSelection();
				if (g_qeglobals.d_nNumPoints)
					g_qeglobals.d_selSelectMode = sel_edge;
			}
			break;
		case ID_SELECTION_DRAGVERTICES:
			if (g_qeglobals.d_selSelectMode == sel_vertex)
			{
				g_qeglobals.d_selSelectMode = sel_brush;
				Sys_UpdateWindows(W_ALL);
			}
			else
			{
				Clip_UnsetMode();
				SetupVertexSelection();
				if (g_qeglobals.d_nNumPoints)
					g_qeglobals.d_selSelectMode = sel_vertex;
			}
			break;

		case ID_SELECTION_CLONE:
			Undo_Start("Clone");
//			Undo_AddBrushList(&g_brSelectedBrushes);
			Select_Clone();
//			Undo_EndBrushList(&g_brSelectedBrushes);
			Undo_End();
			break;
		case ID_SELECTION_DESELECT:
			if (g_qeglobals.d_bClipMode)
				Clip_UnsetMode();
			else
				Select_DeselectAll(true);
			break;
		case ID_SELECTION_INVERT:
			Select_Invert();
			break;
		case ID_SELECTION_DELETE:
			{	// sikk - TODO: Undo doesn't function properly with mixed selection  
				brush_t *brush;

				Undo_Start("Delete");
				Undo_AddBrushList(&g_brSelectedBrushes);
				// add all deleted entities to the undo
				for (brush = g_brSelectedBrushes.next; brush != &g_brSelectedBrushes; brush = brush->next)
					Undo_AddEntity(brush->owner);
				Select_Delete();
				Undo_EndBrushList(&g_brSelectedBrushes);
				Undo_End();
			}
			break;

		case ID_SELECTION_FLIPX:
			Undo_Start("Flip X");
			Undo_AddBrushList(&g_brSelectedBrushes);
			Select_FlipAxis(0);
			Undo_EndBrushList(&g_brSelectedBrushes);
			Undo_End();
			break;
		case ID_SELECTION_FLIPY:
			Undo_Start("Flip Y");
			Undo_AddBrushList(&g_brSelectedBrushes);
			Select_FlipAxis(1);
			Undo_EndBrushList(&g_brSelectedBrushes);
			Undo_End();
			break;
		case ID_SELECTION_FLIPZ:
			Undo_Start("Flip Z");
			Undo_AddBrushList(&g_brSelectedBrushes);
			Select_FlipAxis(2);
			Undo_EndBrushList(&g_brSelectedBrushes);
			Undo_End();
			break;

		case ID_SELECTION_ROTATEX:
			Undo_Start("Rotate X");
			Undo_AddBrushList(&g_brSelectedBrushes);
			Select_RotateAxis(0, RotateAngleForModifiers(), false);
			Undo_EndBrushList(&g_brSelectedBrushes);
			Undo_End();
			break;
		case ID_SELECTION_ROTATEY:
			Undo_Start("Rotate Y");
			Undo_AddBrushList(&g_brSelectedBrushes);
			Select_RotateAxis(1, RotateAngleForModifiers(), false);
			Undo_EndBrushList(&g_brSelectedBrushes);
			Undo_End();
			break;
		case ID_SELECTION_ROTATEZ:
			Undo_Start("Rotate Z");
			Undo_AddBrushList(&g_brSelectedBrushes);
			Select_RotateAxis(2, RotateAngleForModifiers(), false);
			Undo_EndBrushList(&g_brSelectedBrushes);
			Undo_End();
			break;
		case ID_SELECTION_ARBITRARYROTATION:
			Undo_Start("Arbitrary Rotation");
			Undo_AddBrushList(&g_brSelectedBrushes);
			DoRotate();
			Undo_EndBrushList(&g_brSelectedBrushes);
			Undo_End();
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
			Undo_Start("Scale");
			Undo_AddBrushList(&g_brSelectedBrushes);
			DoScale();
			Undo_EndBrushList(&g_brSelectedBrushes);
			Undo_End();
			break;

		case ID_SELECTION_CSGHOLLOW:
			Undo_Start("CSG Hollow");
			Undo_AddBrushList(&g_brSelectedBrushes);
			CSG_Hollow();
			Undo_EndBrushList(&g_brSelectedBrushes);
			Undo_End();
			break;
		case ID_SELECTION_CSGSUBTRACT:
			Undo_Start("CSG Subtract");
			CSG_Subtract();
			Undo_End();
			break;
		case ID_SELECTION_CSGMERGE:
			Undo_Start("CSG Merge");
			Undo_AddBrushList(&g_brSelectedBrushes);
			CSG_Merge();
			Undo_EndBrushList(&g_brSelectedBrushes);
			Undo_End();
			break;

		case ID_SELECTION_CLIPPER:
			if (g_qeglobals.d_selSelectMode != sel_brush)
			{
				g_qeglobals.d_selSelectMode = sel_brush;
				Sys_UpdateWindows(W_ALL);
			}
			Clip_SetMode();
			break;
		// lunaran - moved undo into the actual clip & split actions
		case ID_SELECTION_CLIPSELECTED:
		//	Undo_Start("Clip Selected");
		//	Undo_AddBrushList(&g_brSelectedBrushes);
			Clip_Clip();
		//	Undo_EndBrushList(&g_brSelectedBrushes);
		//	Undo_End();
			break;
		case ID_SELECTION_SPLITSELECTED:
		//	Undo_Start("Split Selected");
		//	Undo_AddBrushList(&g_brSelectedBrushes);
			Clip_Split();
		//	Undo_EndBrushList(&g_brSelectedBrushes);
		//	Undo_End();
			break;
		case ID_SELECTION_FLIPCLIP:
			Clip_Flip();
			break;

		// lunaran - back and forth face<->brush conversion
		case ID_SELECTION_FACESTOBRUSHESPARTIAL:
			Select_FacesToBrushes(true);
			break;
		case ID_SELECTION_FACESTOBRUSHESCOMPLETE:
			Select_FacesToBrushes(false);
			break;
		case ID_SELECTION_BRUSHESTOFACES:
			Select_BrushesToFaces();
			break;

		case ID_SELECTION_SELECTCOMPLETETALL:
			Select_CompleteTall();
			break;
		case ID_SELECTION_SELECTTOUCHING:
			Select_Touching();
			break;
		case ID_SELECTION_SELECTPARTIALTALL:
			Select_PartialTall();
			break;
		case ID_SELECTION_SELECTINSIDE:
			Select_Inside();
			break;
		case ID_SELECTION_SELECTALL:	// sikk - Select All
			// sikk - This check enables standard text editing shortcuts can be used in the Entity Window
			if (GetTopWindow(g_qeglobals.d_hwndMain) == g_qeglobals.d_hwndInspector)
				break;
			Select_All();
			break;
		case ID_SELECTION_SELECTALLTYPE:	// sikk - Select All Type
			Select_AllType();
			break;

		case ID_SELECTION_SELECTMATCHINGTEXTURES:	// sikk - Select Matching Textures
			Select_MatchingTextures();
			break;
		case ID_SELECTION_SELECTMATCHINGKEYVALUE:	// sikk - Select Matching Key/Value
			DoFindKeyValue();
			break;

		case ID_SELECTION_CONNECT:
			Select_ConnectEntities();
			break;
		case ID_SELECTION_UNGROUPENTITY:
			Select_Ungroup();
			break;
		case ID_SELECTION_GROUPNEXTBRUSH:
			Select_NextBrushInGroup();
			break;
		case ID_SELECTION_INSERTBRUSH:	// sikk - Insert Brush into Entity
			Select_InsertBrush();
			break;

		case ID_SELECTION_EXPORTMAP:	// sikk - Export Selection Dialog (Map format)
			ExportDialog(true);
			break;
		case ID_SELECTION_EXPORTPREFAB:	// sikk - Export Selection Dialog (Prefab format)
			ExportDialog(false);
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
			PostMessage(g_qeglobals.d_hwndXYZ[0], WM_PAINT, 0, 0);
			Sys_UpdateWindows(W_Z | W_XY);
			break;

		case ID_GRID_SNAPTOGRID:
			g_qeglobals.d_savedinfo.bNoClamp ^= true;
			Sys_UpdateGridStatusBar();
			break;

//===================================
// Texture menu
//===================================
		case ID_TEXTURES_UPDATEMENU:
			Sys_BeginWait();
			FillTextureMenu();
			InspWnd_SetMode(W_CONSOLE);
			break;

		case ID_TEXTURES_FLUSH_ALL:
			Sys_BeginWait();
//			Map_New();	// sikk - I don't understand the point to start a new map. 
						// We want to flush all textures, not flush everything I've
						// done to this point and forgot to save.
			Texture_FlushAll();
			Texture_Init();
			InspWnd_SetMode(W_TEXTURE);
			Sys_UpdateWindows(W_TEXTURE);
			SetWindowText(g_qeglobals.d_hwndInspector, "Flushed All...");
			break;
		case ID_TEXTURES_FLUSH_UNUSED:
			Sys_BeginWait();
			Texture_ShowInuse();
			Texture_FlushUnused();
			InspWnd_SetMode(W_TEXTURE);
			Sys_UpdateWindows(W_TEXTURE);
			SetWindowText(g_qeglobals.d_hwndInspector, "Flushed Unused...");
			break;
		// sikk - TODO: This doesn't function like Radiant and is, for
		// the most part, pointless. Will later make it toggle along 
		// with a "Show All" command .
		case ID_TEXTURES_SHOWINUSE:
			Sys_BeginWait();
			Texture_ShowInuse();
			InspWnd_SetMode(W_TEXTURE);
			break;

		case ID_TEXTURES_RESETSCALE:	// sikk - Reset Texture View Scale
			TexWnd_SetScale(1.0f);
			Sys_UpdateWindows(W_TEXTURE);
			break;

		case ID_TEXTURES_REPLACEALL:
			DoFindTexture();
			break;
		case ID_TEXTURES_LOCK:
			g_qeglobals.d_bTextureLock ^= true;
			Sys_UpdateWindows(W_CAMERA);
			Sys_UpdateGridStatusBar();
			break;
		case ID_TEXTURES_DEFAULTSCALE:	// sikk - Default Texture Scale Dialog
			DoDefaultTexScale();
			break;

		case ID_TEXTURES_INSPECTOR:
			SurfWnd_Create();
			break;

		case ID_TEXTURES_POPUP:	// sikk - Toolbar Button
			DoTexturesPopup();
			break;

		case ID_TEXTURES_WIREFRAME:
		case ID_TEXTURES_FLATSHADE:
		case ID_TEXTURES_NEAREST:
		case ID_TEXTURES_NEARESTMIPMAP:
		case ID_TEXTURES_LINEAR:
		case ID_TEXTURES_BILINEAR:
		case ID_TEXTURES_BILINEARMIPMAP:
		case ID_TEXTURES_TRILINEAR:
			Texture_SetMode(LOWORD(wParam));
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
			Texture_ShowWad(LOWORD(wParam));
			InspWnd_SetMode(W_TEXTURE);
			break;

//===================================
// Misc menu
//===================================
		case ID_MISC_BENCHMARK:
			SendMessage(g_qeglobals.d_hwndCamera, WM_BENCHMARK, 0, 0);
			InspWnd_SetMode(W_CONSOLE);
			break;

// sikk---> Color Themes
// I will continue to add themes as I am able to get the colors used by other popular 3D apps
		case ID_THEMES_QE4:
			{
				vec3_t v[] = {	{0.0f, 0.0f, 0.0f}, {0.25f, 0.25f, 0.25f}, {0.2f, 0.2f, 0.2f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f},
								{0.5f, 0.5f, 0.5f}, {0.75f, 0.75f, 0.75f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.5f, 0.0f, 0.75f}	};
				DoTheme(v);
			}
			break;
		case ID_THEMES_Q3RADIANT:
			{
				vec3_t v[] = {	{0.0f, 0.0f, 0.0f}, {0.25f, 0.25f, 0.25f}, {0.2f, 0.2f, 0.2f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f},
								{0.5f, 0.5f, 0.5f}, {1.0f,  1.0f,  1.0f},  {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.5f, 0.0f, 0.75f}	};
				DoTheme(v);
			}
			break;
		case ID_THEMES_BLACKGREEN:
			{
				vec3_t v[] = {	{1.0f, 1.0f, 1.0f}, {0.25f, 0.25f, 0.25f}, {0.2f, 0.2f, 0.2f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f},
								{0.3f, 0.5f, 0.5f}, {0.0f,  0.0f,  0.0f},  {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.7f, 0.7f, 0.0f}	};
				DoTheme(v);
			}
			break;
		case ID_THEMES_WORLDCRAFT:
			{
				vec3_t v[] = {	{1.0f,  1.0f,  1.0f},  {0.0f, 0.0f, 0.0f}, {0.2f, 0.2f, 0.2f}, {0.0f,  0.0f,  0.0f},  {0.0f, 0.5f, 0.5f},
								{0.35f, 0.35f, 0.35f}, {0.2f, 0.2f, 0.2f}, {1.0f, 1.0f, 1.0f}, {0.25f, 0.25f, 0.25f}, {0.7f, 0.7f, 0.0f}	};
				DoTheme(v);
			}
			break;
		case ID_THEMES_UNREALED:
			{
				vec3_t v[] = {	{0.0f,  0.0f,  0.0f},  {0.0f,  0.0f,  0.0f},  {0.0f, 0.0f, 0.5f}, {0.64f, 0.64f, 0.64f}, {0.25f, 0.25f, 0.25f},
								{0.47f, 0.47f, 0.47f}, {0.58f, 0.58f, 0.58f}, {0.0f, 0.0f, 0.0f}, {0.0f,  0.0f,  0.5f},  {0.5f,  0.0f,  0.75f}	};
				DoTheme(v);
			}
			break;
		case ID_THEMES_BLENDER:
			{
				vec3_t v[] = {	{0.0f,  0.0f,  0.0f},  {0.25f, 0.25f, 0.25f}, {0.2f, 0.2f, 0.2f}, {0.45f, 0.45f, 0.45f}, {0.25f, 0.25f, 0.25f},
								{0.36f, 0.36f, 0.36f}, {0.4f,  0.4f,  0.4f},  {1.0f, 1.0f, 1.0f}, {0.2f,  0.2f,  0.2f},  {0.5f,  0.0f,  0.0f}	};
				DoTheme(v);
			}
			break;
		case ID_THEMES_MAYA:
			{
				vec3_t v[] = {	{0.0f,  0.0f,  0.0f},  {0.57f, 0.57f, 0.57f}, {0.47f, 0.47f, 0.47f}, {0.64f, 0.64f, 0.64f}, {0.25f, 0.25f, 0.25f},
								{0.47f, 0.47f, 0.47f}, {0.58f, 0.58f, 0.58f}, {0.0f,  0.0f,  0.0f},  {0.2f,  0.2f,  0.2f},  {0.0f,  0.45f, 0.0f}	};
				DoTheme(v);
			}
			break;
		case ID_THEMES_LIGHTWAVE:
			{
				vec3_t v[] = {	{0.0f,  0.0f,  0.0f},  {0.42f, 0.42f, 0.42f}, {0.56f, 0.56f, 0.56f}, {0.42f, 0.42f, 0.42f}, {0.25f, 0.25f, 0.25f},
								{0.56f, 0.56f, 0.56f}, {0.48f, 0.48f, 0.48f}, {0.75f, 0.75f, 0.75f}, {0.32f, 0.32f, 0.32f}, {0.0f,  0.75f, 0.75f}	};
				DoTheme(v);
			}
			break;
		case ID_THEMES_3DSMAX:
			{
				vec3_t v[] = {	{0.0f,  0.0f,  0.0f},  {0.48f, 0.48f, 0.48f}, {0.36f, 0.36f, 0.36f}, {0.48f, 0.48f, 0.48f}, {0.25f, 0.25f, 0.25f},
								{0.36f, 0.36f, 0.36f}, {0.42f, 0.42f, 0.42f}, {0.0f,  0.0f, 0.0f},  {0.2f,  0.2f,  0.2f},  {0.75f,  0.75f, 0.75f}	};
				DoTheme(v);
			}
			break;
// <---sikk
//								{COLOR_BRUSHES},	{COLOR_CAMERABACK},	   {COLOR_CAMERAGRID}, {COLOR_GRIDBACK},   {COLOR_GRIDBLOCK},
//								{COLOR_GRIDMAJOR},	{COLOR_GRIDMINOR},	   {COLOR_GRIDTEXT},   {COLOR_MAPBOUNDRY}, {COLOR_VIEWNAME}
		case ID_THEMES_BLUEGRAY:
			{
				vec3_t v[] = {	{0.75f,0.75f,0.75f},  {0.2f, 0.2f, 0.2f}, {0.3f, 0.3f, 0.3f}, {0.25f, 0.25f, 0.25f}, {0.4f, 0.4f, 0.4f},
								{0.35f, 0.35f, 0.35f}, {0.3f, 0.3f, 0.3f}, {0.8f, 0.8f, 0.8f},  {0.2f,  0.4f,  0.4f},  {1.0f, 1.0f,  1.0f}	};
				DoTheme(v);
			}
			break;

		case ID_COLORS_VIEWNAME:
			DoColor(COLOR_VIEWNAME);
			Sys_UpdateWindows(W_ALL);
			break;
		case ID_COLORS_CAMERABACK:
			DoColor(COLOR_CAMERABACK);
			Sys_UpdateWindows(W_ALL);
			break;
		case ID_COLORS_CAMERAGRID:
			DoColor(COLOR_CAMERAGRID);
			Sys_UpdateWindows(W_ALL);
			break;
		case ID_COLORS_CLIPPER:
			DoColor(COLOR_CLIPPER);
			Sys_UpdateWindows(W_ALL);
			break;
		case ID_COLORS_BRUSHES:
			DoColor(COLOR_BRUSHES);
			Sys_UpdateWindows(W_ALL);
			break;
		case ID_COLORS_GRIDBACK:
			DoColor(COLOR_GRIDBACK);
			Sys_UpdateWindows(W_ALL);
			break;
		case ID_COLORS_GRIDBLOCK:
			DoColor(COLOR_GRIDBLOCK);
			Sys_UpdateWindows(W_ALL);
			break;
		case ID_COLORS_GRIDMAJOR:
			DoColor(COLOR_GRIDMAJOR);
			Sys_UpdateWindows(W_ALL);
			break;
		case ID_COLORS_GRIDMINOR:
			DoColor(COLOR_GRIDMINOR);
			Sys_UpdateWindows(W_ALL);
			break;
		case ID_COLORS_GRIDTEXT:
			DoColor(COLOR_GRIDTEXT);
			Sys_UpdateWindows(W_ALL);
			break;
		case ID_COLORS_MAPBOUNDRY:
			DoColor(COLOR_MAPBOUNDRY);
			Sys_UpdateWindows(W_ALL);
			break;
		case ID_COLORS_SELECTEDBRUSH:
			DoColor(COLOR_SELBRUSHES);
			Sys_UpdateWindows(W_ALL);
			break;
		case ID_COLORS_TEXTUREBACK:
			DoColor(COLOR_TEXTUREBACK);
			Sys_UpdateWindows(W_ALL);
			break;
		case ID_COLORS_TEXTURETEXT:
			DoColor(COLOR_TEXTURETEXT);
			Sys_UpdateWindows(W_ALL);
			break;

		case ID_MISC_NEXTLEAKSPOT:
			Pointfile_Next();
			break;
		case ID_MISC_PREVIOUSLEAKSPOT:
			Pointfile_Prev();
			break;

		case ID_MISC_SELECTENTITYCOLOR:
			if ((g_qeglobals.d_nInspectorMode == W_ENTITY) && DoColor(COLOR_ENTITY) == true)
			{
				char buffer[64];
				
				sprintf(buffer, "%f %f %f", g_qeglobals.d_savedinfo.v3Colors[COLOR_ENTITY][0],
											g_qeglobals.d_savedinfo.v3Colors[COLOR_ENTITY][1],
											g_qeglobals.d_savedinfo.v3Colors[COLOR_ENTITY][2]);
				
				SetWindowText(g_hwndEnt[ENT_KEYFIELD], "_color");
				SetWindowText(g_hwndEnt[ENT_VALUEFIELD], buffer);
				EntWnd_AddKeyValue();
			}
			Sys_UpdateWindows(W_ALL);
			break;

		case ID_MISC_TESTMAP:
			DoTestMap();
			break;

//===================================
// Region menu
//===================================
		case ID_REGION_OFF:
			Map_RegionOff();
			break;

		case ID_REGION_SETXY:
			Map_RegionXY();
			break;
// sikk---> Multiple Orthographic Views
		case ID_REGION_SETXZ:
			Map_RegionXZ();
			break;
		case ID_REGION_SETYZ:
			Map_RegionYZ();
			break;
// <---sikk
		case ID_REGION_SETTALLBRUSH:
			Map_RegionTallBrush();
			break;
		case ID_REGION_SETBRUSH:
			Map_RegionBrush();
			break;
		case ID_REGION_SETSELECTION:
			Map_RegionSelectedBrushes();
			break;

//===================================
// Brush menu
//===================================
			/*
		case ID_BRUSH_3SIDED:
			Undo_Start("3 Sided");
			Undo_AddBrushList(&g_brSelectedBrushes);
			Brush_MakeSided(3);
			Undo_EndBrushList(&g_brSelectedBrushes);
			Undo_End();
			break;
		case ID_BRUSH_4SIDED:
			Undo_Start("4 Sided");
			Undo_AddBrushList(&g_brSelectedBrushes);
			Brush_MakeSided(4);
			Undo_EndBrushList(&g_brSelectedBrushes);
			Undo_End();
			break;
		case ID_BRUSH_5SIDED:
			Undo_Start("5 Sided");
			Undo_AddBrushList(&g_brSelectedBrushes);
			Brush_MakeSided(5);
			Undo_EndBrushList(&g_brSelectedBrushes);
			Undo_End();
			break;
		case ID_BRUSH_6SIDED:
			Undo_Start("6 Sided");
			Undo_AddBrushList(&g_brSelectedBrushes);
			Brush_MakeSided(6);
			Undo_EndBrushList(&g_brSelectedBrushes);
			Undo_End();
			break;
		case ID_BRUSH_7SIDED:
			Undo_Start("7 Sided");
			Undo_AddBrushList(&g_brSelectedBrushes);
			Brush_MakeSided(7);
			Undo_EndBrushList(&g_brSelectedBrushes);
			Undo_End();
			break;
		case ID_BRUSH_8SIDED:
			Undo_Start("8 Sided");
			Undo_AddBrushList(&g_brSelectedBrushes);
			Brush_MakeSided(8);
			Undo_EndBrushList(&g_brSelectedBrushes);
			Undo_End();
			break;
		case ID_BRUSH_9SIDED:
			Undo_Start("9 Sided");
			Undo_AddBrushList(&g_brSelectedBrushes);
			Brush_MakeSided(9);
			Undo_EndBrushList(&g_brSelectedBrushes);
			Undo_End();
			break;
			*/

// sikk---> Brush Primitives
		case ID_BRUSH_CYLINDER:
			Undo_Start("Make Cylinder");
			Undo_AddBrushList(&g_brSelectedBrushes);
			DoSides(0);
			Undo_EndBrushList(&g_brSelectedBrushes);
			Undo_End();
			break;
		case ID_BRUSH_CONE:
			Undo_Start("Make Cone");
			Undo_AddBrushList(&g_brSelectedBrushes);
			DoSides(1);
			Undo_EndBrushList(&g_brSelectedBrushes);
			Undo_End();
			break;
		case ID_BRUSH_SPHERE:
			Undo_Start("Make Sphere");
			Undo_AddBrushList(&g_brSelectedBrushes);
			DoSides(2);
			Undo_EndBrushList(&g_brSelectedBrushes);
			Undo_End();
			break;
//		case ID_BRUSH_TORUS:
//			Undo_Start("Make Torus");
//			Undo_AddBrushList(&g_brSelectedBrushes);
//			DoSides(3);
//			Undo_EndBrushList(&g_brSelectedBrushes);
//			Undo_End();
//			break;
// <---sikk

//===================================
// Window menu
//===================================
// sikk---> Window Positions
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
		case ID_WINDOW_4WINDOWFULLZCAMLEFT:
			DoWindowPosition(6);
			break;
		case ID_WINDOW_4WINDOWFULLZCAMRIGHT:
			DoWindowPosition(7);
			break;
		case ID_WINDOW_4WINDOWFULLNOZCAMLEFT:
			DoWindowPosition(8);
			break;
		case ID_WINDOW_4WINDOWFULLNOZCAMRIGHT:
			DoWindowPosition(9);
			break;
		case ID_WINDOW_2WINDOWCAMTOP:
			DoWindowPosition(10);
			break;
		case ID_WINDOW_2WINDOWCAMBOTTOM:
			DoWindowPosition(11);
			break;
		case ID_WINDOW_2WINDOWCAMLEFT:
			DoWindowPosition(12);
			break;
		case ID_WINDOW_2WINDOWCAMRIGHT:
			DoWindowPosition(13);
			break;
// <---sikk

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
	HDC		maindc;
	LPTOOLTIPTEXT	lpToolTipText;
	char	szToolTip[128];
    time_t	lTime;
	int		i, nBandIndex;	// sikk - Save Rebar Band Info

	GetClientRect(hWnd, &rect);

	switch (uMsg)
	{
	case WM_TIMER:	// 1/sec
		QE_CountBrushesAndUpdateStatusBar();
		QE_CheckAutoSave();
		return 0;

	case WM_DESTROY:
		SaveMruInReg(g_qeglobals.d_lpMruMenu, QE3_WIN_REGISTRY_MRU);
		DeleteMruMenu(g_qeglobals.d_lpMruMenu);
		PostQuitMessage(0);
		KillTimer(hWnd, QE_TIMER0);
		return 0;

	case WM_CREATE:
		maindc = GetDC(hWnd);
//		QEW_SetupPixelFormat(maindc, false);
		g_qeglobals.d_lpMruMenu = CreateMruMenuDefault();
		LoadMruInReg(g_qeglobals.d_lpMruMenu, QE3_WIN_REGISTRY_MRU);
	
		// Refresh the File menu.
		PlaceMenuMRUItem(g_qeglobals.d_lpMruMenu, GetSubMenu(GetMenu(hWnd), 0), ID_FILE_EXIT);
		return 0;

	case WM_SIZE:
		// resize the status window
		MoveWindow(g_qeglobals.d_hwndStatus, -100, 100, 10, 10, true);
		return 0;

	case WM_KEYDOWN:
		return QE_KeyDown(wParam);

   	case WM_CLOSE:
		/* call destroy window to cleanup and go away */
		if (!ConfirmModified())
			return TRUE;

		SaveWindowState(g_qeglobals.d_hwndXYZ[0],		"XYZWindow0");
		SaveWindowState(g_qeglobals.d_hwndXYZ[1],		"XYZWindow1");	// sikk - Multiple Orthographic Views
		SaveWindowState(g_qeglobals.d_hwndXYZ[2],		"XYZWindow2");	// sikk - Multiple Orthographic Views
		SaveWindowState(g_qeglobals.d_hwndCamera,		"CameraWindow");
		SaveWindowState(g_qeglobals.d_hwndZ,			"ZWindow");
		SaveWindowState(g_qeglobals.d_hwndInspector,	"EntityWindow");
		SaveWindowState(g_qeglobals.d_hwndMain,			"MainWindow");

// sikk---> Save Rebar Band Info
		for (i = 0; i < 11; i++)
		{
			nBandIndex = SendMessage(g_qeglobals.d_hwndRebar, RB_IDTOINDEX, (WPARAM)ID_TOOLBAR + i, (LPARAM)0);
			Sys_Printf("Band %d\n", nBandIndex);
			g_qeglobals.d_savedinfo.rbiSettings[i].cbSize	= sizeof(REBARBANDINFO);
			g_qeglobals.d_savedinfo.rbiSettings[i].fMask	= RBBIM_CHILDSIZE | RBBIM_STYLE;
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
		strcpy(g_qeglobals.d_savedinfo.szLastMap, g_szCurrentMap); // sikk - save current map name for Load Last Map option
		SaveRegistryInfo("SavedInfo", &g_qeglobals.d_savedinfo, sizeof(g_qeglobals.d_savedinfo));

		time(&lTime);	// sikk - Print current time for logging purposes
		Sys_Printf("\nSession Stopped: %s", ctime(&lTime));

		DestroyWindow(hWnd);
		return 0;

	case WM_COMMAND:
		return CommandHandler(hWnd, wParam, lParam);

	case WM_INITMENUPOPUP :
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

// sikk---> Mousewheel Handling (All Windows)
	default:
		if (uMsg == g_unMouseWheel)
		{
			int		fwKeys = LOWORD(wParam);
			short	zDelta = (short)HIWORD(wParam);
			HWND	hwndTarget;
			POINT	point;

			point.x = (short)LOWORD(lParam);
			point.y = (short)HIWORD(lParam);
			hwndTarget = ChildWindowFromPoint(g_qeglobals.d_hwndMain, point);

			if (hwndTarget == g_qeglobals.d_hwndInspector && g_qeglobals.d_nInspectorMode == W_TEXTURE)
			{
				if (zDelta < 0)
					g_qeglobals.d_texturewin.originy -= 64;
				else
					g_qeglobals.d_texturewin.originy += 64;

				if (g_qeglobals.d_texturewin.originy > 0)
					g_qeglobals.d_texturewin.originy = 0;

				Sys_UpdateWindows(W_TEXTURE);
			}
			if (hwndTarget == g_qeglobals.d_hwndCamera)
			{
				if (zDelta < 0)
					VectorMA(g_qeglobals.d_camera.origin, -64, g_qeglobals.d_camera.forward, g_qeglobals.d_camera.origin);
				else
					VectorMA(g_qeglobals.d_camera.origin, 64, g_qeglobals.d_camera.forward, g_qeglobals.d_camera.origin);

				Sys_UpdateWindows(W_ALL);
			}
			if (hwndTarget == g_qeglobals.d_hwndXYZ[0])
			{
				if (zDelta < 0)
				{
					g_qeglobals.d_xyz[0].scale *= 4.0f / 5;
					if (g_qeglobals.d_xyz[0].scale < 0.05)
						g_qeglobals.d_xyz[0].scale = 0.05f;
					Sys_UpdateWindows(W_XY);
				}
				else
				{
					g_qeglobals.d_xyz[0].scale *= 5.0 / 4;
					if (g_qeglobals.d_xyz[0].scale > 32)
						g_qeglobals.d_xyz[0].scale = 32;
					Sys_UpdateWindows(W_XY);
				}
			}
			if (hwndTarget == g_qeglobals.d_hwndXYZ[2])
			{
				if (zDelta < 0)
				{
					g_qeglobals.d_xyz[2].scale *= 4.0f / 5;
					if (g_qeglobals.d_xyz[2].scale < 0.05)
						g_qeglobals.d_xyz[2].scale = 0.05f;
					Sys_UpdateWindows(W_XY);
				}
				else
				{
					g_qeglobals.d_xyz[2].scale *= 5.0 / 4;
					if (g_qeglobals.d_xyz[2].scale > 32)
						g_qeglobals.d_xyz[2].scale = 32;
					Sys_UpdateWindows(W_XY);
				}
			}
			if (hwndTarget == g_qeglobals.d_hwndXYZ[1])
			{
				if (zDelta < 0)
				{
					g_qeglobals.d_xyz[1].scale *= 4.0f / 5;
					if (g_qeglobals.d_xyz[1].scale < 0.05)
						g_qeglobals.d_xyz[1].scale = 0.05f;
					Sys_UpdateWindows(W_XY);
				}
				else
				{
					g_qeglobals.d_xyz[1].scale *= 5.0 / 4;
					if (g_qeglobals.d_xyz[1].scale > 32)
						g_qeglobals.d_xyz[1].scale = 32;
					Sys_UpdateWindows(W_XY);
				}
			}
			if (hwndTarget == g_qeglobals.d_hwndZ)
			{
				if (zDelta < 0)
				{
					g_qeglobals.d_z.scale *= 4.0f / 5;
					if (g_qeglobals.d_z.scale < 0.05)
						g_qeglobals.d_z.scale = 0.05f;
					Sys_UpdateWindows(W_Z);
				}
				else
				{
					g_qeglobals.d_z.scale *= 5.0 / 4;
					if (g_qeglobals.d_z.scale > 32)
						g_qeglobals.d_z.scale = 32;
					Sys_UpdateWindows(W_Z);
				}
			}
		}
// <---sikk

	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

/*
==============
WMain_Create
==============
*/
void WMain_Create (HINSTANCE hInstance)
{
	WNDCLASS	wc;
	int			i;
	HMENU		hMenu;
	
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

	// create a timer so that we can count brushes
	SetTimer(g_qeglobals.d_hwndMain, QE_TIMER0, 1000, NULL);
	
	LoadWindowState(g_qeglobals.d_hwndMain, "MainWindow");

	g_qeglobals.d_hwndRebar		= CreateReBar(g_qeglobals.d_hwndMain, hInstance);
	g_qeglobals.d_hwndToolbar1	= CreateToolBar(g_qeglobals.d_hwndRebar, hInstance, 0, 0, 7, 146);
	g_qeglobals.d_hwndToolbar2	= CreateToolBar(g_qeglobals.d_hwndRebar, hInstance, 1, 7, 16, 308);
	g_qeglobals.d_hwndToolbar3	= CreateToolBar(g_qeglobals.d_hwndRebar, hInstance, 2, 23, 6, 144);
	g_qeglobals.d_hwndToolbar4	= CreateToolBar(g_qeglobals.d_hwndRebar, hInstance, 3, 29, 4, 96);
	g_qeglobals.d_hwndToolbar5	= CreateToolBar(g_qeglobals.d_hwndRebar, hInstance, 4, 33, 3, 72);
	g_qeglobals.d_hwndToolbar6	= CreateToolBar(g_qeglobals.d_hwndRebar, hInstance, 5, 36, 7, 148);
	g_qeglobals.d_hwndToolbar7	= CreateToolBar(g_qeglobals.d_hwndRebar, hInstance, 6, 43, 3, 72);
	g_qeglobals.d_hwndToolbar8	= CreateToolBar(g_qeglobals.d_hwndRebar, hInstance, 7, 46, 3, 72);
	g_qeglobals.d_hwndToolbar9	= CreateToolBar(g_qeglobals.d_hwndRebar, hInstance, 8, 49, 2, 48);
	g_qeglobals.d_hwndToolbar10	= CreateToolBar(g_qeglobals.d_hwndRebar, hInstance, 9, 51, 13, 256);
	g_qeglobals.d_hwndToolbar11	= CreateToolBar(g_qeglobals.d_hwndRebar, hInstance, 10, 64, 2, 48);
	/*
	g_qeglobals.d_hwndRebar = CreateToolBar(g_qeglobals.d_hwndMain, hInstance, 0, 0, 0, 0);
	*/
	g_qeglobals.d_hwndStatus	= CreateStatusBar(g_qeglobals.d_hwndMain);

	//
	// load misc info from registry
	//
	i = sizeof(g_qeglobals.d_savedinfo);
	LoadRegistryInfo("SavedInfo", &g_qeglobals.d_savedinfo, &i);
	
	if (g_qeglobals.d_savedinfo.nSize != sizeof(g_qeglobals.d_savedinfo))
	{
		// fill in new defaults
		g_qeglobals.d_savedinfo.nSize				= sizeof(g_qeglobals.d_savedinfo);
		g_qeglobals.d_savedinfo.nTexMenu			= ID_TEXTURES_TRILINEAR;

		g_qeglobals.d_savedinfo.bShow_XZ			= false;	// sikk - Multiple Orthographic Views
		g_qeglobals.d_savedinfo.bShow_YZ			= false;	// sikk - Multiple Orthographic Views
		g_qeglobals.d_savedinfo.bShow_Z				= true;		// Saved Window Toggle

		g_qeglobals.d_savedinfo.nExclude			= 0;
		g_qeglobals.d_savedinfo.bShow_Axis			= true;		// sikk - Show Axis
		g_qeglobals.d_savedinfo.bShow_Blocks		= false;
		g_qeglobals.d_savedinfo.bShow_CameraGrid	= true;		// sikk - Show Camera Grid
		g_qeglobals.d_savedinfo.bShow_Coordinates	= true;
		g_qeglobals.d_savedinfo.bShow_LightRadius	= false;	// sikk - Show Light Radius
		g_qeglobals.d_savedinfo.bShow_MapBoundry	= true;		// sikk - Show Map Boundry Box
		g_qeglobals.d_savedinfo.bShow_Names			= true;
		g_qeglobals.d_savedinfo.bShow_SizeInfo		= true;
		g_qeglobals.d_savedinfo.bShow_Viewname		= true;		// sikk - Show View Name
		g_qeglobals.d_savedinfo.bShow_Workzone		= false;
		g_qeglobals.d_savedinfo.bNoClamp			= false;
		g_qeglobals.d_savedinfo.bScaleLockX			= false;
		g_qeglobals.d_savedinfo.bScaleLockY			= false;
		g_qeglobals.d_savedinfo.bScaleLockZ			= false;
		g_qeglobals.d_savedinfo.bCubicClip			= false;	// sikk - Cubic Clipping
		g_qeglobals.d_savedinfo.nCubicScale			= 32;		// sikk - Cubic Clipping
		g_qeglobals.d_savedinfo.nCameraSpeed		= 1024;		// sikk - Camera Speed Trackbar
// sikk---> Preferences Dialog
		g_qeglobals.d_savedinfo.bAutosave			= true;
		g_qeglobals.d_savedinfo.bRadiantLights		= true;
		g_qeglobals.d_savedinfo.bVertexSplitsFace	= true;
		g_qeglobals.d_savedinfo.nAutosave			= 5;
		g_qeglobals.d_savedinfo.nMapSize			= 8192;
		g_qeglobals.d_savedinfo.nUndoLevels			= 32;
		g_qeglobals.d_savedinfo.fGamma				= 1.0;
		strcpy(g_qeglobals.d_savedinfo.szHeapsize, "16384");
		strcpy(g_qeglobals.d_savedinfo.szSkill, "1");
// <---sikk
		for (i = 0; i < 3; i++)
		{
			g_qeglobals.d_savedinfo.v3Colors[COLOR_BRUSHES][i]		= 0.0f;
			g_qeglobals.d_savedinfo.v3Colors[COLOR_CAMERABACK][i]	= 0.25f;
			g_qeglobals.d_savedinfo.v3Colors[COLOR_CAMERAGRID][i]	= 0.2f;
			g_qeglobals.d_savedinfo.v3Colors[COLOR_GRIDBACK][i]		= 1.0f;
			g_qeglobals.d_savedinfo.v3Colors[COLOR_GRIDMAJOR][i]	= 0.5f;
			g_qeglobals.d_savedinfo.v3Colors[COLOR_GRIDMINOR][i]	= 0.75f;
			g_qeglobals.d_savedinfo.v3Colors[COLOR_GRIDTEXT][i]		= 0.0f;
			g_qeglobals.d_savedinfo.v3Colors[COLOR_MAPBOUNDRY][i]	= 0.0f;
			g_qeglobals.d_savedinfo.v3Colors[COLOR_TEXTUREBACK][i]	= 0.25f;
			g_qeglobals.d_savedinfo.v3Colors[COLOR_TEXTURETEXT][i]	= 0.0f;
		}

		g_qeglobals.d_savedinfo.v3Colors[COLOR_CLIPPER][0]		= 0.0f;
		g_qeglobals.d_savedinfo.v3Colors[COLOR_CLIPPER][1]		= 0.0f;
		g_qeglobals.d_savedinfo.v3Colors[COLOR_CLIPPER][2]		= 1.0f;

		g_qeglobals.d_savedinfo.v3Colors[COLOR_GRIDBLOCK][0]	= 0.0f;
		g_qeglobals.d_savedinfo.v3Colors[COLOR_GRIDBLOCK][1]	= 0.0f;
		g_qeglobals.d_savedinfo.v3Colors[COLOR_GRIDBLOCK][2]	= 1.0f;

		g_qeglobals.d_savedinfo.v3Colors[COLOR_SELBRUSHES][0]	= 1.0f;
		g_qeglobals.d_savedinfo.v3Colors[COLOR_SELBRUSHES][1]	= 0.0f;
		g_qeglobals.d_savedinfo.v3Colors[COLOR_SELBRUSHES][2]	= 0.0f;

		g_qeglobals.d_savedinfo.v3Colors[COLOR_VIEWNAME][0]		= 0.5f;
		g_qeglobals.d_savedinfo.v3Colors[COLOR_VIEWNAME][1]		= 0.0f;
		g_qeglobals.d_savedinfo.v3Colors[COLOR_VIEWNAME][2]		= 0.75f;

		// sikk - Set window positions to QE3 Default
		PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_WINDOW_QE3DEFAULT, 0);
	}

	if ((hMenu = GetMenu(g_qeglobals.d_hwndMain)) != 0)
	{
		/*
		** by default all of these are checked because that's how they're defined in the menu editor
		*/
		if (!g_qeglobals.d_savedinfo.bShow_Axis)	// sikk - Show Axis
			CheckMenuItem(hMenu, ID_VIEW_SHOWAXIS, MF_UNCHECKED);
		if (g_qeglobals.d_savedinfo.bShow_Blocks)	// sikk - Show Blocks moved to savedinfo_t
			CheckMenuItem(hMenu, ID_VIEW_SHOWBLOCKS, MF_CHECKED);
		if (!g_qeglobals.d_savedinfo.bShow_CameraGrid)	// sikk - Show Camera Grid
			CheckMenuItem(hMenu, ID_VIEW_SHOWCAMERAGRID, MF_UNCHECKED);
		if (!g_qeglobals.d_savedinfo.bShow_Coordinates)
			CheckMenuItem(hMenu, ID_VIEW_SHOWCOORDINATES, MF_UNCHECKED);
		if (g_qeglobals.d_savedinfo.bShow_LightRadius)	// sikk - Show Light Radius
			CheckMenuItem(hMenu, ID_VIEW_SHOWLIGHTRADIUS, MF_CHECKED);
		if (!g_qeglobals.d_savedinfo.bShow_MapBoundry)	// sikk - Show Map Boundry
			CheckMenuItem(hMenu, ID_VIEW_SHOWMAPBOUNDRY, MF_UNCHECKED);
		if (!g_qeglobals.d_savedinfo.bShow_Names)
			CheckMenuItem(hMenu, ID_VIEW_SHOWNAMES, MF_UNCHECKED);
		if (!g_qeglobals.d_savedinfo.bShow_SizeInfo)
			CheckMenuItem(hMenu, ID_VIEW_SHOWSIZEINFO, MF_UNCHECKED);
		if (!g_qeglobals.d_savedinfo.bShow_Viewname)
			CheckMenuItem(hMenu, ID_VIEW_SHOWVIEWNAME, MF_UNCHECKED);
		if (g_qeglobals.d_savedinfo.bShow_Workzone)	// sikk - Show Workzone moved to savedinfo_t
			CheckMenuItem(hMenu, ID_VIEW_SHOWWORKZONE, MF_CHECKED);

		if (g_qeglobals.d_savedinfo.nExclude & EXCLUDE_ANGLES)
			CheckMenuItem(hMenu, ID_VIEW_SHOWANGLES, MF_UNCHECKED);
		if (g_qeglobals.d_savedinfo.nExclude & EXCLUDE_LIGHTS)
			CheckMenuItem(hMenu, ID_VIEW_SHOWLIGHTS, MF_UNCHECKED);
		if (g_qeglobals.d_savedinfo.nExclude & EXCLUDE_ENT)
			CheckMenuItem(hMenu, ID_VIEW_ENTITY, MF_UNCHECKED);
		if (g_qeglobals.d_savedinfo.nExclude & EXCLUDE_PATHS)
			CheckMenuItem(hMenu, ID_VIEW_SHOWPATH, MF_UNCHECKED);
		if (g_qeglobals.d_savedinfo.nExclude & EXCLUDE_WATER)
			CheckMenuItem(hMenu, ID_VIEW_SHOWWATER, MF_UNCHECKED);
		if (g_qeglobals.d_savedinfo.nExclude & EXCLUDE_WORLD)
			CheckMenuItem(hMenu, ID_VIEW_SHOWWORLD, MF_UNCHECKED);
		if (g_qeglobals.d_savedinfo.nExclude & EXCLUDE_CLIP)
			CheckMenuItem(hMenu, ID_VIEW_SHOWCLIP, MF_UNCHECKED);
		if (g_qeglobals.d_savedinfo.nExclude & EXCLUDE_HINT)
			CheckMenuItem(hMenu, ID_VIEW_SHOWHINT, MF_UNCHECKED);
		if (g_qeglobals.d_savedinfo.nExclude & EXCLUDE_DETAIL)
			CheckMenuItem(hMenu, ID_VIEW_SHOWDETAIL, MF_UNCHECKED);
		if (g_qeglobals.d_savedinfo.bNoClamp)
			CheckMenuItem(hMenu, ID_GRID_SNAPTOGRID, MF_UNCHECKED);

		if (g_qeglobals.d_savedinfo.bScaleLockX)
		{
			CheckMenuItem(hMenu, ID_SELECTION_SCALELOCKX, MF_CHECKED);
			SendMessage(g_qeglobals.d_hwndToolbar6, TB_CHECKBUTTON, (WPARAM)ID_SELECTION_SCALELOCKX, (g_qeglobals.d_savedinfo.bScaleLockX ? (LPARAM)true : (LPARAM)false));
		}
		if (g_qeglobals.d_savedinfo.bScaleLockY)
		{
			CheckMenuItem(hMenu, ID_SELECTION_SCALELOCKY, MF_CHECKED);
			SendMessage(g_qeglobals.d_hwndToolbar6, TB_CHECKBUTTON, (WPARAM)ID_SELECTION_SCALELOCKY, (g_qeglobals.d_savedinfo.bScaleLockY ? (LPARAM)true : (LPARAM)false));
		}
		if (g_qeglobals.d_savedinfo.bScaleLockZ)
		{
			CheckMenuItem(hMenu, ID_SELECTION_SCALELOCKZ, MF_CHECKED);
			SendMessage(g_qeglobals.d_hwndToolbar6, TB_CHECKBUTTON, (WPARAM)ID_SELECTION_SCALELOCKZ, (g_qeglobals.d_savedinfo.bScaleLockZ ? (LPARAM)true : (LPARAM)false));
		}

		if (g_qeglobals.d_savedinfo.bCubicClip)
		{
			CheckMenuItem(hMenu, ID_VIEW_CUBICCLIP, MF_CHECKED);
			SendMessage(g_qeglobals.d_hwndToolbar7, TB_CHECKBUTTON, (WPARAM)ID_VIEW_CUBICCLIP, (g_qeglobals.d_savedinfo.bCubicClip ? (LPARAM)true : (LPARAM)false));
		}
	}

	ShowWindow(g_qeglobals.d_hwndMain, SW_SHOWMAXIMIZED);	// sikk - changed from "SW_SHOWDEFAULT" (personal preference)
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

// sikk---> Preferences: Reset Registry
	if (g_qeglobals.d_bResetRegistry)
	{
		RegDeleteKey(HKEY_CURRENT_USER, QE3_WIN_REGISTRY_MRU);
		RegDeleteKey(HKEY_CURRENT_USER, QE3_WIN_REGISTRY);
		return FALSE;
	}
// <---sikk

	lres = RegCreateKeyEx(HKEY_CURRENT_USER, QE3_WIN_REGISTRY, 0, NULL,
			REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeyId, &dwDisp);

	if (lres != ERROR_SUCCESS)
		return FALSE;


	lres = RegSetValueEx(hKeyId, pszName, 0, REG_BINARY, pvBuf, lSize);

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

	lres = RegQueryValueEx(hKey, pszName, NULL, &lType, pvBuf, plSize);

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
//	extern int	g_nNumBrushes, g_nNumEntities, g_nNumTextures;
	char		numbrushbuffer[128] = "";

	sprintf(numbrushbuffer, "Brushes: %d  Entities: %d  Textures: %d", g_nNumBrushes, 
																	   g_nNumEntities, 
																	   g_nNumTextures);
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

	sprintf(gridstatus, "Grid Size: %d  Grid Snap: %s  Clip Mode: %s  Texture Lock: %s  Cubic Clip: %d", 
			g_qeglobals.d_nGridSize, 
			g_qeglobals.d_savedinfo.bNoClamp ? "OFF" : "On", 
			g_qeglobals.d_bClipMode ? "On" : "Off", 
			g_qeglobals.d_bTextureLock ? "On" : "Off",
			g_qeglobals.d_savedinfo.nCubicScale);	// sikk - Cubic Clipping
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
	int partsize[4] = { 192, 640, 960, -1 };	// EER //sikk - adjusted sizes

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
