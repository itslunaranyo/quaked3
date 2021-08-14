//==============================
//	texdef.h
//==============================

#ifndef __TEXDEF_H__
#define __TEXDEF_H__

class Texture;

class TexDef
{
public:
	TexDef();

	void	Set(Texture* stx);
	void	Set(const char* txn);
	void	Set(const std::string& txn);
	void	Clamp();

	Texture	*tex;
	std::string	name;		// must be limited to match the WAD2 spec (16)
	float	shift[2];
	float	scale[2];
	float	rotate;

	vec3	sv, tv;
	//	int		contents;
	//	int		flags;
	//	int		value;
};

#endif
