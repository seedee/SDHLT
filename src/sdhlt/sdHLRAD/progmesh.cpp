/*
 *  Progressive Mesh type Polygon Reduction Algorithm
 *  by Stan Melax (c) 1998
 *  Permission to use any of this code wherever you want is granted..
 *  Although, please do acknowledge authorship if appropriate.
 *
 *  See the header file progmesh.h for a description of this module
 */

#include "qrad.h"
#include "meshdesc.h"

/*
 *  For the polygon reduction algorithm we use data structures
 *  that contain a little bit more information than the usual
 *  indexed face set type of data structure.
 *  From a vertex we wish to be able to quickly get the
 *  neighboring faces and vertices.
 */
 
class CTriangle;
class CVertex;

class CTriangle
{
public:
	CVertex*	vertex[3]; // the 3 points that make this tri
	vector	normal;    // unit vector othogonal to this face

	CTriangle( CVertex *v0, CVertex *v1, CVertex *v2 );
	~CTriangle();

	void ComputeNormal();
	void ReplaceVertex( CVertex *vold, CVertex *vnew );
	int HasVertex( CVertex *v );
};

class CVertex
{
public:
	vector		position; // location of point in euclidean space
	int		id;       // place of vertex in original list
	List<CVertex *>	neighbor; // adjacent vertices
	List<CTriangle *>	face;     // adjacent triangles
	float		objdist;  // cached cost of collapsing edge
	CVertex*		collapse; // candidate vertex for collapse

	CVertex( const vector &v, int _id );
	~CVertex();
	void RemoveIfNonNeighbor( CVertex *n );
};

List<CVertex *> vertices;
List<CTriangle *> triangles;

CTriangle :: CTriangle( CVertex *v0, CVertex *v1, CVertex *v2 )
{
	assert( v0 != v1 && v1 != v2 && v2 != v0 );

	vertex[0] = v0;
	vertex[1] = v1;
	vertex[2] = v2;

	ComputeNormal();

	triangles.Add( this );

	for( int i = 0; i < 3; i++ )
	{
		vertex[i]->face.Add( this );

		for( int j = 0; j < 3; j++ )
		{
			if( i == j ) continue;

			vertex[i]->neighbor.AddUnique( vertex[j] );
		}
	}
}

CTriangle :: ~CTriangle()
{
	int i1;

	triangles.Remove( this );

	for( i1 = 0; i1 < 3; i1++ )
	{
		if( vertex[i1] )
			vertex[i1]->face.Remove( this );
	}

	for( i1 = 0; i1 < 3; i1++ )
	{
		int i2 = (i1+1) % 3;

		if( !vertex[i1] || !vertex[i2] )
			continue;

		vertex[i1]->RemoveIfNonNeighbor( vertex[i2] );
		vertex[i2]->RemoveIfNonNeighbor( vertex[i1] );
	}
}

int CTriangle :: HasVertex( CVertex *v )
{
	return (v == vertex[0] || v == vertex[1] || v == vertex[2]);
}

void CTriangle :: ComputeNormal( void )
{
	vector	a, b;

	vector v0 = vertex[0]->position;
	vector v1 = vertex[1]->position;
	vector v2 = vertex[2]->position;

	VectorSubtract( v1, v0, a );
	VectorSubtract( v2, v1, b );
	CrossProduct( a, b, normal );

	if( VectorLength( normal ) == 0.0f )
		return;

	VectorNormalize( normal );
}

void CTriangle :: ReplaceVertex( CVertex *vold, CVertex *vnew )
{
	assert( vold && vnew );
	assert( vold == vertex[0] || vold == vertex[1] || vold == vertex[2] );
	assert( vnew != vertex[0] && vnew != vertex[1] && vnew != vertex[2] );

	if( vold == vertex[0] )
	{
		vertex[0] = vnew;
	}
	else if( vold == vertex[1] )
	{
		vertex[1] = vnew;
	}
	else
	{
		assert( vold == vertex[2] );
		vertex[2] = vnew;
	}

	vold->face.Remove( this );
	assert( !vnew->face.Contains( this ));
	vnew->face.Add( this );

	int i;

	for( i = 0; i < 3; i++ )
	{
		vold->RemoveIfNonNeighbor( vertex[i] );
		vertex[i]->RemoveIfNonNeighbor( vold );
	}

	for( i = 0; i < 3; i++ )
	{
		assert( vertex[i]->face.Contains( this ) == 1 );
		for( int j = 0; j < 3; j++ )
		{
			if( i == j ) continue;
			vertex[i]->neighbor.AddUnique( vertex[j] );
		}
	}

	ComputeNormal();
}

CVertex :: CVertex( const vector &v, int _id )
{
	position = v;
	id = _id;

	vertices.Add( this );
}

CVertex :: ~CVertex()
{
	assert( face.num == 0 );

	while( neighbor.num )
	{
		neighbor[0]->neighbor.Remove( this );
		neighbor.Remove( neighbor[0] );
	}

	vertices.Remove( this );
}

void CVertex :: RemoveIfNonNeighbor( CVertex *n )
{
	// removes n from neighbor list if n isn't a neighbor.
	if( !neighbor.Contains( n ))
		return;

	for( int i = 0; i < face.num; i++ )
	{
		if( face[i]->HasVertex( n ))
			return;
	}

	neighbor.Remove(n);
}

static float ComputeEdgeCollapseCost( CVertex *u, CVertex *v )
{
	// if we collapse edge uv by moving u to v then how 
	// much different will the model change, i.e. how much "error".
	// Texture, vertex normal, and border vertex code was removed
	// to keep this demo as simple as possible.
	// The method of determining cost was designed in order 
	// to exploit small and coplanar regions for
	// effective polygon reduction.
	// Is is possible to add some checks here to see if "folds"
	// would be generated.  i.e. normal of a remaining face gets
	// flipped.  I never seemed to run into this problem and
	// therefore never added code to detect this case.
	vector edgedir;

	VectorSubtract( v->position, u->position, edgedir );

	float edgelength = VectorLength( edgedir );
	float curvature = 0.0f;

	int i;

	// find the "sides" triangles that are on the edge uv
	List<CTriangle *> sides;

	for( i = 0; i < u->face.num; i++ )
	{
		if( u->face[i]->HasVertex( v ))
		{
			sides.Add( u->face[i] );
		}
	}

	// use the triangle facing most away from the sides 
	// to determine our curvature term
	for( i = 0; i < u->face.num; i++ )
	{
		float mincurv = 1.0f; // curve for face i and closer side to it

		for( int j = 0; j < sides.num; j++ )
		{
			float dotprod = DotProduct( u->face[i]->normal, sides[j]->normal );
			mincurv = qmin( mincurv, ( 1.0f - dotprod ) / 2.0f );
		}

		curvature = qmax( curvature, mincurv );
	}

	// the more coplanar the lower the curvature term   
	return edgelength * curvature;
}

static void ComputeEdgeCostAtVertex( CVertex *v )
{
	// compute the edge collapse cost for all edges that start
	// from vertex v.  Since we are only interested in reducing
	// the object by selecting the min cost edge at each step, we
	// only cache the cost of the least cost edge at this vertex
	// (in member variable collapse) as well as the value of the 
	// cost (in member variable objdist).

	if( v->neighbor.num == 0 )
	{
		// v doesn't have neighbors so it costs nothing to collapse
		v->collapse = NULL;
		v->objdist = -0.01f;
		return;
	}

	v->objdist = 1000000.0f;
	v->collapse = NULL;

	// search all neighboring edges for "least cost" edge
	for( int i = 0; i < v->neighbor.num; i++ )
	{
		float dist;
		dist = ComputeEdgeCollapseCost( v, v->neighbor[i] );

		if( dist < v->objdist )
		{
			v->collapse = v->neighbor[i];	// candidate for edge collapse
			v->objdist = dist;		// cost of the collapse
		}
	}
}

static void ComputeAllEdgeCollapseCosts( void )
{
	// For all the edges, compute the difference it would make
	// to the model if it was collapsed.  The least of these
	// per vertex is cached in each vertex object.
	for( int i = 0; i < vertices.num; i++ )
	{
		ComputeEdgeCostAtVertex( vertices[i] );
	}
}

static void Collapse( CVertex *u, CVertex *v )
{
	// Collapse the edge uv by moving vertex u onto v
	// Actually remove tris on uv, then update tris that
	// have u to have v, and then remove u.
	if( !v )
	{
		// u is a vertex all by itself so just delete it
		delete u;
		return;
	}

	int i;

	List<CVertex *>tmp;
	// make tmp a list of all the neighbors of u
	for( i = 0; i < u->neighbor.num; i++ )
	{
		tmp.Add( u->neighbor[i] );
	}

	// delete triangles on edge uv:
	for( i = u->face.num - 1; i >= 0; i-- )
	{
		if( u->face[i]->HasVertex( v ))
		{
			delete( u->face[i] );
		}
	}

	// update remaining triangles to have v instead of u
	for( i = u->face.num - 1; i >= 0; i-- )
	{
		u->face[i]->ReplaceVertex( u, v );
	}

	delete u; 

	// recompute the edge collapse costs for neighboring vertices
	for( i = 0; i < tmp.num; i++ )
	{
		ComputeEdgeCostAtVertex( tmp[i] );
	}
}

static void AddVertex( List<vector> &vert )
{
	for( int i = 0; i < vert.num; i++ )
	{
		CVertex *v = new CVertex( vert[i], i );
	}
}

static void AddFaces( List<triset> &tri )
{
	for( int i = 0; i < tri.num; i++ )
	{
		CTriangle *t = new CTriangle( vertices[tri[i].v[0]], vertices[tri[i].v[1]], vertices[tri[i].v[2]] );
	}
}

static CVertex *MinimumCostEdge( void )
{
	// Find the edge that when collapsed will affect model the least.
	// This funtion actually returns a CVertex, the second vertex
	// of the edge (collapse candidate) is stored in the vertex data.
	// Serious optimization opportunity here: this function currently
	// does a sequential search through an unsorted list :-(
	// Our algorithm could be O(n*lg(n)) instead of O(n*n)
	CVertex *mn = vertices[0];
	
	for( int i = 0; i < vertices.num; i++ )
	{
		if( vertices[i]->objdist < mn->objdist )
		{
			mn = vertices[i];
		}
	}

	return mn;
}

void ProgressiveMesh( List<vector> &vert, List<triset> &tri, List<int> &map, List<int> &permutation )
{
	AddVertex( vert );  // put input data into our data structures
	AddFaces( tri );

	ComputeAllEdgeCollapseCosts();	// cache all edge collapse costs
	permutation.SetSize( vertices.num );	// allocate space
	map.SetSize( vertices.num );		// allocate space

	// reduce the object down to nothing:
	while( vertices.num > 0 )
	{
		// get the next vertex to collapse
		CVertex *mn = MinimumCostEdge();
		// keep track of this vertex, i.e. the collapse ordering
		permutation[mn->id] = vertices.num - 1;
		// keep track of vertex to which we collapse to
		map[vertices.num-1] = (mn->collapse) ? mn->collapse->id : -1;
		// Collapse this edge
		Collapse( mn, mn->collapse );
	}

	// reorder the map list based on the collapse ordering
	for( int i = 0; i < map.num; i++ )
	{
		map[i] = (map[i] == -1 ) ? 0 : permutation[map[i]];
	}

	// The caller of this function should reorder their vertices
	// according to the returned "permutation".
}

void PermuteVertices( List<int> &permutation, List<vector> &vert, List<triset> &tris )
{
	assert( permutation.num == vert.num );

	// rearrange the vertex list 
	List<vector> temp_list;

	int i;

	for( i = 0; i < vert.num; i++ )
	{
		temp_list.Add( vert[i] );
	}

	for( i = 0; i < vert.num; i++ )
	{
		vert[permutation[i]] = temp_list[i];
	}

	// update the changes in the entries in the triangle list
	for( i = 0; i < tris.num; i++ )
	{
		for( int j = 0; j < 3; j++ )
		{
			tris[i].v[j] = permutation[tris[i].v[j]];
		}
	}
}

int MapVertex( int a, int mx, List<int> &map )
{
	if( mx <= 0 ) return 0;

	while( a >= mx ) { a = map[a]; }

	return a;
}