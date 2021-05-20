//==============================
//	v_tex.cpp
//==============================

#include "qe3.h"

#define	FONT_HEIGHT	10

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
	if (stale)
		Layout();

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

			Sys_UpdateWindows(W_TEXTURE);
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

			Sys_UpdateWindows(W_TEXTURE);
		}
	}
	else
		MouseOver(x, height - 1 - y);
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

	tex = TexAtPos(x, y);
	if (tex)
	{
		sprintf(texstring, "%s (%dx%d)", tex->name, tex->width, tex->height);
		Sys_Status(texstring, 0);
		return;
	}
	sprintf(texstring, "");
	Sys_Status(texstring, 0);
}


void TextureView::Scroll(int dist, bool fast)
{
	float lscale = fast ? 4 : 1;

	origin[1] += dist * lscale;
	if (origin[1] < length)
		origin[1] = length;
	if (origin[1] > 0)
		origin[1] = 0;
}



/*
============================================================================

TEXTURE LAYOUT

lunaran: cache texture window layout instead of rebuilding it every frame

============================================================================
*/

int TextureView::AddToLayout(TextureGroup* tg, int top, int* curIdx)
{
	Texture	*q;
	int		curX, curY, curRowHeight;

	curX = 8;
	curY = top;
	curRowHeight = 0;

	for (q = tg->first; q; q = q->next)
	{
		if (Textures::texMap[label_t(q->name)] != q)
			continue;

		// go to the next row unless the texture is the first on the row
		if (curRowHeight)
		{
			if (curX + q->width * scale > width - 8)
			{
				curX = 8;
				curY -= curRowHeight + FONT_HEIGHT + 4;
				curRowHeight = 0;
			}
		}

		layout[*curIdx].tex = q;
		layout[*curIdx].x = curX;
		layout[*curIdx].y = curY;
		layout[*curIdx].w = q->width * scale;
		layout[*curIdx].h = q->height * scale;

		// Is our texture larger than the row? If so, grow the row height to match it
		if (curRowHeight < q->height * scale)	// sikk - Mouse Zoom Texture Window
			curRowHeight = q->height * scale;	// sikk - Mouse Zoom Texture Window

		// never go less than 64, or the names get all crunched up
		curX += q->width * scale < 64 ? 64 : q->width * scale;	// sikk - Mouse Zoom Texture Window
		curX += 8;

		(*curIdx)++;
		count++;
	}

	// new row
	return curY - (curRowHeight + FONT_HEIGHT + 4);
}

/*
============
TextureView::Layout
============
*/
void TextureView::Layout()
{
	int curY, curIdx;

	curY = -8;

	if (layout)
	{
		delete[] layout;
		layout = nullptr;
	}

	count = Textures::group_unknown.numTextures;
	for (auto tgIt = Textures::groups.begin(); tgIt != Textures::groups.end(); tgIt++)
		count += (*tgIt)->numTextures;

	if (count)
	{
		layout = new texWndPlacement_t[count];

		curIdx = 0;
		count = 0;
		for (auto tgIt = Textures::groups.begin(); tgIt != Textures::groups.end(); tgIt++)
		{
			curY = AddToLayout(*tgIt, curY, &curIdx) - 12;	// padding
		}
		curY = AddToLayout(&Textures::group_unknown, curY, &curIdx);

		length = curY + height;
		Scroll(0, false);	// snap back to bounds
	}
	stale = false;
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



// sikk--->	Mouse Zoom Texture Window
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
	char		texscalestring[128];
	texWndPlacement_t* twp;
	Texture *firsttexture;
	int i;

	if (inscale > 8)
		inscale = 8;
	if (inscale < 0.25)
		inscale = 0.25f;

	// Find first visible texture with old scale and determine the y origin
	// that will place it at the top with the new scale
	for (i = 0; i < count; i++)
	{
		twp = &layout[i];
		firsttexture = twp->tex;

		if (scale < inscale)
		{
			if ((twp->y - twp->h < origin[1]) && (twp->y > origin[1] - 64))
				break;
		}
		else
		{
			if ((twp->y - twp->h - FONT_HEIGHT < origin[1]) && (twp->y > origin[1] - 64))
				break;
		}
	}

	if (i < count)
	{
		scale = inscale;
		Layout();

		for (i = 0; i < count; i++)
		{
			twp = &layout[i];

			if (twp->tex == firsttexture)
			{
				origin[1] = twp->y;
				break;
			}
		}
	}
	sprintf(texscalestring, "Texture scale: %3.0f%%", scale * 100.0f);
	Sys_Status(texscalestring, 0);

	Layout();
}
// <---sikk

/*
==============
TextureView::TexAtPos

Sets texture window's caption to the name and size of the texture
under the current mouse position.
==============
*/
Texture* TextureView::TexAtPos(int x, int y)
{
	texWndPlacement_t* twp;

	y += origin[1] - height;

	for (int i = 0; i < count; i++)
	{
		twp = &layout[i];
		if (x > twp->x && x - twp->x < twp->w	&& // sikk - Mouse Zoom Texture Window
			y < twp->y && twp->y - y < twp->h + FONT_HEIGHT)	// sikk - Mouse Zoom Texture Window
		{
			return twp->tex;
		}
	}
	return NULL;
}

/*
==============
TextureView::TexAtCursorPos
==============
*/
Texture* TextureView::TexAtCursorPos(const int cx, const int cy)
{
	return TexAtPos(cx, height - 1 - cy);
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
	Sys_Status(sz, 3);
}

/*
============
TextureView::ChooseTexture
============
*/
void TextureView::ChooseTexture(TexDef *texdef)
{
	//int			x, y;
	//qtexture_t *q;
	texWndPlacement_t* twp;

	if (texdef->name[0] == '#')
	{
		Warning("Cannot select an entity texture.");
		return;
	}
	g_qeglobals.d_workTexDef = *texdef;
	Sys_UpdateWindows(W_TEXTURE | W_SURF);

	// scroll origin so the texture is completely on screen
	for (int i = 0; i < count; i++)
	{
		twp = &layout[i];
		if (!_strcmpi(texdef->name, twp->tex->name))
		{
			if (twp->y > origin[1])
			{
				origin[1] = twp->y;
				Sys_UpdateWindows(W_TEXTURE);
				UpdateStatus(texdef);
				return;
			}

			if (twp->y - twp->h - 2 * FONT_HEIGHT < origin[1] - height)	// sikk - Mouse Zoom Texture Window
			{
				origin[1] = twp->y - twp->h - 2 * FONT_HEIGHT + height;	// sikk - Mouse Zoom Texture Window
				Sys_UpdateWindows(W_TEXTURE);
				UpdateStatus(texdef);
				return;
			}
			UpdateStatus(texdef);
			return;
		}
	}
}

/*
==============
TextureView::SelectTexture

By mouse click
==============
*/
/*
void TextureView::SelectTexture(int x, int y)
{
	TexDef texdef;
	Texture* tw;

	tw = TexAtPos(x, y);
	if (tw)
	{
		texdef.scale[0] = g_qeglobals.d_fDefaultTexScale;	// sikk - Default Texture Scale Dialog
		texdef.scale[1] = g_qeglobals.d_fDefaultTexScale;	// sikk - Default Texture Scale Dialog

		texdef.Set(tw);
		ChooseTexture(&texdef);
		Surface::SetTexdef(texdef, 0);
		return;
	}

	Warning("Did not select a texture.");
}
*/


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
==============
TextureView::SortTextures
==============
*/
void TextureView::SortTextures()
{
	Texture	*q, *qtemp, *qhead, *qcur, *qprev;

	Sys_Printf("Sorting active textures...\n");

	// standard insertion sort
	// Take the first texture from the list and
	// add it to our new list
	if (g_qeglobals.d_qtextures == NULL)
		return;

	qhead = g_qeglobals.d_qtextures;
	q = g_qeglobals.d_qtextures->next;
	qhead->next = NULL;

	// while there are still things on the old
	// list, keep adding them to the new list
	while (q)
	{
		qtemp = q;
		q = q->next;

		qprev = NULL;
		qcur = qhead;

		while (qcur)
		{
			// Insert it here?
			if (texcmp(qtemp, qcur) < 0)
			{
				qtemp->next = qcur;
				if (qprev)
					qprev->next = qtemp;
				else
					qhead = qtemp;
				break;
			}

			// Move on
			qprev = qcur;
			qcur = qcur->next;

			// is this one at the end?
			if (qcur == NULL)
			{
				qprev->next = qtemp;
				qtemp->next = NULL;
			}
		}
	}
	g_qeglobals.d_qtextures = qhead;
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
	texWndPlacement_t* twp;
	char	   *name;
	vec3 txavg;

	txavg = 0.5f * (g_colors.texBackground + g_colors.texText);

	if (stale) Layout();
	
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

	for (int i = 0; i < count; i++)
	{
		twp = &layout[i];

		// Is this texture visible?
		if ((twp->y - twp->h - FONT_HEIGHT < origin[1]) &&
			(twp->y > origin[1] - height))	// sikk - Mouse Zoom Texture Window
		{
			// if in use, draw a background
			if (twp->tex->used)
			{
				glLineWidth(1);
				glColor3fv(&txavg.r);
				glDisable(GL_TEXTURE_2D);

				glBegin(GL_LINE_LOOP);
				glVertex2f(twp->x - 1, twp->y + 1 - FONT_HEIGHT);
				glVertex2f(twp->x - 1, twp->y - twp->h - 1 - FONT_HEIGHT);	// sikk - Mouse Zoom Texture Window
				glVertex2f(twp->x + 1 + twp->w, twp->y - twp->h - 1 - FONT_HEIGHT);	// sikk - Mouse Zoom Texture Window
				glVertex2f(twp->x + 1 + twp->w, twp->y + 1 - FONT_HEIGHT);		// sikk - Mouse Zoom Texture Window
				glEnd();

				glEnable(GL_TEXTURE_2D);
			}

			// Draw the texture
			glColor3f(1, 1, 1);
			glBindTexture(GL_TEXTURE_2D, twp->tex->texture_number);
			glBegin(GL_QUADS);
			glTexCoord2f(0, 0);
			glVertex2f(twp->x, twp->y - FONT_HEIGHT);
			glTexCoord2f(1, 0);
			glVertex2f(twp->x + twp->w, twp->y - FONT_HEIGHT);	// sikk - Mouse Zoom Texture Window
			glTexCoord2f(1, 1);
			glVertex2f(twp->x + twp->w, twp->y - FONT_HEIGHT - twp->h);	// sikk - Mouse Zoom Texture Window
			glTexCoord2f(0, 1);
			glVertex2f(twp->x, twp->y - FONT_HEIGHT - twp->h);
			glEnd();

			// draw the selection border
			//if (!_strcmpi(g_qeglobals.d_workTexDef.name, twp->tex->name))
			if (g_qeglobals.d_workTexDef.tex == twp->tex)
			{
				glLineWidth(3);
				glColor3fv(&g_colors.selection.r);
				glDisable(GL_TEXTURE_2D);

				glBegin(GL_LINE_LOOP);
				glVertex2f(twp->x - 4, twp->y - FONT_HEIGHT + 4);
				glVertex2f(twp->x - 4, twp->y - FONT_HEIGHT - twp->h - 4);	// sikk - Mouse Zoom Texture Window
				glVertex2f(twp->x + 4 + twp->w, twp->y - FONT_HEIGHT - twp->h - 4);	// sikk - Mouse Zoom Texture Window
				glVertex2f(twp->x + 4 + twp->w, twp->y - FONT_HEIGHT + 4);		// sikk - Mouse Zoom Texture Window
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

			glRasterPos2f(twp->x, twp->y - FONT_HEIGHT + 2);
			glCallLists(strlen(name), GL_UNSIGNED_BYTE, name);
			glEnable(GL_TEXTURE_2D);
		}
	}

	// reset the current texture
	glBindTexture(GL_TEXTURE_2D, 0);
	glFinish();
}
