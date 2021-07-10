//==============================
//	qe3.h
//==============================

#ifndef __QE3_H__
#define __QE3_H__

// disable data conversion warnings for gl
#pragma warning(disable : 4244)     // MIPS
#pragma warning(disable : 4136)     // X86
#pragma warning(disable : 4051)     // ALPHA
#pragma warning(1 : 4706)	// assignment in conditional: never not a typo

#include "resource.h"

#include "qeBuffer.h"	// lunaran - generic heap space; malloc as an object
#include "mathlib.h"
#include "cmdlib.h"
#include "syslib.h"
//#include "errors.h"

#include "qedefs.h"
#include "qfiles.h"
#include "config.h"

#include "WndMain.h"

#include "textures.h"
#include "texdef.h"
#include "plane.h"
#include "face.h"
#include "brush.h"
#include "entclass.h"
#include "entity.h"


//========================================================================

// most of the QE globals are stored in this structure
// lunaran: shrank this drastically
typedef struct
{
	bool		d_bShowGrid;
	int			d_nGridSize;
	bool		bGridSnap;

	vec3		d_v3WorkMin, 			// defines the boundaries of the current work area, used to guess
				d_v3WorkMax;			// brushes and drop points third coordinate when creating from 2D view
	TexDef		d_workTexDef;			// lunaran: moved out of texturewin_t

	HINSTANCE	d_hInstance;

	int			d_nPointfileDisplayList;

	bool		d_bTextureLock;
	float		d_fDefaultTexScale;		// sikk - Default Texture Scale Dialog
} qeglobals_t;

//========================================================================

/*
** extern declarations
*/
extern qeglobals_t g_qeglobals;
extern double	g_deltaTime;

extern char		g_qeAppName[64];
extern char		g_qePath[MAX_PATH];

extern HWND		g_hwndMain;

/*
** global declarations
*/
extern bool	g_bSnapCheck;

//========================================================================

// QE function declarations
void	QE_TestSomething();

void	QE_CheckAutoSave();
void	QE_CheckOpenGLForErrors(void);
void	QE_OpenGLError(int errornum, const char *errorstr);
void	QE_Init();
bool	QE_KeyDown(int key);
bool	QE_KeyUp(int key);
void	QE_SaveMap();
bool	QE_InitProject();
int		QE_BestViewAxis();
bool	QE_SingleBrush();
void	QE_UpdateWorkzone(Brush *b);
char   *QE_ExpandProjectPath (char *p);

vec3	pointOnGrid(const vec3 point);
vec3	AxisForVector(const vec3 &v);		// TODO: mathlib
vec3	AxializeVector(const vec3 &v);

// WndConfig.c
void DoConfigWindow();
void DoConfigWindowProject();
bool SelectDir(HWND h, bool format, char* title);


#endif
