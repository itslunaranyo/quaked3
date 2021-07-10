//==============================
//	map.cpp
//============================== 

#include "pre.h"
#include "qe3.h"
#include "map.h"
#include "select.h"
#include "modify.h"
#include "points.h"
#include "winding.h"
#include "parse.h"

#include "CameraView.h"
#include "GridView.h"
#include "CmdImportMap.h"
#include "CmdPaste.h"
#include "win_dlg.h"

#include <fstream>
#include <sstream>
#include <string>

Map		g_map;
Brush	*g_pbrRegionSides[4];

Map::Map() : numBrushes(0), numEntities(0), numTextures(0), world(nullptr)
{
	ClearBounds(regionMins, regionMaxs);
}

/*
===========
Map::New
===========
*/
void Map::New()
{
	qeBuffer between(0);

	Log::Print("Map::New\n");

	SaveBetween(between);
	Free();

	world = new Entity();
	world->SetKeyValue("classname", "worldspawn");
	world->SetKeyValue("wad", g_project.defaultWads);
	world->eclass = EntClass::ForName("worldspawn", true, true);

	g_vCamera.Reset();
	g_vGrid[0].Center(vec3(0));

	if (LoadBetween(between))
		BuildBrushData(g_brSelectedBrushes);	// in case something was betweened

	WndMain_SetInspectorMode(W_TEXTURE);
	WndMain_UpdateWindows(W_ALL);
}

/*
================
Map::Free

trashes all map data, but does not restore a workable blank state (use New() for that)
================
*/
void Map::Free()
{
	Pointfile_Clear();
	targetGraph.Clear();

	name[0] = 0;
	hasFilename = false;
	//Sys_SetTitle(name);
	//g_qeglobals.d_nNumEntities = 0;
	g_map.numBrushes = 0;
	g_map.numEntities = 0;

	while (brActive.Next() != &brActive)
		delete brActive.Next();

	while (brRegioned.Next() != &brRegioned)
		delete brRegioned.Next();

	while (entities.Next() != &entities)
		delete entities.Next();

	while (g_brSelectedBrushes.Next() != &g_brSelectedBrushes)
		delete g_brSelectedBrushes.Next();

	if (world)
		delete world;
	world = nullptr;

	Textures::FlushAll();
	g_qeglobals.d_workTexDef = TexDef();

	g_cmdQueue.Clear();
	autosaveTime = 0;

	// winding clear must come last, as it resets the winding allocator, so that all
	// brush geometry that might point into the winding pages is already destroyed
	Winding::Clear();

	WndMain_UpdateWindows(W_ALL);
}

/*
==================
Map::BuildBrushData
==================
*/
void Map::BuildBrushData(Brush &blist)
{
	Brush	*b, *next;

	if (!blist.IsLinked())
		return;

	for (b = blist.Next(); b != NULL && b != &blist; b = next)
	{
		next = b->Next();

		// buildbrushdata is called after a wad is loaded or refreshed to make sure names match texture pointers
		// wads are also not loaded until after worldspawn is parsed during a map load
		if (!b->FullBuild() || !b->faces)
		{
			Log::Warning(_S("Removed degenerate brush with mins (%v) maxs (%v).") << b->mins << b->maxs);
			delete b;
		}
	}
}

/*
==================
Map::BuildBrushData
==================
*/
void Map::BuildBrushData()
{
	double time;

	Log::Print("Map::BuildBrushData\n");

	Sys_BeginWait();	// this could take a while
	time = Sys_DoubleTime();

	BuildBrushData(brActive);
	BuildBrushData(g_brSelectedBrushes);
	BuildBrushData(brRegioned);
	WndMain_UpdateBrushStatusBar();

	time = Sys_DoubleTime() - time;
	if (time)
		Log::Print(_S("Brush data built in %f seconds\n")<< time);
	Sys_EndWait();
}

//================================================================

/*
================
Map::ParseBufferMerge

parse all entities and brushes from the text buffer, assuming the scene is not empty

only called by loadbetween, now
================
*/
bool Map::ParseBufferMerge(const char *data)
{
	Entity elist;
	Brush blist;

	try
	{
		Read(data, blist, elist);
		for (Entity *ent = elist.Next(); ent != &elist; ent = ent->Next())
		{
			// world brushes need to be merged into the existing worldspawn
			if (ent->eclass == EntClass::worldspawn)
			{
				ent->RemoveFromList();
				Brush *b, *next;
				for (b = ent->brushes.ENext(); b != &ent->brushes; b = next)
				{
					next = b->ENext();
					Entity::UnlinkBrush(b);
					world->LinkBrush(b);
				}
				delete ent;
				break;
			}
		}
	}
	catch (qe3_exception)
	{
		//ReportError(ex);
		// blist and elist are auto-freed by stack unwinding without a merger,
		// so the map isn't polluted with a partial import when we exit
		return false;
	}

	Selection::DeselectAll();
	elist.MergeListIntoList(&entities);
	blist.MergeListIntoList(g_brSelectedBrushes);	// merge to selection
	SanityCheck();
	return true;
}

/*
================
Map::LoadFromFile_Parse

parse all entities and brushes from the text buffer, assuming the scene is empty

only for File > Load
================
*/
bool Map::LoadFromFile_Parse(const char *data)
{
	Entity elist;
	Brush blist;

	try
	{
		Read(data, blist, elist);
		for (Entity *ent = elist.Next(); ent != &elist; ent = ent->Next())
		{
			if (ent->eclass == EntClass::worldspawn)
			{
				ent->RemoveFromList();
				world = ent;
				break;
			}
		}
	}
	catch (qe3_exception)
	{
		//ReportError(ex);
		// don't need to free here, loaded map wasn't merged into the now-empty scene yet
		return false;
	}

	// now merge the fully loaded data into the scene
	elist.MergeListIntoList(&entities);
	blist.MergeListIntoList(brActive);
	SanityCheck();
	return true;
}


void Map::SanityCheck()
{
#ifdef _DEBUG
	Entity *e;
	Brush *b;
	for (e = entities.Next(); e != &entities; e = e->Next())
	{
		assert(e->Next()->Prev() == e);
		assert(e->brushes.ENext()->EPrev() == &e->brushes);
		assert(e->brushes.EPrev()->ENext() == &e->brushes);

		for (b = e->brushes.ENext(); b != &e->brushes; b = b->ENext())
		{
			assert(b->ENext()->EPrev() == b);
		}
	}

#endif
}


/*
================
Map::LoadFromFile

replace all current map data with the contents of a file

load map
load wads
build brush data
flush wads
================
*/
void Map::LoadFromFile(const char *filename)
{
	char	temp[1024];
	Entity	*ent;
	bool	bSnapCheck = false;
	qeBuffer between(0);

	Sys_BeginWait();

	// sikk---> make sure Grid Snap is off to insure complex brushes remain intact
	if (g_qeglobals.bGridSnap)
	{
		g_qeglobals.bGridSnap = false;
		bSnapCheck = true;
	}
	// <---sikk

	WndMain_SetInspectorMode(W_CONSOLE);

	Sys_ConvertDOSToUnixName(temp, filename);
	Log::Print(_S("Map::LoadFromFile: %s\n") << temp);

	SaveBetween(between);
	Free();

	qeBuffer buf;
	if (IO_LoadFile(filename, buf) < 1)
		Error(_S("Couldn't load %s!") << filename);

	if (LoadFromFile_Parse((char*)*buf))
	{
		strcpy(name, filename);
		hasFilename = true;

		if (!world)
		{
			Log::Warning("No worldspawn in map! Creating new empty worldspawn ...");

			world = new Entity();
			world->SetKeyValue("classname", "worldspawn");
		}

		if (!*world->GetKeyValue("wad"))
			Log::Warning("No \"wad\" key.");
		else
			Textures::LoadWadsFromWadstring(world->GetKeyValue("wad"));

		for (Brush* b = brActive.Next(); b != &brActive; b = b->Next())
			numBrushes++;
		for (Entity* e = entities.Next(); e != &entities; e = e->Next())
		{
			if (e->IsPoint())
				numBrushes--;
			numEntities++;
		}

		Log::Print("--- LoadMapFile ---\n");
		Log::Print(_S("%s\n")<< temp);
		Log::Print(_S("%5i brushes\n%5i entities\n")<< numBrushes<< numEntities);

		LoadBetween(between);
		g_map.BuildBrushData();

		// move the view to a start position
		ent = g_map.FindEntity("classname", "info_player_start");
		if (!ent)
			ent = g_map.FindEntity("classname", "info_player_deathmatch");

		g_vCamera.Reset();
		g_vGrid[0].Center(vec3(0));

		if (ent)
		{
			vec3 startOrg;
			ent->GetKeyValueVector("origin", startOrg);
			ent->GetKeyValueVector("origin", startOrg);
			g_vCamera.SetOrigin(startOrg);
			g_vGrid[0].Center(startOrg);
			g_vCamera.Turn(ent->GetKeyValueFloat("angle"), true);
		}

		Textures::FlushUnused();	// should be FlushUnusedFromWadstring technically but those are the only wads loaded yet

		WndMain_UpdateTitle();
		RegionOff();
	}

	WndMain_UpdateBrushStatusBar();
	WndMain_SetInspectorMode(W_TEXTURE);
	WndMain_UpdateWindows(W_ALL);

	if (bSnapCheck)	// sikk - turn Grid Snap back on if it was on before map load
		g_qeglobals.bGridSnap = true;

	Sys_EndWait();
}

/*
================
Map::ImportFromFile

merge the contents of a file into the current map data

load map
reload current wads + load wads added to wadstring
build brush data
flush only wads added to wadstring
================
*/
void Map::ImportFromFile(const char *filename)
{
	char	temp[1024];
	bool	bSnapCheck = false;

	Sys_BeginWait();

	WndMain_SetInspectorMode(W_CONSOLE);

	Sys_ConvertDOSToUnixName(temp, filename);
	Log::Print(_S("Map::ImportFromFile: %s\n")<< temp);

	CmdImportMap* cmdIM = new CmdImportMap();

	if (ImportOptionsDialog(cmdIM))
	{
		cmdIM->File(filename);
		g_cmdQueue.Complete(cmdIM);

		if (!*world->GetKeyValue("wad"))
			Log::Warning("No \"wad\" key.");
		else
			Textures::LoadWadsFromWadstring(world->GetKeyValue("wad"));
		Textures::RefreshUsedStatus();

		g_map.BuildBrushData();

		WndMain_UpdateWindows(W_ALL);
	}
	else
	{
		Log::Print("Import canceled.\n");
		delete cmdIM;
	}

	Sys_EndWait();
}

/*
================
Map::SaveToFile

write entire contents of the scene to a file
================
*/
void Map::SaveToFile(const char *filename, bool use_region)
{
	std::ofstream	   *f;
	char        temp[1024];

	Sys_ConvertDOSToUnixName(temp, filename);

	if (!use_region)
	{
		char backup[MAX_PATH];

		// rename current to .bak
		strcpy(backup, filename);
		StripExtension(backup);
		strcat(backup, ".bak");
		_unlink(backup);
		rename(filename, backup);
	}

	Log::Print(_S("Map::SaveToFile: %s\n") << filename);

	f = new std::ofstream(filename);
	if (!f)
	{
		Log::Print(_S("ERROR: Could not open %s\n") << filename);
		return;
	}

	if (use_region)
		RegionAdd();

	WriteAll(*f, use_region);

	f->close();

	if (use_region)
		RegionRemove();

	Log::Print("Saved.\n");
	WndMain_Status("Saved.", 0);
}

/*
================
Map::ExportToFile

write selected brushes and entities to a file
================
*/
void Map::ExportToFile(const char *filename)
{
	std::ofstream	   *f;

	Log::Print(_S("Map::ExportToFile: %s\n")<< filename);

	f = new std::ofstream(filename);
	if (!f)
	{
		Log::Print(_S("ERROR: Could not open %s\n")<< filename);
		return;
	}
	WriteSelected(*f);
	f->close();

	Log::Print(_S("Selection exported.\n")<< filename);
}

/*
================
Map::Cut

write selected brushes and entities to the windows clipboard and delete them
================
*/
void Map::Cut()
{
	Copy();
	Modify::Delete();
}

/*
================
Map::Copy

write selected brushes and entities to the windows clipboard
================
*/
void Map::Copy()
{
	HGLOBAL hglbCopy;
	int copylen;

	if (!Selection::HasBrushes())
		return;
	if (!OpenClipboard(g_hwndMain))
		return;

	std::stringstream sstr;
	WriteSelected(sstr);

	copylen = (int)sstr.tellp();
	hglbCopy = GlobalAlloc(GMEM_MOVEABLE, copylen + 1);
	if (hglbCopy)
	{
		char* cpbuf;
		EmptyClipboard();
		cpbuf = (char*)GlobalLock(hglbCopy);
		// copy stringstream buffer straight to clipboard mem so we don't copy twice through an intermediate std::string
		sstr.read(cpbuf, copylen);
		cpbuf[copylen] = 0;
		GlobalUnlock(hglbCopy);
		SetClipboardData(CF_TEXT, hglbCopy);
	}
	CloseClipboard();
}

/*
================
Map::Paste

merge the contents of the windows clipboard into the current map data
================
*/
void Map::Paste()
{
	Sys_BeginWait();

	CmdPaste *cmdP = new CmdPaste();
	g_cmdQueue.Complete(cmdP);

	Sys_EndWait();
}

//================================================================

/*
================
Map::Read

parse the map data and link all brushes and entities to the provided lists
================
*/
void Map::Read(const char *data, Brush &blist, Entity &elist)
{
	int numEntities;
	Entity* ent;
	Brush* next;
	StartTokenParsing(data);
	bool foundWorld = false;

	numEntities = 0;

	while (1)
	{
		ent = Entity::Parse(false);
		if (!ent)
			break;

		if (!strcmp(ent->GetKeyValue("classname"), "worldspawn"))
		{
			if (foundWorld)
				Log::Warning("Multiple worldspawn.");
			foundWorld = true;

			// add the worldspawn to the beginning of the entity list so it's easy to find
			ent->AddToList(&elist, false);
		}
		else
		{
			// add the entity to the end of the entity list
			ent->AddToList(&elist, true);
			numEntities++;
		}

		// add all the brushes to the brush list
		for (Brush* b = ent->brushes.ENext(); b != &ent->brushes; b = next)
		{
			next = b->ENext();
			b->AddToList(blist);
		}
	}
}

/*
================
Map::WriteSelected

map-print only selected brushes and entities to the buffer
================
*/
void Map::WriteSelected(std::ostream &out)
{
	int count = 0;
	Entity *e, *next;

	// write world entity first
	world->WriteSelected(out, count++);

	// then write all other ents
	for (e = entities.Next(); e != &entities; e = next)
	{
		next = e->Next();
		/*
		if (e->brushes.onext == &e->brushes)
		{
			assert(0);
			delete e;	// no brushes left, so remove it
		}
		else
			e->WriteSelected(out);
		*/
		if (e->brushes.ENext() != &e->brushes)
			e->WriteSelected(out, count++);
	}
}

/*
================
Map::WriteAll

map-print all brushes and entities to the buffer
================
*/
void Map::WriteAll(std::ostream &out, bool use_region)
{
	int count;
	Entity *e, *next;

	// write world entity first
	world->Write(out, use_region);

	// then write all other ents
	count = 1;
	for (e = entities.Next(); e != &entities; e = next)
	{
		out << "// entity " << count << "\n";
		count++;
		next = e->Next();

		if (e->brushes.ENext() != &e->brushes)
			e->Write(out, use_region);
	}
}

/*
==================
Map::SaveBetween
==================
*/
void Map::SaveBetween(qeBuffer &buf)
{
	int copylen;

	if (!Selection::HasBrushes())
		return;
	if (MessageBox(g_hwndMain, "Copy selection to new map?", "QuakeEd 3", MB_YESNO | MB_ICONQUESTION) == IDNO)
		return;

	std::stringstream sstr;
	WriteSelected(sstr);

//	sstr.seekp(sstr.end);
	copylen = (int)sstr.tellp();
	buf.resize(copylen + 1);

	char* cpbuf = (char*)*buf;
	sstr.read(cpbuf, copylen);
	cpbuf[copylen] = 0;
}

/*
==================
Map::LoadBetween
==================
*/
bool Map::LoadBetween(qeBuffer &buf)
{
	if (!buf.size())
		return false;		// nothing saved in it

	if (ParseBufferMerge((char*)*buf))
	{
		//BuildBrushData(g_brSelectedBrushes);
		Selection::Changed();
		//modified = true;
	}
	return true;
}

//================================================================

void Map::RegionOff()
{
	Brush	*b, *next;

	regionActive = false;

	for (int i = 0; i < 3; i++)
	{
		regionMaxs[i] = g_cfgEditor.MapSize / 2;
		regionMins[i] = -g_cfgEditor.MapSize / 2;
	}

	for (b = brRegioned.Next(); b != &brRegioned; b = next)
	{
		next = b->Next();
		if (IsBrushFiltered(b))
			continue;		// still filtered
		b->RemoveFromList();
		b->AddToList(brActive);
	}

	WndMain_UpdateWindows(W_SCENE | W_TARGETGRAPH);
}

void Map::RegionXYZ(int gwin)
{
	RegionOff();

	regionMins = g_vGrid[gwin].GetMins();
	regionMaxs = g_vGrid[gwin].GetMaxs();

	regionMins[g_vGrid[gwin].GetAxis()] = -g_cfgEditor.MapSize / 2;
	regionMaxs[g_vGrid[gwin].GetAxis()] = g_cfgEditor.MapSize / 2;

	RegionApply();
}
void Map::RegionXY() { RegionXYZ(0); }
void Map::RegionXZ() { RegionXYZ(2); }
void Map::RegionYZ() { RegionXYZ(1); }

void Map::RegionTallBrush()
{
	Brush	*b;

	if (!QE_SingleBrush())
		return;

	b = g_brSelectedBrushes.Next();

	RegionOff();

	regionMins = b->mins;
	regionMaxs = b->maxs;
	regionMins[2] = -g_cfgEditor.MapSize / 2;
	regionMaxs[2] = g_cfgEditor.MapSize / 2;

	Modify::Delete();
	RegionApply();
}

void Map::RegionBrush()
{
	Brush	*b;

	if (!QE_SingleBrush())
		return;

	b = g_brSelectedBrushes.Next();

	RegionOff();

	regionMins = b->mins;
	regionMaxs = b->maxs;

	Modify::Delete();
	RegionApply();
}

void Map::RegionSelectedBrushes()
{
	RegionOff();

	if (!Selection::HasBrushes())
		return;

	regionActive = true;
	Selection::GetBounds(regionMins, regionMaxs);

	// move the entire active_brushes list to filtered_brushes
	brActive.MergeListIntoList(brRegioned);

	// move the entire g_brSelectedBrushes list to brActive
	g_brSelectedBrushes.MergeListIntoList(brActive);
	Selection::Changed();
	WndMain_UpdateWindows(W_SCENE | W_TARGETGRAPH);
}

void Map::RegionApply()
{
	Brush	*b, *next;

	regionActive = true;
	for (b = brActive.Next(); b != &brActive; b = next)
	{
		next = b->Next();
		if (!IsBrushFiltered(b))
			continue;		// still filtered
		b->RemoveFromList();
		b->AddToList(brRegioned);
	}

	WndMain_UpdateWindows(W_SCENE | W_TARGETGRAPH);
}

void Map::RegionAdd()
{
	vec3		mins, maxs;
	int			i;
	TexDef	texdef;

	if (!regionActive)
		return;

	memset(&texdef, 0, sizeof(texdef));
	strcpy(texdef.name, "REGION");

	mins[0] = regionMins[0] - 16;
	maxs[0] = regionMins[0] + 1;
	mins[1] = regionMins[1] - 16;
	maxs[1] = regionMaxs[1] + 16;
	mins[2] = -2048;
	maxs[2] = 2048;
	g_pbrRegionSides[0] = Brush::Create(mins, maxs, &texdef);

	mins[0] = regionMaxs[0] - 1;
	maxs[0] = regionMaxs[0] + 16;
	g_pbrRegionSides[1] = Brush::Create(mins, maxs, &texdef);

	mins[0] = regionMins[0] - 16;
	maxs[0] = regionMaxs[0] + 16;
	mins[1] = regionMins[1] - 16;
	maxs[1] = regionMins[1] + 1;
	g_pbrRegionSides[2] = Brush::Create(mins, maxs, &texdef);

	mins[1] = regionMaxs[1] - 1;
	maxs[1] = regionMaxs[1] + 16;
	g_pbrRegionSides[3] = Brush::Create(mins, maxs, &texdef);

	for (i = 0; i < 4; i++)
	{
		g_pbrRegionSides[i]->AddToList(g_brSelectedBrushes);
		world->LinkBrush(g_pbrRegionSides[i]);
		g_pbrRegionSides[i]->Build();
	}
}

void Map::RegionRemove()
{
	if (!regionActive)
		return;
	for (int i = 0; i < 4; i++)
		delete g_pbrRegionSides[i];
}

bool Map::IsBrushFiltered(Brush *b)
{
	if (regionActive == false)
		return false;

	for (int i = 0; i < 3; i++)
	{
		if (b->mins[i] > regionMaxs[i])
			return true;
		if (b->maxs[i] < regionMins[i])
			return true;
	}
	return false;
}

//================================================================


/*
==============
Map::FindEntity
==============
*/
Entity *Map::FindEntity(char *pszKey, char *pszValue)
{
	Entity *pe;

	pe = entities.Next();

	for (; pe != nullptr && pe != &entities; pe = pe->Next())
		if (!strcmp(pe->GetKeyValue(pszKey), pszValue))
			return pe;

	return nullptr;
}

Entity *Map::FindEntity(char *pszKey, int iValue)
{
	Entity *pe;

	pe = entities.Next();

	for (; pe != nullptr && pe != &entities; pe = pe->Next())
		if (pe->GetKeyValueInt(pszKey) == iValue)
			return pe;

	return nullptr;
}
