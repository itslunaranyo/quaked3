//==============================
//	textures.h
//==============================
#ifndef __TEXTURES_H__
#define __TEXTURES_H__

class TextureGroup;

// lunaran - for mapping texture names to addresses without screwing around with std::strings
typedef struct label_s
{
	char n[MAX_TEXNAME];		// matches the WAD2 spec (16)
	label_s() { n[0] = 0; }
	label_s(const char* name) { strncpy(n, name, MAX_TEXNAME); }
	bool operator<(const label_s &other) const { return (strncmp(n, other.n, MAX_TEXNAME) < 0); }
	bool operator==(const label_s &other) const { return (strncmp(n, other.n, MAX_TEXNAME) == 0); }
} label_t;



class Texture
{
public:
	Texture(int w, int h, const char* n, const vec3 c, int gltex);
	~Texture() {};

	Texture		*next;
	char		name[MAX_TEXNAME];		// matches the WAD2 spec (16)
    int			width, height;
	int			texture_number;	// gl bind number
	vec3		color;			// for flat shade mode
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
	Texture *ByColor(const vec3 oc);
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
	int MakeGLTexture(int w, int h, qeBuffer &data);	// lunaran FIXME: temporary, make part of gltexture class
private:
	bool ReadWad(const char* filename, qeBuffer &wadFileBuf);
	TextureGroup *ParseWad(qeBuffer &wadFileBuf);
	void MiptexToRGB(miptex_t *mip, qeBuffer &texDataBuf, vec3 &avg);
};



//========================================================================

#include <map>

namespace Textures
{
	void Init();
	void Flush();
	void FlushUnused(bool rebuild = true);
	void RefreshUsedStatus();

	Texture *CreateSolid(const char *name);
	Texture *MakeNullTexture();
	Texture *ForName(const char *name);

	void LoadWad(const char* wadfile);
	void MenuLoadWad(const int menunum);
	void SetRenderMode(const int menunum);
	void SetParameters();
	void RemoveFromNameMap(TextureGroup* tg);
	void AddToNameMap(TextureGroup* tg);
	void SelectFirstTexture();

	extern std::vector<TextureGroup*> groups;
	extern std::map<label_t, Texture*> texMap;
	extern Texture *nulltexture;

	extern TextureGroup group_unknown;	// bucket for bad textures
	extern TextureGroup group_solid;	// bucket for silly solid color entity textures
};


class TexDef
{
public:
	TexDef();

	void	Set(Texture *stx) { tex = stx; strncpy(name, tex->name, MAX_TEXNAME); stx->used = true; }
	void	Set(const char* txn) { strncpy(name, txn, MAX_TEXNAME); tex = Textures::ForName(name); }
	void	Clamp();

	Texture	*tex;
	char	name[MAX_TEXNAME];		// matches the WAD2 spec (16)
	float	shift[2];
	float	scale[2];
	float	rotate;

	//	int		contents;
	//	int		flags;
	//	int		value;
};



// as yet orphan bs:

void SetTexParameters ();
void Texture_SetMode (int iMenu);	// GL_NEAREST, etc..
void FillTextureMenu ();


#endif
