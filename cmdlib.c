//==============================
//	cmdlib.c
//==============================

#include "cmdlib.h"


#define PATHSEPERATOR   '/'

char	g_szComToken[1024];
bool	g_bComEOF;

/*
================
I_FloatTime
================
*/
double I_FloatTime (void)
{
	time_t t;
	
	time(&t);
	
	return t;
#if 0
	// more precise, less portable
	struct timeval	tp;
	struct timezone tzp;
	static int		secbase;

	gettimeofday(&tp, &tzp);
	
	if (!secbase)
	{
		secbase = tp.tv_sec;
		return tp.tv_usec / 1000000.0;
	}
	
	return (tp.tv_sec - secbase) + tp.tv_usec / 1000000.0;
#endif
}

/*
==============
COM_Parse

Parse a token out of a string
==============
*/
char *COM_Parse (char *data)
{
	int		c;
	int		len;
	
	len = 0;
	g_szComToken[0] = 0;
	
	if (!data)
		return NULL;
		
	// skip whitespace
skipwhite:
	while ((c = *data) <= ' ')
	{
		if (c == 0)
		{
			g_bComEOF = true;
			return NULL;			// end of file;
		}
		data++;
	}
	
	// skip // comments
	if (c == '/' && data[1] == '/')
	{
		while (*data && *data != '\n')
			data++;
		goto skipwhite;
	}
	
	// handle quoted strings specially
	if (c == '\"')
	{
		data++;
		do
		{
			c = *data++;
			if (c == '\"')
			{
				g_szComToken[len] = 0;
				return data;
			}
			g_szComToken[len] = c;
			len++;
		}while (1);
	}

	// parse single characters
	if (c == '{' || c == '}'|| c == ')'|| c == '(' || c == '\'' || c == ':')
	{
		g_szComToken[len] = c;
		len++;
		g_szComToken[len] = 0;
		return data + 1;
	}

	// parse a regular word
	do
	{
		g_szComToken[len] = c;
		data++;
		len++;
		c = *data;
		if (c == '{' || c == '}'|| c ==')'|| c == '(' || c == '\'' || c == ':')
			break;
	}while (c > 32);
	
	g_szComToken[len] = 0;
	return data;
}

/*
==============
Q_strncasecmp
==============
*/
int Q_strncasecmp (char *s1, char *s2, int n)
{
	int	c1, c2;
	
	while (1)
	{
		c1 = *s1++;
		c2 = *s2++;

		if (!n--)
			return 0;		// strings are equal until end point
		
		if (c1 != c2)
		{
			if (c1 >= 'a' && c1 <= 'z')
				c1 -= ('a' - 'A');
			if (c2 >= 'a' && c2 <= 'z')
				c2 -= ('a' - 'A');
			if (c1 != c2)
				return -1;	// strings not equal
		}
		if (!c1)
			return 0;		// strings are equal
	}
	
	return -1;
}

/*
==============
Q_strcasecmp
==============
*/
int Q_strcasecmp (char *s1, char *s2)
{
	return Q_strncasecmp (s1, s2, 99999);
}

/*
=============================================================================

						MISC FUNCTIONS

=============================================================================
*/

int		g_nArgC;
char   *g_pszArgV[MAX_NUM_ARGVS];

/*
================
ParseCommandLine
================
*/
void ParseCommandLine (char *lpCmdLine)
{
	g_nArgC = 1;
	g_pszArgV[0] = "programname";

	while (*lpCmdLine && (g_nArgC < MAX_NUM_ARGVS))
	{
		while (*lpCmdLine && ((*lpCmdLine <= 32) || (*lpCmdLine > 126)))
			lpCmdLine++;

		if (*lpCmdLine)
		{
			g_pszArgV[g_nArgC] = lpCmdLine;
			g_nArgC++;

			while (*lpCmdLine && ((*lpCmdLine > 32) && (*lpCmdLine <= 126)))
				lpCmdLine++;

			if (*lpCmdLine)
			{
				*lpCmdLine = 0;
				lpCmdLine++;
			}
		}
	}
}

/*
=================
CheckParm

Checks for the given parameter in the program's command line arguments
Returns the argument number (1 to argc-1) or 0 if not present
=================
*/
int CheckParm (char *check)
{
	int	i;

	for (i = 1; i < g_nArgC; i++)
		if (!Q_strcasecmp(check, g_pszArgV[i]))
			return i;

	return 0;
}

/*
================
Q_filelength
================
*/
int Q_filelength (FILE *f)
{
	int		pos, end;

	pos = ftell(f);
	fseek(f, 0, SEEK_END);
	end = ftell(f);
	fseek(f, pos, SEEK_SET);

	return end;
}

/*
==============
SafeOpenWrite
==============
*/
FILE *SafeOpenWrite (char *filename)
{
	FILE *f;

	f = fopen(filename, "wb");

	if (!f)
		Error("Error opening %s: %s", filename, strerror(errno));

	return f;
}

/*
==============
SafeOpenRead
==============
*/
FILE *SafeOpenRead (char *filename)
{
	FILE *f;

	f = fopen(filename, "rb");

	if (!f)
		Error("Error opening %s: %s", filename, strerror(errno));

	return f;
}

/*
==============
SafeRead
==============
*/
void SafeRead (FILE *f, void *buffer, int count)
{
	if ((int)fread(buffer, 1, count, f) != count)
		Error("File read failure.");
}

/*
==============
SafeWrite
==============
*/
void SafeWrite (FILE *f, void *buffer, int count)
{
	if ((int)fwrite(buffer, 1, count, f) != count)
		Error("File read failure.");
}

/*
==============
LoadFile
==============
*/
int LoadFile (char *filename, void **bufferptr)
{
	int			 length;
	FILE		*f;
	void		*buffer;
	extern void *qmalloc(size_t size);

	f = fopen(filename, "rb");
	if (!f)
	{
		*bufferptr = NULL;
		return -1;
	}
	length = Q_filelength(f);
	buffer = qmalloc(length + 1);
	((char *)buffer)[length] = 0;
	SafeRead(f, buffer, length);
	fclose(f);

	*bufferptr = buffer;
	return length;
}

/*
==============
LoadFileNoCrash

returns -1 length if not present
==============
*/
int LoadFileNoCrash (char *filename, void **bufferptr)
{
	int		length;
	void    *buffer;
	FILE	*f;

	f = fopen(filename, "rb");
	if (!f)
		return -1;
	length = Q_filelength(f);
	buffer = qmalloc(length + 1);
	((char *)buffer)[length] = 0;
	SafeRead(f, buffer, length);
	fclose(f);

	*bufferptr = buffer;
	return length;
}

/*
==============
SaveFile
==============
*/
void SaveFile (char *filename, void *buffer, int count)
{
	FILE *f;

	f = SafeOpenWrite(filename);
	SafeWrite(f, buffer, count);
	fclose(f);
}

/*
==============
DefaultExtension
==============
*/
void DefaultExtension (char *path, char *extension)
{
	char *src;

	// if path doesn't have a .EXT, append extension
	// (extension should include the .)
	src = path + strlen(path) - 1;

	while (*src != PATHSEPERATOR && src != path)
	{
		if (*src == '.')
			return;                 // it has an extension
		src--;
	}

	strcat(path, extension);
}

/*
==============
DefaultPath
==============
*/
void DefaultPath (char *path, char *basepath)
{
	char temp[128];

	if (path[0] == PATHSEPERATOR)
		return;                   // absolute path location
	strcpy(temp, path);
	strcpy(path, basepath);
	strcat(path, temp);
}

/*
==============
StripFilename
==============
*/
void StripFilename (char *path)
{
	int	length;

	length = strlen(path) - 1;
	while (length > 0 && path[length] != PATHSEPERATOR)
		length--;
	path[length] = 0;
}

/*
==============
StripExtension
==============
*/
void StripExtension (char *path)
{
	int	length;

	length = strlen(path) - 1;
	while (length > 0 && path[length] != '.')
	{
		length--;
		if (path[length] == '/')
			return;		// no extension
	}
	if (length)
		path[length] = 0;
}

/*
====================
ExtractFilePath
====================
*/
void ExtractFilePath (char *path, char *dest)
{
	char *src;

	src = path + strlen(path) - 1;

	// back up until a \ or the start
	while (src != path && *(src - 1) != PATHSEPERATOR)
		src--;

	memcpy(dest, path, src - path);
	dest[src - path] = 0;
}

/*
===============
ExtractFileName
===============
*/
void ExtractFileName (char *path, char *dest)
{
	char *src;

	src = path + strlen(path) - 1;

	// back up until a \ or the start
	while ((src != path) && (*(src - 1) != '/') && (*(src - 1) != '\\'))
		src--;

	while (*src)
		*dest++ = *src++;

	*dest = 0;
}

/*
===============
ExtractFileBase
===============
*/
void ExtractFileBase (char *path, char *dest)
{
	char *src;

	src = path + strlen(path) - 1;

	// back up until a \ or the start
	while (src != path && *(src - 1) != '/' && *(src - 1) != '\\' )
		src--;

	while (*src && *src != '.')
		*dest++ = *src++;

	*dest = 0;
}

/*
====================
ExtractFileExtension
====================
*/
void ExtractFileExtension (char *path, char *dest)
{
	char *src;

	src = path + strlen(path) - 1;

	// back up until a . or the start
	while (src != path && *(src - 1) != '.')
		src--;

	if (src == path)
	{
		*dest = 0;	// no extension
		return;
	}

	strcpy(dest, src);
}

/*
==============
ParseHex
==============
*/
int ParseHex (char *hex)
{
	int		num;
	char   *str;

	num = 0;
	str = hex;

	while (*str)
	{
		num <<= 4;
		if (*str >= '0' && *str <= '9')
			num += *str - '0';
		else if (*str >= 'a' && *str <= 'f')
			num += 10 + *str -'a';
		else if (*str >= 'A' && *str <= 'F')
			num += 10 + *str -'A';
		else
			Error("Bad hex number: %s",hex);
		str++;
	}

	return num;
}

/*
==============
ParseNum
==============
*/
int ParseNum (char *str)
{
	if (str[0] == '$')
		return ParseHex(str + 1);
	if (str[0] == '0' && str[1] == 'x')
		return ParseHex(str + 2);
	return atol(str);
}

/*
==============
StringTolower
==============
*/
void StringTolower (char *string)
{
	char *in;

	in = string;
	while (*in)
	{
		*in = tolower(*in);
		in++;
	}
} 

/*
==============
StringToupper
==============
*/
void StringToupper (char *string)
{
	char *in;

	in = string;
	while (*in)
	{
		*in = toupper(*in);
		in++;
	}
} 


/*
============================================================================

					BYTE ORDER FUNCTIONS

============================================================================
*/

#ifdef _SGI_SOURCE
#define	__BIG_ENDIAN__
#endif

#ifdef __BIG_ENDIAN__

/*
==============
LittleShort
==============
*/
short LittleShort (short l)
{
	byte	b1, b2;

	b1 = l & 255;
	b2 = (l >> 8) & 255;

	return (b1 << 8) + b2;
}

/*
==============
BigShort
==============
*/
short BigShort (short l)
{
	return l;
}

/*
==============
LittleLong
==============
*/
int LittleLong (int l)
{
	byte	b1, b2, b3, b4;

	b1 = l & 255;
	b2 = (l >> 8) & 255;
	b3 = (l >> 16) & 255;
	b4 = (l >> 24) & 255;

	return ((int)b1 << 24) + ((int)b2 << 16) + ((int)b3 << 8) + b4;
}

/*
==============
BigLong
==============
*/
int BigLong (int l)
{
	return l;
}

/*
==============
LittleFloat
==============
*/
float LittleFloat (float l)
{
	union {byte b[4]; float f;} in, out;
	
	in.f = l;
	out.b[0] = in.b[3];
	out.b[1] = in.b[2];
	out.b[2] = in.b[1];
	out.b[3] = in.b[0];
	
	return out.f;
}

/*
==============
BigFloat
==============
*/
float BigFloat (float l)
{
	return l;
}

#else

/*
==============
BigShort
==============
*/
short BigShort (short l)
{
	byte    b1, b2;

	b1 = l & 255;
	b2 = (l >> 8) & 255;

	return (b1 << 8) + b2;
}

/*
==============
LittleShort
==============
*/
short LittleShort (short l)
{
	return l;
}

/*
==============
BigLong
==============
*/
int BigLong (int l)
{
	byte    b1, b2, b3, b4;

	b1 = l & 255;
	b2 = (l >> 8) & 255;
	b3 = (l >> 16) & 255;
	b4 = (l >> 24) & 255;

	return ((int)b1 << 24) + ((int)b2 << 16) + ((int)b3 << 8) + b4;
}

/*
==============
LittleLong
==============
*/
int LittleLong (int l)
{
	return l;
}

/*
==============
BigFloat
==============
*/
float BigFloat (float l)
{
	union {byte b[4]; float f;} in, out;
	
	in.f = l;
	out.b[0] = in.b[3];
	out.b[1] = in.b[2];
	out.b[2] = in.b[1];
	out.b[3] = in.b[0];
	
	return out.f;
}

/*
==============
LittleFloat
==============
*/
float LittleFloat (float l)
{
	return l;
}

#endif

// sikk---> Project Settings Dialog
/*
==================================================================

	PROJECT SETTINGS DIALOG

	*ripped from QE5
==================================================================
*/

#if 0

/*
==============
SetStr
==============
*/
int SetStr (char *dest, char *s1, char *s2, char *s3, char *s4)
{
	int		i, i2, len;
	char   *s, *sa[5] = {s1, s2, s3, s4, NULL};

	for (i = len = 0; (s = sa[i]); i++)
	{
		for (i2 = 0; *s; i2++)
		{
			*dest++ = *s++;
			len++;
		}
	}
	*dest = 0;
	return len + 1;
}

#else

/*
==============
SetStr
==============
*/
int SetStr (char *dest, ...)
{
	int		len = 0;
	char   *s;
	va_list	list;

	va_start(list, dest);

	while (s = va_arg(list, char *))
	{
		while (*s)
		{
			*dest++ = *s++;
			len++;
		}
	}
	*dest = 0;
	va_end(list);
	return len + 1;
}

#endif

/*
==============
SetDirStr

copy up to 4 strings into one while stripping 
out multiple consecutive path separators.
also convert "/" to "\\"
==============
*/
int SetDirStr (char *dest, ...)
{
	int		len = 0;
	char   *s, last = 0;
	va_list	list;

	va_start(list, dest);

	while (s = va_arg(list, char *))
	{
		while (*s)
		{
			if (*s == '/' || *s == '\\')
			{
				if (last != '\\')
				{
					len++;
					last = *dest++ = '\\';
				}

				while (*s == '/' || *s == '\\') // skip consecutive path separators
					s++;
			}
			else
			{
				len++;
				last = *dest++ = *s++;
			}
		}
	}
	*dest = 0;
	va_end(list);
	return len + 1;
}

/*
==============
SetDirStr2
==============
*/
int SetDirStr2 (char *dest, ...)
{
	int		len = 0;
	char   *s, last = 0;
	va_list	list;

	va_start(list, dest);

	while (s = va_arg(list, char *))
	{
		while (*s)
		{
			if (*s == '/' || *s == '\\')
			{
				if (last != '/')
				{
					len++;
					last = *dest++ = '/';
				}

				while (*s == '/' || *s == '\\') // skip consecutive path separators
					s++;
			}
			else
			{
				len++;
				last = *dest++ = *s++;
			}
		}
	}
	*dest = 0;
	va_end(list);
	return len + 1;
}
// <---sikk