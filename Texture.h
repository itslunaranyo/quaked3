#pragma once

#include "mathlib.h"
#include "GLTexture.h"
#include <string>

//constexpr int MAX_TEXNAME = 16;

class Texture
{
public:
	Texture(int w, int h, const std::string& n, const vec3 c, const char* texData);
	~Texture() {};

	Texture*	next;
	std::string	name;		// must remain limited to the WAD2 spec (16)
	int			width, height;
	GLTexture	glTex;		// gl bind
	vec3		color;			// for flat shade mode
	bool		used;			// true = is present on the level
	unsigned	showflags;

	void	Use();
private:
	void	SetFlags();
};
