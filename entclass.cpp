//==============================
//	entclass.cpp
//==============================

#include "qe3.h"
#include "io.h"
#include <algorithm>

/*
the classname, color triple, and bounding box are parsed out of comments
A ? size means take the exact brush size.

/*QUAKED <classname> (0 0 0) ?
/*QUAKED <classname> (0 0 0) (-8 -8 -8) (8 8 8)

Flag names can follow the size description:

/*QUAKED func_door (0 .5 .8) ? START_OPEN STONE_SOUND DOOR_DONT_LINK GOLD_KEY SILVER_KEY
*/

std::vector<EntClass*> EntClass::pointclasses;
std::vector<EntClass*> EntClass::brushclasses;
std::vector<EntClass*> EntClass::entclasses;
EntClass	*EntClass::badclass;
EntClass	*EntClass::worldspawn;

/*
==============
EntClass::EntClass
==============
*/
EntClass::EntClass() :
	name("UNINITIALIZED"), comments(0), 
	form(ECF_UNKNOWN), showFlags(0)
{
	mins = vec3(0);
	maxs = vec3(0);
	color = vec3(0);

	memset(&texdef, 0, sizeof(texdef));
	memset(flagnames, 0, MAX_FLAGS * 32);
}

/*
==============
EntClass::EntClass
==============
*/
EntClass::EntClass(const EntClass& other) :
	form(other.form), comments(other.comments),
	showFlags(other.showFlags), texdef(other.texdef)
{
	mins = other.mins;
	maxs = other.maxs;
	color = other.color;

	strcpy(name, other.name);
	memcpy(flagnames, other.flagnames, MAX_FLAGS * 32);
}

/*
==============
EntClass::~EntClass
==============
*/
EntClass::~EntClass()
{
}

/*
==============
EntClass::InitFromText
==============
*/
EntClass *EntClass::InitFromText(char *text)
{
	char		*t;
	int			len;
	int			r, i;
	char		*p, parms[256];
	EntClass	*e;
	char		color[MAX_TEXNAME];

	e = new EntClass();

	text += strlen("/*QUAKED ");

	// grab the name
	text = COM_Parse(text);
	strcpy(e->name, g_szComToken);

	// grab the color, reformat as texture name
	r = sscanf(text, " (%f %f %f)", &e->color[0], &e->color[1], &e->color[2]);
	if (r != 3)
	{
		Sys_Printf("Error parsing: %s\n", e->name);
		delete e;
		return nullptr;
	}
	//sprintf(color, "#%1.3f %1.3f %1.3f", e->color[0], e->color[1], e->color[2]);
	//strcpy(e->texdef.name, color);
	rgbToHex(e->color, color);
	e->texdef.Set(color);	// make a solid color texture

	while (*text != ')')
	{
		if (!*text)
		{
			Sys_Printf("Error parsing: %s\n", e->name);
			delete e;
			return nullptr;
		}
		text++;
	}
	text++;

	// get the size	
	text = COM_Parse(text);
	if (g_szComToken[0] == '(')
	{	// parse the size as two vectors
		e->form = ECF_POINT;

		r = sscanf(text, "%f %f %f) (%f %f %f)", 
			&e->mins[0], &e->mins[1], &e->mins[2], 
			&e->maxs[0], &e->maxs[1], &e->maxs[2]);
		if (r != 6)
		{
			Sys_Printf("Error parsing: %s\n", e->name);
			delete e;
			return nullptr;
		}

		for (i = 0; i < 2; i++)
		{
			while (*text != ')')
			{
				if (!*text)
				{
					Sys_Printf("Error parsing: %s\n", e->name);
					delete e;
					return nullptr;
				}
				text++;
			}
			text++;
		}
	}
	else
	{	// use the brushes
		e->form = ECF_BRUSH;
	}

	// get the flags

	// copy to the first /n
	p = parms;
	while (*text && *text != '\n')
		*p++ = *text++;
	*p = 0;
	text++;

	// any remaining words are parm flags
	p = parms;
	for (i = 0; i < 8; i++)
	{
		p = COM_Parse(p);
		if (!p)
			break;
		if (!strcmp(g_szComToken, "?"))		// lunaran - respect ? convention for unused spawnflags
		{
			e->flagnames[i][0] = 0;
			continue;
		}
		strcpy(e->flagnames[i], g_szComToken);
	}

	// find the length until close comment
	for (t = text; t[0] && !(t[0] == '*' && t[1] == '/'); t++)
		;

	// copy the comment block out
	len = t - text;
	e->comments.resize(len + 1);
	memcpy(*e->comments, text, len);
#if 0
	for (i = 0; i < len; i++)
		if (text[i] == '\n')
			e->comments[i] = '\r';
		else
			e->comments[i] = text[i];
#endif
	e->comments[len] = 0;

	// setup show flags
	e->showFlags = 0;

	if (_strnicmp(e->name, "worldspawn", 10) == 0)
	{
		e->showFlags = EFL_WORLDSPAWN;
	}
	else
	{
		if (e->form == ECF_BRUSH)
		{
			e->showFlags = EFL_BRUSHENTITY;
			if (_strnicmp(e->name, "trigger", 7) == 0)
				e->showFlags |= EFL_TRIGGER;
			if (_strnicmp(e->name, "func_wall", 9) == 0)
				e->showFlags |= EFL_FUNCWALL;
			if (_strnicmp(e->name, "func_detail", 11) == 0)
				e->showFlags |= EFL_DETAIL;
		}
		else
		{
			e->showFlags = EFL_POINTENTITY;
			if (_strnicmp(e->name, "light", 5) == 0)
				e->showFlags |= EFL_LIGHT;
			else if (_strnicmp(e->name, "monster", 7) == 0)
				e->showFlags |= EFL_MONSTER;
		}
	}

	if ((_strnicmp(e->name, "info_player", 11) == 0) ||
		(_strnicmp(e->name, "info_teleport", 13) == 0) ||
		(_strnicmp(e->name, "info_intermission", 17) == 0) ||
		(_strnicmp(e->name, "monster", 7) == 0) ||	// sikk: added monsters
		(_strnicmp(e->name, "path_corner", 11) == 0) ||
		(_strnicmp(e->name, "viewthing", 9) == 0))
		e->form |= ECF_ANGLE;
	/*
	if (_strcmpi(e->name, "path") == 0)
		e->showFlags |= ECLASS_PATH;
	*/
	return e;
}

/*
==============
EntClass::InitFromText
==============
*/
EntClass *EntClass::InitFromTextAndAdd(char *text)
{
	EntClass *e;
	e = InitFromText(text);
	if (e)
	{
		entclasses.push_back(e);
		e->AddToClassList();
	}
	return e;
}


/*
=================
EntClass::CreateOppositeForm

lunaran - create duplicate eclass with opposite fixedsize when a hacked point entity
with brushes or brush entity without any is created
=================
*/
EntClass *EntClass::CreateOppositeForm(EntClass *e)
{
	EntClass *dupe;

	dupe = new EntClass(*e);

	if (dupe->form & ECF_BRUSH)
	{
		dupe->form = (ECF_POINT | ECF_HACKED);
		dupe->mins[0] = dupe->mins[1] = dupe->mins[2] = -8;
		dupe->maxs[0] = dupe->maxs[1] = dupe->maxs[2] = 8;
		Sys_Printf("Creating fixed-size %s entity class definition\n", dupe->name);
	}
	else if (dupe->form & ECF_POINT)
	{
		dupe->form = (ECF_BRUSH | ECF_HACKED);
		Sys_Printf("Creating brush-based %s entity class definition\n", dupe->name);
	}
	else
		Error("Bad EntClass passed to CreateOppositeForm!\n");

	dupe->AddToClassList();
	return dupe;
}

/*
=================
EntClass::AddToClassList
=================
*/
void EntClass::AddToClassList()
{
	if (IsPointClass())
		pointclasses.push_back(this);
	else
		brushclasses.push_back(this);
}

/*
=================
EntClass::Clear
=================
*/
void EntClass::Clear()
{
	for (auto ecIt = brushclasses.begin(); ecIt != brushclasses.end(); ecIt++)
		delete *ecIt;
	for (auto ecIt = pointclasses.begin(); ecIt != pointclasses.end(); ecIt++)
		delete *ecIt;

	entclasses.clear();
	pointclasses.clear();
	brushclasses.clear();

	delete badclass;
}

/*
=================
EntClass::ScanFile
=================
*/
void EntClass::ScanFile(const char *filename)
{
	int			size;
	int			i;
	char		temp[1024];
	qeBuffer	fdata;

	QE_ConvertDOSToUnixName(temp, filename);
	Sys_Printf("ScanFile: %s\n", temp);

	size = IO_LoadFile(filename, fdata);

	for (i = 0; i < size; i++)
	{
		if (!strncmp((char*)&fdata[i], "/*QUAKED", 8))
		{
			if (!EntClass::InitFromTextAndAdd((char*)&fdata[i]))
				Warning("couldn't scan %s for entity definitions", filename);
		}
	}
}

/*
==============
EntClass::InitForSourceDirectory
==============
*/
void EntClass::InitForSourceDirectory(const char *path)
{
	struct _finddata_t	fileinfo;
	int		handle;
	char	filename[1024];
	char	filebase[1024];
	char    temp[1024];
	char	ext[8];
	char	fname[_MAX_FNAME];

	QE_ConvertDOSToUnixName(temp, path);
	Sys_Printf("ScanEntityPath: %s\n", temp);

	Clear();

	// check if entityFiles is a single file, directory path, or a wildcard path
	ExtractFileExtension(path, ext);
	if (*ext)
	{
		ExtractFilePath(path, filebase);
		ExtractFileName(path, fname);
		//if wildcard, scanfile on everything matching in the folder (likely '/*.qc')
		if (strchr(fname, '*'))
		{
			handle = _findfirst(path, &fileinfo);
			if (handle != -1)
			{
				do
				{
					sprintf(filename, "%s/%s", filebase, fileinfo.name);
					EntClass::ScanFile(filename);
				} while (_findnext(handle, &fileinfo) != -1);

				_findclose(handle);
			}
		}
		else
		{
			// just scan the single file (likely 'something.def')
			EntClass::ScanFile(path);
		}
	}
	else
	{
		//is directory, scanfile on everything in the folder
		sprintf(filebase, "%s/*.*", path);
		handle = _findfirst(filebase, &fileinfo);
		if (handle != -1)
		{
			do
			{
				sprintf(filename, "%s/%s", path, fileinfo.name);
				EntClass::ScanFile(filename);
			} while (_findnext(handle, &fileinfo) != -1);

			_findclose(handle);
		}
	}

	Sort();

	worldspawn = ForName("worldspawn", true, true);
	if (!worldspawn)
	{
		Warning("No worldspawn definition found in source! Creating a default worldspawn ...");
		worldspawn = EntClass::InitFromTextAndAdd("/*QUAKED worldspawn (0 0 0) ?\nthis is a default worldspawn definition. no worldspawn definition was found in source - are your project settings correct?\n");
	}

	badclass = EntClass::InitFromText("/*QUAKED UNKNOWN_CLASS (0 0.5 0) ?");
}

/*
==============
EntClass::Sort
==============
*/
void EntClass::Sort()
{
	std::sort(begin(), end(), eccmp);
}

/*
==============
EntClass::ForName

strict - do not look for or create a hacked opposite (ignores has_brushes bc the entclass form has priority anyway)
==============
*/
EntClass *EntClass::ForName(const char *name, bool has_brushes, bool strict)
{
	EntClass   *e;
	char		init[1024];

	if (!name)
		return badclass;

	if (strict)
	{
		for (auto ecIt = entclasses.begin(); ecIt != entclasses.end(); ecIt++)
		{
			e = (*ecIt);
			if (!strcmp(name, e->name))
				return e;
		}
		return nullptr;
	}

	if (has_brushes)
	{
		// check the brush classes
		for (auto ecIt = brushclasses.begin(); ecIt != brushclasses.end(); ecIt++)
		{
			e = (*ecIt);
			if (!strcmp(name, e->name))
				return e;
		}

		// if there isn't a brush class by that name, check the point classes for a hacked opposite
		for (auto ecIt = pointclasses.begin(); ecIt != pointclasses.end(); ecIt++)
		{
			e = (*ecIt);
			if (!strcmp(name, e->name))
			{
				// create a point-to-brush duplicate of the entclass
				e = CreateOppositeForm(e);
				return e;
			}
		}
	}
	else
	{
		// check the point classes
		for (auto ecIt = pointclasses.begin(); ecIt != pointclasses.end(); ecIt++)
		{
			e = (*ecIt);
			if (!strcmp(name, e->name))
				return e;
		}

		// if there isn't a point class by that name, check the brush classes for a hacked opposite
		for (auto ecIt = brushclasses.begin(); ecIt != brushclasses.end(); ecIt++)
		{
			e = (*ecIt);
			if (!strcmp(name, e->name))
			{
				if (!strcmp(name, "worldspawn"))	// never make a point-hacked worldspawn class
					return e;

				// create a brush-to-point duplicate of the entclass
				e = CreateOppositeForm(e);
				return e;
			}
		}
	}

	// create a new dummy class for it
	sprintf(init, "/*QUAKED %s (0 0.5 0) %s\nNot found in source.\n", name, has_brushes ? "?" : "(-8 -8 -8) (8 8 8)");
	e = EntClass::InitFromTextAndAdd(init);

	Sort();

	return e;
}