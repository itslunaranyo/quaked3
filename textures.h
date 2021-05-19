//==============================
//	textures.h
//==============================
#ifndef __TEXTURES_H__
#define __TEXTURES_H__

class TextureGroup;

// lunaran - for mapping texture names to addresses without screwing around with std::strings
typedef struct label_s
{
	char n[16];
	label_s() { n[0] = 0; }
	label_s(const char* name) { strncpy(n, name, 16); }
	bool operator<(const label_s &other) const { return (strncmp(n, other.n, 16) < 0); }
	bool operator==(const label_s &other) const { return (strncmp(n, other.n, 16) == 0); }
} label_t;



class Texture
{
public:
	Texture(int w, int h, const char* n, vec3_t c, int gltex);
	~Texture() {};

	Texture		*next;
	char		name[32];		// longer than the WAD2 spec to make room for the (rgb) single color texture hack names
    int			width, height;
	int			texture_number;	// gl bind number
	vec3_t		color;			// for flat shade mode
	bool		used;			// true = is present on the level
	unsigned	showflags;

private:
	void SetFlags();
};

class TextureGroup
{
public:
	TextureGroup();
	~TextureGroup();

	bool operator< (const TextureGroup& other) const {
		return (strncmp(name, other.name, 32) < 0);
	}
	static bool sortcmp(const TextureGroup* a, const TextureGroup* b) { return *a < *b; }

	Texture* first;
	int	numTextures;
	char name[32];

	Texture *ForName(const char *name);
	Texture *ByColor(vec3_t oc);
	void ClearUsed();
	void FlushUnused();
	void Add(Texture* tx);
private:
};

class WadLoader
{
public:
	WadLoader() {};
	~WadLoader() {};

	TextureGroup* LoadTexturesFromWad(const char* filename);
	int MakeGLTexture(int w, int h, qeBuffer &data);	// FIXME: temporary, make part of gltexture class
private:
	bool ReadWad(const char* filename, qeBuffer &wadFileBuf);
	TextureGroup *ParseWad(qeBuffer &wadFileBuf);
	void MiptexToRGB(miptex_t *mip, qeBuffer &texDataBuf, vec3_t avg);
};


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

#include <vector>
#include <map>

namespace Textures
{
	void Init();
	void Flush();
	void FlushUnused();
	void ClearUsed();

	Texture *CreateSolid(const char *name);
	Texture *MakeNullTexture();
	Texture *ForName(const char *name);

	void LoadWad(const char* wadfile);
	void MenuLoadWad(const int menunum);
	void SetRenderMode(const int menunum);
	void SetParameters();
	void RemoveFromNameMap(TextureGroup* tg);
	void AddToNameMap(TextureGroup* tg);

	extern std::vector<TextureGroup*> groups;
	extern std::map<label_t, Texture*> texMap;
	extern Texture *nulltexture;

	extern TextureGroup group_unknown;	// bucket for bad textures
	extern TextureGroup group_solid;	// bucket for silly solid color entity textures
};

// as yet orphan bs:

void SetTexParameters ();
void Texture_SetMode (int iMenu);	// GL_NEAREST, etc..
void FillTextureMenu ();


#endif
