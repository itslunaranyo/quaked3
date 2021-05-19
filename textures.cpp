//==============================
//	textures.c
//==============================

#include "qe3.h"
#include "io.h"
#include <algorithm>	// for sort

#define	MAX_TEXTUREDIRS	128

static unsigned	tex_palette[256];
static Palette texpal;
Texture	*Textures::nulltexture;
static bool		nomips;

static HGLRC s_hglrcTexture;
static HDC	 s_hdcTexture;


// sikk---> Removed Anisotropy
//#define	TX_NEAREST				1
//#define	TX_NEAREST_ANISOTROPY	2
//#define	TX_LINEAR				3
//#define	TX_LINEAR_ANISOTROPY	4

int		g_nTextureMode = GL_LINEAR_MIPMAP_LINEAR; // default
//<---sikk
int		g_nTextureExtensionNumber = 1;
int		g_nTextureNumMenus;
char	g_szTextureMenuNames[MAX_TEXTUREDIRS][64];

//=====================================================

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
	// load palette
	texpal.LoadFromFile("palette.lmp");	// TODO: specify name in project

	// prepare null texture
	MakeNullTexture();

	// create solid color group
	strcpy(group_solid.name, "*solid");
	// * is invalid as a filename character, so a theoretical wad named "solid" can't collide
	
	g_qeglobals.d_qtextures = nullptr;
}

/*
==================
Textures::Flush
==================
*/
void Textures::Flush()
{
	// delete all texture groups
	for (auto tgIt = groups.begin(); tgIt != groups.end(); tgIt++)
		delete *tgIt;

	groups.clear();
	Textures::texMap.clear();

	g_qeglobals.d_texturewin.stale = true;

	// if a map is loaded, we must force all brushes to refresh their texture assignments to 
	// properly populate the unknown wad and redo texture projections for the 64x64 notexture
	Map_BuildBrushData();
	Sys_UpdateWindows(W_CAMERA);
}

/*
==================
Textures::FlushUnused
==================
*/
void Textures::FlushUnused()
{
	// call flushunused on all texture groups
	for (auto tgIt = groups.begin(); tgIt != groups.end(); tgIt++)
	{
		Textures::RemoveFromNameMap(*tgIt);	// hurf
		(*tgIt)->FlushUnused();
		Textures::AddToNameMap(*tgIt);		// durf
	}

	g_qeglobals.d_texturewin.stale = true;
	Sys_UpdateWindows(W_CAMERA);
}

/*
==================
Textures::ClearUsed
==================
*/
void Textures::ClearUsed()
{
	// call clearused on all texture groups
	for (auto tgIt = groups.begin(); tgIt != groups.end(); tgIt++)
		(*tgIt)->ClearUsed();

	// TODO: should "refresh used" instead, so used status contains no false negatives
	g_qeglobals.d_texturewin.stale = true;
}

/*
==================
Textures::CreateSolid
==================
*/
Texture *Textures::CreateSolid(const char *name)
{
	int			gltex;
	WadLoader	wl;
	vec3_t		color;
	qeBuffer	data(4);
	Texture		*solidtex;

	// make solid texture from name
	sscanf(name, "(%f %f %f)", &color[0], &color[1], &color[2]);

	data[0] = color[0] * 255;
	data[1] = color[1] * 255;
	data[2] = color[2] * 255;
	data[3] = 255;

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
	vec3_t		color;
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
	if (name[0] == '(')
	{
		// keep the solid textures out of the main texture space
		tx = group_solid.ForName(name);
		if (!tx)
		{
			tx = CreateSolid(name);
		}
		return tx;
	}

	// check the global texture name map
	tx = texMap[label_t(name)];
	if (tx)
	{
		tx->used = true;
		return tx;
	}

	// still not found, make wrapper for nulltexture and put it in the unknown wad
	tx = new Texture(*nulltexture);
	strncpy(tx->name, name, 32);
	tx->next = nullptr;
	tx->used = true;

	group_unknown.Add(tx);
	texMap[label_t(name)] = tx;
	g_qeglobals.d_texturewin.stale = true;
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
		texMap[label_t(q->name)] = nullptr;
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
}

/*
==================
Textures::LoadWad
==================
*/
void Textures::LoadWad(const char* wadfile)
{
	WadLoader wl;
	TextureGroup* wad;
	bool refresh = false;

	Sys_Printf("CMD: Loading all textures...\n");

	auto tgIt = groups.begin();
	// check if the wad is already loaded and trash it first
	for (tgIt; tgIt != groups.end(); tgIt++)
	{
		if (!strncmp((*tgIt)->name, wadfile, 32))
			break;
	}
	if (tgIt != groups.end())
	{
		wad = *tgIt;	// old wad
		Textures::RemoveFromNameMap(wad);
		*tgIt = wl.LoadTexturesFromWad(wadfile);
		if (!wad)
		{
			InspWnd_SetMode(W_CONSOLE);	// show errors
			return;
		}
		Textures::AddToNameMap(*tgIt);
		delete wad;
	}
	else
	{
		wad = wl.LoadTexturesFromWad(wadfile);
		if (!wad)
		{
			InspWnd_SetMode(W_CONSOLE);
			return;
		}

		Textures::AddToNameMap(wad);
		groups.push_back(wad);
		std::sort(groups.begin(),groups.end(),TextureGroup::sortcmp);
	}

	g_qeglobals.d_texturewin.stale = true;
	g_qeglobals.d_texturewin.origin[1] = 0;

	InspWnd_SetMode(W_TEXTURE);
	Sys_UpdateWindows(W_TEXTURE|W_CAMERA);

	// select the first texture in the list
	if (!g_qeglobals.d_workTexDef.name[0])
		g_qeglobals.d_texturewin.SelectTexture(16, g_qeglobals.d_texturewin.height - 16);
}

/*
==================
Textures::MenuLoadWad
==================
*/
void Textures::MenuLoadWad(const int menunum)
{
	LoadWad(g_szTextureMenuNames[menunum - CMD_TEXTUREWAD]);

	// refresh texture assignments on all brushes after a manual wad load, since
	// the loaded wad might refresh an existing wad or fill in some broken textures
	Map_BuildBrushData();
	Sys_UpdateWindows(W_CAMERA);
}

/*
==================
Textures::ForName
==================
*/
void Textures::SetRenderMode(const int menunum)
{

}

/*
==================
Textures::ForName
==================
*/
void Textures::SetParameters()
{

}

//=====================================================

/*
==================
Texture::Texture
==================
*/
Texture::Texture(int w, int h, const char* n, vec3_t c, int gltex) :
	next(nullptr), width(w), height(h), used(false), texture_number(gltex)
{
	strncpy(name, n, 32);
	VectorCopy(c, color);
	SetFlags();
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
		showflags |= BFL_LIQUID;
	else if (!strncmp(name, "sky", 3))
		showflags |= BFL_SKY;
	else if (!strncmp(name, "clip", 4))
		showflags |= BFL_CLIP;
	else if (!strncmp(name, "hint", 4))
		showflags |= BFL_HINT;
	else if (!strncmp(name, "skip", 4))
		showflags |= BFL_SKIP;
	//else if (!strncmp(name, "hintskip"))
	//	showflags |= BFL_HINT | BFL_SKIP;
}


//=====================================================

/*
==================
TextureGroup::TextureGroup
==================
*/
TextureGroup::TextureGroup()
{
	first = nullptr;
	numTextures = 0;
	strncpy_s(name, "?", 2);
}

/*
==================
TextureGroup::~TextureGroup
==================
*/
TextureGroup::~TextureGroup()
{
	Texture *tex, *nexttex;
	
	for (tex = first; tex; tex = nexttex)
	{
		nexttex = tex->next;
		delete tex;
	}
}


/*
==================
TextureGroup::ByColor

lunaran - this would be a quick way to find entity solid color textures if I were smart
==================
*/
Texture *TextureGroup::ByColor(vec3_t oc)
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
		if (!strncmp(name, tex->name, 32))
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
	if (strncmp(tx->name, first->name, 32) < 0)
	{
		tx->next = first;
		first = tx;
		return;
	}

	Texture *qtex;
	qtex = first;
	while (qtex->next)
	{
		if (strncmp(tx->name, qtex->next->name, 32) < 0)
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

	sprintf(filepath, "%s/%s", g_qeglobals.d_entityProject->GetKeyValue("texturepath"), filename);

	if (LoadFile(filepath, wadFileBuf) <= 0)
	{
		Sys_Printf("WARNING: %s could not be loaded.\n", filename);
		return false;
	}

	if ( strncmp(((char*)*wadFileBuf), "WAD2", 4) )
	{
		Sys_Printf("WARNING: %s is not a valid wadfile.\n", filename);
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
	vec3_t		color;

	wadinfo_t *wadinfo = (wadinfo_t*)*wadFileBuf;

	// loop through lumps, validate & gather up their sizes
	lumpinfo = (lumpinfo_t *)((byte*)*wadFileBuf + wadinfo->infotableofs);

	for (i = 0; i < wadinfo->numlumps; i++, lumpinfo++)
	{
		qmip = (miptex_t *)((byte*)*wadFileBuf + lumpinfo->filepos);

		if (lumpinfo->type != TYP_MIPTEX)
		{
			Sys_Printf("Warning: %s is not a miptex, ignoring", lumpinfo->name);
			continue;
		}

		if (qmip->width > 1024 || qmip->width < 1 || qmip->height > 1024 || qmip->height < 1)
		{
			Sys_Printf("Warning: %s has bad size, ignoring", lumpinfo->name);
			continue;
		}

		if (lumpinfo->filepos + (qmip->width * qmip->height) > wadFileBuf.size())
		{
			Sys_Printf("Warning: %s is incomplete, stopping after %i textures", lumpinfo->name, i);
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
void WadLoader::MiptexToRGB(miptex_t *mip, qeBuffer &texDataBuf, vec3_t avg)
{
	unsigned int i, count;
	byte		*src;
	vec3_t pxc;

	VectorCopy(g_v3VecOrigin, avg);
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
		texpal.ColorAsVec3(src[i], pxc);
		VectorAdd(avg, pxc, avg);
	}
	VectorScale(avg, 1.0 / count, avg);
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
	SetTexParameters();

	if (nomips)
		glTexImage2D(GL_TEXTURE_2D, 0, 3, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, *texData);
	else
		gluBuild2DMipmaps(GL_TEXTURE_2D, 3, w, h, GL_RGBA, GL_UNSIGNED_BYTE, *texData);

	glBindTexture(GL_TEXTURE_2D, 0);

	return texnum;
}

//=====================================================

/*
============
SetTexParameters
============
*/
void SetTexParameters ()
{
// sikk---> Removed Anisotropy
/*	GLfloat max_anisotropy;

	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_anisotropy);

	switch (g_nTextureMode)
	{
	case TX_NEAREST:
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0f);
		break;
	case TX_NEAREST_ANISOTROPY:
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, max_anisotropy);
		break;
	default: 
	case TX_LINEAR:
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0f);
		break;
	case TX_LINEAR_ANISOTROPY:
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, max_anisotropy);
		break;
	}*/
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, g_nTextureMode);

	switch (g_nTextureMode)
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
// <---sikk
	}
}

/*
============
Texture_SetMode
============
*/
void Texture_SetMode (int iMenu)
{
	int		i, iMode;
	bool	texturing = true;
	HMENU	hMenu;

	hMenu = GetMenu(g_qeglobals.d_hwndMain);

// sikk---> Removed Anisotropy
	switch (iMenu) 
	{
	case ID_TEXTURES_WIREFRAME:
		iMode = 0;
		texturing = false;
		break;
	case ID_TEXTURES_FLATSHADE:
		iMode = 0;
		texturing = false;
		break;
	case ID_TEXTURES_NEAREST:
		iMode = GL_NEAREST;
		break;
	case ID_TEXTURES_NEARESTMIPMAP:
		iMode = GL_NEAREST_MIPMAP_NEAREST;
		break;
	case ID_TEXTURES_LINEAR:
		iMode = GL_NEAREST_MIPMAP_LINEAR;
		break;
	case ID_TEXTURES_BILINEAR:
		iMode = GL_LINEAR;
		break;
	case ID_TEXTURES_BILINEARMIPMAP:
		iMode = GL_LINEAR_MIPMAP_NEAREST;
		break;
	case ID_TEXTURES_TRILINEAR:
		iMode = GL_LINEAR_MIPMAP_LINEAR;
		break;
	}

	CheckMenuItem(hMenu, ID_TEXTURES_WIREFRAME, MF_UNCHECKED);
	CheckMenuItem(hMenu, ID_TEXTURES_FLATSHADE, MF_UNCHECKED);
	CheckMenuItem(hMenu, ID_TEXTURES_NEAREST, MF_UNCHECKED);
	CheckMenuItem(hMenu, ID_TEXTURES_NEARESTMIPMAP, MF_UNCHECKED);
	CheckMenuItem(hMenu, ID_TEXTURES_LINEAR, MF_UNCHECKED);
	CheckMenuItem(hMenu, ID_TEXTURES_BILINEAR, MF_UNCHECKED);
	CheckMenuItem(hMenu, ID_TEXTURES_BILINEARMIPMAP, MF_UNCHECKED);
	CheckMenuItem(hMenu, ID_TEXTURES_TRILINEAR, MF_UNCHECKED);
	
	CheckMenuItem(hMenu, iMenu, MF_CHECKED);
// <---sikk

	g_qeglobals.d_savedinfo.nTexMenu = iMenu;
	g_nTextureMode = iMode;
	if (texturing)
		SetTexParameters();

	if (!texturing && iMenu == ID_TEXTURES_WIREFRAME)
	{
		g_qeglobals.d_camera.draw_mode = cd_wire;
		Map_BuildBrushData();
		Sys_UpdateWindows(W_ALL);
		return;
	}
	else if (!texturing && iMenu == ID_TEXTURES_FLATSHADE)
	{
		g_qeglobals.d_camera.draw_mode = cd_solid;
		Map_BuildBrushData();
		Sys_UpdateWindows(W_ALL);
		return;
	}

	for (i = 1; i < g_nTextureExtensionNumber; i++)
	{
		glBindTexture(GL_TEXTURE_2D, i);
		SetTexParameters();
	}

	// select the default texture
	glBindTexture(GL_TEXTURE_2D, 0);

	glFinish();

	if (g_qeglobals.d_camera.draw_mode != cd_texture)
	{
		g_qeglobals.d_camera.draw_mode = cd_texture;
		Map_BuildBrushData();
	}

	Sys_UpdateWindows(W_ALL);
}

/*
==================
FillTextureMenu
==================
*/
void FillTextureMenu ()
{
	HMENU	hmenu;
	int		i;
	struct _finddata_t fileinfo;
	int		handle;
	char    temp[1024];
	char	path[1024];
	char    dirstring[1024];
	char   *s;

	hmenu = GetSubMenu(GetMenu(g_qeglobals.d_hwndMain), MENU_TEXTURE);

	// delete everything
	for (i = 0; i < g_nTextureNumMenus; i++)
		DeleteMenu(hmenu, CMD_TEXTUREWAD + i, MF_BYCOMMAND);

	g_nTextureNumMenus = 0;

//	strcpy(g_szWadString, "");	// sikk - Wad Loading

	// add everything
	strcpy(dirstring, g_qeglobals.d_entityProject->GetKeyValue("texturepath"));
	sprintf(path, "%s*.wad", dirstring);	// sikk - Wad Loading

	QE_ConvertDOSToUnixName(temp, dirstring);
	Sys_Printf("CMD: ScanTexturePath: %s\n", temp);

	s = dirstring + strlen(dirstring) - 1;
	while ((*s != '\\') && (*s != '/') && (s != dirstring))
		s--;
	*s = 0;

	handle = _findfirst(path, &fileinfo);
	if (handle != -1)
	{
		do
		{
			Sys_Printf("MSG: FoundFile: %s/%s\n", dirstring, fileinfo.name);

			AppendMenu(hmenu, MF_ENABLED | MF_STRING, CMD_TEXTUREWAD + g_nTextureNumMenus, (LPCTSTR)fileinfo.name);
			strcpy(g_szTextureMenuNames[g_nTextureNumMenus], fileinfo.name);
//			sprintf(g_szWadString, "%s%s%s", g_szWadString, g_szWadString[0] ? ";" : "", filename);	// sikk - Wad Loading

			if (++g_nTextureNumMenus == MAX_TEXTUREDIRS)
				break;
		} while (_findnext(handle, &fileinfo) != -1);

		_findclose(handle);
	}
}


//=====================================================

/*
============
WTexWndProc
============
*/
LONG WINAPI WTex_WndProc (
    HWND	hWnd,	// handle to window
    UINT	uMsg,	// message
    WPARAM	wParam,	// first message parameter
    LPARAM	lParam 	// second message parameter
	)
{
	int		xPos, yPos;
    RECT	rect;

    GetClientRect(hWnd, &rect);

    switch (uMsg)
    {
	case WM_CREATE:
        s_hdcTexture = GetDC(hWnd);
		QEW_SetupPixelFormat(s_hdcTexture, false);

		if ((s_hglrcTexture = wglCreateContext(s_hdcTexture)) == 0)
			Error("WTex_WndProc: wglCreateContext failed.");

        if (!wglMakeCurrent(s_hdcTexture, s_hglrcTexture))
			Error("WTex_WndProc: wglMakeCurrent failed.");

		if (!wglShareLists(g_qeglobals.d_hglrcBase, s_hglrcTexture))
			Error("WTex_WndProc: wglShareLists failed.");
		return 0;

	case WM_DESTROY:
		wglMakeCurrent(NULL, NULL);
		wglDeleteContext(s_hglrcTexture);
		ReleaseDC(hWnd, s_hdcTexture);
		return 0;

	case WM_PAINT:
        { 
		    PAINTSTRUCT	ps;

		    BeginPaint(hWnd, &ps);

            if (!wglMakeCurrent(s_hdcTexture, s_hglrcTexture))
				Error("wglMakeCurrent: Failed.");
			g_qeglobals.d_texturewin.Draw(rect.right-rect.left, rect.bottom-rect.top);
			SwapBuffers(s_hdcTexture);

		    EndPaint(hWnd, &ps);
        }
		return 0;

	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_LBUTTONDOWN:
// sikk---> LMB Bring to Top
		if (GetTopWindow(g_qeglobals.d_hwndMain) != g_qeglobals.d_hwndInspector)
			BringWindowToTop(g_qeglobals.d_hwndInspector);
// <---sikk
		SetCapture(g_qeglobals.d_hwndTexture);
		xPos = (short)LOWORD(lParam);  // horizontal position of cursor 
		yPos = (short)HIWORD(lParam);  // vertical position of cursor 
		
		g_qeglobals.d_texturewin.MouseDown(xPos, yPos, wParam);
		return 0;

	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
	case WM_LBUTTONUP:
		xPos = (short)LOWORD(lParam);  // horizontal position of cursor 
		yPos = (short)HIWORD(lParam);  // vertical position of cursor 
		
		g_qeglobals.d_texturewin.MouseUp(xPos, yPos, wParam);
		if (!(wParam & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON)))
			ReleaseCapture();
		return 0;

	case WM_MOUSEMOVE:
		xPos = (short)LOWORD(lParam);  // horizontal position of cursor 
		yPos = (short)HIWORD(lParam);  // vertical position of cursor 
		
		g_qeglobals.d_texturewin.MouseMoved(xPos, yPos, wParam);
		return 0;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

/*
==================
TexWnd_Create

We need to create a seperate window for the textures
in the inspector window, because we can't share
gl and gdi drawing in a single window
==================
*/
HWND TexWnd_Create (HINSTANCE hInstance)
{
    WNDCLASS	wc;

    /* Register the texture class */
	memset (&wc, 0, sizeof(wc));

    wc.style		 = 0;
    wc.lpfnWndProc   = (WNDPROC)WTex_WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = g_qeglobals.d_hInstance;
    wc.hIcon         = 0;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = TEXTURE_WINDOW_CLASS;

    if (!RegisterClass (&wc))
        Error("WTex_Register: Failed.");

	g_qeglobals.d_hwndTexture = CreateWindowEx(0,//WS_EX_CLIENTEDGE,	// extended window style
		TEXTURE_WINDOW_CLASS,				// registered class name
		"Texture View",						// window name
		WS_BORDER | WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,	// window style
		20,	20,	64,	64,						// size and position of window
		g_qeglobals.d_hwndInspector,		// parent or owner window
		0,									// menu or child-window identifier
		hInstance,							// application instance
		NULL);								// window-creation data

	if (!g_qeglobals.d_hwndTexture)
		Error("Could not create Texture Window.");

	return g_qeglobals.d_hwndTexture;
}

/*
===============
TexWnd_Resize
===============
*/
void TexWnd_Resize(RECT rc)
{
	InspWnd_MoveRect(g_qeglobals.d_hwndTexture, rc);
	g_qeglobals.d_texturewin.width = rc.right;
	g_qeglobals.d_texturewin.height = rc.bottom;

	g_qeglobals.d_texturewin.stale = true;
}



