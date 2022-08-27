#include "bsp5.h"

//  WriteClipNodes_r
//  WriteClipNodes
//  WriteDrawLeaf
//  WriteFace
//  WriteDrawNodes_r
//  FreeDrawNodes_r
//  WriteDrawNodes
//  BeginBSPFile
//  FinishBSPFile

#include <map>

typedef std::map< int, int > PlaneMap;
static PlaneMap gPlaneMap;
static int gNumMappedPlanes;
static dplane_t gMappedPlanes[MAX_MAP_PLANES];
extern bool g_noopt;

typedef std::map< int, int > texinfomap_t;
static int g_nummappedtexinfo;
static texinfo_t g_mappedtexinfo[MAX_MAP_TEXINFO];
static texinfomap_t g_texinfomap;

int count_mergedclipnodes;
typedef std::map< std::pair< int, std::pair< int, int > >, int > clipnodemap_t;
inline clipnodemap_t::key_type MakeKey (const dclipnode_t &c)
{
	return std::make_pair (c.planenum, std::make_pair (c.children[0], c.children[1]));
}

// =====================================================================================
//  WritePlane
//  hook for plane optimization
// =====================================================================================
static int WritePlane(int planenum)
{
	planenum = planenum & (~1);

	if(g_noopt)
	{
		return planenum;
	}

	PlaneMap::iterator item = gPlaneMap.find(planenum);
	if(item != gPlaneMap.end())
	{
		return item->second;
	}
	//add plane to BSP
	hlassume(gNumMappedPlanes < MAX_MAP_PLANES, assume_MAX_MAP_PLANES);
	gMappedPlanes[gNumMappedPlanes] = g_dplanes[planenum];
	gPlaneMap.insert(PlaneMap::value_type(planenum,gNumMappedPlanes));

	return gNumMappedPlanes++;
}

// =====================================================================================
//  WriteTexinfo
// =====================================================================================
static int WriteTexinfo (int texinfo)
{
	if (texinfo < 0 || texinfo >= g_numtexinfo)
	{
		Error ("Bad texinfo number %d.\n", texinfo);
	}

	if (g_noopt)
	{
		return texinfo;
	}

	texinfomap_t::iterator it;
	it = g_texinfomap.find (texinfo);
	if (it != g_texinfomap.end ())
	{
		return it->second;
	}

	int c;
	hlassume (g_nummappedtexinfo < MAX_MAP_TEXINFO, assume_MAX_MAP_TEXINFO);
	c = g_nummappedtexinfo;
	g_mappedtexinfo[g_nummappedtexinfo] = g_texinfo[texinfo];
	g_texinfomap.insert (texinfomap_t::value_type (texinfo, g_nummappedtexinfo));
	g_nummappedtexinfo++;
	return c;
}

// =====================================================================================
//  WriteClipNodes_r
// =====================================================================================
static int      WriteClipNodes_r(node_t* node
								 , const node_t *portalleaf
								 , clipnodemap_t *outputmap
								 )
{
    int             i, c;
    dclipnode_t*    cn;
    int             num;

	if (node->isportalleaf)
	{
		if (node->contents == CONTENTS_SOLID)
		{
			free (node);
			return CONTENTS_SOLID;
		}
		else
		{
			portalleaf = node;
		}
	}
	if (node->planenum == -1)
	{
		if (node->iscontentsdetail)
		{
			num = CONTENTS_SOLID;
		}
		else
		{
			num = portalleaf->contents;
		}
		free (node->markfaces);
		free (node);
		return num;
	}

	dclipnode_t tmpclipnode; // this clipnode will be inserted into g_dclipnodes[c] if it can't be merged
	cn = &tmpclipnode;
	c = g_numclipnodes;
	g_numclipnodes++;
    if (node->planenum & 1)
    {
        Error("WriteClipNodes_r: odd planenum");
    }
    cn->planenum = WritePlane(node->planenum);
    for (i = 0; i < 2; i++)
    {
        cn->children[i] = WriteClipNodes_r(node->children[i]
			, portalleaf
			, outputmap
			);
    }
	clipnodemap_t::iterator output;
	output = outputmap->find (MakeKey (*cn));
	if (g_noclipnodemerge || output == outputmap->end ())
	{
		hlassume (c < MAX_MAP_CLIPNODES, assume_MAX_MAP_CLIPNODES);
		g_dclipnodes[c] = *cn;
		(*outputmap)[MakeKey (*cn)] = c;
	}
	else
	{
		count_mergedclipnodes++;
		if (g_numclipnodes != c + 1)
		{
			Error ("Merge clipnodes: internal error");
		}
		g_numclipnodes = c;
		c = output->second; // use existing clipnode
	}

    free(node);
    return c;
}

// =====================================================================================
//  WriteClipNodes
//      Called after the clipping hull is completed.  Generates a disk format
//      representation and frees the original memory.
// =====================================================================================
void            WriteClipNodes(node_t* nodes)
{
	// we only merge among the clipnodes of the same hull of the same model
	clipnodemap_t outputmap;
    WriteClipNodes_r(nodes
		, NULL
		, &outputmap
		);
}

// =====================================================================================
//  WriteDrawLeaf
// =====================================================================================
static int		WriteDrawLeaf (node_t *node, const node_t *portalleaf)
{
    face_t**        fp;
    face_t*         f;
    dleaf_t*        leaf_p;
	int				leafnum = g_numleafs;

    // emit a leaf
	hlassume (g_numleafs < MAX_MAP_LEAFS, assume_MAX_MAP_LEAFS);
    leaf_p = &g_dleafs[g_numleafs];
    g_numleafs++;

	leaf_p->contents = portalleaf->contents;

    //
    // write bounding box info
    //
	vec3_t mins, maxs;
#if 0
	printf ("leaf isdetail = %d loosebound = (%f,%f,%f)-(%f,%f,%f) portalleaf = (%f,%f,%f)-(%f,%f,%f)\n", node->isdetail,
		node->loosemins[0], node->loosemins[1], node->loosemins[2], node->loosemaxs[0], node->loosemaxs[1], node->loosemaxs[2],
		portalleaf->mins[0], portalleaf->mins[1], portalleaf->mins[2], portalleaf->maxs[0], portalleaf->maxs[1], portalleaf->maxs[2]);
#endif
	if (node->isdetail)
	{
		// intersect its loose bounds with the strict bounds of its parent portalleaf
		VectorCompareMaximum (portalleaf->mins, node->loosemins, mins);
		VectorCompareMinimum (portalleaf->maxs, node->loosemaxs, maxs);
	}
	else
	{
		VectorCopy (node->mins, mins);
		VectorCopy (node->maxs, maxs);
	}
	for (int k = 0; k < 3; k++)
	{
		leaf_p->mins[k] = (short)qmax (-32767, qmin ((int)mins[k], 32767));
		leaf_p->maxs[k] = (short)qmax (-32767, qmin ((int)maxs[k], 32767));
	}

    leaf_p->visofs = -1;                                   // no vis info yet

    //
    // write the marksurfaces
    //
    leaf_p->firstmarksurface = g_nummarksurfaces;

    hlassume(node->markfaces != NULL, assume_EmptySolid);

    for (fp = node->markfaces; *fp; fp++)
    {
        // emit a marksurface
        f = *fp;
        do
        {
			// fix face 0 being seen everywhere
			if (f->outputnumber == -1)
			{
				f = f->original;
				continue;
			}
			bool ishidden = false;
			{
				const char *name = GetTextureByNumber (f->texturenum);
				if (strlen (name) >= 7 && !strcasecmp (&name[strlen (name) - 7], "_HIDDEN"))
				{
					ishidden = true;
				}
			}
			if (ishidden)
			{
				f = f->original;
				continue;
			}
            g_dmarksurfaces[g_nummarksurfaces] = f->outputnumber;
            hlassume(g_nummarksurfaces < MAX_MAP_MARKSURFACES, assume_MAX_MAP_MARKSURFACES);
            g_nummarksurfaces++;
            f = f->original;                               // grab tjunction split faces
        }
        while (f);
    }
    free(node->markfaces);

    leaf_p->nummarksurfaces = g_nummarksurfaces - leaf_p->firstmarksurface;
	return leafnum;
}

// =====================================================================================
//  WriteFace
// =====================================================================================
static void     WriteFace(face_t* f)
{
    dface_t*        df;
    int             i;
    int             e;

    if (    CheckFaceForHint(f)
        ||  CheckFaceForSkip(f)
        ||  CheckFaceForNull(f)  // AJM
		|| CheckFaceForDiscardable (f)
		|| f->texturenum == -1
		|| f->referenced == 0 // this face is not referenced by any nonsolid leaf because it is completely covered by func_details

// =====================================================================================
//Cpt_Andrew - Env_Sky Check
// =====================================================================================
       ||  CheckFaceForEnv_Sky(f)
// =====================================================================================

       )
    {
		f->outputnumber = -1;
        return;
    }

    f->outputnumber = g_numfaces;

    df = &g_dfaces[g_numfaces];
    hlassume(g_numfaces < MAX_MAP_FACES, assume_MAX_MAP_FACES);
    g_numfaces++;

	df->planenum = WritePlane(f->planenum);
	df->side = f->planenum & 1;
    df->firstedge = g_numsurfedges;
    df->numedges = f->numpoints;

	df->texinfo = WriteTexinfo (f->texturenum);

    for (i = 0; i < f->numpoints; i++)
    {
		e = f->outputedges[i];
        hlassume(g_numsurfedges < MAX_MAP_SURFEDGES, assume_MAX_MAP_SURFEDGES);
        g_dsurfedges[g_numsurfedges] = e;
        g_numsurfedges++;
    }
	free (f->outputedges);
	f->outputedges = NULL;
}

// =====================================================================================
//  WriteDrawNodes_r
// =====================================================================================
static int WriteDrawNodes_r (node_t *node, const node_t *portalleaf)
{
	if (node->isportalleaf)
	{
		if (node->contents == CONTENTS_SOLID)
		{
			return -1;
		}
		else
		{
			portalleaf = node;
			// Warning: make sure parent data have not been freed when writing children.
		}
	}
	if (node->planenum == -1)
	{
		if (node->iscontentsdetail)
		{
			free(node->markfaces);
			return -1;
		}
		else
		{
			int leafnum = WriteDrawLeaf (node, portalleaf);
			return -1 - leafnum;
		}
	}
    dnode_t*        n;
    int             i;
    face_t*         f;
	int nodenum = g_numnodes;

    // emit a node
    hlassume(g_numnodes < MAX_MAP_NODES, assume_MAX_MAP_NODES);
    n = &g_dnodes[g_numnodes];
    g_numnodes++;

	vec3_t mins, maxs;
#if 0
	if (node->isdetail || node->isportalleaf)
		printf ("node isdetail = %d loosebound = (%f,%f,%f)-(%f,%f,%f) portalleaf = (%f,%f,%f)-(%f,%f,%f)\n", node->isdetail,
			node->loosemins[0], node->loosemins[1], node->loosemins[2], node->loosemaxs[0], node->loosemaxs[1], node->loosemaxs[2],
			portalleaf->mins[0], portalleaf->mins[1], portalleaf->mins[2], portalleaf->maxs[0], portalleaf->maxs[1], portalleaf->maxs[2]);
#endif
	if (node->isdetail)
	{
		// intersect its loose bounds with the strict bounds of its parent portalleaf
		VectorCompareMaximum (portalleaf->mins, node->loosemins, mins);
		VectorCompareMinimum (portalleaf->maxs, node->loosemaxs, maxs);
	}
	else
	{
		VectorCopy (node->mins, mins);
		VectorCopy (node->maxs, maxs);
	}
	for (int k = 0; k < 3; k++)
	{
		n->mins[k] = (short)qmax (-32767, qmin ((int)mins[k], 32767));
		n->maxs[k] = (short)qmax (-32767, qmin ((int)maxs[k], 32767));
	}

    if (node->planenum & 1)
    {
        Error("WriteDrawNodes_r: odd planenum");
    }
    n->planenum = WritePlane(node->planenum);
    n->firstface = g_numfaces;

    for (f = node->faces; f; f = f->next)
    {
        WriteFace(f);
    }

    n->numfaces = g_numfaces - n->firstface;

    //
    // recursively output the other nodes
    //
    for (i = 0; i < 2; i++)
    {
		n->children[i] = WriteDrawNodes_r (node->children[i], portalleaf);
    }
	return nodenum;
}

// =====================================================================================
//  FreeDrawNodes_r
// =====================================================================================
static void     FreeDrawNodes_r(node_t* node)
{
    int             i;
    face_t*         f;
    face_t*         next;

    for (i = 0; i < 2; i++)
    {
        if (node->children[i]->planenum != -1)
        {
            FreeDrawNodes_r(node->children[i]);
        }
    }

    //
    // free the faces on the node
    //
    for (f = node->faces; f; f = next)
    {
        next = f->next;
        FreeFace(f);
    }

    free(node);
}

// =====================================================================================
//  WriteDrawNodes
//      Called after a drawing hull is completed
//      Frees all nodes and faces
// =====================================================================================
void OutputEdges_face (face_t *f)
{
	if (CheckFaceForHint(f)
		|| CheckFaceForSkip(f)
        || CheckFaceForNull(f)  // AJM
		|| CheckFaceForDiscardable (f)
		|| f->texturenum == -1
		|| f->referenced == 0
		|| CheckFaceForEnv_Sky(f)//Cpt_Andrew - Env_Sky Check
		)
	{
		return;
	}
	f->outputedges = (int *)malloc (f->numpoints * sizeof (int));
	hlassume (f->outputedges != NULL, assume_NoMemory);
	int i;
	for (i = 0; i < f->numpoints; i++)
	{
		int e = GetEdge (f->pts[i], f->pts[(i + 1) % f->numpoints], f);
		f->outputedges[i] = e;
	}
}
int OutputEdges_r (node_t *node, int detaillevel)
{
	int next = -1;
	if (node->planenum == -1)
	{
		return next;
	}
	face_t *f;
	for (f = node->faces; f; f = f->next)
	{
		if (f->detaillevel > detaillevel)
		{
			if (next == -1? true: f->detaillevel < next)
			{
				next = f->detaillevel;
			}
		}
		if (f->detaillevel == detaillevel)
		{
			OutputEdges_face (f);
		}
	}
	int i;
	for (i = 0; i < 2; i++)
	{
		int r = OutputEdges_r (node->children[i], detaillevel);
		if (r == -1? false: next == -1? true: r < next)
		{
			next = r;
		}
	}
	return next;
}
static void RemoveCoveredFaces_r (node_t *node)
{
	if (node->isportalleaf)
	{
		if (node->contents == CONTENTS_SOLID)
		{
			return; // stop here, don't go deeper into children
		}
	}
	if (node->planenum == -1)
	{
		// this is a leaf
		if (node->iscontentsdetail)
		{
			return;
		}
		else
		{
			face_t **fp;
			for (fp = node->markfaces; *fp; fp++)
			{
				for (face_t *f = *fp; f; f = f->original) // for each tjunc subface
				{
					f->referenced++; // mark the face as referenced
				}
			}
		}
		return;
	}
	
	// this is a node
	for (face_t *f = node->faces; f; f = f->next)
	{
		f->referenced = 0; // clear the mark
	}

	RemoveCoveredFaces_r (node->children[0]);
	RemoveCoveredFaces_r (node->children[1]);
}
void            WriteDrawNodes(node_t* headnode)
{
	RemoveCoveredFaces_r (headnode); // fill "referenced" value
	// higher detail level should not compete for edge pairing with lower detail level.
	int detaillevel, nextdetaillevel;
	for (detaillevel = 0; detaillevel != -1; detaillevel = nextdetaillevel)
	{
		nextdetaillevel = OutputEdges_r (headnode, detaillevel);
	}
	WriteDrawNodes_r (headnode, NULL);
}


// =====================================================================================
//  BeginBSPFile
// =====================================================================================
void            BeginBSPFile()
{
    // these values may actually be initialized
    // if the file existed when loaded, so clear them explicitly
	gNumMappedPlanes = 0;
	gPlaneMap.clear();

	g_nummappedtexinfo = 0;
	g_texinfomap.clear ();

	count_mergedclipnodes = 0;
    g_nummodels = 0;
    g_numfaces = 0;
    g_numnodes = 0;
    g_numclipnodes = 0;
    g_numvertexes = 0;
    g_nummarksurfaces = 0;
    g_numsurfedges = 0;

    // edge 0 is not used, because 0 can't be negated
    g_numedges = 1;

    // leaf 0 is common solid with no faces
    g_numleafs = 1;
    g_dleafs[0].contents = CONTENTS_SOLID;
}

// =====================================================================================
//  FinishBSPFile
// =====================================================================================
void            FinishBSPFile()
{
    Verbose("--- FinishBSPFile ---\n");

	if (g_dmodels[0].visleafs > MAX_MAP_LEAFS_ENGINE)
	{
		Warning ("Number of world leaves(%d) exceeded MAX_MAP_LEAFS(%d)\nIf you encounter problems when running your map, consider this the most likely cause.\n", g_dmodels[0].visleafs, MAX_MAP_LEAFS_ENGINE);
	}
	if (g_dmodels[0].numfaces > MAX_MAP_WORLDFACES)
	{
		Warning ("Number of world faces(%d) exceeded %d. Some faces will disappear in game.\nTo reduce world faces, change some world brushes (including func_details) to func_walls.\n", g_dmodels[0].numfaces, MAX_MAP_WORLDFACES);
	}
	Developer (DEVELOPER_LEVEL_MESSAGE, "count_mergedclipnodes = %d\n", count_mergedclipnodes);
	if (!g_noclipnodemerge)
	{
		Log ("Reduced %d clipnodes to %d\n", g_numclipnodes + count_mergedclipnodes, g_numclipnodes);
	}
	if(!g_noopt)
	{
		{
			Log ("Reduced %d texinfos to %d\n", g_numtexinfo, g_nummappedtexinfo);
			for (int i = 0; i < g_nummappedtexinfo; i++)
			{
				g_texinfo[i] = g_mappedtexinfo[i];
			}
			g_numtexinfo = g_nummappedtexinfo;
		}
		{
			dmiptexlump_t *l = (dmiptexlump_t *)g_dtexdata;
			int &g_nummiptex = l->nummiptex;
			bool *Used = (bool *)calloc (g_nummiptex, sizeof(bool));
			int Num = 0, Size = 0;
			int *Map = (int *)malloc (g_nummiptex * sizeof(int));
			int i;
			hlassume (Used != NULL && Map != NULL, assume_NoMemory);
			int *lumpsizes = (int *)malloc (g_nummiptex * sizeof (int));
			const int newdatasizemax = g_texdatasize - ((byte *)&l->dataofs[g_nummiptex] - (byte *)l);
			byte *newdata = (byte *)malloc (newdatasizemax);
			int newdatasize = 0;
			hlassume (lumpsizes != NULL && newdata != NULL, assume_NoMemory);
			int total = 0;
			for (i = 0; i < g_nummiptex; i++)
			{
				if (l->dataofs[i] == -1)
				{
					lumpsizes[i] = -1;
					continue;
				}
				lumpsizes[i] = g_texdatasize - l->dataofs[i];
				for (int j = 0; j < g_nummiptex; j++)
				{
					int lumpsize = l->dataofs[j] - l->dataofs[i];
					if (l->dataofs[j] == -1 || lumpsize < 0 || lumpsize == 0 && j <= i)
						continue;
					if (lumpsize < lumpsizes[i])
						lumpsizes[i] = lumpsize;
				}
				total += lumpsizes[i];
			}
			if (total != newdatasizemax)
			{
				Warning ("Bad texdata structure.\n");
				goto skipReduceTexdata;
			}
			for (i = 0; i < g_numtexinfo; i++)
			{
				texinfo_t *t = &g_texinfo[i];
				if (t->miptex < 0 || t->miptex >= g_nummiptex)
				{
					Warning ("Bad miptex number %d.\n", t->miptex);
					goto skipReduceTexdata;
				}
				Used[t->miptex] = true;
			}
			for (i = 0; i < g_nummiptex; i++)
			{
				const int MAXWADNAME = 16;
				char name[MAXWADNAME];
				int j, k;
				if (l->dataofs[i] < 0)
					continue;
				if (Used[i] == true)
				{
					miptex_t *m = (miptex_t *)((byte *)l + l->dataofs[i]);
					if (m->name[0] != '+' && m->name[0] != '-')
						continue;
					safe_strncpy (name, m->name, MAXWADNAME);
					if (name[1] == '\0')
						continue;
					for (j = 0; j < 20; j++)
					{
						if (j < 10)
							name[1] = '0' + j;
						else
							name[1] = 'A' + j - 10;
						for (k = 0; k < g_nummiptex; k++)
						{
							if (l->dataofs[k] < 0)
								continue;
							miptex_t *m2 = (miptex_t *)((byte *)l + l->dataofs[k]);
							if (!strcasecmp (name, m2->name))
								Used[k] = true;
						}
					}
				}
			}
			for (i = 0; i < g_nummiptex; i++)
			{
				if (Used[i])
				{
					Map[i] = Num;
					Num++;
				}
				else
				{
					Map[i] = -1;
				}
			}
			for (i = 0; i < g_numtexinfo; i++)
			{
				texinfo_t *t = &g_texinfo[i];
				t->miptex = Map[t->miptex];
			}
			Size += (byte *)&l->dataofs[Num] - (byte *)l;
			for (i = 0; i < g_nummiptex; i++)
			{
				if (Used[i])
				{
					if (lumpsizes[i] == -1)
					{
						l->dataofs[Map[i]] = -1;
					}
					else
					{
						memcpy ((byte *)newdata + newdatasize, (byte *)l + l->dataofs[i], lumpsizes[i]);
						l->dataofs[Map[i]] = Size;
						newdatasize += lumpsizes[i];
						Size += lumpsizes[i];
					}
				}
			}
			memcpy (&l->dataofs[Num], newdata, newdatasize);
			Log ("Reduced %d texdatas to %d (%d bytes to %d)\n", g_nummiptex, Num, g_texdatasize, Size);
			g_nummiptex = Num;
			g_texdatasize = Size;
			skipReduceTexdata:;
			free (lumpsizes);
			free (newdata);
			free (Used);
			free (Map);
		}
		Log ("Reduced %d planes to %d\n", g_numplanes, gNumMappedPlanes);

		for(int counter = 0; counter < gNumMappedPlanes; counter++)
		{
			g_dplanes[counter] = gMappedPlanes[counter];
		}
		g_numplanes = gNumMappedPlanes;
	}
	else
	{
		hlassume (g_numtexinfo < MAX_MAP_TEXINFO, assume_MAX_MAP_TEXINFO);
		hlassume (g_numplanes < MAX_MAP_PLANES, assume_MAX_MAP_PLANES);
	}

	if (!g_nobrink)
	{
		Log ("FixBrinks:\n");
		dclipnode_t *clipnodes; //[MAX_MAP_CLIPNODES]
		int numclipnodes;
		clipnodes = (dclipnode_t *)malloc (MAX_MAP_CLIPNODES * sizeof (dclipnode_t));
		hlassume (clipnodes != NULL, assume_NoMemory);
		void *(*brinkinfo)[NUM_HULLS]; //[MAX_MAP_MODELS]
		int (*headnode)[NUM_HULLS]; //[MAX_MAP_MODELS]
		brinkinfo = (void *(*)[NUM_HULLS])malloc (MAX_MAP_MODELS * sizeof (void *[NUM_HULLS]));
		hlassume (brinkinfo != NULL, assume_NoMemory);
		headnode = (int (*)[NUM_HULLS])malloc (MAX_MAP_MODELS * sizeof (int [NUM_HULLS]));
		hlassume (headnode != NULL, assume_NoMemory);

		int i, j, level;
		for (i = 0; i < g_nummodels; i++)
		{
			dmodel_t *m = &g_dmodels[i];
			Developer (DEVELOPER_LEVEL_MESSAGE, " model %d\n", i);
			for (j = 1; j < NUM_HULLS; j++)
			{
				brinkinfo[i][j] = CreateBrinkinfo (g_dclipnodes, m->headnode[j]);
			}
		}
		for (level = BrinkAny; level > BrinkNone; level--)
		{
			numclipnodes = 0;
			count_mergedclipnodes = 0;
			for (i = 0; i < g_nummodels; i++)
			{
				for (j = 1; j < NUM_HULLS; j++)
				{
					if (!FixBrinks (brinkinfo[i][j], (bbrinklevel_e) level, headnode[i][j], clipnodes, MAX_MAP_CLIPNODES, numclipnodes, numclipnodes))
					{
						break;
					}
				}
				if (j < NUM_HULLS)
				{
					break;
				}
			}
			if (i == g_nummodels)
			{
				break;
			}
		}
		for (i = 0; i < g_nummodels; i++)
		{
			for (j = 1; j < NUM_HULLS; j++)
			{
				DeleteBrinkinfo (brinkinfo[i][j]);
			}
		}
		if (level == BrinkNone)
		{
			Warning ("No brinks have been fixed because clipnode data is almost full.");
		}
		else
		{
			if (level != BrinkAny)
			{
				Warning ("Not all brinks have been fixed because clipnode data is almost full.");
			}
			Developer (DEVELOPER_LEVEL_MESSAGE, "count_mergedclipnodes = %d\n", count_mergedclipnodes);
			Log ("Increased %d clipnodes to %d.\n", g_numclipnodes, numclipnodes);
			g_numclipnodes = numclipnodes;
			memcpy (g_dclipnodes, clipnodes, numclipnodes * sizeof (dclipnode_t));
			for (i = 0; i < g_nummodels; i++)
			{
				dmodel_t *m = &g_dmodels[i];
				for (j = 1; j < NUM_HULLS; j++)
				{
					m->headnode[j] = headnode[i][j];
				}
			}
		}
		free (brinkinfo);
		free (headnode);
		free (clipnodes);
	}
	
#ifdef PLATFORM_CAN_CALC_EXTENT
	WriteExtentFile (g_extentfilename);
#else
	Warning ("The " PLATFORM_VERSIONSTRING " version of hlbsp couldn't create extent file. The lack of extent file may cause hlrad error.");
#endif
	if (g_chart)
    {
        PrintBSPFileSizes();
    }

#undef dplane_t // this allow us to temporarily access the raw data directly without the layer of indirection
#undef g_dplanes
	for (int i = 0; i < g_numplanes; i++)
	{
		plane_t *mp = &g_mapplanes[i];
		dplane_t *dp = &g_dplanes[i];
		VectorCopy (mp->normal, dp->normal);
		dp->dist = mp->dist;
		dp->type = mp->type;
	}
#define dplane_t plane_t
#define g_dplanes g_mapplanes
    WriteBSPFile(g_bspfilename);
}
