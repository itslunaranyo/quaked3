//==============================
//	surface.h
//==============================
#ifndef __SURFACE_H__
#define __SURFACE_H__


namespace Surface
{
	//void ProjectOnPlane(const vec3 normal, const float dist, const vec3 ez, vec3 &p);
	void Back(const vec3 dir, vec3 &p);
	void Clamp(float *f, int nClamp);
	void ComputeScale(const vec3 rex, const vec3 rey, vec3 &p, TexDef &td);
	void ComputeAbsolute(Face *f, vec3 &p1, vec3 &p2, vec3 &p3);
	void ComputeAbsolute(Plane &p, TexDef &td, vec3 &p1, vec3 &p2, vec3 &p3);
	void AbsoluteToLocal(Plane normal2, TexDef &td, vec3 &p1, vec3 &p2, vec3 &p3);
	void RotateFaceTexture(Face* f, int nAxis, float fDeg, const vec3 vOrigin);

	void FindReplace(char *pFind, char *pReplace, bool bSelected);// , bool bForce);
	void SetTexdef(TexDef &texdef, unsigned flags);
	void ApplyTexdef(TexDef &dst, TexDef &src, unsigned flags);
	void RotateForTransform(int nAxis, float fDeg, const vec3 vOrigin);

	void WrapProjection(Plane &from, Plane &to, TexDef &td);
}

#endif
