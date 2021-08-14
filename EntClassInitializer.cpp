#include "pre.h"
#include "strlib.h"
#include "qedefs.h"
#include "EntClassInitializer.h"
#include "EntClass.h"
#include "QcDefReader.h"
#include "FgdReader.h"
#include "Config.h"

EntClassInitializer::EntClassInitializer()
{
}

EntClassInitializer::~EntClassInitializer()
{
}

/*
=================
EntClassInitializer::InitForProject
=================
*/
bool EntClassInitializer::InitForProject()
{
	StringViewList classnames;
	std::string_view ext;
	EntityDefReader *defrd1, *defrd2 = nullptr;
	bool dual = false;

	// eliminate all existing entclasses
	EntClass::Reset();
	EPair::defaultTypes.clear();

	// user can specify at least one source of entity definitions, with an optional second 
	// as a supplement, because .qc and .fgd both define their own subsets of useful info
	// (.def is implicitly .qc) 
	//	- fgd: types and labels per class field, some helpfully overloaded types
	//	- qc: long tutorial text for class use, progs-accurate field definitions
	// user is also allowed to specify them in the config in either order, with conflicts 
	// resolved in favor of the first one

	ext = std::string_view(g_project.entityFiles).substr(g_project.entityFiles.find('.', 0) + 1);
	if (ext == "fgd")
		defrd1 = new FgdReader();
	else if (ext == "qc" || ext == "def")
		defrd1 = new QcDefReader();
	else
	{
		Error(_S("Unknown file format '%s' specified for entity definitions") << ext);
		return false;
	}
	defrd1->ReadFromPath(g_project.entityFiles);

	if (!g_project.entityFiles2.empty())
	{
		dual = true;
		ext = std::string_view(g_project.entityFiles2).substr(g_project.entityFiles2.find(".", 0) + 1);
		if (ext == "fgd")
			defrd2 = new FgdReader();
		else if (ext == "qc" || ext == "def")
			defrd2 = new QcDefReader();
		else
		{
			Error(_S("Unknown file format '%s' specified for entity definitions") << ext);
			return false;
		}
		defrd2->ReadFromPath(g_project.entityFiles2);
	}

	// build classname list
	defrd1->GetClassnames(classnames);
	if (dual)
	{
		// getting * from both is 100% totally redundant if the fgd/def agree like they should
		defrd2->GetClassnames(classnames);
		strlib::RemoveDuplicates(classnames);
		// ... "if"
	}

	// zip the two channels of information together
	for (std::string_view& classname : classnames)
	{
		EntClass* ec = new EntClass(classname);

		// cheap conflict resolution: read in reverse order and let late overwrite early
		if (dual) defrd2->GetDefinition(*ec);
		defrd1->GetDefinition(*ec);

		FinalizeEntClass(*ec);
		ec->AddToClassList();
	}
	
	// digest global field defs - has to come after eclass parsing because the fgd reader
	// has to extract types on a per-entity basis and see which option is most popular
	// these are order-independent because there's a correct way some types override others
	defrd1->GetTypes(EPair::defaultTypes);
	if (dual) defrd2->GetTypes(EPair::defaultTypes);

	return true;
}

void EntClassInitializer::FinalizeEntClass(EntClass& ec)
{
	// setup show flags
	ec.showFlags = 0;

	if (ec.name == "worldspawn")
	{
		ec.showFlags = EFL_WORLDSPAWN;
		EntClass::worldspawn = &ec;
	}
	else
	{
		if (ec.form == EntClass::ECF_BRUSH)
		{
			ec.showFlags = EFL_BRUSHENTITY;
			if (ec.name._Starts_with("trigger"))
				ec.showFlags |= EFL_TRIGGER;
			else if (ec.name._Starts_with("func_wall"))
				ec.showFlags |= EFL_FUNCWALL;
			else if (ec.name._Starts_with("func_detail"))
				ec.showFlags |= EFL_DETAIL;
		}
		else
		{
			ec.showFlags = EFL_POINTENTITY;
			if (ec.name._Starts_with("light"))
				ec.showFlags |= EFL_LIGHT;
			else if (ec.name._Starts_with("monster"))
			{
				ec.showFlags |= EFL_MONSTER;
			}
			else if (ec.name._Starts_with("path"))
			{
				ec.showFlags |= EFL_PATH;
			}
		}
	}

	if ((ec.name._Starts_with("info_player")) ||
		(ec.name._Starts_with("info_teleport")) ||
		(ec.name._Starts_with("info_intermission")) ||
		(ec.name._Starts_with("monster")) ||
		(ec.name._Starts_with("path")) ||
		(ec.name._Starts_with("viewthing")))
		ec.form |= EntClass::ECF_ANGLE;
}

