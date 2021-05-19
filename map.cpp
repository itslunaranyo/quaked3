//==============================
//	map.c
//============================== 

#include "qe3.h"


BOOL		g_bModified;		// for quit confirmation (0 = clean, 1 = unsaved,
								// 2 = autosaved, but not regular saved) 

char		g_szCurrentMap[MAX_PATH];

brush_t		g_brActiveBrushes;		// brushes currently being displayed
brush_t		g_brFilteredBrushes;	// brushes that have been filtered or regioned

entity_t	g_entEntities;			// head/tail of doubly linked list
entity_t   *g_peWorldEntity;

brush_t		g_brCopiedBrushes;		// sikk - For Cut/Copy/Paste
entity_t	g_entCopiedEntities;	// sikk - For Cut/Copy/Paste


// Cross map selection saving
// this could fuck up if you have only part of a complex entity selected...
brush_t		g_brBetweenBrushes;
entity_t	g_entBetweenEntities;

int	g_nNumBrushes, g_nNumEntities, g_nNumTextures;


/*
==================
Map_SaveBetween
==================
*/
void Map_SaveBetween ()
{
	brush_t		*b;
	entity_t	*e, *e2;

	g_brBetweenBrushes.next = g_brSelectedBrushes.next;
	g_brBetweenBrushes.prev = g_brSelectedBrushes.prev;
	g_brBetweenBrushes.next->prev = &g_brBetweenBrushes;
	g_brBetweenBrushes.prev->next = &g_brBetweenBrushes;

	g_entBetweenEntities.next = g_entBetweenEntities.prev = &g_entBetweenEntities;
	g_brSelectedBrushes.next = g_brSelectedBrushes.prev = &g_brSelectedBrushes;

	for (b = g_brBetweenBrushes.next; b != &g_brBetweenBrushes; b = b->next)
	{
		e = b->owner;
		if (e == g_peWorldEntity)
			b->owner = NULL;
		else
		{
			for (e2 = g_entBetweenEntities.next; e2 != &g_entBetweenEntities; e2 = e2->next)
				if (e2 == e)
					goto next;	// already got the entity

			// move the entity over
			e->prev->next = e->next;
			e->next->prev = e->prev;
			e->next = g_entBetweenEntities.next;
			e->prev = &g_entBetweenEntities;
			e->next->prev = e;
			e->prev->next = e;
		}
next: ;
	}
}

/*
==================
Map_RestoreBetween
==================
*/
void Map_RestoreBetween ()
{
	entity_t	*head, *tail;
	brush_t		*b;

	if (!g_brBetweenBrushes.next)
		return;

	for (b = g_brBetweenBrushes.next; b != &g_brBetweenBrushes; b = b->next)
	{
		if (!b->owner)
		{
			b->owner = g_peWorldEntity;
			b->onext = g_peWorldEntity->brushes.onext;
			b->oprev = &g_peWorldEntity->brushes;
			b->onext->oprev = b;
			b->oprev->onext = b;
		}
	}

	g_brSelectedBrushes.next = g_brBetweenBrushes.next;
	g_brSelectedBrushes.prev = g_brBetweenBrushes.prev;
	g_brSelectedBrushes.next->prev = &g_brSelectedBrushes;
	g_brSelectedBrushes.prev->next = &g_brSelectedBrushes;

	head = g_entBetweenEntities.next;
	tail = g_entBetweenEntities.prev;

	if (head != tail)
	{
		g_entEntities.prev->next = head;
		head->prev = g_entEntities.prev;
		tail->next = &g_entEntities;
		g_entEntities.prev = tail;
	}

	g_brBetweenBrushes.next = NULL;
	g_entBetweenEntities.next = NULL;
}

//============================================================================

/*
==================
Map_BuildBrushData
==================
*/
void Map_BuildBrushData ()
{
	brush_t	*b, *next;

	if (g_brActiveBrushes.next == NULL)
		return;

	Sys_BeginWait();	// this could take a while

	for (b = g_brActiveBrushes.next; b != NULL && b != &g_brActiveBrushes; b = next)
	{
		next = b->next;
		Brush_Build(b);
		if (!b->brush_faces)
		{
			Brush_Free(b);
			Sys_Printf("MSG: Removed degenerate brush.\n");
		}
	}

	Sys_EndWait();
}

/*
==================
Map_FindClass
==================
*/
entity_t *Map_FindClass (char *cname)
{
	entity_t *ent;

	for (ent = g_entEntities.next; ent != &g_entEntities; ent = ent->next)
		if (!strcmp(cname, ValueForKey(ent, "classname")))
			return ent;

	return NULL;
}

/*
================
Map_Free
================
*/
void Map_Free ()
{
	if (g_brSelectedBrushes.next && (g_brSelectedBrushes.next != &g_brSelectedBrushes))
	    if (MessageBox(g_qeglobals.d_hwndMain, "Copy selection to new map?", "QuakeEd 3", MB_YESNO | MB_ICONQUESTION) == IDYES)
			Map_SaveBetween();

	Texture_ClearInuse();
	Pointfile_Clear();
	strcpy(g_szCurrentMap, "unnamed.map");
	Sys_SetTitle(g_szCurrentMap);
	g_qeglobals.d_nNumEntities = 0;

	if (!g_brActiveBrushes.next)
	{	// first map
		g_brActiveBrushes.prev = g_brActiveBrushes.next = &g_brActiveBrushes;
		g_brSelectedBrushes.prev = g_brSelectedBrushes.next = &g_brSelectedBrushes;
		g_brFilteredBrushes.prev = g_brFilteredBrushes.next = &g_brFilteredBrushes;
		g_entEntities.prev = g_entEntities.next = &g_entEntities;
	}
	else
	{
		while (g_brActiveBrushes.next != &g_brActiveBrushes)
			Brush_Free(g_brActiveBrushes.next);
		while (g_brSelectedBrushes.next != &g_brSelectedBrushes)
			Brush_Free(g_brSelectedBrushes.next);
		while (g_brFilteredBrushes.next != &g_brFilteredBrushes)
			Brush_Free(g_brFilteredBrushes.next);
		while (g_entEntities.next != &g_entEntities)
			Entity_Free(g_entEntities.next);
	}

	g_peWorldEntity = NULL;
}

/*
================
Map_LoadFile
================
*/
void Map_LoadFile (char *filename)
{
    char	   *buf;
	char		temp[1024];
	char	   *tempwad, wadkey[1024];
	entity_t   *ent;
	bool		bSnapCheck = false;

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
	Sys_Printf("CMD: Map_LoadFile: %s\n", temp );

	Map_Free();

	g_qeglobals.d_nParsedBrushes = 0;
	strcpy(g_szCurrentMap, filename);
    LoadFile(filename, (void **)&buf);

	StartTokenParsing(buf);
 
	g_qeglobals.d_nNumEntities = 0;

	while (1)
	{
		ent = Entity_Parse(false);
		if (!ent)
			break;
		if (!strcmp(ValueForKey(ent, "classname"), "worldspawn"))
		{
			if (g_peWorldEntity)
				Sys_Printf("WARNING: Multiple worldspawn.\n");
			g_peWorldEntity = ent;
		}
		else
		{
			// add the entity to the end of the entity list
			ent->next = &g_entEntities;
			ent->prev = g_entEntities.prev;
			g_entEntities.prev->next = ent;
			g_entEntities.prev = ent;
			g_qeglobals.d_nNumEntities++;
		}
	}

    free(buf);

	if (!g_peWorldEntity)
	{
		Sys_Printf("WARNING: No worldspawn in map.\n");
		Map_New();
		return;
	}

	if (!*ValueForKey(g_peWorldEntity, "wad"))
		Sys_Printf("WARNING: No \"wad\" key.\n");
	else
	{
		strcpy(wadkey, ValueForKey(g_peWorldEntity, "wad"));

		for (tempwad = strtok(wadkey, ";"); tempwad; tempwad = strtok(0, ";"))
			Texture_InitFromWad(tempwad);
	}

    Sys_Printf("--- LoadMapFile ---\n");
    Sys_Printf("%s\n", temp );
    Sys_Printf("%5i brushes\n", g_qeglobals.d_nParsedBrushes);
    Sys_Printf("%5i entities\n", g_qeglobals.d_nNumEntities);

	Map_RestoreBetween();

	Sys_Printf("CMD: Map_BuildBrushData\n");
    Map_BuildBrushData();

	// move the view to a start position
	ent = Map_FindClass("info_player_start");
	if (!ent)
		ent = Map_FindClass("info_player_deathmatch");

	g_qeglobals.d_camera.angles[PITCH] = 0;

	if (ent)
	{
		GetVectorForKey(ent, "origin", g_qeglobals.d_camera.origin);
		GetVectorForKey(ent, "origin", g_qeglobals.d_xyz[0].origin);
		g_qeglobals.d_camera.angles[YAW] = FloatForKey(ent, "angle");
	}
	else
	{
		g_qeglobals.d_camera.angles[YAW] = 0;
		VectorCopy(g_v3VecOrigin, g_qeglobals.d_camera.origin);
		VectorCopy(g_v3VecOrigin, g_qeglobals.d_xyz[0].origin);
	}

	Sys_UpdateWindows(W_ALL);

	Map_RegionOff();

	g_bModified = false;
	Sys_SetTitle(temp);

	Texture_ShowInuse();
	Texture_FlushUnused();

	if (bSnapCheck)	// sikk - turn Grid Snap back on if it was on before map load
		g_qeglobals.d_savedinfo.bNoClamp = false;

	Sys_EndWait();
}

/*
===========
Map_SaveFile
===========
*/
void Map_SaveFile (char *filename, bool use_region)
{
	entity_t   *e, *next;
	FILE	   *f;
	char        temp[1024];
	int			count;

	QE_ConvertDOSToUnixName(temp, filename);

	if (!use_region)
	{
		char backup[1024];

		// rename current to .bak
		strcpy(backup, filename);
		StripExtension(backup);
		strcat(backup, ".bak");
		_unlink(backup);
		rename(filename, backup);
	}

	Sys_Printf("CMD: Map_SaveFile: %s\n", filename);

	f = fopen(filename, "w");
	if (!f)
	{
		Sys_Printf("ERROR: Could not open %s\n", filename);
		return;
	}

	if (use_region)
		Map_AddRegionBrushes();

	// write world entity first
	Entity_Write(g_peWorldEntity, f, use_region);

	// then write all other ents
	count = 1;
	for (e = g_entEntities.next; e != &g_entEntities; e = next)
	{
		fprintf(f, "// entity %d\n", count);
		count++;
		next = e->next;
		if (e->brushes.onext == &e->brushes)
			Entity_Free(e);	// no brushes left, so remove it
		else
			Entity_Write(e, f, use_region);
	}

	fclose(f);

	if (use_region)
		Map_RemoveRegionBrushes();

	g_bModified = false;

	if (!strstr(temp, "autosave"))
		Sys_SetTitle(temp);

	if (!use_region)
	{
		MessageBeep(MB_ICONEXCLAMATION);
		/*
		time_t	timer;
		FILE   *f;
		char	logname[1024];

		strcpy(logname, filename);
		StripExtension(logname);
		strcat(logname, ".log");

		time(&timer);
//		f = fopen("c:/tstamps.log", "a");
		f = fopen(logname, "a");
		if (f)
		{
			fprintf(f, "%4i : %35s : %s", g_qeglobals.d_nWorkCount, filename, ctime(&timer));
			fclose(f);
			g_qeglobals.d_nWorkCount = 0;
		}
		fclose(f);
		*/
	}

	g_bMBCheck = false;	// sikk - Reset this to false
	g_nBrushNumCheck = -1;	// sikk - Reset this to -1

	Sys_Printf("MSG: Saved.\n");
	Sys_Status("Saved.", 0);
}

/*
===========
Map_New
===========
*/
void Map_New ()
{
	char buf[1024];

	Sys_Printf("CMD: Map_New\n");
	Map_Free();

	g_peWorldEntity = (entity_t*)qmalloc(sizeof(*g_peWorldEntity));
	g_peWorldEntity->brushes.onext = g_peWorldEntity->brushes.oprev = &g_peWorldEntity->brushes;
	SetKeyValue(g_peWorldEntity, "classname", "worldspawn");
// sikk---> Wad Loading
	//	SetKeyValue(g_peWorldEntity, "wad", g_szWadString);
	strcpy(buf, ValueForKey(g_qeglobals.d_entityProject, "defaultwads"));
	if (strlen(buf))
	{
		int i = 0;
		char *temp, tempwads[1024] = "";
		char *texpath = ValueForKey(g_qeglobals.d_entityProject, "texturepath");

		for (temp = strtok(buf, ";"); temp; temp = strtok(0, ";"), i++)
		{
			if (i)
				strncat(tempwads, ";", 1);
			strcat(tempwads, texpath);
			strcat(tempwads, temp);
		}
		SetKeyValue(g_peWorldEntity, "wad", tempwads);
	}
// <---sikk
	g_peWorldEntity->eclass = Eclass_ForName("worldspawn", true);
	
	g_qeglobals.d_camera.angles[YAW] = 0;
	VectorCopy(g_v3VecOrigin, g_qeglobals.d_camera.origin);
	g_qeglobals.d_camera.origin[2] = 48;
	VectorCopy(g_v3VecOrigin, g_qeglobals.d_xyz[0].origin);

	Map_RestoreBetween();

	Sys_UpdateWindows(W_ALL);
	g_bModified = false;
}


/*
===========================================================

  REGION

===========================================================
*/

bool		g_bRegionActive;
vec3_t		g_v3RegionMins = {-4096, -4096, -4096};
vec3_t		g_v3RegionMaxs = {4096, 4096, 4096};
brush_t	   *g_pbrRegionSides[4];

/*
===========
Map_AddRegionBrushes

a regioned map will have temp walls put up at the region boundary
===========
*/
void Map_AddRegionBrushes ()
{
	vec3_t		mins, maxs;
	int			i;
	texdef_t	texdef;

	if (!g_bRegionActive)
		return;

	memset(&texdef, 0, sizeof(texdef));
	strcpy(texdef.name, "REGION");

	mins[0] = g_v3RegionMins[0] - 16;
	maxs[0] = g_v3RegionMins[0] + 1;
	mins[1] = g_v3RegionMins[1] - 16;
	maxs[1] = g_v3RegionMaxs[1] + 16;
	mins[2] = -2048;
	maxs[2] = 2048;
	g_pbrRegionSides[0] = Brush_Create(mins, maxs, &texdef);

	mins[0] = g_v3RegionMaxs[0] - 1;
	maxs[0] = g_v3RegionMaxs[0] + 16;
	g_pbrRegionSides[1] = Brush_Create(mins, maxs, &texdef);

	mins[0] = g_v3RegionMins[0] - 16;
	maxs[0] = g_v3RegionMaxs[0] + 16;
	mins[1] = g_v3RegionMins[1] - 16;
	maxs[1] = g_v3RegionMins[1] + 1;
	g_pbrRegionSides[2] = Brush_Create(mins, maxs, &texdef);

	mins[1] = g_v3RegionMaxs[1] - 1;
	maxs[1] = g_v3RegionMaxs[1] + 16;
	g_pbrRegionSides[3] = Brush_Create(mins, maxs, &texdef);

	for (i = 0; i < 4; i++)
	{
		Brush_AddToList(g_pbrRegionSides[i], &g_brSelectedBrushes);
		Entity_LinkBrush(g_peWorldEntity, g_pbrRegionSides[i]);
		Brush_Build(g_pbrRegionSides[i]);
	}
}

/*
==================
Map_RemoveRegionBrushes
==================
*/
void Map_RemoveRegionBrushes ()
{
	int	i;

	if (!g_bRegionActive)
		return;
	for (i = 0; i < 4; i++)
		Brush_Free(g_pbrRegionSides[i]);
}

/*
==================
Map_IsBrushFiltered
==================
*/
bool Map_IsBrushFiltered (brush_t *b)
{
	int	i;

	for (i = 0; i < 3; i++)
	{
		if (b->mins[i] > g_v3RegionMaxs[i])
			return true;
		if (b->maxs[i] < g_v3RegionMins[i])
			return true;
	}
	return false;
}

/*
===========
Map_RegionOff

Other filtering options may still be on
===========
*/
void Map_RegionOff ()
{
	int			i;
	brush_t	   *b, *next;

	g_bRegionActive = false;
	for (i = 0; i < 3; i++)
	{
		g_v3RegionMaxs[i] =  g_qeglobals.d_savedinfo.nMapSize * 0.5;//4096;	// sikk - Map Size
		g_v3RegionMins[i] = -g_qeglobals.d_savedinfo.nMapSize * 0.5;//-4096;	// sikk - Map Size
	}
	
	for (b = g_brFilteredBrushes.next; b != &g_brFilteredBrushes; b = next)
	{
		next = b->next;
		if (Map_IsBrushFiltered(b))
			continue;		// still filtered
		Brush_RemoveFromList(b);
		Brush_AddToList(b, &g_brActiveBrushes);
	}

	Sys_UpdateWindows(W_ALL);
}

/*
==================
Map_ApplyRegion
==================
*/
void Map_ApplyRegion ()
{
	brush_t	*b, *next;

	g_bRegionActive = true;
	for (b = g_brActiveBrushes.next; b != &g_brActiveBrushes; b = next)
	{
		next = b->next;
		if (!Map_IsBrushFiltered(b))
			continue;		// still filtered
		Brush_RemoveFromList(b);
		Brush_AddToList(b, &g_brFilteredBrushes);
	}

	Sys_UpdateWindows(W_ALL);
}

/*
========================
Map_RegionSelectedBrushes
========================
*/
void Map_RegionSelectedBrushes ()
{
	Map_RegionOff();

	if (!Select_HasBrushes())
		return;

	g_bRegionActive = true;
	Select_GetBounds(g_v3RegionMins, g_v3RegionMaxs);

	// move the entire active_brushes list to filtered_brushes
	g_brFilteredBrushes.next = g_brActiveBrushes.next;
	g_brFilteredBrushes.prev = g_brActiveBrushes.prev;
	g_brFilteredBrushes.next->prev = &g_brFilteredBrushes;
	g_brFilteredBrushes.prev->next = &g_brFilteredBrushes;

	// move the entire g_brSelectedBrushes list to g_brActiveBrushes
	g_brActiveBrushes.next = g_brSelectedBrushes.next;
	g_brActiveBrushes.prev = g_brSelectedBrushes.prev;
	g_brActiveBrushes.next->prev = &g_brActiveBrushes;
	g_brActiveBrushes.prev->next = &g_brActiveBrushes;

	// clear g_brSelectedBrushes
	g_brSelectedBrushes.next = g_brSelectedBrushes.prev = &g_brSelectedBrushes;

	Sys_UpdateWindows(W_ALL);
}

/*
===========
Map_RegionXY
===========
*/
void Map_RegionXY ()
{
	Map_RegionOff();

	float w, h;

	w = 0.5 * g_qeglobals.d_xyz[0].width / g_qeglobals.d_xyz[0].scale;
	h = 0.5 * g_qeglobals.d_xyz[0].height / g_qeglobals.d_xyz[0].scale;

// sikk---> Proper Regioning for XZ & YZ Views

	if (g_qeglobals.d_xyz[0].dViewType == XY)
	{
		g_v3RegionMins[0] = g_qeglobals.d_xyz[0].origin[0] - w;
		g_v3RegionMaxs[0] = g_qeglobals.d_xyz[0].origin[0] + w;
		g_v3RegionMins[1] = g_qeglobals.d_xyz[0].origin[1] - h;
		g_v3RegionMaxs[1] = g_qeglobals.d_xyz[0].origin[1] + h;
		g_v3RegionMins[2] = -g_qeglobals.d_savedinfo.nMapSize * 0.5;//-4096;	// sikk - Map Size
		g_v3RegionMaxs[2] = g_qeglobals.d_savedinfo.nMapSize * 0.5;//4096;	// sikk - Map Size
	}
	if (g_qeglobals.d_xyz[0].dViewType == XZ)
	{
		g_v3RegionMins[0] = g_qeglobals.d_xyz[0].origin[0] - w;
		g_v3RegionMaxs[0] = g_qeglobals.d_xyz[0].origin[0] + w;
		g_v3RegionMins[1] = -g_qeglobals.d_savedinfo.nMapSize * 0.5;//-4096;	// sikk - Map Size
		g_v3RegionMaxs[1] = g_qeglobals.d_savedinfo.nMapSize * 0.5;//4096;	// sikk - Map Size
		g_v3RegionMins[2] = g_qeglobals.d_xyz[0].origin[2] - h;
		g_v3RegionMaxs[2] = g_qeglobals.d_xyz[0].origin[2] + h;
	}
	if (g_qeglobals.d_xyz[0].dViewType == YZ)
	{
		g_v3RegionMins[0] = -g_qeglobals.d_savedinfo.nMapSize * 0.5;//-4096;	// sikk - Map Size
		g_v3RegionMaxs[0] = g_qeglobals.d_savedinfo.nMapSize * 0.5;//4096;	// sikk - Map Size
		g_v3RegionMins[1] = g_qeglobals.d_xyz[0].origin[1] - w;
		g_v3RegionMaxs[1] = g_qeglobals.d_xyz[0].origin[1] + w;
		g_v3RegionMins[2] = g_qeglobals.d_xyz[0].origin[2] - h;
		g_v3RegionMaxs[2] = g_qeglobals.d_xyz[0].origin[2] + h;
	}
// <---sikk
	Map_ApplyRegion();
}

/*
===========
Map_RegionXZ
===========
*/
void Map_RegionXZ ()
{
	Map_RegionOff();
	g_v3RegionMins[0] = g_qeglobals.d_xyz[2].origin[0] - 0.5 * g_qeglobals.d_xyz[2].width / g_qeglobals.d_xyz[2].scale;
	g_v3RegionMaxs[0] = g_qeglobals.d_xyz[2].origin[0] + 0.5 * g_qeglobals.d_xyz[2].width / g_qeglobals.d_xyz[2].scale;
	g_v3RegionMins[1] = -g_qeglobals.d_savedinfo.nMapSize * 0.5;//-4096;	// sikk - Map Size
	g_v3RegionMaxs[1] = g_qeglobals.d_savedinfo.nMapSize * 0.5;//4096;	// sikk - Map Size
	g_v3RegionMins[2] = g_qeglobals.d_xyz[2].origin[2] - 0.5 * g_qeglobals.d_xyz[2].height / g_qeglobals.d_xyz[2].scale;
	g_v3RegionMaxs[2] = g_qeglobals.d_xyz[2].origin[2] + 0.5 * g_qeglobals.d_xyz[2].height / g_qeglobals.d_xyz[2].scale;
	Map_ApplyRegion();
}

/*
===========
Map_RegionYZ
===========
*/
void Map_RegionYZ ()
{
	Map_RegionOff();
	g_v3RegionMins[0] = -g_qeglobals.d_savedinfo.nMapSize * 0.5;//-4096;	// sikk - Map Size
	g_v3RegionMaxs[0] = g_qeglobals.d_savedinfo.nMapSize * 0.5;//4096;	// sikk - Map Size
	g_v3RegionMins[1] = g_qeglobals.d_xyz[1].origin[1] - 0.5 * g_qeglobals.d_xyz[1].width / g_qeglobals.d_xyz[1].scale;
	g_v3RegionMaxs[1] = g_qeglobals.d_xyz[1].origin[1] + 0.5 * g_qeglobals.d_xyz[1].width / g_qeglobals.d_xyz[1].scale;
	g_v3RegionMins[2] = g_qeglobals.d_xyz[1].origin[2] - 0.5 * g_qeglobals.d_xyz[1].height / g_qeglobals.d_xyz[1].scale;
	g_v3RegionMaxs[2] = g_qeglobals.d_xyz[1].origin[2] + 0.5 * g_qeglobals.d_xyz[1].height / g_qeglobals.d_xyz[1].scale;
	Map_ApplyRegion();
}

/*
===========
Map_RegionTallBrush
===========
*/
void Map_RegionTallBrush ()
{
	brush_t	*b;

	if (!QE_SingleBrush())
		return;

	b = g_brSelectedBrushes.next;

	Map_RegionOff();

	VectorCopy(b->mins, g_v3RegionMins);
	VectorCopy(b->maxs, g_v3RegionMaxs);
	g_v3RegionMins[2] = -g_qeglobals.d_savedinfo.nMapSize * 0.5;//-4096;	// sikk - Map Size
	g_v3RegionMaxs[2] = g_qeglobals.d_savedinfo.nMapSize * 0.5;//4096;	// sikk - Map Size

	Select_Delete();
	Map_ApplyRegion();
}

/*
===========
Map_RegionBrush
===========
*/
void Map_RegionBrush ()
{
	brush_t	*b;

	if (!QE_SingleBrush())
		return;

	b = g_brSelectedBrushes.next;

	Map_RegionOff();

	VectorCopy(b->mins, g_v3RegionMins);
	VectorCopy(b->maxs, g_v3RegionMaxs);

	Select_Delete();
	Map_ApplyRegion();
}

//=====================================================================

// sikk---> Import/Export Selection (Map/Prefab)
/*
================
Map_ImportFile
================
*/
void Map_ImportFile (char *filename, bool bCheck)
{
	int			i, nCount = 0;
	char	   *buf;
	char		temp[1024];
	char	   *tempwad, wadkey[1024];
	brush_t	   *b = NULL, *bNext;
	brush_t    *brArray[MAX_MAP_BRUSHES];
	entity_t   *ent;
	bool		bSnapCheck = false;	// sikk

	Sys_BeginWait();

// sikk---> make sure Grid Snap is off to insure complex brushes remain intact
	if (!g_qeglobals.d_savedinfo.bNoClamp)
	{
		g_qeglobals.d_savedinfo.bNoClamp = true;
		bSnapCheck = true;
	}
// <---sikk

	Select_DeselectAll(true);

	Undo_Start("Import File");	// sikk - Undo/Redo

	InspWnd_SetMode(W_CONSOLE);

	QE_ConvertDOSToUnixName(temp, filename);
	Sys_Printf("CMD: Map_ImportFile: %s\n", temp);

	g_qeglobals.d_nParsedBrushes = 0;

    LoadFile(filename, (void **)&buf);

	StartTokenParsing(buf);
 
	g_qeglobals.d_nNumEntities = 0;

	while (1)
	{
		ent = Entity_Parse(false);
		if (!ent)
			break;
		
		//end entity for undo
		Undo_EndEntity(ent);	// sikk - Undo/Redo
		//end brushes for undo
		for(b = ent->brushes.onext; b && b != &ent->brushes; b = b->onext)
			Undo_EndBrush(b);	// sikk - Undo/Redo

		if (!strcmp(ValueForKey(ent, "classname"), "worldspawn"))
		{
			// world brushes need to be added to the current world entity
			b = ent->brushes.onext;
			while (b && b != &ent->brushes)
			{
				bNext = b->onext;
				Brush_RemoveFromList(b);
				Brush_AddToList(b, &g_brActiveBrushes);
				Entity_UnlinkBrush(b);
				Entity_LinkBrush(g_peWorldEntity, b);
				Brush_Build(b);
				brArray[nCount] = b;
				nCount++;
				b = bNext;
			}
		}
		else
		{
			// add the entity to the end of the entity list
			ent->next = &g_entEntities;
			ent->prev = g_entEntities.prev;
			g_entEntities.prev->next = ent;
			g_entEntities.prev = ent;
			g_qeglobals.d_nNumEntities++;

			for (b = ent->brushes.onext; b != &ent->brushes; b = b->onext)
			{
				brArray[nCount] = b;
				nCount++;
			}
		}
	}

	for (i = 0; i < nCount; i++)
	{
		Brush_Build(brArray[i]);
		Select_HandleBrush(brArray[i], true);
	}

    free(buf);

	if (!*ValueForKey(g_peWorldEntity, "wad"))
		Sys_Printf("WARNING: No \"wad\" key.\n");
	else
	{
		strcpy(wadkey, ValueForKey(g_peWorldEntity, "wad"));
		for (tempwad = strtok(wadkey, ";"); tempwad; tempwad = strtok(0, ";"))
			Texture_InitFromWad(tempwad);
	}

	if (bCheck)
		Sys_Printf("--- ImportMapFile ---\n");
	else
		Sys_Printf("--- ImportPrefab ---\n");

    Sys_Printf("%s\n", temp );

    Sys_Printf("%5i brushes\n", g_qeglobals.d_nParsedBrushes);
    Sys_Printf("%5i entities\n", g_qeglobals.d_nNumEntities);

	Sys_Printf("CMD: Map_BuildBrushData\n");
    Map_BuildBrushData();

	Undo_End();	// sikk - Undo/Redo

	Sys_UpdateWindows(W_ALL);

	g_bModified = true;

//	Texture_ShowInuse();
//	Texture_FlushUnused();

	if (bSnapCheck)	// sikk - turn Grid Snap back on if it was on before map load
		g_qeglobals.d_savedinfo.bNoClamp = false;

	Sys_EndWait();
}

/*
================
Map_ExportFile
================
*/
void Map_ExportFile (char *filename, bool bCheck)
{
	entity_t   *e, *next;
	FILE	   *f;
	char		temp[1024];
	int			nCount;

	QE_ConvertDOSToUnixName(temp, filename);
	f = fopen(filename, "w");

	if (!f)
	{
		Sys_Printf("ERROR: Could not open %s\n", filename);
		return;
	}

	// write world entity first
	Entity_WriteSelected(g_peWorldEntity, f);

	// then write all other ents
	nCount = 1;
	for (e = g_entEntities.next; e != &g_entEntities; e = next)
	{
  		fprintf(f, "// entity %d\n", nCount);
   		nCount++;
 		Entity_WriteSelected(e, f);
		next = e->next;
	}
	fclose(f);

	if (bCheck)
		Sys_Printf("MSG: Selection Exported to map file: \"%s\"\n", filename);
	else
		Sys_Printf("MSG: Selection Exported to prefab file: \"%s\"\n", filename);
}
// <---sikk
