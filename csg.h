//==============================
//	csg.h
//==============================

#ifndef __CSG_H__
#define __CSG_H__

namespace CSG
{
	void SplitBrushByFace (Brush *in, Face *f, Brush **front, Brush **back = nullptr);
	Brush *DoMerge(std::vector<Face*> &fList, bool notexmerge);
	Brush *DoBridge(std::vector<Face*>& fList);
	Brush *DoMerge(std::vector<Brush*> &brList, bool notexmerge);
	Brush *BrushMergeListPairs (Brush *brushlist, bool onlyshape);

	void Subtract();
	void Hollow();
	void Merge();
	void Bridge();
}
#endif
