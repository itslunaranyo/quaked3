#ifndef __FACE_H__
#define __FACE_H__

//==============================
//	face.h
//==============================

#include "SlabAllocator.h"
#include "Plane.h"
#include "TexDef.h"
#include "Winding.h"

// the normals on planes point OUT of the brush
#define	MAXPOINTS	16
#define	MAX_FACES	16
#define MAX_POINTS_ON_WINDING	64
#define MAX_MOVE_FACES	64

//========================================================================

class Brush;

#ifdef OLDWINDING
struct winding_t;
#endif

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
	Brush		*owner;
	Plane		plane;
	TexDef		texdef;
	vec3		d_color;

	Face	*Clone();
	static void	ClearChain(Face **f);
	void	BoundsOnAxis(const vec3 a, float* min, float* max);
	void	AddBounds(vec3 &mins, vec3 &maxs);
	bool	TestSideSelect(const vec3 origin, const vec3 dir);
	bool	TestSideSelectAxis(const vec3 origin, const int axis);
	bool	ConcaveTo(Face& f2);

	inline bool ClipLine(vec3 &p1, vec3 &p2) { return plane.ClipLine(p1, p2); }
	inline void MakePlane() { plane.Make(); }

	void	FitTexture(const float fHeight, const float fWidth);
	void	Transform(mat4 mat, bool bTexLock);
	void	MoveTexture(const vec3 delta);
	void	ColorAndTexture();
	void	SetTexture(TexDef *texdef, unsigned flags);

	void	Draw();
	void	DrawWire();

	bool MakeWinding();
	void FreeWinding();	// call when moving into undo limbo
	inline bool const HasWinding() { return winding.Count() > 0; }
	inline Winding& GetWinding() { return winding; }
	inline void SwapWinding(Face& other) {
		winding.Swap(other.GetWinding());
	}

private:
	//winding_t	*winding;
	Winding winding;
	void	SetColor();
	float	ShadeForPlane();
};


#endif