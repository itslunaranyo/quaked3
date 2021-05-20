//==============================
//	WndMain_Status.cpp
//==============================

#include "pre.h"
#include "qe3.h"
#include "WndMain.h"
#include "map.h"
#include "Tool.h"

/*
===============================================================

	STATUS BAR

===============================================================
*/

HWND g_hwndStatus;

/*
0: help/mouseover/etc
1: grid
2: brush
3: selection
*/

/*
==============
WndMain_Status
==============
*/
void WndMain_Status(const char *psz, int part)
{
	SendMessage(g_hwndStatus, SB_SETTEXT, part, (LPARAM)psz);
}

/*
==============
WndMain_UpdateBrushStatusBar
==============
*/
void WndMain_UpdateBrushStatusBar()
{
	char		numbrushbuffer[128] = "";

	sprintf(numbrushbuffer, "Brushes: %d  Entities: %d  Textures: %d",
		g_map.numBrushes, g_map.numEntities, g_map.numTextures);
	WndMain_Status(numbrushbuffer, 2);
}

/*
==============
WndMain_UpdateGridStatusBar
==============
*/
void WndMain_UpdateGridStatusBar()
{
	char gridstatus[128] = "";

	sprintf(gridstatus, "Grid Size: %d  Grid Snap: %s  Tool: %s  Texture Lock: %s  Cubic Clip: %d",
		g_qeglobals.d_nGridSize,
		g_qeglobals.bGridSnap ? "On" : "OFF",
		Tool::stack.back()->modal ? Tool::stack.back()->name : "None",
		g_qeglobals.d_bTextureLock ? "On" : "Off",
		g_cfgEditor.CubicClip ? (int)g_cfgEditor.CubicScale * 64 : 0
		);
	WndMain_Status(gridstatus, 1);
}

/*
==============
WndMain_CreateStatusBar
==============
*/
void WndMain_CreateStatusBar(HWND hWnd)
{
	HWND hwndSB;
	int partsize[4] = { 256, 720, 960, -1 };

	hwndSB = CreateStatusWindow(
		QE3_STATUSBAR_STYLE,	// window styles
		"Ready", 				// text for first pane 
		hWnd, 					// parent window
		ID_STATUSBAR);			// window ID


	SendMessage(hwndSB, SB_SETPARTS, 4, (long)partsize); // EER

	g_hwndStatus = hwndSB;
}



