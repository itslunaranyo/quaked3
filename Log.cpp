#include "pre.h"
#include "config.h"
#include "strlib.h"
//#include <cstdarg>
#include <fstream>
#include "WndConsole.h"

static std::ofstream g_logFile;
static bool g_warningOrError = false;

void Log::Init()
{
	g_warningOrError = false;

	if (g_cfgEditor.LogConsole)
	{
		std::string logPath;
		logPath = "qe3.log";

		g_logFile.open(logPath, std::ios::trunc | std::ios::out);

		if (g_logFile.fail())
		{
			g_logFile.open(logPath, std::ios::out);
			if (g_logFile.fail())
			{
				//Dialog::AlertWarning("Failed to create log file.", "Console will not be logged to disk. Check write permissions in your QE3 directory.");
				MessageBox(g_hwndMain, "Failed to create log file. Console will not be logged to disk. Check write permissions in your QE3 directory.", "QuakeEd 3", MB_OK | MB_ICONEXCLAMATION);

				return;
			}
		}
	}
	Print("Console Logging: Started...\n");
}

// ================================================================

static void _print(const std::string& str, const char* prefix = nullptr)
{
	if (g_logFile.is_open())
	{
		if (prefix) g_logFile << prefix;
		g_logFile << str << std::flush;	// keep this buffer flushed so we have everything after a crash
	}
#ifdef _DEBUG
	if (prefix)
		OutputDebugStringA(prefix);
	OutputDebugStringA(str.c_str());
#endif

	WndConsole::AddText(str.c_str());
}

void Log::Print(const char* msg)
{
	_print(msg);
}

void Log::Warning(const char* warning)
{
	g_warningOrError = true;
	_print(warning, "Warning:");
}

void Log::Error(const char* error)
{
	g_warningOrError = true;
	_print(error, "ERROR:");
}

bool Log::CheckWarningOrError()
{
	if (g_warningOrError)
	{
		g_warningOrError = false;
		return true;
	}
	return false;
}
