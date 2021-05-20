//==============================
//	CmdGeoMod.h
//==============================

#ifndef __COMMAND_GEO_MOD_H__
#define __COMMAND_GEO_MOD_H__

#include "qe3.h"
#include "Command.h"
#include "CmdBrushMod.h"

class CmdGeoMod : public Command
{
public:
	CmdGeoMod();
	~CmdGeoMod();

	void SetBrush(Brush* br);
	void SetBrushes(Brush* brList);
	void SetBrushes(std::vector<Brush*>& brList);

	void SetPoint(Brush* br, const vec3 pt);
	void SetPoints(Brush* br, const std::vector<vec3>& pts);
	void SetPoint(const vec3 pt, const std::vector<Brush*> &brList);

	bool Translate(const vec3 trans, bool relative = false);

private:
	CmdBrushMod cmdBM;
	vec3 transTotal;

	class Mesh;
	class Polygon {
	public:
		Polygon(CmdGeoMod &gm, Face* f, Mesh *m);
		~Polygon() {}
		Mesh *mesh;
		Plane pOrig;
		TexDef tdOrig;
		bool dynamic, solved;
		std::vector<Face*> newFaces;
		std::vector<Polygon*> neighbors;
		std::vector<vec3*> vertices;

		void Clear();
		bool Resolve();
		bool Adjacent(const Polygon& other);
		bool Merge(Polygon *other, std::vector<vec3*> &verts);
		bool HasPoint(const vec3 *v);
		static bool dyncmp(const Polygon &a, const Polygon &b);
		Polygon* FindNeighborForMerge(const std::vector<Polygon*> &neighbs, const vec3 *v);
	};

	class Mesh {
	public:
		Mesh(CmdGeoMod &gm, Brush* br);
		~Mesh() {}
		Brush *bOrig;
		std::vector<Face*> newFaces;
		std::vector<vec3*> vertices;
		std::vector<Polygon> polies;

		void Clear();
		bool Resolve();
		void LinkNeighbors();
		void SetPointDynamic(vec3 *vStat, vec3 *vDyn);
	};

	std::vector<Brush*> brMods;
	std::vector<vec3> vertMaster;
	std::vector<Mesh> brushMeshes;

	enum {
		NOTHING_DONE,	// still accepting brushes
		BRUSHES_DONE,	// not accepting brushes, still accepting points
		POLIES_DONE,	// points & polygons complete, ready to mangle
	} modState;

	void UniquePoints(std::vector<vec3> &bigList, std::vector<vec3>& uList);
	void UniquePoints(std::vector<vec3*> &bigList, std::vector<vec3*>& uList);

	void ApplyTranslation(vec3 tr);
	void FinalizeBrushList();
	void FinalizePolygons();
	Mesh* EnsureMesh(Brush* br);

	void Do_Impl();
	void Undo_Impl();
	void Redo_Impl();
	void Sel_Impl();
};

#endif	// __COMMAND_GEO_MOD_H__
