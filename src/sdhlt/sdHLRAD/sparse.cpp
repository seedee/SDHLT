#include "qrad.h"



typedef struct
{
    unsigned        offset:24;
    unsigned        values:8;
}
sparse_row_t;

typedef struct
{
    sparse_row_t*   row;
    int             count;
}
sparse_column_t;

sparse_column_t* s_vismatrix;

// Vismatrix protected
static unsigned IsVisbitInArray(const unsigned x, const unsigned y)
{
    int             first, last, current;
    int             y_byte = y / 8;
    sparse_row_t*  row;
    sparse_column_t* column = s_vismatrix + x;

    if (!column->count)
    {
        return -1;
    }

    first = 0;
    last = column->count - 1;

    //    Warning("Searching . . .");
    // binary search to find visbit
    while (1)
    {
        current = (first + last) / 2;
        row = column->row + current;
        //        Warning("first %u, last %u, current %u, row %p, row->offset %u", first, last, current, row, row->offset);
        if ((row->offset) < y_byte)
        {
            first = current + 1;
        }
        else if ((row->offset) > y_byte)
        {
            last = current - 1;
        }
        else
        {
            return current;
        }
        if (first > last)
        {
            return -1;
        }
    }
}

static void		SetVisColumn (int patchnum, bool uncompressedcolumn[MAX_SPARSE_VISMATRIX_PATCHES])
{
	sparse_column_t *column;
	int mbegin;
	int m;
	int i;
	unsigned int bits;
	
	column = &s_vismatrix[patchnum];
	if (column->count || column->row)
	{
		Error ("SetVisColumn: column has been set");
	}

	for (mbegin = 0; mbegin < g_num_patches; mbegin += 8)
	{
		bits = 0;
		for (m = mbegin; m < mbegin + 8; m++)
		{
			if (m >= g_num_patches)
			{
				break;
			}
			if (uncompressedcolumn[m]) // visible
			{
				if (m < patchnum)
				{
					Error ("SetVisColumn: invalid parameter: m < patchnum");
				}
				bits |= (1 << (m - mbegin));
			}
		}
		if (bits)
		{
			column->count++;
		}
	}

	if (!column->count)
	{
		return;
	}
	column->row = (sparse_row_t *)malloc (column->count * sizeof (sparse_row_t));
	hlassume (column->row != NULL, assume_NoMemory);
	
	i = 0;
	for (mbegin = 0; mbegin < g_num_patches; mbegin += 8)
	{
		bits = 0;
		for (m = mbegin; m < mbegin + 8; m++)
		{
			if (m >= g_num_patches)
			{
				break;
			}
			if (uncompressedcolumn[m]) // visible
			{
				bits |= (1 << (m - mbegin));
			}
		}
		if (bits)
		{
			column->row[i].offset = mbegin / 8;
			column->row[i].values = bits;
			i++;
		}
	}
	if (i != column->count)
	{
		Error ("SetVisColumn: internal error");
	}
}

// Vismatrix public
static bool     CheckVisBitSparse(unsigned x, unsigned y
								  , vec3_t &transparency_out
								  , unsigned int &next_index
								  )
{
    int                offset;

    	VectorFill(transparency_out, 1.0);

    if (x == y)
    {
        return 1;
    }

    const unsigned a = x;
    const unsigned b = y;

    if (x > y)
    {
        x = b;
        y = a;
    }

    if (x > g_num_patches)
    {
        Warning("in CheckVisBit(), x > num_patches");
    }
    if (y > g_num_patches)
    {
        Warning("in CheckVisBit(), y > num_patches");
    }

    if ((offset = IsVisbitInArray(x, y)) != -1)
    {
    	if(g_customshadow_with_bouncelight)
    	{
    	     GetTransparency(a, b, transparency_out, next_index);
    	}
        return s_vismatrix[x].row[offset].values & (1 << (y & 7));
    }

	return false;
}

/*
 * ==============
 * TestPatchToFace
 * 
 * Sets vis bits for all patches in the face
 * ==============
 */
static void     TestPatchToFace(const unsigned patchnum, const int facenum, const int head
								, byte *pvs
								, bool uncompressedcolumn[MAX_SPARSE_VISMATRIX_PATCHES]
								)
{
    patch_t*        patch = &g_patches[patchnum];
    patch_t*        patch2 = g_face_patches[facenum];

    // if emitter is behind that face plane, skip all patches

    if (patch2)
    {
        const dplane_t* plane2 = getPlaneFromFaceNumber(facenum);

		if (DotProduct (patch->origin, plane2->normal) > PatchPlaneDist (patch2) + ON_EPSILON - patch->emitter_range)
        {
            // we need to do a real test
            const dplane_t* plane = getPlaneFromFaceNumber(patch->faceNumber);

            for (; patch2; patch2 = patch2->next)
            {
                unsigned        m = patch2 - g_patches;

                vec3_t		transparency = {1.0,1.0,1.0};
				int opaquestyle = -1;

                // check vis between patch and patch2
                // if bit has not already been set
                //  && v2 is not behind light plane
                //  && v2 is visible from v1
                if (m > patchnum)
				{
					if (patch2->leafnum == 0 || !(pvs[(patch2->leafnum - 1) >> 3] & (1 << ((patch2->leafnum - 1) & 7))))
					{
						continue;
					}
					vec3_t origin1, origin2;
					vec3_t delta;
					vec_t dist;
					VectorSubtract (patch->origin, patch2->origin, delta);
					dist = VectorLength (delta);
					if (dist < patch2->emitter_range - ON_EPSILON)
					{
						GetAlternateOrigin (patch->origin, plane->normal, patch2, origin2);
					}
					else
					{
						VectorCopy (patch2->origin, origin2);
					}
					if (DotProduct (origin2, plane->normal) <= PatchPlaneDist (patch) + MINIMUM_PATCH_DISTANCE)
					{
						continue;
					}
					if (dist < patch->emitter_range - ON_EPSILON)
					{
						GetAlternateOrigin (patch2->origin, plane2->normal, patch, origin1);
					}
					else
					{
						VectorCopy (patch->origin, origin1);
					}
					if (DotProduct (origin1, plane2->normal) <= PatchPlaneDist (patch2) + MINIMUM_PATCH_DISTANCE)
					{
						continue;
					}
                    if (TestLine(
						origin1, origin2
						) != CONTENTS_EMPTY)
					{
						continue;
					}
                    if (TestSegmentAgainstOpaqueList(
						origin1, origin2
						, transparency
						, opaquestyle
					))
					{
						continue;
					}

					if (opaquestyle != -1)
					{
						AddStyleToStyleArray (m, patchnum, opaquestyle);
						AddStyleToStyleArray (patchnum, m, opaquestyle);
					}
                                        
                    if(g_customshadow_with_bouncelight && !VectorCompare(transparency, vec3_one) )
                    {
                    	AddTransparencyToRawArray(patchnum, m, transparency);
                    }
					uncompressedcolumn[m] = true;
                }
            }
        }
    }
}


/*
 * ===========
 * BuildVisLeafs
 * 
 * This is run by multiple threads
 * ===========
 */
#ifdef SYSTEM_WIN32
#pragma warning(push)
#pragma warning(disable: 4100)                             // unreferenced formal parameter
#endif
static void     BuildVisLeafs(int threadnum)
{
    int             i;
    int             lface, facenum, facenum2;
    byte            pvs[(MAX_MAP_LEAFS + 7) / 8];
    dleaf_t*        srcleaf;
    dleaf_t*        leaf;
    patch_t*        patch;
    int             head;
    unsigned        patchnum;
	bool *uncompressedcolumn = (bool *)malloc (MAX_SPARSE_VISMATRIX_PATCHES * sizeof (bool));
	hlassume (uncompressedcolumn != NULL, assume_NoMemory);

    while (1)
    {
        //
        // build a minimal BSP tree that only
        // covers areas relevent to the PVS
        //
        i = GetThreadWork();
        if (i == -1)
        {
            break;
        }
        i++;                                               // skip leaf 0
        srcleaf = &g_dleafs[i];
        if (!g_visdatasize)
		{
			memset (pvs, 255, (g_dmodels[0].visleafs + 7) / 8);
		}
		else
		{
		if (srcleaf->visofs == -1)
		{
			Developer (DEVELOPER_LEVEL_ERROR, "Error: No visdata for leaf %d\n", i);
			continue;
		}
        DecompressVis(&g_dvisdata[srcleaf->visofs], pvs, sizeof(pvs));
		}
        head = 0;

        //
        // go through all the faces inside the
        // leaf, and process the patches that
        // actually have origins inside
        //
		for (facenum = 0; facenum < g_numfaces; facenum++)
		{
			for (patch = g_face_patches[facenum]; patch; patch = patch->next)
			{
				if (patch->leafnum != i)
					continue;
				patchnum = patch - g_patches;
				for (int m = 0; m < g_num_patches; m++)
				{
					uncompressedcolumn[m] = false;
				}
				for (facenum2 = facenum + 1; facenum2 < g_numfaces; facenum2++)
					TestPatchToFace (patchnum, facenum2, head, pvs
									, uncompressedcolumn
									);
				SetVisColumn (patchnum, uncompressedcolumn);
			}
		}

    }
	free (uncompressedcolumn);
}

#ifdef SYSTEM_WIN32
#pragma warning(pop)
#endif

/*
 * ==============
 * BuildVisMatrix
 * ==============
 */
static void     BuildVisMatrix()
{
    s_vismatrix = (sparse_column_t*)AllocBlock(g_num_patches * sizeof(sparse_column_t));

    if (!s_vismatrix)
    {
        Log("Failed to allocate vismatrix");
        hlassume(s_vismatrix != NULL, assume_NoMemory);
    }

    NamedRunThreadsOn(g_dmodels[0].visleafs, g_estimate, BuildVisLeafs);
}

static void     FreeVisMatrix()
{
    if (s_vismatrix)
    {
        unsigned        x;
        sparse_column_t* item;

        for (x = 0, item = s_vismatrix; x < g_num_patches; x++, item++)
        {
            if (item->row)
            {
                free(item->row);
            }
        }
        if (FreeBlock(s_vismatrix))
        {
            s_vismatrix = NULL;
        }
        else
        {
            Warning("Unable to free vismatrix");
        }
    }


}

static void     DumpVismatrixInfo()
{
    unsigned        totals[8];
    size_t          total_vismatrix_memory;
	total_vismatrix_memory = sizeof(sparse_column_t) * g_num_patches;

    sparse_column_t* column_end = s_vismatrix + g_num_patches;
    sparse_column_t* column = s_vismatrix;

    memset(totals, 0, sizeof(totals));

    while (column < column_end)
    {
        total_vismatrix_memory += column->count * sizeof(sparse_row_t);
        column++;
    }

    Log("%-20s: %5.1f megs\n", "visibility matrix", total_vismatrix_memory / (1024 * 1024.0));
}

//
// end old vismat.c
////////////////////////////

void            MakeScalesSparseVismatrix()
{
    char            transferfile[_MAX_PATH];

    hlassume(g_num_patches < MAX_SPARSE_VISMATRIX_PATCHES, assume_MAX_PATCHES);

	safe_snprintf(transferfile, _MAX_PATH, "%s.inc", g_Mapname);

    if (!g_incremental || !readtransfers(transferfile, g_num_patches))
    {
        // determine visibility between g_patches
        BuildVisMatrix();
        DumpVismatrixInfo();
        g_CheckVisBit = CheckVisBitSparse;

        CreateFinalTransparencyArrays("custom shadow array");
        
	if(g_rgb_transfers)
		{NamedRunThreadsOn(g_num_patches, g_estimate, MakeRGBScales);}
	else
		{NamedRunThreadsOn(g_num_patches, g_estimate, MakeScales);}
        FreeVisMatrix();
        FreeTransparencyArrays();

        if (g_incremental)
        {
            writetransfers(transferfile, g_num_patches);
        }
        else
        {
            _unlink(transferfile);
        }
        // release visibility matrix
        DumpTransfersMemoryUsage();
		CreateFinalStyleArrays ("dynamic shadow array");
    }
}
