//==============================
//	v_tex.cpp
//==============================

#include "qe3.h"

static int	cursorX, cursorY;
#define	FONT_HEIGHT	10


TextureView::TextureView()
{
	scale = 1.0f;	// sikk - Mouse Zoom Texture Window
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
void TextureView::MouseDown(int x, int y, int buttons)
{
	int cx, cy;

	Sys_GetCursorPos(&cursorX, &cursorY);
	cx = x;
	cy = height - 1 - y;

	// lbutton = select texture
	if (buttons == MK_LBUTTON)
	{
		SelectTexture(cx, cy);
		return;
	}
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
	float	lscale = 1;	// sikk - Mouse Zoom Texture Window (changed from "int")

	if (buttons & MK_SHIFT)
		lscale = 4;

	// sikk--->	Mouse Zoom Texture Window
	//rbutton+control = zoom texture view
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
			origin[1] += (y - cursorY) * lscale;
			if (origin[1] < length)
				origin[1] = length;
			if (origin[1] > 0)
				origin[1] = 0;
			Sys_SetCursorPos(cursorX, cursorY);
			Sys_UpdateWindows(W_TEXTURE);
		}
		//		return;
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
	qtexture_t*	tex;

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





/*
============================================================================

TEXTURE LAYOUT

lunaran: cache texture window layout instead of rebuilding it every frame

============================================================================
*/


/*
============
TextureView::Layout
============
*/
void TextureView::Layout()
{
	qtexture_t	*q;
	int			curIdx, curX, curY, curRowHeight;
	char*		wad;

	curX = 8;
	curY = -8;
	curRowHeight = 0;
	count = 0;

	if (layout)
	{
		free(layout);
		layout = NULL;
	}
	for (q = g_qeglobals.d_qtextures; q; q = q->next)
	{
		if (q->name[0] == '(') continue;	// fake color texture
		count++;
	}
	if (!count) return;

	layout = (texWndPlacement_t*)qmalloc(sizeof(texWndPlacement_t) * count);

	curIdx = 0;
	wad = NULL;
	for (q = g_qeglobals.d_qtextures; q; q = q->next)
	{
		if (q->name[0] == '(') continue;

		// go to the next row unless the texture is the first on the row
		if (curRowHeight)
		{
			if (curX + q->width * scale > width - 8)
			{
				curX = 8;
				curY -= curRowHeight + FONT_HEIGHT + 4;
				curRowHeight = 0;
			}
			else if (wad && g_qeglobals.d_savedinfo.bSortTexByWad && q->wad && strcmp(q->wad, wad) != 0)
			{
				curX = 8;
				curY -= curRowHeight + FONT_HEIGHT + 16;
				curRowHeight = 0;
			}
		}
		// lunaran: row breaks for new wads when sorted
		wad = q->wad;

		layout[curIdx].tex = q;
		layout[curIdx].x = curX;
		layout[curIdx].y = curY;
		layout[curIdx].w = q->width * scale;
		layout[curIdx].h = q->height * scale;

		// Is our texture larger than the row? If so, grow the row height to match it
		if (curRowHeight < q->height * scale)	// sikk - Mouse Zoom Texture Window
			curRowHeight = q->height * scale;	// sikk - Mouse Zoom Texture Window

		// never go less than 64, or the names get all crunched up
		curX += q->width * scale < 64 ? 64 : q->width * scale;	// sikk - Mouse Zoom Texture Window
		curX += 8;

		curIdx++;
	}
	length = curY - curRowHeight - FONT_HEIGHT - 8 + height;
}



// sikk--->	Mouse Zoom Texture Window
/*
==============
TextureView::SetScale
==============
*/
void TextureView::SetScale(float inscale)
{
	char		texscalestring[128];
	texWndPlacement_t* twp;
	qtexture_t *firsttexture;
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
qtexture_t* TextureView::TexAtPos(int x, int y)
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
============
TextureView::UpdateStatus
============
*/
void TextureView::UpdateStatus(texdef_t* texdef)
{
	char		sz[256];
	sprintf(sz, "Selected texture: %s (%dx%d)\n", texdef->name, 0, 0);// q->width, q->height);	// lunaran TODO: texdef doesn't contain a pointer to the texture?
	Sys_Status(sz, 3);
}

/*
============
TextureView::ChooseTexture
============
*/
void TextureView::ChooseTexture(texdef_t *texdef, bool bSetSelection)
{
	//int			x, y;
	//qtexture_t *q;
	texWndPlacement_t* twp;

	if (texdef->name[0] == '(')
	{
		Sys_Printf("WARNING: Cannot select an entity texture.\n");
		return;
	}
	g_qeglobals.d_workTexDef = *texdef;

	Sys_UpdateWindows(W_TEXTURE);
	//	sprintf(sz, "Selected texture: %s\n", texdef->name);
	//	Sys_Status(sz, 0);
	// sikk---> Multiple Face Selection
	// Check if we want to set current selection's texture
	if (bSetSelection)
		Surf_SetTexdef(texdef, 0);

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
void TextureView::SelectTexture(int x, int y)
{
	texdef_t	texdef;

	qtexture_t*	tw;

	tw = TexAtPos(x, y);
	if (tw)
	{
		memset(&texdef, 0, sizeof(texdef));
		texdef.scale[0] = g_qeglobals.d_fDefaultTexScale;	// sikk - Default Texture Scale Dialog
		texdef.scale[1] = g_qeglobals.d_fDefaultTexScale;	// sikk - Default Texture Scale Dialog

		strcpy(texdef.name, tw->name);
		ChooseTexture(&texdef, true);
		return;
	}

	Sys_Printf("WARNING: Did not select a texture.\n");
}





/*
==============
texcmp
==============
*/
int texcmp(qtexture_t* a, qtexture_t* b)
{
	if (g_qeglobals.d_savedinfo.bSortTexByWad && a->wad && b->wad)
	{
		int cmp;
		cmp = strcmp(a->wad, b->wad);
		if (cmp != 0)
			return cmp;
	}
	return strcmp(a->name, b->name);
}

/*
==============
TextureView::SortTextures
==============
*/
void TextureView::SortTextures()
{
	qtexture_t	*q, *qtemp, *qhead, *qcur, *qprev;

	Sys_Printf("CMD: Sorting active textures...\n");

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
void TextureView::Draw(int dw, int dh)
{
	texWndPlacement_t* twp;
	char	   *name;

	glClearColor(g_qeglobals.d_savedinfo.v3Colors[COLOR_TEXTUREBACK][0],
		g_qeglobals.d_savedinfo.v3Colors[COLOR_TEXTUREBACK][1],
		g_qeglobals.d_savedinfo.v3Colors[COLOR_TEXTUREBACK][2],
		0);
	glViewport(0, 0, dw, dh);
	glClear(GL_COLOR_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, dw, origin[1] - dh, origin[1], -100, 100);
	glEnable(GL_TEXTURE_2D);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	for (int i = 0; i < count; i++)
	{
		twp = &layout[i];

		// Is this texture visible?
		if ((twp->y - twp->h - FONT_HEIGHT < origin[1]) &&
			(twp->y > origin[1] - dh))	// sikk - Mouse Zoom Texture Window
		{
			// if in use, draw a background
			if (twp->tex->inuse)
			{
				glLineWidth(1);
				glColor3f(0.5, 1, 0.5);
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
			if (!_strcmpi(g_qeglobals.d_workTexDef.name, twp->tex->name))
			{
				glLineWidth(3);
				glColor3f(1, 0, 0);
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
			glColor3fv(g_qeglobals.d_savedinfo.v3Colors[COLOR_TEXTURETEXT]);
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
