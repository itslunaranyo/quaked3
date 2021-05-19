//==============================
//	modify.h
//==============================
#ifndef __MODIFY_H__
#define __MODIFY_H__

// miscellaneous operations, mostly wrappers around Cmd execution

void	Modify_Delete();
void	Modify_Clone();

void	Modify_Ungroup();
void	Modify_InsertBrush();
void	Modify_HideSelected();
void	Modify_HideUnselected();
void	Modify_ShowHidden();

void	Modify_ConnectEntities();
void	Modify_SetKeyValue(const char *key, const char *value);
void	Modify_SetColor(const vec3 color);

void	Modify_MakeCzgCylinder(int degree);
void	Modify_MakeSided(int sides);
void	Modify_MakeSidedCone(int sides);	// sikk - Brush Primitves
void	Modify_MakeSidedSphere(int sides);	// sikk - Brush Primitves


#endif