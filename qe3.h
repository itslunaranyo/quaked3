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

//#define GLEW_STATIC 1
#include <GL\glew.h>
//#include <GLFW\glfw3.h>
#include <gl\gl.h>
//#include <gl\glu.h>
//#include <gl\glaux.h>
#include <math.h>
#include <stdlib.h>
#include <cassert>	// lunaran - for my own sanity
#include <string>
#include <vector>
#include <utility>

#include "qeBuffer.h"	// lunaran - generic heap space; malloc as an object
#include "mathlib.h"
#include "cmdlib.h"
#include "errors.h"

#include "qedefs.h"
#include "qfiles.h"
#include "config.h"

#include "textures.h"
#include "texdef.h"
#include "plane.h"
#include "face.h"
#include "brush.h"
#include "entclass.h"
#include "entity.h"
#include "TargetGraph.h"
#include "map.h"
#include "select.h"
#include "modify.h"
#include "transform.h"
#include "surface.h"
#include "winding.h"

#include "v_view.h"
#include "v_tex.h"
#include "v_camera.h"
#include "v_xy.h"
#include "v_z.h"
#include "mru.h"

#include "Command.h"

#include "WndView.h"
#include "WndCamera.h"
#include "WndTexture.h"
#include "WndGrid.h"
#include "WndZChecker.h"
#include "WndEntity.h"
#include "WndConsole.h"

#include "Tool.h"
#include "NavTool.h"
#include "SelectTool.h"
#include "ManipTool.h"
#include "TextureTool.h"
#include "ClipTool.h"
#include "GeoTool.h"
#include "PolyTool.h"

//========================================================================

// persistent preferences saved into the registry
typedef struct
{
	int		nSize;				// structure size
	bool	bScaleLockX,	//obsolete
			bScaleLockY, 	//obsolete
			bScaleLockZ;	//obsolete
	REBARBANDINFO rbiSettings[11];	// sikk - Save Rebar Band Info
	/*
	int		nRenderMode;			// nearest, linear, etc
//	int		nVTexMenu;			// sikk - unused
	int		nViewFilter;
	bool	bShow_XYZ[4],		// lunaran - grid view reunification
			bShow_Z,			// sikk - Saved Window Toggle
			bShow_Axis,			// sikk - Show Axis
			bShow_Blocks,
			bShow_CameraGrid,	// sikk - Show Camera Grid
			bShow_Coordinates,
			bShow_LightRadius,	// sikk - Show Light Radius
			bShow_MapBoundary,	// sikk - Show Map Boundary Box
			bShow_Names,
			bShow_SizeInfo,
			bShow_Viewname,		// sikk - Show View Name
			bShow_Workzone,
			bShow_Angles,		// lunaran - no longer an entity flag
			bShow_Paths,		// lunaran - no longer an entity flag
			bNoClamp;	//obsolete
	*/
	/*
	bool	bCubicClip;			// sikk - Cubic Clipping
	int		nCubicScale;		// sikk - Cubic Clipping
	int		nCameraSpeed;		// sikk - Camera Speed Trackbar 
	vec3	v3Colors[COLOR_LAST];
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
			bVFEModesExclusive,
			bSortTexByWad,
			bModName,
			bHeapsize,
			bSkill,
			bDeathmatch,
			bDeveloper,
			bRSpeeds,
			bPointfile,
			bTestAfterBSP;
	*/
// <---sikk
} savedinfo_t;

/*
** most of the QE globals are stored in this structure
*/
typedef struct
{
	bool		d_bShowGrid;
	int			d_nGridSize;
	bool		bGridSnap;

	vec3		d_v3WorkMin, 			// defines the boundaries of the current work area, used to guess
				d_v3WorkMax;			// brushes and drop points third coordinate when creating from 2D view
	TexDef		d_workTexDef;			// lunaran: moved out of texturewin_t

	HGLRC		d_hglrcBase;
	HDC			d_hdcBase;
	HINSTANCE	d_hInstance;
	HWND		d_hInstanceColor,		// eerie
				d_hwndMain,
				d_hwndCamera,
				d_hwndEntity,
				d_hwndConsole,
				d_hwndTexture,
				d_hwndXYZ[4],			// lunaran - grid view reunification
				d_hwndZ,
				d_hwndStatus,
				d_hwndToolbar[11],
				d_hwndRebar,			// sikk - Rebar 
				d_hwndSurfaceDlg,		// lunaran - moved here from outer global
				d_hwndSetKeyvalsDlg;

	WndCamera	*d_wndCamera;
	WndGrid		*d_wndGrid[4];
	WndZChecker	*d_wndZ;
	WndTexture	*d_wndTexture;
	WndEntity	*d_wndEntity;
	WndConsole	*d_wndConsole;
	int         d_nInspectorMode;		// W_TEXTURE, W_ENTITY, or W_CONSOLE

	CameraView	d_vCamera;				// sikk - moved camera object here
	XYZView		d_vXYZ[4];				// lunaran - grid view reunification
	ZView		d_vZ;
	TextureView d_vTexture;

	Entity		*d_entityProject;
	Texture		*d_qtextures;
	int			d_nPointfileDisplayList;

	LPMRUMENU   d_lpMruMenu;
	savedinfo_t d_savedinfo;
	select_t    d_selSelectMode;
	int		    d_nFontList;

	bool		d_bTextureLock;
	float		d_fDefaultTexScale;		// sikk - Default Texture Scale Dialog
	float		d_fTexFitW, d_fTexFitH;
	vec3		d_lastColor;

	std::vector<Tool*> d_tools;
	TextureTool	*d_texTool;

	// handle to the console log file
	// we use low level I/O to get rid of buffering and have everything on file if we crash
	int         d_nLogFile;

	//bool		d_bResetRegistry;		// sikk - this is used for Preferences 'Reset Registry' command
} qeglobals_t;

//========================================================================

/*
** extern declarations
*/
extern qeglobals_t g_qeglobals;
extern double	g_deltaTime;
extern int		g_nUpdateBits;
extern int		g_nScreenWidth;
extern int		g_nScreenHeight;
extern char		*g_szBSP_Commands[256];
extern HANDLE	g_hBSP_Process;

extern char		g_qeAppName[64];
extern char		g_qePath[MAX_PATH];

/*
** global declarations
*/
extern bool	g_bSnapCheck;

//========================================================================

// QE function declarations
void	QE_TestSomething();

void	QE_CheckAutoSave();
void	QE_CheckOpenGLForErrors(void);
void	QE_OpenGLError(int errornum, const char *errorstr);
void	QE_ConvertDOSToUnixName (char *dst, const char *src);
void	QE_ExpandBspString (char *bspaction, char *out, char *mapname);
void	QE_Init();
bool	QE_KeyDown(int key);
bool	QE_KeyUp(int key);
void	QE_SaveMap();
void	QE_UpdateTitle();
bool	QE_InitProject();
int		QE_BestViewAxis();
bool	QE_SingleBrush();
void	QE_CheckMenuItem(HMENU hMenu, unsigned item, bool check);
void	QE_UpdateCommandUI();
void	QE_UpdateCommandUIFilters(HMENU hMenu);
char   *QE_ExpandRelativePath (char *p);
void	QE_SetInspectorMode(int nType);

vec3	pointOnGrid(const vec3 point);
vec3	AxisForVector(const vec3 &v);		// TODO: mathlib
vec3	AxializeVector(const vec3 &v);

// system functions
void    Sys_UpdateBrushStatusBar();
void	Sys_UpdateGridStatusBar();
void    Sys_UpdateWindows(int bits);
void	Sys_ForceUpdateWindows(int bits);
void    Sys_Beep();
void    Sys_ClearPrintf();
void    Sys_Printf(char *text, ...);
double	Sys_DoubleTime();
void    Sys_GetCursorPos(int *x, int *y);
void    Sys_SetCursorPos(int x, int y);
void    Sys_SetTitle(char *text);
void    Sys_BeginWait();
void    Sys_EndWait();
void    Sys_Status(const char *psz, int part);
void	Sys_LogFile();
void	Sys_CheckBspProcess(void);
void	Sys_DeltaTime();

// win_qe3.c
void	FillBSPMenu();
char   *TranslateString(char *buf);
//void	ProjectDialog ();
//void	NewProjectDialog ();	// sikk - New Project Dialog
void	OpenDialog();
void	SaveAsDialog();
bool	ConfirmModified();
void	ImportDialog();
void	ExportDialog();

XYZView*	XYZWnd_WinFromHandle(HWND xyzwin);

// win_main.c
void WMain_Create ();
LONG WINAPI CommandHandler (HWND hWnd, WPARAM wParam, LPARAM lParam); // sikk - Declaration for Popup menu
BOOL DoColor (int iIndex);
bool DoColorSelect(const vec3 rgbIn, vec3 &rgbOut);
void DoTestMap ();	// sikk - Test Map
void DoWindowPosition (int nStyle);	// sikk - Window Positions
void FillTextureMenu();

static HWND CreateStatusBar (HWND hWnd);
static HWND CreateReBar (HWND hWnd, HINSTANCE hInst);	// sikk - Rebar
static HWND CreateToolBar (HWND hWnd, HINSTANCE hInst, int nIndex, int nPos, int nButtons); // sikk - Toolbars
//static HWND CreateToolBar(HWND hWnd, HINSTANCE hInst);
//static HWND CreateTrackBar (HWND hWnd, HINSTANCE hInst, int nIndex);	// sikk - Camera Speed Trackbar

extern BOOL SaveWindowState (HWND hWnd, const char *pszName);
extern BOOL LoadWindowState (HWND hWnd, const char *pszName);
extern BOOL SaveRegistryInfo (const char *pszName, void *pvBuf, long lSize);
extern BOOL LoadRegistryInfo (const char *pszName, void *pvBuf, long *plSize);

// win_dlg.c
void DoFindBrush();
void DoRotate();
void DoSides(int nType);	// sikk - Brush Primitives (previously took no arguments)
void DoKeylist();
void DoMouselist();
void DoAbout();
void DoFindTexture();
//void DoNewProject();	// sikk - New Project Dialog
//void DoProject (bool bFirst);	// sikk - Project Settings Dialog
void FillEntityListbox(HWND hwnd, bool bPointbased, bool bBrushbased);	// sikk - Create Entity Dialog
bool ConfirmClassnameHack(EntClass *desired);
void DoCreateEntity(bool bPointbased, bool bBrushbased, bool bSel, const vec3 origin);	// sikk - Create Entity Dialog
void DoMapInfo();	// sikk - Map Info Dialog
void DoEntityInfo();	// sikk - Entity Info Dialog
//void DoPreferences();	// sikk - Preferences Dialog
void DoScale();	// sikk - Brush Scaling Dialog
void DoCamSpeed();	// sikk - Camera Speed Dialog
void DoDefaultTexScale();	// sikk - Default Texture Scale Dialog
void DoFindKeyValue();	// sikk - Find Key/Value Dialog
void DoSetKeyValues();
bool ImportOptionsDialog(class CmdImportMap *cmdIM);

// win_surf.c
void WndSurf_UpdateUI();
void WndSurf_Close();
void WndSurf_Create();

// WndConfig.c
void DoConfigWindow();
void DoConfigWindowProject();

// win_proj.c
/*
BOOL CALLBACK ProjectSettingsDlgProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK SelectDirDlgProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

void GetProjectDirectory(HWND hwndDlg);
void GetRemoteBasePath(HWND hwndDlg);
void GetMapsDirectory(HWND hwndDlg);
void WndCfg_GetAutosaveMap(HWND hwndDlg);
void WndCfg_GetEntityFiles(HWND hwndDlg);
void GetTextureDirectory(HWND hwndDlg);
void GetToolsDirectory(HWND hwndDlg);
void WndCfg_GetDefaultWads(HWND hwndDlg);

int  GetNextFreeBspIndex ();
int  GetBspIndex (char *text);
void NewBspCommand (HWND hwndDlg);
void AcceptBspCommand (HWND hwndDlg);
void DeleteBspCommand (HWND hwndDlg);

void SaveSettings (HWND hwndDlg);
*/
bool SelectDir(HWND h, bool format, char* title);


#endif
