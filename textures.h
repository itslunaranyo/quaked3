//==============================
//	textures.h
//==============================

typedef struct
{
	char	name[32];
	float	shift[2];
	float	scale[2];
	float	rotate;

//	int		contents;
//	int		flags;
//	int		value;
} texdef_t;

typedef struct
{
	int			originy;
	int			width, height;
	float		scale;	// sikk - Mouse Zoom Texture Window
	texdef_t	texdef;
} texturewin_t;

typedef struct qtexture_s
{
	struct qtexture_s	*next;
	char	name[64];		// includes partial directory and extension
    int		width, height;
	int		texture_number;	// gl bind number
	vec3_t	color;			// for flat shade mode
	bool	inuse;			// true = is present on the level

//	int			contents;
//	int			flags;
//	int			value;
} qtexture_t;

// a texturename of the form (0 0 0) will
// create a solid color texture

//========================================================================

extern char g_szCurrentWad[1024];
//extern char g_szWadString[1024];	// sikk - Wad Loading

//========================================================================

void Texture_Init ();
void Texture_Flush ();
void Texture_FlushUnused ();
void Texture_MakeNotexture ();
void Texture_ClearInuse ();
void Texture_ShowInuse ();
void Texture_ShowWad (int menunum);
void Texture_InitFromWad (char *file);
void Texture_InitPalette (byte *pal);
void Texture_SetTexture (texdef_t *texdef, bool bSetSelection);	// sikk - Multiple Face Selection: added bSetSelection
void Texture_SetMode (int iMenu);	// GL_NEAREST, etc..
void Texture_MouseDown (int x, int y, int buttons);
void Texture_MouseUp (int x, int y, int buttons);
void Texture_MouseMoved (int x, int y, int buttons);
void Texture_Draw (int width, int height);
void Texture_StartPos ();
qtexture_t *Texture_NextPos (int *x, int *y);
qtexture_t *Texture_LoadTexture (miptex_t *qtex);
qtexture_t *Texture_CreateSolid (char *name);
qtexture_t *Texture_ForName (char *name);

void FillTextureMenu ();

void GetTextureUnderMouse (int mx, int my);
void GetTextureZoomPos (float oldscale, float newscale);

void SetTexParameters ();
void SelectTexture (int mx, int my);
void SortTextures ();
