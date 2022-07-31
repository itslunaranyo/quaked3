#pragma once

#include "mathlib.h"

typedef std::vector<std::string_view> StringViewList;

namespace strlib {
	void ToLower(std::string& str);
    bool StartsWith(const std::string_view& str, const std::string_view& pattern);
    bool EndsWith(const std::string_view& str, const std::string_view& pattern);
    bool Contains(const std::string_view& str, const std::string_view& pattern);

    std::string VecToString(const vec3 vec);
    std::string DVecToString(const dvec3 dvec);
    std::string VecToStringNice(const vec3 vec, const int dec = 3);
    std::string DVecToStringNice(const dvec3 dvec, const int dec = 6);
	std::string FloatToStringNice(const float f, const int dec = 3, const bool noTrunc = false);
	std::string DoubleToStringNice(const double f, const int dec = 6, const bool noTrunc = false);

	bool StringToVec(const std::string_view str, vec3& out);
	bool StringToDVec(const std::string_view str, dvec3& out);

	void RemoveDuplicates(StringViewList& svl);

	std::string RGBToHex(const vec3 vrgb);
	bool HexToRGB(const std::string& hex, vec3& vrgb);
    int Replace(std::string& target, const char* find, const char* repl);
}

