//==============================
//	parse.h
//==============================

// text file parsing routines

#define	MAXTOKEN	1024

//========================================================================

extern char	g_szToken[MAXTOKEN];
extern int	g_nScriptLine;

//========================================================================

void StartTokenParsing (char *data);
bool GetToken (bool crossline);
void UngetToken ();
bool TokenAvailable ();