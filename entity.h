#ifndef __ENTITY_H__
#define __ENTITY_H__
//==============================
//	entity.h
//==============================
#include "mathlib.h"

class EntClass;
class EPair;

class Entity
{
public:
	Entity();
	~Entity();

	EntClass*	eclass;
	Brush		brushes;	// head/tail of list - TODO: make this brush the dummy brush for point entities instead of a stupid linked one?
	EPair		*epairs;
	int			showflags;

	inline Entity* Next() const { return next; }
	inline Entity* Prev() const { return prev; }
private:
	Entity		*prev, *next;

public:
	bool		IsPoint() const;
	bool		IsBrush() const;
	bool		IsWorld() const;
	const EntClass	*GetEntClass() const { return eclass; }
	EPair		*GetFirstEPair() const { return epairs; }

	static bool	Create (EntClass *c);
	Entity		*Clone();
	void		LinkBrush (Brush *b, bool tail = false);
	static void	UnlinkBrush (Brush *b, bool preserveOwner = false);

	void	ChangeClass(EntClass* ec);
	void	ChangeClassname(const std::string& classname);
	Brush	*MakeBrush();

	vec3	GetOrigin() const;
	void	SetOrigin(const vec3 org);
	void	SetOriginFromKeyvalue();
	void	SetOriginFromBrush();
	void	Transform(mat4 mat);

	void	SetSpawnFlag(int flag, bool on);
	void	SetSpawnflagFilter();

	void	SetKeyValue(const std::string& key, const std::string& value);
	void	SetKeyValue(const std::string& key, const float fvalue);
	void	SetKeyValue(const std::string& key, const int ivalue);
	void	SetKeyValueFVector(const std::string& key, const vec3 vec);
	void	SetKeyValueIVector(const std::string& key, const vec3 vec);

	EPair	*GetEPair(const std::string&key) const;

	std::string GetKeyValue(const std::string& key) const;
	float	GetKeyValueFloat(const std::string& key) const;
	int		GetKeyValueInt(const std::string& key) const;
	bool	GetKeyValueVector(const std::string& key, vec3 &out) const;
	void 	DeleteKeyValue(const std::string& key);

	bool	IsFiltered() const;
	vec3	GetCenter() const;

	void	RemoveFromList();
	bool	IsLinked() const;
	void	AddToList(Entity *list, bool tail = false);
	void	MergeListIntoList(Entity *dest, bool tail = false);
	void	FreeEpairs();
};

#endif
