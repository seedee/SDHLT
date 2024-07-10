//#pragma warning(disable: 4018) // '<' : signed/unsigned mismatch

/*
 
    CONSTRUCTIVE SOLID GEOMETRY    -aka-    C S G 

    Code based on original code from Valve Software, 
    Modified by Sean "Zoner" Cavanaugh (seanc@gearboxsoftware.com) with permission.
    Modified by Tony "Merl" Moore (merlinis@bigpond.net.au) [AJM]
    Modified by amckern (amckern@yahoo.com)
    Modified by vluzacn (vluzacn@163.com)
    Modified by seedee (cdaniel9000@gmail.com)
    
*/

#include "csg.h" 
#ifdef SYSTEM_WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h> //--vluzacn
#endif

/*

 NOTES

 - check map size for +/- 4k limit at load time
 - allow for multiple wad.cfg configurations per compile

*/

static FILE*    out[NUM_HULLS]; // pointer to each of the hull out files (.p0, .p1, ect.)  
static FILE*    out_view[NUM_HULLS];
static FILE*    out_detailbrush[NUM_HULLS];
static int      c_tiny;        
static int      c_tiny_clip;
static int      c_outfaces;
static int      c_csgfaces;
BoundingBox     world_bounds;


vec_t           g_tiny_threshold = DEFAULT_TINY_THRESHOLD;
     
bool            g_noclip = DEFAULT_NOCLIP;              // no clipping hull "-noclip"
bool            g_onlyents = DEFAULT_ONLYENTS;          // onlyents mode "-onlyents"
bool            g_wadtextures = DEFAULT_WADTEXTURES;    // "-nowadtextures"
bool            g_chart = DEFAULT_CHART;                // show chart "-chart"
bool            g_skyclip = DEFAULT_SKYCLIP;            // no sky clipping "-noskyclip"
bool            g_estimate = DEFAULT_ESTIMATE;          // progress estimates "-estimate"
bool            g_info = DEFAULT_INFO;                  // "-info" ?
const char*     g_hullfile = NULL;                      // external hullfile "-hullfie sdfsd"
const char*		g_wadcfgfile = NULL;
const char*		g_wadconfigname = NULL;

bool            g_bUseNullTex = DEFAULT_NULLTEX;        // "-nonulltex"

cliptype		g_cliptype = DEFAULT_CLIPTYPE;			// "-cliptype <value>"

const char*			g_nullfile = NULL;

bool            g_bClipNazi = DEFAULT_CLIPNAZI;         // "-noclipeconomy"

bool            g_bWadAutoDetect = DEFAULT_WADAUTODETECT; // "-nowadautodetect"


vec_t g_scalesize = DEFAULT_SCALESIZE;
bool g_resetlog = DEFAULT_RESETLOG;
bool g_nolightopt = DEFAULT_NOLIGHTOPT;
#ifdef HLCSG_GAMETEXTMESSAGE_UTF8
bool g_noutf8 = DEFAULT_NOUTF8;
#endif
bool g_nullifytrigger = DEFAULT_NULLIFYTRIGGER;
bool g_viewsurface = false;

// =====================================================================================
//  GetParamsFromEnt
//      parses entity keyvalues for setting information
// =====================================================================================
void            GetParamsFromEnt(entity_t* mapent)
{
    int     iTmp;
    char    szTmp[256];

    Log("\nCompile Settings detected from info_compile_parameters entity\n");

    // verbose(choices) : "Verbose compile messages" : 0 = [ 0 : "Off" 1 : "On" ]
    iTmp = IntForKey(mapent, "verbose");
    if (iTmp == 1)
    {
        g_verbose = true;
    }
    else if (iTmp == 0)
    {
        g_verbose = false;
    }
    Log("%30s [ %-9s ]\n", "Compile Option", "setting");
    Log("%30s [ %-9s ]\n", "Verbose Compile Messages", g_verbose ? "on" : "off");

    // estimate(choices) :"Estimate Compile Times?" : 0 = [ 0: "Yes" 1: "No" ]
    if (IntForKey(mapent, "estimate")) 
    {
        g_estimate = true;
    }
    else
    {
        g_estimate = false;
    }
    Log("%30s [ %-9s ]\n", "Estimate Compile Times", g_estimate ? "on" : "off");

	// priority(choices) : "Priority Level" : 0 = [	0 : "Normal" 1 : "High"	-1 : "Low" ]
	if (!strcmp(ValueForKey(mapent, "priority"), "1"))
    {
        g_threadpriority = eThreadPriorityHigh;
        Log("%30s [ %-9s ]\n", "Thread Priority", "high");
    }
    else if (!strcmp(ValueForKey(mapent, "priority"), "-1"))
    {
        g_threadpriority = eThreadPriorityLow;
        Log("%30s [ %-9s ]\n", "Thread Priority", "low");
    }

    // texdata(string) : "Texture Data Memory" : "4096"
    iTmp = IntForKey(mapent, "texdata") * 1024;
    if (iTmp > g_max_map_miptex)
    {
        g_max_map_miptex = iTmp;
    }
	sprintf_s(szTmp, "%i", g_max_map_miptex);
    Log("%30s [ %-9s ]\n", "Texture Data Memory", szTmp);

    // hullfile(string) : "Custom Hullfile"
    if (ValueForKey(mapent, "hullfile"))
    {
        g_hullfile = ValueForKey(mapent, "hullfile");
        Log("%30s [ %-9s ]\n", "Custom Hullfile", g_hullfile);
    }

    // wadautodetect(choices) : "Wad Auto Detect" : 0 =	[ 0 : "Off" 1 : "On" ]
    /*if (!strcmp(ValueForKey(mapent, "wadautodetect"), "1"))
    { 
        g_bWadAutoDetect = true;
    }
    else
    {
        g_bWadAutoDetect = false;
    }*/
    const char* wadautodetectValue = ValueForKey(mapent, "wadautodetect"); //seedee
    g_bWadAutoDetect = (wadautodetectValue && atoi(wadautodetectValue) >= 1);
    Log("%30s [ %-9s ]\n", "Wad Auto Detect", g_bWadAutoDetect ? "on" : "off");
	
	// wadconfig(string) : "Custom Wad Configuration" : ""
    if (*ValueForKey(mapent, "wadconfig"))
    {
        g_wadconfigname = _strdup (ValueForKey(mapent, "wadconfig"));
        Log("%30s [ %-9s ]\n", "Custom Wad Configuration Name", g_wadconfigname);
    }
	// wadcfgfile(string) : "Custom Wad Configuration File" : ""
    if (*ValueForKey(mapent, "wadcfgfile"))
    {
        g_wadcfgfile = _strdup (ValueForKey(mapent, "wadcfgfile"));
        Log("%30s [ %-9s ]\n", "Custom Wad Configuration File", g_wadcfgfile);
    }

    // noclipeconomy(choices) : "Strip Uneeded Clipnodes?" : 1 = [ 1 : "Yes" 0 : "No" ]
    iTmp = IntForKey(mapent, "noclipeconomy");
    if (iTmp == 1)
    {
        g_bClipNazi = true;
    }
    else if (iTmp == 0)
    {
        g_bClipNazi = false;
    }        
    Log("%30s [ %-9s ]\n", "Clipnode Economy Mode", g_bClipNazi ? "on" : "off");

    /*
    hlcsg(choices) : "HLCSG" : 1 =
    [
        1 : "Normal"
        2 : "Onlyents"
        0 : "Off"
    ]
    */
    iTmp = IntForKey(mapent, "hlcsg");
    g_onlyents = false;
    if (iTmp == 2)
    {
        g_onlyents = true;
    }
    else if (iTmp == 0)
    {
        Fatal(assume_TOOL_CANCEL, 
            "%s was set to \"Off\" (0) in info_compile_parameters entity, execution cancelled", g_Program);
        CheckFatal();  
    }
    Log("%30s [ %-9s ]\n", "Onlyents", g_onlyents ? "on" : "off");

    /*
    nocliphull(choices) : "Generate clipping hulls" : 0 =
    [
        0 : "Yes"
        1 : "No"
    ]
    */
    iTmp = IntForKey(mapent, "nocliphull");
    if (iTmp == 1)
    {
        g_noclip = true;
    }
    else 
    {
        g_noclip = false;
    }
    Log("%30s [ %-9s ]\n", "Clipping Hull Generation", g_noclip ? "off" : "on");
    // cliptype(choices) : "Clip Hull Type" : 4 = [ 0 : "Smallest" 1 : "Normalized" 2: "Simple" 3 : "Precise" 4 : "Legacy" ]
    iTmp = IntForKey(mapent, "cliptype");
	switch(iTmp)
	{
	case 0:
		g_cliptype = clip_smallest;
		break;
	case 1:
		g_cliptype = clip_normalized;
		break;
	case 2:
		g_cliptype = clip_simple;
		break;
	case 3:
		g_cliptype = clip_precise;
		break;
	default:
		g_cliptype = clip_legacy;
		break;
	}
    Log("%30s [ %-9s ]\n", "Clip Hull Type", GetClipTypeString(g_cliptype));
    /*
    noskyclip(choices) : "No Sky Clip" : 0 =
    [
        1 : "On"
        0 : "Off"
    ]
    */
    iTmp = IntForKey(mapent, "noskyclip");
    if (iTmp == 1)
    {
        g_skyclip = false;
    }
    else 
    {
        g_skyclip = true;
    }
    Log("%30s [ %-9s ]\n", "Sky brush clip generation", g_skyclip ? "on" : "off");

    ///////////////
    Log("\n");
}


// =====================================================================================
//  NewFaceFromFace
//      Duplicates the non point information of a face, used by SplitFace
// =====================================================================================
bface_t*        NewFaceFromFace(const bface_t* const in)
{
    bface_t*        newf;

    newf = (bface_t*)Alloc(sizeof(bface_t));

    newf->contents = in->contents;
    newf->texinfo = in->texinfo;
    newf->planenum = in->planenum;
    newf->plane = in->plane;
	newf->backcontents = in->backcontents;

    return newf;
}

// =====================================================================================
//  FreeFace
// =====================================================================================
void            FreeFace(bface_t* f)
{
    delete f->w;
    Free(f);
}


// =====================================================================================
//  WriteFace
// =====================================================================================
void            WriteFace(const int hull, const bface_t* const f
						  , int detaillevel
						  )
{
    unsigned int    i;
    Winding*        w;

    ThreadLock();
    if (!hull)
        c_csgfaces++;

    // .p0 format
    w = f->w;

    // plane summary
	fprintf (out[hull], "%i %i %i %i %u\n", detaillevel, f->planenum, f->texinfo, f->contents, w->m_NumPoints);

    // for each of the points on the face
    for (i = 0; i < w->m_NumPoints; i++)
    {
        // write the co-ords
        fprintf(out[hull], "%5.8f %5.8f %5.8f\n", w->m_Points[i][0], w->m_Points[i][1], w->m_Points[i][2]);
    }

    // put in an extra line break
    fprintf(out[hull], "\n");
	if (g_viewsurface)
	{
		static bool side = false;
		side = !side;
		if (side)
		{
			vec3_t center, center2;
			w->getCenter (center);
			VectorAdd (center, f->plane->normal, center2);
			fprintf (out_view[hull], "%5.2f %5.2f %5.2f\n", center2[0], center2[1], center2[2]);
			for (i = 0; i < w->m_NumPoints; i++)
			{
				vec_t *p1, *p2;
				p1 = w->m_Points[i];
				p2 = w->m_Points[(i+1)%w->m_NumPoints];
				fprintf (out_view[hull], "%5.2f %5.2f %5.2f\n", center[0], center[1], center[2]);
				fprintf (out_view[hull], "%5.2f %5.2f %5.2f\n", p1[0], p1[1], p1[2]);
				fprintf (out_view[hull], "%5.2f %5.2f %5.2f\n", p2[0], p2[1], p2[2]);
			}
			fprintf (out_view[hull], "%5.2f %5.2f %5.2f\n", center[0], center[1], center[2]);
			fprintf (out_view[hull], "%5.2f %5.2f %5.2f\n", center2[0], center2[1], center2[2]);
		}
	}

    ThreadUnlock();
}
void WriteDetailBrush (int hull, const bface_t *faces)
{
	ThreadLock ();
	fprintf (out_detailbrush[hull], "0\n");
	for (const bface_t *f = faces; f; f = f->next)
	{
		Winding *w = f->w;
		fprintf (out_detailbrush[hull], "%i %u\n", f->planenum, w->m_NumPoints);
		for (int i = 0; i < w->m_NumPoints; i++)
		{
			fprintf (out_detailbrush[hull], "%5.8f %5.8f %5.8f\n", w->m_Points[i][0], w->m_Points[i][1], w->m_Points[i][2]);
		}
	}
	fprintf (out_detailbrush[hull], "-1 -1\n");
	ThreadUnlock ();
}

// =====================================================================================
//  SaveOutside
//      The faces remaining on the outside list are final polygons.  Write them to the 
//      output file.
//      Passable contents (water, lava, etc) will generate a mirrored copy of the face 
//      to be seen from the inside.
// =====================================================================================
static void     SaveOutside(const brush_t* const b, const int hull, bface_t* outside, const int mirrorcontents)
{
    bface_t*        f;
    bface_t*        f2;
    bface_t*        next;
    int             i;
    vec3_t          temp;

    for (f = outside; f; f = next)
    {
        next = f->next;

		int frontcontents, backcontents;
		int texinfo = f->texinfo;
		const char *texname = GetTextureByNumber_CSG (texinfo);
		frontcontents = f->contents;
		if (mirrorcontents == CONTENTS_TOEMPTY)
		{
			backcontents = f->backcontents;
		}
		else
		{
			backcontents = mirrorcontents;
		}
		if (frontcontents == CONTENTS_TOEMPTY)
		{
			frontcontents = CONTENTS_EMPTY;
		}
		if (backcontents == CONTENTS_TOEMPTY)
		{
			backcontents = CONTENTS_EMPTY;
		}

		bool frontnull, backnull;
		frontnull = false;
		backnull = false;
		if (mirrorcontents == CONTENTS_TOEMPTY)
		{
            if (strncasecmp(texname, "SKIP", 4)
                && strncasecmp(texname, "HINT", 4)
                && strncasecmp(texname, "SOLIDHINT", 9)
                && strncasecmp(texname, "BEVELHINT", 9)
                )
				// SKIP and HINT are special textures for hlbsp
			{
				backnull = true;
			}
		}
        if (!strncasecmp(texname, "SOLIDHINT", 9)
            || !strncasecmp(texname, "BEVELHINT", 9))
		{
			if (frontcontents != backcontents)
			{
				frontnull = backnull = true; // not discardable, so remove "SOLIDHINT" texture name and behave like NULL
			}
		}
		if (b->entitynum != 0 && !strncasecmp (texname, "!", 1))
		{
			backnull = true; // strip water face on one side
		}

		f->contents = frontcontents;
		f->texinfo = frontnull? -1: texinfo;
        if (f->w->getArea() < g_tiny_threshold)
        {
            c_tiny++;
            Verbose("Entity %i, Brush %i: tiny fragment\n", 
				b->originalentitynum, b->originalbrushnum
				);
            continue;
        }

        // count unique faces
        if (!hull)
        {
            for (f2 = b->hulls[hull].faces; f2; f2 = f2->next)
            {
                if (f2->planenum == f->planenum)
                {
                    if (!f2->used)
                    {
                        f2->used = true;
                        c_outfaces++;
                    }
                    break;
                }
            }
        }

		// check the texture alignment of this face
		if (!hull)
		{
			int texinfo = f->texinfo;
			const char *texname = GetTextureByNumber_CSG (texinfo);
			texinfo_t *tex = &g_texinfo[texinfo];

            if (texinfo != -1 // nullified textures (NULL, BEVEL, aaatrigger, etc.)
                && !(tex->flags & TEX_SPECIAL) // sky
                && strncasecmp(texname, "SKIP", 4)
                && strncasecmp(texname, "HINT", 4) // HINT and SKIP will be nullified only after hlbsp
                && strncasecmp(texname, "SOLIDHINT", 9)
                && strncasecmp(texname, "BEVELHINT", 9)
                )
			{
				// check for "Malformed face (%d) normal"
				vec3_t texnormal;
				CrossProduct (tex->vecs[1], tex->vecs[0], texnormal);
				VectorNormalize (texnormal);
				if (fabs (DotProduct (texnormal, f->plane->normal)) <= NORMAL_EPSILON)
				{
					Warning ("Entity %i, Brush %i: Malformed texture alignment (texture %s): Texture axis perpendicular to face.",
						b->originalentitynum, b->originalbrushnum,
						texname
						);
				}

				// check for "Bad surface extents"
				bool bad;
				int i;
				int j;
				vec_t val;
				
				bad = false;
				for (i = 0; i < f->w->m_NumPoints; i++)
				{
					for (j = 0; j < 2; j++)
					{
						val = DotProduct (f->w->m_Points[i], tex->vecs[j]) + tex->vecs[j][3];
						if (val < -99999 || val > 999999)
						{
							bad = true;
						}
					}
				}
				if (bad)
				{
					Warning ("Entity %i, Brush %i: Malformed texture alignment (texture %s): Bad surface extents.",
						b->originalentitynum, b->originalbrushnum,
						texname
						);
				}
			}
		}

        WriteFace(hull, f
			, 
			(hull? b->clipnodedetaillevel: b->detaillevel)
			);

        //              if (mirrorcontents != CONTENTS_SOLID)
        {
            f->planenum ^= 1;
            f->plane = &g_mapplanes[f->planenum];
			f->contents = backcontents;
			f->texinfo = backnull? -1: texinfo;

            // swap point orders
            for (i = 0; i < f->w->m_NumPoints / 2; i++)      // add points backwards
            {
                VectorCopy(f->w->m_Points[i], temp);
                VectorCopy(f->w->m_Points[f->w->m_NumPoints - 1 - i], f->w->m_Points[i]);
                VectorCopy(temp, f->w->m_Points[f->w->m_NumPoints - 1 - i]);
            }
            WriteFace(hull, f
				, 
				(hull? b->clipnodedetaillevel: b->detaillevel)
				);
        }

        FreeFace(f);
    }
}

// =====================================================================================
//  CopyFace
// =====================================================================================
bface_t*        CopyFace(const bface_t* const f)
{
    bface_t*        n;

    n = NewFaceFromFace(f);
    n->w = f->w->Copy();
    n->bounds = f->bounds;
    return n;
}

// =====================================================================================
//  CopyFaceList
// =====================================================================================
bface_t*        CopyFaceList(bface_t* f)
{
    bface_t*        head;
    bface_t*        n;

    if (f)
    {
        head = CopyFace(f);
        n = head;
        f = f->next;

        while (f)
        {
            n->next = CopyFace(f);

            n = n->next;
            f = f->next;
        }

        return head;
    }
    else
    {
        return NULL;
    }
}

// =====================================================================================
//  FreeFaceList
// =====================================================================================
void            FreeFaceList(bface_t* f)
{
    if (f)
    {
        if (f->next)
        {
            FreeFaceList(f->next);
        }
        FreeFace(f);
    }
}

// =====================================================================================
//  CopyFacesToOutside
//      Make a copy of all the faces of the brush, so they can be chewed up by other 
//      brushes.
//      All of the faces start on the outside list.
//      As other brushes take bites out of the faces, the fragments are moved to the 
//      inside list, so they can be freed when they are determined to be completely 
//      enclosed in solid.
// =====================================================================================
static bface_t* CopyFacesToOutside(brushhull_t* bh)
{
    bface_t*        f;
    bface_t*        newf;
    bface_t*        outside;

    outside = NULL;

    for (f = bh->faces; f; f = f->next)
    {
        newf = CopyFace(f);
        newf->w->getBounds(newf->bounds);
        newf->next = outside;
        outside = newf;
    }

    return outside;
}

// =====================================================================================
//  CSGBrush
// =====================================================================================
extern const char *ContentsToString (const contents_t type);
static void     CSGBrush(int brushnum)
{
    int             hull;
    brush_t*        b1;
    brush_t*        b2;
    brushhull_t*    bh1;
    brushhull_t*    bh2;
    int             bn;
    bool            overwrite;
    bface_t*        f;
    bface_t*        f2;
    bface_t*        next;
    bface_t*        outside;
    entity_t*       e;
    vec_t           area;

    // get entity and brush info from the given brushnum that we can work with
    b1 = &g_mapbrushes[brushnum];
    e = &g_entities[b1->entitynum];

    // for each of the hulls
    for (hull = 0; hull < NUM_HULLS; hull++)
    {
        bh1 = &b1->hulls[hull];
		if (bh1->faces && 
			(hull? b1->clipnodedetaillevel: b1->detaillevel)
			)
		{
			switch (b1->contents)
			{
			case CONTENTS_ORIGIN:
			case CONTENTS_BOUNDINGBOX:
			case CONTENTS_HINT:
			case CONTENTS_TOEMPTY:
				break;
			default:
				Error ("Entity %i, Brush %i: %s brushes not allowed in detail\n", 
					b1->originalentitynum, b1->originalbrushnum, 
					ContentsToString((contents_t)b1->contents));
				break;
			case CONTENTS_SOLID:
				WriteDetailBrush (hull, bh1->faces);
				break;
			}
		}

        // set outside to a copy of the brush's faces
        outside = CopyFacesToOutside(bh1);
        overwrite = false;
		if (b1->contents == CONTENTS_TOEMPTY)
		{
			for (f = outside; f; f = f->next)
			{
				f->contents = CONTENTS_TOEMPTY;
				f->backcontents = CONTENTS_TOEMPTY;
			}
		}

        // for each brush in entity e
        for (bn = 0; bn < e->numbrushes; bn++)
        {
            // see if b2 needs to clip a chunk out of b1
			if (e->firstbrush + bn == brushnum)
			{
				continue;
			}
            overwrite = e->firstbrush + bn > brushnum;

            b2 = &g_mapbrushes[e->firstbrush + bn];
            bh2 = &b2->hulls[hull];
			if (b2->contents == CONTENTS_TOEMPTY)
				continue;
			if (
				(hull? (b2->clipnodedetaillevel - 0 > b1->clipnodedetaillevel + 0): (b2->detaillevel - b2->chopdown > b1->detaillevel + b1->chopup))
				)
				continue; // you can't chop
			if (b2->contents == b1->contents && 
				(hull? (b2->clipnodedetaillevel != b1->clipnodedetaillevel): (b2->detaillevel != b1->detaillevel))
				)
			{
				overwrite = 
					(hull? (b2->clipnodedetaillevel < b1->clipnodedetaillevel): (b2->detaillevel < b1->detaillevel))
					;
			}
			if (b2->contents == b1->contents
				&& hull == 0 && b2->detaillevel == b1->detaillevel
				&& b2->coplanarpriority != b1->coplanarpriority)
			{
				overwrite = b2->coplanarpriority > b1->coplanarpriority;
			}

            if (!bh2->faces)
                continue;                                  // brush isn't in this hull

            // check brush bounding box first
            // TODO: use boundingbox method instead
            if (bh1->bounds.testDisjoint(bh2->bounds))
            {
                continue;
            }

            // divide faces by the planes of the b2 to find which
            // fragments are inside

            f = outside;
            outside = NULL;
            for (; f; f = next)
            {
                next = f->next;

                // check face bounding box first
                if (bh2->bounds.testDisjoint(f->bounds))
                {                                          // this face doesn't intersect brush2's bbox
                    f->next = outside;
                    outside = f;
                    continue;
                }
				if (
					(hull? (b2->clipnodedetaillevel > b1->clipnodedetaillevel): (b2->detaillevel > b1->detaillevel))
					)
				{
					const char *texname = GetTextureByNumber_CSG (f->texinfo);
                    if (f->texinfo == -1
                        || !strncasecmp(texname, "SKIP", 4)
                        || !strncasecmp(texname, "HINT", 4)
                        || !strncasecmp(texname, "SOLIDHINT", 9)
                        || !strncasecmp(texname, "BEVELHINT", 9)
                        )
					{
						// should not nullify the fragment inside detail brush
						f->next = outside;
						outside = f;
						continue;
					}
				}


                // throw pieces on the front sides of the planes
                // into the outside list, return the remains on the inside
				// find the fragment inside brush2
				Winding *w = new Winding (*f->w);
				for (f2 = bh2->faces; f2; f2 = f2->next)
				{
					if (f->planenum == f2->planenum)
					{
						if (!overwrite)
						{
							// face plane is outside brush2
							w->m_NumPoints = 0;
							break;
						}
						else
						{
							continue;
						}
					}
					if (f->planenum == (f2->planenum ^ 1))
					{
						continue;
					}
					Winding *fw;
					Winding *bw;
					w->Clip (f2->plane->normal, f2->plane->dist, &fw, &bw);
					if (fw)
					{
						delete fw;
					}
					if (bw)
					{
						delete w;
						w = bw;
					}
					else
					{
						w->m_NumPoints = 0;
						break;
					}
				}
				// do real split
				if (w->m_NumPoints)
				{
					for (f2 = bh2->faces; f2; f2 = f2->next)
					{
						if (f->planenum == f2->planenum || f->planenum == (f2->planenum ^ 1))
						{
							continue;
						}
						int valid = 0;
						int x;
						for (x = 0; x < w->m_NumPoints; x++)
						{
							vec_t dist = DotProduct (w->m_Points[x], f2->plane->normal) - f2->plane->dist;
							if (dist >= -ON_EPSILON*4) // only estimate
							{
								valid++;
							}
						}
						if (valid >= 2)
						{ // this splitplane forms an edge
							Winding *fw;
							Winding *bw;
							f->w->Clip (f2->plane->normal, f2->plane->dist, &fw, &bw);
							if (fw)
							{
								bface_t *front = NewFaceFromFace (f);
								front->w = fw;
								fw->getBounds (front->bounds);
								front->next = outside;
								outside = front;
							}
							if (bw)
							{
								delete f->w;
								f->w = bw;
								bw->getBounds (f->bounds);
							}
							else
							{
								FreeFace (f);
								f = NULL;
								break;
							}
						}
					}
				}
				else
				{
					f->next = outside;
					outside = f;
					f = NULL;
				}
				delete w;

                area = f ? f->w->getArea() : 0;
                if (f && area < g_tiny_threshold)
                {
                    Verbose("Entity %i, Brush %i: tiny penetration\n", 
						b1->originalentitynum, b1->originalbrushnum
						);
                    c_tiny_clip++;
                    FreeFace(f);
                    f = NULL;
                }
                if (f)
                {
                    // there is one convex fragment of the original
                    // face left inside brush2

					if (
						(hull? (b2->clipnodedetaillevel > b1->clipnodedetaillevel): (b2->detaillevel > b1->detaillevel))
						)
					{ // don't chop or set contents, only nullify
						f->next = outside;
						outside = f;
						f->texinfo = -1;
						continue;
					}
					if (
						(hull? b2->clipnodedetaillevel < b1->clipnodedetaillevel: b2->detaillevel < b1->detaillevel)
						&& b2->contents == CONTENTS_SOLID)
					{ // real solid
						FreeFace (f);
						continue;
					}
					if (b1->contents == CONTENTS_TOEMPTY)
					{
						bool onfront = true, onback = true;
						for (f2 = bh2->faces; f2; f2 = f2->next)
						{
							if (f->planenum == (f2->planenum ^ 1))
								onback = false;
							if (f->planenum == f2->planenum)
								onfront = false;
						}
						if (onfront && f->contents < b2->contents)
							f->contents = b2->contents;
						if (onback && f->backcontents < b2->contents)
							f->backcontents = b2->contents;
						if (f->contents == CONTENTS_SOLID && f->backcontents == CONTENTS_SOLID
							&& strncasecmp (GetTextureByNumber_CSG (f->texinfo), "SOLIDHINT", 9)
                            && strncasecmp(GetTextureByNumber_CSG(f->texinfo), "BEVELHINT", 9)
							)
						{
							FreeFace (f);
						}
						else
						{
							f->next = outside;
							outside = f;
						}
						continue;
					}
                    if (b1->contents > b2->contents
						|| b1->contents == b2->contents && !strncasecmp (GetTextureByNumber_CSG (f->texinfo), "SOLIDHINT", 9)
                        || b1->contents == b2->contents && !strncasecmp(GetTextureByNumber_CSG(f->texinfo), "BEVELHINT", 9)
						)
                    {                                      // inside a water brush
                        f->contents = b2->contents;
                        f->next = outside;
                        outside = f;
                    }
                    else                                   // inside a solid brush
                    {
                        FreeFace(f);                       // throw it away
                    }
                }
            }

        }

        // all of the faces left in outside are real surface faces
        SaveOutside(b1, hull, outside, b1->contents);
    }
}

//
// =====================================================================================
//

// =====================================================================================
//  EmitPlanes
// =====================================================================================
static void     EmitPlanes()
{
    int             i;
    dplane_t*       dp;
    plane_t*        mp;

    g_numplanes = g_nummapplanes;
    mp = g_mapplanes;
    dp = g_dplanes;
	{
		char name[_MAX_PATH];
		safe_snprintf (name, _MAX_PATH, "%s.pln", g_Mapname);
		FILE *planeout = fopen (name, "wb");
		if (!planeout)
			Error("Couldn't open %s", name);
		SafeWrite (planeout, g_mapplanes, g_nummapplanes * sizeof (plane_t));
		fclose (planeout);
	}
    for (i = 0; i < g_nummapplanes; i++, mp++, dp++)
    {
        //if (!(mp->redundant))
        //{
        //    Log("EmitPlanes: plane %i non redundant\n", i);
            VectorCopy(mp->normal, dp->normal);
            dp->dist = mp->dist;
            dp->type = mp->type;
       // }
        //else
       // {
       //     Log("EmitPlanes: plane %i redundant\n", i);
       // }
    }
}

// =====================================================================================
//  SetModelNumbers
//      blah
// =====================================================================================
static void     SetModelNumbers()
{
    int             i;
    int             models;
    char            value[10];

    models = 1;
    for (i = 1; i < g_numentities; i++)
    {
        if (g_entities[i].numbrushes)
        {
            safe_snprintf(value, sizeof(value), "*%i", models);
            models++;
            SetKeyValue(&g_entities[i], "model", value);
        }
    }
}

void     ReuseModel ()
{
	int i;
	for (i = g_numentities - 1; i >= 1; i--) // so it won't affect the remaining entities in the loop when we move this entity backward
	{
		const char *name = ValueForKey (&g_entities[i], "zhlt_usemodel");
		if (!*name)
		{
			continue;
		}
		int j;
		for (j = 1; j < g_numentities; j++)
		{
			if (*ValueForKey (&g_entities[j], "zhlt_usemodel"))
			{
				continue;
			}
			if (!strcmp (name, ValueForKey (&g_entities[j], "targetname")))
			{
				break;
			}
		}
		if (j == g_numentities)
		{
			if (!strcasecmp (name, "null"))
			{
				SetKeyValue (&g_entities[i], "model", "");
				continue;
			}
			Error ("zhlt_usemodel: can not find target entity '%s', or that entity is also using 'zhlt_usemodel'.\n", name);
		}
		SetKeyValue (&g_entities[i], "model", ValueForKey (&g_entities[j], "model"));
		if (j > i)
		{
			// move this entity backward
			// to prevent precache error in case of .mdl/.spr and wrong result of EntityForModel in case of map model
			entity_t tmp;
			tmp = g_entities[i];
			memmove (&g_entities[i], &g_entities[i + 1], ((j + 1) - (i + 1)) * sizeof (entity_t));
			g_entities[j] = tmp;
		}
	}
}

// =====================================================================================
//  SetLightStyles
// =====================================================================================
#define	MAX_SWITCHED_LIGHTS	    32 
#define MAX_LIGHTTARGETS_NAME   64

static void     SetLightStyles()
{
    int             stylenum;
    const char*     t;
    entity_t*       e;
    int             i, j;
    char            value[10];
    char            lighttargets[MAX_SWITCHED_LIGHTS][MAX_LIGHTTARGETS_NAME];

    	bool			newtexlight = false;

    // any light that is controlled (has a targetname)
    // must have a unique style number generated for it

    stylenum = 0;
    for (i = 1; i < g_numentities; i++)
    {
        e = &g_entities[i];

        t = ValueForKey(e, "classname");
        if (strncasecmp(t, "light", 5))
        {
            //LRC:
			// if it's not a normal light entity, allocate it a new style if necessary.
	        t = ValueForKey(e, "style");
			switch (atoi(t))
			{
			case 0: // not a light, no style, generally pretty boring
				continue;
			case -1: // normal switchable texlight
				safe_snprintf(value, sizeof(value), "%i", 32 + stylenum);
				SetKeyValue(e, "style", value);
				stylenum++;
				continue;
			case -2: // backwards switchable texlight
				safe_snprintf(value, sizeof(value), "%i", -(32 + stylenum));
				SetKeyValue(e, "style", value);
				stylenum++;
				continue;
			case -3: // (HACK) a piggyback texlight: switched on and off by triggering a real light that has the same name
				SetKeyValue(e, "style", "0"); // just in case the level designer didn't give it a name
				newtexlight = true;
				// don't 'continue', fall out
			}
	        //LRC (ends)
        }
        t = ValueForKey(e, "targetname");
		if (*ValueForKey (e, "zhlt_usestyle"))
		{
			t = ValueForKey(e, "zhlt_usestyle");
			if (!strcasecmp (t, "null"))
			{
				t = "";
			}
		}
        if (!t[0])
        {
            continue;
        }

        // find this targetname
        for (j = 0; j < stylenum; j++)
        {
            if (!strcmp(lighttargets[j], t))
            {
                break;
            }
        }
        if (j == stylenum)
        {
            hlassume(stylenum < MAX_SWITCHED_LIGHTS, assume_MAX_SWITCHED_LIGHTS);
            safe_strncpy(lighttargets[j], t, MAX_LIGHTTARGETS_NAME);
            stylenum++;
        }
        safe_snprintf(value, sizeof(value), "%i", 32 + j);
        SetKeyValue(e, "style", value);
    }

}

// =====================================================================================
//  ConvertHintToEmtpy
// =====================================================================================
static void     ConvertHintToEmpty()
{
    int             i;

    // Convert HINT brushes to EMPTY after they have been carved by csg
    for (i = 0; i < MAX_MAP_BRUSHES; i++)
    {
        if (g_mapbrushes[i].contents == CONTENTS_HINT)
        {
            g_mapbrushes[i].contents = CONTENTS_EMPTY;
        }
    }
}

// =====================================================================================
//  WriteBSP
// =====================================================================================
void LoadWadValue ()
{
	char *wadvalue;
	ParseFromMemory (g_dentdata, g_entdatasize);
	epair_t *e;
	entity_t ent0;
	entity_t *mapent = &ent0;
	memset (mapent, 0, sizeof (entity_t));
	if (!GetToken (true))
	{
		wadvalue = strdup ("");
	}
	else
	{
		if (strcmp (g_token, "{"))
		{
			Error ("ParseEntity: { not found");
		}
		while (1)
		{
			if (!GetToken (true))
			{
				Error ("ParseEntity: EOF without closing brace");
			}
			if (!strcmp (g_token, "}"))
			{
				break;
			}
			e = ParseEpair ();
			e->next = mapent->epairs;
			mapent->epairs = e;
		}
		wadvalue = strdup (ValueForKey (mapent, "wad"));
		epair_t *next;
		for (e = mapent->epairs; e; e = next)
		{
			next = e->next;
			free (e->key);
			free (e->value);
			free (e);
		}
	}
	SetKeyValue (&g_entities[0], "wad", wadvalue);
	free (wadvalue);
}
void WriteBSP(const char* const name)
{
    char path[_MAX_PATH];

	safe_snprintf(path, _MAX_PATH, "%s.bsp", name);

    SetModelNumbers();
	ReuseModel();
    SetLightStyles();

    if (!g_onlyents)
        WriteMiptex();
	if (g_onlyents)
		LoadWadValue ();

    UnparseEntities();
    ConvertHintToEmpty(); // this is ridiculous. --vluzacn
    if (g_chart)
        PrintBSPFileSizes();
    WriteBSPFile(path);
}

//
// =====================================================================================
//

// AJM: added in function
// =====================================================================================
//  CopyGenerictoCLIP
//      clips a generic brush
// =====================================================================================
/*static void     CopyGenerictoCLIP(const brush_t* const b)
{
    // code blatently ripped from CopySKYtoCLIP()

    int             i;
    entity_t*       mapent;
    brush_t*        newbrush;

    mapent = &g_entities[b->entitynum];
    mapent->numbrushes++;

    newbrush = &g_mapbrushes[g_nummapbrushes];
#ifdef HLCSG_COUNT_NEW
	newbrush->originalentitynum = b->originalentitynum;
	newbrush->originalbrushnum = b->originalbrushnum;
#endif
    newbrush->entitynum = b->entitynum;
    newbrush->brushnum = g_nummapbrushes - mapent->firstbrush;
    newbrush->firstside = g_numbrushsides;
    newbrush->numsides = b->numsides;
    newbrush->contents = CONTENTS_CLIP;
    newbrush->noclip = 0;

    for (i = 0; i < b->numsides; i++)
    {
        int             j;

        side_t*         side = &g_brushsides[g_numbrushsides];

        *side = g_brushsides[b->firstside + i];
        safe_strncpy(side->td.name, "CLIP", sizeof(side->td.name));

        for (j = 0; j < NUM_HULLS; j++)
        {
            newbrush->hulls[j].faces = NULL;
            newbrush->hulls[j].bounds = b->hulls[j].bounds;
        }

        g_numbrushsides++;
        hlassume(g_numbrushsides < MAX_MAP_SIDES, assume_MAX_MAP_SIDES);
    }

    g_nummapbrushes++;
    hlassume(g_nummapbrushes < MAX_MAP_BRUSHES, assume_MAX_MAP_BRUSHES);
}*/

// AJM: added in 
unsigned int    BrushClipHullsDiscarded = 0; 
unsigned int    ClipNodesDiscarded = 0;

//AJM: added in function
static void     MarkEntForNoclip(entity_t*  ent)
{
    int             i;
    brush_t*        b;

    for (i = ent->firstbrush; i < ent->firstbrush + ent->numbrushes; i++)
    {
        b = &g_mapbrushes[i];
        b->noclip = 1;  

        BrushClipHullsDiscarded++;
        ClipNodesDiscarded += b->numsides;
    }
}

// AJM
// =====================================================================================
//  CheckForNoClip
//      marks the noclip flag on any brushes that dont need clipnode generation, eg. func_illusionaries
// =====================================================================================
static void     CheckForNoClip()
{
    int             i;
    entity_t*       ent;

    char            entclassname[MAX_KEY]; 
    int             spawnflags;
	int				count = 0;

    if (!g_bClipNazi) 
        return; // NO CLIP FOR YOU!!!

    for (i = 0; i < g_numentities; i++)
    {
        if (!g_entities[i].numbrushes) 
            continue; // not a model

        if (!i) 
            continue; // dont waste our time with worldspawn

        ent = &g_entities[i];

        strcpy_s(entclassname, ValueForKey(ent, "classname"));
        spawnflags = atoi(ValueForKey(ent, "spawnflags"));
		int skin = IntForKey(ent, "skin"); //vluzacn

		if ((skin != -16) &&
			(
				!strcmp(entclassname, "env_bubbles")
				|| !strcmp(entclassname, "func_illusionary")
				|| (spawnflags & 8) && 
				(   /* NOTE: func_doors as far as i can tell may need clipnodes for their
							player collision detection, so for now, they stay out of it. */
					!strcmp(entclassname, "func_train")
					|| !strcmp(entclassname, "func_door")
					|| !strcmp(entclassname, "func_water")
					|| !strcmp(entclassname, "func_door_rotating")
					|| !strcmp(entclassname, "func_pendulum")
					|| !strcmp(entclassname, "func_train")
					|| !strcmp(entclassname, "func_tracktrain")
					|| !strcmp(entclassname, "func_vehicle")
				)
				|| (skin != 0) && (!strcmp(entclassname, "func_door") || !strcmp(entclassname, "func_water"))
				|| (spawnflags & 2) && (!strcmp(entclassname, "func_conveyor"))
				|| (spawnflags & 1) && (!strcmp(entclassname, "func_rot_button"))
				|| (spawnflags & 64) && (!strcmp(entclassname, "func_rotating"))
			))
		{
			MarkEntForNoclip(ent);
			count++;
		}
        /*
        // condition 6: its a func_wall, while we noclip it, we remake the clipnodes manually 
        else if (!strncasecmp(entclassname, "func_wall", 9)) 
        {
            for (int j = ent->firstbrush; j < ent->firstbrush + ent->numbrushes; j++)
                CopyGenerictoCLIP(&g_mapbrushes[i]);

            MarkEntForNoclip(ent);
        }
*/
    }

    Log("%i entities discarded from clipping hulls\n", count);
}

// =====================================================================================
//  ProcessModels
// =====================================================================================


static void     ProcessModels()
{
    int             i, j, type;
    int             placed;
    int             first, contents;
    brush_t         temp;

    for (i = 0; i < g_numentities; i++)
    {
        if (!g_entities[i].numbrushes) // only models
            continue;

        // sort the contents down so stone bites water, etc
        first = g_entities[i].firstbrush;
		brush_t *temps = (brush_t *)malloc (g_entities[i].numbrushes * sizeof (brush_t));
		hlassume (temps, assume_NoMemory);
		for (j = 0; j < g_entities[i].numbrushes; j++)
		{
			temps[j] = g_mapbrushes[first + j];
		}
		int placedcontents;
		bool b_placedcontents = false;
		for (placed = 0; placed < g_entities[i].numbrushes; )
		{
			bool b_contents = false;
			for (j = 0; j < g_entities[i].numbrushes; j++)
			{
				brush_t *brush = &temps[j];
				if (b_placedcontents && brush->contents <= placedcontents)
					continue;
				if (b_contents && brush->contents >= contents)
					continue;
				b_contents = true;
				contents = brush->contents;
			}
			for (j = 0; j < g_entities[i].numbrushes; j++)
			{
				brush_t *brush = &temps[j];
				if (brush->contents == contents)
				{
					g_mapbrushes[first + placed] = *brush;
					placed++;
				}
			}
			b_placedcontents = true;
			placedcontents = contents;
		}
		free (temps);

        // csg them in order
        if (i == 0) // if its worldspawn....
        {
            NamedRunThreadsOnIndividual(g_entities[i].numbrushes, g_estimate, CSGBrush);
            CheckFatal();
        }
        else
        {
            for (j = 0; j < g_entities[i].numbrushes; j++)
            {
                CSGBrush(first + j);
            }
        }

        // write end of model marker
        for (j = 0; j < NUM_HULLS; j++)
        {
			fprintf (out[j], "-1 -1 -1 -1 -1\n");
			fprintf (out_detailbrush[j], "-1\n");
        }
    }
}

// =====================================================================================
//  SetModelCenters
// =====================================================================================
static void     SetModelCenters(int entitynum)
{
    int             i;
    int             last;
    char            string[MAXTOKEN];
    entity_t*       e = &g_entities[entitynum];
    BoundingBox     bounds;
    vec3_t          center;

    if ((entitynum == 0) || (e->numbrushes == 0)) // skip worldspawn and point entities
        return;

    if (!*ValueForKey(e, "light_origin")) // skip if its not a zhlt_flags light_origin
        return;

    for (i = e->firstbrush, last = e->firstbrush + e->numbrushes; i < last; i++)
    {
        if (g_mapbrushes[i].contents != CONTENTS_ORIGIN
			&& g_mapbrushes[i].contents != CONTENTS_BOUNDINGBOX
			)
        {
            bounds.add(g_mapbrushes[i].hulls->bounds);
        }
    }

    VectorAdd(bounds.m_Mins, bounds.m_Maxs, center);
    VectorScale(center, 0.5, center);

    safe_snprintf(string, MAXTOKEN, "%i %i %i", (int)center[0], (int)center[1], (int)center[2]);
    SetKeyValue(e, "model_center", string);
}

//
// =====================================================================================
//

// =====================================================================================
//  BoundWorld
// =====================================================================================
static void     BoundWorld()
{
    int             i;
    brushhull_t*    h;

    world_bounds.reset();

    for (i = 0; i < g_nummapbrushes; i++)
    {
        h = &g_mapbrushes[i].hulls[0];
        if (!h->faces)
        {
            continue;
        }
        world_bounds.add(h->bounds);
    }

    Verbose("World bounds: (%i %i %i) to (%i %i %i)\n",
            (int)world_bounds.m_Mins[0], (int)world_bounds.m_Mins[1], (int)world_bounds.m_Mins[2],
            (int)world_bounds.m_Maxs[0], (int)world_bounds.m_Maxs[1], (int)world_bounds.m_Maxs[2]);
}

// =====================================================================================
//  Usage
//      prints out usage sheet
// =====================================================================================
static void     Usage()
{
    Banner(); // TODO: Call banner from main CSG process? 

    Log("\n-= %s Options =-\n\n", g_Program);
	Log("    -console #       : Set to 0 to turn off the pop-up console (default is 1)\n");
	Log("    -lang file       : localization file\n");
    Log("    -nowadtextures   : Include all used textures into bsp\n");
    Log("    -wadinclude file : Include specific wad or directory into bsp\n");
    Log("    -noclip          : don't create clipping hull\n");
    
    Log("    -clipeconomy     : turn clipnode economy mode on\n");

	Log("    -cliptype value  : set to smallest, normalized, simple, precise, or legacy (default)\n");
	Log("    -nullfile file   : specify list of entities to retexture with NULL\n");

    Log("    -onlyents        : do an entity update from .map to .bsp\n");
    Log("    -noskyclip       : disable automatic clipping of SKY brushes\n");
    Log("    -tiny #          : minmum brush face surface area before it is discarded\n");
    Log("    -brushunion #    : threshold to warn about overlapping brushes\n\n");
    Log("    -hullfile file   : Reads in custom collision hull dimensions\n");
	Log("    -wadcfgfile file : wad configuration file\n");
	Log("    -wadconfig name  : use the old wad configuration approach (select a group from wad.cfg)\n");
    Log("    -texdata #       : Alter maximum texture memory limit (in kb)\n");
    Log("    -lightdata #     : Alter maximum lighting memory limit (in kb)\n");
    Log("    -chart           : display bsp statitics\n");
    Log("    -low | -high     : run program an altered priority level\n");
    Log("    -nolog           : don't generate the compile logfiles\n");
	Log("    -noresetlog      : Do not delete log file\n");
    Log("    -threads #       : manually specify the number of threads to run\n");
#ifdef SYSTEM_WIN32
    Log("    -estimate        : display estimated time during compile\n");
#endif
#ifdef SYSTEM_POSIX
    Log("    -noestimate      : do not display continuous compile time estimates\n");
#endif
    Log("    -verbose         : compile with verbose messages\n");
    Log("    -noinfo          : Do not show tool configuration information\n");

    Log("    -nonulltex       : Turns off null texture stripping\n");
	Log("    -nonullifytrigger: don't remove 'aaatrigger' texture\n");


	Log("    -nolightopt      : don't optimize engine light entities\n");

#ifdef HLCSG_GAMETEXTMESSAGE_UTF8
	Log("    -notextconvert   : don't convert game_text message from Windows ANSI to UTF8 format\n");
#endif

    Log("    -dev #           : compile with developer message\n\n");


    Log("    -nowadautodetect : Disable auto-detection of wadfiles\n");

	Log("    -scale #         : Scale the world. Use at your own risk.\n");
    Log("    -worldextent #   : Extend map geometry limits beyond +/-32768.\n");
    Log("    mapfile          : The mapfile to compile\n\n");

    exit(1);
}

// =====================================================================================
//  DumpWadinclude
//      prints out the wadinclude list
// =====================================================================================
static void     DumpWadinclude()
{
    Log("Wadinclude list\n");
    Log("---------------\n");
    WadInclude_i it;

    for (it = g_WadInclude.begin(); it != g_WadInclude.end(); it++)
    {
        Log("%s\n", it->c_str());
    }
    Log("---------------\n\n");
}

// =====================================================================================
//  Settings
//      prints out settings sheet
// =====================================================================================
static void     Settings()
{
    char*           tmp;

    if (!g_info)
        return; 

    Log("\nCurrent %s Settings\n", g_Program);
    Log("Name                 |  Setting  |  Default\n"
        "---------------------|-----------|-------------------------\n");

    // ZHLT Common Settings
    if (DEFAULT_NUMTHREADS == -1)
    {
        Log("threads               [ %7d ] [  Varies ]\n", g_numthreads);
    }
    else
    {
        Log("threads               [ %7d ] [ %7d ]\n", g_numthreads, DEFAULT_NUMTHREADS);
    }

    Log("verbose               [ %7s ] [ %7s ]\n", g_verbose ? "on" : "off", DEFAULT_VERBOSE ? "on" : "off");
    Log("log                   [ %7s ] [ %7s ]\n", g_log ? "on" : "off", DEFAULT_LOG ? "on" : "off");
    Log("reset logfile         [ %7s ] [ %7s ]\n", g_resetlog ? "on" : "off", DEFAULT_RESETLOG ? "on" : "off");

    Log("developer             [ %7d ] [ %7d ]\n", g_developer, DEFAULT_DEVELOPER);
    Log("chart                 [ %7s ] [ %7s ]\n", g_chart ? "on" : "off", DEFAULT_CHART ? "on" : "off");
    Log("estimate              [ %7s ] [ %7s ]\n", g_estimate ? "on" : "off", DEFAULT_ESTIMATE ? "on" : "off");
    Log("max texture memory    [ %7d ] [ %7d ]\n", g_max_map_miptex, DEFAULT_MAX_MAP_MIPTEX);
	Log("max lighting memory   [ %7d ] [ %7d ]\n", g_max_map_lightdata, DEFAULT_MAX_MAP_LIGHTDATA);

    switch (g_threadpriority)
    {
    case eThreadPriorityNormal:
    default:
        tmp = "Normal";
        break;
    case eThreadPriorityLow:
        tmp = "Low";
        break;
    case eThreadPriorityHigh:
        tmp = "High";
        break;
    }
    Log("priority              [ %7s ] [ %7s ]\n", tmp, "Normal");
    Log("\n");

    // HLCSG Specific Settings

    Log("noclip                [ %7s ] [ %7s ]\n", g_noclip          ? "on" : "off", DEFAULT_NOCLIP       ? "on" : "off");

    Log("null texture stripping[ %7s ] [ %7s ]\n", g_bUseNullTex     ? "on" : "off", DEFAULT_NULLTEX      ? "on" : "off");


    Log("clipnode economy mode [ %7s ] [ %7s ]\n", g_bClipNazi       ? "on" : "off", DEFAULT_CLIPNAZI     ? "on" : "off");

	Log("clip hull type        [ %7s ] [ %7s ]\n", GetClipTypeString(g_cliptype), GetClipTypeString(DEFAULT_CLIPTYPE));

    Log("onlyents              [ %7s ] [ %7s ]\n", g_onlyents        ? "on" : "off", DEFAULT_ONLYENTS     ? "on" : "off");
    Log("wadtextures           [ %7s ] [ %7s ]\n", g_wadtextures     ? "on" : "off", DEFAULT_WADTEXTURES  ? "on" : "off");
    Log("skyclip               [ %7s ] [ %7s ]\n", g_skyclip         ? "on" : "off", DEFAULT_SKYCLIP      ? "on" : "off");
    Log("hullfile              [ %7s ] [ %7s ]\n", g_hullfile ? g_hullfile : "None", "None");
	Log("wad.cfg file          [ %7s ] [ %7s ]\n", g_wadcfgfile? g_wadcfgfile: "None", "None");
	Log("wad.cfg config name   [ %7s ] [ %7s ]\n", g_wadconfigname? g_wadconfigname: "None", "None");
	Log("nullfile              [ %7s ] [ %7s ]\n", g_nullfile ? g_nullfile : "None", "None");
	Log("nullify trigger       [ %7s ] [ %7s ]\n", g_nullifytrigger? "on": "off", DEFAULT_NULLIFYTRIGGER? "on": "off");
    // calc min surface area
    {
        char            tiny_penetration[10];
        char            default_tiny_penetration[10];

        safe_snprintf(tiny_penetration, sizeof(tiny_penetration), "%3.3f", g_tiny_threshold);
        safe_snprintf(default_tiny_penetration, sizeof(default_tiny_penetration), "%3.3f", DEFAULT_TINY_THRESHOLD);
        Log("min surface area      [ %7s ] [ %7s ]\n", tiny_penetration, default_tiny_penetration);
    }

    // calc union threshold
    {
        char            brush_union[10];
        char            default_brush_union[10];

        safe_snprintf(brush_union, sizeof(brush_union), "%3.3f", g_BrushUnionThreshold);
        safe_snprintf(default_brush_union, sizeof(default_brush_union), "%3.3f", DEFAULT_BRUSH_UNION_THRESHOLD);
        Log("brush union threshold [ %7s ] [ %7s ]\n", brush_union, default_brush_union);
    }
    {
        char            buf1[10];
        char            buf2[10];

		if (g_scalesize > 0)
			safe_snprintf(buf1, sizeof(buf1), "%3.3f", g_scalesize);
		else
			strcpy (buf1, "None");
		if (DEFAULT_SCALESIZE > 0)
			safe_snprintf(buf2, sizeof(buf2), "%3.3f", DEFAULT_SCALESIZE);
		else
			strcpy (buf2, "None");
        Log("map scaling           [ %7s ] [ %7s ]\n", buf1, buf2);
    }
    Log("light name optimize   [ %7s ] [ %7s ]\n", !g_nolightopt? "on" : "off", !DEFAULT_NOLIGHTOPT? "on" : "off");
#ifdef HLCSG_GAMETEXTMESSAGE_UTF8
	Log("convert game_text     [ %7s ] [ %7s ]\n", !g_noutf8? "on" : "off", !DEFAULT_NOUTF8? "on" : "off");
#endif
    Log("world extent          [ %7d ] [ %7d ]\n", g_iWorldExtent, 65536);

    Log("\n");
}

// AJM: added in
// =====================================================================================
//  CSGCleanup
// =====================================================================================
void            CSGCleanup()
{
    //Log("CSGCleanup\n");
    FreeWadPaths();
}

// =====================================================================================
//  Main
//      Oh, come on.
// =====================================================================================
int             main(const int argc, char** argv)
{
    int             i;                          
    char            name[_MAX_PATH];            // mapanme 
    double          start, end;                 // start/end time log
    const char*     mapname_from_arg = NULL;    // mapname path from passed argvar

    g_Program = "sdHLCSG";

	int argcold = argc;
	char ** argvold = argv;
	{
		int argc;
		char ** argv;
		ParseParamFile (argcold, argvold, argc, argv);
		{
	if (InitConsole (argc, argv) < 0)
		Usage();
    if (argc == 1)
        Usage();

    // Hard coded list of -wadinclude files, used for HINT texture brushes so lazy
    // mapmakers wont cause beta testers (or possibly end users) to get a wad 
    // error on zhlt.wad etc
    g_WadInclude.push_back("sdhlt.wad"); //seedee

	InitDefaultHulls ();

    // detect argv
    for (i = 1; i < argc; i++)
    {
        if (!strcasecmp(argv[i], "-threads"))
        {
            if (i + 1 < argc)	//added "1" .--vluzacn
            {
                g_numthreads = atoi(argv[++i]);
                if (g_numthreads < 1)
                {
                    Log("Expected value of at least 1 for '-threads'\n");
                    Usage();
                }
            }
            else
            {
                Usage();
            }
        }

        else if (!strcasecmp(argv[i], "-worldextent"))
        {
            g_iWorldExtent = atoi(argv[++i]);
        }

		else if (!strcasecmp(argv[i], "-console"))
		{
#ifndef SYSTEM_WIN32
			Warning("The option '-console #' is only valid for Windows.");
#endif
			if (i + 1 < argc)
				++i;
			else
				Usage();
		}
#ifdef SYSTEM_WIN32
        else if (!strcasecmp(argv[i], "-estimate"))
        {
            g_estimate = true;
        }
#endif

#ifdef SYSTEM_POSIX
        else if (!strcasecmp(argv[i], "-noestimate"))
        {
            g_estimate = false;
        }
#endif

        else if (!strcasecmp(argv[i], "-dev"))
        {
            if (i + 1 < argc)	//added "1" .--vluzacn
            {
                g_developer = (developer_level_t)atoi(argv[++i]);
            }
            else
            {
                Usage();
            }
        }
        else if (!strcasecmp(argv[i], "-verbose"))
        {
            g_verbose = true;
        }
        else if (!strcasecmp(argv[i], "-noinfo"))
        {
            g_info = false;
        }
        else if (!strcasecmp(argv[i], "-chart"))
        {
            g_chart = true;
        }
        else if (!strcasecmp(argv[i], "-low"))
        {
            g_threadpriority = eThreadPriorityLow;
        }
        else if (!strcasecmp(argv[i], "-high"))
        {
            g_threadpriority = eThreadPriorityHigh;
        }
        else if (!strcasecmp(argv[i], "-nolog"))
        {
            g_log = false;
        }
        else if (!strcasecmp(argv[i], "-skyclip"))
        {
            g_skyclip = true;
        }
        else if (!strcasecmp(argv[i], "-noskyclip"))
        {
            g_skyclip = false;
        }
        else if (!strcasecmp(argv[i], "-noclip"))
        {
            g_noclip = true;
        }
        else if (!strcasecmp(argv[i], "-onlyents"))
        {
            g_onlyents = true;
        }

        else if (!strcasecmp(argv[i], "-nonulltex"))
        {
            g_bUseNullTex = false;
        }

        else if (!strcasecmp(argv[i], "-clipeconomy"))
        {
            g_bClipNazi = true;
        }

		else if (!strcasecmp(argv[i], "-cliptype"))
		{
			if (i + 1 < argc)	//added "1" .--vluzacn
			{
				++i;
				if(!strcasecmp(argv[i],"smallest"))
				{ g_cliptype = clip_smallest; }
				else if(!strcasecmp(argv[i],"normalized"))
				{ g_cliptype = clip_normalized; }
				else if(!strcasecmp(argv[i],"simple"))
				{ g_cliptype = clip_simple; }
				else if(!strcasecmp(argv[i],"precise"))
				{ g_cliptype = clip_precise; }
				else if(!strcasecmp(argv[i],"legacy"))
				{ g_cliptype = clip_legacy; }
			}
            else
            {
                Log("Error: -cliptype: incorrect usage of parameter\n");
                Usage();
            }
		}

		else if (!strcasecmp(argv[i], "-nullfile"))
		{
            if (i + 1 < argc)	//added "1" .--vluzacn
            {
                g_nullfile = argv[++i];
            }
            else
            {
            	Log("Error: -nullfile: expected path to null ent file following parameter\n");
                Usage();
            }
		}
        else if (!strcasecmp(argv[i], "-nowadautodetect"))
        { 
            g_bWadAutoDetect = false;
        }
        else if (!strcasecmp(argv[i], "-nowadtextures"))
        {
            g_wadtextures = false;
        }
        else if (!strcasecmp(argv[i], "-wadinclude"))
        {
            if (i + 1 < argc)	//added "1" .--vluzacn
            {
                g_WadInclude.push_back(argv[++i]);
            }
            else
            {
                Usage();
            }
        }
        else if (!strcasecmp(argv[i], "-texdata"))
        {
            if (i + 1 < argc)	//added "1" .--vluzacn
            {
                int             x = atoi(argv[++i]) * 1024;

                //if (x > g_max_map_miptex) //--vluzacn
                {
                    g_max_map_miptex = x;
                }
            }
            else
            {
                Usage();
            }
        }
        else if (!strcasecmp(argv[i], "-lightdata"))
        {
            if (i + 1 < argc)	//added "1" .--vluzacn
            {
                int             x = atoi(argv[++i]) * 1024;

                //if (x > g_max_map_lightdata) //--vluzacn
                {
                    g_max_map_lightdata = x;
                }
            }
            else
            {
                Usage();
            }
        }
        else if (!strcasecmp(argv[i], "-brushunion"))
        {
            if (i + 1 < argc)	//added "1" .--vluzacn
            {
                g_BrushUnionThreshold = (float)atof(argv[++i]);
            }
            else
            {
                Usage();
            }
        }
        else if (!strcasecmp(argv[i], "-tiny"))
        {
            if (i + 1 < argc)	//added "1" .--vluzacn
            {
                g_tiny_threshold = (float)atof(argv[++i]);
            }
            else
            {
                Usage();
            }
        }
        else if (!strcasecmp(argv[i], "-hullfile"))
        {
            if (i + 1 < argc)	//added "1" .--vluzacn
            {
                g_hullfile = argv[++i];
            }
            else
            {
                Usage();
            }
        }
		else if (!strcasecmp (argv[i], "-wadcfgfile"))
		{
			if (i + 1 < argc)
			{
				g_wadcfgfile = argv[++i];
			}
			else
			{
				Usage ();
			}
		}
		else if (!strcasecmp (argv[i], "-wadconfig"))
		{
			if (i + 1 < argc)
			{
				g_wadconfigname = argv[++i];
			}
			else
			{
				Usage ();
			}
		}
        else if (!strcasecmp(argv[i], "-scale"))
        {
            if (i + 1 < argc)
            {
                g_scalesize = atof(argv[++i]);
            }
            else
            {
                Usage();
            }
        }
		else if (!strcasecmp (argv[i], "-lang"))
		{
			if (i + 1 < argc)
			{
				char tmp[_MAX_PATH];
#ifdef SYSTEM_WIN32
				GetModuleFileName (NULL, tmp, _MAX_PATH);
#else
				safe_strncpy (tmp, argv[0], _MAX_PATH);
#endif
				LoadLangFile (argv[++i], tmp);
			}
			else
			{
				Usage();
			}
		}
		else if (!strcasecmp (argv[i], "-noresetlog"))
		{
			g_resetlog = false;
		}
		else if (!strcasecmp (argv[i], "-nolightopt"))
		{
			g_nolightopt = true;
		}
#ifdef HLCSG_GAMETEXTMESSAGE_UTF8
		else if (!strcasecmp (argv[i], "-notextconvert"))
		{
			g_noutf8 = true;
		}
#endif
		else if (!strcasecmp (argv[i], "-viewsurface"))
		{
			g_viewsurface = true;
		}
		else if (!strcasecmp (argv[i], "-nonullifytrigger"))
		{
			g_nullifytrigger = false;
		}
        else if (argv[i][0] == '-')
        {
            Log("Unknown option \"%s\"\n", argv[i]);
            Usage();
        }
        else if (!mapname_from_arg)
        {
            mapname_from_arg = argv[i];
        }
        else
        {
            Log("Unknown option \"%s\"\n", argv[i]);
            Usage();
        }
    }

    // no mapfile?
    if (!mapname_from_arg)
    {
        // what a shame.
        Log("No mapfile specified\n");
        Usage();
    }

    // handle mapname
    safe_strncpy(g_Mapname, mapname_from_arg, _MAX_PATH);
    FlipSlashes(g_Mapname);
    StripExtension(g_Mapname);

    // onlyents
    if (!g_onlyents)
        ResetTmpFiles();

    // other stuff
    ResetErrorLog();                     
	if (!g_onlyents && g_resetlog)
		ResetLog();                          
    OpenLog(g_clientid);                  
    atexit(CloseLog);                       
    LogStart(argcold, argvold);
	{
		int			 i;
		Log("Arguments: ");
		for (i = 1; i < argc; i++)
		{
			if (strchr(argv[i], ' '))
			{
				Log("\"%s\" ", argv[i]);
			}
			else
			{
				Log("%s ", argv[i]);
			}
		}
		Log("\n");
	}
#ifdef PLATFORM_CAN_CALC_EXTENT
	hlassume (CalcFaceExtents_test (), assume_first);
#endif
    atexit(CSGCleanup); // AJM
    dtexdata_init();                        
    atexit(dtexdata_free);

    // START CSG
    // AJM: re-arranged some stuff up here so that the mapfile is loaded
    //  before settings are finalised and printed out, so that the info_compile_parameters
    //  entity can be dealt with effectively
    start = I_FloatTime();
	if (g_hullfile)
	{
		char temp[_MAX_PATH];
		char test[_MAX_PATH];
		safe_strncpy (temp, g_Mapname, _MAX_PATH);
		ExtractFilePath (temp, test);
		safe_strncat (test, g_hullfile, _MAX_PATH);
		if (q_exists (test))
		{
			g_hullfile = strdup (test);
		}
		else
		{
#ifdef SYSTEM_WIN32
			GetModuleFileName (NULL, temp, _MAX_PATH);
#else
			safe_strncpy (temp, argv[0], _MAX_PATH);
#endif
			ExtractFilePath (temp, test);
			safe_strncat (test, g_hullfile, _MAX_PATH);
			if (q_exists (test))
			{
				g_hullfile = strdup (test);
			}
		}
	}
	if (g_nullfile)
	{
		char temp[_MAX_PATH];
		char test[_MAX_PATH];
		safe_strncpy (temp, g_Mapname, _MAX_PATH);
		ExtractFilePath (temp, test);
		safe_strncat (test, g_nullfile, _MAX_PATH);
		if (q_exists (test))
		{
			g_nullfile = strdup (test);
		}
		else
		{
#ifdef SYSTEM_WIN32
			GetModuleFileName (NULL, temp, _MAX_PATH);
#else
			safe_strncpy (temp, argv[0], _MAX_PATH);
#endif
			ExtractFilePath (temp, test);
			safe_strncat (test, g_nullfile, _MAX_PATH);
			if (q_exists (test))
			{
				g_nullfile = strdup (test); //Todo rename all these vars //seedee
			}
		}
	}
	if (g_wadcfgfile) //If wad.cfg exists //seedee
	{
		char mapDirPath[_MAX_PATH];
		safe_strncpy (mapDirPath, g_Mapname, _MAX_PATH); //Extract path
        char wadCfgPath[_MAX_PATH];
		ExtractFilePath (mapDirPath, wadCfgPath); //Append wad.cfg name
		safe_strncat (wadCfgPath, g_wadcfgfile, _MAX_PATH);
		if (q_exists (wadCfgPath)) //Update global if file exists
		{
			g_wadcfgfile = strdup (wadCfgPath); 
		}
		else
		{
#ifdef SYSTEM_WIN32 //Look relative to exe
			GetModuleFileName (NULL, mapDirPath, _MAX_PATH);
#else //Fallback
			safe_strncpy (mapDirPath, argv[0], _MAX_PATH);
#endif
			ExtractFilePath (mapDirPath, wadCfgPath);
			safe_strncat (wadCfgPath, g_wadcfgfile, _MAX_PATH);
			if (q_exists (wadCfgPath))
			{
				g_wadcfgfile = strdup (wadCfgPath);
			}
		}
	}
    Verbose("Loading hull file\n");
    LoadHullfile(g_hullfile);               // if the user specified a hull file, load it now
	if(g_bUseNullTex)
	{ properties_initialize(g_nullfile); }
    safe_strncpy(name, mapname_from_arg, _MAX_PATH); // make a copy of the nap name
	FlipSlashes(name);
    DefaultExtension(name, ".map");                  // might be .reg
    Verbose("Loading map file\n");
    LoadMapFile(name);
    ThreadSetDefault();                    
    ThreadSetPriority(g_threadpriority);  
    Settings();


#ifdef HLCSG_GAMETEXTMESSAGE_UTF8
	if (!g_noutf8)
	{
		int count = 0;

		for (i = 0; i < g_numentities; i++)
		{
			entity_t *ent = &g_entities[i];
			const char *value;
			char *newvalue;

			if (strcmp (ValueForKey (ent, "classname"), "game_text"))
			{
				continue;
			}

			value = ValueForKey (ent, "message");
			if (*value)
			{
				newvalue = ANSItoUTF8 (value);
				if (strcmp (newvalue, value))
				{
					SetKeyValue (ent, "message", newvalue);
					count++;
				}
				free (newvalue);
			}
		}

		if (count)
		{
			Log ("%d game_text messages converted from Windows ANSI(CP_ACP) to UTF-8 encoding\n", count);
		}
	}
#endif
  if (!g_onlyents)
  {
	if (g_wadconfigname) //If wadconfig had a name provided //seedee
	{
        char exePath[_MAX_PATH];
        char wadCfgPath[_MAX_PATH];
#ifdef SYSTEM_WIN32 //Get exe path
        GetModuleFileName(NULL, exePath, _MAX_PATH);
#else //Fallback
        safe_strncpy(exePath, argv[0], _MAX_PATH);
#endif
        ExtractFilePath(exePath, wadCfgPath);
        safe_strncat(wadCfgPath, "wad.cfg", _MAX_PATH);

        if (g_wadcfgfile) //If provided override the default
        {
            safe_strncpy(wadCfgPath, g_wadcfgfile, _MAX_PATH);
        }
        LoadWadconfig(wadCfgPath, g_wadconfigname);
	}
	else if (g_wadcfgfile)
	{
		if (!q_exists (g_wadcfgfile))
		{
			Error("Couldn't find wad configuration file '%s'\n", g_wadcfgfile);
		}
		LoadWadcfgfile (g_wadcfgfile);
	}
	else
	{
		Log("Loading mapfile wad configuration by default\n");
		GetUsedWads();
	}

    if (!g_bWadAutoDetect)
    {
        Warning("Unused textures will not be excluded\n");
    }
    DumpWadinclude();
    Log("\n");
  }

    // if onlyents, just grab the entites and resave
    if (g_onlyents)
    {
        char            out[_MAX_PATH];

        safe_snprintf(out, _MAX_PATH, "%s.bsp", g_Mapname);
        LoadBSPFile(out);

        // Write it all back out again.
        WriteBSP(g_Mapname);

        end = I_FloatTime();
        LogTimeElapsed(end - start);
        return 0;
    }

    CheckForNoClip(); 

    // createbrush
    NamedRunThreadsOnIndividual(g_nummapbrushes, g_estimate, CreateBrush);
    CheckFatal();


    // boundworld
    BoundWorld();

    Verbose("%5i map planes\n", g_nummapplanes);

    // Set model centers
    for (i = 0; i < g_numentities; i++) SetModelCenters (i); //NamedRunThreadsOnIndividual(g_numentities, g_estimate, SetModelCenters); //--vluzacn

    // Calc brush unions
    if ((g_BrushUnionThreshold > 0.0) && (g_BrushUnionThreshold <= 100.0))
    {
        NamedRunThreadsOnIndividual(g_nummapbrushes, g_estimate, CalculateBrushUnions);
    }

    // open hull files
    for (i = 0; i < NUM_HULLS; i++)
    {
        char            name[_MAX_PATH];

        safe_snprintf(name, _MAX_PATH, "%s.p%i", g_Mapname, i);

        out[i] = fopen(name, "w");

        if (!out[i]) 
            Error("Couldn't open %s", name);
		safe_snprintf(name, _MAX_PATH, "%s.b%i", g_Mapname, i);
		out_detailbrush[i] = fopen(name, "w");
		if (!out_detailbrush[i])
			Error("Couldn't open %s", name);
		if (g_viewsurface)
		{
			safe_snprintf (name, _MAX_PATH, "%s_surface%i.pts", g_Mapname, i);
			out_view[i] = fopen (name, "w");
			if (!out[i])
				Error ("Counldn't open %s", name);
		}
    }
	{
		FILE			*f;
		char			name[_MAX_PATH];
		safe_snprintf (name, _MAX_PATH, "%s.hsz", g_Mapname);
		f = fopen (name, "w");
		if (!f)
			Error("Couldn't open %s", name);
		float x1,y1,z1;
		float x2,y2,z2;
		for (i = 0; i < NUM_HULLS; i++)
		{
			x1 = g_hull_size[i][0][0];
			y1 = g_hull_size[i][0][1];
			z1 = g_hull_size[i][0][2];
			x2 = g_hull_size[i][1][0];
			y2 = g_hull_size[i][1][1];
			z2 = g_hull_size[i][1][2];
			fprintf (f, "%g %g %g %g %g %g\n", x1, y1, z1, x2, y2, z2);
		}
		fclose (f);
	}

    ProcessModels();

    Verbose("%5i csg faces\n", c_csgfaces);
    Verbose("%5i used faces\n", c_outfaces);
    Verbose("%5i tiny faces\n", c_tiny);
    Verbose("%5i tiny clips\n", c_tiny_clip);

    // close hull files 
    for (i = 0; i < NUM_HULLS; i++)
	{
        fclose(out[i]);
		fclose (out_detailbrush[i]);
		if (g_viewsurface)
		{
			fclose (out_view[i]);
		}
	}

    EmitPlanes();


    WriteBSP(g_Mapname);

    // AJM: debug
#if 0
    Log("\n---------------------------------------\n"
        "Map Plane Usage:\n"
        "  #  normal             origin             dist   type\n"
        "    (   x,    y,    z) (   x,    y,    z) (     )\n"
        );
    for (i = 0; i < g_nummapplanes; i++)
    {
        plane_t* p = &g_mapplanes[i];

        Log(
        "%3i (%4.0f, %4.0f, %4.0f) (%4.0f, %4.0f, %4.0f) (%5.0f) %i\n",
        i,     
        p->normal[1], p->normal[2], p->normal[3],
        p->origin[1], p->origin[2], p->origin[3],
        p->dist,
        p->type
        );
    }
    Log("---------------------------------------\n\n");
#endif

    // elapsed time
    end = I_FloatTime();
    LogTimeElapsed(end - start);

		}
	}
    return 0;
}
