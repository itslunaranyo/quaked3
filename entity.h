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

class Entity
{
public:
	Entity();
	~Entity();

	Entity		*prev, *next;
	Brush		brushes;	// head/tail of list
	vec3_t		origin;
	EntClass	*eclass;
	EPair		*epairs;

// sikk---> Undo/Redo
	int			undoId, redoId;
	int			ownerId;	// entityId of the owner entity for undo
	int			entityId;	// entity ID
// <---sikk

	static Entity	*Create (EntClass *c);
	Entity			*Clone();
	Entity			*Copy();
	void			LinkBrush (Brush *b);
	static void		UnlinkBrush (Brush *b);

	void	ChangeClassname(EntClass* ec);
	void	ChangeClassname(const char *classname);
	void	MakeBrush();

	void	SetOrigin(vec3_t org);
	void	SetOriginFromMember();
	void	SetOriginFromKeyvalue();
	void	SetOriginFromBrush();

	void	SetSpawnFlag(int flag, bool on);

	void	SetKeyValue(const char *key, const char *value);
	void	SetKeyValue(const char *key, const float fvalue);
	void	SetKeyValue(const char *key, const int ivalue);
	void	SetKeyValueFVector(const char *key, const vec3_t vec);
	void	SetKeyValueIVector(const char *key, const vec3_t vec);

	char	*GetKeyValue(const char *key) const;
	float	GetKeyValueFloat(const char *key) const;
	int		GetKeyValueInt(const char *key) const;
	void 	GetKeyValueVector(const char *key, vec3_t vec) const;

	void 	DeleteKeyValue(const char *key);

	// sikk---> Undo/Redo
	int		MemorySize();
	void	AddToList(Entity *list);
	void	RemoveFromList();
	void	FreeEpairs();
	// <---sikk

	static void CleanCopiedList();
	static Entity* Find (char *pszKey, char *pszValue);
	static Entity* Find (char *pszKey, int iValue);

	static Entity* Parse (bool onlypairs);
	void	Write (FILE *f, bool use_region);
	void	WriteSelected (FILE *f);	// sikk - Export Selection (Map/Prefab)
};

//========================================================================



EPair		*ParseEpair ();



#endif
