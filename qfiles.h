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
#define MAX_WADNAME	32

// key / value pair sizes
#define	MAX_KEY		32
#define	MAX_VALUE	1024

#endif
