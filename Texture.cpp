#include "pre.h"
#include "qedefs.h"
#include "qfiles.h"
#include "strlib.h"
#include "Texture.h"


/*
==================
Texture::Texture
==================
*/
Texture::Texture(int w, int h, const std::string& n, vec3 c, const char* texData) :
	next(nullptr), name(n), width(w), height(h), used(false), color(c)
{
	glTex.Make(w, h, texData);
	if (name.size() > MAX_TEXNAME)
	{
		name.resize(MAX_TEXNAME);
		name[MAX_TEXNAME - 1] = 0;
	}
	strlib::ToLower(name);
	SetFlags();
}

/*
==================
Texture::SetFlags
==================
*/
void Texture::SetFlags()
{
	showflags = 0;

	if (name._Starts_with("*"))
		showflags |= BFL_LIQUID | BFL_TRANS;
	else if (name._Starts_with("sky"))
		showflags |= BFL_SKY;
	else if (name._Starts_with("clip"))
		showflags |= BFL_CLIP | BFL_TRANS;
	else if (name._Starts_with("hint"))
		showflags |= BFL_HINT | BFL_TRANS;
	else if (name._Starts_with("skip"))
		showflags |= BFL_SKIP;
	else if (name._Starts_with("trigger"))
		showflags |= BFL_TRANS;
}
