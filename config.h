//==============================
//	Config.h
//==============================
#ifndef __QECONFIG_H__
#define __QECONFIG_H__

#include "cfgvars.h"
class ConfigReader;

/*
config vars are just wrappers around a reference to some other editor state (in 
one of the above structs)

the cfgVar isn't necessary to the workings of any part of the editor other than 
reading/writing qe3.cfg, so they only contain methods for doing this - all other
getting/setting by the application is safe to perform directly on the referenced
variables in cfgvars.h
*/
class ConfigVar
{
public:
	ConfigVar(const char* varName) : name(varName) {}
	virtual ~ConfigVar() {}

	const char*		Name() { return name; }
	virtual void	Read(ConfigReader& cfgFile) {}
	virtual void	Write(std::ofstream &f) {}
	virtual void	Reset() {}
protected:
	const char*		name;

};

// these should be templated and could have been if I hadn't gotten sick of the fucking vagaries of template inheritance syntax
class ConfigVarInt : public ConfigVar
{
public:
	ConfigVarInt(int &v, const char* varName, const int defaultValue) : ConfigVar(varName), val(v), defaultVal(defaultValue) { Reset(); }
	~ConfigVarInt() {}

	inline int Value() { return val; }
	inline void Set(int newVal) { val = newVal; }
	void Read(ConfigReader& cfgFile);
	void Write(std::ofstream &f);
	void Reset() { val = defaultVal; }
private:
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
	void Read(ConfigReader& cfgFile);
	void Write(std::ofstream &f);
	void Reset() { val = defaultVal; }
private:
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
	void Read(ConfigReader& cfgFile);
	void Write(std::ofstream &f);
private:
	float &val;
	const float defaultVal;
};

class ConfigVarString : public ConfigVar
{
public:
	ConfigVarString(std::string& v, const char* varName, const std::string& defaultValue) : ConfigVar(varName), val(v), defaultVal(defaultValue) { Reset(); }
	~ConfigVarString() {}

	inline const char* Value() { return &val[0]; }
	//explicit operator const char*() { return (const char*)*val; }
	void Read(ConfigReader& cfgFile);
	void Write(std::ofstream &f);
	inline void Set(std::string& newVal) { val = newVal; }
	void Reset() { Set(defaultVal); }
private:
	std::string defaultVal;
	std::string& val;
};


class Config
{
public:
	Config();
	~Config();

	std::vector<qecfgProject_t> projectPresets;	// current project is always moved to the first index
	std::vector<qecfgColors_t> colorPresets;

	bool Load();
	void Save();
	static void ExpandProjectPath(const std::string& src, std::string& dest, const std::string& qPath = g_cfgEditor.QuakePath, bool dir = false);
	void ExpandProjectPaths(qecfgProject_t &src, qecfgProject_t &dest, const std::string& qPath = g_cfgEditor.QuakePath);
	void StandardColorPresets();

private:
	void Defaults();
	void ExpandProjectPaths();
	void WriteColor(std::ofstream &f, qecfgColors_t &col);
	void WriteProject(std::ofstream &f, qecfgProject_t &proj);

	bool ParseUI(ConfigReader& cfgFile);
	bool ParseEditor(ConfigReader& cfgFile);
	bool ParseColors(ConfigReader& cfgFile, qecfgColors_t &colors);
	bool ParseProject(ConfigReader& cfgFile);
};

extern Config g_qeconfig;

#endif
