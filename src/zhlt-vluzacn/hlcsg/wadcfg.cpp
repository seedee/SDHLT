// AJM: ADDED THIS ENTIRE FILE IN

#include "csg.h"
void LoadWadconfig (const char *filename, const char *configname)
{
	Log ("Loading wad configuration '%s' from '%s' :\n", configname, filename);
	int found = 0;
	int count = 0;
	int size;
	char *buffer;
	size = LoadFile (filename, &buffer);
	ParseFromMemory (buffer, size);
	while (GetToken (true))
	{
		bool skip = true;
		if (!strcasecmp (g_token, configname))
		{
			skip = false;
			found++;
		}
		if (!GetToken (true) || strcasecmp (g_token, "{"))
		{
			Error ("parsing '%s': missing '{'.", filename);
		}
		while (1)
		{
			if (!GetToken (true))
			{
				Error ("parsing '%s': unexpected end of file.", filename);
			}
			if (!strcasecmp (g_token, "}"))
			{
				break;
			}
			if (skip)
			{
				continue;
			}
			Log (" ");
			bool include = false;
			if (!strcasecmp (g_token, "include"))
			{
				Log ("include ");
				include = true;
				if (!GetToken (true))
				{
					Error ("parsing '%s': unexpected end of file.", filename);
				}
			}
			Log ("\"%s\"\n", g_token);
			if (g_iNumWadPaths >= MAX_WADPATHS)
			{
				Error ("parsing '%s': too many wad files.", filename);
			}
			count++;
			PushWadPath (g_token, !include);
		}
	}
	if (found == 0)
	{
		Error ("Couldn't find wad configuration '%s' in file '%s'.\n", configname, filename);
	}
	if (found >= 2)
	{
		Error ("Found more than one wad configuration for '%s' in file '%s'.\n", configname, filename);
	}
	free (buffer); // should not be freed because it is still being used as script buffer
	//Log ("Using custom wadfile configuration: '%s' (with %i wad%s)\n", configname, count, count > 1 ? "s" : "");
}
void LoadWadcfgfile (const char *filename)
{
	Log ("Loading wad configuration file '%s' :\n", filename);
	int count = 0;
	int size;
	char *buffer;
	size = LoadFile (filename, &buffer);
	ParseFromMemory (buffer, size);
	while (GetToken (true))
	{
		Log (" ");
		bool include = false;
		if (!strcasecmp (g_token, "include"))
		{
			Log ("include ");
			include = true;
			if (!GetToken (true))
			{
				Error ("parsing '%s': unexpected end of file.", filename);
			}
		}
		Log ("\"%s\"\n", g_token);
		if (g_iNumWadPaths >= MAX_WADPATHS)
		{
			Error ("parsing '%s': too many wad files.", filename);
		}
		count++;
		PushWadPath (g_token, !include);
	}
	free (buffer); // should not be freed because it is still being used as script buffer
	//Log ("Using custom wadfile configuration: '%s' (with %i wad%s)\n", filename, count, count > 1 ? "s" : "");
}
