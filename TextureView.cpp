//==============================
//	v_tex.cpp
//==============================

#include "pre.h"
#include "qe3.h"
#include "TextureGroup.h"
#include "TextureView.h"
#include "Tool.h"


TextureView g_vTexture;

TextureView::TextureView() : 
	stale(true), scale(1.0f), 
	scroll(0), layoutLength(0)
{
}

TextureView::~TextureView()
{
}

void TextureView::ResetScroll()
{
	scroll = 0;
}

void TextureView::Scroll(int dist, bool fast)
{
	float lscale = fast ? 4 : 1;

	scroll += dist * lscale;
	if (scroll < -layoutLength + height)
		scroll = -layoutLength + height;
	if (scroll > 0)
		scroll = 0;
}



/*
============================================================================

TEXTURE LAYOUT

cache texture window layout instead of rebuilding it every frame

	* top of the window is y=0, down is -y *

============================================================================
*/


/*
============
TextureView::ArrangeGroup
============
*/
void TextureView::ArrangeGroup(TextureGroup &txGrp)
{
	if (txGrp.numTextures == 0)
		return;

	Texture	*q;
	int		curX, curY, curRowHeight;

	layoutGroups.emplace_back();
	texWndGroup_t &twGrp = layoutGroups.back();
	twGrp.layout.reserve(txGrp.numTextures);
	twGrp.tg = &txGrp;
	twGrp.tgID = txGrp.id;

	// title bar
	curX = MARGIN_X;
	curY = 0 - FONT_HEIGHT - 8;
	curRowHeight = 0;

	for (q = txGrp.first; q; q = q->next)
	{
		if (Textures::texMap[q->name] != q)
			continue;

		// go to the next row unless the texture is the first on the row
		if (curRowHeight)
		{
			if (curX + q->width * scale > width - MARGIN_X)
			{
				curX = MARGIN_X;
				curY -= curRowHeight + FONT_HEIGHT + 4;
				curRowHeight = 0;
			}
		}
		twGrp.layout.emplace_back(curX, curY, q->width * scale, q->height * scale, q);

		// Is our texture larger than the row? If so, grow the row height to match it
		if (curRowHeight < q->height * scale)	// sikk - Mouse Zoom Texture Window
			curRowHeight = q->height * scale;	// sikk - Mouse Zoom Texture Window

		// never go less than 64, or the names get all crunched up
		curX += q->width * scale < 64 ? 64 : q->width * scale;	// sikk - Mouse Zoom Texture Window
		curX += MARGIN_X;
	}

	// new row
	twGrp.height = curRowHeight + FONT_HEIGHT + 12 - curY;
}

/*
============
TextureView::ArrangeGroup
============
*/
void TextureView::VertAlignGroups()
{
	int curY = -4;

	for (auto twgIt = layoutGroups.begin(); twgIt != layoutGroups.end(); ++twgIt)
	{
		twgIt->top = curY;
		curY -= twgIt->folded ? FONT_HEIGHT * 2 : twgIt->height;
	}

	layoutLength = -curY;
}


/*
============
TextureView::Arrange
============
*/
void TextureView::Arrange()
{
	if (!stale) return;

	// remember which layoutgroups are folded
	std::vector<int> foldedGroups;

	if (layoutGroups.size() > 0)
	{
		foldedGroups.resize(layoutGroups.size());

		for (auto twgIt = layoutGroups.begin(); twgIt != layoutGroups.end(); ++twgIt)
		{
			if (!twgIt->folded) continue;
			// id is required to provide a soft (ie not pointer-based) link between layout and 
			// texturegroup. texturegroups can have been flushed & deleted by the time we get to
			// this point, so no twg->tg dereference is safe.
			foldedGroups.push_back(twgIt->tgID);
		}
	}

	layoutGroups.clear();
	for (auto tgIt = Textures::groups.begin(); tgIt != Textures::groups.end(); tgIt++)
		ArrangeGroup(**tgIt);
	ArrangeGroup(Textures::group_unknown);

	if (foldedGroups.size() > 0)
	{
		for (auto fgIt = foldedGroups.begin(); fgIt != foldedGroups.end(); ++fgIt)
		{
			for (auto twgIt = layoutGroups.begin(); twgIt != layoutGroups.end(); ++twgIt)
			{
				if (twgIt->tg->id == *fgIt)
					twgIt->folded = true;
			}
		}
	}

	VertAlignGroups();

	Scroll(0, false);	// snap back to bounds
	stale = false;
}

void TextureView::Refresh()
{
	stale = true;
}

/*
============
TextureView::Resize
============
*/
void TextureView::Resize(const int w, const int h)
{
	width = w;
	height = h;
	stale = true;
}


/*
==============
TextureView::SetScale
==============
*/
void TextureView::Scale(float inscale)
{
	SetScale(scale * inscale);
}

void TextureView::SetScale(float inscale)
{
	char	texscalestring[128];
	Texture	*firsttexture;

	if (inscale > 8)
		inscale = 8;
	if (inscale < 0.25)
		inscale = 0.25f;

	// Find first visible texture with old scale and determine the y origin
	// that will place it at the top with the new scale
	texWndGroup_t *twg = nullptr;
	for (auto twgIt = layoutGroups.begin(); twgIt != layoutGroups.end(); ++twgIt)
	{
		for (auto twpIt = twgIt->layout.begin(); twpIt != twgIt->layout.end(); ++twpIt)
		{
			firsttexture = twpIt->tex;

			if (scale < inscale)
			{
				if ((twpIt->y - twpIt->h < scroll) && (twpIt->y > scroll - 64))
				{
					twg = &(*twgIt);
					break;
				}
			}
			else
			{
				if ((twpIt->y - twpIt->h - FONT_HEIGHT < scroll) && (twpIt->y > scroll - 64))
				{
					twg = &(*twgIt);
					break;
				}
			}
		}
		if (twg)
			break;
	}

	if (twg)
	{
		scale = inscale;
		stale = true;
		Arrange();

		for (auto twpIt = twg->layout.begin(); twpIt != twg->layout.end(); ++twpIt)
		{
			if (twpIt->tex == firsttexture)
			{
				scroll = twg->top + twpIt->y;
				break;
			}
		}
	}
	sprintf(texscalestring, "Texture scale: %3.0f%%", scale * 100.0f);
	WndMain_Status(texscalestring, 0);

	stale = true;
	Arrange();
}


/*
==============
TextureView::TexAtPos

gets the texture under the x y window position
==============
*/
Texture* TextureView::TexAtPos(int x, int y)
{
	Arrange();
	y += scroll;
	int cy;

	for (auto twgIt = layoutGroups.begin(); twgIt != layoutGroups.end(); ++twgIt)
	{
		if (twgIt->top - twgIt->height > y) continue;
		cy = y - twgIt->top + 4;
		if (twgIt->folded) continue;
		for (auto twpIt = twgIt->layout.begin(); twpIt != twgIt->layout.end(); ++twpIt)
		{
			if (x > twpIt->x && x - twpIt->x < twpIt->w	&&
				cy < twpIt->y && twpIt->y - cy < twpIt->h + FONT_HEIGHT)
			{
				return twpIt->tex;
			}
		}
	}
	return nullptr;
}


/*
==============
TextureView::FoldTextureGroup
==============
*/
bool TextureView::FoldTextureGroup(texWndGroup_t* grp)
{
	if (!grp) return false;

	grp->folded = !grp->folded;
	VertAlignGroups();
	Scroll(0, false);	// snap back to bounds

	WndMain_UpdateWindows(W_TEXTURE);
	return true;
}

bool TextureView::FoldTextureGroup(const int cx, const int cy)
{
	Arrange();
	texWndGroup_t* grp = LayoutGroupAtCursorPos(cx, cy);
	if (!grp) return false;

	return FoldTextureGroup(grp);
}


/*
==============
TextureView::LayoutGroupAtPos

gets the wad header under the x y window position
==============
*/
TextureView::texWndGroup_t* TextureView::LayoutGroupAtPos(int x, int y)
{
	Arrange();
	y += scroll;

	for (auto twgIt = layoutGroups.begin(); twgIt != layoutGroups.end(); ++twgIt)
	{
		if (y < twgIt->top && y > twgIt->top - 2 * FONT_HEIGHT)
			return &(*twgIt);
	}
	return nullptr;
}


/*
==============
TextureView::TexAtCursorPos

gets the texture currently under the cursor
==============
*/
Texture* TextureView::TexAtCursorPos(const int cx, const int cy)
{
	return TexAtPos(cx, -1 - cy);
}

TextureGroup * TextureView::TexGroupAtCursorPos(const int cx, const int cy)
{
	texWndGroup_t* lgap = LayoutGroupAtPos(cx, -1 - cy);
	if (!lgap) return nullptr;
	return lgap->tg;
}

/*
==============
TextureView::LayoutGroupAtCursorPos

gets the texture currently under the cursor
==============
*/
TextureView::texWndGroup_t* TextureView::LayoutGroupAtCursorPos(const int cx, const int cy)
{
	return LayoutGroupAtPos(cx, -1 - cy);
}

/*
============
TextureView::UpdateStatus
============
*/
void TextureView::UpdateStatus(TexDef* texdef)
{
	char		sz[256];
	sprintf(sz, "Selected texture: %s (%dx%d)\n", texdef->name.c_str(), texdef->tex->width, texdef->tex->height);
	WndMain_Status(sz, 3);
}


TextureView::texWndItem_t* TextureView::GetItemForTexture(const Texture* tex)
{
	for (auto twgIt = layoutGroups.begin(); twgIt != layoutGroups.end(); ++twgIt)
	{
		for (auto twpIt = twgIt->layout.begin(); twpIt != twgIt->layout.end(); ++twpIt)
		{
			if (twpIt->tex == tex)
			{
				return &(*twpIt);
			}
		}
	}
	return nullptr;
}

/*
============
TextureView::ChooseFirstTexture
============
*/
void TextureView::ChooseFirstTexture()
{
	Arrange();
	Texture* tex = nullptr;
	for (auto twgIt = layoutGroups.begin(); twgIt != layoutGroups.end(); ++twgIt)
	{
		if (twgIt->tg == &Textures::group_unknown) continue;
		
		tex = twgIt->layout.front().tex;
		break;
	}
	if (!tex) return;

	g_qeglobals.d_workTexDef.Set(tex);
	ChooseTexture(&g_qeglobals.d_workTexDef);
}

/*
============
TextureView::ChooseTexture
============
*/
void TextureView::ChooseTexture(TexDef *texdef)
{
	if (texdef->name[0] == '#')
	{
		Log::Warning("Cannot select an entity texture.");
		return;
	}
	g_qeglobals.d_workTexDef = *texdef;
	WndMain_UpdateWindows(W_TEXTURE | W_SURF);

	// scroll origin so the texture is completely on screen
	Texture* tx = Textures::ForName(texdef->name);
	Arrange();
	texWndItem_t* twi;
	
	for (auto twgIt = layoutGroups.begin(); twgIt != layoutGroups.end(); ++twgIt)
	{
		for (auto twpIt = twgIt->layout.begin(); twpIt != twgIt->layout.end(); ++twpIt)
		{
			if (twpIt->tex == tx)
			{
				twi = &(*twpIt);

				// unfurl it if it's folded up
				if (twgIt->folded) FoldTextureGroup(&(*twgIt));

				if (twi->y + twgIt->top > scroll)
				{
					scroll = twi->y + twgIt->top;
					WndMain_UpdateWindows(W_TEXTURE);
				}
				else if (twi->y + twgIt->top - twi->h - 2 * FONT_HEIGHT < scroll - height)
				{
					scroll = twi->y + twgIt->top - twi->h - 2 * FONT_HEIGHT + height;
					WndMain_UpdateWindows(W_TEXTURE);
				}
				UpdateStatus(texdef);
				return;
			}
		}
	}
	UpdateStatus(texdef);
	return;
}

/*
==============
texcmp
==============
*/
int texcmp(Texture* a, Texture* b)
{
	return (a->name < b->name);
}

