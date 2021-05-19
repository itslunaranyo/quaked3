//==============================
//	transform.h
//==============================
#ifndef __TRANSFORM_H__
#define __TRANSFORM_H__

void	Transform_Move(const vec3 delta);
void	Transform_ApplyMatrix();
void	Transform_FlipAxis(int axis);
void	Transform_RotateAxis(int axis, float deg, bool bMouse);  // sikk - Free Rotate: bool bMouse argument added
void	Transform_Scale(float x, float y, float z);	// sikk - Brush Scaling

#endif