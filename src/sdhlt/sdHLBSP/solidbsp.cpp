//#pragma warning(disable: 4018) // '<' : signed/unsigned mismatch

#include "bsp5.h"

//  FaceSide
//  ChooseMidPlaneFromList
//  ChoosePlaneFromList
//  SelectPartition

//  CalcSurfaceInfo
//  DivideSurface
//  SplitNodeSurfaces
//  RankForContents
//  ContentsForRank

//  FreeLeafSurfs
//  LinkLeafFaces
//  MakeNodePortal
//  SplitNodePortals
//  CalcNodeBounds
//  CopyFacesToNode
//  BuildBspTree_r
//  SolidBSP

//  Each node or leaf will have a set of portals that completely enclose
//  the volume of the node and pass into an adjacent node.
#include <vector>

int             g_maxnode_size = DEFAULT_MAXNODE_SIZE;

static bool g_reportProgress = false;
static int  g_numProcessed = 0;
static int  g_numReported = 0;

static void ResetStatus(bool report_progress)
{
	g_reportProgress = report_progress;
	g_numProcessed = g_numReported = 0;
}

static void UpdateStatus(void)
{
	if(g_reportProgress)
	{
		++g_numProcessed;
		if((g_numProcessed / 500) > g_numReported)
		{
			g_numReported = (g_numProcessed / 500);
			Log("%d...",g_numProcessed);
		}
	}
}	

// =====================================================================================
//  FaceSide
//      For BSP hueristic
// =====================================================================================
static int      FaceSide(face_t* in, const dplane_t* const split
						 , double *epsilonsplit = NULL
						 )
{
	const vec_t		epsilonmin = 0.002, epsilonmax = 0.2;
	vec_t			d_front, d_back;
    vec_t           dot;
    int             i;
    vec_t*          p;

	d_front = d_back = 0;

    // axial planes are fast
    if (split->type <= last_axial)
    {
        vec_t           splitGtEp = split->dist + ON_EPSILON;   // Invariant moved out of loop
        vec_t           splitLtEp = split->dist - ON_EPSILON;   // Invariant moved out of loop

        for (i = 0, p = in->pts[0] + split->type; i < in->numpoints; i++, p += 3)
        {
			dot = *p - split->dist;
			if (dot > d_front)
				d_front = dot;
			if (dot < d_back)
				d_back = dot;
        }
    }
    else
    {
        // sloping planes take longer
        for (i = 0, p = in->pts[0]; i < in->numpoints; i++, p += 3)
        {
            dot = DotProduct(p, split->normal);
            dot -= split->dist;
			if (dot > d_front)
				d_front = dot;
			if (dot < d_back)
				d_back = dot;
        }
    }
	if (d_front <= ON_EPSILON)
	{
		if (d_front > epsilonmin || d_back > -epsilonmax)
		{
			if (epsilonsplit)
				(*epsilonsplit)++;
		}
		return SIDE_BACK;
	}
	if (d_back >= -ON_EPSILON)
	{
		if (d_back < -epsilonmin || d_front < epsilonmax)
		{
			if (epsilonsplit)
				(*epsilonsplit)++;
		}
		return SIDE_FRONT;
	}
	if (d_front < epsilonmax || d_back > -epsilonmax)
	{
		if (epsilonsplit)
			(*epsilonsplit)++;
	}
	return SIDE_ON;
}

// organize all surfaces into a tree structure to accelerate intersection test
// can reduce more than 90% compile time for very complicated maps

typedef struct surfacetreenode_s
{
	int size; // can be zero, which invalidates mins and maxs
	int size_discardable;
	vec3_t mins;
	vec3_t maxs;
	bool isleaf;
	// node
	surfacetreenode_s *children[2];
	std::vector< face_t * > *nodefaces;
	int nodefaces_discardablesize;
	// leaf
	std::vector< face_t * > *leaffaces;
}
surfacetreenode_t;

typedef struct
{
	bool dontbuild;
	vec_t epsilon; // if a face is not epsilon far from the splitting plane, put it in result.middle
	surfacetreenode_t *headnode;
	struct
	{
		int frontsize;
		int backsize;
		std::vector< face_t * > *middle; // may contains coplanar faces and discardable(SOLIDHINT) faces
	}
	result; // "public"
}
surfacetree_t;

void BuildSurfaceTree_r (surfacetree_t *tree, surfacetreenode_t *node)
{
	node->size = node->leaffaces->size ();
	node->size_discardable = 0;
	if (node->size == 0)
	{
		node->isleaf = true;
		return;
	}

	VectorFill (node->mins, BOGUS_RANGE);
	VectorFill (node->maxs, -BOGUS_RANGE);
	for (std::vector< face_t * >::iterator i = node->leaffaces->begin (); i != node->leaffaces->end (); ++i)
	{
		face_t *f = *i;
		for (int x = 0; x < f->numpoints; x++)
		{
			VectorCompareMinimum (node->mins, f->pts[x], node->mins);
			VectorCompareMaximum (node->maxs, f->pts[x], node->maxs);
		}
		if (f->facestyle == face_discardable)
		{
			node->size_discardable++;
		}
	}

	int bestaxis = -1;
	{
		vec_t bestdelta = 0;
		for (int k = 0; k < 3; k++)
		{
			if (node->maxs[k] - node->mins[k] > bestdelta + ON_EPSILON)
			{
				bestaxis = k;
				bestdelta = node->maxs[k] - node->mins[k];
			}
		}
	}
	if (node->size <= 5 || tree->dontbuild == true || bestaxis == -1)
	{
		node->isleaf = true;
		return;
	}

	node->isleaf = false;
	vec_t dist, dist1, dist2;
	dist = (node->mins[bestaxis] + node->maxs[bestaxis]) / 2;
	dist1 = (3 * node->mins[bestaxis] + node->maxs[bestaxis]) / 4;
	dist2 = (node->mins[bestaxis] + 3 * node->maxs[bestaxis]) / 4;
	// Each child node is at most 3/4 the size of the parent node.
	// Most faces should be passed to a child node, faces left in the parent node are the ones whose dimensions are large enough to be comparable to the dimension of the parent node.
	node->nodefaces = new std::vector< face_t * >;
	node->nodefaces_discardablesize = 0;
	node->children[0] = (surfacetreenode_t *)malloc (sizeof (surfacetreenode_t));
	node->children[0]->leaffaces = new std::vector< face_t * >;
	node->children[1] = (surfacetreenode_t *)malloc (sizeof (surfacetreenode_t));
	node->children[1]->leaffaces = new std::vector< face_t * >;
	for (std::vector< face_t * >::iterator i = node->leaffaces->begin (); i != node->leaffaces->end (); ++i)
	{
		face_t *f = *i;
		vec_t low = BOGUS_RANGE;
		vec_t high = -BOGUS_RANGE;
		for (int x = 0; x < f->numpoints; x++)
		{
			low = qmin (low, f->pts[x][bestaxis]);
			high = qmax (high, f->pts[x][bestaxis]);
		}
		if (low < dist1 + ON_EPSILON && high > dist2 - ON_EPSILON)
		{
			node->nodefaces->push_back (f);
			if (f->facestyle == face_discardable)
			{
				node->nodefaces_discardablesize++;
			}
		}
		else if (low >= dist1 && high <= dist2)
		{
			if ((low + high) / 2 > dist)
			{
				node->children[0]->leaffaces->push_back (f);
			}
			else
			{
				node->children[1]->leaffaces->push_back (f);
			}
		}
		else if (low >= dist1)
		{
			node->children[0]->leaffaces->push_back (f);
		}
		else if (high <= dist2)
		{
			node->children[1]->leaffaces->push_back (f);
		}
	}
	if (node->children[0]->leaffaces->size () == node->leaffaces->size () || node->children[1]->leaffaces->size () == node->leaffaces->size ())
	{
		Warning ("BuildSurfaceTree_r: didn't split node with bound (%f,%f,%f)-(%f,%f,%f)", node->mins[0], node->mins[1], node->mins[2], node->maxs[0], node->maxs[1], node->maxs[2]);
		delete node->children[0]->leaffaces;
		delete node->children[1]->leaffaces;
		free (node->children[0]);
		free (node->children[1]);
		delete node->nodefaces;
		node->isleaf = true;
		return;
	}
	delete node->leaffaces;
	BuildSurfaceTree_r (tree, node->children[0]);
	BuildSurfaceTree_r (tree, node->children[1]);
}

surfacetree_t *BuildSurfaceTree (surface_t *surfaces, vec_t epsilon)
{
	surfacetree_t *tree;
	tree = (surfacetree_t *)malloc (sizeof (surfacetree_t));
	tree->epsilon = epsilon;
	tree->result.middle = new std::vector< face_t * >;
	tree->headnode = (surfacetreenode_t *)malloc (sizeof (surfacetreenode_t));
	tree->headnode->leaffaces = new std::vector< face_t * >;
	{
		surface_t *p2;
		face_t *f;
		for (p2 = surfaces; p2; p2 = p2->next)
		{
			if (p2->onnode)
			{
				continue;
			}
			for (f = p2->faces; f; f = f->next)
			{
				tree->headnode->leaffaces->push_back (f);
			}
		}
	}
	tree->dontbuild = tree->headnode->leaffaces->size () < 20;
	BuildSurfaceTree_r (tree, tree->headnode);
	if (tree->dontbuild)
	{
		*tree->result.middle = *tree->headnode->leaffaces;
		tree->result.backsize = 0;
		tree->result.frontsize = 0;
	}
	return tree;
}

void TestSurfaceTree_r (surfacetree_t *tree, const surfacetreenode_t *node, const dplane_t *split)
{
	if (node->size == 0)
	{
		return;
	}
	vec_t low, high;
	low = high = -split->dist;
	for (int k = 0; k < 3; k++)
	{
		if (split->normal[k] >= 0)
		{
			high += split->normal[k] * node->maxs[k];
			low += split->normal[k] * node->mins[k];
		}
		else
		{
			high += split->normal[k] * node->mins[k];
			low += split->normal[k] * node->maxs[k];
		}
	}
	if (low > tree->epsilon)
	{
		tree->result.frontsize += node->size;
		tree->result.frontsize -= node->size_discardable;
		return;
	}
	if (high < -tree->epsilon)
	{
		tree->result.backsize += node->size;
		tree->result.backsize -= node->size_discardable;
		return;
	}
	if (node->isleaf)
	{
		for (std::vector< face_t * >::iterator i = node->leaffaces->begin (); i != node->leaffaces->end (); ++i)
		{
			tree->result.middle->push_back (*i);
		}
	}
	else
	{
		for (std::vector< face_t * >::iterator i = node->nodefaces->begin (); i != node->nodefaces->end (); ++i)
		{
			tree->result.middle->push_back (*i);
		}
		TestSurfaceTree_r (tree, node->children[0], split);
		TestSurfaceTree_r (tree, node->children[1], split);
	}
}

void TestSurfaceTree (surfacetree_t *tree, const dplane_t *split)
{
	if (tree->dontbuild)
	{
		return;
	}
	tree->result.middle->clear ();
	tree->result.backsize = 0;
	tree->result.frontsize = 0;
	TestSurfaceTree_r (tree, tree->headnode, split);
}

void DeleteSurfaceTree_r (surfacetreenode_t *node)
{
	if (node->isleaf)
	{
		delete node->leaffaces;
	}
	else
	{
		DeleteSurfaceTree_r (node->children[0]);
		free (node->children[0]);
		DeleteSurfaceTree_r (node->children[1]);
		free (node->children[1]);
		delete node->nodefaces;
	}
}

void DeleteSurfaceTree (surfacetree_t *tree)
{
	DeleteSurfaceTree_r (tree->headnode);
	free (tree->headnode);
	delete tree->result.middle;
	free (tree);
}

// =====================================================================================
//  ChooseMidPlaneFromList
//      When there are a huge number of planes, just choose one closest
//      to the middle.
// =====================================================================================
static surface_t* ChooseMidPlaneFromList(surface_t* surfaces, const vec3_t mins, const vec3_t maxs
										 , int detaillevel
										 )
{
    int             j, l;
    surface_t*      p;
    surface_t*      bestsurface;
    vec_t           bestvalue;
    vec_t           value;
    vec_t           dist;
    dplane_t*       plane;
	surfacetree_t*	surfacetree;
	std::vector< face_t * >::iterator it;
	face_t*			f;

	surfacetree = BuildSurfaceTree (surfaces, ON_EPSILON);

    //
    // pick the plane that splits the least
    //
	bestvalue = 9e30;
    bestsurface = NULL;

    for (p = surfaces; p; p = p->next)
    {
        if (p->onnode)
        {
            continue;
        }
		if (p->detaillevel != detaillevel)
		{
			continue;
		}

        plane = &g_dplanes[p->planenum];

        // check for axis aligned surfaces
        l = plane->type;
        if (l > last_axial)
        {
            continue;
        }

        //
        // calculate the split metric along axis l, smaller values are better
        //
        value = 0;

        dist = plane->dist * plane->normal[l];
		if (maxs[l] - dist < ON_EPSILON || dist - mins[l] < ON_EPSILON)
			continue;
		if (maxs[l] - dist < g_maxnode_size/2.0 - ON_EPSILON || dist - mins[l] < g_maxnode_size/2.0 - ON_EPSILON)
			continue;
		double crosscount = 0;
		double frontcount = 0;
		double backcount = 0;
		double coplanarcount = 0;

		TestSurfaceTree (surfacetree, plane);
		frontcount += surfacetree->result.frontsize;
		backcount += surfacetree->result.backsize;
		for (it = surfacetree->result.middle->begin (); it != surfacetree->result.middle->end (); ++it)
		{
			f = *it;
			if (f->facestyle == face_discardable)
			{
				continue;
			}
			if (f->planenum == p->planenum || f->planenum == (p->planenum ^ 1))
			{
				coplanarcount++;
				continue;
			}
			switch (FaceSide (f, plane))
			{
			case SIDE_FRONT:
				frontcount++;
				break;
			case SIDE_BACK:
				backcount++;
				break;
			case SIDE_ON:
				crosscount++;
				break;
			}
		}

		double frontsize = frontcount + 0.5 * coplanarcount + 0.5 * crosscount;
		double frontfrac = (maxs[l] - dist) / (maxs[l] - mins[l]);
		double backsize = backcount + 0.5 * coplanarcount + 0.5 * crosscount;
		double backfrac = (dist - mins[l]) / (maxs[l] - mins[l]);
		value = crosscount + 0.1 * (frontsize * (log (frontfrac) / log (2.0)) + backsize * (log (backfrac) / log (2.0)));
		// the first part is how the split will increase the number of faces
		// the second part is how the split will increase the average depth of the bsp tree

        if (value > bestvalue)
        {
            continue;
        }

        //
        // currently the best!
        //
        bestvalue = value;
        bestsurface = p;
    }

	DeleteSurfaceTree (surfacetree);
    if (!bestsurface)
    {
		return NULL;
    }

    return bestsurface;
}

// =====================================================================================
//  ChoosePlaneFromList
//      Choose the plane that splits the least faces
// =====================================================================================
static surface_t* ChoosePlaneFromList(surface_t* surfaces, const vec3_t mins, const vec3_t maxs
									  // mins and maxs are invalid when detaillevel > 0
									  , int detaillevel
									  )
{
	surface_t*      p;
	surface_t*      p2;
	surface_t*      bestsurface;
	vec_t           bestvalue;
	vec_t           value;
	dplane_t*       plane;
	face_t*         f;
	double			planecount;
	double			totalsplit;
	double			avesplit;
	double			(*tmpvalue)[2];
	surfacetree_t*	surfacetree;
	std::vector< face_t * >::iterator it;

	planecount = 0;
	totalsplit = 0;
	tmpvalue = (double (*)[2])malloc (g_numplanes * sizeof (double [2]));
	surfacetree = BuildSurfaceTree (surfaces, ON_EPSILON);

	//
	// pick the plane that splits the least
	//
	bestvalue = 9e30;
	bestsurface = NULL;

	for (p = surfaces; p; p = p->next)
	{
		if (p->onnode)
		{
			continue;
		}
		if (p->detaillevel != detaillevel)
		{
			continue;
		}
		planecount++;

		double crosscount = 0; // use double here because we need to perform "crosscount++"
		double frontcount = 0;
		double backcount = 0;
		double coplanarcount = 0;
		double epsilonsplit = 0;

		plane = &g_dplanes[p->planenum];

		for (f = p->faces; f; f = f->next)
		{
			if (f->facestyle == face_discardable)
			{
				continue;
			}
			coplanarcount++;
		}
		TestSurfaceTree (surfacetree, plane);
		{
			frontcount += surfacetree->result.frontsize;
			backcount += surfacetree->result.backsize;
			for (it = surfacetree->result.middle->begin (); it != surfacetree->result.middle->end (); ++it)
			{
				f = *it;
				if (f->planenum == p->planenum || f->planenum == (p->planenum ^ 1))
				{
					continue;
				}
				if (f->facestyle == face_discardable)
				{
					FaceSide (f, plane, &epsilonsplit);
					continue;
				}
				switch (FaceSide(f, plane
					, &epsilonsplit
					))
				{
				case SIDE_FRONT:
					frontcount++;
					break;
				case SIDE_BACK:
					backcount++;
					break;
				case SIDE_ON:
					totalsplit++;
					crosscount++;
					break;
				}
			}
		}

		value = crosscount - sqrt (coplanarcount); // Not optimized. --vluzacn
		if (coplanarcount == 0)
		{
			crosscount += 1;
		}
		// This is the most efficient code among what I have ever tested:
		// (1) BSP file is small, despite possibility of slowing down vis and rad (but still faster than the original non BSP balancing method).
		// (2) Factors need not adjust across various maps.
		double frac = (coplanarcount / 2 + crosscount / 2 + frontcount) / (coplanarcount + frontcount + backcount + crosscount);
		double ent = (0.0001 < frac && frac < 0.9999)? (- frac * log (frac) / log (2.0) - (1 - frac) * log (1 - frac) / log (2.0)): 0.0; // the formula tends to 0 when frac=0,1
		tmpvalue[p->planenum][1] = crosscount * (1 - ent);
		value += epsilonsplit * 10000;

		tmpvalue[p->planenum][0] = value;
	}
	avesplit = totalsplit / planecount;
	for (p = surfaces; p; p = p->next)
	{
		if (p->onnode)
		{
			continue;
		}
		if (p->detaillevel != detaillevel)
		{
			continue;
		}
		value = tmpvalue[p->planenum][0] + avesplit * tmpvalue[p->planenum][1];
		if (value < bestvalue)
		{
			bestvalue = value;
			bestsurface = p;
		}
	}

	if (!bestsurface)
		Error("ChoosePlaneFromList: no valid planes");
	free (tmpvalue);
	DeleteSurfaceTree (surfacetree);
	return bestsurface;
}

// =====================================================================================
//  SelectPartition
//      Selects a surface from a linked list of surfaces to split the group on
//      returns NULL if the surface list can not be divided any more (a leaf)
// =====================================================================================
int CalcSplitDetaillevel (const node_t *node)
{
	int bestdetaillevel = -1;
	surface_t *s;
	face_t *f;
	for (s = node->surfaces; s; s = s->next)
	{
		if (s->onnode)
		{
			continue;
		}
		for (f = s->faces; f; f = f->next)
		{
			if (f->facestyle == face_discardable)
			{
				continue;
			}
			if (bestdetaillevel == -1 || f->detaillevel < bestdetaillevel)
			{
				bestdetaillevel = f->detaillevel;
			}
		}
	}
	return bestdetaillevel;
}
static surface_t* SelectPartition(surface_t* surfaces, const node_t* const node, const bool usemidsplit
								  , int splitdetaillevel
								  , vec3_t validmins, vec3_t validmaxs
								  )
{
	if (splitdetaillevel == -1)
	{
		return NULL;
	}
	// now we MUST choose a surface of this detail level

	if (usemidsplit)
	{
		surface_t *s = ChooseMidPlaneFromList(surfaces, 
			validmins, validmaxs
			, splitdetaillevel
			);
		if (s != NULL)
			return s;
	}
	return ChoosePlaneFromList(surfaces, node->mins, node->maxs
		, splitdetaillevel
		);
}

// =====================================================================================
//  CalcSurfaceInfo
//      Calculates the bounding box
// =====================================================================================
static void     CalcSurfaceInfo(surface_t* surf)
{
    int             i;
    int             j;
    face_t*         f;

    hlassume(surf->faces != NULL, assume_ValidPointer);    // "CalcSurfaceInfo() surface without a face"

    //
    // calculate a bounding box
    //
    for (i = 0; i < 3; i++)
    {
        surf->mins[i] = 99999;
        surf->maxs[i] = -99999;
    }

	surf->detaillevel = -1;
    for (f = surf->faces; f; f = f->next)
    {
        if (f->contents >= 0)
        {
            Error("Bad contents");
        }
        for (i = 0; i < f->numpoints; i++)
        {
            for (j = 0; j < 3; j++)
            {
                if (f->pts[i][j] < surf->mins[j])
                {
                    surf->mins[j] = f->pts[i][j];
                }
                if (f->pts[i][j] > surf->maxs[j])
                {
                    surf->maxs[j] = f->pts[i][j];
                }
            }
        }
		if (surf->detaillevel == -1 || f->detaillevel < surf->detaillevel)
		{
			surf->detaillevel = f->detaillevel;
		}
    }
}
void FixDetaillevelForDiscardable (node_t *node, int detaillevel)
{
	// when we move on to the next detaillevel, some discardable faces of previous detail level remain not on node (because they are discardable). remove them now
	surface_t *s, **psnext;
	face_t *f, **pfnext;
	for (psnext = &node->surfaces; s = *psnext, s != NULL; )
	{
		if (s->onnode)
		{
			psnext = &s->next;
			continue;
		}
		hlassume (s->faces, assume_ValidPointer);
		for (pfnext = &s->faces; f = *pfnext, f != NULL; )
		{
			if (detaillevel == -1 || f->detaillevel < detaillevel)
			{
				*pfnext = f->next;
				FreeFace (f);
			}
			else
			{
				pfnext = &f->next;
			}
		}
		if (!s->faces)
		{
			*psnext = s->next;
			FreeSurface (s);
		}
		else
		{
			psnext = &s->next;
			CalcSurfaceInfo (s);
			hlassume (!(detaillevel == -1 || s->detaillevel < detaillevel), assume_first);
		}
	}
}

// =====================================================================================
//  DivideSurface
// =====================================================================================
static void     DivideSurface(surface_t* in, const dplane_t* const split, surface_t** front, surface_t** back)
{
    face_t*         facet;
    face_t*         next;
    face_t*         frontlist;
    face_t*         backlist;
    face_t*         frontfrag;
    face_t*         backfrag;
    surface_t*      news;
    dplane_t*       inplane;

    inplane = &g_dplanes[in->planenum];

    // parallel case is easy

    if (inplane->normal[0] == split->normal[0]
     && inplane->normal[1] == split->normal[1]
     && inplane->normal[2] == split->normal[2])
    {
        if (inplane->dist > split->dist)
        {
            *front = in;
            *back = NULL;
        }
        else if (inplane->dist < split->dist)
        {
            *front = NULL;
            *back = in;
        }
        else
        {                                                  // split the surface into front and back
            frontlist = NULL;
            backlist = NULL;
            for (facet = in->faces; facet; facet = next)
            {
                next = facet->next;
                if (facet->planenum & 1)
                {
                    facet->next = backlist;
                    backlist = facet;
                }
                else
                {
                    facet->next = frontlist;
                    frontlist = facet;
                }
            }
            goto makesurfs;
        }
        return;
    }

    // do a real split.  may still end up entirely on one side
    // OPTIMIZE: use bounding box for fast test
    frontlist = NULL;
    backlist = NULL;

    for (facet = in->faces; facet; facet = next)
    {
        next = facet->next;
        SplitFace(facet, split, &frontfrag, &backfrag);
        if (frontfrag)
        {
            frontfrag->next = frontlist;
            frontlist = frontfrag;
        }
        if (backfrag)
        {
            backfrag->next = backlist;
            backlist = backfrag;
        }
    }

    // if nothing actually got split, just move the in plane
makesurfs:
	if (frontlist == NULL && backlist == NULL)
	{
		*front = NULL;
		*back = NULL;
		return;
	}
    if (frontlist == NULL)
    {
        *front = NULL;
        *back = in;
        in->faces = backlist;
        return;
    }

    if (backlist == NULL)
    {
        *front = in;
        *back = NULL;
        in->faces = frontlist;
        return;
    }

    // stuff got split, so allocate one new surface and reuse in
    news = AllocSurface();
    *news = *in;
    news->faces = backlist;
    *back = news;

    in->faces = frontlist;
    *front = in;

    // recalc bboxes and flags
    CalcSurfaceInfo(news);
    CalcSurfaceInfo(in);
}

// =====================================================================================
//  SplitNodeSurfaces
// =====================================================================================
static void     SplitNodeSurfaces(surface_t* surfaces, const node_t* const node)
{
    surface_t*      p;
    surface_t*      next;
    surface_t*      frontlist;
    surface_t*      backlist;
    surface_t*      frontfrag;
    surface_t*      backfrag;
    dplane_t*       splitplane;

    splitplane = &g_dplanes[node->planenum];

    frontlist = NULL;
    backlist = NULL;
	
    for (p = surfaces; p; p = next)
    {
        next = p->next;
        DivideSurface(p, splitplane, &frontfrag, &backfrag);

        if (frontfrag)
        {
            if (!frontfrag->faces)
            {
                Error("surface with no faces");
            }
            frontfrag->next = frontlist;
            frontlist = frontfrag;
        }
        if (backfrag)
        {
            if (!backfrag->faces)
            {
                Error("surface with no faces");
            }
            backfrag->next = backlist;
            backlist = backfrag;
        }
    }

    node->children[0]->surfaces = frontlist;
    node->children[1]->surfaces = backlist;
}
static void SplitNodeBrushes (brush_t *brushes, const node_t *node)
{
	brush_t *frontlist, *frontfrag;
	brush_t *backlist, *backfrag;
	brush_t *b, *next;
	const dplane_t *splitplane;
	frontlist = NULL;
	backlist = NULL;
	splitplane = &g_dplanes[node->planenum];
	for (b = brushes; b; b = next)
	{
		next = b->next;
		SplitBrush (b, splitplane, &frontfrag, &backfrag);
		if (frontfrag)
		{
			frontfrag->next = frontlist;
			frontlist = frontfrag;
		}
		if (backfrag)
		{
			backfrag->next = backlist;
			backlist = backfrag;
		}
	}
	node->children[0]->detailbrushes = frontlist;
	node->children[1]->detailbrushes = backlist;
}

// =====================================================================================
//  RankForContents
// =====================================================================================
static int      RankForContents(const int contents)
{
    //Log("SolidBSP::RankForContents - contents type is %i ",contents);
    switch (contents)
    {

    case CONTENTS_EMPTY:
        //Log("(empty)\n");
        return 0;
    case CONTENTS_WATER:
        //Log("(water)\n");
        return 1;
    case CONTENTS_TRANSLUCENT:
        //Log("(traslucent)\n");
        return 2;
    case CONTENTS_CURRENT_0:
        //Log("(current_0)\n");
        return 3;
    case CONTENTS_CURRENT_90:
        //Log("(current_90)\n");
        return 4;
    case CONTENTS_CURRENT_180:
        //Log("(current_180)\n");
        return 5;
    case CONTENTS_CURRENT_270:
        //Log("(current_270)\n");
        return 6;
    case CONTENTS_CURRENT_UP:
        //Log("(current_up)\n");
        return 7;
    case CONTENTS_CURRENT_DOWN:
        //Log("(current_down)\n");
        return 8;
    case CONTENTS_SLIME:
        //Log("(slime)\n");
        return 9;
    case CONTENTS_LAVA:
        //Log("(lava)\n");
        return 10;
    case CONTENTS_SKY:
        //Log("(sky)\n");
        return 11;
    case CONTENTS_SOLID:
        //Log("(solid)\n");
        return 12;


    default:
        hlassert(false);
        Error("RankForContents: bad contents %i", contents);
    }
    return -1;
}

// =====================================================================================
//  ContentsForRank
// =====================================================================================
static int      ContentsForRank(const int rank)
{
    switch (rank)
    {

    case -1:
		return CONTENTS_EMPTY;
    case 0:
        return CONTENTS_EMPTY;
    case 1:
        return CONTENTS_WATER;
    case 2:
        return CONTENTS_TRANSLUCENT;
    case 3:
        return CONTENTS_CURRENT_0;
    case 4:
        return CONTENTS_CURRENT_90;
    case 5:
        return CONTENTS_CURRENT_180;
    case 6:
        return CONTENTS_CURRENT_270;
    case 7:
        return CONTENTS_CURRENT_UP;
    case 8:
        return CONTENTS_CURRENT_DOWN;
    case 9:
        return CONTENTS_SLIME;
    case 10:
        return CONTENTS_LAVA;
    case 11:
        return CONTENTS_SKY;
    case 12:
        return CONTENTS_SOLID;


    default:
        hlassert(false);
        Error("ContentsForRank: bad rank %i", rank);
    }
    return -1;
}

// =====================================================================================
//  FreeLeafSurfs
// =====================================================================================
static void     FreeLeafSurfs(node_t* leaf)
{
    surface_t*      surf;
    surface_t*      snext;
    face_t*         f;
    face_t*         fnext;

    for (surf = leaf->surfaces; surf; surf = snext)
    {
        snext = surf->next;
        for (f = surf->faces; f; f = fnext)
        {
            fnext = f->next;
            FreeFace(f);
        }
        FreeSurface(surf);
    }

    leaf->surfaces = NULL;
}
static void FreeLeafBrushes (node_t *leaf)
{
	brush_t *b, *next;
	for (b = leaf->detailbrushes; b; b = next)
	{
		next = b->next;
		FreeBrush (b);
	}
	leaf->detailbrushes = NULL;
}

// =====================================================================================
//  LinkLeafFaces
//      Determines the contents of the leaf and creates the final list of original faces 
//      that have some fragment inside this leaf
// =====================================================================================
#define	MAX_LEAF_FACES	16384

const char*     ContentsToString(int contents)
{
    switch (contents)
    {
    case CONTENTS_EMPTY:
        return "EMPTY";
    case CONTENTS_SOLID:
        return "SOLID";
    case CONTENTS_WATER:
        return "WATER";
    case CONTENTS_SLIME:
        return "SLIME";
    case CONTENTS_LAVA:
        return "LAVA";
    case CONTENTS_SKY:
        return "SKY";
    case CONTENTS_CURRENT_0:
        return "CURRENT_0";
    case CONTENTS_CURRENT_90:
        return "CURRENT_90";
    case CONTENTS_CURRENT_180:
        return "CURRENT_180";
    case CONTENTS_CURRENT_270:
        return "CURRENT_270";
    case CONTENTS_CURRENT_UP:
        return "CURRENT_UP";
    case CONTENTS_CURRENT_DOWN:
        return "CURRENT_DOWN";
    case CONTENTS_TRANSLUCENT:
        return "TRANSLUCENT";
    default:
        return "UNKNOWN";
    }
}
static void     LinkLeafFaces(surface_t* planelist, node_t* leafnode)
{
    face_t*         f;
    surface_t*      surf;
    int             rank, r;

    rank = -1;
    for (surf = planelist; surf; surf = surf->next)
    {
		if (!surf->onnode)
		{
			continue;
		}
        for (f = surf->faces; f; f = f->next)
        {
            if ((f->contents == CONTENTS_HINT))
            {
                f->contents = CONTENTS_EMPTY;
            }
			if (f->detaillevel)
			{
				continue;
			}
            r = RankForContents(f->contents);
            if (r > rank)
            {
                rank = r;
            }
        }
    }
	for (surf = planelist; surf; surf = surf->next)
	{
		if (!surf->onnode)
		{
			continue;
		}
		for (f = surf->faces; f; f = f->next)
		{
			if (f->detaillevel)
			{
				continue;
			}
			r = RankForContents(f->contents);
			if (r != rank)
				break;
		}
		if (f)
			break;
	}
	if (surf)
	{
		entity_t *ent = EntityForModel (g_nummodels - 1);
		if (g_nummodels - 1 != 0 && ent == &g_entities[0])
		{
			ent = NULL;
		}
		Warning ("Ambiguous leafnode content ( %s and %s ) at (%.0f,%.0f,%.0f)-(%.0f,%.0f,%.0f) in hull %d of model %d (entity: classname \"%s\", origin \"%s\", targetname \"%s\")", 
			ContentsToString (ContentsForRank(r)), ContentsToString (ContentsForRank(rank)), 
			leafnode->mins[0], leafnode->mins[1], leafnode->mins[2], leafnode->maxs[0], leafnode->maxs[1], leafnode->maxs[2], 
			g_hullnum, g_nummodels - 1, 
			(ent? ValueForKey (ent, "classname"): "unknown"), 
			(ent? ValueForKey (ent, "origin"): "unknown"), 
			(ent? ValueForKey (ent, "targetname"): "unknown"));
		for (surface_t *surf2 = planelist; surf2; surf2 = surf2->next)
		{
			for (face_t *f2 = surf2->faces; f2; f2 = f2->next)
			{
				Developer (DEVELOPER_LEVEL_SPAM, "content = %d plane = %d normal = (%g,%g,%g)\n", f2->contents, f2->planenum, 
					g_dplanes[f2->planenum].normal[0], g_dplanes[f2->planenum].normal[1], g_dplanes[f2->planenum].normal[2]);
				for (int i = 0; i < f2->numpoints; i++)
				{
					Developer (DEVELOPER_LEVEL_SPAM, "(%g,%g,%g)\n", f2->pts[i][0], f2->pts[i][1], f2->pts[i][2]);
				}
			}
		}
	}

    leafnode->contents = ContentsForRank(rank);

}
static void MakeLeaf (node_t *leafnode)
{
	int				nummarkfaces;
	face_t			*markfaces[MAX_LEAF_FACES + 1];
	surface_t		*surf;
	face_t			*f;	

    leafnode->planenum = -1;

	leafnode->iscontentsdetail = leafnode->detailbrushes != NULL;
	FreeLeafBrushes (leafnode);
	leafnode->detailbrushes = NULL;
	if (leafnode->boundsbrush)
	{
		FreeBrush (leafnode->boundsbrush);
	}
	leafnode->boundsbrush = NULL;

	if (!(leafnode->isportalleaf && leafnode->contents == CONTENTS_SOLID))
	{
		nummarkfaces = 0;
		for (surf = leafnode->surfaces; surf; surf = surf->next)
		{
			if (!surf->onnode)
			{
				continue;
			}
			if (!surf->onnode)
			{
				continue;
			}
			for (f = surf->faces; f; f = f->next)
			{
				if (f->original == NULL)
				{ // because it is not on node or its content is solid
					continue;
				}
				if (f->original == NULL)
				{
					continue;
				}
				hlassume(nummarkfaces < MAX_LEAF_FACES, assume_MAX_LEAF_FACES);

				markfaces[nummarkfaces++] = f->original;
			}
		}
		markfaces[nummarkfaces] = NULL;                    // end marker
		nummarkfaces++;

		leafnode->markfaces = (face_t**)malloc(nummarkfaces * sizeof(*leafnode->markfaces));
		memcpy(leafnode->markfaces, markfaces, nummarkfaces * sizeof(*leafnode->markfaces));
	}

	FreeLeafSurfs(leafnode);
	leafnode->surfaces = NULL;

}

// =====================================================================================
//  MakeNodePortal
//      Create the new portal by taking the full plane winding for the cutting plane and 
//      clipping it by all of the planes from the other portals.
//      Each portal tracks the node that created it, so unused nodes can be removed later.
// =====================================================================================
static void     MakeNodePortal(node_t* node)
{
    portal_t*       new_portal;
    portal_t*       p;
    dplane_t*       plane;
    dplane_t        clipplane;
    Winding *       w;
    int             side = 0;

    plane = &g_dplanes[node->planenum];
    w = new Winding(*plane);

    new_portal = AllocPortal();
    new_portal->plane = *plane;
    new_portal->onnode = node;

    for (p = node->portals; p; p = p->next[side])
    {
        clipplane = p->plane;
        if (p->nodes[0] == node)
        {
            side = 0;
        }
        else if (p->nodes[1] == node)
        {
            clipplane.dist = -clipplane.dist;
            VectorSubtract(vec3_origin, clipplane.normal, clipplane.normal);
            side = 1;
        }
        else
        {
            Error("MakeNodePortal: mislinked portal");
        }

        w->Clip(clipplane, true);
		if (w->m_NumPoints == 0)
        {
			Developer (DEVELOPER_LEVEL_WARNING, 
				"MakeNodePortal:new portal was clipped away from node@(%.0f,%.0f,%.0f)-(%.0f,%.0f,%.0f)",
                    node->mins[0], node->mins[1], node->mins[2], node->maxs[0], node->maxs[1], node->maxs[2]);
            FreePortal(new_portal);
            return;
        }
    }

    new_portal->winding = w;
    AddPortalToNodes(new_portal, node->children[0], node->children[1]);
}

// =====================================================================================
//  SplitNodePortals
//      Move or split the portals that bound node so that the node's children have portals instead of node.
// =====================================================================================
static void     SplitNodePortals(node_t *node)
{
    portal_t*       p;
    portal_t*       next_portal;
    portal_t*       new_portal;
    node_t*         f;
    node_t*         b;
    node_t*         other_node;
    int             side = 0;
    dplane_t*       plane;
    Winding*        frontwinding;
    Winding*        backwinding;

    plane = &g_dplanes[node->planenum];
    f = node->children[0];
    b = node->children[1];

    for (p = node->portals; p; p = next_portal)
    {
        if (p->nodes[0] == node)
        {
            side = 0;
        }
        else if (p->nodes[1] == node)
        {
            side = 1;
        }
        else
        {
            Error("SplitNodePortals: mislinked portal");
        }
        next_portal = p->next[side];

        other_node = p->nodes[!side];
        RemovePortalFromNode(p, p->nodes[0]);
        RemovePortalFromNode(p, p->nodes[1]);

        // cut the portal into two portals, one on each side of the cut plane
        p->winding->Divide(*plane, &frontwinding, &backwinding);

		if (!frontwinding && !backwinding)
		{
			continue;
		}
        if (!frontwinding)
        {
            if (side == 0)
            {
                AddPortalToNodes(p, b, other_node);
            }
            else
            {
                AddPortalToNodes(p, other_node, b);
            }
            continue;
        }
        if (!backwinding)
        {
            if (side == 0)
            {
                AddPortalToNodes(p, f, other_node);
            }
            else
            {
                AddPortalToNodes(p, other_node, f);
            }
            continue;
        }

        // the winding is split
        new_portal = AllocPortal();
        *new_portal = *p;
        new_portal->winding = backwinding;
        delete p->winding;
        p->winding = frontwinding;

        if (side == 0)
        {
            AddPortalToNodes(p, f, other_node);
            AddPortalToNodes(new_portal, b, other_node);
        }
        else
        {
            AddPortalToNodes(p, other_node, f);
            AddPortalToNodes(new_portal, other_node, b);
        }
    }

    node->portals = NULL;
}

// =====================================================================================
//  CalcNodeBounds
//      Determines the boundaries of a node by minmaxing all the portal points, whcih 
//      completely enclose the node.
//      Returns true if the node should be midsplit.(very large)
// =====================================================================================
static bool     CalcNodeBounds(node_t* node
							   , vec3_t validmins, vec3_t validmaxs
							   )
{
    int             i;
    int             j;
    vec_t           v;
    portal_t*       p;
    portal_t*       next_portal;
    int             side = 0;

	if (node->isdetail)
	{
		return false;
	}
    node->mins[0] = node->mins[1] = node->mins[2] = BOGUS_RANGE;
    node->maxs[0] = node->maxs[1] = node->maxs[2] = -BOGUS_RANGE;

    for (p = node->portals; p; p = next_portal)
    {
        if (p->nodes[0] == node)
        {
            side = 0;
        }
        else if (p->nodes[1] == node)
        {
            side = 1;
        }
        else
        {
            Error("CalcNodeBounds: mislinked portal");
        }
        next_portal = p->next[side];

        for (i = 0; i < p->winding->m_NumPoints; i++)
        {
            for (j = 0; j < 3; j++)
            {
                v = p->winding->m_Points[i][j];
                if (v < node->mins[j])
                {
                    node->mins[j] = v;
                }
                if (v > node->maxs[j])
                {
                    node->maxs[j] = v;
                }
            }
        }
    }

	if (node->isportalleaf)
	{
		return false;
	}
	for (i = 0; i < 3; i++)
	{
		validmins[i] = qmax (node->mins[i], -(ENGINE_ENTITY_RANGE + g_maxnode_size));
		validmaxs[i] = qmin (node->maxs[i], ENGINE_ENTITY_RANGE + g_maxnode_size);
	}
	for (i = 0; i < 3; i++)
	{
		if (validmaxs[i] - validmins[i] <= ON_EPSILON)
		{
			return false;
		}
	}
	for (i = 0; i < 3; i++)
	{
		if (validmaxs[i] - validmins[i] > g_maxnode_size + ON_EPSILON)
		{
			return true;
		}
	}
    return false;
}

// =====================================================================================
//  CopyFacesToNode
//      Do a final merge attempt, then subdivide the faces to surface cache size if needed.
//      These are final faces that will be drawable in the game.
//      Copies of these faces are further chopped up into the leafs, but they will reference these originals.
// =====================================================================================
static void     CopyFacesToNode(node_t* node, surface_t* surf)
{
    face_t**        prevptr;
    face_t*         f;
    face_t*         newf;

    // merge as much as possible
    MergePlaneFaces(surf);

    // subdivide large faces
    prevptr = &surf->faces;
    while (1)
    {
        f = *prevptr;
        if (!f)
        {
            break;
        }
        SubdivideFace(f, prevptr);
        f = *prevptr;
        prevptr = &f->next;
    }

    // copy the faces to the node, and consider them the originals
    node->surfaces = NULL;
    node->faces = NULL;
    for (f = surf->faces; f; f = f->next)
    {
		if (f->facestyle == face_discardable)
		{
			continue;
		}
        if (f->contents != CONTENTS_SOLID)
        {
            newf = AllocFace();
            *newf = *f;
            f->original = newf;
            newf->next = node->faces;
            node->faces = newf;
        }
    }
}

// =====================================================================================
//  BuildBspTree_r
// =====================================================================================
static void     BuildBspTree_r(node_t* node)
{
    surface_t*      split;
    bool            midsplit;
    surface_t*      allsurfs;
	vec3_t			validmins, validmaxs;

    midsplit = CalcNodeBounds(node
		, validmins, validmaxs
		);
	if (node->boundsbrush)
	{
		CalcBrushBounds (node->boundsbrush, node->loosemins, node->loosemaxs);
	}
	else
	{
		VectorFill (node->loosemins, BOGUS_RANGE);
		VectorFill (node->loosemaxs, -BOGUS_RANGE);
	}

	int splitdetaillevel = CalcSplitDetaillevel (node);
	FixDetaillevelForDiscardable (node, splitdetaillevel);
    split = SelectPartition(node->surfaces, node, midsplit
		, splitdetaillevel
		, validmins, validmaxs
		);
	if (!node->isdetail && (!split || split->detaillevel > 0))
	{
		node->isportalleaf = true;
		LinkLeafFaces (node->surfaces, node); // set contents
		if (node->contents == CONTENTS_SOLID)
		{
			split = NULL;
		}
	}
	else
	{
		node->isportalleaf = false;
	}
    if (!split)
    {                                                      // this is a leaf node
		MakeLeaf (node);
        return;
    }

    // these are final polygons
    split->onnode = node;                                  // can't use again
    allsurfs = node->surfaces;
    node->planenum = split->planenum;
    node->faces = NULL;
    CopyFacesToNode(node, split);

    node->children[0] = AllocNode();
    node->children[1] = AllocNode();
	node->children[0]->isdetail = split->detaillevel > 0;
	node->children[1]->isdetail = split->detaillevel > 0;

    // split all the polysurfaces into front and back lists
    SplitNodeSurfaces(allsurfs, node);
	SplitNodeBrushes (node->detailbrushes, node);
	if (node->boundsbrush)
	{
		for (int k = 0; k < 2; k++)
		{
			dplane_t p;
			brush_t *copy, *front, *back;
			if (k == 0)
			{ // front child
				VectorCopy (g_dplanes[split->planenum].normal, p.normal);
				p.dist = g_dplanes[split->planenum].dist - BOUNDS_EXPANSION;
			}
			else
			{ // back child
				VecSubtractVector (0, g_dplanes[split->planenum].normal, p.normal);
				p.dist = -g_dplanes[split->planenum].dist - BOUNDS_EXPANSION;
			}
			copy = NewBrushFromBrush (node->boundsbrush);
			SplitBrush (copy, &p, &front, &back);
			if (back)
			{
				FreeBrush (back);
			}
			if (!front)
			{
				Warning ("BuildBspTree_r: bounds was clipped away at (%f,%f,%f)-(%f,%f,%f).", node->loosemins[0], node->loosemins[1], node->loosemins[2], node->loosemaxs[0], node->loosemaxs[1], node->loosemaxs[2]);
			}
			node->children[k]->boundsbrush = front;
		}
		FreeBrush (node->boundsbrush);
	}
	node->boundsbrush = NULL;

	if (!split->detaillevel)
	{
		MakeNodePortal (node);
		SplitNodePortals (node);
	}

    // recursively do the children
    BuildBspTree_r(node->children[0]);
    BuildBspTree_r(node->children[1]);
	UpdateStatus();
}

// =====================================================================================
//  SolidBSP
//      Takes a chain of surfaces plus a split type, and returns a bsp tree with faces 
//      off the nodes.
//      The original surface chain will be completely freed.
// =====================================================================================
node_t*         SolidBSP(const surfchain_t* const surfhead, 
						 brush_t *detailbrushes, 
						 bool report_progress)
{
    node_t*         headnode;

	ResetStatus(report_progress);
	double start_time = I_FloatTime();
	if(report_progress)
	{
		Log("SolidBSP [hull %d] ",g_hullnum);
	}
	else
	{
	    Verbose("----- SolidBSP -----\n");
	}

    headnode = AllocNode();
    headnode->surfaces = surfhead->surfaces;
	headnode->detailbrushes = detailbrushes;
	headnode->isdetail = false;
	vec3_t brushmins, brushmaxs;
	VectorAddVec (surfhead->mins, -SIDESPACE, brushmins);
	VectorAddVec (surfhead->maxs, SIDESPACE, brushmaxs);
	headnode->boundsbrush = BrushFromBox (brushmins, brushmaxs);


    // generate six portals that enclose the entire world
    MakeHeadnodePortals(headnode, surfhead->mins, surfhead->maxs);

    // recursively partition everything
    BuildBspTree_r(headnode);

	double end_time = I_FloatTime();
	if(report_progress)
	{
		Log("%d (%.2f seconds)\n",++g_numProcessed,(end_time - start_time));
	}

    return headnode;
}
