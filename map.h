//==============================
//	map.h
//==============================

#ifndef __MAP_H__
#define __MAP_H__

#include "Brush.h"
#include "Entity.h"
#include "TargetGraph.h"
#include <time.h>

// the state of the current world that all views are displaying

class Map
{
public:
	Map();
	~Map() {};

	// PER MAP
	std::string	name;
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
	void	Save();	// does not change name
	void	Save(const std::string& filename);	// changes map name to filename
	void	SaveSelection(const std::string& filename);
	void	Load(const std::string& filename);

	void	Free();
	Entity* CreateWorldspawn();
	void	BuildBrushData(Brush &blist);
	void	BuildBrushData();

	void	SanityCheck();
	bool	LoadFromFile(const std::string& filename, Entity& elist, Brush& blist);
	void	ImportFromFile(const std::string& filename);
	void	SaveBetween(std::string& buf);
	bool	LoadBetween(const std::string& buf);

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
private:
	void	RegionAdd();
	void	RegionRemove();

};

extern Map	g_map;


#endif
