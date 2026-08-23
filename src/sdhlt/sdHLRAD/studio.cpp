#include "qrad.h"
#include "meshtrace.h"
#include "filelib.h"
#include "stringlib.h"

#define MAX_MODELS		MAX_STUDIOMODELS

model_t models[MAX_MODELS];
int num_models;

void LoadStudioModel( const char *modelname, const vec3_t origin, const vec3_t angles, const vec3_t scale, int body, int skin, int trace_mode )
{
	if( num_models >= MAX_MODELS )
	{
		Developer( DEVELOPER_LEVEL_ERROR, "LoadStudioModel: MAX_MODELS exceeded\n" );
		return;
	}
	model_t *m = &models[num_models];
	sprintf(m->name, "%s%s", g_Wadpath, modelname);
	FlipSlashes(m->name);
	ExtractFile(modelname, m->shortname);

	if (!q_exists(m->name))
	{
		Warning("LoadStudioModel: couldn't load %s\n", m->name);
		return;
	}
	LoadFile(m->name, (char**)&m->extradata);

	studiohdr_t *phdr = (studiohdr_t *)m->extradata;

	// well the textures place in separate file (very stupid case)
	if( phdr->numtextures == 0 )
	{
		char texname[128], texpath[128];
		byte *texdata, *moddata;
		studiohdr_t *thdr, *newhdr;
		safe_strncpy(texname, modelname, 128);
		StripExtension(texname);

		sprintf(texpath, "%s%sT.mdl", g_Wadpath, texname);
		FlipSlashes(texpath);

		LoadFile(texpath, (char**)&texdata);
		moddata = (byte *)m->extradata;
		phdr = (studiohdr_t *)moddata;

		thdr = (studiohdr_t *)texdata;

		// merge textures with main model buffer
		m->extradata = malloc( phdr->length + thdr->length - sizeof( studiohdr_t ));	// we don't need two headers
		memcpy( m->extradata, moddata, phdr->length );
		memcpy( (byte *)m->extradata + phdr->length, texdata + sizeof( studiohdr_t ), thdr->length - sizeof( studiohdr_t ));

		// merge header
		newhdr = (studiohdr_t *)m->extradata;

		newhdr->numskinfamilies = thdr->numskinfamilies;
		newhdr->numtextures = thdr->numtextures;
		newhdr->numskinref = thdr->numskinref;
		newhdr->textureindex = phdr->length;
		newhdr->skinindex = newhdr->textureindex + ( newhdr->numtextures * sizeof( mstudiotexture_t ));
		newhdr->texturedataindex = newhdr->skinindex + (newhdr->numskinfamilies * newhdr->numskinref * sizeof( short ));
		newhdr->length = phdr->length + thdr->length - sizeof( studiohdr_t );

		// and finally merge datapointers for textures
		for( int i = 0; i < newhdr->numtextures; i++ )
		{
			mstudiotexture_t *ptexture = (mstudiotexture_t *)(((byte *)newhdr) + newhdr->textureindex);
			ptexture[i].index += ( phdr->length - sizeof( studiohdr_t ));
//			printf( "Texture %i [%s]\n", i, ptexture[i].name );
			// now we can replace offsets with real pointers
//			ptexture[i].pixels = (byte *)newhdr + ptexture[i].index;
		}

		free( moddata );
		free( texdata );
	}
	else
	{
#if 0
		for( int i = 0; i < phdr->numtextures; i++ )
		{
			mstudiotexture_t *ptexture = (mstudiotexture_t *)(((byte *)phdr) + phdr->textureindex);
			// now we can replace offsets with real pointers
			ptexture[i].pixels = (byte *)phdr + ptexture[i].index;
		}
#endif
	}

	VectorCopy( origin, m->origin );
	VectorCopy( angles, m->angles );
	VectorCopy( scale, m->scale );

	m->trace_mode = trace_mode;

	m->body = body;
	m->skin = skin;

	m->mesh.StudioConstructMesh( m );

	num_models++;
}

// =====================================================================================
//  LoadStudioModels
// =====================================================================================
void LoadStudioModels( void )
{
	memset( models, 0, sizeof( models ));
	num_models = 0;

	if( !g_studioshadow ) return;

	for( int i = 0; i < g_numentities; i++ )
	{
		const char *name, *model;
		vec3_t origin, angles;

		entity_t* e = &g_entities[i];
		name = ValueForKey( e, "classname" );

		if( !Q_stricmp( name, "env_static" ))
		{
			int spawnflags = IntForKey( e, "spawnflags" );
			if( spawnflags & 4 ) continue; // shadow disabled
		
			model = ValueForKey( e, "model" );

			if( !model || !*model )
			{
				Developer( DEVELOPER_LEVEL_WARNING, "env_static has empty model field\n" );
				continue;
			}
		}
		else if( IntForKey( e, "zhlt_studioshadow" ))
		{
			model = ValueForKey( e, "model" );

			if( !model || !*model )
				continue;
		}
		else
		{
			continue;
		}

		GetVectorForKey( e, "origin", origin );
		GetVectorForKey( e, "angles", angles );

		angles[0] = -angles[0]; // Stupid quake bug workaround
		int trace_mode = SHADOW_NORMAL;	// default mode

		// make sure what field is present
		if( strcmp( ValueForKey( e, "zhlt_shadowmode" ), "" ))
			trace_mode = IntForKey( e, "zhlt_shadowmode" );

		int body = IntForKey( e, "body" );
		int skin = IntForKey( e, "skin" );

		float scale = FloatForKey( e, "scale" );
		vec3_t xform;

		GetVectorForKey( e, "xform", xform );

		if( VectorCompare( xform, vec3_origin ))
			VectorFill( xform, scale );

		// check xform values
		if( xform[0] < 0.01f ) xform[0] = 1.0f;
		if( xform[1] < 0.01f ) xform[1] = 1.0f;
		if( xform[2] < 0.01f ) xform[2] = 1.0f;
		if( xform[0] > 16.0f ) xform[0] = 16.0f;
		if( xform[1] > 16.0f ) xform[1] = 16.0f;
		if( xform[2] > 16.0f ) xform[2] = 16.0f;

		LoadStudioModel( model, origin, angles, xform, body, skin, trace_mode );
	}
	char line[64];
	int len = snprintf( line, sizeof( line ), "%i opaque studio models", num_models );
	Log("%s\n", line);

	if (num_models > 0)
	{
		for (int i = 0; i < len; i++)
		{
			Log("-");
		}
		Log("\n");
		int indexWidth = 1; //Grows when an index needs 2nd/3rd digit

		for (int m = (num_models > 0 ? num_models - 1 : 0); m >= 10; m /= 10)
			indexWidth++;

		for (int i = 0; i < num_models; i++)
		{
			mmesh_t* mesh = models[i].mesh.GetMesh();
			const char* modestr =
				models[i].trace_mode == SHADOW_FAST ? "fast" :
				models[i].trace_mode == SHADOW_NORMAL ? "normal" :
				models[i].trace_mode == SHADOW_SLOW ? "slow" : "unknown";

			Log("  [%*d] %-16s tris=%-6u mode=%-6s bbox=(%.0f %.0f %.0f)-(%.0f %.0f %.0f)\n",
				indexWidth, i, models[i].shortname, mesh ? mesh->numfacets : 0u, modestr,
				mesh ? mesh->mins[0] : 0.0f, mesh ? mesh->mins[1] : 0.0f, mesh ? mesh->mins[2] : 0.0f,
				mesh ? mesh->maxs[0] : 0.0f, mesh ? mesh->maxs[1] : 0.0f, mesh ? mesh->maxs[2] : 0.0f);
		}
		for (int i = 0; i < len; i++)
		{
			Log("-");
		}
		Log("\n");
	}
}

void FreeStudioModels( void )
{
	for( int i = 0; i < num_models; i++ )
	{
		model_t *m = &models[i];

		// first, delete the mesh
		m->mesh.FreeMesh();

		// unload the model
		Free( m->extradata );
	}

	memset( models, 0, sizeof( models ));
	num_models = 0;
}

void MoveBounds( const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, vec3_t outmins, vec3_t outmaxs )
{
	for( int i = 0; i < 3; i++ )
	{
		if( end[i] > start[i] )
		{
			outmins[i] = start[i] + mins[i] - 1.0f;
			outmaxs[i] = end[i] + maxs[i] + 1.0f;
		}
		else
		{
			outmins[i] = end[i] + mins[i] - 1.0f;
			outmaxs[i] = start[i] + maxs[i] + 1.0f;
		}
	}
}

bool TestSegmentAgainstStudioList( const vec_t* p1, const vec_t* p2, studiotracectl_t* ctl)
{
	if( !num_models ) return false; // easy out

	vec3_t	trace_mins, trace_maxs;

	MoveBounds( p1, vec3_origin, vec3_origin, p2, trace_mins, trace_maxs );

	const int *cands = NULL;
	int ncands = num_models;

	if( ctl && ctl->candidates )
	{
		cands = ctl->candidates;
		ncands = ctl->ncandidates;
	}

	for (int i = 0; i < ncands; i++)
	{
		model_t *m = &models[cands ? cands[i] : i];
		mmesh_t *pMesh = m->mesh.GetMesh();
		areanode_t *pHeadNode = m->mesh.GetHeadNode();

		if (ctl) ctl->models_tested++;
		if( !pMesh || !m->mesh.Intersect( trace_mins, trace_maxs ))
			continue; // bad model or not intersect with trace
		if (ctl) ctl->models_traced++;
		TraceMesh	trm;	// a name like Doom3 :-)

		trm.SetTraceModExtradata( m->extradata );
		trm.SetTraceMesh( pMesh, pHeadNode );

		if( ctl && ctl->mode_override >= 0 ) trm.SetModeOverride( ctl->mode_override ); //seedee: AO-only override; direct lighting passes ctl == NULL and keeps the model's zhlt_shadowmode
		double t0 = 0.0;

		if( ctl && ctl->timing ) t0 = I_FloatTime();
		trm.SetupTrace( p1, vec3_origin, vec3_origin, p2 );

		if( trm.DoTrace())
		{
			if( ctl )
			{
				if( ctl->timing ) ctl->time_trace += I_FloatTime() - t0;
				ctl->hits++;
				ctl->hit_model = cands ? cands[i] : i;
			}
			return true; // we hit studio model
		}
		if( ctl && ctl->timing ) ctl->time_trace += I_FloatTime() - t0;
	}

	return false;
}

// =====================================================================================
//  BuildStudioCandidates														//seedee
//      Indices of studiomodels whose AABB intersects 'mins/maxs' inflated by 'pad'
//		Rays travel at most 'g_ao_scale' so models outside the padded box can't be hit
// =====================================================================================
int BuildStudioCandidates( const vec3_t mins, const vec3_t maxs, float pad, int *out_indices, int max_out )
{
	int n = 0;

	for( int i = 0; i < num_models && n < max_out; i++ )
	{
		mmesh_t *mesh = models[i].mesh.GetMesh();

		if( !mesh ) continue;
		if( mesh->mins[0] > maxs[0] + pad || mesh->maxs[0] < mins[0] - pad ) continue;
		if( mesh->mins[1] > maxs[1] + pad || mesh->maxs[1] < mins[1] - pad ) continue;
		if( mesh->mins[2] > maxs[2] + pad || mesh->maxs[2] < mins[2] - pad ) continue;
		out_indices[n++] = i;
	}
	return n;
}

// =====================================================================================
//  StudioModelShortname														//seedee
// =====================================================================================
const char *StudioModelShortname( int index )
{
	if( index < 0 || index >= num_models ) return "???";
	return models[index].shortname;
}