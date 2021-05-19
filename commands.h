//==============================
//	commands.h
//==============================
#ifndef __COMMANDS_H__
#define __COMMANDS_H__

// base commands
#include "Command.h"
#include "CmdAddRemove.h"
#include "CmdReparentBrush.h"
#include "CmdBrushMod.h"

// scene
#include "CmdPaste.h"
#include "CmdImportMap.h"

// general edits
#include "CmdClone.h"
#include "CmdDelete.h"
#include "CmdTranslate.h"
#include "CmdRotate.h"
#include "CmdScale.h"

// entity edits
#include "CmdCreatePointEntity.h"
#include "CmdCreateBrushEntity.h"
#include "CmdSetKeyvalue.h"
#include "CmdSetSpawnflag.h"

// brush edits
#include "CmdBrushClip.h"
#include "CmdCylinder.h"
#include "CmdCzgCylinder.h"
#include "CmdCone.h"
#include "CmdSphere.h"
#include "CmdHollow.h"
#include "CmdMerge.h"

//#include "Cmd.h"

extern CommandQueue g_cmdQueue;


#endif