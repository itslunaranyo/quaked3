//==============================
//	textures.c
//==============================

#include "qe3.h"
#include "io.h"


#define	MAX_TEXTUREDIRS	128

static unsigned		tex_palette[256];
static qtexture_t	*notexture;
static bool			nomips;

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

/*
==================
Texture_Init
==================
*/
void Texture_Init ()
{
	char	name[1024];
	byte   *pal;

	// load the palette
	// sikk - Palette now uses Texture Directory instead of hardcoded as basepath/gfx/
	// lunaran - check both, old location was hardcoded because that's where quake.exe expects it to be
	sprintf(name, "%s/gfx/palette.lmp", ValueForKey(g_qeglobals.d_entityProject, "basepath"));
	LoadFile(name, (void**)&pal);
	if (!pal)
	{
		Sys_Printf("Could not load %s, trying texturepath ...\n", name);
		sprintf(name, "%s/palette.lmp", ValueForKey(g_qeglobals.d_entityProject, "texturepath"));
		LoadFile(name, (void**)&pal);
		if (!pal)
		{
			Error("Could not load %s", name);
		}
	}
	Texture_InitPalette(pal);
	free(pal);

	// create the fallback texture
	Texture_MakeNotexture();

	g_qeglobals.d_qtextures = NULL;
}



//=====================================================

/*
==============
Texture_InitPalette
==============
*/
void Texture_InitPalette (byte *pal)
{
    int		r, g, b, v;
    int		i;
	int		inf;
	byte	gammatable[256];
	float	gamma;

	gamma = g_qeglobals.d_savedinfo.fGamma;

	if (gamma == 1.0)
	{
		for (i = 0; i < 256; i++)
			gammatable[i] = i;
	}
	else
	{
		for (i = 0; i < 256; i++)
		{
			inf = 255 * pow((i + 0.5) / 255.5 , gamma) + 0.5;
			if (inf < 0)
				inf = 0;
			if (inf > 255)
				inf = 255;
			gammatable[i] = inf;
		}
	}

    for (i = 0; i < 256; i++)
    {
		r = gammatable[pal[0]];
		g = gammatable[pal[1]];
		b = gammatable[pal[2]];
		pal += 3;
		
		v = (r << 24) + (g << 16) + (b << 8) + 255;
		v = BigLong(v);
		
		tex_palette[i] = v;
    }
}

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
=================
Texture_LoadTexture
=================
*/
qtexture_t *Texture_LoadTexture (miptex_t *qtex)
{
    byte	   *source;
    unsigned   *dest;
    int			width, height, i, count;
	int			total[3];
    qtexture_t *q;
    
    q = (qtexture_t*)qmalloc(sizeof(*q));
    width = LittleLong(qtex->width);
    height = LittleLong(qtex->height);

    q->width = width;
    q->height = height;

	dest = (unsigned int*)qmalloc(width * height * 4);

    count = width * height;
    source = (byte *)qtex + LittleLong(qtex->offsets[0]);

	// The dib is upside down so we want to copy it into 
	// the buffer bottom up.
	total[0] = total[1] = total[2] = 0;

    for (i = 0; i < count; i++)
	{
		dest[i] = tex_palette[source[i]];

		total[0] += ((byte *)(dest + i))[0];
		total[1] += ((byte *)(dest + i))[1];
		total[2] += ((byte *)(dest + i))[2];
	}

	q->color[0] = (float)total[0] / (count * 255);
	q->color[1] = (float)total[1] / (count * 255);
	q->color[2] = (float)total[2] / (count * 255);

    q->texture_number = g_nTextureExtensionNumber++;

	glBindTexture(GL_TEXTURE_2D, q->texture_number);
	SetTexParameters();

	if (nomips)
		glTexImage2D(GL_TEXTURE_2D, 0, 3, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, dest);
	else
		gluBuild2DMipmaps(GL_TEXTURE_2D, 3, width, height, GL_RGBA, GL_UNSIGNED_BYTE, dest);

	free(dest);

	glBindTexture(GL_TEXTURE_2D, 0);

    return q;
}

/*
===============
Texture_CreateSolid

Create a single pixel texture of the apropriate color
===============
*/
qtexture_t *Texture_CreateSolid (char *name)
{
	byte		data[4];
	qtexture_t *q;

    q = (qtexture_t*)qmalloc(sizeof(*q));
	
	sscanf(name, "(%f %f %f)", &q->color[0], &q->color[1], &q->color[2]);

	data[0] = q->color[0] * 255;
	data[1] = q->color[1] * 255;
	data[2] = q->color[2] * 255;
	data[3] = 255;

	q->width = q->height = 1;
    q->texture_number = g_nTextureExtensionNumber++;
	glBindTexture(GL_TEXTURE_2D, q->texture_number);
	SetTexParameters();

	if (nomips)
		glTexImage2D(GL_TEXTURE_2D, 0, 3, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	else
		gluBuild2DMipmaps(GL_TEXTURE_2D, 3, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, data);

	glBindTexture(GL_TEXTURE_2D, 0);

	return q;
}

/*
=================
Texture_MakeNotexture
=================
*/
void Texture_MakeNotexture ()
{
    qtexture_t *q;
    byte		data[4][4];

	notexture = q = (qtexture_t*)qmalloc(sizeof(*q));
	strcpy(q->name, "notexture");
    q->width = q->height = 64;
	q->wad = NULL;
    
	memset(data, 0, sizeof(data));
	data[0][2] = data[3][2] = 255;

	q->color[0] = 0;
	q->color[1] = 0;
	q->color[2] = 0.5;

    q->texture_number = g_nTextureExtensionNumber++;
	glBindTexture(GL_TEXTURE_2D, q->texture_number);
	SetTexParameters();

	if (nomips)
		glTexImage2D(GL_TEXTURE_2D, 0, 3, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	else
		gluBuild2DMipmaps(GL_TEXTURE_2D, 3, 2, 2, GL_RGBA, GL_UNSIGNED_BYTE, data);

	glBindTexture(GL_TEXTURE_2D, 0);
}


/*
===============
Texture_ForName

TODO: this slows down map loading by a feckton
FIXME: not anymore ... somehow ... why?
===============
*/
qtexture_t *Texture_ForName (char *name)
{
	qtexture_t	*q;

	// return notexture;
	for (q = g_qeglobals.d_qtextures; q; q = q->next)
	{
		if (!strncmp(name, q->name, 32))
		{
			q->inuse = true;
		    return q;
		}
	}

	if (name[0] == '(')
	{
		q = Texture_CreateSolid(name);
		strncpy(q->name, name, sizeof(q->name) - 1); 

		q->inuse = true;
		q->next = g_qeglobals.d_qtextures;
		g_qeglobals.d_qtextures = q;
 
		return q;
	}
	
	return notexture;
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
	strcpy(dirstring, ValueForKey(g_qeglobals.d_entityProject, "texturepath"));
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

/*
==================
Texture_ClearInuse

A new map is being loaded, so clear inuse markers
==================
*/
void Texture_ClearInuse ()
{
	qtexture_t *q;

	for (q = g_qeglobals.d_qtextures; q; q = q->next)
		q->inuse = false;
}


/*
==================
Texture_FlushAll
==================
*/
void Texture_FlushAll()
{
	qtexture_t	*q, *qprev, *qnextex;
	unsigned int n;

	if (g_qeglobals.d_qtextures)
	{
		q = g_qeglobals.d_qtextures->next;
		qprev = g_qeglobals.d_qtextures;
		while (q != NULL && q != g_qeglobals.d_qtextures)
		{
			qnextex = q->next;

			n = q->texture_number;
			glDeleteTextures(1, &n);
			qprev->next = qnextex;
			free(q);

			q = qnextex;
		}
	}
	g_qeglobals.d_texturewin.Layout();
}


/*
==================
Texture_FlushUnused
==================
*/
void Texture_FlushUnused ()
{
	qtexture_t	*q, *qprev, *qnextex;
	unsigned int n;

	Sys_Printf("CMD: Flushing unused textures...\n");
	if (g_qeglobals.d_qtextures)
	{
		q = g_qeglobals.d_qtextures->next;
		qprev = g_qeglobals.d_qtextures;
		while (q != NULL && q != g_qeglobals.d_qtextures)
		{
			qnextex = q->next;

			if (!q->inuse)
			{
				n = q->texture_number;
				glDeleteTextures(1, &n);
				qprev->next = qnextex;
				free(q);
			}
			else
				qprev = q;

			q = qnextex;
		}
	}

	g_qeglobals.d_texturewin.Layout();
}

/*
==================
Texture_InitFromWad
==================
*/ 
void Texture_InitFromWad (char *file)
{
	char		 filepath[1024];
	FILE		*f;
	byte		*wadfile;
	wadinfo_t	*wadinfo;
	lumpinfo_t	*lumpinfo;
	miptex_t	*qtex;
	qtexture_t	*q;
	int			 numlumps;
	int			 i;
	char		*wname;

	sprintf(filepath, "%s/%s", ValueForKey(g_qeglobals.d_entityProject, "texturepath"), file);

	if ((f = fopen(filepath, "rb")) == NULL)
	{
		Sys_Printf("WARNING: Could not open %s\n", file);
		return;
	}
	fclose(f);

	Sys_Printf("CMD: Opening %s\n", file);
	
//	LoadFileNoCrash (filepath, (void **)&wadfile);
	LoadFile(filepath, (void **)&wadfile);

	if (strncmp((char*)wadfile, "WAD2", 4))
	{
		Sys_Printf("WARNING: %s is not a valid wadfile.\n", file);
		free(wadfile);
		return;
	}

	wadinfo = (wadinfo_t *)wadfile;
	numlumps = LittleLong(wadinfo->numlumps);
	lumpinfo = (lumpinfo_t *)(wadfile + LittleLong(wadinfo->infotableofs));

	wname = (char*)qmalloc(32 * sizeof(char));	// we do leak this tiny bit of memory if the same wad is loaded twice but I can live with that
	strncpy(wname, file, 31);

	for (i = 0; i < numlumps; i++, lumpinfo++)
	{
		qtex = (miptex_t *)(wadfile + LittleLong(lumpinfo->filepos));

		StringTolower(lumpinfo->name);

		if (lumpinfo->type != TYP_MIPTEX)
		{
			Sys_Printf("WARNING: %s is not a miptex. Ignoring...\n", lumpinfo->name);
			continue;
		}

		// sanity check.
		if (LittleLong(qtex->width) > 1024 || 
			LittleLong(qtex->width) < 1 || 
			LittleLong(qtex->height) > 1024 || 
			LittleLong(qtex->height) < 1)
		{
			Sys_Printf("WARNING: %s has wrong size. Ignoring...\n", lumpinfo->name);
			continue;
		}

		for (q = g_qeglobals.d_qtextures; q; q = q->next)
		{
			if (!strcmp(lumpinfo->name, q->name))
			{
				Sys_Printf("WARNING: %s is already loaded. Skipping..\n", lumpinfo->name);
				goto skip;
			}
		}
		
		Sys_Printf("CMD: Loading %s\n", lumpinfo->name);

//		q = Texture_LoadTexture((miptex_t *)(wadfile + LittleLong(lumpinfo->filepos)));
		q = Texture_LoadTexture(qtex);

		strncpy(q->name, lumpinfo->name, sizeof(q->name) - 1);
		q->inuse = false;
		q->wad = wname;
		q->next = g_qeglobals.d_qtextures;
		g_qeglobals.d_qtextures = q;

skip:;
	}
	free(wadfile);
} 

/*
==============
Texture_ShowWad
==============
*/
void Texture_ShowWad (int menunum)
{
	g_qeglobals.d_texturewin.origin[1] = 0;
	Sys_Printf("CMD: Loading all textures...\n");

	// load .wad file
	//strcpy(wadname, g_szTextureMenuNames[menunum - CMD_TEXTUREWAD]);

	Texture_InitFromWad(g_szTextureMenuNames[menunum - CMD_TEXTUREWAD]);

	g_qeglobals.d_texturewin.SortTextures();
	InspWnd_SetMode(W_TEXTURE);
	//TexWnd_Layout();
	Sys_UpdateWindows(W_TEXTURE);

	// select the first texture in the list
	if (!g_qeglobals.d_workTexDef.name[0])
		g_qeglobals.d_texturewin.SelectTexture(16, g_qeglobals.d_texturewin.height - 16);
}


/*
==============
Texture_ShowInuse
==============
*/
void Texture_ShowInuse ()
{
	Face *f;

	g_qeglobals.d_texturewin.origin[1] = 0;
	Sys_Printf("CMD: Selecting active textures...\n");
	Texture_ClearInuse();

	for (Brush *b = g_brActiveBrushes.next; b != NULL && b != &g_brActiveBrushes; b = b->next)
		for (f = b->brush_faces; f; f = f->next)
			Texture_ForName(f->texdef.name);

	for (Brush *b = g_brSelectedBrushes.next; b != NULL && b != &g_brSelectedBrushes; b = b->next)
		for (f = b->brush_faces; f; f = f->next)
			Texture_ForName(f->texdef.name);

	g_qeglobals.d_texturewin.SortTextures();
	InspWnd_SetMode(W_TEXTURE);
	Sys_UpdateWindows(W_TEXTURE);

	SetWindowText(g_qeglobals.d_hwndInspector, "Textures: in use");

	// select the first texture in the list
	if (!g_qeglobals.d_workTexDef.name[0])
		g_qeglobals.d_texturewin.SelectTexture(16, g_qeglobals.d_texturewin.height - 16);
}

//HFONT ghFont = NULL;	sikk - unused

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

	if(g_qeglobals.d_nInspectorMode == W_TEXTURE)
		g_qeglobals.d_texturewin.Layout();
}



