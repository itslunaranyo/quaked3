#ifndef __CLIP_H__
#define __CLIP_H__
//==============================
//	clip.h
//==============================

// 3-Point Clipping Tool

#include "xy.h"

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
void Clip_StartQuickClip(xyz_t* xyz, int x, int y);
void Clip_EndQuickClip();
void Clip_DropPoint (xyz_t* xyz, int x, int y);
void Clip_MovePoint (xyz_t* xyz, int x, int y);
void Clip_DrawPoint (xyz_t* xyz);
void Clip_EndPoint ();

#endif
