//==============================
//	xy.h
//==============================

// window system independent xy view code
#ifndef __XYZ_H__
#define __XYZ_H__

typedef struct
{
	HDC		hdc;
	HGLRC	hglrc;
	int		dViewType;	// current orientation of this view
	int		width, height;
	float	scale;
	vec3_t	origin;	// at center of window
	float	topclip, bottomclip;
	bool	timing;
	bool	d_dirty;
} xyz_t;

//========================================================================

void XYZ_Init (xyz_t* xyz);

void XYZ_ToPoint (xyz_t* xyz, int x, int y, vec3_t point);
void XYZ_ToGridPoint (xyz_t* xyz, int x, int y, vec3_t point);
void XYZ_SnapToPoint (xyz_t* xyz, int x, int y, vec3_t point);

void XYZ_MouseDown (xyz_t* xyz, int x, int y, int buttons);
void XYZ_MouseUp (xyz_t* xyz, int x, int y, int buttons);
void XYZ_MouseMoved (xyz_t* xyz, int x, int y, int buttons);
void XYZ_PositionView (xyz_t* xyz);
void XYZ_VectorCopy (xyz_t* xyz, vec3_t in, vec3_t out);

void XYZ_Draw (xyz_t* xyz);
void XYZ_DrawGrid (xyz_t* xyz);
void XYZ_DrawBlockGrid (xyz_t* xyz);
void XYZ_DrawCoords (xyz_t* xyz);	// sikk - made separate function so coords and axis layed on top
void XYZ_DrawCameraIcon (xyz_t* xyz);
void XYZ_DrawLightRadius (brush_t *pBrush, int nViewType);
void XYZ_DrawSizeInfo (xyz_t* xyz, int nDim1, int nDim2, vec3_t vMinBounds, vec3_t vMaxBounds);
void XYZ_DrawRotateIcon (xyz_t* xyz);	// sikk - Free Rotate: Pivot Icon
void XYZ_DrawZIcon (xyz_t* xyz);
//void XY_Overlay ();

bool XYZ_DragDelta (xyz_t* xyz, int x, int y, vec3_t move);
void XYZ_DragNewBrush (xyz_t* xyz, int x, int y);

bool FilterBrush (brush_t *b);

void DrawPathLines ();

#endif
