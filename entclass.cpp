//==============================
//	entclass.cpp
//==============================

#include "pre.h"
#include "mathlib.h"
#include "StringFormatter.h"
#include "EntClass.h"
#include <algorithm>

std::map<std::string, EntClass*> EntClass::pointclasses;
std::map<std::string, EntClass*> EntClass::brushclasses;
std::vector<EntClass*> EntClass::entclasses;
EntClass	*EntClass::badclass;
EntClass	*EntClass::worldspawn;

/*
==============
EntClass::EntClass
==============
*/
EntClass::EntClass() :
	name("UNINITIALIZED"), comments("Not found in source."), 
	form(ECF_UNKNOWN), showFlags(0), mins(0), maxs(0), color(0)
{
	//memset(&texdef, 0, sizeof(texdef));
	//for (int i = 0; i < MAX_FLAGS; ++i)
	//	flagnames[i] = "";
}

EntClass::EntClass(std::string_view& _n) :
	name(_n), comments("Not found in source."),
	form(ECF_UNKNOWN), showFlags(0), mins(0), maxs(0), color(0)
{
	//memset(&texdef, 0, sizeof(texdef));
	//for (int i = 0; i < MAX_FLAGS; ++i)
	//	flagnames[i] = "";
}

EntClass::EntClass(const EntClass& other) :
	form(other.form), comments(other.comments), name(other.name),
	mins(other.mins), maxs(other.maxs), color(other.color),
	showFlags(other.showFlags)//, texdef(other.texdef)
{
	for (int i = 0; i < MAX_FLAGS; ++i)
		flagnames[i] = other.flagnames[i];
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
EntClass::ForName

strict - do not look for or create a hacked opposite (ignores has_brushes bc the entclass form has priority anyway)
==============
*/
EntClass* EntClass::ForName(const std::string& name, const bool has_brushes, const bool strict)
{
	EntClass* e;

	if (name.empty())
		return badclass;

	if (strict)
	{
		for (auto ecIt = entclasses.begin(); ecIt != entclasses.end(); ecIt++)
		{
			e = (*ecIt);
			if (name == e->name)
				return e;
		}
		return nullptr;
	}

	if (has_brushes)
	{
		// check the brush classes
		e = brushclasses[name];
		if (e) return e;

		// if there isn't a brush class by that name, check the point classes to make a hacked opposite
		e = pointclasses[name];
		if (e)
		{
			// create a point-to-brush duplicate of the entclass
			e = CreateOppositeForm(e);
			return e;
		}
	}
	else
	{
		// check the point classes
		e = pointclasses[name];
		if (e) return e;

		// if there isn't a point class by that name, check the brush classes to make a hacked opposite
		if (name != "worldspawn")	// never make a point-hacked worldspawn class
		{
			e = brushclasses[name];
			if (e)
			{
				// create a point-to-brush duplicate of the entclass
				e = CreateOppositeForm(e);
				return e;
			}
		}
	}

	// create a new dummy class for it
	e = new EntClass();
	e->name = name;
	e->comments = "Not found in source.";
	e->color = vec3(0, 0.5f, 0);
	if (has_brushes)
	{
		e->form = ECF_BRUSH;
	}
	else
	{
		e->form = ECF_POINT;
		e->mins = vec3(-8);
		e->maxs = vec3(8);
	}
	e->AddToClassList();

	Sort();

	return e;
}



/*
=================
EntClass::AddToClassList
=================
*/
void EntClass::AddToClassList()
{
	entclasses.push_back(this);
	if (IsPointClass())
	{
		if (pointclasses[this->name])
			Error(_S("Created duplicate pointclass %s") << this->name);
		pointclasses[this->name] = this;
	}
	else
	{
		if (brushclasses[this->name])
			Error(_S("Created duplicate brushclass %s") << this->name);
		brushclasses[this->name] = this;
	}
}


/*
=================
EntClass::CreateOppositeForm

lunaran - create duplicate eclass with opposite fixedsize when a hacked point entity
with brushes or brush entity without any is created
=================
*/
EntClass* EntClass::CreateOppositeForm(EntClass* e)
{
	EntClass* dupe;

	dupe = new EntClass(*e);

	if (dupe->form & ECF_BRUSH)
	{
		dupe->form = (ECF_POINT | ECF_HACKED);
		dupe->mins[0] = dupe->mins[1] = dupe->mins[2] = -8;
		dupe->maxs[0] = dupe->maxs[1] = dupe->maxs[2] = 8;
		Log::Print(_S("Creating fixed-size %s entity class definition\n") << dupe->name);
		pointclasses[dupe->name] = dupe;
	}
	else if (dupe->form & ECF_POINT)
	{
		dupe->form = (ECF_BRUSH | ECF_HACKED);
		Log::Print(_S("Creating brush-based %s entity class definition\n") << dupe->name);
		brushclasses[dupe->name] = dupe;
	}
	else
		Error("Bad EntClass passed to CreateOppositeForm!\n");

	return dupe;
}
/*
=================
EntClass::Reset
=================
*/
void EntClass::Reset()
{
	for (auto ecIt = brushclasses.begin(); ecIt != brushclasses.end(); ecIt++)
		delete ecIt->second;
	for (auto ecIt = pointclasses.begin(); ecIt != pointclasses.end(); ecIt++)
		delete ecIt->second;

	entclasses.clear();
	pointclasses.clear();
	brushclasses.clear();

	delete badclass;
	//delete worldspawn;
	badclass = nullptr;
	worldspawn = nullptr;
}

/*
==============
EntClass::Sort
==============
*/
void EntClass::Sort()
{
	std::sort(begin(), end(), 
		[] (EntClass* a, EntClass* b) -> bool {
			return (a->name.compare(b->name) < 0);
		} );
}

