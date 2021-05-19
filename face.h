#ifndef __FACE_H__
#define __FACE_H__

//==============================
//	face.h
//==============================

// the normals on planes point OUT of the brush
#define	MAXPOINTS	16
#define	MAX_FACES	16
#define MAX_POINTS_ON_WINDING	64
#define MAX_MOVE_FACES	64

//========================================================================

class Brush;

extern const vec3_t g_v3BaseAxis[18];
extern const float g_fLightAxis[3];

struct winding_s;
typedef winding_s winding_t;

//========================================================================

class Plane
{
public:
	Plane();
	vec3_t	normal;
	double	dist;

	bool		EqualTo(Plane *b, int flip);	// returns true if the planes are equal
	bool		FromPoints(vec3_t p1, vec3_t p2, vec3_t p3);	// returns false if the points are colinear
	winding_t	*BasePoly();
	void		GetTextureAxis(vec3_t xv, vec3_t yv);
};

//========================================================================

class Face
{
public:
	Face();
	Face(Brush* b);	// no orphan (brushless) faces
	~Face();

	Face		*next;
	Face		*original;	// sikk - Vertex Editing Splits Face: used for vertex movement
	Brush		*owner;		// sikk - brush of selected face
	vec3_t		planepts[3];
	texdef_t	texdef;
	Plane		plane;
	winding_t	*face_winding;
	vec3_t		d_color;
	Texture	*d_texture;

	Face   *Clone();
	Face   *FullClone(Brush *own);	// sikk - Undo/Redo
	int		MemorySize();	// sikk - Undo/Redo
	void	BoundsOnAxis(vec3_t a, float* min, float* max);
	bool	ClipLine(vec3_t p1, vec3_t p2);

	void	FitTexture(float fHeight, float fWidth);
	void	MoveTexture(vec3_t delta);
	void	ColorAndTexture();
	void	SetTexture(texdef_t *texdef, int nSkipFlags);

	void	MakePlane();
	void	Draw();
private:
	void	Init();
	void	SetColor();
	float	ShadeForPlane();
};

//========================================================================

// TODO: find me a home
int		AddPlanePoint(float *f);



typedef struct
{
	int		p1, p2;
	Face *f1, *f2;
} pedge_t;



#endif