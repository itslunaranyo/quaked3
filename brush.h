#ifndef __BRUSH_H__
#define __BRUSH_H__

//==============================
//	brush.h
//==============================

#include "SlabAllocator.h"

class Entity;
class Brush : public SlabAllocator<Brush>
{
public:
	Brush();
	~Brush();

	Brush	*prev, *next;	// links in active/selected
	Brush	*oprev, *onext;	// links in entity
	Entity	*owner;
	Face	*faces;
	vec3	mins, maxs;

	unsigned int showFlags;	// lunaran: hiddenBrush now rolled into a whole set of bitflags

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
	void	Move(const vec3 move, const bool texturelock);
	void	Transform(const glm::mat4 mat, const bool textureLock);
	void	RefreshFlags();

	bool	FullBuild();

	bool	Build();	// lunaran: now returns false if brush disappeared when built
	void	MakeFacePlanes();
	void	SnapPlanePoints();
	void	RemoveEmptyFaces();

	void	CheckTexdef(Face *f, char *pszName);	// sikk - Check Texdef - temp fix for Multiple Entity Undo Bug
	void	FitTexture(int nHeight, int nWidth);
	void	SetTexture(TexDef *texdef, unsigned flags);
	void	RefreshTexdefs();

	Face	*RayTest(const vec3 origin, const vec3 dir, float *dist);
	void	SelectFaceForDragging(Face *f, bool shear);
	void	SideSelect(const vec3 origin, const vec3 dir, bool shear = false);
	bool	PointTest(const vec3 origin);
	vec3	Center() { return (maxs + mins) * 0.5f; }

	// sikk---> Vertex Editing Splits Face
	bool	MoveVertex(const vec3 vertex, const vec3 delta, vec3 &end);
	void	ResetFaceOriginals();
	// <---sikk

	// TODO: make not static; these all start with a single brush anyway
	static void MakeCzgCylinder(int degree);
	static void	MakeSided (int sides);
	static void	MakeSidedCone (int sides);	// sikk - Brush Primitves
	static void	MakeSidedSphere (int sides);	// sikk - Brush Primitves

	// TODO: promote brushlist to its own container class
	void	RemoveFromList();
	void	CloseLinks();
	void	AddToList(Brush *list, bool tail = false);
	void	MergeListIntoList(Brush *dest, bool tail = false);
	static void	FreeList(Brush *pList);
	static void	CopyList(Brush *pFrom, Brush *pTo);

	static Brush *Parse();
	void	Write(std::ostream &out);
	//void	Write(FILE *f);

	void	Draw ();
	void	DrawXY (int nViewType);
	void	DrawFacingAngle ();
	void	DrawEntityName ();
	void	DrawLight ();
};

//========================================================================

extern bool g_bMBCheck;
extern int	g_nBrushNumCheck;
			 
//========================================================================



void		FacingVectors (const Brush *b, vec3 &forward, vec3 &right, vec3 &up);

#endif