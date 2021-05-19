//==============================
//	qfiles.h
//==============================

#ifndef __QFILES_H__
#define __QFILES_H__
// Quake file formats
// This file must be identical in the quake and utils directories

/*
==============================================================================

	Quake .BSP file format

==============================================================================
*/

// little-endian "IBSP"
#define IDBSPHEADER	(('P'<<24)+('S'<<16)+('B'<<8)+'I')

#define BSPVERSION	29
#define	TOOLVERSION	2

#define	MAX_MAP_MODELS			65536	//256	// sikk - bjp's tools max
#define	MAX_MAP_BRUSHES			65536	//4096	// sikk - bjp's tools max
#define	MAX_MAP_ENTITIES		65536	//1024	// sikk - bjp's tools max
#define	MAX_MAP_ENTSTRING		0x100000//65536	// sikk - bjp's tools max

#define	MAX_MAP_VERTS			65535
#define	MAX_MAP_FACES			65535
#define	MAX_MAP_EDGES			0x200000//256000	// sikk - bjp's tools max
#define	MAX_MAP_TEXINFO			32767	//4096
#define	MAX_MAP_TEXTURES		512

#define MAX_BRUSH_SIZE			8192	//4096 // quake default 

#define MAX_TEXNAME	16

// key / value pair sizes
#define	MAX_KEY		32
#define	MAX_VALUE	1024

//  .MIP file format
#define	TYP_MIPTEX	68
#define	MIPLEVELS	4


//=============================================================================

typedef struct miptex_s
{
	char		name[MAX_TEXNAME];
	unsigned	width, height;
	unsigned	offsets[MIPLEVELS];	// four mip maps stored
} miptex_t;


/*
==============================================================================

	Quake .WAD file format
	from wad.h

==============================================================================
*/

typedef struct
{
	char	identification[4];	// should be WAD2 or 2DAW
	int		numlumps;
	int		infotableofs;
} wadinfo_t;

typedef struct
{
	int		filepos;
	int		disksize;
	int		size;				// uncompressed
	char	type;
	char	compression;
	char	pad1, pad2;
	char	name[16];			// must be null terminated
} lumpinfo_t;

#endif
