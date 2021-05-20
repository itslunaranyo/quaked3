//==============================
//	v_tex.cpp
//==============================

#include "pre.h"
#include "qe3.h"
#include "TextureView.h"
#include "Tool.h"

#define	FONT_HEIGHT	10
#define MARGIN_X 8

TextureView g_vTexture;

TextureView::TextureView() : stale(true)
{
}

TextureView::~TextureView()
{
}


/*
============================================================================

MOUSE ACTIONS

============================================================================
*/

/*
==============
TextureView::MouseDown
==============
*/
void TextureView::MouseDown(const int x, const int y, const int buttons)
{
	// necessary for scrolling
	Sys_GetCursorPos(&cursorX, &cursorY);
	if (buttons == (MK_LBUTTON))
		FoldTextureGroup(x, y);
}

/*
==============
TextureView::MouseUp
==============
*/
void TextureView::MouseUp(int x, int y, int buttons)
{
}

/*
==============
TextureView::MouseMoved
==============
*/
void TextureView::MouseMoved(int x, int y, int buttons)
{
	Arrange();

	// sikk--->	Mouse Zoom Texture Window
	// rbutton+control = zoom texture view
	if (buttons == (MK_CONTROL | MK_RBUTTON))
	{
		SetCursor(NULL); // sikk - Remove Cursor
		Sys_GetCursorPos(&x, &y);
		if (y != cursorY)
		{
			if (y > cursorY)
				scale *= powf(1.01f, fabs(y - cursorY));
			else
				scale *= powf(0.99f, fabs(y - cursorY));

			cursorX = x;
			cursorY = y;
			SetScale(scale);

			WndMain_UpdateWindows(W_TEXTURE);
		}
		return;
	}
	// <---sikk

	// rbutton = drag texture origin
	if (buttons & MK_RBUTTON)
	{
		SetCursor(NULL); // sikk - Remove Cursor
		Sys_GetCursorPos(&x, &y);
		if (y != cursorY)
		{
			Scroll(y - cursorY, (buttons & MK_SHIFT) > 0);
			Sys_SetCursorPos(cursorX, cursorY);

			WndMain_UpdateWindows(W_TEXTURE);
		}
	}
	else
		MouseOver(x, y);
}

/*
============
TextureView::MouseOver
============
*/
void TextureView::MouseOver(int x, int y)
{
	char		texstring[256];
	Texture*	tex;

	tex = TexAtCursorPos(x, y);
	if (tex)
	{
		sprintf(texstring, "%s (%dx%d)", tex->name, tex->width, tex->height);
		WndMain_Status(texstring, 0);
		return;
	}
	sprintf(texstring, "");
	WndMain_Status(texstring, 0);
}

void TextureView::ResetScroll()
{
	origin[1] = 0;
}

void TextureView::Scroll(int dist, bool fast)
{
	float lscale = fast ? 4 : 1;

	origin[1] += dist * lscale;
	if (origin[1] < -layoutLength + height)
		origin[1] = -layoutLength + height;
	if (origin[1] > 0)
		origin[1] = 0;
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
		if (Textures::texMap[label_t(q->name)] != q)
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
TextureView::ResizeControls
============
*/
void TextureView::Resize(int w, int h)
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
				if ((twpIt->y - twpIt->h < origin[1]) && (twpIt->y > origin[1] - 64))
				{
					twg = &(*twgIt);
					break;
				}
			}
			else
			{
				if ((twpIt->y - twpIt->h - FONT_HEIGHT < origin[1]) && (twpIt->y > origin[1] - 64))
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
				origin[1] = twg->top + twpIt->y;
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
	y += origin[1];
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
	texWndGroup_t* grp = TexGroupAtCursorPos(cx, cy);
	if (!grp) return false;

	return FoldTextureGroup(grp);
}


/*
==============
TextureView::TexGroupAtPos

gets the wad header under the x y window position
==============
*/
TextureView::texWndGroup_t* TextureView::TexGroupAtPos(int x, int y)
{
	Arrange();
	y += origin[1];

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

/*
==============
TextureView::TexGroupAtCursorPos

gets the texture currently under the cursor
==============
*/
TextureView::texWndGroup_t* TextureView::TexGroupAtCursorPos(const int cx, const int cy)
{
	return TexGroupAtPos(cx, -1 - cy);
}

/*
============
TextureView::UpdateStatus
============
*/
void TextureView::UpdateStatus(TexDef* texdef)
{
	char		sz[256];
	sprintf(sz, "Selected texture: %s (%dx%d)\n", texdef->name, texdef->tex->width, texdef->tex->height);
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
		Warning("Cannot select an entity texture.");
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

				if (twi->y + twgIt->top > origin[1])
				{
					origin[1] = twi->y + twgIt->top;
					WndMain_UpdateWindows(W_TEXTURE);
				}
				else if (twi->y + twgIt->top - twi->h - 2 * FONT_HEIGHT < origin[1] - height)
				{
					origin[1] = twi->y + twgIt->top - twi->h - 2 * FONT_HEIGHT + height;
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
	return strcmp(a->name, b->name);
}




/*
============================================================================

DRAWING

============================================================================
*/


/*
============
TextureView::Draw
============
*/
void TextureView::Draw()
{
	char	*name;
	vec3	txavg;
	int		yTop;

	txavg = 0.5f * (g_colors.texBackground + g_colors.texText);

	Arrange();
	
	glClearColor(g_colors.texBackground[0],
		g_colors.texBackground[1],
		g_colors.texBackground[2],
		0);

	glViewport(0, 0, width, height);
	glClear(GL_COLOR_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, width, origin[1] - height, origin[1], -100, 100);
	glEnable(GL_TEXTURE_2D);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	for (auto twgIt = layoutGroups.begin(); twgIt != layoutGroups.end(); ++twgIt)
	{
		// is this wad visible?
		if (twgIt->top < origin[1] - height)
			break;

		glLineWidth(1);

		// draw the wad header
		glColor3fv(&g_colors.texText.r);
		glDisable(GL_TEXTURE_2D);

		glRasterPos2f(MARGIN_X + 4, twgIt->top - FONT_HEIGHT);
		glCallLists(1, GL_UNSIGNED_BYTE, twgIt->folded ? "+" : "-" );
		glRasterPos2f(MARGIN_X + FONT_HEIGHT + 4, twgIt->top - FONT_HEIGHT);
		glCallLists(strlen(twgIt->tg->name), GL_UNSIGNED_BYTE, twgIt->tg->name);

		glColor3fv(&txavg.r);
		glBegin(GL_LINE_LOOP);
		glVertex2f(MARGIN_X, twgIt->top - FONT_HEIGHT - 4);
		glVertex2f(width - MARGIN_X, twgIt->top - FONT_HEIGHT - 4);
		glEnd();
		glEnable(GL_TEXTURE_2D);

		if (twgIt->folded) continue;

		for (auto twp = twgIt->layout.begin(); twp != twgIt->layout.end(); ++twp)
		{
			// Is this texture visible?
			if (twgIt->top + twp->y < origin[1] - height)
				break;
			if (twgIt->top + twp->y - twp->h - FONT_HEIGHT > origin[1])
				continue;

			yTop = twgIt->top + twp->y;

			// if in use, draw a background
			if (twp->tex->used)
			{
				glColor3fv(&txavg.r);
				glDisable(GL_TEXTURE_2D);

				glBegin(GL_LINE_LOOP);
				glVertex2f(twp->x - 1, yTop + 1 - FONT_HEIGHT);
				glVertex2f(twp->x - 1, yTop - twp->h - 1 - FONT_HEIGHT);	// sikk - Mouse Zoom Texture Window
				glVertex2f(twp->x + 1 + twp->w, yTop - twp->h - 1 - FONT_HEIGHT);	// sikk - Mouse Zoom Texture Window
				glVertex2f(twp->x + 1 + twp->w, yTop + 1 - FONT_HEIGHT);		// sikk - Mouse Zoom Texture Window
				glEnd();

				glEnable(GL_TEXTURE_2D);
			}

			// Draw the texture
			glColor3f(1, 1, 1);
			glBindTexture(GL_TEXTURE_2D, twp->tex->texture_number);
			glBegin(GL_QUADS);
			glTexCoord2f(0, 0);
			glVertex2f(twp->x, yTop - FONT_HEIGHT);
			glTexCoord2f(1, 0);
			glVertex2f(twp->x + twp->w, yTop - FONT_HEIGHT);	// sikk - Mouse Zoom Texture Window
			glTexCoord2f(1, 1);
			glVertex2f(twp->x + twp->w, yTop - FONT_HEIGHT - twp->h);	// sikk - Mouse Zoom Texture Window
			glTexCoord2f(0, 1);
			glVertex2f(twp->x, yTop - FONT_HEIGHT - twp->h);
			glEnd();

			// draw the selection border
			//if (!_strcmpi(g_qeglobals.d_workTexDef.name, twp->tex->name))
			if (g_qeglobals.d_workTexDef.tex == twp->tex)
			{
				glLineWidth(3);
				glColor3fv(&g_colors.selection.r);
				glDisable(GL_TEXTURE_2D);

				glBegin(GL_LINE_LOOP);
				glVertex2f(twp->x - 4, yTop - FONT_HEIGHT + 4);
				glVertex2f(twp->x - 4, yTop - FONT_HEIGHT - twp->h - 4);	// sikk - Mouse Zoom Texture Window
				glVertex2f(twp->x + 4 + twp->w, yTop - FONT_HEIGHT - twp->h - 4);	// sikk - Mouse Zoom Texture Window
				glVertex2f(twp->x + 4 + twp->w, yTop - FONT_HEIGHT + 4);		// sikk - Mouse Zoom Texture Window
				glEnd();

				glEnable(GL_TEXTURE_2D);
				glLineWidth(1);
			}

			// draw the texture name
			glColor3fv(&g_colors.texText.r);
			glDisable(GL_TEXTURE_2D);

			// don't draw the directory name
			for (name = twp->tex->name; *name && *name != '/' && *name != '\\'; name++)
				;
			if (!*name)
				name = twp->tex->name;
			else
				name++;

			glRasterPos2f(twp->x, yTop - FONT_HEIGHT + 2);
			glCallLists(strlen(name), GL_UNSIGNED_BYTE, name);
			glEnable(GL_TEXTURE_2D);
		}
	}

	// reset the current texture
	glBindTexture(GL_TEXTURE_2D, 0);
	glFinish();
}
