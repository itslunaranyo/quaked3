//==============================
//	TexBrowserRenderer.cpp
//==============================
#include "pre.h"
#include "qe3.h"
#include "TextureGroup.h"
#include "TexBrowserRenderer.h"
#include "TextureView.h"

// a renderer is bound to one view for its entire lifetime
TexBrowserRenderer::TexBrowserRenderer(TextureView &view) : tv(view) {}
TexBrowserRenderer::~TexBrowserRenderer() {}


void TexBrowserRenderer::Draw()
{
	char	*name;
	vec3	txavg;
	int		yTop, tvw, tvh;

	txavg = 0.5f * (g_colors.texBackground + g_colors.texText);
	tvw = tv.GetWidth();
	tvh = tv.GetHeight();

	tv.Arrange();

	glClearColor(g_colors.texBackground[0],
		g_colors.texBackground[1],
		g_colors.texBackground[2],
		0);

	glViewport(0, 0, tvw, tvh);
	glClear(GL_COLOR_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, tvw, tv.GetScroll() - tvh, tv.GetScroll(), -100, 100);
	glEnable(GL_TEXTURE_2D);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	for (auto thumbGroup = tv.layoutGroups.begin(); thumbGroup != tv.layoutGroups.end(); ++thumbGroup)
	{
		// is this wad visible?
		if (thumbGroup->top < tv.GetScroll() - tvh)
			break;

		glLineWidth(1);

		// draw the wad header
		glColor3fv(&g_colors.texText.r);
		glDisable(GL_TEXTURE_2D);

		glRasterPos2f(MARGIN_X + 4, thumbGroup->top - FONT_HEIGHT);
		glCallLists(1, GL_UNSIGNED_BYTE, thumbGroup->folded ? "+" : "-");
		glRasterPos2f(MARGIN_X + FONT_HEIGHT + 4, thumbGroup->top - FONT_HEIGHT);
		glCallLists(thumbGroup->tg->name.length(), GL_UNSIGNED_BYTE, thumbGroup->tg->name.c_str());

		glColor3fv(&txavg.r);
		glBegin(GL_LINE_LOOP);
		glVertex2f(MARGIN_X, thumbGroup->top - FONT_HEIGHT - 4);
		glVertex2f(tvw - MARGIN_X, thumbGroup->top - FONT_HEIGHT - 4);
		glEnd();
		glEnable(GL_TEXTURE_2D);

		if (thumbGroup->folded) continue;

		for (auto thumb = thumbGroup->layout.begin(); thumb != thumbGroup->layout.end(); ++thumb)
		{
			// Is this texture visible?
			if (thumbGroup->top + thumb->y < tv.GetScroll() - tvh)
				break;
			if (thumbGroup->top + thumb->y - thumb->h - FONT_HEIGHT > tv.GetScroll())
				continue;

			yTop = thumbGroup->top + thumb->y;

			// if in use, draw a background
			if (thumb->tex->used)
			{
				glColor3fv(&txavg.r);
				glDisable(GL_TEXTURE_2D);

				glBegin(GL_LINE_LOOP);
				glVertex2f(thumb->x - 1, yTop + 1 - FONT_HEIGHT);
				glVertex2f(thumb->x - 1, yTop - thumb->h - 1 - FONT_HEIGHT);	// sikk - Mouse Zoom Texture Window
				glVertex2f(thumb->x + 1 + thumb->w, yTop - thumb->h - 1 - FONT_HEIGHT);	// sikk - Mouse Zoom Texture Window
				glVertex2f(thumb->x + 1 + thumb->w, yTop + 1 - FONT_HEIGHT);		// sikk - Mouse Zoom Texture Window
				glEnd();

				glEnable(GL_TEXTURE_2D);
			}

			// Draw the texture
			glColor3f(1, 1, 1);
			thumb->tex->glTex.Bind();
			glBegin(GL_QUADS);
			glTexCoord2f(0, 0);
			glVertex2f(thumb->x, yTop - FONT_HEIGHT);
			glTexCoord2f(1, 0);
			glVertex2f(thumb->x + thumb->w, yTop - FONT_HEIGHT);	// sikk - Mouse Zoom Texture Window
			glTexCoord2f(1, 1);
			glVertex2f(thumb->x + thumb->w, yTop - FONT_HEIGHT - thumb->h);	// sikk - Mouse Zoom Texture Window
			glTexCoord2f(0, 1);
			glVertex2f(thumb->x, yTop - FONT_HEIGHT - thumb->h);
			glEnd();

			// draw the selection border
			if (g_qeglobals.d_workTexDef.Tex() == thumb->tex)
			{
				glLineWidth(3);
				glColor3fv(&g_colors.selection.r);
				glDisable(GL_TEXTURE_2D);

				glBegin(GL_LINE_LOOP);
				glVertex2f(thumb->x - 4, yTop - FONT_HEIGHT + 4);
				glVertex2f(thumb->x - 4, yTop - FONT_HEIGHT - thumb->h - 4);	// sikk - Mouse Zoom Texture Window
				glVertex2f(thumb->x + 4 + thumb->w, yTop - FONT_HEIGHT - thumb->h - 4);	// sikk - Mouse Zoom Texture Window
				glVertex2f(thumb->x + 4 + thumb->w, yTop - FONT_HEIGHT + 4);		// sikk - Mouse Zoom Texture Window
				glEnd();

				glEnable(GL_TEXTURE_2D);
				glLineWidth(1);
			}

			// draw the texture name
			glColor3fv(&g_colors.texText.r);
			glDisable(GL_TEXTURE_2D);

			// don't draw the directory name
			for (name = thumb->tex->name.data(); *name && *name != '/' && *name != '\\'; name++)
				;
			if (!*name)
				name = thumb->tex->name.data();
			else
				name++;

			glRasterPos2f(thumb->x, yTop - FONT_HEIGHT + 2);
			glCallLists(strlen(name), GL_UNSIGNED_BYTE, name);
			glEnable(GL_TEXTURE_2D);
		}
	}

	// reset the current texture
	glBindTexture(GL_TEXTURE_2D, 0);
	glFinish();
}

