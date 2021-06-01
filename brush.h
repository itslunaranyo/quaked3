#ifndef __BRUSH_H__
#define __BRUSH_H__

//==============================
//	brush.h
//==============================

#include "SlabAllocator.h"

class Entity;
class Face;

class Brush : public SlabAllocator<Brush>
{
public:
	Brush();
	~Brush();

	Entity	*owner;
	Face	*faces;
	vec3	mins, maxs;

	unsigned int showFlags;	// lunaran: hiddenBrush now rolled into a whole set of bitflags

private:
	friend class _brush_ent_accessor;
	Brush	*prev, *next;	// links in active/selected
	Brush	*oprev, *onext;	// links in entity

public:
	// membership and linking
	inline Brush* Next() const { return next; }		// next brush in its list
	inline Brush* Prev() const { return prev; }		// previous brush in its list
	inline Brush* ENext() const { return onext; }	// next brush in its owning entity
	inline Brush* EPrev() const { return oprev; }	// previous brush in its owning entity

	void	RemoveFromList();
	bool	IsLinked();
	void	AddToList(Brush &list);
	void	AddToListTail(Brush &list);
	void	MergeListIntoList(Brush &dest, bool tail = false);	// donates all brushes to dest, leaving this brush empty
	static void	FreeList(Brush *pList);

	//====================================

	int		NumFaces() const;
	int		MemorySize() const;	// sikk - Undo/Redo
	bool	IsConvex() const;	// sikk - Vertex Editing Splits Face
	bool	IsFiltered() const;
	bool	IsHidden() const { return !!(showFlags & BFL_HIDDEN); }

	static Brush *Create (const vec3 inMins, const vec3 inMaxs, TexDef *texdef);
	void	Recreate(const vec3 inMins, const vec3 inMaxs, TexDef *inTexDef);
	Brush	*Clone() const;
	//Brush	*FullClone() const;	// sikk - Undo/Redo
	void	ClearFaces();
	//void	Move(const vec3 move, const bool texturelock);
	void	Transform(const mat4 mat, const bool textureLock);
	void	RefreshFlags();

	bool	FullBuild();	// lunaran: full refreshes texture pointers/flags, regular is just windings
	bool	Build();		// lunaran: now returns false if brush disappeared when built

	void	MakeFacePlanes();
	void	SnapPlanePoints();
	void	RemoveEmptyFaces();

	void	FitTexture(int nHeight, int nWidth);
	void	SetTexture(TexDef *texdef, unsigned flags);
	void	RefreshTexdefs();

	Face	*RayTest(const vec3 origin, const vec3 dir, float *dist);
	bool	PointTest(const vec3 origin);
	vec3	Center() { return (maxs + mins) * 0.5f; }


	static Brush *Parse();
	void	Write(std::ostream &out);

	void	Draw ();
	void	DrawXY (int nViewType);
	void	DrawFacingAngle ();
	void	DrawEntityName ();
	void	DrawLight ();

};

class _brush_ent_accessor
{
	friend Entity;
	static void LinkToEntity(Brush* b, Entity* e);
	static void LinkToEntityTail(Brush* b, Entity* e);
	static void UnlinkFromEntity(Brush* b, bool preserveOwner = false);
};

#endif