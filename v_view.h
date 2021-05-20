//==============================
//	view.h
//==============================
#ifndef __VIEW_H__
#define __VIEW_H__

// window system independent view code

typedef struct mouseContext_s {
	mouseContext_s() : pt(vec3(0)), up(vec3(0)), right(vec3(0)), ray(vec3(0)), org(vec3(0)), dims(0) {}
	vec3 pt;
	vec3 up;
	vec3 right;
	vec3 ray;
	vec3 org;
	int dims;
} mouseContext_t;

class View
{
public:
	View();
	~View();

	int		width, height;
	float	scale;
	vec3	origin;
	bool	timing;

	virtual mouseContext_t	const GetMouseContext(const int x, const int y) { return mouseContext_t(); }

	virtual void Resize(int w, int h) { width = w; height = h; }
	virtual void MouseDown(int x, int y, int buttons) {};
	virtual void MouseUp(int x, int y, int buttons) {};
	virtual void MouseMoved(int x, int y, int buttons) {};
	virtual void Draw();
	virtual bool DrawTools();

	void	DrawPathLines();
	bool	GetBasis(vec3 &right, vec3 &up, vec3 &forward) { return false; }
	void	GLSelectionColor();
	void	GLSelectionColorAlpha(float alpha);

	int	cursorX, cursorY;
private:
};

#endif