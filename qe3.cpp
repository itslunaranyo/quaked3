//==============================
//	qe3.c
//==============================

#include "qe3.h"


#define	SPEED_MOVE	32
#define	SPEED_TURN	22.5

qeglobals_t  g_qeglobals;


/*
==================
qmalloc
==================
*/
void *qmalloc (int size)
{
	void *b;

	b = malloc(size);
	memset(b, 0, size);

	return b;
}

/*
==================
CopyString
==================
*/
char *CopyString (char *s)
{
	char *b;

	b = (char*)malloc(strlen(s) + 1);
	strcpy(b, s);

	return b;
}

/*
==================
QE_Init
==================
*/
void QE_Init ()
{
	int i;	// sikk - Save Rebar Band Info

	/*
	** initialize variables
	*/
	g_qeglobals.d_nGridSize = 8;
	g_qeglobals.d_bShowGrid = true;
	g_qeglobals.d_xyz[0].dViewType = XY;
	g_qeglobals.d_fDefaultTexScale = 1.00f;	// sikk - Default Texture Size Dialog

	// set maximium undo levels
	Undo_SetMaxSize(g_qeglobals.d_savedinfo.nUndoLevels);

	Sys_UpdateGridStatusBar();

	// sikk - For bad texture name check during map save. Setting it to
	// -1 insures that if brush #0 has bad face, it'll be listed in console
	g_nBrushNumCheck = -1;

// sikk---> Save Rebar Band Info
	for (i = 0; i < 11; i++)
	{
		g_qeglobals.d_savedinfo.rbiSettings[i].cbSize	= sizeof(REBARBANDINFO);
		SendMessage(g_qeglobals.d_hwndRebar, RB_SETBANDINFO, (WPARAM)i, (LPARAM)(LPREBARBANDINFO)&g_qeglobals.d_savedinfo.rbiSettings[i]);
	}
/*	Code below Sets the saved Band order but doesn't function correctly
	as it doesn't update until a band is moved. ???
	j = 0;
	while (j < 11)
	{
		for (i = 0; i < 11; i++)
			if (ID_TOOLBAR + i == g_qeglobals.d_savedinfo.nRebarSavedIndex[j])
				SendMessage(g_qeglobals.d_hwndRebar, RB_MOVEBAND, (WPARAM)i, (LPARAM)j);

		j++;
	}
*/
// <---sikk

	/*
	** other stuff
	*/
	Texture_Init();
	Cam_Init();
	for (int i = 0; i < 4;i++)
		XYZ_Init(&g_qeglobals.d_xyz[i]);
	Z_Init();

	// sikk - Update User Interface
	QE_UpdateCommandUI();
}

/*
===========
QE_KeyDown
===========
*/
bool QE_KeyDown (int key)
{
// sikk - temp fix for accelerator problem
	if (GetTopWindow(g_qeglobals.d_hwndMain) == g_qeglobals.d_hwndInspector)
	{
		switch (key)
		{
		case 'N':
			PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_VIEW_ENTITY, 0);
			break;
		case 'O':
			PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_VIEW_CONSOLE, 0);
			break;
		case 'T':
			PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_VIEW_TEXTURE, 0);
			break;
		case VK_ESCAPE:
			PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_SELECTION_DESELECT, 0);
			break;

		default:
			return false;
		}
		return true;
	}
// <---sikk

// sikk---> Keyboard Texture Manipulation
	switch (key)
	{
	case VK_UP:
		if (GetKeyState(VK_SHIFT) < 0)
		{
			if (GetKeyState(VK_CONTROL) < 0)	
				Surf_RotateTexture(1);
			else
				Surf_ShiftTexture(0, g_qeglobals.d_nGridSize);
		}
		else if (GetKeyState(VK_CONTROL) < 0)
			Surf_ScaleTexture(0, 5);
		else
			VectorMA(g_qeglobals.d_camera.origin, SPEED_MOVE, g_qeglobals.d_camera.forward, g_qeglobals.d_camera.origin);

		Sys_UpdateWindows(W_CAMERA | W_XY);
		break;
	case VK_DOWN:
		if (GetKeyState(VK_SHIFT) < 0)
		{
			if (GetKeyState(VK_CONTROL) < 0)
				Surf_RotateTexture(-1);
			else
				Surf_ShiftTexture(0, -g_qeglobals.d_nGridSize);
		}
		else if (GetKeyState(VK_CONTROL) < 0)
			Surf_ScaleTexture(0, -5);
		else
			VectorMA(g_qeglobals.d_camera.origin, -SPEED_MOVE, g_qeglobals.d_camera.forward, g_qeglobals.d_camera.origin);

		Sys_UpdateWindows(W_CAMERA | W_XY);
		break;
	case VK_LEFT:
		if (GetKeyState(VK_SHIFT) < 0)
		{
			if (GetKeyState(VK_CONTROL) < 0)
				Surf_RotateTexture(15);
			else
				Surf_ShiftTexture(g_qeglobals.d_nGridSize, 0);
		}
		else if (GetKeyState(VK_CONTROL) < 0)
			Surf_ScaleTexture(-5, 0);
		else
			g_qeglobals.d_camera.angles[1] += SPEED_TURN;

		Sys_UpdateWindows(W_CAMERA | W_XY);
		break;
	case VK_RIGHT:
		if (GetKeyState(VK_SHIFT) < 0)
		{
			if (GetKeyState(VK_CONTROL) < 0)
				Surf_RotateTexture(-15);
			else
				Surf_ShiftTexture(-g_qeglobals.d_nGridSize, 0);
		}
		else if (GetKeyState(VK_CONTROL) < 0)
			Surf_ScaleTexture(5, 0);
		else
			g_qeglobals.d_camera.angles[1] -= SPEED_TURN;

		Sys_UpdateWindows(W_CAMERA | W_XY);
		break;
// <---sikk
	case 'D':
		g_qeglobals.d_camera.origin[2] += SPEED_MOVE;
		Sys_UpdateWindows(W_CAMERA | W_XY | W_Z);
		break;
	case 'C':
		g_qeglobals.d_camera.origin[2] -= SPEED_MOVE;
		Sys_UpdateWindows(W_CAMERA | W_XY | W_Z);
		break;
	case 'A':
		g_qeglobals.d_camera.angles[0] += SPEED_TURN;
		Cam_BoundAngles();
		Sys_UpdateWindows(W_CAMERA | W_XY);
		break;
	case 'Z':
		g_qeglobals.d_camera.angles[0] -= SPEED_TURN;
		Cam_BoundAngles();
		Sys_UpdateWindows(W_CAMERA | W_XY);
		break;
	case VK_COMMA:
		VectorMA (g_qeglobals.d_camera.origin, -SPEED_MOVE, g_qeglobals.d_camera.right, g_qeglobals.d_camera.origin);
		Sys_UpdateWindows(W_CAMERA | W_XY);
		break;
	case VK_PERIOD:
		VectorMA (g_qeglobals.d_camera.origin, SPEED_MOVE, g_qeglobals.d_camera.right, g_qeglobals.d_camera.origin);
		Sys_UpdateWindows(W_CAMERA | W_XY);
		break;
	case '0':	// sikk - fixed and added shortcut key
		PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_GRID_TOGGLE, 0);
		break;
	case '1':
		PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_GRID_1, 0);
		break;
	case '2':
		PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_GRID_2, 0);
		break;
	case '3':
		PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_GRID_4, 0);
		break;
	case '4':
		PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_GRID_8, 0);
		break;
	case '5':
		PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_GRID_16, 0);
		break;
	case '6':
		PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_GRID_32, 0);
		break;
	case '7':
		PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_GRID_64, 0);
		break;
	case '8':
		PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_GRID_128, 0);
		break;
	case '9':
		PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_GRID_256, 0);
		break;
	case 'B':
		PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_BRUSH_CYLINDER, 0);
		break;
	case 'E':
		PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_SELECTION_DRAGEDGES, 0);
		break;
	case 'F':	// sikk - added shortcut key
		PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_TEXTURES_FLUSH_UNUSED, 0);
		break;
		/*
	case 'G':	// sikk - added shortcut key
		// lunaran - removed it again because turning off snap to grid should require two submarine commanders to turn their keys simultaneously
		PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_GRID_SNAPTOGRID, 0);
		break;*/
	case 'H':
		PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_VIEW_HIDESHOW_HIDESELECTED, 0);
		break;
	case 'I':
		PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_SELECTION_INVERT, 0);
		break;
	case 'K':
		PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_MISC_SELECTENTITYCOLOR, 0);
		break;
	case 'L':	// sikk - added shortcut key
		PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_TEXTURES_LOCK, 0);
		break;
	case 'M':
		PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_FILE_IMPORTMAP, 0);
		break;
	case 'N':
		PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_VIEW_ENTITY, 0);
		break;
	case 'O':
		PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_VIEW_CONSOLE, 0);
		break;
	case 'P':
		PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_FILE_IMPORTPREFAB, 0);
		break;
	case 'Q':
		PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_VIEW_CAMERA, 0);
		break;
	case 'R':	// sikk - added shortcut key
		PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_SELECTION_ARBITRARYROTATION, 0);
		break;
	case 'S':
		PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_TEXTURES_INSPECTOR, 0);
		break;
	case 'T':
		PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_VIEW_TEXTURE, 0);
		break;
	case 'U':
		PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_TEXTURES_SHOWINUSE, 0);
		break;
	case 'V':
		PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_SELECTION_DRAGVERTICES, 0);
		break;
	case 'X':
		PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_SELECTION_CLIPPER, 0);
		break;
	case ' ':
		PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_SELECTION_CLONE, 0);
		break;
	case VK_BACK:
		PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_SELECTION_DELETE, 0);
		break;
	case VK_ESCAPE:
		PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_SELECTION_DESELECT, 0);
		break;
	case VK_RETURN:
		PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_SELECTION_CLIPSELECTED, 0);
		break;
	case VK_TAB:
		if (GetKeyState(VK_SHIFT) < 0)
			PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_VIEW_SWAPGRIDCAM, 0);
		else
			PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_VIEW_NEXTVIEW, 0);
		break;
	case VK_END:
		PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_VIEW_CENTER, 0);
		break;
	case VK_HOME:	// sikk - added shortcut key
		PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_VIEW_100, 0);
		break;
	case VK_INSERT:
		PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_VIEW_ZOOMIN, 0);
		break;
	case VK_DELETE:
		PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_VIEW_ZOOMOUT, 0);
		break;
	case VK_NEXT:
		PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_VIEW_DOWNFLOOR, 0);
		break;
	case VK_PRIOR:
		PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_VIEW_UPFLOOR, 0);
		break;
	case VK_BACKSLASH:	// This may not function on foreign keyboards (non English)
		PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_VIEW_CENTERONSELECTION, 0);
		break;
	case VK_MINUS:	// sikk - GridSize Decrease: This may not function on foreign keyboards (non English)
		{
			int i = g_qeglobals.d_nGridSize / 2;
			switch (i)
			{
			case 1:
				PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_GRID_1, 0);
				break;
			case 2:
				PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_GRID_2, 0);
				break;
			case 4:
				PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_GRID_4, 0);
				break;
			case 8:
				PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_GRID_8, 0);
				break;
			case 16:
				PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_GRID_16, 0);
				break;
			case 32:
				PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_GRID_32, 0);
				break;
			case 64:
				PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_GRID_64, 0);
				break;
			case 128:
				PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_GRID_128, 0);
				break;
			default:
				break;
			}
		}
		break;	
	case VK_PLUS:	// sikk - GridSize Increase: This may not function on foreign keyboards (non English)
		{
			int i = g_qeglobals.d_nGridSize * 2;
			switch (i)
			{
			case 2:
				PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_GRID_2, 0);
				break;
			case 4:
				PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_GRID_4, 0);
				break;
			case 8:
				PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_GRID_8, 0);
				break;
			case 16:
				PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_GRID_16, 0);
				break;
			case 32:
				PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_GRID_32, 0);
				break;
			case 64:
				PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_GRID_64, 0);
				break;
			case 128:
				PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_GRID_128, 0);
				break;
			case 256:
				PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_GRID_256, 0);
				break;
			default:
				break;
			}
		}
		break;
	case VK_F1: // sikk - open QE3 manual
		PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_HELP_HELP, 0);
		break;
	case VK_F2: // sikk - Map Info Dialog
		PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_EDIT_MAPINFO, 0);
		break;
	case VK_F3: // sikk - Entity Info Dialog
		PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_EDIT_ENTITYINFO, 0);
		break;
	case VK_F4: // sikk - Preferences Dialog
		PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_EDIT_PREFERENCES, 0);
		break;
	case VK_F5: // sikk - Project Dialog
		PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_FILE_EDITPROJECT, 0);
		break;
	case VK_F10: // sikk - added shortcut key
		PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_MISC_BENCHMARK, 0);
		break;
	case VK_F12:	// sikk - Test Map
		PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_MISC_TESTMAP, 0);
		break;

	default:
		return false;
	}

	return true;
}

/*
==================
QE_CheckOpenGLForErrors
==================
*/
void QE_CheckOpenGLForErrors (void)
{
    int	i;
    
    while ((i = glGetError()) != GL_NO_ERROR)
    {
		char buffer[100];
		sprintf(buffer, "OpenGL Error: %s", gluErrorString(i));
		Sys_Printf("***%s***\n", buffer);
    }
}

/*
===============
QE_CheckAutoSave

If five minutes have passed since making a change
and the map hasn't been saved, save it out.
===============
*/
void QE_CheckAutoSave ()
{
	static clock_t s_start;
	clock_t        now;

	if (!g_qeglobals.d_savedinfo.bAutosave)	// sikk - Preferences Dialog
		return;

	now = clock();

	if (g_bModified != 1 || !s_start)
	{
		s_start = now;
		return;
	}

	if (now - s_start > (CLOCKS_PER_SEC * 60 * g_qeglobals.d_savedinfo.nAutosave))	// sikk - Preferences Dialog
	{
		Sys_Printf("CMD: Autosaving...\n");
		Sys_Status("Autosaving...", 0);

		Map_SaveFile(ValueForKey(g_qeglobals.d_entityProject, "autosave"), false);

		Sys_Printf("MSG: Autosave successful.\n");
		Sys_Status("Autosave successful.", 0);

		g_bModified = 2;
		s_start = now;
	}
}

/*
===========
QE_LoadProject
===========
*/
bool QE_LoadProject (char *projectfile)
{
	char *data;

	Sys_Printf("CMD: QE_LoadProject (%s)\n", projectfile);

	if (LoadFileNoCrash(projectfile, (void**)&data) == -1)
		return false;
	StartTokenParsing(data);
	g_qeglobals.d_entityProject = Entity_Parse(true);
	if (!g_qeglobals.d_entityProject)
		Error("QE_LoadProject: Could not parse %s", projectfile);
	free(data);

	// usefull for the log file and debugging fucked up configurations from users:
	// output the basic information of the .qe3 project file
	Sys_Printf("MSG: basepath: %s\n",		ValueForKey(g_qeglobals.d_entityProject, "basepath"));
	Sys_Printf("MSG: remotebasepath: %s\n",	ValueForKey(g_qeglobals.d_entityProject, "remotebasepath"));
	Sys_Printf("MSG: mapspath: %s\n",		ValueForKey(g_qeglobals.d_entityProject, "mapspath"));
	Sys_Printf("MSG: autosavemap: %s\n",	ValueForKey(g_qeglobals.d_entityProject, "autosave"));
	Sys_Printf("MSG: entitypath: %s\n",		ValueForKey(g_qeglobals.d_entityProject, "entitypath"));
	Sys_Printf("MSG: texturepath: %s\n",	ValueForKey(g_qeglobals.d_entityProject, "texturepath"));
	Sys_Printf("MSG: defaultwads: %s\n",	ValueForKey(g_qeglobals.d_entityProject, "defaultwads"));
	Sys_Printf("MSG: toolspath: %s\n",		ValueForKey(g_qeglobals.d_entityProject, "rshcmd"));

	Eclass_InitForSourceDirectory(ValueForKey(g_qeglobals.d_entityProject, "entitypath"));

	EntWnd_FillClassList();	// list in entity window
	FillTextureMenu();
	FillBSPMenu();

	Map_New();

	return true;
}

/*
==================
QE_SingleBrush
==================
*/
bool QE_SingleBrush ()
{
	if ((!Select_HasBrushes()) ||
		(g_brSelectedBrushes.next->next != &g_brSelectedBrushes))
	{
		Sys_Printf("WARNING: Must have a single brush selected.\n");
		return false;
	}
	if (g_brSelectedBrushes.next->owner->eclass->fixedsize)
	{
		Sys_Printf("WARNING: Cannot manipulate fixed size entities.\n");
		return false;
	}

	return true;
}

/*
==================
QE_ConvertDOSToUnixName
==================
*/
void QE_ConvertDOSToUnixName (char *dst, const char *src)
{
	while (*src)
	{
		if (*src == '\\')
			*dst = '/';
		else
			*dst = *src;
		dst++; src++;
	}
	*dst = 0;
}

/*
==============
QE_ExpandBspString
==============
*/
void QE_ExpandBspString (char *bspaction, char *out, char *mapname)
{
	char   *in;
	char	src[1024];
	char	rsh[1024];
	char	base[512];

	ExtractFileName(mapname, base);
// sikk - using "mapspath" instead.... What other use is "mapspath" for?
//	sprintf(src, "%s/maps/%s", ValueForKey(g_qeglobals.d_entityProject, "remotebasepath"), base);
	sprintf(src, "%s%s", ValueForKey(g_qeglobals.d_entityProject, "mapspath"), base);

	StripExtension(src);
	strcpy(rsh, ValueForKey(g_qeglobals.d_entityProject, "rshcmd"));

	in = ValueForKey(g_qeglobals.d_entityProject, bspaction);
	while (*in)
	{
		if (in[0] == '!')
		{
			strcpy(out, rsh);
			out += strlen(rsh);
			in++;
			continue;
		}
		if (in[0] == '$')
		{
			strcpy(out, src);
			out += strlen(src);
			in++;
			continue;
		}
		if (in[0] == '@')
		{
			*out++ = '"';
			in++;
			continue;
		}
		*out++ = *in++;
	}
	*out = 0;
}

/*
==================
QE_ExpandRelativePath
==================
*/
char *QE_ExpandRelativePath (char *p)
{
	static char	temp[1024];
	char	   *base;

	if (!p || !p[0])
		return NULL;
	if (p[0] == '/' || p[0] == '\\')
		return p;

	base = ValueForKey(g_qeglobals.d_entityProject, "basepath");
	sprintf(temp, "%s/%s", base, p);
	return temp;
}

/*
==================
QE_CountBrushesAndUpdateStatusBar
==================
*/
void QE_CountBrushesAndUpdateStatusBar ()
{
	static int	s_lastbrushcount, s_lastentitycount, s_lasttexturecount;
	static bool	s_didonce;
	
//	entity_t	*e;
	brush_t		*b, *next;
	qtexture_t	*q;

	g_nNumBrushes = 0;
	g_nNumEntities = 0;
	g_nNumTextures = 0;
	
	if (g_brActiveBrushes.next != NULL)
	{
		for (b = g_brActiveBrushes.next; b != NULL && b != &g_brActiveBrushes; b = next)
		{
			next = b->next;
			if (b->brush_faces)
			{
				if (!b->owner->eclass->fixedsize)
					g_nNumBrushes++;
				else
					g_nNumEntities++;
			}
		}
	}

	for (q = g_qeglobals.d_qtextures; q; q = q->next)
    {
		if (q->name[0] == '(')
			continue; // don't count entity textures

		if (q->inuse)
			g_nNumTextures++;
	}

	if (((g_nNumBrushes != s_lastbrushcount) || 
		(g_nNumEntities != s_lastentitycount) || 
		(g_nNumTextures != s_lasttexturecount)) || 
		(!s_didonce))
	{
		Sys_UpdateBrushStatusBar();

		s_lastbrushcount = g_nNumBrushes;
		s_lastentitycount = g_nNumEntities;
		s_lasttexturecount = g_nNumTextures;
		s_didonce = true;
	}

	if (g_bModified)
	{
		char title[1024];
		sprintf(title, "%s *", g_szCurrentMap);
		QE_ConvertDOSToUnixName(title, title);
		Sys_SetTitle(title);
	}
}

// sikk--->	Update Menu Items & Toolbar Buttons
/*
==================
QE_UpdateCommandUI
==================
*/
void QE_UpdateCommandUI ()
{
	HMENU hMenu = GetMenu(g_qeglobals.d_hwndMain);

//===================================
// Edit Menu
//===================================
	// Cut
	EnableMenuItem(hMenu, ID_EDIT_CUT, (Select_HasBrushes() ? MF_ENABLED : MF_GRAYED));
	SendMessage(g_qeglobals.d_hwndToolbar2, TB_SETSTATE, (WPARAM)ID_EDIT_CUT, (Select_HasBrushes() ? (LPARAM)TBSTATE_ENABLED : (LPARAM)TBSTATE_INDETERMINATE));
	// Copy
	EnableMenuItem(hMenu, ID_EDIT_COPY, (Select_HasBrushes() ? MF_ENABLED : MF_GRAYED));
	SendMessage(g_qeglobals.d_hwndToolbar2, TB_SETSTATE, (WPARAM)ID_EDIT_COPY, (Select_HasBrushes() ? (LPARAM)TBSTATE_ENABLED : (LPARAM)TBSTATE_INDETERMINATE));
	// Paste
	EnableMenuItem(hMenu, ID_EDIT_PASTE, (g_brCopiedBrushes.next != NULL ? MF_ENABLED : MF_GRAYED));
	SendMessage(g_qeglobals.d_hwndToolbar2, TB_SETSTATE, (WPARAM)ID_EDIT_PASTE, (g_brCopiedBrushes.next != NULL ? (LPARAM)TBSTATE_ENABLED : (LPARAM)TBSTATE_INDETERMINATE));
	// Undo
	EnableMenuItem(hMenu, ID_EDIT_UNDO, Undo_UndoAvailable() ? MF_ENABLED : MF_GRAYED);
	SendMessage(g_qeglobals.d_hwndToolbar2, TB_SETSTATE, (WPARAM)ID_EDIT_UNDO, Undo_UndoAvailable() ? (LPARAM)TBSTATE_ENABLED : (LPARAM)TBSTATE_INDETERMINATE);
	// Redo
	EnableMenuItem(hMenu, ID_EDIT_REDO, Undo_RedoAvailable() ? MF_ENABLED : MF_GRAYED);
	SendMessage(g_qeglobals.d_hwndToolbar2, TB_SETSTATE, (WPARAM)ID_EDIT_REDO, Undo_RedoAvailable() ? (LPARAM)TBSTATE_ENABLED : (LPARAM)TBSTATE_INDETERMINATE);

//===================================
// View Menu
//===================================
	// Toolbar Bands
	CheckMenuItem(hMenu, ID_VIEW_TOOLBAR_FILEBAND,		(IsWindowVisible(g_qeglobals.d_hwndToolbar1) ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(hMenu, ID_VIEW_TOOLBAR_EDITBAND,		(IsWindowVisible(g_qeglobals.d_hwndToolbar2) ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(hMenu, ID_VIEW_TOOLBAR_EDIT2BAND,		(IsWindowVisible(g_qeglobals.d_hwndToolbar3) ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(hMenu, ID_VIEW_TOOLBAR_SELECTBAND,	(IsWindowVisible(g_qeglobals.d_hwndToolbar4) ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(hMenu, ID_VIEW_TOOLBAR_CSGBAND,		(IsWindowVisible(g_qeglobals.d_hwndToolbar5) ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(hMenu, ID_VIEW_TOOLBAR_MODEBAND,		(IsWindowVisible(g_qeglobals.d_hwndToolbar6) ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(hMenu, ID_VIEW_TOOLBAR_ENTITYBAND,	(IsWindowVisible(g_qeglobals.d_hwndToolbar7) ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(hMenu, ID_VIEW_TOOLBAR_BRUSHBAND,		(IsWindowVisible(g_qeglobals.d_hwndToolbar8) ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(hMenu, ID_VIEW_TOOLBAR_TEXTUREBAND,	(IsWindowVisible(g_qeglobals.d_hwndToolbar9) ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(hMenu, ID_VIEW_TOOLBAR_VIEWBAND,		(IsWindowVisible(g_qeglobals.d_hwndToolbar10) ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(hMenu, ID_VIEW_TOOLBAR_MISCBAND,		(IsWindowVisible(g_qeglobals.d_hwndToolbar11) ? MF_CHECKED : MF_UNCHECKED));
	// Status Bar
	CheckMenuItem(hMenu, ID_VIEW_STATUSBAR, (IsWindowVisible(g_qeglobals.d_hwndStatus) ? MF_CHECKED : MF_UNCHECKED));
	// XY Windows
	CheckMenuItem(hMenu, ID_VIEW_CAMERA,	(IsWindowVisible(g_qeglobals.d_hwndCamera) ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(hMenu, ID_VIEW_TOGGLE_XY, (g_qeglobals.d_savedinfo.bShow_XYZ[0] ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(hMenu, ID_VIEW_TOGGLE_XZ,	(g_qeglobals.d_savedinfo.bShow_XYZ[2] ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(hMenu, ID_VIEW_TOGGLE_YZ,	(g_qeglobals.d_savedinfo.bShow_XYZ[1] ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(hMenu, ID_VIEW_TOGGLE_Z,	(g_qeglobals.d_savedinfo.bShow_Z ? MF_CHECKED : MF_UNCHECKED));
	// Cubic Clipping
	CheckMenuItem(hMenu, ID_VIEW_CUBICCLIP, (g_qeglobals.d_savedinfo.bCubicClip ? MF_CHECKED : MF_UNCHECKED));
	SendMessage(g_qeglobals.d_hwndToolbar7, TB_CHECKBUTTON, (WPARAM)ID_VIEW_CUBICCLIP, (g_qeglobals.d_savedinfo.bCubicClip ? (LPARAM)TRUE : (LPARAM)FALSE));
	// Filter Commands
	CheckMenuItem(hMenu, ID_VIEW_SHOWAXIS,			(g_qeglobals.d_savedinfo.bShow_Axis ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(hMenu, ID_VIEW_SHOWBLOCKS,		(g_qeglobals.d_savedinfo.bShow_Blocks ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(hMenu, ID_VIEW_SHOWCAMERAGRID,	(g_qeglobals.d_savedinfo.bShow_CameraGrid ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(hMenu, ID_VIEW_SHOWCOORDINATES,	(g_qeglobals.d_savedinfo.bShow_Coordinates ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(hMenu, ID_VIEW_SHOWLIGHTRADIUS,	(g_qeglobals.d_savedinfo.bShow_LightRadius ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(hMenu, ID_VIEW_SHOWMAPBOUNDRY,	(g_qeglobals.d_savedinfo.bShow_MapBoundry ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(hMenu, ID_VIEW_SHOWNAMES,			(g_qeglobals.d_savedinfo.bShow_Names ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(hMenu, ID_VIEW_SHOWSIZEINFO,		(g_qeglobals.d_savedinfo.bShow_SizeInfo ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(hMenu, ID_VIEW_SHOWVIEWNAME,		(g_qeglobals.d_savedinfo.bShow_Viewname ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(hMenu, ID_VIEW_SHOWWORKZONE,		(g_qeglobals.d_savedinfo.bShow_Workzone ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(hMenu, ID_VIEW_SHOWANGLES,		((g_qeglobals.d_savedinfo.nExclude & EXCLUDE_ANGLES) ? MF_UNCHECKED : MF_CHECKED));
	CheckMenuItem(hMenu, ID_VIEW_SHOWCLIP,			((g_qeglobals.d_savedinfo.nExclude & EXCLUDE_CLIP) ? MF_UNCHECKED : MF_CHECKED));
	CheckMenuItem(hMenu, ID_VIEW_SHOWENT,			((g_qeglobals.d_savedinfo.nExclude & EXCLUDE_ENT) ? MF_UNCHECKED : MF_CHECKED));
	CheckMenuItem(hMenu, ID_VIEW_SHOWFUNCWALL,		((g_qeglobals.d_savedinfo.nExclude & EXCLUDE_FUNC_WALL) ? MF_UNCHECKED : MF_CHECKED));
	CheckMenuItem(hMenu, ID_VIEW_SHOWLIGHTS,		((g_qeglobals.d_savedinfo.nExclude & EXCLUDE_LIGHTS) ? MF_UNCHECKED : MF_CHECKED));
	CheckMenuItem(hMenu, ID_VIEW_SHOWPATH,			((g_qeglobals.d_savedinfo.nExclude & EXCLUDE_PATHS) ? MF_UNCHECKED : MF_CHECKED));
	CheckMenuItem(hMenu, ID_VIEW_SHOWSKY,			((g_qeglobals.d_savedinfo.nExclude & EXCLUDE_SKY) ? MF_UNCHECKED : MF_CHECKED));
	CheckMenuItem(hMenu, ID_VIEW_SHOWWATER,			((g_qeglobals.d_savedinfo.nExclude & EXCLUDE_WATER) ? MF_UNCHECKED : MF_CHECKED));
	CheckMenuItem(hMenu, ID_VIEW_SHOWWORLD,			((g_qeglobals.d_savedinfo.nExclude & EXCLUDE_WORLD) ? MF_UNCHECKED : MF_CHECKED));
	CheckMenuItem(hMenu, ID_VIEW_SHOWHINT,			((g_qeglobals.d_savedinfo.nExclude & EXCLUDE_HINT) ? MF_UNCHECKED : MF_CHECKED));
	CheckMenuItem(hMenu, ID_VIEW_SHOWDETAIL,		((g_qeglobals.d_savedinfo.nExclude & EXCLUDE_DETAIL) ? MF_UNCHECKED : MF_CHECKED));

//===================================
// Selection Menu
//===================================
	// Clipper Mode
	CheckMenuItem(hMenu, ID_SELECTION_CLIPPER, (g_qeglobals.d_bClipMode ? MF_CHECKED : MF_UNCHECKED));
	SendMessage(g_qeglobals.d_hwndToolbar6, TB_CHECKBUTTON, (WPARAM)ID_SELECTION_CLIPPER, (g_qeglobals.d_bClipMode ? (LPARAM)TRUE : (LPARAM)FALSE));
	// Drag Edge Mode 
	CheckMenuItem(hMenu, ID_SELECTION_DRAGEDGES, (g_qeglobals.d_selSelectMode == sel_edge) ? MF_CHECKED : MF_UNCHECKED);
	SendMessage(g_qeglobals.d_hwndToolbar6, TB_CHECKBUTTON, (WPARAM)ID_SELECTION_DRAGEDGES, (g_qeglobals.d_selSelectMode == sel_edge ? (LPARAM)TRUE : (LPARAM)FALSE));
	// Drag Vertex Mode 
	CheckMenuItem(hMenu, ID_SELECTION_DRAGVERTICES, (g_qeglobals.d_selSelectMode == sel_vertex) ? MF_CHECKED : MF_UNCHECKED);
	SendMessage(g_qeglobals.d_hwndToolbar6, TB_CHECKBUTTON, (WPARAM)ID_SELECTION_DRAGVERTICES, (g_qeglobals.d_selSelectMode == sel_vertex ? (LPARAM)TRUE : (LPARAM)FALSE));
	// Scale Lock X
	CheckMenuItem(hMenu, ID_SELECTION_SCALELOCKX, (g_qeglobals.d_savedinfo.bScaleLockX ? MF_CHECKED : MF_UNCHECKED));
	SendMessage(g_qeglobals.d_hwndToolbar6, TB_CHECKBUTTON, (WPARAM)ID_SELECTION_SCALELOCKX, (g_qeglobals.d_savedinfo.bScaleLockX ? (LPARAM)TRUE : (LPARAM)FALSE));
	// Scale Lock Y
	CheckMenuItem(hMenu, ID_SELECTION_SCALELOCKY, (g_qeglobals.d_savedinfo.bScaleLockY ? MF_CHECKED : MF_UNCHECKED));
	SendMessage(g_qeglobals.d_hwndToolbar6, TB_CHECKBUTTON, (WPARAM)ID_SELECTION_SCALELOCKY, (g_qeglobals.d_savedinfo.bScaleLockY ? (LPARAM)TRUE : (LPARAM)FALSE));
	// Scale Lock Z
	CheckMenuItem(hMenu, ID_SELECTION_SCALELOCKZ, (g_qeglobals.d_savedinfo.bScaleLockZ ? MF_CHECKED : MF_UNCHECKED));
	SendMessage(g_qeglobals.d_hwndToolbar6, TB_CHECKBUTTON, (WPARAM)ID_SELECTION_SCALELOCKZ, (g_qeglobals.d_savedinfo.bScaleLockZ ? (LPARAM)TRUE : (LPARAM)FALSE));

//===================================
// Grid Menu
//===================================
	CheckMenuItem(hMenu, ID_GRID_TOGGLE, (g_qeglobals.d_bShowGrid ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(hMenu, ID_GRID_SNAPTOGRID, (g_qeglobals.d_savedinfo.bNoClamp ? MF_UNCHECKED : MF_CHECKED));

//===================================
// Texture Menu
//===================================
	CheckMenuItem(hMenu, ID_TEXTURES_LOCK, (g_qeglobals.d_bTextureLock ? MF_CHECKED : MF_UNCHECKED));

//===================================
// Region Menu
//===================================
	EnableMenuItem(hMenu, ID_REGION_SETXZ, (g_qeglobals.d_savedinfo.bShow_XYZ[2] ? MF_ENABLED : MF_GRAYED));
	EnableMenuItem(hMenu, ID_REGION_SETYZ, (g_qeglobals.d_savedinfo.bShow_XYZ[1] ? MF_ENABLED : MF_GRAYED));
}