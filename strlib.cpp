#include "pre.h"
#include "strlib.h"
#include <algorithm>
#include <charconv>

namespace strlib {

void ToLower(std::string& str)
{
	for (int i = 0; i < str.length(); ++i) 
	{
		str[i] = std::tolower(str[i]);
	}
}

bool StartsWith(const std::string_view& str, const std::string_view& pattern)
{
	if (str.length() < pattern.length())
		return false;
	return std::strncmp(&str[0], &pattern[0], pattern.length()) == 0;
}

bool EndsWith(const std::string_view& str, const std::string_view& pattern)
{
	if (str.length() < pattern.length())
		return false;
	return std::strncmp(&str[str.length() - pattern.length()], &pattern[0], pattern.length()) == 0;
}

bool Contains(const std::string_view& str, const std::string_view& pattern)
{
	if (str.length() < pattern.length())
		return false;
	for (int i = 0; i < str.length() - pattern.length(); ++i)
	{
		if (std::strncmp(&str[i], &pattern[0], pattern.length()) == 0)
			return true;
	}
	return false;
}

std::string VecToString(const vec3 v)
{
	std::string out;
	size_t l;
	l = std::snprintf(nullptr, 0, "%f %f %f", v.x, v.y, v.z);
	out.resize(l, 0);
	std::snprintf(out.data(), l + 1, "%f %f %f", v.x, v.y, v.z);
	return out;
}

std::string DVecToString(const dvec3 v)
{
	std::string out;
	size_t l;
	l = std::snprintf(nullptr, 0, "%lf %lf %lf", v.x, v.y, v.z);
	out.resize(l, 0);
	std::snprintf(out.data(), l + 1, "%lf %lf %lf", v.x, v.y, v.z);
	return out;
}


// ================================
// the Nice prints
// for use in presenting numbers to the UI (when you don't need to know that your
// texture is rotated 45.00000001 degrees)
// ================================
std::string VecToStringNice(const vec3 vec, const int dec)
{
	std::string vs;
	std::string c;
	vs.reserve((size_t)dec * 6 + 6);

	c = DoubleToStringNice(vec.x, dec);
	vs.append(c);
	vs.append(" ");
	c = DoubleToStringNice(vec.y, dec);
	vs.append(c);
	vs.append(" ");
	c = DoubleToStringNice(vec.z, dec);
	vs.append(c);

	return vs;
}

std::string DVecToStringNice(const dvec3 vec, const int dec)
{
	std::string vs;
	std::string c;
	vs.reserve((size_t)dec * 6 + 6);

	c = DoubleToStringNice(vec.x, dec);
	vs.append(c);
	vs.append(" ");
	c = DoubleToStringNice(vec.y, dec);
	vs.append(c);
	vs.append(" ");
	c = DoubleToStringNice(vec.z, dec);
	vs.append(c);

	return vs;
}

std::string FloatToStringNice(const float f, const int dec, const bool noTrunc)
{
	return DoubleToStringNice(f, dec, noTrunc);
}
std::string DoubleToStringNice(const double f, const int dec, const bool noTrunc)
{
	char fmt[16];
	size_t p;
	std::string str;

	std::sprintf(fmt, "%%1.%if", dec);

	// trunc safety
	float fp = f + ((f < 0) ? -0.000001f : 0.000001f);

	p = std::snprintf(nullptr, 0, fmt, f);
	str.resize(p);
	std::sprintf(&str[0], fmt, f);

	for (p; p > 0; --p)
	{
		if (str[p] == '0') continue;
		if (str[p] == '.')
		{
			if (noTrunc)
				p += dec;
			break;
		}
		break;
	}
	str.resize(p);
	return str;
}




bool StringToVec(const std::string_view str, vec3& out)
{
	if (str.empty())
		return false;

	vec3 temp;

	// wastes a lot of time in strnlen:
	//if (std::sscanf(&str[0], "%f %f %f", &temp.x, &temp.y, &temp.z) != 3)
		//return false;

	// ugly and 20x faster:
	const char *mark, *start, *end;
	start = &str[0];
	end = start + str.size();
	while (std::isspace(*start))
		++start;
	for (int i = 0; i < 3; ++i)
	{
		mark = std::from_chars(start, end, temp[i]).ptr;
		if (mark == start)
			return false;
		while (std::isspace(*mark))
			++mark;
		start = mark;
	}
	out = temp;
	return true;
}

bool StringToDVec(const std::string_view str, dvec3& out)
{
	if (str.empty())
		return false;

	dvec3 temp;
	const char* mark, * start, * end;
	start = &str[0];
	end = start + str.size();
	while (std::isspace(*start))
		++start;
	for (int i = 0; i < 3; ++i)
	{
		mark = std::from_chars(start, end, temp[i]).ptr;
		if (mark == start)
			return false;
		while (std::isspace(*mark))
			++mark;
		start = mark;
	}
	out = temp;
	return true;
}

void RemoveDuplicates(StringViewList& svl)
{
	std::sort(svl.begin(), svl.end(), [](const std::string_view& a, const std::string_view& b) {return a < b;});
	auto end = std::unique(svl.begin(), svl.end());
	svl.erase(end, svl.end());
}

std::string RGBToHex(const vec3 vrgb)
{
	int i, c;
	unsigned rgb[3];
	char hx[16] = { '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f' };
	std::string hex("#ff00ff");

	c = 1;
	for (i = 0; i < 3; i++)
	{
		rgb[i] = (unsigned)(255 * fabs(vrgb[i]) + 0.5f);
		hex[c++] = hx[rgb[i] >> 4];
		hex[c++] = hx[rgb[i] & 15];
	}
	hex[c] = 0;
	return hex;
}

bool HexToRGB(const std::string& hex, vec3& vrgb)
{
	int i;
	unsigned rgb[6];

	if (hex[0] != '#')
		return false;

	for (i = 0; i < 6; i++)
	{
		if (hex[i + 1] >= 'a')
			rgb[i] = hex[i + 1] - 'a' + 10;
		else
			rgb[i] = hex[i + 1] - '0';
	}
	for (i = 0; i < 3; i++)
	{
		vrgb[i] = (rgb[i * 2] * 16 + rgb[i * 2 + 1]) / 255.0f;
	}
	return true;
}

}

