//==============================
//	z.h
//==============================
#ifndef __ZVIEW_H__
#define __ZVIEW_H__

// window system independent z view code

class ZView : public View
{
public:
	ZView();
	~ZView();

	void	MouseDown(int x, int y, int buttons);
	void	MouseUp(int x, int y, int buttons);
	void	MouseMoved(int x, int y, int buttons);
	void	Draw ();

	void	ToPoint(int x, int y, vec3 &point);

private:
	void	Init();
	void	DrawGrid ();
	void	DrawCameraIcon ();
	void	DrawCoords ();
};

//========================================================================

#endif
