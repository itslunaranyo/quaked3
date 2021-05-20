//==============================
//	map.cpp
//============================== 

#include "qe3.h"
#include "parse.h"
#include <fstream>
#include <sstream>
#include <string>

#include "CmdImportMap.h"
#include "CmdPaste.h"
#include "points.h"

Map		g_map;
Brush	*g_pbrRegionSides[4];

Map::Map() : numBrushes(0), numEntities(0), numTextures(0), world(nullptr)
{
	g_brSelectedBrushes.CloseLinks();
	ClearBounds(regionMins, regionMaxs);

	/*
	entities.CloseLinks();
	brActive.CloseLinks();
	brRegioned.CloseLinks();
	*/
}

/*
===========
Map::New
===========
*/
void Map::New()
{
	qeBuffer between(0);

	Sys_Printf("Map::New\n");

	SaveBetween(between);
	Free();

	world = new Entity();
	world->SetKeyValue("classname", "worldspawn");
	world->CloseLinks();

	// sikk---> Wad Loading
	/*
	char buf[1024];
	strcpy(buf, g_project.defaultWads);
	if (strlen(buf))
	{
		int i = 0;
		char *temp, tempwads[1024] = "";
		char *texpath = g_project.wadPath;
		for (temp = strtok(buf, ";"); temp; temp = strtok(0, ";"), i++)
		{
			if (i)
				strncat(tempwads, ";", 1);
			strcat(tempwads, texpath);
			strcat(tempwads, temp);
		}
		world->SetKeyValue("wad", tempwads);
	}
	*/
	world->SetKeyValue("wad", g_project.defaultWads);
	// <---sikk

	world->eclass = EntClass::ForName("worldspawn", true, true);

	g_qeglobals.d_vCamera.angles[YAW] = 0;
	g_qeglobals.d_vCamera.origin = vec3(0);
	g_qeglobals.d_vCamera.origin[2] = 48;
	g_qeglobals.d_vXYZ[0].origin = vec3(0);

	if (LoadBetween(between))
		BuildBrushData(g_brSelectedBrushes);	// in case something was betweened

	QE_SetInspectorMode(W_TEXTURE);
	Sys_UpdateWindows(W_ALL);
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

	if (brActive.next)
		while (brActive.next != &brActive)
			delete brActive.next;
	else
		brActive.CloseLinks();

	if (brRegioned.next)
		while (brRegioned.next != &brRegioned)
			delete brRegioned.next;
	else
		brRegioned.CloseLinks();

	if (entities.next)
		while (entities.next != &entities)
			delete entities.next;
	else
		entities.CloseLinks();

	if (g_brSelectedBrushes.next)
		while (g_brSelectedBrushes.next != &g_brSelectedBrushes)
			delete g_brSelectedBrushes.next;
	else
		g_brSelectedBrushes.CloseLinks();

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

	Sys_UpdateWindows(W_ALL);
}

/*
==================
Map::BuildBrushData
==================
*/
void Map::BuildBrushData(Brush &blist)
{
	Brush	*b, *next;

	if (!blist.next || blist.next == &blist)
		return;

	for (b = blist.next; b != NULL && b != &blist; b = next)
	{
		next = b->next;

		// buildbrushdata is called after a wad is loaded or refreshed to make sure names match texture pointers
		// wads are also not loaded until after worldspawn is parsed during a map load
		if (!b->FullBuild() || !b->faces)
		{
			Warning("Removed degenerate brush with mins (%f %f %f) maxs (%f %f %f).",
				b->mins[0], b->mins[1], b->mins[2],
				b->maxs[0], b->maxs[1], b->maxs[2]);
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

	Sys_Printf("Map::BuildBrushData\n");

	Sys_BeginWait();	// this could take a while
	time = Sys_DoubleTime();

	BuildBrushData(brActive);
	BuildBrushData(g_brSelectedBrushes);
	BuildBrushData(brRegioned);
	Sys_UpdateBrushStatusBar();

	time = Sys_DoubleTime() - time;
	if (time)
		Sys_Printf("Brush data built in %f seconds\n", time);
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
	elist.CloseLinks();
	blist.CloseLinks();

	try
	{
		Read(data, blist, elist);
		for (Entity *ent = elist.next; ent != &elist; ent = ent->next)
		{
			// world brushes need to be merged into the existing worldspawn
			if (ent->eclass == EntClass::worldspawn)
			{
				ent->RemoveFromList();
				Brush *b, *next;
				for (b = ent->brushes.onext; b != &ent->brushes; b = next)
				{
					next = b->onext;
					Entity::UnlinkBrush(b);
					world->LinkBrush(b);
				}
				delete ent;
				break;
			}
		}
	}
	catch (qe3_exception &ex)
	{
		ReportError(ex);
		// blist and elist are auto-freed by stack unwinding without a merger,
		// so the map isn't polluted with a partial import when we exit
		return false;
	}

	Selection::DeselectAll();
	elist.MergeListIntoList(&entities);
	blist.MergeListIntoList(&g_brSelectedBrushes);	// merge to selection
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

	elist.CloseLinks();
	blist.CloseLinks();

	try
	{
		Read(data, blist, elist);
		for (Entity *ent = elist.next; ent != &elist; ent = ent->next)
		{
			if (ent->eclass == EntClass::worldspawn)
			{
				ent->RemoveFromList();
				world = ent;
				break;
			}
		}
	}
	catch (qe3_exception &ex)
	{
		ReportError(ex);
		// don't need to free here, loaded map wasn't merged into the now-empty scene yet
		return false;
	}

	// now merge the fully loaded data into the scene
	elist.MergeListIntoList(&entities);
	blist.MergeListIntoList(&brActive);
	SanityCheck();
	return true;
}


void Map::SanityCheck()
{
#ifdef _DEBUG
	Entity *e;
	Brush *b;
	for (e = entities.next; e != &entities; e = e->next)
	{
		assert(e->next->prev == e);
		assert(e->brushes.onext->oprev == &e->brushes);
		assert(e->brushes.oprev->onext == &e->brushes);

		for (b = e->brushes.onext; b != &e->brushes; b = b->onext)
		{
			assert(b->onext->oprev == b);
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

	QE_SetInspectorMode(W_CONSOLE);

	QE_ConvertDOSToUnixName(temp, filename);
	Sys_Printf("Map::LoadFromFile: %s\n", temp);

	SaveBetween(between);
	Free();

	qeBuffer buf;
	if (IO_LoadFile(filename, buf) < 1)
		Error("Couldn't load %s!", filename);

	if (LoadFromFile_Parse((char*)*buf))
	{
		strcpy(name, filename);
		hasFilename = true;

		if (!world)
		{
			Warning("No worldspawn in map! Creating new empty worldspawn ...");

			world = new Entity();
			world->SetKeyValue("classname", "worldspawn");
		}
		world->CloseLinks();

		if (!*world->GetKeyValue("wad"))
			Warning("No \"wad\" key.");
		else
			Textures::LoadWadsFromWadstring(world->GetKeyValue("wad"));

		for (Brush* b = brActive.next; b != &brActive; b = b->next)
			numBrushes++;
		for (Entity* e = entities.next; e != &entities; e = e->next)
		{
			if (e->IsPoint())
				numBrushes--;
			numEntities++;
		}

		Sys_Printf("--- LoadMapFile ---\n");
		Sys_Printf("%s\n", temp);
		Sys_Printf("%5i brushes\n%5i entities\n", numBrushes, numEntities);

		LoadBetween(between);
		g_map.BuildBrushData();

		// move the view to a start position
		ent = g_map.FindEntity("classname", "info_player_start");
		if (!ent)
			ent = g_map.FindEntity("classname", "info_player_deathmatch");

		g_qeglobals.d_vCamera.angles[PITCH] = 0;

		if (ent)
		{
			ent->GetKeyValueVector("origin", g_qeglobals.d_vCamera.origin);
			ent->GetKeyValueVector("origin", g_qeglobals.d_vXYZ[0].origin);
			g_qeglobals.d_vCamera.angles[YAW] = ent->GetKeyValueFloat("angle");
		}
		else
		{
			g_qeglobals.d_vCamera.angles[YAW] = 0;
			g_qeglobals.d_vCamera.origin = vec3(0);
			g_qeglobals.d_vXYZ[0].origin = vec3(0);
		}

		Textures::FlushUnused();	// should be FlushUnusedFromWadstring technically but those are the only wads loaded yet

		QE_UpdateTitle();
		RegionOff();
	}

	Sys_UpdateBrushStatusBar();
	QE_SetInspectorMode(W_TEXTURE);
	Sys_UpdateWindows(W_ALL);

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
	const char	*w;
	char	temp[1024];
	bool	bSnapCheck = false;

	Sys_BeginWait();

	QE_SetInspectorMode(W_CONSOLE);

	QE_ConvertDOSToUnixName(temp, filename);
	Sys_Printf("Map::ImportFromFile: %s\n", temp);

	CmdImportMap* cmdIM = new CmdImportMap();

	if (ImportOptionsDialog(cmdIM))
	{
		cmdIM->File(filename);
		g_cmdQueue.Complete(cmdIM);

		Textures::ReloadAll();
		w = cmdIM->WadsAdded();
		if (w)
		{
			Textures::LoadWadsFromWadstring(w);
			Textures::RefreshUsedStatus();
			Textures::FlushUnusedFromWadstring(w);
		}
		g_map.BuildBrushData();

		Sys_UpdateWindows(W_ALL);
	}
	else
	{
		Sys_Printf("Import canceled.\n");
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

	QE_ConvertDOSToUnixName(temp, filename);

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

	Sys_Printf("Map::SaveToFile: %s\n", filename);

	f = new std::ofstream(filename);
	if (!f)
	{
		Sys_Printf("ERROR: Could not open %s\n", filename);
		return;
	}

	if (use_region)
		RegionAdd();

	WriteAll(*f, use_region);

	f->close();

	if (use_region)
		RegionRemove();

	Sys_Printf("Saved.\n");
	Sys_Status("Saved.", 0);
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

	Sys_Printf("Map::ExportToFile: %s\n", filename);

	f = new std::ofstream(filename);
	if (!f)
	{
		Sys_Printf("ERROR: Could not open %s\n", filename);
		return;
	}
	WriteSelected(*f);
	f->close();

	Sys_Printf("Selection exported.\n", filename);
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
	if (!OpenClipboard(g_qeglobals.d_hwndMain))
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
				Warning("Multiple worldspawn.");
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
		for (Brush* b = ent->brushes.onext; b != &ent->brushes; b = next)
		{
			next = b->onext;
			b->next = blist.next;
			blist.next->prev = b;
			b->prev = &blist;
			blist.next = b;
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
	int count;
	Entity *e, *next;

	// write world entity first
	world->WriteSelected(out);

	// then write all other ents
	count = 1;
	for (e = entities.next; e != &entities; e = next)
	{
		out << "// entity " << count << "\n";
		count++;
		next = e->next;
		/*
		if (e->brushes.onext == &e->brushes)
		{
			assert(0);
			delete e;	// no brushes left, so remove it
		}
		else
			e->WriteSelected(out);
		*/
		if (e->brushes.onext != &e->brushes)
			e->WriteSelected(out);
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
	for (e = entities.next; e != &entities; e = next)
	{
		out << "// entity " << count << "\n";
		count++;
		next = e->next;

		if (e->brushes.onext != &e->brushes)
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
	if (MessageBox(g_qeglobals.d_hwndMain, "Copy selection to new map?", "QuakeEd 3", MB_YESNO | MB_ICONQUESTION) == IDNO)
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

	for (b = brRegioned.next; b != &brRegioned; b = next)
	{
		next = b->next;
		if (IsBrushFiltered(b))
			continue;		// still filtered
		b->RemoveFromList();
		b->AddToList(&brActive);
	}

	Sys_UpdateWindows(W_SCENE | W_TARGETGRAPH);
}

void Map::RegionXY()
{
	RegionOff();

	float w, h;

	w = 0.5 * g_qeglobals.d_vXYZ[0].width / g_qeglobals.d_vXYZ[0].scale;
	h = 0.5 * g_qeglobals.d_vXYZ[0].height / g_qeglobals.d_vXYZ[0].scale;

	// sikk---> Proper Regioning for XZ & YZ Views

	if (g_qeglobals.d_vXYZ[0].GetAxis() == XY)
	{
		regionMins[0] = g_qeglobals.d_vXYZ[0].origin[0] - w;
		regionMaxs[0] = g_qeglobals.d_vXYZ[0].origin[0] + w;
		regionMins[1] = g_qeglobals.d_vXYZ[0].origin[1] - h;
		regionMaxs[1] = g_qeglobals.d_vXYZ[0].origin[1] + h;
		regionMins[2] = -g_cfgEditor.MapSize / 2;
		regionMaxs[2] = g_cfgEditor.MapSize / 2;
	}
	else if (g_qeglobals.d_vXYZ[0].GetAxis() == XZ)
	{
		regionMins[0] = g_qeglobals.d_vXYZ[0].origin[0] - w;
		regionMaxs[0] = g_qeglobals.d_vXYZ[0].origin[0] + w;
		regionMins[1] = -g_cfgEditor.MapSize / 2;
		regionMaxs[1] = g_cfgEditor.MapSize / 2;
		regionMins[2] = g_qeglobals.d_vXYZ[0].origin[2] - h;
		regionMaxs[2] = g_qeglobals.d_vXYZ[0].origin[2] + h;
	}
	else if (g_qeglobals.d_vXYZ[0].GetAxis() == YZ)
	{
		regionMins[0] = -g_cfgEditor.MapSize / 2;
		regionMaxs[0] = g_cfgEditor.MapSize / 2;
		regionMins[1] = g_qeglobals.d_vXYZ[0].origin[1] - w;
		regionMaxs[1] = g_qeglobals.d_vXYZ[0].origin[1] + w;
		regionMins[2] = g_qeglobals.d_vXYZ[0].origin[2] - h;
		regionMaxs[2] = g_qeglobals.d_vXYZ[0].origin[2] + h;
	}
	// <---sikk
	RegionApply();
}

void Map::RegionXZ()
{
	RegionOff();
	regionMins[0] = g_qeglobals.d_vXYZ[2].origin[0] - 0.5 * g_qeglobals.d_vXYZ[2].width / g_qeglobals.d_vXYZ[2].scale;
	regionMaxs[0] = g_qeglobals.d_vXYZ[2].origin[0] + 0.5 * g_qeglobals.d_vXYZ[2].width / g_qeglobals.d_vXYZ[2].scale;
	regionMins[1] = -g_cfgEditor.MapSize / 2;
	regionMaxs[1] = g_cfgEditor.MapSize / 2;
	regionMins[2] = g_qeglobals.d_vXYZ[2].origin[2] - 0.5 * g_qeglobals.d_vXYZ[2].height / g_qeglobals.d_vXYZ[2].scale;
	regionMaxs[2] = g_qeglobals.d_vXYZ[2].origin[2] + 0.5 * g_qeglobals.d_vXYZ[2].height / g_qeglobals.d_vXYZ[2].scale;
	RegionApply();
}

void Map::RegionYZ()
{
	RegionOff();
	regionMins[0] = -g_cfgEditor.MapSize / 2;
	regionMaxs[0] = g_cfgEditor.MapSize / 2;
	regionMins[1] = g_qeglobals.d_vXYZ[1].origin[1] - 0.5 * g_qeglobals.d_vXYZ[1].width / g_qeglobals.d_vXYZ[1].scale;
	regionMaxs[1] = g_qeglobals.d_vXYZ[1].origin[1] + 0.5 * g_qeglobals.d_vXYZ[1].width / g_qeglobals.d_vXYZ[1].scale;
	regionMins[2] = g_qeglobals.d_vXYZ[1].origin[2] - 0.5 * g_qeglobals.d_vXYZ[1].height / g_qeglobals.d_vXYZ[1].scale;
	regionMaxs[2] = g_qeglobals.d_vXYZ[1].origin[2] + 0.5 * g_qeglobals.d_vXYZ[1].height / g_qeglobals.d_vXYZ[1].scale;
	RegionApply();
}

void Map::RegionTallBrush()
{
	Brush	*b;

	if (!QE_SingleBrush())
		return;

	b = g_brSelectedBrushes.next;

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

	b = g_brSelectedBrushes.next;

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
	brActive.MergeListIntoList(&brRegioned);

	// move the entire g_brSelectedBrushes list to brActive
	g_brSelectedBrushes.MergeListIntoList(&brActive);
	Selection::Changed();
	Sys_UpdateWindows(W_SCENE | W_TARGETGRAPH);
}

void Map::RegionApply()
{
	Brush	*b, *next;

	regionActive = true;
	for (b = brActive.next; b != &brActive; b = next)
	{
		next = b->next;
		if (!IsBrushFiltered(b))
			continue;		// still filtered
		b->RemoveFromList();
		b->AddToList(&brRegioned);
	}

	Sys_UpdateWindows(W_SCENE | W_TARGETGRAPH);
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
		g_pbrRegionSides[i]->AddToList(&g_brSelectedBrushes);
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

	pe = entities.next;

	for (; pe != nullptr && pe != &entities; pe = pe->next)
		if (!strcmp(pe->GetKeyValue(pszKey), pszValue))
			return pe;

	return nullptr;
}

Entity *Map::FindEntity(char *pszKey, int iValue)
{
	Entity *pe;

	pe = entities.next;

	for (; pe != nullptr && pe != &entities; pe = pe->next)
		if (pe->GetKeyValueInt(pszKey) == iValue)
			return pe;

	return nullptr;
}
