//==============================
//	CmdFaceMod.h
//==============================

#ifndef __COMMAND_FACE_MOD_H__
#define __COMMAND_FACE_MOD_H__

/*
========================================================================

FACE MODIFICATION

For any change which modifies the properties of any number of brush faces,
without adding or removing them or changing their relationships. 
- does not change memory location of modified faces

========================================================================
*/

class CmdFaceMod : public Command
{
public:
	CmdFaceMod();
	~CmdFaceMod();

	// ModifyFace: clone off the data of these Faces and keep it
	void ModifyFace(Face* f);
	void ModifyFaces(std::vector<Face*> &fList);
	void ModifyFaces(Face *fList);

	// RestoreFace: restore cloned data from these Faces
	void RestoreFace(Face* f);
	void RestoreFaces(std::vector<Face*> &fList);
	void RestoreFaces(Face *fList);
	void RestoreAll();

	// UnmodifyFace: throw away cloned data from these Faces and stop tracking - does NOT restore
	void UnmodifyFace(Face* f);
	void UnmodifyFaces(std::vector<Face*> &fList);
	void UnmodifyFaces(Face *fList);
	void UnmodifyAll();

	// RevertFace: restore cloned data from these Faces and stop tracking
	void RevertFace(Face* f);
	void RevertFaces(std::vector<Face*> &fList);
	void RevertFaces(Face *fList);
	void RevertAll();

	void RebuildAll();

private:
	typedef struct fBasis_s {
		fBasis_s() {};
		fBasis_s(Face *fOrig) : f(fOrig), plane(fOrig->plane), texdef(fOrig->texdef) {};
		Face *f;
		Plane plane;
		TexDef texdef;
	} fBasis;

	std::vector<fBasis> faceCache;

	static bool fbasisCmp(const fBasis &a, const fBasis &b);
	void Swap(fBasis &fb);
	void SwapAll();

	void Do_Impl();
	void Undo_Impl();
	void Redo_Impl();
};

#endif	// __COMMAND_FACE_MOD_H__
