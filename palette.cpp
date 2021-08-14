//==============================
//	palette.cpp
//==============================

#include "pre.h"
#include "mathlib.h"
#include "palette.h"

Palette g_palette;

vec3 Palette::ColorAsVec3(const byte i)
{
	vec3 c;
	c[0] = pal[i].r / 255.0f;
	c[1] = pal[i].g / 255.0f;
	c[2] = pal[i].b / 255.0f;
	return c;
}

