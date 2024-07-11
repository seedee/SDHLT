/*
trace.h - trace triangle meshes
Copyright (C) 2012 Uncle Mike

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#ifndef TRACEMESH_H
#define TRACEMESH_H

#include "meshdesc.h"

#define FRAC_EPSILON		(1.0f / 32.0f)
#define BARY_EPSILON		0.01f
#define ASLF_EPSILON		0.0001f
#define COPLANAR_EPSILON		0.25f
#define NEAR_SHADOW_EPSILON		1.5f
#define SELF_SHADOW_EPSILON		0.5f

#define STRUCT_FROM_LINK( l, t, m )	((t *)((byte *)l - (int)(long int)&(((t *)0)->m)))
#define FACET_FROM_AREA( l )		STRUCT_FROM_LINK( l, mfacet_t, area )
#define bound( min, num, max )	((num) >= (min) ? ((num) < (max) ? (num) : (max)) : (min))

class TraceMesh
{
private:
	vec3_t		m_vecStart, m_vecEnd;
	vec3_t		m_vecStartMins, m_vecEndMins;
	vec3_t		m_vecStartMaxs, m_vecEndMaxs;
	vec3_t		m_vecAbsMins, m_vecAbsMaxs;
	vec3_t		m_vecTraceDirection;// ray direction
	float		m_flTraceDistance;
	bool		m_bHitTriangle;	// now we hit triangle
	areanode_t	*areanodes;	// AABB for static meshes
	mmesh_t		*mesh;		// mesh to trace
	int		checkcount;	// debug
	void		*m_extradata;	// pointer to model extradata

	void ClearBounds( vec3_t mins, vec3_t maxs )
	{
		// make bogus range
		mins[0] = mins[1] = mins[2] =  999999.0f;
		maxs[0] = maxs[1] = maxs[2] = -999999.0f;
	}

	void AddPointToBounds( const vec3_t v, vec3_t mins, vec3_t maxs )
	{
		for( int i = 0; i < 3; i++ )
		{
			if( v[i] < mins[i] ) mins[i] = v[i];
			if( v[i] > maxs[i] ) maxs[i] = v[i];
		}
	}

	bool BoundsIntersect( const vec3_t mins1, const vec3_t maxs1, const vec3_t mins2, const vec3_t maxs2 )
	{
		if( mins1[0] > maxs2[0] || mins1[1] > maxs2[1] || mins1[2] > maxs2[2] )
			return false;
		if( maxs1[0] < mins2[0] || maxs1[1] < mins2[1] || maxs1[2] < mins2[2] )
			return false;
		return true;
	}
public:
	TraceMesh() { mesh = NULL; }
	~TraceMesh() {}

	// trace stuff
	void SetTraceMesh( mmesh_t *cached_mesh, areanode_t *tree ) { mesh = cached_mesh; areanodes = tree; }
	void SetupTrace( const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end ); 
	void SetTraceModExtradata( void *data ) { m_extradata = data; }
	bool ClipRayToBox( const vec3_t mins, const vec3_t maxs );
	bool ClipRayToTriangle( const mfacet_t *facet );	// obsolete
	bool ClipRayToFacet( const mfacet_t *facet );
	bool ClipRayToFace( const mfacet_t *facet ); // ripped out from q3map2
	void ClipToLinks( areanode_t *node );
	bool DoTrace( void );
};

#endif//TRACEMESH_H