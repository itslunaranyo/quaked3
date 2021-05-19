//==============================
//	qe3defs.h
//==============================

#ifndef __QEDEFS_H__
#define __QEDEFS_H__

#define QE_VERSION  0x0301

#ifdef _DEBUG
#define QE3_WIN_REGISTRY "Software\\id\\QuakeEd3X"
#define QE3_WIN_REGISTRY_MRU "Software\\id\\QuakeEd3X\\MRU"
#else
#define QE3_WIN_REGISTRY "Software\\id\\QuakeEd3"
#define QE3_WIN_REGISTRY_MRU "Software\\id\\QuakeEd3\\MRU"
#endif

#define QE3_MAIN_STYLE (WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS |	WS_CLIPCHILDREN)
#define QE3_CHILD_STYLE (WS_CAPTION | WS_THICKFRAME | WS_MAXIMIZEBOX | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_CHILD)
#define QE3_REBAR_STYLE (WS_CHILD | WS_BORDER | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE | RBS_AUTOSIZE | RBS_BANDBORDERS | RBS_DBLCLKTOGGLE | RBS_TOOLTIPS)
#define QE3_TOOLBAR_STYLE (TBSTYLE_FLAT | TBSTYLE_TOOLTIPS | TBSTYLE_TRANSPARENT | WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE | CCS_NODIVIDER | CCS_NORESIZE /* | CCS_ADJUSTABLE | WS_BORDER*/)
#define QE3_TRACKBAR_STYLE (WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS | TBS_FIXEDLENGTH | TBS_TOOLTIPS)
#define QE3_STATUSBAR_STYLE (WS_CHILD | WS_BORDER | WS_VISIBLE | SBARS_SIZEGRIP)

#define	MAIN_WINDOW_CLASS		"QMAIN"
#define	CAMERA_WINDOW_CLASS		"QCAM"
#define	XYZ_WINDOW_CLASS		"QXYZ%d"
#define	XY_WINDOW_CLASS			"QXY"
#define	XZ_WINDOW_CLASS			"QXZ"	// sikk - Multiple Orthographic Views
#define	YZ_WINDOW_CLASS			"QYZ"	// sikk - Multiple Orthographic Views
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

#define QE_TIMER0   1
#define QE_TIMER1   2

#define	ON_EPSILON	0.01

#define	KEY_FORWARD		1
#define	KEY_BACK		2
#define	KEY_TURNLEFT	4
#define	KEY_TURNRIGHT	8
#define	KEY_LEFT		16
#define	KEY_RIGHT		32
#define	KEY_LOOKUP		64
#define	KEY_LOOKDOWN	128
#define	KEY_UP			256
#define	KEY_DOWN		512

// xy.c
#define	EXCLUDE_LIGHTS		1
#define	EXCLUDE_ENT			2
#define	EXCLUDE_PATHS		4
#define	EXCLUDE_WATER		8
#define	EXCLUDE_WORLD		16
#define	EXCLUDE_CLIP		32
#define	EXCLUDE_FUNC_WALL	64
#define	EXCLUDE_DETAIL		128
#define	EXCLUDE_SKY			256
#define	EXCLUDE_ANGLES		512
#define	EXCLUDE_HINT		1024

#define	ECLASS_LIGHT		0x00000001
#define	ECLASS_ANGLE		0x00000002
#define	ECLASS_PATH			0x00000004

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
#define	W_Z				0x0008
#define	W_TEXTURE		0x0010
#define W_CONSOLE		0x0040
#define W_ENTITY		0x0080

#define	W_ALL			0xFFFFFFFF

#define WM_BENCHMARK		(WM_USER + 267)

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
#define COLOR_MAPBOUNDRY	10
#define COLOR_SELBRUSHES	11
#define COLOR_TEXTUREBACK	12
#define COLOR_TEXTURETEXT	13
#define COLOR_VIEWNAME		14
#define COLOR_LAST			15

// win_ent.c ID's
#define ENT_CLASSLIST	0
#define ENT_COMMENT		1
#define ENT_CHECK1		2
#define ENT_CHECK2		3
#define ENT_CHECK3		4
#define ENT_CHECK4		5
#define ENT_CHECK5		6
#define ENT_CHECK6		7
#define ENT_CHECK7		8
#define ENT_CHECK8		9
#define ENT_CHECK9		10
#define ENT_CHECK10		11
#define ENT_CHECK11		12
#define ENT_CHECK12		13
#define ENT_PROPS		14
#define	ENT_DIR0		15
#define	ENT_DIR45		16
#define	ENT_DIR90		17
#define	ENT_DIR135		18
#define	ENT_DIR180		19
#define	ENT_DIR225		20
#define	ENT_DIR270		21
#define	ENT_DIR315		22
#define	ENT_DIRUP		23
#define	ENT_DIRDOWN		24
#define ENT_ADDPROP		25	// sikk - Entity Window Addition
#define ENT_DELPROP		26
#define ENT_CREATE		27	// sikk - Entity Window Addition
#define	ENT_KEYLABEL	28
#define	ENT_KEYFIELD	29
#define	ENT_VALUELABEL	30
#define	ENT_VALUEFIELD	31
#define ENT_COLOR		32
#define ENT_LAST		33

// used in some Drawing routines
enum VIEWTYPE {YZ, XZ, XY};
// XY = x0, y1; XZ = x0, y2; YZ = x1, y2.

enum entitymask_t
{
	// entity flags
	EFL_WORLDSPAWN	= 1,
	EFL_POINTENTITY	= 1 << 1,
	EFL_BRUSHENTITY	= 1 << 2,
	EFL_TRIGGER		= 1 << 3,
	EFL_LIGHT		= 1 << 4,
	EFL_MONSTER		= 1 << 5,
	EFL_FUNCWALL	= 1 << 6,
	EFL_FUNCDETAIL	= 1 << 7,
	EFL_X1			= 1 << 8,
	EFL_X2			= 1 << 9,
	EFL_X3			= 1 << 10,
	EFL_X4			= 1 << 11,
	EFL_CUSTOM1		= 1 << 12,
	EFL_CUSTOM2		= 1 << 13,
	EFL_CUSTOM3		= 1 << 14,
	EFL_CUSTOM4		= 1 << 15,

	// brush flags
	BFL_HIDDEN = 1 << 16,
	BFL_CLIP = 1 << 17,
	BFL_HINT = 1 << 18,
	BFL_SKIP = 1 << 19,
	BFL_LIQUID = 1 << 20,
	BFL_SKY = 1 << 21,
};

#endif
