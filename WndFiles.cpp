/*
======================================================================

FILE DIALOGS

======================================================================
*/

#include "pre.h"
#include "qe3.h"
#include "strlib.h"
#include "WndFiles.h"

#include "map.h"
#include "Command.h"

/*
==================
the secret rules for making windows open the file dialog where you want:

- lpstrInitialDir is used by windows to track when the user navigated away from
	that dir in a previous file dialog, so the dialog initializes there first
- lpstrFile receives the filename chosen in the dialog but can start with a
	filename already in it, so the dialog initializes there next
- lspstrInitialDir itself is used if the first two yield nothing
- ending a dir path with a trailing slash is bad and counts as no input!
- using unix style slashes is bad and counts as no input!
==================
*/

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


static const char szMapFilter[32] = "QuakeEd Map (*.map)\0*.map\0\0";	// filter string for map files

bool Dlg_FileCommon(std::string& sel, const char* _dir, const char* _filter, const char* _title, DWORD flags, bool save)
{
	OPENFILENAME ofn;			// common dialog box structure
	memset(&ofn, 0, sizeof(OPENFILENAME));
	sel.reserve(MAX_PATH);

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = g_hwndMain;
	ofn.lpstrFilter = _filter;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = sel.data();
	ofn.nMaxFile = sel.capacity();
	ofn.lpstrTitle = (LPSTR)_title;
	ofn.lpstrInitialDir = _dir;
	ofn.Flags = flags;

	if (save)
	{
		if (!GetSaveFileName(&ofn))
			return false;	// canceled
	}
	else
	{
		if (!GetOpenFileName(&ofn))
			return false;	// canceled
	}
	sel = ofn.lpstrFile;
	return true;
}

bool Dlg_FileOpen(std::string& sel, const char* _dir, const char* _filter, const char* _title, DWORD flags)
{
	return Dlg_FileCommon(sel, _dir, _filter, _title, flags, false);
}
bool Dlg_FileSave(std::string& sel, const char* _dir, const char* _filter, const char* _title, DWORD flags)
{
	return Dlg_FileCommon(sel, _dir, _filter, _title, flags, true);
}

/*
==================
Dlg_MapOpen
==================
*/
void Dlg_MapOpen()
{
	std::string mapfile, mappath;

	if (g_map.hasFilename)
		mapfile = g_map.name;

	// windows requires this path to not end with a trailing slash
	mappath = g_project.mapPath.substr(0, g_project.mapPath.length() - 1);

	if (!Dlg_FileOpen(mapfile,
		mappath.data(),
		szMapFilter,
		"Open Map",
		OFN_SHOWHELP | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST))
		return;

	// Add the file to MRU
	WndMain_AddMRUItem(mapfile.data());

	// Open the file
	g_map.Load(mapfile);
}

/*
==================
Dlg_MapImport
==================
*/
void Dlg_MapImport()
{
	std::string mapfile, mappath;

	if (g_map.hasFilename)
		mapfile = g_map.name;

	// windows requires this path to not end with a trailing slash
	mappath = g_project.mapPath.substr(0, g_project.mapPath.length() - 1);

	if (!Dlg_FileOpen(mapfile,
		mappath.data(),
		szMapFilter,
		"Import Map",
		OFN_SHOWHELP | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST))
		return;

	// Open the file.
	g_map.ImportFromFile(mapfile);
}

/*
==================
Dlg_MapSaveAs
==================
*/
bool Dlg_MapSaveAs()
{
	std::string mapfile, mappath;

	if (g_map.hasFilename)
		mapfile = g_map.name;

	// windows requires this path to not end with a trailing slash
	mappath = g_project.mapPath.substr(0, g_project.mapPath.length() - 1);

	if (!Dlg_FileSave(mapfile,
		mappath.data(),
		szMapFilter,
		"Save Map As",
		OFN_SHOWHELP | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_OVERWRITEPROMPT))
		return false;

	DefaultExtension(mapfile, ".map");
	g_map.name = mapfile;
	g_map.hasFilename = true;

	// Add the file to MRU
	WndMain_AddMRUItem(mapfile.data());

	return true;
}

/*
==================
Dlg_MapExport
==================
*/
void Dlg_MapExport()
{
	std::string mapfile, mappath;

	if (g_map.hasFilename)
		mapfile = g_map.name;

	// windows requires this path to not end with a trailing slash
	mappath = g_project.mapPath.substr(0, g_project.mapPath.length() - 1);

	if (!Dlg_FileSave(mapfile,
		mappath.data(),
		szMapFilter,
		"Export Map",
		OFN_SHOWHELP | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_OVERWRITEPROMPT))
		return;

	DefaultExtension(mapfile, ".map");

	g_map.SaveSelection(mapfile);	// ignore region
}
