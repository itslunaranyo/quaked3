//==============================
//	transform.cpp
//==============================

#include "pre.h"
#include "qe3.h"
#include "select.h"
#include "transform.h"
#include "CmdClone.h"
#include "CmdCompound.h"
#include "CmdRotate.h"
#include "CmdScale.h"
#include <glm/gtc/matrix_transform.hpp>

/*
================
Transform_Move

non command based - only used by Clone right now
================
*/
void Transform_Move(const vec3 delta)
{
	mat4 mat = glm::translate(mat4(1), delta);

	for (Brush *b = g_brSelectedBrushes.next; b != &g_brSelectedBrushes; b = b->next)
	{
		if (b->owner->IsPoint())
			b->owner->Transform(mat);
		else
			b->Transform(mat, g_qeglobals.d_bTextureLock);
	}
}

/*
===============
Transform_FlipAxis
===============
*/
void Transform_FlipAxis(int axis)
{
	vec3 scx;
	CmdCompound *cmdCmpd;

	scx = vec3(1);
	scx[axis] = -1;

	if (g_cfgEditor.CloneStyle == CLONE_DRAG && GetKeyState(VK_SPACE) < 0)
	{
		cmdCmpd = new CmdCompound("Clone Mirror");
		CmdClone *cmdCl = new CmdClone(&g_brSelectedBrushes);
		Selection::DeselectAll();
		cmdCmpd->Complete(cmdCl);
	}

	CmdScale *cmdS = new CmdScale();
	cmdS->UseBrushes(&g_brSelectedBrushes);
	cmdS->TextureLock(g_qeglobals.d_bTextureLock);
	cmdS->Scale(Selection::GetMid(), scx);

	if (g_cfgEditor.CloneStyle == CLONE_DRAG && GetKeyState(VK_SPACE) < 0)
	{
		cmdCmpd->Complete(cmdS);
		g_cmdQueue.Complete(cmdCmpd);
	}
	else
	{
		g_cmdQueue.Complete(cmdS);
	}
}

/*
===============
Transform_RotateAxis
===============
*/
void Transform_RotateAxis(int axis, float deg, bool bMouse)
{
	vec3 rx;
	CmdCompound *cmdCmpd;

	while (deg >= 360)
		deg -= 360;
	while (deg < 0)
		deg += 360;

	if (deg == 0)
		return;

	rx[axis] = -1;

	if (g_cfgEditor.CloneStyle == CLONE_DRAG && GetKeyState(VK_SPACE) < 0)
	{
		cmdCmpd = new CmdCompound("Clone Rotate");
		CmdClone *cmdCl = new CmdClone(&g_brSelectedBrushes);
		Selection::DeselectAll();
		cmdCmpd->Complete(cmdCl);
	}

	CmdRotate *cmdR = new CmdRotate();
	cmdR->UseBrushes(&g_brSelectedBrushes);
	cmdR->TextureLock(g_qeglobals.d_bTextureLock);
	cmdR->Rotate(deg, Selection::GetMid(), rx);

	if (g_cfgEditor.CloneStyle == CLONE_DRAG && GetKeyState(VK_SPACE) < 0)
	{
		cmdCmpd->Complete(cmdR);
		g_cmdQueue.Complete(cmdCmpd);
	}
	else
	{
		g_cmdQueue.Complete(cmdR);
	}
}

/*
===========
Transform_Scale
===========
*/
void Transform_Scale(float x, float y, float z)
{
	if (x == 0 || y == 0 || z == 0)
		return;

	CmdScale *cmdS = new CmdScale();
	cmdS->UseBrushes(&g_brSelectedBrushes);
	cmdS->TextureLock(g_qeglobals.d_bTextureLock);
	cmdS->Scale(Selection::GetMid(), vec3(x, y, z));
	g_cmdQueue.Complete(cmdS);
}
