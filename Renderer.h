//==============================
//	Renderer.h
//==============================

#ifndef __RENDERER_H__
#define __RENDERER_H__


class Renderer
{
public:
	Renderer();
	virtual ~Renderer();

	bool	timing;

	virtual void Draw();
protected:
	void	DrawPathLines();
	void	GLSelectionColor();
	void	GLSelectionColorAlpha(float alpha);
	void	Text(const char* str, int x, int y);
};

#endif
