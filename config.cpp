//==============================
//	config.cpp
//==============================

#include "pre.h"
#include "qedefs.h"
#include "strlib.h"
#include "cmdlib.h"
#include "Config.h"
#include "ConfigReader.h"
#include "WndConfig.h"

#include <algorithm>

Config::Config() { /*Defaults();*/ }
Config::~Config() {}

qecfgUI_t g_cfgUI;
qecfgEditor_t g_cfgEditor;
qecfgProject_t g_project;
qecfgColors_t g_colors;

Config	g_qeconfig;

// here we are, a big dumbass flat global list of cvars, just like every id engine

// ui/drawing vars:
ConfigVarInt	cfgv_RenderMode(g_cfgUI.TextureMode, "TextureMode", 1);
ConfigVarInt	cfgv_DrawMode(g_cfgUI.DrawMode, "DrawMode", 2);
ConfigVarInt	cfgv_ViewFilter(g_cfgUI.ViewFilter, "ViewFilter", BFL_HIDDEN);
ConfigVarInt	cfgv_Stipple(g_cfgUI.Stipple, "Stipple", 1);
ConfigVarInt	cfgv_RadiantLights(g_cfgUI.RadiantLights, "RadiantLights", 1);
ConfigVarFloat	cfgv_Gamma(g_cfgUI.Gamma, "Gamma", 1.0f);
ConfigVarFloat	cfgv_Brightness(g_cfgUI.Brightness, "Brightness", 1.0f);
ConfigVarInt	cfgv_PathlineMode(g_cfgUI.PathlineMode, "PathMode", 1);
ConfigVarBool	cfgv_HideUnusedTextures(g_cfgUI.HideUnusedTextures, "HideUnusedTextures", 1);

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

ConfigVar* const cfgUIVars[] = {
	&cfgv_RenderMode,
	&cfgv_DrawMode,
	&cfgv_ViewFilter,
	&cfgv_Stipple,
	&cfgv_RadiantLights,
	&cfgv_Gamma,
	&cfgv_Brightness,
	&cfgv_PathlineMode,
	&cfgv_HideUnusedTextures,

	&cfgv_ShowAxis,
	&cfgv_ShowBlocks,
	&cfgv_ShowCameraGrid,
	&cfgv_ShowCoordinates,
	&cfgv_ShowLightRadius,
	&cfgv_ShowMapBoundary,
	&cfgv_ShowNames,
	&cfgv_ShowSizeInfo,
	&cfgv_ShowWorkzone,
	&cfgv_ShowAngles
};
const int cfgUIVarCount = sizeof(cfgUIVars) / sizeof(ConfigVar*);

// editor/env vars:
ConfigVarString	cfgv_QuakePath(g_cfgEditor.QuakePath, "QuakePath", "c:/quake/");
#ifdef _DEBUG
ConfigVarBool	cfgv_LogConsole(g_cfgEditor.LogConsole, "LogConsole", 1);
#else
ConfigVarBool	cfgv_LogConsole(g_cfgEditor.LogConsole, "LogConsole", 0);
#endif
ConfigVarBool	cfgv_LoadLastMap(g_cfgEditor.LoadLastMap, "LoadLastMap", 0);
ConfigVarInt	cfgv_AutosaveTime(g_cfgEditor.AutosaveTime, "AutosaveTime", 5);
ConfigVarBool	cfgv_Autosave(g_cfgEditor.Autosave, "Autosave", 1);

ConfigVarInt	cfgv_MapSize(g_cfgEditor.MapSize, "MapSize", 8192);
ConfigVarInt	cfgv_UndoLevels(g_cfgEditor.UndoLevels, "UndoLevels", 32);
ConfigVarBool	cfgv_BrushPrecision(g_cfgEditor.BrushPrecision, "BrushPrecision", 0);
ConfigVarBool	cfgv_VFEModesExclusive(g_cfgEditor.VFEModesExclusive, "VFEModesExclusive", 1);
ConfigVarInt	cfgv_CloneStyle(g_cfgEditor.CloneStyle, "CloneStyle", CLONE_DRAG);
ConfigVarInt	cfgv_CameraMoveStyle(g_cfgEditor.CameraMoveStyle, "CameraMoveStyle", CAMERA_WASD);
ConfigVarInt	cfgv_TexProjectionMode(g_cfgEditor.TexProjectionMode, "TexProjectionMode", TEX_PROJECT_AXIAL);

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
	&cfgv_CameraMoveStyle,
	&cfgv_TexProjectionMode,

	&cfgv_CubicClip,
	&cfgv_CubicScale,
	&cfgv_CameraSpeed
};
const int cfgEditorVarCount = sizeof(cfgEditorVars) / sizeof(ConfigVar*);

void ConfigVarInt::Write(std::ofstream &f)		{ f << "\"" << name << "\" \"" << val << "\"\n"; }
void ConfigVarBool::Write(std::ofstream &f)		{ f << "\"" << name << "\" \"" << (val ? "1" : "0") << "\"\n"; }
void ConfigVarFloat::Write(std::ofstream &f)	{ f << "\"" << name << "\" \"" << val << "\"\n"; }
void ConfigVarString::Write(std::ofstream &f)	{ f << "\"" << name << "\" \"" << val << "\"\n"; }

void ConfigVarInt::Read(ConfigReader& cfgFile)		{ cfgFile.ReadInt(name, val); }
void ConfigVarBool::Read(ConfigReader& cfgFile)		{ cfgFile.ReadBool(name, val); }
void ConfigVarFloat::Read(ConfigReader& cfgFile)	{ cfgFile.ReadFloat(name, val); }
void ConfigVarString::Read(ConfigReader& cfgFile)	{ cfgFile.ReadString(name, val); }

void Config::Defaults()
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

bool Config::ParseUI(ConfigReader& cfgFile)
{
	for (int i = 0; i < sizeof(cfgUIVars) / sizeof(cfgUIVars[0]); i++)
	{
		cfgUIVars[i]->Read(cfgFile);
	}
	return false;
}

bool Config::ParseEditor(ConfigReader& cfgFile)
{
	for (int i = 0; i < sizeof(cfgEditorVars)/sizeof(cfgEditorVars[0]); i++)
	{
		cfgEditorVars[i]->Read(cfgFile);
	}
	std::string temp = g_cfgEditor.QuakePath;
	ExpandProjectPath(temp, g_cfgEditor.QuakePath, "", true);
	return false;
}


/*
=====================================================================

	PROJECT

=====================================================================
*/

void Config::ExpandProjectPaths()
{
	ExpandProjectPaths(*projectPresets.begin(), g_project);
}

void Config::ExpandProjectPaths(qecfgProject_t &src, qecfgProject_t &dest, const std::string& qPath)
{
	//auto prj = projectPresets.begin();
	dest.name = src.name;
	ExpandProjectPath(src.basePath, dest.basePath, qPath, true);
	ExpandProjectPath(src.mapPath, dest.mapPath, qPath, true);
	ExpandProjectPath(src.autosaveFile, dest.autosaveFile, qPath);
	ExpandProjectPath(src.entityFiles, dest.entityFiles, qPath);
	ExpandProjectPath(src.entityFiles2, dest.entityFiles2, qPath);
	ExpandProjectPath(src.wadPath, dest.wadPath, qPath, true);
	ExpandProjectPath(src.defaultWads, dest.defaultWads, qPath);
	ExpandProjectPath(src.paletteFile, dest.paletteFile, qPath);

	dest.extTargets = src.extTargets;
}

void Config::ExpandProjectPath(const std::string& src, std::string& dest, const std::string& qPath, bool dir)
{
	std::string path = src;
	size_t ofs = 0, last = 0;
	size_t srclen = src.size();

	dest.clear();
	dest.reserve(srclen * 2);

	while (ofs < srclen)
	{
		if (!(src[ofs] == '$' || src[ofs] == '\\' || src[ofs] == '/'))
		{
			++ofs;
			continue;
		}
		dest.append(&src[last], ofs - last);
		if (src[ofs] == '\\' || src[ofs] == '/')
		{
			if (!strlib::EndsWith(dest,"\\"))
				dest.append("\\");
			while (src[ofs] == '\\' || src[ofs] == '/')
			{
				++ofs;
				if (ofs == srclen)
					break;
			}
			last = ofs;
			continue;
		}
		++ofs;
		if (std::strncmp(&src[ofs], "QUAKE", 5) == 0)
		{
			dest.append(qPath);
			ofs += 5;
			last = ofs;
			continue;
		}
		if (std::strncmp(&src[ofs], "QE3", 3) == 0)
		{
			dest.append(g_qePath);
			ofs += 3;
			last = ofs;
			continue;
		}
	}
	dest.append(&src[last], ofs - last);

	if (dir && dest[dest.size() - 1] != '\\')
		dest.append("\\");
}

bool Config::ParseProject(ConfigReader& cfgFile)
{
	qecfgProject_t proj;

	cfgFile.ReadString("name", proj.name);
	cfgFile.ReadString("basePath", proj.basePath);
	cfgFile.ReadString("mapPath", proj.mapPath);
	cfgFile.ReadString("autosaveFile", proj.autosaveFile);
	cfgFile.ReadString("entityFiles", proj.entityFiles);
	cfgFile.ReadString("entityFiles2", proj.entityFiles2);
	cfgFile.ReadString("wadPath", proj.wadPath);
	cfgFile.ReadString("defaultWads", proj.defaultWads);
	cfgFile.ReadString("paletteFile", proj.paletteFile);

	cfgFile.ReadBool("extTargets", proj.extTargets);

	projectPresets.push_back(proj);
	return false;
}

void Config::WriteProject(std::ofstream &f, qecfgProject_t &proj)
{
	f << "\"name\" \"" << proj.name << "\"\n";
	f << "\"basePath\" \"" << proj.basePath << "\"\n";
	f << "\"mapPath\" \"" << proj.mapPath << "\"\n";
	f << "\"autosaveFile\" \"" << proj.autosaveFile << "\"\n";
	f << "\"entityFiles\" \"" << proj.entityFiles << "\"\n";
	f << "\"wadPath\" \"" << proj.wadPath << "\"\n";
	f << "\"defaultWads\" \"" << proj.defaultWads << "\"\n";
	f << "\"paletteFile\" \"" << proj.paletteFile << "\"\n";

	f << "\"extTargets\" \"" << (proj.extTargets ? "1" : "0") << "\"\n";
}

/*
=====================================================================

	COLORS

=====================================================================
*/

bool Config::ParseColors(ConfigReader& cfgFile, qecfgColors_t &colors)
{
	cfgFile.ReadString("name", colors.name);

	cfgFile.ReadVec3("brush", colors.brush);
	cfgFile.ReadVec3("selection", colors.selection);
	cfgFile.ReadVec3("tool", colors.tool);
	cfgFile.ReadVec3("camBackground", colors.camBackground);
	cfgFile.ReadVec3("camGrid", colors.camGrid);
	cfgFile.ReadVec3("gridBackground", colors.gridBackground);
	cfgFile.ReadVec3("gridMinor", colors.gridMinor);
	cfgFile.ReadVec3("gridMajor", colors.gridMajor);
	cfgFile.ReadVec3("gridBlock", colors.gridBlock);
	cfgFile.ReadVec3("gridText", colors.gridText);
	cfgFile.ReadVec3("texBackground", colors.texBackground);
	cfgFile.ReadVec3("texText", colors.texText);

	return false;
}

void Config::WriteColor(std::ofstream &f, qecfgColors_t &col)
{
	std::string vec;

	f << "\"name\" \"" << col.name << "\"\n";
	f << "\"brush\" \"" <<			strlib::VecToString(col.brush) << "\"\n";
	f << "\"selection\" \"" <<		strlib::VecToString(col.selection) << "\"\n";
	f << "\"tool\" \"" <<			strlib::VecToString(col.tool) << "\"\n";
	f << "\"camBackground\" \"" <<	strlib::VecToString(col.camBackground) << "\"\n";
	f << "\"camGrid\" \"" <<		strlib::VecToString(col.camGrid) << "\"\n";
	f << "\"gridBackground\" \"" <<	strlib::VecToString(col.gridBackground) << "\"\n";
	f << "\"gridMinor\" \"" <<		strlib::VecToString(col.gridMinor) << "\"\n";
	f << "\"gridMajor\" \"" <<		strlib::VecToString(col.gridMajor) << "\"\n";
	f << "\"gridBlock\" \"" <<		strlib::VecToString(col.gridBlock) << "\"\n";
	f << "\"gridText\" \"" <<		strlib::VecToString(col.gridText) << "\"\n";
	f << "\"texBackground\" \"" <<	strlib::VecToString(col.texBackground) << "\"\n";
	f << "\"texText\" \"" <<		strlib::VecToString(col.texText) << "\"\n";
}

void Config::StandardColorPresets()
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

bool Config::Load()
{
	std::string cfgPath, cfgNextLine;
	bool	haveProject, haveColors, haveColorPresets;

#ifdef _DEBUG
	cfgPath = g_qePath + "\\qe3d.cfg";
#else
	cfgPath = g_qePath + "\\qe3.cfg";
#endif

	ConfigReader cfgFile;
	if (!cfgFile.Open(cfgPath) || cfgFile.Empty())
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

	while (cfgFile.ReadSection())
	{
		if (cfgFile.currentSection() == "editor")
			ParseEditor(cfgFile);
		else if (cfgFile.currentSection() == "ui")
			ParseUI(cfgFile);
		else if (cfgFile.currentSection() == "project")
		{
			haveProject = true;
			ParseProject(cfgFile);
		}
		else if (cfgFile.currentSection() == "colors")
		{
			haveColors = true;
			ParseColors(cfgFile, g_colors);
		}
		else if (cfgFile.currentSection() == "colorpreset")
		{
			haveColorPresets = true;
			colorPresets.push_back(g_colors);
			ParseColors(cfgFile, *colorPresets.rbegin());
		}
	};

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
				if (!_stricmp(projIt->name.c_str(), g_pszArgV[i]))
				{
					Log::Print(_S("Selecting project %s from command line\n") << projIt->name);
					std::rotate(projectPresets.begin(), projIt, projIt + 1);
					break;
				}
			}
		}
	}

	ExpandProjectPaths();
	return true;
}

void Config::Save()
{
	std::string cfgpath = g_qePath;

#ifdef _DEBUG
	cfgpath += "\\qe3d.cfg";
#else
	cfgpath += "\\qe3.cfg";
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

	Log::Print("Settings saved to qe3.cfg\n");
}

