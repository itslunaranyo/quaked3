//==============================
//	WndMain_Menu.cpp
//==============================

#include "pre.h"
#include "qe3.h"
#include "WndMain.h"
#include "WndFiles.h"
#include "win_dlg.h"

#include "points.h"
#include "csg.h"
#include "mru.h"

#include "CameraView.h"
#include "TextureView.h"
#include "GridView.h"
#include "ZView.h"
#include "WndCamera.h"
#include "WndGrid.h"
#include "WndZChecker.h"
#include "WndEntity.h"
#include "WndConsole.h"
#include "WndSurf.h"
#include "WndConfig.h"

#include "Command.h"
#include "map.h"
#include "select.h"
#include "transform.h"
#include "modify.h"

#include "Tool.h"
#include "ClipTool.h"
#include "GeoTool.h"
#include "PolyTool.h"


/*
==============================================================================

	MENUS

==============================================================================
*/

//#define	MAX_TEXTUREDIRS	128
//int		g_nTextureNumMenus;
//char	g_szTextureMenuNames[MAX_TEXTUREDIRS][64];
LPMRUMENU   g_lpMruMenu;

/*
==============
WndMain_Command

handle all WM_COMMAND messages here
==============
*/
LONG WINAPI WndMain_Command(
	HWND	hWnd,
	WPARAM	wParam,
	LPARAM	lParam
)
{
	HMENU hMenu = GetMenu(hWnd);

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
		Dlg_MapOpen();
		break;
	case ID_FILE_SAVE:
		QE_SaveMap();
		break;
	case ID_FILE_SAVEAS:
		if (Dlg_MapSaveAs())
			QE_SaveMap();
		break;
	case ID_FILE_IMPORTMAP:	// sikk - Import Map Dialog
		Dlg_MapImport();
		break;

	case ID_FILE_POINTFILE:
		if (g_qeglobals.d_nPointfileDisplayList)
			Pointfile_Clear();
		else
			Pointfile_Check();
		break;

	case ID_SELECTION_EXPORTMAP:	// sikk - Export Selection Dialog
		Dlg_MapExport();
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
		WndMain_UpdateMRU(LOWORD(wParam) - IDMRU);
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
		if (!g_wndEntity->TryCut())
			g_map.Cut();
		break;
	case ID_EDIT_COPY:
		if (g_wndEntity->TryCopy()) break;
		if (g_wndConsole->TryCopy()) break;
		g_map.Copy();
		break;
	case ID_EDIT_PASTE:
		if (!g_wndEntity->TryPaste())
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
	case ID_VIEW_TOOLBAR_FILEBAND:
	case ID_VIEW_TOOLBAR_EDITBAND:
	case ID_VIEW_TOOLBAR_WINDOWBAND:
	case ID_VIEW_TOOLBAR_EDIT2BAND:
	case ID_VIEW_TOOLBAR_SELECTBAND:
	case ID_VIEW_TOOLBAR_CSGBAND:
	case ID_VIEW_TOOLBAR_MODEBAND:
	case ID_VIEW_TOOLBAR_ENTITYBAND:
	case ID_VIEW_TOOLBAR_BRUSHBAND:
	case ID_VIEW_TOOLBAR_TEXTUREBAND:
	case ID_VIEW_TOOLBAR_VIEWBAND:
	case ID_VIEW_TOOLBAR_MISCBAND:
	{
		int bandID = LOWORD(wParam) - ID_VIEW_TOOLBAR_FILEBAND;
		int nBandIndex = SendMessage(g_hwndRebar, RB_IDTOINDEX, (WPARAM)ID_TOOLBAR + bandID, (LPARAM)0);
		if (IsWindowVisible(g_hwndToolbar[bandID]))
			SendMessage(g_hwndRebar, RB_SHOWBAND, (WPARAM)nBandIndex, (LPARAM)FALSE);
		else
			SendMessage(g_hwndRebar, RB_SHOWBAND, (WPARAM)nBandIndex, (LPARAM)TRUE);
		break;
	}
	case ID_VIEW_STATUSBAR:
		if (IsWindowVisible(g_hwndStatus))
			DestroyWindow(g_hwndStatus);
		else
		{
			WndMain_CreateStatusBar(g_hwndMain);
			WndMain_UpdateGridStatusBar();
			WndMain_UpdateBrushStatusBar();
		}
		break;

	case ID_VIEW_SHOWAXIS:	// sikk - Show Axis
		g_cfgUI.ShowAxis ^= true;
		WndMain_UpdateWindows(W_XY | W_CAMERA);
		break;
	case ID_VIEW_SHOWBLOCKS:
		g_cfgUI.ShowBlocks ^= true;
		WndMain_UpdateWindows(W_XY);
		break;
	case ID_VIEW_SHOWCAMERAGRID:	// sikk - Camera Grid
		g_cfgUI.ShowCameraGrid ^= true;
		WndMain_UpdateWindows(W_CAMERA);
		break;
	case ID_VIEW_SHOWCOORDINATES:
		g_cfgUI.ShowCoordinates ^= true;
		WndMain_UpdateWindows(W_XY | W_Z);
		break;
	case ID_VIEW_SHOWLIGHTRADIUS:	// sikk - Show Light Radius
		g_cfgUI.ShowLightRadius ^= true;
		WndMain_UpdateWindows(W_XY);
		break;
	case ID_VIEW_SHOWMAPBOUNDARY:	// sikk - Show Map Boundary Box
		g_cfgUI.ShowMapBoundary ^= true;
		WndMain_UpdateWindows(W_CAMERA);
		break;
	case ID_VIEW_SHOWNAMES:
		g_cfgUI.ShowNames ^= true;
		WndMain_UpdateWindows(W_XY);
		break;
	case ID_VIEW_SHOWSIZEINFO:
		g_cfgUI.ShowSizeInfo ^= true;
		WndMain_UpdateWindows(W_XY);
		break;
	case ID_VIEW_SHOWWORKZONE:
		g_cfgUI.ShowWorkzone ^= true;
		WndMain_UpdateWindows(W_XY);
		break;
	case ID_VIEW_SHOWANGLES:
		g_cfgUI.ShowAngles ^= true;
		WndMain_UpdateWindows(W_XY | W_CAMERA);
		break;

	case ID_TARGETLINES_ALL:
		g_cfgUI.PathlineMode = TargetGraph::tgm_all;
		WndMain_UpdateWindows(W_XY | W_CAMERA | W_TARGETGRAPH);
		break;
	case ID_TARGETLINES_SEL:
		g_cfgUI.PathlineMode = TargetGraph::tgm_selected;
		WndMain_UpdateWindows(W_XY | W_CAMERA | W_TARGETGRAPH);
		break;
	case ID_TARGETLINES_SELPATH:
		g_cfgUI.PathlineMode = TargetGraph::tgm_selected_path;
		WndMain_UpdateWindows(W_XY | W_CAMERA | W_TARGETGRAPH);
		break;
	case ID_TARGETLINES_NONE:
		g_cfgUI.PathlineMode = TargetGraph::tgm_none;
		WndMain_UpdateWindows(W_XY | W_CAMERA | W_TARGETGRAPH);
		break;

	case ID_VIEW_SHOWCLIP:
		WndMain_ToggleViewFilter(BFL_CLIP);
		break;
	case ID_VIEW_SHOWPOINTENTS:
		WndMain_ToggleViewFilter(EFL_POINTENTITY);
		break;
	case ID_VIEW_SHOWBRUSHENTS:
		WndMain_ToggleViewFilter(EFL_BRUSHENTITY);
		break;
	case ID_VIEW_SHOWFUNCWALL:
		WndMain_ToggleViewFilter(EFL_FUNCWALL);
		break;
	case ID_VIEW_SHOWLIGHTS:
		WndMain_ToggleViewFilter(EFL_LIGHT);
		break;
	case ID_VIEW_SHOWSKY:
		WndMain_ToggleViewFilter(BFL_SKY);
		break;
	case ID_VIEW_SHOWWATER:
		WndMain_ToggleViewFilter(BFL_LIQUID);
		break;
	case ID_VIEW_SHOWWORLD:
		WndMain_ToggleViewFilter(EFL_WORLDSPAWN);
		break;
	case ID_VIEW_SHOWHINT:
		WndMain_ToggleViewFilter(BFL_HINT);
		break;
	case ID_VIEW_SHOWDETAIL:
		WndMain_ToggleViewFilter(EFL_DETAIL);
		break;
	case ID_VIEW_SHOWMONSTERS:
		WndMain_ToggleViewFilter(EFL_MONSTER);
		break;
	case ID_VIEW_SHOWTRIGGERS:
		WndMain_ToggleViewFilter(EFL_TRIGGER);
		break;

	case ID_VIEW_FILTER_POPUP:
		WndMain_ViewFilterPopup();
		break;

	/*
	"all" all flags 0
	"all singleplayer" none of !e, !m, or !h
	"easy only" not !e
	"normal only" not !m
	"hard only" not !h
	"deathmatch only" not !dm
	*/

	case ID_FILTER_SHOWALLSKILLS:
		g_cfgUI.ViewFilter -= g_cfgUI.ViewFilter & (EFL_EASY | EFL_MEDIUM | EFL_HARD | EFL_DEATHMATCH);
		WndMain_UpdateWindows(W_SCENE | W_TARGETGRAPH | W_ENTITY);
		break;
	case ID_FILTER_SHOWEASYSKILL:
		g_cfgUI.ViewFilter -= g_cfgUI.ViewFilter & (EFL_EASY | EFL_MEDIUM | EFL_HARD | EFL_DEATHMATCH);
		g_cfgUI.ViewFilter |= EFL_EASY;
		WndMain_UpdateWindows(W_SCENE | W_TARGETGRAPH | W_ENTITY);
		break;
	case ID_FILTER_SHOWMEDIUMSKILL:
		g_cfgUI.ViewFilter -= g_cfgUI.ViewFilter & (EFL_EASY | EFL_MEDIUM | EFL_HARD | EFL_DEATHMATCH);
		g_cfgUI.ViewFilter |= EFL_MEDIUM;
		WndMain_UpdateWindows(W_SCENE | W_TARGETGRAPH | W_ENTITY);
		break;
	case ID_FILTER_SHOWHARDSKILL:
		g_cfgUI.ViewFilter -= g_cfgUI.ViewFilter & (EFL_EASY | EFL_MEDIUM | EFL_HARD | EFL_DEATHMATCH);
		g_cfgUI.ViewFilter |= EFL_HARD;
		WndMain_UpdateWindows(W_SCENE | W_TARGETGRAPH | W_ENTITY);
		break;
	case ID_FILTER_SHOWDEATHMATCH:
		g_cfgUI.ViewFilter -= g_cfgUI.ViewFilter & (EFL_EASY | EFL_MEDIUM | EFL_HARD | EFL_DEATHMATCH);
		g_cfgUI.ViewFilter |= EFL_DEATHMATCH;
		WndMain_UpdateWindows(W_SCENE | W_TARGETGRAPH | W_ENTITY);
		break;


	case ID_VIEW_CAMERA:	// sikk - Toggle Camera View
		g_wndCamera->Toggle();
		break;

	case ID_VIEW_ENTITY:
		WndMain_SetInspectorMode(W_ENTITY);
		break;
	case ID_VIEW_CONSOLE:
		WndMain_SetInspectorMode(W_CONSOLE);
		break;
	case ID_VIEW_TEXTURE:
		WndMain_SetInspectorMode(W_TEXTURE);
		break;

	case ID_VIEW_TOGGLE_XY:
		g_wndGrid[0]->Toggle();
		break;
	case ID_VIEW_TOGGLE_XZ:
		g_wndGrid[2]->Toggle();
		break;
	case ID_VIEW_TOGGLE_YZ:
		g_wndGrid[1]->Toggle();
		break;
	case ID_VIEW_TOGGLE_Z:
		g_wndZ->Toggle();
		break;

	case ID_VIEW_CENTER:
		g_vCamera.LevelView();
		WndMain_UpdateWindows(W_XY|W_CAMERA);
		break;
	case ID_VIEW_UPFLOOR:
		g_vCamera.ChangeFloor(true);
		WndMain_UpdateWindows(W_SCENE);
		break;
	case ID_VIEW_DOWNFLOOR:
		g_vCamera.ChangeFloor(false);
		WndMain_UpdateWindows(W_SCENE);
		break;

	case ID_VIEW_CENTERONSELECTION:
		if (Selection::HasBrushes())
		{
			for (int i = 0; i < 4; i++)
				g_vGrid[i].PositionView();
			g_vCamera.PositionCenter();
			g_vZ.PositionCenter();
			WndMain_UpdateWindows(W_SCENE);
		}
		break;

	case ID_VIEW_NEXTVIEW:
		g_wndGrid[0]->CycleAxis();
		break;
	case ID_VIEW_XY:
		g_wndGrid[0]->SetAxis(GRID_XY);
		break;
	case ID_VIEW_XZ:
		g_wndGrid[0]->SetAxis(GRID_XZ);
		break;
	case ID_VIEW_YZ:
		g_wndGrid[0]->SetAxis(GRID_YZ);
		break;
	case ID_VIEW_SWAPGRIDCAM:
		WndMain_SwapGridCam();
		WndMain_UpdateWindows(W_XY | W_CAMERA);
		break;

	// lunaran TODO: 3-grid mode should keep all views synchronized
	case ID_VIEW_100:
		g_vGrid[0].ResetZoom();
		WndMain_UpdateWindows(W_XY);
		break;
	case ID_VIEW_ZOOMIN:
		g_vGrid[0].ZoomIn();
		WndMain_UpdateWindows(W_XY);
		break;
	case ID_VIEW_ZOOMOUT:
		g_vGrid[0].ZoomOut();
		WndMain_UpdateWindows(W_XY);
		break;

	case ID_VIEW_Z100:
		g_vZ.ResetScale();
		WndMain_UpdateWindows(W_Z);
		break;
	case ID_VIEW_ZZOOMIN:
		g_vZ.ScaleUp();
		WndMain_UpdateWindows(W_Z);
		break;
	case ID_VIEW_ZZOOMOUT:
		g_vZ.ScaleDown();
		WndMain_UpdateWindows(W_Z);
		break;

	case ID_VIEW_CAMSPEED:	// sikk - Camera Speed Trackbar in toolbar
		DoCamSpeed();
		break;

		// sikk---> Cubic Clipping
	case ID_VIEW_CUBICCLIP:
		g_cfgEditor.CubicClip ^= true;
		WndMain_UpdateGridStatusBar();
		WndMain_UpdateWindows(W_CAMERA);
		break;
	case ID_VIEW_CUBICCLIPNEARER:
		g_cfgEditor.CubicScale = max((int)g_cfgEditor.CubicScale - 1, 1);
		WndMain_UpdateGridStatusBar();
		WndMain_UpdateWindows(W_CAMERA);
		break;

	case ID_VIEW_CUBICCLIPFARTHER:
		g_cfgEditor.CubicScale = min((int)g_cfgEditor.CubicScale + 1, 64);
		WndMain_UpdateGridStatusBar();
		WndMain_UpdateWindows(W_CAMERA);
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
	// Tools/Operations
	//===================================
	case ID_NUDGE_UP:
	case ID_NUDGE_DOWN:
	case ID_NUDGE_LEFT:
	case ID_NUDGE_RIGHT:
	{
		// translateAccelerator helpfully destroys both the message and record of the message's intended window
		// so NUDGEs directed at a certain window systemically cannot get there :(
		HWND dstHwnd = GetFocus();
		if (dstHwnd != g_hwndGrid[0] &&
			dstHwnd != g_hwndGrid[1] &&
			dstHwnd != g_hwndGrid[2] &&
			dstHwnd != g_hwndGrid[3] &&
			dstHwnd != g_hwndZ)
			dstHwnd = g_hwndCamera;
		SendMessage(dstHwnd, WM_COMMAND, wParam, lParam);
	}
	break;

	case ID_SELECTION_DRAGEDGES:
		GeoTool::ToggleMode(GeoTool::GT_EDGE);
		WndMain_UpdateWindows(W_XY | W_CAMERA);
		break;
	case ID_SELECTION_DRAGVERTICES:
		GeoTool::ToggleMode(GeoTool::GT_VERTEX);
		WndMain_UpdateWindows(W_XY | W_CAMERA);
		break;
	case ID_SELECTION_DRAGFACES:
		GeoTool::ToggleMode(GeoTool::GT_FACE);
		WndMain_UpdateWindows(W_XY | W_CAMERA);
		break;

	case ID_SELECTION_DESELECT:
		if (!Tool::HotTool())
		{
			Tool *mt = Tool::ModalTool();
			if (mt)
			{
				delete mt;
				WndMain_UpdateWindows(W_SCENE);
			}
			else
				Selection::DeselectAll();
		}
		break;
	case ID_SELECTION_DELETE:
		Modify::Delete();
		break;

	case ID_SELECTION_FLIPX:
		Transform_FlipAxis(vec3(1, 0, 0));
		break;
	case ID_SELECTION_FLIPY:
		Transform_FlipAxis(vec3(0, 1, 0));
		break;
	case ID_SELECTION_FLIPZ:
		Transform_FlipAxis(vec3(0, 0, 1));
		break;

	case ID_SELECTION_ROTATEX:
		Transform_RotateAxis(0, WndMain_RotForModifiers(), false);
		break;
	case ID_SELECTION_ROTATEY:
		Transform_RotateAxis(1, WndMain_RotForModifiers(), false);
		break;
	case ID_SELECTION_ROTATEZ:
		Transform_RotateAxis(2, WndMain_RotForModifiers(), false);
		break;
	case ID_SELECTION_ARBITRARYROTATION:
		DoRotate();
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
		if (Selection::g_selMode != sel_brush)
		{
			Selection::DeselectAllFaces();
			Selection::g_selMode = sel_brush;
			WndMain_UpdateWindows(W_XY | W_CAMERA);
		}
		if (dynamic_cast<ClipTool*>(Tool::stack.back()))
			delete Tool::stack.back();
		else
			new ClipTool();
		break;

	case ID_TOOLS_DRAWBRUSHESTOOL:
		if (dynamic_cast<PolyTool*>(Tool::stack.back()))
			delete Tool::stack.back();
		else
			new PolyTool();
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

	case ID_BRUSH_CYLINDER:
		DoSides(0);
		break;
	case ID_BRUSH_CONE:
		DoSides(1);
		break;
	case ID_BRUSH_SPHERE:
		DoSides(2);
		break;
	case ID_PRIMITIVES_CZGCYLINDER1:
		Modify::MakeCzgCylinder(1);
		break;
	case ID_PRIMITIVES_CZGCYLINDER2:
		Modify::MakeCzgCylinder(2);
		break;

	/*
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
	*/

	//===================================
	// Grid Menu
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
		WndMain_UpdateWindows(W_XY | W_Z);
		WndMain_UpdateGridStatusBar();
		break;

	case ID_GRID_TOGGLE:	// sikk - Gridd Toggle
		g_qeglobals.d_bShowGrid ^= true;
		//PostMessage(g_hwndGrid[0], WM_PAINT, 0, 0);
		WndMain_UpdateWindows(W_Z | W_XY);
		break;

	case ID_GRID_SNAPTOGRID:
		g_qeglobals.bGridSnap ^= true;
		WndMain_UpdateGridStatusBar();
		break;


	//===================================
	// Texture menu
	// more in texturetool.cpp
	//===================================
	case ID_TEXTURES_INSPECTOR:
		WndSurf_Create();
		break;

	case ID_DRAWMODE_WIREFRAME:
		g_cfgUI.DrawMode = CD_WIRE;
		Textures::SetDrawMode(CD_WIRE);
		g_map.BuildBrushData();
		WndMain_UpdateWindows(W_CAMERA);
		break;
	case ID_DRAWMODE_FLATSHADE:
		g_cfgUI.DrawMode = CD_FLAT;
		Textures::SetDrawMode(CD_FLAT);
		g_map.BuildBrushData();
		WndMain_UpdateWindows(W_CAMERA);
		break;
	case ID_DRAWMODE_TEXTURED:
		g_cfgUI.DrawMode = CD_TEXTURED;
		Textures::SetDrawMode(CD_TEXTURED);
		g_map.BuildBrushData();
		WndMain_UpdateWindows(W_CAMERA);
		break;


	//===================================
	// Misc menu
	//===================================
	case ID_MISC_NEXTLEAKSPOT:
		Pointfile_Next();
		break;
	case ID_MISC_PREVIOUSLEAKSPOT:
		Pointfile_Prev();
		break;

	case ID_MISC_SELECTENTITYCOLOR:
		g_wndEntity->SelectEntityColor();
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
	// Window menu
	//===================================
	case ID_WINDOW_QE3DEFAULT:
		WndMain_DefaultLayout(0);
		break;
	case ID_WINDOW_QE3REVERSE:
		WndMain_DefaultLayout(1);
		break;
	case ID_WINDOW_4WINDOWZCAMLEFT:
		WndMain_DefaultLayout(2);
		break;
	case ID_WINDOW_4WINDOWZCAMRIGHT:
		WndMain_DefaultLayout(3);
		break;
	case ID_WINDOW_4WINDOWNOZCAMLEFT:
		WndMain_DefaultLayout(4);
		break;
	case ID_WINDOW_4WINDOWNOZCAMRIGHT:
		WndMain_DefaultLayout(5);
		break;
	case ID_WINDOW_4REVERSEZCAMLEFT:
		WndMain_DefaultLayout(6);
		break;
	case ID_WINDOW_4REVERSEZCAMRIGHT:
		WndMain_DefaultLayout(7);
		break;
	case ID_WINDOW_4REVERSENOZCAMLEFT:
		WndMain_DefaultLayout(8);
		break;
	case ID_WINDOW_4REVERSENOZCAMRIGHT:
		WndMain_DefaultLayout(9);
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
	WndMain_UpdateMenu();

	return TRUE;
}

/*
==================
WndMain_UpdateMenu
==================
*/
#pragma warning(disable : 4800)     // shutup int to bool conversion warning
void WndMain_UpdateMenuFilters(HMENU hMenu)
{
	WndMain_CheckMenuItem(hMenu, ID_VIEW_SHOWAXIS, g_cfgUI.ShowAxis);
	WndMain_CheckMenuItem(hMenu, ID_VIEW_SHOWBLOCKS, g_cfgUI.ShowBlocks);
	WndMain_CheckMenuItem(hMenu, ID_VIEW_SHOWCAMERAGRID, g_cfgUI.ShowCameraGrid);
	WndMain_CheckMenuItem(hMenu, ID_VIEW_SHOWCOORDINATES, g_cfgUI.ShowCoordinates);
	WndMain_CheckMenuItem(hMenu, ID_VIEW_SHOWLIGHTRADIUS, g_cfgUI.ShowLightRadius);
	WndMain_CheckMenuItem(hMenu, ID_VIEW_SHOWMAPBOUNDARY, g_cfgUI.ShowMapBoundary);
	WndMain_CheckMenuItem(hMenu, ID_VIEW_SHOWNAMES, g_cfgUI.ShowNames);
	WndMain_CheckMenuItem(hMenu, ID_VIEW_SHOWSIZEINFO, g_cfgUI.ShowSizeInfo);
	WndMain_CheckMenuItem(hMenu, ID_VIEW_SHOWWORKZONE, g_cfgUI.ShowWorkzone);
	WndMain_CheckMenuItem(hMenu, ID_VIEW_SHOWANGLES, g_cfgUI.ShowAngles);

	WndMain_CheckMenuItem(hMenu, ID_TEXTURES_HIDEUNUSED, g_cfgUI.HideUnusedTextures);

	WndMain_CheckMenuItem(hMenu, ID_TARGETLINES_ALL, g_cfgUI.PathlineMode == TargetGraph::tgm_all);
	WndMain_CheckMenuItem(hMenu, ID_TARGETLINES_SEL, g_cfgUI.PathlineMode == TargetGraph::tgm_selected);
	WndMain_CheckMenuItem(hMenu, ID_TARGETLINES_SELPATH, g_cfgUI.PathlineMode == TargetGraph::tgm_selected_path);
	WndMain_CheckMenuItem(hMenu, ID_TARGETLINES_NONE, g_cfgUI.PathlineMode == TargetGraph::tgm_none);

	WndMain_CheckMenuItem(hMenu, ID_VIEW_SHOWCLIP, !(g_cfgUI.ViewFilter & BFL_CLIP));
	WndMain_CheckMenuItem(hMenu, ID_VIEW_SHOWBRUSHENTS, !(g_cfgUI.ViewFilter & EFL_BRUSHENTITY));
	WndMain_CheckMenuItem(hMenu, ID_VIEW_SHOWPOINTENTS, !(g_cfgUI.ViewFilter & EFL_POINTENTITY));
	WndMain_CheckMenuItem(hMenu, ID_VIEW_SHOWFUNCWALL, !(g_cfgUI.ViewFilter & EFL_FUNCWALL));
	WndMain_CheckMenuItem(hMenu, ID_VIEW_SHOWLIGHTS, !(g_cfgUI.ViewFilter & EFL_LIGHT));
	WndMain_CheckMenuItem(hMenu, ID_VIEW_SHOWSKY, !(g_cfgUI.ViewFilter & BFL_SKY));
	WndMain_CheckMenuItem(hMenu, ID_VIEW_SHOWWATER, !(g_cfgUI.ViewFilter & BFL_LIQUID));
	WndMain_CheckMenuItem(hMenu, ID_VIEW_SHOWWORLD, !(g_cfgUI.ViewFilter & EFL_WORLDSPAWN));
	WndMain_CheckMenuItem(hMenu, ID_VIEW_SHOWHINT, !(g_cfgUI.ViewFilter & BFL_HINT));
	WndMain_CheckMenuItem(hMenu, ID_VIEW_SHOWDETAIL, !(g_cfgUI.ViewFilter & EFL_DETAIL));
	WndMain_CheckMenuItem(hMenu, ID_VIEW_SHOWMONSTERS, !(g_cfgUI.ViewFilter & EFL_MONSTER));
	WndMain_CheckMenuItem(hMenu, ID_VIEW_SHOWTRIGGERS, !(g_cfgUI.ViewFilter & EFL_TRIGGER));

	CheckMenuItem(hMenu, ID_FILTER_SHOWEASYSKILL, MF_UNCHECKED);
	CheckMenuItem(hMenu, ID_FILTER_SHOWMEDIUMSKILL, MF_UNCHECKED);
	CheckMenuItem(hMenu, ID_FILTER_SHOWHARDSKILL, MF_UNCHECKED);
	CheckMenuItem(hMenu, ID_FILTER_SHOWDEATHMATCH, MF_UNCHECKED);
	CheckMenuItem(hMenu, ID_FILTER_SHOWALLSKILLS, MF_UNCHECKED);
	if (g_cfgUI.ViewFilter & EFL_EASY)
		CheckMenuItem(hMenu, ID_FILTER_SHOWEASYSKILL, MF_CHECKED);
	else if (g_cfgUI.ViewFilter & EFL_MEDIUM)
		CheckMenuItem(hMenu, ID_FILTER_SHOWMEDIUMSKILL, MF_CHECKED);
	else if (g_cfgUI.ViewFilter & EFL_HARD)
		CheckMenuItem(hMenu, ID_FILTER_SHOWHARDSKILL, MF_CHECKED);
	else if (g_cfgUI.ViewFilter & EFL_DEATHMATCH)
		CheckMenuItem(hMenu, ID_FILTER_SHOWDEATHMATCH, MF_CHECKED);
	else
		CheckMenuItem(hMenu, ID_FILTER_SHOWALLSKILLS, MF_CHECKED);
}

/*
==================
WndMain_UpdateMenu
==================
*/
void WndMain_UpdateMenu ()
{
	HMENU hMenu = GetMenu(g_hwndMain);

//===================================
// Edit Menu
//===================================
	// lunaran - leave cut/copy/paste always enabled, so they work in the console and other text fields

	// Undo
	EnableMenuItem(hMenu, ID_EDIT_UNDO, g_cmdQueue.CanUndo() ? MF_ENABLED : MF_GRAYED);
	SendMessage(g_hwndToolbar[1], TB_SETSTATE, (WPARAM)ID_EDIT_UNDO, g_cmdQueue.CanUndo() ? (LPARAM)TBSTATE_ENABLED : (LPARAM)TBSTATE_INDETERMINATE);
	// Redo
	EnableMenuItem(hMenu, ID_EDIT_REDO, g_cmdQueue.CanRedo() ? MF_ENABLED : MF_GRAYED);
	SendMessage(g_hwndToolbar[1], TB_SETSTATE, (WPARAM)ID_EDIT_REDO, g_cmdQueue.CanRedo() ? (LPARAM)TBSTATE_ENABLED : (LPARAM)TBSTATE_INDETERMINATE);

//===================================
// View Menu
//===================================
	// Toolbar Bands
	WndMain_CheckMenuItem(hMenu, ID_VIEW_TOOLBAR_FILEBAND,		IsWindowVisible(g_hwndToolbar[0]));
	WndMain_CheckMenuItem(hMenu, ID_VIEW_TOOLBAR_EDITBAND,		IsWindowVisible(g_hwndToolbar[1]));
	WndMain_CheckMenuItem(hMenu, ID_VIEW_TOOLBAR_WINDOWBAND,	IsWindowVisible(g_hwndToolbar[2]));
	WndMain_CheckMenuItem(hMenu, ID_VIEW_TOOLBAR_EDIT2BAND,		IsWindowVisible(g_hwndToolbar[3]));
	WndMain_CheckMenuItem(hMenu, ID_VIEW_TOOLBAR_SELECTBAND,	IsWindowVisible(g_hwndToolbar[4]));
	WndMain_CheckMenuItem(hMenu, ID_VIEW_TOOLBAR_CSGBAND,		IsWindowVisible(g_hwndToolbar[5]));
	WndMain_CheckMenuItem(hMenu, ID_VIEW_TOOLBAR_MODEBAND,		IsWindowVisible(g_hwndToolbar[6]));
	WndMain_CheckMenuItem(hMenu, ID_VIEW_TOOLBAR_ENTITYBAND,	IsWindowVisible(g_hwndToolbar[7]));
	WndMain_CheckMenuItem(hMenu, ID_VIEW_TOOLBAR_BRUSHBAND,		IsWindowVisible(g_hwndToolbar[8]));
	WndMain_CheckMenuItem(hMenu, ID_VIEW_TOOLBAR_TEXTUREBAND,	IsWindowVisible(g_hwndToolbar[9]));
	WndMain_CheckMenuItem(hMenu, ID_VIEW_TOOLBAR_VIEWBAND,		IsWindowVisible(g_hwndToolbar[10]));
	WndMain_CheckMenuItem(hMenu, ID_VIEW_TOOLBAR_MISCBAND,		IsWindowVisible(g_hwndToolbar[11]));

	// Status Bar
	WndMain_CheckMenuItem(hMenu, ID_VIEW_STATUSBAR,	IsWindowVisible(g_hwndStatus));
	// XY Windows
	WndMain_CheckMenuItem(hMenu, ID_VIEW_CAMERA,	IsWindowVisible(g_hwndCamera));
	WndMain_CheckMenuItem(hMenu, ID_VIEW_TOGGLE_XY,	IsWindowVisible(g_hwndGrid[0]));
	WndMain_CheckMenuItem(hMenu, ID_VIEW_TOGGLE_XZ,	IsWindowVisible(g_hwndGrid[2]));
	WndMain_CheckMenuItem(hMenu, ID_VIEW_TOGGLE_YZ,	IsWindowVisible(g_hwndGrid[1]));
	WndMain_CheckMenuItem(hMenu, ID_VIEW_TOGGLE_Z,	IsWindowVisible(g_hwndZ));

	// Cubic Clipping
	WndMain_CheckMenuItem(hMenu, ID_VIEW_CUBICCLIP, g_cfgEditor.CubicClip);
	SendMessage(g_hwndToolbar[10], TB_CHECKBUTTON, (WPARAM)ID_VIEW_CUBICCLIP, (g_cfgEditor.CubicClip ? (LPARAM)TRUE : (LPARAM)FALSE));
	// Filter Commands

	WndMain_UpdateMenuFilters(hMenu);

//===================================
// Selection Menu
//===================================
	{
		bool modeCheck;

		// Clipper Mode
		modeCheck = (dynamic_cast<ClipTool*>(Tool::ModalTool()) != nullptr);
		WndMain_CheckMenuItem(hMenu, ID_SELECTION_CLIPPER, modeCheck);
		SendMessage(g_hwndToolbar[6], TB_CHECKBUTTON, (WPARAM)ID_SELECTION_CLIPPER, (modeCheck ? (LPARAM)TRUE : (LPARAM)FALSE));

		// Drag Edge Mode 
		GeoTool* gt;
		gt = dynamic_cast<GeoTool*>(Tool::ModalTool());
		modeCheck = (gt && (gt->mode & GeoTool::GT_EDGE));
		WndMain_CheckMenuItem(hMenu, ID_SELECTION_DRAGEDGES, modeCheck);
		SendMessage(g_hwndToolbar[6], TB_CHECKBUTTON, (WPARAM)ID_SELECTION_DRAGEDGES, (modeCheck ? (LPARAM)TRUE : (LPARAM)FALSE));

		// Drag Vertex Mode 
		modeCheck = (gt && (gt->mode & GeoTool::GT_VERTEX));
		WndMain_CheckMenuItem(hMenu, ID_SELECTION_DRAGVERTICES, modeCheck);
		SendMessage(g_hwndToolbar[6], TB_CHECKBUTTON, (WPARAM)ID_SELECTION_DRAGVERTICES, (modeCheck ? (LPARAM)TRUE : (LPARAM)FALSE));

		// Drag Face Mode 
		modeCheck = (gt && (gt->mode & GeoTool::GT_FACE));
		WndMain_CheckMenuItem(hMenu, ID_SELECTION_DRAGFACES, modeCheck);
		SendMessage(g_hwndToolbar[6], TB_CHECKBUTTON, (WPARAM)ID_SELECTION_DRAGFACES, (modeCheck ? (LPARAM)TRUE : (LPARAM)FALSE));

		modeCheck = (dynamic_cast<PolyTool*>(Tool::ModalTool()) != nullptr);
		WndMain_CheckMenuItem(hMenu, ID_TOOLS_DRAWBRUSHESTOOL, modeCheck);
		SendMessage(g_hwndToolbar[6], TB_CHECKBUTTON, (WPARAM)ID_TOOLS_DRAWBRUSHESTOOL, (modeCheck ? (LPARAM)TRUE : (LPARAM)FALSE));
	}

//===================================
// Grid Menu
//===================================
	WndMain_CheckMenuItem(hMenu, ID_GRID_TOGGLE, g_qeglobals.d_bShowGrid);
	WndMain_CheckMenuItem(hMenu, ID_GRID_SNAPTOGRID, g_qeglobals.bGridSnap);

//===================================
// Texture Menu
//===================================
	WndMain_CheckMenuItem(hMenu, ID_TEXTURES_LOCK, g_qeglobals.d_bTextureLock);

//===================================
// Region Menu
//===================================
	EnableMenuItem(hMenu, ID_REGION_SETXZ, (g_wndGrid[2]->IsOpen() ? MF_ENABLED : MF_GRAYED));
	EnableMenuItem(hMenu, ID_REGION_SETYZ, (g_wndGrid[1]->IsOpen() ? MF_ENABLED : MF_GRAYED));
}

/*
============
WndMain_ViewFilterPopup
============
*/
void WndMain_ViewFilterPopup()
{
	BOOL	retval;
	POINT	point;

	HMENU	menu = GetSubMenu(LoadMenu(g_qeglobals.d_hInstance, MAKEINTRESOURCE(IDR_POPUP_FILTER)), 0);

	WndMain_UpdateMenuFilters(menu);

	GetCursorPos(&point);

	retval = TrackPopupMenu(menu, TPM_LEFTALIGN | TPM_NONOTIFY | TPM_RETURNCMD | TPM_RIGHTBUTTON, point.x, point.y, 0, g_hwndRebar, NULL);

	PostMessage(g_hwndMain, WM_COMMAND, retval, 0);

	DestroyMenu(menu);
}

/*
==================
WndMain_CheckMenuItem
==================
*/
void WndMain_CheckMenuItem(HMENU hMenu, unsigned item, bool check)
{
	CheckMenuItem(hMenu, item, (check ? MF_CHECKED : MF_UNCHECKED));
}

/*
==================
WndMain_ToggleViewFilter
==================
*/
void WndMain_ToggleViewFilter(int filt)
{
	g_cfgUI.ViewFilter ^= filt;
	WndMain_UpdateWindows(W_SCENE | W_TARGETGRAPH);
}

/*
==================
WndMain_RotForModifiers
==================
*/
int WndMain_RotForModifiers()
{
	if (GetKeyState(VK_SHIFT) < 0) return 180;
	if (GetKeyState(VK_CONTROL) < 0) return -90;
	return 90;
}


/*
==============
WndMain_UpdateMRU

Copied from MSDN
==============
*/
BOOL WndMain_UpdateMRU(WORD wId)
{
	char		szFileName[128];
	OFSTRUCT	of;
	BOOL		fExist;

	// lunaran: save confirmation from MRU
	if (!ConfirmModified())
		return FALSE;

	GetMenuItem(g_lpMruMenu, wId + IDMRU, TRUE, szFileName, sizeof(szFileName));

	// Test if the file exists.
	fExist = OpenFile(szFileName, &of, OF_EXIST) != HFILE_ERROR;

	if (fExist)
	{
		// Place the file on the top of MRU
		AddNewItem(g_lpMruMenu, (LPSTR)szFileName);

		// Now perform opening this file !!!
		g_map.Load(szFileName);
	}
	else
	{
		Log::Warning(_S("%s not found on disk!") << szFileName);
		// Remove the file on MRU.
		DelMenuItem(g_lpMruMenu, wId + IDMRU, TRUE);
	}

	// Refresh the File menu.
	PlaceMenuMRUItem(g_lpMruMenu, GetSubMenu(GetMenu(g_hwndMain), 0), ID_FILE_EXIT);

	return fExist;
}

/*
==============
WndMain_AddMRUItem
==============
*/
void WndMain_AddMRUItem(LPSTR lpItem)
{
	// Add the file in MRU.
	AddNewItem(g_lpMruMenu, lpItem);

	// Refresh the File menu.
	PlaceMenuMRUItem(g_lpMruMenu, GetSubMenu(GetMenu(g_hwndMain), 0), ID_FILE_EXIT);
}

/*
==============
WndMain_CreateMRU
==============
*/
void WndMain_CreateMRU()
{
	g_lpMruMenu = CreateMruMenuDefault();
	LoadMruInReg(g_lpMruMenu, QE3_WIN_REGISTRY_MRU);

	// Refresh the File menu.
	PlaceMenuMRUItem(g_lpMruMenu, GetSubMenu(GetMenu(g_hwndMain), 0), ID_FILE_EXIT);
}

/*
==============
WndMain_DestroyMRU
==============
*/
void WndMain_DestroyMRU()
{
	SaveMruInReg(g_lpMruMenu, QE3_WIN_REGISTRY_MRU);
	DeleteMruMenu(g_lpMruMenu);
}
