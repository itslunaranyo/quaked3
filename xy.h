//==============================
//	xy.h
//==============================

// window system independent xy view code

typedef struct
{
	int		width, height;
	float	scale;
	float	topclip, bottomclip;
	vec3_t	origin;	// at center of window
	bool	timing;
	bool	d_dirty;
} xy_t;

//========================================================================

void XY_Init ();
void XY_MouseDown (int x, int y, int buttons);
void XY_MouseUp (int x, int y, int buttons);
void XY_MouseMoved (int x, int y, int buttons);
void XY_PositionView ();
void XY_Draw ();
void XY_DrawGrid ();
void XY_DrawBlockGrid ();
void XY_DrawCoords ();	// sikk - made separate function so coords and axis layed on top
void XY_DrawCameraIcon ();
void XY_DrawLightRadius (brush_t *pBrush, int nViewType);
void XY_DrawSizeInfo (int nDim1, int nDim2, vec3_t vMinBounds, vec3_t vMaxBounds);
void XY_DrawRotateIcon ();	// sikk - Free Rotate: Pivot Icon
void XY_DrawZIcon ();
void XY_Overlay ();
void XY_VectorCopy (vec3_t in, vec3_t out);

void XY_ToPoint (int x, int y, vec3_t point);
void XY_ToGridPoint (int x, int y, vec3_t point);
void XY_SnapToPoint (int x, int y, vec3_t point);

bool Drag_Delta (int x, int y, vec3_t move);
void Drag_NewBrush (int x, int y);

bool FilterBrush (brush_t *b);

void DrawPathLines ();

float Betwixt (float f1, float f2);
float fDiff (float f1, float f2);
