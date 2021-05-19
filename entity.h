//==============================
//	entity.h
//==============================

#define	MAX_FLAGS	8

typedef struct eclass_s
{
	struct eclass_s	   *next;
	char			   *name;
	bool				fixedsize;
	bool				unknown;		// wasn't found in source
	vec3_t				mins, maxs;
	vec3_t				color;
	texdef_t			texdef;
	char			   *comments;
	char				flagnames[MAX_FLAGS][32];
	unsigned int		nShowFlags;
} eclass_t;

typedef struct epair_s
{
	struct epair_s	*next;
	char			*key;
	char			*value;
} epair_t;

typedef struct entity_s
{
	struct entity_s	   *prev, *next;
	brush_t				brushes;	// head/tail of list
	vec3_t				origin;
	eclass_t		   *eclass;
	epair_t			   *epairs;
// sikk---> Undo/Redo
	int					undoId;		// undo ID		
	int					redoId;		// redo ID
	int					ownerId;	// entityId of the owner entity for undo
	int					entityId;	// entity ID
// <---sikk
} entity_t;

//========================================================================

extern eclass_t	*g_pecEclass;

extern HWND	g_hwndEnt[ENT_LAST];
extern int	g_nEntDlgIds[ENT_LAST];

//========================================================================

eclass_t   *Eclass_InitFromText (char *text);
void		Eclass_InitForSourceDirectory (char *path);
void		Eclass_InsertAlphabetized (eclass_t *e);
eclass_t   *Eclass_ForName (char *name, bool has_brushes);
void		Eclass_ScanFile (char *filename);

void		SetSpawnFlag(entity_t *ent, int flag, bool on);
char	   *ValueForKey (entity_t *ent, char *key);
void		SetKeyValue (entity_t *ent, char *key, char *value);
void 		DeleteKey (entity_t *ent, char *key);
float		FloatForKey (entity_t *ent, char *key);
int			IntForKey (entity_t *ent, char *key);
void 		GetVectorForKey (entity_t *ent, char *key, vec3_t vec);
void		Entity_Free (entity_t *e);
entity_t   *Entity_Parse (bool onlypairs);
void		Entity_Write (entity_t *e, FILE *f, bool use_region);
void		Entity_WriteSelected (entity_t *e, FILE *f);	// sikk - Export Selection (Map/Prefab)
entity_t   *Entity_Create (eclass_t *c);
entity_t   *Entity_Clone (entity_t *e);
void		Entity_LinkBrush (entity_t *e, brush_t *b);
void		Entity_UnlinkBrush (brush_t *b);
// sikk---> Undo/Redo
int			Entity_MemorySize (entity_t *e);
void		Entity_AddToList (entity_t *e, entity_t *list);
void		Entity_RemoveFromList (entity_t *e);
void		Entity_FreeEpairs (entity_t *e);
// <---sikk
// sikk---> Cut/Copy/Paste
entity_t   *Entity_Copy (entity_t *e);	
void		Entity_CleanList ();
// <---sikk
entity_t   *FindEntity (char *pszKey, char *pszValue);
entity_t   *FindEntityInt (char *pszKey, int iValue);

epair_t	   *ParseEpair ();

int			GetUniqueTargetId (int iHint);

void		FillEntityListbox (HWND hwnd, int pointbased, int brushbased);	// sikk - Create Entity Dialog

bool		IsBrushSelected (brush_t* bSel);	// sikk - Export Selection (Map/Prefab)

