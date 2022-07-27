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

#include "TextFileReader.h"
#include "MapParser.h"
#include "MapWriter.h"
#include "IO.h"

#include "CameraView.h"
#include "GridView.h"
#include "TextureView.h"
#include "CmdImportMap.h"
#include "CmdPaste.h"
#include "win_dlg.h"

#include <fstream>
#include <sstream>

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
	std::string between;

	Log::Print("Map::New\n");

	SaveBetween(between);
	Free();

	world = CreateWorldspawn();
	std::string wads = world->GetKeyValue("wad");
	if (!wads.empty())
		Textures::LoadWadsFromWadstring(wads);

	g_vCamera.Reset();
	g_vGrid[0].Center(vec3(0));
	g_vTexture.ChooseFirstTexture();

	if (LoadBetween(between))
		BuildBrushData(g_brSelectedBrushes);

	WndMain_SetInspectorMode(W_TEXTURE);
	WndMain_UpdateWindows(W_ALL);
}

/*
================
Map::Save
================
*/
void Map::Save()
{
	assert(hasFilename);
	Save(name);
}

void Map::Save(const std::string& filename)
{
//	if (!use_region)
	{
		if (IO::FileExists(filename))
			IO::RenameFile(filename, IO::ChangeExtension(filename, "bak"));
	}

	std::ofstream fOut(filename, std::ios::out | std::ios::trunc);
	if (fOut.fail())
		Error(_S("Couldn't write to %s!\n") << filename);

	//if (use_region)
	//	RegionAdd();

//	name = filename;
	MapWriter mOut;
	mOut.WriteMap(*this, fOut);

	fOut.close();

	//if (use_region)
	//	RegionRemove();

	Log::Print("Saved.\n");
	WndMain_Status("Saved.", 0);
}

/*
================
Map::SaveSelection

write selected brushes and entities to a file
================
*/
void Map::SaveSelection(const std::string& filename)
{
	Log::Print(_S("Map::SaveSelection: %s\n") << filename);

	std::ofstream fOut(filename, std::ios::out | std::ios::trunc);
	if (fOut.fail())
		Error(_S("Couldn't write to %s!\n") << filename);

	MapWriter mOut;
	mOut.WriteMap(*this, fOut, (int)MapWriter::BrushListFlag::SELECTED);

	fOut.close();
	Log::Print("Selection exported.\n");
}


bool Map::LoadFromFile(const std::string& filename, Entity& elist, Brush& blist)
{
	// turn all this into something that returns new pools, not new data in the existing ones
	TextFileReader reader;
	try
	{
		reader.Open(filename);
	}
	catch (qe3_exception)
	{
		return false;
	}

	std::string buf;
	reader.ReadAll(buf);

	MapParser parser(buf);
	try
	{
		parser.Read(blist, elist);
	}
	catch (qe3_exception)
	{
		while (elist.Next() != &elist)
			delete elist.Next();
		return false;
	}
	return true;
}

/*
================
Map::Load

replace all current map data with the contents of a file

load map
load wads
build brush data
================
*/
void Map::Load(const std::string& filename)
{
	bool bSnapCheck;

	Log::Print(_S("Map::Load: %s\n") << filename);

	Sys_BeginWait();

	if (g_qeglobals.bGridSnap)
	{
		g_qeglobals.bGridSnap = false;
		bSnapCheck = true;
	}

	WndMain_SetInspectorMode(W_CONSOLE);

	// FIXME: parsing the whole map into brushes and entities, then freeing the old map,
	// keeps loads/pastes/etc isolated until the parse is complete for exception guarantee
	// but leaves a ton of freed space in brush/face pools, and never at the end
	Entity elist;
	Brush blist;
	if (!LoadFromFile(filename, elist, blist))
		return;

	Free();
	name = filename;
	hasFilename = true;

	// now merge the fully loaded data into the scene
	elist.MergeListIntoList(&entities);
	blist.MergeListIntoList(brActive);

	for (Brush* b = brActive.Next(); b != &brActive; b = b->Next())
		numBrushes++;

	for (Entity* e = entities.Next(); e != &entities; e = e->Next())
	{
		// create fixed/non-fixed entclasses on the fly for point entities with brushes or brush 
		//	entities without any, so that all the downstream code Just Works
		bool has_brushes = (e->brushes.ENext() != &e->brushes);
		e->eclass = EntClass::ForName(e->GetKeyValue("classname"), has_brushes, false);

		if (e->eclass->IsPointClass())
		{	// create a custom brush
			e->MakeBrush()->AddToList(brActive);
		}
		else if (e->eclass == EntClass::Worldspawn())
		{
			if (world)
				Log::Warning("Multiple worldspawns!\n");
			world = e;
		}

		numEntities++;
	}

	Log::Print(_S("%i brushes, %i entities\n") << numBrushes << numEntities);

	if (world)
	{
		world->RemoveFromList();
	}
	else
	{
		Log::Warning("No worldspawn in map! Creating new empty worldspawn ...\n");

		world = CreateWorldspawn();
	}

	std::string wads = world->GetKeyValue("wad");
	if (wads.empty())
		Log::Warning("No \"wad\" key.");
	else
		Textures::LoadWadsFromWadstring(wads);

	BuildBrushData();

	// move the view to a start position
	Entity *ent = g_map.FindEntity("classname", "info_player_start");
	if (!ent)
		ent = g_map.FindEntity("classname", "info_player_deathmatch");

	g_vCamera.Reset();
	g_vGrid[0].Center(vec3(0));
	g_vTexture.ChooseFirstTexture();

	if (ent)
	{
		vec3 startOrg;
		ent->GetKeyValueVector("origin", startOrg);
		ent->GetKeyValueVector("origin", startOrg);
		g_vCamera.SetOrigin(startOrg);
		g_vGrid[0].Center(startOrg);
		g_vCamera.Turn(ent->GetKeyValueFloat("angle"), true);
	}


	WndMain_UpdateTitle();
	RegionOff();

	SanityCheck();

	WndMain_UpdateWindows(W_ALL);
	WndMain_SetInspectorMode(W_TEXTURE);

	if (bSnapCheck)
		g_qeglobals.bGridSnap = true;

	Sys_EndWait();
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

	name.clear();
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

	// winding allocator reset must come last, after all
	// brush geometry that might point into the winding pages is destroyed
	Winding::OnMapFree();

	WndMain_UpdateWindows(W_ALL);
}

/*
==================
Map::CreateWorldspawn
==================
*/
Entity* Map::CreateWorldspawn()
{
	Entity* w = new Entity();
	w->SetKeyValue("classname", "worldspawn");
	w->SetKeyValue("wad", g_project.defaultWads);
	w->eclass = EntClass::ForName("worldspawn", true, true);
	return w;
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

	Winding::OnBeforeMapRebuild();

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
Map::ImportFromFile

merge the contents of a file into the current map data

load map
reload current wads + load wads added to wadstring
build brush data
================
*/
void Map::ImportFromFile(const std::string& filename)
{
	char	temp[1024];
	bool	bSnapCheck = false;

	Sys_BeginWait();

	WndMain_SetInspectorMode(W_CONSOLE);

	//Sys_ConvertDOSToUnixName(temp, filename);
	Log::Print(_S("Map::ImportFromFile: %s\n") << temp);

	CmdImportMap* cmdIM = new CmdImportMap();

	if (ImportOptionsDialog(cmdIM))
	{
		cmdIM->File(filename);
		g_cmdQueue.Complete(cmdIM);

		if (world->GetKeyValue("wad").empty())
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

//================================================================

/*
==================
Map::SaveBetween
==================
*/
void Map::SaveBetween(std::string& buf)
{
	if (!Selection::HasBrushes())
		return;

	if (MessageBox(g_hwndMain, "Copy selection to new map?", "QuakeEd 3", MB_YESNO | MB_ICONQUESTION) == IDNO)
		return;

	MapWriter mw;
	std::stringstream sstr;
	mw.WriteMap(*this, sstr, (int)MapWriter::BrushListFlag::SELECTED);
	buf = sstr.str();	// TODO: so many copies between stringstream and this
}

/*
==================
Map::LoadBetween
==================
*/
bool Map::LoadBetween(const std::string& buf)
{
	if (buf.empty())
		return false;		// nothing saved in it

	// TODO: generalized CmdMergeMapText or something

	MapParser parser(buf);
	Entity elist, *ne;
	Brush blist;
	try { parser.Read(blist, elist); }
	catch (qe3_exception)
	{
		while (elist.Next() != &elist)
			delete elist.Next();
		return false;
	}

	elist.MergeListIntoList(&entities);
	blist.MergeListIntoList(g_brSelectedBrushes);	// select copied stuff	

	for (Brush* b = g_brSelectedBrushes.Next(); b != &g_brSelectedBrushes; b = b->Next())
		numBrushes++;
	for (Entity* e = entities.Next(); e != &entities; e = ne)
	{
		ne = e->Next();
		// create fixed/non-fixed entclasses on the fly for point entities with brushes or brush 
		//	entities without any, so that all the downstream code Just Works
		bool has_brushes = (e->brushes.ENext() != &e->brushes);
		e->eclass = EntClass::ForName(e->GetKeyValue("classname"), has_brushes, false);

		if (e->eclass->IsPointClass())
		{	// create a custom brush
			e->MakeBrush()->AddToList(g_brSelectedBrushes);
		}
		else if (e->eclass == EntClass::Worldspawn())
		{
			assert(world);
			for (Brush* b = e->brushes.EPrev(); b != &e->brushes; b = e->brushes.EPrev())
			{
				Entity::UnlinkBrush(b);
				world->LinkBrush(b);
			}
			assert(e->brushes.ENext() == &e->brushes);
			delete e;
		}

		numEntities++;
	}

	Log::Print(_S("%i brushes, %i entities\n") << numBrushes << numEntities);

	Selection::Changed();

	return true;
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
	if (!Selection::HasBrushes())
		return;
	if (!OpenClipboard(g_hwndMain))
		return;

	HGLOBAL hglbCopy;
	int copylen;
	std::stringstream sstr;
	MapWriter mw;
	mw.WriteMap(*this, sstr, (int)MapWriter::BrushListFlag::SELECTED);

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

	texdef.Reset();
	texdef.SetTemp("REGION");

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
		if (pe->GetKeyValue(pszKey) == pszValue)
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
