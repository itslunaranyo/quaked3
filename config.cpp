//==============================
//	config.cpp
//==============================

#include "qe3.h"
#include <fstream>
#include <algorithm>

qeConfig::qeConfig() { /*Defaults();*/ }
qeConfig::~qeConfig() {}

qecfgUI_t g_cfgUI;
qecfgEditor_t g_cfgEditor;
qecfgProject_t g_project;
qecfgColors_t g_colors;

// here we are, a big dumbass flat global list of cvars, just like every id engine

// ui/drawing vars:
ConfigVarInt	cfgv_RenderMode(g_cfgUI.TextureMode, "TextureMode", 1);
ConfigVarInt	cfgv_DrawMode(g_cfgUI.DrawMode, "DrawMode", 2);
ConfigVarInt	cfgv_ViewFilter(g_cfgUI.ViewFilter, "ViewFilter", BFL_HIDDEN);
ConfigVarInt	cfgv_Stipple(g_cfgUI.Stipple, "Stipple", 1);
ConfigVarInt	cfgv_RadiantLights(g_cfgUI.RadiantLights, "RadiantLights", 1);
ConfigVarFloat	cfgv_Gamma(g_cfgUI.Gamma, "Gamma", 1.0f);

ConfigVarBool	cfgv_ShowAxis(g_cfgUI.ShowAxis, "ShowAxis", 0);
ConfigVarBool	cfgv_ShowBlocks(g_cfgUI.ShowBlocks, "ShowBlocks", 0);
ConfigVarBool	cfgv_ShowCameraGrid(g_cfgUI.ShowCameraGrid, "ShowCameraGrid", 0);
ConfigVarBool	cfgv_ShowCoordinates(g_cfgUI.ShowCoordinates, "ShowCoordinates", 1);
ConfigVarBool	cfgv_ShowLightRadius(g_cfgUI.ShowLightRadius, "ShowLightRadius", 0);
ConfigVarBool	cfgv_ShowMapBoundary(g_cfgUI.ShowMapBoundary, "ShowMapBoundary", 1);
ConfigVarBool	cfgv_ShowNames(g_cfgUI.ShowNames, "ShowNames", 0);
ConfigVarBool	cfgv_ShowSizeInfo(g_cfgUI.ShowSizeInfo, "ShowSizeInfo", 1);
ConfigVarBool	cfgv_ShowWorkzone(g_cfgUI.ShowWorkzone, "ShowWorkzone", 0);
ConfigVarBool	cfgv_ShowAngles(g_cfgUI.ShowAngles, "ShowAngles", 1);
ConfigVarBool	cfgv_ShowPaths(g_cfgUI.ShowPaths, "ShowPaths", 1);

ConfigVar* const cfgUIVars[] = {
	&cfgv_RenderMode,
	&cfgv_DrawMode,
	&cfgv_ViewFilter,
	&cfgv_Stipple,
	&cfgv_RadiantLights,
	&cfgv_Gamma,

	&cfgv_ShowAxis,
	&cfgv_ShowBlocks,
	&cfgv_ShowCameraGrid,
	&cfgv_ShowCoordinates,
	&cfgv_ShowLightRadius,
	&cfgv_ShowMapBoundary,
	&cfgv_ShowNames,
	&cfgv_ShowSizeInfo,
	&cfgv_ShowWorkzone,
	&cfgv_ShowAngles,
	&cfgv_ShowPaths
};
const int cfgUIVarCount = sizeof(cfgUIVars) / sizeof(ConfigVar*);

// editor/env vars:
ConfigVarString	cfgv_QuakePath(g_cfgEditor.QuakePath, "QuakePath", "c:/quake/");
#ifdef _DEBUG
ConfigVarBool	cfgv_LogConsole(g_cfgEditor.LogConsole, "LogConsole", 1);
#else
ConfigVarBool	cfgv_LogConsole(g_cfgEditor.LogConsole, "LogConsole", 0);
#endif
ConfigVarInt	cfgv_LoadLastMap(g_cfgEditor.LoadLastMap, "LoadLastMap", 0);
ConfigVarInt	cfgv_AutosaveTime(g_cfgEditor.AutosaveTime, "AutosaveTime", 5);
ConfigVarBool	cfgv_Autosave(g_cfgEditor.Autosave, "Autosave", 1);

ConfigVarInt	cfgv_MapSize(g_cfgEditor.MapSize, "MapSize", 8192);
ConfigVarInt	cfgv_UndoLevels(g_cfgEditor.UndoLevels, "UndoLevels", 32);
ConfigVarBool	cfgv_BrushPrecision(g_cfgEditor.BrushPrecision, "BrushPrecision", 0);
ConfigVarBool	cfgv_VFEModesExclusive(g_cfgEditor.VFEModesExclusive, "VFEModesExclusive", 1);
ConfigVarInt	cfgv_CloneStyle(g_cfgEditor.CloneStyle, "CloneStyle", CLONE_OFFSET);

ConfigVarBool	cfgv_CubicClip(g_cfgEditor.CubicClip, "CubicClip", 1);
ConfigVarInt	cfgv_CubicScale(g_cfgEditor.CubicScale, "CubicScale", 32);
ConfigVarInt	cfgv_CameraSpeed(g_cfgEditor.CameraSpeed, "CameraSpeed", 1024);

ConfigVar* const cfgEditorVars[] = {
	&cfgv_QuakePath,
	&cfgv_LogConsole,
	&cfgv_LoadLastMap,
	&cfgv_AutosaveTime,
	&cfgv_Autosave,

	&cfgv_MapSize,
	&cfgv_UndoLevels,
	&cfgv_BrushPrecision,
	&cfgv_VFEModesExclusive,
	&cfgv_CloneStyle,

	&cfgv_CubicClip,
	&cfgv_CubicScale,
	&cfgv_CameraSpeed
};
const int cfgEditorVarCount = sizeof(cfgEditorVars) / sizeof(ConfigVar*);

void ConfigVar::Read(Entity &epc)
{
	EPair* ep = epc.GetEPair(name);
	if (!ep) return;
	ReadPair(*ep);
}

void ConfigVarInt::Write(std::ofstream &f)		{ f << "\"" << name << "\" \"" << val << "\"\n"; }
void ConfigVarBool::Write(std::ofstream &f)		{ f << "\"" << name << "\" \"" << (val ? "1" : "0") << "\"\n"; }
void ConfigVarFloat::Write(std::ofstream &f)	{ f << "\"" << name << "\" \"" << val << "\"\n"; }
void ConfigVarString::Write(std::ofstream &f)	{ f << "\"" << name << "\" \"" << val << "\"\n"; }
//void ConfigVarProject::Write(std::ofstream &f) {}

void ConfigVarInt::ReadPair(EPair &ep)		{ val = (sscanf((char*)*ep.value, "%i", &val) == 1) ? val : defaultVal; }
void ConfigVarBool::ReadPair(EPair &ep)		{ int x; val = (sscanf((char*)*ep.value, "%i", &x) == 1) ? (x != 0) : defaultVal; }
void ConfigVarFloat::ReadPair(EPair &ep)	{ val = (sscanf((char*)*ep.value, "%f", &val) == 1) ? val : defaultVal; }
void ConfigVarString::ReadPair(EPair &ep)	{ Set((char*)*ep.value); }
//void ConfigVarProject::Read(Entity &epc) {}

void qeConfig::Defaults()
{
	for (int i = 0; i < cfgEditorVarCount; i++)
		cfgEditorVars[i]->Reset();
	for (int i = 0; i < cfgUIVarCount; i++)
		cfgUIVars[i]->Reset();

	colorPresets.clear();
	StandardColorPresets();
	g_colors = colorPresets[0];

	projectPresets.clear();
	projectPresets.emplace_back();
	ExpandProjectPaths();
}

EPair *qeConfig::ParseCfgPairs()
{
	EPair *epnext, *ep;
	ep = nullptr;
	epnext = nullptr;
	while (1)
	{
		if (!GetToken(true))
			break;
		if (g_szToken[0] == '[')
			break;
		ep = EPair::ParseEpair();
		ep->next = epnext;
		epnext = ep;
	} 
	return ep;
}

bool qeConfig::ParseUI()
{
	Entity ent;
	ent.epairs = ParseCfgPairs();
	for (int i = 0; i < sizeof(cfgUIVars) / sizeof(cfgUIVars[0]); i++)
	{
		cfgUIVars[i]->Read(ent);
	}
	return false;
}

bool qeConfig::ParseEditor()
{
	Entity ent;
	ent.epairs = ParseCfgPairs();

	for (int i = 0; i < sizeof(cfgEditorVars)/sizeof(cfgEditorVars[0]); i++)
	{
		cfgEditorVars[i]->Read(ent);
	}

	return false;
}


/*
=====================================================================

	PROJECT

=====================================================================
*/

void qeConfig::ExpandProjectPaths()
{
	auto prj = projectPresets.begin();
	strncpy(g_project.name, prj->name, MAX_PROJNAME);
	ExpandProjectPath(prj->basePath, g_project.basePath, true);
	ExpandProjectPath(prj->mapPath, g_project.mapPath, true);
	ExpandProjectPath(prj->autosaveFile, g_project.autosaveFile);
	ExpandProjectPath(prj->entityFiles, g_project.entityFiles);
	ExpandProjectPath(prj->wadPath, g_project.wadPath, true);
	ExpandProjectPath(prj->defaultWads, g_project.defaultWads);
	ExpandProjectPath(prj->paletteFile, g_project.paletteFile);
}

void qeConfig::ExpandProjectPath(char *src, char *dest, bool dir)
{
	char *s, *d, *ds;
	s = src;
	d = dest;
	ds = dest;

	while (*s)
	{
		if (*s == '\\' || *s == '/')
		{
			if (*ds != '/')
				*d++ = '/';
			while (*s == '\\' || *s == '/')
				s++;
		}
		else if (*s == '$')
		{
			if (!strncmp(s, "$QUAKE", 6))
			{
				strcpy(d, g_cfgEditor.QuakePath);
				d += strlen(g_cfgEditor.QuakePath);
				if (*(d-1) != '/')
					*d++ = '/';
				s += 6;
			}
			else if (!strncmp(s, "$QE3", 4))
			{
				// $QE3 is just cwd, skip the characters
				s += 4;
				while (*s == '\\' || *s == '/')
					*s++;
				*d++ = '.';
				*d++ = '/';
			}
			else
			{
				*d++ = *s++;
			}
		}
		else
			*d++ = *s++;
		ds = d - 1;
	}
	if (dir && (*(d - 1) != '/'))
	{
		*d++ = '/';
	}
	*d = 0;
}

bool qeConfig::ParseProject()
{
	Entity ent;
	qecfgProject_t proj;
	EPair* ep;

	ent.epairs = ParseCfgPairs();
	if ((ep = ent.GetEPair("name")) != nullptr)
		strncpy(proj.name, (char*)*ep->value, MAX_PROJNAME);
	if ((ep = ent.GetEPair("basePath")) != nullptr)
		strncpy(proj.basePath, (char*)*ep->value, _MAX_DIR);
	if ((ep = ent.GetEPair("mapPath")) != nullptr)
		strncpy(proj.mapPath, (char*)*ep->value, _MAX_DIR);
	if ((ep = ent.GetEPair("autosaveFile")) != nullptr)
		strncpy(proj.autosaveFile, (char*)*ep->value, _MAX_FNAME);
	if ((ep = ent.GetEPair("entityFiles")) != nullptr)
		strncpy(proj.entityFiles, (char*)*ep->value, _MAX_FNAME);
	if ((ep = ent.GetEPair("wadPath")) != nullptr)
		strncpy(proj.wadPath, (char*)*ep->value, _MAX_DIR);
	if ((ep = ent.GetEPair("defaultWads")) != nullptr)
		strncpy(proj.defaultWads, (char*)*ep->value, _MAX_FNAME);
	if ((ep = ent.GetEPair("paletteFile")) != nullptr)
		strncpy(proj.paletteFile, (char*)*ep->value, _MAX_FNAME);

	projectPresets.push_back(proj);
	return false;
}

bool qeConfig::WriteProject(std::ofstream &f, qecfgProject_t &proj)
{
	f << "\"name\" \"" << proj.name << "\"\n";
	f << "\"basePath\" \"" << proj.basePath << "\"\n";
	f << "\"mapPath\" \"" << proj.mapPath << "\"\n";
	f << "\"autosaveFile\" \"" << proj.autosaveFile << "\"\n";
	f << "\"entityFiles\" \"" << proj.entityFiles << "\"\n";
	f << "\"wadPath\" \"" << proj.wadPath << "\"\n";
	f << "\"defaultWads\" \"" << proj.defaultWads << "\"\n";
	f << "\"paletteFile\" \"" << proj.paletteFile << "\"\n";
	return true;
}


/*
=====================================================================

	COLORS

=====================================================================
*/

bool qeConfig::ParseColors(qecfgColors_t &colors)
{
	Entity ent;
	EPair* ep;
	ent.epairs = ParseCfgPairs();
	
	if ((ep = ent.GetEPair("name")) != nullptr)
		strncpy(colors.name, (char*)*ep->value, MAX_PROJNAME);

	ent.GetKeyValueVector("brush", colors.brush);
	ent.GetKeyValueVector("selection", colors.selection);
	ent.GetKeyValueVector("tool", colors.tool);
	ent.GetKeyValueVector("camBackground", colors.camBackground);
	ent.GetKeyValueVector("camGrid", colors.camGrid);
	ent.GetKeyValueVector("gridBackground", colors.gridBackground);
	ent.GetKeyValueVector("gridMinor", colors.gridMinor);
	ent.GetKeyValueVector("gridMajor", colors.gridMajor);
	ent.GetKeyValueVector("gridBlock", colors.gridBlock);
	ent.GetKeyValueVector("gridText", colors.gridText);
	ent.GetKeyValueVector("texBackground", colors.texBackground);
	ent.GetKeyValueVector("texText", colors.texText);

	return false;
}

void qeConfig::WriteColor(std::ofstream &f, qecfgColors_t &col)
{
	char vec[64];

	f << "\"name\" \"" << col.name << "\"\n";

	VecToString(col.brush, vec);
	f << "\"brush\" \"" << vec << "\"\n";
	VecToString(col.selection, vec);
	f << "\"selection\" \"" << vec << "\"\n";
	VecToString(col.tool, vec);
	f << "\"tool\" \"" << vec << "\"\n";
	VecToString(col.camBackground, vec);
	f << "\"camBackground\" \"" << vec << "\"\n";
	VecToString(col.camGrid, vec);
	f << "\"camGrid\" \"" << vec << "\"\n";
	VecToString(col.gridBackground, vec);
	f << "\"gridBackground\" \"" << vec << "\"\n";
	VecToString(col.gridMinor, vec);
	f << "\"gridMinor\" \"" << vec << "\"\n";
	VecToString(col.gridMajor, vec);
	f << "\"gridMajor\" \"" << vec << "\"\n";
	VecToString(col.gridBlock, vec);
	f << "\"gridBlock\" \"" << vec << "\"\n";
	VecToString(col.gridText, vec);
	f << "\"gridText\" \"" << vec << "\"\n";
	VecToString(col.texBackground, vec);
	f << "\"texBackground\" \"" << vec << "\"\n";
	VecToString(col.texText, vec);
	f << "\"texText\" \"" << vec << "\"\n";
}

void qeConfig::StandardColorPresets()
{
	colorPresets.reserve(colorPresets.size() + 10);
	colorPresets.push_back({ "QE/Radiant",
		vec3(0),		// brushes
		vec3(1,0,0),	// selection
		vec3(0,0,1),	// tool (clipper/etc)
		vec3(0.25f),		// camBack
		vec3(0.2f),		// camGrid
		vec3(1),		// gridBack
		vec3(0.75f),		// gridMinor
		vec3(0.5f),		// gridMajor
		vec3(0,0,1),	// gridBlock
		vec3(0),		// gridText
		vec3(0.2f),		// texBack
		vec3(0.7f)		// texText
	});
	colorPresets.push_back({ "Black & Green",
		vec3(0.8f),		// brushes
		vec3(0,1,0),	// selection
		vec3(1,1,0),	// tool (clipper/etc)
		vec3(0.25f),		// camBack
		vec3(0.2f),		// camGrid
		vec3(0),		// gridBack
		vec3(0.2f),		// gridMinor
		vec3(0.3f,0.5f,0.5f),		// gridMajor
		vec3(0,0,1),	// gridBlock
		vec3(1),		// gridText
		vec3(0.0f),		// texBack
		vec3(0.5f)		// texText
	});
	colorPresets.push_back({ "Lava",
		vec3(0.75f),	// brushes
		vec3(1,0,0),	// selection
		vec3(1.0f,0.9f,0.0f),	// tool (clipper/etc)
		vec3(0.2f),		// camBack
		vec3(0.3f),		// camGrid
		vec3(0.2f),		// gridBack
		vec3(0.25f),	// gridMinor
		vec3(0.3f),		// gridMajor
		vec3(0.35f),	// gridBlock
		vec3(0.9f),		// gridText
		vec3(0.2f),		// texBack
		vec3(0.7f)		// texText
	});
	colorPresets.push_back({ "Worldcraft",
		vec3(1),		// brushes
		vec3(1,0,0),	// selection
		vec3(0,1,1),	// tool (clipper/etc)
		vec3(0),		// camBack
		vec3(0.2f),		// camGrid
		vec3(0),		// gridBack
		vec3(0.2f),		// gridMinor
		vec3(0.35f),		// gridMajor
		vec3(0,0.5f,0.5f),	// gridBlock
		vec3(1),		// gridText
		vec3(0.0f),		// texBack
		vec3(0.75f)		// texText
	});
	colorPresets.push_back({ "UnrealEd",
		vec3(0),		// brushes
		vec3(1,0,0),	// selection
		vec3(0,1,1),	// tool (clipper/etc)
		vec3(0),		// camBack
		vec3(0,0,0.5f),		// camGrid
		vec3(0.64f),		// gridBack
		vec3(0.58f),		// gridMinor
		vec3(0.47f),		// gridMajor
		vec3(0.25f),	// gridBlock
		vec3(0),		// gridText
		vec3(0.0f),		// texBack
		vec3(0.6f)		// texText
	});
	colorPresets.push_back({ "Blender",
		vec3(0),		// brushes
		vec3(1),		// selection
		vec3(0.3f,0.7f,0.7f),	// tool (clipper/etc)
		vec3(0.25f),		// camBack
		vec3(0.2f),		// camGrid
		vec3(0.45f),		// gridBack
		vec3(0.4f),		// gridMinor
		vec3(0.36f),		// gridMajor
		vec3(0.25f),		// gridBlock
		vec3(1),		// gridText
		vec3(0.2f),		// texBack
		vec3(0.7f)		// texText
	});
	colorPresets.push_back({ "Maya",
		vec3(0,0,0.25f),		// brushes
		vec3(0,1,0.5f),		// selection
		vec3(1,0.5f,0.25f),	// tool (clipper/etc)
		vec3(0.57f),		// camBack
		vec3(0.47f),		// camGrid
		vec3(0.64f),		// gridBack
		vec3(0.58f),		// gridMinor
		vec3(0.47f),		// gridMajor
		vec3(0.25f),		// gridBlock
		vec3(0),		// gridText
		vec3(0.0f),		// texBack
		vec3(0.6f)		// texText
	});
	colorPresets.push_back({ "3DS Max",
		vec3(0.7f),		// brushes
		vec3(1),		// selection
		vec3(1,0,0),	// tool (clipper/etc)
		vec3(0.48f),	// camBack
		vec3(0.36f),	// camGrid
		vec3(0.48f),	// gridBack
		vec3(0.42f),	// gridMinor
		vec3(0.36f),	// gridMajor
		vec3(0.25f),	// gridBlock
		vec3(0),		// gridText
		vec3(0.2f),		// texBack
		vec3(0.7f)		// texText
	});
	colorPresets.push_back({ "Lightwave",
		vec3(0.75f),		// brushes
		vec3(1,1,0.75f),	// selection
		vec3(0,0.5f,1),	// tool (clipper/etc)
		vec3(0.42f),	// camBack
		vec3(0.56f),	// camGrid
		vec3(0.42f),	// gridBack
		vec3(0.48f),	// gridMinor
		vec3(0.56f),	// gridMajor
		vec3(0.25f),	// gridBlock
		vec3(0.75f),		// gridText
		vec3(0.2f),		// texBack
		vec3(0.7f)		// texText
	});
}


/*
=====================================================================

	IO

=====================================================================
*/

bool qeConfig::Load()
{
	qeBuffer cfgbuf;
	char	cfgpath[MAX_PATH];
	bool	haveProject, haveColors, haveColorPresets;

	strcpy(cfgpath, g_qePath);
#ifdef _DEBUG
	strcpy(cfgpath + strlen(g_qePath), "\\qe3d.cfg");
#else
	strcpy(cfgpath + strlen(g_qePath), "\\qe3.cfg");
#endif
	if (IO_LoadFile(cfgpath, cfgbuf) < 1)
	{
		// cfg doesn't exist, do first run dialog song and dance
		projectPresets.emplace_back();	// add one default project
		StandardColorPresets();
		g_colors = colorPresets[0];
		DoConfigWindowProject();
		return false;
	}

	projectPresets.clear();
	colorPresets.clear();

	haveProject = false;
	haveColors = false; 
	haveColorPresets = false;

	StartTokenParsing((char*)*cfgbuf);
	GetToken(true);
	while (*g_szToken)
	{
		if (g_szToken[0] == '[')
		{
			if (!strcmp(g_szToken, "[editor]"))
				ParseEditor();
			else if (!strcmp(g_szToken, "[ui]"))
				ParseUI();
			else if (!strcmp(g_szToken, "[project]"))
			{
				haveProject = true;
				ParseProject();
			}
			else if (!strcmp(g_szToken, "[colors]"))
			{
				haveColors = true;
				ParseColors(g_colors);
			}
			else if (!strcmp(g_szToken, "[colorpreset]"))
			{
				haveColorPresets = true;
				colorPresets.push_back(g_colors);
				ParseColors(*colorPresets.rbegin());
			}
		}
		else
		{
			Sys_Printf("Warning: Unrecognized token in qe3.cfg: %s\n", g_szToken);
			GetToken(true);
		}
	}

	if (!haveColorPresets)
		StandardColorPresets();
	if (!haveColors)
		g_colors = colorPresets[0];
	if (!haveProject)
	{
		// we had a config but no project could be parsed from it
		projectPresets.emplace_back();	// add one default project
		DoConfigWindowProject();	// pop up the window
		return false;
	}

	// check for a project name on the command line
	if (g_nArgC > 1)
	{
		for (auto projIt = projectPresets.begin(); projIt != projectPresets.end(); ++projIt)
		{
			for (int i = 1; i < g_nArgC; i++)
			{
				if (!_stricmp(g_pszArgV[i], projIt->name))
				{
					std::rotate(projectPresets.begin(), projIt, projIt + 1);
					break;
				}
			}
		}
	}
	ExpandProjectPaths();
	return true;
}

void qeConfig::Save()
{
	char	cfgpath[MAX_PATH];

	strcpy(cfgpath, g_qePath);
#ifdef _DEBUG
	strcpy(cfgpath + strlen(g_qePath), "\\qe3d.cfg");
#else
	strcpy(cfgpath + strlen(g_qePath), "\\qe3.cfg");
#endif

	std::ofstream fs(cfgpath);
	fs << "[editor]\n";
	for (int i = 0; i < cfgEditorVarCount; i++)
		cfgEditorVars[i]->Write(fs);

	fs << "\n[ui]\n";
	for (int i = 0; i < cfgUIVarCount; i++)
		cfgUIVars[i]->Write(fs);

	for (auto prjIt = projectPresets.begin(); prjIt != projectPresets.end(); ++prjIt)
	{
		fs << "\n[project]\n";
		WriteProject(fs, *prjIt);
	}

	fs << "\n[colors]\n";
	WriteColor(fs, g_colors);

	for (auto cpIt = colorPresets.begin(); cpIt != colorPresets.end(); ++cpIt)
	{
		fs << "\n[colorpreset]\n";
		WriteColor(fs, *cpIt);
	}

	fs << "\n";

	Sys_Printf("Settings saved to qe3.cfg\n");
}

