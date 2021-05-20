//==============================
//	TexDef.cpp
//==============================

#include "pre.h"
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

	int i;

	// lunaran: don't bound a value by repeatedly summing floats
	for (i = 0; i * tex->width < shift[0]; i++);
	shift[0] -= i * tex->width;
	for (i = 0; i * tex->height < shift[1]; i++);
	shift[1] -= i * tex->height;

	for (i = 0; i * tex->width > shift[0]; i--);
	shift[0] -= i * tex->width;
	for (i = 0; i * tex->height > shift[1]; i--);
	shift[1] -= i * tex->height;
}
