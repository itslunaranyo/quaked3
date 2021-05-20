//==============================
//	WndMain.h
//==============================

#ifndef __WND_MAIN_H__
#define __WND_MAIN_H__

extern HWND g_hwndMain;
extern HWND g_hwndStatus;
extern HWND g_hwndToolbar[11];
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

void	WndMain_SwapGridCam();
void	WndMain_SetInspectorMode(int nType);
HWND	WndMain_WindowForPoint(int x, int y);
void	WndMain_DefaultLayout(int nStyle = 0);
bool	WndMain_SaveWindowState(HWND hWnd, const char *pszName);
bool	WndMain_LoadWindowState(HWND hWnd, const char *pszName);

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
void	WndMain_UpdateTextureMenu();
void	WndMain_CreateMRU();
void	WndMain_DestroyMRU();
BOOL	WndMain_UpdateMRU(WORD wId);
void	WndMain_AddMRUItem(LPSTR lpItem);
void	WndMain_ViewFilterPopup();
void	WndMain_ToggleViewFilter(int filt);
int		WndMain_RotForModifiers();
const char*	WndMain_WadForMenuItem(int mnum);

// WndMain_Status
void    WndMain_Status(const char *psz, int part);
void    WndMain_UpdateBrushStatusBar();
void	WndMain_UpdateGridStatusBar();
void	WndMain_CreateStatusBar(HWND hWnd);

// WndMain_Toolbar
HWND	WndMain_CreateToolBar(HWND hWnd, HINSTANCE hInst, int nIndex, int nPos, int nButtons);
void	WndMain_CreateReBar(HWND hWnd, HINSTANCE hInst);


class WndMain {	// sooon
	WndMain();
	~WndMain() {};
};

#endif