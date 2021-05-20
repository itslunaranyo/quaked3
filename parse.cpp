//==============================
//	parse.cpp
//==============================

#include "pre.h"
#include "qe3.h"
#include "parse.h"

char	g_szToken[MAXTOKEN];
bool	g_bUnget;
const char   *g_pszScript;
int		g_nScriptLine;

/*
==================
StartTokenParsing
==================
*/
void StartTokenParsing (const char *data)
{
	g_nScriptLine = 1;
	g_pszScript = data;
	g_bUnget = false;
}

/*
==================
GetToken
==================
*/
bool GetToken (bool crossline)
{
	char *token_p;

	if (g_bUnget)	// is a token already waiting?
		return true;

	bool skip = true;
	while (skip)
	{
		skip = false;
		while (*g_pszScript <= 32)
		{
			if (!*g_pszScript)
			{
				if (!crossline)
					Error("Line %d is incomplete.", g_nScriptLine);
				*g_szToken = 0;
				return false;
			}
			if (*g_pszScript++ == '\n')
			{
				if (!crossline)
					Error("Line %d is incomplete.", g_nScriptLine);
				g_nScriptLine++;
			}
		}

		if (g_pszScript[0] == '/' && g_pszScript[1] == '/')	// comment field
		{
			if (!crossline)
				Error("Line %d is incomplete.", g_nScriptLine);

			while (*g_pszScript++ != '\n')
				if (!*g_pszScript)
				{
					if (!crossline)
						Error("Line %d is incomplete.", g_nScriptLine);
					*g_szToken = 0;
					return false;
				}
			skip = true;
			continue;
		}
	}

	// copy token
	token_p = g_szToken;

	if (*g_pszScript == '"')
	{
		g_pszScript++;
		while (*g_pszScript != '"')
		{
			if (!*g_pszScript)
				Error("EOF inside quoted token.");

			*token_p++ = *g_pszScript++;

			if (token_p == &g_szToken[MAXTOKEN])
				Error("Token too large on line %d.", g_nScriptLine);
		}
		g_pszScript++;
	}
	else 
		while (*g_pszScript > 32)
		{
			*token_p++ = *g_pszScript++;

			if (token_p == &g_szToken[MAXTOKEN])
				Error("Token too large on line %d.", g_nScriptLine);
		}

	*token_p = 0;
	
	return true;
}

/*
==================
UngetToken
==================
*/
void UngetToken ()
{
	g_bUnget = true;
}

/*
==============
TokenAvailable

Returns true if there is another token on the line
==============
*/
bool TokenAvailable ()
{
	const char *search_p;

	search_p = g_pszScript;

	while (*search_p <= 32)
	{
		if (*search_p == '\n')
			return false;
		if (*search_p == 0)
			return false;
		search_p++;
	}

	if (*search_p == ';')
		return false;

	return true;
}
