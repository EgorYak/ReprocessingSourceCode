/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
// mathlib.h

#pragma once

#include <cmath>

typedef float vec_t;

#include "vector.h"

typedef vec_t vec4_t[4]; // x,y,z,w
typedef vec_t vec5_t[5];

typedef short vec_s_t;
typedef vec_s_t vec3s_t[3];
typedef vec_s_t vec4s_t[4]; // x,y,z,w
typedef vec_s_t vec5s_t[5];

typedef int fixed4_t;
typedef int fixed8_t;
typedef int fixed16_t;
#ifndef M_PI
#define M_PI 3.14159265358979323846 // matches value in gcc v2 math.h
#endif

struct mplane_s;

constexpr Vector vec3_origin(0, 0, 0);
constexpr Vector g_vecZero(0, 0, 0);
extern int nanmask;

#define IS_NAN(x) (((*(int*)&x) & nanmask) == nanmask)

#define VectorSubtract(a, b, c)   \
	{                             \
		(c)[0] = (a)[0] - (b)[0]; \
		(c)[1] = (a)[1] - (b)[1]; \
		(c)[2] = (a)[2] - (b)[2]; \
	}
#define VectorAdd(a, b, c)        \
	{                             \
		(c)[0] = (a)[0] + (b)[0]; \
		(c)[1] = (a)[1] + (b)[1]; \
		(c)[2] = (a)[2] + (b)[2]; \
	}
#define VectorCopy(a, b) \
	{                    \
		(b)[0] = (a)[0]; \
		(b)[1] = (a)[1]; \
		(b)[2] = (a)[2]; \
	}
#define RAD2DEG( x  )  ( (float)(x) * (float)(180.f / M_PI) )

inline void MatrixAngles(const float matrix[3][4], float *angles)
{
	float forward[3];
	float left[3];
	float up[3];

	//
	// Extract the basis vectors from the matrix. Since we only need the Z
	// component of the up vector, we don't get X and Y.
	//
	forward[0] = matrix[0][0];
	forward[1] = matrix[1][0];
	forward[2] = matrix[2][0];
	left[0] = matrix[0][1];
	left[1] = matrix[1][1];
	left[2] = matrix[2][1];
	up[2] = matrix[2][2];

	float xyDist = sqrtf(forward[0] * forward[0] + forward[1] * forward[1]);

	// enough here to get angles?
	if (xyDist > 0.001f)
	{
		// (yaw)    y = ATAN( forward.y, forward.x );        -- in our space, forward is the X axis
		angles[1] = RAD2DEG(atan2f(forward[1], forward[0]));

		// (pitch)    x = ATAN( -forward.z, sqrt(forward.x*forward.x+forward.y*forward.y) );
		angles[0] = RAD2DEG(atan2f(-forward[2], xyDist));

		// (roll)    z = ATAN( left.z, up.z );
		angles[2] = RAD2DEG(atan2f(left[2], up[2]));
	}
	else    // forward is mostly Z, gimbal lock-
	{
		// (yaw)    y = ATAN( -left.x, left.y );            -- forward is mostly z, so use right for yaw
		angles[1] = RAD2DEG(atan2f(-left[0], left[1]));

		// (pitch)    x = ATAN( -forward.z, sqrt(forward.x*forward.x+forward.y*forward.y) );
		angles[0] = RAD2DEG(atan2f(-forward[2], xyDist));

		// Assume no roll in this case as one degree of freedom has been lost (i.e. yaw == roll)
		angles[2] = 0;
	}
}

inline void VectorClear(float* a)
{
	a[0] = 0.0;
	a[1] = 0.0;
	a[2] = 0.0;
}

void VectorMA(const float* veca, float scale, const float* vecb, float* vecc);

bool VectorCompare(const float* v1, const float* v2);
float Length(const float* v);
void CrossProduct(const float* v1, const float* v2, float* cross);
float VectorNormalize(float* v); // returns vector length
void VectorInverse(float* v);
void VectorScale(const float* in, float scale, float* out);
int Q_log2(int val);

void R_ConcatRotations(float in1[3][3], float in2[3][3], float out[3][3]);
void R_ConcatTransforms(float in1[3][4], float in2[3][4], float out[3][4]);

void FloorDivMod(double numer, double denom, int* quotient,
	int* rem);
fixed16_t Invert24To16(fixed16_t val);
int GreatestCommonDivisor(int i1, int i2);

void AngleVectors(const Vector& angles, Vector* forward, Vector* right, Vector* up);
void AngleVectorsTranspose(const Vector& angles, Vector* forward, Vector* right, Vector* up);
#define AngleIVectors AngleVectorsTranspose

void AngleMatrix(const float* angles, float (*matrix)[4]);
void AngleIMatrix(const Vector& angles, float (*matrix)[4]);
void VectorTransform(const float* in1, float in2[3][4], float* out);

void NormalizeAngles(float* angles);
void InterpolateAngles(float* start, float* end, float* output, float frac);
float AngleBetweenVectors(const float* v1, const float* v2);


void VectorMatrix(const Vector& forward, Vector& right, Vector& up);
void VectorAngles(const float* forward, float* angles);

int InvertMatrix(const float* m, float* out);

int BoxOnPlaneSide(const Vector& emins, const Vector& emaxs, struct mplane_s* plane);
float anglemod(float a);



#define BOX_ON_PLANE_SIDE(emins, emaxs, p)                                                                 \
	(((p)->type < 3) ? (                                                                                   \
						   ((p)->dist <= (emins)[(p)->type]) ? 1                                           \
															 : (                                           \
																   ((p)->dist >= (emaxs)[(p)->type]) ? 2   \
																									 : 3)) \
					 : BoxOnPlaneSide((emins), (emaxs), (p)))
