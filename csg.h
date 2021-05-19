#ifndef __CSG_H__
#define __CSG_H__
//==============================
//	csg.h
//==============================

void CSG_Hollow ();
void CSG_Merge ();
void CSG_Subtract ();
void CSG_SplitBrushByFace (brush_t *in, face_t *f, brush_t **front, brush_t **back);

brush_t *Brush_Merge (brush_t *brush1, brush_t *brush2, int onlyshape);
brush_t *Brush_MergeListPairs (brush_t *brushlist, int onlyshape);
brush_t *Brush_MergeList (brush_t *brushlist, int onlyshape);
brush_t *Brush_Subtract (brush_t *a, brush_t *b);

#endif
