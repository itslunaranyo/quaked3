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
	tex->Use();
}
void TexDef::Set(const char* txn)
{
	name = std::string(txn); 
	tex = Textures::ForName(name);
	tex->Use();
}
void TexDef::Set(const std::string& txn)
{
	name = txn; 
	tex = Textures::ForName(name);
	tex->Use();
}
void TexDef::SetTemp(const std::string_view txn)
{
	name = txn;
	tex = nullptr;
}

Texture* const TexDef::Tex()
{
	if (!tex)
	{
		if (name.empty())
			name = "none";
		tex = Textures::ForName(name);
	}
	return tex;
}

const std::string& TexDef::Name()
{
	if (name.empty())
		name = "none";
	return name;
}

void TexDef::Clamp()
{
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
