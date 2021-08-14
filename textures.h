//==============================
//	textures.h
//==============================
#ifndef __TEXTURES_H__
#define __TEXTURES_H__

#include <map>

class TextureGroup;
class Texture;

namespace Textures
{
	extern std::vector<TextureGroup*> groups;
	extern std::map<std::string, Texture*> texMap;
	extern Texture *nulltexture;

	extern TextureGroup group_unknown;	// bucket for bad textures
	extern TextureGroup group_solid;	// bucket for silly solid color entity textures

	void Init();
	void LibraryChanged();
	void HandleChange();
	void ReloadAll();
	void RefreshUsedStatus();
	void FlushAll();
	void FlushUnused();
	void FlushUnused(TextureGroup* tg);
	void FlushUnusedFromWadstring(const std::string& wadstring);

	Texture* CreateSolid(const std::string& name);
	Texture* MakeNullTexture();
	Texture* ForName(const std::string& name);

	void MergeWadStrings(const std::string& wad1, const std::string& wad2, std::string& out);
	void LoadWadsFromWadstring(const std::string& wadstring);
	void LoadWad(const std::string& wadfile);
	void LoadPalette();
	void RemoveFromNameMap(TextureGroup* tg);
	void AddToNameMap(TextureGroup* tg);
	void SelectFirstTexture();
	void ReloadGroup(TextureGroup* tg);
	void ReloadGroup(std::vector<TextureGroup*>::iterator tgIt);
	void CleanUnknowns();
	void FixWorkTexDef();

	void MenuLoadWad(const std::string& menuwad);
	void MenuReloadAll();
	void MenuReloadGroup(TextureGroup* tg);
	void MenuFlushUnused();

	void SetTextureMode(const int mode);
	void SetDrawMode(const int mode);
};



#endif
