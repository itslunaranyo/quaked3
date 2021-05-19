#ifndef __ENTCLASS_H__
#define __ENTCLASS_H__
//==============================
//	entclass.h
//==============================

#define	MAX_FLAGS	8

class EntClass
{
public:
	EntClass();
	EntClass(const EntClass& other);
	~EntClass();

	char			*name;
	typedef enum {
		ECF_POINT = 0x01,
		ECF_BRUSH = 0x02,
		ECF_HACKED = 0x04,	// is supposed to be the opposite form
		ECF_UNKNOWN = 0x08
	} eclass_form_e;

	int				form;
	vec3			mins, maxs;
	vec3			color;
	texdef_t		texdef;
	qeBuffer		comments;
	char			flagnames[MAX_FLAGS][32];
	unsigned int	nShowFlags;

	bool			IsPointClass() { return !!(form & ECF_POINT); }
	static EntClass	*ForName(const char *name, bool has_brushes, bool strict);
	void			AddToClassList();

	static void		InitForSourceDirectory(const char *path);
	static void		Clear();
	static void		Sort();
	static bool		eccmp(const EntClass* a, const EntClass* b) { return (strcmp(a->name, b->name) < 0); }
	static std::vector<EntClass*>::iterator begin() { return entclasses.begin(); }
	static std::vector<EntClass*>::iterator end() { return entclasses.end(); }
	static EntClass	*worldspawn;

private:
	static void		ScanFile(const char *filename);
	static EntClass	*InitFromText(char *text);
	static EntClass *CreateOppositeForm(EntClass* e);

	static std::vector<EntClass*> pointclasses;
	static std::vector<EntClass*> brushclasses;
	static std::vector<EntClass*> entclasses;
	static EntClass	*badclass;
};


#endif
