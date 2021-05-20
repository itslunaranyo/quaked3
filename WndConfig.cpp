//==============================
//	WndEntity.cpp
//==============================

#include "qe3.h"
#include "io.h"
#include <algorithm>
#include <Shlobj.h>

static HWND hwndCfg, hwndCfgEditor, hwndCfgProject, hwndCfgColor, hwndCfgUI;

struct cfgContext_t
{
	qecfgEditor_t &cfgEd;
	qecfgUI_t &cfgUI;
	qecfgProject_t &cfgProj;
	qecfgColors_t &cfgColor;
	cfgContext_t(qecfgEditor_t &e, qecfgUI_t &u, qecfgProject_t &p, qecfgColors_t &c) :
		cfgEd(e), cfgUI(u), cfgProj(p), cfgColor(c) {}
};

/*
=====================================================================

	PROJECT

=====================================================================
*/

static bool g_noProject;
static bool g_cfgNeedReload;
static bool g_needApply;

static OPENFILENAME ofn;			// common dialog box structure
static char szDirName[_MAX_PATH];   // directory string
static char szFile[_MAX_PATH];		// filename string
static char szFileTitle[_MAX_FNAME];// file title string
static char szFilter[64] = "Quake Executables (*.exe)\0*.exe\0\0";	// filter string
char	g_szCurrentDirectory[MAX_PATH];

/*
==================
WndCfg_FormatPath
==================
*/
void WndCfg_FormatPath(char* dst, char* src)
{
	assert(src != dst);
	char szTemp[512];

	QE_ConvertDOSToUnixName(szTemp, src);

	if (!_strnicmp(szTemp, g_cfgEditor.QuakePath, strlen(g_cfgEditor.QuakePath)))
		sprintf(dst, "$QUAKE/%s", &szTemp[strlen(g_cfgEditor.QuakePath)]);
	else
		strcpy(dst, szTemp);
}

/*
==================
SelectDir
==================
*/
bool SelectDir(HWND h, bool format, char* title)
{
	BROWSEINFO bi = { 0 };
	LPITEMIDLIST pidl;
	
	bi.hwndOwner = h;
	bi.lpszTitle = title;
	pidl = SHBrowseForFolder(&bi);

	if (pidl != 0)
	{
		char DirName[MAX_PATH];

		if (SHGetPathFromIDList(pidl, DirName))
		{
			SetStr(szDirName, DirName, "\\", NULL, NULL);
			if (format)
				WndCfg_FormatPath(DirName, szDirName);
			else
				QE_ConvertDOSToUnixName(DirName, szDirName);
			SetWindowText(h, DirName);
		}

		GlobalFree(pidl);
		return true;
	}
	return false;
}

bool SelectDir(HWND hwndDlg, int idDlgItem, bool format, char* title)
{
	HWND hwndEdit = GetDlgItem(hwndDlg, idDlgItem);

	if (!SendMessage(hwndEdit, WM_GETTEXT, (WPARAM)MAX_PATH - 1, (LPARAM)szDirName))
		strcpy(szDirName, g_szCurrentDirectory);

	return SelectDir(hwndEdit, format, title);
}

/*
==================
WndCfg_GetAutosaveMap
==================
*/
bool WndCfg_GetAutosaveMap(HWND hwndDlg)
{
	sprintf(szDirName, "%smaps", g_szCurrentDirectory);

	szFile[0] = 0;

	GetCurrentDirectory(MAX_PATH - 1, szDirName);

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hwndDlg;
	ofn.lpstrFilter = "QuakeEd File (*.map)\0*.map\0\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFileTitle = szFileTitle;
	ofn.nMaxFileTitle = sizeof(szFileTitle);
	ofn.lpstrInitialDir = szDirName;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
	ofn.lpstrTitle = "Select Auto-Save Map File";

	if (GetSaveFileName(&ofn))
	{
		char szTemp[512];
		WndCfg_FormatPath(szTemp, szFile);
		SetDlgItemText(hwndDlg, IDC_EDIT_AUTOSAVEMAP, szTemp);
		return true;
	}
	return false;
}

/*
==================
WndCfg_GetEntityFiles
==================
*/
bool WndCfg_GetEntityFiles(HWND hwndDlg)
{
	szFile[0] = 0;

	//MessageBox(g_qeglobals.d_hwndMain, "To load multiple entity definition files,\nuse the wildcard expression (EG: *.def or *.qc)", "Quake Ed 3: Info", MB_OK | MB_ICONINFORMATION);

	GetCurrentDirectory(MAX_PATH - 1, szDirName);

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hwndDlg;
	ofn.lpstrFilter = "QuakeEd Entity Definition File (*.qc)\0*.qc\0QuakeEd Entity Definition File (*.def)\0*.def\0QuakeEd Entity Definition File (*.c)\0*.c\0All Files\0*.*\0\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFileTitle = szFileTitle;
	ofn.nMaxFileTitle = sizeof(szFileTitle);
	ofn.lpstrInitialDir = g_szCurrentDirectory;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
	ofn.lpstrTitle = "Select Entity Definition File";

	if (GetOpenFileName(&ofn))
	{
		char szTemp[512];
		WndCfg_FormatPath(szTemp, szFile);
		SetDlgItemText(hwndDlg, IDC_EDIT_ENTITYFILES, szTemp);
		return true;
	}
	return false;
}

/*
==================
WndCfg_GetDefaultWads
==================
*/
bool WndCfg_GetDefaultWads(HWND hwndDlg)
{
	HWND hwndEdit = GetDlgItem(hwndDlg, IDC_EDIT_TEXTUREDIRECTORY);

	if (!SendMessage(hwndEdit, WM_GETTEXT, (WPARAM)MAX_PATH - 1, (LPARAM)szDirName))
		strcpy(szDirName, g_szCurrentDirectory);

	szFile[0] = 0;

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hwndDlg;
	ofn.lpstrFilter = "QuakeEd Wad File (*.wad)\0*.wad\0\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFileTitle = szFileTitle;
	ofn.nMaxFileTitle = sizeof(szFileTitle);
	ofn.lpstrInitialDir = szDirName;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_LONGNAMES | OFN_EXPLORER | OFN_ALLOWMULTISELECT;
	ofn.lpstrTitle = "Select Default Wad File(s)";

	if (GetOpenFileName(&ofn))
	{
		if (strlen(ofn.lpstrFile))
		{
			char *dir, *file;

			dir = ofn.lpstrFile;
			file = (ofn.lpstrFile + strlen(ofn.lpstrFile) + 1);
			if (*file == '\0') // Single file selected
			{
				// need to strip off the path
				file = strrchr(dir, '\\') + 1;
				SetDlgItemText(hwndDlg, IDC_EDIT_DEFAULTWADS, file);
			}
			else
			{
				szFile[0] = 0;
				while (*file)
				{
					if (szFile[0])
						strcat(szFile, ";");
					strcat(szFile, file);
					file = (file + strlen(file) + 1);
				}
				SetDlgItemText(hwndDlg, IDC_EDIT_DEFAULTWADS, szFile);
			}
		}
		return true;
	}
	return false;
	/*
	else
	{
		switch (CommDlgExtendedError())
		{
		case CDERR_DIALOGFAILURE:
			MessageBox(g_qeglobals.d_hwndMain, "CDERR_DIALOGFAILURE", "Common Dialog Error", MB_OK);
			break;
		case CDERR_FINDRESFAILURE:
			MessageBox(g_qeglobals.d_hwndMain, "CDERR_FINDRESFAILURE", "Common Dialog Error", MB_OK);
			break;
		case CDERR_NOHINSTANCE:
			MessageBox(g_qeglobals.d_hwndMain, "CDERR_NOHINSTANCE", "Common Dialog Error", MB_OK);
			break;
		case CDERR_INITIALIZATION:
			MessageBox(g_qeglobals.d_hwndMain, "CDERR_INITIALIZATION", "Common Dialog Error", MB_OK);
			break;
		case CDERR_NOHOOK:
			MessageBox(g_qeglobals.d_hwndMain, "CDERR_NOHOOK", "Common Dialog Error", MB_OK);
			break;
		case CDERR_LOCKRESFAILURE:
			MessageBox(g_qeglobals.d_hwndMain, "CDERR_LOCKRESFAILURE", "Common Dialog Error", MB_OK);
			break;
		case CDERR_NOTEMPLATE:
			MessageBox(g_qeglobals.d_hwndMain, "CDERR_NOTEMPLATE", "Common Dialog Error", MB_OK);
			break;
		case CDERR_LOADRESFAILURE:
			MessageBox(g_qeglobals.d_hwndMain, "CDERR_LOADRESFAILURE", "Common Dialog Error", MB_OK);
			break;
		case CDERR_STRUCTSIZE:
			MessageBox(g_qeglobals.d_hwndMain, "CDERR_STRUCTSIZE", "Common Dialog Error", MB_OK);
			break;
		case CDERR_LOADSTRFAILURE:
			MessageBox(g_qeglobals.d_hwndMain, "CDERR_LOADSTRFAILURE", "Common Dialog Error", MB_OK);
			break;
		case FNERR_BUFFERTOOSMALL:
			MessageBox(g_qeglobals.d_hwndMain, "FNERR_BUFFERTOOSMALL", "Common Dialog Error", MB_OK);
			break;
		case CDERR_MEMALLOCFAILURE:
			MessageBox(g_qeglobals.d_hwndMain, "CDERR_MEMALLOCFAILURE", "Common Dialog Error", MB_OK);
			break;
		case FNERR_INVALIDFILENAME:
			MessageBox(g_qeglobals.d_hwndMain, "FNERR_INVALIDFILENAME", "Common Dialog Error", MB_OK);
			break;
		case CDERR_MEMLOCKFAILURE:
			MessageBox(g_qeglobals.d_hwndMain, "CDERR_MEMLOCKFAILURE", "Common Dialog Error", MB_OK);
			break;
		case FNERR_SUBCLASSFAILURE:
			MessageBox(g_qeglobals.d_hwndMain, "FNERR_SUBCLASSFAILURE", "Common Dialog Error", MB_OK);
			break;
		}
	}
	*/
}


/*
============
WndCfg_ProjectComboChanged
============
*/
void WndCfg_ProjectComboChanged()
{
	int i = SendDlgItemMessage(hwndCfgProject, IDC_COMBO_PROJECT, CB_GETCURSEL, 0, 0);

	qecfgProject_t &prj = g_qeconfig.projectPresets[i];
	SetDlgItemText(hwndCfgProject, IDC_EDIT_GAMEBASEPATH, prj.basePath);
	SetDlgItemText(hwndCfgProject, IDC_EDIT_MAPSDIRECTORY, prj.mapPath);
	SetDlgItemText(hwndCfgProject, IDC_EDIT_AUTOSAVEMAP, prj.autosaveFile);
	SetDlgItemText(hwndCfgProject, IDC_EDIT_ENTITYFILES, prj.entityFiles);
	SetDlgItemText(hwndCfgProject, IDC_EDIT_TEXTUREDIRECTORY, prj.wadPath);
	SetDlgItemText(hwndCfgProject, IDC_EDIT_DEFAULTWADS, prj.defaultWads);
	SetDlgItemText(hwndCfgProject, IDC_EDIT_PALETTEFILE, prj.paletteFile);

	SendDlgItemMessage(hwndCfgProject, IDC_CHECK_EXTTARGETS, BM_SETCHECK, (prj.extTargets ? BST_CHECKED : BST_UNCHECKED), 0);	
}


/*
============
WndCfg_ProjectsDifferent

only for critical project differences - if any of these fields are different,
reinitializing the editor/project state is necessary
============
*/
bool WndCfg_ProjectsDifferent(qecfgProject_t &a, qecfgProject_t &b)
{
	if (strcmp(a.name, b.name)) return true;
	if (strcmp(a.basePath, b.basePath)) return true;
	if (strcmp(a.entityFiles, b.entityFiles)) return true;
	if (strcmp(a.paletteFile, b.paletteFile)) return true;
	return false;
}

/*
============
WndCfg_CheckProjectAlteration
============
*/
void WndCfg_CheckProjectAlteration()
{
	qecfgProject_t oldPrj = g_project;
	int p = SendDlgItemMessage(hwndCfgProject, IDC_COMBO_PROJECT, CB_GETCURSEL, 0, 0);
	if (p != 0)		// put the selected project first in the list
		std::rotate(g_qeconfig.projectPresets.begin(), g_qeconfig.projectPresets.begin() + p, g_qeconfig.projectPresets.begin() + p + 1);

	g_qeconfig.ExpandProjectPaths();

	// if the config has changed in a significant way, force a save and reload to 
	// get any new entity definitions/palette/etc
	if (g_cfgNeedReload || WndCfg_ProjectsDifferent(oldPrj, g_project))
	{
		if (g_cmdQueue.IsModified())
		{
			if (MessageBox(g_qeglobals.d_hwndMain,
				"Changes to the configuration require the current map to be reloaded.\nWould you like to do this now? (Map will be saved first.)",
				"QuakeEd 3: Confirm Project Change", MB_YESNO | MB_ICONQUESTION) == IDNO)
			{
				return;
			}
			SendMessage(g_qeglobals.d_hwndMain, WM_COMMAND, 0, ID_FILE_SAVE);
		}
		char szMapTemp[_MAX_FNAME];
		szMapTemp[0] = 0;
		if (g_map.hasFilename)
			strncpy(szMapTemp, g_map.name, _MAX_FNAME);
		QE_InitProject();
		if (*szMapTemp)
			g_map.LoadFromFile(szMapTemp);
	}
}

/*
============
WndCfg_ProjectPresetsToWnd
============
*/
void WndCfg_ProjectPresetsToWnd()
{
	SendDlgItemMessage(hwndCfgProject, IDC_COMBO_PROJECT, CB_RESETCONTENT, 0, 0);
	if (g_qeconfig.projectPresets.empty())
		return;
	for (auto prjIt = g_qeconfig.projectPresets.begin(); prjIt != g_qeconfig.projectPresets.end(); ++prjIt)
		SendDlgItemMessage(hwndCfgProject, IDC_COMBO_PROJECT, CB_ADDSTRING, 0, (LPARAM)prjIt->name);
	qecfgProject_t &prj = *g_qeconfig.projectPresets.begin();
	SendDlgItemMessage(hwndCfgProject, IDC_COMBO_PROJECT, CB_SETCURSEL, 0, 0);
	WndCfg_ProjectComboChanged();
}


/*
============
ConfigNewProjDlgProc
============
*/
BOOL CALLBACK ConfigNewProjDlgProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	char sz[256];
	sz[0] = 0;

	switch (uMsg)
	{
	case WM_INITDIALOG:
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			GetDlgItemText(hwndDlg, IDC_EDIT_NEWNAME, sz, 255);
			if (!*sz)
			{
				EndDialog(hwndDlg, 0);
				break;
			}
			g_qeconfig.projectPresets.emplace_back();
			strcpy(g_qeconfig.projectPresets.back().name, sz);
			WndCfg_ProjectPresetsToWnd();
			SendDlgItemMessage(hwndCfgProject, IDC_COMBO_PROJECT, CB_SETCURSEL, (WPARAM)g_qeconfig.projectPresets.size() - 1, 0);
			WndCfg_ProjectComboChanged();

			EndDialog(hwndDlg, 1);
			break;
		case IDCANCEL:
			EndDialog(hwndDlg, 0);
			break;
		}
		return 0;
	}
	return FALSE;
}

/*
============
WndCfg_DoNewProject
============
*/
void WndCfg_DoNewProject()
{
	DialogBox(g_qeglobals.d_hInstance, MAKEINTRESOURCE(IDD_NEWNAME), g_qeglobals.d_hwndMain, ConfigNewProjDlgProc);
}

/*
============
WndCfg_DoDeleteProject
============
*/
void WndCfg_DoDeleteProject()
{
	char szMsg[256];
	int i = SendDlgItemMessage(hwndCfgProject, IDC_COMBO_PROJECT, CB_GETCURSEL, 0, 0);
	if (i == -1)
		return;
	sprintf(szMsg, "Are you sure you want to delete project %s?", g_qeconfig.projectPresets[i].name);
	if (MessageBox(g_qeglobals.d_hwndMain, szMsg, "QuakeEd 3: Confirm Project Delete", MB_YESNO | MB_ICONQUESTION) == IDNO)
		return;

	g_qeconfig.projectPresets.erase(g_qeconfig.projectPresets.begin() + i);
	if (g_qeconfig.projectPresets.size() == 0)
		g_qeconfig.projectPresets.emplace_back();	// always have one project
	WndCfg_ProjectPresetsToWnd();
}



/*
=====================================================================

	COLORS

=====================================================================
*/

struct cfgColorTab_s {
	int btnIDD = 0;
	vec3 color;			// 0-1 float
	HWND hwndBtn = 0;
	HBRUSH br = NULL;	// 0-255 int
};

#define CFG_COLORS 12
static cfgColorTab_s cfgColorTabs[CFG_COLORS];

/*
============
WndCfg_WndToColors
============
*/
void WndCfg_WndToColors(qecfgColors_t &colors = g_colors)
{
	colors.brush = cfgColorTabs[0].color;
	colors.selection = cfgColorTabs[1].color;
	colors.tool = cfgColorTabs[2].color;
	colors.camBackground = cfgColorTabs[3].color;
	colors.camGrid = cfgColorTabs[4].color;
	colors.gridBackground = cfgColorTabs[5].color;
	colors.gridMinor = cfgColorTabs[6].color;
	colors.gridMajor = cfgColorTabs[7].color;
	colors.gridBlock = cfgColorTabs[8].color;
	colors.gridText = cfgColorTabs[9].color;
	colors.texBackground = cfgColorTabs[10].color;
	colors.texText = cfgColorTabs[11].color;
}

/*
============
WndCfg_ColorMakeHBrush
============
*/
void WndCfg_ColorMakeHBrush(int i)
{
	if (cfgColorTabs[i].br)
		DeleteObject(cfgColorTabs[i].br);
	cfgColorTabs[i].br = CreateSolidBrush(RGB(
		(int)(cfgColorTabs[i].color.r * 255), 
		(int)(cfgColorTabs[i].color.g * 255),
		(int)(cfgColorTabs[i].color.b * 255)
	));
}

/*
============
WndCfg_SetColorTab
============
*/
void WndCfg_SetupColorTab(int i, int btnIDD, vec3 col)
{
	cfgColorTabs[i].btnIDD = btnIDD;
	cfgColorTabs[i].hwndBtn = GetDlgItem(hwndCfgColor, btnIDD);
	cfgColorTabs[i].color = col;
	WndCfg_ColorMakeHBrush(i);
}

/*
============
WndCfg_SetColorTabs
============
*/
void WndCfg_ColorsToWnd(qecfgColors_t &colors = g_colors)
{
	WndCfg_SetupColorTab(0, IDC_BUTTON_COLOR1, colors.brush);
	WndCfg_SetupColorTab(1, IDC_BUTTON_COLOR2, colors.selection);
	WndCfg_SetupColorTab(2, IDC_BUTTON_COLOR3, colors.tool);
	WndCfg_SetupColorTab(3, IDC_BUTTON_COLOR4, colors.camBackground);
	WndCfg_SetupColorTab(4, IDC_BUTTON_COLOR5, colors.camGrid);
	WndCfg_SetupColorTab(5, IDC_BUTTON_COLOR6, colors.gridBackground);
	WndCfg_SetupColorTab(6, IDC_BUTTON_COLOR7, colors.gridMinor);
	WndCfg_SetupColorTab(7, IDC_BUTTON_COLOR8, colors.gridMajor);
	WndCfg_SetupColorTab(8, IDC_BUTTON_COLOR9, colors.gridBlock);
	WndCfg_SetupColorTab(9, IDC_BUTTON_COLOR10, colors.gridText);
	WndCfg_SetupColorTab(10, IDC_BUTTON_COLOR11, colors.texBackground);
	WndCfg_SetupColorTab(11, IDC_BUTTON_COLOR12, colors.texText);
}

void WndCfg_FillColorCombo()
{
	SendDlgItemMessage(hwndCfgColor, IDC_COMBO_COLOR, CB_RESETCONTENT, 0, 0);

	SendDlgItemMessage(hwndCfgColor, IDC_COMBO_COLOR, CB_ADDSTRING, 0, (LPARAM)"Current");
	for (auto colIt = g_qeconfig.colorPresets.begin(); colIt != g_qeconfig.colorPresets.end(); ++colIt)
		SendDlgItemMessage(hwndCfgColor, IDC_COMBO_COLOR, CB_ADDSTRING, 0, (LPARAM)colIt->name);

	SendDlgItemMessage(hwndCfgColor, IDC_COMBO_COLOR, CB_SETCURSEL, 0,0);
}

/*
============
WndCfg_SelectColor
============
*/
void WndCfg_SelectColor(int btnIDD)
{
	for (int i = 0; i < CFG_COLORS; i++)
	{
		if (cfgColorTabs[i].btnIDD == btnIDD)
		{
			DoColorSelect(cfgColorTabs[i].color, cfgColorTabs[i].color);
			WndCfg_ColorMakeHBrush(i);
			return;
		}
	}
}

BOOL CALLBACK ConfigSaveColorsDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	char sz[MAX_PROJNAME];
	sz[0] = 0;
	int i;
	switch (uMsg)
	{
	case WM_INITDIALOG:
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			GetDlgItemText(hwndDlg, IDC_EDIT_NEWNAME, sz, MAX_PROJNAME-1);
			if (!*sz)
			{
				EndDialog(hwndDlg, 0);
				break;
			}
			i = SendDlgItemMessage(hwndCfgColor, IDC_COMBO_COLOR, CB_GETCURSEL, 0, 0);
			if (i==0)
				g_qeconfig.colorPresets.push_back(g_colors);
			else if (i > 0)
				g_qeconfig.colorPresets.push_back(g_qeconfig.colorPresets[i-1]);
			WndCfg_WndToColors(g_qeconfig.colorPresets.back());
			strcpy(g_qeconfig.colorPresets.back().name, sz);

			EndDialog(hwndDlg, 1);
			WndCfg_ColorsToWnd();
			WndCfg_FillColorCombo();
			break;
		case IDCANCEL:
			EndDialog(hwndDlg, 0);
			break;
		}
		return 0;
	}
	return FALSE;
}

void WndCfg_DoSaveColors() 
{
	DialogBox(g_qeglobals.d_hInstance, MAKEINTRESOURCE(IDD_NEWNAME), g_qeglobals.d_hwndMain, ConfigSaveColorsDlgProc);
}

void WndCfg_DoDeleteColors() 
{
	char szMsg[256];

	int i = SendDlgItemMessage(hwndCfgColor, IDC_COMBO_COLOR, CB_GETCURSEL, 0, 0);
	if (i < 1)
		return;
	sprintf(szMsg, "Are you sure you want to delete color preset %s?", g_qeconfig.colorPresets[i-1].name);
	if (MessageBox(g_qeglobals.d_hwndMain, szMsg, "QuakeEd 3: Confirm Preset Delete", MB_YESNO | MB_ICONQUESTION) == IDNO)
		return;

	g_qeconfig.colorPresets.erase(g_qeconfig.colorPresets.begin() + i - 1);
	WndCfg_ColorsToWnd();
	WndCfg_FillColorCombo();
}

void WndCfg_ColorComboChanged()
{
	int i = SendDlgItemMessage(hwndCfgColor, IDC_COMBO_COLOR, CB_GETCURSEL, 0, 0);
	if (i < 0)
		return;
	if (i)
		WndCfg_ColorsToWnd(g_qeconfig.colorPresets[i - 1]);
	else
		WndCfg_ColorsToWnd();
}

/*
=====================================================================

	MAIN WINDOW

=====================================================================
*/

/*
============
WndCfg_ListSel
============
*/
void WndCfg_ListSel(int idx)
{
	ShowWindow(hwndCfgEditor, SW_HIDE);
	ShowWindow(hwndCfgProject, SW_HIDE);
	ShowWindow(hwndCfgColor, SW_HIDE);
	ShowWindow(hwndCfgUI, SW_HIDE);

	switch (idx)
	{
	case 0:
		ShowWindow(hwndCfgEditor, SW_SHOWNORMAL);
		UpdateWindow(hwndCfgEditor);
		break;
	case 1:
		ShowWindow(hwndCfgUI, SW_SHOWNORMAL);
		UpdateWindow(hwndCfgUI);
		break;
	case 2:
		ShowWindow(hwndCfgProject, SW_SHOWNORMAL);
		UpdateWindow(hwndCfgProject);
		break;
	case 3:
		ShowWindow(hwndCfgColor, SW_SHOWNORMAL);
		UpdateWindow(hwndCfgColor);
		break;
	}
}

void WndCfg_ListSel(HWND hwndDlg)
{
	HWND	hwndList;
	int		iIndex;
	hwndList = GetDlgItem(hwndDlg, IDC_CONFIG_LIST);
	iIndex = SendMessage(hwndList, LB_GETCURSEL, 0, 0);
	if (iIndex != LB_ERR)
		WndCfg_ListSel(iIndex);
}

/*
============
WndCfg_ConfigToWnd
============
*/
void WndCfg_ConfigToWnd()
{
	char sz[128];

	// --------------------------------
	// Editor
	SetDlgItemText(hwndCfgEditor, IDC_EDIT_GAMEPATH, g_cfgEditor.QuakePath);

	sprintf(sz, "%d", (int)g_cfgEditor.UndoLevels);
	SetDlgItemText(hwndCfgEditor, IDC_EDIT_UNDOLEVELS, sz);

	SendDlgItemMessage(hwndCfgEditor, IDC_CHECK_AUTOSAVE, BM_SETCHECK, (g_cfgEditor.Autosave ? BST_CHECKED : BST_UNCHECKED), 0);
	sprintf(sz, "%d", (int)g_cfgEditor.AutosaveTime);
	SetDlgItemText(hwndCfgEditor, IDC_EDIT_AUTOSAVE, sz);

	SendDlgItemMessage(hwndCfgEditor, IDC_COMBO_CLONESTYLE, CB_SETCURSEL, g_cfgEditor.CloneStyle, 0);
	SendDlgItemMessage(hwndCfgEditor, IDC_COMBO_CAMSTYLE, CB_SETCURSEL, g_cfgEditor.CameraMoveStyle, 0);
	// TexProjectionMode is in the surface dlg

	SendDlgItemMessage(hwndCfgEditor, IDC_CHECK_LOGCONSOLE, BM_SETCHECK, (g_cfgEditor.LogConsole ? BST_CHECKED : BST_UNCHECKED), 0);
	SendDlgItemMessage(hwndCfgEditor, IDC_CHECK_VFEEXCLUSIVE, BM_SETCHECK, (g_cfgEditor.VFEModesExclusive ? BST_CHECKED : BST_UNCHECKED), 0);
	SendDlgItemMessage(hwndCfgEditor, IDC_CHECK_BRUSHPRECISION, BM_SETCHECK, (g_cfgEditor.BrushPrecision ? BST_CHECKED : BST_UNCHECKED), 0);
	SendDlgItemMessage(hwndCfgEditor, IDC_CHECK_LOADLASTMAP, BM_SETCHECK, (g_cfgEditor.LoadLastMap ? BST_CHECKED : BST_UNCHECKED), 0);

	sprintf(sz, "%d", g_cfgEditor.MapSize);
	SendDlgItemMessage(hwndCfgEditor, IDC_COMBO_MAPSIZE, CB_SELECTSTRING, (WPARAM)-1, (LPARAM)sz);

	// --------------------------------
	// UI
	SendDlgItemMessage(hwndCfgUI, IDC_CHECK_NOSTIPPLE, BM_SETCHECK, (g_cfgUI.Stipple ? BST_CHECKED : BST_UNCHECKED), 0);
	SendDlgItemMessage(hwndCfgUI, IDC_CHECK_RADIANTLIGHTS, BM_SETCHECK, (g_cfgUI.RadiantLights ? BST_CHECKED : BST_UNCHECKED), 0);

	SendDlgItemMessage(hwndCfgUI, IDC_COMBO_RENDERMODE, CB_SETCURSEL, g_cfgUI.TextureMode, 0);
		
	// --------------------------------
	// Project
	WndCfg_ProjectPresetsToWnd();

	// --------------------------------
	// Color
	WndCfg_ColorsToWnd();
	WndCfg_FillColorCombo();
}

/*
============
WndCfg_WndToConfigEditor
============
*/
void WndCfg_WndToConfigEditor(qecfgEditor_t &cfgEd)
{
	char	sz[128];

	GetDlgItemText(hwndCfgEditor, IDC_EDIT_GAMEPATH, cfgEd.QuakePath, 255);

	cfgEd.Autosave = SendDlgItemMessage(hwndCfgEditor, IDC_CHECK_AUTOSAVE, BM_GETCHECK, 0, 0) != 0;
	GetDlgItemText(hwndCfgEditor, IDC_EDIT_AUTOSAVE, sz, 4);
	cfgEd.AutosaveTime = atoi(sz);

	GetDlgItemText(hwndCfgEditor, IDC_EDIT_UNDOLEVELS, sz, 4);
	cfgEd.UndoLevels = atoi(sz);
	g_cmdQueue.SetSize((int)cfgEd.UndoLevels);

	GetDlgItemText(hwndCfgEditor, IDC_COMBO_MAPSIZE, sz, 8);
	cfgEd.MapSize = atoi(sz);

	cfgEd.CloneStyle = SendDlgItemMessage(hwndCfgEditor, IDC_COMBO_CLONESTYLE, CB_GETCURSEL, 0, 0);
	cfgEd.CameraMoveStyle = SendDlgItemMessage(hwndCfgEditor, IDC_COMBO_CAMSTYLE, CB_GETCURSEL, 0, 0);
	// TexProjectionMode is in the surface dlg

	cfgEd.LogConsole = SendDlgItemMessage(hwndCfgEditor, IDC_CHECK_LOGCONSOLE, BM_GETCHECK, 0, 0) != 0;
	cfgEd.VFEModesExclusive = SendDlgItemMessage(hwndCfgEditor, IDC_CHECK_VFEEXCLUSIVE, BM_GETCHECK, 0, 0) != 0;
	cfgEd.BrushPrecision = SendDlgItemMessage(hwndCfgEditor, IDC_CHECK_BRUSHPRECISION, BM_GETCHECK, 0, 0) != 0;
	cfgEd.LoadLastMap = SendDlgItemMessage(hwndCfgEditor, IDC_CHECK_LOADLASTMAP, BM_GETCHECK, 0, 0);
}

/*
============
WndCfg_WndToConfigUI
============
*/
void WndCfg_WndToConfigUI(qecfgUI_t &cfgUI)
{
	cfgUI.Stipple = SendDlgItemMessage(hwndCfgUI, IDC_CHECK_NOSTIPPLE, BM_GETCHECK, 0, 0);
	cfgUI.RadiantLights = SendDlgItemMessage(hwndCfgUI, IDC_CHECK_RADIANTLIGHTS, BM_GETCHECK, 0, 0);
	cfgUI.Gamma = SendDlgItemMessage(hwndCfgUI, IDC_SLIDER_GAMMA, TBM_GETPOS, 0, 0) * 0.1f;
	cfgUI.Brightness = SendDlgItemMessage(hwndCfgUI, IDC_SLIDER_BRIGHTNESS, TBM_GETPOS, 0, 0) * 0.1f;
	cfgUI.TextureMode = min(max(0, SendDlgItemMessage(hwndCfgUI, IDC_COMBO_RENDERMODE, CB_GETCURSEL, 0, 0)), 5);
}

/*
============
WndCfg_WndToConfigProject
============
*/
void WndCfg_WndToConfigProject(qecfgProject_t &prj)
{
	GetDlgItemText(hwndCfgProject, IDC_COMBO_PROJECT, prj.name, MAX_PROJNAME);

	GetDlgItemText(hwndCfgProject, IDC_EDIT_GAMEBASEPATH, prj.basePath, _MAX_DIR);
	GetDlgItemText(hwndCfgProject, IDC_EDIT_MAPSDIRECTORY, prj.mapPath, _MAX_DIR);
	GetDlgItemText(hwndCfgProject, IDC_EDIT_AUTOSAVEMAP, prj.autosaveFile, _MAX_FNAME);
	GetDlgItemText(hwndCfgProject, IDC_EDIT_ENTITYFILES, prj.entityFiles, _MAX_FNAME);
	GetDlgItemText(hwndCfgProject, IDC_EDIT_TEXTUREDIRECTORY, prj.wadPath, _MAX_DIR);
	GetDlgItemText(hwndCfgProject, IDC_EDIT_DEFAULTWADS, prj.defaultWads, _MAX_FNAME);
	GetDlgItemText(hwndCfgProject, IDC_EDIT_PALETTEFILE, prj.paletteFile, _MAX_FNAME);

	prj.extTargets = SendDlgItemMessage(hwndCfgProject, IDC_CHECK_EXTTARGETS, BM_GETCHECK, 0, 0) != 0;

}

/*
============
WndCfg_WndToConfig
============
*/
void WndCfg_WndToConfig(cfgContext_t cfgCtx)
{
	WndCfg_WndToConfigEditor(cfgCtx.cfgEd);
	WndCfg_WndToConfigUI(cfgCtx.cfgUI);
	WndCfg_WndToConfigProject(cfgCtx.cfgProj);
	WndCfg_WndToColors(cfgCtx.cfgColor);
}

bool WndCfg_VerifyConfig(cfgContext_t cfgCtx)
{
	int		nTexMode = g_cfgUI.TextureMode;
	bool	cfgOK;
	char	szErrors[2048];
	char*	errCur;

	errCur = szErrors;
	*errCur = 0;
	
	strcpy(errCur, "Unable to apply config for the following reasons:\n\n");
	errCur += 38;

	cfgOK = true;

	// Editor
	if (!strlen(cfgCtx.cfgEd.QuakePath))
	{
		strcpy(errCur, "No Quake directory specified.\n");
		errCur += 30;
		cfgOK = false;
	}

	// UI

	// Project
	if (!strlen(cfgCtx.cfgProj.basePath))
	{
		strcpy(errCur, "No game basepath specified.\n");
		errCur += 28;
		cfgOK = false;
	}

	if (!strlen(cfgCtx.cfgProj.mapPath))
	{
		strcpy(errCur, "No maps path specified.\n");
		errCur += 24;
		cfgOK = false;
	}

	if (!strlen(cfgCtx.cfgProj.autosaveFile))
	{
		strcpy(errCur, "No autosave map specified.\n");
		errCur += 27;
		cfgOK = false;
	}

	if (!strlen(cfgCtx.cfgProj.entityFiles))
	{
		strcpy(errCur, "No entity definitions specified.\n");
		errCur += 33;
		cfgOK = false;
	}

	if (!strlen(cfgCtx.cfgProj.wadPath))
	{
		strcpy(errCur, "No texture path specified.\n");
		errCur += 27;
		cfgOK = false;
	}

	// Color

	if (!cfgOK)
		MessageBox(g_qeglobals.d_hwndMain, szErrors, "QuakeEd 3: Error", MB_OK | MB_ICONEXCLAMATION);
	return cfgOK;
}

/*
============
WndCfg_ApplyToConfig
============
*/
void WndCfg_ApplyToConfig(cfgContext_t cfgCtx)
{
	bool reloadPalette = false;

	// Editor
	if (g_cfgEditor.MapSize != cfgCtx.cfgEd.MapSize)
		g_map.RegionOff();

	// UI
	if (cfgCtx.cfgUI.Gamma != g_cfgUI.Gamma || cfgCtx.cfgUI.Brightness != g_cfgUI.Brightness)
	{
		// TODO: gamma is still applied at texture load time :(
		g_cfgNeedReload = true;
		reloadPalette = true;
	}

	if (cfgCtx.cfgUI.TextureMode != g_cfgUI.TextureMode)
		Textures::SetTextureMode(g_cfgUI.TextureMode);

	// copy values to the real config
	g_cfgEditor = cfgCtx.cfgEd;
	g_cfgUI = cfgCtx.cfgUI;
	g_colors = cfgCtx.cfgColor;
	SendDlgItemMessage(hwndCfgColor, IDC_COMBO_COLOR, CB_SETCURSEL, 0, 0);

	if (reloadPalette)
		Textures::LoadPalette();

	int i = SendDlgItemMessage(hwndCfgProject, IDC_COMBO_PROJECT, CB_GETCURSEL, 0, 0);
	g_qeconfig.projectPresets[i] = cfgCtx.cfgProj;

	g_needApply = false;
}

/*
============
ConfigSubDlgProc

catch-all proc for all config subpanes
============
*/
BOOL CALLBACK ConfigSubDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int nAutosave, nUndoLevel;
	char sz[256];

	switch (uMsg)
	{
	case WM_INITDIALOG:
		return TRUE;

	case WM_HSCROLL:
		if ((HWND)lParam == GetDlgItem(hwndDlg, IDC_SLIDER_GAMMA) || 
			(HWND)lParam == GetDlgItem(hwndDlg, IDC_SLIDER_BRIGHTNESS))
			g_needApply = true;
		return TRUE;

	case WM_COMMAND:
		if (HIWORD(wParam) == EN_CHANGE)
		{
			switch (LOWORD(wParam))
			{
			case IDC_EDIT_GAMEBASEPATH:
			case IDC_EDIT_MAPSDIRECTORY:
			case IDC_EDIT_AUTOSAVEMAP:
			case IDC_EDIT_ENTITYFILES:
			case IDC_EDIT_TEXTUREDIRECTORY:
			case IDC_EDIT_DEFAULTWADS:
			case IDC_EDIT_PALETTEFILE:
			case IDC_EDIT_GAMEPATH:
			case IDC_EDIT_UNDOLEVELS:
			case IDC_EDIT_AUTOSAVE:
				g_needApply = true;
			}
			return TRUE;
		}

		if (HIWORD(wParam) == CBN_SELCHANGE)
		{
			switch (LOWORD(wParam))
			{
			case IDC_COMBO_RENDERMODE:
			case IDC_COMBO_MAPSIZE:
				g_needApply = true;
				return TRUE;
			case IDC_COMBO_PROJECT:
				g_needApply = true;
				WndCfg_ProjectComboChanged();
				return TRUE;
			case IDC_COMBO_COLOR:
				WndCfg_ColorComboChanged();
				WndCfg_ListSel(hwndCfg);
				g_needApply = true;
				return TRUE;
			}
		}

		switch (LOWORD(wParam))
		{
		case IDC_BUTTON_GAMEPATH:
			SelectDir(hwndDlg, IDC_EDIT_GAMEPATH, false, "Select Quake Directory");
			break;
		case IDC_BUTTON_GAMEBASEPATH:
			SelectDir(hwndDlg, IDC_EDIT_GAMEBASEPATH, true, "Select Game Base Directory");
			break;
		case IDC_BUTTON_MAPSDIRECTORY:
			SelectDir(hwndDlg, IDC_EDIT_MAPSDIRECTORY, true, "Select Maps Directory");
			break;
		case IDC_BUTTON_TEXTUREDIRECTORY:
			SelectDir(hwndDlg, IDC_EDIT_TEXTUREDIRECTORY, true, "Select Texture Directory");
			break;
		case IDC_BUTTON_AUTOSAVEMAP:
			WndCfg_GetAutosaveMap(hwndDlg);
			break;
		case IDC_BUTTON_ENTITYFILES:
			WndCfg_GetEntityFiles(hwndDlg);
			break;
		case IDC_BUTTON_DEFAULTWADS:
			WndCfg_GetDefaultWads(hwndDlg);
			break;
		case IDC_BUTTON_NEWPROJ:
			WndCfg_DoNewProject();
			break;
		case IDC_BUTTON_DELPROJ:
			WndCfg_DoDeleteProject();
			break;
		case IDC_BUTTON_COLOR1:
		case IDC_BUTTON_COLOR2:
		case IDC_BUTTON_COLOR3:
		case IDC_BUTTON_COLOR4:
		case IDC_BUTTON_COLOR5:
		case IDC_BUTTON_COLOR6:
		case IDC_BUTTON_COLOR7:
		case IDC_BUTTON_COLOR8:
		case IDC_BUTTON_COLOR9:
		case IDC_BUTTON_COLOR10:
		case IDC_BUTTON_COLOR11:
		case IDC_BUTTON_COLOR12:
			WndCfg_SelectColor(LOWORD(wParam));
			//InvalidateRect(hwndCfg, NULL, TRUE);
			//UpdateWindow(hwndCfg);
			WndCfg_ListSel(hwndCfg);
			g_needApply = true;
			break;
		case IDC_BUTTON_COLORSAVE:
			WndCfg_DoSaveColors();
			break;
		case IDC_BUTTON_COLORDELETE:
			WndCfg_DoDeleteColors();
			break;

		case IDC_CHECK_NOSTIPPLE:
		case IDC_CHECK_RADIANTLIGHTS:
		case IDC_CHECK_AUTOSAVE:
		case IDC_CHECK_LOGCONSOLE:
		case IDC_CHECK_VFEEXCLUSIVE:
		case IDC_CHECK_BRUSHPRECISION:
		case IDC_CHECK_LOADLASTMAP:
			g_needApply = true;
			break;
		}
		return 0;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code)
		{
		case UDN_DELTAPOS:
			switch ((int)wParam)
			{
			case IDC_SPIN_UNDOLEVELS:
				if (((LPNMUPDOWN)lParam)->iDelta < 0)
				{
					GetDlgItemText(hwndDlg, IDC_EDIT_UNDOLEVELS, sz, 255);
					nUndoLevel = min(1024, atoi(sz) + 1);
					sprintf(sz, "%d", nUndoLevel);
					SetDlgItemText(hwndDlg, IDC_EDIT_UNDOLEVELS, sz);
				}
				else
				{
					GetDlgItemText(hwndDlg, IDC_EDIT_UNDOLEVELS, sz, 255);
					nUndoLevel = max(1, atoi(sz) - 1);
					sprintf(sz, "%d", nUndoLevel);
					SetDlgItemText(hwndDlg, IDC_EDIT_UNDOLEVELS, sz);
				}
				break;
			case IDC_SPIN_AUTOSAVE:
				if (((LPNMUPDOWN)lParam)->iDelta < 0)
				{
					GetDlgItemText(hwndDlg, IDC_EDIT_AUTOSAVE, sz, 255);
					nAutosave = min(60, atoi(sz) + 1);
					sprintf(sz, "%d", nAutosave);
					SetDlgItemText(hwndDlg, IDC_EDIT_AUTOSAVE, sz);
				}
				else
				{
					GetDlgItemText(hwndDlg, IDC_EDIT_AUTOSAVE, sz, 255);
					nAutosave = max(1, atoi(sz) - 1);
					sprintf(sz, "%d", nAutosave);
					SetDlgItemText(hwndDlg, IDC_EDIT_AUTOSAVE, sz);
				}
				break;
			}
			return 0;
		}
		return 0;
	case WM_CTLCOLORBTN:
		for (int i = 0; i < CFG_COLORS; i++)
		{
			if (cfgColorTabs[i].hwndBtn == (HWND)lParam)
				return (LRESULT)cfgColorTabs[i].br;
		}
		return 0;
	}

	return FALSE;
}

/*
============
WndCfg_CreateWnd
============
*/
void WndCfg_CreateWnd(HWND hwndDlg)
{
	HBITMAP hb = (HBITMAP)LoadImage(g_qeglobals.d_hInstance, (LPCTSTR)IDB_FIND, IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE | LR_LOADTRANSPARENT | LR_LOADMAP3DCOLORS);

	SendDlgItemMessage(hwndCfg, IDC_CONFIG_LIST, LB_ADDSTRING, 0, (LPARAM)"Editor");
	SendDlgItemMessage(hwndCfg, IDC_CONFIG_LIST, LB_ADDSTRING, 0, (LPARAM)"UI");
	SendDlgItemMessage(hwndCfg, IDC_CONFIG_LIST, LB_ADDSTRING, 0, (LPARAM)"Project");
	SendDlgItemMessage(hwndCfg, IDC_CONFIG_LIST, LB_ADDSTRING, 0, (LPARAM)"Colors");

	// --------------------------------
	// Editor

	hwndCfgEditor = CreateDialog(g_qeglobals.d_hInstance, MAKEINTRESOURCE(IDD_CONFIG_EDITOR), hwndCfg, (DLGPROC)ConfigSubDlgProc);
	SetWindowPos(hwndCfgEditor, HWND_TOP, 160, 12, 480, 325, SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOZORDER);
	ShowWindow(hwndCfgEditor, SW_HIDE);
	//UpdateWindow(hwndCfgEditor);

	SendDlgItemMessage(hwndCfgEditor, IDC_COMBO_MAPSIZE, CB_ADDSTRING, 0, (LPARAM)"8192 (default)");
	SendDlgItemMessage(hwndCfgEditor, IDC_COMBO_MAPSIZE, CB_ADDSTRING, 0, (LPARAM)"16384");
	SendDlgItemMessage(hwndCfgEditor, IDC_COMBO_MAPSIZE, CB_ADDSTRING, 0, (LPARAM)"32768");
	SendDlgItemMessage(hwndCfgEditor, IDC_COMBO_MAPSIZE, CB_ADDSTRING, 0, (LPARAM)"65536");

	SendDlgItemMessage(hwndCfgEditor, IDC_COMBO_CLONESTYLE, CB_ADDSTRING, 0, (LPARAM)"Press, w/ offset");
	SendDlgItemMessage(hwndCfgEditor, IDC_COMBO_CLONESTYLE, CB_ADDSTRING, 0, (LPARAM)"Press, in place");
	SendDlgItemMessage(hwndCfgEditor, IDC_COMBO_CLONESTYLE, CB_ADDSTRING, 0, (LPARAM)"Hold and drag");

	SendDlgItemMessage(hwndCfgEditor, IDC_COMBO_CAMSTYLE, CB_ADDSTRING, 0, (LPARAM)"Classic (RMB drive)");
	SendDlgItemMessage(hwndCfgEditor, IDC_COMBO_CAMSTYLE, CB_ADDSTRING, 0, (LPARAM)"WASD (RMB pan)");

	SendDlgItemMessage(hwndCfgEditor, IDC_BUTTON_GAMEPATH, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hb);

	// --------------------------------
	// UI

	hwndCfgUI = CreateDialog(g_qeglobals.d_hInstance, MAKEINTRESOURCE(IDD_CONFIG_UI), hwndCfg, (DLGPROC)ConfigSubDlgProc);
	SetWindowPos(hwndCfgUI, HWND_TOP, 160, 12, 480, 325, SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOZORDER);
	SendDlgItemMessage(hwndCfgUI, IDC_SLIDER_GAMMA, TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(0, 20));
	SendDlgItemMessage(hwndCfgUI, IDC_SLIDER_GAMMA, TBM_SETTICFREQ, (WPARAM)2, (LPARAM)0);
	SendDlgItemMessage(hwndCfgUI, IDC_SLIDER_GAMMA, TBM_SETLINESIZE, 0, (LPARAM)1);
	SendDlgItemMessage(hwndCfgUI, IDC_SLIDER_GAMMA, TBM_SETPAGESIZE, 0, (LPARAM)2);
	SendDlgItemMessage(hwndCfgUI, IDC_SLIDER_GAMMA, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)(g_cfgUI.Gamma * 10));

	SendDlgItemMessage(hwndCfgUI, IDC_SLIDER_BRIGHTNESS, TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(0, 20));
	SendDlgItemMessage(hwndCfgUI, IDC_SLIDER_BRIGHTNESS, TBM_SETTICFREQ, (WPARAM)2, (LPARAM)0);
	SendDlgItemMessage(hwndCfgUI, IDC_SLIDER_BRIGHTNESS, TBM_SETLINESIZE, 0, (LPARAM)1);
	SendDlgItemMessage(hwndCfgUI, IDC_SLIDER_BRIGHTNESS, TBM_SETPAGESIZE, 0, (LPARAM)2);
	SendDlgItemMessage(hwndCfgUI, IDC_SLIDER_BRIGHTNESS, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)(g_cfgUI.Brightness * 10));

	SendDlgItemMessage(hwndCfgUI, IDC_COMBO_RENDERMODE, CB_ADDSTRING, 0, (LPARAM)"Nearest");
	SendDlgItemMessage(hwndCfgUI, IDC_COMBO_RENDERMODE, CB_ADDSTRING, 0, (LPARAM)"Nearest Mipmap");
	SendDlgItemMessage(hwndCfgUI, IDC_COMBO_RENDERMODE, CB_ADDSTRING, 0, (LPARAM)"Linear");
	SendDlgItemMessage(hwndCfgUI, IDC_COMBO_RENDERMODE, CB_ADDSTRING, 0, (LPARAM)"Bilinear");
	SendDlgItemMessage(hwndCfgUI, IDC_COMBO_RENDERMODE, CB_ADDSTRING, 0, (LPARAM)"Bilinear Mipmap");
	SendDlgItemMessage(hwndCfgUI, IDC_COMBO_RENDERMODE, CB_ADDSTRING, 0, (LPARAM)"Trilinear");

	ShowWindow(hwndCfgUI, SW_HIDE);

	// --------------------------------
	// Project

	hwndCfgProject = CreateDialog(g_qeglobals.d_hInstance, MAKEINTRESOURCE(IDD_CONFIG_PROJECT), hwndCfg, (DLGPROC)ConfigSubDlgProc);
	SetWindowPos(hwndCfgProject, HWND_TOP, 160, 12, 480, 325, SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOZORDER);
	ShowWindow(hwndCfgProject, SW_HIDE);

	SendDlgItemMessage(hwndCfgProject, IDC_BUTTON_GAMEBASEPATH, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hb);
	SendDlgItemMessage(hwndCfgProject, IDC_BUTTON_MAPSDIRECTORY, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hb);
	SendDlgItemMessage(hwndCfgProject, IDC_BUTTON_AUTOSAVEMAP, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hb);
	SendDlgItemMessage(hwndCfgProject, IDC_BUTTON_ENTITYFILES, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hb);
	SendDlgItemMessage(hwndCfgProject, IDC_BUTTON_TEXTUREDIRECTORY, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hb);
	SendDlgItemMessage(hwndCfgProject, IDC_BUTTON_DEFAULTWADS, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hb);
	SendDlgItemMessage(hwndCfgProject, IDC_BUTTON_PALETTEFILE, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hb);

	// --------------------------------
	// Color

	hwndCfgColor = CreateDialog(g_qeglobals.d_hInstance, MAKEINTRESOURCE(IDD_CONFIG_COLOR), hwndCfg, (DLGPROC)ConfigSubDlgProc);
	SetWindowPos(hwndCfgColor, HWND_TOP, 160, 12, 480, 325, SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOZORDER);
	WndCfg_ColorsToWnd();
	WndCfg_FillColorCombo();
	ShowWindow(hwndCfgColor, SW_HIDE);

	// --------------------------------

	WndCfg_ConfigToWnd();
	if (g_noProject)
	{
		SendDlgItemMessage(hwndCfg, IDC_CONFIG_LIST, LB_SETCURSEL, 2, 0);
		WndCfg_ListSel(2);
		SelectDir(hwndCfgEditor, IDC_EDIT_GAMEPATH, false, "This is your first time running QuakeEd3. Please select your Quake directory:");
	}
	else
	{
		SendDlgItemMessage(hwndCfg, IDC_CONFIG_LIST, LB_SETCURSEL, 0, 0);
		WndCfg_ListSel(0);
	}

	g_needApply = false;
}


void WndCfg_OnApply()
{
	qecfgEditor_t cfgEd = g_cfgEditor;
	qecfgUI_t cfgUI = g_cfgUI;
	qecfgProject_t cfgProj;
	qecfgColors_t cfgColor;
	cfgContext_t cfgCtx(cfgEd, cfgUI, cfgProj, cfgColor);

	// copy window values to temp values
	WndCfg_WndToConfig(cfgCtx);

	// check temp values for errors
	if (!WndCfg_VerifyConfig(cfgCtx))
		return;

	// apply to live config
	WndCfg_ApplyToConfig(cfgCtx);
}

bool WndCfg_OnClose()
{
	qecfgEditor_t cfgEd = g_cfgEditor;
	qecfgUI_t cfgUI = g_cfgUI;
	qecfgProject_t cfgProj;
	qecfgColors_t cfgColor;
	cfgContext_t cfgCtx(cfgEd, cfgUI, cfgProj, cfgColor);

	// copy window values to temp values
	WndCfg_WndToConfig(cfgCtx);

	// compare to live config, ask to apply
	if (g_needApply)
	{
		int res = MessageBox(g_qeglobals.d_hwndMain, "Apply changes?", "QuakeEd 3: Confirm Changes", MB_YESNOCANCEL | MB_ICONQUESTION);
		if (res == IDCANCEL)
			return false;
		if (res == IDNO)
			return true;

		// check temp values for errors
		if (!WndCfg_VerifyConfig(cfgCtx))
			return false;

		// apply to live config
		WndCfg_ApplyToConfig(cfgCtx);
	}
	return true;
}

/*
============
ConfigDlgProc
============
*/
BOOL CALLBACK ConfigDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		hwndCfg = hwndDlg;
		GetCurrentDirectory(MAX_PATH, g_szCurrentDirectory);
		WndCfg_CreateWnd(hwndDlg);
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDAPPLY:
			WndCfg_OnApply();
			Sys_ForceUpdateWindows(W_SCENE);
			return TRUE;
		case IDCLOSE:
			if (!WndCfg_OnClose())
				return TRUE;
			EndDialog(hwndDlg, 0);
			WndCfg_CheckProjectAlteration();
			g_noProject = false;
			return TRUE;
		case IDC_CONFIG_LIST:
			switch (HIWORD(wParam))
			{
			case LBN_SELCHANGE:
				WndCfg_ListSel(hwndDlg);
				return TRUE;
			}
		}
		return FALSE;
	case WM_CLOSE:
		EndDialog(hwndDlg, 0);
		WndCfg_CheckProjectAlteration();
		g_noProject = false;
		return TRUE;
	}
	return FALSE;
}



void DoConfigWindow()
{
	g_noProject = false;
	g_cfgNeedReload = false;
	DialogBox(g_qeglobals.d_hInstance, MAKEINTRESOURCE(IDD_CONFIG), g_qeglobals.d_hwndMain, ConfigDlgProc);
}

void DoConfigWindowProject()
{
	g_noProject = true;
	g_cfgNeedReload = false;
	DialogBox(g_qeglobals.d_hInstance, MAKEINTRESOURCE(IDD_CONFIG), g_qeglobals.d_hwndMain, ConfigDlgProc);
}