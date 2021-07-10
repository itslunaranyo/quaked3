#ifndef __PRECOMP_H__
#define __PRECOMP_H__

#define WINVER 0x0501
#define _WIN32_WINNT_WINXP 0x0501 // Windows XP  

#include <windows.h>
#include <afxres.h>
#include <commctrl.h>

#include <cassert>
#include <string>
#include <vector>

// REMOVE ONE BY ONE:
//#define GLEW_STATIC 1
#include <GL\glew.h>
#include <gl\gl.h>
#include <stdlib.h>
#include <utility>
#include <io.h>

#include "Log.h"
#include "Exceptions.h"

typedef unsigned char byte;
extern HWND g_hwndMain;

#endif