//==============================
//	textures.cpp
//==============================

#include "pre.h"
#include "qe3.h"
#include "cfgvars.h"
#include "Tokenizer.h"
#include "StringFormatter.h"
#include "IO.h"
#include "PaletteReader.h"
#include "Texture.h"
#include "TextureGroup.h"
#include "Textures.h"
#include "Map.h"
#include "select.h"
#include "WadReader.h"
#include "TextureView.h"
#include "WndMain.h"
#include <algorithm>


int		g_nTextureMode = GL_LINEAR_MIPMAP_LINEAR; // default
int		g_nTextureExtensionNumber = 1;
bool	g_bTexturesChanged = false;

//=====================================================

Texture						*Textures::nulltexture;
std::vector<TextureGroup*>	Textures::groups;
std::map<std::string, Texture*>	Textures::texMap;
TextureGroup				Textures::group_solid;
TextureGroup				Textures::group_unknown;

/*
==================
Textures::Init
==================
*/
void Textures::Init()
{
	Log::Print("Initializing textures\n");

	LoadPalette();

	// prepare null texture
	MakeNullTexture();
	if (!g_qeglobals.d_workTexDef.tex)
		g_qeglobals.d_workTexDef.Set(nulltexture);

	// create solid color group
	group_solid.name = "*solid";
	// * is invalid as a filename character, so a theoretical wad named "solid" can't collide
	
	//g_qeglobals.d_qtextures = nullptr;
}



/*
==================
Textures::LibraryChanged
==================
*/
void Textures::LibraryChanged()
{
	g_bTexturesChanged = true;
	g_vTexture.Refresh();
}

/*
==================
Textures::HandleChange

some texture housekeeping is best done post-frame so it isn't repeated
wastefully, ie after each wad loaded in sequence
==================
*/
void Textures::HandleChange()
{
	if (!g_bTexturesChanged) return;

	// 1) remove dead unknown textures that may have been replaced by real textures
	Textures::CleanUnknowns();
	// 2) remove any dead icons in the texture browser
	g_vTexture.Refresh();
	// 3) sort out what texture is selected/bound
	Textures::FixWorkTexDef();
	// 2 comes before 3 because FWTD may select the first texture in the browser as a
	// default, but if the map is new and all textures are flushed, that first texture
	// is a null reference

	WndMain_UpdateWindows(W_CAMERA | W_TEXTURE | W_SURF);

	g_bTexturesChanged = false;
}


/*
==================
Textures::CleanUnknowns

clean up old unknown textures since they're not owned by a wad, and thus not
cleaned up the normal way

TODO: interfaces exist to solve this problem
==================
*/
void Textures::CleanUnknowns()
{
	Texture **tex, *temp;

	if (group_unknown.numTextures > 0)
	{
		for (tex = &group_unknown.first; *tex; )
		{
			if (*tex != texMap[(*tex)->name])
			{
				temp = *tex;
				*tex = (*tex)->next;
				delete temp;
				group_unknown.numTextures--;
			}
			else
			{
				tex = &(*tex)->next;
			}
		}
	}
}

/*
==================
Textures::FixWorkTexDef

keep texture name in surface inspector, texture pointer in worktexdef, and selected
texture in the texture browser all valid and in sync

TODO: interfaces exist to solve this problem
==================
*/
void Textures::FixWorkTexDef()
{
	if (g_qeglobals.d_workTexDef.tex == nullptr && texMap[g_qeglobals.d_workTexDef.name] != nullptr)
	{
		// desired workdef texture was bad but is now in the name map, select it
		g_qeglobals.d_workTexDef.Set(g_qeglobals.d_workTexDef.name);
		g_vTexture.ChooseTexture(&g_qeglobals.d_workTexDef);
	}
	else if (g_qeglobals.d_workTexDef.name[0] == 0 || !texMap[g_qeglobals.d_workTexDef.name])
	{
		// no texture is named or selected, default to the first one in the list
		g_qeglobals.d_workTexDef.tex = nullptr;
		g_vTexture.ChooseFirstTexture();
		g_vTexture.ResetScroll();
	}
}


//=====================================================



/*
==================
Textures::FlushAll
==================
*/
void Textures::FlushAll()
{
	g_map.numTextures = 0;

	// delete all texture groups
	for (auto tgIt = groups.begin(); tgIt != groups.end(); tgIt++)
		delete *tgIt;
	groups.clear();
	group_unknown.Flush();
	g_qeglobals.d_workTexDef.name = "";
	g_qeglobals.d_workTexDef.tex = nullptr;
	Textures::texMap.clear();
	LibraryChanged();
}

/*
==================
Textures::FlushUnused
==================
*/
void Textures::FlushUnused()
{
	group_unknown.FlushUnused();
	for (auto tgIt = groups.begin(); tgIt != groups.end(); tgIt++)
	{
		(*tgIt)->FlushUnused();
	}
	
	LibraryChanged();
}

/*
==================
Textures::FlushUnused
==================
*/
void Textures::FlushUnused(TextureGroup *tg)
{
	if (!tg) return;
	RefreshUsedStatus();
	tg->FlushUnused();

	LibraryChanged();
}


/*
==================
Textures::FlushUnusedFromWadstring
==================
*/
void Textures::FlushUnusedFromWadstring(const std::string& wadstring)
{
	std::string w = wadstring;
	std::replace(w.begin(), w.end(), ';', ' ');
	Tokenizer tkr(w);

	while(!tkr.AtEnd())
	{
		auto wad = tkr.Next();
		for (auto tgIt = groups.begin(); tgIt != groups.end(); tgIt++)
		{
			if (wad == (*tgIt)->name)
				(*tgIt)->FlushUnused();
		}
	}

	LibraryChanged();
}

/*
==================
Textures::RefreshUsedStatus
==================
*/
void Textures::RefreshUsedStatus()
{
	g_map.numTextures = 0;

	// call clearused on all texture groups
	for (auto tgIt = groups.begin(); tgIt != groups.end(); tgIt++)
		(*tgIt)->ClearUsed();
	group_unknown.ClearUsed();	// don't forget meee

	// refresh texdefs to reset used status
	// TODO: this should go anywhere else?
	Brush* b;
	for (b = g_brSelectedBrushes.Next(); b != &g_brSelectedBrushes; b = b->Next())
		b->RefreshTexdefs();
	for (b = g_map.brActive.Next(); b != &g_map.brActive; b = b->Next())
		b->RefreshTexdefs();
	for (b = g_map.brRegioned.Next(); b != &g_map.brRegioned; b = b->Next())
		b->RefreshTexdefs();

	g_vTexture.Refresh();
}



//=====================================================




/*
==================
Textures::CreateSolid

lunaran TODO: do we really still need a 1px texture for a color, or can we just
	render vtx color on a universal white texture
==================
*/
Texture *Textures::CreateSolid(const std::string& name)
{
	int			gltex = 0;
	WadReader	wl;
	vec3		color;
	byte		data[4];
	Texture		*solidtex;

	// make solid texture from name
	//sscanf(name, "#%f %f %f", &color[0], &color[1], &color[2]);
	strlib::HexToRGB(name, color);

	data[0] = color[0];// * 255;
	data[1] = color[1];// * 255;
	data[2] = color[2];// * 255;
	data[3] = 1;

	solidtex = new Texture(1, 1, name, color, (char*)data);

	// stash in the solid color group
	group_solid.Add(solidtex);
	return solidtex;
}

/*
==================
Textures::MakeNullTexture

one texture map that is universally pointed to by all broken texture entries
==================
*/
Texture *Textures::MakeNullTexture()
{
	int			gltex = 0;
	WadReader	wl;
	vec3		color;
	byte		data[64 * 64 * 4] = { 0 };
	int			x, y, ofs;

	// hot programmer pink checkerboard
	for (x = 0; x < 32; x++) for (y = 0; y < 32; y++)
	{
		ofs = 4 * (x * 64 + y);
		data[ofs] = 255;
		data[ofs + 2] = 255;
		data[ofs + 3] = 255;

		ofs = 4 * ((x+32) * 64 + y + 32);
		data[ofs] = 255;
		data[ofs + 2] = 255;
		data[ofs + 3] = 255;
	}
	
	color[0] = color[2] = 128;
	color[1] = 0;

	nulltexture = new Texture(64, 64, "nulltexture", color, (char*)data);
	return nulltexture;
}

/*
==================
Textures::ForName

keeping every wad in a separate group means textures with matching names
never clash, which would be great if not for the fact that it's inconsistent with
the way qbsp behaves (where textures loaded later override those loaded earlier).
an unordered_map speeds up lookups, and also provides late-stomps-early as a
convenient side effect.
==================
*/
Texture *Textures::ForName(const std::string& name)
{
	Texture* tx;
	std::string namelc = name;
	strlib::ToLower(namelc);

	// keep the solid textures out of the main texture space
	if (name[0] == '#')
	{
		// don't use ForName here - if it's not in the hash map, it's strcmp city
		vec3 color;
		strlib::HexToRGB(name, color);
		tx = group_solid.ByColor(color);
		if (!tx)
		{
			tx = CreateSolid(name);
		}
		return tx;
	}

	// check the global texture name map
	tx = texMap[namelc];
	if (tx)
	{
		tx->Use();
		return tx;
	}

	// still not found, make wrapper for nulltexture and put it in the unknown wad
	tx = new Texture(*nulltexture);
	tx->name = namelc;
	tx->next = nullptr;

	group_unknown.Add(tx);
	texMap[namelc] = tx;

	LibraryChanged();
	return tx;
}

/*
==================
Textures::RemoveFromNameMap

do before deleting a texturegroup that is potentially used

the null indexes are okay because they'll turn into notextures in 
the unknown group if they're used again
==================
*/
void Textures::RemoveFromNameMap(TextureGroup* tg)
{
	for (Texture* q = tg->first; q; q = q->next)
	{
		texMap[q->name] = nullptr;
		if (q->used)
			g_map.numTextures--;
	}
	
	LibraryChanged();
}

/*
==================
Textures::AddToNameMap
==================
*/
void Textures::AddToNameMap(TextureGroup* tg)
{
	Texture* old;
	for (Texture* q = tg->first; q; q = q->next)
	{
		old = texMap[q->name];
		texMap[q->name] = q;
	}

	LibraryChanged();
}

/*
==================
Textures::SelectFirstTexture
FIXME: belongs in the view?
==================
*/
void Textures::SelectFirstTexture()
{
	if (groups.empty())
		return;
	if (!groups.front()->numTextures)
		return;

	Texture* tex;
	for (tex = groups.front()->first; tex; tex = tex->next)
	{
		if (tex->glTex.TexNum() != nulltexture->glTex.TexNum())
			break;
	}

	if (!tex) return;

	g_qeglobals.d_workTexDef.Set(tex);
	g_vTexture.ChooseTexture(&g_qeglobals.d_workTexDef);
}


/*
==================
Textures::ReloadGroup
==================
*/
void Textures::ReloadGroup(TextureGroup* tg)
{
	for (auto tgIt = groups.begin(); tgIt != groups.end(); tgIt++)
	{
		if (*tgIt == tg)
		{
			ReloadGroup(tgIt);
			return;
		}
	}
}

void Textures::ReloadGroup(std::vector<TextureGroup*>::iterator tgIt)
{
	TextureGroup *oldwad, *newwad;
	Texture* temp;
	WadReader wl;

	oldwad = *tgIt;
	newwad = wl.Read(oldwad->name);
	if (!newwad)
		return;	// errors will already be in the console

	// copy used status so we don't have to rebuild the whole map to restore it
	for (Texture* tex = newwad->first; tex; tex = tex->next)
	{
		Texture *otex = oldwad->ForName(tex->name);
		if (otex)	// already flushed if not
			tex->used = otex->used;
	}

	Textures::RemoveFromNameMap(oldwad);

	// swap new wad data into old TextureGroup so its position doesn't have to change
	// this lets the texture browser keep persistent state about texture groups
	assert(oldwad->id == newwad->id);
	temp = oldwad->first;
	oldwad->first = newwad->first;
	newwad->first = temp;
	oldwad->numTextures = newwad->numTextures;
	delete newwad;	// will now take oldwad's textures with it

	if (oldwad->flushed)
		oldwad->FlushUnused();

	Textures::AddToNameMap(oldwad);

	LibraryChanged();
}


/*
==================
Textures::ReloadAll
==================
*/
void Textures::ReloadAll()
{
	for (auto tgIt = groups.begin(); tgIt != groups.end(); tgIt++)
		ReloadGroup(tgIt);

	LibraryChanged();
}

/*
==================
Textures::MergeWadStrings
==================
*/
void Textures::MergeWadStrings(const std::string& wad1, const std::string& wad2, std::string& out)
{
	if (wad1.length() <= 1 && wad2.length() <= 1)
		return;
	if (wad1.length() <= 1)
	{
		out = wad2;
		return;
	}
	if (wad2.length() <= 1)
	{
		out = wad1;
		return;
	}

	std::string w = wad1;
	StringViewList wads;
	w.append(" ");
	w.append(wad2);
	std::replace(w.begin(), w.end(), ';', ' ');

	Tokenizer tkr(w);
	tkr.All(wads);
	strlib::RemoveDuplicates(wads);

	out.clear();
	out.reserve(wads.size() * 16);
	for (int i = 0; i < wads.size(); out.append("; "))
	{
		out.append(wads[i++]);
		if (i == wads.size())
			break;
	}
}

/*
==================
Textures::LoadWadsFromWadstring
==================
*/
void Textures::LoadWadsFromWadstring(const std::string& wadstring)
{
	std::string w = wadstring;
	std::replace(w.begin(), w.end(), ';', ' ');
	Tokenizer tkr(w);

	while(!tkr.AtEnd())
	{
		Textures::LoadWad(std::string(tkr.Next()));
	}

	LibraryChanged();
}

/*
==================
Textures::LoadWad
==================
*/
void Textures::LoadWad(const std::string& wadfile)
{
	WadReader wl;
	TextureGroup *oldwad, *newwad;
	bool refresh = false;
	std::string wadpath;

	if (!IO::IsPathAbsolute(wadfile))	// relative, append wad path
		wadpath = g_project.wadPath;
	wadpath.append(wadfile);

	Log::Print(_S("Loading all textures from %s\n") << wadpath);

	// check if the wad is already loaded and replace it
	auto tgIt = groups.begin();
	for (tgIt; tgIt != groups.end(); tgIt++)
	{
		if ((*tgIt)->name == wadfile)
			break;
	}
	if (tgIt != groups.end())
	{
		oldwad = *tgIt;

		newwad = wl.Read(wadpath);
		if (!newwad) 
			return;	// errors will already be in the console
		newwad->name = wadfile;
		Textures::RemoveFromNameMap(oldwad);
		*tgIt = newwad;
		delete oldwad;
	}
	else
	{
		newwad = wl.Read(wadpath);
		if (!newwad)
			return;

		newwad->name = wadfile;
		groups.push_back(newwad);
		//std::sort(groups.begin(),groups.end(),TextureGroup::sortcmp);
	}

	Textures::AddToNameMap(newwad);
	LibraryChanged();
}

void Textures::LoadPalette()
{
	// palette load attempts, in order:
	// g_project.paletteFile
	// g_project.wadPath + paletteFile name
	// working dir/palette.lmp

	PaletteReader pr;

	if (!g_project.paletteFile.empty())
	{
		Log::Print(_S("Loading palette from project: %s\n") << g_project.paletteFile);
		if (pr.Load(g_project.paletteFile))
			return;

		std::string pfname = _S("%s/%s") << g_project.wadPath << IO::FileName(g_project.paletteFile);
		Log::Print(_S("Checking for palette in texture path: %s\n") << pfname);
		if (pr.Load(pfname))
			return;

		Log::Warning("Couldn't load palette!");
	}

	pr.LoadDefault();
}


//=====================================================



/*
==================
Textures::MenuLoadWad
==================
*/
void Textures::MenuLoadWad(const std::string& menuwad)
{
	LoadWad(menuwad);

	LibraryChanged();

	// refresh texture assignments on all brushes after a manual wad load, since
	// the loaded wad might refresh an existing wad or fill in some broken textures
	g_map.BuildBrushData();
}

/*
==================
Textures::MenuReloadAll
==================
*/
void Textures::MenuReloadAll()
{
	ReloadAll();
	g_map.BuildBrushData();
}

/*
==================
Textures::MenuReloadGroup
==================
*/
void Textures::MenuReloadGroup(TextureGroup* tg)
{
	ReloadGroup(tg);
	g_map.BuildBrushData();
}

/*
==================
Textures::MenuFlushUnused
==================
*/
void Textures::MenuFlushUnused()
{
	RefreshUsedStatus();
	FlushUnused();
}


/*
==================
Textures::SetTextureMode
==================
*/
void Textures::SetTextureMode(const int mode)
{
	g_cfgUI.TextureMode = min(max(0, mode), 5);

	for (auto tmIt = texMap.begin(); tmIt != texMap.end(); ++tmIt)
	{
		tmIt->second->glTex.Bind();
		tmIt->second->glTex.SetMode(mode);
	}

	// select the default texture
	glBindTexture(GL_TEXTURE_2D, 0);

	glFinish();
}

/*
==================
Textures::SetDrawMode
==================
*/
void Textures::SetDrawMode(const int mode)
{
	HMENU	hMenu;
	hMenu = GetMenu(g_hwndMain);

	CheckMenuItem(hMenu, ID_DRAWMODE_WIREFRAME, MF_UNCHECKED);
	CheckMenuItem(hMenu, ID_DRAWMODE_FLATSHADE, MF_UNCHECKED);
	CheckMenuItem(hMenu, ID_DRAWMODE_TEXTURED, MF_UNCHECKED);

	switch (mode)
	{
	case 0:	// wireframe
		g_cfgUI.DrawMode = CD_WIRE;
		CheckMenuItem(hMenu, ID_DRAWMODE_WIREFRAME, MF_CHECKED);
		break;
	case 1:	// flatshade
		g_cfgUI.DrawMode = CD_FLAT;
		CheckMenuItem(hMenu, ID_DRAWMODE_FLATSHADE, MF_CHECKED);
		break;
	case 2:	// textured
	default:
		g_cfgUI.DrawMode = CD_TEXTURED;
		CheckMenuItem(hMenu, ID_DRAWMODE_TEXTURED, MF_CHECKED);
		break;
	}
}
