//==============================
//	select.h
//==============================
#ifndef __SELECT_H__
#define __SELECT_H__

#define	DIST_START	999999

#define	SF_SELECTED_ONLY	0x0001
#define	SF_ENTITIES_FIRST	0x0002
#define	SF_FACES			0x0004
#define SF_CYCLE			0x0008	// sikk - Single Selection Cycle(Shift+Alt+LMB)
#define SF_NOFIXEDSIZE		0x0010	// lunaran - avoid selecting the 'faces' on entity brushes
#define SF_EXCLUSIVE		0x0020	// always deselect all first
#define SF_SELECTED			0x0040
#define SF_UNSELECTED		0x0080
#define SF_EXPAND			0x0100	// lunaran - gtkr style doubleclick select

//========================================================================

typedef enum
{
	sel_brush,
	sel_face,
	sel_component
} select_t;

typedef struct trace_s
{
	Brush	*brush;
	Face	*face;
	float	dist;
	bool	selected;
	trace_s() : brush(nullptr), face(nullptr), dist(0), selected(false) {};
} trace_t;

//========================================================================

extern bool			g_bSelectionChanged;
extern Brush		g_brSelectedBrushes;

//========================================================================

namespace Selection
{
	extern std::vector<Face*> faces;

	void	Changed();
	void	HandleChange();

	bool	HasBrushes();
	bool	IsEmpty();
	bool	OnlyPointEntities();
	bool	OnlyBrushEntities();
	bool	OneBrushEntity();
	int		NumBrushes();
	int		NumFaces();

	bool	IsBrushSelected(Brush *bSel);
	bool	IsEntitySelected(Entity *eSel);
	void	SelectBrush(Brush *b);
	void	SelectBrushSorted(Brush *b);
	void	HandleBrush(Brush *b, bool bComplete);

	bool	DeselectAllFaces();
	bool	IsFaceSelected(Face *face);
	void	SelectFace(Face* f);
	void	SelectFaces(Brush *b);
	bool	DeselectFace(Face* f);
	void	DeselectFaces(Brush *b);
	int		NumBrushFacesSelected(Brush* b);
	void	FacesToBrushes(bool partial);
	void	BrushesToFaces();

	trace_t	TestRay(const vec3 origin, const vec3 dir, int flags);
	int		Ray(const vec3 origin, const vec3 dir, int flags);
	trace_t	TestPoint(const vec3 origin, int flags);
	void	Point(const vec3 origin, int flags);

	void	SelectAll();	// sikk - Select All
	void	DeselectFiltered();
	void	DeselectAll();
	bool	GetBounds(vec3 &mins, vec3 &maxs);
	vec3	GetTrueMid();
	vec3	GetMid();

	void	MatchingKeyValue(char *szKey, char *szValue);	// sikk - Select Matching Key/Value
	void	MatchingTextures();	// sikk - Select All Matching Textures

	void	Invert();
	void	AllType();	// sikk - Select All Type

	void	CompleteTall();
	void	PartialTall();
	void	Touching();
	void	Inside();

	void	NextBrushInGroup();
}
// ================================


// sikk - Multiple Face Selection: returns true if pFind is in pList
bool	OnBrushList(Brush *pFind, Brush *pList[MAX_MAP_BRUSHES], int nSize);

// updating workzone to a given brush(depends on current view)
void	UpdateWorkzone(Brush *b);


#endif


