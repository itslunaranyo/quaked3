//==============================
//	select.h
//==============================

#define	DIST_START	999999

#define	SF_SELECTED_ONLY	0x01
#define	SF_ENTITIES_FIRST	0x02
#define	SF_SINGLEFACE		0x04
#define SF_CYCLE			0x08	// sikk - Single Selection Cycle (Shift+Alt+LMB)
#define SF_NOFIXEDSIZE		0x10	// lunaran - avoid selecting the 'faces' on entity brushes

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

extern bool			g_bSelectionChanged;
extern brush_t		g_brSelectedBrushes;

// sikk - Multiple Face Selection
extern face_t		*g_pfaceSelectedFaces[MAX_MAP_FACES];

// <-- sikk

//extern face_t		*g_pfaceSelectedFace;
//extern brush_t	*g_pfaceSelectedFaceBrush;	// sikk - g_pfaceSelectedFace has "owner" brush
vec3_t				g_v3RotateOrigin;	// sikk - Free Rotate

//========================================================================

void Select_HandleChange();
void Select_SelectBrush(brush_t *b);

trace_t Test_Ray (vec3_t origin, vec3_t dir, int flags);
void Select_Ray (vec3_t origin, vec3_t dir, int flags);

bool Select_HasBrushes();
int Select_FaceCount();
bool Select_IsEmpty();

void Select_GetBounds (vec3_t mins, vec3_t maxs);
void Select_GetTrueMid (vec3_t mid);
void Select_GetMid (vec3_t mid);
void Select_ApplyMatrix ();
void Select_HandleBrush (brush_t *b, bool bComplete);
void Select_Delete ();
void Select_DeselectFiltered();
void Select_DeselectAll (bool bDeselectFaces);
// sikk---> Multiple Face Selection
bool Select_IsFaceSelected (face_t *face);
bool Select_DeselectAllFaces ();
// <---sikk
void Select_Clone ();
void Select_Move (vec3_t delta);
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


// updating workzone to a given brush (depends on current view)
void UpdateWorkzone (brush_t *b);



