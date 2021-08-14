#include <GL/glew.h>

#include "pre.h"
#include "cfgvars.h"
#include "GLTexture.h"

const int g_textureModes[] =
{
	GL_NEAREST,
	GL_NEAREST_MIPMAP_NEAREST,
	GL_NEAREST_MIPMAP_LINEAR,
	GL_LINEAR,
	GL_LINEAR_MIPMAP_NEAREST,
	GL_LINEAR_MIPMAP_LINEAR
};


void GLTexture::Make(int w, int h, const char* texData)
{
	glGenTextures(1, &n);

	Bind();

	SetMode(g_cfgUI.TextureMode);

	glTexImage2D(GL_TEXTURE_2D, 0, 3, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, texData);
	glGenerateMipmap(GL_TEXTURE_2D);

	Unbind();
}

void GLTexture::SetMode(const int mode)
{
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, g_textureModes[mode]);

	switch (g_textureModes[mode])
	{
		case GL_NEAREST:
		case GL_NEAREST_MIPMAP_NEAREST:
		case GL_NEAREST_MIPMAP_LINEAR:
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			break;
		case GL_LINEAR:
		case GL_LINEAR_MIPMAP_NEAREST:
		case GL_LINEAR_MIPMAP_LINEAR:
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			break;
	}
}

GLTexture::~GLTexture()
{
	if (n)
		glDeleteTextures(1, &n);
}

void GLTexture::Bind()
{
	glBindTexture(GL_TEXTURE_2D, n);
}

void GLTexture::Unbind()
{
	glBindTexture(GL_TEXTURE_2D, 0);
}
