#ifndef __BRUSH_H__
#define __BRUSH_H__

//==============================
//	brush.h
//==============================

// the normals on planes point OUT of the brush
#define	MAXPOINTS	16
#define	MAX_FACES	16
#define MAX_POINTS_ON_WINDING	64
#define MAX_MOVE_FACES	64

//========================================================================

typedef struct
{
    vec3_t	normal;
    double	dist;
	//int		type;	// lunaran - unused
} plane_t;

typedef struct
{
	int		numpoints;
	int		maxpoints;
	float 	points[8][5];			// variable sized
} winding_t;

typedef struct face_s
{
	struct face_s	*next;
	struct face_s	*original;	// sikk - Vertex Editing Splits Face: used for vertex movement
	struct brush_s	*owner;		// sikk - brush of selected face
	vec3_t		planepts[3];
    texdef_t	texdef;
    plane_t		plane;
	winding_t  *face_winding;
	vec3_t		d_color;
	qtexture_t *d_texture;
} face_t;

typedef struct
{
	int		p1, p2;
	face_t *f1, *f2;
} pedge_t;

typedef struct brush_s
{
	struct brush_s	*prev, *next;	// links in active/selected
	struct brush_s	*oprev, *onext;	// links in entity
	struct entity_s	*owner;
	face_t *brush_faces;
	vec3_t	mins, maxs;
	bool	hiddenBrush;

// sikk---> Undo/Redo
	int undoId;			// undo ID		
	int redoId;			// redo ID
	int ownerId;		// entityId of the owner entity for undo
// <---sikk
} brush_t;

class Bush
{
	Bush();
	~Bush();

	Bush	*prev, *next;	// links in active/selected
	Bush	*oprev, *onext;	// links in entity
	struct entity_s	*owner;
	face_t *brush_faces;
	vec3_t	mins, maxs;
	bool	hiddenBrush;

	// sikk---> Undo/Redo
	int undoId;			// undo ID		
	int redoId;			// redo ID
	int ownerId;		// entityId of the owner entity for undo
	// <---sikk
};

//========================================================================

extern bool g_bMBCheck;
extern int	g_nBrushNumCheck;
			 
//========================================================================

void		Brush_AddToList (brush_t *b, brush_t *list);
void		Brush_RemoveFromList (brush_t *b);
void		Brush_MergeListIntoList(brush_t *src, brush_t *dest);
void		Brush_FreeList (brush_t *pList);
void		Brush_CopyList (brush_t *pFrom, brush_t *pTo);

brush_t	   *Brush_Alloc ();
int			Brush_NumFaces(brush_t *b);
void		Brush_Build (brush_t *b);
void		Brush_BuildWindings (brush_t *b);
void		Brush_CheckTexdef (brush_t *b, face_t *f, char *pszName);	// sikk - Check Texdef - temp fix for Multiple Entity Undo Bug
brush_t	   *Brush_Clone (brush_t *b);
brush_t	   *Brush_Create (vec3_t mins, vec3_t maxs, texdef_t *texdef);
void		Brush_Draw (brush_t *b);
void		Brush_DrawFacingAngle (brush_t *b);
void		Brush_DrawXY (brush_t *b, int nViewType);
void		Brush_FitTexture (brush_t *b, int nHeight, int nWidth);
void		Brush_Free (brush_t *b);
brush_t	   *Brush_FullClone (brush_t *b);	// sikk - Undo/Redo
void		Brush_MakeFacePlanes (brush_t *b);
void		Brush_MakeSided (int sides);
void		Brush_MakeSidedCone (int sides);	// sikk - Brush Primitves
void		Brush_MakeSidedSphere (int sides);	// sikk - Brush Primitves
//void		Brush_MakeSidedTorus (int sides);	// sikk - Brush Primitves
int			Brush_MemorySize (brush_t *b);	// sikk - Undo/Redo
void		Brush_Move (brush_t *b, vec3_t move);
brush_t    *Brush_Parse ();
face_t     *Brush_Ray (vec3_t origin, vec3_t dir, brush_t *b, float *dist);
void		Brush_RemoveEmptyFaces (brush_t *b);
void		Brush_SelectFaceForDragging (brush_t *b, face_t *f, bool shear);
void		Brush_SetTexture (brush_t *b, texdef_t *texdef, int nSkipFlags);
void		Brush_SideSelect (brush_t *b, vec3_t origin, vec3_t dir, bool shear);
void		Brush_SnapPlanepts (brush_t *b);
void		Brush_Write (brush_t *b, FILE *f);
winding_t  *Brush_MakeFaceWinding (brush_t *b, face_t *face);

// sikk---> Vertex Editing Splits Face
bool		Brush_Convex (brush_t *b);
bool		Brush_MoveVertex (brush_t *b, vec3_t vertex, vec3_t delta, vec3_t end);
void		Brush_ResetFaceOriginals (brush_t *b);
// <---sikk

void		Face_MakePlane (face_t *f);
void		Face_SetColor (brush_t *b, face_t *f);

face_t	   *Face_Alloc ();
face_t	   *Face_Clone (face_t *f);
face_t	   *Face_FullClone (face_t *f);	// sikk - Undo/Redo
void		Face_Draw (face_t *face);
void		Face_FitTexture(face_t *face, float fHeight, float fWidth);
//void		Face_FitTexture (face_t *face, int nHeight, int nWidth);
void		Face_Free (face_t *f);
int			Face_MemorySize (face_t *f);	// sikk - Undo/Redo
void		Face_MoveTexture (face_t *f, vec3_t delta);
void		Face_SetTexture (face_t *f, texdef_t *texdef, int nSkipFlags);

int			AddPlanept (float *f);
float		SetShadeForPlane (plane_t *p);
winding_t  *BasePolyForPlane (plane_t *p);
bool		ClipLineToFace (vec3_t p1, vec3_t p2, face_t *f);
void		FacingVectors (brush_t *b, vec3_t forward, vec3_t right, vec3_t up);

void		TextureAxisFromPlane (plane_t *pln, vec3_t xv, vec3_t yv);
void		BeginTexturingFace (brush_t *b, face_t *f, qtexture_t *q);
void	   _EmitTextureCoordinates (vec3_t v, qtexture_t *q);
void		EmitTextureCoordinates (float *xyzst, qtexture_t *q, face_t *f);

void		DrawBrushEntityName (brush_t *b);
void		DrawLight (brush_t *b);

#endif