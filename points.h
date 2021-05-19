#ifndef __POINTS_H__
#define __POINTS_H__
//==============================
//	points.h
//==============================

#define	MAX_POINTFILE	8192	// eerie - changed from 4096

void Pointfile_Delete ();
void Pointfile_Next ();
void Pointfile_Prev ();
bool Pointfile_Check ();	// sikk - return value used for Test Map after BSP
void Pointfile_Draw ();
void Pointfile_Clear ();
//void Pointfile_Load ();	// unused

#endif
