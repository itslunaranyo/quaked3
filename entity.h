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
};

//==============================

class Entity
{
public:
	Entity();
	~Entity();

	Entity		*prev, *next;
	Brush		brushes;	// head/tail of list - TODO: make this brush the dummy brush for point entities instead of a stupid linked one?
	vec3_t		origin;
	EntClass	*eclass;
	EPair		*epairs;

// sikk---> Undo/Redo
	int			undoId, redoId;
	int			ownerId;	// entityId of the owner entity for undo
	int			entityId;	// entity ID
// <---sikk

	bool	IsPoint() const { return (eclass->IsPointClass()); }
	bool	IsBrush() const { return !(eclass->IsPointClass()); }
	bool	IsWorld() const { return (eclass == EntClass::worldspawn); }
	EntClass	*GetEntClass() const { return eclass; }
	EPair		*GetFirstEPair() const { return epairs; }

	static bool		Create (EntClass *c);
	Entity			*Clone();
	void			LinkBrush (Brush *b);
	static void		UnlinkBrush (Brush *b);

	void	ChangeClassname(EntClass* ec);
	void	ChangeClassname(const char *classname);
	Brush	*MakeBrush();

	// TODO: properly constrain the ways origin is set and stored
	void	SetOrigin(vec3_t org);
	void	SetOriginFromMember();
	void	SetOriginFromKeyvalue();
	void	SetOriginFromBrush();
	void	Move(vec3_t trans);

	void	SetSpawnFlag(int flag, bool on);

	void	SetKeyValue(const char *key, const char *value);
	void	SetKeyValue(const char *key, const float fvalue);
	void	SetKeyValue(const char *key, const int ivalue);
	void	SetKeyValueFVector(const char *key, const vec3_t vec);
	void	SetKeyValueIVector(const char *key, const vec3_t vec);

	EPair	*GetEPair(const char *key) const;

	char	*GetKeyValue(const char *key) const;
	float	GetKeyValueFloat(const char *key) const;
	int		GetKeyValueInt(const char *key) const;
	void 	GetKeyValueVector(const char *key, vec3_t vec) const;

	void 	DeleteKeyValue(const char *key);

	// sikk---> Undo/Redo
	int		MemorySize();
	void	CloseLinks();
	void	AddToList(Entity *list);
	static void	MergeListIntoList(Entity *src, Entity *dest);
	void	RemoveFromList();
	void	FreeEpairs();
	// <---sikk

	// TODO: move to Map class
	static Entity* Find (char *pszKey, char *pszValue);
	static Entity* Find (char *pszKey, int iValue);

	static Entity* Parse (bool onlypairs);
	void	CheckOrigin();
	void	Write(std::ostream & stream, bool use_region);
	//void	Write (FILE *f, bool use_region);
	void	WriteSelected(std::ostream & out);
	//void	WriteSelected (FILE *f);	// sikk - Export Selection (Map/Prefab)
};

//========================================================================



EPair		*ParseEpair ();



#endif
