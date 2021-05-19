//==============================
//	csg.h
//==============================

#ifndef __CSG_H__
#define __CSG_H__

void CSG_Hollow ();
void CSG_Merge();
void CSG_ConvexMerge();
void CSG_Subtract ();
void CSG_SplitBrushByFace (Brush *in, Face *f, Brush **front, Brush **back);

Brush* Brush_ConvexMerge(Brush *bList);
Brush *Brush_Merge (Brush *brush1, Brush *brush2, int onlyshape);
Brush *Brush_MergeListPairs (Brush *brushlist, int onlyshape);
Brush *Brush_MergeList (Brush *brushlist, int onlyshape);
Brush *Brush_Subtract (Brush *a, Brush *b);

#endif
