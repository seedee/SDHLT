#ifndef HLRAD_H__
#define HLRAD_H__

#if _MSC_VER >= 1000
#pragma once
#endif

#include "cmdlib.h"
#include "messages.h"
#include "win32fix.h"
#include "log.h"
#include "hlassert.h"
#include "mathlib.h"
#include "bspfile.h"
#include "winding.h"
#include "scriplib.h"
#include "threads.h"
#include "blockmem.h"
#include "filelib.h"
#include "winding.h"
#include "compress.h"
#include "cmdlinecfg.h"

#include <vector>

#ifdef SYSTEM_WIN32
#pragma warning(disable: 4142 4028)
#include <io.h>
#pragma warning(default: 4142 4028)
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef STDC_HEADERS
#include <ctype.h>
#endif

#ifdef SYSTEM_WIN32
#include <direct.h>
#endif
#include <string>

#define DEFAULT_PRE25UPDATE         false
#define DEFAULT_FASTMODE			false
#define DEFAULT_METHOD eMethodSparseVismatrix
#define DEFAULT_LERP_ENABLED        true
#define DEFAULT_STUDIOSHADOW        true //seedee
#define DEFAULT_FADE                1.0
#define DEFAULT_BOUNCE              8
#define DEFAULT_DUMPPATCHES         false
#define DEFAULT_AMBIENT_RED         0.0
#define DEFAULT_AMBIENT_GREEN       0.0
#define DEFAULT_AMBIENT_BLUE        0.0
// 188 is the fullbright threshold for Goldsrc before 25th anniversary, regardless of the brightness and gamma settings in the graphic options. This is no longer necessary
// However, hlrad can only control the light values of each single light style. So the final in-game brightness may exceed 188 if you have set a high value in the "custom appearance" of the light, or if the face receives light from different styles.
#define DEFAULT_LIMITTHRESHOLD		255.0 // We override to 188 with pre25 argument. //seedee
#define DEFAULT_TEXSCALE            true
#define DEFAULT_CHOP                64.0
#define DEFAULT_TEXCHOP             32.0
#define DEFAULT_LIGHTSCALE          2.0 //1.0 //vluzacn
#define DEFAULT_DLIGHT_THRESHOLD	10.0
#define DEFAULT_DLIGHT_SCALE        1.0 //2.0 //vluzacn
#define DEFAULT_SMOOTHING_VALUE     50.0
#define DEFAULT_SMOOTHING2_VALUE	0
#define DEFAULT_INCREMENTAL         false


// ------------------------------------------------------------------------
// Changes by Adam Foster - afoster@compsoc.man.ac.uk

// superseded by DEFAULT_COLOUR_LIGHTSCALE_*

// superseded by DEFAULT_COLOUR_GAMMA_*
// ------------------------------------------------------------------------

#define DEFAULT_INDIRECT_SUN        1.0
#define DEFAULT_EXTRA               false
#define DEFAULT_SKY_LIGHTING_FIX    true
#define DEFAULT_CIRCUS              false
#define DEFAULT_CORING				0.01
#define DEFAULT_SUBDIVIDE           true
#define DEFAULT_CHART               true //seedee
#define DEFAULT_INFO                true
#define DEFAULT_ALLOW_OPAQUES       true
#define DEFAULT_ALLOW_SPREAD		true

// ------------------------------------------------------------------------
// Changes by Adam Foster - afoster@compsoc.man.ac.uk

#define DEFAULT_COLOUR_GAMMA_RED		0.55
#define DEFAULT_COLOUR_GAMMA_GREEN		0.55
#define DEFAULT_COLOUR_GAMMA_BLUE		0.55

#define DEFAULT_COLOUR_LIGHTSCALE_RED		2.0 //1.0 //vluzacn
#define DEFAULT_COLOUR_LIGHTSCALE_GREEN		2.0 //1.0 //vluzacn
#define DEFAULT_COLOUR_LIGHTSCALE_BLUE		2.0 //1.0 //vluzacn

#define DEFAULT_COLOUR_JITTER_HACK_RED		0.0
#define DEFAULT_COLOUR_JITTER_HACK_GREEN	0.0
#define DEFAULT_COLOUR_JITTER_HACK_BLUE		0.0

#define DEFAULT_JITTER_HACK_RED			0.0
#define DEFAULT_JITTER_HACK_GREEN		0.0
#define DEFAULT_JITTER_HACK_BLUE		0.0



// ------------------------------------------------------------------------

// O_o ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Changes by Jussi Kivilinna <hullu@unitedadmins.com> [http://hullu.xtragaming.com/]
	// Transparency light support for bounced light(transfers) is extreamly slow 
	// for 'vismatrix' and 'sparse' atm. 
	// Only recommended to be used with 'nomatrix' mode
	#define DEFAULT_CUSTOMSHADOW_WITH_BOUNCELIGHT false

	// RGB Transfers support for HLRAD .. to be used with -customshadowwithbounce
	#define DEFAULT_RGB_TRANSFERS false
// o_O ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

	#define DEFAULT_TRANSTOTAL_HACK 0.2 //0.5 //vluzacn
	#define DEFAULT_MINLIGHT 0
	#define DEFAULT_TRANSFER_COMPRESS_TYPE FLOAT16
	#define DEFAULT_RGBTRANSFER_COMPRESS_TYPE VECTOR32
	#define DEFAULT_SOFTSKY true
	#define DEFAULT_BLOCKOPAQUE 1
	#define DEFAULT_TRANSLUCENTDEPTH 2.0f
	#define DEFAULT_NOTEXTURES false
	#define DEFAULT_TEXREFLECTGAMMA 1.76f // 2.0(texgamma cvar) / 2.5 (gamma cvar) * 2.2 (screen gamma) = 1.76
	#define DEFAULT_TEXREFLECTSCALE 0.7f // arbitrary (This is lower than 1.0, because textures are usually brightened in order to look better in Goldsrc. Textures are made brightened because Goldsrc is only able to darken the texture when combining the texture with the lightmap.)
	#define DEFAULT_BLUR 1.5 // classic lighting is equivalent to "-blur 1.0"
	#define DEFAULT_NOEMITTERRANGE false
	#define DEFAULT_BLEEDFIX true
	#define DEFAULT_EMBEDLIGHTMAP_POWEROFTWO true
	#define DEFAULT_EMBEDLIGHTMAP_DENOMINATOR 188.0
	#define DEFAULT_EMBEDLIGHTMAP_GAMMA 1.05
	#define DEFAULT_EMBEDLIGHTMAP_RESOLUTION 1
	#define DEFAULT_TEXLIGHTGAP 0.0


#ifdef SYSTEM_WIN32
#define DEFAULT_ESTIMATE    false
#endif
#ifdef SYSTEM_POSIX
#define DEFAULT_ESTIMATE    true
#endif

// Ideally matches what is in the FGD :)
#define SPAWNFLAG_NOBLEEDADJUST    (1 << 0)

// DEFAULT_HUNT_OFFSET is how many units in front of the plane to place the samples
// Unit of '1' causes the 1 unit crate trick to cause extra shadows
#define DEFAULT_HUNT_OFFSET 0.5
// DEFAULT_HUNT_SIZE number of iterations (one based) of radial search in HuntForWorld
#define DEFAULT_HUNT_SIZE   11
// DEFAULT_HUNT_SCALE amount to grow from origin point per iteration of DEFAULT_HUNT_SIZE in HuntForWorld
#define DEFAULT_HUNT_SCALE 0.1
#define DEFAULT_EDGE_WIDTH 0.8

#define PATCH_HUNT_OFFSET 0.5 //--vluzacn
#define HUNT_WALL_EPSILON (3 * ON_EPSILON) // place sample at least this distance away from any wall //--vluzacn

#define MINIMUM_PATCH_DISTANCE ON_EPSILON
#define ACCURATEBOUNCE_THRESHOLD 4.0 // If the receiver patch is closer to emitter patch than EXACTBOUNCE_THRESHOLD * emitter_patch->radius, calculate the exact visibility amount.
#define ACCURATEBOUNCE_DEFAULT_SKYLEVEL 5 // sample 1026 normals


#define ALLSTYLES 64 // HL limit. //--vluzacn

#define BOGUS_RANGE 131072

typedef struct
{
	vec_t v[4][3];
}
matrix_t;

// a 4x4 matrix that represents the following transformation (see the ApplyMatrix function)
//
//  / X \    / v[0][0] v[1][0] v[2][0] v[3][0] \ / X \.
//  | Y | -> | v[0][1] v[1][1] v[2][1] v[3][1] | | Y |
//  | Z |    | v[0][2] v[1][2] v[2][2] v[3][2] | | Z |
//  \ 1 /    \    0       0       0       1    / \ 1 /

//
// LIGHTMAP.C STUFF
//

typedef enum
{
    emit_surface,
    emit_point,
    emit_spotlight,
    emit_skylight
}
emittype_t;

typedef struct directlight_s
{
    struct directlight_s* next;
    emittype_t      type;
    int             style;
    vec3_t          origin;
    vec3_t          intensity;
    vec3_t          normal;                                // for surfaces and spotlights
    float           stopdot;                               // for spotlights
    float           stopdot2;                              // for spotlights

    // 'Arghrad'-like features
    vec_t           fade;                                  // falloff scaling for linear and inverse square falloff 1.0 = normal, 0.5 = farther, 2.0 = shorter etc

	// -----------------------------------------------------------------------------------
	// Changes by Adam Foster - afoster@compsoc.man.ac.uk
	// Diffuse light_environment light colour
	// Really horrible hack which probably won't work!
	vec3_t			diffuse_intensity;
	// -----------------------------------------------------------------------------------
	vec3_t			diffuse_intensity2;
	vec_t			sunspreadangle;
	int				numsunnormals;
	vec3_t*			sunnormals;
	vec_t*			sunnormalweights;

	vec_t			patch_area;
	vec_t			patch_emitter_range;
	struct patch_s	*patch;
	vec_t			texlightgap;
	bool			topatch;
} directlight_t;


typedef struct
{
    unsigned size  : 12;
    unsigned index : 20;
} transfer_index_t;

typedef unsigned transfer_raw_index_t;
typedef unsigned char transfer_data_t;

typedef unsigned char rgb_transfer_data_t;

#define MAX_COMPRESSED_TRANSFER_INDEX_SIZE ((1 << 12) - 1)

#define	MAX_PATCHES	(65535*16) // limited by transfer_index_t
#define MAX_VISMATRIX_PATCHES 65535
#define MAX_SPARSE_VISMATRIX_PATCHES MAX_PATCHES

typedef enum
{
    ePatchFlagNull = 0,
    ePatchFlagOutside = 1
} ePatchFlags;

typedef struct patch_s
{
    struct patch_s* next;                                  // next in face
    vec3_t          origin;                                // Center centroid of winding (cached info calculated from winding)
    vec_t           area;                                  // Surface area of this patch (cached info calculated from winding)
	vec_t			exposure;
	vec_t			emitter_range;                         // Range from patch origin (cached info calculated from winding)
	int				emitter_skylevel;                      // The "skylevel" used for sampling of normals, when the receiver patch is within the range of ACCURATEBOUNCE_THRESHOLD * this->radius. (cached info calculated from winding)
    Winding*        winding;                               // Winding (patches are triangles, so its easy)
    vec_t           scale;                                 // Texture scale for this face (blend of S and T scale)
    vec_t           chop;                                  // Texture chop for this face factoring in S and T scale

    unsigned        iIndex;
    unsigned        iData;

    transfer_index_t* tIndex;
    transfer_data_t*  tData;
    rgb_transfer_data_t*	tRGBData;

    int             faceNumber;
    ePatchFlags     flags;
	bool			translucent_b;                           // gather light from behind
	vec3_t			translucent_v;
	vec3_t			texturereflectivity;
	vec3_t			bouncereflectivity;

	unsigned char	totalstyle[MAXLIGHTMAPS];
	unsigned char	directstyle[MAXLIGHTMAPS];
	// HLRAD_AUTOCORING: totallight: all light gathered by patch
	vec3_t          totallight[MAXLIGHTMAPS];				// accumulated by radiosity does NOT include light accounted for by direct lighting
	// HLRAD_AUTOCORING: directlight: emissive light gathered by sample
	vec3_t			directlight[MAXLIGHTMAPS];				// direct light only
	int				bouncestyle; // light reflected from this patch must convert to this style. -1 = normal (don't convert)
	unsigned char	emitstyle;
    vec3_t          baselight;                             // emissivity only, uses emitstyle
	bool			emitmode;								// texlight emit mode. 1 for normal, 0 for fast.
	vec_t			samples;
	vec3_t*			samplelight_all;						// NULL, or [ALLSTYLES] during BuildFacelights
	unsigned char*	totalstyle_all;						// NULL, or [ALLSTYLES] during BuildFacelights
	vec3_t*			totallight_all;						// NULL, or [ALLSTYLES] during BuildFacelights
	vec3_t*			directlight_all;						// NULL, or [ALLSTYLES] during BuildFacelights
	int				leafnum;
} patch_t;

//LRC
vec3_t* GetTotalLight(patch_t* patch, int style
	);

typedef struct facelist_s
{
	dface_t*		face;
	facelist_s*	next;
} facelist_t;
typedef struct
{
    dface_t*        faces[2];
    vec3_t          interface_normal; // HLRAD_GetPhongNormal_VL: this field must be set when smooth==true
	vec3_t			vertex_normal[2];
    vec_t           cos_normals_angle; // HLRAD_GetPhongNormal_VL: this field must be set when smooth==true
    bool            coplanar;
	bool			smooth;
	facelist_t*		vertex_facelist[2]; //possible smooth faces, not include faces[0] and faces[1]
	matrix_t		textotex[2]; // how we translate texture coordinates from one face to the other face
} edgeshare_t;

extern edgeshare_t g_edgeshare[MAX_MAP_EDGES];

//
// lerp.c stuff
//

// These are bitflags for lighting adjustments for special cases
typedef enum
{
    eModelLightmodeNull     = 0,
    eModelLightmodeOpaque   = 0x02,
	eModelLightmodeNonsolid = 0x08, // for opaque entities with {texture
}
eModelLightmodes;


typedef struct
{
	int entitynum;
	int modelnum;
	vec3_t origin;

    vec3_t transparency_scale;
    bool transparency;
	int style; // -1 = no style; transparency must be false if style >= 0
	// style0 and same style will change to this style, other styles will be blocked.
	bool block; // this entity can't be seen inside, so all lightmap sample should move outside.

} opaqueList_t;

#define OPAQUE_ARRAY_GROWTH_SIZE 1024

typedef struct
{
	char name[16]; // not always same with the name in texdata
	int width, height;
	byte *canvas; //[height][width]
	byte palette[256][3];
	vec3_t reflectivity;
} radtexture_t;
extern int g_numtextures;
extern radtexture_t *g_textures;
extern void AddWadFolder (const char *path);
extern void LoadTextures ();
extern void EmbedLightmapInTextures ();

typedef struct
{
	std::string     name;
	float          value;
}
minlight_t; //info_minlights

typedef std::vector<minlight_t>::iterator minlight_i;

//
// qrad globals
//

extern std::vector<minlight_t> s_minlights;
extern patch_t* g_face_patches[MAX_MAP_FACES];
extern entity_t* g_face_entity[MAX_MAP_FACES];
extern vec3_t   g_face_offset[MAX_MAP_FACES];              // for models with origins
extern eModelLightmodes g_face_lightmode[MAX_MAP_FACES];
extern vec3_t   g_face_centroids[MAX_MAP_EDGES];
extern entity_t* g_face_texlights[MAX_MAP_FACES];
extern patch_t* g_patches; // shrinked to its real size, because 1048576 patches * 256 bytes = 256MB will be too big
extern unsigned g_num_patches;

extern float    g_lightscale;
extern float    g_dlight_threshold;
extern float    g_coring;
extern int      g_lerp_enabled;

extern void     MakeShadowSplits();

//==============================================

extern bool		g_fastmode;
extern bool     g_extra;
extern vec3_t   g_ambient;
extern vec_t    g_direct_scale;
extern vec_t	g_limitthreshold;
extern bool		g_drawoverload;
extern unsigned g_numbounce;
extern float    g_qgamma;
extern float    g_indirect_sun;
extern float    g_smoothing_threshold;
extern float    g_smoothing_value;
extern float g_smoothing_threshold_2;
extern float g_smoothing_value_2;
extern vec_t *g_smoothvalues; //[nummiptex]
extern bool     g_estimate;
extern char     g_source[_MAX_PATH];
extern vec_t    g_fade;
extern bool     g_incremental;
extern bool     g_circus;
extern bool		g_allow_spread;
extern bool     g_sky_lighting_fix;
extern vec_t    g_chop;    // Chop value for normal textures
extern vec_t    g_texchop; // Chop value for texture lights
extern opaqueList_t* g_opaque_face_list;
extern unsigned      g_opaque_face_count; // opaque entity count //HLRAD_OPAQUE_NODE
extern unsigned      g_max_opaque_face_count;    // Current array maximum (used for reallocs)


// ------------------------------------------------------------------------
// Changes by Adam Foster - afoster@compsoc.man.ac.uk

extern vec3_t	g_colour_qgamma;
extern vec3_t	g_colour_lightscale;

extern vec3_t	g_colour_jitter_hack;
extern vec3_t	g_jitter_hack;

// ------------------------------------------------------------------------


	extern bool	g_customshadow_with_bouncelight;
	extern bool	g_rgb_transfers;
	extern const vec3_t vec3_one;

	extern float g_transtotal_hack;
	extern unsigned char g_minlight;
	extern float_type g_transfer_compress_type;
	extern vector_type g_rgbtransfer_compress_type;
	extern bool g_softsky;
	extern int g_blockopaque;
	extern bool g_drawpatch;
	extern bool g_drawsample;
	extern vec3_t g_drawsample_origin;
	extern vec_t g_drawsample_radius;
	extern bool g_drawedge;
	extern bool g_drawlerp;
	extern bool g_drawnudge;
	extern float g_corings[ALLSTYLES];
	extern int stylewarningcount; // not thread safe
	extern int stylewarningnext; // not thread safe
	extern vec3_t *g_translucenttextures;
	extern vec_t g_translucentdepth;
	extern vec3_t *g_lightingconeinfo; //[nummiptex]; X component = power, Y component = scale, Z component = nothing
	extern bool g_notextures;
	extern vec_t g_texreflectgamma;
	extern vec_t g_texreflectscale;
	extern vec_t g_blur;
	extern bool g_noemitterrange;
	extern bool g_bleedfix;
	extern vec_t g_maxdiscardedlight;
	extern vec3_t g_maxdiscardedpos;
	extern vec_t g_texlightgap;

extern void     MakeTnodes(dmodel_t* bm);
extern void     PairEdges();
#define SKYLEVELMAX 8
#define SKYLEVEL_SOFTSKYON 7
#define SKYLEVEL_SOFTSKYOFF 4
#define SUNSPREAD_SKYLEVEL 7
#define SUNSPREAD_THRESHOLD 15.0
extern int		g_numskynormals[SKYLEVELMAX+1]; // 0, 6, 18, 66, 258, 1026, 4098, 16386, 65538
extern vec3_t*	g_skynormals[SKYLEVELMAX+1]; //[numskynormals]
extern vec_t*	g_skynormalsizes[SKYLEVELMAX+1]; // the weight of each normal
extern void     BuildDiffuseNormals ();
extern void     BuildFacelights(int facenum);
extern void     PrecompLightmapOffsets();
extern void		ReduceLightmap ();
extern void     FinalLightFace(int facenum);
extern void		ScaleDirectLights (); // run before AddPatchLights
extern void		CreateFacelightDependencyList (); // run before AddPatchLights
extern void		AddPatchLights (int facenum);
extern void		FreeFacelightDependencyList ();
extern int      TestLine(const vec3_t start, const vec3_t stop
						 , vec_t *skyhitout = NULL
						 );
#define OPAQUE_NODE_INLINECALL
#ifdef OPAQUE_NODE_INLINECALL
typedef struct
{
	vec3_t mins, maxs;
	int headnode;
} opaquemodel_t;
extern opaquemodel_t *opaquemodels;
#endif
extern void		CreateOpaqueNodes();
extern int		TestLineOpaque(int modelnum, const vec3_t modelorigin, const vec3_t start, const vec3_t stop);
extern int		CountOpaqueFaces(int modelnum);
extern void		DeleteOpaqueNodes();
#ifdef OPAQUE_NODE_INLINECALL
extern int TestPointOpaque_r (int nodenum, bool solid, const vec3_t point);
FORCEINLINE int TestPointOpaque (int modelnum, const vec3_t modelorigin, bool solid, const vec3_t point) // use "forceinline" because "inline" does nothing here
{
	opaquemodel_t *thismodel = &opaquemodels[modelnum];
	vec3_t newpoint;
	VectorSubtract (point, modelorigin, newpoint);
	int axial;
	for (axial = 0; axial < 3; axial++)
	{
		if (newpoint[axial] > thismodel->maxs[axial])
			return 0;
		if (newpoint[axial] < thismodel->mins[axial])
			return 0;
	}
	return TestPointOpaque_r (thismodel->headnode, solid, newpoint);
}
#else
extern int		TestPointOpaque (int modelnum, const vec3_t modelorigin, bool solid, const vec3_t point);
#endif
extern void     CreateDirectLights();
extern void     DeleteDirectLights();
extern void     GetPhongNormal(int facenum, const vec3_t spot, vec3_t phongnormal); // added "const" --vluzacn

typedef bool (*funcCheckVisBit) (unsigned, unsigned
								 , vec3_t&
								 , unsigned int&
								 );
extern funcCheckVisBit g_CheckVisBit;
extern bool CheckVisBitBackwards(unsigned receiver, unsigned emitter, const vec3_t &backorigin, const vec3_t &backnormal
								, vec3_t &transparency_out
								);
extern void	    MdlLightHack(void);

// qradutil.c
extern vec_t    PatchPlaneDist(const patch_t* const patch);
extern dleaf_t* PointInLeaf(const vec3_t point);
extern void     MakeBackplanes();
extern const dplane_t* getPlaneFromFace(const dface_t* const face);
extern const dplane_t* getPlaneFromFaceNumber(unsigned int facenum);
extern void     getAdjustedPlaneFromFaceNumber(unsigned int facenum, dplane_t* plane);
extern dleaf_t* HuntForWorld(vec_t* point, const vec_t* plane_offset, const dplane_t* plane, int hunt_size, vec_t hunt_scale, vec_t hunt_offset);
extern void		ApplyMatrix (const matrix_t &m, const vec3_t in, vec3_t &out);
extern void		ApplyMatrixOnPlane (const matrix_t &m_inverse, const vec3_t in_normal, vec_t in_dist, vec3_t &out_normal, vec_t &out_dist);
extern void		MultiplyMatrix (const matrix_t &m_left, const matrix_t &m_right, matrix_t &m);
extern matrix_t	MultiplyMatrix (const matrix_t &m_left, const matrix_t &m_right);
extern void		MatrixForScale (const vec3_t center, vec_t scale, matrix_t &m);
extern matrix_t	MatrixForScale (const vec3_t center, vec_t scale);
extern vec_t	CalcMatrixSign (const matrix_t &m);
extern void		TranslateWorldToTex (int facenum, matrix_t &m);
extern bool		InvertMatrix (const matrix_t &m, matrix_t &m_inverse);
extern void		FindFacePositions (int facenum);
extern void		FreePositionMaps ();
extern bool		FindNearestPosition (int facenum, const Winding *texwinding, const dplane_t &texplane, vec_t s, vec_t t, vec3_t &pos, vec_t *best_s, vec_t *best_t, vec_t *best_dist
									, bool *nudged
									);

// makescales.c
extern void     MakeScalesVismatrix();
extern void     MakeScalesSparseVismatrix();
extern void     MakeScalesNoVismatrix();

// transfers.c
extern size_t   g_total_transfer;
extern bool     readtransfers(const char* const transferfile, long numpatches);
extern void     writetransfers(const char* const transferfile, long total_patches);

// vismatrixutil.c (shared between vismatrix.c and sparse.c)
extern void     MakeScales(int threadnum);
extern void     DumpTransfersMemoryUsage();
extern void     MakeRGBScales(int threadnum);

// transparency.c (transparency array functions - shared between vismatrix.c and sparse.c)
extern void	GetTransparency(const unsigned p1, const unsigned p2, vec3_t &trans, unsigned int &next_index);
extern void	AddTransparencyToRawArray(const unsigned p1, const unsigned p2, const vec3_t trans);
extern void	CreateFinalTransparencyArrays(const char *print_name);
extern void	FreeTransparencyArrays();
extern void GetStyle(const unsigned p1, const unsigned p2, int &style, unsigned int &next_index);
extern void	AddStyleToStyleArray(const unsigned p1, const unsigned p2, const int style);
extern void	CreateFinalStyleArrays(const char *print_name);
extern void	FreeStyleArrays();

// lerp.c
extern void CreateTriangulations (int facenum);
extern void GetTriangulationPatches (int facenum, int *numpatches, const int **patches);
extern void InterpolateSampleLight (const vec3_t position, int surface, int numstyles, const int *styles, vec3_t *outs
				);
extern void FreeTriangulations ();

// mathutil.c
extern bool     TestSegmentAgainstOpaqueList(const vec_t* p1, const vec_t* p2
					, vec3_t &scaleout
					, int &opaquestyleout
					);
extern bool     intersect_line_plane(const dplane_t* const plane, const vec_t* const p1, const vec_t* const p2, vec3_t point);
extern bool     intersect_linesegment_plane(const dplane_t* const plane, const vec_t* const p1, const vec_t* const p2,vec3_t point);
extern void     plane_from_points(const vec3_t p1, const vec3_t p2, const vec3_t p3, dplane_t* plane);
extern bool     point_in_winding(const Winding& w, const dplane_t& plane, const vec_t* point
					, vec_t epsilon = 0.0
					);
extern bool     point_in_winding_noedge(const Winding& w, const dplane_t& plane, const vec_t* point, vec_t width);
extern void     snap_to_winding(const Winding& w, const dplane_t& plane, vec_t* point);
extern vec_t	snap_to_winding_noedge(const Winding& w, const dplane_t& plane, vec_t* point, vec_t width, vec_t maxmove);
extern void     SnapToPlane(const dplane_t* const plane, vec_t* const point, vec_t offset);
extern vec_t	CalcSightArea (const vec3_t receiver_origin, const vec3_t receiver_normal, const Winding *emitter_winding, int skylevel
					, vec_t lighting_power, vec_t lighting_scale
					);
extern vec_t	CalcSightArea_SpotLight (const vec3_t receiver_origin, const vec3_t receiver_normal, const Winding *emitter_winding, const vec3_t emitter_normal, vec_t emitter_stopdot, vec_t emitter_stopdot2, int skylevel
					, vec_t lighting_power, vec_t lighting_scale
					);
extern void		GetAlternateOrigin (const vec3_t pos, const vec3_t normal, const patch_t *patch, vec3_t &origin);

// studio.cpp
extern void LoadStudioModels(void);
extern void FreeStudioModels(void);
extern bool TestSegmentAgainstStudioList(const vec_t* p1, const vec_t* p2);
extern bool g_studioshadow;

#endif //HLRAD_H__
