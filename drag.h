#ifndef __DRAG_H__
#define __DRAG_H__
//==============================
//	drag.h
//==============================

void Drag_Setup (int x, int y, int buttons, vec3_t xaxis, vec3_t yaxis, vec3_t origin, vec3_t dir);
void Drag_Begin (int x, int y, int buttons, vec3_t xaxis, vec3_t yaxis, vec3_t origin, vec3_t dir);
void Drag_MouseMoved (int x, int y, int buttons);
void Drag_MouseUp ();

void AxializeVector (vec3_t v);
void MoveSelection (vec3_t move);
void UpdateTarget (vec3_t origin, vec3_t dir);

#endif
