#ifndef __ENTCLASS_H__
#define __ENTCLASS_H__
//==============================
//	entclass.h
//==============================

#include "TexDef.h"
#include <map>

class EntClass
{
public:
	EntClass();
	EntClass(std::string_view& _n);
	EntClass(const EntClass& other);
	~EntClass();

	std::string		name;
	vec3			mins, maxs;
	vec3			color;
	TexDef			texdef;	// TODO: god this is unneccesary
	static const int MAX_FLAGS = 16;
	std::string		flagnames[MAX_FLAGS];
	std::string		comments;

	enum eclass_form_e {
		ECF_POINT	= 0x0001,
		ECF_BRUSH	= 0x0002,
		ECF_HACKED	= 0x0004,	// is supposed to be the opposite form
		ECF_UNKNOWN	= 0x0008,
		ECF_ANGLE	= 0x0010
	};
	unsigned int	form, showFlags;

	bool			IsPointClass() const { return !!(form & ECF_POINT); }

	static EntClass* ForName(const std::string& name, const bool has_brushes, const bool strict);
	static const EntClass* Worldspawn() { return worldspawn; }

	static std::vector<EntClass*>::const_iterator cbegin() { return entclasses.cbegin(); }
	static std::vector<EntClass*>::const_iterator cend() { return entclasses.cend(); }

private:
	friend class EntClassInitializer;

	static void		Reset();
	static void		Sort();
	static EntClass *CreateOppositeForm(EntClass* e);

	void			AddToClassList();

	static std::map<std::string, EntClass*> pointclasses;
	static std::map<std::string, EntClass*> brushclasses;
	static std::vector<EntClass*> entclasses;
	static EntClass *worldspawn;
	static EntClass	*badclass;
	static std::vector<EntClass*>::iterator begin() { return entclasses.begin(); }
	static std::vector<EntClass*>::iterator end() { return entclasses.end(); }
};


#endif
