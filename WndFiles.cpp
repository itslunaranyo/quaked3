/*
======================================================================

FILE DIALOGS

======================================================================
*/

#include "pre.h"
#include "qe3.h"
#include "WndFiles.h"

#include "map.h"
#include "Command.h"


/*
==================
ConfirmModified
==================
*/
bool ConfirmModified()
{
	if (!g_cmdQueue.IsModified())
		return true;

	switch (MessageBox(g_hwndMain, 
		_S("Save Changes to %s?") << (g_map.name.empty() ? "untitled" : g_map.name), 
		"QuakeEd 3",
		MB_YESNOCANCEL | MB_ICONEXCLAMATION))
	{
	case IDYES:
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

// TODO: fix this not going to the same folder no matter what
OPENFILENAME CommonOFN(DWORD flags)
{
	OPENFILENAME ofn;			// common dialog box structure
	memset(&ofn, 0, sizeof(OPENFILENAME));

	strcpy(szDirName, g_project.mapPath.data());
	if (strlen(szDirName) == 0)
	{
		strcpy(szDirName, g_project.basePath.data());
		strcat(szDirName, "/maps");
	}
	szFile[0] = '\0';

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = g_hwndMain;
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

	// Add the file to MRU
	WndMain_AddMRUItem(ofn.lpstrFile);

	// Open the file
	g_map.Load(ofn.lpstrFile);
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
bool SaveAsDialog()
{
	OPENFILENAME ofn = CommonOFN(OFN_SHOWHELP | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_OVERWRITEPROMPT);

	// Display the Open dialog box. 
	if (!GetSaveFileName(&ofn))
		return false;

	DefaultExtension(ofn.lpstrFile, ".map");
	g_map.name = ofn.lpstrFile;
	g_map.hasFilename = true;

	// Add the file to MRU
	WndMain_AddMRUItem(ofn.lpstrFile);

	return true;
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

	g_map.SaveSelection(ofn.lpstrFile);	// ignore region
}
