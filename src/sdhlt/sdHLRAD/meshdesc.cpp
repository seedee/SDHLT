/*
meshdesc.cpp - cached mesh for tracing custom objects
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
#include "meshdesc.h"
#include "stringlib.h"
#include "TimeCounter.h"

//#define AABB_OFFSET
#define SIMPLIFICATION_FACTOR_HIGH	0.15f
#define SIMPLIFICATION_FACTOR_MED	0.55f
#define SIMPLIFICATION_FACTOR_LOW	0.85f

CMeshDesc :: CMeshDesc( void )
{
	memset( &m_mesh, 0, sizeof( m_mesh ));

	m_debugName = NULL;
	planehash = NULL;
	planepool = NULL;
	facets = NULL;
	m_iNumTris = 0;
}

CMeshDesc :: ~CMeshDesc( void )
{
	FreeMesh ();
}

void CMeshDesc :: InsertLinkBefore( link_t *l, link_t *before )
{
	l->next = before;
	l->prev = before->prev;
	l->prev->next = l;
	l->next->prev = l;
}

void CMeshDesc :: RemoveLink( link_t *l )
{
	l->next->prev = l->prev;
	l->prev->next = l->next;
}

void CMeshDesc :: ClearLink( link_t *l )
{
	l->prev = l->next = l;
}

/*
===============
CreateAreaNode

builds a uniformly subdivided tree for the given mesh size
===============
*/
areanode_t *CMeshDesc :: CreateAreaNode( int depth, const vec3_t mins, const vec3_t maxs )
{
	areanode_t	*anode;
	vec3_t		size;
	vec3_t		mins1, maxs1;
	vec3_t		mins2, maxs2;

	anode = &areanodes[numareanodes++];

	ClearLink( &anode->facets );
	
	if( depth == AREA_DEPTH )
	{
		anode->axis = -1;
		anode->children[0] = anode->children[1] = NULL;
		return anode;
	}

	VectorSubtract( maxs, mins, size );	

	if( size[0] > size[1] )
		anode->axis = 0;
	else anode->axis = 1;
	
	anode->dist = 0.5f * ( maxs[anode->axis] + mins[anode->axis] );

	VectorCopy( mins, mins1 );
	VectorCopy( mins, mins2 );
	VectorCopy( maxs, maxs1 );
	VectorCopy( maxs, maxs2 );

	maxs1[anode->axis] = mins2[anode->axis] = anode->dist;
	anode->children[0] = CreateAreaNode( depth+1, mins2, maxs2 );
	anode->children[1] = CreateAreaNode( depth+1, mins1, maxs1 );

	return anode;
}

void CMeshDesc :: FreeMesh( void )
{
	if( m_mesh.numfacets <= 0 )
		return;

	// single memory block
	free( m_mesh.planes );

	FreeMeshBuild();

	memset( &m_mesh, 0, sizeof( m_mesh ));
}

bool CMeshDesc :: InitMeshBuild( const char *debug_name, int numTriangles )
{
	if( numTriangles <= 0 )
		return false;

	m_debugName = debug_name;

	// perfomance warning
	if( numTriangles >= 65536 )
	{
		Log( "Error: %s have too many triangles (%i). Mesh cannot be build\n", m_debugName, numTriangles );
		return false; // failed to build (too many triangles)
	}
	else if( numTriangles >= 32768 )
		Warning( "%s have too many triangles (%i)\n", m_debugName, numTriangles );
	else if( numTriangles >= 16384 )
		Developer( DEVELOPER_LEVEL_WARNING, "%s have too many triangles (%i)\n", m_debugName, numTriangles );

	if( numTriangles >= 256 )
		has_tree = true;	// too many triangles invoke to build AABB tree
	else has_tree = false;

	ClearBounds( m_mesh.mins, m_mesh.maxs );

	memset( areanodes, 0, sizeof( areanodes ));
	numareanodes = 0;
	m_iNumTris = numTriangles;
	m_iTotalPlanes = 0;

	// create pools for construct mesh
	facets = (mfacet_t *)calloc( sizeof( mfacet_t ), numTriangles );
	planehash = (hashplane_t **)calloc( sizeof( hashplane_t* ), PLANE_HASHES );
	planepool = (hashplane_t *)calloc( sizeof( hashplane_t ), MAX_PLANES );

	mplane_t	badplane;

	// create default invalid plane for index 0
	memset( &badplane, 0, sizeof( badplane ));
	AddPlaneToPool( &badplane );

	return true;
}

bool CMeshDesc :: PlaneEqual( const mplane_t *p0, const mplane_t *p1 )
{
	float	t;

	if( -PLANE_DIST_EPSILON < ( t = p0->dist - p1->dist ) && t < PLANE_DIST_EPSILON
	 && -PLANE_NORMAL_EPSILON < ( t = p0->normal[0] - p1->normal[0] ) && t < PLANE_NORMAL_EPSILON
	 && -PLANE_NORMAL_EPSILON < ( t = p0->normal[1] - p1->normal[1] ) && t < PLANE_NORMAL_EPSILON
	 && -PLANE_NORMAL_EPSILON < ( t = p0->normal[2] - p1->normal[2] ) && t < PLANE_NORMAL_EPSILON )
		return true;

	return false;
}

uint CMeshDesc :: AddPlaneToPool( const mplane_t *pl )
{
	hashplane_t *p;
	int hash;

	// trying to find equal plane
	hash = (int)fabs( pl->dist );
	hash &= (PLANE_HASHES - 1);

	// search the border bins as well
	for( int i = -1; i <= 1; i++ )
	{
		int h = (hash + i) & (PLANE_HASHES - 1);
		for( p = planehash[h]; p; p = p->hash )
		{
			if( PlaneEqual( &p->pl, pl ))
				return (uint)(p - planepool);	// already exist
		}
	}

	if( m_mesh.numplanes >= MAX_PLANES )
          {
		Error( "AddPlaneToPool: plane limit exceeded: planes %i, maxplanes %i\n", m_mesh.numplanes, MAX_PLANES );
		return 0;	// index of our bad plane
          }

	// create a new one
	p = &planepool[m_mesh.numplanes++];
	p->hash = planehash[hash];
	planehash[hash] = p;

	// record the new plane
	p->pl = *pl;

	return (m_mesh.numplanes - 1);
}

/*
=====================
PlaneFromPoints

Returns false if the triangle is degenrate.
The normal will point out of the clock for clockwise ordered points
=====================
*/
bool CMeshDesc :: PlaneFromPoints( const mvert_t triangle[3], mplane_t *plane )
{
	vec3_t	v1, v2;

	VectorSubtract( triangle[1].point, triangle[0].point, v1 );
	VectorSubtract( triangle[2].point, triangle[0].point, v2 );
	CrossProduct( v2, v1, plane->normal );

	if( VectorLength( plane->normal ) == 0.0f )
	{
		VectorClear( plane->normal );
		return false;
	}

	VectorNormalize( plane->normal );
	plane->dist = DotProduct( triangle[0].point, plane->normal );

	return true;
}

/*
=================
ComparePlanes
=================
*/
bool CMeshDesc :: ComparePlanes( const mplane_t *plane, const vec3_t normal, float dist )
{
	if( fabs( plane->normal[0] - normal[0] ) < PLANE_NORMAL_EPSILON
	 && fabs( plane->normal[1] - normal[1] ) < PLANE_NORMAL_EPSILON
	 && fabs( plane->normal[2] - normal[2] ) < PLANE_NORMAL_EPSILON
	 && fabs( plane->dist - dist ) < PLANE_DIST_EPSILON )
		return true;
	return false;
}

/*
==================
SnapVectorToGrid
==================
*/
void CMeshDesc :: SnapVectorToGrid( vec3_t normal )
{
	for( int i = 0; i < 3; i++ )
	{
		if( fabs( normal[i] - 1.0f ) < PLANE_NORMAL_EPSILON )
		{
			VectorClear( normal );
			normal[i] = 1.0f;
			break;
		}

		if( fabs( normal[i] - -1.0f ) < PLANE_NORMAL_EPSILON )
		{
			VectorClear( normal );
			normal[i] = -1.0f;
			break;
		}
	}
}

/*
==============
SnapPlaneToGrid
==============
*/
void CMeshDesc :: SnapPlaneToGrid( mplane_t *plane )
{
	SnapVectorToGrid( plane->normal );

	if( fabs( plane->dist - Q_rint( plane->dist )) < PLANE_DIST_EPSILON )
		plane->dist = Q_rint( plane->dist );
}

/*
=================
CategorizePlane

A slightly more complex version of SignbitsForPlane and PlaneTypeForNormal,
which also tries to fix possible floating point glitches (like -0.00000 cases)
=================
*/
void CMeshDesc :: CategorizePlane( mplane_t *plane )
{
	plane->signbits = 0;
	plane->type = 3; // non-axial

	for( int i = 0; i < 3; i++ )
	{
		if( plane->normal[i] < 0.0f )
		{
			plane->signbits |= (1<<i);

			if( plane->normal[i] == -1.0f )
			{
				plane->signbits = (1<<i);
				VectorClear( plane->normal );
				plane->normal[i] = -1.0f;
				break;
			}
		}
		else if( plane->normal[i] == 1.0f )
		{
			plane->type = i;
			plane->signbits = 0;
			VectorClear( plane->normal );
			plane->normal[i] = 1.0f;
			break;
		}
	}
}

/*
====================
AngleQuaternion
====================
*/
void CMeshDesc :: AngleQuaternion( const vec3_t angles, vec4_t quat )
{
	float sr, sp, sy, cr, cp, cy;
	float angle;

	// FIXME: rescale the inputs to 1/2 angle
	angle = angles[2] * 0.5;
	sy = sin( angle );
	cy = cos( angle );
	angle = angles[1] * 0.5;
	sp = sin( angle );
	cp = cos( angle );
	angle = angles[0] * 0.5;
	sr = sin( angle );
	cr = cos( angle );

	quat[0] = sr*cp*cy-cr*sp*sy; // X
	quat[1] = cr*sp*cy+sr*cp*sy; // Y
	quat[2] = cr*cp*sy-sr*sp*cy; // Z
	quat[3] = cr*cp*cy+sr*sp*sy; // W
}

/*
====================
AngleMatrix
====================
*/
void CMeshDesc :: AngleMatrix( const vec3_t angles, const vec3_t origin, const vec3_t scale, float (*matrix)[4] )
{
	float sr, sp, sy, cr, cp, cy;
	float angle;
	
	angle = angles[1] * (M_PI * 2.0f / 360.0f);
	sy = sin( angle );
	cy = cos( angle );
	angle = angles[0] * (M_PI * 2.0f / 360.0f);
	sp = sin( angle );
	cp = cos( angle );
	angle = angles[2] * (M_PI * 2.0f / 360.0f);
	sr = sin( angle );
	cr = cos( angle );

	// matrix = (YAW * PITCH) * ROLL
	matrix[0][0] = cp*cy * scale[0];
	matrix[1][0] = cp*sy * scale[0];
	matrix[2][0] = -sp * scale[0];
	matrix[0][1] = sr*sp*cy+cr*-sy * scale[1];
	matrix[1][1] = sr*sp*sy+cr*cy * scale[1];
	matrix[2][1] = sr*cp * scale[1];
	matrix[0][2] = (cr*sp*cy+-sr*-sy) * scale[2];
	matrix[1][2] = (cr*sp*sy+-sr*cy) * scale[2];
	matrix[2][2] = cr*cp * scale[2];
	matrix[0][3] = origin[0];
	matrix[1][3] = origin[1];
	matrix[2][3] = origin[2];
}

/*
================
QuaternionMatrix
================
*/
void CMeshDesc :: QuaternionMatrix( vec4_t quat, const vec3_t origin, float (*matrix)[4] )
{
	matrix[0][0] = 1.0 - 2.0 * quat[1] * quat[1] - 2.0 * quat[2] * quat[2];
	matrix[1][0] = 2.0 * quat[0] * quat[1] + 2.0 * quat[3] * quat[2];
	matrix[2][0] = 2.0 * quat[0] * quat[2] - 2.0 * quat[3] * quat[1];

	matrix[0][1] = 2.0 * quat[0] * quat[1] - 2.0 * quat[3] * quat[2];
	matrix[1][1] = 1.0 - 2.0 * quat[0] * quat[0] - 2.0 * quat[2] * quat[2];
	matrix[2][1] = 2.0 * quat[1] * quat[2] + 2.0 * quat[3] * quat[0];

	matrix[0][2] = 2.0 * quat[0] * quat[2] + 2.0 * quat[3] * quat[1];
	matrix[1][2] = 2.0 * quat[1] * quat[2] - 2.0 * quat[3] * quat[0];
	matrix[2][2] = 1.0 - 2.0 * quat[0] * quat[0] - 2.0 * quat[1] * quat[1];

	matrix[0][3] = origin[0];
	matrix[1][3] = origin[1];
	matrix[2][3] = origin[2];
}

/*
================
ConcatTransforms
================
*/
void CMeshDesc :: ConcatTransforms( float in1[3][4], float in2[3][4], float out[3][4] )
{
	out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] + in1[0][2] * in2[2][0];
	out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] + in1[0][2] * in2[2][1];
	out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] + in1[0][2] * in2[2][2];
	out[0][3] = in1[0][0] * in2[0][3] + in1[0][1] * in2[1][3] + in1[0][2] * in2[2][3] + in1[0][3];
	out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] + in1[1][2] * in2[2][0];
	out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] + in1[1][2] * in2[2][1];
	out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] + in1[1][2] * in2[2][2];
	out[1][3] = in1[1][0] * in2[0][3] + in1[1][1] * in2[1][3] + in1[1][2] * in2[2][3] + in1[1][3];
	out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] + in1[2][2] * in2[2][0];
	out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] + in1[2][2] * in2[2][1];
	out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] + in1[2][2] * in2[2][2];
	out[2][3] = in1[2][0] * in2[0][3] + in1[2][1] * in2[1][3] + in1[2][2] * in2[2][3] + in1[2][3];
}

/*
====================
VectorTransform
====================
*/
void CMeshDesc :: VectorTransform( const vec3_t in1, float in2[3][4], vec3_t out )
{
	out[0] = DotProduct( in1, in2[0] ) + in2[0][3];
	out[1] = DotProduct( in1, in2[1] ) + in2[1][3];
	out[2] = DotProduct( in1, in2[2] ) + in2[2][3];
}

void CMeshDesc :: StudioCalcBoneQuaterion( mstudiobone_t *pbone, mstudioanim_t *panim, vec4_t q )
{
	mstudioanimvalue_t *panimvalue;
	vec3_t angle;

	for( int j = 0; j < 3; j++ )
	{
		if( panim->offset[j+3] == 0 )
		{
			angle[j] = pbone->value[j+3]; // default;
		}
		else
		{
			panimvalue = (mstudioanimvalue_t *)((byte *)panim + panim->offset[j+3]);
			angle[j] = panimvalue[1].value;
			angle[j] = pbone->value[j+3] + angle[j] * pbone->scale[j+3];
		}
	}

	AngleQuaternion( angle, q );
}

void CMeshDesc :: StudioCalcBonePosition( mstudiobone_t *pbone, mstudioanim_t *panim, vec3_t pos )
{
	mstudioanimvalue_t *panimvalue;

	for( int j = 0; j < 3; j++ )
	{
		pos[j] = pbone->value[j]; // default;

		if( panim->offset[j] != 0 )
		{
			panimvalue = (mstudioanimvalue_t *)((byte *)panim + panim->offset[j]);
			pos[j] += panimvalue[1].value * pbone->scale[j];
		}
	}
}

bool CMeshDesc :: StudioConstructMesh( model_t *pModel )
{
	int i;
	studiohdr_t *phdr = (studiohdr_t *)pModel->extradata;

	if( !phdr || phdr->numbones < 1 )
	{
		Developer( DEVELOPER_LEVEL_ERROR, "StudioConstructMesh: bad model header\n" );
		return false;
	}

	TimeCounter profile;

	profile.start();
	bool simplify_model = (pModel->trace_mode == 2) ? true : false; // trying to reduce polycount and the speedup compilation

	// compute default pose for building mesh from
	mstudioseqdesc_t *pseqdesc = (mstudioseqdesc_t *)((byte *)phdr + phdr->seqindex);
	mstudioseqgroup_t *pseqgroup = (mstudioseqgroup_t *)((byte *)phdr + phdr->seqgroupindex) + pseqdesc->seqgroup;

	// sanity check
	if( pseqdesc->seqgroup != 0 )
	{
		Developer( DEVELOPER_LEVEL_ERROR, "StudioConstructMesh: bad sequence group (must be 0)\n" );
		return false;
	}

#ifdef VERSION_64BIT
	mstudioanim_t *panim = (mstudioanim_t *)((byte *)phdr + /*pseqgroup->data +*/ pseqdesc->animindex);
#else
	mstudioanim_t *panim = (mstudioanim_t *)((byte *)phdr + pseqgroup->data + pseqdesc->animindex);
#endif
	mstudiobone_t *pbone = (mstudiobone_t *)((byte *)phdr + phdr->boneindex);
	static vec3_t pos[MAXSTUDIOBONES];
	static vec4_t q[MAXSTUDIOBONES];
	int totalVertSize = 0;

	for( int i = 0; i < phdr->numbones; i++, pbone++, panim++ ) 
	{
		StudioCalcBoneQuaterion( pbone, panim, q[i] );
		StudioCalcBonePosition( pbone, panim, pos[i] );
	}

	pbone = (mstudiobone_t *)((byte *)phdr + phdr->boneindex);
	matrix3x4	transform, bonematrix, bonetransform[MAXSTUDIOBONES];
	AngleMatrix( pModel->angles, pModel->origin, pModel->scale, transform );

	// compute bones for default anim
	for( i = 0; i < phdr->numbones; i++ ) 
	{
		// initialize bonematrix
		QuaternionMatrix( q[i], pos[i], bonematrix );

		if( pbone[i].parent == -1 ) 
			ConcatTransforms( transform, bonematrix, bonetransform[i] );
		else ConcatTransforms( bonetransform[pbone[i].parent], bonematrix, bonetransform[i] );
	}

	// through all bodies to determine max vertices count
	for( i = 0; i < phdr->numbodyparts; i++ )
	{
		mstudiobodyparts_t *pbodypart = (mstudiobodyparts_t *)((byte *)phdr + phdr->bodypartindex) + i;

		int index = pModel->body / pbodypart->base;
		index = index % pbodypart->nummodels;

		mstudiomodel_t *psubmodel = (mstudiomodel_t *)((byte *)phdr + pbodypart->modelindex) + index;
		totalVertSize += psubmodel->numverts;
	}

	vec3_t *verts = (vec3_t *)malloc( sizeof( vec3_t ) * totalVertSize * 8 ); // allocate temporary vertices array
	float *coords = (float *)malloc( sizeof( float ) * totalVertSize * 16 ); // allocate temporary texcoords array
	mstudiotexture_t **textures = (mstudiotexture_t **)malloc( sizeof( mstudiotexture_t* ) * totalVertSize * 8 ); // lame way...
	unsigned int *indices = (unsigned int *)malloc( sizeof( int ) * totalVertSize * 24 );
	int numVerts = 0, numElems = 0, numTris = 0;
	mvert_t triangle[3];

	for( int k = 0; k < phdr->numbodyparts; k++ )
	{
		mstudiobodyparts_t *pbodypart = (mstudiobodyparts_t *)((byte *)phdr + phdr->bodypartindex) + k;

		int index = pModel->body / pbodypart->base;
		index = index % pbodypart->nummodels;
		int m_skinnum = qmin( qmax( 0, pModel->skin ), MAXSTUDIOSKINS );

		mstudiomodel_t *psubmodel = (mstudiomodel_t *)((byte *)phdr + pbodypart->modelindex) + index;
		vec3_t *pstudioverts = (vec3_t *)((byte *)phdr + psubmodel->vertindex);
		vec3_t *m_verts = (vec3_t *)malloc( sizeof( vec3_t ) * psubmodel->numverts );
		byte *pvertbone = ((byte *)phdr + psubmodel->vertinfoindex);

		// setup all the vertices
		for( i = 0; i < psubmodel->numverts; i++ )
			VectorTransform( pstudioverts[i], bonetransform[pvertbone[i]], m_verts[i] );

		mstudiotexture_t *ptexture = (mstudiotexture_t *)((byte *)phdr + phdr->textureindex);
		short *pskinref = (short *)((byte *)phdr + phdr->skinindex);
		if( m_skinnum != 0 && m_skinnum < phdr->numskinfamilies )
			pskinref += (m_skinnum * phdr->numskinref);

		for( int j = 0; j < psubmodel->nummesh; j++ ) 
		{
			mstudiomesh_t *pmesh = (mstudiomesh_t *)((byte *)phdr + psubmodel->meshindex) + j;
			short *ptricmds = (short *)((byte *)phdr + pmesh->triindex);
			int flags = ptexture[pskinref[pmesh->skinref]].flags;
			float s = 1.0f / (float)ptexture[pskinref[pmesh->skinref]].width;
			float t = 1.0f / (float)ptexture[pskinref[pmesh->skinref]].height;

			while( i = *( ptricmds++ ))
			{
				int	vertexState = 0;
				bool	tri_strip;

				if( i < 0 )
				{
					tri_strip = false;
					i = -i;
				}
				else tri_strip = true;

				numTris += (i - 2);

				for( ; i > 0; i--, ptricmds += 4 )
				{
					// build in indices
					if( vertexState++ < 3 )
					{
						indices[numElems++] = numVerts;
					}
					else if( tri_strip )
					{
						// flip triangles between clockwise and counter clockwise
						if( vertexState & 1 )
						{
							// draw triangle [n-2 n-1 n]
							indices[numElems++] = numVerts - 2;
							indices[numElems++] = numVerts - 1;
							indices[numElems++] = numVerts;
						}
						else
						{
							// draw triangle [n-1 n-2 n]
							indices[numElems++] = numVerts - 1;
							indices[numElems++] = numVerts - 2;
							indices[numElems++] = numVerts;
						}
					}
					else
					{
						// draw triangle fan [0 n-1 n]
						indices[numElems++] = numVerts - ( vertexState - 1 );
						indices[numElems++] = numVerts - 1;
						indices[numElems++] = numVerts;
					}

					VectorCopy( m_verts[ptricmds[0]], verts[numVerts] );
					textures[numVerts] = &ptexture[pskinref[pmesh->skinref]];
					if( flags & STUDIO_NF_CHROME )
					{
						// probably always equal 64 (see studiomdl.c for details)
						coords[numVerts*2+0] = s;
						coords[numVerts*2+1] = t;
					}
					else if( flags & STUDIO_NF_UV_COORDS )
					{
						coords[numVerts*2+0] = HalfToFloat( ptricmds[2] );
						coords[numVerts*2+1] = HalfToFloat( ptricmds[3] );
					}
					else
					{
						coords[numVerts*2+0] = ptricmds[2] * s;
						coords[numVerts*2+1] = ptricmds[3] * t;
					}
					numVerts++;
				}
			}
		}

		free( m_verts ); // don't keep this because different submodels may have difference count of vertices
	}

	if( numTris != ( numElems / 3 ))
		Developer( DEVELOPER_LEVEL_ERROR, "StudioConstructMesh: mismatch triangle count (%i should be %i)\n", (numElems / 3), numTris );

	// member trace mode
	m_mesh.trace_mode = pModel->trace_mode;

	InitMeshBuild( pModel->name, numTris );

	if( simplify_model )
	{
		// begin model simplification
		List<vector>	vert;		// list of vertices
		List<triset>	tris;		// list of triangles
		List<int>		collapse_map;	// to which neighbor each vertex collapses
		List<int>		permutation;	// permutation list

		// build the list of vertices
		for( i = 0; i < numVerts; i++ )
			vert.Add( verts[i] );

		// build the list of indices
		for( i = 0; i < numElems; i += 3 )
		{
			triset td;

			// fill the triangle
			td.v[0] = indices[i+0];
			td.v[1] = indices[i+1];
			td.v[2] = indices[i+2];
			tris.Add( td );
		}

		// do mesh simplification
		ProgressiveMesh( vert, tris, collapse_map, permutation );
		PermuteVertices( permutation, vert, tris );

		int verts_reduced;
		int tris_reduced = 0;

		// don't simplfy low-poly models too much
		if( numVerts <= 400 )
			verts_reduced = numVerts; // don't simplfy low-poly meshes!
		else if( numVerts <= 600 )
			verts_reduced = int( numVerts * SIMPLIFICATION_FACTOR_LOW );
		else if( numVerts <= 1500 )
			verts_reduced = int( numVerts * SIMPLIFICATION_FACTOR_MED );
		else verts_reduced = int( numVerts * SIMPLIFICATION_FACTOR_HIGH );

		for( i = 0; i < tris.num; i++ )
		{
			int p0 = MapVertex( tris[i].v[0], verts_reduced, collapse_map );
			int p1 = MapVertex( tris[i].v[1], verts_reduced, collapse_map );
			int p2 = MapVertex( tris[i].v[2], verts_reduced, collapse_map );

			if( p0 == p1 || p1 == p2 || p2 == p0 )
				continue; // degenerate

			// fill the triangle
			VectorCopy( vert[p0], triangle[0].point );
			VectorCopy( vert[p1], triangle[1].point );
			VectorCopy( vert[p2], triangle[2].point );

			triangle[0].st[0] = coords[p0*2+0];
			triangle[0].st[1] = coords[p0*2+1];
			triangle[1].st[0] = coords[p1*2+0];
			triangle[1].st[1] = coords[p1*2+1];
			triangle[2].st[0] = coords[p2*2+0];
			triangle[2].st[1] = coords[p2*2+1];

			// add it to mesh
			AddMeshTrinagle( triangle );
			tris_reduced++;
		}

		char mdlname[64];

		ExtractFileBase( pModel->name, mdlname );

		if( numVerts != verts_reduced )
			Developer( DEVELOPER_LEVEL_MESSAGE, "Model %s simplified ( verts %i -> %i, tris %i -> %i )\n", mdlname, numVerts, verts_reduced, numTris, tris_reduced );
	}
	else
	{
		for( i = 0; i < numElems; i += 3 )
		{
			// fill the triangle
			VectorCopy( verts[indices[i+0]], triangle[0].point );
			VectorCopy( verts[indices[i+1]], triangle[1].point );
			VectorCopy( verts[indices[i+2]], triangle[2].point );

			triangle[0].st[0] = coords[indices[i+0]*2+0];
			triangle[0].st[1] = coords[indices[i+0]*2+1];
			triangle[1].st[0] = coords[indices[i+1]*2+0];
			triangle[1].st[1] = coords[indices[i+1]*2+1];
			triangle[2].st[0] = coords[indices[i+2]*2+0];
			triangle[2].st[1] = coords[indices[i+2]*2+1];

			// add it to mesh
			AddMeshTrinagle( triangle, textures[indices[i]] );
		}
	}

	free( verts );
	free( coords );
	free( indices );
	free( textures );

	if( !FinishMeshBuild( ))
	{
		Developer( DEVELOPER_LEVEL_ERROR, "StudioConstructMesh: failed to build mesh from %s\n", pModel->name );
		return false;
	}
	profile.stop();
#if 1
	// g-cont. i'm leave this for debug
	Verbose( "%s: build time %g secs, size %s\n", m_debugName, profile.getTotal(), Q_memprint( mesh_size ));
#endif
	// done
	return true;
}

bool CMeshDesc :: AddMeshTrinagle( const mvert_t triangle[3], mstudiotexture_t *texture )
{
	int	i;

	if( m_iNumTris <= 0 )
		return false; // were not in a build mode!

	if( m_mesh.numfacets >= m_iNumTris )
	{
		Developer( DEVELOPER_LEVEL_ERROR, "AddMeshTriangle: %s overflow (%i >= %i)\n", m_debugName, m_mesh.numfacets, m_iNumTris );
		return false;
	}

	// add triangle to bounds
	for( i = 0; i < 3; i++ )
		AddPointToBounds( triangle[i].point, m_mesh.mins, m_mesh.maxs );

	mfacet_t *facet = &facets[m_mesh.numfacets];
	mplane_t mainplane;

	// calculate plane for this triangle
	PlaneFromPoints( triangle, &mainplane );

	if( ComparePlanes( &mainplane, vec3_origin, 0.0f ))
		return false; // bad plane

	mplane_t planes[MAX_FACET_PLANES];
	vec3_t normal;
	int numplanes;
	float dist;

	facet->numplanes = numplanes = 0;
	facet->texture = texture;

	// add front plane
	SnapPlaneToGrid( &mainplane );

	VectorCopy( mainplane.normal, planes[numplanes].normal );
	planes[numplanes].dist = mainplane.dist;
	numplanes++;

	// calculate mins & maxs
	ClearBounds( facet->mins, facet->maxs );

	for( i = 0; i < 3; i++ )
	{
		AddPointToBounds( triangle[i].point, facet->mins, facet->maxs );
		facet->triangle[i] = triangle[i];
	}

	VectorSubtract( facet->triangle[1].point, facet->triangle[0].point, facet->edge1 );
	VectorSubtract( facet->triangle[2].point, facet->triangle[0].point, facet->edge2 );

	// add the axial planes
	for( int axis = 0; axis < 3; axis++ )
	{
		for( int dir = -1; dir <= 1; dir += 2 )
		{
			for( i = 0; i < numplanes; i++ )
			{
				if( planes[i].normal[axis] == dir )
					break;
			}

			if( i == numplanes )
			{
				VectorClear( normal );
				normal[axis] = dir;
				if( dir == 1 )
					dist = facet->maxs[axis];
				else dist = -facet->mins[axis];

				VectorCopy( normal, planes[numplanes].normal );
				planes[numplanes].dist = dist;
				numplanes++;
			}
		}
	}

	// add the edge bevels
	for( i = 0; i < 3; i++ )
	{
		int j = (i + 1) % 3;
		vec3_t vec;

		VectorSubtract( triangle[i].point, triangle[j].point, vec );
		if( VectorLength( vec ) < 0.5f ) continue;

		VectorNormalize( vec );
		SnapVectorToGrid( vec );

		for( j = 0; j < 3; j++ )
		{
			if( vec[j] == 1.0f || vec[j] == -1.0f )
				break; // axial
		}

		if( j != 3 ) continue; // only test non-axial edges

		// try the six possible slanted axials from this edge
		for( int axis = 0; axis < 3; axis++ )
		{
			for( int dir = -1; dir <= 1; dir += 2 )
			{
				// construct a plane
				vec3_t vec2 = { 0.0f, 0.0f, 0.0f };
				vec2[axis] = dir;
				CrossProduct( vec, vec2, normal );

				if( VectorLength( normal ) < 0.5f )
					continue;

				VectorNormalize( normal );
				dist = DotProduct( triangle[i].point, normal );

				for( j = 0; j < numplanes; j++ )
				{
					// if this plane has already been used, skip it
					if( ComparePlanes( &planes[j], normal, dist ))
						break;
				}

				if( j != numplanes ) continue;

				// if all other points are behind this plane, it is a proper edge bevel
				for( j = 0; j < 3; j++ )
				{
					if( j != i )
					{
						float d = DotProduct( triangle[j].point, normal ) - dist;
						// point in front: this plane isn't part of the outer hull
						if( d > 0.1f ) break;
					}
				}

				if( j != 3 ) continue;

				// add this plane
				VectorCopy( normal, planes[numplanes].normal );
				planes[numplanes].dist = dist;
				numplanes++;
			}
		}
	}

	facet->indices = (uint *)malloc( sizeof( uint ) * numplanes );
	facet->numplanes = numplanes;

	for( i = 0; i < facet->numplanes; i++ )
	{
		SnapPlaneToGrid( &planes[i] );
		CategorizePlane( &planes[i] );

		// add plane to global pool
		facet->indices[i] = AddPlaneToPool( &planes[i] );
	}

#ifdef AABB_OFFSET
	for( i = 0; i < 3; i++ )
	{
		// spread the mins / maxs by a pixel
		facet->mins[i] -= 1.0f;
		facet->maxs[i] += 1.0f;
	}
#endif
	// added
	m_mesh.numfacets++;
	m_iTotalPlanes += numplanes;

	return true;
}

void CMeshDesc :: RelinkFacet( mfacet_t *facet )
{
	// find the first node that the facet box crosses
	areanode_t *node = areanodes;

	while( 1 )
	{
		if( node->axis == -1 ) break;
		if( facet->mins[node->axis] > node->dist )
			node = node->children[0];
		else if( facet->maxs[node->axis] < node->dist )
			node = node->children[1];
		else break; // crosses the node
	}
	
	// link it in	
	InsertLinkBefore( &facet->area, &node->facets );
}

bool CMeshDesc :: FinishMeshBuild( void )
{
	if( m_mesh.numfacets <= 0 )
	{
		FreeMesh();
		Developer( DEVELOPER_LEVEL_ERROR, "FinishMeshBuild: failed to build triangle mesh (no sides)\n" );
		return false;
	}
	int i;

#ifdef AABB_OFFSET
	for( i = 0; i < 3; i++ )
	{
		// spread the mins / maxs by a pixel
		m_mesh.mins[i] -= 1.0f;
		m_mesh.maxs[i] += 1.0f;
	}
#endif
	size_t memsize = ( sizeof( mfacet_t ) * m_mesh.numfacets) + (sizeof( mplane_t ) * m_mesh.numplanes) + (sizeof( uint ) * m_iTotalPlanes );

	// create non-fragmented memory piece and move mesh
	byte *buffer = (byte *)malloc( memsize );
	byte *bufend = buffer + memsize;

	// setup pointers
	m_mesh.planes = (mplane_t *)buffer; // so we free mem with planes
	buffer += (sizeof( mplane_t ) * m_mesh.numplanes);
	m_mesh.facets = (mfacet_t *)buffer;
	buffer += (sizeof( mfacet_t ) * m_mesh.numfacets);

	// setup mesh pointers
	for( i = 0; i < m_mesh.numfacets; i++ )
	{
		m_mesh.facets[i].indices = (uint *)buffer;
		buffer += (sizeof( uint ) * facets[i].numplanes);
	}

	if( buffer != bufend )
		Developer( DEVELOPER_LEVEL_ERROR, "FinishMeshBuild: memory representation error! %x != %x\n", buffer, bufend );

	// copy planes into mesh array (probably aligned block)
	for( i = 0; i < m_mesh.numplanes; i++ )
		m_mesh.planes[i] = planepool[i].pl;

	// copy planes into mesh array (probably aligned block)
	for( i = 0; i < m_mesh.numfacets; i++ )
	{
		VectorCopy( facets[i].mins, m_mesh.facets[i].mins );
		VectorCopy( facets[i].maxs, m_mesh.facets[i].maxs );
		VectorCopy( facets[i].edge1, m_mesh.facets[i].edge1 );
		VectorCopy( facets[i].edge2, m_mesh.facets[i].edge2 );
		m_mesh.facets[i].area.next = m_mesh.facets[i].area.prev = NULL;
		m_mesh.facets[i].numplanes = facets[i].numplanes;
		m_mesh.facets[i].texture = facets[i].texture;

		for( int j = 0; j < facets[i].numplanes; j++ )
			m_mesh.facets[i].indices[j] = facets[i].indices[j];

		for( int k = 0; k < 3; k++ )
			m_mesh.facets[i].triangle[k] = facets[i].triangle[k];
	}

	if( has_tree )
	{
		// create tree
		CreateAreaNode( 0, m_mesh.mins, m_mesh.maxs );

		for( int i = 0; i < m_mesh.numfacets; i++ )
			RelinkFacet( &m_mesh.facets[i] );
	}

	FreeMeshBuild();

	mesh_size = sizeof( m_mesh ) + memsize;
#if 0
	Developer( DEVELOPER_LEVEL_ALWAYS, "FinishMesh: %s %i k", m_debugName, ( mesh_size / 1024 ));
	Developer( DEVELOPER_LEVEL_ALWAYS, " (planes reduced from %i to %i)", m_iTotalPlanes, m_mesh.numplanes );
	Developer( DEVELOPER_LEVEL_ALWAYS, "\n" );
#endif
	return true;
}

void CMeshDesc :: FreeMeshBuild( void )
{
	// no reason to keep these arrays
	for( int i = 0; facets && i < m_mesh.numfacets; i++ )
		free( facets[i].indices );

	free( planehash );
	free( planepool );
	free( facets );

	planehash = NULL;
	planepool = NULL;
	facets = NULL;
}