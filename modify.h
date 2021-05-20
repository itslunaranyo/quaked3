//==============================
//	modify.h
//==============================
#ifndef __MODIFY_H__
#define __MODIFY_H__

// miscellaneous operations, mostly wrappers around Cmd execution

namespace Modify  {

void	Delete();
void	Clone();

void	Ungroup();
void	InsertBrush();
void	HideSelected();
void	HideUnselected();
void	ShowHidden();

void	ConnectEntities();
void	SetKeyValue(const char *key, const char *value);
void	SetColor(const vec3 color);

void	MakeCzgCylinder(int degree);
void	MakeSided(int sides);
void	MakeSidedCone(int sides);	// sikk - Brush Primitves
void	MakeSidedSphere(int sides);	// sikk - Brush Primitves

}

#endif