//==============================
//	textures.h
//==============================
#ifndef __TEXTURES_H__
#define __TEXTURES_H__

typedef struct qtexture_s
{
	struct qtexture_s	*next;
	char	name[32];		// longer than the WAD2 spec to make room for the (rgb) single color texture hack names
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
	char	name[32];		
//	qtexture_t* tex;		// some day
	float	shift[2];
	float	scale[2];
	float	rotate;

//	int		contents;
//	int		flags;
//	int		value;
} texdef_t;

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
void Texture_SetMode (int iMenu);	// GL_NEAREST, etc..
qtexture_t *Texture_LoadTexture (miptex_t *qtex);
qtexture_t *Texture_CreateSolid (char *name);
qtexture_t *Texture_ForName (char *name);

void FillTextureMenu ();

void SetTexParameters ();

#endif
