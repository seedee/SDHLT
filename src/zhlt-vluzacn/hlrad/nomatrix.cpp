#include "qrad.h"

// =====================================================================================
//  CheckVisBit
// =====================================================================================
static bool     CheckVisBitNoVismatrix(unsigned patchnum1, unsigned patchnum2
									   , vec3_t &transparency_out
									   , unsigned int &
									   )
	// patchnum1=receiver, patchnum2=emitter. //HLRAD_CheckVisBitNoVismatrix_NOSWAP
{
    
    if (patchnum1 > g_num_patches)
    {
        Warning("in CheckVisBit(), patchnum1 > num_patches");
    }
    if (patchnum2 > g_num_patches)
    {
        Warning("in CheckVisBit(), patchnum2 > num_patches");
    }
	
    patch_t*        patch = &g_patches[patchnum1];
    patch_t*        patch2 = &g_patches[patchnum2];

    VectorFill(transparency_out, 1.0);

    // if emitter is behind that face plane, skip all patches

    if (patch2)
    {
        const dplane_t* plane2 = getPlaneFromFaceNumber(patch2->faceNumber);

		if (DotProduct (patch->origin, plane2->normal) > PatchPlaneDist (patch2) + ON_EPSILON - patch->emitter_range)
        {
            // we need to do a real test

            const dplane_t* plane = getPlaneFromFaceNumber(patch->faceNumber);

            vec3_t transparency = {1.0,1.0,1.0};
			int opaquestyle = -1;

            // check vis between patch and patch2
            //  if v2 is not behind light plane
            //  && v2 is visible from v1
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
				return false;
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
				return false;
			}
            if (TestLine(
				origin1, origin2
				) != CONTENTS_EMPTY)
			{
				return false;
			}
            if (TestSegmentAgainstOpaqueList(
				origin1, origin2
				, transparency
				, opaquestyle
				))
			{
				return false;
			}

            {
				if (opaquestyle != -1)
				{
					AddStyleToStyleArray (patchnum1, patchnum2, opaquestyle);
				}
            	if(g_customshadow_with_bouncelight)
            	{
            		VectorCopy(transparency, transparency_out);
            	}
                return true;
            }
        }
    }

    return false;
}
       bool     CheckVisBitBackwards(unsigned receiver, unsigned emitter, const vec3_t &backorigin, const vec3_t &backnormal
									   , vec3_t &transparency_out
									   )
{	
    patch_t*        patch = &g_patches[receiver];
    patch_t*        emitpatch = &g_patches[emitter];

    VectorFill(transparency_out, 1.0);

    if (emitpatch)
    {
        const dplane_t* emitplane = getPlaneFromFaceNumber(emitpatch->faceNumber);

        if (DotProduct(backorigin, emitplane->normal) > (PatchPlaneDist(emitpatch) + MINIMUM_PATCH_DISTANCE))
        {

            vec3_t transparency = {1.0,1.0,1.0};
			int opaquestyle = -1;

			vec3_t emitorigin;
			vec3_t delta;
			vec_t dist;
			VectorSubtract (backorigin, emitpatch->origin, delta);
			dist = VectorLength (delta);
			if (dist < emitpatch->emitter_range - ON_EPSILON)
			{
				GetAlternateOrigin (backorigin, backnormal, emitpatch, emitorigin);
			}
			else
			{
				VectorCopy (emitpatch->origin, emitorigin);
			}
			if (DotProduct (emitorigin, backnormal) <= DotProduct (backorigin, backnormal) + MINIMUM_PATCH_DISTANCE)
			{
				return false;
			}
            if (TestLine(
				backorigin, emitorigin
				) != CONTENTS_EMPTY)
			{
				return false;
			}
            if (TestSegmentAgainstOpaqueList(
				backorigin, emitorigin
				, transparency
				, opaquestyle
				))
			{
				return false;
			}

            {
				if (opaquestyle != -1)
				{
					AddStyleToStyleArray (receiver, emitter, opaquestyle);
				}
            	if(g_customshadow_with_bouncelight)
            	{
            		VectorCopy(transparency, transparency_out);
            	}
                return true;
            }
        }
    }

    return false;
}

//
// end old vismat.c
////////////////////////////

void            MakeScalesNoVismatrix()
{
    char            transferfile[_MAX_PATH];

    hlassume(g_num_patches < MAX_PATCHES, assume_MAX_PATCHES);

	safe_snprintf(transferfile, _MAX_PATH, "%s.inc", g_Mapname);

    if (!g_incremental || !readtransfers(transferfile, g_num_patches))
    {
        g_CheckVisBit = CheckVisBitNoVismatrix;
	if(g_rgb_transfers)
		{NamedRunThreadsOn(g_num_patches, g_estimate, MakeRGBScales);}
	else
		{NamedRunThreadsOn(g_num_patches, g_estimate, MakeScales);}

        if (g_incremental)
        {
            writetransfers(transferfile, g_num_patches);
        }
        else
        {
            unlink(transferfile);
        }
        DumpTransfersMemoryUsage();
		CreateFinalStyleArrays ("dynamic shadow array");
    }
}
