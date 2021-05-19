//==============================
//	select.h
//==============================
#ifndef __SELECT_H__
#define __SELECT_H__

#define	DIST_START	999999

#define	SF_SELECTED_ONLY	0x01
#define	SF_ENTITIES_FIRST	0x02
#define	SF_FACES			0x04
#define SF_CYCLE			0x08	// sikk - Single Selection Cycle(Shift+Alt+LMB)
#define SF_NOFIXEDSIZE		0x10	// lunaran - avoid selecting the 'faces' on entity brushes
#define SF_MULTIFACE		0x20
#define SF_SELECTED			0x40
#define SF_UNSELECTED		0x80

//========================================================================

typedef enum
{
	sel_brush,
	sel_face,
	sel_component,
	sel_vertex,
	sel_edge
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

// sikk - Multiple Face Selection
// lunaran TODO: make this a std vector
//extern Face		*g_vfSelectedFaces[MAX_MAP_FACES];
// <-- sikk

extern vec3		g_v3RotateOrigin;	// sikk - Free Rotate

//========================================================================

namespace Selection
{
	extern std::vector<Face*> faces;

	void	Changed();
	void	HandleChange();

	bool	HasBrushes();
	bool	IsEmpty();
	bool	OnlyPointEntities();
	int		NumBrushes();
	int		NumFaces();

	bool	IsBrushSelected(Brush* bSel);
	void	SelectBrush(Brush *b);
	void	SelectBrushSorted(Brush *b);
	void	HandleBrush(Brush *b, bool bComplete);

	bool	DeselectAllFaces();
	bool	IsFaceSelected(Face *face);
	void	SelectFace(Face* f);
	bool	DeselectFace(Face* f);
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


