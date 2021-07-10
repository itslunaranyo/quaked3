#pragma once

#include "StringFormatter.h"

namespace Log
{
	void Init();
	bool CheckWarningOrError();

	void Print(const char* msg);
	void Warning(const char* msg);

	// should only be called when exceptions are handled - use global Error() to throw
	void Error(const char* err);
};
