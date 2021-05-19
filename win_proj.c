//==============================
//	win_proj.c
//==============================

//=================================================
//
//	SIKK - PROJECT SETTINGS DIALOG
//
//	*ripped from QE5
//=================================================

#include "qe3.h"
#include "io.h"
#include <Shlobj.h>


#define MAX_BSPCOMMANDS 64

bool	g_bFirstProject;

char   *g_szBspNames[MAX_BSPCOMMANDS];
char   *g_szBspCommands[MAX_BSPCOMMANDS];

char	g_szProjectDir[MAX_PATH];
char	g_szCurrentDirectory[MAX_PATH];

//=======================================================================

/*
===========================================================

	BSP FUNCTIONS

===========================================================
*/

/*
==================
GetNextFreeBspIndex
==================
*/
int GetNextFreeBspIndex ()
{
	int i;

	for (i = 0; i < MAX_BSPCOMMANDS; i++)
		if (!g_szBspNames[i])
			return i;

	return -1;
}

/*
==================
GetBspIndex
==================
*/
int GetBspIndex (char *text)
{
	int i;

	for (i = 0; i < MAX_BSPCOMMANDS; i++)
		if (g_szBspNames[i] && !strcmp(g_szBspNames[i], text))
			return i;

	return -1;
}

/*
==================
NewBspCommand
==================
*/
void NewBspCommand (HWND hwndDlg)
{
	HWND	h = GetDlgItem(hwndDlg, IDC_COMBO_BSPNAME);	
	int		i = SendMessage(h, CB_GETCURSEL, 0, 0);
	int		index = GetNextFreeBspIndex();
	char	buf[256], buf2[256], sz[128];

	if (index == -1)
	{
		MessageBox(g_qeglobals.d_hwndMain, "Unable to add new BSP command.\nAn existing command must be deleted.", "QuakeEd 3: Error", MB_OK | MB_ICONEXCLAMATION);
		return;
	}
	if (!GetDlgItemText(hwndDlg, IDC_COMBO_BSPNAME, buf, sizeof(buf)))
	{
		MessageBox(g_qeglobals.d_hwndMain, "Name field must be filled in.", "QuakeEd 3: Error", MB_OK | MB_ICONEXCLAMATION);
		return;
	}

	strcpy(buf2, buf);

	if (SendMessage(h, CB_FINDSTRING, (WPARAM)-1, (LPARAM)buf2) != CB_ERR)
	{
		sprintf(sz, "\"%s\" already exits.\nNew command name must be unique.", buf);
		MessageBox(g_qeglobals.d_hwndMain, sz, "QuakeEd 3: Error", MB_OK | MB_ICONEXCLAMATION);
		return;
	}

	i = SendMessage(h, CB_ADDSTRING, 0, (LPARAM)buf);

	if (i != CB_ERR && i != CB_ERRSPACE)
	{
		g_szBspNames[index] = strdup(buf);
		g_szBspCommands[index] = strdup("");
		SendMessage(h, CB_SETCURSEL, (WPARAM)i, 0);
		SendMessage(hwndDlg, WM_COMMAND, (WPARAM)(CBN_SELCHANGE << 16) + IDC_COMBO_BSPNAME, (LPARAM)h);
	}
	else
		MessageBox(g_qeglobals.d_hwndMain, "Error adding string to combobox", "QuakeEd 3: Error", MB_OK | MB_ICONEXCLAMATION);
}

/*
==================
DeleteBspCommand
==================
*/
void DeleteBspCommand (HWND hwndDlg)
{
	HWND	h = GetDlgItem(hwndDlg, IDC_COMBO_BSPNAME);	
	char	buf[256];
	int		i, i2 = SendMessage(h, CB_GETCURSEL, 0, 0);
	
	SendMessage(h, CB_GETLBTEXT, (WPARAM)i2, (LPARAM)buf);

	i = GetBspIndex(buf);

	if (i == -1)
	{
		MessageBox(g_qeglobals.d_hwndMain, "Could not delete BSP command: Unmatched.", "QuakeEd 3: Error", MB_OK | MB_ICONEXCLAMATION);
		return;
	}

	free(g_szBspNames[i]);

	if (g_szBspCommands[i])
		free(g_szBspCommands[i]);

	g_szBspNames[i] = g_szBspCommands[i] = NULL;

	SendMessage(h, CB_DELETESTRING, (WPARAM)i2, 0);
	SendMessage(h, CB_SETCURSEL, 0, 0);
	SendMessage(hwndDlg, WM_COMMAND, (WPARAM)(CBN_SELCHANGE << 16) + IDC_COMBO_BSPNAME, (LPARAM)h);
}

/*
==================
AcceptBspCommand
==================
*/
void AcceptBspCommand (HWND hwndDlg)
{
	HWND	h = GetDlgItem(hwndDlg, IDC_COMBO_BSPNAME);	
	int		index;
	int		i = SendMessage(h, CB_GETCURSEL, 0, 0);
	char	buf[256], buf2[512];

	SendMessage(h, CB_GETLBTEXT, (WPARAM)i, (LPARAM)buf);
	index = GetBspIndex(buf);
	GetDlgItemText(hwndDlg, IDC_EDIT_BSPCOMMAND, buf2, 511);
	free(g_szBspCommands[index]);
	g_szBspCommands[index] = strdup(buf2);
}

//=======================================================================


static OPENFILENAME ofn;					// common dialog box structure
static char szDirName[MAX_PATH];			// directory string
static char szFile[MAX_PATH];				// filename string
static char szFileTitle[_MAX_FNAME];		// file title string
static char szProjectFilter[260] = 
	"QuakeEd project (*.qe3)\0*.qe3\0\0";	// filter string

/*
==================
SelectDir
==================
*/
bool SelectDir (HWND h)
{
	BROWSEINFO bi = { 0 };
	LPITEMIDLIST pidl;

	bi.hwndOwner = h; 
	bi.lpszTitle = "Choose a Directory";
	pidl = SHBrowseForFolder(&bi);
	
	if (pidl != 0)
	{
		char DirName[MAX_PATH];

		if (SHGetPathFromIDList(pidl, DirName))
		{
			SetStr(szDirName, DirName, "\\", NULL, NULL);
			SetWindowText(h, szDirName);
		}
		
		GlobalFree(pidl);
		return true;
	}
	return false;
}


/*
===========================================================

	GET FUNCTIONS

===========================================================
*/

/*
==================
GetProjectDirectory
==================
*/
void GetProjectDirectory (HWND hwndDlg)
{
	HWND hwndEdit = GetDlgItem(hwndDlg, IDC_EDIT_PROJECTDIRECTORY);

	if (!SendMessage(hwndEdit, WM_GETTEXT, (WPARAM)MAX_PATH - 1, (LPARAM)szDirName))
		strcpy(szDirName, g_szCurrentDirectory);

	SelectDir(hwndEdit);
}

/*
==================
GetRemoteBasePath
==================
*/
void GetRemoteBasePath (HWND hwndDlg)
{
	HWND hwndEdit = GetDlgItem(hwndDlg, IDC_EDIT_REMOTEBASEPATH);

	if (!SendMessage(hwndEdit, WM_GETTEXT, (WPARAM)MAX_PATH - 1, (LPARAM)szDirName))
		strcpy(szDirName, g_szCurrentDirectory);

	SelectDir(hwndEdit);
}

/*
==================
GetMapsDirectory
==================
*/
void GetMapsDirectory (HWND hwndDlg)
{
	HWND hwndEdit = GetDlgItem(hwndDlg, IDC_EDIT_MAPSDIRECTORY);

	if (!SendMessage(hwndEdit, WM_GETTEXT, (WPARAM)MAX_PATH - 1, (LPARAM)szDirName))
		strcpy(szDirName, g_szCurrentDirectory);

	SelectDir(hwndEdit);
}

/*
==================
GetAutosaveMap
==================
*/
void GetAutosaveMap (HWND hwndDlg)
{
	sprintf(szDirName, "%smaps", g_szCurrentDirectory);

	szFile[0] = 0;

	GetDlgItemText(hwndDlg, IDC_EDIT_PROJECTDIRECTORY, g_szProjectDir, 511);

	GetCurrentDirectory(MAX_PATH - 1, szDirName);

	ofn.lStructSize		= sizeof(OPENFILENAME);
	ofn.hwndOwner		= hwndDlg;
	ofn.lpstrFilter		= "QuakeEd File (*.map)\0*.map\0\0";
	ofn.nFilterIndex	= 1;
	ofn.lpstrFile		= szFile;
	ofn.nMaxFile		= sizeof(szFile);
	ofn.lpstrFileTitle	= szFileTitle;
	ofn.nMaxFileTitle	= sizeof(szFileTitle);
	ofn.lpstrInitialDir	= szDirName;
	ofn.Flags			= OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
	ofn.lpstrTitle		= "Select Auto-Save Map File";

	if (GetSaveFileName(&ofn))
	{
		if (!strncmp(szFile, g_szProjectDir, strlen(g_szProjectDir)))
			SetDlgItemText(hwndDlg, IDC_EDIT_AUTOSAVEMAP, &szFile[strlen(g_szProjectDir)]);
		else
			SetDlgItemText(hwndDlg, IDC_EDIT_AUTOSAVEMAP, szFile);
	}
}

/*
==================
GetEntityFiles
==================
*/
void GetEntityFiles (HWND hwndDlg)
{
	szFile[0] = 0;

	GetDlgItemText(hwndDlg, IDC_EDIT_PROJECTDIRECTORY, g_szProjectDir, 511);

	MessageBox(g_qeglobals.d_hwndMain, "To load multiple entity definition files,\nuse the wildcard expression (EG: *.def or *.qc)", "Quake Ed 3: Info", MB_OK | MB_ICONINFORMATION);
	
	GetCurrentDirectory(MAX_PATH - 1, szDirName);

	ofn.lStructSize		= sizeof(OPENFILENAME);
	ofn.hwndOwner		= hwndDlg;
	ofn.lpstrFilter		= "QuakeEd Entity Definition File (*.qc)\0*.qc\0QuakeEd Entity Definition File (*.def)\0*.def\0QuakeEd Entity Definition File (*.c)\0*.c\0All Files\0*.*\0\0";
	ofn.nFilterIndex	= 1;
	ofn.lpstrFile		= szFile;
	ofn.nMaxFile		= sizeof(szFile);
	ofn.lpstrFileTitle	= szFileTitle;
	ofn.nMaxFileTitle	= sizeof(szFileTitle);
	ofn.lpstrInitialDir	= g_szCurrentDirectory;
	ofn.Flags			= OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
	ofn.lpstrTitle		= "Select Entity Definition File";

	if (GetOpenFileName(&ofn))
	{
		if (!strncmp(szFile, g_szProjectDir, strlen(g_szProjectDir)))
			SetDlgItemText(hwndDlg, IDC_EDIT_ENTITYFILES, &szFile[strlen(g_szProjectDir)]);
		else
			SetDlgItemText(hwndDlg, IDC_EDIT_ENTITYFILES, szFile);
	}
}

/*
==================
GetTextureDirectory
==================
*/
void GetTextureDirectory (HWND hwndDlg)
{
	HWND hwndEdit = GetDlgItem(hwndDlg, IDC_EDIT_TEXTUREDIRECTORY);

	GetDlgItemText(hwndDlg, IDC_EDIT_PROJECTDIRECTORY, g_szProjectDir, MAX_PATH - 1);

	szDirName[0] = 0;

	if (!SendMessage(hwndEdit, WM_GETTEXT, (WPARAM)MAX_PATH - 1, (LPARAM)szDirName))
		strcpy(szDirName, g_szCurrentDirectory);

	// Strip off Project Directory string else wads won't load. I'm looking into a fix
	// so you can load wads from any dir instead of it having to be within in project dir.
	if (SelectDir(hwndEdit))
		SetDlgItemText(hwndDlg, IDC_EDIT_TEXTUREDIRECTORY, &szDirName[strlen(g_szProjectDir)]);
}

/*
==================
GetDefaultWads
==================
*/
void GetDefaultWads(HWND hwndDlg)
{
	HWND hwndEdit = GetDlgItem(hwndDlg, IDC_EDIT_TEXTUREDIRECTORY);

	if (!SendMessage(hwndEdit, WM_GETTEXT, (WPARAM)MAX_PATH - 1, (LPARAM)szDirName))
		strcpy(szDirName, g_szCurrentDirectory);

	szFile[0] = 0;

	ofn.lStructSize		= sizeof(OPENFILENAME);
	ofn.hwndOwner		= hwndDlg;
	ofn.lpstrFilter		= "QuakeEd Wad File (*.wad)\0*.wad\0\0";
	ofn.nFilterIndex	= 1;
	ofn.lpstrFile		= szFile;
	ofn.nMaxFile		= sizeof(szFile);
	ofn.lpstrFileTitle	= szFileTitle;
	ofn.nMaxFileTitle	= sizeof(szFileTitle);
	ofn.lpstrInitialDir	= szDirName;
	ofn.Flags			= OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_LONGNAMES | 
						  OFN_EXPLORER | OFN_ALLOWMULTISELECT;
	ofn.lpstrTitle		= "Select Default Wad File(s)";

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
	}
	else
	{
		switch(CommDlgExtendedError())
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
}

/*
==================
GetToolsDirectory
==================
*/
void GetToolsDirectory (HWND hwndDlg)
{
	HWND hwndEdit = GetDlgItem(hwndDlg, IDC_EDIT_TOOLSDIRECTORY);

	if (!SendMessage(hwndEdit, WM_GETTEXT, (WPARAM)MAX_PATH - 1, (LPARAM)szDirName))
		strcpy(szDirName, g_szCurrentDirectory);

	SelectDir(hwndEdit);
}

//=======================================================================

/*
==================
SaveSettings
==================
*/
void SaveSettings (HWND hwndDlg)
{
	int		i, i2, i3;
	char	buf[MAX_PATH], g_szProjectDir[MAX_PATH], prev;
	FILE   *file;

	GetDlgItemText(hwndDlg, IDC_EDIT_PROJECTDIRECTORY, buf, MAX_PATH);
	SetDirStr(g_szProjectDir, buf, "/", NULL);

	if (g_bFirstProject) //scripts directory probably doesn't exist
	{
		sprintf(buf, "%sscripts", g_szProjectDir);
		CreateDirectory(buf, NULL);
	}

	strcpy(buf, g_qeglobals.d_savedinfo.szLastProject);

	if (!(file = fopen(buf, "w")))
	{
		MessageBox(hwndDlg, "Could not open config file for writing.", "QuakeEd 3: Error", MB_OK | MB_ICONEXCLAMATION);
		return;
	}
	
	fprintf(file, "{\n");
	fprintf(file, "// Maps will be loaded and saved from <basepath>/maps\n");
	GetDlgItemText(hwndDlg, IDC_EDIT_PROJECTDIRECTORY, buf, MAX_PATH);
	fprintf(file, "\"basepath\" \"%s\"\n", buf);

	fprintf(file, "\n");
	fprintf(file, "// If you are using your local machine to bsp, set rshcmd to \"\" and\n");
	fprintf(file, "// remotebasepath to the same thing as basepath.\n");
	fprintf(file, "// If you are using a remote unix host, remotebasepath will usually be different.\n");
	GetDlgItemText(hwndDlg, IDC_EDIT_REMOTEBASEPATH, buf, MAX_PATH);
	if (buf[0])
		fprintf(file, "\"remotebasepath\" \"%s\"\n", buf);
	else
	{
		GetDlgItemText(hwndDlg, IDC_EDIT_PROJECTDIRECTORY, buf, MAX_PATH);
		fprintf(file, "\"remotebasepath\" \"%s\"\n", buf);
	}

	fprintf(file, "\n");
	fprintf(file, "// Every \"!\" symbol in a BSP command will be replaced\n");
	fprintf(file, "// with the text that is in <rshcmd>.\n");
	GetDlgItemText(hwndDlg, IDC_EDIT_TOOLSDIRECTORY, buf, MAX_PATH);
	fprintf(file, "\"rshcmd\" \"%s\"\n", buf);

	fprintf(file, "\n");
	fprintf(file, "// Every five minutes, the editor will autosave a map if it is dirty.\n");
	fprintf(file, "// This should be on a local drive, so multiple people editing maps don't collide\n");
	GetDlgItemText(hwndDlg, IDC_EDIT_MAPSDIRECTORY, buf, MAX_PATH);
	fprintf(file, "\"mapspath\" \"%s\"\n", buf);
	GetDlgItemText(hwndDlg, IDC_EDIT_AUTOSAVEMAP, buf, MAX_PATH);
	fprintf(file, "\"autosave\" \"%s\"\n", buf);

	fprintf(file, "\n");
	fprintf(file, "// The entity classes are found by parsing through all the\n");
	fprintf(file, "// files in <entitypath>, looking for /*QUAKED comments\n");
	GetDlgItemText(hwndDlg, IDC_EDIT_ENTITYFILES, buf, MAX_PATH);
	fprintf(file, "\"entitypath\" \"%s\"\n", buf);

	fprintf(file, "\n");
	fprintf(file, "// The \"Textures\" menu is filled with all of the wads found under <texturepath>.\n");
	fprintf(file, "// All texture references from maps have <texturepath> implicitly prepended.\n");
	fprintf(file, "// The textures directory can be duplicated on a local harddrive for better performance.\n");
	GetDlgItemText(hwndDlg, IDC_EDIT_TEXTUREDIRECTORY, buf, MAX_PATH);
	fprintf(file, "\"texturepath\" \"%s\"\n", buf);

	fprintf(file, "\n");
	fprintf(file, "// All wad files specified under <defaultwads> will be automatically added\n");
	fprintf(file, "// to the map's \"worldspawn\" \'wad\' key when a new map is started.\n");
	GetDlgItemText(hwndDlg, IDC_EDIT_DEFAULTWADS, buf, MAX_PATH);
	fprintf(file, "\"defaultwads\" \"%s\"\n", buf);

	fprintf(file, "\n");
	fprintf(file, "// The \"BSP\" menu in QuakeEd is filled with the following bsp_* commands when\n");
	fprintf(file, "// selected, the value string will be expanded then executed in the background.\n");
	fprintf(file, "// ! will be replaced with <rshcmd>\n");
	fprintf(file, "// $ will be replaced with <remotebasepath>/maps/<currentmap>\n");
	fprintf(file, "// @ is changed to a quote(in case you need one in the command line)\n");
	for (i = 0; i < MAX_BSPCOMMANDS; i++)
	{
		if (g_szBspNames[i] && g_szBspCommands[i][0]) // Only write out a BSP line if there is a command for it
		{
			prev = 0;
			for (i2 = i3 = 0; g_szBspCommands[i][i2]; i2++)
			{
				switch (g_szBspCommands[i][i2])
				{
				case ' ':
					if (prev != ' ')
						prev = buf[i3++] = ' ';
					break;
				case '\t':
					if (prev != ' ')
						prev = buf[i3++] = ' ';
					break;
				case 13:
					if (prev != ' ')
						prev = buf[i3++] = ' ';
					else
						prev = 13;
					break;
				case 10:
					if (prev != ' ')
						buf[i3++] = ' ';
					buf[i3++] = '&';
					buf[i3++] = '&';
					buf[i3++] = ' ';
					while (g_szBspCommands[i][i2] != 0 && (g_szBspCommands[i][i2] == ' ' || g_szBspCommands[i][i2] == '\t'))
						i2++;
					prev = 10;
					break;
				default:
						prev = buf[i3++] = g_szBspCommands[i][i2];
						break;
				}
			}
			buf[i3] = 0;
			fprintf(file, "\"bsp_%s\" \"%s\"\n", g_szBspNames[i], buf);
		}
	}

	fprintf(file, "}\n");
	fclose(file);

	if (!g_bFirstProject)
		MessageBox(g_qeglobals.d_hwndMain, "New settings require a restart to take effect.", "QuakeEd 3: Project Settings", MB_OK | MB_ICONINFORMATION);
}

//=======================================================================

/*
==================
ProjectSettingsDlgProc
==================
*/
BOOL CALLBACK ProjectSettingsDlgProc (
    HWND	hwndDlg,// handle to dialog box
    UINT	uMsg,	// message
    WPARAM	wParam,	// first message parameter
    LPARAM	lParam 	// second message parameter
   )
{
	int			index, i, i2;
	char	   *basepath,
			   *remotebasepath,
			   *mapspath,
			   *autosavemap,
			   *entitypath,
			   *texturepath,
			   *defaultwads,
			   *toolspath,
			   *name, *value, prev, buf[512];
	epair_t	   *ep;
	HWND		h;
	HBITMAP		hb;

	switch (uMsg)
    {
	case WM_INITDIALOG:
		hb = LoadImage(g_qeglobals.d_hInstance, (LPCTSTR)IDB_FIND, IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE | LR_LOADTRANSPARENT | LR_LOADMAP3DCOLORS);
				
		for (i = 0; i < MAX_BSPCOMMANDS; i++)
		{
			g_szBspNames[i] = NULL;
			g_szBspCommands[i] = NULL;
		}

		GetCurrentDirectory(MAX_PATH, g_szCurrentDirectory);
		h = GetDlgItem(hwndDlg, IDC_COMBO_BSPNAME);
		SendMessage(h, CB_LIMITTEXT, (WPARAM)255, 0);
		SendDlgItemMessage(hwndDlg, IDC_EDIT_BSPCOMMAND, EM_LIMITTEXT, (WPARAM)511, 0);
		if (!g_bFirstProject)	// if project file exist
		{
			basepath		= ValueForKey(g_qeglobals.d_entityProject, "basepath");
			remotebasepath	= ValueForKey(g_qeglobals.d_entityProject, "remotebasepath");
			mapspath		= ValueForKey(g_qeglobals.d_entityProject, "mapspath");
			autosavemap		= ValueForKey(g_qeglobals.d_entityProject, "autosave");
			entitypath		= ValueForKey(g_qeglobals.d_entityProject, "entitypath");
			texturepath		= ValueForKey(g_qeglobals.d_entityProject, "texturepath");
			defaultwads		= ValueForKey(g_qeglobals.d_entityProject, "defaultwads");
			toolspath		= ValueForKey(g_qeglobals.d_entityProject, "rshcmd");

			SetDlgItemText(hwndDlg,	IDC_EDIT_PROJECTDIRECTORY,	basepath);
			SetDlgItemText(hwndDlg, IDC_EDIT_REMOTEBASEPATH,	remotebasepath);
			SetDlgItemText(hwndDlg, IDC_EDIT_MAPSDIRECTORY,		mapspath);
			SetDlgItemText(hwndDlg, IDC_EDIT_AUTOSAVEMAP,		autosavemap);
			SetDlgItemText(hwndDlg, IDC_EDIT_ENTITYFILES,		entitypath);				
			SetDlgItemText(hwndDlg, IDC_EDIT_TEXTUREDIRECTORY,	texturepath);
			SetDlgItemText(hwndDlg, IDC_EDIT_DEFAULTWADS,		defaultwads);
			SetDlgItemText(hwndDlg, IDC_EDIT_TOOLSDIRECTORY,	toolspath);
			
			SendDlgItemMessage(hwndDlg, IDC_BUTTON_PROJECTDIRECTORY, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hb);
			SendDlgItemMessage(hwndDlg, IDC_BUTTON_REMOTEBASEPATH,	 BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hb);
			SendDlgItemMessage(hwndDlg, IDC_BUTTON_MAPSDIRECTORY,	 BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hb);
			SendDlgItemMessage(hwndDlg, IDC_BUTTON_AUTOSAVEMAP,		 BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hb);
			SendDlgItemMessage(hwndDlg, IDC_BUTTON_ENTITYFILES,		 BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hb);
			SendDlgItemMessage(hwndDlg, IDC_BUTTON_TEXTUREDIRECTORY, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hb);
			SendDlgItemMessage(hwndDlg, IDC_BUTTON_DEFAULTWADS,		 BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hb);
			SendDlgItemMessage(hwndDlg, IDC_BUTTON_TOOLSDIRECTORY,	 BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hb);

			for (ep = g_qeglobals.d_entityProject->epairs; ep; ep = ep->next)
			{
				if (ep->key[0] == 'b' && ep->key[1] == 's' && ep->key[2] == 'p' && ep->key[3] == '_')
				{
					name = ep->key + 4;
					value = ep->value;
					SendMessage(h, CB_ADDSTRING, 0, (LPARAM)name);
					index = GetNextFreeBspIndex();
					prev = 0;
					for (i = i2 = 0; value[i]; i++)
					{
						switch(value[i])
						{
						case ' ':
							if (prev != ' ' && prev != '&')
								prev = buf[i2++] = ' ';
							break;
						case '\t':
							if (prev != ' ')
								prev = buf[i2++] = ' ';
							break;
						case '&':
							if (prev != '&' && prev != ' ')
								buf[i2++] = ' ';
							if (prev == '&')
							{
								buf[i2++] = 13;
								buf[i2++] = 10;
								while (value[i] != 0 && (value[i] == ' ' || value[i] == '\t'))
									i++;
							}
							prev = '&';
							break;
						default:
							prev = buf[i2++] = value[i];
							break;
						}
					}
					buf[i2] = 0;
					g_szBspNames[index] = strdup(name);
					g_szBspCommands[index] = strdup(buf);
				}
			}
			SendMessage(h, CB_SETCURSEL, 0, 0);
			SendMessage(hwndDlg, WM_COMMAND, (WPARAM)(CBN_SELCHANGE << 16) + IDC_COMBO_BSPNAME, (LPARAM)h);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			SetCurrentDirectory(g_szCurrentDirectory);
			if (!SendDlgItemMessage(hwndDlg, IDC_EDIT_PROJECTDIRECTORY, EM_LINELENGTH, 0, 0))
			{
				MessageBox(g_qeglobals.d_hwndMain, "No Project Directory Specified.", "QuakeEd 3: Error", MB_OK | MB_ICONEXCLAMATION);
				return FALSE;
			}
			else if (!SendDlgItemMessage(hwndDlg, IDC_EDIT_REMOTEBASEPATH, EM_LINELENGTH, 0, 0))
			{
				MessageBox(g_qeglobals.d_hwndMain, "No Remote Basepath Specified.", "QuakeEd 3: Error", MB_OK | MB_ICONEXCLAMATION);
				return FALSE;
			}
			else if (!SendDlgItemMessage(hwndDlg, IDC_EDIT_MAPSDIRECTORY, EM_LINELENGTH, 0, 0))
			{
				MessageBox(g_qeglobals.d_hwndMain, "No Maps Directory Specified.", "QuakeEd 3: Error", MB_OK | MB_ICONEXCLAMATION);
				return FALSE;
			}
			else if (!SendDlgItemMessage(hwndDlg, IDC_EDIT_AUTOSAVEMAP, EM_LINELENGTH, 0, 0))
			{
				MessageBox(g_qeglobals.d_hwndMain, "No Auto-Save Map Specified.", "QuakeEd 3: Error", MB_OK | MB_ICONEXCLAMATION);
				return FALSE;
			}
			else if (!SendDlgItemMessage(hwndDlg, IDC_EDIT_ENTITYFILES, EM_LINELENGTH, 0, 0))
			{
				MessageBox(g_qeglobals.d_hwndMain, "No Entity Definition Files Specified.", "QuakeEd 3: Error", MB_OK | MB_ICONEXCLAMATION);
				return FALSE;
			}
			else if (!SendDlgItemMessage(hwndDlg, IDC_EDIT_TEXTUREDIRECTORY, EM_LINELENGTH, 0, 0))
			{
				MessageBox(g_qeglobals.d_hwndMain, "No Texture Directory Specified.", "QuakeEd 3: Error", MB_OK | MB_ICONEXCLAMATION);
				return FALSE;
			}
			else if (!SendDlgItemMessage(hwndDlg, IDC_EDIT_DEFAULTWADS, EM_LINELENGTH, 0, 0))
			{
				MessageBox(g_qeglobals.d_hwndMain, "No Default Wad Specified.", "QuakeEd 3: Error", MB_OK | MB_ICONEXCLAMATION);
				return FALSE;
			}
			else if (!SendDlgItemMessage(hwndDlg, IDC_EDIT_TOOLSDIRECTORY, EM_LINELENGTH, 0, 0))
			{
				MessageBox(g_qeglobals.d_hwndMain, "No Tools Directory Specified.", "QuakeEd 3: Error", MB_OK | MB_ICONEXCLAMATION);
				return FALSE;
			}
			else
			{
				SaveSettings(hwndDlg);
				EndDialog(hwndDlg, 1);
			}
			return TRUE;											
			
		case IDCANCEL:
			for (i = 0; i < MAX_BSPCOMMANDS; i++)
			{
				if (g_szBspCommands[i])
					free(g_szBspCommands[i]);
				if (g_szBspNames[i])
					free(g_szBspNames[i]);
			}
			if (!g_bFirstProject)
				EndDialog(hwndDlg, 0);
			else
				if (MessageBox(g_qeglobals.d_hwndMain, "QuakeEd cannot run without a default project.\nPress OK to exit, or CANCEL to continue editing the project.", "QuakeEd 3: Info", MB_OKCANCEL | MB_ICONINFORMATION) == IDOK)
					exit(0);
			return TRUE;
									
		case IDC_BUTTON_PROJECTDIRECTORY:
			GetProjectDirectory(hwndDlg);
			break;
		case IDC_BUTTON_REMOTEBASEPATH:
			GetRemoteBasePath(hwndDlg);
			break;
		case IDC_BUTTON_MAPSDIRECTORY:
			GetMapsDirectory(hwndDlg);
			break;
		case IDC_BUTTON_AUTOSAVEMAP:
			GetAutosaveMap(hwndDlg);
			break;
		case IDC_BUTTON_ENTITYFILES:
			GetEntityFiles(hwndDlg);
			break;
		case IDC_BUTTON_TEXTUREDIRECTORY:
			GetTextureDirectory(hwndDlg);
			break;
		case IDC_BUTTON_DEFAULTWADS:
			GetDefaultWads(hwndDlg);
			break;
		case IDC_BUTTON_TOOLSDIRECTORY:
			GetToolsDirectory(hwndDlg);
			break;
		case IDC_BUTTON_BSPNEW:
			NewBspCommand(hwndDlg);
			break;
		case IDC_BUTTON_BSPACCEPT:
			AcceptBspCommand(hwndDlg);
			break;
		case IDC_BUTTON_BSPDELETE:
			DeleteBspCommand(hwndDlg);
			break;

		case IDC_COMBO_BSPNAME:
			switch (HIWORD(wParam))
			{
			case CBN_SELCHANGE:
				{
					char buf[256];
					int index = SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);

					SendMessage((HWND)lParam, CB_GETLBTEXT, (WPARAM)index, (LPARAM)buf);
					index = GetBspIndex(buf);
					if (index == -1)
						return FALSE;
					SendDlgItemMessage(hwndDlg, IDC_EDIT_BSPCOMMAND, WM_SETTEXT, 0, (LPARAM)g_szBspCommands[index]);
				}
				break;
			}
			break;
		}
		return 0;
	
	default:
		return FALSE;
		break;
	}
}

/*
==================
DoProject
==================
*/
void DoProject (bool bFirst)
{
	g_bFirstProject = bFirst;
	DialogBox(g_qeglobals.d_hInstance, (char *)IDD_PROJECT, g_qeglobals.d_hwndMain, ProjectSettingsDlgProc);
}
