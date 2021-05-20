//==============================
//	qe3.c
//==============================

#include "qe3.h"

#define	SPEED_MOVE	32.0f
#define	SPEED_TURN	22.5f

qeglobals_t	g_qeglobals;
qeConfig	g_qeconfig;

void QE_FixMonFlags()
{
	for (Entity *e = g_map.entities.next; e != &g_map.entities; e = e->next)
	{
		int sf, sf_lo, sf_mid, sf_hi, sf_new;
		if (!(e->eclass->showFlags & EFL_MONSTER))
			continue;

		sf = e->GetKeyValueInt("spawnflags");

		if (!strcmp(e->eclass->name, "monster_zombie"))	// fucking crucified flag
		{
			sf_lo = sf & 60;	// the four flags we're shifting up
			sf_mid = sf & 192;  // two flags we shift down
			sf_hi = sf - sf_lo - sf_mid;	// the flags we're leaving alone

			sf_lo *= 4;
			sf_mid /= 16;
		}
		else
		{
			sf_lo = sf & 30;	// the four flags we're shifting up
			sf_mid = sf & 224;  // three flags we shift down
			sf_hi = sf - sf_lo - sf_mid;	// the flags we're leaving alone

			sf_lo *= 8;
			sf_mid /= 16;
		}
		sf_new = sf_lo + sf_mid + sf_hi;
		// invert tfog flag, which is now 32
		if (sf_new & 16)
			sf_new ^= 32;

		if (sf_new != sf)
		{
			e->SetKeyValue("spawnflags", sf_new);
			Sys_Printf("%s, %i -> %i\n", e->eclass->name, sf, sf_new);
		}
	}
}

// it can test aaanything you want just press PEE
#pragma optimize("", off)
void QE_TestSomething()
{
	QE_FixMonFlags();
	Sys_UpdateWindows(W_ALL);
}

#pragma optimize("", on)



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
QE_Init
==================
*/
void QE_Init ()
{
	Sys_Printf("Initializing QuakeEd\n");

	// check if registry key exists and do default windows if not
	HKEY hKey;
	if (RegOpenKeyEx(HKEY_CURRENT_USER, QE3_WIN_REGISTRY, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
	{
		PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_WINDOW_QE3DEFAULT, 0);
		RegCloseKey(hKey);
	}

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
	g_qeglobals.d_fTexFitW = 1.0f;
	g_qeglobals.d_fTexFitH = 1.0f;

	g_cfgUI.ViewFilter |= BFL_HIDDEN;	// hidden things are always filtered

	g_cmdQueue.SetSize(g_cfgEditor.UndoLevels);

	// create tools - creation order determines which tools get first chance to handle inputs
	Sys_Printf("Creating base tools\n");
	new NavTool();
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

	// check command line for a map to load at startup
	char szMap[_MAX_PATH];
	for (int i = 0; i < g_nArgC; i++)
	{
		int l = strlen(g_pszArgV[i]);
		if (l <= 4)
			continue;
		if (!strcmp(g_pszArgV[i] + (l - 4), ".map"))
		{
			if (IsPathAbsolute(g_pszArgV[i]))
				strcpy(szMap, g_pszArgV[i]);
			else	// path is relative, look in project maps dir
				sprintf(szMap, "%s%s", g_project.mapPath, g_pszArgV[i]);	// config guarantees map path ends with /
			g_map.LoadFromFile(szMap);
			return;
		}
	}
	if (g_cfgEditor.LoadLastMap && GetMenuItem(g_qeglobals.d_lpMruMenu, 0, false, szMap, _MAX_PATH))
		g_map.LoadFromFile(szMap);
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
		PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_TOOLS_DRAWBRUSHESTOOL, 0);
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
		/*
	case ' ':
		if (g_cfgEditor.CloneStyle == CLONE_DRAG)
			Sys_UpdateWindows(W_SCENE);
		else
			PostMessage(g_qeglobals.d_hwndMain, WM_COMMAND, ID_SELECTION_CLONE, 0);
		break;
		*/
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



bool QE_KeyUp(int key)
{
	//bool shift, ctrl;
	//shift = (GetKeyState(VK_SHIFT) < 0);
	//ctrl = (GetKeyState(VK_CONTROL) < 0);
	/*
	switch (key)
	{
	case ' ':
		if (g_cfgEditor.CloneStyle == CLONE_DRAG)
			Sys_UpdateWindows(W_SCENE);
		break;
	default:
		return false;
	}*/
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
		switch (i)
		{
		case GL_INVALID_ENUM:
			Sys_Printf("OpenGL Error: GL_INVALID_ENUM");
			break;
		case GL_INVALID_VALUE:
			Sys_Printf("OpenGL Error: GL_INVALID_VALUE");
			break;
		case GL_INVALID_OPERATION:
			Sys_Printf("OpenGL Error: GL_INVALID_OPERATION");
			break;
		case GL_STACK_OVERFLOW:
			Sys_Printf("OpenGL Error: GL_STACK_OVERFLOW");
			break;
		case GL_STACK_UNDERFLOW:
			Sys_Printf("OpenGL Error: GL_STACK_UNDERFLOW");
			break;
		case GL_OUT_OF_MEMORY:
			Sys_Printf("OpenGL Error: GL_OUT_OF_MEMORY");
			break;
		case GL_INVALID_FRAMEBUFFER_OPERATION:
			Sys_Printf("OpenGL Error: GL_INVALID_FRAMEBUFFER_OPERATION");
			break;
		case GL_CONTEXT_LOST:
			Sys_Printf("OpenGL Error: GL_CONTEXT_LOST");
			break;
		case GL_TABLE_TOO_LARGE:
			Sys_Printf("OpenGL Error: GL_TABLE_TOO_LARGE");
			break;
		}
    }
}

/*
==================
QE_OpenGLError

unused; openGL error callback is 4.3+ and we're targeting 2.0 to be fair to quake people
==================
*/
void QE_OpenGLError(int errornum, const char *errorstr)
{
	Sys_Printf("*** OpenGL Error %i ***\n%s", errornum, errorstr);
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
QE_UpdateTitle
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
QE_InitProject
===========
*/
bool QE_InitProject()
{
	Sys_Printf("Initializing project '%s'\n", g_project.name);

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
		Warning("Must have a single brush selected.");
		return false;
	}
	if (g_brSelectedBrushes.next->owner->IsPoint())
	{
		Warning("Cannot manipulate fixed size entities.");
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


void QE_CheckMenuItem(HMENU hMenu, unsigned item, bool check)
{
	CheckMenuItem(hMenu, item, (check ? MF_CHECKED : MF_UNCHECKED));
}

/*
==================
QE_UpdateCommandUI
==================
*/
#pragma warning(disable : 4800)     // shutup int to bool conversion warning
void QE_UpdateCommandUIFilters(HMENU hMenu)
{
	QE_CheckMenuItem(hMenu, ID_VIEW_SHOWAXIS, g_cfgUI.ShowAxis);
	QE_CheckMenuItem(hMenu, ID_VIEW_SHOWBLOCKS, g_cfgUI.ShowBlocks);
	QE_CheckMenuItem(hMenu, ID_VIEW_SHOWCAMERAGRID, g_cfgUI.ShowCameraGrid);
	QE_CheckMenuItem(hMenu, ID_VIEW_SHOWCOORDINATES, g_cfgUI.ShowCoordinates);
	QE_CheckMenuItem(hMenu, ID_VIEW_SHOWLIGHTRADIUS, g_cfgUI.ShowLightRadius);
	QE_CheckMenuItem(hMenu, ID_VIEW_SHOWMAPBOUNDARY, g_cfgUI.ShowMapBoundary);
	QE_CheckMenuItem(hMenu, ID_VIEW_SHOWNAMES, g_cfgUI.ShowNames);
	QE_CheckMenuItem(hMenu, ID_VIEW_SHOWSIZEINFO, g_cfgUI.ShowSizeInfo);
	QE_CheckMenuItem(hMenu, ID_VIEW_SHOWWORKZONE, g_cfgUI.ShowWorkzone);
	QE_CheckMenuItem(hMenu, ID_VIEW_SHOWANGLES, g_cfgUI.ShowAngles);

	QE_CheckMenuItem(hMenu, ID_TARGETLINES_ALL, g_cfgUI.PathlineMode == TargetGraph::tgm_all);
	QE_CheckMenuItem(hMenu, ID_TARGETLINES_SEL, g_cfgUI.PathlineMode == TargetGraph::tgm_selected);
	QE_CheckMenuItem(hMenu, ID_TARGETLINES_SELPATH, g_cfgUI.PathlineMode == TargetGraph::tgm_selected_path);
	QE_CheckMenuItem(hMenu, ID_TARGETLINES_NONE, g_cfgUI.PathlineMode == TargetGraph::tgm_none);

	QE_CheckMenuItem(hMenu, ID_VIEW_SHOWCLIP, !(g_cfgUI.ViewFilter & BFL_CLIP));
	QE_CheckMenuItem(hMenu, ID_VIEW_SHOWBRUSHENTS, !(g_cfgUI.ViewFilter & EFL_BRUSHENTITY));
	QE_CheckMenuItem(hMenu, ID_VIEW_SHOWPOINTENTS, !(g_cfgUI.ViewFilter & EFL_POINTENTITY));
	QE_CheckMenuItem(hMenu, ID_VIEW_SHOWFUNCWALL, !(g_cfgUI.ViewFilter & EFL_FUNCWALL));
	QE_CheckMenuItem(hMenu, ID_VIEW_SHOWLIGHTS, !(g_cfgUI.ViewFilter & EFL_LIGHT));
	QE_CheckMenuItem(hMenu, ID_VIEW_SHOWSKY, !(g_cfgUI.ViewFilter & BFL_SKY));
	QE_CheckMenuItem(hMenu, ID_VIEW_SHOWWATER, !(g_cfgUI.ViewFilter & BFL_LIQUID));
	QE_CheckMenuItem(hMenu, ID_VIEW_SHOWWORLD, !(g_cfgUI.ViewFilter & EFL_WORLDSPAWN));
	QE_CheckMenuItem(hMenu, ID_VIEW_SHOWHINT, !(g_cfgUI.ViewFilter & BFL_HINT));
	QE_CheckMenuItem(hMenu, ID_VIEW_SHOWDETAIL, !(g_cfgUI.ViewFilter & EFL_DETAIL));
	QE_CheckMenuItem(hMenu, ID_VIEW_SHOWMONSTERS, !(g_cfgUI.ViewFilter & EFL_MONSTER));
	QE_CheckMenuItem(hMenu, ID_VIEW_SHOWTRIGGERS, !(g_cfgUI.ViewFilter & EFL_TRIGGER));

	CheckMenuItem(hMenu, ID_FILTER_SHOWEASYSKILL, MF_UNCHECKED);
	CheckMenuItem(hMenu, ID_FILTER_SHOWMEDIUMSKILL, MF_UNCHECKED);
	CheckMenuItem(hMenu, ID_FILTER_SHOWHARDSKILL, MF_UNCHECKED);
	CheckMenuItem(hMenu, ID_FILTER_SHOWDEATHMATCH, MF_UNCHECKED);
	CheckMenuItem(hMenu, ID_FILTER_SHOWALLSKILLS, MF_UNCHECKED);
	if (g_cfgUI.ViewFilter & EFL_EASY)
		CheckMenuItem(hMenu, ID_FILTER_SHOWEASYSKILL, MF_CHECKED);
	else if (g_cfgUI.ViewFilter & EFL_MEDIUM)
		CheckMenuItem(hMenu, ID_FILTER_SHOWMEDIUMSKILL, MF_CHECKED);
	else if (g_cfgUI.ViewFilter & EFL_HARD)
		CheckMenuItem(hMenu, ID_FILTER_SHOWHARDSKILL, MF_CHECKED);
	else if (g_cfgUI.ViewFilter & EFL_DEATHMATCH)
		CheckMenuItem(hMenu, ID_FILTER_SHOWDEATHMATCH, MF_CHECKED);
	else
		CheckMenuItem(hMenu, ID_FILTER_SHOWALLSKILLS, MF_CHECKED);
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
	QE_CheckMenuItem(hMenu, ID_VIEW_TOOLBAR_FILEBAND,		IsWindowVisible(g_qeglobals.d_hwndToolbar[0]));
	QE_CheckMenuItem(hMenu, ID_VIEW_TOOLBAR_EDITBAND,		IsWindowVisible(g_qeglobals.d_hwndToolbar[1]));
	QE_CheckMenuItem(hMenu, ID_VIEW_TOOLBAR_EDIT2BAND,		IsWindowVisible(g_qeglobals.d_hwndToolbar[2]));
	QE_CheckMenuItem(hMenu, ID_VIEW_TOOLBAR_SELECTBAND,		IsWindowVisible(g_qeglobals.d_hwndToolbar[3]));
	QE_CheckMenuItem(hMenu, ID_VIEW_TOOLBAR_CSGBAND,		IsWindowVisible(g_qeglobals.d_hwndToolbar[4]));
	QE_CheckMenuItem(hMenu, ID_VIEW_TOOLBAR_MODEBAND,		IsWindowVisible(g_qeglobals.d_hwndToolbar[5]));
	QE_CheckMenuItem(hMenu, ID_VIEW_TOOLBAR_ENTITYBAND,		IsWindowVisible(g_qeglobals.d_hwndToolbar[6]));
	QE_CheckMenuItem(hMenu, ID_VIEW_TOOLBAR_BRUSHBAND,		IsWindowVisible(g_qeglobals.d_hwndToolbar[7]));
	QE_CheckMenuItem(hMenu, ID_VIEW_TOOLBAR_TEXTUREBAND,	IsWindowVisible(g_qeglobals.d_hwndToolbar[8]));
	QE_CheckMenuItem(hMenu, ID_VIEW_TOOLBAR_VIEWBAND,		IsWindowVisible(g_qeglobals.d_hwndToolbar[9]));
	QE_CheckMenuItem(hMenu, ID_VIEW_TOOLBAR_MISCBAND,		IsWindowVisible(g_qeglobals.d_hwndToolbar[10]));

	// Status Bar
	QE_CheckMenuItem(hMenu, ID_VIEW_STATUSBAR,	IsWindowVisible(g_qeglobals.d_hwndStatus));
	// XY Windows
	QE_CheckMenuItem(hMenu, ID_VIEW_CAMERA,		IsWindowVisible(g_qeglobals.d_hwndCamera));
	QE_CheckMenuItem(hMenu, ID_VIEW_TOGGLE_XY,	IsWindowVisible(g_qeglobals.d_hwndXYZ[0]));
	QE_CheckMenuItem(hMenu, ID_VIEW_TOGGLE_XZ,	IsWindowVisible(g_qeglobals.d_hwndXYZ[2]));
	QE_CheckMenuItem(hMenu, ID_VIEW_TOGGLE_YZ,	IsWindowVisible(g_qeglobals.d_hwndXYZ[1]));
	QE_CheckMenuItem(hMenu, ID_VIEW_TOGGLE_Z,	IsWindowVisible(g_qeglobals.d_hwndZ));

	// Cubic Clipping
	QE_CheckMenuItem(hMenu, ID_VIEW_CUBICCLIP, g_cfgEditor.CubicClip);
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
		QE_CheckMenuItem(hMenu, ID_SELECTION_CLIPPER, modeCheck);
		SendMessage(g_qeglobals.d_hwndToolbar[5], TB_CHECKBUTTON, (WPARAM)ID_SELECTION_CLIPPER, (modeCheck ? (LPARAM)TRUE : (LPARAM)FALSE));

		// Drag Edge Mode 
		GeoTool* gt;
		gt = dynamic_cast<GeoTool*>(Tool::ModalTool());
		modeCheck = (gt && (gt->mode & GeoTool::GT_EDGE));
		QE_CheckMenuItem(hMenu, ID_SELECTION_DRAGEDGES, modeCheck);
		SendMessage(g_qeglobals.d_hwndToolbar[5], TB_CHECKBUTTON, (WPARAM)ID_SELECTION_DRAGEDGES, (modeCheck ? (LPARAM)TRUE : (LPARAM)FALSE));

		// Drag Vertex Mode 
		modeCheck = (gt && (gt->mode & GeoTool::GT_VERTEX));
		QE_CheckMenuItem(hMenu, ID_SELECTION_DRAGVERTICES, modeCheck);
		SendMessage(g_qeglobals.d_hwndToolbar[5], TB_CHECKBUTTON, (WPARAM)ID_SELECTION_DRAGVERTICES, (modeCheck ? (LPARAM)TRUE : (LPARAM)FALSE));

		// Drag Face Mode 
		modeCheck = (gt && (gt->mode & GeoTool::GT_FACE));
		QE_CheckMenuItem(hMenu, ID_SELECTION_DRAGFACES, modeCheck);
		SendMessage(g_qeglobals.d_hwndToolbar[5], TB_CHECKBUTTON, (WPARAM)ID_SELECTION_DRAGFACES, (modeCheck ? (LPARAM)TRUE : (LPARAM)FALSE));

		modeCheck = (dynamic_cast<PolyTool*>(Tool::ModalTool()) != nullptr);
		QE_CheckMenuItem(hMenu, ID_TOOLS_DRAWBRUSHESTOOL, modeCheck);
		SendMessage(g_qeglobals.d_hwndToolbar[5], TB_CHECKBUTTON, (WPARAM)ID_TOOLS_DRAWBRUSHESTOOL, (modeCheck ? (LPARAM)TRUE : (LPARAM)FALSE));
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
	QE_CheckMenuItem(hMenu, ID_GRID_TOGGLE, g_qeglobals.d_bShowGrid);
	QE_CheckMenuItem(hMenu, ID_GRID_SNAPTOGRID, g_qeglobals.bGridSnap);

//===================================
// Texture Menu
//===================================
	QE_CheckMenuItem(hMenu, ID_TEXTURES_LOCK, g_qeglobals.d_bTextureLock);

//===================================
// Region Menu
//===================================
	EnableMenuItem(hMenu, ID_REGION_SETXZ, (g_qeglobals.d_wndGrid[2]->Open() ? MF_ENABLED : MF_GRAYED));
	EnableMenuItem(hMenu, ID_REGION_SETYZ, (g_qeglobals.d_wndGrid[1]->Open() ? MF_ENABLED : MF_GRAYED));
}