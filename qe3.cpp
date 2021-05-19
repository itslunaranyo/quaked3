//==============================
//	qe3.c
//==============================

#include "qe3.h"

#define	SPEED_MOVE	32.0f
#define	SPEED_TURN	22.5f

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
pointOnGrid
==================
*/
vec3 pointOnGrid(const vec3 point)
{
	// glm's round proved to be kind of funky
	//point = glm::round(point / (float)g_qeglobals.d_nGridSize + 0.5f) * (float)g_qeglobals.d_nGridSize;
	vec3 out;
	for (int i = 0; i < 3; i++)
	{
		out[i] = qround(point[i], g_qeglobals.d_nGridSize);
	}
	return out;
}

/*
==================
QE_InitProject
==================
*/
void QE_InitProject()
{
	char	szProject[_MAX_PATH];	// sikk - Load Last Project

	// sikk - If 'Load Last Project' is checked, load that file,
	// else load default file
	if (g_qeglobals.d_savedinfo.bLoadLastProject)
	{
		if (strlen(g_qeglobals.d_savedinfo.szLastProject) > 0)
			strcpy(szProject, g_qeglobals.d_savedinfo.szLastProject);
		else
		{
			strcpy(g_qeglobals.d_savedinfo.szLastProject, "scripts/quake.qe3");
			strcpy(szProject, g_qeglobals.d_savedinfo.szLastProject);
		}
	}
	else
	{
		strcpy(szProject, "scripts/quake.qe3");
		strcpy(g_qeglobals.d_savedinfo.szLastProject, szProject);
	}

	// the project file can be specified on the command line, (must be first parameter)
	// or implicitly found in the scripts directory
	if (g_pszArgV[1])
	{
		if (!QE_LoadProject(g_pszArgV[1]))
			Error("Could not load %s project file.", g_pszArgV[1]);
	}
	else if (!QE_LoadProject(szProject))
	{
		DoProject(true);	// sikk - Manually create project file if none is found
		if (!QE_LoadProject("scripts/quake.qe3"))
			Error("Could not load scripts/quake.qe3 project file.");
	}
}

/*
==================
QE_Init
==================
*/
void QE_Init ()
{
	int i;	// sikk - Save Rebar Band Info

	QE_InitProject();

	// initialize variables
	g_qeglobals.d_nGridSize = 8;
	g_qeglobals.d_bShowGrid = true;
	g_qeglobals.d_vXYZ[0].SetAxis(XY);
	g_qeglobals.d_fDefaultTexScale = 1.00f;	// sikk - Default Texture Size Dialog
	g_qeglobals.d_clipTool = nullptr;
	g_qeglobals.d_v3WorkMin = vec3(0);
	g_qeglobals.d_v3WorkMax = vec3(8);
	g_qeglobals.d_savedinfo.nExclude |= BFL_HIDDEN;	// hidden things are always filtered

	// create tools - creation order determines which tools get first chance to handle inputs
	new SelectTool();
	new TextureTool();
	new ManipTool();

	// set maximium undo levels
	//Undo::SetMaxSize(g_qeglobals.d_savedinfo.nUndoLevels);

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

	// other stuff
	Textures::Init();
	g_map.RegionOff();	// sikk - For initiating Map Size change

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
	bool shift, ctrl;
	shift = (GetKeyState(VK_SHIFT) < 0);
	ctrl = (GetKeyState(VK_CONTROL) < 0);

	switch (key)
	{
	case VK_UP:
		if (shift || ctrl)
		{
			if (shift && ctrl)
				g_qeglobals.d_texTool->RotateTexture(1);
			else if (shift)
				g_qeglobals.d_texTool->ShiftTexture(0, g_qeglobals.d_nGridSize);
			else if (ctrl)
				g_qeglobals.d_texTool->ScaleTexture(0, 0.05f);
			Sys_UpdateWindows(W_SURF);
		}
		else
			g_qeglobals.d_vCamera.origin = g_qeglobals.d_vCamera.origin + SPEED_MOVE * g_qeglobals.d_vCamera.forward;

		Sys_UpdateWindows(W_CAMERA | W_XY);
		break;
	case VK_DOWN:
		if (shift || ctrl)
		{
			if (shift && ctrl)
				g_qeglobals.d_texTool->RotateTexture(-1);
			else if (shift)
				g_qeglobals.d_texTool->ShiftTexture(0, -g_qeglobals.d_nGridSize);
			else if (ctrl)
				g_qeglobals.d_texTool->ScaleTexture(0, -0.05f);
			Sys_UpdateWindows(W_SURF);
		}
		else
			g_qeglobals.d_vCamera.origin = g_qeglobals.d_vCamera.origin + -SPEED_MOVE * g_qeglobals.d_vCamera.forward;

		Sys_UpdateWindows(W_CAMERA | W_XY);
		break;
	case VK_LEFT:
		if (shift || ctrl)
		{
			if (shift && ctrl)
				g_qeglobals.d_texTool->RotateTexture(15);
			else if (shift)
				g_qeglobals.d_texTool->ShiftTexture(g_qeglobals.d_nGridSize, 0);
			else if (ctrl)
				g_qeglobals.d_texTool->ScaleTexture(0.05f, 0);
			Sys_UpdateWindows(W_SURF);
		}
		else
			g_qeglobals.d_vCamera.angles[1] += SPEED_TURN;

		Sys_UpdateWindows(W_CAMERA | W_XY);
		break;
	case VK_RIGHT:
		if (shift || ctrl)
		{
			if (shift && ctrl)
				g_qeglobals.d_texTool->RotateTexture(-15);
			else if (shift)
				g_qeglobals.d_texTool->ShiftTexture(-g_qeglobals.d_nGridSize, 0);
			else if (ctrl)
				g_qeglobals.d_texTool->ScaleTexture(-0.05f, 0);
			Sys_UpdateWindows(W_SURF);
		}
		else
			g_qeglobals.d_vCamera.angles[1] -= SPEED_TURN;

		Sys_UpdateWindows(W_CAMERA | W_XY);
		break;

	case 'D':
		g_qeglobals.d_vCamera.origin[2] += SPEED_MOVE;
		Sys_UpdateWindows(W_CAMERA | W_XY | W_Z);
		break;
	case 'C':
		g_qeglobals.d_vCamera.origin[2] -= SPEED_MOVE;
		Sys_UpdateWindows(W_CAMERA | W_XY | W_Z);
		break;
	case 'A':
		g_qeglobals.d_vCamera.angles[0] += SPEED_TURN;
		g_qeglobals.d_vCamera.BoundAngles();
		Sys_UpdateWindows(W_CAMERA | W_XY);
		break;
	case 'Z':
		g_qeglobals.d_vCamera.angles[0] -= SPEED_TURN;
		g_qeglobals.d_vCamera.BoundAngles();
		Sys_UpdateWindows(W_CAMERA | W_XY);
		break;
	case VK_COMMA:
		g_qeglobals.d_vCamera.origin = g_qeglobals.d_vCamera.origin + -SPEED_MOVE * g_qeglobals.d_vCamera.right;
		Sys_UpdateWindows(W_CAMERA | W_XY);
		break;
	case VK_PERIOD:
		g_qeglobals.d_vCamera.origin = g_qeglobals.d_vCamera.origin + SPEED_MOVE * g_qeglobals.d_vCamera.right;
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
	//case 'P':
	//	PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_FILE_IMPORTPREFAB, 0);
	//	break;
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
		if (shift)
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

void QE_SaveMap()
{
	g_map.SaveToFile(g_map.name, false);	// ignore region
	g_cmdQueue.SetSaved();
	g_map.autosaveTime = clock();

	Sys_UpdateWindows(W_TITLE);
}

void QE_UpdateTitle()
{
	char tmp[MAX_PATH + 2];

	sprintf(tmp, "%s%s",
		g_map.hasFilename ? g_map.name : "unnamed",
		g_cmdQueue.IsModified() ? " *" : "" );
	
	Sys_SetTitle(tmp);
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
	//static clock_t s_start;
	clock_t        now;

	if (!g_qeglobals.d_savedinfo.bAutosave)	// sikk - Preferences Dialog
		return;
	if (!g_cmdQueue.IsModified())
		return;

	now = clock();

	if (!g_map.autosaveTime)
	{
		g_map.autosaveTime = now;
		return;
	}

	if (now - g_map.autosaveTime > (CLOCKS_PER_SEC * 60 * g_qeglobals.d_savedinfo.nAutosave))	// sikk - Preferences Dialog
	{
		Sys_Printf("CMD: Autosaving...\n");
		Sys_Status("Autosaving...", 0);

		g_map.SaveToFile(g_qeglobals.d_entityProject->GetKeyValue("autosave"), false);

		Sys_Printf("MSG: Autosave successful.\n");
		Sys_Status("Autosave successful.", 0);

		g_map.autosaveTime = 0;
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

	if (IO_LoadFileNoCrash(projectfile, (void**)&data) == -1)
		return false;
	StartTokenParsing(data);
	g_qeglobals.d_entityProject = Entity::Parse(true);
	if (!g_qeglobals.d_entityProject)
		Error("QE_LoadProject: Could not parse %s", projectfile);
	free(data);

	// usefull for the log file and debugging fucked up configurations from users:
	// output the basic information of the .qe3 project file
	Sys_Printf("MSG: basepath: %s\n",		g_qeglobals.d_entityProject->GetKeyValue("basepath"));
	Sys_Printf("MSG: remotebasepath: %s\n",	g_qeglobals.d_entityProject->GetKeyValue("remotebasepath"));
	Sys_Printf("MSG: mapspath: %s\n",		g_qeglobals.d_entityProject->GetKeyValue("mapspath"));
	Sys_Printf("MSG: autosavemap: %s\n",	g_qeglobals.d_entityProject->GetKeyValue("autosave"));
	Sys_Printf("MSG: entitypath: %s\n",		g_qeglobals.d_entityProject->GetKeyValue("entitypath"));
	Sys_Printf("MSG: texturepath: %s\n",	g_qeglobals.d_entityProject->GetKeyValue("texturepath"));
	Sys_Printf("MSG: defaultwads: %s\n",	g_qeglobals.d_entityProject->GetKeyValue("defaultwads"));
	Sys_Printf("MSG: toolspath: %s\n",		g_qeglobals.d_entityProject->GetKeyValue("rshcmd"));

	EntClass::InitForSourceDirectory(g_qeglobals.d_entityProject->GetKeyValue("entitypath"));

	//EntWnd_FillClassList();	// list in entity window
	g_qeglobals.d_wndEntity->FillClassList();
	FillTextureMenu();
	FillBSPMenu();

	g_map.New();

	return true;
}

/*
==================
QE_BestViewAxis
==================
*/
int QE_BestViewAxis()
{
	for (int i = 0; i < 3; i++)
	{
		if (g_qeglobals.d_wndGrid[i]->IsOnTop())
			return g_qeglobals.d_wndGrid[i]->xyzv->GetAxis();
	}
	return XY;
}

/*
==================
QE_SingleBrush
==================
*/
bool QE_SingleBrush ()
{
	if ((!Selection::HasBrushes()) ||
		(g_brSelectedBrushes.next->next != &g_brSelectedBrushes))
	{
		Sys_Printf("WARNING: Must have a single brush selected.\n");
		return false;
	}
	if (g_brSelectedBrushes.next->owner->IsPoint())
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
//	sprintf(src, "%s/maps/%s", g_qeglobals.d_entityProject->GetKeyValue("remotebasepath"), base);
	sprintf(src, "%s%s", g_qeglobals.d_entityProject->GetKeyValue("mapspath"), base);

	StripExtension(src);
	strcpy(rsh, g_qeglobals.d_entityProject->GetKeyValue("rshcmd"));

	in = g_qeglobals.d_entityProject->GetKeyValue(bspaction);
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

	base = g_qeglobals.d_entityProject->GetKeyValue("basepath");
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
	
//	Entity	*e;
	Brush		*b, *next;
	Texture	*q;

	g_map.numBrushes = 0;
	g_map.numEntities = 0;
	g_map.numTextures = 0;
	
	if (g_map.brActive.next != NULL)
	{
		for (b = g_map.brActive.next; b != NULL && b != &g_map.brActive; b = next)
		{
			next = b->next;
			if (b->faces)
			{
				if (b->owner->IsPoint())
					g_map.numEntities++;
				else
					g_map.numBrushes++;
			}
		}
	}

	for (q = g_qeglobals.d_qtextures; q; q = q->next)
    {
		if (q->name[0] == '#')
			continue; // don't count entity textures

		if (q->used)
			g_map.numTextures++;
	}

	if (((g_map.numBrushes != s_lastbrushcount) || 
		(g_map.numEntities != s_lastentitycount) || 
		(g_map.numTextures != s_lasttexturecount)) || 
		(!s_didonce))
	{
		Sys_UpdateBrushStatusBar();

		s_lastbrushcount = g_map.numBrushes;
		s_lastentitycount = g_map.numEntities;
		s_lasttexturecount = g_map.numTextures;
		s_didonce = true;
	}
	/*
	//if (g_map.modified)
	if (g_cmdQueue.IsModified())
	{
		char title[1024];
		sprintf(title, "%s *", g_map.name);
		QE_ConvertDOSToUnixName(title, title);
		Sys_SetTitle(title);
	}*/
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
	// lunaran - leave cut/copy/paste always enabled, so they work in the console and other text fields
	// Cut
//	EnableMenuItem(hMenu, ID_EDIT_CUT, (Selection::HasBrushes() ? MF_ENABLED : MF_GRAYED));
//	SendMessage(g_qeglobals.d_hwndToolbar[1], TB_SETSTATE, (WPARAM)ID_EDIT_CUT, (Selection::HasBrushes() ? (LPARAM)TBSTATE_ENABLED : (LPARAM)TBSTATE_INDETERMINATE));
	// Copy
//	EnableMenuItem(hMenu, ID_EDIT_COPY, (Selection::HasBrushes() ? MF_ENABLED : MF_GRAYED));
//	SendMessage(g_qeglobals.d_hwndToolbar[1], TB_SETSTATE, (WPARAM)ID_EDIT_COPY, (Selection::HasBrushes() ? (LPARAM)TBSTATE_ENABLED : (LPARAM)TBSTATE_INDETERMINATE));
	// Paste
//	EnableMenuItem(hMenu, ID_EDIT_PASTE, (g_map.copiedBrushes.next != NULL ? MF_ENABLED : MF_GRAYED));
//	SendMessage(g_qeglobals.d_hwndToolbar[1], TB_SETSTATE, (WPARAM)ID_EDIT_PASTE, (g_map.copiedBrushes.next != NULL ? (LPARAM)TBSTATE_ENABLED : (LPARAM)TBSTATE_INDETERMINATE));

	// Undo
	EnableMenuItem(hMenu, ID_EDIT_UNDO, g_cmdQueue.CanUndo() ? MF_ENABLED : MF_GRAYED);
	SendMessage(g_qeglobals.d_hwndToolbar[1], TB_SETSTATE, (WPARAM)ID_EDIT_UNDO, g_cmdQueue.CanUndo() ? (LPARAM)TBSTATE_ENABLED : (LPARAM)TBSTATE_INDETERMINATE);
	// Redo
	EnableMenuItem(hMenu, ID_EDIT_REDO, g_cmdQueue.CanRedo() ? MF_ENABLED : MF_GRAYED);
	SendMessage(g_qeglobals.d_hwndToolbar[1], TB_SETSTATE, (WPARAM)ID_EDIT_REDO, g_cmdQueue.CanRedo() ? (LPARAM)TBSTATE_ENABLED : (LPARAM)TBSTATE_INDETERMINATE);

//===================================
// View Menu
//===================================
	// Toolbar Bands
	CheckMenuItem(hMenu, ID_VIEW_TOOLBAR_FILEBAND,		(IsWindowVisible(g_qeglobals.d_hwndToolbar[0]) ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(hMenu, ID_VIEW_TOOLBAR_EDITBAND,		(IsWindowVisible(g_qeglobals.d_hwndToolbar[1]) ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(hMenu, ID_VIEW_TOOLBAR_EDIT2BAND,		(IsWindowVisible(g_qeglobals.d_hwndToolbar[2]) ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(hMenu, ID_VIEW_TOOLBAR_SELECTBAND,	(IsWindowVisible(g_qeglobals.d_hwndToolbar[3]) ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(hMenu, ID_VIEW_TOOLBAR_CSGBAND,		(IsWindowVisible(g_qeglobals.d_hwndToolbar[4]) ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(hMenu, ID_VIEW_TOOLBAR_MODEBAND,		(IsWindowVisible(g_qeglobals.d_hwndToolbar[5]) ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(hMenu, ID_VIEW_TOOLBAR_ENTITYBAND,	(IsWindowVisible(g_qeglobals.d_hwndToolbar[6]) ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(hMenu, ID_VIEW_TOOLBAR_BRUSHBAND,		(IsWindowVisible(g_qeglobals.d_hwndToolbar[7]) ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(hMenu, ID_VIEW_TOOLBAR_TEXTUREBAND,	(IsWindowVisible(g_qeglobals.d_hwndToolbar[8]) ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(hMenu, ID_VIEW_TOOLBAR_VIEWBAND,		(IsWindowVisible(g_qeglobals.d_hwndToolbar[9]) ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(hMenu, ID_VIEW_TOOLBAR_MISCBAND,		(IsWindowVisible(g_qeglobals.d_hwndToolbar[10]) ? MF_CHECKED : MF_UNCHECKED));
	// Status Bar
	CheckMenuItem(hMenu, ID_VIEW_STATUSBAR, (IsWindowVisible(g_qeglobals.d_hwndStatus) ? MF_CHECKED : MF_UNCHECKED));
	// XY Windows
	CheckMenuItem(hMenu, ID_VIEW_CAMERA,	(IsWindowVisible(g_qeglobals.d_hwndCamera) ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(hMenu, ID_VIEW_TOGGLE_XY, (IsWindowVisible(g_qeglobals.d_hwndXYZ[0]) ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(hMenu, ID_VIEW_TOGGLE_XZ,	(IsWindowVisible(g_qeglobals.d_hwndXYZ[2]) ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(hMenu, ID_VIEW_TOGGLE_YZ,	(IsWindowVisible(g_qeglobals.d_hwndXYZ[1]) ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(hMenu, ID_VIEW_TOGGLE_Z,	(IsWindowVisible(g_qeglobals.d_hwndZ) ? MF_CHECKED : MF_UNCHECKED));
	// Cubic Clipping
	CheckMenuItem(hMenu, ID_VIEW_CUBICCLIP, (g_qeglobals.d_savedinfo.bCubicClip ? MF_CHECKED : MF_UNCHECKED));
	SendMessage(g_qeglobals.d_hwndToolbar[6], TB_CHECKBUTTON, (WPARAM)ID_VIEW_CUBICCLIP, (g_qeglobals.d_savedinfo.bCubicClip ? (LPARAM)TRUE : (LPARAM)FALSE));
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
	CheckMenuItem(hMenu, ID_VIEW_SHOWANGLES,		(g_qeglobals.d_savedinfo.bShow_Angles ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(hMenu, ID_VIEW_SHOWPATH,			(g_qeglobals.d_savedinfo.bShow_Paths ? MF_CHECKED : MF_UNCHECKED));

	CheckMenuItem(hMenu, ID_VIEW_SHOWCLIP,			((g_qeglobals.d_savedinfo.nExclude & BFL_CLIP) ? MF_UNCHECKED : MF_CHECKED));
	CheckMenuItem(hMenu, ID_VIEW_SHOWBRUSHENTS,		((g_qeglobals.d_savedinfo.nExclude & EFL_BRUSHENTITY) ? MF_UNCHECKED : MF_CHECKED));
	CheckMenuItem(hMenu, ID_VIEW_SHOWPOINTENTS,		((g_qeglobals.d_savedinfo.nExclude & EFL_POINTENTITY) ? MF_UNCHECKED : MF_CHECKED));
	CheckMenuItem(hMenu, ID_VIEW_SHOWFUNCWALL,		((g_qeglobals.d_savedinfo.nExclude & EFL_FUNCWALL) ? MF_UNCHECKED : MF_CHECKED));
	CheckMenuItem(hMenu, ID_VIEW_SHOWLIGHTS,		((g_qeglobals.d_savedinfo.nExclude & EFL_LIGHT) ? MF_UNCHECKED : MF_CHECKED));
	CheckMenuItem(hMenu, ID_VIEW_SHOWSKY,			((g_qeglobals.d_savedinfo.nExclude & BFL_SKY) ? MF_UNCHECKED : MF_CHECKED));
	CheckMenuItem(hMenu, ID_VIEW_SHOWWATER,			((g_qeglobals.d_savedinfo.nExclude & BFL_LIQUID) ? MF_UNCHECKED : MF_CHECKED));
	CheckMenuItem(hMenu, ID_VIEW_SHOWWORLD,			((g_qeglobals.d_savedinfo.nExclude & EFL_WORLDSPAWN) ? MF_UNCHECKED : MF_CHECKED));
	CheckMenuItem(hMenu, ID_VIEW_SHOWHINT,			((g_qeglobals.d_savedinfo.nExclude & BFL_HINT) ? MF_UNCHECKED : MF_CHECKED));
	CheckMenuItem(hMenu, ID_VIEW_SHOWDETAIL,		((g_qeglobals.d_savedinfo.nExclude & EFL_DETAIL) ? MF_UNCHECKED : MF_CHECKED));
	CheckMenuItem(hMenu, ID_VIEW_SHOWMONSTERS,		((g_qeglobals.d_savedinfo.nExclude & EFL_MONSTER) ? MF_UNCHECKED : MF_CHECKED));
	CheckMenuItem(hMenu, ID_VIEW_SHOWTRIGGERS,		((g_qeglobals.d_savedinfo.nExclude & EFL_TRIGGER) ? MF_UNCHECKED : MF_CHECKED));


//===================================
// Selection Menu
//===================================
	// Clipper Mode
	CheckMenuItem(hMenu, ID_SELECTION_CLIPPER, (g_qeglobals.d_clipTool ? MF_CHECKED : MF_UNCHECKED));
	SendMessage(g_qeglobals.d_hwndToolbar[5], TB_CHECKBUTTON, (WPARAM)ID_SELECTION_CLIPPER, (g_qeglobals.d_clipTool ? (LPARAM)TRUE : (LPARAM)FALSE));
	// Drag Edge Mode 
	CheckMenuItem(hMenu, ID_SELECTION_DRAGEDGES, (g_qeglobals.d_selSelectMode == sel_edge) ? MF_CHECKED : MF_UNCHECKED);
	SendMessage(g_qeglobals.d_hwndToolbar[5], TB_CHECKBUTTON, (WPARAM)ID_SELECTION_DRAGEDGES, (g_qeglobals.d_selSelectMode == sel_edge ? (LPARAM)TRUE : (LPARAM)FALSE));
	// Drag Vertex Mode 
	CheckMenuItem(hMenu, ID_SELECTION_DRAGVERTICES, (g_qeglobals.d_selSelectMode == sel_vertex) ? MF_CHECKED : MF_UNCHECKED);
	SendMessage(g_qeglobals.d_hwndToolbar[5], TB_CHECKBUTTON, (WPARAM)ID_SELECTION_DRAGVERTICES, (g_qeglobals.d_selSelectMode == sel_vertex ? (LPARAM)TRUE : (LPARAM)FALSE));
	// Scale Lock X
	CheckMenuItem(hMenu, ID_SELECTION_SCALELOCKX, (g_qeglobals.d_savedinfo.bScaleLockX ? MF_CHECKED : MF_UNCHECKED));
	SendMessage(g_qeglobals.d_hwndToolbar[5], TB_CHECKBUTTON, (WPARAM)ID_SELECTION_SCALELOCKX, (g_qeglobals.d_savedinfo.bScaleLockX ? (LPARAM)TRUE : (LPARAM)FALSE));
	// Scale Lock Y
	CheckMenuItem(hMenu, ID_SELECTION_SCALELOCKY, (g_qeglobals.d_savedinfo.bScaleLockY ? MF_CHECKED : MF_UNCHECKED));
	SendMessage(g_qeglobals.d_hwndToolbar[5], TB_CHECKBUTTON, (WPARAM)ID_SELECTION_SCALELOCKY, (g_qeglobals.d_savedinfo.bScaleLockY ? (LPARAM)TRUE : (LPARAM)FALSE));
	// Scale Lock Z
	CheckMenuItem(hMenu, ID_SELECTION_SCALELOCKZ, (g_qeglobals.d_savedinfo.bScaleLockZ ? MF_CHECKED : MF_UNCHECKED));
	SendMessage(g_qeglobals.d_hwndToolbar[5], TB_CHECKBUTTON, (WPARAM)ID_SELECTION_SCALELOCKZ, (g_qeglobals.d_savedinfo.bScaleLockZ ? (LPARAM)TRUE : (LPARAM)FALSE));

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