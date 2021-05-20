//==============================
//	TexDef.cpp
//==============================

#include "qe3.h"


TexDef::TexDef() : tex(nullptr), shift(), rotate(0)
{
	name[0] = 0;
	scale[0] = scale[1] = g_qeglobals.d_fDefaultTexScale;
}

void TexDef::Clamp()
{
	if (!tex)
	{
		name[0] = 0;
		return;
	}

	while (shift[0] > tex->width)
		shift[0] -= tex->width;
	while (shift[1] > tex->height)
		shift[1] -= tex->height;
	while (shift[0] < 0)
		shift[0] += tex->width;
	while (shift[1] < 0)
		shift[1] += tex->height;
}
