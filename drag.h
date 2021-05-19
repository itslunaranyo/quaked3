#ifndef __DRAG_H__
#define __DRAG_H__
//==============================
//	drag.h
//==============================

bool Drag_TrySelect(int buttons, const vec3 origin, const vec3 dir);
void Drag_Setup (int x, int y, int buttons, const vec3 xaxis, const vec3 yaxis, const vec3 origin, const vec3 dir);
void Drag_Begin (int x, int y, int buttons, const vec3 xaxis, const vec3 yaxis, const vec3 origin, const vec3 dir);
void Drag_MouseMoved (int x, int y, int buttons);
void Drag_MouseUp ();

void AxializeVector (vec3 &v);
void MoveSelection (const vec3 move);
//void UpdateTarget (const vec3 origin, const vec3 dir);

#endif
