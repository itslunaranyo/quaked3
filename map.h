#ifndef __MAP_H__
#define __MAP_H__
//==============================
//	map.h
//==============================

// the state of the current world that all views are displaying

extern char			g_szCurrentMap[MAX_PATH];

// head/tail of doubly linked lists
extern Brush		g_brActiveBrushes;		// brushes currently being displayed
extern Brush		g_brFilteredBrushes;	// brushes that have been filtered or regioned

extern Entity		g_entEntities;
extern Entity	   *g_peWorldEntity;		// the world entity is NOT included in the entities chain

extern Brush		g_brCopiedBrushes;		// sikk - For Cut/Copy/Paste
extern Entity		g_entCopiedEntities;	// sikk - For Cut/Copy/Paste

extern BOOL			g_bModified;			// for quit confirmations

extern vec3_t		g_v3RegionMins, g_v3RegionMaxs;
extern bool			g_bRegionActive;

//========================================================================

void Map_SaveBetween ();
void Map_RestoreBetween ();

void Map_LoadFile (char *filename);
void Map_SaveFile (char *filename, bool use_region);
void Map_ImportFile (char *filename, bool bCheck);	// sikk - Import File (Map/Prefab)
void Map_ExportFile (char *filename, bool bCheck);	// sikk - Export Selection (Map/Prefab)
void Map_New ();
void Map_BuildBrushData ();
void Map_Free ();

void Map_RegionOff ();
void Map_RegionXY ();
void Map_RegionXZ ();	// sikk - Multiple Orthographic Views
void Map_RegionYZ ();	// sikk - Multiple Orthographic Views
void Map_RegionTallBrush ();
void Map_RegionBrush ();
void Map_RegionSelectedBrushes ();

void Map_ApplyRegion ();
void Map_AddRegionBrushes ();
void Map_RemoveRegionBrushes ();
bool Map_IsBrushFiltered (Brush *b);

Entity *Map_FindClass (char *cname);



#endif
