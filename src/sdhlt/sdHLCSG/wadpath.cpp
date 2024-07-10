// AJM: added this file in

#include "csg.h"

wadpath_t*  g_pWadPaths[MAX_WADPATHS];
int         g_iNumWadPaths = 0;    


// =====================================================================================
//  PushWadPath
//      adds a wadpath into the wadpaths list, without duplicates
// =====================================================================================
void        PushWadPath(const char* const path, bool inuse)
{
    wadpath_t* currentWad;
	hlassume (g_iNumWadPaths < MAX_WADPATHS, assume_MAX_TEXFILES);
	currentWad = (wadpath_t*)malloc(sizeof(wadpath_t));
    safe_strncpy(currentWad->path, path, _MAX_PATH); //Copy path into currentWad->path
	currentWad->usedbymap = inuse;
	currentWad->usedtextures = 0;  //Updated later in autowad procedures
	currentWad->totaltextures = 0; //Updated later to reflect total

	if (g_iNumWadPaths < MAX_WADPATHS) //Fix buffer overrun //seedee
	{
		g_pWadPaths[g_iNumWadPaths] = currentWad;
		g_iNumWadPaths++;
	}
	else
	{
		free(currentWad);
		Error("PushWadPath: too many wadpaths (i%/i%)", g_iNumWadPaths, MAX_WADPATHS);
}

#ifdef _DEBUG
    Log("[dbg] PushWadPath: %i (%s)\n", g_iNumWadPaths, path);
#endif
}


// =====================================================================================
//  FreeWadPaths
// =====================================================================================
void        FreeWadPaths()
{
    int         i;
    wadpath_t*  current;

    for (i = 0; i < g_iNumWadPaths; i++)
    {
        current = g_pWadPaths[i];
        free(current);
    }
}

// =====================================================================================
//  GetUsedWads
//      parse the "wad" keyvalue into wadpath_t structs
// =====================================================================================
void        GetUsedWads()
{
    const char* pszWadPaths;
    char szTmp[_MAX_PATH];
    int i, j;
    pszWadPaths = ValueForKey(&g_entities[0], "wad");

	for (i = 0; ; ) //Loop through wadpaths
	{
		for (j = i; pszWadPaths[j] != '\0'; j++) //Find end of wadpath (semicolon)
		{
			if (pszWadPaths[j] == ';')
			{
				break;
			}
		}
		if (j - i > 0) //If wadpath is not empty
		{
			int length = qmin (j - i, _MAX_PATH - 1); //Get length of wadpath
			memcpy (szTmp, &pszWadPaths[i], length);
			szTmp[length] = '\0'; //Null terminate

			if (g_iNumWadPaths >= MAX_WADPATHS)
			{
				Error ("Too many wad files (%d/%d)\n", g_iNumWadPaths, MAX_WADPATHS);
			}
			PushWadPath (szTmp, true); //Add wadpath to list
		}
		if (pszWadPaths[j] == '\0') //Break if end of wadpaths
		{
			break;
		}
		i = j + 1;
	}
}