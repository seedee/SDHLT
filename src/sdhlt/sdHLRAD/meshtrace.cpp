/*
trace.cpp - trace triangle meshes
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

#include "qrad.h"
#include "meshtrace.h"

void TraceMesh :: SetupTrace( const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end )
{
	m_bHitTriangle = false;

	VectorCopy( start, m_vecStart );
	VectorCopy( end, m_vecEnd );
	VectorSubtract( end, start, m_vecTraceDirection );
	m_flTraceDistance = VectorNormalize( m_vecTraceDirection );

	// build a bounding box of the entire move
	ClearBounds( m_vecAbsMins, m_vecAbsMaxs );

	VectorAdd( m_vecStart, mins, m_vecStartMins );
	AddPointToBounds( m_vecStartMins, m_vecAbsMins, m_vecAbsMaxs );

	VectorAdd( m_vecStart, maxs, m_vecStartMaxs );
	AddPointToBounds( m_vecStartMaxs, m_vecAbsMins, m_vecAbsMaxs );

	VectorAdd( m_vecEnd, mins, m_vecEndMins );
	AddPointToBounds( m_vecEndMins, m_vecAbsMins, m_vecAbsMaxs );

	VectorAdd( m_vecEnd, maxs, m_vecEndMaxs );
	AddPointToBounds( m_vecEndMaxs, m_vecAbsMins, m_vecAbsMaxs );

	// spread min\max by a pixel
	for( int i = 0; i < 3; i++ )
	{
		m_vecAbsMins[i] -= 1.0f;
		m_vecAbsMaxs[i] += 1.0f;
	}
}

bool TraceMesh :: ClipRayToBox( const vec3_t mins, const vec3_t maxs )
{
	vec3_t	ray_inv, t0, t1;
	vec3_t	n, f;
	float	d, t;

	VectorDivideVec( m_vecTraceDirection, 1.0f, ray_inv );

	VectorSubtract( mins, m_vecStart, t0 );
	VectorSubtract( maxs, m_vecStart, t1 );

	CrossProduct( t0, ray_inv, t0 );
	CrossProduct( t1, ray_inv, t1 );

	n[0] = qmin( t0[0], t1[0] );
	n[1] = qmin( t0[1], t1[1] );
	n[2] = qmin( t0[2], t1[2] );
	d = VectorMaximum( n );

	f[0] = qmax( t0[0], t1[0] );
	f[1] = qmax( t0[1], t1[1] );
	f[2] = qmax( t0[2], t1[2] );
	t = VectorMinimum( f );

	return ( t >= 0.0f ) && ( t >= d ); 
}

bool TraceMesh :: ClipRayToTriangle( const mfacet_t *facet )
{
	float uu, uv, vv, wu, wv, s, t;
	float d1, d2, d, frac;
	vec3_t w, n, p;

	// we have two edge directions, we can calculate the normal
	CrossProduct( facet->edge2, facet->edge1, n );

	if( VectorCompare( n, vec3_origin ))
		return false; // degenerate triangle

	VectorSubtract( m_vecEnd, m_vecStart, p );
	VectorSubtract( m_vecStart, facet->triangle[0].point, w );

	d1 = -DotProduct( n, w );
	d2 = DotProduct( n, p );
	if( fabs( d2 ) < FRAC_EPSILON )
		return false; // parallel with plane

	// get intersect point of ray with triangle plane
	frac = d1 / d2;

	if( frac < 0.0f )
		return false;

	// calculate the impact point
	p[0] = m_vecStart[0] + (m_vecEnd[0] - m_vecStart[0]) * frac;
	p[1] = m_vecStart[1] + (m_vecEnd[1] - m_vecStart[1]) * frac;
	p[2] = m_vecStart[2] + (m_vecEnd[2] - m_vecStart[2]) * frac;

	// does p lie inside triangle?
	uu = DotProduct( facet->edge1, facet->edge1 );
	uv = DotProduct( facet->edge1, facet->edge2 );
	vv = DotProduct( facet->edge2, facet->edge2 );

	VectorSubtract( p, facet->triangle[0].point, w );
	wu = DotProduct( w, facet->edge1 );
	wv = DotProduct( w, facet->edge2 );
	d = uv * uv - uu * vv;

	// get and test parametric coords
	s = (uv * wv - vv * wu) / d;
	if( s < 0.0f || s > 1.0f )
		return false; // p is outside

	t = (uv * wu - uu * wv) / d;
	if( t < 0.0 || (s + t) > 1.0 )
		return false; // p is outside

	return true;
}

bool TraceMesh :: ClipRayToFace( const mfacet_t *facet )
{
	vec3_t tvec, pvec, qvec;

	// begin calculating determinant - also used to calculate u parameter
	CrossProduct( m_vecTraceDirection, facet->edge2, pvec );

	// if determinant is near zero, trace lies in plane of triangle
	float det = DotProduct( facet->edge1, pvec );

	// the non-culling branch
	if( fabs( det ) < COPLANAR_EPSILON )
		return false;

	float invDet = 1.0f / det;

	// calculate distance from first vertex to ray origin
	VectorSubtract( m_vecStart, facet->triangle[0].point, tvec );

	// calculate u parameter and test bounds
	float u = DotProduct( tvec, pvec ) * invDet;
	if( u < -BARY_EPSILON || u > ( 1.0f + BARY_EPSILON ))
		return false;

	// prepare to test v parameter
	CrossProduct( tvec, facet->edge1, qvec );

	// calculate v parameter and test bounds
	float v = DotProduct( m_vecTraceDirection, qvec ) * invDet;
	if( v < -BARY_EPSILON || ( u + v ) > ( 1.0f + BARY_EPSILON ))
		return false;

	// calculate t (depth)
	float depth = DotProduct( facet->edge2, qvec ) * invDet;
	if( depth <= NEAR_SHADOW_EPSILON || depth >= m_flTraceDistance )
		return false;

	// most surfaces are completely opaque
	if( !facet->texture || !facet->texture->index )
		return true;

	if( facet->texture->flags & STUDIO_NF_ADDITIVE )
		return false; // translucent

	if( !( facet->texture->flags & STUDIO_NF_TRANSPARENT ))
		return true;

	// try to avoid double shadows near triangle seams
	if( u < -ASLF_EPSILON || u > ( 1.0f + ASLF_EPSILON ) || v < -ASLF_EPSILON || ( u + v ) > ( 1.0f + ASLF_EPSILON ))
		return false;

	// calculate w parameter
	float w = 1.0f - ( u + v );

	// calculate st from uvw (barycentric) coordinates
	float s = w * facet->triangle[0].st[0] + u * facet->triangle[1].st[0] + v * facet->triangle[2].st[0];
	float t = w * facet->triangle[0].st[1] + u * facet->triangle[1].st[1] + v * facet->triangle[2].st[1];
	s = s - floor( s );
	t = t - floor( t );

	int is = s * facet->texture->width;
	int it = t * facet->texture->height;

	if( is < 0 ) is = 0;
	if( it < 0 ) it = 0;

	if( is > facet->texture->width - 1 )
		is = facet->texture->width - 1;
	if( it > facet->texture->height - 1 )
		it = facet->texture->height - 1;

	byte *pixels = (byte *)m_extradata + facet->texture->index;

	// test pixel
	if( pixels[it * facet->texture->width + is] == 0xFF )
		return false; // last color in palette is indicated alpha-pixel

	return true;
}

bool TraceMesh :: ClipRayToFacet( const mfacet_t *facet )
{
	mplane_t	*p, *clipplane;
	float	enterfrac, leavefrac, distfrac;
	bool	getout, startout;
	float	d, d1, d2, f;

	if( !facet->numplanes )
		return false;

	enterfrac = -1.0f;
	leavefrac = 1.0f;
	clipplane = NULL;
	checkcount++;

	getout = false;
	startout = false;

	for( int i = 0; i < facet->numplanes; i++ )
	{
		p = &mesh->planes[facet->indices[i]];

		// push the plane out apropriately for mins/maxs
		if( p->type < 3 )
		{
			d1 = m_vecStartMins[p->type] - p->dist;
			d2 = m_vecEndMins[p->type] - p->dist;
		}
		else
		{
			switch( p->signbits )
			{
			case 0:
				d1 = p->normal[0] * m_vecStartMins[0] + p->normal[1] * m_vecStartMins[1] + p->normal[2] * m_vecStartMins[2] - p->dist;
				d2 = p->normal[0] * m_vecEndMins[0] + p->normal[1] * m_vecEndMins[1] + p->normal[2] * m_vecEndMins[2] - p->dist;
				break;
			case 1:
				d1 = p->normal[0] * m_vecStartMaxs[0] + p->normal[1] * m_vecStartMins[1] + p->normal[2] * m_vecStartMins[2] - p->dist;
				d2 = p->normal[0] * m_vecEndMaxs[0] + p->normal[1] * m_vecEndMins[1] + p->normal[2] * m_vecEndMins[2] - p->dist;
				break;
			case 2:
				d1 = p->normal[0] * m_vecStartMins[0] + p->normal[1] * m_vecStartMaxs[1] + p->normal[2] * m_vecStartMins[2] - p->dist;
				d2 = p->normal[0] * m_vecEndMins[0] + p->normal[1] * m_vecEndMaxs[1] + p->normal[2] * m_vecEndMins[2] - p->dist;
				break;
			case 3:
				d1 = p->normal[0] * m_vecStartMaxs[0] + p->normal[1] * m_vecStartMaxs[1] + p->normal[2] * m_vecStartMins[2] - p->dist;
				d2 = p->normal[0] * m_vecEndMaxs[0] + p->normal[1] * m_vecEndMaxs[1] + p->normal[2] * m_vecEndMins[2] - p->dist;
				break;
			case 4:
				d1 = p->normal[0] * m_vecStartMins[0] + p->normal[1] * m_vecStartMins[1] + p->normal[2] * m_vecStartMaxs[2] - p->dist;
				d2 = p->normal[0] * m_vecEndMins[0] + p->normal[1] * m_vecEndMins[1] + p->normal[2] * m_vecEndMaxs[2] - p->dist;
				break;
			case 5:
				d1 = p->normal[0] * m_vecStartMaxs[0] + p->normal[1] * m_vecStartMins[1] + p->normal[2] * m_vecStartMaxs[2] - p->dist;
				d2 = p->normal[0] * m_vecEndMaxs[0] + p->normal[1] * m_vecEndMins[1] + p->normal[2] * m_vecEndMaxs[2] - p->dist;
				break;
			case 6:
				d1 = p->normal[0] * m_vecStartMins[0] + p->normal[1] * m_vecStartMaxs[1] + p->normal[2] * m_vecStartMaxs[2] - p->dist;
				d2 = p->normal[0] * m_vecEndMins[0] + p->normal[1] * m_vecEndMaxs[1] + p->normal[2] * m_vecEndMaxs[2] - p->dist;
				break;
			case 7:
				d1 = p->normal[0] * m_vecStartMaxs[0] + p->normal[1] * m_vecStartMaxs[1] + p->normal[2] * m_vecStartMaxs[2] - p->dist;
				d2 = p->normal[0] * m_vecEndMaxs[0] + p->normal[1] * m_vecEndMaxs[1] + p->normal[2] * m_vecEndMaxs[2] - p->dist;
				break;
			default:
				d1 = d2 = 0.0f; // shut up compiler
				break;
			}
		}

		if( d2 > 0.0f ) getout = true;	// endpoint is not in solid
		if( d1 > 0.0f ) startout = true;

		// if completely in front of face, no intersection
		if( d1 > 0 && d2 >= d1 )
			return false;

		if( d1 <= 0 && d2 <= 0 )
			continue;

		// crosses face
		d = 1 / (d1 - d2);
		f = d1 * d;

		if( d > 0.0f )
		{	
			// enter
			if( f > enterfrac )
			{
				distfrac = d;
				enterfrac = f;
				clipplane = p;
			}
		}
		else if( d < 0.0f )
		{	
			// leave
			if( f < leavefrac )
				leavefrac = f;
		}
	}

	if( !startout )
	{
		// original point was inside brush
		return true;
	}

	if((( enterfrac - FRAC_EPSILON ) <= leavefrac ) && ( enterfrac > -1.0f ) && ( enterfrac < 1.0f ))
		return true;

	return false;
}

void TraceMesh :: ClipToLinks( areanode_t *node )
{
	link_t	*l, *next;
	mfacet_t	*facet;

	// touch linked edicts
	for( l = node->facets.next; l != &node->facets; l = next )
	{
		next = l->next;

		facet = FACET_FROM_AREA( l );

		if( !BoundsIntersect( m_vecAbsMins, m_vecAbsMaxs, facet->mins, facet->maxs ))
			continue;

		if( mesh->trace_mode == SHADOW_FAST )
		{
			// ultra-fast mode, no real tracing here
			if( ClipRayToBox( facet->mins, facet->maxs ))
			{
				m_bHitTriangle = true;
				return;
			}
		}
		else if( mesh->trace_mode == SHADOW_NORMAL )
		{
			// does trace for each triangle
			if( ClipRayToFace( facet ))
			{
				m_bHitTriangle = true;
				return;
			}
		}
		else if( mesh->trace_mode == SHADOW_SLOW )
		{
			// does trace for planes bbox for each triangle
			if( ClipRayToFacet( facet ))
			{
				m_bHitTriangle = true;
				return;
			}
		}
		else
		{
			// unknown mode
			return;
		}
	}
	
	// recurse down both sides
	if( node->axis == -1 ) return;

	if( m_vecAbsMaxs[node->axis] > node->dist )
		ClipToLinks( node->children[0] );
	if( m_vecAbsMins[node->axis] < node->dist )
		ClipToLinks( node->children[1] );
}

bool TraceMesh :: DoTrace( void )
{
	if( !mesh || !BoundsIntersect( mesh->mins, mesh->maxs, m_vecAbsMins, m_vecAbsMaxs ))
		return false; // invalid mesh or no intersection

	checkcount = 0;

	if( areanodes )
	{
		ClipToLinks( areanodes );
	}
	else
	{
		mfacet_t *facet = mesh->facets;
		for( int i = 0; i < mesh->numfacets; i++, facet++ )
		{
			if( !BoundsIntersect( m_vecAbsMins, m_vecAbsMaxs, facet->mins, facet->maxs ))
				continue;

			if( mesh->trace_mode == SHADOW_FAST )
			{
				// ultra-fast mode, no real tracing here
				if( ClipRayToBox( facet->mins, facet->maxs ))
					return true;
			}
			else if( mesh->trace_mode == SHADOW_NORMAL )
			{
				// does trace for each triangle
				if( ClipRayToFace( facet ))
					return true;
			}
			else if( mesh->trace_mode == SHADOW_SLOW )
			{
				// does trace for planes bbox for each triangle
				if( ClipRayToFacet( facet ))
					return true;
			}
			else
			{
				// unknown mode
				return false;
			}
		}
	}

//	Developer( DEVELOPER_LEVEL_MESSAGE, "total %i checks for %s\n", checkcount, areanodes ? "tree" : "brute force" );

	return m_bHitTriangle;
}