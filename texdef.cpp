//==============================
//	TexDef.cpp
//==============================

#include "pre.h"
#include "qe3.h"
#include "texture.h"
#include "Textures.h"
#include "texdef.h"

void TexDef::Reset()
{
	tex = nullptr;
	name = "";
	rotate = { 0 };
	shift[0] = shift[1] = 0;
	scale[0] = scale[1] = g_qeglobals.d_fDefaultTexScale;
}

void TexDef::Set(Texture* stx)
{
	tex = stx; 
	name = tex->name;
}
void TexDef::Set(const char* txn)
{
	name = std::string(txn); 
	tex = Textures::ForName(name);
}
void TexDef::Set(const std::string& txn)
{
	name = txn; 
	tex = Textures::ForName(name);
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
