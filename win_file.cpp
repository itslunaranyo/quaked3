/*
======================================================================

FILE DIALOGS

======================================================================
*/

#include "qe3.h"


/*
==================
ConfirmModified
==================
*/
bool ConfirmModified()
{
	char szMessage[128];

	if (!g_cmdQueue.IsModified())
		return true;

	sprintf(szMessage, "Save Changes to %s?", g_map.name[0] ? g_map.name : "untitled");
	switch (MessageBox(g_qeglobals.d_hwndMain, szMessage, "QuakeEd 3: Save Changes?", MB_YESNOCANCEL | MB_ICONEXCLAMATION))
	{
	case IDYES:
		if (!g_map.hasFilename)
			SaveAsDialog();
		else
			QE_SaveMap();
		return true;
	case IDNO:
		return true;
	case IDCANCEL:
		return false;
	}
	return true;
}


static char	szDirName[_MAX_PATH];   // directory string
static char szFile[_MAX_PATH];		// filename string
static char szFileTitle[_MAX_FNAME];// file title string
static char szMapFilter[260] = "QuakeEd Map (*.map)\0*.map\0\0";	// filter string for map files

OPENFILENAME CommonOFN(DWORD flags)
{
	OPENFILENAME ofn;			// common dialog box structure
	memset(&ofn, 0, sizeof(OPENFILENAME));

	strcpy(szDirName, g_project.mapPath);
	if (strlen(szDirName) == 0)
	{
		strcpy(szDirName, g_project.basePath);
		strcat(szDirName, "/maps");
	}
	szFile[0] = '\0';

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = g_qeglobals.d_hwndMain;
	ofn.lpstrFilter = szMapFilter;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFileTitle = szFileTitle;
	ofn.nMaxFileTitle = sizeof(szFileTitle);
	ofn.lpstrInitialDir = szDirName;
	ofn.Flags = flags;

	return ofn;
}

void FileBrowserReadMap();
void FileBrowserWriteMap();

/*
==================
OpenDialog
==================
*/
void OpenDialog()
{
	OPENFILENAME ofn = CommonOFN( OFN_SHOWHELP | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST );

	// Display the Open dialog box.
	if (!GetOpenFileName(&ofn))
		return;	// canceled

	// Add the file in MRU
	AddNewItem(g_qeglobals.d_lpMruMenu, ofn.lpstrFile);

	// Refresh the File menu
	PlaceMenuMRUItem(g_qeglobals.d_lpMruMenu, GetSubMenu(GetMenu(g_qeglobals.d_hwndMain), 0), ID_FILE_EXIT);

	// Open the file
	g_map.LoadFromFile(ofn.lpstrFile);
}

/*
==================
ImportDialog
==================
*/
void ImportDialog()
{
	OPENFILENAME ofn = CommonOFN(OFN_SHOWHELP | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST);

	// Display the Open dialog box.
	if (!GetOpenFileName(&ofn))
		return;	// canceled

	// Open the file.
	g_map.ImportFromFile(ofn.lpstrFile);
}

/*
==================
SaveAsDialog
==================
*/
void SaveAsDialog()
{
	OPENFILENAME ofn = CommonOFN(OFN_SHOWHELP | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_OVERWRITEPROMPT);

	// Display the Open dialog box. 
	if (!GetSaveFileName(&ofn))
		return;	// canceled

	DefaultExtension(ofn.lpstrFile, ".map");
	strcpy(g_map.name, ofn.lpstrFile);
	g_map.hasFilename = true;

	// Add the file in MRU.
	AddNewItem(g_qeglobals.d_lpMruMenu, ofn.lpstrFile);

	// Refresh the File menu.
	PlaceMenuMRUItem(g_qeglobals.d_lpMruMenu, GetSubMenu(GetMenu(g_qeglobals.d_hwndMain), 0), ID_FILE_EXIT);

	QE_SaveMap();
}




/*
==================
ExportDialog
==================
*/
void ExportDialog()
{
	OPENFILENAME ofn = CommonOFN(OFN_SHOWHELP | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_OVERWRITEPROMPT);

	// Display the Open dialog box. 
	if (!GetSaveFileName(&ofn))
		return;	// canceled

	DefaultExtension(ofn.lpstrFile, ".map");

	g_map.ExportToFile(ofn.lpstrFile);	// ignore region
}
