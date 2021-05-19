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

// general edits
#include "CmdClone.h"
#include "CmdDelete.h"

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

//#include "Cmd.h"

extern CommandQueue g_cmdQueue;


#endif