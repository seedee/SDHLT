//#pragma warning(disable: 4018) // '<' : signed/unsigned mismatch

#include "csg.h"

int             g_nummapbrushes;
brush_t         g_mapbrushes[MAX_MAP_BRUSHES];

int             g_numbrushsides;
side_t          g_brushsides[MAX_MAP_SIDES];

int             g_nMapFileVersion;

static const vec3_t   s_baseaxis[18] = {
    {0, 0, 1}, {1, 0, 0}, {0, -1, 0},                      // floor
    {0, 0, -1}, {1, 0, 0}, {0, -1, 0},                     // ceiling
    {1, 0, 0}, {0, 1, 0}, {0, 0, -1},                      // west wall
    {-1, 0, 0}, {0, 1, 0}, {0, 0, -1},                     // east wall
    {0, 1, 0}, {1, 0, 0}, {0, 0, -1},                      // south wall
    {0, -1, 0}, {1, 0, 0}, {0, 0, -1},                     // north wall
};

int				g_numparsedentities;
int				g_numparsedbrushes;

brush_t *CopyCurrentBrush (entity_t *entity, const brush_t *brush)
{
	if (entity->firstbrush + entity->numbrushes != g_nummapbrushes)
	{
		Error ("CopyCurrentBrush: internal error.");
	}
	brush_t *newb = &g_mapbrushes[g_nummapbrushes];
	g_nummapbrushes++;
	hlassume (g_nummapbrushes <= MAX_MAP_BRUSHES, assume_MAX_MAP_BRUSHES);
	memcpy (newb, brush, sizeof (brush_t));
	newb->firstside = g_numbrushsides;
	g_numbrushsides += brush->numsides;
	hlassume (g_numbrushsides <= MAX_MAP_SIDES, assume_MAX_MAP_SIDES);
	memcpy (&g_brushsides[newb->firstside], &g_brushsides[brush->firstside], brush->numsides * sizeof (side_t));
	newb->entitynum = entity - g_entities;
	newb->brushnum = entity->numbrushes;
	entity->numbrushes++;
	for (int h = 0; h < NUM_HULLS; h++)
	{
		if (brush->hullshapes[h] != NULL)
		{
			newb->hullshapes[h] = _strdup (brush->hullshapes[h]);
		}
		else
		{
			newb->hullshapes[h] = NULL;
		}
	}
	return newb;
}
void DeleteCurrentEntity (entity_t *entity)
{
	if (entity != &g_entities[g_numentities - 1])
	{
		Error ("DeleteCurrentEntity: internal error.");
	}
	if (entity->firstbrush + entity->numbrushes != g_nummapbrushes)
	{
		Error ("DeleteCurrentEntity: internal error.");
	}
	for (int i = entity->numbrushes - 1; i >= 0; i--)
	{
		brush_t *b = &g_mapbrushes[entity->firstbrush + i];
		if (b->firstside + b->numsides != g_numbrushsides)
		{
			Error ("DeleteCurrentEntity: internal error. (Entity %i, Brush %i)",
				b->originalentitynum, b->originalbrushnum
				);
		}
		memset (&g_brushsides[b->firstside], 0, b->numsides * sizeof (side_t));
		g_numbrushsides -= b->numsides;
		for (int h = 0; h < NUM_HULLS; h++)
		{
			if (b->hullshapes[h])
			{
				free (b->hullshapes[h]);
			}
		}
	}
	memset (&g_mapbrushes[entity->firstbrush], 0, entity->numbrushes * sizeof (brush_t));
	g_nummapbrushes -= entity->numbrushes;
	while (entity->epairs)
	{
		DeleteKey (entity, entity->epairs->key);
	}
	memset (entity, 0, sizeof(entity_t));
	g_numentities--;
}
// =====================================================================================
//  TextureAxisFromPlane
// =====================================================================================
void            TextureAxisFromPlane(const plane_t* const pln, vec3_t xv, vec3_t yv)
{
    int             bestaxis;
    vec_t           dot, best;
    int             i;

    best = 0;
    bestaxis = 0;

    for (i = 0; i < 6; i++)
    {
        dot = DotProduct(pln->normal, s_baseaxis[i * 3]);
        if (dot > best)
        {
            best = dot;
            bestaxis = i;
        }
    }

    VectorCopy(s_baseaxis[bestaxis * 3 + 1], xv);
    VectorCopy(s_baseaxis[bestaxis * 3 + 2], yv);
}

#define ScaleCorrection	(1.0/128.0)


// =====================================================================================
//  CheckForInvisible
//      see if a brush is part of an invisible entity (KGP)
// =====================================================================================
static bool CheckForInvisible(entity_t* mapent)
{
	using namespace std;

	string keyval(ValueForKey(mapent,"classname"));
	if(g_invisible_items.count(keyval))
	{ return true; }

	keyval.assign(ValueForKey(mapent,"targetname"));
	if(g_invisible_items.count(keyval))
	{ return true; }

	keyval.assign(ValueForKey(mapent,"zhlt_invisible"));
	if(!keyval.empty() && strcmp(keyval.c_str(),"0"))
	{ return true; }

	return false;
}
// =====================================================================================
//  ParseBrush
//      parse a brush from script
// =====================================================================================
static void ParseBrush(entity_t* mapent)
{
    brush_t*        b; //Current brush
    int             i, j; //Loop counters
    side_t*         side; //Current side of the brush
    contents_t      contents; //Contents type of the brush
    bool            ok;
	bool nullify = CheckForInvisible(mapent); //If the current entity is part of an invis entity
    hlassume(g_nummapbrushes < MAX_MAP_BRUSHES, assume_MAX_MAP_BRUSHES);

    b = &g_mapbrushes[g_nummapbrushes]; //Get next brush slot
    g_nummapbrushes++; //Increment the global brush counter, we are adding a new brush
    b->firstside = g_numbrushsides; //Set the first side of the brush to current global side count20
	b->originalentitynum = g_numparsedentities; //Record original entity number brush belongs to
	b->originalbrushnum = g_numparsedbrushes; //Record original brush number
    b->entitynum = g_numentities - 1; //Set brush entity number to last created entity
    b->brushnum = g_nummapbrushes - mapent->firstbrush - 1; //Calculate the brush number within the current entity.
    b->noclip = 0; //Initialize false for now

	if (IntForKey(mapent, "zhlt_noclip")) //If zhlt_noclip
	{
		b->noclip = 1;
	}
	b->cliphull = 0;
	b->bevel = false;
	{ //Validate func_detail values
		b->detaillevel = IntForKey (mapent, "zhlt_detaillevel");
		b->chopdown = IntForKey (mapent, "zhlt_chopdown");
		b->chopup = IntForKey (mapent, "zhlt_chopup");
		b->clipnodedetaillevel = IntForKey (mapent, "zhlt_clipnodedetaillevel");
		b->coplanarpriority = IntForKey (mapent, "zhlt_coplanarpriority");
		bool wrong = false;

		if (b->detaillevel < 0)
		{
			wrong = true;
			b->detaillevel = 0;
		}
		if (b->chopdown < 0)
		{
			wrong = true;
			b->chopdown = 0;
		}
		if (b->chopup < 0)
		{
			wrong = true;
			b->chopup = 0;
		}
		if (b->clipnodedetaillevel < 0)
		{
			wrong = true;
			b->clipnodedetaillevel = 0;
		}
		if (wrong)
		{
			Warning ("Entity %i, Brush %i: incorrect settings for detail brush.",
					b->originalentitynum, b->originalbrushnum
					);
		}
	}
	for (int h = 0; h < NUM_HULLS; h++) //Loop through all hulls
	{
		char key[16]; //Key name for the hull shape.
		const char *value; //Value for the key
		sprintf(key, "zhlt_hull%d", h); //Format key name to include the hull number, used to look up hull shape data in entity properties
		value = ValueForKey(mapent, key);

		if (*value) //If we have a value associated with the key from the entity properties copy the value to brush's hull shape for this hull
		{
			b->hullshapes[h] = _strdup(value);
		}
		else //Set brush hull shape for this hull to NULL
		{
			b->hullshapes[h] = NULL;
		}
	}
    mapent->numbrushes++;
	ok = GetToken(true);

    while (ok) //Loop through brush sides
    {
        if (!strcmp(g_token, "}")) //If we have reached the end of the brush
        {
            break;
        }

        hlassume(g_numbrushsides < MAX_MAP_SIDES, assume_MAX_MAP_SIDES);
        side = &g_brushsides[g_numbrushsides]; //Get next brush side from global array
        g_numbrushsides++; //Global brush side counter
        b->numsides++; //Number of sides for the current brush
		side->bevel = false;
        // read the three point plane definition

        for (i = 0; i < 3; i++) //Read 3 point plane definition for brush side
        {
            if (i != 0) //If not the first point get next token
            {
                GetToken(true);
            }
            if (strcmp(g_token, "(")) //Token must be '('
            {
                Error("Parsing Entity %i, Brush %i, Side %i : Expecting '(' got '%s'",
					b->originalentitynum, b->originalbrushnum, 
					  b->numsides, g_token);
            }
            for (j = 0; j < 3; j++) //Get three coords for the point
            {
                GetToken(false); //Get next token on same line
                side->planepts[i][j] = atof(g_token); //Convert token to float and store in planepts
            }
            GetToken(false);

            if (strcmp(g_token, ")"))
            {
                Error("Parsing	Entity %i, Brush %i, Side %i : Expecting ')' got '%s'",
					b->originalentitynum, b->originalbrushnum, 
					  b->numsides, g_token);
            }
        }

        // read the     texturedef
        GetToken(false);
        _strupr(g_token);
		{ //Check for tool textures on the brush
			if (!strncasecmp (g_token, "NOCLIP", 6) || !strncasecmp (g_token, "NULLNOCLIP", 10))
			{
				strcpy (g_token, "NULL");
				b->noclip = true;
			}
			if (!strncasecmp (g_token, "BEVELBRUSH", 10))
			{
				strcpy (g_token, "NULL");
				b->bevel = true;
			}
			if (!strncasecmp (g_token, "BEVEL", 5))
			{
				strcpy (g_token, "NULL");
				side->bevel = true;
			}
			if (!strncasecmp(g_token, "BEVELHINT", 9))
			{
				side->bevel = true;
			}
			if (!strncasecmp (g_token, "CLIP", 4))
			{
				b->cliphull |= (1 << NUM_HULLS); // arbitrary nonexistent hull
				int h;
				if (!strncasecmp (g_token, "CLIPHULL", 8) && (h = g_token[8] - '0', 0 < h && h < NUM_HULLS))
				{
					b->cliphull |= (1 << h); // hull h
				}
				if (!strncasecmp (g_token, "CLIPBEVEL", 9))
				{
					side->bevel = true;
				}
				if (!strncasecmp (g_token, "CLIPBEVELBRUSH", 14))
				{
					b->bevel = true;
				}
				strcpy (g_token, "SKIP");
			}
		}
        safe_strncpy(side->td.name, g_token, sizeof(side->td.name));

        if (g_nMapFileVersion < 220)                       // Worldcraft 2.1-, Radiant
        {
            GetToken(false);
            side->td.vects.valve.shift[0] = atof(g_token);
            GetToken(false);
            side->td.vects.valve.shift[1] = atof(g_token);
            GetToken(false);
            side->td.vects.valve.rotate = atof(g_token);
            GetToken(false);
            side->td.vects.valve.scale[0] = atof(g_token);
            GetToken(false);
            side->td.vects.valve.scale[1] = atof(g_token);
        }
        else                                               // Worldcraft 2.2+
        {
            // texture U axis
            GetToken(false);
            if (strcmp(g_token, "["))
            {
                hlassume(false, assume_MISSING_BRACKET_IN_TEXTUREDEF);
            }

            GetToken(false);
            side->td.vects.valve.UAxis[0] = atof(g_token);
            GetToken(false);
            side->td.vects.valve.UAxis[1] = atof(g_token);
            GetToken(false);
            side->td.vects.valve.UAxis[2] = atof(g_token);
            GetToken(false);
            side->td.vects.valve.shift[0] = atof(g_token);

            GetToken(false);
            if (strcmp(g_token, "]"))
            {
                Error("missing ']' in texturedef (U)");
            }

            // texture V axis
            GetToken(false);
            if (strcmp(g_token, "["))
            {
                Error("missing '[' in texturedef (V)");
            }

            GetToken(false);
            side->td.vects.valve.VAxis[0] = atof(g_token);
            GetToken(false);
            side->td.vects.valve.VAxis[1] = atof(g_token);
            GetToken(false);
            side->td.vects.valve.VAxis[2] = atof(g_token);
            GetToken(false);
            side->td.vects.valve.shift[1] = atof(g_token);

            GetToken(false);
            if (strcmp(g_token, "]"))
            {
                Error("missing ']' in texturedef (V)");
            }

            // Texture rotation is implicit in U/V axes.
            GetToken(false);
            side->td.vects.valve.rotate = 0;

            // texure scale
            GetToken(false);
            side->td.vects.valve.scale[0] = atof(g_token);
            GetToken(false);
            side->td.vects.valve.scale[1] = atof(g_token);
        }

        ok = GetToken(true);                               // Done with line, this reads the first item from the next line

    };
	if (b->cliphull != 0) // has CLIP* texture
	{
		unsigned int mask_anyhull = 0;
		for (int h = 1; h < NUM_HULLS; h++)
		{
			mask_anyhull |= (1 << h);
		}
		if ((b->cliphull & mask_anyhull) == 0) // no CLIPHULL1 or CLIPHULL2 or CLIPHULL3 texture
		{
			b->cliphull |= mask_anyhull; // CLIP all hulls
		}
	}

    b->contents = contents = CheckBrushContents(b);
	for (j = 0; j < b->numsides; j++)
	{
		side = &g_brushsides[b->firstside + j];
		if (nullify
			&& strncasecmp(side->td.name, "BEVEL", 5)
			&& strncasecmp(side->td.name, "ORIGIN", 6)
			&& strncasecmp(side->td.name, "HINT", 4)
			&& strncasecmp(side->td.name, "SKIP", 4)
			&& strncasecmp(side->td.name, "SOLIDHINT", 9)
			&& strncasecmp(side->td.name, "BEVELHINT", 9)
			&& strncasecmp(side->td.name, "SPLITFACE", 9)
			&& strncasecmp(side->td.name, "BOUNDINGBOX", 11)
			&& strncasecmp(side->td.name, "CONTENT", 7)
			&& strncasecmp(side->td.name, "SKY", 3)
			)
		{
			safe_strncpy(side->td.name,"NULL",sizeof(side->td.name));
		}
	}
	for (j = 0; j < b->numsides; j++)
	{
		// change to SKIP now that we have set brush content.
		side = &g_brushsides[b->firstside + j];
		if (!strncasecmp (side->td.name, "SPLITFACE", 9))
		{
			strcpy (side->td.name, "SKIP");
		}
	}
	for (j = 0; j < b->numsides; j++)
	{
		side = &g_brushsides[b->firstside + j];
		if (!strncasecmp (side->td.name, "CONTENT", 7))
		{
			strcpy (side->td.name, "NULL");
		}
	}
	if (g_nullifytrigger)
	{
		for (j = 0; j < b->numsides; j++)
		{
			side = &g_brushsides[b->firstside + j];
			if (!strncasecmp (side->td.name, "AAATRIGGER", 10))
			{
				strcpy (side->td.name, "NULL");
			}
		}
	}

    //
    // origin brushes are removed, but they set
    // the rotation origin for the rest of the brushes
    // in the entity
    //

    if (contents == CONTENTS_ORIGIN)
    {
		if (*ValueForKey (mapent, "origin"))
		{
			Error ("Entity %i, Brush %i: Only one ORIGIN brush allowed.",
					b->originalentitynum, b->originalbrushnum
					);
		}
        char            string[MAXTOKEN];
        vec3_t          origin;

        b->contents = CONTENTS_SOLID;
        CreateBrush(mapent->firstbrush + b->brushnum);     // to get sizes
        b->contents = contents;

        for (i = 0; i < NUM_HULLS; i++)
        {
            b->hulls[i].faces = NULL;
        }

        if (b->entitynum != 0)  // Ignore for WORLD (code elsewhere enforces no ORIGIN in world message)
        {
            VectorAdd(b->hulls[0].bounds.m_Mins, b->hulls[0].bounds.m_Maxs, origin);
            VectorScale(origin, 0.5, origin);
    
            safe_snprintf(string, MAXTOKEN, "%i %i %i", (int)origin[0], (int)origin[1], (int)origin[2]);
            SetKeyValue(&g_entities[b->entitynum], "origin", string);
        }
    }
	if (*ValueForKey (&g_entities[b->entitynum], "zhlt_usemodel"))
	{
		memset (&g_brushsides[b->firstside], 0, b->numsides * sizeof (side_t));
		g_numbrushsides -= b->numsides;
		for (int h = 0; h < NUM_HULLS; h++)
		{
			if (b->hullshapes[h])
			{
				free (b->hullshapes[h]);
			}
		}
		memset (b, 0, sizeof (brush_t));
		g_nummapbrushes--;
		mapent->numbrushes--;
		return;
	}
	if (!strcmp (ValueForKey (&g_entities[b->entitynum], "classname"), "info_hullshape"))
	{
		// all brushes should be erased, but not now.
		return;
	}
    if (contents == CONTENTS_BOUNDINGBOX)
    {
		if (*ValueForKey (mapent, "zhlt_minsmaxs"))
		{
			Error ("Entity %i, Brush %i: Only one BoundingBox brush allowed.",
					b->originalentitynum, b->originalbrushnum
					);
		}
        char            string[MAXTOKEN];
        vec3_t          mins, maxs;
		char			*origin = NULL;
		if (*ValueForKey (mapent, "origin"))
		{
			origin = strdup (ValueForKey (mapent, "origin"));
			SetKeyValue (mapent, "origin", "");
		}

        b->contents = CONTENTS_SOLID;
        CreateBrush(mapent->firstbrush + b->brushnum);     // to get sizes
        b->contents = contents;

        for (i = 0; i < NUM_HULLS; i++)
        {
            b->hulls[i].faces = NULL;
        }

        if (b->entitynum != 0)  // Ignore for WORLD (code elsewhere enforces no ORIGIN in world message)
        {
            VectorCopy(b->hulls[0].bounds.m_Mins, mins);
            VectorCopy(b->hulls[0].bounds.m_Maxs, maxs);
    
            safe_snprintf(string, MAXTOKEN, "%.0f %.0f %.0f %.0f %.0f %.0f", mins[0], mins[1], mins[2], maxs[0], maxs[1], maxs[2]);
            SetKeyValue(&g_entities[b->entitynum], "zhlt_minsmaxs", string);
        }

		if (origin)
		{
			SetKeyValue (mapent, "origin", origin);
			free (origin);
		}
    }
	if (g_skyclip && b->contents == CONTENTS_SKY && !b->noclip)
	{
		brush_t *newb = CopyCurrentBrush (mapent, b);
		newb->contents = CONTENTS_SOLID;
		newb->cliphull = ~0;
		for (j = 0; j < newb->numsides; j++)
		{
			side = &g_brushsides[newb->firstside + j];
			strcpy (side->td.name, "NULL");
		}
	}
	if (b->cliphull != 0 && b->contents == CONTENTS_TOEMPTY)
	{
		// check for mix of CLIP and normal texture
		bool mixed = false;
		for (j = 0; j < b->numsides; j++)
		{
			side = &g_brushsides[b->firstside + j];
			if (!strncasecmp (side->td.name, "NULL", 4))
			{ // this is not supposed to be a HINT brush, so remove all invisible faces from hull 0.
				strcpy (side->td.name, "SKIP");
			}
			if (strncasecmp (side->td.name, "SKIP", 4))
				mixed = true;
		}
		if (mixed)
		{
			brush_t *newb = CopyCurrentBrush (mapent, b);
			newb->cliphull = 0;
		}
		b->contents = CONTENTS_SOLID;
		for (j = 0; j < b->numsides; j++)
		{
			side = &g_brushsides[b->firstside + j];
			strcpy (side->td.name, "NULL");
		}
	}

}


// =====================================================================================
//  ParseMapEntity
//      parse an entity from script
// =====================================================================================
bool            ParseMapEntity()
{
    bool            all_clip = true;
    int             this_entity;
    entity_t*       mapent;
    epair_t*        e;

	g_numparsedbrushes = 0;
    if (!GetToken(true))
    {
        return false;
    }

    this_entity = g_numentities;

    if (strcmp(g_token, "{"))
    {
        Error("Parsing Entity %i, expected '{' got '%s'", 
			g_numparsedentities, 
			g_token);
    }

    hlassume(g_numentities < MAX_MAP_ENTITIES, assume_MAX_MAP_ENTITIES);
    g_numentities++;

    mapent = &g_entities[this_entity];
    mapent->firstbrush = g_nummapbrushes;
    mapent->numbrushes = 0;

    while (1)
    {
        if (!GetToken(true))
            Error("ParseEntity: EOF without closing brace");

        if (!strcmp(g_token, "}"))  // end of our context
            break;

        if (!strcmp(g_token, "{"))  // must be a brush
        {
			ParseBrush (mapent);
			g_numparsedbrushes++;

        }
        else                        // else assume an epair
        {
            e = ParseEpair();
			if (mapent->numbrushes > 0) Warning ("Error: ParseEntity: Keyvalue comes after brushes."); //--vluzacn

            if (!strcmp(e->key, "mapversion"))
            {
                g_nMapFileVersion = atoi(e->value);
            }

			SetKeyValue (mapent, e->key, e->value);
			Free (e->key);
			Free (e->value);
			Free (e);
        }
    }
	{
		int i;
		for (i = 0; i < mapent->numbrushes; i++)
		{
			brush_t *brush = &g_mapbrushes[mapent->firstbrush + i];
			if (
				brush->cliphull == 0
				&& brush->contents != CONTENTS_ORIGIN
				&& brush->contents != CONTENTS_BOUNDINGBOX
				)
			{
				all_clip = false;
			}
		}
	}
	if (*ValueForKey (mapent, "zhlt_usemodel"))
	{
		if (!*ValueForKey (mapent, "origin"))
			Warning ("Entity %i: 'zhlt_usemodel' requires the entity to have an origin brush.", 
				g_numparsedentities
				);
		mapent->numbrushes = 0;
	}
	if (strcmp (ValueForKey (mapent, "classname"), "info_hullshape")) // info_hullshape is not affected by '-scale'
	{
		bool ent_move_b = false, ent_scale_b = false, ent_gscale_b = false;
		vec3_t ent_move = {0,0,0}, ent_scale_origin = {0,0,0};
		vec_t ent_scale = 1, ent_gscale = 1;

		if (g_scalesize > 0)
		{
			ent_gscale_b = true;
			ent_gscale = g_scalesize;
		}
		double v[4] = {0,0,0,0};
		if (*ValueForKey (mapent, "zhlt_transform"))
		{
			switch
				(sscanf(ValueForKey (mapent, "zhlt_transform"), "%lf %lf %lf %lf", v, v+1, v+2, v+3))
			{
			case 1:
				ent_scale_b = true;
				ent_scale = v[0];
				break;
			case 3:
				ent_move_b = true;
				VectorCopy (v, ent_move);
				break;
			case 4:
				ent_scale_b = true;
				ent_scale = v[0];
				ent_move_b = true;
				VectorCopy (v+1, ent_move);
				break;
			default:
				Warning ("bad value '%s' for key 'zhlt_transform'", ValueForKey (mapent, "zhlt_transform"));
			}
			DeleteKey (mapent, "zhlt_transform");
		}
		GetVectorForKey (mapent, "origin", ent_scale_origin);

		if (ent_move_b || ent_scale_b || ent_gscale_b)
		{
			if (g_nMapFileVersion < 220)
			{
				Warning ("hlcsg scaling hack is not supported in Worldcraft 2.1");
			}
			else
			{
				int ibrush, iside, ipoint;
				brush_t *brush;
				side_t *side;
				vec_t *point;
				for (ibrush = 0, brush = g_mapbrushes + mapent->firstbrush; ibrush < mapent->numbrushes; ++ibrush, ++brush)
				{
					for (iside = 0, side = g_brushsides + brush->firstside; iside < brush->numsides; ++iside, ++side)
					{
						for (ipoint = 0; ipoint < 3; ++ipoint)
						{
							point = side->planepts[ipoint];
							if (ent_scale_b)
							{
								VectorSubtract (point, ent_scale_origin, point);
								VectorScale (point, ent_scale, point);
								VectorAdd (point, ent_scale_origin, point);
							}
							if (ent_move_b)
							{
								VectorAdd (point, ent_move, point);

							}
							if (ent_gscale_b)
							{
								VectorScale (point, ent_gscale, point);
							}
						}
						// note that  tex->vecs = td.vects.valve.Axis / td.vects.valve.scale
						//            tex->vecs[3] = vects.valve.shift + Dot(origin, tex->vecs)
						//      and   texcoordinate = Dot(worldposition, tex->vecs) + tex->vecs[3]
						bool zeroscale = false;
						if (!side->td.vects.valve.scale[0])
						{
							side->td.vects.valve.scale[0] = 1;
						}
						if (!side->td.vects.valve.scale[1])
						{
							side->td.vects.valve.scale[1] = 1;
						}
						if (ent_scale_b)
						{
							vec_t coord[2];
							if (fabs (side->td.vects.valve.scale[0]) > NORMAL_EPSILON)
							{
								coord[0] = DotProduct (ent_scale_origin, side->td.vects.valve.UAxis) / side->td.vects.valve.scale[0] + side->td.vects.valve.shift[0];
								side->td.vects.valve.scale[0] *= ent_scale;
								if (fabs (side->td.vects.valve.scale[0]) > NORMAL_EPSILON)
								{
									side->td.vects.valve.shift[0] = coord[0] - DotProduct (ent_scale_origin, side->td.vects.valve.UAxis) / side->td.vects.valve.scale[0];
								}
								else
								{
									zeroscale = true;
								}
							}
							else
							{
								zeroscale = true;
							}
							if (fabs (side->td.vects.valve.scale[1]) > NORMAL_EPSILON)
							{
								coord[1] = DotProduct (ent_scale_origin, side->td.vects.valve.VAxis) / side->td.vects.valve.scale[1] + side->td.vects.valve.shift[1];
								side->td.vects.valve.scale[1] *= ent_scale;
								if (fabs (side->td.vects.valve.scale[1]) > NORMAL_EPSILON)
								{
									side->td.vects.valve.shift[1] = coord[1] - DotProduct (ent_scale_origin, side->td.vects.valve.VAxis) / side->td.vects.valve.scale[1];
								}
								else
								{
									zeroscale = true;
								}
							}
							else
							{
								zeroscale = true;
							}
						}
						if (ent_move_b)
						{
							if (fabs (side->td.vects.valve.scale[0]) > NORMAL_EPSILON)
							{
								side->td.vects.valve.shift[0] -= DotProduct (ent_move, side->td.vects.valve.UAxis) / side->td.vects.valve.scale[0];
							}
							else
							{
								zeroscale = true;
							}
							if (fabs (side->td.vects.valve.scale[1]) > NORMAL_EPSILON)
							{
								side->td.vects.valve.shift[1] -= DotProduct (ent_move, side->td.vects.valve.VAxis) / side->td.vects.valve.scale[1];
							}
							else
							{
								zeroscale = true;
							}
						}
						if (ent_gscale_b)
						{
							side->td.vects.valve.scale[0] *= ent_gscale;
							side->td.vects.valve.scale[1] *= ent_gscale;
						}
						if (zeroscale)
						{
							Error ("Entity %i, Brush %i: invalid texture scale.\n", 
								brush->originalentitynum, brush->originalbrushnum
								);
						}
					}
				}
				if (ent_gscale_b)
				{
					if (*ValueForKey (mapent, "origin"))
					{
						double v[3];
						int origin[3];
						char string[MAXTOKEN];
						int i;
						GetVectorForKey (mapent, "origin", v);
						VectorScale (v, ent_gscale, v);
						for (i=0; i<3; ++i)
							origin[i] = (int)(v[i]>=0? v[i]+0.5: v[i]-0.5);
						safe_snprintf(string, MAXTOKEN, "%d %d %d", origin[0], origin[1], origin[2]);
						SetKeyValue (mapent, "origin", string);
					}
				}
				{
					double b[2][3];
					if (sscanf (ValueForKey (mapent, "zhlt_minsmaxs"), "%lf %lf %lf %lf %lf %lf", &b[0][0], &b[0][1], &b[0][2], &b[1][0], &b[1][1], &b[1][2]) == 6)
					{
						for (int i = 0; i < 2; i++)
						{
							vec_t *point = b[i];
							if (ent_scale_b)
							{
								VectorSubtract (point, ent_scale_origin, point);
								VectorScale (point, ent_scale, point);
								VectorAdd (point, ent_scale_origin, point);
							}
							if (ent_move_b)
							{
								VectorAdd (point, ent_move, point);

							}
							if (ent_gscale_b)
							{
								VectorScale (point, ent_gscale, point);
							}
						}
						char string[MAXTOKEN];
						safe_snprintf(string, MAXTOKEN, "%.0f %.0f %.0f %.0f %.0f %.0f", b[0][0], b[0][1], b[0][2], b[1][0], b[1][1], b[1][2]);
						SetKeyValue (mapent, "zhlt_minsmaxs", string);
					}
				}
			}
		}
	}



    CheckFatal();
	if (this_entity == 0)
	{
		// Let the map tell which version of the compiler it comes from, to help tracing compiler bugs.
		char versionstring [128];
		sprintf (versionstring, "ZHLT " ZHLT_VERSIONSTRING " " HACK_VERSIONSTRING " (%s)", __DATE__);
		SetKeyValue (mapent, "compiler", versionstring);
	}
    


    if (!strcmp(ValueForKey(mapent, "classname"), "info_compile_parameters"))
    {
        GetParamsFromEnt(mapent);
    }



    GetVectorForKey(mapent, "origin", mapent->origin);

	if (!strcmp("func_group", ValueForKey(mapent, "classname"))
		|| !strcmp("func_detail", ValueForKey (mapent, "classname"))
		)
    {
        // this is pretty gross, because the brushes are expected to be
        // in linear order for each entity
        brush_t*        temp;
        int             newbrushes;
        int             worldbrushes;
        int             i;

        newbrushes = mapent->numbrushes;
        worldbrushes = g_entities[0].numbrushes;

        temp = (brush_t*)Alloc(newbrushes * sizeof(brush_t));
        memcpy(temp, g_mapbrushes + mapent->firstbrush, newbrushes * sizeof(brush_t));

        for (i = 0; i < newbrushes; i++)
        {
            temp[i].entitynum = 0;
			temp[i].brushnum += worldbrushes;
        }

        // make space to move the brushes (overlapped copy)
        memmove(g_mapbrushes + worldbrushes + newbrushes,
                g_mapbrushes + worldbrushes, sizeof(brush_t) * (g_nummapbrushes - worldbrushes - newbrushes));

        // copy the new brushes down
        memcpy(g_mapbrushes + worldbrushes, temp, sizeof(brush_t) * newbrushes);

        // fix up indexes
        g_numentities--;
        g_entities[0].numbrushes += newbrushes;
        for (i = 1; i < g_numentities; i++)
        {
            g_entities[i].firstbrush += newbrushes;
        }
        memset(mapent, 0, sizeof(*mapent));
        Free(temp);
		return true;
    }

	if (!strcmp (ValueForKey (mapent, "classname"), "info_hullshape"))
	{
		bool disabled;
		const char *id;
		int defaulthulls;
		disabled = IntForKey (mapent, "disabled");
		id = ValueForKey (mapent, "targetname");
		defaulthulls = IntForKey (mapent, "defaulthulls");
		CreateHullShape (this_entity, disabled, id, defaulthulls);
		DeleteCurrentEntity (mapent);
		return true;
	}
	if (fabs (mapent->origin[0]) > ENGINE_ENTITY_RANGE + ON_EPSILON ||
		fabs (mapent->origin[1]) > ENGINE_ENTITY_RANGE + ON_EPSILON ||
		fabs (mapent->origin[2]) > ENGINE_ENTITY_RANGE + ON_EPSILON )
	{
		const char *classname = ValueForKey (mapent, "classname");
		if (strncmp (classname, "light", 5))
		{
			Warning ("Entity %i (classname \"%s\"): origin outside +/-%.0f: (%.0f,%.0f,%.0f)", 
				g_numparsedentities, 
				classname, (double)ENGINE_ENTITY_RANGE, mapent->origin[0], mapent->origin[1], mapent->origin[2]);
		}
	}
    return true;
}

// =====================================================================================
//  CountEngineEntities
// =====================================================================================
unsigned int    CountEngineEntities()
{
    unsigned int x;
    unsigned num_engine_entities = 0;
    entity_t*       mapent = g_entities;

    // for each entity in the map
    for (x=0; x<g_numentities; x++, mapent++)
    {
        const char* classname = ValueForKey(mapent, "classname");

        // if its a light_spot or light_env, dont include it as an engine entity!
        if (classname)
        {
            if (   !strncasecmp(classname, "light", 5) 
                || !strncasecmp(classname, "light_spot", 10) 
                || !strncasecmp(classname, "light_environment", 17)
               )
            {
                const char* style = ValueForKey(mapent, "style");
                const char* targetname = ValueForKey(mapent, "targetname");

                // lightspots and lightenviroments dont have a targetname or style
                if (!strlen(targetname) && !atoi(style))
                {
                    continue;
                }
            }
        }

        num_engine_entities++;
    }

    return num_engine_entities;
}

// =====================================================================================
//  LoadMapFile
//      wrapper for LoadScriptFile
//      parse in script entities
// =====================================================================================
const char*     ContentsToString(const contents_t type);

void            LoadMapFile(const char* const filename)
{
    unsigned num_engine_entities;

    LoadScriptFile(filename);

    g_numentities = 0;

	g_numparsedentities = 0;
    while (ParseMapEntity())
    {
		g_numparsedentities++;
    }

    // AJM debug
    /*
    for (int i = 0; i < g_numentities; i++)
    {
        Log("entity: %i - %i brushes - %s\n", i, g_entities[i].numbrushes, ValueForKey(&g_entities[i], "classname"));
    }
    Log("total entities: %i\ntotal brushes: %i\n\n", g_numentities, g_nummapbrushes);

    for (i = g_entities[0].firstbrush; i < g_entities[0].firstbrush + g_entities[0].numbrushes; i++)
    {
        Log("worldspawn brush %i: contents %s\n", i, ContentsToString((contents_t)g_mapbrushes[i].contents)); 
    }
    */

    num_engine_entities = CountEngineEntities();

    hlassume(num_engine_entities < MAX_ENGINE_ENTITIES, assume_MAX_ENGINE_ENTITIES);

    CheckFatal();

    Verbose("Load map:%s\n", filename);
    Verbose("%5i brushes\n", g_nummapbrushes);
    Verbose("%5i map entities \n", g_numentities - num_engine_entities);
    Verbose("%5i engine entities\n", num_engine_entities);

    // AJM: added in 
}
