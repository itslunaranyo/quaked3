//==============================
//	v_tex.h
//==============================
#ifndef __TEXTUREVIEW_H__
#define __TEXTUREVIEW_H__

#include "textures.h"
#include "v_view.h"
#include <vector>

class TextureView : public View
{
public:
	TextureView();
	~TextureView();

	void	Arrange();
	void	Refresh() { stale = true; }

	void	ChooseTexture(TexDef *texdef);
	Texture *TexAtCursorPos(const int cx, const int cy);

	void	MouseDown(const int x, const int y, const int buttons);
	void	MouseUp(const int x, const int y, const int buttons);
	void	MouseMoved(const int x, const int y, const int buttons);
	void	MouseOver(const int x, const int y);
	void	SetScale(float inscale);
	void	Scale(float inscale);
	void	Scroll(int dist, bool fast);

	void	Draw();

private:
	// lunaran: cached layout
	struct texWndPlacement_t {
		texWndPlacement_t(int _x, int _y, int _w, int _h, Texture* _tex) : x(_x), y(_y), w(_w), h(_h), tex(_tex) {};
		int x, y, w, h;
		Texture* tex;
	};
	int		length;
	std::vector<texWndPlacement_t> layout;

	bool	stale;	// layout is old and should be rebuilt before drawing

	int		AddToLayout(TextureGroup * tg, int top);
	void	Resize(const int w, const int h);
	Texture* TexAtPos(int x, int y);
	//void	SelectTexture(int x, int y);
	void	SortTextures();
	void	UpdateStatus(TexDef* texdef);

};


//========================================================================



#endif