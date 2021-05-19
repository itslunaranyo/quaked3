//==============================
//	qe3.h
//==============================

#ifndef __QE3_H__
#define __QE3_H__

// disable data conversion warnings for gl
#pragma warning(disable : 4244)     // MIPS
#pragma warning(disable : 4136)     // X86
#pragma warning(disable : 4051)     // ALPHA
#pragma warning(1 : 4706)	// assignment in conditional: never not a typo

#define WINVER 0x0501
#define _WIN32_WINNT_WINXP 0x0501 // Windows XP  

#include <windows.h>
#include <commctrl.h>
#include "afxres.h"
#include "resource.h"

#include <gl/gl.h>
#include <gl/glu.h>
#include <gl/glaux.h>
#include <math.h>
#include <stdlib.h>
#include <cassert>	// lunaran - for my own sanity
#include <string>
#include <vector>

#include "qeBuffer.h"	// lunaran - generic heap space; malloc as an object
#include "cmdlib.h"
//#include "lbmlib.h"
#include "mathlib.h"
#include "parse.h"

#include "qedefs.h"
#include "qfiles.h"

#include "palette.h"
#include "textures.h"
#include "face.h"
#include "brush.h"
#include "clip.h"
#include "csg.h"
#include "drag.h"
#include "entclass.h"
#include "entity.h"
#include "map.h"
#include "points.h"
#include "select.h"
#include "surface.h"
#include "vertsel.h"
#include "winding.h"

#include "v_view.h"
#include "v_tex.h"
#include "v_camera.h"
#include "v_xy.h"
#include "v_z.h"
#include "mru.h"
#include "undo.h"

#include "commands.h"

//========================================================================

typedef struct
{
	int		nSize;				// structure size
	int		nTexMenu;			// nearest, linear, etc
//	int		nVTexMenu;			// sikk - unused
	int		nExclude;
	bool	bShow_XYZ[4],		// lunaran - grid view reunification
			bShow_Z,			// sikk - Saved Window Toggle
			bShow_Axis,			// sikk - Show Axis
			bShow_Blocks,
			bShow_CameraGrid,	// sikk - Show Camera Grid
			bShow_Coordinates,
			bShow_LightRadius,	// sikk - Show Light Radius
			bShow_MapBoundry,	// sikk - Show Map Boundry Box
			bShow_Names,
			bShow_SizeInfo,
			bShow_Viewname,		// sikk - Show View Name
			bShow_Workzone,
			bNoClamp,
			bScaleLockX,		// sikk - Brush Scaling Axis Lock
			bScaleLockY, 
			bScaleLockZ,
			bCubicClip;			// sikk - Cubic Clipping
	int		nCubicScale;		// sikk - Cubic Clipping
	int		nCameraSpeed;		// sikk - Camera Speed Trackbar 
	vec3	v3Colors[COLOR_LAST];
	REBARBANDINFO rbiSettings[11];	// sikk - Save Rebar Band Info
//	int		nRebarSavedIndex[11];

// sikk---> Preferences Dialog
	char	szGamePath[_MAX_PATH],
			szGameName[_MAX_FNAME],
			szPrefabPath[_MAX_PATH],
			szLastProject[_MAX_PATH],
			szLastMap[_MAX_PATH],
			szModName[_MAX_FNAME],
			szHeapsize[8],
			szSkill[1];
	int		nAutosave,
			nHeapsize,
			nMapSize,
			nUndoLevels;
	float	fGamma;				// gamma for textures
	bool	bAutosave,
			bBrushPrecision,
			bLoadLastProject,
			bLoadLastMap,
			bLogConsole,
			bNoStipple,
			bRadiantLights,
			bVertexSplitsFace,
			bSortTexByWad,
			bModName,
			bHeapsize,
			bSkill,
			bDeathmatch,
			bDeveloper,
			bRSpeeds,
			bPointfile,
			bTestAfterBSP;
// <---sikk
} savedinfo_t;

/*
** most of the QE globals are stored in this structure
*/
typedef struct
{
	bool		d_bShowGrid;
	int			d_nGridSize;

	vec3		d_v3WorkMin, 			// defines the boundaries of the current work area is used to guess
				d_v3WorkMax;			// brushes and drop points third coordinate when creating from 2D view
	texdef_t	d_workTexDef;			// lunaran: moved out of texturewin_t

	HGLRC		d_hglrcBase;
	HDC			d_hdcBase;
	HINSTANCE	d_hInstance;
	HWND		d_hInstanceColor,		// eerie
				d_hwndMain,
				d_hwndCamera,
				d_hwndInspector,
					d_hwndEntity,
					d_hwndConsole,
					d_hwndTexture,
				d_hwndXYZ[4],			// lunaran - grid view reunification
				d_hwndZ,
				d_hwndStatus,
				d_hwndToolbar1,
				d_hwndToolbar2,
				d_hwndToolbar3,
				d_hwndToolbar4,
				d_hwndToolbar5,
				d_hwndToolbar6,
				d_hwndToolbar7,
				d_hwndToolbar8,
				d_hwndToolbar9,
				d_hwndToolbar10,
				d_hwndToolbar11,
				d_hwndRebar,			// sikk - Rebar 
				d_hwndSplash,			// sikk - Splash screen
				d_hwndSurfaceDlg;		// lunaran - moved here from outer global

	Entity		*d_entityProject;
	int			d_nNumEntities;

	int			d_nNumPoints;
	int			d_nNumEdges;
	int			d_nNumMovePoints;
	vec3		d_v3Points[MAX_POINTS];
	pedge_t		d_pEdges[MAX_EDGES];
	vec3		*d_fMovePoints[1024];

	Texture		*d_qtextures;
	TextureView d_texturewin;

	int			d_nPointfileDisplayList;

	CameraView	d_camera;				// sikk - moved camera object here
	XYZView		d_xyz[4];				// lunaran - grid view reunification
	ZView         d_z;

	int         d_nInspectorMode;		// W_TEXTURE, W_ENTITY, or W_CONSOLE

	LPMRUMENU   d_lpMruMenu;

	savedinfo_t d_savedinfo;

	//int         d_nWorkCount;		// no longer necessary for punishing jromero unproductivity

	vec3		d_v3SelectTranslate;    // for dragging w/o making new display lists
	select_t    d_selSelectMode;

	int		    d_nFontList;
	int         d_nParsedBrushes;

	bool		d_bTextureLock;
	float		d_fDefaultTexScale;		// sikk - Default Texture Scale Dialog

	bool	    d_bClipMode;

	// handle to the console log file
	// we use low level I/O to get rid of buffering and have everything on file if we crash
	int         d_nLogFile;

	bool		d_bResetRegistry;		// sikk - this is used for Preferences 'Reset Registry' command
} qeglobals_t;

//========================================================================

/*
** extern declarations
*/
extern qeglobals_t g_qeglobals;

extern int		g_nUpdateBits;
extern int		g_nScreenWidth;
extern int		g_nScreenHeight;
extern char    *g_szBSP_Commands[256];
extern HANDLE	g_hBSP_Process;

//extern UINT		g_unMouseWheel;	// sikk - Mousewheel Handling

/*
** global declarations
*/
extern bool	g_bSnapCheck;

//========================================================================

// QE function declarations
void	QE_CheckAutoSave ();
void	QE_CheckOpenGLForErrors (void);
void	QE_ConvertDOSToUnixName (char *dst, const char *src);
void	QE_CountBrushesAndUpdateStatusBar ();
void	QE_ExpandBspString (char *bspaction, char *out, char *mapname);
void	QE_Init ();
bool	QE_KeyDown (int key);
bool	QE_LoadProject (char *projectfile);
bool	QE_SingleBrush ();
void	QE_UpdateCommandUI ();
char   *QE_ExpandRelativePath (char *p);

// QE Win32 function declarations
int		QEW_SetupPixelFormat (HDC hDC, bool zbuffer);
void	QEW_StopGL (HWND hWnd, HGLRC hGLRC, HDC hDC);

char   *CopyString (char *s);

// system functions
void    Sys_UpdateBrushStatusBar ();
void	Sys_UpdateGridStatusBar ();
void    Sys_UpdateWindows (int bits);
void    Sys_Beep ();
void    Sys_ClearPrintf ();
void    Sys_Printf (char *text, ...);
double	Sys_DoubleTime ();
void    Sys_GetCursorPos (int *x, int *y);
void    Sys_SetCursorPos (int x, int y);
void    Sys_SetTitle (char *text);
void    Sys_BeginWait ();
void    Sys_EndWait ();
void    Sys_Status (const char *psz, int part);
void	Sys_LogFile ();

// win_qe3.c
void	FillBSPMenu ();
char   *TranslateString (char *buf);
void	ProjectDialog ();
void	NewProjectDialog ();	// sikk - New Project Dialog
void	OpenDialog ();
void	SaveAsDialog ();
bool	ConfirmModified ();
void	ImportDialog (bool bCheck);	// sikk - Import Dialog for map/prefab
void	ExportDialog (bool bCheck);	// sikk - Export Dialog for map/prefab

// textures.c
HWND	TexWnd_Create (HINSTANCE hInstance);
LONG	WINAPI WTex_WndProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void	TexWnd_Resize(RECT rc);

// win_insp.c
void	InspWnd_Create (HINSTANCE hInstance);
void	InspWnd_SetMode (int nType);
void	InspWnd_ToTop();
void	InspWnd_Resize();
BOOL	CALLBACK InspWndProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
void	InspWnd_Move(HWND hwnd, int x, int y, int w, int h);
void	InspWnd_MoveRect(HWND hwnd, RECT r);

void	ConsoleWnd_Create(HINSTANCE hInstance);
void	ConsoleWnd_Resize(RECT rc);

// win_ent.c
void	EntWnd_Create (HINSTANCE hInstance);
void	EntWnd_Resize(RECT rc);

void	EntWnd_RefreshEditEntity();
void	EntWnd_UpdateListSel();
void	EntWnd_UpdateUI();
void	EntWnd_CreateEntity ();
void	EntWnd_FillClassList ();
void	EntWnd_SetKeyValue ();
void	EntWnd_SetKeyValue(const char* key, const char* value);
void	EntWnd_RemoveKeyValue ();
void	EntWnd_EditKeyValue ();
void	EntWnd_CreateControls (HINSTANCE hInstance);
void	EntWnd_FlagsFromEnt ();
void	EntWnd_RefreshKeyValues ();

// win_cam.c
void	WCam_Create (HINSTANCE hInstance);
LONG	WINAPI WCam_WndProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// win_xy.c
int		XYZWnd_GetTopWindowViewType();
void	WXYZ_Create (HINSTANCE hInstance, int slot);
LONG	WINAPI XYZWnd_Proc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
//static void WXY_InitPixelFormat (PIXELFORMATDESCRIPTOR *pPFD);	// sikk - unused
void	WXY_Print ();
int		GetSelectionInfo ();	// sikk - Contex Menu
void	XYZWnd_DoPopupMenu(XYZView* xyz, int x, int y);	// sikk - Contex Menu
XYZView*	XYZWnd_WinFromHandle(HWND xyzwin);
void	XYZWnd_CycleViewAxis(HWND xyzwin);
void	XYZWnd_SetViewAxis(HWND xyzwin, int viewAxis);

// win_z.c
void WZ_Create (HINSTANCE hInstance);
LONG WINAPI WZ_WndProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// win_main.c
void WMain_Create (HINSTANCE hInstance);
LONG WINAPI CommandHandler (HWND hWnd, WPARAM wParam, LPARAM lParam); // sikk - Declaration for Popup menu
BOOL DoColor (int iIndex);
void DoTestMap ();	// sikk - Test Map
void DoWindowPosition (int nStyle);	// sikk - Window Positions

static HWND CreateStatusBar (HWND hWnd);
static HWND CreateReBar (HWND hWnd, HINSTANCE hInst);	// sikk - Rebar
static HWND CreateToolBar (HWND hWnd, HINSTANCE hInst, int nIndex, int nPos, int nButtons, int nSize); // sikk - Toolbars
//static HWND CreateToolBar(HWND hWnd, HINSTANCE hInst);
static HWND CreateTrackBar (HWND hWnd, HINSTANCE hInst, int nIndex);	// sikk - Camera Speed Trackbar

extern BOOL SaveWindowState (HWND hWnd, const char *pszName);
extern BOOL LoadWindowState (HWND hWnd, const char *pszName);
extern BOOL SaveRegistryInfo (const char *pszName, void *pvBuf, long lSize);
extern BOOL LoadRegistryInfo (const char *pszName, void *pvBuf, long *plSize);

// win_dlg.c
void DoFindBrush ();
void DoRotate ();
void DoSides (int nType);	// sikk - Brush Primitives (previously took no arguments)
void DoKeylist ();
void DoAbout ();
void DoFindTexture ();
void DoNewProject();	// sikk - New Project Dialog
void DoProject (bool bFirst);	// sikk - Project Settings Dialog
void FillEntityListbox(HWND hwnd, bool bPointbased, bool bBrushbased);	// sikk - Create Entity Dialog
bool ConfirmClassnameHack(EntClass *desired);
void DoCreateEntity (bool bPointbased, bool bBrushbased, bool bSel, const vec3 origin);	// sikk - Create Entity Dialog
void DoMapInfo ();	// sikk - Map Info Dialog
void DoEntityInfo ();	// sikk - Entity Info Dialog
void DoPreferences ();	// sikk - Preferences Dialog
void DoScale ();	// sikk - Brush Scaling Dialog
void DoCamSpeed ();	// sikk - Camera Speed Dialog
void DoDefaultTexScale ();	// sikk - Default Texture Scale Dialog
void DoFindKeyValue ();	// sikk - Find Key/Value Dialog

// win_surf.c
void	SurfWnd_UpdateUI();
void	SurfWnd_Close();
void	SurfWnd_Create();

// win_proj.c
BOOL CALLBACK ProjectSettingsDlgProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK SelectDirDlgProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

void GetProjectDirectory (HWND hwndDlg);
void GetRemoteBasePath (HWND hwndDlg);
void GetMapsDirectory (HWND hwndDlg);
void GetAutosaveMap (HWND hwndDlg);
void GetEntityFiles (HWND hwndDlg);
void GetTextureDirectory (HWND hwndDlg);
void GetToolsDirectory (HWND hwndDlg);

int  GetNextFreeBspIndex ();
int  GetBspIndex (char *text);
void NewBspCommand (HWND hwndDlg);
void AcceptBspCommand (HWND hwndDlg);
void DeleteBspCommand (HWND hwndDlg);

void SaveSettings (HWND hwndDlg);
bool SelectDir (HWND h);

//
// win_snap.c
//
bool FindClosestBottom (HWND h, LPRECT rect, LPRECT parentrect);
bool FindClosestTop (HWND h, LPRECT rect, LPRECT parentrect);
bool FindClosestLeft (HWND h, LPRECT rect, LPRECT parentrect, int widthlimit);
bool FindClosestRight (HWND h, LPRECT rect, LPRECT parentrect, int widthlimit);
bool FindClosestHorizontal (HWND h, LPRECT rect, LPRECT parentrect);
bool FindClosestVertical (HWND h, LPRECT rect, LPRECT parentrect);
bool TryDocking (HWND h, long side, LPRECT rect, int widthlimit); // sikk - Window Management

#endif
