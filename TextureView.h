//==============================
//	v_tex.h
//==============================
#ifndef __TEXTUREVIEW_H__
#define __TEXTUREVIEW_H__

#include "textures.h"
#include "View.h"
#include <vector>

class TextureView : public View
{
public:
	TextureView();
	~TextureView();

	void	Arrange();
	void	Refresh();

	void	ChooseTexture(TexDef *texdef);
	void	ChooseFirstTexture();
	Texture *TexAtCursorPos(const int cx, const int cy);
	bool	FoldTextureGroup(const int cx, const int cy);

	void	MouseDown(const int x, const int y, const int buttons);
	void	MouseUp(const int x, const int y, const int buttons);
	void	MouseMoved(const int x, const int y, const int buttons);
	void	MouseOver(const int x, const int y);
	void	SetScale(float inscale);
	void	Scale(float inscale);
	void	Scroll(int dist, bool fast);
	void	ResetScroll();

	void	Draw();

private:
	// lunaran: cached layout
	struct texWndItem_t {
		texWndItem_t(int _x, int _y, float _w, float _h, Texture* _tex) : x(_x), y(_y), w(_w), h(_h), tex(_tex) {};
		int x, y;
		float w, h;
		Texture* tex;
	};

public:
	struct texWndGroup_t {
		std::vector<texWndItem_t> layout;
		int height, top;
		bool folded;
		TextureGroup *tg;
		int tgID;
	};
	TextureView::texWndGroup_t *TexGroupAtCursorPos(const int cx, const int cy);

private:
	int layoutLength;	// height of layout in pixels
	std::vector<texWndGroup_t> layoutGroups;

	bool	stale;	// layout is old and should be rebuilt before drawing

	void	ArrangeGroup(TextureGroup &txGrp);
	void	VertAlignGroups();
	texWndItem_t* GetItemForTexture(const Texture *tex);
	void	Resize(const int w, const int h);
	Texture* TexAtPos(int x, int y);
	bool	FoldTextureGroup(texWndGroup_t *grp);
	TextureView::texWndGroup_t *TexGroupAtPos(int x, int y);

	void	UpdateStatus(TexDef* texdef);

};


//========================================================================

extern TextureView g_vTexture;


#endif