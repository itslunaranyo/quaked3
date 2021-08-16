#pragma once

#include "mathlib.h"

enum class cloneMode { OFFSET, INPLACE, DRAG };
enum class cameraMode { CLASSIC, WASD };
enum class texProjectMode { AXIAL, FACE };
enum class drawMode { WIRE, FLAT, TEXTURED };

struct qecfgUI_t
{
	int		TextureMode;	// filtering
	int		DrawMode;		// wireframe/flat/textured
	int		ViewFilter;
	int		Stipple;
	int		RadiantLights;

	float	Gamma;
	float	Brightness;

	int		PathlineMode;
	bool	HideUnusedTextures;

	bool	ShowAxis;
	bool	ShowBlocks;
	bool	ShowCameraGrid;
	bool	ShowCoordinates;
	bool	ShowLightRadius;
	bool	ShowMapBoundary;
	bool	ShowNames;
	bool	ShowSizeInfo;
	bool	ShowWorkzone;
	bool	ShowAngles;
};

struct qecfgEditor_t
{
	std::string QuakePath;
	bool LogConsole;
	bool LoadLastMap;
	int	AutosaveTime;
	bool Autosave;

	int	MapSize;
	int	UndoLevels;
	bool BrushPrecision;
	bool VFEModesExclusive;
	int CloneStyle;
	int CameraMoveStyle;
	int TexProjectionMode;

	bool CubicClip;
	int	CubicScale;
	int	CameraSpeed;
};

struct qecfgColors_t
{
	std::string name;
	vec3 brush,
		selection,
		tool,
		camBackground,
		camGrid,
		gridBackground,
		gridMinor,
		gridMajor,
		gridBlock,
		gridText,
		texBackground,
		texText;
};

struct qecfgProject_t
{
	std::string	name,
				basePath,
				mapPath,
				autosaveFile,
				entityFiles,
				entityFiles2,
				wadPath,
				defaultWads,
				paletteFile;
	bool	extTargets;

	qecfgProject_t() :
		name("Quake"), basePath("$QUAKE/id1/"), mapPath("$QUAKE/id1/maps/"), 
		autosaveFile("$QUAKE/id1/maps/autosave.map"), 
		entityFiles("$QE3/defs/quake.def"), entityFiles2(""),
		wadPath("$QUAKE/gfx/"), defaultWads("common.wad"), paletteFile(""),
		extTargets(false)
	{}
};

extern qecfgUI_t g_cfgUI;
extern qecfgEditor_t g_cfgEditor;
extern qecfgProject_t g_project;	// not saved, paths with macros expanded
extern qecfgColors_t g_colors;
