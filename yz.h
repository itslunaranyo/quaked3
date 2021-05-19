//==============================
//	yz.h
//==============================

// window system independent yz view code

typedef struct
{
	int		width, height;
	float	scale;
	float	topclip, bottomclip;
	vec3_t	origin;	// at center of window
	bool	timing;
	bool	d_dirty;
} yz_t;

//========================================================================

void YZ_Init ();
void YZ_MouseDown (int x, int y, int buttons);
void YZ_MouseUp (int x, int y, int buttons);
void YZ_MouseMoved (int x, int y, int buttons);
void YZ_PositionView ();
void YZ_Draw ();
void YZ_DrawGrid ();
void YZ_DrawBlockGrid ();
void YZ_DrawCoords ();	// sikk - made separate function so coords and axis layed on top
void YZ_DrawCameraIcon ();
void YZ_DrawLightRadius (brush_t *pBrush);
void YZ_DrawSizeInfo (int nDim1, int nDim2, vec3_t vMinBounds, vec3_t vMaxBounds);
void YZ_DrawRotateIcon ();	// sikk - Free Rotate: Pivot Icon
void YZ_Overlay ();
void YZ_VectorCopy (vec3_t in, vec3_t out);

void YZ_ToPoint (int x, int y, vec3_t point);
void YZ_ToGridPoint (int x, int y, vec3_t point);
void YZ_SnapToPoint (int x, int y, vec3_t point);

bool YZ_Drag_Delta (int x, int y, vec3_t move);
void YZ_Drag_NewBrush (int x, int y);


