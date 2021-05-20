//==============================
//	texdef.h
//==============================

#ifndef __TEXDEF_H__
#define __TEXDEF_H__

class TexDef
{
public:
	TexDef();

	void	Set(Texture *stx) { tex = stx; strncpy(name, tex->name, MAX_TEXNAME); }
	void	Set(const char* txn) { strncpy(name, txn, MAX_TEXNAME); tex = Textures::ForName(name); }
	void	Clamp();

	Texture	*tex;
	char	name[MAX_TEXNAME];		// matches the WAD2 spec (16)
	float	shift[2];
	float	scale[2];
	float	rotate;

	vec3	sv, tv;
	//	int		contents;
	//	int		flags;
	//	int		value;
};

#endif
