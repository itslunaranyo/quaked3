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

extern const vec3 g_v3BaseAxis[18];
extern const float g_fLightAxis[3];

struct winding_s;
typedef winding_s winding_t;

//========================================================================

class Plane
{
public:
	Plane();
	vec3	normal;
	double	dist;

	bool		EqualTo(Plane *b, int flip);	// returns true if the planes are equal
	bool		FromPoints(const vec3 p1, const vec3 p2, const vec3 p3);	// returns false if the points are colinear
	winding_t	*BasePoly();
	void		GetTextureAxis(vec3 &xv, vec3 &yv);
};

//========================================================================

class Face : public SlabAllocator<Face>
{
public:
	Face();
	Face(Brush* b);	// no orphan (brushless) faces
	~Face();

	Face		*fnext;
	Face		*original;	// sikk - Vertex Editing Splits Face: used for vertex movement
	Brush		*owner;		// sikk - brush of selected face
	winding_t	*face_winding;
	vec3		planepts[3];
	Plane		plane;
	Texture		*d_texture;
	texdef_t	texdef;
	vec3		d_color;

	Face   *Clone();
	Face   *FullClone(Brush *own);	// sikk - Undo/Redo
	int		MemorySize();	// sikk - Undo/Redo
	void	BoundsOnAxis(const vec3 a, float* min, float* max);
	bool	ClipLine(vec3 &p1, vec3 &p2);

	void	FitTexture(float fHeight, float fWidth);
	void	MoveTexture(const vec3 delta);
	void	ColorAndTexture();
	void	SetTexture(texdef_t *texdef, int nSkipFlags);

	void	MakePlane();
	void	Draw();
private:
	void	SetColor();
	float	ShadeForPlane();
};

//========================================================================

// TODO: find me a home
int		AddPlanePoint(vec3 *f);

typedef struct
{
	int		p1, p2;
	Face	*f1, *f2;
} pedge_t;



#endif