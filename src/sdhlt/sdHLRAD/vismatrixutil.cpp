#include "qrad.h"

funcCheckVisBit g_CheckVisBit = NULL;

size_t          g_total_transfer = 0;
size_t          g_transfer_index_bytes = 0;
size_t          g_transfer_data_bytes = 0;

#define COMPRESSED_TRANSFERS
//#undef  COMPRESSED_TRANSFERS

static float GetDistanceSquar(const vec_t* v1, const vec_t* v2)
{
    float x = v1[0] - v2[0];
    float y = v1[1] - v2[1];
    float z = v1[2] - v2[2];
    return x * x + y * y + z * z;
}

int             FindTransferOffsetPatchnum(transfer_index_t* tIndex, const patch_t* const patch, const unsigned patchnum)
{
    //
    // binary search for match
    //
    int             low = 0;
    int             high = patch->iIndex - 1;
    int             offset;

    while (1)
    {
        offset = (low + high) / 2;

        if ((tIndex[offset].index + tIndex[offset].size) < patchnum)
        {
            low = offset + 1;
        }
        else if (tIndex[offset].index > patchnum)
        {
            high = offset - 1;
        }
        else
        {
            unsigned        x;
            unsigned int    rval = 0;
            transfer_index_t* pIndex = tIndex;

            for (x = 0; x < offset; x++, pIndex++)
            {
                rval += pIndex->size + 1;
            }
            rval += patchnum - tIndex[offset].index;
            return rval;
        }
        if (low > high)
        {
            return -1;
        }
    }
}

#ifdef COMPRESSED_TRANSFERS

static unsigned GetLengthOfRun(const transfer_raw_index_t* raw, const transfer_raw_index_t* const end)
{
    unsigned        run_size = 0;

    while (raw < end)
    {
        if (((*raw) + 1) == (*(raw + 1)))
        {
            raw++;
            run_size++;

            if (run_size >= MAX_COMPRESSED_TRANSFER_INDEX_SIZE)
            {
                return run_size;
            }
        }
        else
        {
            return run_size;
        }
    }
    return run_size;
}

static transfer_index_t* CompressTransferIndicies(transfer_raw_index_t* tRaw, const unsigned rawSize, unsigned* iSize)
{
    unsigned        x;
    unsigned        size = rawSize;
    unsigned        compressed_count = 0;

    transfer_raw_index_t* raw = tRaw;
    transfer_raw_index_t* end = tRaw + rawSize - 1;        // -1 since we are comparing current with next and get errors when bumping into the 'end'

    unsigned        compressed_count_1 = 0;

	for (x = 0; x < rawSize; x++)
	{
		x += GetLengthOfRun (tRaw + x, tRaw + rawSize - 1);
		compressed_count_1++;
	}

	if (!compressed_count_1)
	{
		return NULL;
	}

	transfer_index_t* CompressedArray = (transfer_index_t*)AllocBlock(sizeof(transfer_index_t) * compressed_count_1);
    transfer_index_t* compressed = CompressedArray;

    for (x = 0; x < size; x++, raw++, compressed++)
    {
        compressed->index = (*raw);
        compressed->size = GetLengthOfRun(raw, end);       // Zero based (count 0 still implies 1 item in the list, so 256 max entries result)
        raw += compressed->size;
        x += compressed->size;
        compressed_count++;                                // number of entries in compressed table
    }

    *iSize = compressed_count;

	if (compressed_count != compressed_count_1)
	{
		Error ("CompressTransferIndicies: internal error");
	}

	ThreadLock();
	g_transfer_index_bytes += sizeof(transfer_index_t) * compressed_count;
	ThreadUnlock();

	return CompressedArray;
}

#else /*COMPRESSED_TRANSFERS*/

static transfer_index_t* CompressTransferIndicies(const transfer_raw_index_t* tRaw, const unsigned rawSize, unsigned* iSize)
{
    unsigned        x;
    unsigned        size = rawSize;
    unsigned        compressed_count = 0;

    transfer_raw_index_t* raw = tRaw;
    transfer_raw_index_t* end = tRaw + rawSize;

	if (!size)
	{
		return NULL;
	}

	transfer_index_t CompressedArray = (transfer_index_t*)AllocBlock(sizeof(transfer_index_t) * size);
    transfer_index_t* compressed = CompressedArray;

    for (x = 0; x < size; x++, raw++, compressed++)
    {
        compressed->index = (*raw);
        compressed->size = 0;
        compressed_count++;                                // number of entries in compressed table
    }

    *iSize = compressed_count;

	ThreadLock();
	g_transfer_index_bytes += sizeof(transfer_index_t) * size;
	ThreadUnlock();

	return CompressedArray;
}
#endif /*COMPRESSED_TRANSFERS*/

/*
 * =============
 * MakeScales
 * 
 * This is the primary time sink.
 * It can be run multi threaded.
 * =============
 */
#ifdef SYSTEM_WIN32
#pragma warning(push)
#pragma warning(disable: 4100)                             // unreferenced formal parameter
#endif
void MakeScales(const int threadnum)
{
    int             i;
    unsigned        j;
    vec3_t          delta;
    vec_t           dist;
    int             count;
    float           trans;
    patch_t* patch;
    patch_t* patch2;
    float           send;
    vec3_t          origin;
    vec_t           area;
    const vec_t* normal1;
    const vec_t* normal2;

    unsigned int    fastfind_index = 0;

    vec_t           total;

    transfer_raw_index_t* tIndex;
    float* tData;

    transfer_raw_index_t* tIndex_All = (transfer_raw_index_t*)AllocBlock(sizeof(transfer_index_t) * (g_num_patches + 1));
    float* tData_All = (float*)AllocBlock(sizeof(float) * (g_num_patches + 1));

    count = 0;

    while (1)
    {
        i = GetThreadWork();
        if (i == -1)
            break;

        patch = g_patches + i;
        patch->iIndex = 0;
        patch->iData = 0;

        tIndex = tIndex_All;
        tData = tData_All;

        VectorCopy(patch->origin, origin);
        normal1 = getPlaneFromFaceNumber(patch->faceNumber)->normal;

        area = patch->area;
        vec3_t backorigin;
        vec3_t backnormal;
        if (patch->translucent_b)
        {
            VectorMA(patch->origin, -(g_translucentdepth + 2 * PATCH_HUNT_OFFSET), normal1, backorigin);
            VectorSubtract(vec3_origin, normal1, backnormal);
        }
        bool lighting_diversify;
        vec_t lighting_power;
        vec_t lighting_scale;
        int miptex = g_texinfo[g_dfaces[patch->faceNumber].texinfo].miptex;
        lighting_power = g_lightingconeinfo[miptex][0];
        lighting_scale = g_lightingconeinfo[miptex][1];
        lighting_diversify = (lighting_power != 1.0 || lighting_scale != 1.0);

        for (j = 0, patch2 = g_patches; j < g_num_patches; j++, patch2++)
        {
            vec_t           dot1;
            vec_t           dot2;

            vec3_t          transparency = { 1.0,1.0,1.0 };
            bool useback = false;

            vec3_t delta_pre;
            VectorSubtract(patch2->origin, origin, delta_pre);
            vec_t dist2 = DotProduct(delta_pre, delta_pre);

            if (patch2->area / dist2 < 1e-6f) //Conservative upper bound, so cull is safe (|trans| <= area/dist^2)
            {
                continue;
            }
            normal2 = getPlaneFromFaceNumber(patch2->faceNumber)->normal;
            vec_t dot1_pre = DotProduct(delta_pre, normal1);

            if (dot1_pre <= NORMAL_EPSILON)
            {
                if (patch->translucent_b)
                {
                    vec_t dot1_back = DotProduct(delta_pre, backnormal);

                    if (dot1_back <= NORMAL_EPSILON)
                    {
                        continue;
                    }
                    useback = true;
                }
                else
                    continue;
            }
            vec_t dot2_pre = -DotProduct(delta_pre, normal2);
            vec_t dist_pre = sqrt(dist2);

            if (dot2_pre * dist_pre <= MINIMUM_PATCH_DISTANCE)
            {
                continue;
            }
            //Expensive
            if (!g_CheckVisBit(i, j, transparency, fastfind_index) || (i == j))
            {
                if (patch->translucent_b)
                {
                    if ((i == j) || !CheckVisBitBackwards(i, j, backorigin, backnormal, transparency))
                    {
                        continue;
                    }
                    useback = true;
                }
                else
                {
                    continue;
                }
            }
            if (useback)
            {
                VectorSubtract(patch2->origin, backorigin, delta);
            }
            else
            {
                VectorSubtract(patch2->origin, origin, delta);
            }
            VectorMA(delta, -PATCH_HUNT_OFFSET, normal2, delta); //Actual transfer
            dist = VectorNormalize(delta);
            dot1 = DotProduct(delta, useback ? backnormal : normal1);
            dot2 = -DotProduct(delta, normal2);
            bool light_behind_surface = false;

            if (dot1 <= NORMAL_EPSILON)
            {
                light_behind_surface = true;
            }
            if (dot2 * dist <= MINIMUM_PATCH_DISTANCE)
            {
                continue;
            }

            if (lighting_diversify && !light_behind_surface)
            {
                dot1 = lighting_scale * pow(dot1, lighting_power);
            }
            trans = (dot1 * dot2) / (dist * dist);
            if (trans * patch2->area > 0.8f)
                trans = 0.8f / patch2->area;
            if (dist < patch2->emitter_range - ON_EPSILON)
            {
                if (light_behind_surface)
                {
                    trans = 0.0;
                }
                vec_t sightarea;
                const vec_t* receiver_origin;
                const vec_t* receiver_normal;
                const Winding* emitter_winding;
                receiver_origin = origin;
                receiver_normal = normal1;
                if (useback)
                {
                    receiver_origin = backorigin;
                    receiver_normal = backnormal;
                }
                emitter_winding = patch2->winding;
                sightarea = CalcSightArea(receiver_origin, receiver_normal, emitter_winding, patch2->emitter_skylevel, lighting_power, lighting_scale);

                vec_t frac;
                frac = dist / patch2->emitter_range;
                frac = (frac - 0.5f) * 2.0f;
                frac = qmax(0, qmin(frac, 1));
                trans = frac * trans + (1 - frac) * (sightarea / patch2->area);
            }
            else
            {
                if (light_behind_surface)
                {
                    continue;
                }
            }

            trans *= patch2->exposure;
            trans = trans * VectorAvg(transparency);
            if (patch->translucent_b)
            {
                if (useback)
                {
                    trans *= VectorAvg(patch->translucent_v);
                }
                else
                {
                    trans *= 1 - VectorAvg(patch->translucent_v);
                }
            }

            trans = trans * patch2->area;
            if (trans <= 0.0)
            {
                continue;
            }

            *tData = trans;
            *tIndex = j;
            tData++;
            tIndex++;
            patch->iData++;
            count++;
        }

        if (patch->iData)
        {
            unsigned    data_size = patch->iData * float_size[g_transfer_compress_type] + unused_size;

            patch->tData = (transfer_data_t*)AllocBlock(data_size);
            patch->tIndex = CompressTransferIndicies(tIndex_All, patch->iData, &patch->iIndex);

            hlassume(patch->tData != NULL, assume_NoMemory);
            hlassume(patch->tIndex != NULL, assume_NoMemory);

            ThreadLock();
            g_transfer_data_bytes += data_size;
            ThreadUnlock();

            total = 1 / Q_PI;
            {
                unsigned        x;
                transfer_data_t* t1 = patch->tData;
                float* t2 = tData_All;

                float    f;
                for (x = 0; x < patch->iData; x++, t1 += float_size[g_transfer_compress_type], t2++)
                {
                    f = (*t2) * total;
                    float_compress(g_transfer_compress_type, t1, &f);
                }
            }
        }
    }

    FreeBlock(tIndex_All);
    FreeBlock(tData_All);

    ThreadLock();
    g_total_transfer += count;
    ThreadUnlock();
}

#ifdef SYSTEM_WIN32
#pragma warning(pop)
#endif

/*
 * =============
 * SwapTransfersTask
 * 
 * Change transfers from light sent out to light collected in.
 * In an ideal world, they would be exactly symetrical, but
 * because the form factors are only aproximated, then normalized,
 * they will actually be rather different.
 * =============
 */

/*
 * =============
 * MakeScales
 * 
 * This is the primary time sink.
 * It can be run multi threaded.
 * =============
 */
#ifdef SYSTEM_WIN32
#pragma warning(push)
#pragma warning(disable: 4100)                             // unreferenced formal parameter
#endif
void MakeRGBScales(const int threadnum)
{
    int             i;
    unsigned        j;
    vec3_t          delta;
    vec_t           dist;
    int             count;
    vec3_t          trans;
    patch_t* patch;
    patch_t* patch2;
    float           send;
    vec3_t          origin;
    vec_t           area;
    const vec_t* normal1;
    const vec_t* normal2;

    unsigned int    fastfind_index = 0;

    vec_t           total;

    transfer_raw_index_t* tIndex;
    vec3_t* tData;

    transfer_raw_index_t* tIndex_All = (transfer_raw_index_t*)AllocBlock(sizeof(transfer_index_t) * (g_num_patches + 1));
    vec3_t* tData_All = (vec3_t*)AllocBlock(sizeof(vec3_t) * (g_num_patches + 1));

    count = 0;

    while (1)
    {
        i = GetThreadWork();
        if (i == -1)
            break;

        patch = g_patches + i;
        patch->iIndex = 0;
        patch->iData = 0;

        tIndex = tIndex_All;
        tData = tData_All;

        VectorCopy(patch->origin, origin);
        normal1 = getPlaneFromFaceNumber(patch->faceNumber)->normal;

        area = patch->area;
        vec3_t backorigin;
        vec3_t backnormal;
        if (patch->translucent_b)
        {
            VectorMA(patch->origin, -(g_translucentdepth + 2 * PATCH_HUNT_OFFSET), normal1, backorigin);
            VectorSubtract(vec3_origin, normal1, backnormal);
        }
        bool lighting_diversify;
        vec_t lighting_power;
        vec_t lighting_scale;
        int miptex = g_texinfo[g_dfaces[patch->faceNumber].texinfo].miptex;
        lighting_power = g_lightingconeinfo[miptex][0];
        lighting_scale = g_lightingconeinfo[miptex][1];
        lighting_diversify = (lighting_power != 1.0 || lighting_scale != 1.0);

        for (j = 0, patch2 = g_patches; j < g_num_patches; j++, patch2++)
        {
            vec_t           dot1;
            vec_t           dot2;

            vec3_t          transparency = { 1.0,1.0,1.0 };
            bool useback = false;

            vec3_t delta_pre;
            VectorSubtract(patch2->origin, origin, delta_pre);
            vec_t dist2 = DotProduct(delta_pre, delta_pre);

            if (patch2->area / dist2 < 1e-6f) //Conservative upper bound, so cull is safe (|trans| <= area/dist^2)
            {
                continue;
            }
            normal2 = getPlaneFromFaceNumber(patch2->faceNumber)->normal;
            vec_t dot1_pre = DotProduct(delta_pre, normal1);

            if (dot1_pre <= NORMAL_EPSILON)
            {
                if (patch->translucent_b)
                {
                    vec_t dot1_back = DotProduct(delta_pre, backnormal);

                    if (dot1_back <= NORMAL_EPSILON)
                    {
                        continue;
                    }
                    useback = true;
                }
                else
                    continue;
            }
            vec_t dot2_pre = -DotProduct(delta_pre, normal2);
            vec_t dist_pre = sqrt(dist2);

            if (dot2_pre * dist_pre <= MINIMUM_PATCH_DISTANCE)
            {
                continue;
            }
            //Expensive
            if (!g_CheckVisBit(i, j, transparency, fastfind_index) || (i == j))
            {
                if (patch->translucent_b)
                {
                    if ((i == j) || !CheckVisBitBackwards(i, j, backorigin, backnormal, transparency))
                    {
                        continue;
                    }
                    useback = true;
                }
                else
                {
                    continue;
                }
            }
            if (useback)
            {
                VectorSubtract(patch2->origin, backorigin, delta);
            }
            else
            {
                VectorSubtract(patch2->origin, origin, delta);
            }
            VectorMA(delta, -PATCH_HUNT_OFFSET, normal2, delta); //Actual transfer
            dist = VectorNormalize(delta);
            dot1 = DotProduct(delta, normal1);
            if (useback)
            {
                dot1 = DotProduct(delta, backnormal);
            }
            dot2 = -DotProduct(delta, normal2);
            bool light_behind_surface = false;
            if (dot1 <= NORMAL_EPSILON)
            {
                light_behind_surface = true;
            }
            if (dot2 * dist <= MINIMUM_PATCH_DISTANCE)
            {
                continue;
            }

            if (lighting_diversify && !light_behind_surface)
            {
                dot1 = lighting_scale * pow(dot1, lighting_power);
            }
            trans[0] = (dot1 * dot2) / (dist * dist);
            trans[1] = trans[0];
            trans[2] = trans[0];
            if (trans[0] * patch2->area > 0.8f)
            {
                trans[0] = 0.8f / patch2->area;
                trans[1] = trans[0];
                trans[2] = trans[0];
            }
            if (dist < patch2->emitter_range - ON_EPSILON)
            {
                if (light_behind_surface)
                {
                    VectorClear(trans);
                }
                vec_t sightarea;
                const vec_t* receiver_origin;
                const vec_t* receiver_normal;
                const Winding* emitter_winding;
                receiver_origin = origin;
                receiver_normal = normal1;
                if (useback)
                {
                    receiver_origin = backorigin;
                    receiver_normal = backnormal;
                }
                emitter_winding = patch2->winding;
                sightarea = CalcSightArea(receiver_origin, receiver_normal, emitter_winding, patch2->emitter_skylevel, lighting_power, lighting_scale);

                vec_t frac;
                frac = dist / patch2->emitter_range;
                frac = (frac - 0.5f) * 2.0f;
                frac = qmax(0, qmin(frac, 1));
                vec_t base = sightarea / patch2->area;
                for (int c = 0; c < 3; c++)
                {
                    trans[c] = frac * trans[c] + (1 - frac) * base;
                }
            }
            else
            {
                if (light_behind_surface)
                {
                    continue;
                }
            }

            for (int c = 0; c < 3; c++)
            {
                trans[c] *= patch2->exposure;
                trans[c] *= transparency[c];
                if (patch->translucent_b)
                {
                    if (useback)
                    {
                        trans[c] *= patch->translucent_v[c];
                    }
                    else
                    {
                        trans[c] *= 1 - patch->translucent_v[c];
                    }
                }
                trans[c] *= patch2->area;
            }

            if (trans[0] <= 0.0 && trans[1] <= 0.0 && trans[2] <= 0.0)
            {
                continue;
            }

            VectorCopy(trans, *tData);
            *tIndex = j;
            tData++;
            tIndex++;
            patch->iData++;
            count++;
        }

        if (patch->iData)
        {
            unsigned    data_size = patch->iData * vector_size[g_rgbtransfer_compress_type] + unused_size;

            patch->tRGBData = (rgb_transfer_data_t*)AllocBlock(data_size);
            patch->tIndex = CompressTransferIndicies(tIndex_All, patch->iData, &patch->iIndex);

            hlassume(patch->tRGBData != NULL, assume_NoMemory);
            hlassume(patch->tIndex != NULL, assume_NoMemory);

            ThreadLock();
            g_transfer_data_bytes += data_size;
            ThreadUnlock();

            total = 1 / Q_PI;
            {
                unsigned        x;
                rgb_transfer_data_t* t1 = patch->tRGBData;
                vec3_t* t2 = tData_All;

                float    f[3];
                for (x = 0; x < patch->iData; x++, t1 += vector_size[g_rgbtransfer_compress_type], t2++)
                {
                    for (int c = 0; c < 3; c++)
                        f[c] = (*t2)[c] * total;
                    vector_compress(g_rgbtransfer_compress_type, t1, &f[0], &f[1], &f[2]);
                }
            }
        }
    }

    FreeBlock(tIndex_All);
    FreeBlock(tData_All);

    ThreadLock();
    g_total_transfer += count;
    ThreadUnlock();
}

#ifdef SYSTEM_WIN32
#pragma warning(pop)
#endif

/*
 * =============
 * SwapTransfersTask
 * 
 * Change transfers from light sent out to light collected in.
 * In an ideal world, they would be exactly symetrical, but
 * because the form factors are only aproximated, then normalized,
 * they will actually be rather different.
 * =============
 */




//More human readable numbers
void            DumpTransfersMemoryUsage()
{
	if(g_total_transfer > 1000*1000)
		Log("Transfer Lists : %11.0f : %8.2fM transfers\n", (double)g_total_transfer, (double)g_total_transfer/(1000.0f*1000.0f));
	else if(g_total_transfer > 1000)
		Log("Transfer Lists : %11.0f : %8.2fk transfers\n", (double)g_total_transfer, (double)g_total_transfer/1000.0f);
	else
		Log("Transfer Lists : %11.0f transfers\n", (double)g_total_transfer);
	
	if(g_transfer_index_bytes > 1024*1024)
		Log("       Indices : %11.0f : %8.2fM bytes\n", (double)g_transfer_index_bytes, (double)g_transfer_index_bytes/(1024.0f * 1024.0f));
	else if(g_transfer_index_bytes > 1024)
		Log("       Indices : %11.0f : %8.2fk bytes\n", (double)g_transfer_index_bytes, (double)g_transfer_index_bytes/1024.0f);
	else
		Log("       Indices : %11.0f bytes\n", (double)g_transfer_index_bytes);
	
	if(g_transfer_data_bytes > 1024*1024)
		Log("          Data : %11.0f : %8.2fM bytes\n", (double)g_transfer_data_bytes, (double)g_transfer_data_bytes/(1024.0f * 1024.0f));
	else if(g_transfer_data_bytes > 1024)
		Log("          Data : %11.0f : %8.2fk bytes\n", (double)g_transfer_data_bytes, (double)g_transfer_data_bytes/1024.0f);
	else
		Log("          Data : %11.0f bytes\n", (double)g_transfer_data_bytes);
}


