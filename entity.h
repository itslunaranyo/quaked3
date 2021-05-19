#ifndef __ENTITY_H__
#define __ENTITY_H__
//==============================
//	entity.h
//==============================

typedef struct epair_s
{
	struct epair_s	*next;
	char			*key;
	char			*value;
} epair_t;

class Entity
{
public:
	Entity();
	~Entity();

	Entity		*prev, *next;
	Brush		brushes;	// head/tail of list
	vec3_t		origin;
	EntClass	*eclass;
	epair_t		*epairs;

// sikk---> Undo/Redo
	int			undoId, redoId;
	int			ownerId;	// entityId of the owner entity for undo
	int			entityId;	// entity ID
// <---sikk
};

//========================================================================

extern HWND	g_hwndEnt[ENT_LAST];
extern int	g_nEntDlgIds[ENT_LAST];

//========================================================================

void		SetSpawnFlag(Entity *ent, int flag, bool on);
char		*ValueForKey (Entity *ent, char *key);
void		SetKeyValue (Entity *ent, char *key, char *value);
void		SetKeyValueIVector(Entity *ent, char *key, vec3_t vec);
void		SetKeyValueFVector(Entity *ent, char *key, vec3_t vec);
void 		DeleteKey (Entity *ent, char *key);
float		FloatForKey (Entity *ent, char *key);
int			IntForKey (Entity *ent, char *key);
void 		GetVectorForKey (Entity *ent, char *key, vec3_t vec);

void		Entity_Free (Entity *e);
Entity		*Entity_Parse (bool onlypairs);
void		Entity_Write (Entity *e, FILE *f, bool use_region);
void		Entity_WriteSelected (Entity *e, FILE *f);	// sikk - Export Selection (Map/Prefab)
Entity		*Entity_Create (EntClass *c);
Entity		*Entity_Clone (Entity *e);
void		Entity_LinkBrush (Entity *e, Brush *b);
void		Entity_UnlinkBrush (Brush *b);

void		Entity_ChangeClassname(Entity *ent, char *value);
void		Entity_MakeBrush(Entity *e);
void		Entity_SetOrigin(Entity *ent, vec3_t org);
void		Entity_SetOriginFromMember(Entity *ent);
void		Entity_SetOriginFromKeyvalue(Entity *ent);
void		Entity_SetOriginFromBrush(Entity *ent);

// sikk---> Undo/Redo
int			Entity_MemorySize (Entity *e);
void		Entity_AddToList (Entity *e, Entity *list);
void		Entity_RemoveFromList (Entity *e);
void		Entity_FreeEpairs (Entity *e);
// <---sikk
// sikk---> Cut/Copy/Paste
Entity		*Entity_Copy (Entity *e);	
void		Entity_CleanList ();
// <---sikk
Entity		*FindEntity (char *pszKey, char *pszValue);
Entity		*FindEntityInt (char *pszKey, int iValue);

epair_t	   *ParseEpair ();

int			GetUniqueTargetId (int iHint);



#endif
