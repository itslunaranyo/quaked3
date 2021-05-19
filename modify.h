//==============================
//	modify.h
//==============================
#ifndef __MODIFY_H__
#define __MODIFY_H__

void	Modify_Delete();
void	Modify_Clone();

void	Modify_Ungroup();
void	Modify_InsertBrush();
void	Modify_Hide();
void	Modify_ShowHidden();

void	Modify_ConnectEntities();
void	Modify_SetKeyValue(const char *key, const char *value);
void	Modify_SetColor(const vec3 color);

#endif