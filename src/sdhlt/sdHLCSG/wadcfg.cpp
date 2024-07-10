// AJM: ADDED THIS ENTIRE FILE IN

#include "csg.h"
void LoadWadconfig (const char *filename, const char *configname)
{
    char filenameOnly[_MAX_PATH];
    size_t filenameLength = strlen(filename);
    strncpy(filenameOnly, filename, filenameLength);
    filenameOnly[filenameLength] = '\0'; //Null terminate
	auto pos = std::string(filenameOnly).find_last_of("/\\");

	if (pos != std::string::npos) // Copy everything after the last slash to the beginning of filenameOnly
	{
		std::string temp(filenameOnly + pos + 1);
		strncpy(filenameOnly, temp.c_str(), _MAX_PATH);
		filenameOnly[temp.size()] = '\0';
	}
	Log("Loading wadconfig %s from '%s'\n", configname, filenameOnly);
	Log("--------------------------------------\n");
	int wadconfigsFound = 0;
	int wadPathsFound = 0;
	int filesize;
	char *file;
	filesize = LoadFile(filename, &file); //Load file and store it's size
	ParseFromMemory (file, filesize); //Parse contents from memory

	while (GetToken (true)) //Loop through file
	{
		bool skip = true; //Skip until the -wadconfig configname is found

		if (!strcasecmp (g_token, configname)) //If we find configname line
		{
			skip = false;
			wadconfigsFound++;
		}
		if (!GetToken (true) || strcasecmp(g_token, "{")) //If next line is not an opening bracket
		{
			Error ("Parsing %s (missing '{' opening bracket in '%s' config)\n", filenameOnly, configname);
		}
		while (1) //Loop through content of braces
		{
			if (!GetToken (true))
			{
				Error("Parsing '%s': unexpected EOF in '%s'\n", filenameOnly, configname);
			}
			if (!strcasecmp (g_token, "}")) //If we find closing bracket
			{
				break;
			}
			if (skip)
			{
				continue;
			}
			bool include = false;
			if (!strcasecmp (g_token, "include"))
			{
				Log("[include] ");
				include = true;

				if (!GetToken (true))
				{
					Error ("Parsing '%s': unexpected EOF in '%s'\n", filenameOnly, configname);
				}
			}
			Log ("%s\n", g_token);
			if (g_iNumWadPaths >= MAX_WADPATHS)
			{
				Error ("Parsing '%s': too many wad files (%i/%i) in '%s'\n", filenameOnly, configname, g_iNumWadPaths, MAX_WADPATHS);
			}
			wadPathsFound++;
			PushWadPath (g_token, !include);
		}
	}
	Log("- %d wadpaths found in %s\n", wadPathsFound, configname);
	Log("--------------------------------------\n\n");

	if (wadconfigsFound < 1)
	{
		Error ("Couldn't find wad config %s in '%s'\n", configname, filenameOnly);
	}
	if (wadconfigsFound > 1)
	{
		Error("Found more than one wad config %s in '%s'\n", configname, filenameOnly);
	}
	free (file); // should not be freed because it is still being used as script buffer
	//Log ("Using custom wadfile configuration: '%s' (with %i wad%s)\n", configname, wadPathsFound, wadPathsFound > 1 ? "s" : "");
}
void LoadWadcfgfile (const char *filename)
{
	Log ("Loading %s\n", filename);
	Log ("------------\n");
	int wadPathsCount = 0;
	int wadFileSize;
	char *wadFile;
	wadFileSize = LoadFile (filename, &wadFile);
	ParseFromMemory (wadFile, wadFileSize);
	while (GetToken (true)) //Loop through file
	{
		bool include = false;
		if (!strcasecmp (g_token, "include")) //If line starts with include (or contains?)
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
		wadPathsCount++;
		PushWadPath (g_token, !include);
	}
	free (wadFile); // should not be freed because it is still being used as script buffer
	//Log ("Using custom wadfile configuration: '%s' (with %i wad%s)\n", filename, count, count > 1 ? "s" : "");
}
