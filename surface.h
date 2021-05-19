//==============================
//	surface.h
//==============================
#ifndef __SURFACE_H__
#define __SURFACE_H__

#define SURF_MIXEDNAME		0x01
#define SURF_MIXEDSHIFTX	0x02
#define SURF_MIXEDSHIFTY	0x04
#define SURF_MIXEDSCALEX	0x08
#define SURF_MIXEDSCALEY	0x10
#define SURF_MIXEDROTATE	0x20
#define SURF_MIXEDALL		0x3F

void ProjectOnPlane(const vec3 normal, const float dist, const vec3 ez, vec3 &p);
void Back(const vec3 dir, vec3 &p);
void Clamp(float *f, int nClamp);
void ComputeScale(const vec3 rex, const vec3 rey, vec3 &p, Face *f);
void ComputeAbsolute(Face *f, vec3 &p1, vec3 &p2, vec3 &p3);
void AbsoluteToLocal(Plane normal2, Face *f, vec3 &p1, vec3 &p2, vec3 &p3);
void RotateFaceTexture(Face* f, int nAxis, float fDeg, const vec3 vOrigin);

void Surf_FindReplace(char *pFind, char *pReplace, bool bSelected, bool bForce);
void Surf_SetTexdef(texdef_t *texdef, int nSkipFlags);
void Surf_ApplyTexdef(texdef_t *dst, texdef_t *src, int nSkipFlags);
void Surf_RotateForTransform(int nAxis, float fDeg, const vec3 vOrigin);
void Surf_FitTexture(float nHeight, float nWidth);
void Surf_ShiftTexture(int x, int y);
void Surf_ScaleTexture(int x, int y);
void Surf_RotateTexture(int deg);


#endif
