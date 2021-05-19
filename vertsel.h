//==============================
//	vertsel.h
//==============================
#ifndef __VERTSEL_H__
#define __VERTSEL_H__

int	FindPoint (vec3_t point);
int FindEdge (int p1, int p2, Face *f);

void MakeFace (Face *f);

void SetupVertexSelection ();
void SelectFaceEdge (Face *f, int p1, int p2);
void SelectVertex (int p1);
void SelectEdgeByRay (vec3_t org, vec3_t dir);
void SelectVertexByRay (vec3_t org, vec3_t dir);

#endif
