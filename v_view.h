//==============================
//	view.h
//==============================
#ifndef __VIEW_H__
#define __VIEW_H__

// window system independent view code

class View
{
public:
	View();
	~View();

	int		width, height;
	float	scale;
	vec3	origin;
	bool	timing;

	virtual void Resize(int w, int h) { width = w; height = h; }
	virtual void MouseDown(int x, int y, int buttons) {};
	virtual void MouseUp(int x, int y, int buttons) {};
	virtual void MouseMoved(int x, int y, int buttons) {};
	virtual void Draw();

	void	DrawPathLines();
	void	DrawTools();

	int	cursorX, cursorY;
private:
};

#endif