//==============================
//	clip.h
//==============================

// 3-Point Clipping Tool

typedef struct
{
	bool	bSet;
	vec3_t	ptClip;      // the 3d point
} clippoint_t;

//========================================================================

extern clippoint_t	g_cpClip1;
extern clippoint_t	g_cpClip2;
extern clippoint_t	g_cpClip3;
extern clippoint_t *g_pcpMovingClip;

//========================================================================

void Clip_ProduceSplitLists ();
void Clip_SetMode ();
void Clip_UnsetMode ();
void Clip_ResetMode ();
void Clip_Clip();
void Clip_Split ();
void Clip_Flip ();
void Clip_DropPoint (int x, int y);
void Clip_MovePoint (int x, int y, int nView);
void Clip_DrawPoint (int nView);
void Clip_EndPoint ();
