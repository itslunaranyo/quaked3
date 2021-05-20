//==============================
//	map.h
//==============================

#ifndef __MAP_H__
#define __MAP_H__

#include "Brush.h"
#include "Entity.h"
#include "TargetGraph.h"

// the state of the current world that all views are displaying

class Map
{
public:
	Map();
	~Map() {};

	// PER MAP
	char		name[MAX_PATH];
	bool		hasFilename;	// has this map ever been saved?
	int			numBrushes, numEntities, numTextures;
	Entity		*world;			// the world entity is NOT included in the entities chain
	Entity		entities;

	// head/tail of doubly linked lists
	Brush		brActive;		// brushes currently being displayed
	Brush		brRegioned;		// brushes that are outside the region

	vec3		regionMins, regionMaxs;
	bool		regionActive;

	// WHOLE SCENE
	clock_t		autosaveTime; // why is this here
	TargetGraph targetGraph;


	void	New();
	void	BuildBrushData(Brush &blist);
	void	BuildBrushData();

	void	SanityCheck();
	void	LoadFromFile(const char* filename);
	void	SaveToFile(const char* filename, bool use_region);
	void	ImportFromFile(const char* filename);
	void	ExportToFile(const char* filename);
	void	Cut();
	void	Copy();
	void	Paste();

	void	RegionOff();
	void	RegionXYZ(int gwin);
	void	RegionXY();
	void	RegionXZ();	// sikk - Multiple Orthographic Views
	void	RegionYZ();	// sikk - Multiple Orthographic Views
	void	RegionTallBrush();
	void	RegionBrush();
	void	RegionSelectedBrushes();
	void	RegionApply();

	Entity* FindEntity(char *pszKey, char *pszValue);
	Entity* FindEntity(char *pszKey, int iValue);

	bool	IsBrushFiltered(Brush *b);
	void	Read(const char* data, Brush& blist, Entity& elist);
private:
	void	RegionAdd();
	void	RegionRemove();

	void	Free();
	void	SaveBetween(qeBuffer& buf);
	bool	LoadBetween(qeBuffer& buf);

	bool	ParseBufferMerge(const char *data);
	bool	LoadFromFile_Parse(const char *data);
	void	WriteSelected(std::ostream &out);
	void	WriteAll(std::ostream &out, bool use_region);
};

extern Map	g_map;


#endif
