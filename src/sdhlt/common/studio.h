/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

#ifndef STUDIO_H
#define STUDIO_H

/*
==============================================================================

STUDIO MODELS

Studio models are position independent, so the cache manager can move them.
==============================================================================
*/

// header
#define STUDIO_VERSION	10
#define IDSTUDIOHEADER	(('T'<<24)+('S'<<16)+('D'<<8)+'I') // little-endian "IDST"
#define IDSEQGRPHEADER	(('Q'<<24)+('S'<<16)+('D'<<8)+'I') // little-endian "IDSQ"

// studio limits
#define MAXSTUDIOTRIANGLES		65535	// max triangles per model
#define MAXSTUDIOVERTS		32768	// max vertices per submodel
#define MAXSTUDIOSEQUENCES		256	// total animation sequences
#define MAXSTUDIOSKINS		128	// total textures
#define MAXSTUDIOSRCBONES		512	// bones allowed at source movement
#define MAXSTUDIOBONES		128	// total bones actually used
#define MAXSTUDIOMODELS		32	// sub-models per model
#define MAXSTUDIOBODYPARTS		32	// body parts per submodel
#define MAXSTUDIOGROUPS		16	// sequence groups (e.g. barney01.mdl, barney02.mdl, e.t.c)
#define MAXSTUDIOANIMATIONS		512	// max frames per sequence
#define MAXSTUDIOMESHES		256	// max textures per model
#define MAXSTUDIOEVENTS		1024	// events per model
#define MAXSTUDIOPIVOTS		256	// pivot points
#define MAXSTUDIOBLENDS		16	// max anim blends
#define MAXSTUDIOCONTROLLERS		8	// max controllers per model
#define MAXSTUDIOATTACHMENTS		4	// max attachments per model

// client-side model flags
#define STUDIO_ROCKET		0x0001	// leave a trail
#define STUDIO_GRENADE		0x0002	// leave a trail
#define STUDIO_GIB			0x0004	// leave a trail
#define STUDIO_ROTATE		0x0008	// rotate (bonus items)
#define STUDIO_TRACER		0x0010	// green split trail
#define STUDIO_ZOMGIB		0x0020	// small blood trail
#define STUDIO_TRACER2		0x0040	// orange split trail + rotate
#define STUDIO_TRACER3		0x0080	// purple trail
#define STUDIO_DYNAMIC_LIGHT		0x0100	// dynamically get lighting from floor or ceil (flying monsters)
#define STUDIO_TRACE_HITBOX		0x0200	// always use hitbox trace instead of bbox

#define STUDIO_HAS_BUMP		(1<<16)	// loadtime set

// lighting & rendermode options
#define STUDIO_NF_FLATSHADE		0x0001
#define STUDIO_NF_CHROME		0x0002
#define STUDIO_NF_FULLBRIGHT		0x0004
#define STUDIO_NF_COLORMAP		0x0008	// can changed by colormap command
#define STUDIO_NF_NOSMOOTH		0x0010	// rendering as semiblended
#define STUDIO_NF_ADDITIVE		0x0020	// rendering with additive mode
#define STUDIO_NF_TRANSPARENT		0x0040	// use texture with alpha channel
#define STUDIO_NF_NORMALMAP		0x0080	// indexed normalmap
#define STUDIO_NF_GLOSSMAP		0x0100	// glossmap
#define STUDIO_NF_GLOSSPOWER		0x0200
#define STUDIO_NF_LUMA		0x0400	// self-illuminate parts
#define STUDIO_NF_ALPHASOLID		0x0800	// use with STUDIO_NF_TRANSPARENT to have solid alphatest surfaces for env_static
#define STUDIO_NF_TWOSIDE		0x1000	// render mesh as twosided

#define STUDIO_NF_NODRAW		(1<<16)	// failed to create shader for this mesh
#define STUDIO_NF_NODLIGHT		(1<<17)	// failed to create dlight shader for this mesh
#define STUDIO_NF_NOSUNLIGHT		(1<<18)	// failed to create sun light shader for this mesh
#define STUDIO_DXT5_NORMALMAP		(1<<19)	// it's DXT5nm texture
#define STUDIO_NF_HAS_ALPHA		(1<<20)	// external texture has alpha-channel
#define STUDIO_NF_UV_COORDS		(1<<31)	// using half-float coords instead of ST

// motion flags
#define STUDIO_X			0x0001
#define STUDIO_Y			0x0002	
#define STUDIO_Z			0x0004
#define STUDIO_XR			0x0008
#define STUDIO_YR			0x0010
#define STUDIO_ZR			0x0020
#define STUDIO_LX			0x0040
#define STUDIO_LY			0x0080
#define STUDIO_LZ			0x0100
#define STUDIO_AX			0x0200
#define STUDIO_AY			0x0400
#define STUDIO_AZ			0x0800
#define STUDIO_AXR			0x1000
#define STUDIO_AYR			0x2000
#define STUDIO_AZR			0x4000
#define STUDIO_TYPES		0x7FFF
#define STUDIO_RLOOP		0x8000	// controller that wraps shortest distance

// bonecontroller types
#define STUDIO_MOUTH		4

// sequence flags
#define STUDIO_LOOPING		0x0001

// bone flags
#define BONE_HAS_NORMALS		0x0001
#define BONE_HAS_VERTICES		0x0002
#define BONE_HAS_BBOX		0x0004
#define BONE_JIGGLE_PROCEDURAL	0x0008

typedef struct
{
	int		ident;
	int		version;

	char		name[64];
	int		length;

	vec3_t		eyeposition;	// ideal eye position
	vec3_t		min;		// ideal movement hull size
	vec3_t		max;			

	vec3_t		bbmin;		// clipping bounding box
	vec3_t		bbmax;		

	int		flags;

	int		numbones;		// bones
	int		boneindex;

	int		numbonecontrollers;	// bone controllers
	int		bonecontrollerindex;

	int		numhitboxes;	// complex bounding boxes
	int		hitboxindex;			
	
	int		numseq;		// animation sequences
	int		seqindex;

	int		numseqgroups;	// demand loaded sequences
	int		seqgroupindex;

	int		numtextures;	// raw textures
	int		textureindex;
	int		texturedataindex;

	int		numskinref;	// replaceable textures
	int		numskinfamilies;
	int		skinindex;

	int		numbodyparts;		
	int		bodypartindex;

	int		numattachments;	// queryable attachable points
	int		attachmentindex;

#if defined (HLCSG) || defined (HLBSP) || defined (HLVIS) || defined (HLRAD) 
	int		soundtable;
	int		soundindex;
#else
	struct mbodypart_s	*bodyparts;	// pointer to VBO-prepared model (was soundtable)
	struct mstudiomat_s	*materials;	// studio materials
#endif
	int		numjigglebones;	// jiggle bones
	int		jiggleboneindex;

	int		numtransitions;	// animation node to animation node transition graph
	int		transitionindex;
} studiohdr_t;

// header for demand loaded sequence group data
typedef struct 
{
	int		id;
	int		version;

	char		name[64];
	int		length;
} studioseqhdr_t;

// bones
typedef struct 
{
	char		name[32];		// bone name for symbolic links
	int		parent;		// parent bone
	int		flags;		// ??
	int		bonecontroller[6];	// bone controller index, -1 == none
	float		value[6];		// default DoF values
	float		scale[6];		// scale for delta DoF values
} mstudiobone_t;

// JIGGLEBONES
#define JIGGLE_IS_FLEXIBLE		0x01
#define JIGGLE_IS_RIGID		0x02
#define JIGGLE_HAS_YAW_CONSTRAINT	0x04
#define JIGGLE_HAS_PITCH_CONSTRAINT	0x08
#define JIGGLE_HAS_ANGLE_CONSTRAINT	0x10
#define JIGGLE_HAS_LENGTH_CONSTRAINT	0x20
#define JIGGLE_HAS_BASE_SPRING	0x40

typedef struct 
{
	int			boneid;		// Do not use the flags in mstudiobone_t
	int			flags;

	// general params
	float			length;		// how from from bone base, along bone, is tip
	float			tipMass;

	// flexible params
	float			yawStiffness;
	float			yawDamping;	
	float			pitchStiffness;
	float			pitchDamping;	
	float			alongStiffness;
	float			alongDamping;	

	// angle constraint
	float			angleLimit;	// maximum deflection of tip in radians
	
	// yaw constraint
	float			minYaw;		// in radians
	float			maxYaw;		// in radians
	float			yawFriction;
	float			yawBounce;
	
	// pitch constraint
	float			minPitch;		// in radians
	float			maxPitch;		// in radians
	float			pitchFriction;
	float			pitchBounce;

	// base spring
	float			baseMass;
	float			baseStiffness;
	float			baseDamping;	
	float			baseMinLeft;
	float			baseMaxLeft;
	float			baseLeftFriction;
	float			baseMinUp;
	float			baseMaxUp;
	float			baseUpFriction;
	float			baseMinForward;
	float			baseMaxForward;
	float			baseForwardFriction;
} mstudiojigglebone_t;

// bone controllers
typedef struct 
{
	int		bone;		// -1 == 0
	int		type;		// X, Y, Z, XR, YR, ZR, M
	float		start;
	float		end;
	int		rest;		// byte index value at rest
	int		index;		// 0-3 user set controller, 4 mouth
} mstudiobonecontroller_t;

// intersection boxes
typedef struct
{
	int		bone;
	int		group;		// intersection group
	vec3_t		bbmin;		// bounding box
	vec3_t		bbmax;		
} mstudiobbox_t;

#ifndef CACHE_USER
#define CACHE_USER
typedef struct cache_user_s
{
	void		*data;		// extradata
} cache_user_t;
#endif

// demand loaded sequence groups
typedef struct
{
	char		label[32];	// textual name
	char		name[64];		// file name
	cache_user_t	cache;		// cache index pointer
	int		data;		// hack for group 0
} mstudioseqgroup_t;

// sequence descriptions
typedef struct
{
	char		label[32];	// sequence label

	float		fps;		// frames per second	
	int		flags;		// looping/non-looping flags

	int		activity;
	int		actweight;

	int		numevents;
	int		eventindex;

	int		numframes;	// number of frames per sequence

	int		numpivots;	// number of foot pivots
	int		pivotindex;

	int		motiontype;	
	int		motionbone;
	vec3_t		linearmovement;
	int		automoveposindex;
	int		automoveangleindex;

	vec3_t		bbmin;		// per sequence bounding box
	vec3_t		bbmax;		

	int		numblends;
	int		animindex;	// mstudioanim_t pointer relative to start of sequence group data
					// [blend][bone][X, Y, Z, XR, YR, ZR]

	int		blendtype[2];	// X, Y, Z, XR, YR, ZR
	float		blendstart[2];	// starting value
	float		blendend[2];	// ending value
	int		blendparent;

	int		seqgroup;		// sequence group for demand loading

	int		entrynode;	// transition node at entry
	int		exitnode;		// transition node at exit
	int		nodeflags;	// transition rules
	
	int		nextseq;		// auto advancing sequences
} mstudioseqdesc_t;

// events
typedef struct mstudioevent_s
{
	int		frame;
	int		event;
	int		type;
	char 		options[64];
} mstudioevent_t;

// pivots
typedef struct 
{
	vec3_t		org;		// pivot point
	int		start;
	int		end;
} mstudiopivot_t;

// attachment
typedef struct 
{
	char		name[32];
	int		type;
	int		bone;
	vec3_t		org;		// attachment point
	vec3_t		vectors[3];
} mstudioattachment_t;

typedef struct
{
	unsigned short	offset[6];
} mstudioanim_t;

// animation frames
typedef union 
{
	struct
	{
		byte	valid;
		byte	total;
	} num;
	short		value;
} mstudioanimvalue_t;

// body part index
typedef struct
{
	char		name[64];
	int		nummodels;
	int		base;
	int		modelindex;	// index into models array
} mstudiobodyparts_t;

// skin info
typedef struct mstudiotex_s
{
	char		name[64];
	int		flags;
	int		width;
	int		height;
	int		index;
} mstudiotexture_t;

// skin families
// short	index[skinfamilies][skinref]

// studio models
typedef struct
{
	char		name[64];

	int		type;
	float		boundingradius;

	int		nummesh;
	int		meshindex;

	int		numverts;		// number of unique vertices
	int		vertinfoindex;	// vertex bone info
	int		vertindex;	// vertex vec3_t
	int		numnorms;		// number of unique surface normals
	int		norminfoindex;	// normal bone info
	int		normindex;	// normal vec3_t

	int		numgroups;	// UNUSED
	int		groupindex;	// UNUSED
} mstudiomodel_t;

// vec3_t	boundingbox[model][bone][2];	// complex intersection info

// meshes
typedef struct 
{
	int		numtris;
	int		triindex;
	int		skinref;
	int		numnorms;		// per mesh normals
	int		normindex;	// UNUSED!
} mstudiomesh_t;

/*
===========================

USER-DEFINED DATA

===========================
*/
// this struct may be expaned by user request
typedef struct vbomesh_s
{
	unsigned int	skinref;			// skin reference
	unsigned short	numVerts;			// trifan vertices count
	unsigned int	numElems;			// trifan elements count

	unsigned int	vbo, vao, ibo;		// buffer objects
	vec3_t		mins, maxs;		// right transform to get screencopy
	int		parentbone;		// parent bone to transform AABB
} vbomesh_t;

// each mstudiotexture_t has a material
typedef struct mstudiomat_s
{
	mstudiotexture_t	*pSource;			// pointer to original texture

	unsigned short	gl_diffuse_id;		// diffuse texture
	unsigned short	gl_detailmap_id;		// detail texture
	unsigned short	gl_normalmap_id;		// normalmap
	unsigned short	gl_specular_id;		// specular
	unsigned short	gl_glowmap_id;		// self-illuminate parts

	// this part is shared with matdesc_t
	float		smoothness;		// smoothness factor
	float		detailScale[2];		// detail texture scales x, y
	float		reflectScale;		// reflection scale for translucent water
	float		refractScale;		// refraction scale for mirrors, windows, water
	float		aberrationScale;		// chromatic abberation
	struct matdef_s	*effects;			// hit, impact, particle effects etc

	int		flags;			// mstudiotexture_t->flags
	unsigned short	shaderNum;		// constantly assigned shader to this surface
	unsigned short	sunShaderNum;		// constantly assigned shader for sunlight to this surface
	unsigned short	lastRenderMode;		// for catch change render modes
	int		glsl_sequence;		// cache sequence
	int		glsl_sequence_sun;		// cache sequence
	int		lightstatus;		// light status
} mstudiomaterial_t;
	
typedef struct
{
	vbomesh_t		*meshes;			// meshes per submodel
	int		nummesh;			// mstudiomodel_t->nummesh
} msubmodel_t;

// triangles
typedef struct mbodypart_s
{
	int		base;			// mstudiobodyparts_t->base
	msubmodel_t	*models[MAXSTUDIOBODYPARTS];	// submodels per body part
	int		nummodels;		// mstudiobodyparts_t->nummodels
} mbodypart_t;

#endif//STUDIO_H