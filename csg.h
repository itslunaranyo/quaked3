//==============================
//	csg.h
//==============================

#ifndef __CSG_H__
#define __CSG_H__

namespace CSG
{
	void SplitBrushByFace (Brush *in, Face *f, Brush **front, Brush **back = nullptr);
	Brush *DoMerge(std::vector<Brush*> &brList, bool notexmerge);// , bool convex);
	Brush *BrushMergeListPairs (Brush *brushlist, bool onlyshape);

	void Subtract();
	void Hollow();
	void Merge();
}
#endif
