//==============================
//	v_tex.h
//==============================
#ifndef __TEXTUREVIEW_H__
#define __TEXTUREVIEW_H__

#include "textures.h"
#include "v_view.h"

typedef struct
{
	int x, y, w, h;
	Texture* tex;
} texWndPlacement_t;

class TextureView : public View
{
public:
	TextureView();
	~TextureView();

	// lunaran: cached layout
	int		length;
	texWndPlacement_t* layout;
	int		count;

	bool	stale;	// layout is old and should be rebuilt before drawing

	void	MouseDown(int x, int y, int buttons);
	void	MouseUp(int x, int y, int buttons);
	void	MouseMoved(int x, int y, int buttons);
	void	MouseOver(int x, int y);
	void	Draw();
	int		AddToLayout(TextureGroup * tg, int top, int* curIdx);
	void	Layout();
	void	Resize(int w, int h);
	void	SetScale(float scale);
	Texture* TexAtPos(int x, int y);
	void	SelectTexture(int x, int y);
	void	ChooseTexture(texdef_t *texdef, bool bSetSelection);
	void	SortTextures();
	void	UpdateStatus(texdef_t* texdef);

};


//========================================================================



#endif