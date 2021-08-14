#include "pre.h"
#include "qe3.h"
#include "StringFormatter.h"
#include "map.h"
//#include <cstdarg>

/*
=================
CrashSave

alert the user and try to save the scene to a crash file if an unrecoverable
exception makes it all the way to the top
=================
*/
int CrashSave(const char* reason)
{
	static bool crashSaving = false;
	std::string badnews = reason;
	// protect against the wrong (unhandled) kind of crash looping the emergency save 
	// and vomiting bad crashmaps all over
	if (crashSaving)
	{
		badnews += "\nMap could not be saved. Sorry.";
		Terminate(badnews.c_str());
	}
	crashSaving = true;
	std::string crashmap;

	SYSTEMTIME time;

	GetSystemTime(&time);
	//sprintf(crashmap, "%s\\crash.%i%i%i%i%i%i.map", g_qePath, time.wHour, time.wMinute, time.wSecond, time.wDay, time.wMonth, time.wYear);
	crashmap = std::string(_S("%s\\crash.%i%i%i%i%i%i.map") << g_qePath << time.wHour << time.wMinute << time.wSecond << time.wDay << time.wMonth << time.wYear);
	try {
		g_map.Save(crashmap.c_str());
		//sprintf(badnews, "%s\r\nMap written to %s.", reason, crashmap);
		badnews += std::string(_S("\r\nMap written to %s.")  << crashmap);
	}
	catch (...)
	{
		//sprintf(badnews, "%s\r\nMap could not be saved. Sorry.", reason);
		badnews += "\r\nMap could not be saved. Sorry.";
	}

	Terminate(badnews.c_str());
	return 0;
}

void Terminate(const char* badnews)
{
	MessageBox(g_hwndMain, badnews, "QuakeEd 3: Unhandled Exception", MB_OK | MB_ICONERROR);

	// we except out of the main loop anyway so this shouldn't be necessary
	//exit(1);
}

/*
=================
SEHExceptionString

scruting the inscrutable
=================
*/
const char* SEHExceptionString(DWORD code)
{
	//Log::Print("code %i!\n", code);

	switch (code) {
	case EXCEPTION_ACCESS_VIOLATION:
		return "Access Violation";
	case EXCEPTION_DATATYPE_MISALIGNMENT:
		return "Datatype Misalignment";
	case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
		return "Array Bounds Exceeded";
	case EXCEPTION_FLT_DENORMAL_OPERAND:
		return "FP Denormal Operand";
	case EXCEPTION_FLT_DIVIDE_BY_ZERO:
		return "FP Divide by Zero";
	case EXCEPTION_FLT_INEXACT_RESULT:
		return "FP Inexact Result";
	case EXCEPTION_FLT_INVALID_OPERATION:
		return "FP Invalid Op";
	case EXCEPTION_FLT_OVERFLOW:
		return "FP Overflow";
	case EXCEPTION_FLT_STACK_CHECK:
		return "FP Stack Check";
	case EXCEPTION_FLT_UNDERFLOW:
		return "FP Underflow";
	case EXCEPTION_INT_DIVIDE_BY_ZERO:
		return "Int Divide by Zero";
	case EXCEPTION_INT_OVERFLOW:
		return "Int Overflow";
	case EXCEPTION_PRIV_INSTRUCTION:
		return "Privileged Instruction";
	case EXCEPTION_IN_PAGE_ERROR:
		return "In-Page Error";
	case EXCEPTION_ILLEGAL_INSTRUCTION:
		return "Illegal Instruction";
	case EXCEPTION_NONCONTINUABLE_EXCEPTION:
		return "Noncontinuable Exception";
	case EXCEPTION_STACK_OVERFLOW:
		return "Stack Overflow";
	case EXCEPTION_GUARD_PAGE:
		return "Guard Page";
	case EXCEPTION_INVALID_HANDLE:
		return "Invalid Handle";
	default:
		return "Unknown Structured Exception";
	}
}

qe3_exception::~qe3_exception()
{
	Log::Error(what());
	std::exception::~exception();
}

