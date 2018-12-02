#ifndef HLCSG_H__
#define HLCSG_H__

#if _MSC_VER >= 1000
#pragma once
#endif

#pragma warning(disable: 4786)	// identifier was truncated to '255' characters in the browser information
#include <deque>
#include <string>
#include <map>

#include "cmdlib.h"
#include "messages.h"
#include "win32fix.h"
#include "log.h"
#include "hlassert.h"
#include "mathlib.h"
#include "scriplib.h"
#include "winding.h"
#include "threads.h"
#include "bspfile.h"
#include "blockmem.h"
#include "filelib.h"
#include "boundingbox.h"
// AJM: added in
#include "wadpath.h"
#include "cmdlinecfg.h"

#ifndef DOUBLEVEC_T
#error you must add -dDOUBLEVEC_T to the project!
#endif

#define DEFAULT_BRUSH_UNION_THRESHOLD 0.0f
#define DEFAULT_TINY_THRESHOLD        0.0
#define DEFAULT_NOCLIP      false
#define DEFAULT_ONLYENTS    false
#define DEFAULT_WADTEXTURES true
#define DEFAULT_SKYCLIP     true
#define DEFAULT_CHART       false
#define DEFAULT_INFO        true

#define FLOOR_Z 0.7 // Quake default
#define DEFAULT_CLIPTYPE clip_simple //clip_legacy //--vluzacn

#define DEFAULT_NULLTEX     true

#define DEFAULT_CLIPNAZI    false

#define DEFAULT_WADAUTODETECT false


#define DEFAULT_SCALESIZE -1.0 //dont scale
#define DEFAULT_RESETLOG true
#define DEFAULT_NOLIGHTOPT false
#ifdef HLCSG_GAMETEXTMESSAGE_UTF8
#define DEFAULT_NOUTF8 false
#endif
#define DEFAULT_NULLIFYTRIGGER true

// AJM: added in
#define UNLESS(a)  if (!(a))

#ifdef SYSTEM_WIN32
#define DEFAULT_ESTIMATE    false
#endif

#ifdef SYSTEM_POSIX
#define DEFAULT_ESTIMATE    true
#endif

#define BOGUS_RANGE    65534

#define MAX_HULLSHAPES 128 // arbitrary

typedef struct
{
    vec3_t          normal;
    vec3_t          origin;
    vec_t           dist;
    planetypes      type;
} plane_t;



typedef struct
{
    vec3_t          UAxis;
    vec3_t          VAxis;
    vec_t           shift[2];
    vec_t           rotate;
    vec_t           scale[2];
} valve_vects;

typedef struct
{
    float           vects[2][4];
} quark_vects;

typedef union
{
    valve_vects     valve;
    quark_vects     quark;
}
vects_union;

extern int      g_nMapFileVersion;                         // map file version * 100 (ie 201), zero for pre-Worldcraft 2.0.1 maps

typedef struct
{
    char            txcommand;
    vects_union     vects;
    char            name[32];
} brush_texture_t;

typedef struct side_s
{
    brush_texture_t td;
	bool			bevel;
    vec_t           planepts[3][3];
} side_t;

typedef struct bface_s
{
    struct bface_s* next;
    int             planenum;
    plane_t*        plane;
    Winding*        w;
    int             texinfo;
    bool            used;                                  // just for face counting
    int             contents;
    int             backcontents;
	bool			bevel; //used for ExpandBrush
    BoundingBox     bounds;
} bface_t;

// NUM_HULLS should be no larger than MAX_MAP_HULLS
#define NUM_HULLS 4

typedef struct
{
    BoundingBox     bounds;
    bface_t*        faces;
} brushhull_t;

typedef struct brush_s
{
	int				originalentitynum;
	int				originalbrushnum;
    int             entitynum;
    int             brushnum;

    int             firstside;
    int             numsides;

    unsigned int    noclip; // !!!FIXME: this should be a flag bitfield so we can use it for other stuff (ie. is this a detail brush...)
	unsigned int	cliphull;
	bool			bevel;
	int				detaillevel;
	int				chopdown; // allow this brush to chop brushes of lower detail level
	int				chopup; // allow this brush to be chopped by brushes of higher detail level
	int				clipnodedetaillevel;
	int				coplanarpriority;
	char *			hullshapes[NUM_HULLS]; // might be NULL

    int             contents;
    brushhull_t     hulls[NUM_HULLS];
} brush_t;

typedef struct
{
	vec3_t normal;
	vec3_t point;

	int numvertexes;
	vec3_t *vertexes;
} hullbrushface_t;

typedef struct
{
	vec3_t normals[2];
	vec3_t point;

	vec3_t vertexes[2];
	vec3_t delta; // delta has the same direction as CrossProduct(normals[0],normals[1])
} hullbrushedge_t;

typedef struct
{
	vec3_t point;
} hullbrushvertex_t;

typedef struct
{
	int numfaces;
	hullbrushface_t *faces;
	int numedges;
	hullbrushedge_t *edges;
	int numvertexes;
	hullbrushvertex_t *vertexes;
} hullbrush_t;

typedef struct
{
	char *id;
	bool disabled;
	int numbrushes; // must be 0 or 1
	hullbrush_t **brushes;
} hullshape_t;

#ifdef HLCSG_GAMETEXTMESSAGE_UTF8
extern char *	ANSItoUTF8 (const char *);
#endif

//=============================================================================
// map.c

extern int      g_nummapbrushes;
extern brush_t  g_mapbrushes[MAX_MAP_BRUSHES];

#define MAX_MAP_SIDES   (MAX_MAP_BRUSHES*6)

extern int      g_numbrushsides;
extern side_t   g_brushsides[MAX_MAP_SIDES];

extern hullshape_t g_defaulthulls[NUM_HULLS];
extern int		g_numhullshapes;
extern hullshape_t g_hullshapes[MAX_HULLSHAPES];

extern void     TextureAxisFromPlane(const plane_t* const pln, vec3_t xv, vec3_t yv);
extern void     LoadMapFile(const char* const filename);

//=============================================================================
// textures.c

typedef std::deque< std::string >::iterator WadInclude_i;
extern std::deque< std::string > g_WadInclude;  // List of substrings to wadinclude

extern void     WriteMiptex();
extern int      TexinfoForBrushTexture(const plane_t* const plane, brush_texture_t* bt, const vec3_t origin
					);
extern const char *GetTextureByNumber_CSG(int texturenumber);

//=============================================================================
// brush.c

extern brush_t* Brush_LoadEntity(entity_t* ent, int hullnum);
extern contents_t CheckBrushContents(const brush_t* const b);

extern void     CreateBrush(int brushnum);
extern void		CreateHullShape (int entitynum, bool disabled, const char *id, int defaulthulls);
extern void		InitDefaultHulls ();

//=============================================================================
// csg.c

extern bool     g_chart;
extern bool     g_onlyents;
extern bool     g_noclip;
extern bool     g_wadtextures;
extern bool     g_skyclip;
extern bool     g_estimate;         
extern const char* g_hullfile;        

extern bool     g_bUseNullTex; 


extern bool     g_bClipNazi; 

#define EnumPrint(a) #a
typedef enum{clip_smallest,clip_normalized,clip_simple,clip_precise,clip_legacy} cliptype;
extern cliptype g_cliptype;
extern const char*	GetClipTypeString(cliptype);

extern vec_t g_scalesize;
extern bool g_resetlog;
extern bool g_nolightopt;
#ifdef HLCSG_GAMETEXTMESSAGE_UTF8
extern bool g_noutf8;
#endif
extern bool g_nullifytrigger;

extern vec_t    g_tiny_threshold;
extern vec_t    g_BrushUnionThreshold;

extern plane_t  g_mapplanes[MAX_INTERNAL_MAP_PLANES];
extern int      g_nummapplanes;

extern bface_t* NewFaceFromFace(const bface_t* const in);
extern bface_t* CopyFace(const bface_t* const f);

extern void     FreeFace(bface_t* f);

extern bface_t* CopyFaceList(bface_t* f);
extern void     FreeFaceList(bface_t* f);

extern void     GetParamsFromEnt(entity_t* mapent);


//=============================================================================
// brushunion.c
void            CalculateBrushUnions(int brushnum);
 
//============================================================================
// hullfile.cpp
extern vec3_t   g_hull_size[NUM_HULLS][2];
extern void     LoadHullfile(const char* filename);

extern const char *g_wadcfgfile;
extern const char *g_wadconfigname;
extern void LoadWadcfgfile (const char *filename);
extern void LoadWadconfig (const char *filename, const char *configname);

//============================================================================
// autowad.cpp      AJM

extern bool     g_bWadAutoDetect; 


//=============================================================================
// properties.cpp

#include <string>
#include <set>
extern void properties_initialize(const char* filename);
extern std::set< std::string > g_invisible_items;

//============================================================================
#endif//HLCSG_H__
