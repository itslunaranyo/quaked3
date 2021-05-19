//==============================
//	xz.h
//==============================

// window system independent xz view code

typedef struct
{
	int		width, height;
	float	scale;
	float	topclip, bottomclip;
	vec3_t	origin;	// at center of window
	bool	timing;
	bool	d_dirty;
} xz_t;

//========================================================================

void XZ_Init ();
void XZ_MouseDown (int x, int y, int buttons);
void XZ_MouseUp (int x, int y, int buttons);
void XZ_MouseMoved (int x, int y, int buttons);
void XZ_PositionView ();
void XZ_Draw ();
void XZ_DrawGrid ();
void XZ_DrawBlockGrid ();
void XZ_DrawCoords ();	// sikk - made separate function so coords and axis layed on top
void XZ_DrawCameraIcon ();
void XZ_DrawLightRadius (brush_t *pBrush);
void XZ_DrawSizeInfo (int nDim1, int nDim2, vec3_t vMinBounds, vec3_t vMaxBounds);
void XZ_DrawRotateIcon ();	// sikk - Free Rotate: Pivot Icon
void XZ_Overlay ();
void XZ_VectorCopy (vec3_t in, vec3_t out);

void XZ_ToPoint (int x, int y, vec3_t point);
void XZ_ToGridPoint (int x, int y, vec3_t point);
void XZ_SnapToPoint (int x, int y, vec3_t point);

bool XZ_Drag_Delta (int x, int y, vec3_t move);
void XZ_Drag_NewBrush (int x, int y);


