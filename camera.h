#ifndef __CAMERA_H__
#define __CAMERA_H__
//==============================
//	camera.h
//==============================

// window system independent camera view code

typedef enum
{
	cd_wire,
	cd_solid,
	cd_texture,
	cd_blend
} camera_draw_mode;

typedef struct
{
	int		width, height;
	bool	timing;
	vec_t	viewdistance;		// For rotating around a point
	vec3_t	origin;
	vec3_t	angles;
	vec3_t	color;				// background 
	vec3_t	forward, right, up;	// move matrix
	vec3_t	vup, vpn, vright;	// view matrix
	camera_draw_mode	draw_mode;
} camera_t;

//========================================================================

extern int g_nCamButtonState;

//extern camera_t	camera;	// sikk - moved to qeglobals_t

//========================================================================

void Cam_Init ();
void Cam_KeyDown (int key);
void Cam_BoundAngles();
void Cam_PointToRay(int x, int y, vec3_t rayOut);
void Cam_MouseDown (int x, int y, int buttons);
void Cam_MouseUp (int x, int y, int buttons);
void Cam_MouseMoved (int x, int y, int buttons);
void Cam_MouseControl (float dtime);
void Cam_Draw ();
void Cam_DrawGrid ();	// sikk - Camera Grid
void Cam_DrawClipSplits();

void Cam_BuildMatrix ();
void Cam_ChangeFloor (bool up);
void Cam_FreeLook ();
void Cam_PositionDrag ();
void Cam_PositionView ();	// sikk - Center Camera on Selection
void Cam_PositionRotate ();
void Cam_Rotate (int x, int y, vec3_t origin);

void Cam_InitCull ();
bool Cam_CullBrush (brush_t *b);

#endif
