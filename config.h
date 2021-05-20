//==============================
//	config.h
//==============================
#ifndef __CONFIG_H__
#define __CONFIG_H__

class EPair;
class Entity;

#define MAX_PROJNAME	64
#define MAX_CVARSTR		256


struct qecfgUI_t
{
	int		TextureMode;	// filtering
	int		DrawMode;		// wireframe/flat/textured
	float	Gamma;

	int		Stipple;
	int		RadiantLights;

	int		ViewFilter;
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
	bool	ShowPaths;
};

struct qecfgEditor_t
{
	char QuakePath[_MAX_DIR];
	bool LogConsole;
	int	LoadLastMap;
	int	AutosaveTime;
	bool Autosave;

	int	MapSize;
	int	UndoLevels;
	bool BrushPrecision;
	bool VFEModesExclusive;
	int CloneStyle;
	int TexProjectionMode;

	bool CubicClip;
	int	CubicScale;
	int	CameraSpeed;
};

struct qecfgColors_t
{
	char name[MAX_PROJNAME];
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
	char	name[MAX_PROJNAME];
	char	basePath[_MAX_DIR],
			mapPath[_MAX_DIR],
			autosaveFile[_MAX_FNAME],
			entityFiles[_MAX_FNAME],
			wadPath[_MAX_DIR],
			defaultWads[_MAX_FNAME],
			paletteFile[_MAX_FNAME];
	
	qecfgProject_t() :
		name("Quake"), basePath("$QUAKE/id1/"), mapPath("$QUAKE/id1/maps/"), 
		autosaveFile("$QUAKE/id1/maps/autosave.map"), entityFiles("$QE3/defs/quake.def"), 
		wadPath("$QUAKE/gfx/"), defaultWads("common.wad"), paletteFile("")
	{}
};

/*
config vars are just wrappers around a reference to some other editor state (in 
one of the above structs)

the cfgVar isn't necessary to the workings of any part of the editor other than 
reading/writing qe3.cfg, so they only contain methods for doing this - all other
getting/setting by the application is safe to perform directly on the referenced
variables
*/
class ConfigVar
{
public:
	ConfigVar(const char* varName) : name(varName) {}
	virtual ~ConfigVar() {}

	const char*		Name() { return name; }
	virtual void	Read(Entity &epc);
	virtual void	Write(std::ofstream &f) {}
	virtual void	Reset() {}
protected:
	const char*		name;

	virtual void	ReadPair(EPair &ep) {}
};

// these should be templated and could have been if I hadn't gotten sick of the fucking vagaries of template inheritance syntax
class ConfigVarInt : public ConfigVar
{
public:
	ConfigVarInt(int &v, const char* varName, const int defaultValue) : ConfigVar(varName), val(v), defaultVal(defaultValue) { Reset(); }
	~ConfigVarInt() {}

	inline int Value() { return val; }
	inline void Set(int newVal) { val = newVal; }
	void Write(std::ofstream &f);
	void Reset() { val = defaultVal; }
private:
	void ReadPair(EPair &ep);
	int &val;
	const int defaultVal;
};

class ConfigVarBool : public ConfigVar
{
public:
	ConfigVarBool(bool &v, const char* varName, const bool defaultValue) : ConfigVar(varName), val(v), defaultVal(defaultValue) { Reset(); }
	~ConfigVarBool() {}

	inline bool Value() { return val; }
	inline void Set(bool newVal) { val = newVal; }
	void Write(std::ofstream &f);
	void Reset() { val = defaultVal; }
private:
	void ReadPair(EPair &ep);
	bool &val;
	const bool defaultVal;
};

class ConfigVarFloat : public ConfigVar
{
public:
	ConfigVarFloat(float &v, const char* varName, const float defaultValue) : ConfigVar(varName), val(v), defaultVal(defaultValue) { val = defaultValue; }
	~ConfigVarFloat() {}

	inline float Value() { return val; }
	//explicit operator float() { return val; }
	inline void Set(float newVal) { val = newVal; }
	void Write(std::ofstream &f);
private:
	void ReadPair(EPair &ep);
	float &val;
	const float defaultVal;
};

class ConfigVarString : public ConfigVar
{
public:
	ConfigVarString(char* v, const char* varName, const char* defaultValue) : ConfigVar(varName), val(v), defaultVal(defaultValue) { Reset(); }
	~ConfigVarString() {}

	inline const char* Value() { return (const char*)*val; }
	//explicit operator const char*() { return (const char*)*val; }
	void Write(std::ofstream &f);
	inline void Set(const char* newVal) { strncpy(val, newVal, MAX_CVARSTR); }
	void Reset() { Set(defaultVal); }
private:
	void ReadPair(EPair &ep);
	const char* defaultVal;
	char *val;
};
/*
class ConfigVarProject : public ConfigVar
{
public:
	ConfigVarProject(qecfgProject_t &v, const char* varName, const qecfgProject_t defaultValue) : 
		ConfigVar(varName), val(v), defaultVal(defaultValue)
		{ Reset(); }
	~ConfigVarProject() {}

	inline void Set(qecfgProject_t newVal) { val = newVal; }
	void Read(Entity &epc);
	void Write(std::ofstream &f);
	void Reset() { val = defaultVal; }
private:
	qecfgProject_t &val;
	const qecfgProject_t defaultVal;
};
*/

class qeConfig
{
public:
	qeConfig();
	~qeConfig();

	// belongs in registry
	REBARBANDINFO rbiSettings[11];	// sikk - Save Rebar Band Info

	std::vector<qecfgProject_t> projectPresets;	// current project is always moved to the first index
	std::vector<qecfgColors_t> colorPresets;

	bool Load();
	void Save();
	void ExpandProjectPaths();
	void StandardColorPresets();

private:
	void Defaults();
	void WriteColor(std::ofstream &f, qecfgColors_t &col);
	bool WriteProject(std::ofstream &f, qecfgProject_t &proj);
	void ExpandProjectPath(char* src, char* dest, bool dir = false);

	bool ParseUI();
	bool ParseEditor();
	bool ParseColors(qecfgColors_t &colors);
	bool ParseProject();
	EPair *ParseCfgPairs();
};

extern qeConfig g_qeconfig;
extern qecfgUI_t g_cfgUI;
extern qecfgEditor_t g_cfgEditor;
extern qecfgProject_t g_project;	// not saved, paths with macros expanded
extern qecfgColors_t g_colors;


#endif
