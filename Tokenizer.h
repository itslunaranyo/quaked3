#pragma once

#include "strlib.h"

// does not own the string it is tokenizing, so make sure it lasts at least
// as long as you want the tokens to last
class Tokenizer
{
public:
	Tokenizer(const std::string& _str) : str(_str) {};
	Tokenizer(const std::string_view _str) : str(_str) {};
	~Tokenizer() {};

	void SplitBy(const char* ch) { splits.append(ch); }
	void SplitParens() { splits.append("()"); }
	void SplitCode() { splits.append("(),;"); }
	void DoubleQuotedTokens(bool b = true) { dblQuotedTokens = b; }
	void ParentheticalTokens(bool b = true) { parenTokens = b; }
	bool AtEnd() { return ofs == str.length(); }
	void Jump(size_t pos);

	std::size_t CurrentLine() { return line; }
	std::size_t Position() { return ofs; }
	bool CurrentInQuotes() { return currentInQuotes; }
	bool CurrentInParens() { return currentInParens; }

	std::string_view Next();
	std::string_view Current();
	std::string_view Prev();
	void All(StringViewList& tokenlist);
	void Line(StringViewList& tokenlist);

private:
	std::size_t ofs = 0, line = 0;
	std::string_view str, cur, prev;
	std::string splits;
	bool parenTokens = false;
	bool dblQuotedTokens = true;
	bool currentInQuotes = false;
	bool currentInParens = false;

	bool IsWordChar(const char& ch);
	std::string_view Advance(std::size_t start, std::size_t end, std::size_t newofs);
};

