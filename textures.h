//==============================
//	textures.h
//==============================
#ifndef __TEXTURES_H__
#define __TEXTURES_H__

class TextureGroup;
struct _finddata_t;

// lunaran - for mapping texture names to addresses without screwing around with std::strings
typedef struct label_s
{
	char n[MAX_TEXNAME];		// matches the WAD2 spec (16)
	label_s() { n[0] = 0; }
	label_s(const char* name) { strncpy(n, name, MAX_TEXNAME); }
	bool operator<(const label_s &other) const { return (strncmp(n, other.n, MAX_TEXNAME) < 0); }
	bool operator==(const label_s &other) const { return (strncmp(n, other.n, MAX_TEXNAME) == 0); }
	bool operator!=(const label_s &other) const { return (strncmp(n, other.n, MAX_TEXNAME) != 0); }
	bool operator<(const char* &other) const { return (strncmp(n, other, MAX_TEXNAME) < 0); }
	bool operator==(const char* &other) const { return (strncmp(n, other, MAX_TEXNAME) == 0); }
	bool operator!=(const char* &other) const { return (strncmp(n, other, MAX_TEXNAME) != 0); }
} label_t;

const int g_textureModes[] =
{
	GL_NEAREST,
	GL_NEAREST_MIPMAP_NEAREST,
	GL_NEAREST_MIPMAP_LINEAR,
	GL_LINEAR,
	GL_LINEAR_MIPMAP_NEAREST,
	GL_LINEAR_MIPMAP_LINEAR
};

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
	void		Use();
private:
	void SetFlags();
};

class TextureGroup
{
public:
	TextureGroup();
	~TextureGroup();

	bool operator< (const TextureGroup& other) const {
		return (strncmp(name, other.name, MAX_WADNAME) < 0);
	}
	static bool sortcmp(const TextureGroup* a, const TextureGroup* b) { return *a < *b; }

	Texture* first;
	int	numTextures;
	int id;
	char name[MAX_WADNAME];
	bool flushed;

	Texture *ForName(const char *name);
	Texture *ByColor(const vec3 oc);
	void ClearUsed();
	void FlushUnused();
	void Flush();
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
	void HandleChange();
	void ReloadAll();
	void MenuReloadAll();
	void MenuReloadGroup(TextureGroup *tg);
	void RefreshUsedStatus();
	void FlushAll();
	void FlushUnused();
	void FlushUnused(TextureGroup *tg);
	void FlushUnusedFromWadstring(const char* wadstring);
	void MenuFlushUnused();

	Texture *CreateSolid(const char *name);
	Texture *MakeNullTexture();
	Texture *ForName(const char *name);

	void LoadWadsFromWadstring(const char *wadstring);
	void LoadWad(const char* wadfile);
	void LoadPalette();
	void MenuLoadWad(const char* menuwad);
	void SetTextureMode(const int mode);
	void SetDrawMode(const int mode);
	void SetParameters();
	void RemoveFromNameMap(TextureGroup* tg);
	void AddToNameMap(TextureGroup* tg);
	void SelectFirstTexture();
	void ReloadGroup(TextureGroup *tg);
	void ReloadGroup(std::vector<TextureGroup*>::iterator tgIt);
	void LibraryChanged();
	void CleanUnknowns();
	void FixWorkTexDef();

	std::vector<_finddata_t> ListWadsInDirectory(const char* wadpath);

	extern std::vector<TextureGroup*> groups;
	extern std::map<label_t, Texture*> texMap;
	extern Texture *nulltexture;

	extern TextureGroup group_unknown;	// bucket for bad textures
	extern TextureGroup group_solid;	// bucket for silly solid color entity textures
};



#endif
