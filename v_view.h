//==============================
//	view.h
//==============================
#ifndef __VIEW_H__
#define __VIEW_H__

// window system independent view code

typedef struct mouseContext_s {
	mouseContext_s() : up(vec3(0)), right(vec3(0)), ray(vec3(0)), org(vec3(0)), scale(1.0f), dims(0) {}
	vec3 up;
	vec3 right;
	vec3 ray;
	vec3 org;
	float scale;
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

	int	cursorX, cursorY;
private:
};

#endif