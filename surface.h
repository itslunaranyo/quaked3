//==============================
//	surface.h
//==============================
#ifndef __SURFACE_H__
#define __SURFACE_H__

void ProjectOnPlane(const vec3 normal, const float dist, const vec3 ez, vec3 &p);
void Back(const vec3 dir, vec3 &p);
void Clamp(float *f, int nClamp);
void ComputeScale(const vec3 rex, const vec3 rey, vec3 &p, Face *f);
void ComputeAbsolute(Face *f, vec3 &p1, vec3 &p2, vec3 &p3);
void AbsoluteToLocal(Plane normal2, Face *f, vec3 &p1, vec3 &p2, vec3 &p3);
void RotateFaceTexture(Face* f, int nAxis, float fDeg, const vec3 vOrigin);

void Surf_FindReplace(char *pFind, char *pReplace, bool bSelected, bool bForce);
void Surf_SetTexdef(TexDef &texdef, unsigned flags);
void Surf_ApplyTexdef(TexDef &dst, TexDef &src, unsigned flags);
void Surf_RotateForTransform(int nAxis, float fDeg, const vec3 vOrigin);

#endif
