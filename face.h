#ifndef __FACE_H__
#define __FACE_H__

//==============================
//	face.h
//==============================

#include "SlabAllocator.h"

// the normals on planes point OUT of the brush
#define	MAXPOINTS	16
#define	MAX_FACES	16
#define MAX_POINTS_ON_WINDING	64
#define MAX_MOVE_FACES	64

//========================================================================

class Brush;

struct winding_s;
typedef winding_s winding_t;

//========================================================================

class Face : public SlabAllocator<Face>
{
public:
	Face();
	Face(Brush* b);	// no orphan (brushless) faces
	Face(Face* f);	// copy face
	Face(Brush* b, Face* f);	// copy face to new brush
	Face(Plane &p, TexDef &td);
	~Face();

	Face		*fnext;
	//Face		*original;	// sikk - Vertex Editing Splits Face: used for vertex movement
	Brush		*owner;		// sikk - brush of selected face
	winding_t	*face_winding;
	Plane		plane;
	TexDef		texdef;
	vec3		d_color;

	Face	*Clone();
	static void	ClearChain(Face **f);
	//Face	*FullClone(Brush *own);	// sikk - Undo/Redo
	int		MemorySize();	// sikk - Undo/Redo
	winding_t *MakeWinding();
	void	BoundsOnAxis(const vec3 a, float* min, float* max);
	void	AddBounds(vec3 &mins, vec3 &maxs);
	bool	TestSideSelect(const vec3 origin, const vec3 dir);

	inline bool ClipLine(vec3 &p1, vec3 &p2) { return plane.ClipLine(p1, p2); }
	inline void MakePlane() { plane.Make(); }

	void	FitTexture(float fHeight, float fWidth);
	void	Transform(mat4 mat, bool bTexLock);
	void	MoveTexture(const vec3 delta);
	void	ColorAndTexture();
	void	SetTexture(TexDef *texdef, unsigned flags);

	void	Draw();
	void	DrawWire();
private:
	void	SetColor();
	float	ShadeForPlane();
};

//========================================================================

typedef struct
{
	int		p1, p2;
	Face	*f1, *f2;
} pedge_t;



#endif