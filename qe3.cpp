//==============================
//	qe3.c
//==============================

#include "pre.h"
#include "qe3.h"
#include "map.h"
#include "select.h"
#include "points.h"

#include "Command.h"
#include "NavTool.h"
#include "SelectTool.h"
#include "TextureTool.h"
#include "ManipTool.h"
#include "EntClassInitializer.h"

#include "CameraView.h"
#include "GridView.h"
#include "WndMain.h"
#include "WndEntity.h"
#include "WndGrid.h"
#include "WndFiles.h"

#define	SPEED_MOVE	32.0f
#define	SPEED_TURN	22.5f

qeglobals_t	g_qeglobals;

void QE_FixMonFlags()
{
	/*
	for (Entity *e = g_map.entities.Next(); e != &g_map.entities; e = e->Next())
	{
		int sf, sf_lo, sf_mid, sf_hi, sf_new;
		sf = e->GetKeyValueInt("spawnflags");

		if ((e->eclass->showFlags & EFL_MONSTER))
		{

			if (e->eclass->name == "monster_zombie")	// fucking crucified flag
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
		}
		else
		{
			sf_new = sf;
		}
		if (e->GetKeyValueInt("cooponly"))
		{
			sf_new ^= 4096;
			e->DeleteKeyValue("cooponly");
		}
		if (e->GetKeyValueInt("notincoop"))
		{
			sf_new ^= 8192;
			e->DeleteKeyValue("notincoop");
		}

		if (sf_new != sf)
		{
			e->SetKeyValue("spawnflags", sf_new);
			Log::Print(_S("%s, %i -> %i\n")<< e->eclass->name<< sf<< sf_new);
		}
	}
	*/
}

void QE_FindKTs()
{/*
	char *killtarget = nullptr;
	char *targetname = nullptr;
	char *target = nullptr;
	char *target2 = nullptr;
	char *target3 = nullptr;
	char *target4 = nullptr;

	for (Entity *e = g_map.entities.Next(); e != &g_map.entities; e = e->Next())
	{
		killtarget = e->GetKeyValue("killtarget");
		if (!killtarget || killtarget[0] == 0) continue;
		targetname = e->GetKeyValue("targetname");
		target = e->GetKeyValue("target");
		target2 = e->GetKeyValue("target2");
		target3 = e->GetKeyValue("target3");
		target4 = e->GetKeyValue("target4");

		if (!strcmp(killtarget, targetname) ||
			!strcmp(killtarget, target) ||
			!strcmp(killtarget, target2) ||
			!strcmp(killtarget, target3) ||
			!strcmp(killtarget, target4))
			Selection::HandleBrush(e->brushes.Next(), true);
	}*/
}


#pragma optimize("", off)
void QE_TestSomething()
{
#ifdef _DEBUG
	QE_FixMonFlags();
	//QE_RebarSpew();
	//QE_FindKTs();
	//WndMain_UpdateWindows(W_ALL);
#endif
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
	Log::Print("Initializing QuakeEd\n");

	// check if registry key exists and do default windows if not
	HKEY hKey;
	if (RegOpenKeyEx(HKEY_CURRENT_USER, QE3_WIN_REGISTRY, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
	{
		PostMessage(g_hwndMain, WM_COMMAND, ID_WINDOW_QE3DEFAULT, 0);
		RegCloseKey(hKey);
	}

	g_qeconfig.Load();
	Textures::Init();
	QE_InitProject();

	// initialize variables
	g_qeglobals.d_nGridSize = 8;
	g_qeglobals.d_bShowGrid = true;
	g_qeglobals.bGridSnap = true;
	g_vGrid[0].SetAxis(GRID_XY);
	g_qeglobals.d_fDefaultTexScale = 1.00f;	// sikk - Default Texture Size Dialog
	g_qeglobals.d_v3WorkMin = vec3(0);
	g_qeglobals.d_v3WorkMax = vec3(8);

	g_cfgUI.ViewFilter |= BFL_HIDDEN;	// hidden things are always filtered

	g_cmdQueue.SetSize(g_cfgEditor.UndoLevels);

	// create tools - creation order determines which tools get first chance to 
	// handle inputs (created later = top of stack, first to handle)
	Log::Print("Creating base tools\n");
	new NavTool();
	new SelectTool();
	new TextureTool();
	new ManipTool();

	WndMain_UpdateGridStatusBar();

	// other stuff
	Textures::Init();
	g_map.RegionOff();	// sikk - For initiating Map Size change

	// sikk - Update User Interface
	WndMain_UpdateMenu();

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
				sprintf(szMap, "%s%s", g_project.mapPath.data(), g_pszArgV[i]);	// config guarantees map path ends with /
			g_map.Load(szMap);
			return;
		}
	}
	if (g_cfgEditor.LoadLastMap)// && GetMenuItem(g_qeglobals.d_lpMruMenu, 0, false, szMap, _MAX_PATH))
		WndMain_UpdateMRU(1);
		//g_map.LoadFromFile(szMap);
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
				g_texTool->RotateTexture(1);
			else if (shift)
				g_texTool->ShiftTexture(0, g_qeglobals.d_nGridSize);
			else if (ctrl)
				g_texTool->ScaleTexture(0, 0.05f);
			WndMain_UpdateWindows(W_SURF);
		}
		else
			g_vCamera.Step(SPEED_MOVE, 0, 0);

		WndMain_UpdateWindows(W_CAMERA | W_XY);
		break;
	case VK_DOWN:
		if (shift || ctrl)
		{
			if (shift && ctrl)
				g_texTool->RotateTexture(-1);
			else if (shift)
				g_texTool->ShiftTexture(0, -g_qeglobals.d_nGridSize);
			else if (ctrl)
				g_texTool->ScaleTexture(0, -0.05f);
			WndMain_UpdateWindows(W_SURF);
		}
		else
			g_vCamera.Step(-SPEED_MOVE, 0, 0);

		WndMain_UpdateWindows(W_CAMERA | W_XY);
		break;
	case VK_LEFT:
		if (shift || ctrl)
		{
			if (shift && ctrl)
				g_texTool->RotateTexture(15);
			else if (shift)
				g_texTool->ShiftTexture(g_qeglobals.d_nGridSize, 0);
			else if (ctrl)
				g_texTool->ScaleTexture(0.05f, 0);
			WndMain_UpdateWindows(W_SURF);
		}
		else
			g_vCamera.Turn(SPEED_TURN);

		WndMain_UpdateWindows(W_CAMERA | W_XY);
		break;
	case VK_RIGHT:
		if (shift || ctrl)
		{
			if (shift && ctrl)
				g_texTool->RotateTexture(-15);
			else if (shift)
				g_texTool->ShiftTexture(-g_qeglobals.d_nGridSize, 0);
			else if (ctrl)
				g_texTool->ScaleTexture(-0.05f, 0);
			WndMain_UpdateWindows(W_SURF);
		}
		else
			g_vCamera.Turn(-SPEED_TURN);

		WndMain_UpdateWindows(W_CAMERA | W_XY);
		break;

	case 'D':
		g_vCamera.Step(0, 0, SPEED_MOVE);
		WndMain_UpdateWindows(W_CAMERA | W_XY | W_Z);
		break;
	case 'C':
		g_vCamera.Step(0, 0, -SPEED_MOVE);
		WndMain_UpdateWindows(W_CAMERA | W_XY | W_Z);
		break;
	case 'A':
		g_vCamera.Pitch(SPEED_TURN);
		g_vCamera.BoundAngles();
		WndMain_UpdateWindows(W_CAMERA | W_XY);
		break;
	case 'Z':
		g_vCamera.Pitch(-SPEED_TURN);
		g_vCamera.BoundAngles();
		WndMain_UpdateWindows(W_CAMERA | W_XY);
		break;
	case VK_COMMA:
		g_vCamera.Step(0, -SPEED_MOVE, 0);
		WndMain_UpdateWindows(W_CAMERA | W_XY);
		break;
	case VK_PERIOD:
		g_vCamera.Step(0, SPEED_MOVE, 0);
		WndMain_UpdateWindows(W_CAMERA | W_XY);
		break;
	case '0':	// sikk - fixed and added shortcut key
		PostMessage(g_hwndMain, WM_COMMAND, ID_GRID_TOGGLE, 0);
		break;
	case '1':
		PostMessage(g_hwndMain, WM_COMMAND, ID_GRID_1, 0);
		break;
	case '2':
		PostMessage(g_hwndMain, WM_COMMAND, ID_GRID_2, 0);
		break;
	case '3':
		PostMessage(g_hwndMain, WM_COMMAND, ID_GRID_4, 0);
		break;
	case '4':
		PostMessage(g_hwndMain, WM_COMMAND, ID_GRID_8, 0);
		break;
	case '5':
		PostMessage(g_hwndMain, WM_COMMAND, ID_GRID_16, 0);
		break;
	case '6':
		PostMessage(g_hwndMain, WM_COMMAND, ID_GRID_32, 0);
		break;
	case '7':
		PostMessage(g_hwndMain, WM_COMMAND, ID_GRID_64, 0);
		break;
	case '8':
		PostMessage(g_hwndMain, WM_COMMAND, ID_GRID_128, 0);
		break;
	case '9':
		PostMessage(g_hwndMain, WM_COMMAND, ID_GRID_256, 0);
		break;
	case 'B':
		PostMessage(g_hwndMain, WM_COMMAND, ID_TOOLS_DRAWBRUSHESTOOL, 0);
		break;
	case 'E':
		PostMessage(g_hwndMain, WM_COMMAND, ID_SELECTION_DRAGEDGES, 0);
		break;
	case 'F':
		PostMessage(g_hwndMain, WM_COMMAND, ID_SELECTION_DRAGFACES, 0);
		break;
	case 'V':
		PostMessage(g_hwndMain, WM_COMMAND, ID_SELECTION_DRAGVERTICES, 0);
		break;
		/*
	case 'G':	// sikk - added shortcut key
		// lunaran - removed it again because turning off snap to grid should require two submarine commanders to turn their keys simultaneously
		PostMessage(g_hwndMain, WM_COMMAND, ID_GRID_SNAPTOGRID, 0);
		break;*/
	case 'H':
		if (shift)
			PostMessage(g_hwndMain, WM_COMMAND, ID_VIEW_HIDESHOW_HIDEUNSELECTED, 0);
		else
			PostMessage(g_hwndMain, WM_COMMAND, ID_VIEW_HIDESHOW_HIDESELECTED, 0);
		break;
	case 'I':
		PostMessage(g_hwndMain, WM_COMMAND, ID_SELECTION_INVERT, 0);
		break;
	case 'K':
		PostMessage(g_hwndMain, WM_COMMAND, ID_MISC_SELECTENTITYCOLOR, 0);
		break;
	case 'L':	// sikk - added shortcut key
		PostMessage(g_hwndMain, WM_COMMAND, ID_TEXTURES_LOCK, 0);
		break;
	case 'M':
		PostMessage(g_hwndMain, WM_COMMAND, ID_SELECTION_CSGMERGE, 0);
		//PostMessage(g_hwndMain, WM_COMMAND, ID_FILE_IMPORTMAP, 0);
		break;
	case 'N':
		PostMessage(g_hwndMain, WM_COMMAND, ID_VIEW_ENTITY, 0);
		break;
	case 'O':
		PostMessage(g_hwndMain, WM_COMMAND, ID_VIEW_CONSOLE, 0);
		break;
#ifdef _DEBUG
	case 'P':
		QE_TestSomething();
		break;
#endif
	case 'Q':
		PostMessage(g_hwndMain, WM_COMMAND, ID_VIEW_SHOWSIZEINFO, 0);
		break;
	case 'R':	// sikk - added shortcut key
		PostMessage(g_hwndMain, WM_COMMAND, ID_SELECTION_ARBITRARYROTATION, 0);
		break;
	case 'S':
		PostMessage(g_hwndMain, WM_COMMAND, ID_TEXTURES_INSPECTOR, 0);
		break;
	case 'T':
		PostMessage(g_hwndMain, WM_COMMAND, ID_VIEW_TEXTURE, 0);
		break;
	case 'U':
		PostMessage(g_hwndMain, WM_COMMAND, ID_TEXTURES_HIDEUNUSED, 0);
		break;
	case 'X':
		PostMessage(g_hwndMain, WM_COMMAND, ID_SELECTION_CLIPPER, 0);
		break;
		/*
	case ' ':
		if (g_cfgEditor.CloneStyle == CLONE_DRAG)
			WndMain_UpdateWindows(W_SCENE);
		else
			PostMessage(g_hwndMain, WM_COMMAND, ID_SELECTION_CLONE, 0);
		break;
		*/
	case VK_BACK:
		PostMessage(g_hwndMain, WM_COMMAND, ID_SELECTION_DELETE, 0);
		break;
	case VK_ESCAPE:
		PostMessage(g_hwndMain, WM_COMMAND, ID_SELECTION_DESELECT, 0);
		break;
	case VK_RETURN:
		PostMessage(g_hwndMain, WM_COMMAND, ID_SELECTION_CLIPSELECTED, 0);
		break;
	case VK_TAB:
		if (shift)
			PostMessage(g_hwndMain, WM_COMMAND, ID_VIEW_SWAPGRIDCAM, 0);
		else
			PostMessage(g_hwndMain, WM_COMMAND, ID_VIEW_NEXTVIEW, 0);
		break;
	case VK_END:
		PostMessage(g_hwndMain, WM_COMMAND, ID_VIEW_CENTER, 0);
		break;
	case VK_HOME:	// sikk - added shortcut key
		PostMessage(g_hwndMain, WM_COMMAND, ID_VIEW_100, 0);
		break;
	case VK_INSERT:	// lunaran FIXME: ins/del with mouse over a grid view zooms to point
		PostMessage(g_hwndMain, WM_COMMAND, ID_VIEW_ZOOMIN, 0);
		break;
	case VK_DELETE:
		PostMessage(g_hwndMain, WM_COMMAND, ID_VIEW_ZOOMOUT, 0);
		break;
	case VK_NEXT:
		PostMessage(g_hwndMain, WM_COMMAND, ID_VIEW_DOWNFLOOR, 0);
		break;
	case VK_PRIOR:
		PostMessage(g_hwndMain, WM_COMMAND, ID_VIEW_UPFLOOR, 0);
		break;
	case VK_BACKSLASH:	// This may not function on foreign keyboards (non English)
		PostMessage(g_hwndMain, WM_COMMAND, ID_VIEW_CENTERONSELECTION, 0);
		break;
	case VK_MINUS:	// sikk - GridSize Decrease: This may not function on foreign keyboards (non English)
		{
			int i = g_qeglobals.d_nGridSize / 2;
			switch (i)
			{
			case 1:
				PostMessage(g_hwndMain, WM_COMMAND, ID_GRID_1, 0);
				break;
			case 2:
				PostMessage(g_hwndMain, WM_COMMAND, ID_GRID_2, 0);
				break;
			case 4:
				PostMessage(g_hwndMain, WM_COMMAND, ID_GRID_4, 0);
				break;
			case 8:
				PostMessage(g_hwndMain, WM_COMMAND, ID_GRID_8, 0);
				break;
			case 16:
				PostMessage(g_hwndMain, WM_COMMAND, ID_GRID_16, 0);
				break;
			case 32:
				PostMessage(g_hwndMain, WM_COMMAND, ID_GRID_32, 0);
				break;
			case 64:
				PostMessage(g_hwndMain, WM_COMMAND, ID_GRID_64, 0);
				break;
			case 128:
				PostMessage(g_hwndMain, WM_COMMAND, ID_GRID_128, 0);
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
				PostMessage(g_hwndMain, WM_COMMAND, ID_GRID_2, 0);
				break;
			case 4:
				PostMessage(g_hwndMain, WM_COMMAND, ID_GRID_4, 0);
				break;
			case 8:
				PostMessage(g_hwndMain, WM_COMMAND, ID_GRID_8, 0);
				break;
			case 16:
				PostMessage(g_hwndMain, WM_COMMAND, ID_GRID_16, 0);
				break;
			case 32:
				PostMessage(g_hwndMain, WM_COMMAND, ID_GRID_32, 0);
				break;
			case 64:
				PostMessage(g_hwndMain, WM_COMMAND, ID_GRID_64, 0);
				break;
			case 128:
				PostMessage(g_hwndMain, WM_COMMAND, ID_GRID_128, 0);
				break;
			case 256:
				PostMessage(g_hwndMain, WM_COMMAND, ID_GRID_256, 0);
				break;
			default:
				break;
			}
		}
		break;
	case VK_F1:
		PostMessage(g_hwndMain, WM_COMMAND, ID_HELP_HELP, 0);
		break;
	case VK_F2:
		PostMessage(g_hwndMain, WM_COMMAND, ID_EDIT_MAPINFO, 0);
		break;
	case VK_F3:
		PostMessage(g_hwndMain, WM_COMMAND, ID_EDIT_ENTITYINFO, 0);
		break;
	case VK_F4:
		PostMessage(g_hwndMain, WM_COMMAND, ID_EDIT_PREFERENCES, 0);
		break;
	case VK_F12:
		PostMessage(g_hwndMain, WM_COMMAND, ID_MISC_TESTMAP, 0);
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
			WndMain_UpdateWindows(W_SCENE);
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
			Log::Print("OpenGL Error: GL_INVALID_ENUM");
			break;
		case GL_INVALID_VALUE:
			Log::Print("OpenGL Error: GL_INVALID_VALUE");
			break;
		case GL_INVALID_OPERATION:
			Log::Print("OpenGL Error: GL_INVALID_OPERATION");
			break;
		case GL_STACK_OVERFLOW:
			Log::Print("OpenGL Error: GL_STACK_OVERFLOW");
			break;
		case GL_STACK_UNDERFLOW:
			Log::Print("OpenGL Error: GL_STACK_UNDERFLOW");
			break;
		case GL_OUT_OF_MEMORY:
			Log::Print("OpenGL Error: GL_OUT_OF_MEMORY");
			break;
		case GL_INVALID_FRAMEBUFFER_OPERATION:
			Log::Print("OpenGL Error: GL_INVALID_FRAMEBUFFER_OPERATION");
			break;
		case GL_CONTEXT_LOST:
			Log::Print("OpenGL Error: GL_CONTEXT_LOST");
			break;
		case GL_TABLE_TOO_LARGE:
			Log::Print("OpenGL Error: GL_TABLE_TOO_LARGE");
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
	Log::Print(_S("*** OpenGL Error %i ***\n%s")<< errornum<< errorstr);
}

/*
==================
QE_SaveMap
==================
*/
void QE_SaveMap()
{
	if (!g_map.hasFilename)
		if (!SaveAsDialog())
			return;
	
	g_map.Save();	// ignore region
	g_cmdQueue.SetSaved();
	g_map.autosaveTime = clock();

	Pointfile_Clear();
	WndMain_UpdateWindows(W_TITLE);
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
		Log::Print("Autosaving...\n");
		WndMain_Status("Autosaving...", 0);

		g_map.Save(g_project.autosaveFile);

		Log::Print("Autosave successful.\n");
		WndMain_Status("Autosave successful.", 0);

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
	Log::Print(_S("Initializing project '%s'\n") << g_project.name);

	Log::Print(_S("basePath: %s\n") << g_project.basePath);
	Log::Print(_S("mapPath: %s\n") << g_project.mapPath);
	Log::Print(_S("autosaveFile: %s\n") << g_project.autosaveFile);
	Log::Print(_S("entityFiles: %s\n") << g_project.entityFiles);
	Log::Print(_S("wadPath: %s\n") << g_project.wadPath);
	Log::Print(_S("defaultWads: %s\n") << g_project.defaultWads);
	Log::Print(_S("paletteFile: %s\n") << g_project.paletteFile);

	EntClassInitializer eci;
	eci.InitForProject();

	g_wndEntity->FillClassList();
	//FillBSPMenu();

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
		if (g_wndGrid[i]->IsOnTop())
			return g_wndGrid[i]->gv->GetAxis();
	}
	return GRID_XY;
}

/*
==================
QE_SingleBrush
==================
*/
bool QE_SingleBrush ()
{
	if ((!Selection::HasBrushes()) ||
		(g_brSelectedBrushes.Next()->Next() != &g_brSelectedBrushes))
	{
		Log::Warning("Must have a single brush selected.");
		return false;
	}
	if (g_brSelectedBrushes.Next()->owner->IsPoint())
	{
		Log::Warning("Cannot manipulate fixed size entities.");
		return false;
	}

	return true;
}

/*
===============
QE_UpdateWorkzone

update the workzone to a given brush
===============
*/
void QE_UpdateWorkzone(Brush* b)
{
	if (!b) return;
	assert(b != &g_brSelectedBrushes);

	// will update the workzone to the given brush
	g_qeglobals.d_v3WorkMin = b->mins;
	g_qeglobals.d_v3WorkMax = b->maxs;
}


/*
==================
QE_ExpandProjectPath
==================
*/
char *QE_ExpandProjectPath (char *p)
{
	static char	temp[1024];

	if (!p || !p[0])
		return NULL;
	if (p[0] == '/' || p[0] == '\\')
		return p;

	sprintf(temp, "%s/%s", g_project.basePath.data(), p);
	return temp;
}


