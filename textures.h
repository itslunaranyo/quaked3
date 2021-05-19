//==============================
//	textures.h
//==============================

typedef struct qtexture_s
{
	struct qtexture_s	*next;
	char	name[16];		// lunaran: changed to 16 to match the .WAD2 spec
    int		width, height;
	int		texture_number;	// gl bind number
	vec3_t	color;			// for flat shade mode
	bool	inuse;			// true = is present on the level
	char*	wad;			// lunaran: wad the texture was sourced from, for sorting (just points to the menu labels)
//	int			contents;
//	int			flags;
//	int			value;
} qtexture_t;

typedef struct
{
	char	name[16];		// lunaran: changed to 16 to match the .WAD2 spec
//	qtexture_t* tex;		// some day
	float	shift[2];
	float	scale[2];
	float	rotate;

//	int		contents;
//	int		flags;
//	int		value;
} texdef_t;

typedef struct
{
	int x, y, w, h;
	qtexture_t* tex;
} texWndPlacement_t;

typedef struct
{
	int			originy;
	int			width, height;
	float		scale;	// sikk - Mouse Zoom Texture Window
	//texdef_t	texdef;

	// lunaran: cached layout
	int			length;
	texWndPlacement_t* layout;
	int			count;
} texturewin_t;

// a texturename of the form (0 0 0) will
// create a solid color texture

//========================================================================

void Texture_Init ();
void Texture_FlushAll ();
void Texture_FlushUnused ();
void Texture_MakeNotexture ();
void Texture_ClearInuse ();
void Texture_ShowInuse ();
void Texture_ShowWad (int menunum);
void Texture_InitFromWad (char *file);
void Texture_InitPalette (byte *pal);
void Texture_ChooseTexture (texdef_t *texdef, bool bSetSelection);	// sikk - Multiple Face Selection: added bSetSelection
void Texture_SetMode (int iMenu);	// GL_NEAREST, etc..
void TexWnd_MouseDown (int x, int y, int buttons);
void TexWnd_MouseUp (int x, int y, int buttons);
void TexWnd_MouseMoved (int x, int y, int buttons);
void TexWnd_Draw (int width, int height);
qtexture_t *Texture_LoadTexture (miptex_t *qtex);
qtexture_t *Texture_CreateSolid (char *name);
qtexture_t *Texture_ForName (char *name);

void FillTextureMenu ();
void TexWnd_Layout();
void TexWnd_SetScale(float scale);
qtexture_t* TexWnd_TexAtPos (int wx, int wy);

void SetTexParameters ();
void TexWnd_SelectTexture (int mx, int my);
void SortTextures ();
