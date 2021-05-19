//==============================
//	cmdlib.h
//==============================

#ifndef __CMDLIB_H__
#define __CMDLIB_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>
#include "qeBuffer.h"

#ifndef __BYTEBOOL__
#define __BYTEBOOL__
//typedef enum {false, true} bool;
typedef unsigned char byte;
#endif

// the dec offsetof macro doesn't work very well...
#define myoffsetof (type, identifier) ((size_t) & ((type *)0)->identifier)
#define tmalloc(num, typ) (typ*)malloc((num) * sizeof(typ))

#define	MAX_NUM_ARGVS	32

//========================================================================

// set these before calling CheckParm
//extern int		myargc;	// sikk - unused
//extern char   **myargv;	// sikk - unused

extern int		g_nArgC;
extern char	   *g_pszArgV[MAX_NUM_ARGVS];

extern char		g_szComToken[1024];
extern bool		g_bComEOF;

//========================================================================

void   *qmalloc(int size);

char   *COM_Parse (char *data);

int		Q_strncasecmp (char *s1, char *s2, int n);
int		Q_strcasecmp (char *s1, char *s2);

int		IO_FileLength (FILE *f);

double	I_FloatTime (void);

void	Error (char *error, ...);
int		CheckParm (char *check);
void	ParseCommandLine (char *lpCmdLine);

FILE   *IO_SafeOpenWrite (char *filename);
FILE   *IO_SafeOpenRead (char *filename);
void	IO_SafeRead (FILE *f, void *buffer, int count);
void	IO_SafeWrite (FILE *f, void *buffer, int count);

int		IO_LoadFile(const char *filename, void **bufferptr);
int		IO_LoadFile(const char *filename, qeBuffer &fileBuf);
int		IO_LoadFileNoCrash (char *filename, void **bufferptr);
void	IO_SaveFile (char *filename, void *buffer, int count);

void 	DefaultExtension (char *path, char *extension);
void 	DefaultPath (char *path, char *basepath);
void 	StripFilename (char *path);
void 	StripExtension (char *path);

void 	ExtractFilePath (const char *path, char *dest);
void	ExtractFileName (const char *path, char *dest);
void 	ExtractFileBase (const char *path, char *dest);
void	ExtractFileExtension (const char *path, char *dest);

int		ParseHex (char *hex);
int 	ParseNum (char *str);

void	StringTolower (char *string);
void	StringToupper (char *string);

short	BigShort (short l);
short	LittleShort (short l);
int		BigLong (int l);
int		LittleLong (int l);
float	BigFloat (float l);
float	LittleFloat (float l);

// sikk---> Project Settings Dialog 
int		SetStr (char *dest, ...);
int		SetDirStr (char *dest, ...);
int		SetDirStr2 (char *dest, ...);
// <---sikk

#endif
