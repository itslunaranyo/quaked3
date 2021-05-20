#ifndef __R_CAMERA_H__
#define __R_CAMERA_H__
//==============================
//	CameraRenderer.h
//==============================

#include "Renderer.h"

class CameraView;
class DisplayView;

class CameraRenderer : public Renderer
{
public:
	CameraRenderer(CameraView &view);
	~CameraRenderer();

	glm::mat4 matProj;
	void Draw();
	void DrawSelected(Brush	*pList, vec3 selColor);

	DisplayView* GetDisplayView() { return (DisplayView*)&cv; }

private:
	CameraView &cv;
	std::vector<Brush*> transBrushes;
	void DrawMode(int mode);
	void DrawActive();
	void DrawGrid();
	void DrawBoundary();
	void DrawAxis();
	bool DrawTools();

};

#endif