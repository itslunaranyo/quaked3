#ifndef __CLIP_H__
#define __CLIP_H__
//==============================
//	clip.h
//==============================

// 3-Point Clipping Tool

class XYZView;
class CmdBrushClip;

typedef struct
{
	bool	bSet;
	vec3	ptClip;      // the 3d point
} clippoint_t;

//========================================================================

extern clippoint_t	g_cpClip1;
extern clippoint_t	g_cpClip2;
extern clippoint_t	g_cpClip3;
extern clippoint_t	*g_pcpMovingClip;
extern CmdBrushClip	*g_pcmdBC;

//========================================================================

void SnapToPoint(vec3 &point);

void Clip_SetMode();
void Clip_UnsetMode();
void Clip_ResetMode();
void Clip_Clip();
void Clip_Split();
void Clip_Flip();

void Clip_CamStartQuickClip(int x, int y);
void Clip_CamEndQuickClip();
void Clip_CamDropPoint(int x, int y);
void Clip_CamMovePoint(int x, int y);
void Clip_CamEndPoint();

void Clip_StartQuickClip(XYZView* xyz, int x, int y);
void Clip_EndQuickClip();
void Clip_DropPoint(XYZView* xyz, int x, int y);
void Clip_MovePoint(XYZView* xyz, int x, int y);
void Clip_EndPoint();
void Clip_Draw();

#endif
