//==============================
//	qe3.c
//==============================

#include "qe3.h"

#define	SPEED_MOVE	32.0f
#define	SPEED_TURN	22.5f

qeglobals_t	g_qeglobals;
qeConfig	g_qeconfig;

// it can test aaanything you want
void QE_TestSomething()
{

}



/*
==================
pointOnGrid
==================
*/
vec3 pointOnGrid(const vec3 point)
{
	vec3 out;
	out = glm::round(point / (float)g_qeglobals.d_nGridSize) * (float)g_qeglobals.d_nGridSize;

	return out;
}


/*
==============
AxializeVector
lunaran: matches function of TextureAxisFromPlane now, used to fail on diagonal cases
==============
*/
vec3 AxializeVector(const vec3 &v)
{
	vec3	out;

	out = AxisForVector(v);

	for (int i = 0; i < 3; i++)
	{
		out[i] = fabs(v[i]) * out[i];
	}
	return out;
}

/*
==============
AxisForVector
==============
*/
vec3 AxisForVector(const vec3 &v)
{
	int		i, bestaxis;
	float	dot, best;

	best = 0;
	bestaxis = 0;

	for (i = 0; i < 6; i++)
	{
		dot = DotProduct(v, g_v3BaseAxis[i * 3]);
		if (dot > best)
		{
			best = dot;
			bestaxis = i;
		}
	}

	return g_v3BaseAxis[bestaxis * 3];
}

/*
==================
QE_InitProject
==================
*/
void QE_InitProject()
{
	char	szProject[_MAX_PATH];	// sikk - Load Last Project
	szProject[0] = 0;
	Sys_Printf("Initializing project settings ...\n");
	/*
	if (g_pszArgV[1])
	{
		// the project file can be specified on the command line (must be first parameter)
		strcpy(szProject, g_pszArgV[1]);
		Sys_Printf("Loading project from command line: %s\n", szProject);
	}
	else if (g_qeglobals.d_savedinfo.bLoadLastProject)
	{
		// if 'Load Last Project' is checked, load that file
		if (strlen(g_qeglobals.d_savedinfo.szLastProject) > 0)
		{
			strcpy(szProject, g_qeglobals.d_savedinfo.szLastProject);
			Sys_Printf("Loading last project: %s\n", szProject);
		}
		else
		{
			Sys_Printf("'Load Last Project' specified, but no last project found!\n");
			strcpy(szProject, "scripts/quake.qe3");
			strcpy(g_qeglobals.d_savedinfo.szLastProject, szProject);
			Sys_Printf("Loading default project: %s\n", szProject);
		}
	}
	else
	{
		// default
		strcpy(szProject, "scripts/quake.qe3");
		strcpy(g_qeglobals.d_savedinfo.szLastProject, szProject);
		Sys_Printf("Loading default project: %s\n", szProject);
	}
	*/
	if (!QE_LoadProject())
	{
		//DoProject(true);	// sikk - Manually create project file if none is found
		ProjectDialog();	// lunaran - i know it's down there somewhere just let me have a look
		//if (!QE_LoadProject("scripts/quake.qe3"))
		//	Error("Could not load scripts/quake.qe3 project file.");
	}
}

/*
==================
QE_Init
==================
*/
void QE_Init ()
{
	Sys_Printf("Initializing QuakeEd\n");

	g_qeconfig.Load();

	QE_InitProject();

	// initialize variables
	g_qeglobals.d_nGridSize = 8;
	g_qeglobals.d_bShowGrid = true;
	g_qeglobals.bGridSnap = true;
	g_qeglobals.d_vXYZ[0].SetAxis(XY);
	g_qeglobals.d_fDefaultTexScale = 1.00f;	// sikk - Default Texture Size Dialog
	g_qeglobals.d_v3WorkMin = vec3(0);
	g_qeglobals.d_v3WorkMax = vec3(8);
	//g_qeglobals.d_savedinfo.nViewFilter |= BFL_HIDDEN;	// hidden things are always filtered
	g_cfgUI.ViewFilter |= BFL_HIDDEN;	// hidden things are always filtered

	// create tools - creation order determines which tools get first chance to handle inputs
	Sys_Printf("Creating base tools\n");
	new SelectTool();
	new TextureTool();
	new ManipTool();

	Sys_UpdateGridStatusBar();

	// sikk - For bad texture name check during map save. Setting it to
	// -1 insures that if brush #0 has bad face, it'll be listed in console
	//g_nBrushNumCheck = -1;

// sikk---> Save Rebar Band Info
	for (int i = 0; i < 11; i++)
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

any key combos which are ctrl+ or alt+ are in the accelerator table
any key combos which are shift+ can't be and have to go here, or else capital letters typed
	into non-modal dialogs are intercepted early ...
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
	case 'F':
		PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_SELECTION_DRAGFACES, 0);
		break;
	case 'V':
		PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_SELECTION_DRAGVERTICES, 0);
		break;
		/*
	case 'G':	// sikk - added shortcut key
		// lunaran - removed it again because turning off snap to grid should require two submarine commanders to turn their keys simultaneously
		PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_GRID_SNAPTOGRID, 0);
		break;*/
	case 'H':
		if (shift)
			PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_VIEW_HIDESHOW_HIDEUNSELECTED, 0);
		else
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
#ifdef _DEBUG
	case 'P':
		QE_TestSomething();
		break;
#endif
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
	//case VK_F5: // sikk - Project Dialog
	//	PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_FILE_EDITPROJECT, 0);
	//	break;
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
==================
QE_SaveMap
==================
*/
void QE_SaveMap()
{
	g_map.SaveToFile(g_map.name, false);	// ignore region
	g_cmdQueue.SetSaved();
	g_map.autosaveTime = clock();

	Sys_UpdateWindows(W_TITLE);
}

/*
==================
QE_CheckOpenGLForErrors
==================
*/
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

If x minutes have passed since making a change and the map hasn't been saved, save it
===============
*/
void QE_CheckAutoSave ()
{
	//static clock_t s_start;
	clock_t        now;

	if (!g_cfgEditor.Autosave)	// sikk - Preferences Dialog
		return;
	if (!g_cmdQueue.IsModified() || g_map.autosaveTime == -1)
		return;

	now = clock();

	if (!g_map.autosaveTime)
	{
		g_map.autosaveTime = now;
		return;
	}

	if (now - g_map.autosaveTime > (CLOCKS_PER_SEC * 60 * g_cfgEditor.AutosaveTime))	// sikk - Preferences Dialog
	{
		Sys_Printf("Autosaving...\n");
		Sys_Status("Autosaving...", 0);

		g_map.SaveToFile(g_project.autosaveFile, false);

		Sys_Printf("Autosave successful.\n");
		Sys_Status("Autosave successful.", 0);

		g_map.autosaveTime = -1;
	}
}

/*
===========
QE_LoadProject
===========
*/
bool QE_LoadProject()
{
	Sys_Printf("QE_LoadProject (%s)\n", g_project.name);

	Sys_Printf("basePath: %s\n", g_project.basePath);
	Sys_Printf("mapPath: %s\n", g_project.mapPath);
	Sys_Printf("autosaveFile: %s\n", g_project.autosaveFile);
	Sys_Printf("entityFiles: %s\n", g_project.entityFiles);
	Sys_Printf("wadPath: %s\n", g_project.wadPath);
	Sys_Printf("defaultWads: %s\n", g_project.defaultWads);
	Sys_Printf("paletteFile: %s\n", g_project.paletteFile);

	EntClass::InitForSourceDirectory(g_project.entityFiles);

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
	sprintf(src, "%s%s", g_project.mapPath, base);

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

	if (!p || !p[0])
		return NULL;
	if (p[0] == '/' || p[0] == '\\')
		return p;

	sprintf(temp, "%s/%s", g_project.basePath, p);
	return temp;
}


// sikk--->	Update Menu Items & Toolbar Buttons
/*
==================
QE_UpdateCommandUI
==================
*/

void QE_UpdateCommandUIFilters(HMENU hMenu)
{
	CheckMenuItem(hMenu, ID_VIEW_SHOWAXIS, (g_cfgUI.ShowAxis ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(hMenu, ID_VIEW_SHOWBLOCKS, (g_cfgUI.ShowBlocks ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(hMenu, ID_VIEW_SHOWCAMERAGRID, (g_cfgUI.ShowCameraGrid ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(hMenu, ID_VIEW_SHOWCOORDINATES, (g_cfgUI.ShowCoordinates ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(hMenu, ID_VIEW_SHOWLIGHTRADIUS, (g_cfgUI.ShowLightRadius ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(hMenu, ID_VIEW_SHOWMAPBOUNDARY, (g_cfgUI.ShowMapBoundary ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(hMenu, ID_VIEW_SHOWNAMES, (g_cfgUI.ShowNames ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(hMenu, ID_VIEW_SHOWSIZEINFO, (g_cfgUI.ShowSizeInfo ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(hMenu, ID_VIEW_SHOWWORKZONE, (g_cfgUI.ShowWorkzone ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(hMenu, ID_VIEW_SHOWANGLES, (g_cfgUI.ShowAngles ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(hMenu, ID_VIEW_SHOWPATH, (g_cfgUI.ShowPaths ? MF_CHECKED : MF_UNCHECKED));

	CheckMenuItem(hMenu, ID_VIEW_SHOWCLIP, ((g_cfgUI.ViewFilter & BFL_CLIP) ? MF_UNCHECKED : MF_CHECKED));
	CheckMenuItem(hMenu, ID_VIEW_SHOWBRUSHENTS, ((g_cfgUI.ViewFilter & EFL_BRUSHENTITY) ? MF_UNCHECKED : MF_CHECKED));
	CheckMenuItem(hMenu, ID_VIEW_SHOWPOINTENTS, ((g_cfgUI.ViewFilter & EFL_POINTENTITY) ? MF_UNCHECKED : MF_CHECKED));
	CheckMenuItem(hMenu, ID_VIEW_SHOWFUNCWALL, ((g_cfgUI.ViewFilter & EFL_FUNCWALL) ? MF_UNCHECKED : MF_CHECKED));
	CheckMenuItem(hMenu, ID_VIEW_SHOWLIGHTS, ((g_cfgUI.ViewFilter & EFL_LIGHT) ? MF_UNCHECKED : MF_CHECKED));
	CheckMenuItem(hMenu, ID_VIEW_SHOWSKY, ((g_cfgUI.ViewFilter & BFL_SKY) ? MF_UNCHECKED : MF_CHECKED));
	CheckMenuItem(hMenu, ID_VIEW_SHOWWATER, ((g_cfgUI.ViewFilter & BFL_LIQUID) ? MF_UNCHECKED : MF_CHECKED));
	CheckMenuItem(hMenu, ID_VIEW_SHOWWORLD, ((g_cfgUI.ViewFilter & EFL_WORLDSPAWN) ? MF_UNCHECKED : MF_CHECKED));
	CheckMenuItem(hMenu, ID_VIEW_SHOWHINT, ((g_cfgUI.ViewFilter & BFL_HINT) ? MF_UNCHECKED : MF_CHECKED));
	CheckMenuItem(hMenu, ID_VIEW_SHOWDETAIL, ((g_cfgUI.ViewFilter & EFL_DETAIL) ? MF_UNCHECKED : MF_CHECKED));
	CheckMenuItem(hMenu, ID_VIEW_SHOWMONSTERS, ((g_cfgUI.ViewFilter & EFL_MONSTER) ? MF_UNCHECKED : MF_CHECKED));
	CheckMenuItem(hMenu, ID_VIEW_SHOWTRIGGERS, ((g_cfgUI.ViewFilter & EFL_TRIGGER) ? MF_UNCHECKED : MF_CHECKED));
}

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
	CheckMenuItem(hMenu, ID_VIEW_CUBICCLIP, (g_cfgEditor.CubicClip ? MF_CHECKED : MF_UNCHECKED));
	SendMessage(g_qeglobals.d_hwndToolbar[6], TB_CHECKBUTTON, (WPARAM)ID_VIEW_CUBICCLIP, (g_cfgEditor.CubicClip ? (LPARAM)TRUE : (LPARAM)FALSE));
	// Filter Commands

	QE_UpdateCommandUIFilters(hMenu);

//===================================
// Selection Menu
//===================================
	{
		bool modeCheck;

		// Clipper Mode
		modeCheck = (dynamic_cast<ClipTool*>(Tool::ModalTool()) != nullptr);
		CheckMenuItem(hMenu, ID_SELECTION_CLIPPER, (modeCheck ? MF_CHECKED : MF_UNCHECKED));
		SendMessage(g_qeglobals.d_hwndToolbar[5], TB_CHECKBUTTON, (WPARAM)ID_SELECTION_CLIPPER, (modeCheck ? (LPARAM)TRUE : (LPARAM)FALSE));

		// Drag Edge Mode 
		GeoTool* gt;
		gt = dynamic_cast<GeoTool*>(Tool::ModalTool());
		modeCheck = (gt && (gt->mode & GeoTool::GT_EDGE));
		CheckMenuItem(hMenu, ID_SELECTION_DRAGEDGES, modeCheck ? MF_CHECKED : MF_UNCHECKED);
		SendMessage(g_qeglobals.d_hwndToolbar[5], TB_CHECKBUTTON, (WPARAM)ID_SELECTION_DRAGEDGES, (modeCheck ? (LPARAM)TRUE : (LPARAM)FALSE));

		// Drag Vertex Mode 
		modeCheck = (gt && (gt->mode & GeoTool::GT_VERTEX));
		CheckMenuItem(hMenu, ID_SELECTION_DRAGVERTICES, modeCheck ? MF_CHECKED : MF_UNCHECKED);
		SendMessage(g_qeglobals.d_hwndToolbar[5], TB_CHECKBUTTON, (WPARAM)ID_SELECTION_DRAGVERTICES, (modeCheck ? (LPARAM)TRUE : (LPARAM)FALSE));

		// TODO: add a button for drag face mode also
	}

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
	CheckMenuItem(hMenu, ID_GRID_SNAPTOGRID, (g_qeglobals.bGridSnap ? MF_CHECKED : MF_UNCHECKED));

//===================================
// Texture Menu
//===================================
	CheckMenuItem(hMenu, ID_TEXTURES_LOCK, (g_qeglobals.d_bTextureLock ? MF_CHECKED : MF_UNCHECKED));

//===================================
// Region Menu
//===================================
	EnableMenuItem(hMenu, ID_REGION_SETXZ, (g_qeglobals.d_wndGrid[2]->Open() ? MF_ENABLED : MF_GRAYED));
	EnableMenuItem(hMenu, ID_REGION_SETYZ, (g_qeglobals.d_wndGrid[1]->Open() ? MF_ENABLED : MF_GRAYED));
}