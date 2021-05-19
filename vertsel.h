//==============================
//	vertsel.h
//==============================
#ifndef __VERTSEL_H__
#define __VERTSEL_H__

int	FindPoint (const vec3 point);
int FindEdge (int p1, int p2, Face *f);

void MakeFace (Face *f);

void SetupVertexSelection ();
void SelectFaceEdge (Face *f, int p1, int p2);
void SelectVertex (int p1);
void SelectEdgeByRay (const vec3 org, const vec3 dir);
void SelectVertexByRay (const vec3 org, const vec3 dir);
int AddPlanePoint(vec3 *f);

#endif
