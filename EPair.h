#pragma once

#include <map>

// all epairs are stored as strings regardless of field type
// str/float/vec3 are the only legal types in quake progs, others are editor-only
// overloads of those types to denote how progs will interpret them
enum class EPairType {
	UNKNOWN,
	STRING,	// quakeC derived
	FLOAT,	// quakeC derived
	VEC3,	// quakeC derived
	INT,	// FGD derived, masks float
	CHOICE,	// FGD derived, masks float
	FLAGS,	// FGD derived, masks float
	TARGETNAME,	// FGD derived, masks string
	//COLOR,
};
// EntClasses still contain overrides for these types on a per-field basis,
// because entity code in progs is free to inconsistently interpret floats as
// other numeric types ("delay" is a choice on lights but a float on most other
// entities, for example)

class EPair
{
public:
	EPair() : next(nullptr) {};
	EPair(const std::string& k, const std::string& v) : key(k), value(v), next(nullptr) { UpdateFlags(); };
	EPair(const std::string_view k, const std::string_view v) : key(k), value(v), next(nullptr) { UpdateFlags(); };
	EPair(const EPair& other) : next(other.next), key(other.key), value(other.value) { UpdateFlags(); };
	~EPair() {};

	EPair*		next;

	const std::string& GetKey() const { return key; }
	const std::string& GetValue() const { return value; }

	void Set(const std::string& k, const std::string& v);
	void SetKey(const std::string& k);
	void SetValue(const std::string& v);

	bool IsTarget() const;
	bool IsTargetName() const;

	static std::map<std::string, EPairType> defaultTypes;

private:
	std::string	key, value;
	enum class EPairFlags {
		NONE = 0x00,
		TARGET = 0x01,
		TARGETNAME = 0x02,
	} flags = EPairFlags::NONE;
	void		UpdateFlags();
};
