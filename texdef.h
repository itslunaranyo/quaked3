//==============================
//	texdef.h
//==============================

#ifndef __TEXDEF_H__
#define __TEXDEF_H__

class Texture;

class TexDef
{
public:
	TexDef() { Reset(); }

	void	Reset();
	void	Set(Texture* stx);
	void	Set(const char* txn);
	void	Set(const std::string& txn);
	void	SetTemp(const std::string_view txn);
	Texture* const Tex();
	const std::string& Name();
	void	Clamp();

	float	shift[2];
	float	scale[2];
	float	rotate;

private:
	dvec3	s, t;
	double	sTrans, sScale, tTrans, tScale;
	std::string	name;		// must be limited to match the WAD2 spec (16)
	Texture	*tex;
		
	//	int		contents;
	//	int		flags;
	//	int		value;
};

#endif
