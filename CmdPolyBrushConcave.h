//==============================
//	CmdPolyBrushConcave.h
//==============================

#ifndef __COMMAND_POLY_BRUSH_CONCAVE_H__
#define __COMMAND_POLY_BRUSH_CONCAVE_H__

#include <list>

class CmdPolyBrush;

class CmdPolyBrushConcave : public Command
{
public:
	CmdPolyBrushConcave();
	~CmdPolyBrushConcave();

	void SetPoints(std::vector<vec3> &points);
	void SetBounds(int low, int high);	// from workzone
	void SetTexDef(TexDef &tdef);		// from workzone
	void SetAxis(int ax);

	int BrushDelta();
	int EntityDelta();

private:
	std::vector<vec3> pointList;
	std::vector<CmdPolyBrush*> cmdPBs;
	std::list< std::vector<vec3>* > convexLoops;
	int axis, lowBound, highBound;
	TexDef texdef;

	enum splitWeights {
		axial =			1,
		diagonal =		1 << 1,
		angular =		1 << 2,

		collinear =		1 << 3,
		cone_reflex =	1 << 4,
		cone_side =		1 << 5,
		cone_core =		1 << 6,

		vertex =		1 << 7,
		vertex_reflex = 1 << 8,
		edge =			1 << 9,
		edge_straight = 1 << 10,

		edge_grid2 =	1 << 11,
		edge_grid8 =	1 << 12,
		edge_grid32 =	1 << 13,
	};

	struct loopRay_t {
		vec3 end;	// endpoint of ray
		int endv1, endv2;	// vert or edge the ray hits first
		unsigned wflags;

		loopRay_t() = delete;
		loopRay_t(const vec3 _end, const int _v1, const int _v2) :
			end(_end), endv1(_v1), endv2(_v2), wflags(0)
		{};
	};

	void ToWorldSpace(std::vector<vec3>& points);
	//void DetermineAxis(std::vector<vec3> &points);
	void RemoveDuplicates(std::vector<vec3> &points);
	void RemoveCollinear(std::vector<vec3>& points);
	void VerifyPoints(const std::vector<vec3> &points);
	void MakeClockwise(std::vector<vec3> &points);

	bool IsReflexVertex(const std::vector<vec3>& loop, const int v);
	int PickReflexVertex(std::vector<vec3>& loop);

	loopRay_t InitialRay(const std::vector<vec3>& loop, const int rv, const vec3 end);
	void InitialRays(std::vector<loopRay_t>& rays, const std::vector<vec3>& loop, const int rv);
	void VertexRays(std::vector<loopRay_t>& rays, const std::vector<vec3>& loop, const int rv);

	loopRay_t BestSplit(const std::vector<vec3>& loop, const int rv);
	float SplitWeight(const vec3 rp, const loopRay_t & split);
	void CleaveLoop(const std::vector<vec3>& loop, const int rv, const loopRay_t split, std::vector<vec3>& loopLeft, std::vector<vec3>& loopRight);
	void Decompose();

	void Do_Impl();
	void Undo_Impl();
	void Redo_Impl();
	void Sel_Impl();

};

#endif	// __COMMAND_POLY_BRUSH_CONCAVE_H__
