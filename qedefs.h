//==============================
//	qe3defs.h
//==============================

#ifndef __QEDEFS_H__
#define __QEDEFS_H__

#define QE_VERSION_MAJOR	3
#define	QE_VERSION_MINOR	2
#define QE_VERSION_BUILD	54

#ifdef _DEBUG
#define QE3_WIN_REGISTRY "Software\\id\\QuakeEd3X"
#define QE3_WIN_REGISTRY_MRU "Software\\id\\QuakeEd3X\\MRU"
#else
#define QE3_WIN_REGISTRY "Software\\id\\QuakeEd3"
#define QE3_WIN_REGISTRY_MRU "Software\\id\\QuakeEd3\\MRU"
#endif

extern std::string g_qeAppName;
extern std::string g_qePath;

#define QE3_MAIN_STYLE (WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS |	WS_CLIPCHILDREN)
#define QE3_CHILD_STYLE (WS_CAPTION | WS_THICKFRAME | WS_MAXIMIZEBOX | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_CHILD)
#define QE3_REBAR_STYLE (WS_CHILD | WS_BORDER | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE | RBS_AUTOSIZE | RBS_BANDBORDERS | RBS_DBLCLKTOGGLE | RBS_TOOLTIPS)
#define QE3_TOOLBAR_STYLE (TBSTYLE_FLAT | TBSTYLE_TOOLTIPS | TBSTYLE_TRANSPARENT | WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE | CCS_NODIVIDER | CCS_NORESIZE /* | CCS_ADJUSTABLE | WS_BORDER*/)
#define QE3_TRACKBAR_STYLE (WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS | TBS_FIXEDLENGTH | TBS_TOOLTIPS)
#define QE3_STATUSBAR_STYLE (WS_CHILD | WS_BORDER | WS_VISIBLE | SBARS_SIZEGRIP)

#define	MAIN_WINDOW_CLASS		"QMAIN"
#define BASE_WINDOW_CLASS		"QBASEWND"
#define VIEW_WINDOW_CLASS		"QVIEWWND"
#define	CAMERA_WINDOW_CLASS		"QCAM"
#define	XYZ_WINDOW_CLASS		"QXYZ%d"
#define	Z_WINDOW_CLASS   		"QZ"
#define	INSPECTOR_WINDOW_CLASS	"QINSP"
#define	CONSOLE_WINDOW_CLASS	"QCONS"
#define	ENTITY_WINDOW_CLASS		"QENT"
#define	TEXTURE_WINDOW_CLASS	"QTEX"

#define	ZWIN_WIDTH		48
#define CWIN_SIZE		(0.4)

#define	MAX_EDGES		256
#define	MAX_POINTS		512

#define	CMD_TEXTUREWAD	60000
#define	CMD_BSPCOMMAND	61000

#define	PITCH		0	// up / down
#define	YAW			1	// left / right
#define	ROLL		2	// fall over

#define QE_TIMERAUTOSAVE	1
#define QE_TIMERSPLASH		2

#define	ON_EPSILON	0.01

// menu indexes for modifying menus
#define	MENU_VIEW		2
#define	MENU_TEXTURE	6
#define	MENU_BSP		8

// odd key ID's not in windows header...
#define	VK_PLUS			187
#define	VK_MINUS		189
#define	VK_COMMA		188
#define	VK_PERIOD		190
#define	VK_BACKSLASH	220

// window bits
#define	W_CAMERA		0x0001
#define	W_XY			0x0002
#define	W_Z				0x0004
#define W_SCENE			0x0007
#define	W_TEXTURE		0x0010
#define W_CONSOLE		0x0020
#define W_ENTITY		0x0040
#define W_SURF			0x0080
#define W_TITLE			0x0100
#define W_TARGETGRAPH	0x0200
#define	W_ALL			0xFFFF

#define WM_MOUSEOVER	(WM_USER + 265)
#define WM_REALTIME		(WM_USER + 266)
#define WM_BENCHMARK	(WM_USER + 267)

// toolbar ID's
#define ID_REBAR		1000
#define ID_TOOLBAR		2000
#define ID_TRACKBAR		3000
#define ID_STATUSBAR	4000

// color ID's
#define	COLOR_BRUSHES		0
#define	COLOR_CAMERABACK	1
#define	COLOR_CAMERAGRID	2
#define	COLOR_CLIPPER		3
#define	COLOR_ENTITY		4
#define	COLOR_GRIDBACK		5
#define COLOR_GRIDBLOCK		6
#define COLOR_GRIDMAJOR		7
#define COLOR_GRIDMINOR		8
#define COLOR_GRIDTEXT		9
#define COLOR_MAPBOUNDARY	10
#define COLOR_SELBRUSHES	11
#define COLOR_TEXTUREBACK	12
#define COLOR_TEXTURETEXT	13
#define COLOR_VIEWNAME		14
#define COLOR_LAST			15


#define CLONE_OFFSET	0
#define CLONE_INPLACE	1
#define CLONE_DRAG		2

#define CAMERA_CLASSIC	0
#define CAMERA_WASD		1

#define TEX_PROJECT_AXIAL	0
#define TEX_PROJECT_FACE	1

// used in some Drawing routines
enum eViewType_t { GRID_YZ = 0, GRID_XZ = 1, GRID_XY = 2 };
// XY = x0, y1; XZ = x0, y2; YZ = x1, y2.

enum texModType_t {
	TM_NOTSET,	// not chosen yet
	TM_SHIFT,
	TM_SCALE,
	TM_ROTATE,
};

enum drawMode_t
{
	CD_WIRE,
	CD_FLAT,
	CD_TEXTURED
};

enum surfIgnoreFlags_t
{
	SFI_NAME = 0x01,
	SFI_SHIFTX = 0x02,
	SFI_SHIFTY = 0x04,
	SFI_SCALEX = 0x08,
	SFI_SCALEY = 0x10,
	SFI_ROTATE = 0x20,

	SFI_ALL = 0x3F
};

enum entitymask_t
{
	// brush flags
	BFL_HIDDEN	= 1,
	BFL_CLIP	= 1 << 1,
	BFL_HINT	= 1 << 2,
	BFL_SKIP	= 1 << 3,
	BFL_LIQUID	= 1 << 4,
	BFL_SKY		= 1 << 5,
	BFL_TRANS	= 1 << 6,

	// entity flags
	EFL_WORLDSPAWN	= 1 << 7,
	EFL_EASY		= 1 << 8,	// < these 4 filter flags match the spawnflags that they filter
	EFL_MEDIUM		= 1 << 9,	// <
	EFL_HARD		= 1 << 10,	// <
	EFL_DEATHMATCH	= 1 << 11,	// <
	EFL_POINTENTITY	= 1 << 12,
	EFL_BRUSHENTITY	= 1 << 13,
	EFL_LIGHT		= 1 << 14,
	EFL_TRIGGER		= 1 << 15,
	EFL_MONSTER		= 1 << 16,
	EFL_FUNCWALL	= 1 << 17,
	EFL_DETAIL		= 1 << 18,	// func_detail*, including ericw-tools wall/illusionary detail
	EFL_PATH		= 1 << 19,
	EFL_CUSTOM1		= 1 << 20,
	EFL_CUSTOM2		= 1 << 21,
	EFL_CUSTOM3		= 1 << 22,
	EFL_CUSTOM4		= 1 << 23,
};

#endif
