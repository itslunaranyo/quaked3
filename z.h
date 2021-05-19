//==============================
//	z.h
//==============================

// window system independent z view code

#define	PAGEFLIPS	2
#define CAM_HEIGHT	48	// height of main part
#define CAM_GIZMO	8	// height of the gizmo

typedef struct
{
	int		width, height;
	float	scale;
	vec3_t	origin;		// at center of window
	bool	timing;
} z_t;

//========================================================================

void Z_Init ();
void Z_MouseDown (int x, int y, int buttons);
void Z_MouseUp (int x, int y, int buttons);
void Z_MouseMoved (int x, int y, int buttons);
void Z_Draw ();
void Z_DrawGrid ();
void Z_DrawCameraIcon ();
void Z_DrawCoords ();
