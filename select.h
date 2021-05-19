//==============================
//	select.h
//==============================

#define	DIST_START	999999

#define	SF_SELECTED_ONLY	1
#define	SF_ENTITIES_FIRST	2
#define	SF_SINGLEFACE		4
#define SF_CYCLE			8	// sikk - Single Selection Cycle (Shift+Alt+LMB)

//========================================================================

typedef enum
{
	sel_brush,
	sel_vertex,
	sel_edge
//	sel_sticky_brush,
//	sel_face,
} select_t;

typedef struct
{
	brush_t	   *brush;
	face_t	   *face;
	float		dist;
	bool		selected;
} trace_t;

//========================================================================

int g_nSelFaceCount, g_nSelFacePos;	// sikk - Multiple Face Selection
vec3_t g_v3RotateOrigin;	// sikk - Free Rotate

//========================================================================

trace_t Test_Ray (vec3_t origin, vec3_t dir, int flags);

bool Select_HasBrushes();
bool Select_HasFaces();
bool Select_IsEmpty();

void Select_GetBounds (vec3_t mins, vec3_t maxs);
void Select_GetTrueMid (vec3_t mid);
void Select_GetMid (vec3_t mid);
void Select_ApplyMatrix ();
void Select_Brush (brush_t *b, bool bComplete);
void Select_Ray (vec3_t origin, vec3_t dir, int flags);
void Select_Delete ();
void Select_Deselect (bool bDeselectFaces);
// sikk---> Multiple Face Selection
bool Select_IsFaceSelected (face_t *face);
void Select_DeselectFaces ();
// <---sikk
void Select_Clone ();
void Select_Move (vec3_t delta);
void Select_SetTexture (texdef_t *texdef);
void Select_FlipAxis (int axis);
void Select_RotateAxis (int axis, float deg, bool bMouse);  // sikk - Free Rotate: bool bMouse argument added
void Select_Scale (float x, float y, float z);	// sikk - Brush Scaling
void Select_All ();	// sikk - Select All
void Select_AllType ();	// sikk - Select All Type
void Select_CompleteTall ();
void Select_PartialTall ();
void Select_Touching ();
void Select_Inside ();
void Select_Ungroup ();	// sikk - made sense to put it here
void Select_NextBrushInGroup ();
void Select_InsertBrush ();	// sikk - Insert Brush into Entity
void Select_Invert ();
void Select_Hide ();
void Select_ShowAllHidden ();
void Select_MatchingTextures ();	// sikk - Select All Matching Textures
void Select_FitTexture (int nHeight, int nWidth);
void Select_ShiftTexture (int x, int y);
void Select_ScaleTexture (int x, int y);
void Select_RotateTexture (int deg);
void Select_ConnectEntities ();
void Select_MatchingKeyValue (char *szKey, char *szValue);	// sikk - Select Matching Key/Value
// sikk---> Cut/Copy/Paste
void Select_Cut ();
void Select_Copy ();
void Select_Paste ();
// returns true if pFind is in pList
bool OnEntityList (entity_t *pFind, entity_t *pList[MAX_MAP_ENTITIES], int nSize);
// <---sikk

// sikk - Multiple Face Selection: returns true if pFind is in pList
bool OnBrushList (brush_t *pFind, brush_t *pList[MAX_MAP_BRUSHES], int nSize);

void Clamp (float *f, int nClamp);
void ProjectOnPlane (vec3_t normal, float dist, vec3_t ez, vec3_t p);
void Back (vec3_t dir, vec3_t p);
void ComputeScale (vec3_t rex, vec3_t rey, vec3_t p, face_t *f);
void ComputeAbsolute (face_t *f, vec3_t p1, vec3_t p2, vec3_t p3);
void AbsoluteToLocal (plane_t normal2, face_t *f, vec3_t p1, vec3_t p2, vec3_t p3);
void RotateFaceTexture (face_t* f, int nAxis, float fDeg);
void RotateTextures (int nAxis, float fDeg, vec3_t vOrigin);

// updating workzone to a given brush (depends on current view)
void UpdateWorkzone (brush_t *b);

void FindReplaceTextures (char *pFind, char *pReplace, bool bSelected, bool bForce);

