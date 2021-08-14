#include "pre.h"
#include "strlib.h"
#include "Tokenizer.h"

bool Tokenizer::IsWordChar(const char& ch) { 
	return !(std::isspace(ch) || splits.find(ch) != -1);
}

std::string_view Tokenizer::Advance(std::size_t start, std::size_t end, std::size_t newofs)
{
	prev = cur;
	cur = std::string_view(str).substr(start, end - start);
	ofs = newofs;
	return cur;
}

void Tokenizer::Jump(size_t pos)
{
	if (pos > str.length())
		ofs = str.length();
	else
		ofs = pos;
}

std::string_view Tokenizer::Next()
{
	std::size_t next, len, nl;
	char closeQuote;

	currentInParens = false;
	currentInQuotes = false;

	len = str.length();
	while (ofs < len)
	{
		if (std::isspace(str[ofs]))
		{
			if (str[ofs] == '\n')
				++line;
			++ofs;
			continue;
		}
		closeQuote = 0;

		// skip comments
		if (str[ofs] == '/' && str[ofs + 1] == '/')
		{
			nl = str.find('\n', ofs + 2);
			if (nl == -1)
			{
				ofs = len;	// eof in comment
				return std::string_view();
			}
			ofs = nl;
			continue;
		}

		// grab quoted strings as single tokens
		if (dblQuotedTokens && str[ofs] == '\"')
		{
			currentInQuotes = true;
			closeQuote = '\"';
		}
		if (parenTokens && str[ofs] == '(')
		{
			currentInParens = true;
			closeQuote = ')';
		}

		if (closeQuote)
		{
			next = str.find(closeQuote, ofs + 1);
			if (next == -1)
			{
				ofs = len;
				currentInParens = false;
				currentInQuotes = false;
				Error(_S("Quoted token is incomplete (EOF): %s\n") << str);
				return std::string_view();
			}
			nl = str.find('\n', ofs + 1);
			if (nl != -1 && nl < next)
			{
				currentInParens = false;
				currentInQuotes = false;
				Error(_S("Quoted token is incomplete (EOL): %s\n") << str);
				return std::string_view();
			}
			return Advance(ofs + 1, next, next + 1);
		}

		// treat special characters as delimiters regardless of whitespace
		if (!IsWordChar(str[ofs]))
		{
			return Advance(ofs, ofs + 1, ofs + 1);
		}

		next = ofs;
		while (next < len && IsWordChar(str[next]))
			++next;
		return Advance(ofs, next, next);
	}
	return std::string_view();
}

std::string_view Tokenizer::Current() { return cur; }
std::string_view Tokenizer::Prev() { return prev; }

void Tokenizer::All(StringViewList& tokenlist)
{
	while (!AtEnd())
	{
		Next();
		if (cur.empty())
			return;
		tokenlist.push_back(cur);
	}
}

void Tokenizer::Line(StringViewList& tokenlist)
{
	size_t end = str.find_first_of("\r\n", ofs);
	while (ofs < end)
	{
		Next();
		if (cur.empty())
			return;
		tokenlist.push_back(cur);
	}
}
