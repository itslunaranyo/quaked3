//==============================
//	cmdlib.c
//==============================

#include "pre.h"
#include "qe3.h"

#define PATHSEPERATOR   '/'

char	g_szComToken[1024];
bool	g_bComEOF;

/*
==================
qmalloc
==================
*/
void *qmalloc(int size)
{
	void *b;

	b = malloc(size);
	memset(b, 0, size);

	return b;
}

/*
================
I_FloatTime
================
*/
double I_FloatTime (void)
{
	time_t t;
	
	time(&t);
	
	return (double)t;
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
	g_pszArgV[0] = "qe3.exe";

	while (*lpCmdLine && (g_nArgC < MAX_NUM_ARGVS))
	{
		while (*lpCmdLine && ((*lpCmdLine <= ' ') || (*lpCmdLine > '~')))
			lpCmdLine++;

		if (*lpCmdLine)
		{
			// lunaran: get quoted parm
			if (*lpCmdLine == '"')
			{
				g_pszArgV[g_nArgC++] = ++lpCmdLine;
				while (*lpCmdLine && *lpCmdLine != '"')
					lpCmdLine++;
			}
			else
			{
				g_pszArgV[g_nArgC++] = lpCmdLine;

				while (*lpCmdLine && ((*lpCmdLine > ' ') && (*lpCmdLine <= '~')))
					lpCmdLine++;
			}
			if (*lpCmdLine)
			{
				*lpCmdLine = 0;
				lpCmdLine++;
			}
		}
	}

	// lunaran: 0-init other parms
	int nargs = g_nArgC;
	while (g_nArgC < MAX_NUM_ARGVS)
	{
		g_pszArgV[g_nArgC] = 0;
		g_nArgC++;
	}
	g_nArgC = nargs;
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
IO_FileLength
================
*/
int IO_FileLength (FILE *f)
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
IO_SafeOpenWrite
==============
*/
FILE *IO_SafeOpenWrite (char *filename)
{
	FILE *f;

	f = fopen(filename, "wb");

	if (!f)
		Error(_S("Error opening %s: %s") << filename << strerror(errno));

	return f;
}

/*
==============
IO_SafeOpenRead
==============
*/
FILE *IO_SafeOpenRead (char *filename)
{
	FILE *f;

	f = fopen(filename, "rb");

	if (!f)
		Error(_S("Error opening %s: %s") << filename << strerror(errno));

	return f;
}

/*
==============
IO_SafeRead
==============
*/
void IO_SafeRead (FILE *f, void *buffer, int count)
{
	if ((int)fread(buffer, 1, count, f) != count)
		Error("File read failure.");
}

/*
==============
IO_SafeWrite
==============
*/
void IO_SafeWrite
(FILE *f, void *buffer, int count)
{
	if ((int)fwrite(buffer, 1, count, f) != count)
		Error("File read failure.");
}

/*
==============
IO_FileExists
==============
*/
bool IO_FileExists(const char *filename)
{
	// see if we're checking a wildcard path
	const char *src;
	src = filename + strlen(filename) - 1;
	while (*src != PATHSEPERATOR && src != filename)
	{
		if (*src == '.')
		{
			if (*--src == '*')
			{
				// we are, scan the directory instead
				char dir[1024];
				int len = src - filename;
				strncpy_s(dir, filename, len);
				dir[len] = 0;
				return IO_FileWithExtensionExists(dir, src + 2);
			}

			// a single file
			FILE *f = fopen(filename, "rb");
			if (!f)
				return false;
			fclose(f);
			return true;
		}
		src--;
	}
	
	return false;	// 'filename' was a directory with no file/extension
}

bool IO_FileExists(const char *dir, const char *filename)
{
	char fullpath[1024];

	sprintf(fullpath, "%s/%s\0", dir, filename);

	return IO_FileExists(fullpath);
}

bool IO_FileWithExtensionExists(const char* dir, const char* ext)
{
	_finddata_t fileinfo;
	int		handle;
	char	path[1024];

	sprintf(path, "%s/*.%s", dir, ext);

	handle = _findfirst(path, &fileinfo);
	return (handle != -1);
}

/*
==============
IO_LoadFile
==============
*/
int IO_LoadFile (const char *filename, void **bufferptr)
{
	int			 length;
	FILE		*f;
	void		*buffer;

	f = fopen(filename, "rb");
	if (!f)
	{
		*bufferptr = NULL;
		return -1;
	}
	length = IO_FileLength(f);
	buffer = qmalloc(length + 1);
	((char *)buffer)[length] = 0;
	IO_SafeRead(f, buffer, length);
	fclose(f);

	*bufferptr = buffer;
	return length;
}

/*
==============
IO_LoadFile
==============
*/
int IO_LoadFile(const char *filename, qeBuffer &fileBuf)
{
	FILE* f;
	int length = -1;

	if ((f = fopen(filename, "rb")) == NULL)
	{
		return -1;
	}

	length = IO_FileLength(f);
	fileBuf.resize(length + 1);
	((char*)*fileBuf)[length] = 0;
	IO_SafeRead(f, *fileBuf, min(length,(int)fileBuf.size()));	// truncate if buffer is too small
	fclose(f);

	return length;
}

/*
==============
IO_LoadFileNoCrash

returns -1 length if not present
==============
*/
int IO_LoadFileNoCrash (char *filename, void **bufferptr)
{
	int		length;
	void    *buffer;
	FILE	*f;

	f = fopen(filename, "rb");
	if (!f)
		return -1;
	length = IO_FileLength(f);
	buffer = qmalloc(length + 1);
	((char *)buffer)[length] = 0;
	IO_SafeRead(f, buffer, length);
	fclose(f);

	*bufferptr = buffer;
	return length;
}

/*
==============
IO_SaveFile
==============
*/
void IO_SaveFile (char *filename, void *buffer, int count)
{
	FILE *f;

	f = IO_SafeOpenWrite(filename);
	IO_SafeWrite(f, buffer, count);
	fclose(f);
}

/*
==============
IsPathAbsolute

TODO: this needs to be made portable one day
==============
*/
bool IsPathAbsolute(const char* path)
{
	if (path[1] == ':' && (path[2] == '/' || path[2] == '\\')) return true;
	if (!strncmp(path, "//", 2) || !strncmp(path, "\\\\", 2)) return true;
	return false;
}

/*
==============
IsPathDirectory
==============
*/
bool IsPathDirectory(char* path)
{
	char *src;
	src = path + strlen(path) - 1;

	while (*src != PATHSEPERATOR && src != path)
	{
		if (*src == '.')
			return false;                 // it has an extension
		src--;
	}

	return true;
}

/*
==============
DefaultExtension
==============
*/
void DefaultExtension(std::string& path, const char* extension)
{
	char* src;

	// if path doesn't have a .EXT, append extension
	// (extension should include the .)
	src = path.data() + path.length() - 1;

	while (*src != PATHSEPERATOR && src != path)
	{
		if (*src == '.')
			return;                 // it has an extension
		src--;
	}

	if (extension[0] != '.')
		path.append(".");
	path.append(extension);
}

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
		if (path[length] == PATHSEPERATOR)
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
void ExtractFilePath (const char *path, char *dest)
{
	const char *src;

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
void ExtractFileName (const char *path, char *dest)
{
	const char *src;

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
void ExtractFileBase (const char *path, char *dest)
{
	const char *src;

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
void ExtractFileExtension (const char *path, char *dest)
{
	const char *src;
	
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
			Error(_S("Bad hex number: %s") << hex);
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
==============
VecToString
==============
*/
void VecToString(const vec3 vec, char *string)
{
	char szVal[3][64];
//	char *pos;

	for (int i = 0; i < 3; i++)
		FloatToString(vec[i], szVal[i]);

	sprintf(string, "%s %s %s", szVal[0], szVal[1], szVal[2]);
}

/*
==============
VecToString

lunaran: trim trailing zeroes, including the . if necessary, for no other reason
than I think they don't look very nice
==============
*/
void FloatToString(const float f, char *string, int dec)
{
	char fmt[8];
	char *szVal;
	char *pos;

	sprintf(fmt, "%%0.%if", dec);

	// trunc safety
	float fp = f + ((f < 0) ? -0.000001f : 0.000001f);
	szVal = string;
	sprintf(szVal, fmt, f);
	pos = &szVal[strlen(szVal) - 1];
	while (*pos == '0' && pos != szVal)
	{
		*pos-- = 0;
	}
	if (*pos == '.')
		*pos = 0;
//	else if (*pos == 'e')
//	{
		// we got a scientific notation version of a very tiny float, mark it zero
//		string[0] = '0';
//		string[1] = 0;
//	}
}

/*
==================================================================

	PROJECT SETTINGS DIALOG

	*ripped from QE5
==================================================================
*/

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
	s = va_arg(list, char *);
	while (s)
	{
		while (*s)
		{
			*dest++ = *s++;
			len++;
		}
		s = va_arg(list, char *);
	}
	*dest = 0;
	va_end(list);
	return len + 1;
}

/*
==============
Path_Convert

copy up to 4 strings into one while stripping 
out multiple consecutive path separators.
also convert slashes
==============
*/
int Path_Convert(char* dest, const char sep, const char *src1, const char *src2, const char *src3, const char *src4)
{
	int len = 0;
	const char* s;
	char last = 0;
	int arg = 0;

	s = src1;
	while (s)
	{
		while (*s)
		{
			if (*s == '/' || *s == '\\')
			{
				if (last != sep)
				{
					len++;
					last = *dest++ = sep;
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
		switch (arg++) {
			case 0: s = src2; break;
			case 1: s = src3; break;
			case 2: s = src4; break;
			default: s = nullptr;
		}
	}
	*dest = 0;
	return len + 1;
}
