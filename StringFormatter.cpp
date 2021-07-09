#include "pre.h"
#include "strlib.h"
#include "StringFormatter.h"

StringFormatter _S(const char* fmt) { return StringFormatter(fmt); }


StringFormatter::StringFormatter(const char* fmt) : format(fmt), ipct(0)
{
	buf.reserve(512);
	if (!Next())
		Commit();
}

bool StringFormatter::Next()
{
	if (ipct >= format.size())
		return false;
	size_t mark = ipct;
	while (format[mark] != '%')
	{
		mark = format.find("%", mark);
		if (mark == -1 || mark == format.size() - 1)
		{
			buf.append(format.substr(ipct));
			return false;
		}
		if (format[mark + 1] == '%')
		{
			mark += 2;
			continue;
		}
	}
	buf.append(format.substr(ipct, mark - ipct));
	ipct = mark + 2;
	return true;
}

std::string StringFormatter::Emit()
{
	std::string out;
	out.swap(buf);
	return out;
}

int StringFormatter::Precision()
{
	return std::atoi(&format[ipct - 1]);
}

void StringFormatter::Put(const char* str)
{
	assert(!this->format.empty());
	this->buf.append(str);
	if (!this->Next())
		Commit();
}
void StringFormatter::Put(const std::string_view str)
{
	assert(!this->format.empty());
	this->buf.append(str);
	if (!this->Next())
		Commit();
}


// ================================================================


StringFormatter& StringFormatter::operator<<(const int& i)
{
	char buf[32];
	std::snprintf(buf, 32, "%i", i);
	Put(buf);
	return *this;
}

StringFormatter& StringFormatter::operator<<(const size_t& i)
{
	char buf[32];
	std::snprintf(buf, 32, "%zi", i);
	Put(buf);
	return *this;
}

StringFormatter& StringFormatter::operator<<(const float& f)
{
	int prec = Precision();
	char buf[128];
	char fmt[16] = "%f";
	if (prec > 0)
		std::snprintf(fmt, 16, "%%1.%df", prec);
	std::snprintf(buf, 64, fmt, f);
	Put(buf);
	return *this;
}

StringFormatter& StringFormatter::operator<<(const double& d)
{
	int prec = Precision();
	char buf[512];
	char fmt[16] = "%lf";
	if (prec > 0)
		std::snprintf(fmt, 16, "%%1.%df", prec);
	std::snprintf(buf, 512, fmt, d);
	Put(buf);
	return *this;
}

StringFormatter& StringFormatter::operator<<(const vec3& v)
{
	int prec = Precision();
	if (prec)
		Put(strlib::VecToStringNice(v,prec));
	else
		Put(strlib::VecToString(v));
	return *this;
}

StringFormatter& StringFormatter::operator<<(const dvec3& v)
{
	int prec = Precision();
	if (prec)
		Put(strlib::DVecToStringNice(v, prec));
	else
		Put(strlib::DVecToString(v));
	return *this;
}

StringFormatter& StringFormatter::operator<<(const char* c)
{
	Put(c);
	return *this;
}

StringFormatter& StringFormatter::operator<<(const std::string_view sv)
{
	Put(sv);
	return *this;
}

