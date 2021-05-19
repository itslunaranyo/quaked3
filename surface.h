//==============================
//	surface.h
//==============================
#ifndef __SURFACE_H__
#define __SURFACE_H__

#define SURF_MIXEDNAME	0x01
#define SURF_MIXEDSHIFTX	0x02
#define SURF_MIXEDSHIFTY	0x04
#define SURF_MIXEDSCALEX	0x08
#define SURF_MIXEDSCALEY	0x10
#define SURF_MIXEDROTATE	0x20
#define SURF_MIXEDALL		0x3F

void ProjectOnPlane(vec3_t normal, float dist, vec3_t ez, vec3_t p);
void Back(vec3_t dir, vec3_t p);
void Clamp(float *f, int nClamp);
void ComputeScale(vec3_t rex, vec3_t rey, vec3_t p, Face *f);
void ComputeAbsolute(Face *f, vec3_t p1, vec3_t p2, vec3_t p3);
void AbsoluteToLocal(Plane normal2, Face *f, vec3_t p1, vec3_t p2, vec3_t p3);
void RotateFaceTexture(Face* f, int nAxis, float fDeg, vec3_t vOrigin);

void Surf_FindReplace(char *pFind, char *pReplace, bool bSelected, bool bForce);
void Surf_SetTexdef(texdef_t *texdef, int nSkipFlags);
void Surf_ApplyTexdef(texdef_t *dst, texdef_t *src, int nSkipFlags);
void Surf_RotateForTransform(int nAxis, float fDeg, vec3_t vOrigin);
void Surf_FitTexture(float nHeight, float nWidth);
void Surf_ShiftTexture(int x, int y);
void Surf_ScaleTexture(int x, int y);
void Surf_RotateTexture(int deg);


#endif
