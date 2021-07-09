#pragma once

#include "mathlib.h"

// convenience class for formatting a string in place where a text parameter is expected
class StringFormatter {
public:
	StringFormatter(const char* fmt);
	~StringFormatter() {};

	StringFormatter& operator<<(const int& i);
	StringFormatter& operator<<(const size_t& i);
	StringFormatter& operator<<(const float& f);
	StringFormatter& operator<<(const double& d);
	StringFormatter& operator<<(const vec3& v);
	StringFormatter& operator<<(const dvec3& v);
	StringFormatter& operator<<(const char* c);
	StringFormatter& operator<<(const std::string_view sv);

	operator const std::string&() const { return buf; }
	operator const char*() const { return buf.c_str(); }

	std::string Emit();
protected:
	virtual void Commit() {};
	std::string buf;
	size_t ipct;
private:
	std::string_view format;

	bool Next();
	void Put(const char* str);
	void Put(const std::string_view str);
	int Precision();
};

// shorthand function for in-place formatting
StringFormatter _S(const char* fmt);
