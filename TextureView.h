//==============================
//	v_tex.h
//==============================
#ifndef __TEXTUREVIEW_H__
#define __TEXTUREVIEW_H__

#include "textures.h"
#include "DisplayView.h"
#include <vector>

#define	FONT_HEIGHT	10
#define MARGIN_X 8

class TextureView : public DisplayView
{
public:
	TextureView();
	~TextureView();

	// VIEW:
public:
	void	Arrange();
	void	Refresh();
	void	Resize(const int w, const int h);

	Texture *TexAtCursorPos(const int cx, const int cy);
	TextureGroup *TexGroupAtCursorPos(const int cx, const int cy);
	bool	FoldTextureGroup(const int cx, const int cy);

	void	SetScale(float inscale);
	void	Scale(float inscale);
	void	Scroll(int dist, bool fast);
	void	ResetScroll();
	inline int GetScroll() { return scroll; }

protected:
	friend class TexBrowserRenderer;
	struct texWndItem_t {
		texWndItem_t(int _x, int _y, float _w, float _h, Texture* _tex) : x(_x), y(_y), w(_w), h(_h), tex(_tex) {};
		int x, y;
		float w, h;
		Texture* tex;
	};
	struct texWndGroup_t {
		std::vector<texWndItem_t> layout;
		int height, top;
		bool folded;
		TextureGroup *tg;
		int tgID;
	};
	std::vector<texWndGroup_t> layoutGroups;

private:
	float	scale;
	int		scroll, layoutLength;	// height of layout in pixels
	bool	stale;	// layout is old and should be rebuilt before drawing

	void	ArrangeGroup(TextureGroup &txGrp);
	void	VertAlignGroups();
	bool	FoldTextureGroup(texWndGroup_t *grp);
	void	UpdateStatus(TexDef* texdef);

	Texture* TexAtPos(int x, int y);
	texWndItem_t* GetItemForTexture(const Texture *tex);
	TextureView::texWndGroup_t *LayoutGroupAtCursorPos(const int cx, const int cy);
	TextureView::texWndGroup_t *LayoutGroupAtPos(int x, int y);

	// TOOL:
public:
	void	ChooseTexture(TexDef *texdef);
	void	ChooseFirstTexture();	// make 'getfirsttexture'
};


//========================================================================

extern TextureView g_vTexture;


#endif