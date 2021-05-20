//==============================
//	WndMain.h
//==============================

#ifndef __WND_MAIN_H__
#define __WND_MAIN_H__

#define REBARBANDCOUNT 12

extern HWND g_hwndMain;
extern HWND g_hwndStatus;
extern HWND g_hwndToolbar[REBARBANDCOUNT];
extern HWND g_hwndRebar;

class WndCamera;
class WndGrid;
class WndZChecker;
class WndTexture;
class WndEntity;
class WndConsole;

extern WndCamera	*g_wndCamera;
extern WndGrid		*g_wndGrid[4];
extern WndZChecker	*g_wndZ;
extern WndTexture	*g_wndTexture;
extern WndEntity	*g_wndEntity;
extern WndConsole	*g_wndConsole;

void	WndSplash_Create();
void	WndSplash_Destroy();

void	WndMain_SwapGridCam();
void	WndMain_SyncInspectorPlacement();
void	WndMain_SetInspectorMode(int nType);
HWND	WndMain_WindowForPoint(POINT point);
void	WndMain_DefaultLayout(int nStyle = 0);
bool	WndMain_SaveWindowStates();
bool	WndMain_LoadStateForWindow(HWND hWnd, const char *pszName);

void    WndMain_UpdateWindows(int bits);
void	WndMain_ForceUpdateWindows(int bits = 0);

void	WndMain_UpdateTitle();
void	WndMain_CreateViews();
void	WndMain_Create();


// WndMain_Menu
LONG WINAPI WndMain_Command(HWND hWnd, WPARAM wParam, LPARAM lParam);
void	WndMain_CheckMenuItem(HMENU hMenu, unsigned item, bool check);
void	WndMain_UpdateMenu();
void	WndMain_UpdateMenuFilters(HMENU hMenu);
void	WndMain_CreateMRU();
void	WndMain_DestroyMRU();
BOOL	WndMain_UpdateMRU(WORD wId);
void	WndMain_AddMRUItem(LPSTR lpItem);
void	WndMain_ViewFilterPopup();
void	WndMain_ToggleViewFilter(int filt);
int		WndMain_RotForModifiers();

// WndMain_Status
void    WndMain_Status(const char *psz, int part);
void    WndMain_UpdateBrushStatusBar();
void	WndMain_UpdateGridStatusBar();
void	WndMain_CreateStatusBar(HWND hWnd);

// WndMain_Toolbar
void	WndMain_CreateReBar(HWND hWnd, HINSTANCE hInst);
void	WndMain_SaveRebarPositions();

class WndMain {	// sooon
	WndMain();
	~WndMain() {};
};

#endif