//==============================
//	textures.cpp
//==============================

#include "pre.h"
#include "qe3.h"
#include "palette.h"
#include "map.h"
#include "select.h"
#include "TextureView.h"
//#include <algorithm>	// for sort


int		g_nTextureMode = GL_LINEAR_MIPMAP_LINEAR; // default
int		g_nTextureExtensionNumber = 1;
int		g_nTextureGroupID = 1;
bool	g_bTexturesChanged = false;

//=====================================================

static Palette texpal;

Texture						*Textures::nulltexture;
std::vector<TextureGroup*>	Textures::groups;
std::map<label_t, Texture*>	Textures::texMap;
TextureGroup				Textures::group_solid;
TextureGroup				Textures::group_unknown;

/*
==================
Textures::Init
==================
*/
void Textures::Init()
{
	Sys_Printf("Initializing textures\n");

	LoadPalette();

	// prepare null texture
	MakeNullTexture();
	if (!g_qeglobals.d_workTexDef.tex)
		g_qeglobals.d_workTexDef.Set(nulltexture);

	// create solid color group
	strcpy(group_solid.name, "*solid");
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
			if (*tex != texMap[label_t((*tex)->name)])
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
	if (g_qeglobals.d_workTexDef.tex == nullptr && texMap[label_t(g_qeglobals.d_workTexDef.name)] != nullptr)
	{
		// desired workdef texture was bad but is now in the name map, select it
		g_qeglobals.d_workTexDef.Set(g_qeglobals.d_workTexDef.name);
		g_vTexture.ChooseTexture(&g_qeglobals.d_workTexDef);
	}
	else if (g_qeglobals.d_workTexDef.name[0] == 0 || !texMap[label_t(g_qeglobals.d_workTexDef.name)])
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
Textures::MenuLoadWad
==================
*/
void Textures::MenuLoadWad(const char* menuwad)
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
	g_qeglobals.d_workTexDef.name[0] = 0;
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
void Textures::FlushUnusedFromWadstring(const char* wadstring)
{
	char	*tempwad, wadkey[2048];
	strncpy_s(wadkey, wadstring, 2047);

	// fixme: what happens when wads are separated by a semicolon and whitespace?
	for (tempwad = strtok(wadkey, ";"); tempwad; tempwad = strtok(0, ";"))
	{
		for (auto tgIt = groups.begin(); tgIt != groups.end(); tgIt++)
		{
			if (!strcmp(tempwad, (*tgIt)->name))
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
	Brush* b;
	for (b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
		b->RefreshTexdefs();
	for (b = g_map.brActive.next; b != &g_map.brActive; b = b->next)
		b->RefreshTexdefs();
	for (b = g_map.brRegioned.next; b != &g_map.brRegioned; b = b->next)
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
Texture *Textures::CreateSolid(const char *name)
{
	int			gltex;
	WadLoader	wl;
	vec3		color;
	qeBuffer	data(4);
	Texture		*solidtex;

	// make solid texture from name
	//sscanf(name, "#%f %f %f", &color[0], &color[1], &color[2]);
	hexToRGB(name, color);

	data[0] = color[0];// * 255;
	data[1] = color[1];// * 255;
	data[2] = color[2];// * 255;
	data[3] = 1;

	gltex = wl.MakeGLTexture(1, 1, data);
	solidtex = new Texture(1, 1, name, color, gltex);

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
	int			gltex;
	WadLoader	wl;
	vec3		color;
	qeBuffer	data(64*64*4);
	int			x, y, ofs;

	data.zero();

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

	gltex = wl.MakeGLTexture(64, 64, data);
	nulltexture = new Texture(64, 64, "nulltexture", color, gltex);
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
Texture *Textures::ForName(const char *name)
{
	Texture* tx;
	char nameci[MAX_TEXNAME];
	nameci[MAX_TEXNAME-1] = 0;

	// keep the solid textures out of the main texture space
	if (name[0] == '#')
	{
		// don't use ForName here - if it's not in the hash map, it's strcmp city
		vec3 color;
		hexToRGB(name, color);
		tx = group_solid.ByColor(color);
		if (!tx)
		{
			tx = CreateSolid(name);
		}
		return tx;
	}

	strncpy_s(nameci, name, MAX_TEXNAME-1);
	StringTolower(nameci);

	// check the global texture name map
	tx = texMap[label_t(nameci)];
	if (tx)
	{
		tx->Use();
		return tx;
	}

	// still not found, make wrapper for nulltexture and put it in the unknown wad
	tx = new Texture(*nulltexture);
	strncpy(tx->name, nameci, MAX_TEXNAME-1);
	tx->next = nullptr;

	group_unknown.Add(tx);
	texMap[label_t(nameci)] = tx;


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
		texMap[label_t(q->name)] = nullptr;
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
		label_t lbl(q->name);
		old = texMap[lbl];
		texMap[lbl] = q;
	}

	LibraryChanged();
}

/*
==================
Textures::SelectFirstTexture
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
		if (tex->texture_number != nulltexture->texture_number)
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
	WadLoader wl;

	oldwad = *tgIt;

	newwad = wl.LoadTexturesFromWad(oldwad->name);
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
Textures::LoadWadsFromWadstring
==================
*/
void Textures::LoadWadsFromWadstring(const char* wadstring)
{
	char	*tempwad, wadkey[2048];
	strncpy_s(wadkey, wadstring, 2047);

	// fixme: what happens when wads are separated by a semicolon and whitespace?
	for (tempwad = strtok(wadkey, ";"); tempwad; tempwad = strtok(0, ";"))
		Textures::LoadWad(tempwad);

	LibraryChanged();
}

/*
==================
Textures::LoadWad

FIXME: 'wadfile' could be an absolute path, use IsPathAbsolute() to check
and act accordingly
==================
*/
void Textures::LoadWad(const char* wadfile)
{
	WadLoader wl;
	TextureGroup *oldwad, *newwad;
	bool refresh = false;

	Sys_Printf("Loading all textures from %s...\n", wadfile);

	// check if the wad is already loaded and replace it
	auto tgIt = groups.begin();
	for (tgIt; tgIt != groups.end(); tgIt++)
	{
		if (!strncmp((*tgIt)->name, wadfile, MAX_WADNAME))
			break;
	}
	if (tgIt != groups.end())
	{
		oldwad = *tgIt;

		newwad = wl.LoadTexturesFromWad(wadfile);
		if (!newwad) 
			return;	// errors will already be in the console

		Textures::RemoveFromNameMap(oldwad);
		*tgIt = newwad;
		delete oldwad;
	}
	else
	{
		newwad = wl.LoadTexturesFromWad(wadfile);
		if (!newwad)
			return;

		groups.push_back(newwad);
		//std::sort(groups.begin(),groups.end(),TextureGroup::sortcmp);
	}

	Textures::AddToNameMap(newwad);
	LibraryChanged();
}

void Textures::LoadPalette()
{
	// load palette
	if (*g_project.paletteFile)
		texpal.LoadFromFile(g_project.paletteFile);
	else
		texpal.LoadFromFile("palette.lmp");
}

/*
==================
Textures::SetTextureMode
==================
*/
void Textures::SetTextureMode(const int mode)
{
	//	g_cfgUI.TextureMode = min(max(0, mode), 5);
//	SetParameters();

	for (int i = 0; i < g_nTextureExtensionNumber; i++)
	{
		glBindTexture(GL_TEXTURE_2D, i);
		SetParameters();
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

/*
==================
Textures::SetParameters
==================
*/
void Textures::SetParameters()
{
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, g_textureModes[g_cfgUI.TextureMode]);

	switch (g_textureModes[g_cfgUI.TextureMode])
	{
	case GL_NEAREST:
	case GL_NEAREST_MIPMAP_NEAREST:
	case GL_NEAREST_MIPMAP_LINEAR:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		break;
	case GL_LINEAR:
	case GL_LINEAR_MIPMAP_NEAREST:
	case GL_LINEAR_MIPMAP_LINEAR:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		break;
	}
}

//=====================================================

/*
==================
Texture::Texture
==================
*/
Texture::Texture(int w, int h, const char* n, vec3 c, int gltex) :
	next(nullptr), width(w), height(h), used(false), texture_number(gltex), color(c)
{
	strncpy(name, n, MAX_TEXNAME);
	StringTolower(name);
	SetFlags();
}

/*
==================
Texture::Use
==================
*/
void Texture::Use()
{
	if (used) return;

	g_map.numTextures++;
	used = true;
}

/*
==================
Texture::SetFlags
==================
*/
void Texture::SetFlags()
{
	showflags = 0;
	if (!strncmp(name, "*", 1))
		showflags |= BFL_LIQUID | BFL_TRANS;
	else if (!strncmp(name, "sky", 3))
		showflags |= BFL_SKY;
	else if (!strncmp(name, "clip", 4))
		showflags |= BFL_CLIP | BFL_TRANS;
	else if (!strncmp(name, "hint", 4))
		showflags |= BFL_HINT | BFL_TRANS;
	else if (!strncmp(name, "skip", 4))
		showflags |= BFL_SKIP;
	else if (!strcmp(name, "trigger"))
		showflags |= BFL_TRANS;

	//else if (!strncmp(name, "hintskip"))
	//	showflags |= BFL_HINT | BFL_SKIP;
}


//=====================================================

/*
==================
TextureGroup::TextureGroup
==================
*/
TextureGroup::TextureGroup() : first(nullptr), numTextures(0), flushed(false), id(0)
{
	strncpy_s(name, "?", 2);
}

/*
==================
TextureGroup::~TextureGroup
==================
*/
TextureGroup::~TextureGroup()
{
	Flush();
}

/*
==================
TextureGroup::Flush
==================
*/
void TextureGroup::Flush()
{
	Texture *tex, *nexttex;
	numTextures = 0;
	for (tex = first; tex; tex = nexttex)
	{
		nexttex = tex->next;
		delete tex;
	}
	first = nullptr;
}

/*
==================
TextureGroup::ByColor

a quick way to find entity solid color textures
==================
*/
Texture *TextureGroup::ByColor(const vec3 oc)
{
	for (Texture *tex = first; tex; tex = tex->next)
	{
		if (VectorCompare(tex->color, oc))
			return tex;
	}
	return nullptr;
}

/*
==================
TextureGroup::ForName
==================
*/
Texture *TextureGroup::ForName(const char *name)
{
	for (Texture *tex = first; tex; tex = tex->next)
	{
		if (!strncmp(name, tex->name, MAX_TEXNAME))
			return tex;
	}
	return nullptr;
}

/*
==================
TextureGroup::ClearUsed
==================
*/
void TextureGroup::ClearUsed()
{
	for (Texture *tex = first; tex; tex = tex->next)
		tex->used = false;
}

/*
==================
TextureGroup::Add
==================
*/
void TextureGroup::Add(Texture* tx)
{
	numTextures++;

	// add sorted by name
	if (!first)
	{
		first = tx;
		return;
	}
	if (strncmp(tx->name, first->name, MAX_TEXNAME) < 0)
	{
		tx->next = first;
		first = tx;
		return;
	}

	Texture *qtex;
	qtex = first;
	while (qtex->next)
	{
		if (strncmp(tx->name, qtex->next->name, MAX_TEXNAME) < 0)
		{
			tx->next = qtex->next;
			qtex->next = tx;
			return;
		}
		qtex = qtex->next;
	}
	qtex->next = tx;
}

/*
==================
TextureGroup::FlushUnused
==================
*/
void TextureGroup::FlushUnused()
{
	Texture *head, *cur, *temp, *prev;
	head = cur = first;
	prev = nullptr;
	while (cur)
	{
		if (!cur->used)
		{
			temp = cur;
			if (head == cur)
				head = cur->next;
			if (prev)
				prev->next = cur->next;
			cur = cur->next;
			if (Textures::texMap[label_t(temp->name)] == temp)	// don't nuke textures from other wads by the same name
				Textures::texMap[label_t(temp->name)] = nullptr;
			delete temp;
			numTextures--;
		}
		else
		{
			prev = cur;
			cur = cur->next;
		}
	}
	first = head;
	flushed = true;
}

//=====================================================

/*
==================
WadLoader::LoadTexturesFromWad
==================
*/
TextureGroup* WadLoader::LoadTexturesFromWad(const char* filename)
{
	qeBuffer wadFileBuf;
	if (!ReadWad(filename, wadFileBuf)) return nullptr;

	TextureGroup *wad;
	wad = ParseWad(wadFileBuf);
	//ExtractFileBase(filename, wad->name);
	strcpy(wad->name, filename);

	// assign unique ID per-name so future matches don't involve strcmps
	for (auto tgIt = Textures::groups.begin(); tgIt != Textures::groups.end(); ++tgIt)
	{
		if (strncmp((*tgIt)->name, wad->name, MAX_WADNAME))
			continue;
		wad->id = (*tgIt)->id;
		break;
	}
	if (!wad->id)
		wad->id = g_nTextureGroupID++;

	return wad;
}

/*
==================
WadLoader::ReadWad
==================
*/
bool WadLoader::ReadWad(const char* filename, qeBuffer &wadFileBuf)
{
	char	filepath[MAX_PATH];

	if (IsPathAbsolute(filename))
		strcpy(filepath, filename);
	else	// relative, append wad path
		sprintf(filepath, "%s/%s", g_project.wadPath, filename);

	DefaultExtension(filepath, "wad");

	if (IO_LoadFile(filepath, wadFileBuf) <= 0)
	{
		Warning("%s could not be loaded.", filename);
		return false;
	}

	if ( strncmp(((char*)*wadFileBuf), "WAD2", 4) )
	{
		Warning("%s is not a valid wadfile.", filename);
		return false;
	}
	return true;
}

/*
==================
WadLoader::ParseWad
==================
*/
TextureGroup *WadLoader::ParseWad(qeBuffer &wadFileBuf)
{
	TextureGroup *texGroup = new TextureGroup();
	lumpinfo_t	*lumpinfo;
	miptex_t	*qmip;
	int			i, gltex;
	vec3		color;

	wadinfo_t *wadinfo = (wadinfo_t*)*wadFileBuf;

	// loop through lumps, validate & gather up their sizes
	lumpinfo = (lumpinfo_t *)((byte*)*wadFileBuf + wadinfo->infotableofs);

	for (i = 0; i < wadinfo->numlumps; i++, lumpinfo++)
	{
		qmip = (miptex_t *)((byte*)*wadFileBuf + lumpinfo->filepos);

		if (lumpinfo->type != TYP_MIPTEX)
		{
			Warning("%s is not a miptex, ignoring", lumpinfo->name);
			continue;
		}

		if (qmip->width > 1024 || qmip->width < 1 || qmip->height > 1024 || qmip->height < 1)
		{
			Warning("%s has bad size, ignoring", lumpinfo->name);
			continue;
		}

		if (lumpinfo->filepos + (qmip->width * qmip->height) > wadFileBuf.size())
		{
			Warning("%s is incomplete, stopping after %i textures", lumpinfo->name, i);
			return texGroup;
		}

		qeBuffer texData;
		MiptexToRGB(qmip, texData, color);
		gltex = MakeGLTexture(qmip->width, qmip->height, texData);

		// instantiate a Texture and give it *miptex to draw parms and bitmap data from
		texGroup->Add(new Texture(qmip->width, qmip->height, qmip->name, color, gltex));
	}

	return texGroup;
}

/*
==================
WadLoader::MiptexToRGB
==================
*/
void WadLoader::MiptexToRGB(miptex_t *mip, qeBuffer &texDataBuf, vec3 &avg)
{
	unsigned int i, count;
	byte	*src;
	//vec3	pxc;

	avg = vec3(0);
	src = (byte *)mip + mip->offsets[0];
	count = mip->width * mip->height;
	texDataBuf.resize(count * 4);

	// The bitmaps in wads are upside down, which is right side up relative to a 
	// GL bottom-left-origin, so we can just copy straight
	unsigned int* pixel;
	for (i = 0; i < count; i++)
	{
		pixel = (unsigned*)(*texDataBuf) + i;
		*pixel = texpal.ColorAsInt(src[i]);
		avg += texpal.ColorAsVec3(src[i]);
	}
	avg = avg / (float)count;
}

/*
==================
WadLoader::MakeGLTexture
==================
*/
int WadLoader::MakeGLTexture(int w, int h, qeBuffer &texData)
{
	int texnum = g_nTextureExtensionNumber++;

	glBindTexture(GL_TEXTURE_2D, texnum);
	Textures::SetParameters();

	glTexImage2D(GL_TEXTURE_2D, 0, 3, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, *texData);
	glGenerateMipmap(GL_TEXTURE_2D);

	glBindTexture(GL_TEXTURE_2D, 0);

	return texnum;
}

//=====================================================


/*
==================
Textures::ListWadsInDirectory
==================
*/
std::vector<_finddata_t> Textures::ListWadsInDirectory(const char* wadpath)
{
	_finddata_t fileinfo;
	std::vector<_finddata_t> wadlist;
	int		handle;
	char	path[1024];
	char    dirstring[1024];

	sprintf(path, "%s*.wad", wadpath);
	Sys_ConvertDOSToUnixName(dirstring, wadpath);
	Sys_Printf("ListWadsInDirectory: %s\n", dirstring);

	handle = _findfirst(path, &fileinfo);
	if (handle != -1)
	{
		do {
			Sys_Printf("Found wad file: %s%s\n", dirstring, fileinfo.name);
			wadlist.push_back(fileinfo);
		} while (_findnext(handle, &fileinfo) != -1);
		_findclose(handle);
	}
	return wadlist;
}