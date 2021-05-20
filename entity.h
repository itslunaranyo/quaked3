#ifndef __ENTITY_H__
#define __ENTITY_H__
//==============================
//	entity.h
//==============================

class EPair
{
public:
	EPair() : next(nullptr) {};
	EPair(const EPair& other) : next(other.next), key(other.key), value(other.value) {};
	~EPair() {};

	EPair		*next;
	qeBuffer	key, value;

	static EPair *ParseEpair();
	bool		IsTarget();
	bool		IsTargetName();
};

//==============================

class Entity
{
public:
	Entity();
	~Entity();

	Entity		*prev, *next;
	Brush		brushes;	// head/tail of list - TODO: make this brush the dummy brush for point entities instead of a stupid linked one?
	vec3		origin;
	EntClass	*eclass;
	EPair		*epairs;
	int			showflags;

	bool		IsPoint() const { return (eclass->IsPointClass()); }
	bool		IsBrush() const { return !(eclass->IsPointClass()); }
	bool		IsWorld() const { return (eclass == EntClass::worldspawn); }
	EntClass	*GetEntClass() const { return eclass; }
	EPair		*GetFirstEPair() const { return epairs; }

	static bool	Create (EntClass *c);
	Entity		*Clone();
	void		LinkBrush (Brush *b);
	static void	UnlinkBrush (Brush *b);

	void	ChangeClassname(EntClass* ec);
	void	ChangeClassname(const char *classname);
	Brush	*MakeBrush();

	// TODO: properly constrain the ways origin is set and stored
	void	SetOrigin(const vec3 org);
	void	SetOriginFromMember();
	void	SetOriginFromKeyvalue();
	void	SetOriginFromBrush();
	void	Move(const vec3 trans);
	void	Transform(mat4 mat);

	void	SetSpawnFlag(int flag, bool on);
	void	SetSpawnflagFilter();

	void	SetKeyValue(const char *key, const char *value);
	void	SetKeyValue(const char *key, const float fvalue);
	void	SetKeyValue(const char *key, const int ivalue);
	void	SetKeyValueFVector(const char *key, const vec3 vec);
	void	SetKeyValueIVector(const char *key, const vec3 vec);

	EPair	*GetEPair(const char *key) const;

	char	*GetKeyValue(const char *key) const;
	float	GetKeyValueFloat(const char *key) const;
	int		GetKeyValueInt(const char *key) const;
	bool	GetKeyValueVector(const char *key, vec3 &out) const;
	void 	DeleteKeyValue(const char *key);

	bool	IsFiltered() const;
	vec3	GetCenter() const;

	// sikk---> Undo/Redo
	int		MemorySize();
	void	RemoveFromList();
	void	CloseLinks();
	void	AddToList(Entity *list, bool tail = false);
	void	MergeListIntoList(Entity *dest, bool tail = false);
	void	FreeEpairs();
	// <---sikk

	static Entity* Parse (bool onlypairs);
	void	CheckOrigin();
	void	Write(std::ostream & stream, bool use_region);
	void	WriteSelected(std::ostream & out);
};

#endif
