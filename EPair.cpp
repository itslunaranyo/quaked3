#include "pre.h"
#include "strlib.h"
#include "config.h"
#include "EPair.h"

std::map<std::string, EPairType> EPair::defaultTypes;

bool EPair::IsTarget() const { return (flags == EPairFlags::TARGET); }
bool EPair::IsTargetName() const { return (flags == EPairFlags::TARGETNAME); }

void EPair::UpdateFlags()
{
	// check for all the ways an entity can have an incoming target reference,
	// which so far thankfully all start with 'targetname'
	// in extended mode, any 'targetname' w/suffix is a valid target destination
	// in non-extended mode, only 'targetname' is valid
	if (g_project.extTargets ? strlib::StartsWith(key, "targetname") : (key == "targetname"))
	{
		flags = EPairFlags::TARGETNAME;
		return;
	}

	// check for all the ways an entity can have an outgoing target reference
	// (target/target2/3/4/killtarget/whatever silly ones dumptruck added)
	// in extended mode, any kv with 'target' in it is a valid target source
	// (not 'targetname'! but we already checked for it)
	// in non-extended mode, only 'target' is valid
	if (g_project.extTargets ? strlib::Contains(key, "target") : (key == "target"))
	{
		flags = EPairFlags::TARGET;
	}
}

void EPair::Set(const std::string& k, const std::string& v)
{
	SetKey(k);
	SetValue(v);
}

void EPair::SetKey(const std::string& k)
{
	assert(!k.empty());
	key = k;
	UpdateFlags();
}

void EPair::SetValue(const std::string& v) {
	value = v;
}
