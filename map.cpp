//==============================
//	map.c
//============================== 

#include "qe3.h"
#include <fstream>
#include <sstream>
#include <string>

qeMap	g_map;

// Cross map selection saving
// this could fuck up if you have only part of a complex entity selected...
Brush		g_brBetweenBrushes;
Entity		g_entBetweenEntities;

Brush	   *g_pbrRegionSides[4];

qeMap::qeMap() : numBrushes(0), numEntities(0), numTextures(0), world(nullptr)
{
	regionMins[0] = regionMins[1] = regionMins[2] = -4096;
	regionMaxs[0] = regionMaxs[1] = regionMaxs[2] = 4096;
	g_brSelectedBrushes.CloseLinks();
	/*
	entities.CloseLinks();
	brActive.CloseLinks();
	brRegioned.CloseLinks();

	copiedBrushes.CloseLinks();
	copiedEntities.CloseLinks();
	*/
}

/*
===========
qeMap::New
===========
*/
void qeMap::New()
{
	char buf[1024];
	qeBuffer between(0);

	Sys_Printf("CMD: Map::New\n");

	SaveBetween(between);
	Free();

	world = new Entity();
	world->SetKeyValue("classname", "worldspawn");
	world->CloseLinks();

	// sikk---> Wad Loading
	strcpy(buf, g_qeglobals.d_entityProject->GetKeyValue("defaultwads"));
	if (strlen(buf))
	{
		int i = 0;
		char *temp, tempwads[1024] = "";
		char *texpath = g_qeglobals.d_entityProject->GetKeyValue("texturepath");

		for (temp = strtok(buf, ";"); temp; temp = strtok(0, ";"), i++)
		{
			if (i)
				strncat(tempwads, ";", 1);
			strcat(tempwads, texpath);
			strcat(tempwads, temp);
		}
		world->SetKeyValue("wad", tempwads);
	}
	// <---sikk

	world->eclass = EntClass::ForName("worldspawn", true, true);

	g_qeglobals.d_camera.angles[YAW] = 0;
	VectorCopy(g_v3VecOrigin, g_qeglobals.d_camera.origin);
	g_qeglobals.d_camera.origin[2] = 48;
	VectorCopy(g_v3VecOrigin, g_qeglobals.d_xyz[0].origin);

	LoadBetween(between);
	BuildBrushData(g_brSelectedBrushes);	// in case something was betweened

	Sys_UpdateWindows(W_ALL);
	modified = false;
}

/*
================
qeMap::Free

trashes all map data, but does not restore a workable blank state (use New() for that)
================
*/
void qeMap::Free()
{
	Pointfile_Clear();
	strcpy(name, "unnamed.map");
	Sys_SetTitle(name);
	g_qeglobals.d_nNumEntities = 0;

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

	// dump the wads after we dump the geometry, because flush calls MapRebuild 
	// which will generate a bunch of notextures that just get thrown away
	Textures::Flush();
	Winding::Clear();
}

/*
==================
qeMap::BuildBrushData
==================
*/
void qeMap::BuildBrushData(Brush &blist)
{
	Brush	*b, *next;

	if (!blist.next || blist.next == &blist)
		return;

	for (b = blist.next; b != NULL && b != &blist; b = next)
	{
		next = b->next;
		b->Build();
		if (!b->brush_faces)
		{
			delete b;
			Sys_Printf("MSG: Removed degenerate brush.\n");
		}
	}
}

/*
==================
qeMap::BuildBrushData
==================
*/
void qeMap::BuildBrushData()
{
	double time;

	Sys_Printf("CMD: Map::BuildBrushData\n");

	Sys_BeginWait();	// this could take a while
	time = Sys_DoubleTime();

	BuildBrushData(brActive);
	BuildBrushData(g_brSelectedBrushes);
	BuildBrushData(brRegioned);

	time = Sys_DoubleTime() - time;
	if (time)
		Sys_Printf("Brush data built in %f seconds\n", time);
	Sys_EndWait();
}

//================================================================

/*
================
qeMap::ParseBufferReplace

parse all entities and brushes from the text buffer, assuming the scene is not empty
================
*/
bool qeMap::ParseBufferMerge(const char *data)
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
	catch (std::exception &ex)
	{
		MessageBox(g_qeglobals.d_hwndMain, ex.what(), "QuakeEd 3: Exception", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}

	Select_DeselectAll(true);
	Entity::MergeListIntoList(&elist, &entities);
	Brush::MergeListIntoList(&blist, &g_brSelectedBrushes);	// merge to selection
	return true;
}

/*
================
qeMap::ParseBufferReplace

parse all entities and brushes from the text buffer, assuming the scene is empty
================
*/
bool qeMap::ParseBufferReplace(const char *data)
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
	catch (std::exception &ex)
	{
		MessageBox(g_qeglobals.d_hwndMain, ex.what(), "QuakeEd 3: Exception", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}

	// now merge the fully loaded data into the scene
	Entity::MergeListIntoList(&elist, &entities);
	Brush::MergeListIntoList(&blist, &brActive);
	return true;
}

/*
================
qeMap::LoadFromFile

replace all current map data with the contents of a file
================
*/
void qeMap::LoadFromFile(const char *filename)
{
	char	temp[1024];
	char	*tempwad, wadkey[1024];
	Entity	*ent;
	bool	bSnapCheck = false;
	qeBuffer between(0);

	Sys_BeginWait();

	// sikk---> make sure Grid Snap is off to insure complex brushes remain intact
	if (!g_qeglobals.d_savedinfo.bNoClamp)
	{
		g_qeglobals.d_savedinfo.bNoClamp = true;
		bSnapCheck = true;
	}
	// <---sikk

	InspWnd_SetMode(W_CONSOLE);

	QE_ConvertDOSToUnixName(temp, filename);
	Sys_Printf("CMD: Map::LoadFromFile: %s\n", temp);

	SaveBetween(between);
	Free();

	g_qeglobals.d_nParsedBrushes = 0;

	qeBuffer buf;
	IO_LoadFile(filename, buf);

	if (ParseBufferReplace((char*)*buf))
	{
		strcpy(name, filename);

		if (!world)
		{
			Sys_Printf("WARNING: No worldspawn in map! Creating new empty worldspawn ...\n");

			world = new Entity();
			world->SetKeyValue("classname", "worldspawn");
		}
		world->CloseLinks();

		if (!*world->GetKeyValue("wad"))
			Sys_Printf("WARNING: No \"wad\" key.\n");
		else
		{
			strcpy(wadkey, world->GetKeyValue("wad"));

			for (tempwad = strtok(wadkey, ";"); tempwad; tempwad = strtok(0, ";"))
				Textures::LoadWad(tempwad);
		}

		Sys_Printf("--- LoadMapFile ---\n");
		Sys_Printf("%s\n", temp);
		Sys_Printf("%5i brushes\n", g_qeglobals.d_nParsedBrushes);
		Sys_Printf("%5i entities\n", g_qeglobals.d_nNumEntities);

		LoadBetween(between);

		BuildBrushData();

		// move the view to a start position
		ent = Map_FindClass("info_player_start");
		if (!ent)
			ent = Map_FindClass("info_player_deathmatch");

		g_qeglobals.d_camera.angles[PITCH] = 0;

		if (ent)
		{
			ent->GetKeyValueVector("origin", g_qeglobals.d_camera.origin);
			ent->GetKeyValueVector("origin", g_qeglobals.d_xyz[0].origin);
			g_qeglobals.d_camera.angles[YAW] = ent->GetKeyValueFloat("angle");
		}
		else
		{
			g_qeglobals.d_camera.angles[YAW] = 0;
			VectorCopy(g_v3VecOrigin, g_qeglobals.d_camera.origin);
			VectorCopy(g_v3VecOrigin, g_qeglobals.d_xyz[0].origin);
		}

		//Texture_ShowInuse();
		Textures::FlushUnused();
		modified = false;
		Sys_SetTitle(temp);
		RegionOff();
	}

	Sys_UpdateWindows(W_ALL);

	if (bSnapCheck)	// sikk - turn Grid Snap back on if it was on before map load
		g_qeglobals.d_savedinfo.bNoClamp = false;

	Sys_EndWait();
}

/*
================
qeMap::ImportFromFile

merge the contents of a file into the current map data
================
*/
void qeMap::ImportFromFile(const char *filename)
{
	char	temp[1024];
	bool	bSnapCheck = false;

	Sys_BeginWait();

	// sikk---> make sure Grid Snap is off to insure complex brushes remain intact
	if (!g_qeglobals.d_savedinfo.bNoClamp)
	{
		g_qeglobals.d_savedinfo.bNoClamp = true;
		bSnapCheck = true;
	}
	// <---sikk

	InspWnd_SetMode(W_CONSOLE);

	QE_ConvertDOSToUnixName(temp, filename);
	Sys_Printf("CMD: Map::ImportFromFile: %s\n", temp);

	g_qeglobals.d_nParsedBrushes = 0;

	qeBuffer buf;
	IO_LoadFile(filename, buf);
	if (ParseBufferMerge((char*)*buf))
	{
		g_bSelectionChanged = true;
		modified = true;
		BuildBrushData(g_brSelectedBrushes);
	}

	if (bSnapCheck)	// sikk - turn Grid Snap back on if it was on before map load
		g_qeglobals.d_savedinfo.bNoClamp = false;

	Sys_EndWait();
	Sys_UpdateWindows(W_ALL);
}

/*
================
qeMap::SaveToFile

write entire contents of the scene to a file
================
*/
void qeMap::SaveToFile(const char *filename, bool use_region)
{
//	Entity   *e, *next;
	std::ofstream	   *f;
	char        temp[1024];
//	int			count;

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

	Sys_Printf("CMD: Map::SaveToFile: %s\n", filename);

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

	modified = false;

	if (!strstr(temp, "autosave"))
		Sys_SetTitle(temp);

	g_bMBCheck = false;	// sikk - Reset this to false
	g_nBrushNumCheck = -1;	// sikk - Reset this to -1

	Sys_Printf("MSG: Saved.\n");
	Sys_Status("Saved.", 0);
}

/*
================
qeMap::ExportToFile

write selected brushes and entities to a file
================
*/
void qeMap::ExportToFile(const char *filename)
{
//	Entity   *e, *next;
	std::ofstream	   *f;
//	int			count;

	Sys_Printf("CMD: Map::ExportToFile: %s\n", filename);

	f = new std::ofstream(filename);
	if (!f)
	{
		Sys_Printf("ERROR: Could not open %s\n", filename);
		return;
	}
	WriteSelected(*f);
	f->close();

	Sys_Printf("MSG: Selection exported.\n", filename);
}

/*
================
qeMap::Cut

write selected brushes and entities to the windows clipboard and delete them
================
*/
void qeMap::Cut()
{
	Copy();
	Select_Delete();
}

/*
================
qeMap::Copy

write selected brushes and entities to the windows clipboard
================
*/
void qeMap::Copy()
{
	HGLOBAL hglbCopy;
	int copylen;

	if (!Select_HasBrushes())
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
qeMap::Paste

merge the contents of the windows clipboard into the current map data
================
*/
void qeMap::Paste()
{
	HGLOBAL hglb;
	char*	cbdata;

	if (!IsClipboardFormatAvailable(CF_TEXT)) return;
	if (!OpenClipboard(g_qeglobals.d_hwndMain)) return;

	Sys_BeginWait();
	hglb = GetClipboardData(CF_TEXT);
	if (hglb != nullptr)
	{
		cbdata = (char*)GlobalLock(hglb);
		if (cbdata != nullptr && cbdata[0] == '{')	// no opening brace = definitely not map data, don't even complain
		{
			bool	bSnapCheck = false;

			// sikk---> make sure Grid Snap is off to insure complex brushes remain intact
			if (!g_qeglobals.d_savedinfo.bNoClamp)
			{
				g_qeglobals.d_savedinfo.bNoClamp = true;
				bSnapCheck = true;
			}
			// <---sikk

			g_qeglobals.d_nParsedBrushes = 0;

			if (ParseBufferMerge(cbdata))
			{
				g_bSelectionChanged = true;
				modified = true;
				BuildBrushData(g_brSelectedBrushes);
			}

			if (bSnapCheck)	// sikk - turn Grid Snap back on if it was on before map load
				g_qeglobals.d_savedinfo.bNoClamp = false;

			Sys_UpdateWindows(W_ALL);
		}
		GlobalUnlock(hglb);
	}
	CloseClipboard();
	Sys_EndWait();
}

//================================================================

/*
================
qeMap::Read

parse the map data and link all brushes and entities to the provided lists
================
*/
void qeMap::Read(const char *data, Brush &blist, Entity &elist)
{
	int numEntities;
	Entity* ent;
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
				Sys_Printf("WARNING: Multiple worldspawn.\n");
			foundWorld = true;

			// add the worldspawn to the beginning of the entity list so it's easy to find
			ent->prev = &elist;
			ent->next = elist.next;
			elist.next->prev = ent;
			elist.next = ent;
		}
		else
		{
			// add the entity to the end of the entity list
			ent->next = &elist;
			ent->prev = elist.prev;
			elist.prev->next = ent;
			elist.prev = ent;
			numEntities++;
		}

		// add all the brushes to the brush list
		for (Brush* b = ent->brushes.onext; b != &ent->brushes; b = b->onext)
		{
			b->next = blist.next;
			blist.next->prev = b;
			b->prev = &blist;
			blist.next = b;
		}
	}
}

/*
================
qeMap::WriteSelected

map-print only selected brushes and entities to the buffer
================
*/
void qeMap::WriteSelected(std::ostream &out)
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
		if (e->brushes.onext == &e->brushes)
		{
			assert(0);
			delete e;	// no brushes left, so remove it
		}
		else
			e->WriteSelected(out);
	}
}

/*
================
qeMap::WriteSelected

map-print all brushes and entities to the buffer
================
*/
void qeMap::WriteAll(std::ostream &out, bool use_region)
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
		if (e->brushes.onext == &e->brushes)
		{
			assert(0);
			delete e;	// no brushes left, so remove it
		}
		else
			e->Write(out, use_region);
	}
}

/*
==================
qeMap::SaveBetween
==================
*/
void qeMap::SaveBetween(qeBuffer &buf)
{
	int copylen;

	if (!Select_HasBrushes())
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
qeMap::LoadBetween
==================
*/
void qeMap::LoadBetween(qeBuffer &buf)
{
	if (!buf.size())
		return;		// nothing saved in it

	if (ParseBufferMerge((char*)*buf))
	{
		//BuildBrushData(g_brSelectedBrushes);
		g_bSelectionChanged = true;
		modified = true;
	}
}

//================================================================

void qeMap::RegionOff()
{
	Brush	*b, *next;

	regionActive = false;

	for (int i = 0; i < 3; i++)
	{
		regionMaxs[i] = g_qeglobals.d_savedinfo.nMapSize * 0.5;//4096;	// sikk - Map Size
		regionMins[i] = -g_qeglobals.d_savedinfo.nMapSize * 0.5;//-4096;	// sikk - Map Size
	}

	for (b = brRegioned.next; b != &brRegioned; b = next)
	{
		next = b->next;
		if (IsBrushFiltered(b))
			continue;		// still filtered
		b->RemoveFromList();
		b->AddToList(&brActive);
	}

	Sys_UpdateWindows(W_ALL);
}

void qeMap::RegionXY()
{
	RegionOff();

	float w, h;

	w = 0.5 * g_qeglobals.d_xyz[0].width / g_qeglobals.d_xyz[0].scale;
	h = 0.5 * g_qeglobals.d_xyz[0].height / g_qeglobals.d_xyz[0].scale;

	// sikk---> Proper Regioning for XZ & YZ Views

	if (g_qeglobals.d_xyz[0].dViewType == XY)
	{
		regionMins[0] = g_qeglobals.d_xyz[0].origin[0] - w;
		regionMaxs[0] = g_qeglobals.d_xyz[0].origin[0] + w;
		regionMins[1] = g_qeglobals.d_xyz[0].origin[1] - h;
		regionMaxs[1] = g_qeglobals.d_xyz[0].origin[1] + h;
		regionMins[2] = -g_qeglobals.d_savedinfo.nMapSize * 0.5;//-4096;	// sikk - Map Size
		regionMaxs[2] = g_qeglobals.d_savedinfo.nMapSize * 0.5;//4096;	// sikk - Map Size
	}
	if (g_qeglobals.d_xyz[0].dViewType == XZ)
	{
		regionMins[0] = g_qeglobals.d_xyz[0].origin[0] - w;
		regionMaxs[0] = g_qeglobals.d_xyz[0].origin[0] + w;
		regionMins[1] = -g_qeglobals.d_savedinfo.nMapSize * 0.5;//-4096;	// sikk - Map Size
		regionMaxs[1] = g_qeglobals.d_savedinfo.nMapSize * 0.5;//4096;	// sikk - Map Size
		regionMins[2] = g_qeglobals.d_xyz[0].origin[2] - h;
		regionMaxs[2] = g_qeglobals.d_xyz[0].origin[2] + h;
	}
	if (g_qeglobals.d_xyz[0].dViewType == YZ)
	{
		regionMins[0] = -g_qeglobals.d_savedinfo.nMapSize * 0.5;//-4096;	// sikk - Map Size
		regionMaxs[0] = g_qeglobals.d_savedinfo.nMapSize * 0.5;//4096;	// sikk - Map Size
		regionMins[1] = g_qeglobals.d_xyz[0].origin[1] - w;
		regionMaxs[1] = g_qeglobals.d_xyz[0].origin[1] + w;
		regionMins[2] = g_qeglobals.d_xyz[0].origin[2] - h;
		regionMaxs[2] = g_qeglobals.d_xyz[0].origin[2] + h;
	}
	// <---sikk
	RegionApply();
}

void qeMap::RegionXZ()
{
	RegionOff();
	regionMins[0] = g_qeglobals.d_xyz[2].origin[0] - 0.5 * g_qeglobals.d_xyz[2].width / g_qeglobals.d_xyz[2].scale;
	regionMaxs[0] = g_qeglobals.d_xyz[2].origin[0] + 0.5 * g_qeglobals.d_xyz[2].width / g_qeglobals.d_xyz[2].scale;
	regionMins[1] = -g_qeglobals.d_savedinfo.nMapSize * 0.5;//-4096;	// sikk - Map Size
	regionMaxs[1] = g_qeglobals.d_savedinfo.nMapSize * 0.5;//4096;	// sikk - Map Size
	regionMins[2] = g_qeglobals.d_xyz[2].origin[2] - 0.5 * g_qeglobals.d_xyz[2].height / g_qeglobals.d_xyz[2].scale;
	regionMaxs[2] = g_qeglobals.d_xyz[2].origin[2] + 0.5 * g_qeglobals.d_xyz[2].height / g_qeglobals.d_xyz[2].scale;
	RegionApply();
}

void qeMap::RegionYZ()
{
	RegionOff();
	regionMins[0] = -g_qeglobals.d_savedinfo.nMapSize * 0.5;//-4096;	// sikk - Map Size
	regionMaxs[0] = g_qeglobals.d_savedinfo.nMapSize * 0.5;//4096;	// sikk - Map Size
	regionMins[1] = g_qeglobals.d_xyz[1].origin[1] - 0.5 * g_qeglobals.d_xyz[1].width / g_qeglobals.d_xyz[1].scale;
	regionMaxs[1] = g_qeglobals.d_xyz[1].origin[1] + 0.5 * g_qeglobals.d_xyz[1].width / g_qeglobals.d_xyz[1].scale;
	regionMins[2] = g_qeglobals.d_xyz[1].origin[2] - 0.5 * g_qeglobals.d_xyz[1].height / g_qeglobals.d_xyz[1].scale;
	regionMaxs[2] = g_qeglobals.d_xyz[1].origin[2] + 0.5 * g_qeglobals.d_xyz[1].height / g_qeglobals.d_xyz[1].scale;
	RegionApply();
}

void qeMap::RegionTallBrush()
{
	Brush	*b;

	if (!QE_SingleBrush())
		return;

	b = g_brSelectedBrushes.next;

	RegionOff();

	VectorCopy(b->mins, regionMins);
	VectorCopy(b->maxs, regionMaxs);
	regionMins[2] = -g_qeglobals.d_savedinfo.nMapSize * 0.5;//-4096;	// sikk - Map Size
	regionMaxs[2] = g_qeglobals.d_savedinfo.nMapSize * 0.5;//4096;	// sikk - Map Size

	Select_Delete();
	RegionApply();
}

void qeMap::RegionBrush()
{
	Brush	*b;

	if (!QE_SingleBrush())
		return;

	b = g_brSelectedBrushes.next;

	RegionOff();

	VectorCopy(b->mins, regionMins);
	VectorCopy(b->maxs, regionMaxs);

	Select_Delete();
	RegionApply();
}

void qeMap::RegionSelectedBrushes()
{
	RegionOff();

	if (!Select_HasBrushes())
		return;

	regionActive = true;
	Select_GetBounds(regionMins, regionMaxs);

	// move the entire active_brushes list to filtered_brushes
	Brush::MergeListIntoList(&brActive, &brRegioned);

	// move the entire g_brSelectedBrushes list to brActive
	Brush::MergeListIntoList(&g_brSelectedBrushes, &brActive);

	Sys_UpdateWindows(W_ALL);
}

void qeMap::RegionApply()
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

	Sys_UpdateWindows(W_ALL);
}

void qeMap::RegionAdd()
{
	vec3_t		mins, maxs;
	int			i;
	texdef_t	texdef;

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

void qeMap::RegionRemove()
{
	if (!regionActive)
		return;
	for (int i = 0; i < 4; i++)
		delete g_pbrRegionSides[i];
}

bool qeMap::IsBrushFiltered(Brush *b)
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
==================
Map_FindClass
==================
*/
Entity *Map_FindClass (char *cname)
{
	Entity *ent;

	for (ent = g_map.entities.next; ent != &g_map.entities; ent = ent->next)
		if (!strcmp(ent->GetKeyValue("classname"), cname))
			return ent;

	return NULL;
}
