#ifndef __BRUSH_H__
#define __BRUSH_H__

//==============================
//	brush.h
//==============================

class Brush
{
public:
	Brush();
	~Brush();

	Brush	*prev, *next;	// links in active/selected
	Brush	*oprev, *onext;	// links in entity
	struct entity_s	*owner;
	Face *brush_faces;
	vec3_t	mins, maxs;
	bool	hiddenBrush;

	// sikk---> Undo/Redo
	int undoId;			// undo ID		
	int redoId;			// redo ID
	int ownerId;		// entityId of the owner entity for undo
	// <---sikk

	//====================================

	int		NumFaces() const;
	int		MemorySize() const;	// sikk - Undo/Redo
	bool	IsConvex() const;	// sikk - Vertex Editing Splits Face
	bool	IsFiltered() const;

	static Brush *Create (vec3_t mins, vec3_t maxs, texdef_t *texdef);
	Brush	*Clone() const;
	Brush	*FullClone() const;	// sikk - Undo/Redo
	void	Move(vec3_t move);

	void	Build();
	void	BuildWindings();
	winding_t *MakeFaceWinding(Face *face);
	void	MakeFacePlanes();
	void	SnapPlanePoints();
	void	RemoveEmptyFaces();

	void	CheckTexdef(Face *f, char *pszName);	// sikk - Check Texdef - temp fix for Multiple Entity Undo Bug
	void	FitTexture(int nHeight, int nWidth);
	void	SetTexture(texdef_t *texdef, int nSkipFlags);

	Face	*RayTest(vec3_t origin, vec3_t dir, float *dist);
	void	SelectFaceForDragging(Face *f, bool shear);
	void	SideSelect(vec3_t origin, vec3_t dir, bool shear);

	// sikk---> Vertex Editing Splits Face
	bool	MoveVertex(vec3_t vertex, vec3_t delta, vec3_t end);
	void	ResetFaceOriginals();
	// <---sikk

	// TODO: make not static; these all start with a single brush anyway
	static void	MakeSided (int sides);
	static void	MakeSidedCone (int sides);	// sikk - Brush Primitves
	static void	MakeSidedSphere (int sides);	// sikk - Brush Primitves

	// TODO: promote brushlist to its own container class
	void	AddToList(Brush *list);
	void	RemoveFromList();
	static void	MergeListIntoList(Brush *src, Brush *dest);
	static void	FreeList(Brush *pList);
	static void	CopyList(Brush *pFrom, Brush *pTo);

	static	Brush *Parse();
	void	Write(FILE *f);

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



void		FacingVectors (Brush *b, vec3_t forward, vec3_t right, vec3_t up);

#endif